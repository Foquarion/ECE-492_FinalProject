/*
 * switches.h
 *
 *  Created on: Sep 21, 2025
 *      Author: Dallas.Owens
 */

#ifndef SWITCHES_H_
#define SWITCHES_H_

// Switch press flags (set by ISR, consumed by Switches_Process)
extern volatile unsigned int SW1_pressed;
extern volatile unsigned int SW2_pressed;

// Function prototype
void Switches_Process(void);

#endif /* SWITCHES_H_ */
