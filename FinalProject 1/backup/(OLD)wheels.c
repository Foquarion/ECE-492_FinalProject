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
#include  "timers.h"
#include  "LED.h"



typedef enum {
    DRIVE_IDLE,             // Not running
    DRIVE_CALIBRATING,      // Calibrating sensors
    DRIVE_START,            // Driving to black line
    DRIVE_BRAKE_RECOVER,    // 500ms delay after intercept brake pulse
    DRIVE_ALIGN,            // Aligning perpendicular intercept to line
    DRIVE_BRAKE_DELAY,      // 500ms brake delay between state transitions
    DRIVE_INTERCEPT,        // Stopped at black line, waiting 15 seconds
    DRIVE_TURN,             // Turning onto line
    DRIVE_TRAVEL,           // Traveling along line
    DRIVE_CIRCLE,           // Circling
    DRIVE_EXIT,             // Exiting circle
    DRIVE_STOP              // Stopped - complete
} drive_state_t;

drive_state_t drive_state = DRIVE_IDLE;


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
unsigned char drive_pause_complete = FALSE;         // 10-20 second pause flag
unsigned int circle_lap_count = 0;                  // Circle lap counter
drive_state_t next_drive_state = DRIVE_IDLE;       // State to transition to after brake delay

// GLOBALS
extern char display_line[4][11];
extern volatile unsigned char display_changed;

// ISR FLAGS
volatile unsigned char process_line_follow = FALSE;
volatile unsigned char update_state_timer = FALSE;
extern unsigned char display_menu;

// PROPORTIONAL CONTROLLER CONSTANTS

// DRIFT VARIABLES
unsigned int left_error = 0;
unsigned int right_error = 0;
int drift_error = 0;


// unsigned int current_dac_speed = DAC_MOTOR_MEDIUM;

#define CAR_Left_Detect     (ADC_Right_Detect)
#define CAR_Right_Detect    (ADC_Left_Detect)




void Line_Follow_Start_Autonomous(void)                         // Entry point for autonomous line following
{                                                               // TRIGGERED BY: IoT command "AUTONOMOUS" or SW2 press
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

    DAC_Set_Voltage(DAC_MOTOR_MEDIUM);                          // Start driving toward line
    PWM_FORWARD();
    TB1CCTL1 &= ~CCIFG;                     // Clear possible pending interrupt
    TB1CCTL1 |=  CCIE;                      // CCR1 enable interrupt
    TB1CCR1 = TB1R + TB1CCR1_INTERVAL;

}


void Line_Follow_Process(void)                                                  // Called from main loop when process_line_follow flag set by ISR
{                                                                               // drive_timer increments each call(100ms intervals)
	if (!process_line_follow) return;

    if (drive_state == DRIVE_IDLE) {
        return;
    }

    unsigned char left_on_line = (CAR_Left_Detect > LEFT_THRESHOLD);            // Greater than threshold == On Line
    unsigned char right_on_line = (CAR_Right_Detect > RIGHT_THRESHOLD);         // (less than thresh is off or drifting)
    unsigned char left_first = FALSE;
    unsigned char right_first = FALSE;

    switch (drive_state) 
    {
    case DRIVE_START:
        if (left_on_line || right_on_line) {
            if(left_on_line) {
                left_first = TRUE;
            }
            if(right_on_line) {
                right_first = TRUE;
            }
            // Line detected - 50ms reverse pulse to ebrake
            Wheels_Intercept_Brake();

            drive_state = DRIVE_BRAKE_RECOVER;                          // Wait 500ms for motor recovery
            drive_timer = 0;

            strcpy(display_line[0], "Brake     ");
            display_changed = TRUE;
        }
        break;

    case DRIVE_BRAKE_RECOVER:
        // After 50ms intercept brake pulse, wait 500ms total for motor recovery
        // Motors are already stopped by Wheels_Intercept_Brake()
        if (drive_timer >= DRIVE_DELAY_MS(500)) {
            // 500ms safety delay elapsed - safe to proceed with alignment
            drive_state = DRIVE_ALIGN;
            drive_timer = 0;
            strcpy(display_line[0], "Align     ");
            display_changed = TRUE;
        }
        break;

    case DRIVE_ALIGN:
        // After intercept brake, align the car to be parallel with the line
        // This handles perpendicular and angled approaches
        
        if(right_first){
            PWM_TURN_LEFT();
        }
        if(left_first){
            PWM_TURN_RIGHT();
        }
        if (drive_timer >= DRIVE_DELAY_MS(500)) {
            Wheels_Safe_Stop();
            next_drive_state = DRIVE_TRAVEL;
            drive_state = DRIVE_BRAKE_DELAY;
            drive_timer = 0;
            strcpy(display_line[0], "Brake ... ");
            display_changed = TRUE;
        }
        break;

        // case DRIVE_ALIGN: // alt SIDE SPECIFIC version
        // PWM_TURN_LEFT();
        // if (drive_timer >= DRIVE_DELAY_MS(500)) {
        //     Wheels_Safe_Stop();
        //     next_drive_state = DRIVE_INTERCEPT;
        //     drive_state = DRIVE_BRAKE_DELAY;
        //     drive_timer = 0;
        //     strcpy(display_line[0], "Brake ... ");
        //     display_changed = TRUE;
        // }
        // break;


    case DRIVE_BRAKE_DELAY:
        // Generic 500ms brake delay between state transitions
        // Ensures motors fully stop and settle before changing direction
        // Useful for display updating as well
        Wheels_Safe_Stop();
        
        if (drive_timer >= DRIVE_DELAY_MS(500)) {
            // 500ms brake delay complete - transition to next state
            drive_state = next_drive_state;
            drive_timer = 0;
            drive_pause_complete = FALSE;
            
            if (next_drive_state == DRIVE_INTERCEPT) {
                strcpy(display_line[0], "Intercept ");
            }
            else if (next_drive_state == DRIVE_TRAVEL) {
                strcpy(display_line[0], "BL Travel ");
            }
            else if (next_drive_state == DRIVE_EXIT) {
                strcpy(display_line[0], "BL Exit   ");
            }
            display_changed = TRUE;
        }
        break;

    // case DRIVE_INTERCEPT:
    //     // Keep motors running forward during intercept wait
    //     PWM_FORWARD();
        
    //     if (drive_timer >= DRIVE_DELAY_MS(15000)) {                                   // 15 seconds
    //         drive_state = DRIVE_TURN;
    //         drive_timer = 0;

    //         strcpy(display_line[0], "BL Turn   ");
    //         display_changed = TRUE;

    //         DAC_Set_Voltage(DAC_MOTOR_SLOW);                                    // Start turn onto line
    //     }
    //     break;

    // case DRIVE_TURN:
    //     if (left_on_line && !right_on_line) {
    //         PWM_TURN_LEFT();                                                    // Left sensor on, turn left
    //     }
    //     else if(!left_on_line && right_on_line) {
    //         PWM_TURN_RIGHT();                                                   // Right sensor on, turn right
    //     }
    //     else if(left_on_line && right_on_line) {
    //         // Centered on line - apply brake delay before travel
    //         Wheels_Safe_Stop();
    //         next_drive_state = DRIVE_TRAVEL;
    //         drive_state = DRIVE_BRAKE_DELAY;
    //         drive_timer = 0;
    //         drive_pause_complete = FALSE;
    //         strcpy(display_line[0], "Brake ... ");
    //         display_changed = TRUE;
    //     }
    //     break;

    case DRIVE_TRAVEL:
        if (!drive_pause_complete) {
            if (drive_timer >= DRIVE_DELAY_MS(2000)) {
                drive_pause_complete = TRUE;
                drive_timer = 0;
            }
        }
        else {
            Line_Follow_Proportional();                                     // Use ADC based proportional control
        }
        break;

    case DRIVE_CIRCLE:
        if (!drive_pause_complete) {
            PWM_FULLSTOP();

            if (drive_timer >= DRIVE_DELAY_MS(15000)) {
                drive_pause_complete = TRUE;
                strcpy(display_line[0], "BL Circle ");
                display_changed = TRUE;
            }
        }
        else {
            Line_Follow_Proportional();                                     // Use ADC based proportional control
        }
        break;

    case DRIVE_EXIT:
        strcpy(display_line[0], "BL Exit   ");
        display_changed = 1;

        DAC_Set_Voltage(DAC_MOTOR_MEDIUM);
        PWM_FORWARD();

        if (drive_timer >= DRIVE_DELAY_MS(3000)) {                          // 3 seconds
            drive_state = DRIVE_STOP;
        }
        break;

    case DRIVE_STOP:
        Wheels_Safe_Stop();                                             // Safely stop all motors
        DAC_Set_Voltage(DAC_MOTOR_OFF);

        strcpy(display_line[0], "BL Stop   ");
        strcpy(display_line[1], "Course    ");
        strcpy(display_line[2], "Complete! ");
        display_changed = TRUE;

        process_line_follow = FALSE;

        TB1CCTL1 &= ~CCIFG;                     // Clear possible pending interrupt
        TB1CCTL1 &= ~CCIE;                      // CCR1 disable interrupt

        display_menu = TRUE;
        break;

    default:
        break;

    }
}


void Line_Follow_Exit_Circle(void)                       // Call via IoT to leave circle
{   
    drive_state = DRIVE_EXIT;
    drive_timer = 0;
}


void Line_Follow_Stop(void)                             // E-Stop line following
{
    process_line_follow = FALSE;
    drive_state = DRIVE_IDLE;
    Wheels_Safe_Stop();                                 // Safely stop all motors
    DAC_Set_Voltage(DAC_MOTOR_OFF);

    strcpy(display_line[0], "STOPPED   ");
    display_changed = TRUE;
}



void Line_Follow_Proportional(void) {
    /*
     * Proportional controller based on sensor error
     * see: https://x-engineer.org/proportional-controller/ for tuning and walkthrough
     *
     *         u(t) = Kp * e(t)
     *       Output = Kp * Total Error
     *
     *       Error = Sensor - Threshold              -->  Greater error as sensor moves away from line
     *       Total Error = Left Error - Right Error  -->  Positive = drifting right, Negative = drifting left
     *
     *       Kp = Proportional gain constant         -->  Controls response speed to error
     *              - Higher Kp = faster response
     *              - Lower Kp = slower response
     *
     *       Output:
     */


     // Determine difference from threshold (positive = toward black, negative = toward white)
    int left_drift = CAR_Left_Detect - LEFT_THRESHOLD;
    int right_drift = CAR_Right_Detect - RIGHT_THRESHOLD;

    // Drift difference: determines drift direction
    // Positive = right sensor drifting MORE toward white = car drifting right,  correct with turn LEFT
    // Negative =  left sensor drifting MORE toward white = car drifting left,   correct with turn RIGHT
    int steering_error = right_error - left_error;

    // CENTERED - Both sensors near threshold, minimal drift
    //==========================================================================
    if (abs(drift_error) < DEADZONE) {
        // Both sensors seeing similar values, well centered
        DAC_Set_Voltage(DAC_MOTOR_MEDIUM);
        PWM_FORWARD();
    }

    // STARTING TO DRIFT - Proportional correction BEFORE leaving line
    //==========================================================================
    else if (abs(drift_error) < SHARP_TURN_THRESHOLD) {
        DAC_Set_Voltage(DAC_MOTOR_SLOW);

        int pwm_correction = (drift_error * Kp);                        // Calculate proportional adjustment based on drift

        if (drift_error > 0) {                                          // Car drifting RIGHT - correct by turning LEFT
            LEFT_FORWARD_SPEED = LEFT_FAST;;
            RIGHT_FORWARD_SPEED = RIGHT_FAST - pwm_correction;        // Slow down RIGHT wheel

            if (RIGHT_FORWARD_SPEED < (RIGHT_FAST / 2)) {             // Clamp minimum speed
                RIGHT_FORWARD_SPEED = RIGHT_FAST / 2;
            }
            LEFT_REVERSE_SPEED = WHEEL_OFF;
            RIGHT_REVERSE_SPEED = WHEEL_OFF;
        }
        else {                                                          // Car drifting LEFT - correct by turning RIGHT
            LEFT_FORWARD_SPEED = LEFT_FAST + pwm_correction;         // Slow down LEFT wheel
            RIGHT_FORWARD_SPEED = RIGHT_FAST;                         // pwm_correction is negative

            if (LEFT_FORWARD_SPEED < (LEFT_FAST / 2)) {              // Clamp minimum speed
                LEFT_FORWARD_SPEED = LEFT_FAST / 2;
            }
            LEFT_REVERSE_SPEED = WHEEL_OFF;
            RIGHT_REVERSE_SPEED = WHEEL_OFF;
        }
    }

    // SIGNIFICANT DRIFT - Turn off outside wheel for sharp correction
    //==========================================================================
    else {
        DAC_Set_Voltage(DAC_MOTOR_SLOW);

        if (drift_error > 0) {                                          // Drifting RIGHT significantly - sharp turn LEFT
            PWM_TURN_LEFT();
        }
        else {                                                          // Drifting LEFT significantly - sharp turn RIGHT
            PWM_TURN_RIGHT();
        }
    }
}

