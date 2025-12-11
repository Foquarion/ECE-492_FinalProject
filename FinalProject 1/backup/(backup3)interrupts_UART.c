/*
 * interrupts_UART.c
 *
 *  Created on: Oct 29, 2025
 *      Author: Dallas.Owens
 */

#include "msp430.h"
#include "timers.h"
#include "ports.h"
#include "macros.h"
#include "functions.h"
#include  <string.h>

 // ************ MY VARIABLES *******************
//#define BEGINNING 				(0)
//#define SMALL_RING_SIZE			(16)
//#define LARGE_RING_SIZE			(16)



// Globals

// Serial
volatile unsigned char IOT_2_PC[SMALL_RING_SIZE];
volatile unsigned int iot_rx_wr;
unsigned int iot_rx_rd;   // Only used in Main 
unsigned int direct_iot;  // Only used it Interrupt

volatile unsigned char SEND_2_IOT[SMALL_RING_SIZE];
volatile unsigned int usb_rx_wr;
unsigned int usb_rx_rd;  // Only used in Main 
unsigned int direct_usb; // Only used it Interrupt

volatile unsigned int temp;
volatile unsigned int temp_rx;
volatile unsigned char allow_comms = FALSE;          // Prevent communications until first char received from PC



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
        IOT_2_PC[temp] = UCA0RXBUF;                 // Rx -> IOT_2_PC character array 
        if (iot_rx_wr >= (sizeof(IOT_2_PC))) {
            iot_rx_wr = BEGINNING;                  // Circular buffer back to beginning 
        }
        UCA1IE |= UCTXIE;                           // Enable A1 Tx interrupt    
    } break;

    case 4: 
    { // TXIFG: UCA0 SEND TO IOT [received from A1 Rx]
		if (!allow_comms) { break; } 				 // Prevent sending to IOT until allowed
		UCA0TXBUF = SEND_2_IOT[direct_iot++];		    // Send next char from PC_2_IOT to IOT
        if (direct_iot >= (sizeof(SEND_2_IOT))) {
			direct_iot = BEGINNING;			        // Circular buffer back to beginning
        }
        if (direct_iot == usb_rx_wr) {
            UCA0IE &= ~UCTXIE;                      // Disable A0 TX interrupt 
        }
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
			allow_comms = TRUE;                   // Allow communications after first char received from PC
        }
        temp_rx = UCA1RXBUF;                 // Echo back to PC
		//UCA1TXBUF = temp_rx;               // Echo back to PC immediately

        temp = usb_rx_wr++;
        PC_2_IOT[temp] = temp_rx;               // Rx -> PC_2_IOT character array 
        if (usb_rx_wr >= (sizeof(PC_2_IOT))) {
            usb_rx_wr = BEGINNING;                // Circular buffer back to beginning 
        }
        UCA0IE |= UCTXIE;                         // Enable A0 Tx interrupt
    } break;

	case 4: { // TXIFG: UCA1 SEND TO PC  [received from A0 Rx]
		if (!allow_comms) { break; }              // Prevent sending to PC until allowed
        UCA1TXBUF = IOT_2_PC[direct_usb++];
		//IOT_2_PC[direct_usb-1] = 0;               // Clear after sending, advance index
        if (direct_usb >= (sizeof(IOT_2_PC))) {
            direct_usb = BEGINNING;               // Circular buffer back to beginning 
        }
        if (direct_usb == iot_rx_wr) {
            UCA1IE &= ~UCTXIE;                   // Disable A1 TX interrupt
        }
    } break;

    default:
        break;
    }
}


