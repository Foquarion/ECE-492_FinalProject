/*
 * ADC.c
 *
 *  Created on: Oct 16, 2025
 *      Author: Dallas.Owens
 */

#include  "msp430.h"
#include  "functions.h"
#include "macros.h"
#include  "ports.h"
#include "timers.h"

extern volatile unsigned long last_adc_isr_time;
extern volatile unsigned long tb0_ccr0_hits;
extern volatile unsigned char adc_case;
extern volatile unsigned char sample_adc;
extern volatile unsigned char ADC_Channel;



 // _______________ ADC USAGE NOTES ___________________
 // Transfer function:
 //    n = FLOOR [ ( (V_in / V_ref) * (2^N) ) + 0.5 ]



char adc_char[4]; // Array to hold the converted ADC value in BCD format


void Sample_ADC(void) {
	sample_adc = TRUE;
    TB1CCTL0 &= ~CCIFG;                     // Clear possible pending interrupt
//    TB1CCR0   = TB1R + ADC_SAMPLE_INTERVAL(2);     // Set CCR1 for debounce interval
    TB1CCTL0 |=  CCIE;                      // TB1_0 enable interrupt
}
void Stop_ADC(void) {
    sample_adc = FALSE;
    TB1CCTL0 &= ~CCIFG;                     // Clear possible pending interrupt
    TB1CCTL0 &= ~CCIE;                      // TB1_0 enable interrupt
}


void Reset_ADC(void){
    if((tb0_ccr0_hits - last_adc_isr_time) > 1000){
        __disable_interrupt();
        ADCCTL0 &= ~ADCENC;
        ADCCTL0 |=  ADCENC;
        sample_adc = 0;
        adc_case = 0;
        ADC_Channel = 0;
        __enable_interrupt();
    }
}


void Init_ADC(void)
{ //------------------------------------------------------------------------------
// V_DETECT_L           (0x04) // Pin 2 A2
// V_DETECT_R           (0x08) // Pin 3 A3
// V_THUMB              (0x20) // Pin 5 A5
//------------------------------------------------------------------------------
    // ADCCTL0 Register
    ADCCTL0  = 0;                       // Reset
    ADCCTL0 |= ADCSHT_2;                // 16 ADC clocks
    ADCCTL0 |= ADCMSC;                  // MSC
    ADCCTL0 |= ADCON;                   // ADC ON

    // ADCCTL1 Register
    ADCCTL1  = 0;                       // Reset
    ADCCTL1 |=  ADCSHS_0;               // 00b = ADCSC bit
    ADCCTL1 |=  ADCSHP;                 // ADC sample-and-hold SAMPCON signal from sampling timer.
    ADCCTL1 &= ~ADCISSH;                // ADC invert signal sample-and-hold.
    ADCCTL1 |=  ADCDIV_0;               // ADC clock divider - 000b = Divide by 1
    ADCCTL1 |=  ADCSSEL_0;              // ADC clock MODCLK
    ADCCTL1 |=  ADCCONSEQ_0;            // ADC conversion sequence 00b = Single-channel single-conversion
                                        // ADCCTL1 & ADCBUSY  identifies a conversion is in process

    // ADCCTL2 Register
    ADCCTL2  = 0;                       // Reset
    ADCCTL2 |=  ADCPDIV0;               // ADC pre-divider 00b = Pre-divide by 1
    ADCCTL2 |=  ADCRES_2;               // ADC resolution 10b = 12 bit (14 clock cycle conversion time)
    ADCCTL2 &= ~ADCDF;                  // ADC data read-back format 0b = Binary unsigned.
    ADCCTL2 &= ~ADCSR;                  // ADC sampling rate 0b = ADC buffer supports up to 200 ksps

    // ADCMCTL0 Register
    ADCMCTL0 |= ADCSREF_0;              // VREF - 000b = {VR+ = AVCC and VR� = AVSS }
    ADCMCTL0 |= ADCINCH_5;              // V_THUMB (0x20) Pin 5 A5

    ADCIE   |= ADCIE0;                  // Enable ADC conv complete interrupt
    ADCCTL0 |= ADCENC;                  // ADC enable conversion
    //ADCCTL0 |= ADCSC;                 // ADC start conversion (only started by timer interrupt after Sample_ADC())
}


//-----------------------------------------------------------------
// Hex to BCD Conversion
// Convert a Hex number to a BCD for display on an LCD or monitor 
//     Stores in GLOBAL array adc_char[4]
//-----------------------------------------------------------------
void HEXtoBCD(int hex_value){
    int value = 0;
    unsigned int i;
	for (i = 0; i < 4; i++) {
        adc_char[i] = '0';
	}
	while (hex_value > 999) {
		hex_value -= 1000;
		value++;
        adc_char[0] = 0x30 + value;
	}
	value = 0;
	while (hex_value > 99) {
		hex_value -= 100;
		value++;
		adc_char[1] = 0x30 + value;
	}
	value = 0;
	while (hex_value > 9) {
		hex_value -= 10;
		value++;
		adc_char[2] = 0x30 + value;
	}
	adc_char[3] = 0x30 + hex_value;
}


//-------------------------------------------------------------
// ADC Line insert
// Take the HEX to BCD value in the array adc_char and place it 
// in the desired location on the desired line of the display. 
//     char line => Specifies the line 1 thru 4
//     char location => Is the location 0 thru 9
//-------------------------------------------------------------
void adc_line(char line, char location) {
    int i;
    unsigned int realLine;
    realLine = line - 1;
    for (i = 0; i < 4; i++) {
        display_line[realLine][i + location] = adc_char[i];
    }
}





