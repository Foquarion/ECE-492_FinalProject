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

// Drive state machine
typedef enum {
    DRIVE_IDLE,             // Not running
    DRIVE_CALIBRATING,      // Calibrating sensors
    DRIVE_START,            // Driving to black line
    DRIVE_INTERCEPT,        // Stopped at black line
    DRIVE_TURN,             // Turning onto line
    DRIVE_TRAVEL,           // Traveling along line
    DRIVE_CIRCLE,           // Circling
    DRIVE_EXIT,             // Exiting circle
    DRIVE_STOP              // Stopped - complete
} drive_state_t;

drive_state_t drive_state = DRIVE_IDLE;

// Line following state
typedef enum {
    CENTERED,                           // Both sensors on line
    SLIGHT_LEFT,                        // Right sensor off, left on - turn left
    SLIGHT_RIGHT,                       // Left sensor off, right on - turn right
    SHARP_LEFT,                         // Way off right - sharp left
    SHARP_RIGHT,                        // Way off left - sharp right
    LINE_LOST                           // Both off - search
} line_state_t;

line_state_t line_state = CENTERED;
line_state_t last_line_state = CENTERED;

// STATE TIMERS
volatile unsigned int drive_timer = 0;                       // Timer for state pauses
unsigned char drive_pause_complete = FALSE;                  // 10-20 second pause flag
unsigned int circle_lap_count = 0;                           // Circle lap counter

// GLOBALS
extern char display_line[4][11];
extern volatile unsigned char display_changed;
extern unsigned char display_menu;

// ISR FLAGS
volatile unsigned char process_line_follow = FALSE;
volatile unsigned char update_state_timer = FALSE;

//==============================================================================
// FUNCTION: Line_Follow_Proportional
// Proportional line following controller
// CORRECTED: Fixed sensor drift calculation
//==============================================================================
void Line_Follow_Proportional(void) {
    /*
     * SENSOR BEHAVIOR:
     * - Higher ADC value = BLACK (on line)
     * - Lower ADC value = WHITE (off line)
     * - Threshold is midpoint
     * 
     * Your values:
     * Left:  Black=820, White=561, Threshold=690
     * Right: Black=806, White=582, Threshold=694
     */

    // Calculate how far each sensor is from threshold
    // Positive value = sensor is toward BLACK (on line)
    // Negative value = sensor is toward WHITE (off line)
    int left_error = ADC_Left_Detect - LEFT_THRESHOLD;
    int right_error = ADC_Right_Detect - RIGHT_THRESHOLD;

    // Calculate steering error
    // If right_error is MORE NEGATIVE (more white) than left, car is drifting RIGHT
    // If left_error is MORE NEGATIVE (more white) than right, car is drifting LEFT
    int steering_error = right_error - left_error;
    
    // Positive steering_error = right sensor seeing more white = drifting RIGHT, need to turn LEFT
    // Negative steering_error = left sensor seeing more white = drifting LEFT, need to turn RIGHT

    // CENTERED - Both sensors near threshold, minimal drift
    //==========================================================================
    if (abs(steering_error) < DEADZONE) {
        // Well centered on line
        DAC_Set_Voltage(DAC_MOTOR_FAST);
        
        LEFT_FORWARD_SPEED = WHEEL_PERIOD;
        RIGHT_FORWARD_SPEED = WHEEL_PERIOD;
        LEFT_REVERSE_SPEED = WHEEL_OFF;
        RIGHT_REVERSE_SPEED = WHEEL_OFF;
    }

    // STARTING TO DRIFT - Proportional correction BEFORE leaving line
    //==========================================================================
    else if (abs(steering_error) < SHARP_TURN_THRESHOLD) {
        DAC_Set_Voltage(DAC_MOTOR_MEDIUM);

        // Calculate proportional adjustment
        int pwm_adjustment = (steering_error * Kp);

        if (steering_error > 0) {
            // Drifting RIGHT - turn LEFT by slowing right wheel
            LEFT_FORWARD_SPEED = WHEEL_PERIOD;
            RIGHT_FORWARD_SPEED = WHEEL_PERIOD - pwm_adjustment;

            // Clamp minimum speed
            if (RIGHT_FORWARD_SPEED < (WHEEL_PERIOD / 2)) {
                RIGHT_FORWARD_SPEED = WHEEL_PERIOD / 2;
            }
        }
        else {
            // Drifting LEFT - turn RIGHT by slowing left wheel
            RIGHT_FORWARD_SPEED = WHEEL_PERIOD;
            LEFT_FORWARD_SPEED = WHEEL_PERIOD + pwm_adjustment;  // pwm_adjustment is negative

            // Clamp minimum speed
            if (LEFT_FORWARD_SPEED < (WHEEL_PERIOD / 2)) {
                LEFT_FORWARD_SPEED = WHEEL_PERIOD / 2;
            }
        }
        
        LEFT_REVERSE_SPEED = WHEEL_OFF;
        RIGHT_REVERSE_SPEED = WHEEL_OFF;
    }

    // SIGNIFICANT DRIFT - Sharp correction (turn off outside wheel)
    //==========================================================================
    else {
        DAC_Set_Voltage(DAC_MOTOR_SLOW);

        if (steering_error > 0) {
            // Drifting RIGHT - sharp turn LEFT
            LEFT_FORWARD_SPEED = FAST;
            RIGHT_FORWARD_SPEED = SLOW;
            LEFT_REVERSE_SPEED = WHEEL_OFF;
            RIGHT_REVERSE_SPEED = WHEEL_OFF;
        }
        else {
            // Drifting LEFT - sharp turn RIGHT
            LEFT_FORWARD_SPEED = SLOW;
            RIGHT_FORWARD_SPEED = FAST;
            LEFT_REVERSE_SPEED = WHEEL_OFF;
            RIGHT_REVERSE_SPEED = WHEEL_OFF;
        }
    }
}

//==============================================================================
// FUNCTION: Line_Follow_Start_Autonomous
// Entry point for autonomous line following
// Triggered by IoT command "AUTONOMOUS" or SW2 press
//==============================================================================
void Line_Follow_Start_Autonomous(void) {
    if (!calibration_complete) {
        strcpy(display_line[0], "ERROR:    ");
        strcpy(display_line[1], "Must      ");
        strcpy(display_line[2], "calibrate ");
        strcpy(display_line[3], "first!    ");
        display_changed = 1;
        return;
    }

    drive_state = DRIVE_START;
    drive_timer = 0;
    drive_pause_complete = FALSE;
    circle_lap_count = 0;
    process_line_follow = TRUE;

    strcpy(display_line[0], "BL Start  ");
    display_changed = 1;

    // Start driving toward line
    DAC_Set_Voltage(DAC_MOTOR_MEDIUM);
    
    // DIRECTLY set PWM - motors should move NOW
    LEFT_FORWARD_SPEED = SLOW;
    RIGHT_FORWARD_SPEED = SLOW;
    LEFT_REVERSE_SPEED = WHEEL_OFF;
    RIGHT_REVERSE_SPEED = WHEEL_OFF;

    // Enable line following timer interrupt
    TB1CCTL1 &= ~CCIFG;                     // Clear possible pending interrupt
    TB1CCTL1 |= CCIE;                       // CCR1 enable interrupt
    TB1CCR1 = TB1R + TB1CCR1_INTERVAL;
}

//==============================================================================
// FUNCTION: Line_Follow_Process
// Main line following state machine
// Called from main loop when process_line_follow flag set by ISR
// drive_timer increments each call (100ms intervals)
//==============================================================================
void Line_Follow_Process(void) {
    if (!process_line_follow) return;

    if (drive_state == DRIVE_IDLE) {
        return;
    }

    // Check sensor states
    // Greater than threshold = On Line (black)
    // Less than threshold = Off Line (white)
    unsigned char left_on_line = (ADC_Left_Detect > LEFT_THRESHOLD);
    unsigned char right_on_line = (ADC_Right_Detect > RIGHT_THRESHOLD);

    switch (drive_state) {
    //==========================================================================
    // DRIVE_START: Driving toward black line
    //==========================================================================
    case DRIVE_START:
        // KEEP MOVING FORWARD
        LEFT_FORWARD_SPEED = SLOW;
        RIGHT_FORWARD_SPEED = SLOW;
        LEFT_REVERSE_SPEED = WHEEL_OFF;
        RIGHT_REVERSE_SPEED = WHEEL_OFF;
        
        if (left_on_line || right_on_line) {
            // LINE DETECTED! Stop immediately
            LEFT_FORWARD_SPEED = WHEEL_OFF;
            RIGHT_FORWARD_SPEED = WHEEL_OFF;
            LEFT_REVERSE_SPEED = WHEEL_OFF;
            RIGHT_REVERSE_SPEED = WHEEL_OFF;

            drive_state = DRIVE_INTERCEPT;
            drive_timer = 0;

            strcpy(display_line[0], "Intercept ");
            display_changed = TRUE;
        }
        break;

    //==========================================================================
    // DRIVE_INTERCEPT: Stopped at line, waiting 15 seconds
    //==========================================================================
    case DRIVE_INTERCEPT:
        // Stay stopped during intercept wait
        LEFT_FORWARD_SPEED = WHEEL_OFF;
        RIGHT_FORWARD_SPEED = WHEEL_OFF;
        LEFT_REVERSE_SPEED = WHEEL_OFF;
        RIGHT_REVERSE_SPEED = WHEEL_OFF;
        
        if (drive_timer >= DRIVE_DELAY_MS(15000)) {  // 15 seconds
            drive_state = DRIVE_TURN;
            drive_timer = 0;

            strcpy(display_line[0], "BL Turn   ");
            display_changed = TRUE;

            DAC_Set_Voltage(DAC_MOTOR_SLOW);
        }
        break;

    //==========================================================================
    // DRIVE_TURN: Turning to align with line
    //==========================================================================
    case DRIVE_TURN:
        if (left_on_line && !right_on_line) {
            // Left sensor on, turn left to center
            LEFT_FORWARD_SPEED = SLOW;
            RIGHT_FORWARD_SPEED = FAST;
            LEFT_REVERSE_SPEED = WHEEL_OFF;
            RIGHT_REVERSE_SPEED = WHEEL_OFF;
        }
        else if (!left_on_line && right_on_line) {
            // Right sensor on, turn right to center
            LEFT_FORWARD_SPEED = FAST;
            RIGHT_FORWARD_SPEED = SLOW;
            LEFT_REVERSE_SPEED = WHEEL_OFF;
            RIGHT_REVERSE_SPEED = WHEEL_OFF;
        }
        else if (left_on_line && right_on_line) {
            // Both sensors on line - aligned!
            LEFT_FORWARD_SPEED = WHEEL_OFF;
            RIGHT_FORWARD_SPEED = WHEEL_OFF;
            LEFT_REVERSE_SPEED = WHEEL_OFF;
            RIGHT_REVERSE_SPEED = WHEEL_OFF;
            
            drive_state = DRIVE_TRAVEL;
            drive_timer = 0;
            drive_pause_complete = FALSE;

            strcpy(display_line[0], "BL Travel ");
            display_changed = 1;
        }
        break;

    //==========================================================================
    // DRIVE_TRAVEL: Following line to circle
    //==========================================================================
    case DRIVE_TRAVEL:
        if (!drive_pause_complete) {
            // Pausing for 15 seconds
            LEFT_FORWARD_SPEED = WHEEL_OFF;
            RIGHT_FORWARD_SPEED = WHEEL_OFF;
            LEFT_REVERSE_SPEED = WHEEL_OFF;
            RIGHT_REVERSE_SPEED = WHEEL_OFF;
            
            if (drive_timer >= DRIVE_DELAY_MS(15000)) {
                drive_pause_complete = TRUE;
                drive_timer = 0;
            }
        }
        else {
            // Actually following line with proportional control
            Line_Follow_Proportional();
        }
        break;

    //==========================================================================
    // DRIVE_CIRCLE: Circling (must complete 2 laps minimum)
    //==========================================================================
    case DRIVE_CIRCLE:
        if (!drive_pause_complete) {
            // Initial pause before circling
            LEFT_FORWARD_SPEED = WHEEL_OFF;
            RIGHT_FORWARD_SPEED = WHEEL_OFF;
            LEFT_REVERSE_SPEED = WHEEL_OFF;
            RIGHT_REVERSE_SPEED = WHEEL_OFF;

            if (drive_timer >= DRIVE_DELAY_MS(15000)) {
                drive_pause_complete = TRUE;
                strcpy(display_line[0], "BL Circle ");
                display_changed = TRUE;
            }
        }
        else {
            // Follow circle using proportional control
            Line_Follow_Proportional();
        }
        break;

    //==========================================================================
    // DRIVE_EXIT: Exiting circle after command received
    //==========================================================================
    case DRIVE_EXIT:
        strcpy(display_line[0], "BL Exit   ");
        display_changed = 1;

        // Drive straight away from circle
        DAC_Set_Voltage(DAC_MOTOR_MEDIUM);
        LEFT_FORWARD_SPEED = SLOW;
        RIGHT_FORWARD_SPEED = SLOW;
        LEFT_REVERSE_SPEED = WHEEL_OFF;
        RIGHT_REVERSE_SPEED = WHEEL_OFF;

        if (drive_timer >= DRIVE_DELAY_MS(3000)) {  // 3 seconds
            drive_state = DRIVE_STOP;
        }
        break;

    //==========================================================================
    // DRIVE_STOP: Course complete
    //==========================================================================
    case DRIVE_STOP:
        LEFT_FORWARD_SPEED = WHEEL_OFF;
        RIGHT_FORWARD_SPEED = WHEEL_OFF;
        LEFT_REVERSE_SPEED = WHEEL_OFF;
        RIGHT_REVERSE_SPEED = WHEEL_OFF;
        DAC_Set_Voltage(DAC_MOTOR_OFF);

        strcpy(display_line[0], "BL Stop   ");
        strcpy(display_line[1], "Course    ");
        strcpy(display_line[2], "Complete! ");
        display_changed = TRUE;

        process_line_follow = FALSE;

        // Disable line following timer interrupt
        TB1CCTL1 &= ~CCIFG;
        TB1CCTL1 &= ~CCIE;

        display_menu = TRUE;
        break;

    default:
        break;
    }
}

//==============================================================================
// FUNCTION: Line_Follow_Exit_Circle
// Called via IoT to leave circle after 2+ laps
//==============================================================================
void Line_Follow_Exit_Circle(void) {
    drive_state = DRIVE_EXIT;
    drive_timer = 0;
}

//==============================================================================
// FUNCTION: Line_Follow_Stop
// Emergency stop for line following
//==============================================================================
void Line_Follow_Stop(void) {
    process_line_follow = FALSE;
    drive_state = DRIVE_IDLE;
    
    LEFT_FORWARD_SPEED = WHEEL_OFF;
    RIGHT_FORWARD_SPEED = WHEEL_OFF;
    LEFT_REVERSE_SPEED = WHEEL_OFF;
    RIGHT_REVERSE_SPEED = WHEEL_OFF;
    DAC_Set_Voltage(DAC_MOTOR_OFF);

    strcpy(display_line[0], "STOPPED   ");
    display_changed = TRUE;
}
