/*
 * calibration.c
 *
 *  Created on: Nov 30, 2025
 *      Author: Dallas.Owens
 */

#include "timers.h"
#include "msp430.h"
#include "macros.h"
#include "functions.h"
#include "ADC.h"
#include "LCD.h"
#include <string.h>
#include "calibration.h"
#include "switches.h"
#include "wheels.h"
#include "DAC.h"


calib_state_t calib_state = CALIB_IDLE;

volatile unsigned int calib_timer = 0;
unsigned char calib_sample_count = 0;
unsigned long calib_left_sum = 0;
unsigned long calib_right_sum = 0;

volatile unsigned char process_calibration = FALSE;

// CALIBRATION Value storage
unsigned int LEFT_BLACK_VALUE = 0;
unsigned int LEFT_WHITE_VALUE = 0;
unsigned int RIGHT_BLACK_VALUE = 0;
unsigned int RIGHT_WHITE_VALUE = 0;
unsigned int LEFT_THRESHOLD = 0;
unsigned int RIGHT_THRESHOLD = 0;

volatile unsigned char calibrating = FALSE;
unsigned char calibration_complete = FALSE;
extern unsigned char display_menu;

//unsigned char exit_circle = FALSE;


void IR_Calibrate_Menu(void)
{ // Calibration Entry Point
    calib_state = CALIB_PRIME_BLACK;
    calib_timer = 0;
    calib_sample_count = 0;
    calibrating = TRUE;

//    drive_state = DRIVE_IDLE;               // For safety
    LEFT_FORWARD_SPEED = WHEEL_OFF;
    RIGHT_FORWARD_SPEED = WHEEL_OFF;


    strcpy(display_line[0], "IR CALIBRT");
    strcpy(display_line[1], "Place on  ");
    strcpy(display_line[2], "BLACK line");
    strcpy(display_line[3], "Press SW1 ");
    display_changed = TRUE;

    TB1CCTL1 &= ~CCIFG;                     // Clear possible pending interrupt
    TB1CCR1 = TB1R + TB1CCR1_INTERVAL;
    TB1CCTL1 |=  CCIE;                      // CCR1 enable interrupt
}

void IR_Calibrate_Process(void) {
	if (!calibrating) return;               // Only process if calibrating
    switch (calib_state) 
    {
    case CALIB_IDLE:                                    // Do nothing catch state if not calibrating
        break;  

    case CALIB_PRIME_BLACK:
        if (SW2_pressed) {
            SW2_pressed = 0;
            calib_state = CALIB_SAMPLING_BLACK;
            calib_sample_count = 0;
            calib_left_sum = 0;
            calib_right_sum = 0;
        }
        break;

    case CALIB_SAMPLING_BLACK:
        // calib_left_sum accumulates actual left sensor (CAR_Left_Detect)
        // calib_right_sum accumulates actual right sensor (CAR_Right_Detect)
        calib_left_sum += CAR_Left_Detect;
        calib_right_sum += CAR_Right_Detect;
        calib_sample_count++;

        if (calib_sample_count >= SAMPLES) {
            LEFT_BLACK_VALUE = calib_left_sum / SAMPLES;
            RIGHT_BLACK_VALUE = calib_right_sum / SAMPLES;

            strcpy(display_line[0], "BLACK Val ");
            strcpy(display_line[1], "L:        ");
            HEXtoBCD(LEFT_BLACK_VALUE);
            adc_line(2, 3);

            strcpy(display_line[2], "R:        ");
            HEXtoBCD(RIGHT_BLACK_VALUE);
            adc_line(3, 3);

            strcpy(display_line[3], "Next:WHITE");
            display_changed = 1;

            calib_state = CALIB_DISPLAY_BLACK;
            calib_timer = 0;
        }
        break;

    case CALIB_DISPLAY_BLACK:
		if (calib_timer >= DELAY_MS(2000)) {                  // Display previous calibrated values for 2 seconds then proceed
            strcpy(display_line[1], "Place on  ");
            strcpy(display_line[2], "WHITE spot");
            strcpy(display_line[3], "Press SW1 ");
            display_changed = TRUE;
            
            calib_state = CALIB_PRIME_WHITE;
        }
        break;

    case CALIB_PRIME_WHITE:
        if (SW2_pressed) {
            SW2_pressed = 0;
            calib_state = CALIB_SAMPLING_WHITE;
            calib_sample_count = 0;
            calib_left_sum = 0;
            calib_right_sum = 0;
        }
        break;

    case CALIB_SAMPLING_WHITE:
        // calib_left_sum accumulates actual left sensor (CAR_Left_Detect)
        // calib_right_sum accumulates actual right sensor (CAR_Right_Detect)
        calib_left_sum += CAR_Left_Detect;
        calib_right_sum += CAR_Right_Detect;
        calib_sample_count++;

        if (calib_sample_count >= SAMPLES) {
            LEFT_WHITE_VALUE = calib_left_sum / SAMPLES;
            RIGHT_WHITE_VALUE = calib_right_sum / SAMPLES;
            LEFT_THRESHOLD = (LEFT_BLACK_VALUE + LEFT_WHITE_VALUE) / 2;
            RIGHT_THRESHOLD = (RIGHT_BLACK_VALUE + RIGHT_WHITE_VALUE) / 2;

            strcpy(display_line[0], "WHITE Val ");
            strcpy(display_line[1], "L:        ");
            HEXtoBCD(LEFT_WHITE_VALUE);
            adc_line(2, 3);

            strcpy(display_line[2], "R:        ");
            HEXtoBCD(RIGHT_WHITE_VALUE);
            adc_line(3, 3);

            strcpy(display_line[3], "Complete! ");
            display_changed = TRUE;

            calib_state = CALIB_DISPLAY_WHITE;
            calib_timer = 0;
        }
        break;

    case CALIB_DISPLAY_WHITE:
        if (calib_timer >= DELAY_MS(2000)) {
            strcpy(display_line[0], "Threshold ");
            strcpy(display_line[1], "L:        ");
            HEXtoBCD(LEFT_THRESHOLD);
            adc_line(2, 3);

            strcpy(display_line[2], "R:        ");
            HEXtoBCD(RIGHT_THRESHOLD);
            adc_line(3, 3);

            display_changed = TRUE;

            calib_state = CALIB_DISPLAY_RESULTS;
            calib_timer = 0;
        }
        break;

    case CALIB_DISPLAY_RESULTS:
        if (calib_timer >= DELAY_MS(2000)) {
            //lf_state = LF_WAITING;

            strcpy(display_line[0], "L:         ");
            strcpy(display_line[1], "           ");
            strcpy(display_line[2], "R:         ");
            strcpy(display_line[3], "           ");

            // Left Vals
            HEXtoBCD(LEFT_THRESHOLD);
            adc_line(1, 4);
            HEXtoBCD(LEFT_WHITE_VALUE);
            adc_line(2, 0);
            HEXtoBCD(LEFT_BLACK_VALUE);
            adc_line(2, 6);

            // Right Vals
            HEXtoBCD(RIGHT_THRESHOLD);
            adc_line(3, 4);
            HEXtoBCD(RIGHT_WHITE_VALUE);
            adc_line(4, 0);
            HEXtoBCD(RIGHT_BLACK_VALUE);
            adc_line(4, 6);
            
            
            display_changed = TRUE;

            calib_state = CALIB_COMPLETE;
            calib_timer = 0;
        }
        break;

    case CALIB_COMPLETE:

        calibration_complete = TRUE;
        calibrating = FALSE;

        calib_state = CALIB_IDLE;
        calib_sample_count = 0;
        calib_left_sum = 0;
        calib_right_sum = 0;

        TB1CCTL1 &= ~CCIFG;                     // Clear possible pending interrupt
        TB1CCTL1 &= ~CCIE;                      // CCR1 enable interrupt

        display_menu = TRUE;
        break;
    }
}


void Display_Calibration(void){
    strcpy(display_line[0], "L:         ");
    strcpy(display_line[1], "           ");
    strcpy(display_line[2], "R:         ");
    strcpy(display_line[3], "           ");

    // Left Vals
    HEXtoBCD(LEFT_THRESHOLD);
    adc_line(1, 6);
    HEXtoBCD(LEFT_WHITE_VALUE);
    adc_line(2, 0);
    HEXtoBCD(LEFT_BLACK_VALUE);
    adc_line(2, 6);

    // Right Vals
    HEXtoBCD(RIGHT_THRESHOLD);
    adc_line(3, 6);
    HEXtoBCD(RIGHT_WHITE_VALUE);
    adc_line(4, 0);
    HEXtoBCD(RIGHT_BLACK_VALUE);
    adc_line(4, 6);

    display_changed = TRUE;
}
