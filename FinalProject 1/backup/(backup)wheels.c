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


// unsigned int current_dac_speed = DAC_MOTOR_MEDIUM;



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
    int left_drift = ADC_Left_Detect - LEFT_THRESHOLD;
    int right_drift = ADC_Right_Detect - RIGHT_THRESHOLD;

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
        DAC_Set_Voltage(DAC_MOTOR_MEDIUM);

        int pwm_correction = (drift_error * Kp);                        // Calculate proportional adjustment based on drift

        if (drift_error > 0) {                                          // Car drifting RIGHT - correct by turning LEFT
            LEFT_FORWARD_SPEED = WHEEL_PERIOD;
            RIGHT_FORWARD_SPEED = WHEEL_PERIOD - pwm_correction;        // Slow down RIGHT wheel

            if (RIGHT_FORWARD_SPEED < (WHEEL_PERIOD / 2)) {             // Clamp minimum speed
                RIGHT_FORWARD_SPEED = WHEEL_PERIOD / 2;
            }
            LEFT_REVERSE_SPEED = WHEEL_OFF;
            RIGHT_REVERSE_SPEED = WHEEL_OFF;
        }
        else {                                                          // Car drifting LEFT - correct by turning RIGHT
            LEFT_FORWARD_SPEED = WHEEL_PERIOD + pwm_correction;         // Slow down LEFT wheel
            RIGHT_FORWARD_SPEED = WHEEL_PERIOD;                         // pwm_correction is negative

            if (LEFT_FORWARD_SPEED < (WHEEL_PERIOD / 2)) {              // Clamp minimum speed
                LEFT_FORWARD_SPEED = WHEEL_PERIOD / 2;
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

    unsigned char left_on_line = (ADC_Left_Detect > LEFT_THRESHOLD);            // Greater than threshold == On Line 
    unsigned char right_on_line = (ADC_Right_Detect > RIGHT_THRESHOLD);         // (less than thresh is off or drifting)

    switch (drive_state) 
    {
    case DRIVE_START:
        if (left_on_line || right_on_line) {
            // Line detected - apply 50ms reverse pulse to quickly stop
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
        
        if (left_on_line && right_on_line) {
            // CENTERED: Both sensors on line - car is parallel or nearly parallel
            // Wait briefly to stabilize, then proceed to intercept with brake delay
            if (drive_timer >= DRIVE_DELAY_MS(500)) {
                Wheels_Safe_Stop();
                next_drive_state = DRIVE_INTERCEPT;
                drive_state = DRIVE_BRAKE_DELAY;
                drive_timer = 0;
                strcpy(display_line[0], "Brake ... ");
                display_changed = TRUE;
            }
            else {
                PWM_FORWARD();                                          // Gentle forward if not yet stabilized
            }
        }
        else if (left_on_line && !right_on_line) {
            // LEFT sensor on, RIGHT off - car front is angled RIGHT
            // Rotate LEFT to align (left wheel reverse, right wheel forward)
            PWM_ALIGN_LEFT();
            
            if (drive_timer >= DRIVE_DELAY_MS(2000)) {                  // Timeout after 2 seconds
                Wheels_Safe_Stop();
                next_drive_state = DRIVE_INTERCEPT;
                drive_state = DRIVE_BRAKE_DELAY;
                drive_timer = 0;
                strcpy(display_line[0], "Brake ... ");
                display_changed = TRUE;
            }
        }
        else if (!left_on_line && right_on_line) {
            // RIGHT sensor on, LEFT off - car front is angled LEFT
            // Rotate RIGHT to align (left wheel forward, right wheel reverse)
            PWM_ALIGN_RIGHT();
            
            if (drive_timer >= DRIVE_DELAY_MS(2000)) {                  // Timeout after 2 seconds
                Wheels_Safe_Stop();
                next_drive_state = DRIVE_INTERCEPT;
                drive_state = DRIVE_BRAKE_DELAY;
                drive_timer = 0;
                strcpy(display_line[0], "Brake ... ");
                display_changed = TRUE;
            }
        }
        else {
            // BOTH sensors OFF - car is completely off the line
            // This shouldn't happen right after intercept, but if it does, pause and retry
            if (drive_timer >= DRIVE_DELAY_MS(1000)) {
                // Timeout - move to intercept anyway
                Wheels_Safe_Stop();
                next_drive_state = DRIVE_INTERCEPT;
                drive_state = DRIVE_BRAKE_DELAY;
                drive_timer = 0;
                strcpy(display_line[0], "Brake ... ");
                display_changed = TRUE;
            }
        }
        break;

    case DRIVE_BRAKE_DELAY:
        // Generic 500ms brake delay between state transitions
        // Ensures motors fully stop and settle before changing direction
        Wheels_Safe_Stop();
        
        if (drive_timer >= DRIVE_DELAY_MS(500)) {
            // 500ms brake delay complete - transition to next state
            drive_state = next_drive_state;
            drive_timer = 0;
            
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

    case DRIVE_INTERCEPT:
        // Keep motors running forward during intercept wait
        PWM_FORWARD();
        
        if (drive_timer >= DRIVE_DELAY_MS(15000)) {                                   // 15 seconds
            drive_state = DRIVE_TURN;
            drive_timer = 0;

            strcpy(display_line[0], "BL Turn   ");
            display_changed = TRUE;

            DAC_Set_Voltage(DAC_MOTOR_SLOW);                                    // Start turn onto line
        }
        break;

    case DRIVE_TURN:
        if (left_on_line && !right_on_line) {
            PWM_TURN_LEFT();                                                    // Left sensor on, turn left
        }
        else if(!left_on_line && right_on_line) {
            PWM_TURN_RIGHT();                                                   // Right sensor on, turn right
        }
        else if(left_on_line && right_on_line) {
            // Centered on line - apply brake delay before travel
            Wheels_Safe_Stop();
            next_drive_state = DRIVE_TRAVEL;
            drive_state = DRIVE_BRAKE_DELAY;
            drive_timer = 0;
            drive_pause_complete = FALSE;
            strcpy(display_line[0], "Brake ... ");
            display_changed = TRUE;
        }
        break;

    case DRIVE_TRAVEL:
        if (!drive_pause_complete) {
            if (drive_timer >= DRIVE_DELAY_MS(15000)) {
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




//void IR_Calibrate_Process(void) {
//    /*
//     * Called from MAIN LOOP when process_calibration flag set
//     * Timer ISR sets flag every 50ms
//     */
//    if (drive_state != LF_CALIBRATING) return;
//
//    switch (calib_state)
//    {
//    case CALIB_WAIT_BLACK:
//        if (SW1_pressed) {
//            SW1_pressed = FALSE;
//            calib_state = CALIB_SAMPLING_BLACK;
//            calib_sample_count = 0;
//            calib_left_sum = 0;
//            calib_right_sum = 0;
//        }
//        break;
//
//    case CALIB_SAMPLING_BLACK:                                  // Accumulate ADC values (average of values used to calibrate)
//        calib_left_sum += ADC_Left_Detect;
//        calib_right_sum += ADC_Right_Detect;
//        calib_sample_count++;
//
//        if (calib_sample_count >= SAMPLES) {                    // Calculate averages
//
//            LEFT_BLACK_VALUE = calib_left_sum / SAMPLES;
//            RIGHT_BLACK_VALUE = calib_right_sum / SAMPLES;
//
//            strcpy(display_line[0], "BLACK Val ");              // Display using HEXtoBCD
//            strcpy(display_line[1], "L:        ");
//            HEXtoBCD(LEFT_BLACK_VALUE);
//            adc_line(2, 3);
//
//            strcpy(display_line[2], "R:        ");
//            HEXtoBCD(RIGHT_BLACK_VALUE);
//            adc_line(3, 3);
//
//            strcpy(display_line[3], "Next:WHITE");
//            display_changed = TRUE;
//
//            calib_state = CALIB_DISPLAY_BLACK;
//            calib_timer = 0;
//        }
//        break;
//
//    case CALIB_DISPLAY_BLACK:
//        calib_timer++;
//        if (calib_timer >= DELAY_MS(2000)) {
//            strcpy(display_line[1], "Place on  ");
//            strcpy(display_line[2], "WHITE spot");
//            strcpy(display_line[3], "Press SW2 ");
//            display_changed = TRUE;
//            calib_state = CALIB_WAIT_WHITE;
//        }
//        break;
//
//    case CALIB_WAIT_WHITE:
//        if (SW2_pressed) {
//            SW2_pressed = FALSE;
//            calib_state = CALIB_SAMPLING_WHITE;
//            calib_sample_count = 0;
//            calib_left_sum = 0;
//            calib_right_sum = 0;
//        }
//        break;
//
//    case CALIB_SAMPLING_WHITE:
//        calib_left_sum += ADC_Left_Detect;
//        calib_right_sum += ADC_Right_Detect;
//        calib_sample_count++;
//
//        if (calib_sample_count >= 10) {                             // Calculate averages and thresholds
//
//            LEFT_WHITE_VALUE = calib_left_sum / 10;
//            RIGHT_WHITE_VALUE = calib_right_sum / 10;
//            LEFT_THRESHOLD = (LEFT_BLACK_VALUE + LEFT_WHITE_VALUE) / 2;
//            RIGHT_THRESHOLD = (RIGHT_BLACK_VALUE + RIGHT_WHITE_VALUE) / 2;
//
//            // Display white values
//            strcpy(display_line[0], "WHITE Val ");
//            strcpy(display_line[1], "L:        ");
//            HEXtoBCD(LEFT_WHITE_VALUE);
//            adc_line(2, 3);
//
//            strcpy(display_line[2], "R:        ");
//            HEXtoBCD(RIGHT_WHITE_VALUE);
//            adc_line(3, 3);
//
//            strcpy(display_line[3], "Complete! ");
//            display_changed = TRUE;
//
//            calib_state = CALIB_DISPLAY_WHITE;
//            calib_timer = 0;
//        }
//        break;

//    case CALIB_DISPLAY_WHITE:
//        calib_timer++;
//        if (calib_timer >= DELAY_MS(2000)) {
//            // Show thresholds
//            strcpy(display_line[0], "Threshold ");
//            strcpy(display_line[1], "L:        ");
//            HEXtoBCD(LEFT_THRESHOLD);
//            adc_line(2, 3);
//
//            strcpy(display_line[2], "R:        ");
//            HEXtoBCD(RIGHT_THRESHOLD);
//            adc_line(3, 3);
//
//            display_changed = TRUE;
//
//            calib_state = CALIB_DISPLAY_THRESH;
//            calib_timer = 0;
//        }
//        break;
//
//    case CALIB_DISPLAY_THRESH:
//        calib_timer++;
//        if (calib_timer >= DELAY_MS(2000)) {
//            calibration_complete = TRUE;
//            drive_state = LF_WAITING;
//
//            strcpy(display_line[0], "Waiting   ");
//            strcpy(display_line[1], "for input ");
//            display_changed = TRUE;
//        }
//        break;
//    }
//}


//void Line_Follow_Start_Autonomous(void)             // Entry point to Line follow
//{                                                   // Adaption for Wi-Fi initiation
//    if (!calibration_complete) {
//        return;                                     // Must calibrate first
//    }
//
//    drive_state = LF_BL_START;
//    state_timer = 0;
//    pause_complete = FALSE;
//    circle_lap_count = 0;
//
//    strcpy(display_line[0], "BL Start  ");          // Display "BL Start"
//
//    display_changed = TRUE;
//
//    DAC_Set_Voltage(DAC_MOTOR_MEDIUM);              // Start driving toward line
//    LEFT_FORWARD_SPEED = WHEEL_PERIOD;
//    RIGHT_FORWARD_SPEED = WHEEL_PERIOD;
//    LEFT_REVERSE_SPEED = WHEEL_OFF;
//    RIGHT_REVERSE_SPEED = WHEEL_OFF;
//}


//void Line_Follow_Process(void) {
//    /*
//     * Called from MAIN LOOP when process_line_follow flag set
//     * Implements Project 10 autonomous line following
//     */
//
//    unsigned char left_on_line = (ADC_Left_Detect < LEFT_THRESHOLD);
//    unsigned char right_on_line = (ADC_Right_Detect < RIGHT_THRESHOLD);
//
//    switch (drive_state) {
//
//        // BL START - Driving to black line
//        //======================================================================
//    case LF_BL_START:
//        if (left_on_line || right_on_line)         // Check if line detected
//        {                                          // Line found! Stop for 10-20 second
//            LEFT_FORWARD_SPEED = WHEEL_OFF;
//            RIGHT_FORWARD_SPEED = WHEEL_OFF;
//
//            drive_state = LF_INTERCEPT;
//            state_timer = 0;
//            pause_complete = FALSE;
//
//            strcpy(display_line[0], "Intercept ");
//            display_changed = TRUE;
//        }
//        break;
//
//        // INTERCEPT - Stopped at line, waiting 10-20 seconds
//        //======================================================================
//    case LF_INTERCEPT:
//        state_timer++;
//        if (state_timer >= DELAY_MS(15000)) {           // 15s pause
//            pause_complete = TRUE;
//            drive_state = LF_BL_TURN;
//            state_timer = 0;
//
//            strcpy(display_line[0], "BL Turn   ");
//            display_changed = TRUE;
//
//            DAC_Set_Voltage(DAC_MOTOR_SLOW);            // Start turn onto line
//
//            // Determine turn direction and execute
//            // (Implementation depends on which sensor detected line)
//        }
//        break;
//
//        //======================================================================
//        // BL TURN - Turning onto line
//        //======================================================================
//    case LF_BL_TURN:
//        // Simple turn logic - turn until centered
//        if (left_on_line && right_on_line) {
//            // Centered! Stop for 10-20 seconds
//            LEFT_FORWARD_SPEED = WHEEL_OFF;
//            RIGHT_FORWARD_SPEED = WHEEL_OFF;
//
//            drive_state = LF_BL_TRAVEL;
//            state_timer = 0;
//            pause_complete = FALSE;
//
//            strcpy(display_line[0], "BL Travel ");
//            display_changed = TRUE;
//        }
//        break;
//
//        //======================================================================
//        // BL TRAVEL - Following line to circle
//        //======================================================================
//    case LF_BL_TRAVEL:
//        if (!pause_complete) {
//            // Pausing for 10-20 seconds
//            state_timer++;
//            if (state_timer >= DELAY_MS(15000)) {
//                pause_complete = TRUE;
//                state_timer = 0;
//            }
//        }
//        else {
//            Line_Follow_Control();            // Actually following line
//
//
//            // Detect circle entry (both sensors on for extended time)
//            // Transition to LF_BL_CIRCLE when detected
//        }
//        break;
//
//        //======================================================================
//        // BL CIRCLE - Circling (must complete 2x)
//        //======================================================================
//    case LF_BL_CIRCLE:
//        if (!pause_complete) {
//            // Initial pause
//            LEFT_FORWARD_SPEED = WHEEL_OFF;
//            RIGHT_FORWARD_SPEED = WHEEL_OFF;
//
//            state_timer++;
//            if (state_timer >= DELAY_MS(15000)) {
//                pause_complete = TRUE;
//                strcpy(display_line[0], "BL Circle ");
//                display_changed = TRUE;
//            }
//        }
//        else {
//            // Follow circle
//            Line_Follow_Control();
//
//            // Count laps (detect crossing point)
//            // When circle_lap_count >= 2, wait for exit command
//        }
//        break;
//
//        //======================================================================
//        // BL EXIT - Exiting circle (after command received)
//        //======================================================================
//    case LF_BL_EXIT:
//        strcpy(display_line[0], "BL Exit   ");
//        display_changed = TRUE;
//
//        // Drive straight away from circle
//        DAC_Set_Voltage(DAC_MOTOR_MEDIUM);
//        LEFT_FORWARD_SPEED = WHEEL_PERIOD;
//        RIGHT_FORWARD_SPEED = WHEEL_PERIOD;
//
//        state_timer++;
//        if (state_timer >= DELAY_MS(3000)) {  // Drive 3 seconds
//            drive_state = LF_BL_STOP;
//        }
//        break;
//
//        //======================================================================
//        // BL STOP - Stopped, course complete
//        //======================================================================
//    case LF_BL_STOP:
//        LEFT_FORWARD_SPEED = WHEEL_OFF;
//        RIGHT_FORWARD_SPEED = WHEEL_OFF;
//        DAC_Set_Voltage(DAC_MOTOR_STOP);
//
//        strcpy(display_line[0], "BL Stop   ");
//        strcpy(display_line[1], "Course    ");
//        strcpy(display_line[2], "Complete! ");
//        display_changed = TRUE;
//        break;
//
//    default:
//        break;
//    }
//}
//
//
//void Line_Follow_Control(void) {
//    /*
//     * Actual line following control logic
//     * DAC-based speed, PWM fine-tuning, motor off for sharp turns
//     */
//    unsigned char left_on_line = (ADC_Left_Detect < LEFT_THRESHOLD);
//    unsigned char right_on_line = (ADC_Right_Detect < RIGHT_THRESHOLD);
//
//    int left_error = abs(ADC_Left_Detect - LEFT_THRESHOLD);
//    int right_error = abs(ADC_Right_Detect - RIGHT_THRESHOLD);
//
//    if (left_on_line && right_on_line) {
//        // CENTERED
//        DAC_Set_Voltage(DAC_MOTOR_FAST);
//        LEFT_FORWARD_SPEED = WHEEL_PERIOD;
//        RIGHT_FORWARD_SPEED = WHEEL_PERIOD;
//
//    }
//    else if (left_on_line && !right_on_line) {
//        // Turn LEFT
//        if (right_error < 500) {
//            // Gentle
//            DAC_Set_Voltage(DAC_MOTOR_MEDIUM);
//            LEFT_FORWARD_SPEED = WHEEL_PERIOD;
//            RIGHT_FORWARD_SPEED = (WHEEL_PERIOD * 85) / 100;
//        }
//        else {
//            // Sharp
//            DAC_Set_Voltage(DAC_MOTOR_SLOW);
//            LEFT_FORWARD_SPEED = WHEEL_PERIOD;
//            RIGHT_FORWARD_SPEED = WHEEL_OFF;
//        }
//
//    }
//    else if (!left_on_line && right_on_line) {
//        // Turn RIGHT
//        if (left_error < 500) {
//            DAC_Set_Voltage(DAC_MOTOR_MEDIUM);
//            LEFT_FORWARD_SPEED = (WHEEL_PERIOD * 85) / 100;
//            RIGHT_FORWARD_SPEED = WHEEL_PERIOD;
//        }
//        else {
//            DAC_Set_Voltage(DAC_MOTOR_SLOW);
//            LEFT_FORWARD_SPEED = WHEEL_OFF;
//            RIGHT_FORWARD_SPEED = WHEEL_PERIOD;
//        }
//    }
//}
//
//void Line_Follow_Exit_Circle(void) {
//    /*
//     * Called when exit command received from WiFi
//     * Project 10: Single command to exit after 2 laps
//     */
//    drive_state = LF_BL_EXIT;
//    state_timer = 0;
//}
//
//unsigned char Get_LF_State(void) {
//    return drive_state;
//}



//----------------------------------------------------
//  IR Sensor Calibration Routine
//  Instructions for user:
//      1. Place car centered over BLACK line
//      2. Press SW1 to capture black values
//      3. Place car centered over WHITE surface
//      4. Press SW2 to capture white values
//      5. Calibration complete - thresholds calculated
//-----------------------------------------------------
//void IR_Calibrate(void) {
//        
//    unsigned char state = 0;  // 0 = waiting for black, 1 = waiting for white
//
//    strcpy(display_line[0], "IR CALIBRT");
//    strcpy(display_line[1], "Place on  ");
//    strcpy(display_line[2], "BLACK line");
//    strcpy(display_line[3], "Press SW1 ");
//    display_changed = TRUE;
//
//    while (!calibration_complete) {
//
//        // State 0: Capture BLACK values
//        if (state == 0 && SW1_pressed) {
//            SW1_pressed = FALSE;
//
//            // Sample sensors multiple times for stability
//            unsigned long left_sum = 0;
//            unsigned long right_sum = 0;
//            unsigned char count = 0;
//
//
//            while(count < SAMPLES){
//				if (!sample_adc) {
//                    sample_adc = TRUE;
//					left_sum += ADC_Left_Detect;
//					right_sum += ADC_Right_Detect;
//					count++;
//				}
//            }
//
//            LEFT_BLACK_VALUE = left_sum / SAMPLES;
//            RIGHT_BLACK_VALUE = right_sum / SAMPLES;
//
//            // Display captured values
//            strcpy(display_line[0], "BLACK Val ");
//            strcpy(display_line[1], "L:        ");
//            HEXtoBCD(LEFT_BLACK_VALUE);
//            adc_line(2, 3);
//
//            strcpy(display_line[2], "R:        ");
//            HEXtoBCD(RIGHT_BLACK_VALUE);
//            adc_line(3, 3);
//
//            //strcpy(display_line[3], "Next:WHITE");
//            //display_changed = TRUE;
//
//            strcpy(display_line[1], "Place on  ");
//            strcpy(display_line[2], "WHITE spot");
//            strcpy(display_line[3], "Press SW2 ");
//            display_changed = TRUE;
//
//            state = 1;
//        }
//
//        // State 1: Capture WHITE values
//        if (state == 1 && SW2_pressed) {
//            SW2_pressed = FALSE;
//
//            // Sample sensors multiple times
//            unsigned long left_sum = 0;
//            unsigned long right_sum = 0;
//            unsigned char count = 0;
//
//            while (count < SAMPLES) {
//                if (!sample_adc) {
//                    sample_adc = TRUE;
//                }
//                if (!sample_adc) {
//                    left_sum += ADC_Left_Detect;
//                    right_sum += ADC_Right_Detect;
//                    count++;
//                }
//            }
//
//            LEFT_WHITE_VALUE = left_sum / SAMPLES;
//            RIGHT_WHITE_VALUE = right_sum / SAMPLES;
//
//            // Calculate thresholds (midpoint between black and white)
//            LEFT_THRESHOLD = (LEFT_BLACK_VALUE + LEFT_WHITE_VALUE) / 2;
//            RIGHT_THRESHOLD = (RIGHT_BLACK_VALUE + RIGHT_WHITE_VALUE) / 2;
//
//            // Display results
//            strcpy(display_line[0], "WHITE Val ");
//            strcpy(display_line[1], "L:        ");
//            HEXtoBCD(LEFT_WHITE_VALUE);
//            adc_line(2, 3);
//
//            strcpy(display_line[2], "R:        ");
//            HEXtoBCD(RIGHT_WHITE_VALUE);
//            adc_line(3, 3);
//
//            strcpy(display_line[3], "Complete! ");
//            display_changed = TRUE;
//
//            unsigned int display_timer = 0;
//            while (display_timer < DELAY_MS(2000)) {
//                Display_Process();
//                display_timer++;
//            }
//
//            // Show thresholds
//            strcpy(display_line[0], "Threshold ");
//            strcpy(display_line[1], "L:        ");
//            HEXtoBCD(LEFT_THRESHOLD);
//            adc_line(2, 3);
// 
//            strcpy(display_line[2], "R:        ");
//            HEXtoBCD(RIGHT_THRESHOLD);
//            adc_line(3, 3);
//            display_changed = TRUE;
//
//            calibration_complete = TRUE;
//        }
//        Display_Process();
//    }
//}


//void Line_Follow_Proportional(void) {
//    /*
//     * Proportional controller based on sensor error
//	 * see: https://x-engineer.org/proportional-controller/ for tuning and walkthrough
//	 *        
//     *         u(t) = Kp * e(t)
//	 *       Output = Kp * Total Error
//     * 
//	 *       Error = Sensor - Threshold              -->  Greater error as sensor moves away from line
//	 *       Total Error = Left Error - Right Error  -->  Positive = drifting right, Negative = drifting left
//     *          
//     *       Kp = Proportional gain constant         -->  Controls response speed to error
//	 *              - Higher Kp = faster response 
//	 *              - Lower Kp = slower response
//     * 
//     *       Output: 
//     */
//
//	if (drive_state != RUNNING) { return; }                     // Return immediatlly if not running
//     // Calculate error (how far from center)
//    int left_error = ADC_Left_Detect - LEFT_THRESHOLD;
//    int right_error = ADC_Right_Detect - RIGHT_THRESHOLD;
//
//    // Combined error (positive = drifting right, negative = drifting left)
//    int total_error = left_error - right_error;
//
//    // Proportional gain
//#define Kp  (0.05)  // Tune this value
//
//// Calculate steering correction
//    int steering = total_error * Kp;
//
//    // Clamp steering to reasonable range
//    if (steering > 50) steering = 50;
//    if (steering < -50) steering = -50;
//
//    // Apply steering
//    if (steering > 0) {
//        // Turn right
//        Set_Base_Speed(SPEED_MEDIUM);
//        Set_Motor_PWM(100, 100 - steering);
//    }
//    else {
//        // Turn left
//        Set_Base_Speed(SPEED_MEDIUM);
//        Set_Motor_PWM(100 + steering, 100);
//    }
//}
