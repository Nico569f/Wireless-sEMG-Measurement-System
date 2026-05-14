/*
 * I2C_SERCOM3.cpp - I2C master implementation for SAMD21 using SERCOM3
 * This file implements the I2C master functions for the SAMD21 microcontroller using the SERCOM3 peripheral.
 * It provides functions to initialize the I2C interface, probe for devices, and read/write bytes to/from I2C slave devices.
 * The implementation uses polling with timeouts to ensure that operations do not hang indefinitely. The I2C bus is configured
 * for fast-mode operation at 400 kHz, and the necessary pin configurations are set up for the appropriate SDA and SCL pins.
 *
*/

#include "I2C_SERCOM3.h"

void I2C_init(uint32_t f_BAUD)
{
  uint32_t timeout;

  // Setup Generic Clock Generator 4 to source from DFLL (48MHz clock) and divide it to 8MHz for SERCOM3
  GCLK->GENCTRL[4].reg = 0x6 |
                         0x1 << 8 |
                         0x6 << 16;
  
  // Wait for synchronization                       
  timeout = 10000;
  while (GCLK->SYNCBUSY.bit.GENCTRL4);

  // Route the generic clock to SERCOM3 before enabling the peripheral.
  GCLK->PCHCTRL[GCLK_SERCOM3_CORE].reg = 0x4 |
                                         1 << 6;

  // Turn on the synchronous clock for the SERCOM3 module
  MCLK->APBBMASK.bit.SERCOM3_ = 1;

  // Enable peripheral multiplexing for SERCOM3 on the appropriate pins
  // SDA on PA22 SERCOM3/PAD[0]
  // SCL on PA23 SERCOM3/PAD[1]
  PORT->Group[PORTA].PINCFG[22].bit.PMUXEN = 0x1; 
  PORT->Group[PORTA].PINCFG[23].bit.PMUXEN = 0x1; 

  // Setup the peripheral multiplexing to Function C
  // - PMUXE[n] even-numbered pins, PORTnumber = 2*n
  // - PMUXO[n] odd-numbered pins, PORTnumber = 2*n+1
  PORT->Group[PORTA].PMUX[11].bit.PMUXE = 0x2;  
  PORT->Group[PORTA].PMUX[11].bit.PMUXO = 0x2;

  // Software Reset SERCOM3 and wait for reset to complete
  SERCOM3->I2CM.CTRLA.bit.SWRST = 0x1;
  while (SERCOM3->I2CM.CTRLA.bit.SWRST);
  while (SERCOM3->I2CM.SYNCBUSY.bit.SWRST);

  // Configure SERCOM3 for I2C master operation section 36.8.1
  SERCOM3->I2CM.CTRLA.bit.MODE = 0x5; // I2C master operation
  SERCOM3->I2CM.CTRLA.bit.SPEED = 0x0; // Fast-mode up to 400 kHz
  SERCOM3->I2CM.CTRLA.bit.SDAHOLD = 0x2; // 300-600ns hold time

  // Table 33-2 gives the formula for calculating the BAUD value for I2C fast-mode (Fm) operation:
  // BAUD = f_ref / (2 * f_BAUD) - 1, where f_ref is the frequency of the reference clock (8MHz in this case)
  SERCOM3->I2CM.BAUD.reg = 9;

  // Enable the I2C master and wait for synchronization
  SERCOM3->I2CM.CTRLA.bit.ENABLE = 0x1;
  timeout = 10000;
  while (SERCOM3->I2CM.SYNCBUSY.bit.ENABLE);

  // Set bus state to idle
  SERCOM3->I2CM.STATUS.bit.BUSSTATE = 1;
  timeout = 10000;
  while (SERCOM3->I2CM.SYNCBUSY.bit.SYSOP);
}

// I2C_sendStop sends a stop condition on the I2C bus to end a transaction.
static void I2C_sendStop()
{
  SERCOM3->I2CM.CTRLB.bit.CMD = 0x3;
  uint32_t timeout = 10000; 
  while (SERCOM3->I2CM.SYNCBUSY.bit.SYSOP && --timeout);
}

// I2C_startWrite initiates an I2C write operation by sending the slave address with the write bit set and checking for acknowledgment.
static bool I2C_startWrite(uint8_t addr7)
{
  uint32_t timeout = 10000; 
  SERCOM3->I2CM.ADDR.reg = (addr7 << 1) | 0x0;
  while (SERCOM3->I2CM.INTFLAG.bit.MB == 0x0 && --timeout);
  if (timeout == 0)
    return false;
  return SERCOM3->I2CM.STATUS.bit.RXNACK == 0x0;
}

// I2C_startRead initiates an I2C read operation by sending the slave address with the read bit set and checking for acknowledgment.
static bool I2C_startRead(uint8_t addr7)
{
  uint32_t timeout = 10000; 
  SERCOM3->I2CM.ADDR.reg = (addr7 << 1) | 0x1;
  while (SERCOM3->I2CM.INTFLAG.bit.SB == 0x0 && --timeout);
  if (timeout == 0)
    return false;
  return SERCOM3->I2CM.STATUS.bit.RXNACK == 0x0;
}

// I2C_writeByte writes a byte to the I2C bus and checks for acknowledgment from the slave device.
static bool I2C_writeByte(uint8_t value)
{
  uint32_t timeout = 10000; 
  SERCOM3->I2CM.DATA.reg = value;
  while (SERCOM3->I2CM.INTFLAG.bit.MB == 0x0 && --timeout);
  if (timeout == 0)
    return false;
  return SERCOM3->I2CM.STATUS.bit.RXNACK == 0x0;
}

// I2C_readByte reads a byte from the I2C bus and sends an ACK or NACK depending on whether it is the last byte to read.
static bool I2C_readByte(uint8_t* rxValue, bool isLastByte)
{
  uint32_t timeout = 10000;

  while (SERCOM3->I2CM.INTFLAG.bit.SB == 0x0 && --timeout);
  if (timeout == 0)
    return false;

  *rxValue = (uint8_t)SERCOM3->I2CM.DATA.reg;

  SERCOM3->I2CM.CTRLB.bit.ACKACT = isLastByte ? 0x1 : 0x0;
  SERCOM3->I2CM.CTRLB.bit.CMD = isLastByte ? 0x3 : 0x2;

  timeout = 10000;
  while (SERCOM3->I2CM.SYNCBUSY.bit.SYSOP && --timeout);
  if (timeout == 0)
    return false;

  return true;
}

// Write a single byte to an I2C slave device at a specific register address
void I2C_write8(uint8_t addr, uint8_t reg, uint8_t value)
{
  if (!I2C_startWrite(addr))
  {
    I2C_sendStop();
    return;
  }

  if (!I2C_writeByte(reg))
  {
    I2C_sendStop();
    return;
  }

  if (!I2C_writeByte(value))
  {
    I2C_sendStop();
    return;
  }

  I2C_sendStop();
}

// Read a single byte from an I2C slave device at a specific register address
uint8_t I2C_read8(uint8_t addr, uint8_t reg)
{
  uint8_t readValue = 0;

  if (!I2C_startWrite(addr))
  {
    I2C_sendStop();
    return readValue;
  }

  if (!I2C_writeByte(reg))
  {
    I2C_sendStop();
    return readValue;
  }

  if (!I2C_startRead(addr))
  {
    I2C_sendStop();
    return readValue;
  }

  if (!I2C_readByte(&readValue, true))
  {
    I2C_sendStop();
    return 0;
  }

  return readValue;
}

// Read multiple bytes from an I2C slave device starting from a specific register address
void I2C_readBytes(uint8_t addr, uint8_t reg, uint8_t* buffer, uint8_t length)
{
  if (buffer == nullptr || length == 0)
    return;

  if (!I2C_startWrite(addr))
  {
    I2C_sendStop();
    return;
  }

  if (!I2C_writeByte(reg))
  {
    I2C_sendStop();
    return;
  }

  if (!I2C_startRead(addr))
  {
    I2C_sendStop();
    return;
  }

  for (uint8_t i = 0; i < length; i++)
  {
    bool isLastByte = (i == (uint8_t)(length - 1));
    if (!I2C_readByte(&buffer[i], isLastByte))
    {
      I2C_sendStop();
      return;
    }
  }
}

// Write multiple bytes to an I2C slave device starting from a specific register address  
void I2C_writeBytes(uint8_t addr, uint8_t reg, const uint8_t* buffer, uint16_t length)
{
  if (buffer == nullptr || length == 0)
    return;

  if (!I2C_startWrite(addr))
  {
    I2C_sendStop();
    return;
  }

  if (!I2C_writeByte(reg))
  {
    I2C_sendStop();
    return;
  }

  for (uint16_t i = 0; i < length; i++)
  {
    if (!I2C_writeByte(buffer[i]))
    {
      I2C_sendStop();
      return;
    }
  }

  I2C_sendStop();
}