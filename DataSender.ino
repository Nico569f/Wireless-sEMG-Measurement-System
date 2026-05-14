/*
 * DataSender.ino - Main sketch for streaming IMU and EMG data over Bluetooth
 * This sketch initializes the BMI270 IMU and three ADC channels for EMG input, then continuously reads the sensor data 
 * and sends it as CSV lines over Serial1 (Bluetooth). 
 * 
 * The sample rate is controlled to achieve a consistent 1000 Hz output, 
 * and the data is formatted to include a timestamp, EMG values, and IMU readings in each line.
*/

#include "BMI270.h"
#include "I2C_SERCOM3.h"
#include "ctrlADC.h"

// Main sketch: initialize the IMU and ADC channels, then stream CSV samples over Serial1.

// Target sample rate
#define SAMPLE_RATE_HZ   1000
#define SAMPLE_PERIOD_US (1000000 / SAMPLE_RATE_HZ)  // 1000 µs

// Global variables to hold the latest sensor values for printing.
int16_t ax, ay, az;
int16_t gx, gy, gz;
uint16_t emg0, emg1, emg2;

// Setup is called once at startup or reset. Initialize the BMI270 and ADC channels here.
void setup()
{
  Serial1.begin(115200);  // Data output

  // I2C must be initialized before IMU
  I2C_init(400000);

  // IMU init
  if (!BMI270_init())
  {
    while (1);
  }

  // ADC init 
  InitADC0();
  InitADC1();
  InitADC2();

  // setup complete
}
// Main loop: read sensors and print CSV lines at the target sample rate. 
// Sample timing is controlled by a simple micros() check to ensure consistent 
// sample periods regardless of processing time.
void loop()
{
  static uint32_t lastSampleTime = 0; // Track the timestamp of the last sample for timing control.

  // Block until next sample period
  uint32_t now = micros();
  if ((int32_t)(now - lastSampleTime) < SAMPLE_PERIOD_US)
    return;

  lastSampleTime = now; // Timestamp for this sample (in milliseconds since startup)
      
  uint32_t timestamp = millis();

  // Read IMU (single 12-byte burst)
  BMI270_readAll(&ax, &ay, &az, &gx, &gy, &gz);

  // Read EMG (free-running, no wait)
  emg0 = ADC0_ReadData();
  emg1 = ADC1_ReadData();
  emg2 = ADC2_ReadData();

  // Send CSV over Serial1 via BlueSMiRF v2
  // snrintf is used to format the line in a single step to minimize timing jitter between prints.
  char buf[80];
  snprintf(buf, sizeof(buf), "%u,%u,%u,%u,%d,%d,%d,%d,%d,%d\n",
          timestamp, emg0, emg1, emg2, ax, ay, az, gx, gy, gz);

  Serial1.print(buf);
}