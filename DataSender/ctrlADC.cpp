/*
 * ctrlADC.cpp - ADC initialization and reading functions for SAMD21
 * This file implements the initialization and reading functions for the three ADC channels used for EMG input
 * on the SAMD21 microcontroller. The ADCs are configured for single-ended input with a reference voltage of 3.3V,
 * and the read functions perform a single conversion and return the result. GCLK5 is used as the clock source for the ADCs, 
 * and the necessary pin configurations are set up for the appropriate analog input pins. 
 * 
*/

#include "ctrlADC.h"


void InitADC0() {

  	////// Clock setup //////

	// Generic Clock Generato5 with DFLL48M as source, and divide it to 100kHz
	GCLK->GENCTRL[5].reg = 0x6 | 
						0x1 << 8 | 
						0x1E0  << 16;
	while (GCLK->SYNCBUSY.bit.GENCTRL5); // Wait for synchronization

    // Setup Peripheral Channel Control for the ADC module with GCLK5 as source table 14.9
	GCLK->PCHCTRL[GCLK_ADC0_CORE].reg = 0x5 | 1<<6;
	
	// Turn on the synchronous clock for ADC0
	MCLK->APBDMASK.bit.ADC0_ = 1;

  	////// Port configuration setup //////
	
	// Configure the pin for peripheral multiplexing (PA02)
	PORT->Group[PORTA].PINCFG[2].reg = PORT_PINCFG_PMUXEN;

    // The port labelled A0 can be used with ADC0
	// - PMUXE[n]  even-numbered pins, PORTnumber = 2*n
	PORT->Group[PORTA].PMUX[1].bit.PMUXE = 0x1;

    ////// ADC setup //////

	// Software Reset (Reset all registers)
	ADC0->CTRLA.bit.SWRST = 0x1;
	while (ADC0->SYNCBUSY.bit.SWRST) ;  // Wait for clock domain sync
	
	// Select VDDANA as reference. VDDANA corresponds to a 3.3V reference.
	ADC0->REFCTRL.bit.REFSEL = 0x3;
	while (ADC0->SYNCBUSY.bit.REFCTRL) ;  // Wait for clock domain sync
	
	// Set ADC resolution to 12-bits
	ADC0->CTRLB.bit.RESSEL = 0x0;
	while (ADC0->SYNCBUSY.bit.CTRLB) ;  // Wait for clock domain sync
	
	// Set clock divider which determine the speed of conversion section 45.8.1
	ADC0->CTRLA.bit.PRESCALER = 1;

	// Set the negative (reference) input MUX register to Ground.
	ADC0->INPUTCTRL.bit.MUXNEG = 0x18;
	while (ADC0->SYNCBUSY.bit.INPUTCTRL) ;  // Wait for clock domain sync

	ADC0->CTRLA.bit.ENABLE = 0x1;        // Enable the ADC
	while (ADC0->SYNCBUSY.bit.CTRLB);	// Wait for clock domain sync

	ADC0->INTENSET.bit.RESRDY = 0x1;     // Activate result ready interrupt

	ADC0->INTFLAG.bit.RESRDY = 0x1;      // Clear results ready interrupt flag
}

void InitADC1() {
	////// Port configuration setup //////
	// Configure the pin for peripheral multiplexing for channel AIN2 (A1)
	PORT->Group[PORTB].PINCFG[8].reg = PORT_PINCFG_PMUXEN;

	// The port labelled A1 can be used with ADC0
	// - PMUXE[n]  even-numbered pins, PORTnumber = 2*n
	PORT->Group[PORTB].PMUX[4].bit.PMUXE = 0x1;
}

void InitADC2() {
	////// Port configuration setup //////

	// Configure the pin for peripheral multiplexing for channel AIN3 (A2)
	PORT->Group[PORTB].PINCFG[9].reg = PORT_PINCFG_PMUXEN;

	// The port labelled A2 can be used with ADC0
	// - PMUXO[n]  odd-numbered pins, PORTnumber = 2*n + 1
	PORT->Group[PORTB].PMUX[4].bit.PMUXO = 0x1;
}

uint ADC0_ReadData()
{
	// Wait while the  ADC is busy
	while (ADC0->STATUS.bit.ADCBUSY) {}

		ADC0->INPUTCTRL.bit.MUXPOS = ADC_INPUTCTRL_MUXPOS_AIN0;
	
		// Start the ADC reading, byt changing the SWTRIG register
	ADC0->SWTRIG.bit.START = 0x1 ;
	while (ADC0->SYNCBUSY.bit.SWTRIG) ;  // Wait for clock domain sysch

	// Wait for conversion to finish
	while (!ADC0->INTFLAG.bit.RESRDY) {}

	// Return the result
	return(ADC0->RESULT.reg);
}

uint ADC1_ReadData()
{
	// Wait while the  ADC is busy
		while (ADC0->STATUS.bit.ADCBUSY) {}

			ADC0->INPUTCTRL.bit.MUXPOS = ADC_INPUTCTRL_MUXPOS_AIN2;


		// Start the ADC reading, byt changing the SWTRIG register
		ADC0->SWTRIG.bit.START = 0x1 ;
		while (ADC0->SYNCBUSY.bit.SWTRIG) ;  // Wait for clock domain sysch

		// Wait for conversion to finish
		while (!ADC0->INTFLAG.bit.RESRDY) {}

		// Return the result
		return(ADC0->RESULT.reg);
}
uint ADC2_ReadData()
{
	// Wait while the  ADC is busy
		while (ADC0->STATUS.bit.ADCBUSY) {}

			ADC0->INPUTCTRL.bit.MUXPOS = ADC_INPUTCTRL_MUXPOS_AIN3;


		// Start the ADC reading, byt changing the SWTRIG register
		ADC0->SWTRIG.bit.START = 0x1 ;
		while (ADC0->SYNCBUSY.bit.SWTRIG) ;  // Wait for clock domain sysch

		// Wait for conversion to finish
		while (!ADC0->INTFLAG.bit.RESRDY) {}

		// Return the result
		return(ADC0->RESULT.reg);
}
