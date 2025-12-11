/*
 * DAC.h
 *
 *  Created on: Nov 27, 2025
 *      Author: Dallas.Owens
 *
 *  Description: Header file for DAC (Digital to Analog Converter)
 */

#ifndef DAC_H_
#define DAC_H_

// DAC Configuration Values
// Note: Lower values = Higher voltage (inverse relationship in buck-boost converter)

// Safety Values
#define DAC_ABSOLUTE_MAX    (725)		// 6.53V - DO NOT EXCEED - Absolute hardware limit
#define DAC_SAFE_MAX        (875)       // 6.05V - Safe operating maximum
#define DAC_MIN 			(2100)      // Minimum DAC value	

// DAC Voltage Presets for Motor Control
#define DAC_MOTOR_OFF   DAC_BEGIN		// Motors off: 2V
//#define DAC_MOTOR_OFF       (2000)      // 2.49V - Motors stopped
#define DAC_MOTOR_CRAWL     (1800)      // 3.44V - Crawl speed
#define DAC_MOTOR_SLOW      (1500)      // 4.08V - Slow/precise movement
#define DAC_MOTOR_MEDIUM    (1200)      // 5.03V - Medium speed
#define DAC_MOTOR_FAST      (975)       // 5.80V - Fast speed

// Line Following Speed Settings
//#define DAC_LINE_SEARCH     (1500)      // 4.08V - Slow search speed
//#define DAC_LINE_FOLLOW     (1200)      // 5.03V - Normal following speed
//#define DAC_LINE_STRAIGHT   (975)       // 5.80V - Fast straight sections

// Voltage Ramping Step Size
#define DAC_STEP_SMALL      (25)				// ~0.1V per step
#define DAC_STEP_MEDIUM     (50)				// ~0.2V per step
#define DAC_STEP_LARGE      (100)				// ~0.4V per step
#define DAC_RAMP_STEP       (DAC_STEP_LARGE)

// OLD VALUES
#define DAC_BEGIN			(2150)      // Starting voltage: 2.0V
#define DAC_LIMIT			(950)       // Target voltage limit: 5.87V		(875 - 6.054V)
#define DAC_ADJUST			(975)       // Final adjusted voltage: 5.739V		(900 - 6.00V)

//#define DAC_MAX			(4095)      // Maximum 12-bit DAC value
//#define DAC_MIN			(0)         // Minimum DAC value
//#define DAC_RAMP_STEP		(100)       // Decrement step for voltage ramping

// Global Variables
extern volatile unsigned int DAC_data;
extern volatile unsigned char dac_voltage_set;


// Function Prototypes
void Init_DAC(void);
void DAC_Set_Voltage(unsigned int target_value);
unsigned int DAC_Get_Voltage(void);
unsigned char DAC_Ready(void);

//void DAC_Ramp_ISR(void);

void DAC_Increase_Speed(unsigned int target_dac);
void DAC_Decrease_Speed(unsigned int target_dac);
unsigned int DAC_Battery_Adjust(unsigned int V_BAT_current);

#endif /* DAC_H_ */
