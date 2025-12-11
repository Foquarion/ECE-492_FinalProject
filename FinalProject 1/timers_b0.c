/*
 * timers_b0.c
 *
 *  Created on: Oct 2, 2025
 *      Author: Dallas.Owens
 *
 *  Timer initialization for LED Marquee project:
 *    - TB0: Switch debounce (CCR1 for SW1, CCR2 for SW2)
 *    - TB2: Marquee timing (CCR2 for 50ms ticks)
 */

#include "msp430.h"
#include "timers.h"

#define RESET_REGISTER  (0x0000)
void Init_Timer_B0(void);
void Init_Timer_B1(void);
void Init_Timer_B2(void);
void Init_Timer_B3(void);


void Init_Timers(void) {
    Init_Timer_B0();
    Init_Timer_B1();
    Init_Timer_B2();
    Init_Timer_B3();
}


//==============================================================================
// TIMER B0 - Switch Debounce Timer
// SMCLK (8MHz) / 2 / 8 = 500kHz
// CCR1: SW1 debounce (50ms intervals)
// CCR2: SW2 debounce (50ms intervals)
//==============================================================================
void Init_Timer_B0(void) {
    TB0CTL = RESET_REGISTER;
    TB0EX0 = RESET_REGISTER;

    TB0CTL |= TBSSEL__SMCLK;        // SMCLK source (8MHz)
    TB0CTL |= MC__CONTINUOUS;       // Continuous mode
    TB0CTL |= ID__2;                // Divide by 2
    TB0EX0 |= TBIDEX__8;            // Divide by 8 (total /16 = 500kHz)
    TB0CTL |= TBCLR;                // Clear counter

    // CCR0 - Not used, but enabled for potential future use
    TB0CCR0 = TB0CCR0_INTERVAL;
    TB0CCTL0 &= ~CCIFG;
    TB0CCTL0 &= ~CCIE;              // Disabled

    // CCR1 - SW1 Debounce (starts disabled, enabled by switch ISR)
    TB0CCR1 = TB0CCR1_INTERVAL;
    TB0CCTL1 &= ~CCIFG;
    TB0CCTL1 &= ~CCIE;              // Disabled until switch pressed

    // CCR2 - SW2 Debounce (starts disabled, enabled by switch ISR)
    TB0CCR2 = TB0CCR2_INTERVAL;
    TB0CCTL2 &= ~CCIFG;
    TB0CCTL2 &= ~CCIE;              // Disabled until switch pressed

    // Overflow - not used
    TB0CTL &= ~TBIE;
    TB0CTL &= ~TBIFG;
}


//==============================================================================
// TIMER B1 - Not used in this project
//==============================================================================
void Init_Timer_B1(void) {
    TB1CTL = RESET_REGISTER;
    TB1EX0 = RESET_REGISTER;

    TB1CTL |= TBSSEL__SMCLK;
    TB1CTL |= MC__CONTINUOUS;
    TB1CTL |= ID__2;
    TB1EX0 |= TBIDEX__8;
    TB1CTL |= TBCLR;

    // All CCRs disabled
    TB1CCTL0 &= ~CCIE;
    TB1CCTL1 &= ~CCIE;
    TB1CCTL2 &= ~CCIE;
    TB1CTL &= ~TBIE;
}


//==============================================================================
// TIMER B2 - Marquee Timing
// SMCLK (8MHz) / 8 / 8 = 125kHz
// CCR2: 50ms ticks for marquee LED sequencing
//==============================================================================
void Init_Timer_B2(void) {
    TB2CTL = RESET_REGISTER;
    TB2EX0 = RESET_REGISTER;

    TB2CTL |= TBSSEL__SMCLK;        // SMCLK source (8MHz)
    TB2CTL |= MC__CONTINUOUS;       // Continuous mode
    TB2CTL |= ID__8;                // Divide by 8
    TB2EX0 |= TBIDEX__8;            // Divide by 8 (total /64 = 125kHz)
    TB2CTL |= TBCLR;                // Clear counter

    // CCR0 - Not used
    TB2CCR0 = TB2CCR0_INTERVAL;
    TB2CCTL0 &= ~CCIFG;
    TB2CCTL0 &= ~CCIE;

    // CCR1 - Not used
    TB2CCR1 = TB2CCR1_INTERVAL;
    TB2CCTL1 &= ~CCIFG;
    TB2CCTL1 &= ~CCIE;

    // CCR2 - Marquee timing (starts disabled, enabled by Marquee_Start)
    TB2CCR2 = TB2CCR2_INTERVAL;
    TB2CCTL2 &= ~CCIFG;
    TB2CCTL2 &= ~CCIE;              // Disabled until marquee starts

    // Overflow - not used
    TB2CTL &= ~TBIE;
    TB2CTL &= ~TBIFG;
}


//==============================================================================
// TIMER B3 - Not used in this project (available for PWM)
//==============================================================================
void Init_Timer_B3(void) {
    TB3CTL = RESET_REGISTER;
    TB3EX0 = RESET_REGISTER;
    
    // Leave disabled
    TB3CTL &= ~MC__CONTINUOUS;
}
