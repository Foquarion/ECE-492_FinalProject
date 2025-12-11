/*
 * interrupts_ADC.c
 *
 *  Created on: Oct 17, 2025
 *      Author: Dallas.Owens
 */


#include "macros.h"
#include  "functions.h"
#include  "msp430.h"
#include "timers.h"
#include "LED.h"
#include "ADC.h"

volatile unsigned int ADC_Thumb;
volatile unsigned int ADC_Left_Detect;
volatile unsigned int ADC_Right_Detect;
volatile unsigned char ADC_Channel;

volatile unsigned char display_thumb;
volatile unsigned char display_left_detect;
volatile unsigned char display_right_detect;

volatile unsigned long adc_isr_hits  = 0;
volatile unsigned long adc_isr_start_time = 0;
volatile unsigned long max_adc_isr_duration = 0;
volatile unsigned long last_adc_isr_time = 0;

extern volatile unsigned long tb0_ccr0_hits;





#pragma vector=ADC_VECTOR
__interrupt void ADC_ISR(void)
{
    adc_isr_hits++;       // debug flag
    adc_isr_start_time = TB0R;
    last_adc_isr_time = tb0_ccr0_hits;

    switch (__even_in_range(ADCIV, ADCIV_ADCIFG))
    {
    case ADCIV_NONE:
        break;

    case ADCIV_ADCOVIFG:   // When a conversion result is written to the ADCMEM0   
        break;             // before its previous conversion result was read.

    case ADCIV_ADCTOVIFG:   // ADC conversion-time overflow
        break;

    case ADCIV_ADCHIIFG:    // Window comparator interrupt flags
        break;

    case ADCIV_ADCLOIFG:    // Window comparator interrupt flag
        break;

    case ADCIV_ADCINIFG:    // Window comparator interrupt flag
        break;

    case ADCIV_ADCIFG:     // ADCMEM0 memory register with the conversion result
        
        if (!sample_adc) { return; }
        
        ADCCTL0 &= ~ADCENC;
        switch (ADC_Channel++)                       // Cycle through ADC Channel case each tick
        {
        case 0: // V_DETECT_LEFT
            ADCMCTL0 &= ~ADCINCH_2;
            ADCMCTL0 = ADCINCH_3;
            ADC_Left_Detect = ADCMEM0;                      // Move result into Global
            ADC_Left_Detect = (ADC_Left_Detect >> 2);       // Divide the result by 4
            display_left_detect = TRUE;
            ADCCTL0 |= ADCSC;                               // Start next conversion
            break;

        case 1: // V_DETECT_RIGHT
            IR_OFF();
            ADCMCTL0 &= ~ADCINCH_3;
            ADCMCTL0 = ADCINCH_5;
            ADC_Right_Detect = ADCMEM0;                     // Move result into Global
            ADC_Right_Detect = (ADC_Right_Detect >> 2);     // Divide the result by 4
		    display_right_detect = TRUE;
            ADCCTL0 |= ADCSC;
            break;

        case 2: // V_THUMB
            ADCMCTL0 &= ~ADCINCH_5;
            ADCMCTL0 = ADCINCH_2;
            ADC_Thumb = ADCMEM0;                            // V_THUMB
            ADC_Thumb = (ADC_Thumb >> 2);                   // Divide the result by 4
            ADC_Channel = 0;
		    display_thumb = TRUE;
            //display_changed = TRUE;
            break;

        default:
            IR_OFF();
            sample_adc = 0;
            ADC_Channel = 0;
            break;
        }
        ADCCTL0 |= ADCENC;
        break;
        
    default:
        break;
    }
    //unsigned long duration = TB0R - adc_isr_start_time;
    //if (duration > max_adc_isr_duration) {
    //    max_adc_isr_duration = duration;
    //}
}
