/*
  * BMI270.cpp - Bosch BMI270 IMU driver implementation
  *
  * This file implements the initialization and data reading functions for the Bosch BMI270 IMU.
  * It uses the I2C_SERCOM3 library for I2C communication and follows the initialization sequence
  * specified in the official Bosch documentation to ensure proper configuration of the sensor.
  *
  * The BMI270 is configured to output accelerometer data at 100 Hz and gyroscope data at 200 Hz,
  * with both sensors operating in normal mode with filtering enabled. The power mode is set to
  * disable advanced power save and enable FIFO self-wakeup for optimal performance.
  * 
  * The configuration file used for initialization is sourced from the official Bosch library and is
  * uploaded to the sensor via the INIT_DATA register as part of the initialization sequence.
  * 
  * The readAll function performs a single burst read of the accelerometer and gyroscope data registers
  * to efficiently retrieve all sensor data in one I2C transaction.
  * 
*/


#include "BMI270.h"
#include "I2C_SERCOM3.h"

// Runtime BMI270 helper functions that wrap the Bosch init and sample flow.
static uint8_t gBMI270Addr = BMI270_I2C_ADDR;

// BMI270 Configuration file (8 kB)
// Source: Official Bosch bmi270.c - bmi270_config_file array
// The config data is loaded from the official Bosch library (bmi270.c)
extern const uint8_t bmi270_config_file[];

/**
 * @brief Upload BMI270 configuration file via INIT_DATA register
 * @param configData Pointer to 8 kB configuration data
 * @return true if successful, false otherwise
 */
/**
 * @brief Verify BMI270 configuration file
 * @param configData Pointer to 8 kB configuration data to verify against
 * @return true if verification successful, false otherwise
 */
/**
 * @brief Full Power-On-Reset initialization sequence as per Bosch specification
 * @return true if initialization successful, false otherwise
 */

bool BMI270_initSequence()
{
  uint8_t status;
  uint32_t startTime;
  const uint32_t INIT_TIMEOUT_MS = 1500;

  // Step 1: Dummy read
  I2C_read8(gBMI270Addr, REG_CHIPID);
  delayMicroseconds(10);

  // Step 2: Disable advanced power save
  I2C_write8(gBMI270Addr, REG_PWR_CONF, 0x00);

  // Step 3: Wait 450us
  delayMicroseconds(450);

  // Step 4: Prepare config load
  I2C_write8(gBMI270Addr, REG_INIT_CTRL, BMI270_INIT_CTRL_PREPARE);
  delayMicroseconds(10);
  uint8_t initCtrl = I2C_read8(gBMI270Addr, REG_INIT_CTRL);

  // Step 5: Upload config
  unsigned long t0 = millis();
  I2C_writeBytes(gBMI270Addr, REG_INIT_DATA, bmi270_config_file, BMI270_CONFIG_FILE_SIZE);

  // Step 6: Complete config load
  I2C_write8(gBMI270Addr, REG_INIT_CTRL, BMI270_INIT_CTRL_COMPLETE);
  delayMicroseconds(10);
  initCtrl = I2C_read8(gBMI270Addr, REG_INIT_CTRL);

  // Step 7: Poll INTERNAL_STATUS
  startTime = millis();
  while ((millis() - startTime) < INIT_TIMEOUT_MS)
  {
    status = I2C_read8(gBMI270Addr, REG_INTERNAL_STATUS);

    if ((status & 0x0F) == 0x01)
    {
      return true;
    }

    delay(5);
  }

  return false;
}
// Initialize the BMI270 IC
bool BMI270_init()
{
  uint8_t chipId;

  // Check chip ID
  chipId = I2C_read8(0x68, REG_CHIPID);
  if (chipId == 0x24)
    gBMI270Addr = 0x68;
  else
  {
    chipId = I2C_read8(0x69, REG_CHIPID);
    if (chipId == 0x24)
      gBMI270Addr = 0x69;
    else
      return false;
  }

  // Soft reset
  I2C_write8(gBMI270Addr, REG_CMD, 0xB6);
  delay(50); // Wait for POR to complete

  // Perform full Power-On-Reset initialization sequence
  if (!BMI270_initSequence())
  {
    return false;
  }

  // Enable accelerometer + gyroscope
  I2C_write8(gBMI270Addr, REG_PWR_CTRL, 0x0E); // PWR_CTRL: enable accel, gyro, temp sensors

  delay(10);

  // Configure accelerometer (100 Hz, normal mode with filter enabled)
  I2C_write8(gBMI270Addr, REG_ACC_CONFIG, 0xA8); // ACC_CONF: acc_filter_perf=1, acc_bwp=normal, acc_odr=100Hz

  // Configure gyroscope (200 Hz, normal mode with filter enabled)
  I2C_write8(gBMI270Addr, REG_GYR_CONFIG, 0xA9); // GYR_CONF: gyr_filter_perf=1, gyr_bwp=normal, gyr_odr=200Hz

  // Configure power mode (disable advanced power save, enable FIFO self-wakeup)
  I2C_write8(gBMI270Addr, REG_PWR_CONF, 0x02); // PWR_CONF: adv_power_save=0, fifo_self_wakeup=1

  // Set ranges
  I2C_write8(gBMI270Addr, REG_ACC_RANGE, 0x00); // ±2g
  I2C_write8(gBMI270Addr, REG_GYR_RANGE, 0x00); // ±2000 dps

  return true;
}

void BMI270_readAll(int16_t* ax, int16_t* ay, int16_t* az,
                    int16_t* gx, int16_t* gy, int16_t* gz)
{
  uint8_t buf[12];
  I2C_readBytes(gBMI270Addr, REG_acc_x_7_0, buf, 12);
  *ax = (int16_t)((buf[1]  << 8) | buf[0]);
  *ay = (int16_t)((buf[3]  << 8) | buf[2]);
  *az = (int16_t)((buf[5]  << 8) | buf[4]);
  *gx = (int16_t)((buf[7]  << 8) | buf[6]);
  *gy = (int16_t)((buf[9]  << 8) | buf[8]);
  *gz = (int16_t)((buf[11] << 8) | buf[10]);
}

uint8_t BMI270_readInternalStatus()
{
  return I2C_read8(gBMI270Addr, REG_INTERNAL_STATUS);
}
