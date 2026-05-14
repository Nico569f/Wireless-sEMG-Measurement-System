/*
 * ctrlADC.h - Header file for ADC initialization and reading functions for SAMD21
 * This file declares the initialization and reading functions for the three ADC channels used for EMG input
 * on the SAMD21 microcontroller. The ADCs are configured for single-ended input with a reference voltage of 3.3V,
 * and the read functions perform a single conversion and return the result. GCLK5 is used as the clock source for the ADCs, 
 * and the necessary pin configurations are set up for the appropriate analog input pins. 
 */

#ifndef CTRL_ADC_H
#define CTRL_ADC_H

#include <Arduino.h>

// ADC clock route used by the shared ADC0 setup.
#define GCLK_ADC0_CORE  40  // Name of the ADC input to use for reference (ADC in differential mode)

// Initialization functions for the three ADC channels used for EMG input
void InitADC0();
void InitADC1();
void InitADC2();  


// Start ADC conversion, wait for the result, and return the result
uint ADC0_ReadData();            // Read a sample
uint ADC1_ReadData();            // Read a sample
uint ADC2_ReadData();            // Read a sample

#endif /* CTRL_ADC_H */
