/*
 * wheels.c
 *
 *  Created on: Nov 27, 2025
 *      Author: Dallas.Owens
 */


#include "msp430.h"
#include "macros.h"
#include "functions.h"
#include "ADC.h"
#include "LCD.h"
#include <string.h>
#include "DAC.h"
#include "calibration.h"
#include "wheels.h"
#include "timers.h"
#include "LED.h"
#include "PWM.h"
#include "switches.h"


typedef enum {
    PART1_IDLE,              // Not running
    PART1_DRIVE_TO_LINE,     // Step 1: Driving forward
    PART1_BRAKE,             // Step 2: Apply 50ms reverse pulse
    PART1_BRAKE_WAIT,        // Step 2: Waiting for 50ms pulse to complete
    PART1_CHECK_LINE,        // Step 3: Check if line still detected
    PART1_BACKUP,            // Step 3: Backing up to find line
    PART1_BACKUP_WAIT,       // Step 3: Waiting for backup pulse
    PART1_BACKUP_SETTLE,     // Step 3: Settling pause after backup pulse
    PART1_ALIGN              // Step 4: Aligning to line
} part1_state_t;

part1_state_t part1_state = PART1_IDLE;

// Globals
extern unsigned char command_enabled;
extern char display_line[4][11];
extern volatile unsigned char display_changed;
volatile unsigned char process_line_follow = FALSE;
volatile unsigned int drive_timer;

// Which sensor detected first (for alignment)
unsigned char first_sensor_left = FALSE;
unsigned char first_sensor_right = FALSE;
extern volatile unsigned char ebraking;
extern unsigned char display_menu;




void Line_Follow_Start_Autonomous(void) {
    if (!calibration_complete) {
        strcpy(display_line[0], "ERROR:    ");
        strcpy(display_line[1], "Must      ");
        strcpy(display_line[2], "calibrate ");
        strcpy(display_line[3], "first!    ");
        display_changed = 1;
        return;
    }

    part1_state = PART1_DRIVE_TO_LINE;
    command_enabled = FALSE;
    process_line_follow = TRUE;
    first_sensor_left = FALSE;
    first_sensor_right = FALSE;
    drive_timer = 0;

    strcpy(display_line[0], "Part1     ");
    strcpy(display_line[1], "Drive to  ");
    strcpy(display_line[2], "line...   ");
    display_changed = 1;

    // Step 1: Start driving forward
    Sample_ADC();
    DAC_Set_Voltage(DAC_MOTOR_CRAWL);
    PWM_FORWARD();  // Use immediate to avoid any delay issues
    
    TB1CCTL1 &= ~CCIFG;
    TB1CCR1 = TB1R + TB1CCR1_INTERVAL;
    TB1CCTL1 |= CCIE;
}

void Line_Follow_Process(void) {
    if (!process_line_follow) return;
    if (safety_stop) return;


    // Read sensors
    // Higher ADC value = BLACK (on line)
    unsigned char left_on_line = (ADC_Left_Detect > LEFT_THRESHOLD);
    unsigned char right_on_line = (ADC_Right_Detect > RIGHT_THRESHOLD);

    unsigned char left_crossed_line = (ADC_Left_Detect > LEFT_BLACK_VALUE);
    unsigned char right_crossed_line = (ADC_Right_Detect > RIGHT_BLACK_VALUE);

    if (SW1_pressed || SW2_pressed){
        SW1_pressed = FALSE;
        SW2_pressed = FALSE;
        Wheels_Safe_Stop();
        process_line_follow = FALSE;
        part1_state = PART1_IDLE;
        display_menu = TRUE;
    }

    switch (part1_state) {

    //==========================================================================
    // STEP 1: Drive forward until line detected
    //==========================================================================
    case PART1_DRIVE_TO_LINE:
        // Keep driving forward
        PWM_FORWARD();

        // Check for line detection
        if (left_crossed_line || right_crossed_line) {
            // Record which sensor detected first
            if (left_crossed_line && !first_sensor_left && !first_sensor_right) {
                first_sensor_left = TRUE;
            }
            if (right_crossed_line && !first_sensor_right && !first_sensor_left) {
                first_sensor_right = TRUE;
            }
            // Move to brake state
            part1_state = PART1_BRAKE;
            drive_timer = 0;

            strcpy(display_line[0], "Part1     ");
            strcpy(display_line[1], "Line      ");
            strcpy(display_line[2], "detected! ");
            display_changed = 1;
        }
        break;

    //==========================================================================
    // STEP 2: Apply 50ms reverse pulse to stop
    //==========================================================================
    case PART1_BRAKE:
        // Apply reverse pulse (ebrake now sets motors to reverse)
        PWM_EBRAKE();
        part1_state = PART1_BRAKE_WAIT;
        break;

    case PART1_BRAKE_WAIT:
        // Wait for 50ms (TB2-2 ISR)
        if (!ebraking) {            // 50ms
            // Stop motors using PWM function
            Wheels_Safe_Stop();

            // Move to check state
            part1_state = PART1_CHECK_LINE;
            drive_timer = 0;

            strcpy(display_line[0], "Part1     ");
            strcpy(display_line[1], "Braked    ");
            strcpy(display_line[2], "Checking  ");
            display_changed = 1;
        }
        break;

    //==========================================================================
    // STEP 3: Check if line still under car
    //==========================================================================
    case PART1_CHECK_LINE:
        // Motors should be stopped
        Wheels_Safe_Stop();

        if (left_on_line || right_on_line) {
            // Line still detected - proceed to alignment
            part1_state = PART1_ALIGN;

            strcpy(display_line[0], "Part1     ");
            strcpy(display_line[1], "Line OK   ");
            strcpy(display_line[2], "Aligning  ");
            display_changed = 1;
        }
        else {
            // Line lost - need to backup
            part1_state = PART1_BACKUP;
            drive_timer = 0;
            DAC_Set_Voltage(DAC_MOTOR_CRAWL);

            strcpy(display_line[0], "Part1     ");
            strcpy(display_line[1], "Line lost ");
            strcpy(display_line[2], "Backing up");
            display_changed = 1;
        }
        break;

    //==========================================================================
    // STEP 3: Backup in short pulses until line found
    //==========================================================================
    case PART1_BACKUP:
        PWM_REVERSE();
        part1_state = PART1_BACKUP_WAIT;
        drive_timer = 0;
        break;

    case PART1_BACKUP_WAIT:
        // Wait for 100ms
        if (drive_timer > DELAY_MS(100)) {  // 100ms
            // Stop and check
            Wheels_Safe_Stop();
            part1_state = PART1_BACKUP_SETTLE;
            drive_timer = 0;
        }
        break;

    case PART1_BACKUP_SETTLE:
        // Wait for 2000ms to settle
        if (drive_timer > DELAY_MS(200)) {
            // Check if line found
            if (left_on_line || right_on_line) {
                // Found line! Record sensor and align
                part1_state = PART1_ALIGN;

                strcpy(display_line[0], "Part1     ");
                strcpy(display_line[1], "Line found");
                strcpy(display_line[2], "Aligning  ");
                display_changed = 1;
            }
            else {
                // Still not found, pulse again
                part1_state = PART1_BACKUP;
            }
        }
        break;

    //==========================================================================
    // STEP 4: Align car to line based on which sensor detected first
    //==========================================================================
    case PART1_ALIGN:
        // Check if both sensors see line (aligned)
        if (left_on_line && right_on_line) {
            // ALIGNED! Stop everything
            Wheels_Safe_Stop();

            strcpy(display_line[0], "Part1     ");
            strcpy(display_line[1], "ALIGNED!  ");
            strcpy(display_line[2], "Complete  ");
            display_changed = 1;

            // Stop processing - Part 1 complete!
            part1_state = PART1_IDLE;
            process_line_follow = FALSE;
            command_enabled = TRUE;
        }
        else if (left_on_line && !right_on_line) {
            // Left on line, turn to get right on
            PWM_ALIGN_LEFT();

        }
        else if (right_on_line && !left_on_line) {
            // Right on line, turn to get left on
            PWM_ALIGN_RIGHT();
        }
        else if (first_sensor_left && !right_on_line) {
            // Left detected first, right not on line
            // Turn left (left wheel slower) to get right wheel on line
            PWM_ALIGN_LEFT();
        }
        else if (first_sensor_right && !left_on_line) {
            // Right detected first, left not on line
            // Turn right (right wheel slower) to get left wheel on line
            PWM_ALIGN_RIGHT();

        }
        break;

    default:
        break;
    }
}

//==============================================================================
// FUNCTION: Line_Follow_Stop
// Emergency stop
//==============================================================================
void Line_Follow_Stop(void) {
    process_line_follow = FALSE;
    part1_state = PART1_IDLE;

    Wheels_Safe_Stop();
    DAC_Set_Voltage(DAC_MOTOR_OFF);

    strcpy(display_line[0], "STOPPED   ");
    display_changed = TRUE;
}
