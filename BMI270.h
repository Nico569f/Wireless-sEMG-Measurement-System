/*
  * BMI270.h - Header file for BMI270 IMU sensor helper functions.
  * This file defines the interface for initializing and reading from the BMI270 IMU sensor,
  * as well as the necessary register addresses and configuration values used by the implementation in BMI270.cpp.
  * The BMI270 is a 6-axis IMU that provides accelerometer and gyroscope data, and this header
  * abstracts the details of communicating with the sensor over I2C using the I2C_SERCOM3 library.
  * 
  * The initialization sequence follows the official Bosch documentation to ensure proper configuration
  * of the sensor, including uploading the required configuration file via the INIT_DATA register.
  * The readAll function allows for efficient retrieval of all sensor data in a single burst read.
  * 
 */

#ifndef BMI270_H
#define BMI270_H

#include "Arduino.h"

#define BMI270_I2C_ADDR 0x68

// Public BMI270 helpers used by the sketch and startup sequence.
bool  BMI270_init();                                             // Initialize the BMI270 IC
bool  BMI270_initSequence();                                     // Full POR initialization sequence
void  BMI270_readAll(int16_t* ax, int16_t* ay, int16_t* az,
                     int16_t* gx, int16_t* gy, int16_t* gz);
uint8_t BMI270_readInternalStatus();                            // Read internal status register (0x21)
float BMI270_accToMps2(int16_t raw);                            // Convert raw acceleration to m/s^2
float BMI270_gyroToDps(int16_t raw);                            // Convert raw gyroscope to deg/s

// Register addresses used by the BMI270 wrapper.
enum
{
  // LSB
  REG_acc_x_7_0      = 0x0C,
  REG_acc_y_7_0      = 0x0E,
  REG_acc_z_7_0      = 0x10,
  // MSB
  REG_acc_x_15_8     = 0x0D,
  REG_acc_y_15_8     = 0x0F,
  REG_acc_z_15_8     = 0x11,
  // LSB
  REG_gyr_x_7_0      = 0x12,
  REG_gyr_y_7_0      = 0x14,
  REG_gyr_z_7_0      = 0x16,
  // MSB
  REG_gyr_x_15_8     = 0x13,
  REG_gyr_y_15_8     = 0x15,
  REG_gyr_z_15_8     = 0x17,

  REG_CHIPID         = 0x00, // Default value 0x24

  REG_NV_CONFIG         = 0x70,

  REG_SATURATION = 0x4A,

  REG_INTERNAL_STATUS = 0x21,

  REG_ERR_REG = 0x02,

  REG_INT_STATUS_1     = 0x1D,
  REG_INIT_CTRL        = 0x59,
  REG_INIT_DATA           = 0x5E,

  REG_ACC_SELF_TEST       = 0x6D,
  REG_GYR_SELF_TEST_AXES       = 0x6E,

  REG_GYR_CONFIG       = 0x42,
  REG_ACC_CONFIG       = 0x40,

  REG_PWR_CONF         = 0x7C,
  REG_PWR_CTRL         = 0x7D,

  REG_ACC_RANGE        = 0x41,
  REG_GYR_RANGE        = 0x43,

  REG_SENSORTIME_0     = 0x18,  // SENSORTIME LSB for timing reference

  BMI2_I2C_PRIM_ADDR   = 0x68,

  REG_CMD = 0x7E // softreset can be initiated at any time by writing the command softreset (0xB6) to register CMD
};

// PWR_CONF register bit definitions
#define BMI270_ADV_POWER_SAVE_BIT   0x01
#define BMI270_ADV_POWER_SAVE_MASK  0x01

// INIT_CTRL register values
#define BMI270_INIT_CTRL_PREPARE    0x00
#define BMI270_INIT_CTRL_COMPLETE   0x01

// Configuration file size (8 kB)
#define BMI270_CONFIG_FILE_SIZE     8192

// Calibration data struct
typedef struct
{
  uint16_t acc_x_7_0;
  uint16_t acc_y_7_0;
  uint16_t acc_z_7_0;
  uint16_t gyr_x_7_0;
  uint16_t gyr_y_7_0;
  uint16_t gyr_z_7_0;

  uint16_t gyr_x_15_8;
  uint16_t gyr_y_15_8;
  uint16_t gyr_z_15_8;
  uint16_t acc_x_15_8;
  uint16_t acc_y_15_8;
  uint16_t acc_z_15_8;


} calib_data;

#endif /* BMI270_H */
