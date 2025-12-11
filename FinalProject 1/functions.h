//******************************************************************************
//
//  Description: This file contains the Function prototypes
//
//  Created by: Jim Carlson
//  Aug 2013
// 
//  Modified for use by: Dallas Owens
//  9/2025
//******************************************************************************

#ifndef FUNCTIONS_H_
#define FUNCTIONS_H_

#include <stdbool.h>

//==============================================================================
// GLOBAL VARIABLE DECLARATIONS
//==============================================================================

extern char display_line[4][11];
extern char *display[4];
extern volatile unsigned char display_changed;
extern volatile unsigned char update_display;


//==============================================================================
// INITIALIZATION FUNCTIONS
//==============================================================================

void Init_Conditions(void);
void Init_Clocks(void);
void Init_Ports(void);
void Init_Port1(void);
void Init_Port2(void);
void Init_Port3(char SMCLK_SETTING);
void Init_Port4(void);
void Init_Port5(void);
void Init_Port6(void);
void Init_Timers(void);
void Init_Timer_B0(void);
void Init_Timer_B1(void);
void Init_Timer_B2(void);
void Init_Timer_B3(void);

void enable_interrupts(void);


//==============================================================================
// LED FUNCTIONS (led.c)
//==============================================================================

void LEDS_3_9_ON(void);
void LEDS_3_9_OFF(void);
void LED_Set_By_Index(unsigned char index, unsigned char on);

void Marquee_Start(void);
void Marquee_Stop(void);
void Marquee_Process(void);
unsigned char Marquee_Is_Running(void);

void Handle_SW1_Press(void);
void Handle_SW2_Press(void);


//==============================================================================
// SWITCH FUNCTIONS (switches.c)
//==============================================================================

void Switches_Process(void);


//==============================================================================
// INTERRUPT SERVICE ROUTINES
//==============================================================================

__interrupt void PORT2_ISR(void);
__interrupt void PORT3_ISR(void);

__interrupt void TB0_CCR0_ISR(void);
__interrupt void TB0_CCR1_CCR2_ISR(void);

__interrupt void TB2_CCR0_ISR(void);
__interrupt void TB2_CCR1_CCR2_ISR(void);


#endif /* FUNCTIONS_H_ */
