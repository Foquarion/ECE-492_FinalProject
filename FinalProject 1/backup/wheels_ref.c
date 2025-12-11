/*
 * wheels_ref.c
 *
 *  Created on: Dec 2, 2025
 *      Author: Dallas.Owens
 */



//===============================================================
//  File Name: wheels.c
//  Description: This file contains the functions needed for control
//                    operations of the wheels
//
//  Author: Sam Messick
//  Date Feb. 2018
//  Compiler Build with IAR Embedded Workbench Version: V4.10A/W32 (7.11.2)
//===============================================================

#include "wheels.h"

// H-bridge protection flags:
// don't allow for motion forward/backward simultaneously in one motor

uint8_t left_forward_flag;     // Is left motor rotating forward?
uint8_t left_reverse_flag;     // Is left motor rotating reverse?
uint8_t right_forward_flag;    // Is right motor rotating forward?
uint8_t right_reverse_flag;    // Is right motor rotating reverse?
uint8_t instruction_label_tmp; // Global of current instruction step
uint8_t LastDir = 3;

//--------PID fields-----------//
int16_t   right_pwm;
int16_t   left_pwm;
PID ld; // Left infrared detector
PID rd;
//-----------------------------//


void init_PID(void){
  /*
  ld.windup_guard = L_PWM_MAX;
  rd.windup_guard = R_PWM_MAX;
  ld.p_gain = 1.5;//((double)(ADC_Thumb))/1000;     // 1.15
  ld.i_gain = 0;
  ld.d_gain = 6;//1.5;
  rd.p_gain = 3;//((double)(ADC_Thumb))/1000;
  rd.i_gain = 0;
  rd.d_gain = 6;//1.5;
  right_pwm = RIGHT_FORWARD_SPEED;
  left_pwm = LEFT_FORWARD_SPEED;
*/
}

void pid_udpate_left(void){

  if(ADC_Left_Detector >= ir_black_reading)
    if(ADC_Right_Detector >= ir_black_reading)
    {
      Left_Motor_ON_FORWARD(LEFT_FORWARD_SPEED1);
      Right_Motor_ON_FORWARD(RIGHT_FORWARD_SPEED1);
    }
    else
    {
      Left_Motor_ON_FORWARD(LEFT_L_SEARCH_SPEED);
      Right_Motor_ON_FORWARD(RIGHT_L_SEARCH_SPEED);
      LastDir = GOING_LEFT;
    }
  else
    if(ADC_Right_Detector >= ir_black_reading)
    {
      Left_Motor_ON_FORWARD(LEFT_R_SEARCH_SPEED);
      Right_Motor_ON_FORWARD(RIGHT_R_SEARCH_SPEED);
      LastDir = GOING_RIGHT;
    }
    else
    {
      if(LastDir == GOING_RIGHT)
      {
        Left_Motor_ON_FORWARD(LEFT_R_SEARCH_SPEED);
        Right_Motor_ON_FORWARD(RIGHT_R_SEARCH_SPEED);
      }
      else if(LastDir == GOING_LEFT)
      {
        Left_Motor_ON_FORWARD(LEFT_L_SEARCH_SPEED);
        Right_Motor_ON_FORWARD(RIGHT_L_SEARCH_SPEED);
      }
      else
      {
        Left_Motor_ON_FORWARD(LEFT_FORWARD_SPEED);
        Right_Motor_ON_FORWARD(RIGHT_FORWARD_SPEED);
      }
    }
  /*
  // Attempted PID
   Uses a PID with noise bands at ir_white_reading and
   ir_black_reading, adjusting for changes to follow
   a black line.

  // Take NUM_SAMPLES from PID in pid_samples for averaging
  static uint8_t samp_index;
  static int16_t[NUM_SAMPLES] pid_error_l_samples;
  static int16_t error_l;
  static int16_t d_error_l;

  int16_t error_l_w;
  int16_t error_l_b;

  // Update PID errors
  error_l_w = ADC_Left_Detector - ir_white_reading;
  error_l_b = ADC_Left_Detector - ir_black_reading;

  // Check if error falls outside of noise bands
  if(error_l_w < 0)
  {
    error_l = error_l_w;
    pid_error_l_samples
  }
  else if(error_l_b > 0)
    error_l = error_l_b;
  else
    return;



  if(error_l IS_POSITIVE)
    P1OUT  |= RED_LED;
  else
    P1OUT  &= ~RED_LED;
  ld.i_error = error_l + ld.i_error;
  d_error_l = error_l - ld.prev_error;
  if(d_error_l IS_POSITIVE)
    P1OUT  |= GRN_LED;
  else
    P1OUT  &= ~GRN_LED;

  if(ld.i_error > ld.windup_guard)
    ld.i_error = ld.windup_guard;
  else if(ld.i_error < -ld.windup_guard)
    ld.i_error = -ld.windup_guard;

  // Calculate adjusted pwm
  left_pwm =  (uint8_t)(LEFT_FORWARD_SPEED    -
              (ld.p_gain * error_l) -
              (ld.i_gain * ld.i_error) -
              (ld.d_gain * d_error_l));
  right_pwm = (uint8_t)(RIGHT_FORWARD_SPEED  +
              (rd.p_gain * error_l) +
              (rd.i_gain * ld.i_error) +
              (rd.d_gain * d_error_l));
  if(left_pwm > L_PWM_MAX)
    left_pwm = L_PWM_MAX;
  if(left_pwm < L_PWM_MIN)
    left_pwm = L_PWM_MIN;
  if(right_pwm > R_PWM_MAX)
    right_pwm = R_PWM_MAX;
  if(right_pwm < R_PWM_MIN)
    right_pwm = R_PWM_MIN;

  // Enable motor
  Left_Motor_ON_FORWARD(left_pwm);
  Right_Motor_ON_FORWARD(right_pwm);

  ld.prev_error = error_l;
*/
}

void initialize_delay(uint16_t delay){
  delay_time = delay;                          // send delay time to global accessible by timer A1
  TA0CCTL2 |= CCIE;                            // enable timer A1.2 to count time
}

void update_instruction_label(uint8_t* instruction_label){
  (*instruction_label)++;                         // Set up next instruction
  instruction_label_tmp = *instruction_label;   // Set static instruction visable by menu to update LCD
}

void WHEELS_test(void);
//------------------------
// Turning wheels off
//------------------------
void Left_Motor_OFF(void){
  TB0CCR3 = MOTOR_OFF;         // Set PWM to 0 percent duty cycle
  TB0CCR4 = MOTOR_OFF;
  left_forward_flag = left_reverse_flag = false;
}
void Right_Motor_OFF(void){
  TB0CCR5 = MOTOR_OFF;         // Set PWM to 0 percent duty cycle
  TB0CCR6 = MOTOR_OFF;
  right_forward_flag = right_reverse_flag = false;
}
void Wheels_OFF(void){
  Left_Motor_OFF();
  Right_Motor_OFF();
}

//------------------------
// Turning wheels on
//------------------------
//**********
// Forward *
//**********
void Left_Motor_ON_FORWARD(uint8_t speed){
  if(!left_reverse_flag)                         // If left motor is not set to move backward already
  {
    TB0CCR4 = speed*DUTY_CYCLE_DIVISOR;         // Set left motor forward speed as one of 255 settings
    left_forward_flag = true;                   // Prevent left motor from moving backward simultaneously
  }
}

void Right_Motor_ON_FORWARD(uint8_t speed){
  if(!right_reverse_flag)                        // If right motor is not set to move backward already
  {
    TB0CCR6 = speed*DUTY_CYCLE_DIVISOR;         // Set right motor forward speed as one of 255 settings
    right_forward_flag = true;                  // Prevent right motor from moving backward simultaneously
  }
}

void Drive_Straight_Forward(uint8_t speed){
  Left_Motor_ON_FORWARD(speed);
  Right_Motor_ON_FORWARD(speed);
}

//**********
// Reverse *
//**********
void Left_Motor_ON_REVERSE(uint8_t speed){
  if(!left_forward_flag)                        // If left motor is not set to move forward already
  {
    TB0CCR3 = speed*DUTY_CYCLE_DIVISOR;         // Set left motor reverse speed as one of 255 settings
    left_reverse_flag = true;                   // Prevent left motor from moving forward simultaneously
  }
}

void Right_Motor_ON_REVERSE(char speed){
  if(!right_forward_flag)                       // If right motor is not set to move forward already
  {
    TB0CCR5 = speed*DUTY_CYCLE_DIVISOR;         // Set right motor reverse speed as one of 255 settings
    right_reverse_flag = true;                  // Prevent right motor from moving forward simultaneously
  }
}

void turn(uint8_t degrees){
  uint32_t time_ms_temp;

  if(degrees == LEAVE_COURSE)
  {
    time_ms_temp = time_ms;
    LCD_print(" Exiting  ", word2, word3, word4);
    Left_Motor_ON_FORWARD(LEFT_FORWARD_SPEED);
    Right_Motor_ON_FORWARD(RIGHT_FORWARD_SPEED);
    while(time_ms - time_ms_temp < EXIT_TIME);     // Drive forward for x seconds

    Wheels_OFF();                                  // Stop car
    TA0CCTL0 &= ~CCIE;                             // Disable timer interrupts
    TA0CCTL1 &= ~CCIE;                             // Disable button interrupts
    LCD_print("P10 Finit ", word2, "  ^____^  ", "          ");
    waiting = true;
    while(waiting);                                // Do nothing
  }
  else
  {
    delay_time = LONG_DELAY;
    Wheels_OFF();
    waiting = true;
    TA0CCTL2 |= CCIE;
    while(waiting);
    delay_time = SHORT_DELAY;
    LCD_print("", word2, word3, word4);
    Left_Motor_ON_FORWARD(LEFT_FORWARD_SPEED);
    Right_Motor_ON_FORWARD(RIGHT_FORWARD_SPEED);
    waiting = true;
    TA0CCTL2 |= CCIE;
    while(waiting);
    delay_time = SHORT_DELAY;
    Wheels_OFF();
    waiting = true;
    TA0CCTL2 |= CCIE;
    while(waiting);
    delay_time = LONG_DELAY;
    LCD_print("Turning in", word2, word3, word4);
    Left_Motor_ON_REVERSE(LEFT_SPIN_SPEED);
    Right_Motor_ON_FORWARD(RIGHT_SPIN_SPEED);
    waiting = true;
    TA0CCTL2 |= CCIE;
    while(waiting);
    delay_time = SHORT_DELAY;
    Wheels_OFF();
    waiting = true;
    TA0CCTL2 |= CCIE;
    while(waiting);
  }
}

void drive_to_black_line(void){
  uint32_t time_ms_temp = time_ms;
  Wheels_OFF();
  LCD_print("Searching", word2, word3, word4);

  Left_Motor_ON_FORWARD(LEFT_FORWARD_SPEED);
  Right_Motor_ON_FORWARD(RIGHT_FORWARD_SPEED);
  while(time_ms - time_ms_temp < TWO_SEC);
  LCD_print("SearchingW", word2, word3, word4);
  while(ADC_Left_Detector > ir_white_reading          // Drive to white surface
     || ADC_Right_Detector > ir_white_reading)
  {
    update_ir_reading();
  }
  LCD_print("SearchingB", word2, word3, word4);
  while(ADC_Left_Detector < ir_black_reading          // Drive to black line
     && ADC_Right_Detector < ir_black_reading)
  {
    update_ir_reading();
  }
  Wheels_OFF();                                       // Stop car on black line
}









    //Backup Code
  /*
  if(ADC_Left_Detector < ir_active_reading && ADC_Right_Detector > ir_active_reading)
  {
    left_pwm += 120;
    if(left_pwm > L_PWM_MAX)
    left_pwm = L_PWM_MAX;
    Left_Motor_ON_FORWARD(left_pwm);
    right_pwm -= 100;
    if(right_pwm < R_PWM_MIN)
    right_pwm = R_PWM_MIN;
    Right_Motor_ON_FORWARD(right_pwm);
  }
  else
  {
    right_pwm += 160;
    if(right_pwm > R_PWM_MAX)
    right_pwm = R_PWM_MAX;
    Right_Motor_ON_FORWARD(right_pwm);
    left_pwm -= 120;
    if(left_pwm < L_PWM_MIN)
    left_pwm = L_PWM_MIN;
    Left_Motor_ON_FORWARD(left_pwm);
  }
*/
