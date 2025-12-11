/*
 * macros.h
 *
 *  Created on: Sep 9, 2025
 *      Author: Dallas.Owens
 */

#ifndef MACROS_H_
#define MACROS_H_



// PREDEFINED MACROS
#define ALWAYS                  (1)
#define RESET_STATE             (0)
#define RED_LED					(0x01)			// RED LED 0      //DEFINED IN PORTS
#define GRN_LED					(0x40)			// GREEN LED 1    //DEFINED IN PORTS
#define TEST_PROBE				(0x01)			// 0 TEST PROBE   //DEFINED IN PORTS
#define TRUE					(0x01)
#define FALSE					(0x00)
#define IR_LED					(0x04)			// P2.2 - IR_LED
#define RESET_REGISTER          (0x0000)
#define LCD_BACKLITE_DURATION   (10)


// BOOT UP SEQUENCE MACROS
#define BOOT_INIT				(0)				// Initial pre-boot state
#define BOOT_LCD				(1)				// start lcd boot flag
#define BOOT_LCD_WAIT			(2)				// delay state for lcd
#define BOOT_IOT_ENABLE			(3)				// Boot the IOT module
#define BOOT_IOT_WAIT			(4)				// delay state for IOT
#define BOOT_DAC_ENABLE			(5)				// boot the DAC board
#define BOOT_DAC_INIT			(6)				// Initialize DAC
#define BOOT_DAC_WAIT			(7)				// Wait for DAC voltage ramp
#define BOOT_COMPLETE			(8)				// Boot finished


// SERIAL COMMUNICATIONS MACROS
#define BAUD_115200             (0)
#define BAUD_460800             (1)

#define BEGINNING 				(0)
#define SMALL_RING_SIZE			(32)
#define PROCESS_BUFFER_SIZE		(32)
#define PROCESS_BUFFER_LINES	(4)

#define COMMAND_BUFFER_SIZE		(20)
#define MAX_COMMANDS			(4)

#define COMMAND_PREFIX         ('^')
#define COMMAND_TERMINATOR     (0x0D)   // Carriage Return
#define PIN_CODE               ("^5115")



// CLOCK SETTING MACROS
#define USE_GPIO				(0x00)
#define USE_SMCLK				(0x01)

// -------------------- PWM Macros -------------------
// TIMER B3 MACORS ________________________
#define PWM_PERIOD              (TB3CCR0)
#define LEFT_FORWARD_SPEED      (TB3CCR1)
#define RIGHT_FORWARD_SPEED     (TB3CCR2)
#define LEFT_REVERSE_SPEED      (TB3CCR3)
#define RIGHT_REVERSE_SPEED     (TB3CCR4)
#define LCD_BACKLITE_DIMING     (TB3CCR5)

// PWM SETTINGS ___________________________

#define RIGHT_FAST              (LINE_SPEED)
#define LEFT_FAST               (LINE_SPEED)

//#define RIGHT_SLOW
//#define LEFT_SLOW
#define CAR_Left_Detect         (ADC_Right_Detect)
#define CAR_Right_Detect        (ADC_Left_Detect)

#define WHEEL_PERIOD			(50005)     // SETS PWM PERIOD FOR WHEELS (dont assign to speed variables, use below
#define WHEEL_OFF				(0)
#define TIPTOE                  (7500)      // Slow for precise movements
#define CRAWL					(10000)     // Very slow for precise movements
#define LINE_SPEED              (20000)
#define SLOW					(30000)
#define FAST					(45000)
#define PERCENT_100				(50000)
#define PERCENT_80				(45000)
#define PERCENT(per)			((per * 0.01) * PERCENT_100)			// CONVERT MS TO TICK COUNT (easier than doing math each time)

#define LCD_BACKLITE_ON			(PERCENT_80)
#define LCD_BACKLITE_DIM		(PERCENT(60))
#define LCD_BACKLITE_OFF		(0)


// TEST MACROS ______________________
#define TOGGLE_PROBE()			do{P3OUT ^= TEST_PROBE; } while(0)        // Toggle test probe to read on PCB test point
#define TOGGLE_BACKLITE()		do{P6OUT ^= LCD_BACKLITE; } while(0)      // Toggle LCD Backlight


// -------------------- StateMachine Macros -------------------
// States ----------------------
#define IDLE					('L')
#define WAIT					('M')
#define SET						('N')
#define SEEK					('S')
#define FOUND					('O')
#define ALIGN					('A')
#define NUDGE					('D')
#define BRAKE					('B')
#define STOP					('P')
#define EBRAKE 					('E')

#define FORWARD					('F')			// FORWARD
#define REVERSE					('R')			// REVERSE
#define OFF						('X')			// OFF

#define DRIVE_DELAY_MS(ms)      ((ms)/100)

//typedef enum { nudge_start, nudge_pulse, nudge_settle } nudge_steps;		// NUDGE steps

// TIMER MACROS
#define MS_PER_INTFLAG			(100)                    // MS per each clock tick
#define DELAY_MS(ms)			((ms)/MS_PER_INTFLAG)      // CONVERT MS TO TICK COUNT (easier than doing math each time)
//#define DELAY_1SEC				DELAY_MS(1000)          // 1 second counter
//#define DELAY_2SEC				DELAY_MS(2000)          // 2 second counter
//#define DELAY_3SEC				DELAY_MS(3000)          // 3 second counter

#define WAITING2START           (DELAY_1SEC)							// Delay after button tap
#define PAUSE_DELAY             (DELAY_MS(4000))

#define STOP_DELAY				(DELAY_2SEC)		// Pause time between shape change directions
#define FOUND_DELAY				(DELAY_1SEC)		// Pause time after line found before braking
#define BRAKE_DELAY			    (DELAY_MS(1000))		// Duration of braking before stopping
#define EBRAKE_DURATION			(DELAY_MS(50))		// Duration of electronic brake reverse pulse
#define NUDGE_PULSE_DURATION	(DELAY_MS(500))		// Duration of nudge forward pulse
#define NUDGE_SETTLE_DURATION	(DELAY_MS(500))		// Duration of nudge settle pause

#define IOT_BOOT_DELAY			(DELAY_MS(500))		// Delay time for IoT module to boot up
#define MESSAGE_DELAY			(DELAY_1SEC)		// Delay time between IOT messages

// ADC Macros ______________________
#define DETECT_THRESHOLD        (200)       // (500)    // Base Threshold value
#define DETECT_THRESH_LEFT		(200)	    // (500)    // Threshold for LEFT side line detection
#define DETECT_THRESH_RIGHT		(200)	    // (500)    // Threshold for RIGHT side line detection
#define HORSE_GRENADES			(100)	    // (100)    // (ALIGN) Max detect difference considered close enough to line for horseshoe and hand grenade aligning
#define HORSE_NUDGE				(50)	    // (50)     // (NUDGE) Max detect difference considered close enough to line for horseshoe and hand grenade nudging


// Direction Macros ________________
#define R_FORWARD_DIR	        ('F')
#define L_FORWARD_DIR	        ('F')

#define R_REVERSE_DIR	        ('R')
#define L_REVERSE_DIR	        ('R')

#define R_BRAKE_DIR             ('X')
#define L_BRAKE_DIR             ('X')

#define R_TURN_LEFT_DIR         ('F')
#define L_TURN_LEFT_DIR         ('F')

#define R_TURN_RIGHT_DIR        ('F')
#define L_TURN_RIGHT_DIR        ('F')

#define R_ALIGN_LEFT_DIR        ('F')
#define L_ALIGN_LEFT_DIR        ('R')

#define R_ALIGN_RIGHT_DIR       ('R')
#define L_ALIGN_RIGHT_DIR       ('F')

#define R_NUDGE_LEFT_DIR        ('F')
#define L_NUDGE_LEFT_DIR        ('F')

#define R_NUDGE_RIGHT_DIR       ('F')
#define L_NUDGE_RIGHT_DIR       ('F')


#endif /* MACROS_H_ */
