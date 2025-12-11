/*
 * timers.c
 *
 *  Created on: Oct 2, 2025
 *      Author: Dallas.Owens
 */
#include "macros.h"
#include  "functions.h"
#include  "msp430.h"
#include "timers.h"



void Init_Timers(void){
    Init_Timer_B0();
    Init_Timer_B1();
    Init_Timer_B2();
    Init_Timer_B3();

}

// --------------------- TIMER B0 settings------------------------
void Init_Timer_B0(void){
    TB0CTL = RESET_REGISTER;            // Clear TB0 Control Register
    TB0EX0 = RESET_REGISTER;            // Clear TBIDEX Register

    TB0CTL |= TBSSEL__SMCLK;             // SMCLK SOURCE  // 'B' COULD BE 'A' FOR 'A' TIMERS
    TB0CTL |= MC__CONTINUOUS;            // CONTINUOUS UPCOUNTING MODE  // TB0 - TB8 AVAILABLE
    TB0CTL |= ID__2;                    // DIVIDE CLOCK BY 2
    TB0EX0 |= TBIDEX__8;                 // DIVIDE ADDITIONAL 8
    TB0CTL |= TBCLR;                    // CLEAR COUNTER - RESETS TB0R, CLOCK DIVIDER, DIRECTION

//     CCR0 - (50ms) -----------------
    TB0CCR0 = TB0CCR0_INTERVAL;         // TB0.0 - CCR0
    TB0CCTL0 &= ~CCIFG;                 // Clear possible pending interrupt
    TB0CCTL0 |= CCIE;                   // CCR0 enable interrupt

//     CCR1 - (50ms) -----------------
    TB0CCR1 = TB0CCR1_INTERVAL;         // TB0 - CCR1
    TB0CCTL1 &= ~CCIFG;                 // Clear possible pending interrupt
    TB0CCTL1 &= ~CCIE;

//     CCR2 - (50ms) -----------------
    TB0CCR2 = TB0CCR2_INTERVAL;         // TB0 - CCR2
    TB0CCTL2 &= ~CCIFG;                 // Clear possible pending interrupt
    TB0CCTL2 &= ~CCIE;

//     Overflow
    TB0CTL &= ~TBIE;                    // Disable Overflow Interrupt
    TB0CTL &= ~TBIFG;                   // Clear Overflow Interrupt flag
}


void Init_Timer_B1(void) {
	TB1CTL = RESET_REGISTER;            // Clear TB1 Control Register
	TB1EX0 = RESET_REGISTER;            // Clear TBIDEX Register
	
    TB1CTL |= TBSSEL__SMCLK;             // SMCLK SOURCE  // 'B' COULD BE 'A' FOR 'A' TIMERS
	TB1CTL |= MC__CONTINUOUS;            // UP COUNTING MODE  // TB0 - TB8 AVAILABLE
	TB1CTL |= ID__2;                    // DIVIDE CLOCK BY 2
	TB1EX0 |= TBIDEX__8;                 // DIVIDE ADDITIONAL 8
	TB1CTL |= TBCLR;                    // CLEAR COUNTER - RESETS TB1R, CLOCK DIVIDER, DIRECTION

	//     CCR0 - (5ms) -----------------
	TB1CCR0 = TB1CCR0_INTERVAL;         // TB1 - CCR0
	TB1CCTL0 &= ~CCIFG;                 // Clear possible pending interrupt
	TB1CCTL0 |=  CCIE;                  // CCR0 enable interrupt

	//     CCR01 - (N/A) -----------------
	TB1CCR1 = TB1CCR1_INTERVAL; 	   // TB1 - CCR1
	TB1CCTL1 &= ~CCIFG;                 // Clear possible pending interrupt
	TB1CCTL1 |=  CCIE;                  // CCR1 enable interrupt

	//     CCR02 - (N/A) -----------------
	TB1CCR2 = TB1CCR2_INTERVAL;		 // TB1 - CCR2
	TB1CCTL2 &= ~CCIFG;                 // Clear possible pending interrupt
	TB1CCTL2 &= ~CCIE;                  // CCR2 disable interrupt

	//     Overflow
	TB1CTL &= ~TBIE;                    // Disable Overflow Interrupt
	TB1CTL &= ~TBIFG;                   // Clear Overflow Interrupt flag
}

void Init_Timer_B2(void) {
	// TIMER B2 - NOT USED
	TB2CTL = RESET_REGISTER;            // Clear TB2 Control Register
	TB2EX0 = RESET_REGISTER;            // Clear TBIDEX Register
	
	TB2CTL |= TBSSEL__SMCLK;             // SMCLK SOURCE  // 'B' COULD BE 'A' FOR 'A' TIMERS
	TB2CTL |= MC__CONTINUOUS;            // continuous COUNTING MODE  // TB0 - TB8 AVAILABLE
	TB2CTL |= ID__8;                    // DIVIDE CLOCK BY 8
	TB2EX0 |= TBIDEX__8;                 // DIVIDE ADDITIONAL 8
	TB2CTL |= TBCLR;                    // CLEAR COUNTER - RESETS TB2R, CLOCK DIVIDER, DIRECTION

	//     CCR0 - (N/A) -----------------
	TB2CCR0 = TB2CCR0_INTERVAL;         // TB2 - CCR0
	TB2CCTL0 &= ~CCIFG;                 // Clear possible pending interrupt
	TB2CCTL0 &= ~CCIE;                  // CCR0 disable interrupt

	//     CCR1 - (N/A) -----------------
	TB2CCR1 = TB2CCR1_INTERVAL;         // TB2 - CCR1
	TB2CCTL1 &= ~CCIFG;                 // Clear possible pending interrupt
	TB2CCTL1 |=  CCIE;                  // CCR1 enable interrupt

	//     CCR2 - (N/A) -----------------
	TB2CCR2 = TB2CCR2_INTERVAL;         // TB2 - CCR2
	TB2CCTL2 &= ~CCIFG;                 // Clear possible pending interrupt
	TB2CCTL2 &= ~CCIE;                  // CCR2 enable interrupt

	//     Overflow
	TB2CTL &= ~TBIE;                    // Disable Overflow Interrupt
	TB2CTL &= ~TBIFG;                   // Clear Overflow Interrupt flag
}



