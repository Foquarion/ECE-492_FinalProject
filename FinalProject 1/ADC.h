/*
 * ADC.h
 *
 *  Created on: Oct 16, 2025
 *      Author: Dallas.Owens
 */

#ifndef ADC_H_
#define ADC_H_

// ================ MACROS =====================
// Detection macros
#define LEFT_ON_BLACK   (ADC_Left_Detect < LEFT_THRESHOLD)
#define LEFT_ON_WHITE   (ADC_Left_Detect > LEFT_THRESHOLD)
#define RIGHT_ON_BLACK  (ADC_Right_Detect < RIGHT_THRESHOLD)
#define RIGHT_ON_WHITE  (ADC_Right_Detect > RIGHT_THRESHOLD)

#define SAMPLES        (10)    // Number of samples to average during calibration


// ================ GLOBALS =====================
// Calibration values (set during calibration routine)
extern unsigned int LEFT_BLACK_VALUE;
extern unsigned int LEFT_WHITE_VALUE;
extern unsigned int RIGHT_BLACK_VALUE;
extern unsigned int RIGHT_WHITE_VALUE;


// Calculated thresholds (midpoint between black and white)
extern unsigned int LEFT_THRESHOLD;
extern unsigned int RIGHT_THRESHOLD;


extern volatile unsigned int ADC_Thumb;
extern volatile unsigned int ADC_Left_Detect;
extern volatile unsigned int ADC_Right_Detect;

extern volatile unsigned char display_thumb;
extern volatile unsigned char display_left_detect;
extern volatile unsigned char display_right_detect;

extern volatile unsigned char sample_adc;


void Sample_ADC(void);
void Stop_ADC(void);

// OFFSET READINGS ATTEMPT
//#define LINE_THRESHOLD  ((LEFT_BLACK_VALUE + LEFT_WHITE_VALUE) / 2)			// Single threshold
//#define LEFT_ON_BLACK   (LEFT_corrected < LINE_THRESHOLD)					// Detection
//#define RIGHT_ON_BLACK  (RIGHT_corrected < LINE_THRESHOLD)
//
//int sensor_offset = (LEFT_WHITE_VALUE - RIGHT_WHITE_VALUE) / 2;				// Calculate offset 
//int LEFT_corrected = ADC_Left_Detect - sensor_offset;						// Corrected sensor values
//int RIGHT_corrected = ADC_Right_Detect + sensor_offset;


#endif /* ADC_H_ */
