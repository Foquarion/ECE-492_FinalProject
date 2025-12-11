/*
 * wheels.c
 *
 *  Created on: Nov 27, 2025
 *      Author: Dallas.Owens
 *
 *  Description: Line following state machine for Project 10
 *               Uses centered detection threshold for improved accuracy
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


//==============================================================================
// LINE FOLLOWING STATE MACHINE
//==============================================================================
typedef enum {
    BL_IDLE,
    BL_START,               // "BL Start" - driving to find line
    BL_INTERCEPT,           // "Intercept" - line detected, stop
    BL_INTERCEPT_WAIT,      // Wait 12 seconds
    BL_TURN,                // "BL Turn" - timed 300ms spin
    BL_TURN_WAIT,           // Wait 12 seconds after turn
    BL_TRAVEL,              // "BL Travel" - following line
    BL_TRAVEL_WAIT,         // Wait 12 seconds before circle
    BL_CIRCLE,              // "BL Circle" - following circle
    BL_CIRCLE_WAIT,         // Wait after exit command
    BL_EXIT,                // "BL Exit" - driving away
    BL_STOP                 // "BL Stop" - finished
} bl_state_t;

static bl_state_t bl_state = BL_IDLE;


//==============================================================================
// DIRECTION TRACKING
//==============================================================================
#define GOING_STRAIGHT  (0)
#define GOING_LEFT      (1)
#define GOING_RIGHT     (2)

static unsigned char LastDir = GOING_STRAIGHT;


//==============================================================================
// CENTERED DETECTION VARIABLES
//==============================================================================
static int sensor_offset = 0;                   // Offset between sensors (calibration)
static unsigned int BLACK_DETECT_THRESHOLD = 0; // Single centered threshold

// Real-time centered values
static int left_centered = 0;
static int right_centered = 0;
static int IR_CAR_CENTERED = 0;

// Error for corrections
static int steering_error = 0;

// Thresholds for correction type
#define SHARP_CORRECTION_THRESHOLD  (150)   // Above this = stop wheel (smaller for inverted sensors)
#define PROPORTIONAL_GAIN           (3)     // PWM adjustment per unit error (gentle for small range)


//==============================================================================
// GLOBALS
//==============================================================================
extern unsigned char command_enabled;
extern char display_line[4][11];
extern volatile unsigned char display_changed;
volatile unsigned char process_line_follow = FALSE;
volatile unsigned int drive_timer;

extern volatile unsigned char ebraking;
extern unsigned char display_menu;
extern volatile unsigned char safety_stop;

// Turn direction after intercept
static unsigned char turn_left_after_intercept = FALSE;

// Exit flag from IoT command
volatile unsigned char exit_command_received = FALSE;


//==============================================================================
// TIMING PARAMETERS
//==============================================================================
#define STATE_PAUSE_TIME        DELAY_MS(2000)     // 12 seconds pause
#define TURN_SPIN_TIME          DELAY_MS(500)       // 300ms timed spin
#define TRAVEL_TO_CIRCLE_TIME   DELAY_MS(8000)      // Time before circle state
#define MIN_CIRCLE_TIME         DELAY_MS(25000)     // Min time for 2 rotations
#define EXIT_DRIVE_TIME         DELAY_MS(3500)      // Time to drive 2+ feet


//==============================================================================
// FUNCTION: Calculate_Centered_Values
// Calculates the centered threshold from calibration values
// Call once after calibration is complete
//==============================================================================
static void Calculate_Centered_Values(void) {
    // Sensor offset: difference between left and right black readings / 2
    sensor_offset = ((int)LEFT_BLACK_VALUE - (int)RIGHT_BLACK_VALUE) / 2;
    
    // Single centered threshold: average of all four calibration values
    BLACK_DETECT_THRESHOLD = (LEFT_BLACK_VALUE + RIGHT_BLACK_VALUE + 
                              LEFT_WHITE_VALUE + RIGHT_WHITE_VALUE) / 4;
}


//==============================================================================
// FUNCTION: Update_Centered_Readings
// Updates real-time centered sensor readings
// Call before making line detection decisions
//==============================================================================
static void Update_Centered_Readings(void) {
    // Apply offset to center both sensors
    left_centered = (int)ADC_Left_Detect - sensor_offset;
    right_centered = (int)ADC_Right_Detect + sensor_offset;
    
    // Average of centered readings = car's centered position
    IR_CAR_CENTERED = (left_centered + right_centered) / 2;
    
    // Steering error: positive = drifting left, negative = drifting right
    steering_error = left_centered - right_centered;
}


//==============================================================================
// FUNCTION: Is_On_Line
// Returns TRUE if car is on the line based on centered threshold
//==============================================================================
static unsigned char Is_On_Line(void) {
    return (IR_CAR_CENTERED > (int)BLACK_DETECT_THRESHOLD);
}


//==============================================================================
// FUNCTION: Line_Follow_Update
// Main line following logic with proportional control and line-loss recovery
//==============================================================================
static void Line_Follow_Update(void) {
    Update_Centered_Readings();
    
    int abs_error = (steering_error >= 0) ? steering_error : -steering_error;
    
    if (!Is_On_Line()) {
        // Lost line - do quick reverse pulse to find it again
        LEFT_FORWARD_SPEED = WHEEL_OFF;
        RIGHT_FORWARD_SPEED = WHEEL_OFF;
        LEFT_REVERSE_SPEED = SLOW;
        RIGHT_REVERSE_SPEED = SLOW;
        
        // This will trigger interrupt-driven timing for pulse duration
        return;
    }
    
    // On line - determine correction type
    if (abs_error < SHARP_CORRECTION_THRESHOLD) {
        //==================================================================
        // PROPORTIONAL CORRECTION - smooth steering based on real-time drift
        // Uses steering_error to detect LEFT (positive) vs RIGHT (negative) drift
        // Reduces the drifting wheel proportionally to error magnitude
        //==================================================================
        int pwm_adjustment = abs_error * PROPORTIONAL_GAIN;
        
        // Clamp adjustment to prevent overshooting
        if (pwm_adjustment > (SLOW - CRAWL)) {
            pwm_adjustment = SLOW - CRAWL;
        }
        
        LEFT_REVERSE_SPEED = WHEEL_OFF;
        RIGHT_REVERSE_SPEED = WHEEL_OFF;
        
        if (steering_error > 0) {
            // Drifting LEFT (left sensor darker) - reduce left wheel speed to turn right
            LEFT_FORWARD_SPEED = SLOW - pwm_adjustment;
            RIGHT_FORWARD_SPEED = SLOW;
            LastDir = GOING_LEFT;
            
            // Clamp minimum
            if (LEFT_FORWARD_SPEED < CRAWL) {
                LEFT_FORWARD_SPEED = CRAWL;
            }
        }
        else if (steering_error < 0) {
            // Drifting RIGHT (right sensor darker) - reduce right wheel speed to turn left
            LEFT_FORWARD_SPEED = SLOW;
            RIGHT_FORWARD_SPEED = SLOW - pwm_adjustment;
            LastDir = GOING_RIGHT;
            
            // Clamp minimum
            if (RIGHT_FORWARD_SPEED < CRAWL) {
                RIGHT_FORWARD_SPEED = CRAWL;
            }
        }
        else {
            // Perfectly centered - go straight at full speed
            LEFT_FORWARD_SPEED = SLOW;
            RIGHT_FORWARD_SPEED = SLOW;
            LastDir = GOING_STRAIGHT;
        }
    }
    else {
        //==================================================================
        // SHARP CORRECTION - significant drift, stop one wheel
        //==================================================================
        LEFT_REVERSE_SPEED = WHEEL_OFF;
        RIGHT_REVERSE_SPEED = WHEEL_OFF;
        
        if (steering_error > 0) {
            // Drifting left significantly - stop left wheel
            LEFT_FORWARD_SPEED = WHEEL_OFF;
            RIGHT_FORWARD_SPEED = SLOW;
            LastDir = GOING_LEFT;
        }
        else {
            // Drifting right significantly - stop right wheel
            LEFT_FORWARD_SPEED = SLOW;
            RIGHT_FORWARD_SPEED = WHEEL_OFF;
            LastDir = GOING_RIGHT;
        }
    }
}


//==============================================================================
// FUNCTION: Line_Follow_Start_Autonomous
// Entry point - called when IoT sends "T" command
//==============================================================================
void Line_Follow_Start_Autonomous(void) {
    if (!calibration_complete) {
        strcpy(display_line[0], "  ERROR:  ");
        strcpy(display_line[1], "Calibrate ");
        strcpy(display_line[2], "  First!  ");
        display_changed = 1;
        return;
    }

    // Calculate centered values from calibration
    Calculate_Centered_Values();

    // Initialize state machine
    bl_state = BL_START;
    command_enabled = FALSE;
    process_line_follow = TRUE;
    drive_timer = 0;
    exit_command_received = FALSE;
    LastDir = GOING_STRAIGHT;
    turn_left_after_intercept = FALSE;

    // Display "BL Start"
    strcpy(display_line[0], " BL Start ");
    display_changed = 1;

    // Set voltage and start moving
    DAC_Set_Voltage(DAC_MOTOR_CRAWL);
    Sample_ADC();
    
    // Drive forward
    LEFT_FORWARD_SPEED = SLOW;
    RIGHT_FORWARD_SPEED = SLOW;
    LEFT_REVERSE_SPEED = WHEEL_OFF;
    RIGHT_REVERSE_SPEED = WHEEL_OFF;
    
    // Enable drive timer (100ms intervals)
    TB1CCTL1 &= ~CCIFG;
    TB1CCR1 = TB1R + TB1CCR1_INTERVAL;
    TB1CCTL1 |= CCIE;
}


//==============================================================================
// FUNCTION: Line_Follow_Process
// Main state machine - called from main loop
//==============================================================================
void Line_Follow_Process(void) {
    if (!process_line_follow) return;
    if (safety_stop) return;

    // Update centered readings
    Update_Centered_Readings();

    // Emergency stop check
    if (SW1_pressed || SW2_pressed) {
        SW1_pressed = FALSE;
        SW2_pressed = FALSE;
        Line_Follow_Stop();
        display_menu = TRUE;
        return;
    }

    switch (bl_state) {

    //==========================================================================
    // BL_START: Drive forward until line detected
    //==========================================================================
    case BL_START:
        // Keep driving forward
        LEFT_FORWARD_SPEED = SLOW;
        RIGHT_FORWARD_SPEED = SLOW;
        LEFT_REVERSE_SPEED = WHEEL_OFF;
        RIGHT_REVERSE_SPEED = WHEEL_OFF;

        // Check for line detection using centered threshold
        if (Is_On_Line()) {
            // Determine turn direction based on which side is darker
            if (steering_error > 0) {
                // Left sensor darker - line is more to the left - turn left
                turn_left_after_intercept = TRUE;
            }
            else {
                // Right sensor darker - line is more to the right - turn right
                turn_left_after_intercept = FALSE;
            }
            
            bl_state = BL_INTERCEPT;
            drive_timer = 0;
        }
        break;

    //==========================================================================
    // BL_INTERCEPT: Stop and wait 12 seconds
    //==========================================================================
    case BL_INTERCEPT:
        // Stop motors
        Wheels_Safe_Stop();
        
        strcpy(display_line[0], " Intercept");
        display_changed = 1;
        
        bl_state = BL_INTERCEPT_WAIT;
        drive_timer = 0;
        break;

    case BL_INTERCEPT_WAIT:
        Wheels_Safe_Stop();
        
        if (drive_timer >= STATE_PAUSE_TIME) {
            bl_state = BL_TURN;
            drive_timer = 0;
            
            strcpy(display_line[0], "  BL Turn ");
            display_changed = 1;
        }
        break;

    //==========================================================================
    // BL_TURN: Rotate slowly until BOTH sensors detect the line
    //==========================================================================
    case BL_TURN:
        Update_Centered_Readings();
        
        // Check if BOTH sensors are on the line (both values > threshold)
        unsigned char left_on_line = (left_centered > (int)BLACK_DETECT_THRESHOLD);
        unsigned char right_on_line = (right_centered > (int)BLACK_DETECT_THRESHOLD);
        
        if (left_on_line && right_on_line) {
            // Both sensors on line - well centered, ready to travel
            Wheels_Safe_Stop();
            bl_state = BL_TURN_WAIT;
            drive_timer = 0;
        }
        else if (!left_on_line && !right_on_line) {
            // Neither sensor on line - do quick reverse pulse to find line
            LEFT_FORWARD_SPEED = WHEEL_OFF;
            RIGHT_FORWARD_SPEED = WHEEL_OFF;
            LEFT_REVERSE_SPEED = SLOW;
            RIGHT_REVERSE_SPEED = SLOW;
            
            // Brief reverse pulse timing
            if (drive_timer >= DELAY_MS(100)) {
                // Pulse done, stop and continue rotating
                LEFT_REVERSE_SPEED = WHEEL_OFF;
                RIGHT_REVERSE_SPEED = WHEEL_OFF;
                drive_timer = 0;
            }
        }
        else {
            // At least one sensor on line - continue rotating slowly toward center
            // Rotate in direction to align both sensors
            if (left_on_line && !right_on_line) {
                // Left sensor on line, right off - rotate right (slow left, faster right)
                LEFT_FORWARD_SPEED = CRAWL;
                RIGHT_FORWARD_SPEED = LINE_SPEED;
                LEFT_REVERSE_SPEED = WHEEL_OFF;
                RIGHT_REVERSE_SPEED = WHEEL_OFF;
            }
            else {
                // Right sensor on line, left off - rotate left (faster left, slow right)
                LEFT_FORWARD_SPEED = LINE_SPEED;
                RIGHT_FORWARD_SPEED = CRAWL;
                LEFT_REVERSE_SPEED = WHEEL_OFF;
                RIGHT_REVERSE_SPEED = WHEEL_OFF;
            }
        }
        break;

    case BL_TURN_WAIT:
        Wheels_Safe_Stop();
        
        if (drive_timer >= STATE_PAUSE_TIME) {
            bl_state = BL_TRAVEL;
            drive_timer = 0;
            LastDir = GOING_STRAIGHT;
            
            strcpy(display_line[0], " BL Travel");
            display_changed = 1;
        }
        break;

    //==========================================================================
    // BL_TRAVEL: Follow line using centered detection
    //==========================================================================
    case BL_TRAVEL:
        Line_Follow_Update();
        
        if (drive_timer >= TRAVEL_TO_CIRCLE_TIME) {
            Wheels_Safe_Stop();
            bl_state = BL_TRAVEL_WAIT;
            drive_timer = 0;
        }
        break;

    case BL_TRAVEL_WAIT:
        Wheels_Safe_Stop();
        
        if (drive_timer >= STATE_PAUSE_TIME) {
            bl_state = BL_CIRCLE;
            drive_timer = 0;
            LastDir = GOING_STRAIGHT;
            
            strcpy(display_line[0], " BL Circle");
            display_changed = 1;
        }
        break;

    //==========================================================================
    // BL_CIRCLE: Follow circle, wait for exit command
    //==========================================================================
    case BL_CIRCLE:
        Line_Follow_Update();
        
        // Exit when command received and minimum time elapsed
        if (exit_command_received && (drive_timer >= MIN_CIRCLE_TIME)) {
            Wheels_Safe_Stop();
            bl_state = BL_CIRCLE_WAIT;
            drive_timer = 0;
        }
        break;

    case BL_CIRCLE_WAIT:
        Wheels_Safe_Stop();
        
        if (drive_timer >= DELAY_MS(2000)) {
            bl_state = BL_EXIT;
            drive_timer = 0;
            
            strcpy(display_line[0], "  BL Exit ");
            display_changed = 1;
        }
        break;

    //==========================================================================
    // BL_EXIT: Drive straight forward away from circle
    //==========================================================================
    case BL_EXIT:
        LEFT_FORWARD_SPEED = SLOW;
        RIGHT_FORWARD_SPEED = SLOW;
        LEFT_REVERSE_SPEED = WHEEL_OFF;
        RIGHT_REVERSE_SPEED = WHEEL_OFF;
        
        if (drive_timer >= EXIT_DRIVE_TIME) {
            Wheels_Safe_Stop();
            bl_state = BL_STOP;
            drive_timer = 0;
        }
        break;

    //==========================================================================
    // BL_STOP: Finished
    //==========================================================================
    case BL_STOP:
        Wheels_Safe_Stop();
        DAC_Set_Voltage(DAC_MOTOR_OFF);
        
        strcpy(display_line[0], "  BL Stop ");
        strcpy(display_line[1], " Complete!");
        strcpy(display_line[2], "  Dallas  ");
        display_changed = 1;
        
        process_line_follow = FALSE;
        bl_state = BL_IDLE;
        command_enabled = TRUE;
        
        TB1CCTL1 &= ~CCIE;
        break;

    default:
        bl_state = BL_IDLE;
        break;
    }
}


//==============================================================================
// FUNCTION: Line_Follow_Exit_Circle
// Called by IoT command "X" to exit the circle
//==============================================================================
void Line_Follow_Exit_Circle(void) {
    exit_command_received = TRUE;
}


//==============================================================================
// FUNCTION: Line_Follow_Stop
// Emergency stop
//==============================================================================
void Line_Follow_Stop(void) {
    process_line_follow = FALSE;
    bl_state = BL_IDLE;
    exit_command_received = FALSE;
    LastDir = GOING_STRAIGHT;

    Wheels_Safe_Stop();
    DAC_Set_Voltage(DAC_MOTOR_OFF);

    strcpy(display_line[0], " STOPPED  ");
    display_changed = TRUE;
    
    command_enabled = TRUE;
    TB1CCTL1 &= ~CCIE;
}
