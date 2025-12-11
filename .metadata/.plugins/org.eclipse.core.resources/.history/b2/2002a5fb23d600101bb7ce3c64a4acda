/*
 * wheels.h
 *
 *  Created on: Nov 30, 2025
 *      Author: Dallas.Owens
 *
 *  Description: Header file for line following state machine
 */

#ifndef WHEELS_H_
#define WHEELS_H_

//==============================================================================
// EXTERNAL VARIABLES
//==============================================================================
extern volatile unsigned char process_line_follow;
extern volatile unsigned int drive_timer;

#define DEADZONE                    (50)        // Previously 100   // Sensor deadzone threshold
#define Kp                          (20)        // 10 worked pretty well
#define SHARP_TURN_THRESHOLD        (150) 


#define MAX_AUTO_SPEED              (LINE_SPEED)   // Max speed during autonomous line following
#define MIN_AUTO_SPEED              (CRAWL)       // Min speed during autonomous line following
#define MAX_ERROR                    (300)          // 600 previously // Max sensor error for speed scaling

//==============================================================================
// FUNCTION PROTOTYPES
//==============================================================================

// Main line following functions
void Line_Follow_Start_Autonomous(void);    // Entry point - start autonomous mode (command T)
void Line_Follow_Process(void);             // Main state machine (call from main loop)
void Line_Follow_Exit_Circle(void);         // IoT command to exit circle (command X)
void Line_Follow_Stop(void);                // Emergency stop


#endif /* WHEELS_H_ */
