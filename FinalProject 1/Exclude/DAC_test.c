/*
 * DAC_test.c
 *
 *  Created on: Nov 29, 2025
 *      Author: Dallas.Owens
 */


 /*
  * DAC_test.c - Safe DAC testing functions
  */

#include "msp430.h"
#include "macros.h"
#include "functions.h"
#include "ports.h"
#include "DAC.h"
#include "switches.h"
#include  "timers.h"
#include <string.h>


  // Test mode globals
unsigned char dac_test_mode = FALSE;
unsigned int test_dac_value = DAC_BEGIN;

void DAC_Test_Mode(void) {
    /*
     * DAC testing with switches
     * SW1: Decrease DAC value by 100 (increase voltage)
     * SW2: Increase DAC value by 100 (decrease voltage)
     * Display shows current DAC value
     */

    dac_test_mode = TRUE;

    strcpy(display_line[0], "DAC TEST  ");
    strcpy(display_line[1], "SW1: +Volt");
    strcpy(display_line[2], "SW2: -Volt");
    display_changed = TRUE;

    test_dac_value = DAC_BEGIN;  // Start safe at 2V
    DAC_Set_Voltage(test_dac_value);

    while (dac_test_mode) {
        // Handle switch presses
        if (SW1_pressed) {
            SW1_pressed = FALSE;

            // Decrease DAC value = Increase voltage
            if (test_dac_value >= 100) {
                test_dac_value -= 100;
                DAC_Set_Voltage(test_dac_value);
            }
        }

        if (SW2_pressed) {
            SW2_pressed = FALSE;

            // Increase DAC value = Decrease voltage
            if (test_dac_value <= (DAC_SAFE_MAX - 100)) {
                test_dac_value += 100;
                DAC_Set_Voltage(test_dac_value);
            }
        }

        // Update display with current value
        strcpy(display_line[0], "DAC TEST  ");
        strcpy(display_line[1], "Val:      ");

        // Convert DAC value to string
        display_line[1][5] = ((test_dac_value / 1000) % 10) + '0';
        display_line[1][6] = ((test_dac_value / 100) % 10) + '0';
        display_line[1][7] = ((test_dac_value / 10) % 10) + '0';
        display_line[1][8] = (test_dac_value % 10) + '0';

        display_changed = TRUE;
        Display_Update(0, 0, 0, 0);

        // Small delay for display stability
        __delay_cycles(100000);
    }
}


void DAC_Test_Preset(unsigned int dac_value) {
    /*
     * Set DAC to specific value for measurement
     * Use this for systematic testing
     */

     // Safety check
    if (dac_value < 700) {  // Don't go above ~6.5V
        dac_value = 700;
    }
    if (dac_value > DAC_BEGIN) {
        dac_value = DAC_BEGIN;
    }

    DAC_Set_Voltage(dac_value);

    // Display current value
    strcpy(display_line[0], "DAC:      ");
    display_line[0][5] = ((dac_value / 1000) % 10) + '0';
    display_line[0][6] = ((dac_value / 100) % 10) + '0';
    display_line[0][7] = ((dac_value / 10) % 10) + '0';
    display_line[0][8] = (dac_value % 10) + '0';

    strcpy(display_line[1], "Measure V ");
    display_changed = TRUE;
}

