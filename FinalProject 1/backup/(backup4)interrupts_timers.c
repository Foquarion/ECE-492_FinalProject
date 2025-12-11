/*
 * interrupts_timers.c
 *
 *  Created on: Oct 12, 2025
 *      Author: Dallas.Owens
 */

#include "msp430.h"
#include "timers.h"
#include "ports.h"
#include "macros.h"
#include "functions.h"
#include  <string.h>


// Defines
//#define FIFTY_MS           (10)
#define UPDATE_READY        (DELAY_200MS)
#define BLINK_INTERVAL      (DELAY_MS(200))
#define SEQUENCE_RESET      (DELAY_MS(1250))
#define DEBOUNCE_TIME       (DELAY_200MS)


// Globals
volatile unsigned char updateCount;
volatile unsigned int blinkCount;
volatile unsigned char LCD_Blink_Enable;

volatile unsigned int Time_Sequence;
volatile unsigned char update_display;
//volatile unsigned int time_remaining;     // Used in Time-based PWM loops

volatile unsigned int time_change;

volatile unsigned char SW1_debounce,  SW2_debounce;
volatile unsigned int  SW1_cnt,       SW2_cnt;
volatile unsigned char SW1_display,   SW2_display;
volatile unsigned char default_display;

volatile unsigned char brake_initiated;
volatile unsigned int brake_wait;
volatile unsigned char brake_complete;

volatile unsigned char transition_initiated;
volatile unsigned int transition_wait;
volatile unsigned char transition_wait_complete;

volatile unsigned long tb0_ccr0_hits = 0;
volatile unsigned long tb1_ccr0_hits = 0;
volatile unsigned long tb1_ccr1_hits = 0;

volatile unsigned char adc_case = 0;
volatile unsigned char sample_adc = 0;

volatile unsigned char reset_iot = 0;
volatile unsigned char iot_boot = 0;
volatile unsigned char send_message = 0;
volatile unsigned char message_timer = 0;
volatile unsigned char boot_next = 0;

volatile unsigned char init_iot_connection = 0;
extern volatile unsigned char parse_iot = 0;
extern volatile unsigned char parse_iot_ready = 0;

volatile int TB2_CCR0_debug_ticks = 0;

unsigned char iotBootCount = 0;

extern volatile unsigned int command_timer;
extern volatile unsigned char command_complete;



// ================== TIMER B0 - 0 INTERRUPT HANDLERS =================
// --------------------------- 50ms INTERVAL --------------------------

// ================== TIMER B0 - 0 INTERRUPT HANDLERS =================
// --------------------------- 50ms INTERVAL --------------------------
#pragma vector = TIMER_B0_CCR0_VECTOR
__interrupt void TIMER_B0_CCR0_ISR(void){
    TB0CCR0 += TB0CCR0_INTERVAL;         // ADD OFFSET TO TBCCR0
    //tb0_ccr0_hits++;    //testing

    // CONTROL TIME VARIABLE
    time_change = 1;

    // LCD UPDATE DISPLAY (200ms - 4 x 50ms ticks)
    if (++updateCount >= UPDATE_READY){
        updateCount = 0;
        update_display = 1;
    }

    // TIME SEQUENCE Interrupt
    if(Time_Sequence++ > SEQUENCE_RESET) {Time_Sequence = 0; }

}


// ================== TIMER B0 1-2 INTERRUPT HANDLERS =================
// --------------------------- 50ms INTERVALs -------------------------
#pragma vector = TIMER_BO_CCR1_2_0V_VECTOR
__interrupt void TIMER_BO_CCR1_2_0V_ISR(void)
{ // TB0 CCR1 & CCR2 VECTOR
    switch(__even_in_range(TB0IV, 14))
    {
    case 0: break;                                         // NO INTERRUPT

    // ____________________ SW1 DEBOUNCE TIMER - TB0 CCR1 ____________________________________
    case 2:     //SW1 DEBOUNCE HANDLER - 50ms 
        if (SW1_debounce){
            if(++SW1_cnt >= DEBOUNCE_TIME){                // Check if debounce time expired
                SW1_debounce = 0;                          // Reset debounce flag -> debounce done
                SW1_cnt = 0;                               // Reset counter for next debounce
                P4IFG &= ~SW1;                             // Clear interrupt flag
                P4IE |= SW1;                               // Re-enable SW1 interrupt for next press
                TB0CCTL1 &= ~CCIE;                         // Disable CCR1 until next press
                //update_display = 1;
            }
            else{
                TB0CCR1 += TB0CCR1_INTERVAL;
            }
        }
        else{
            TB0CCTL1 &= ~CCIE;                               // Keep normally off (when not used)
        }
        break;

    // ____________________ SW2 DEBOUNCE TIMER - TB0 CCR2 ____________________________________
    case 4:     //SW2 DEBOUNCE HANDLER - 50ms
        if (SW2_debounce) {
            if (++SW2_cnt >= DEBOUNCE_TIME) {
                SW2_debounce = 0;
                SW2_cnt = 0;
                P2IFG &= ~SW2;
                P2IE |= SW2;
                TB0CCTL2 &= ~CCIE;                              // Disable CCR2 until next press
                //update_display = 1;
            }
            else{
                TB0CCR2 += TB0CCR2_INTERVAL;
            }
        }
        else {
            TB0CCTL2 &= ~CCIE;
        }
        break;

    // ____________________ OVERFLOW INTERRUPT ____________________________________
    case 14:                            // OVERFLOW
        // Add OVERFLOW interrupt service here if needed
        // ---------------------------------------------
        break;

	default:
        break;
    }
}

// ================== TIMER B1 INTERRUPT HANDLERS ==================

// ----------------- TIMER B1 - 0 ----------------
// ----------------- 5ms INTERVAL ----------------
#pragma vector = TIMER_B1_CCR0_VECTOR
__interrupt void TIMER_B1_CCR0_ISR(void) {      // TB1 CCR0 - 5ms 
	
    TB1CCR0 += TB1CCR0_INTERVAL;                    // ADD OFFSET TO TBCCR0
    //tb1_ccr0_hits++;
    if(!sample_adc){ return; }

    switch (adc_case++)
    { // Cycle through ADC Channel case each tick
    case 0:
        IR_ON();
        break;

    case 1:
        ADCCTL0 &= ~ADCENC;
        ADCMCTL0 &= ~ADCINCH_3;
        ADCMCTL0 = ADCINCH_5;
        ADCCTL0 |= ADCENC;                          // ensure ENC is set
        ADCCTL0 |= ADCSC;
        break;

    case 2:
        ADCCTL0 &= ~ADCENC;
        ADCMCTL0 &= ~ADCINCH_5;
        ADCMCTL0 = ADCINCH_2;
        ADCCTL0 |= ADCENC;                          // ensure ENC is set
        ADCCTL0 |= ADCSC;
        break;

    case 3:
        ADCCTL0 &= ~ADCENC;
        ADCMCTL0 &= ~ADCINCH_2;
        ADCMCTL0 = ADCINCH_3;
        ADCCTL0 |= ADCENC;                          // ensure ENC is set
        ADCCTL0 |= ADCSC;
        break;

    default:
        IR_OFF();
        sample_adc = 0;
        adc_case = 0;
        break;
    }

	//IR_ON();                                      // Turn IR LED ON every 50ms (Synched to TB0)
//	TB1CCR1 = TB1R + IR_OFFSET_TICKS;	            // Add offset delay to TB1CCR1 before turning ADC on

}

// ----------------- TIMER B1 1-2 ----------------
// ----------------- 100ms INTERVAL --------------
#pragma vector = TIMER_B1_CCR1_2_0V_VECTOR
__interrupt void TIMER_B1_CCR1_2_0V_ISR(void){  // TB1 CCR1 & CCR2 VECTOR
	switch (__even_in_range(TB1IV, 14)) {
	case 0: break;                                  // NO INTERRUPT

	case 2:	    // TB1 CCR1 - 100ms
	    //tb1_ccr1_hits++;
	    TB1CCR1 += TB1CCR1_INTERVAL;
        sample_adc = 1;
        adc_case = 0;
		break;

	case 4:	    // TB1 CCR2 - 100ms
        //tb1_ccr2_hits++;
		TB1CCR2 += TB1CCR2_INTERVAL;                // ADD OFFSET TO B1.2
        if (command_timer > 0) {
            command_timer--;
            if (command_timer <= 0) {
				TB1CCTL2 &= ~CCIE;                  // Disable command timer (B1.2)
				command_complete = TRUE;            // Flag main to stop and reset for next command
            }
        }
		break;

	case 14:    // OVERFLOW
		break;

	default: 
        break;
	}
}


// ================== TIMER B2 INTERRUPT HANDLERS ==================

// ----------------- TIMER B2 - 0 -----------------
// ----------------- 100ms INTERVAL ----------------
#pragma vector = TIMER_B2_CCR0_VECTOR
__interrupt void TIMER_B2_CCR0_ISR(void) {          // TB2 CCR0 - 200ms

    // IoT Boot Sequence Timer
    // Sets delay between sending IoT Init commands on startup
	TB2CCR0 += TB2CCR0_INTERVAL;                    // ADD OFFSET TO TBCCR0
    if(init_iot_connection){
        if(++iotBootCount >= 5){
            iotBootCount = 0;
            boot_next = 1;  // Signal next boot state transition
        }
    }
    TB2_CCR0_debug_ticks++;
    
    // if (++message_timer >= TRANSMIT_PERIOD) {       // Send message over UART to PC every second
    //     message_timer = 0;                          // Reset message timer
    //     send_message = 1;                           // Set Flag here, consume in message send
    //     RED_TOGGLE();
    // }
}

// ----------------- TIMER B2 - 1-2 ---------------------
// ----------------- CCR1: 100ms  CCR2: 200ms -----------
#pragma vector = TIMER_B2_CCR1_2_0V_VECTOR
__interrupt void TIMER_B2_CCR1_2_0V_ISR(void) { // TB2 CCR1 & CCR2 VECTOR
    switch (__even_in_range(TB2IV, 14)) {
    case 0: break;                                  // NO INTERRUPT

    // IOT STARTUP TIMER
    // Sets delay between initial power on and resetting the iot
	case 2:     // TB2 CCR1 - 100ms
	    init_iot_connection = 1;                               // Set flag to start IOT boot process
        TB2CCTL1 &= ~CCIE;                          // CCR1 disable interrupt
        
        TB2CCTL0 &= ~CCIFG;
        TB2CCR0 = TB2R + TB2CCR0_INTERVAL;  // Set the interrupt timer
        TB2CCTL0 |= CCIE;
        break;

	case 4:	    // TB2 CCR2 - 200ms
//		TB2CCR2 += TB2CCR2_INTERVAL;              // ADD OFFSET TO TBCCR2
        if (reset_iot)  {
            GRN_OFF();
            P3OUT |= IOT_EN;        // Release reset (enable IOT)
            reset_iot = 0;
            TB2CCTL2 &= ~CCIE;      // Disable further interrupts
        }
	    break;

    case 14:    // TB2 OVERFLOW
        break;

    default:
        break;
    }
}
