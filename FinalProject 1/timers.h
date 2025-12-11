/*
 * timers.h
 *
 *  Created on: Oct 2, 2025
 *      Author: Dallas.Owens
 */

#ifndef TIMERS_H_
#define TIMERS_H_


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
#define TB0CCR0_INTERVAL		(50000) // (100ms)			// TIMER B0.0 INTERRUPT INTERVAL (50ms)	//  8,000,000 / 2 / 8 / (1 / 0.1s)
#define TB0CCR1_INTERVAL		(25000) // (50ms)			// TIMER B0.1 INTERRUPT INTERVAL (50ms)	//  8,000,000 / 2 / 8 / (1 / 0.05s)
#define TB0CCR2_INTERVAL		(25000) // (50ms)	        // TIMER B0.2 INTERRUPT INTERVAL (50ms)	//  8,000,000 / 2 / 8 / (1 / 0.05s)

#define TB1CCR0_INTERVAL		(2500)	// (5ms) 			// TIMER B1.0 INTERRUPT INTERVAL (5ms)	//  8,000,000 / 2 / 8 / (1 / 0.005s)
#define TB1CCR1_INTERVAL		(50000)	// (100ms) 			// TIMER B1.1 INTERRUPT INTERVAL (100ms)	//  8,000,000 / 2 / 8 / (1 / 0.1s)
#define TB1CCR2_INTERVAL		(50000)	// (100ms) 			// TIMER B1.2 INTERRUPT INTERVAL (100ms)	//  8,000,000 / 2 / 8 / (1 / 0.1s)

#define TB2CCR0_INTERVAL		(25000)  // (200ms)50=6250            // 6250 = (50MS)  12500 = (100MS)   25000 = (200MS)
#define TB2CCR1_INTERVAL		(12500)  // (100ms)
#define TB2CCR2_INTERVAL		(6250)   // (50ms)

// INTERRUPT ACCESSORIES

	// Timer B0:
#define B0_TICKS_PER_MS				(500)								// TICKS per MS (1MS = 500 TICKS AT 500 kHz INTERVAL)
#define ADC_SAMPLE_INTERVAL(ms)		((ms) * B0_TICKS_PER_MS)								// ADC SAMPLE DELAY IN MS

	// Timer B1:
#define B1_1_TICKS_PER_MS			(500)								// TICKS per MS (1MS = 500 TICKS AT 500 kHz INTERVAL)
#define B1_2_MS_PER_INTRPT			(100)
#define COMMAND_DELAY(ms)			((ms)/B1_2_MS_PER_INTRPT)

	// Timer B2:
#define B2_TICKS_PER_MS				(125)								// 25000 TICKS = 200MS -> 1MS = 125 TICKS
#define B2_0_MS_PER_INTRPT			(200)								// 100MS PER INTERRUPT for B2.0
#define DELAY_B2_0_MS(ms)			((ms)/B2_0_MS_PER_INTRPT)			// CONVERT MS TO TICK COUNT for B2.0 interrupt
#define B2_2_MS_PER_INTRPT			(200)								// 200MS PER INTERRUPT for B2.2
#define DELAY_B2_2_MS(ms)			((ms)/B2_2_MS_PER_INTRPT)			// CONVERT MS TO TICK COUNT for B2.2 interrupt
#define TRANSMIT_PERIOD				(DELAY_B2_0_MS(1000))				// SEND EVERY 1 SECOND

//#define DELAY_B2_1_MS(ms)           ((ms)/B2_1_MS_PER_INTRPT)           // CONVERT MS TO TICK COUNT for B2.0 interrupt
//#define B2_1_MS_PER_INTRPT          (100)                               // 100MS PER INTERRUPT for B2.0
//#define IOT_BOOT_DELAY              (DELAY_B2_1_MS(500))

//#define IR_LEAD_MS					(5)									// IR LEAD TIME IN MS
//#define IR_OFFSET_TICKS				(IR_LEAD_MS * B1_TICKS_PER_MS)		// IR LEAD TIME IN TICKS	(IR_LEAD_MS * B0_TICKS_PER_MS) // IR LEAD TIME IN TICKS	



// TIMER MACROS
#define MS_PER_INTFLAG			(100)						// MS per each clock tick
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


#endif /* TIMERS_H_ */
