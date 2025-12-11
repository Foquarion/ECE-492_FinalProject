/*
 * interrupts_timers.c
 *
 *  Created on: Oct 12, 2025
 *      Author: Dallas.Owens
 *
 *  Timer interrupt handlers:
 *    - TB0 CCR1/CCR2: Switch debounce timers
 *    - TB2 CCR2: Marquee LED timing (50ms ticks)
 */

#include "msp430.h"
#include "ports.h"
#include "timers.h"
#include "switches.h"

// Debounce variables (from interrupts_switches.c)
extern volatile unsigned char SW1_debounce;
extern volatile unsigned char SW2_debounce;
extern volatile unsigned int SW1_cnt;
extern volatile unsigned int SW2_cnt;

// Marquee timer (from led.c)
extern volatile unsigned int marquee_timer;

// Debounce time in 50ms ticks
#define DEBOUNCE_TICKS  (4)


//==============================================================================
// TIMER B0 CCR0 ISR - Not used for this project
//==============================================================================
#pragma vector = TIMER_B0_CCR0_VECTOR
__interrupt void TB0_CCR0_ISR(void) {
    TB0CCR0 += TB0CCR0_INTERVAL;
    // Available for other uses
}


//==============================================================================
// TIMER B0 CCR1/CCR2 ISR - Switch Debounce
//==============================================================================
#pragma vector = TIMER_BO_CCR1_2_0V_VECTOR
__interrupt void TB0_CCR1_CCR2_ISR(void) {
    switch (__even_in_range(TB0IV, 14)) {
        case 0:     // No interrupt
            break;
            
        case 2:     // CCR1 - SW1 Debounce
            if (SW1_debounce) {
                SW1_cnt++;
                if (SW1_cnt >= DEBOUNCE_TICKS) {
                    // Debounce complete
                    SW1_debounce = 0;
                    SW1_cnt = 0;
                    P3IFG &= ~SW1;              // Clear any pending interrupt
                    P3IE |= SW1;                // Re-enable SW1 interrupt
                    TB0CCTL1 &= ~CCIE;          // Disable debounce timer
                } else {
                    TB0CCR1 += TB0CCR1_INTERVAL;    // Schedule next tick
                }
            } else {
                TB0CCTL1 &= ~CCIE;              // Should not get here, disable
            }
            break;
            
        case 4:     // CCR2 - SW2 Debounce
            if (SW2_debounce) {
                SW2_cnt++;
                if (SW2_cnt >= DEBOUNCE_TICKS) {
                    // Debounce complete
                    SW2_debounce = 0;
                    SW2_cnt = 0;
                    P2IFG &= ~SW2;              // Clear any pending interrupt
                    P2IE |= SW2;                // Re-enable SW2 interrupt
                    TB0CCTL2 &= ~CCIE;          // Disable debounce timer
                } else {
                    TB0CCR2 += TB0CCR2_INTERVAL;    // Schedule next tick
                }
            } else {
                TB0CCTL2 &= ~CCIE;              // Should not get here, disable
            }
            break;
            
        case 14:    // Overflow
            break;
            
        default:
            break;
    }
}


//==============================================================================
// TIMER B2 CCR0 ISR - Not used for this project
//==============================================================================
#pragma vector = TIMER_B2_CCR0_VECTOR
__interrupt void TB2_CCR0_ISR(void) {
    TB2CCR0 += TB2CCR0_INTERVAL;
    // Available for other uses
}


//==============================================================================
// TIMER B2 CCR1/CCR2 ISR - Marquee Timing
//==============================================================================
#pragma vector = TIMER_B2_CCR1_2_0V_VECTOR
__interrupt void TB2_CCR1_CCR2_ISR(void) {
    switch (__even_in_range(TB2IV, 14)) {
        case 0:     // No interrupt
            break;
            
        case 2:     // CCR1 - Not used
            TB2CCR1 += TB2CCR1_INTERVAL;
            break;
            
        case 4:     // CCR2 - Marquee 50ms tick
            TB2CCR2 += TB2CCR2_INTERVAL;
            marquee_timer++;                // Increment marquee delay counter
            break;
            
        case 14:    // Overflow
            break;
            
        default:
            break;
    }
}
