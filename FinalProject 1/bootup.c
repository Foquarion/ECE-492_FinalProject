/*
 * bootup.c
 *
 *  Created on: Nov 27, 2025
 *      Author: Dallas.Owens
 * 
 *  Description: Clean, unified boot sequence for LCD -> IOT -> DAC
 *               Single timer (TB2 CCR0) drives entire boot process
 */

#include "msp430.h"
#include "macros.h"
#include "functions.h"
#include "ports.h"
#include "bootup.h"
//#include "DAC.h"
#include  <string.h>
#include "timers.h"
#include "led.h"
#include  "UART.h"
#include "switches.h"

// Boot Sequence State Variables
volatile unsigned char power_sequence = BOOT_INIT;          // Current boot stage
volatile unsigned char boot_timer_flag = FALSE;     // Timer flag from TB2 CCR0 (200ms)
unsigned char boot_complete = FALSE;                // Boot completion flag

// IoT Boot State Machine
iot_boot_state_t iot_boot_state = IOT_WAIT_READY;            // IoT boot state (0-5)
unsigned char iot_boot_complete = FALSE;    // Flag when current IoT state done
volatile unsigned char iot_boot_next = FALSE;
volatile unsigned char init_iot_connection = FALSE;

// Globals _____________________
extern char display_line[4][11];
extern volatile unsigned char display_changed;
extern volatile unsigned char ready;
extern volatile unsigned char ok_found;
extern char SET_MUX_COMMAND[];
extern char SET_PORT_COMMAND[];
extern char REQUEST_SSID_COMMAND[];
extern char REQUEST_IP_COMMAND[];
extern unsigned char display_menu;


//==============================================================================
// Boot_Sequence() - Main boot state machine
// Called repeatedly from main loop until boot_complete
// Uses single timer (TB2 CCR0 @ 200ms) for all timing
//==============================================================================
void Bootup_Sequence(void)
{
    switch (power_sequence)
    {
        // ========== BOOT INITIALIZATION ==========
    case BOOT_INIT:
        // Start unified boot timer
        TB2CCR0 = TB2R + TB2CCR0_INTERVAL;      // Set first interrupt
        TB2CCTL0 |= CCIE;                       // Enable TB2 CCR0 interrupt

        power_sequence = BOOT_LCD;
        break;

        // ========== LCD POWER-UP ==========
    case BOOT_LCD:
         Init_LCD();                             // Powers up LCD, backlight on
         strcpy(display_line[0], " BOOTING..");
         strcpy(display_line[1], "LCD Init..");
         strcpy(display_line[2], "          ");
         strcpy(display_line[3], "          ");

        boot_timer_flag = FALSE;
        power_sequence = BOOT_LCD_WAIT;
        break;

    case BOOT_LCD_WAIT:
        if (boot_timer_flag) {                  // Wait 200ms for stabilization
            boot_timer_flag = FALSE;
            strcpy(display_line[1], "LCD Ready ");
            display_changed = TRUE;
            power_sequence = BOOT_IOT_INIT;
        }
        break;

        // ========== IOT POWER-UP & INITIALIZATION ==========
    case BOOT_IOT_INIT:
        strcpy(display_line[2], "IOT Init..");
        display_changed = TRUE;

        // Initialize UART and prepare IoT module
        Init_SerialComms();
        Enable_Comms();

        iot_boot_complete = FALSE;
        boot_timer_flag = FALSE;
        
        IOT_Reset();
        power_sequence = BOOT_IOT_SEQUENCE;
        iot_boot_state = IOT_ENABLE;
        break;


        // ========== IOT AT COMMAND SEQUENCE ==========
    case BOOT_IOT_SEQUENCE:
        IOT_Connect();                                    // Execute one step of IoT boot     // broken into cases below for debugging when failing here (time system kept failing, now checking for response before transition)
        
        if (iot_boot_complete) {          // Check if IoT boot complete
            strcpy(display_line[2], "IOT Ready ");
            display_changed = TRUE;
            power_sequence = BOOT_DAC_ENABLE;
        }
        break;

        // ========== DAC POWER-UP ==========
    case BOOT_DAC_ENABLE:
        strcpy(display_line[3], "DAC Init..");
        display_changed = TRUE;

        P2OUT |= DAC_ENB;                       // Enable DAC 

        boot_timer_flag = FALSE;
        power_sequence = BOOT_DAC_INIT;
        break;

    case BOOT_DAC_INIT:
        if (boot_timer_flag) {                  // Wait 200ms for stabilization
            boot_timer_flag = FALSE;
            Init_DAC();                         // Initialize DAC, start voltage ramp
            power_sequence = BOOT_DAC_WAIT;
        }
        break;

    case BOOT_DAC_WAIT:
        if (DAC_Ready()) {                      // Wait for ramp
            strcpy(display_line[3], "DAC Ready ");
            display_changed = TRUE;
            power_sequence = BOOT_COMPLETE;
        }
        break;

        // ========== BOOT COMPLETE ==========
    case BOOT_COMPLETE:
        TB2CCTL0 &= ~CCIE;                      // Disable boot timer

        strcpy(display_line[0], "DAS BOOTED");
        display_changed = TRUE;

        boot_complete = TRUE;                   // Fiinish bootup cycle, pass control to main loop
        display_menu = TRUE;
        break;

    default:
        power_sequence = BOOT_INIT;             // Error: restart boot
        break;
    }
}


//========================= IOT BOOTUP SUB-ROUTINE =============================
// Called every 200ms from BOOT_IOT_SEQUENCE state in Master Bootup_Sequence
//      - Commands still gated by response flags (ready, ok_found)
//      - Wait for response -> Send next command
//==============================================================================
void IOT_Connect(void) {
    if(SW1_pressed){
        power_sequence = BOOT_DAC_ENABLE;
        return;
    }
    if (!iot_boot_next) return;        // Wait for 200ms timer flag
    iot_boot_next = FALSE;            // Consume flag
    GRN_TOGGLE();

    switch (iot_boot_state) 
    {
        case IOT_ENABLE:
            if (!init_iot_connection) break;    // Only run if init IOT requested
            P3OUT |= IOT_EN;                    // Release IoT from reset
            iot_boot_state = IOT_WAIT_READY;
            break;

        case IOT_WAIT_READY:                        // State 0: Wait for "ready"
            if (ready) {
                ready = FALSE;
                iot_boot_state = IOT_MUX_SET;
            }
            break;
            
        case IOT_MUX_SET:                           // State 1: Enable multiple connections
            Send_AT_Command(SET_MUX_COMMAND);
            iot_boot_state = IOT_MUX_OK;
            break;
            
        case IOT_MUX_OK:                       // State 2: Wait for OK
            if (ok_found) {
                ok_found = FALSE;
                iot_boot_state = IOT_SERVER_SET;
            }
            break;
            
        case IOT_SERVER_SET:                        // State 3: Set server mode
            Send_AT_Command(SET_PORT_COMMAND);
            iot_boot_state = IOT_SERVER_OK;
            break;
            
        case IOT_SERVER_OK:                    // State 4: Wait for OK
            if (ok_found) {
                ok_found = FALSE;
                iot_boot_state = IOT_SSID_REQUEST;
            }
            break;
            
        case IOT_SSID_REQUEST:                      // State 5: Request SSID
            Send_AT_Command(REQUEST_SSID_COMMAND);
            iot_boot_state = IOT_SSID_WAIT;
            break;
            
        case IOT_SSID_WAIT:                      // State 6: Wait for OK
            if (ok_found) {
                ok_found = FALSE;
                iot_boot_state = IOT_IP_REQUEST;
            }
            break;
            
        case IOT_IP_REQUEST:                        // State 7: Request IP address
            Send_AT_Command(REQUEST_IP_COMMAND);
            iot_boot_state = IOT_IP_WAIT;
            break;
            
        case IOT_IP_WAIT:                        // State 8: Wait for OK
            if (ok_found) {
                ok_found = FALSE;
                iot_boot_state = IOT_BOOT_COMPLETE;
            }
            break;
            
        case IOT_BOOT_COMPLETE:                     // State 9: Done
            iot_boot_state = IOT_ENABLE;                     // Reset state machine
            init_iot_connection = FALSE;                // Clear the boot initiation flag
            TB2CCTL1 &= ~CCIE;                      // Disable TB2 CCR0 interrupt (no longer need 100ms ticks)
            iot_boot_complete = TRUE;
            break;            
            
        default:
            iot_boot_state = IOT_ENABLE;        // Recovery
            break;
    }
}


void IOT_Reset(void){
    if(!init_iot_connection){        // Only reset if in init IOT requested
        P3OUT &= ~IOT_EN;
    
        TB2CCR1 = TB2R + TB2CCR1_INTERVAL;      // Set IOT boot timer
        TB2CCTL1 &= ~CCIFG;                     // Clear flag
        TB2CCTL1 |= CCIE;                       // Enable Reset timer in TB2-CCR1
    
        init_iot_connection = TRUE;
        iot_boot_state = IOT_ENABLE;
    }
    // else{ CALL CONNECT_IOT FROM HERE FOR SINGLE RESET FUNCTION? }
    // ADD IF NOT IOT BOOT COMPLETE FLAG CHECK FOR SINGLE RUN EXIT ROUTE
}


// Hysterical Porpoises
//    switch (power_sequence) 
    //    {
    //    case BOOT_INIT:                                     // Initialize boot sequence
    //        strcpy(display_line[0], " BOOTING..");          // This would be more helpful if LCD already booted.....
    //        strcpy(display_line[1], "          ");
    //        strcpy(display_line[2], "          ");
    //        strcpy(display_line[3], "          ");
    //                                                        // Start boot timer (TB2 CCR0 @ 200ms intervals)
    //        TB2CCR0 = TB2R + TB2CCR0_INTERVAL;              // Set first interrupt
    //        TB2CCTL0 |= CCIE;                               // Enable TB2 CCR0 interrupt

    //        power_sequence = BOOT_LCD;
    //        break;


    //    case BOOT_LCD:                                      // Power up and initialize LCD
    //        strcpy(display_line[1], "LCD Init..");          // Initialize LCD
    //        Init_LCD();                                     // LCD backlight turns on, splash screen displays

    //        boot_timer_flag = 0;                            // clear timer flag
    //        power_sequence = BOOT_LCD_WAIT;
    //        break;


    //    case BOOT_LCD_WAIT:                                 // Wait 200ms for LCD to stabilize
    //        if (boot_timer_flag) {
    //            boot_timer_flag = 0;
    //            strcpy(display_line[1], "LCD Ready ");      // Display when complete
    //            display_changed = TRUE;
    //            power_sequence = BOOT_IOT_ENABLE;
    //        }
    //        break;


    //    case BOOT_IOT_ENABLE:                               // Enable IOT module
    //        strcpy(display_line[2], "IOT Init..");
    //        display_changed = TRUE;

    //        Init_SerialComms();                             // Initialize UART
    //        P3OUT &= ~IOT_EN;                               // Ensure IOT_EN is LOW first
    //        Enable_Comms();                                 // Prime communication
    //        init_iot_connection = 1;                        // Trigger IOT boot sequence

    //        boot_timer_flag = 0;                            // clear timer flag
    //        power_sequence = BOOT_IOT_WAIT;
    //        break;


    //    case BOOT_IOT_WAIT:                                 // Wait for IOT module to boot and connect
    //                                                        // IOT boot process runs in main loop via Connect_IOT()
    //        if (boot_timer_flag) {
    //            Connect_IOT();                              // Process IOT boot state machine
    //        }
    //        if (iot_boot_complete) {
    //            strcpy(display_line[2], "IOT Ready ");
    //            display_changed = TRUE;
    //            boot_timer_flag = 0;
    //            power_sequence = BOOT_DAC_ENABLE;
    //        }
    //        break;


    //    case BOOT_DAC_ENABLE:                               // Enable DAC power supply
    //        strcpy(display_line[3], "DAC Init..");
    //        display_changed = TRUE;

    //        P2OUT |= DAC_ENB;                               // Enable DAC power (P2.5 HIGH)

    //        boot_timer_flag = 0;
    //        power_sequence = BOOT_DAC_INIT;
    //        break;


    //    case BOOT_DAC_INIT:                                 // Wait 200ms for DAC power to stabilize, then initialize
    //        if (boot_timer_flag) {
    //            boot_timer_flag = 0;
    //            Init_DAC();                                 // Initialize DAC (starts voltage ramp)
    //            power_sequence = BOOT_DAC_WAIT;
    //        }
    //        break;


    //    case BOOT_DAC_WAIT:                                 // Wait for DAC voltage to ramp up to 6V
    //                                                        // DAC_Ramp_ISR() runs in TB0 overflow interrupt
    //        if (DAC_Ready()) {
    //            strcpy(display_line[3], "DAC Ready ");
    //            display_changed = TRUE;
    //            boot_timer_flag = 0;
    //            power_sequence = BOOT_COMPLETE;
    //        }
    //        break;


    //    case BOOT_COMPLETE:                                 // All systems ready
    //        TB2CCTL0 &= ~CCIE;                              // Disable boot timer

    //        strcpy(display_line[0], "DAS BOOTED");
    //        display_changed = TRUE;

    //        boot_complete = TRUE;                              // Exit boot loop
    //        break;

    //    default:
    //        power_sequence = BOOT_INIT;                     // Reset on unknown state
    //        break;
    //    }
    //}
