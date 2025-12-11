/*
 * timers.h
 *
 *  Created on: Oct 2, 2025
 *      Author: Dallas.Owens
 */

#ifndef BACKUP_TIMERS_H_
#define BACKUP_TIMERS_H_


 // TIMER VECTOR NAMES
#define TIMER_B0_CCR0_VECTOR            (TIMER0_B0_VECTOR)
#define TIMER_BO_CCR1_2_0V_VECTOR       (TIMER0_B1_VECTOR)

#define TIMER_B1_CCR0_VECTOR            (TIMER1_B0_VECTOR)
#define TIMER_B1_CCR1_2_0V_VECTOR       (TIMER1_B1_VECTOR)

#define TIMER_B2_CCR0_VECTOR            (TIMER2_B0_VECTOR)
#define TIMER_B2_CCR1_2_0V_VECTOR       (TIMER2_B1_VECTOR)

#define TIMER_B3_CCR0_VECTOR            (TIMER3_B0_VECTOR)
#define TIMER_B3_CCR1_2_0V_VECTOR       (TIMER3_B1_VECTOR)

 // TIMER B0-B2 - INTERRUPT INTERVALS
#define TB0CCR0_INTERVAL		(25000) // (50ms)			// TIMER B0.0 INTERRUPT INTERVAL (50ms)	//  8,000,000 / 2 / 8 / (1 / 0.05s)
#define TB0CCR1_INTERVAL		(25000) // (50ms)			// TIMER B0.1 INTERRUPT INTERVAL (50ms)	//  8,000,000 / 2 / 8 / (1 / 0.05s)
#define TB0CCR2_INTERVAL		(25000) // (50ms)	        // TIMER B0.2 INTERRUPT INTERVAL (50ms)	//  8,000,000 / 2 / 8 / (1 / 0.05s)

#define TB1CCR0_INTERVAL		(2500)	// (5ms) 			// TIMER B1.0 INTERRUPT INTERVAL (5ms)	//  8,000,000 / 2 / 8 / (1 / 0.005s)
#define TB1CCR1_INTERVAL		(50000)	// (100ms) 			// TIMER B1.1 INTERRUPT INTERVAL (100ms)	//  8,000,000 / 2 / 8 / (1 / 0.1s)
#define TB1CCR2_INTERVAL		(50000)	// (100ms) 			// TIMER B1.2 INTERRUPT INTERVAL (100ms)	//  8,000,000 / 2 / 8 / (1 / 0.1s)

#define TB2CCR0_INTERVAL		(25000)  // (200ms)50=6250            // 6250 = (50MS)  12500 = (100MS)   25000 = (200MS)
#define TB2CCR1_INTERVAL		(12500)  // (100ms)
#define TB2CCR2_INTERVAL		(25000)  // (200ms)25000

// INTERRUPT ACCESSORIES
#define B0_TICKS_PER_MS			(500)								// TICKS per MS (1MS = 500 TICKS AT 500 kHz INTERVAL)
#define B1_TICKS_PER_MS			(500)								// TICKS per MS (1MS = 500 TICKS AT 500 kHz INTERVAL)
#define B2_TICKS_PER_MS         (125)

#define B1_2_MS_PER_INTRPT      (100)
#define COMMAND_DELAY(ms)      ((ms)/B1_2_MS_PER_INTRPT)

#define B2_0_MS_PER_INTRPT      (100)
#define B2_2_MS_PER_INTRPT      (200)

#define DELAY_B2_0_MS(ms)       ((ms)/B2_0_MS_PER_INTRPT)
#define DELAY_B2_2_MS(ms)       ((ms)/B2_2_MS_PER_INTRPT)
#define TRANSMIT_PERIOD         (DELAY_B2_0_MS(1000))

#define IR_LEAD_MS				(5)									// IR LEAD TIME IN MS
#define IR_OFFSET_TICKS			(IR_LEAD_MS * B1_TICKS_PER_MS)		// IR LEAD TIME IN TICKS	(IR_LEAD_MS * B0_TICKS_PER_MS) // IR LEAD TIME IN TICKS	


// TIMER MACROS
#define MS_PER_INTFLAG			(50)						// MS per each clock tick
#define DELAY_MS(ms)			((ms)/MS_PER_INTFLAG)		// CONVERT MS TO TICK COUNT (easier than doing math each time)

#define DELAY_200MS				( DELAY_MS( 200) )          // 0.2  second counter
#define DELAY_HALF_SEC			( DELAY_MS( 500) )          // 0.5  second counter
#define DELAY_1SEC				( DELAY_MS(1000) )          // 1    second counter
#define DELAY_2SEC				( DELAY_MS(2000) )          // 2    second counter
#define DELAY_3SEC				( DELAY_MS(3000) )          // 3    second counteR


// TIMER B3 - PWM
//#define WHEEL_PERIOD        (50005)
//#define WHEEL_OFF           (0)
//#define SLOW                (35000)
//#define FAST                (50000)
//#define PERCENT_100         (50000)
//#define PERCENT_80          (45000)
//#define PERCENT(per)        ((per * 0.01) * FAST)      // CONVERT MS TO TICK COUNT (easier than doing math each time)


// Carlson State Machine Conversions   // Replace old main cases with following if used
// (case1 * oldRAte) / NewRate
#define Time_Sequence_Rate          (50)                              // 50 milliseconds - New Rate
#define S1250      (1250 / Time_Sequence_Rate) //CSM 250     // 1.25 seconds     // = (250 * 5) / Rate
#define S1000      (1000 / Time_Sequence_Rate) //CSM 200     // 1    second         // = (200 * 5) / Rate
#define S750       (750 / Time_Sequence_Rate)  //CSM 150     // 0.75 seconds     // = (150 * 5) / Rate
#define S500       (500 / Time_Sequence_Rate)  //CSM 100     // 0.50 seconds     // = (100 * 5) / Rate
#define S250       (250 / Time_Sequence_Rate)  //CSM 50      // 0.25 seconds     // = (50 * 5) / Rate




// GLOBALS
//extern volatile unsigned int  Time_Sequence;
//extern volatile unsigned int  update_display;

//// Timer ID BITS Defines REFERENCE (INCLUDED BY TI)
//#define ID_1            (0x0040)         /* /2 */
//#define ID_2            (0x0080)         /* /4 */
//#define ID_3            (0x00c0)         /* /8 */

//#define ID__2           (0x0040)         /* /2 */
//#define ID__4           (0x0080)         /* /4 */
//#define ID__8           (0x00c0)         /* /8 */


//// Timer TBAIDEX Defines REFERENCE (INCLUDED BY TI)
//#define TBIDEX_0        (0x0000)        /* Divide by 1 */
//#define TBIDEX_1        (0x0001)        /* Divide by 2 */
//#define TBIDEX_2        (0x0002)        /* Divide by 3 */
//#define TBIDEX_3        (0x0003)        /* Divide by 4 */
//#define TBIDEX_4        (0x0004)        /* Divide by 5 */
//#define TBIDEX_5        (0x0005)        /* Divide by 6 */
//#define TBIDEX_6        (0x0006)        /* Divide by 7 */
//#define TBIDEX_7        (0x0007)        /* Divide by 8 */

//#define TBIDEX__1       (0x0000)        /* Divide by 1 */
//#define TBIDEX__2       (0x0001)        /* Divide by 2 */
//#define TBIDEX__3       (0x0002)        /* Divide by 3 */
//#define TBIDEX__4       (0x0003)        /* Divide by 4 */
//#define TBIDEX__5       (0x0004)        /* Divide by 5 */
//#define TBIDEX__6       (0x0005)        /* Divide by 6 */
//#define TBIDEX__7       (0x0006)        /* Divide by 7 */
//#define TBIDEX__8       (0x0007)        /* Divide by 8 */

//// _____________ TIMER DIVS DEFINES_____________
//#define DIVS                             (0x0030)
//#define DIVS_L                           (0x0030)
//#define DIVS0                            (0x0010)
//#define DIVS0_L                          (0x0010)
//#define DIVS1                            (0x0020)
//#define DIVS1_L                          (0x0020)
//#define DIVS_0                           (0x0000)        // /1
//#define DIVS_1                           (0x0010)        // /2
//#define DIVS_1_L                         (0x0010)
//#define DIVS_2                           (0x0020)        // /4
//#define DIVS_2_L                         (0x0020)
//#define DIVS_3                           (0x0030)        // /8
//#define DIVS_3_L                         (0x0030)
//#define DIVS__1                          (0x0000)        // /1
//#define DIVS__2                          (0x0010)        // /2
//#define DIVS__2_L                        (0x0010)
//#define DIVS__4                          (0x0020)        // /4
//#define DIVS__4_L                        (0x0020)
//#define DIVS__8                          (0x0030)        // /8
//#define DIVS__8_L                        (0x0030)
////______________________________________________


#endif /* BACKUP_TIMERS_H_ */
