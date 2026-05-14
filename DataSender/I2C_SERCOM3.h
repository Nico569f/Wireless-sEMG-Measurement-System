/*
 * I2C_SERCOM3.cpp - I2C master implementation for SAMD21 using SERCOM3
 * This file implements the I2C master functions for the SAMD21 microcontroller using the SERCOM3 peripheral.
 * It provides functions to initialize the I2C interface, probe for devices, and read/write bytes to/from I2C slave devices.
 * The implementation uses polling with timeouts to ensure that operations do not hang indefinitely. The I2C bus is configured
 * for fast-mode operation at 400 kHz, and the necessary pin configurations are set up for the appropriate SDA and SCL pins.
 */

#ifndef I2C_SERCOM3_H
#define I2C_SERCOM3_H

#include "Arduino.h"

// Public I2C functions for the sketch and BMI270 driver.
void      I2C_init(uint32_t f_BAUD);
void      I2C_write8(uint8_t addr, uint8_t reg, uint8_t value);
uint8_t   I2C_read8(uint8_t addr, uint8_t reg);
void      I2C_readBytes(uint8_t addr, uint8_t reg, uint8_t* buffer, uint8_t length);
void      I2C_writeBytes(uint8_t addr, uint8_t reg, const uint8_t* buffer, uint16_t length);

// I2C SERCOM3 pin definitions
#define GCLK_SERCOM3_CORE       24

#endif /* I2C_SERCOM3_H */
