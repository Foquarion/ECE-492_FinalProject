/*
 * UART.h
 *
 *  Created on: Oct 29, 2025
 *      Author: Dallas.Owens
 */

#ifndef UART_H_
#define UART_H_

//#define PROCESS_BUFFER_SIZE 11

#define SMALL_RING_SIZE			(32)
#define PROCESS_BUFFER_LINES	(4)
#define PROCESS_BUFFER_SIZE		(32)

#define BEGINNING 				(0)

#define COMMAND_PREFIX         ('^')
#define COMMAND_TERMINATOR     (0x0D)   // Carriage Return

#define BAUD_115200             (0)
#define BAUD_460800             (1)

#define SSID_CHAR               (8)
#define IP_CHAR                 (14)
#define MAX_SSID_LEN            (32)
#define MAX_IP_LEN              (16)


// #define SET_HOME_WIFI           ("AT+CWJAP=\"The LAN Before Time",\"purpleduke1\"\r\n")
// #define SET_NCSU_WIFI           ("AT+CWJAP=\"ncsu\",\"\"\r\n")

// #define SET_MUX_COMMAND         ("AT+CIPMUX=1\r\n")
// #define SET_PORT_COMMAND        ("AT+CIPSERVER=1,55155\r\n")
// #define REQUEST_IP_COMMAND      ("AT+CIFSR\r\n")

//typedef enum {
//	BAUD_115200,		// 0
//	BAUD_460800,		// 1
//
//	BAUD_COUNT			// 2
//} Baud_Rate_t;



#endif /* UART_H_ */
