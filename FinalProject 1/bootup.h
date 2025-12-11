/*
 * bootup.h
 *
 *  Created on: Nov 27, 2025
 *      Author: Dallas.Owens
 * 
 *  Description: Header for unified boot sequence system
 */

#ifndef BOOTUP_H_
#define BOOTUP_H_

//==============================================================================
// BOOT SEQUENCE STATES
//==============================================================================
#define BOOT_INIT           (0)     // Initialize boot sequence
#define BOOT_LCD            (1)     // Power up LCD
#define BOOT_LCD_WAIT       (2)     // Wait for LCD stabilization (200ms)
#define BOOT_IOT_INIT       (3)     // Initialize UART and IoT module
#define BOOT_IOT_SEQUENCE   (4)     // Run IoT AT command sequence
#define BOOT_DAC_ENABLE     (5)     // Enable DAC power supply
#define BOOT_DAC_INIT       (6)     // Initialize DAC
#define BOOT_DAC_WAIT       (7)     // Wait for DAC voltage ramp
#define BOOT_COMPLETE       (8)     // Boot finished

//==============================================================================
// IOT BOOT SUB-STATES
//==============================================================================
// #define IOT_WAIT_READY      (0)     // Wait for "ready" message
// #define IOT_MUX_SET         (1)     // Send AT+CIPMUX=1
// #define IOT_MUX_OK          (2)     // Wait for OK response
// #define IOT_SERVER_SET      (3)     // Send AT+CIPSERVER=1,55155
// #define IOT_SERVER_OK       (4)     // Wait for OK response
// #define IOT_SSID_REQUEST    (5)     // Send AT+CWJAP?
// #define IOT_WAIT_SSID       (6)     // Wait for OK response
// #define IOT_REQUEST_IP      (7)     // Send AT+CIFSR
// #define IOT_WAIT_IP         (8)     // Wait for OK response
// #define IOT_BOOT_COMPLETE   (9)     // IoT boot finished

typedef enum{
    IOT_ENABLE,
    IOT_WAIT_READY,                 // Wait for "ready" message
    IOT_MUX_SET,                    // Send AT+CIPMUX=1
    IOT_MUX_OK,                     // Wait for OK response
    IOT_SERVER_SET,                 // Send AT+CIPSERVER=1,55155
    IOT_SERVER_OK,                  // Wait for OK response
    IOT_SSID_REQUEST,               // Send AT+CWJAP?
    IOT_SSID_WAIT,                  // Wait for OK response
    IOT_IP_REQUEST,                 // Send AT+CIFSR
    IOT_IP_WAIT,                    // Wait for OK response
    IOT_BOOT_COMPLETE
}iot_boot_state_t;

extern iot_boot_state_t iot_boot_state;

//==============================================================================
// GLOBAL VARIABLES
//==============================================================================
extern volatile unsigned char power_sequence;       // Current boot stage
extern volatile unsigned char boot_timer_flag;      // Timer flag (set by TB2 CCR0)
extern unsigned char boot_complete;                 // Boot completion flag
extern volatile unsigned char iot_boot_next;
extern volatile unsigned char init_iot_connection;
extern unsigned char iot_boot_complete;


//==============================================================================
// FUNCTION PROTOTYPES
//==============================================================================
void Bootup_Sequence(void);                           // Main boot state machine
void IOT_Connect(void);                           // Execute one IoT boot step


#endif /* BOOTUP_H_ */
