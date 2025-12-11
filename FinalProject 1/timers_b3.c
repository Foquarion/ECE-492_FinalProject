/*
 * timers_b3.c
 *
 *  Created on: Oct 17, 2025
 *      Author: Dallas.Owens
 */

#include  "timers.h"
#include "macros.h"
#include  "functions.h"
#include  "msp430.h"

 // REFERENCE: ________________________________
//#define WHEEL_PERIOD        (50005)
//#define WHEEL_OFF           (0)
//#define SLOW                (35000)
//#define FAST                (50000)
//#define PERCENT_100         (50000)
//#define PERCENT_80          (45000)
//#define PERCENT(per)        ((per * 0.01) * FAST)      // CONVERT MS TO TICK COUNT (easier than doing math each time)



void Init_Timer_B3(void)
{ //------------------------------------------------------------------------------
// SMCLK source, up count mode, PWM Right Side
// TB3.1    -    P6.0 L_FORWARD
// TB3.2    -    P6.1 R_FORWARD
// TB3.3    -    P6.2 L_REVERSE
// TB3.4    -    P6.3 R_REVERSE
// TB3.5    -    P6.4 LCD_BACKLITE
//------------------------------------------------------------------------------
    TB3CTL = TBSSEL__SMCLK;                 // SMCLK
    TB3CTL |= MC__UP;                       // Up Mode
    TB3CTL |= TBCLR;                        // Clear TAR

    PWM_PERIOD = WHEEL_PERIOD;              // PWM Period    [Set this to 50005]

    TB3CCTL1 = OUTMOD_7;                    // CCR1 reset/set
    LEFT_FORWARD_SPEED = WHEEL_OFF;         // P6.1 Left Forward PWM duty cycle

    TB3CCTL2 = OUTMOD_7;                    // CCR2 reset/set
    RIGHT_FORWARD_SPEED = WHEEL_OFF;        // P6.2 Right Forward PWM duty cycle

    TB3CCTL3 = OUTMOD_7;                    // CCR3 reset/set
    LEFT_REVERSE_SPEED = WHEEL_OFF;         // P6.3 Left Reverse PWM duty cycle

    TB3CCTL4 = OUTMOD_7;                    // CCR4 reset/set
    RIGHT_REVERSE_SPEED = WHEEL_OFF;        // P6.4 Right Reverse PWM duty cycle

    TB3CCTL5 = OUTMOD_7;                    // CCR5 reset/set
    LCD_BACKLITE_DIMING = PERCENT_80;       // P6.5 LCD_BACKLITE On Diming percent
//------------------------------------------------------------------------------
}
