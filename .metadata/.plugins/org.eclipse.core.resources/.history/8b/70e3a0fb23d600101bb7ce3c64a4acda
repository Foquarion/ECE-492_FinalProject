/*
 * calibration.h
 *
 *  Created on: Nov 30, 2025
 *      Author: Dallas.Owens
 */

#ifndef CALIBRATION_H_
#define CALIBRATION_H_


#define SAMPLES        (10)    // Number of samples to average during calibration


typedef enum {
    CALIB_IDLE,
    CALIB_PRIME_BLACK,                   // Waiting for SW1
    CALIB_SAMPLING_BLACK,               // Sampling black values
    CALIB_DISPLAY_BLACK,                // Display black results
    CALIB_PRIME_WHITE,                   // Waiting for SW2
    CALIB_SAMPLING_WHITE,               // Sampling white values
    CALIB_DISPLAY_WHITE,                // Display white results
    CALIB_DISPLAY_RESULTS,               // Display thresholds
    CALIB_COMPLETE                      // Done
} calib_state_t;

extern calib_state_t calib_state;

extern volatile unsigned int calib_timer;
extern unsigned char calib_sample_count;
extern unsigned long calib_left_sum;
extern unsigned long calib_right_sum;

extern volatile unsigned char process_calibration;

// CALIBRATION Value storage
extern unsigned int LEFT_BLACK_VALUE;
extern unsigned int LEFT_WHITE_VALUE ;
extern unsigned int RIGHT_BLACK_VALUE;
extern unsigned int RIGHT_WHITE_VALUE;
extern unsigned int LEFT_THRESHOLD;
extern unsigned int RIGHT_THRESHOLD;

extern volatile unsigned char calibrating;
extern unsigned char calibration_complete;

//extern unsigned char exit_circle;


void IR_Calibrate_Menu(void);
void IR_Calibrate_Process(void);
void Display_Calibration(void);


#endif /* CALIBRATION_H_ */
