/*
 * led.h
 *
 *  Created on: Nov 27, 2025
 *      Author: Dallas.Owens
 */

#ifndef LED_H_
#define LED_H_

#include "ports.h"

//==============================================================================
// LED CONTROL MACROS
//==============================================================================

// Individual LED Control Macros for LEDs 3-9
// Port 6 LEDs (LED_3 through LED_7)
#define LED3_ON()               do { P6OUT |=  LED_3; } while(0)
#define LED3_OFF()              do { P6OUT &= ~LED_3; } while(0)

#define LED4_ON()               do { P6OUT |=  LED_4; } while(0)
#define LED4_OFF()              do { P6OUT &= ~LED_4; } while(0)

#define LED5_ON()               do { P6OUT |=  LED_5; } while(0)
#define LED5_OFF()              do { P6OUT &= ~LED_5; } while(0)

#define LED6_ON()               do { P6OUT |=  LED_6; } while(0)
#define LED6_OFF()              do { P6OUT &= ~LED_6; } while(0)

#define LED7_ON()               do { P6OUT |=  LED_7; } while(0)
#define LED7_OFF()              do { P6OUT &= ~LED_7; } while(0)

// Port 4 LEDs (LED_8 and LED_9)
#define LED8_ON()               do { P4OUT |=  LED_8; } while(0)
#define LED8_OFF()              do { P4OUT &= ~LED_8; } while(0)

#define LED9_ON()               do { P4OUT |=  LED_9; } while(0)
#define LED9_OFF()              do { P4OUT &= ~LED_9; } while(0)


//==============================================================================
// MARQUEE CONFIGURATION
//==============================================================================

// Number of LEDs in the sequence (LED_3 through LED_9)
#define NUM_MARQUEE_LEDS        (7)

// Marquee timing: 300ms delay between each LED
// Timer B2 CCR2 runs at 50ms intervals
// 300ms = 6 x 50ms ticks
#define MARQUEE_DELAY_TICKS     (6)


//==============================================================================
// FUNCTION PROTOTYPES
//==============================================================================

// Basic LED control
void LEDS_3_9_ON(void);
void LEDS_3_9_OFF(void);
void LED_Set_By_Index(unsigned char index, unsigned char on);

// Marquee control
void Marquee_Start(void);
void Marquee_Stop(void);
void Marquee_Process(void);
unsigned char Marquee_Is_Running(void);

// Switch handlers
void Handle_SW1_Press(void);
void Handle_SW2_Press(void);


#endif /* LED_H_ */
