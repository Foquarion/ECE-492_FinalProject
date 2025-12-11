/*
 * switches.c
 *
 *  Created on: Sep 21, 2025
 *      Author: Dallas.Owens
 *
 *  Simple switch handling - processes switch press flags set by ISR
 */

#include "msp430.h"
#include "switches.h"
#include "LED.h"

// Switch press flags (set by ISR, consumed here)
volatile unsigned int SW1_pressed = 0;
volatile unsigned int SW2_pressed = 0;


void Switches_Process(void) {
    if (SW1_pressed) {
        SW1_pressed = 0;            // Consume the flag
        Handle_SW1_Press();         // Handle in led.c
    }
    
    if (SW2_pressed) {
        SW2_pressed = 0;            // Consume the flag
        Handle_SW2_Press();         // Handle in led.c
    }
}
