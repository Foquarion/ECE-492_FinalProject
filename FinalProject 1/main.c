//------------------------------------------------------------------------------
//
//  Description: Main program for LED Marquee demonstration
//
//  Requirements:
//    1. Default state: LEDs 3-9 ON
//    2. SW1: Start/Stop marquee sequence
//    3. SW2: If marquee running -> stop and turn off LEDs
//            If marquee not running -> toggle LEDs on/off
//
//  Marquee Sequence:
//    - Turn off all LEDs
//    - Turn on LEDs 3->9 sequentially (300ms between each)
//    - Turn off LEDs 3->9 sequentially (300ms between each)
//    - Return to default state (all on)
//
//  Author: Dallas Owens
//  Date: Dec 2025
//------------------------------------------------------------------------------

#include "msp430.h"
#include "functions.h"
#include "ports.h"
#include "timers.h"
#include "switches.h"
#include "LED.h"


void main(void) {
    // Stop watchdog timer
    WDTCTL = WDTPW | WDTHOLD;

    // Initialize hardware
    Init_Ports();                   // Configure GPIO pins
    Init_Clocks();                  // Configure clock system (also clears LOCKLPM5)
    Init_Conditions();              // Initialize variables
    Init_Timers();                  // Configure timers

    // Enable global interrupts
    __enable_interrupt();

    // Set default state: LEDs 3-9 ON
    LEDS_3_9_ON();

    // Main loop
    while (1) {
        Switches_Process();         // Check for switch presses
        Marquee_Process();          // Update marquee state machine
    }
}
