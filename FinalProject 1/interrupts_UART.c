/*
 * interrupts_UART.c
 *
 *  Created on: Oct 29, 2025
 *      Author: Dallas.Owens
 */

#include  "timers.h"
#include "msp430.h"
#include "ports.h"
#include "macros.h"
#include "functions.h"
#include  <string.h>
#include  "LED.h"

 // ************ MY VARIABLES *******************


// Globals
extern volatile unsigned char command_index;                        // Current position in command buffer
extern volatile unsigned char command_ready;                        // Command ready to process

// Serial
volatile unsigned char IOT_2_PC[SMALL_RING_SIZE];
volatile unsigned int iot_rx_wr = 0;
unsigned int iot_rx_rd = 0;         // Only used in Main
unsigned int direct_iot = 0;        // Only used it Interrupt

volatile unsigned char SEND_2_IOT[SMALL_RING_SIZE];
volatile unsigned int usb_rx_wr = 0;
unsigned int usb_rx_rd = 0;         // Only used in Main
unsigned int direct_usb = 0;        // Only used it Interrupt

volatile unsigned int temp;
volatile unsigned int temp_rx;
volatile unsigned char allow_comms = FALSE;                         // Prevent communications until first char received from PC

// IOT TX buffer (for UCA0 TX)
//extern volatile unsigned char SEND_2_IOT[PROCESS_BUFFER_SIZE];
//extern volatile unsigned int iot_tx_index;
//extern volatile unsigned int iot_tx_len;

// FRAM-caret command globals
volatile unsigned char command_mode = FALSE;                        // 0=pass-through, 1=command active
volatile unsigned char command_buffer[PROCESS_BUFFER_SIZE];         // Store incoming command

extern volatile unsigned char send_ping;
extern volatile unsigned int ping_time;

volatile unsigned int command_timeout = 0;
#define COMMAND_TIMEOUT_MAX  50  // 50 * 100ms = 5 seconds


//                  -------------------------------
//                  |  UART IMPLEMENTATION NOTES: |  
// --------------------------------------------------------------------------
// - UCA1 is connected to USB (PC)
// 	    - UCA1 RX 
//          - receives from PC
//          - stores in PC_2_IOT
//          - sends to UCA0 TX (to IOT)
//      - UCA1 TX sends to PC from FRAM
// 		    - reveives from UCA0 RX IOT via IOT_2_PC
//          - tracked with direct_usb
//          - sends to PC
//
// - UCA0 is connected to IOT module
//      - UCA0 RX 
// 		    - receives from IOT
//  	    - stores in IOT_2_PC
//          - sends to UCA1 TX (to PC)
// 	    - UCA0 TX sends to IOT
//		    - receives from UCA1 RX PC via PC_2_IOT
// 		    - tracked with direct_iot
// 	        - sends to IOT
//--------------------------------------------------------------------------
// FLOW:
//      - PC <-> UCA1 <-> UCA0 <-> IOT
// 
//      - PC:           transmits to UCA1 RX   -  receives from UCA1 TX
//      - UCA1 RX:      recieves from PC       -  transmits to UCA0 TX
//      - UCA0 TX:      recieves from UCA1 RX  -  transmits to IOT
//      - IoT:          receives from UCA0 TX  -  transmits to UCA0 RX
// 	    - UCA0 RX:      receives from IOT      -  transmits to UCA1 TX
//      - UCA1 TX:      receives from UCA0 RX  -  transmits to PC
// 
// 
// - PC -> UCA1 RX -> USB_Ring_Rx -> UCA0 TX -> IOT
// - IOT -> UCA0 RX -> IOT_2_PC -> UCA1 TX -> PC
//--------------------------------------------------------------------------


#pragma vector=EUSCI_A0_VECTOR
__interrupt void eUSCI_A0_ISR(void)
{
    switch (__even_in_range(UCA0IV, 0x08))
    {
    case 0: break;

        // RXIFG: UCA0 RECEIVE FROM IOT
        // IOT -> UCA0 RX -> IOT_2_PC -> UCA1 TX [ -> PC]
    case 2:
    { // RXIFG: UCA0 RECEIVE FROM IOT
        if (!allow_comms) { break; }				   // Prevent receiving from IOT until allowed
        temp = iot_rx_wr++;
        IOT_2_PC[temp] = UCA0RXBUF;                     // Rx -> IOT_2_PC character array
        if (iot_rx_wr >= (sizeof(IOT_2_PC))) {
            iot_rx_wr = BEGINNING;                      // Circular buffer back to beginning
        }
        UCA1IE |= UCTXIE;                               // Enable A1 Tx interrupt
    } break;

    case 4:
    { // TXIFG: UCA0 SEND TO IOT [received from A1 Rx]
        // 1. In Main:
        //   - Add message to IOT_TX_Buffer
        //   - Set index to 0 and len to string len
        //   - Enable UCA0 TX interrupt (UCA0IE |= UCTXIE)
        if (!allow_comms) { break; } 				                            // Prevent sending to IOT until allowed

        UCA0TXBUF = SEND_2_IOT[direct_iot++];                                   // Send next character to IOT and increment index

        if (direct_iot >= sizeof(SEND_2_IOT)) { direct_iot = BEGINNING; } 		// Circular buffer back to beginning

        if (direct_iot == usb_rx_wr) { UCA0IE &= ~UCTXIE; }                     // Done sending
    } break;


    default:
        break;
    }
}


#pragma vector=EUSCI_A1_VECTOR
__interrupt void eUSCI_A1_ISR(void)
{
    switch (__even_in_range(UCA1IV, 0x08))
    {
    case 0: break;
		// RXIFG: UCA1 RECEIVE FROM PC
        // PC -> UCA1 RX -> PC_2_IOT -> UCA0 TX [ -> IOT]
    case 2: { // RXIFG: UCA1 RECEIVE FROM PC
        if (!allow_comms) {
            allow_comms = TRUE;                                     // Allow communications after first char received from PC
        }
        temp_rx = UCA1RXBUF;                                        // Echo back to PC
//      UCA1TXBUF = temp_rx;                                        // enable if desired Echo back to PC immediately

        if (temp_rx == COMMAND_PREFIX) {                            // First '^' - enter command mode

            if (!command_mode) {
                command_mode = 1;
                command_index = 0;
                command_buffer[command_index++] = temp_rx;              // Store the '^'
            } else {                                                // Second '^' in sequence - special case
                command_buffer[command_index++] = temp_rx;
                // Check for "^^" command immediately
                if (command_index == 2 && command_buffer[0] == '^' && command_buffer[1] == '^') {
                    command_ready = 1;                              // Process immediately
                }
            }
        } 
        else if (command_mode) {                                    // Command Mode Active - store character

            if (command_index < sizeof(command_buffer) - 1) {
                command_buffer[command_index++] = temp_rx;
            }

            if (temp_rx == COMMAND_TERMINATOR) {                    // Check for command termination
                command_buffer[command_index] = '\0';                   // Null terminate
                command_ready = 1;                                      // Command complete
                command_mode = 0;                                       // Exit command mode
            }
        }
        else {                                                  // Pass-Through Mode Active - send to IOT
            temp = usb_rx_wr++;
            SEND_2_IOT[temp] = temp_rx;
            if (usb_rx_wr >= sizeof(SEND_2_IOT)) {
                usb_rx_wr = BEGINNING;
            }
        UCA0IE |= UCTXIE;                                       // Enable A0 Tx interrupt
        }
     } break;

	case 4: { // TXIFG: UCA1 SEND TO PC  [received from A0 Rx]
		if (!allow_comms) { break; }                                // Prevent sending to PC until allowed
        UCA1TXBUF = IOT_2_PC[direct_usb++];
		//IOT_2_PC[direct_usb-1] = 0;                               // Clear after sending, advance index
        if (direct_usb >= (sizeof(IOT_2_PC))) {
            direct_usb = BEGINNING;                                 // Circular buffer back to beginning
        }
        if (direct_usb == iot_rx_wr) {
            UCA1IE &= ~UCTXIE;                                      // Disable A1 TX interrupt
        }
    } break;

    default:
        break;
    }
}


