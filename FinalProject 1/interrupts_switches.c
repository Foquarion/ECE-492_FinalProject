/*
 * interrupts_switches.c
 *
 *  Created on: Oct 12, 2025
 *      Author: Dallas.Owens
 *
 *  Switch interrupt handlers with debounce using Timer B0 CCR1/CCR2
 */

#include "msp430.h"
#include "ports.h"
#include "switches.h"
#include "timers.h"

// Debounce state variables
volatile unsigned char SW1_debounce = 0;
volatile unsigned char SW2_debounce = 0;
volatile unsigned int SW1_cnt = 0;
volatile unsigned int SW2_cnt = 0;

// Debounce time in 50ms ticks (200ms total)
#define DEBOUNCE_TICKS  (4)


//==============================================================================
// PORT 3 ISR - SW1 (P3.4)
//==============================================================================
#pragma vector = PORT3_VECTOR
__interrupt void PORT3_ISR(void) {
    if (P3IFG & SW1) {
        P3IFG &= ~SW1;                      // Clear interrupt flag
        P3IE &= ~SW1;                       // Disable SW1 interrupt (prevent bounces)
        
        SW1_pressed = 1;                    // Set flag for main loop
        
        // Start debounce timer (TB0 CCR1)
        SW1_debounce = 1;
        SW1_cnt = 0;
        TB0CCTL1 &= ~CCIFG;                 // Clear pending interrupt
        TB0CCR1 = TB0R + TB0CCR1_INTERVAL;  // Schedule first tick
        TB0CCTL1 |= CCIE;                   // Enable debounce timer
    }
}


//==============================================================================
// PORT 2 ISR - SW2 (P2.3)
//==============================================================================
#pragma vector = PORT2_VECTOR
__interrupt void PORT2_ISR(void) {
    if (P2IFG & SW2) {
        P2IFG &= ~SW2;                      // Clear interrupt flag
        P2IE &= ~SW2;                       // Disable SW2 interrupt (prevent bounces)
        
        SW2_pressed = 1;                    // Set flag for main loop
        
        // Start debounce timer (TB0 CCR2)
        SW2_debounce = 1;
        SW2_cnt = 0;
        TB0CCTL2 &= ~CCIFG;                 // Clear pending interrupt
        TB0CCR2 = TB0R + TB0CCR2_INTERVAL;  // Schedule first tick
        TB0CCTL2 |= CCIE;                   // Enable debounce timer
    }
}
