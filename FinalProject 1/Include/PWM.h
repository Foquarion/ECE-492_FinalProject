/*
 * PWM.h
 *
 *  Created on: Nov 29, 2025
 *      Author: Dallas.Owens
 */

#ifndef PWM_H_
#define PWM_H_

// Direction change delay timing
#define DIRECTION_CHANGE_DELAY_MS    (500)   // 500ms delay when changing direction
#define INTERCEPT_BRAKE_PULSE_MS     (50)    // 50ms reverse pulse for intercept stop

// Motor direction states
#define MOTOR_OFF       (0)
#define MOTOR_FORWARD   (1)
#define MOTOR_REVERSE   (2)


extern volatile unsigned char safety_stop;



// Function prototypes
void Init_PWM(void);
void Set_Wheel_Speeds(unsigned int left_fwd, unsigned int left_rev, 
                      unsigned int right_fwd, unsigned int right_rev);
void Wheels_Safe_Stop(void);
void Wheels_Intercept_Brake(void);

// Higher-level movement functions with direction management
void PWM_FORWARD(void);
void PWM_REVERSE(void);
void PWM_FULLSTOP(void);
void PWM_TURN_RIGHT(void);
void PWM_TURN_LEFT(void);
void PWM_ROTATE_RIGHT(void);
void PWM_ROTATE_LEFT(void);
void PWM_NUDGE_RIGHT(void);
void PWM_NUDGE_LEFT(void);
void PWM_EBRAKE(void);
void PWM_REVERSE_PULSE(void);

// Direct motor control (bypasses direction management - use with caution)
void PWM_FORWARD_IMMEDIATE(void);
void PWM_REVERSE_IMMEDIATE(void);

#endif /* PWM_H_ */
