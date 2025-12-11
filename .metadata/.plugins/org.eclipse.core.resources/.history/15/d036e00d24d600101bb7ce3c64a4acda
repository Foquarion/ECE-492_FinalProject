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
#include "switches.h"
#include "PWM.h"



typedef enum {
    DRIVE_IDLE,             // Not running

    DRIVE_TURN_LEFT,
    DRIVE_TURN_RIGHT,

    DRIVE_START,            // Driving to black line
    DRIVE_BRAKE_RECOVER,    // 500ms delay after intercept brake pulse
    DRIVE_RAW_ALIGN,            // Aligning perpendicular intercept to line
    DRIVE_TUNE_ALIGN,       // Fine tuning alignment to line
    DRIVE_TRAVEL_DELAY,
    DRIVE_BRAKE_DELAY,      // 500ms brake delay between state transitions
    DRIVE_INTERCEPT,        // Stopped at black line, waiting 15 seconds
    DRIVE_TURN,             // Turning onto line
    DRIVE_TRAVEL,           // Traveling along line
    DRIVE_BACKUP,           // Backing up
    DRIVE_CIRCLE,           // Circling
    DRIVE_EXIT,             // Exiting circle
    DRIVE_STOP              // Stopped - complete
} drive_state_t;

drive_state_t drive_state = DRIVE_IDLE;


typedef enum {
	SETUP_INIT,
    DRIVE_PAST,
    ROTATE_LEFT,
	ROTATE_RIGHT,
    SETUP_COMPLETE,
	SETUP_STOP
} setup_state_t;

setup_state_t setup_state = SETUP_INIT;


//typedef enum {
//    CENTERED,                           // Both sensors on line
//    SLIGHT_LEFT,                        // Right sensor off, left on - turn left
//    SLIGHT_RIGHT,                       // Left sensor off, right on - turn right
//    SHARP_LEFT,                         // Way off right - sharp left
//    SHARP_RIGHT,                        // Way off left - sharp right
//    LINE_LOST                           // Both off - search
//} line_state_t;
//
//line_state_t line_state = CENTERED;
//line_state_t last_line_state = CENTERED;


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
// int drift_error = 0;

// LINE DETECTION TRACKING
unsigned char left_first = FALSE;   // Which sensor detected line first
unsigned char right_first = FALSE;

// unsigned int current_dac_speed = DAC_MOTOR_MEDIUM;
#define PRESENTATION_DELAY  (2000)
#define CAR_Left_Detect     (ADC_Right_Detect)
#define CAR_Right_Detect    (ADC_Left_Detect)

extern unsigned char command_enabled;
extern volatile unsigned char ebraking;
unsigned char first_turn = TRUE;
unsigned char circle_found = FALSE;
unsigned int circle_detect_timer = 0;
unsigned int in_potential_circle = FALSE;
char pad_exit_direction = '\0';

volatile unsigned int setup_timer = 0;
volatile unsigned char process_setup = FALSE;
unsigned char setup_direction = '\0';



void Line_Follow_Setup_LEFT(void) {
    setup_state = SETUP_INIT;
    process_setup = TRUE;
    command_enabled = FALSE;
    setup_timer = 0;
    setup_direction = 'L';

    DAC_Set_Voltage(DAC_MOTOR_SLOW);
    PWM_FORWARD();

    TB1CCTL1 &= ~CCIFG;
    TB1CCTL1 |= CCIE;
    TB1CCR1 = TB1R + TB1CCR1_INTERVAL;

    //strcpy(display_line[0], "Setup L   ");
    //display_changed = TRUE;
}

void Line_Follow_Setup_RIGHT(void) {
    setup_state = SETUP_INIT;
    process_setup = TRUE;
    command_enabled = FALSE;
    setup_timer = 0;
    setup_direction = 'R';

    DAC_Set_Voltage(DAC_MOTOR_SLOW);
    PWM_FORWARD();

    TB1CCTL1 &= ~CCIFG;
    TB1CCTL1 |= CCIE;
    TB1CCR1 = TB1R + TB1CCR1_INTERVAL;

    //strcpy(display_line[0], "Setup R   ");
    //display_changed = TRUE;
}

void Line_Follow_Setup_Process(void) {
    if (!process_setup) return;

    switch (setup_state) {
    case SETUP_INIT:
        // Going forward (already started in entry function)
        if (setup_timer >= DRIVE_DELAY_MS(3000)) {
            setup_timer = 0;
            setup_state = DRIVE_PAST;
        }
        break;

    case DRIVE_PAST:
        // Stop after forward
        Wheels_Safe_Stop();
        if (setup_timer >= DRIVE_DELAY_MS(500)) {
            setup_timer = 0;

            if (setup_direction == 'L') {
                PWM_ROTATE_LEFT();
                setup_state = ROTATE_LEFT;
            }
            else {
                PWM_ROTATE_RIGHT();
                setup_state = ROTATE_RIGHT;
            }
        }
        break;

    case ROTATE_LEFT:
        // Rotating left (already started)
        if (setup_timer >= DRIVE_DELAY_MS(300)) {
            setup_timer = 0;
            setup_state = SETUP_COMPLETE;
        }
        break;

    case ROTATE_RIGHT:
        // Rotating right (already started)
        if (setup_timer >= DRIVE_DELAY_MS(300)) {
            setup_timer = 0;
            setup_state = SETUP_COMPLETE;
        }
        break;

    case SETUP_COMPLETE:
        // Stop after rotation
        Wheels_Safe_Stop();
        process_setup = FALSE;
        command_enabled = TRUE;
        setup_state = SETUP_INIT;

        TB1CCTL1 &= ~CCIFG;
        TB1CCTL1 &= ~CCIE;

        strcpy(display_line[0], "Setup Done");
        display_changed = TRUE;
        break;

    default:
        setup_state = SETUP_INIT;
        break;
    }
}



void Line_Follow_Start_LEFT_TURN(void)  // Exit pad turning LEFT
{
    if (!calibration_complete) {
        strcpy(display_line[0], "ERROR:    ");
        strcpy(display_line[1], "Must      ");
        strcpy(display_line[2], "calibrate ");
        strcpy(display_line[3], "first!    ");
        display_changed = 1;
        return;
    }

    drive_state = DRIVE_TURN_LEFT;
    command_enabled = FALSE;
    process_line_follow = TRUE;
    drive_timer = 0;
    display_menu = FALSE;
    pad_exit_direction = 'L';

    Sample_ADC();
    DAC_Set_Voltage(DAC_MOTOR_SLOW);
    PWM_Opening_Curve_Left();  // Start turning left in arc

    TB1CCTL1 &= ~CCIFG;
    TB1CCTL1 |= CCIE;
    TB1CCR1 = TB1R + TB1CCR1_INTERVAL;
}

void Line_Follow_Start_RIGHT_TURN(void)  // Exit pad turning RIGHT
{
    if (!calibration_complete) {
        strcpy(display_line[0], "ERROR:    ");
        strcpy(display_line[1], "Must      ");
        strcpy(display_line[2], "calibrate ");
        strcpy(display_line[3], "first!    ");
        display_changed = 1;
        return;
    }

    drive_state = DRIVE_TURN_RIGHT;
    command_enabled = FALSE;
    process_line_follow = TRUE;
    drive_timer = 0;
    display_menu = FALSE;
    pad_exit_direction = 'R';

    Sample_ADC();
    DAC_Set_Voltage(DAC_MOTOR_SLOW);
    PWM_Opening_Curve_Right();  // Start turning right in arc

    TB1CCTL1 &= ~CCIFG;
    TB1CCTL1 |= CCIE;
    TB1CCR1 = TB1R + TB1CCR1_INTERVAL;
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
    command_enabled = FALSE;
    process_line_follow = TRUE;
    drive_timer = 0;
    display_menu = FALSE;

    // drive_pause_complete = FALSE;
    // circle_lap_count = 0;

    strcpy(display_line[0], "BL Start  ");
    display_changed = 1;

    Sample_ADC();
    DAC_Set_Voltage(DAC_MOTOR_SLOW);                          // Start driving toward line
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

    if (SW1_pressed || SW2_pressed){
        SW1_pressed = FALSE;
        SW2_pressed = FALSE;
        Wheels_Safe_Stop();
        process_line_follow = FALSE;
        drive_state = DRIVE_IDLE;
        display_menu = TRUE;
        command_enabled = TRUE;
    }

    unsigned char left_on_line = (CAR_Left_Detect > LEFT_THRESHOLD);            // Greater than threshold == On Line
    unsigned char right_on_line = (CAR_Right_Detect > RIGHT_THRESHOLD);         // (less than thresh is off or drifting)

    //DEBUG DISPLAY
    // if(left_on_line || right_on_line){
    //     strcpy(display_line[2], "          ");
    //     if(left_on_line){
    //         display_line[2][9] = 'L';
    //     }
    //     if (right_on_line){
    //         display_line[2][0] = 'R';
    //     }
    //     display_changed = TRUE;
    // }

    switch (drive_state) 
    {
    case DRIVE_TURN_LEFT:
        if (CAR_Left_Detect < (LEFT_WHITE_VALUE + 50) &&
            CAR_Right_Detect < (RIGHT_WHITE_VALUE + 50)) {
            // White detected - transition to seeking black line
            PWM_FORWARD();
            drive_state = DRIVE_START;
            drive_timer = 0;

            strcpy(display_line[0], "BL Start  ");
            display_changed = TRUE;
        }
        else {
            // Continue wide left arc - BOTH WHEELS MOVING
            PWM_Opening_Curve_Left();  //
        }
        break;

    case DRIVE_TURN_RIGHT:
        if (CAR_Left_Detect < (LEFT_WHITE_VALUE + 50) &&
            CAR_Right_Detect < (RIGHT_WHITE_VALUE + 50)) {
            // White detected - transition to seeking black line
            PWM_FORWARD();
            drive_state = DRIVE_START;
            drive_timer = 0;
            
            strcpy(display_line[0], "BL Start  ");
            display_changed = TRUE;
        }
        else {
            // Continue wide right arc - BOTH WHEELS MOVING
            PWM_Opening_Curve_Right();  //
        }
        break;

    case DRIVE_START:
        if (left_on_line || right_on_line) {
            if(left_on_line) {
                left_first = TRUE;
                // strcpy(display_line[1], "L First   ");       //DEBUG
            }
            else if (right_on_line) {
                right_first = TRUE;
                // strcpy(display_line[1], "R First   ");    //DEBUG
            }
            if (left_first && right_first) {
            }
            // Line detected - 50ms reverse pulse to ebrake
            PWM_EBRAKE();

            drive_state = DRIVE_BRAKE_RECOVER;                          // Wait 500ms for motor recovery
            drive_timer = 0;
        }
        break;

    case DRIVE_BRAKE_RECOVER:
        // After 50ms intercept brake pulse, wait 500ms total for motor recovery
        // Motors are already stopped by Wheels_Intercept_Brake()
        if (!ebraking){ // 50ms
            // Stop motors using PWM function
            Wheels_Safe_Stop();
            strcpy(display_line[0], "INTERCEPT ");
            display_changed = TRUE;
            // 500ms safety delay elapsed - safe to proceed with alignment
            if (drive_timer >= DRIVE_DELAY_MS(PRESENTATION_DELAY)) {
                drive_state = DRIVE_RAW_ALIGN;
                drive_timer = 0;
                strcpy(display_line[0], "BL Turn   ");
                display_changed = TRUE;
            }
        }
        break;


        case DRIVE_RAW_ALIGN:
        // After intercept brake, align the car to be parallel with the line
        // This handles perpendicular and angled approaches

            if (!left_on_line && !right_on_line) {
                if (pad_exit_direction != '\0') {
                    switch (pad_exit_direction) {
                    case 'L':
                        PWM_ROTATE_RIGHT();
                        break;
                    case 'R':
                        PWM_ROTATE_LEFT();
                        break;
                    }
                }
                else {
                    if (right_first) {
                        PWM_ROTATE_LEFT();
                    }
                    else if (left_first) {
                        PWM_ROTATE_RIGHT();
                    }
                }
            }
        else{
            Wheels_Safe_Stop();
            if (drive_timer >= DRIVE_DELAY_MS(500)) {
                next_drive_state = DRIVE_TRAVEL;
                drive_state = DRIVE_TUNE_ALIGN;
                // drive_state = DRIVE_BRAKE_DELAY;
                drive_timer = 0;
                drive_pause_complete = FALSE;
                left_first = FALSE;   // Reset tracking for next alignment if needed
                right_first = FALSE;
                // strcpy(display_line[0], "BL Travel ");
                // display_changed = TRUE;
            }
            }
        break;

    case DRIVE_TUNE_ALIGN:
        DAC_Set_Voltage(DAC_MOTOR_CRAWL);
        if(left_on_line && right_on_line){
            Wheels_Safe_Stop();
            // next_drive_state = DRIVE_TRAVEL;
            if (first_turn){
                drive_state  = DRIVE_TRAVEL_DELAY;
            }
            drive_state = DRIVE_BRAKE_DELAY;
            drive_timer = 0;
            drive_pause_complete = FALSE;
        }
        else if (drive_timer >= DRIVE_DELAY_MS(500)){
            drive_timer = 0;
            if(left_on_line && !right_on_line){
                PWM_ROTATE_LEFT();
            }
            else if(!left_on_line && right_on_line){
                PWM_ROTATE_RIGHT();
            }
            else if((CAR_Left_Detect < (LEFT_THRESHOLD - 100)) && (CAR_Right_Detect < (RIGHT_THRESHOLD - 100))){
                // Both far off line - back up
                PWM_REVERSE_PULSE();
                drive_state = DRIVE_BACKUP;
            }
            else{
                if(CAR_Left_Detect > CAR_Right_Detect){
                    PWM_ROTATE_LEFT();
                }
                else{
                    PWM_ROTATE_RIGHT();
                }
            }
        }
        break;

    case DRIVE_TRAVEL_DELAY:
        if (drive_timer >= DRIVE_DELAY_MS(PRESENTATION_DELAY)){
            drive_state = DRIVE_BRAKE_DELAY;
            drive_timer = 0;
            strcpy(display_line[0], "BL Travel ");
            display_changed = TRUE;
            first_turn = FALSE;
        }
        break;

    case DRIVE_BRAKE_DELAY:
        if (drive_timer >= DRIVE_DELAY_MS(500)) {
            drive_timer = 0;

            // 500ms brake delay complete - transition to next state
            if (!right_on_line || !left_on_line){
                drive_state = DRIVE_TUNE_ALIGN;
            }
            else {
                DAC_Set_Voltage(DAC_MOTOR_SLOW);
                drive_state = DRIVE_TRAVEL;
                drive_pause_complete = FALSE;
            }
        }
        break;

    case DRIVE_TRAVEL:
        if (!drive_pause_complete) {
            if (drive_timer >= DRIVE_DELAY_MS(500)) {
                drive_pause_complete = TRUE;
                drive_timer = 0;
                circle_detect_timer = 0;        // Reset circle detection
                in_potential_circle = FALSE;
            }
        }
        else if (!left_on_line && !right_on_line) {
            // Line lost - stop and search
            if (drive_timer >= DRIVE_DELAY_MS(500)) {
                PWM_REVERSE_PULSE();
                drive_state = DRIVE_BACKUP;
                drive_timer = 0;
                circle_detect_timer = 0;        // Reset on line lost
                in_potential_circle = FALSE;
            }
        }
        else {
            drive_timer = 0;

            // Calculate error to detect turning direction
            int left_drift = CAR_Left_Detect - LEFT_THRESHOLD;
            int right_drift = CAR_Right_Detect - RIGHT_THRESHOLD;
            int error = left_drift - right_drift;

            // Circle detection
            // Positive error = drifting LEFT, which = turning RIGHT (clockwise)
            if (!circle_found) {
                if (error > 50) {  // Turning right
                    circle_detect_timer++;
                    if (!in_potential_circle && circle_detect_timer >= DRIVE_DELAY_MS(1000)) {
                        in_potential_circle = TRUE;  // 1 second of right turn detected
                    }
                    // After 3 seconds of sustained right turn, we're in circle
                    if (in_potential_circle && circle_detect_timer >= DRIVE_DELAY_MS(3000)) {
						Wheels_Safe_Stop();
                        drive_state = DRIVE_CIRCLE;
                        drive_timer = 0;
                        circle_detect_timer = 0;
                        in_potential_circle = FALSE;
                        circle_found = TRUE;


                        strcpy(display_line[0], "BL Circle ");
                        display_changed = TRUE;
                    }
                }
                else {
                    // Not turning right consistently - reset detection
                    circle_detect_timer = 0;
                    in_potential_circle = FALSE;
                }
            }

            Line_Follow_PD_V1();
            //strcpy(display_line[0], "BL Travel ");
            display_changed = TRUE;
        }
        break;

	case DRIVE_CIRCLE:
		if (drive_timer >= DRIVE_DELAY_MS(PRESENTATION_DELAY)) {
			drive_timer = 0;
            drive_state = DRIVE_TUNE_ALIGN;
            command_enabled = TRUE;
		}
        break;


    case DRIVE_EXIT:
        // Exit circle and drive away 2+ feet
        strcpy(display_line[0], "BL Exit   ");
        display_changed = TRUE;
        Wheels_Safe_Stop();

        if (drive_timer >= DRIVE_DELAY_MS(PRESENTATION_DELAY)) {
            drive_timer = 0;
            DAC_Set_Voltage(DAC_MOTOR_SLOW);
            PWM_FORWARD();
            drive_state = DRIVE_STOP;
        }
        break;


    case DRIVE_STOP:
        // Course complete
        if (drive_timer >= DRIVE_DELAY_MS(3000)) {
            Wheels_Safe_Stop();
            DAC_Set_Voltage(DAC_MOTOR_OFF);

            strcpy(display_line[0], " BL Stop  ");
            strcpy(display_line[1], "Dont Drink");
            strcpy(display_line[2], "and Derive");
            display_changed = TRUE;

            process_line_follow = FALSE;
            command_enabled = TRUE;
//            display_menu = TRUE;

            TB1CCTL1 &= ~CCIFG;
            TB1CCTL1 &= ~CCIE;
        }
        break;


    case DRIVE_BACKUP:
        if( right_on_line || left_on_line ){
            Wheels_Safe_Stop();
            drive_state = DRIVE_TUNE_ALIGN;
            drive_timer = 0;
        }
        break;
    }
}

void Line_Follow_Exit_Circle(void){                             // FN done!
    drive_state = DRIVE_EXIT;
    Wheels_Safe_Stop();                                 // Safely stop all motors
    DAC_Set_Voltage(DAC_MOTOR_OFF);
}


// =============================================================================
//      PD CONTROLLER LOGIC - THE GOOD STUFF!
// =============================================================================
// REMEMBER:   int error = left_drift - right_drift;   AKA: POSITIVE when line is LEFT
// =============================================================================
void Line_Follow_PD_V1 (void){
    static int previous_error = 0;
    static unsigned char first_run = TRUE;
    // PD Controller Constants (same as V4)
	// MOVE TO MACROS WHEN DONE TUNING
    #define Kp_GENTLE_V4                (150)                       // Proportional gain for small corrections
    #define Kp_SHARP_V4                 (300)                       // Proportional gain for sharp turns
    #define Kd_V4                       (20)                        // Derivative gain
    #define BASE_SPEED_STRAIGHT_V4      (CRAWL)                     // Speed when centered
    #define BASE_SPEED_TURN_V4          (TIPTOE)                    // Slower speed in turns
    #define MAX_CORRECTION_V4           (25000)                     // Maximum PWM correction
    #define SHARP_ERROR_THRESHOLD_V4    (SHARP_TURN_THRESHOLD)      // When to use aggressive response
    
    // Calculate drift 
    int left_drift = CAR_Left_Detect - LEFT_THRESHOLD;
    int right_drift = CAR_Right_Detect - RIGHT_THRESHOLD;
    
    // error calc [proportional] (positive = drifting left, negative = drifting right)
    int error = left_drift - right_drift;
    int abs_error = (error > 0) ? error : -error;
    
    // derivative calc (reaction speed)
    int derivative = 0;
    if (!first_run) {
        derivative = error - previous_error;
    }
    else {
        first_run = FALSE;
    }
    previous_error = error;
    
    // STRONG Kp: Base not enought for sharp turns, Stronger Kp - slower speed
    int Kp_active;
    unsigned int base_speed;
    
    if (abs_error > SHARP_ERROR_THRESHOLD_V4) {        // SHARP TURN

        Kp_active = Kp_SHARP_V4;
        base_speed = BASE_SPEED_TURN_V4;
    }
    else {                                              // GENTLE CORRECTION
        Kp_active = Kp_GENTLE_V4;
        base_speed = BASE_SPEED_STRAIGHT_V4;
    }
    
    int pd_output = (Kp_active * error) + (Kd_V4 * derivative);    // PD calculation
    
    if (pd_output > MAX_CORRECTION_V4) {            // Clamp to avoid ridiculous changes
        pd_output = MAX_CORRECTION_V4;
    }
    else if (pd_output < -MAX_CORRECTION_V4) {
        pd_output = -MAX_CORRECTION_V4;
    }
    
    // Calculate target speeds
    int left_speed, right_speed;
    
    if (abs_error < DEADZONE) {        // centered Dont adjust
        left_speed = base_speed;
        right_speed = base_speed;
    }

    // REMOVE BELOW AND REPLACE WITH HARDER KPs IF CANNOT TUNE
    else if (abs_error > SHARP_ERROR_THRESHOLD_V4) {        // SHARP TURN

        //=================================================
        // CONFRIM DIRECTION-SIGN CALCS ALIGN WITH REALITY!
        //  - UPDATE: They do. 
        //  - ITS WORKING SO DONT MESS WITH THIS DALLAS!
        // ================================================

        if (error > 0) {
            // Drifting LEFT - need sharp RIGHT turn
            // LEFT wheel maintains/increases speed, RIGHT wheel slows/stops
            left_speed = base_speed - (pd_output / 2);   // Outside wheel faster
            right_speed = base_speed + pd_output;        // Inside wheel much slower
            
            // PIVOT on extra sharp
            if (right_speed < TIPTOE) {
                right_speed = WHEEL_OFF;
            }
        }
        else {
            // Drifting RIGHT - need sharp LEFT turn
            // RIGHT wheel maintains/increases speed, LEFT wheel slows/stops
            right_speed = base_speed + (pd_output / 2);  // Outside wheel faster
            left_speed = base_speed - pd_output;         // Inside wheel much slower
            
            // For very sharp turns, stop inside wheel completely
            if (left_speed < TIPTOE) {
                left_speed = WHEEL_OFF;
            }
        }
    }
    else {                                              // GENTLE CORRECTION
        left_speed = base_speed - pd_output;
        right_speed = base_speed + pd_output;
    }

    // Clamp to valid range
    if (left_speed < WHEEL_OFF) left_speed = WHEEL_OFF;                 
    if (left_speed > MAX_AUTO_SPEED) left_speed = MAX_AUTO_SPEED;
    if (right_speed < WHEEL_OFF) right_speed = WHEEL_OFF;
    if (right_speed > MAX_AUTO_SPEED) right_speed = MAX_AUTO_SPEED;
    
    // I dont know... because why not?
    LEFT_REVERSE_SPEED = WHEEL_OFF;
    RIGHT_REVERSE_SPEED = WHEEL_OFF;
    DAC_Set_Voltage(DAC_MOTOR_CRAWL);
    
    // should only pivot on sharp turn case, else clamp minimum to TIPTOE
    LEFT_FORWARD_SPEED = (left_speed > 0 && left_speed < TIPTOE) ? TIPTOE : left_speed;
    RIGHT_FORWARD_SPEED = (right_speed > 0 && right_speed < TIPTOE) ? TIPTOE : right_speed;
}



// =========================================
//      DEBUG CONTROLLER FOR TESTING
// =========================================
// void Line_Follow_P_Debug(void) {
//     static int previous_error = 0;

//     int left_drift  = CAR_Left_Detect  - LEFT_THRESHOLD;
//     int right_drift = CAR_Right_Detect - RIGHT_THRESHOLD;
//     const int MAX_CORRECTION = 20000;
//     int Kp_DEBUG = 100;
//     const unsigned int BASE_SPEED_DEBUG = LINE_SPEED;


//     int error = left_drift - right_drift;   // POSITIVE when line is LEFT

//     int output = Kp_DEBUG * error;         // pick something like 10–20

//     if (output > MAX_CORRECTION) {
//         output = MAX_CORRECTION;
//     }
//     if (output < -MAX_CORRECTION) {
//         output = -MAX_CORRECTION;
//     }

//     int left_target  = BASE_SPEED_DEBUG - output;
//     int right_target = BASE_SPEED_DEBUG + output;

//     // clamp to [WHEEL_OFF, MAX_AUTO_SPEED]

//     LEFT_REVERSE_SPEED  = WHEEL_OFF;
//     RIGHT_REVERSE_SPEED = WHEEL_OFF;

//     LEFT_FORWARD_SPEED  = (left_target  < WHEEL_OFF)       ? WHEEL_OFF       : left_target;
//     RIGHT_FORWARD_SPEED = (right_target < WHEEL_OFF)       ? WHEEL_OFF       : right_target;
// }
