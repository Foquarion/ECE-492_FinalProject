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
    PART1_IDLE,
    PART1_DRIVE_TO_LINE,
    PART1_BRAKE,
    PART1_BRAKE_WAIT,
    PART1_CHECK_LINE,
    PART1_BACKUP,
    PART1_BACKUP_WAIT,
    PART1_BACKUP_SETTLE,
    PART1_ALIGN,
    PART1_ALIGN_REVERSE,        // NEW: Reverse pulse before straightening
    PART1_ALIGN_REVERSE_WAIT,   // NEW: Wait for reverse pulse
    PART1_ALIGN_TURN,           // NEW: Turn to straighten on line
    PART1_ALIGN_TURN_WAIT,      // NEW: Wait for turn pulse
    PART1_ALIGNED,
    LINE_FOLLOWING,
    LINE_RECOVERY_REVERSE,      // NEW: Reverse when line lost
    LINE_RECOVERY_REVERSE_WAIT, // NEW: Wait for line detection
    LINE_RECOVERY_ALIGN_REV,    // NEW: Reverse pulse before straightening
    LINE_RECOVERY_ALIGN_TURN    // NEW: Turn to straighten
} part1_state_t;

part1_state_t part1_state = PART1_IDLE;

// Globals
extern unsigned char command_enabled;
extern char display_line[4][11];
extern volatile unsigned char display_changed;
volatile unsigned char process_line_follow = FALSE;
volatile unsigned int drive_timer;

// Which sensor detected first (for alignment)
unsigned char car_left_detected_first = FALSE;
unsigned char car_right_detected_first = FALSE;
extern volatile unsigned char ebraking;
extern unsigned char display_menu;

// Track darkest reading before line loss for recovery
unsigned char recovery_turn_left = FALSE;   // TRUE = turn left during recovery, FALSE = turn right
unsigned int last_adc_left_value = 0;      // Last ADC reading from left sensor
unsigned int last_adc_right_value = 0;     // Last ADC reading from right sensor

unsigned char current_adc_left_on_line;
unsigned char current_adc_right_on_line;
unsigned char current_car_left_on_line;     // ADC Left = Physical Left
unsigned char current_car_right_on_line;    // ADC Right = Physical Right




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
    car_left_detected_first = FALSE;
    car_right_detected_first = FALSE;
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
    unsigned char adc_left_on_line = (ADC_Left_Detect > LEFT_THRESHOLD);
    unsigned char adc_right_on_line = (ADC_Right_Detect > RIGHT_THRESHOLD);

    unsigned char adc_left_crossed = (ADC_Left_Detect > LEFT_BLACK_VALUE);
    unsigned char adc_right_crossed = (ADC_Right_Detect > RIGHT_BLACK_VALUE);

    unsigned char car_left_on_line = adc_left_on_line;     // ADC Left = Physical Left
    unsigned char car_right_on_line = adc_right_on_line;   // ADC Right = Physical Right

    unsigned char car_left_crossed = adc_left_crossed;     // ADC Left = Physical Left
    unsigned char car_right_crossed = adc_right_crossed;   // ADC Right = Physical Right

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
        if (car_left_crossed || car_right_crossed)
        {

            // Record which PHYSICAL side detected first
            if (car_right_crossed && !car_right_detected_first && !car_left_detected_first)
            {
                car_right_detected_first = TRUE;

                strcpy(display_line[0], "Part1     ");
                strcpy(display_line[1], "RIGHT side");
                strcpy(display_line[2], "detected! ");
                display_changed = 1;
            }
            else if (car_left_crossed && !car_left_detected_first && !car_right_detected_first)
            {
                car_left_detected_first = TRUE;

                strcpy(display_line[0], "Part1     ");
                strcpy(display_line[1], "LEFT side ");
                strcpy(display_line[2], "detected! ");
                display_changed = 1;
            }

            // Move to brake state
            part1_state = PART1_BRAKE;
            drive_timer = 0;
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

        if (car_left_on_line || car_right_on_line) {
            part1_state = PART1_ALIGN;

            strcpy(display_line[0], "Part1     ");
            strcpy(display_line[1], "Line OK   ");
            strcpy(display_line[2], "Aligning  ");
            display_changed = 1;
        }
        else {
            part1_state = PART1_BACKUP;
            drive_timer = 0;
            DAC_Set_Voltage(DAC_MOTOR_SLOW      );

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
            if (car_left_on_line || car_right_on_line) {
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
        if (car_left_on_line && car_right_on_line) {
            // ALIGNED! Now perform reverse+turn alignment sequence
            part1_state = PART1_ALIGN_REVERSE;
            drive_timer = 0;

            strcpy(display_line[0], "Part1     ");
            strcpy(display_line[1], "Both on   ");
            strcpy(display_line[2], "Aligning  ");
            display_changed = 1;
        }
        else if (car_right_detected_first) {
            // PHYSICAL RIGHT detected first → RIGHT on line → turn LEFT to bring LEFT on
            PWM_TURN_LEFT();
        }
        else if (car_left_detected_first) {
            // PHYSICAL LEFT detected first → LEFT on line → turn RIGHT to bring RIGHT on
            PWM_TURN_RIGHT();
        }
        else if (car_left_on_line && !car_right_on_line) {
            // Only left on line - turn right to get right sensor on
            PWM_TURN_RIGHT();
        }
        else if (car_right_on_line && !car_left_on_line) {
            // Only right on line - turn left to get left sensor on
            PWM_TURN_LEFT();
        }
        else {
            // Neither sensor on line - shouldn't happen but stop
            Wheels_Safe_Stop();
        }
        break;

    case PART1_ALIGN_REVERSE:
        PWM_REVERSE();
        part1_state = PART1_ALIGN_REVERSE_WAIT;
        drive_timer = 0;

        strcpy(display_line[0], "Align     ");
        strcpy(display_line[1], "Rev pulse ");
        strcpy(display_line[2], "          ");
        display_changed = 1;
        break;

    case PART1_ALIGN_REVERSE_WAIT:
        if (drive_timer >= DELAY_MS(100)) {  // 100ms reverse pulse
            Wheels_Safe_Stop();
            part1_state = PART1_ALIGN_TURN;
            drive_timer = 0;
        }
        break;

    case PART1_ALIGN_TURN:
        // Turn in direction indicated by first sensor detection
        if (car_right_detected_first) {
            // Right detected first = car veering RIGHT = turn LEFT to correct
            PWM_TURN_LEFT();
        }
        else if (car_left_detected_first) {
            // Left detected first = car veering LEFT = turn RIGHT to correct
            PWM_TURN_RIGHT();
        }
        else {
            // Fallback - shouldn't happen but turn based on current sensors
            if (car_left_on_line && !car_right_on_line) {
                PWM_TURN_RIGHT();
            }
            else if (car_right_on_line && !car_left_on_line) {
                PWM_TURN_LEFT();
            }
            else {
                // Both on or both off - skip turn
                part1_state = PART1_ALIGNED;
                drive_timer = 0;
                break;
            }
        }

        part1_state = PART1_ALIGN_TURN_WAIT;
        drive_timer = 0;

        strcpy(display_line[0], "Align     ");
        strcpy(display_line[1], "Turn pulse");
        strcpy(display_line[2], "          ");
        display_changed = 1;
        break;

    case PART1_ALIGN_TURN_WAIT:
        if (drive_timer >= DELAY_MS(200)) {  // 200ms turn pulse
            Wheels_Safe_Stop();
            part1_state = PART1_ALIGNED;
            drive_timer = 0;

            strcpy(display_line[0], "Part1     ");
            strcpy(display_line[1], "ALIGNED!  ");
            strcpy(display_line[2], "Starting..");
            display_changed = 1;
        }
        break;

    //==========================================================================
    // ALIGNED: Wait then start following
    //==========================================================================
    case PART1_ALIGNED:
        Wheels_Safe_Stop();

        if (drive_timer >= DELAY_MS(1000)) {
            part1_state = LINE_FOLLOWING;
            drive_timer = 0;

            strcpy(display_line[0], "Following ");
            strcpy(display_line[1], "Line...   ");
            strcpy(display_line[2], "          ");
            display_changed = 1;

            DAC_Set_Voltage(DAC_MOTOR_SLOW);
        }
        break;

    //==========================================================================
    // LINE FOLLOWING: Proportional controller
    //==========================================================================
    case LINE_FOLLOWING:
        // Re-read sensors to get current state
        current_adc_left_on_line = (ADC_Left_Detect > LEFT_THRESHOLD);
        current_adc_right_on_line = (ADC_Right_Detect > RIGHT_THRESHOLD);
        current_car_left_on_line = current_adc_left_on_line;     // ADC Left = Physical Left
        current_car_right_on_line = current_adc_right_on_line;   // ADC Right = Physical Right

        // Track which side has darkest reading (for recovery guidance)
        last_adc_left_value = ADC_Left_Detect;    // Physical LEFT sensor
        last_adc_right_value = ADC_Right_Detect;  // Physical RIGHT sensor

        // Determine which side to turn toward if line is lost
        if (current_car_left_on_line && !current_car_right_on_line) {
            // Physical LEFT sensor darker - turn LEFT during recovery
            recovery_turn_left = TRUE;
        }
        else if (!current_car_left_on_line && current_car_right_on_line) {
            // Physical RIGHT sensor darker - turn RIGHT during recovery
            recovery_turn_left = FALSE;
        }

        // Check if both sensors lost line
        if (!current_car_left_on_line && !current_car_right_on_line) {
            Wheels_Safe_Stop();
            part1_state = LINE_RECOVERY_REVERSE;
            drive_timer = 0;

            strcpy(display_line[0], "Recovery  ");
            strcpy(display_line[1], "Line lost ");
            strcpy(display_line[2], "Reversing ");
            display_changed = 1;
        }
        else {
            // Still on line - execute proportional controller
            Line_Follow_Proportional();
        }
        break;

    //==========================================================================
    // NEW: LINE RECOVERY - Reverse until line found
    //==========================================================================
    case LINE_RECOVERY_REVERSE:
        PWM_REVERSE();
        part1_state = LINE_RECOVERY_REVERSE_WAIT;
        drive_timer = 0;
        break;

    case LINE_RECOVERY_REVERSE_WAIT:
        // Keep reversing until line detected
        if (car_left_on_line || car_right_on_line) {
            // Found line!
            Wheels_Safe_Stop();
            part1_state = LINE_RECOVERY_ALIGN_REV;
            drive_timer = 0;

            strcpy(display_line[0], "Recovery  ");
            strcpy(display_line[1], "Found line");
            strcpy(display_line[2], "Aligning  ");
            display_changed = 1;
        }
        else if (drive_timer > DELAY_MS(2000)) {
            // Timeout - couldn't find line
            Wheels_Safe_Stop();
            part1_state = PART1_IDLE;
            process_line_follow = FALSE;
            command_enabled = TRUE;

            strcpy(display_line[0], "ERROR     ");
            strcpy(display_line[1], "Recovery  ");
            strcpy(display_line[2], "Timeout   ");
            display_changed = 1;
        }
        break;

    //==========================================================================
    // NEW: LINE RECOVERY - Reverse pulse before straightening
    //==========================================================================
    case LINE_RECOVERY_ALIGN_REV:
        PWM_REVERSE();

        if (drive_timer >= DELAY_MS(200)) {  // 150ms reverse pulse
            Wheels_Safe_Stop();
            part1_state = LINE_RECOVERY_ALIGN_TURN;
            drive_timer = 0;
        }
        break;

    //==========================================================================
    // NEW: LINE RECOVERY - Turn to straighten on line
    //==========================================================================
    case LINE_RECOVERY_ALIGN_TURN:
        // Determine turn direction based on which side had darkest reading before line loss
        if (recovery_turn_left) {
            // LEFT side was darker - turn LEFT toward line
            PWM_TURN_LEFT();
        }
        else {
            // RIGHT side was darker - turn RIGHT toward line
            PWM_TURN_RIGHT();
        }

        if (drive_timer >= DELAY_MS(300)) {  // 300ms turn pulse
            Wheels_Safe_Stop();
            part1_state = LINE_FOLLOWING;
            drive_timer = 0;

            strcpy(display_line[0], "Recovery  ");
            strcpy(display_line[1], "Complete  ");
            strcpy(display_line[2], "Following ");
            display_changed = 1;
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


void Line_Follow_Proportional(void) {
    /*
     * PROPORTIONAL CONTROLLER
     *
     * Sensor behavior: Higher ADC = BLACK (on line)
     *
     * Calculate error from center:
     *   - Both sensors equal = centered on line
     *   - Left > Right = drifting LEFT, need to turn RIGHT
     *   - Right > Left = drifting RIGHT, need to turn LEFT
     *
     * Sensors are on correct sides:
     *   ADC_Left_Detect is physically LEFT side
     *   ADC_Right_Detect is physically RIGHT side
     */

    // Calculate how far each sensor is from threshold
    int adc_left_error = ADC_Left_Detect - LEFT_THRESHOLD;   // Physical LEFT sensor
    int adc_right_error = ADC_Right_Detect - RIGHT_THRESHOLD; // Physical RIGHT sensor

    // Map to physical car sides
    int car_left_error = adc_left_error;    // Physical LEFT
    int car_right_error = adc_right_error;  // Physical RIGHT

    // Calculate steering error
    // Positive = drifting LEFT (left sensor sees more black), turn LEFT
    // Negative = drifting RIGHT (right sensor sees more black), turn RIGHT
    int steering_error = car_left_error - car_right_error;

    //==========================================================================
    // CENTERED - Both sensors near threshold, minimal drift
    //==========================================================================
    if (abs(steering_error) < DEADZONE) {
        // Well centered on line - go straight at crawl speed
        DAC_Set_Voltage(DAC_MOTOR_SLOW);

        // Set reverse speeds to off to maintain forward direction
        LEFT_REVERSE_SPEED = WHEEL_OFF;
        RIGHT_REVERSE_SPEED = WHEEL_OFF;

        // Both wheels at equal speed
        LEFT_FORWARD_SPEED = CRAWL;
        RIGHT_FORWARD_SPEED = CRAWL;
    }

    //==========================================================================
    // SLIGHT DRIFT - Proportional correction
    //==========================================================================
    else if (abs(steering_error) < SHARP_TURN_THRESHOLD) {
        // Calculate proportional PWM adjustment
        int pwm_adjustment = abs(steering_error) * Kp;

        // Clamp adjustment to reasonable range
        if (pwm_adjustment > 15000) {
            pwm_adjustment = 15000;
        }

        DAC_Set_Voltage(DAC_MOTOR_SLOW);

        // Set reverse speeds to off to maintain forward direction tracking
        LEFT_REVERSE_SPEED = WHEEL_OFF;
        RIGHT_REVERSE_SPEED = WHEEL_OFF;

        if (steering_error > 0) {
            // Drifting LEFT - slow left wheel to turn LEFT
            LEFT_FORWARD_SPEED = CRAWL - pwm_adjustment;
            RIGHT_FORWARD_SPEED = CRAWL;

            // Clamp minimum speed
            if (LEFT_FORWARD_SPEED < TIPTOE) {
                LEFT_FORWARD_SPEED = TIPTOE;
            }
        }
        else {
            // Drifting RIGHT - slow right wheel to turn RIGHT
            LEFT_FORWARD_SPEED = CRAWL;
            RIGHT_FORWARD_SPEED = CRAWL - pwm_adjustment;

            // Clamp minimum speed
            if (RIGHT_FORWARD_SPEED < TIPTOE) {
                RIGHT_FORWARD_SPEED = TIPTOE;
            }
        }
    }

    //==========================================================================
    // SIGNIFICANT DRIFT - Sharp correction
    //==========================================================================
    else {
        DAC_Set_Voltage(DAC_MOTOR_SLOW);

        // Set reverse speeds to off to maintain forward direction
        LEFT_REVERSE_SPEED = WHEEL_OFF;
        RIGHT_REVERSE_SPEED = WHEEL_OFF;

        if (steering_error > 0) {
            // Drifting LEFT - sharp turn LEFT by making left wheel very slow
            LEFT_FORWARD_SPEED = TIPTOE;
            RIGHT_FORWARD_SPEED = CRAWL;
        }
        else {
            // Drifting RIGHT - sharp turn RIGHT by making right wheel very slow
            LEFT_FORWARD_SPEED = CRAWL;
            RIGHT_FORWARD_SPEED = TIPTOE;
        }
    }
}
