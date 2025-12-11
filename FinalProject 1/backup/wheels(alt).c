/*
 * wheels.c
 *
 *  Created on: Nov 27, 2025
 *      Author: Dallas.Owens
 *
 *  Description: Line following state machine for Project 10
 *               Based on reference implementation by Sam Messick
 *
 *  SENSOR NOTES:
 *      - Higher ADC value = BLACK (on line)
 *      - Lower ADC value = WHITE (off line)
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
// DIRECTION TRACKING
//==============================================================================
#define GOING_STRAIGHT  (0)
#define GOING_LEFT      (1)
#define GOING_RIGHT     (2)

static unsigned char LastDir = GOING_STRAIGHT;


//==============================================================================
// LINE FOLLOWING STATE MACHINE
//==============================================================================
typedef enum {
    BL_IDLE,
    BL_START,               // "BL Start" - driving to find line
    BL_INTERCEPT,           // "Intercept" - line detected, stop
    BL_INTERCEPT_WAIT,      // Wait for pause
    BL_TURN,                // "BL Turn" - turning to align
    BL_TURN_WAIT,           // Wait after turn
    BL_TRAVEL,              // "BL Travel" - following line
    BL_TRAVEL_WAIT,         // Wait for pause
    BL_CIRCLE,              // "BL Circle" - following circle
    BL_CIRCLE_WAIT,         // Wait after exit command
    BL_EXIT,                // "BL Exit" - leaving circle
    BL_STOP                 // "BL Stop" - finished
} bl_state_t;

static bl_state_t bl_state = BL_IDLE;


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

// Exit flag from IoT command
volatile unsigned char exit_command_received = FALSE;


//==============================================================================
// TIMING PARAMETERS
//==============================================================================
#define STATE_PAUSE_TIME        DELAY_MS(12000)     // 12 seconds pause
#define TURN_TIME               DELAY_MS(1500)      // Time for 90 degree turn
#define TRAVEL_TO_CIRCLE_TIME   DELAY_MS(8000)      // Time before circle state
#define MIN_CIRCLE_TIME         DELAY_MS(25000)     // Min time for 2 rotations
#define EXIT_DRIVE_TIME         DELAY_MS(3500)      // Time to drive 2+ feet


//==============================================================================
// MOTOR HELPER FUNCTIONS
//==============================================================================

// Left motor forward at specified speed
static void Left_Motor_Forward(unsigned int speed) {
    LEFT_FORWARD_SPEED = speed;
    LEFT_REVERSE_SPEED = WHEEL_OFF;
}

// Left motor reverse at specified speed
static void Left_Motor_Reverse(unsigned int speed) {
    LEFT_FORWARD_SPEED = WHEEL_OFF;
    LEFT_REVERSE_SPEED = speed;
}

// Left motor off
static void Left_Motor_Off(void) {
    LEFT_FORWARD_SPEED = WHEEL_OFF;
    LEFT_REVERSE_SPEED = WHEEL_OFF;
}

// Right motor forward at specified speed
static void Right_Motor_Forward(unsigned int speed) {
    RIGHT_FORWARD_SPEED = speed;
    RIGHT_REVERSE_SPEED = WHEEL_OFF;
}

// Right motor reverse at specified speed
static void Right_Motor_Reverse(unsigned int speed) {
    RIGHT_FORWARD_SPEED = WHEEL_OFF;
    RIGHT_REVERSE_SPEED = speed;
}

// Right motor off
static void Right_Motor_Off(void) {
    RIGHT_FORWARD_SPEED = WHEEL_OFF;
    RIGHT_REVERSE_SPEED = WHEEL_OFF;
}

// Both motors off
static void Motors_Off(void) {
    Left_Motor_Off();
    Right_Motor_Off();
}

// Drive straight forward
static void Drive_Straight(unsigned int speed) {
    Left_Motor_Forward(speed);
    Right_Motor_Forward(speed);
}

// Turn left (slow left, fast right)
static void Drive_Left(unsigned int slow_speed, unsigned int fast_speed) {
    Left_Motor_Forward(slow_speed);
    Right_Motor_Forward(fast_speed);
}

// Turn right (fast left, slow right)
static void Drive_Right(unsigned int fast_speed, unsigned int slow_speed) {
    Left_Motor_Forward(fast_speed);
    Right_Motor_Forward(slow_speed);
}

// Spin left in place
static void Spin_Left(unsigned int speed) {
    Left_Motor_Reverse(speed);
    Right_Motor_Forward(speed);
}

// Spin right in place
static void Spin_Right(unsigned int speed) {
    Left_Motor_Forward(speed);
    Right_Motor_Reverse(speed);
}


//==============================================================================
// LINE FOLLOWING CORE LOGIC
// Based on reference: pid_update_left()
//==============================================================================
static void Line_Follow_Update(void) {
    // Read sensors directly (no swap)
    // Higher value = on black line
    unsigned char left_on_line = (ADC_Left_Detect > LEFT_THRESHOLD);
    unsigned char right_on_line = (ADC_Right_Detect > RIGHT_THRESHOLD);
    
    if (left_on_line) {
        if (right_on_line) {
            // Both sensors on line - go straight
            Drive_Straight(SLOW);
            LastDir = GOING_STRAIGHT;
        }
        else {
            // Left on, Right off - line is to the left, turn left
            Drive_Left(CRAWL, SLOW);
            LastDir = GOING_LEFT;
        }
    }
    else {
        if (right_on_line) {
            // Left off, Right on - line is to the right, turn right
            Drive_Right(SLOW, CRAWL);
            LastDir = GOING_RIGHT;
        }
        else {
            // Both sensors off line - continue in last direction
            if (LastDir == GOING_LEFT) {
                Drive_Left(CRAWL, SLOW);
            }
            else if (LastDir == GOING_RIGHT) {
                Drive_Right(SLOW, CRAWL);
            }
            else {
                // No last direction, go straight slowly
                Drive_Straight(CRAWL);
            }
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

    // Initialize state machine
    bl_state = BL_START;
    command_enabled = FALSE;
    process_line_follow = TRUE;
    drive_timer = 0;
    exit_command_received = FALSE;
    LastDir = GOING_STRAIGHT;

    // Display "BL Start"
    strcpy(display_line[0], " BL Start ");
    display_changed = 1;

    // Set voltage for line following
    DAC_Set_Voltage(DAC_MOTOR_SLOW);
    
    // Start ADC sampling
    Sample_ADC();
    
    // Start driving forward
    Drive_Straight(SLOW);
    
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

    // Read sensors
    unsigned char left_on_line = (ADC_Left_Detect > LEFT_THRESHOLD);
    unsigned char right_on_line = (ADC_Right_Detect > RIGHT_THRESHOLD);
    unsigned char either_on_line = left_on_line || right_on_line;
    unsigned char both_on_line = left_on_line && right_on_line;

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
        Drive_Straight(SLOW);

        // Check for line detection
        if (either_on_line) {
            // Record which side detected for turn direction
            if (left_on_line && !right_on_line) {
                LastDir = GOING_LEFT;
            } else if (right_on_line && !left_on_line) {
                LastDir = GOING_RIGHT;
            } else {
                LastDir = GOING_LEFT;  // Default if both detect
            }
            
            bl_state = BL_INTERCEPT;
            drive_timer = 0;
        }
        break;

    //==========================================================================
    // BL_INTERCEPT: Stop and wait
    //==========================================================================
    case BL_INTERCEPT:
        Motors_Off();
        
        strcpy(display_line[0], " Intercept");
        display_changed = 1;
        
        bl_state = BL_INTERCEPT_WAIT;
        drive_timer = 0;
        break;

    case BL_INTERCEPT_WAIT:
        Motors_Off();
        
        if (drive_timer >= STATE_PAUSE_TIME) {
            bl_state = BL_TURN;
            drive_timer = 0;
            
            strcpy(display_line[0], "  BL Turn ");
            display_changed = 1;
        }
        break;

    //==========================================================================
    // BL_TURN: Spin to align with line (90 degree turn)
    //==========================================================================
    case BL_TURN:
        // Check if aligned (both sensors on line)
        if (both_on_line) {
            Motors_Off();
            bl_state = BL_TURN_WAIT;
            drive_timer = 0;
        }
        else if (drive_timer >= TURN_TIME) {
            // Timeout - stop anyway
            Motors_Off();
            bl_state = BL_TURN_WAIT;
            drive_timer = 0;
        }
        else {
            // Spin in the direction of the line
            if (LastDir == GOING_LEFT) {
                Spin_Left(CRAWL);
            } else {
                Spin_Right(CRAWL);
            }
        }
        break;

    case BL_TURN_WAIT:
        Motors_Off();
        
        if (drive_timer >= STATE_PAUSE_TIME) {
            bl_state = BL_TRAVEL;
            drive_timer = 0;
            LastDir = GOING_STRAIGHT;  // Reset direction tracking
            
            strcpy(display_line[0], " BL Travel");
            display_changed = 1;
        }
        break;

    //==========================================================================
    // BL_TRAVEL: Follow the line to the circle
    //==========================================================================
    case BL_TRAVEL:
        Line_Follow_Update();
        
        if (drive_timer >= TRAVEL_TO_CIRCLE_TIME) {
            Motors_Off();
            bl_state = BL_TRAVEL_WAIT;
            drive_timer = 0;
        }
        break;

    case BL_TRAVEL_WAIT:
        Motors_Off();
        
        if (drive_timer >= STATE_PAUSE_TIME) {
            bl_state = BL_CIRCLE;
            drive_timer = 0;
            LastDir = GOING_STRAIGHT;  // Reset for circle
            
            strcpy(display_line[0], " BL Circle");
            display_changed = 1;
        }
        break;

    //==========================================================================
    // BL_CIRCLE: Follow the circle, wait for exit command
    //==========================================================================
    case BL_CIRCLE:
        Line_Follow_Update();
        
        // Exit when command received and minimum time elapsed
        if (exit_command_received && (drive_timer >= MIN_CIRCLE_TIME)) {
            Motors_Off();
            bl_state = BL_CIRCLE_WAIT;
            drive_timer = 0;
        }
        break;

    case BL_CIRCLE_WAIT:
        Motors_Off();
        
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
        Drive_Straight(SLOW);
        
        if (drive_timer >= EXIT_DRIVE_TIME) {
            Motors_Off();
            bl_state = BL_STOP;
            drive_timer = 0;
        }
        break;

    //==========================================================================
    // BL_STOP: Finished
    //==========================================================================
    case BL_STOP:
        Motors_Off();
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

    Motors_Off();
    DAC_Set_Voltage(DAC_MOTOR_OFF);

    strcpy(display_line[0], " STOPPED  ");
    display_changed = TRUE;
    
    command_enabled = TRUE;
    TB1CCTL1 &= ~CCIE;
}
