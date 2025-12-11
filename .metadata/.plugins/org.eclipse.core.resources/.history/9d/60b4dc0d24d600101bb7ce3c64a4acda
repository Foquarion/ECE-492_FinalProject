/*
 * DAC.c
 *
 *  Created on: Nov 27, 2025
 *      Author: Dallas.Owens
 *  Description: Digital to Analog Converter configuration and control
 *               Controls Buck-Boost converter for variable motor voltage
 */

#include "msp430.h"
#include "macros.h"
#include "functions.h"
#include "ports.h"
#include "DAC.h"
#include  "led.h"
#include  "ports.h"
#include  "timers.h"



// DAC Global Variables
volatile unsigned int DAC_data;
volatile unsigned char dac_voltage_set = FALSE;


// External references
extern volatile unsigned char display_changed;
extern char display_line[4][11];


void Init_DAC(void)
{
    // Configure DAC
    //(using DAC_CTRL_3)
    SAC3DAC = DACSREF_0;                // Select AVCC as DAC reference
    SAC3DAC |= DACLSEL_0;               // DAC latch loads when DACDAT written
    SAC3OA = NMUXEN;                    // SAC Negative input MUX control
    SAC3OA |= PMUXEN;                   // SAC Positive input MUX control
    SAC3OA |= PSEL_1;                   // 12-bit reference DAC source selected
    SAC3OA |= NSEL_1;                   // Select negative pin input
    SAC3OA |= OAPM;                     // Select low speed and low power mode
    SAC3PGA = MSEL_1;                   // Set OA as buffer mode
    SAC3OA |= SACEN;                    // Enable SAC
    SAC3OA |= OAEN;                     // Enable OA

    P3OUT &= ~DAC_CTRL_3;               // Set output to Low
    P3DIR &= ~DAC_CTRL_3;               // Set direction to input
    P3SELC |= DAC_CTRL_3;               // DAC_CNTL DAC operation
    SAC3DAC |= DACEN;                   // Enable DAC

    // Set initial DAC value
    // (2V starting point)
    DAC_data = DAC_BEGIN;               // Starting Low value for DAC output [2V]
    SAC3DAT = DAC_data;                 // Write initial DAC data

    // Enable B0 OVRFLW ramping
    TB0CTL |= TBIE;                     // Timer B0 overflow interrupt enable
    RED_ON();                           // RED LED indicates voltage ramping in progress

    // Enable the DAC
    SAC3DAC |= DACEN;                   // Enable DAC - starts voltage output

    dac_voltage_set = FALSE;            // Flag cleared until target voltage reached
}


//unsigned int DAC_Battery_Adjust(unsigned int V_BAT_current) {
//    unsigned int adjusted_dac = base_dac * (5.5 / V_BAT_current);
//	return adjusted_dac;
//}


void DAC_Increase_Speed(unsigned int target_dac) 
{ // must ramp to higer voltage 
  // (by lowering DAC value)
    unsigned int current = DAC_data;

    while (current > target_dac) {              // CAREFUL!!! LOWER DAC = HIGHER voltage
        current -= DAC_STEP_MEDIUM;
        if (current < target_dac) {
            current = target_dac;
        }
        SAC3DAT = current;
        __delay_cycles(400000);                 // 50ms delay between steps
    }
    DAC_data = current;
}



void DAC_Decrease_Speed(unsigned int target_dac) 
{ // Can decrease voltage in single step
    DAC_data = target_dac;
    SAC3DAT = target_dac;
}


void DAC_Set_Voltage(unsigned int target_value) {
    //==============================================================================
    // Set DAC to a specific value
    // Same as Decrease_Speed but with safety guards
    //   ...Because it is 3am and I have no idea where a fire extinguisher is...
    //==============================================================================
    if (target_value < DAC_SAFE_MAX) {
        target_value = DAC_SAFE_MAX;             // Clamp to maximum safe value
    }
    if (target_value > DAC_MIN) {
        target_value = DAC_MIN;             // Clamp to minimum value
    }

    DAC_data = target_value;
    SAC3DAT = DAC_data;
}


unsigned int DAC_Get_Voltage(void) {
    //==============================================================================
    // Returns current DAC value
    //==============================================================================
    return DAC_data;
}


unsigned char DAC_Ready(void) {
    //==============================================================================
    // Returns TRUE when DAC has reached target voltage
    //==============================================================================
    return dac_voltage_set;
}


void DAC_Ramp_ISR(void) {
    //==============================================================================
    // Called from TB0 overflow ISR (interrupts_timers.c)
    // Gradually decreases DAC value from 2725 (2V) to 875 (6V)
    //
    // Note: Lower DAC values = Higher output voltage (inverse relationship)
    //       This is due to the feedback configuration of the buck-boost converter
    //==============================================================================

    DAC_data = DAC_data - DAC_RAMP_STEP;    // Decrease by ramp step
    SAC3DAT = DAC_data;                     // Update DAC output

    if (DAC_data <= DAC_LIMIT) {            // Check if target reached
        DAC_data = DAC_ADJUST;              // Set final adjustment value
        SAC3DAT = DAC_data;                 // Write final value
        TB0CTL &= ~TBIE;                    // Disable TB0 overflow interrupt
        RED_OFF();                          // Turn off RED LED - voltage set
        dac_voltage_set = TRUE;             // Set completion flag
    }
}




