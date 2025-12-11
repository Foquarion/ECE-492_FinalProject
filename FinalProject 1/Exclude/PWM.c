/*
 * PWM.c
 *
 *  Created on: Oct 17, 2025
 *      Author: Dallas.Owens
 *  
 *  Description: PWM motor control with direction change delay management
 *               Implements 500ms delay between direction changes and 50ms
 *               reverse pulse for line intercept braking.
 */

#include "timers.h"
#include "macros.h"
#include "functions.h"
#include "msp430.h"
#include "LED.h"
#include "PWM.h"

// Track motor directions
static unsigned char left_motor_direction = MOTOR_OFF;
static unsigned char right_motor_direction = MOTOR_OFF;
volatile unsigned char ebraking = FALSE;
volatile unsigned char safety_stop = FALSE;




void PWM_EBRAKE(void){
    PWM_REVERSE();
    ebraking = TRUE;
    TB1CCTL1 &= ~CCIFG;                     // Clear possible pending interrupt
    TB2CCR2 = TB2R + TB2CCR2_INTERVAL;      // Set first interrupt
    TB2CCTL2 |= CCIE;                       // Enable TB2 CCR2 interrupt
}


//==============================================================================
// FUNCTION: Init_PWM
// Initialize PWM system - all motors off
//==============================================================================
void Init_PWM(void) {
    PWM_PERIOD = WHEEL_PERIOD;

    RIGHT_FORWARD_SPEED = WHEEL_OFF;
    LEFT_FORWARD_SPEED = WHEEL_OFF;
    LEFT_REVERSE_SPEED = WHEEL_OFF;
    RIGHT_REVERSE_SPEED = WHEEL_OFF;
    
    left_motor_direction = MOTOR_OFF;
    right_motor_direction = MOTOR_OFF;
    
    LCD_BACKLITE_DIMING = LCD_BACKLITE_OFF;
}

void PWM_Opening_Curve_Left (void) {
    LEFT_REVERSE_SPEED = WHEEL_OFF;
    RIGHT_REVERSE_SPEED = WHEEL_OFF;

    LEFT_FORWARD_SPEED = LINE_SPEED;
    RIGHT_FORWARD_SPEED = SLOW;
}

void PWM_Opening_Curve_Right(void) {
    LEFT_REVERSE_SPEED = WHEEL_OFF;
    RIGHT_REVERSE_SPEED = WHEEL_OFF;

    LEFT_FORWARD_SPEED = SLOW;
    RIGHT_FORWARD_SPEED = LINE_SPEED;
}

//==============================================================================
// FUNCTION: Set_Wheel_Speeds
// Low-level function to directly set PWM values
// Does NOT handle direction change delays - use higher level functions
//==============================================================================
void Set_Wheel_Speeds(unsigned int left_fwd, unsigned int left_rev,
                      unsigned int right_fwd, unsigned int right_rev) {
    LEFT_FORWARD_SPEED = left_fwd;
    LEFT_REVERSE_SPEED = left_rev;
    RIGHT_FORWARD_SPEED = right_fwd;
    RIGHT_REVERSE_SPEED = right_rev;
}

//==============================================================================
// FUNCTION: Apply_Direction_Change_Delay
// Handles 500ms delay when changing motor directions
// Returns: 1 if delay was applied, 0 if no delay needed
//==============================================================================
static unsigned char Apply_Direction_Change_Delay(unsigned char new_left_dir, 
                                                   unsigned char new_right_dir) {
    unsigned char delay_needed = 0;
    
    // Check if LEFT motor is changing direction (not just stopping)
    if (left_motor_direction != MOTOR_OFF && 
        new_left_dir != MOTOR_OFF &&
        left_motor_direction != new_left_dir) {
        delay_needed = 1;
    }
    
    // Check if RIGHT motor is changing direction (not just stopping)
    if (right_motor_direction != MOTOR_OFF &&
        new_right_dir != MOTOR_OFF &&
        right_motor_direction != new_right_dir) {
        delay_needed = 1;
    }
    
    // If direction change detected, stop motors and wait
    if (delay_needed) {
        // Stop all motors
        LEFT_FORWARD_SPEED = WHEEL_OFF;
        LEFT_REVERSE_SPEED = WHEEL_OFF;
        RIGHT_FORWARD_SPEED = WHEEL_OFF;
        RIGHT_REVERSE_SPEED = WHEEL_OFF;
        
        // Wait 500ms for motor to fully stop
        safety_stop = 1;
    }
    
    // Update direction tracking
    left_motor_direction = new_left_dir;
    right_motor_direction = new_right_dir;
    
    return delay_needed;
}

//==============================================================================
// FUNCTION: Wheels_Safe_Stop
// Safely stop all motors and reset direction tracking
//==============================================================================
void Wheels_Safe_Stop(void) {
    LEFT_FORWARD_SPEED = WHEEL_OFF;
    LEFT_REVERSE_SPEED = WHEEL_OFF;
    RIGHT_FORWARD_SPEED = WHEEL_OFF;
    RIGHT_REVERSE_SPEED = WHEEL_OFF;
    
    left_motor_direction = MOTOR_OFF;
    right_motor_direction = MOTOR_OFF;
}

//==============================================================================
// FUNCTION: Wheels_Intercept_Brake
// Special braking for line intercept - 50ms reverse pulse then stop
// ONLY use this when intercepting the line at high speed
//==============================================================================
// void Wheels_Intercept_Brake(void) {
//     // Determine current direction
//     unsigned char left_dir = left_motor_direction;
//     unsigned char right_dir = right_motor_direction;
    
//     // Apply 50ms reverse pulse (opposite of current direction)
//     if (left_dir == MOTOR_FORWARD || right_dir == MOTOR_FORWARD) {
//         // Currently going forward - pulse reverse
//         LEFT_FORWARD_SPEED = WHEEL_OFF;
//         RIGHT_FORWARD_SPEED = WHEEL_OFF;
//         LEFT_REVERSE_SPEED = SLOW;
//         RIGHT_REVERSE_SPEED = SLOW;
        
//         safety_stop = TRUE;
//     }
//     else if (left_dir == MOTOR_REVERSE || right_dir == MOTOR_REVERSE) {
//         // Currently going reverse - pulse forward
//         LEFT_REVERSE_SPEED = WHEEL_OFF;
//         RIGHT_REVERSE_SPEED = WHEEL_OFF;
//         LEFT_FORWARD_SPEED = SLOW;
//         RIGHT_FORWARD_SPEED = SLOW;
        
//         safety_stop = TRUE;
//     }
    
//     // Full stop
//     Wheels_Safe_Stop();
// }

//==============================================================================
// HIGH-LEVEL MOVEMENT FUNCTIONS WITH DIRECTION MANAGEMENT
//==============================================================================

//==============================================================================
// FUNCTION: PWM_FORWARD
// Move forward - handles direction change delay automatically
//==============================================================================
void PWM_FORWARD(void) {
    // Check and apply direction change delay if needed
//    Apply_Direction_Change_Delay(MOTOR_FORWARD, MOTOR_FORWARD);
    
    // Set motors to forward
    LEFT_REVERSE_SPEED = WHEEL_OFF;
    RIGHT_REVERSE_SPEED = WHEEL_OFF;
    LEFT_FORWARD_SPEED = SLOW;
    RIGHT_FORWARD_SPEED = FAST;
}

//==============================================================================
// FUNCTION: PWM_REVERSE  
// Move in reverse - handles direction change delay automatically
//==============================================================================
void PWM_REVERSE(void) {
    // Check and apply direction change delay if needed
//    Apply_Direction_Change_Delay(MOTOR_REVERSE, MOTOR_REVERSE);
    
    // Set motors to reverse
    LEFT_FORWARD_SPEED = WHEEL_OFF;
    RIGHT_FORWARD_SPEED = WHEEL_OFF;
    LEFT_REVERSE_SPEED = FAST;
    RIGHT_REVERSE_SPEED = FAST;
}

void PWM_REVERSE_PULSE(void) {
    // Set motors to reverse
    LEFT_FORWARD_SPEED = WHEEL_OFF;
    RIGHT_FORWARD_SPEED = WHEEL_OFF;
    LEFT_REVERSE_SPEED = LINE_SPEED;
    RIGHT_REVERSE_SPEED = LINE_SPEED;
}


//==============================================================================
// FUNCTION: PWM_FULLSTOP
// Stop all motors
//==============================================================================
void PWM_FULLSTOP(void) {
    Wheels_Safe_Stop();
}

//==============================================================================
// FUNCTION: PWM_TURN_RIGHT
// Turn right (left wheel faster) - handles direction change delay
//==============================================================================
void PWM_TURN_RIGHT(void) {
//    Apply_Direction_Change_Delay(MOTOR_FORWARD, MOTOR_FORWARD);
    
    LEFT_FORWARD_SPEED = SLOW;
    RIGHT_FORWARD_SPEED = WHEEL_OFF;
    LEFT_REVERSE_SPEED = WHEEL_OFF;
    RIGHT_REVERSE_SPEED = WHEEL_OFF;
}

//==============================================================================
// FUNCTION: PWM_TURN_LEFT
// Turn left (right wheel faster) - handles direction change delay
//==============================================================================
void PWM_TURN_LEFT(void) {
//    Apply_Direction_Change_Delay(MOTOR_FORWARD, MOTOR_FORWARD);
    
    LEFT_FORWARD_SPEED = WHEEL_OFF;
    RIGHT_FORWARD_SPEED = FAST;
    LEFT_REVERSE_SPEED = WHEEL_OFF;
    RIGHT_REVERSE_SPEED = WHEEL_OFF;
}

//==============================================================================
// FUNCTION: PWM_ALIGN_RIGHT
// Spin right in place (left forward, right reverse)
//==============================================================================
void PWM_ROTATE_RIGHT(void) {
//    Apply_Direction_Change_Delay(MOTOR_FORWARD, MOTOR_REVERSE);
    LEFT_REVERSE_SPEED = WHEEL_OFF;
    RIGHT_FORWARD_SPEED = WHEEL_OFF;

    LEFT_FORWARD_SPEED = LINE_SPEED;
    RIGHT_REVERSE_SPEED = LINE_SPEED;
}

//==============================================================================
// FUNCTION: PWM_ALIGN_LEFT
// Spin left in place (left reverse, right forward)
//==============================================================================
void PWM_ROTATE_LEFT(void) {
//    Apply_Direction_Change_Delay(MOTOR_REVERSE, MOTOR_FORWARD);
    
     LEFT_FORWARD_SPEED = WHEEL_OFF;
    RIGHT_REVERSE_SPEED = WHEEL_OFF;

    LEFT_REVERSE_SPEED = LINE_SPEED;
    RIGHT_FORWARD_SPEED = LINE_SPEED;
}

//==============================================================================
// FUNCTION: PWM_NUDGE_LEFT
// Nudge left (only right wheel moves forward)
//==============================================================================
void PWM_NUDGE_LEFT(void) {
    Apply_Direction_Change_Delay(MOTOR_OFF, MOTOR_FORWARD);
    
    LEFT_FORWARD_SPEED = WHEEL_OFF;
    RIGHT_FORWARD_SPEED = SLOW;
    LEFT_REVERSE_SPEED = WHEEL_OFF;
    RIGHT_REVERSE_SPEED = WHEEL_OFF;
}

//==============================================================================
// FUNCTION: PWM_NUDGE_RIGHT
// Nudge right (only left wheel moves forward)
//==============================================================================
void PWM_NUDGE_RIGHT(void) {
    Apply_Direction_Change_Delay(MOTOR_FORWARD, MOTOR_OFF);
    
    LEFT_FORWARD_SPEED = SLOW;
    RIGHT_FORWARD_SPEED = WHEEL_OFF;
    LEFT_REVERSE_SPEED = WHEEL_OFF;
    RIGHT_REVERSE_SPEED = WHEEL_OFF;
}

//==============================================================================
// IMMEDIATE FUNCTIONS (BYPASS DIRECTION MANAGEMENT)
// USE WITH EXTREME CAUTION - these bypass safety delays
//==============================================================================

//==============================================================================
// FUNCTION: PWM_FORWARD_IMMEDIATE
// Move forward immediately without direction change delay
// WARNING: Only use when you're certain no direction change is occurring
//==============================================================================
void PWM_FORWARD_IMMEDIATE(void) {
    LEFT_FORWARD_SPEED = SLOW;
    RIGHT_FORWARD_SPEED = SLOW;
    LEFT_REVERSE_SPEED = WHEEL_OFF;
    RIGHT_REVERSE_SPEED = WHEEL_OFF;
    
    left_motor_direction = MOTOR_FORWARD;
    right_motor_direction = MOTOR_FORWARD;
}

//==============================================================================
// FUNCTION: PWM_REVERSE_IMMEDIATE
// Move reverse immediately without direction change delay
// WARNING: Only use when you're certain no direction change is occurring
//==============================================================================
void PWM_REVERSE_IMMEDIATE(void) {
    LEFT_FORWARD_SPEED = WHEEL_OFF;
    RIGHT_FORWARD_SPEED = WHEEL_OFF;
    LEFT_REVERSE_SPEED = SLOW;
    RIGHT_REVERSE_SPEED = SLOW;
    
    left_motor_direction = MOTOR_REVERSE;
    right_motor_direction = MOTOR_REVERSE;
}
