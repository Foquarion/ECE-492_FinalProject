/*
 * UART.c
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
#include "UART.h"
#include "queue.h"
#include  "LED.h"
#include  "bootup.h"


// DONT PRIME THE BUFFER!!!!
// Hysterical Porpoises
//volatile char SEND_2_IOT[PROCESS_BUFFER_SIZE];		        // IOT Transmit Buffer
//volatile unsigned char iot_tx_index = 0;			            // IOT Transmit Buffer Index volatile unsigned char IOT_TX_Buffer[32];
//volatile unsigned int iot_tx_len = 0;


// SERIAL COMMUNICATIONS BUFFERS AND INDEXES____________________________________________________
volatile unsigned char UCA1_TX_Buffer[PROCESS_BUFFER_SIZE];		// UCA1 TX Buffer
volatile unsigned int tx_index;

extern volatile unsigned char IOT_2_PC[SMALL_RING_SIZE];
extern volatile unsigned int iot_rx_wr;
extern volatile unsigned char allow_comms;

extern volatile unsigned char reset_iot;

extern volatile unsigned char SEND_2_IOT[SMALL_RING_SIZE];
extern volatile unsigned int iot_rx_rd;                         // IOT Rx Ring Buffer Read Index

extern char display_line[4][11];                                // LCD display lines
extern volatile unsigned char display_changed;                  // Flag to update display


// IOT PARSING VARIABLES___________________________________________________________________________
volatile char IOT_Data[PROCESS_BUFFER_LINES][PROCESS_BUFFER_SIZE];      // Holds up to 4 lines of IOT messages
volatile unsigned int line = 0;                             // Current line being written
volatile unsigned int character = 0;                        // Current character index in line
unsigned int nextline = 0;                                  // Next line index for processing

unsigned char iot_parse = FALSE;                            // Flag to indicate IOT parsing state
volatile unsigned char iot_parse_ready = FALSE;             // Flag to indicate IOT parse ready
//volatile unsigned char boot_state = 0;                    // State variable for IOT boot process

unsigned char ip_address_found = FALSE;                     // Flag to indicate if a NEW IP address has been assigned
unsigned char ip_address[16];                               // Buffer to hold IP address string
unsigned char ip_address_line = 0;
unsigned char display_ip = FALSE;
unsigned char ip_len = 0;

unsigned char ssid_found = FALSE;                           // Set when +CWJAP response is identified
unsigned char ssid[11];
unsigned char ssid_line = 0;
unsigned char display_ssid = FALSE;
unsigned char ssid_len = 0;

unsigned char iot_cmd_received = FALSE;                     // Flag to indicate IOT command received
unsigned char iot_cmd_len = 0;                              // Length of IOT command
unsigned char ipd_found = FALSE;                            // Flag to indicate +IPD command received
unsigned char ipd_line = 0;                                 // Line where +IPD found
unsigned char iot_cmd_start = 0;                            // Start index of IOT command data

unsigned char ready = FALSE;
unsigned char ok_found = FALSE;


// COMMAND BUFFER GLOBALS_______________________________________________________________________________
volatile unsigned char command_line = 0;                    // Buffer line index for command
volatile unsigned char next_command_line = 0;               // Where next buffer line will be written
const unsigned char Command_PIN[] = PIN_CODE;			    // Command PIN code for validation [defined in macros.h for quick access on game day]
volatile unsigned char command_approved = FALSE;            // Flag to indicate command passed PIN check
unsigned char bad_actor = 0;                                // Flag to indicate invalid command source

extern volatile unsigned char Command_Buffer[PROCESS_BUFFER_LINES][PROCESS_BUFFER_SIZE];        // Buffer to store approved commands
extern volatile unsigned char command_index;                                                    // Index into command buffer
extern volatile unsigned char command_ready;                                                    // Flag to indicate command ready and waiting

extern volatile unsigned char command_buffer[PROCESS_BUFFER_SIZE];         // Store incoming command


unsigned char USB_Char_Rx[PROCESS_BUFFER_SIZE];                            // update to Tx_from_PC
extern volatile unsigned int usb_rx_wr;

extern volatile unsigned int ping_time;
extern volatile unsigned char send_ping;
extern unsigned char  iot_boot_complete;


// IOT Boot Sequencing Globals______________________________________________________________________________
//extern volatile unsigned char init_iot_connection;              // Flag to initiate IOT connection sequence
//extern volatile unsigned char boot_next;                        // Flag to proceed to next boot state
//unsigned char iot_boot_state = 0;                      // Current boot state (0-3)

char SET_HOME_WIFI[]        = "AT+CWJAP=\"The LAN Before Time\",\"purpleduke1\"\r\n";
char SET_NCSU_WIFI[]        = "AT+CWJAP=\"ncsu\",\"\"\r\n";

char SET_MUX_COMMAND[]      = "AT+CIPMUX=1\r\n";                     // Set Multiple connections
char SET_PORT_COMMAND[]     = "AT+CIPSERVER=1,55155\r\n";           // Set Server mode on port 55155
char REQUEST_SSID_COMMAND[] = "AT+CWJAP?\r\n";                  // Request connected SSID
char REQUEST_IP_COMMAND[]   = "AT+CIFSR\r\n";                     // Request IP address
char PING_COMMAND[]         = "AT+PING=\"www.google.com\"\r\n";



//void Connect_IOT(void) {
//    // Boot sequence triggered by iot_boot flag set in TB2 CCR1 ISR (100ms)
//    // Each state transition occurs via boot_next flag set in TB2 CCR0 ISR (100ms intervals)
//    // States: 0->1->2->3->complete, with ~100ms between each transition
//    
//    if (boot_next) {
//        boot_next = 0;  // Consume the flag
//        
//        GRN_TOGGLE();
//
//        switch (iot_boot_state) {
//        case 0:                         // State 0: Enable IOT module (released from reset)
//            P3OUT |= IOT_EN;                // Set IOT EN pin HIGH
//            iot_boot_state++;               // Move to next state
//            break;
//
//        case 1:                         // State 1: Set Multiplex mode (allow multiple connections)
//            if(ready){
//                ready = 0;
//                iot_boot_state++;
//            }
//            break;
//
//        case 2:                         // State 1: Set Multiplex mode (allow multiple connections)
//            Send_AT_Command(SET_MUX_COMMAND);
//            iot_boot_state++;
//            break;
//
//        case 3:
//            if(ok_found){
//                ok_found = 0;
//                iot_boot_state++;
//            }
//            break;
//
//        case 4:                         // State 2: Set Server mode (using port 55155)
//            Send_AT_Command(SET_PORT_COMMAND);
//            iot_boot_state++;
//            break;
//
//        case 5:
//            if(ok_found){
//                ok_found = 0;
//                iot_boot_state++;
//            }
//            break;
//
//        case 6:                         // State 3: Request SSID
//            Send_AT_Command(REQUEST_SSID_COMMAND);
//            iot_boot_state++;
//            break;
//
//        case 7:
//            if(ok_found){
//                ok_found = 0;
//                iot_boot_state++;
//            }
//            break;
//
//        case 8:                         // State 4: Request IP address and complete boot
//            Send_AT_Command(REQUEST_IP_COMMAND);
//            iot_boot_state++;
//            break;
//
//        case 9:
//            iot_boot_state++;
//            break;
//
//        case 10:
//            if(ok_found){
//                ok_found = 0;
//                iot_boot_state++;
//            }
//            break;
//        case 11:
//            iot_boot_state = 0;         // Reset state machine
//            init_iot_connection = 0;    // Clear the boot initiation flag
//            TB2CCTL0 &= ~CCIE;          // Disable TB2 CCR0 interrupt (no longer need 100ms ticks)
//            iot_boot_complete = 1;
//            break;
//
//        default:
//            iot_boot_state = 0;
//            init_iot_connection = 0;
//            break;
//        }
//    }
//}

// void Boot_IOT(void) {
// 	P3OUT |= IOT_EN;
// }

// void Reset_IOT(void){
//     GRN_ON();
//     reset_iot = 1;
//     P3OUT &= ~IOT_EN;

//     TB2CCTL2 &= ~CCIFG;
//     TB2CCR2 = TB2R + TB2CCR2_INTERVAL;      // Set the interrupt timer
//     TB2CCTL2 |= CCIE;                       // Enable Reset timer in TB2-CCR2
// }


void Ping_Pong(void){
    if(!send_ping && iot_boot_complete) return;
    send_ping = FALSE;
    ping_time = 0;
    Send_AT_Command(PING_COMMAND);
}


void Enable_Comms(void) {
    static unsigned char temp;
    allow_comms = TRUE;

    temp = usb_rx_wr++;
    SEND_2_IOT[temp] = 'X';                 // Simulate receiving a character
    if (usb_rx_wr >= sizeof(SEND_2_IOT)) {
        usb_rx_wr = BEGINNING;
    }
    UCA0IE |= UCTXIE;                       // Enable transmission
}

void Process_Command(void) {
    if (!command_ready) return;
    
    command_ready = 0;                      // Consume the command flag
    
    // TESTING command processing
    if (command_index >= 2) {
        if (command_buffer[0] == '^' && command_buffer[1] == '^') { 	    // "^^" command - respond immediately
            Send_Response("I'm here\r\n");
        }

		// CHANGE LOGIC BELOW WHEN NEEDED BUT WORKS FOR NOW
        else if (command_buffer[0] == '^' && command_index >= 3) {		    // Single '^' command, checks if index passed command char.
            switch (command_buffer[1]) {
                case 'F':  												    // ^F - Fast baud rate
                    Set_Baud_115200();
                    Send_Response("115,200\r\n");
                    break;
                    
                case 'S':  												    // ^S - Slow baud rate
                    // Set_Baud_9600();  
                    Send_Response("9,600\r\n");
                    break;
                    
                default:
                    Send_Response("Unknown command\r\n");
                    break;
            }
        }
    }
    command_index = 0;									// Clear command buffer for next command
}

void Send_Response(const char* response) {
    unsigned char i = 0;
    while (response[i] != 0 && i < sizeof(IOT_2_PC) - 1) {		    // Copy response to IOT_2_PC buffer for transmission to PC
        IOT_2_PC[iot_rx_wr++] = response[i++];
        if (iot_rx_wr >= sizeof(IOT_2_PC)) {
            iot_rx_wr = BEGINNING;
        }
    }
        UCA1IE |= UCTXIE;  										    // Enable transmission to PC
}



void Send_AT_Command(const char* IOT_Cmd)
{
    if (!allow_comms) { return; }
    GRN_TOGGLE();

    // Copy up to 32 bytes into IOT_TX_Buffer
    unsigned int i = 0;
    while (IOT_Cmd[i] != '\0' && i < PROCESS_BUFFER_SIZE) {
        SEND_2_IOT[usb_rx_wr++] = IOT_Cmd[i++];         // Load command into TX buffer
        if (usb_rx_wr >= sizeof(SEND_2_IOT)) {
            usb_rx_wr = BEGINNING;
        }
    }
//    SEND_2_IOT[i] = '\0';                             // Null terminate for end string recognition
//    iot_tx_len = i;                                   // Prime for tx ISR
//    iot_tx_index = BEGINNING;                         // Prime for tx ISR
    UCA0IE |= UCTXIE;                                   // Enable UCA0 TX interrupt to start transmission


}


void IOT_Process(void) {               // Process IOT messages
    unsigned int i;
    unsigned int iot_rx_wr_temp;

    // Process all available characters in IOT_2_PC ring buffer
    while (1) {
        iot_rx_wr_temp = iot_rx_wr;

        if (iot_rx_wr_temp == iot_rx_rd) {                      // Ring buffer empty, exit loop
            break;
        }

        IOT_Data[line][character] = IOT_2_PC[iot_rx_rd++];      // Read one character from ring buffer


        if (iot_rx_rd >= sizeof(IOT_2_PC)) {
            iot_rx_rd = BEGINNING;
        }

        if (IOT_Data[line][character] == 0x0A) {
			IOT_Data[line][character] = '\0';                   // Null terminate line
		    if(iot_parse){ iot_parse_ready = TRUE; }
            character = BEGINNING;
            line++;

			if (line >= PROCESS_BUFFER_LINES) {                 // Wrap around current line to first buffer line
                line = BEGINNING;                               
            }

			nextline = line + 1;                                // Set next line for processing

			if (nextline >= PROCESS_BUFFER_LINES) {             // Indicate next line will be at beginning
                nextline = BEGINNING;
            }
        }
		else if (ipd_found && IOT_Data[line][character] == ':') {                   // End of +IPD preamble
            int decade = 1;
            for (i = 1; IOT_Data[line][character - i] != ',' && i < 3; i++) {
                iot_cmd_len += (IOT_Data[line][character - i] - '0') * decade;      // Extract length of incoming data from +IPD preamble
				decade *= 10;
			}
            character++;
			//iot_cmd_start = character + 1;                                        // Set start of incoming data
		}
        
        else if (ipd_found && IOT_Data[line][character] == COMMAND_PREFIX) {        // Check for (^) start of command
			iot_cmd_start = character;                                              // Start of command detected
			command_line = line;
			character++;
        }
       
        else {
            switch (character) 
            { // Check chars as they come in to pre-process response
            case  0:
                if (IOT_Data[line][character] == '+') {             // Got "+"
                    iot_cmd_received = 1;
                }
                else if(IOT_Data[line][character] == 'O'){
                    ok_found = TRUE;
                }

                break;

            case  1:
                // GRN_LED_ON;
                break;

            case  2:

                break;

			case  3:    // COMMAND FROM IOT RECIEVED "+IPD:"
                if (iot_cmd_received) {
                    if ((IOT_Data[line][character] == 'D') && (IOT_Data[line][1] == 'I') && (IOT_Data[line][2] == 'P')) {
                        ipd_found = TRUE;
                        ipd_line = line;
                        iot_parse = TRUE;
                        iot_cmd_received = FALSE;
                    }
                }
                break;

            case  4:
                if (IOT_Data[line][character] == 'y') {         // Got read"y"
                    ready = TRUE;

//                    GRN_LED_OFF;
//                    RED_LED_ON;
                }
                break;

            case 5:
//              // CHECK FOR 'P' OF "+CWJAP" TO CONFIRM SSID MESSAGE
				if (iot_cmd_received) {
                    if (IOT_Data[line][character] == 'P') {
                        ssid_found = 1;
                        ssid_line = line;

                        // Clear out old ssid
                        for (i = 0; i < MAX_SSID_LEN; i++) {
                            ssid[i] = '\0';
                        }
                        iot_cmd_received = FALSE;
                        iot_parse = TRUE;
                    }
                }

//                if (IOT_Data[line][character] == 'G') {      // Got IP
//                    for (i = 0; i < sizeof(ip_mac); i++) {
//                        IOT_TX_buf[i] = ip_mac[i];
//                    }
//
//                    iot_tx = 0;
//                    UCA0IE |= UCTXIE;
//                }

                break;

            case  6:
                 break;

//            case  8:
//                // POPULATE SSID IF FOUND ABOVE
//                if (ssid_found){
//                    for (i = 0; i < 11; i++){
//                        if (IOT_Data[line][character + i] == '"'){
//                            ssid[i] = '/r';
//                            ssid[i+1] = '/n';
//                            ssid_found = FALSE;
//                            break;
//                        }
//                        else{
//                            ssid[i] = IOT_Data[line][character + i];
//                        }
//                    }
//                }
//                break;

            case  10:
                // CHECK FOR 'I' OF "APIP" in "+CIFSR:APIP" TO CONFIRM IP ADDRESS MESSAGE
                if (iot_cmd_received)
                {
                    if (IOT_Data[line][character] == 'I') {
                        ip_address_found = TRUE;
                        ip_address_line = line;
                        strcpy(display_line[1], "IP Address");

                        // Clear old IP ADDRESS
                        for (i = 0; i < MAX_IP_LEN; i++) {
                            ip_address[i] = '\0';
                        }
                        iot_cmd_received = FALSE;
                        iot_parse = TRUE;

                        // iot_index = 0;
                    }
                }
                break;

//            case 14:
//                if (ip_address_found){
//                    for (i = 0; i < sizeof(ip_address); i++){
//                        if(IOT_Data[line][character + i] == '"'){
//                            ip_address[i] = '/r';
//                            ip_address[i+1] = '/n';
//                            ip_address_found = FALSE;
//                            break;
//                        }
//                        else{
//                            ip_address[i] = IOT_Data[line][character + i];
//                        }
//                    }
//                }
//                break;

            
            default: break;
            }
            character++;

//            if (character >= PROCESS_BUFFER_SIZE - 1) {
//                IOT_Data[line][character] = '\0';
//                character = BEGINNING;
//                line++;
//                if (line >= PROCESS_BUFFER_LINES) {
//                    line = BEGINNING;
//                }
//            }
        }
    }  // End of while loop - process all available characters
}


void IOT_Parse_Data(void) {
    if (!iot_parse_ready) { return; }
    
    iot_parse_ready = 0;
    unsigned int i;

    // POPULATE SSID IF FOUND during IOT PROCESS
    if (ssid_found) {
        for (i = 0; i < MAX_SSID_LEN - 1; i++) {
            if (IOT_Data[ssid_line][SSID_CHAR + i] == '"') {
                ssid[i] = '\0';
                ssid_found = FALSE;
                display_ssid = TRUE;
                break;
            }
            else {
                ssid[i] = IOT_Data[ssid_line][SSID_CHAR + i];
            }
        }
        if (i == MAX_SSID_LEN - 1) {
            ssid[i] = '\0';
        }
        ssid_len = i;
    }

    // POPULATE SSID IF FOUND during IOT PROCESS
    if (ip_address_found)
    {
        for (i = 0; i < MAX_IP_LEN - 1; i++)
        {
            if (IOT_Data[ip_address_line][IP_CHAR + i] == '"')
            {
                ip_address[i] = '\0';
                ip_address_found = FALSE;
                display_ip = TRUE;
                break;
            }
            else {
                ip_address[i] = IOT_Data[ip_address_line][IP_CHAR + i];
            }
        }
        if (i == MAX_IP_LEN - 1) {
            ip_address[i] = '\0';
        }
        ip_len = i;
    }


    if (ipd_found) { //  "+IPD,0,13:Hello World/r/n"
        for (i = 0; i < (sizeof(Command_PIN) - 2); i++) {
            if (IOT_Data[ipd_line][iot_cmd_start + i] != Command_PIN[i]) {
                command_approved = FALSE;
                bad_actor = TRUE;
                ipd_found = FALSE;
                return;
            }
            else {
                command_approved = TRUE;
            }
        }

        if (command_approved)
        {  // Extract command and queue it for processing in Main
            char temp_command[COMMAND_BUFFER_SIZE];
            unsigned char temp_index = BEGINNING;

            iot_cmd_start += (sizeof(Command_PIN) - 1);     // Move start index past PIN code
            iot_cmd_len -= (sizeof(Command_PIN) - 1);       // Adjust command length accordingly;


            for (i = 0; i < iot_cmd_len && temp_index < COMMAND_BUFFER_SIZE - 1; i++) {
                temp_command[temp_index++] = IOT_Data[ipd_line][iot_cmd_start + i];
            }
            temp_command[temp_index] = '\0';                // Null terminate
            Queue_AddCommand(temp_command);
            command_approved = FALSE;                       // Reset flag

        }
        ipd_found = FALSE; // Reset +IPD found flag
    }
    Display_IOT_Parse();
}



void Display_IOT_Parse(void){
    unsigned char ssid_pad = 0;
    char ssid_print[11] = "          ";

    unsigned char dotCount = 0;
    char group1[8];
    char group1_print[11] = "          ";
    unsigned char group1_pad;

    char group2[6];
    char group2_print[11] = "          ";
    unsigned char group2_pad;


    unsigned char second_dot_position = 0;
    unsigned int i = 0;

    // SSID DISPLAY CASE
    if (display_ssid)
    {
        display_ssid = FALSE;

        if(!ssid){
            strcpy(display_line[0], " NO  SSID ");
        }
        else{
            for (i = 0; i < 10; i++) {
                ssid_print[i] = ' ';
            }

            if (ssid_len > 10){
                ssid_len = 10;
                ssid_pad = 0;
            }
            else{
                ssid_pad = (10 - ssid_len) / 2;
            }

            for (i = 0; i < ssid_len && ssid[i] != '\0'; i++) {
                ssid_print[ssid_pad + i] = ssid[i];
            }
            strcpy(display_line[0], ssid_print);
        }
    }

    // IP_ADDRESS DISPLAY CASE
    if (display_ip)
    {
        display_ip = FALSE;

        if(!ip_address){
            strcpy(display_line[0], "IP Address");
            strcpy(display_line[0], "Not  Found");
        }
        else{
            // Find second dot (.) location, use as index for splitting groups
            for (i = 0; i < MAX_IP_LEN - 1; i++) {
                if (ip_address[i] == '.') {
                    dotCount++;
                    if (dotCount == 2) {
                        second_dot_position = i;
                        break;
                    }
                }
            }

            // Group 1: Beginning to second dot
            for (i = 0; i < second_dot_position; i++){
                group1[i] = ip_address[i];
            }
            group1[i] = '\0';

            // Group 2: Second Dot to end;
            for (i = 0; ip_address[i + second_dot_position + 1] != '\0'; i++){
                group2[i] = ip_address[i + second_dot_position + 1];
            }
            group2[i] = '\0';
            unsigned char group2_len = i;
            // Padding: lcd line
            group1_pad = (10 - second_dot_position) / 2;
            group2_pad = (10 - group2_len) / 2;

            for (i = 0; i < second_dot_position && group1[i] != '\0'; i++){
                group1_print[group1_pad + i] = group1[i];
            }
            for (i = 0; i < (group2_len) && group2[i] != '\0'; i++){
                group2_print[group2_pad + i] = group2[i];
            }
            strcpy(display_line[1],"IP ADDRESS");
            strcpy(display_line[2], group1_print);
            strcpy(display_line[3], group2_print);
        }
    }
    display_changed = 1;
}


//void A0_transmit(void) {
//    unsigned int i = 0;
//	// Contents must be in process_buffer
//	// End of Transmission is identified by NULL character in process_buffer
//	// process_buffer includes Carriage Return and Line Feed
//	while (process_buffer[i] != '\0') {
//		IOT_TX_buf[i] = process_buffer[i++];
//		if (i >= sizeof(IOT_TX_buf)) {
//			break;											    // Prevent buffer overflow
//		}
//		if (i < sizeof(IOT_TX_buf)) {
//			IOT_TX_buf[i] = '\0';							    // Null terminate the buffer
//		}
//		else {
//			IOT_TX_buf[sizeof(IOT_TX_buf) - 1] = '\0';		    // Ensure buffer is null terminated
//		}
//	}
//	iot_tx = BEGINNING;									        // Set Array index to first location
//	UCA0IE |= UCTXIE;									        // Enable TX interrupt
//}

void A1_transmit(const char SEND_BUFFER[PROCESS_BUFFER_SIZE])
{ // Populate UCA1 TX Buffer and enable TX interrupt for transmitting
  //	- SEND_BUFFER holds message to transmit (must be null terminated)
  //    - UCA1_TX_Buffer volatile is populated from SEND_BUFFER - Ref'd in interrupt
	unsigned char A1_tx_char = 0;
	if (!SEND_BUFFER) { return; }

	while ((SEND_BUFFER[A1_tx_char] != 0) && (A1_tx_char < sizeof(UCA1_TX_Buffer) - 1)) {
		UCA1_TX_Buffer[A1_tx_char] = SEND_BUFFER[A1_tx_char++];
	}
	UCA1_TX_Buffer[A1_tx_char] = '\0';
	tx_index = BEGINNING;
	UCA1IE |= UCTXIE;
}


void A1_TEST_transmit(void)
{
    if (!allow_comms) { return; }
    GRN_TOGGLE();
    static const unsigned char TEST_MESSAGE[SMALL_RING_SIZE] = "NCSU  #1\r\n";
    unsigned char A1_tx_char = 0;
//    if (!TEST_MESSAGE) { return; }


    while ((TEST_MESSAGE[A1_tx_char] != 0) && (A1_tx_char < sizeof(TEST_MESSAGE) - 1)) {
        IOT_2_PC[iot_rx_wr++] = TEST_MESSAGE[A1_tx_char++];
//        SEND_BUFFER[iot_rx_wr-1] = NULL;                  // Null Proc_Buffer
        if (iot_rx_wr >= sizeof(IOT_2_PC)){
            iot_rx_wr = BEGINNING;
        }
    }
//    IOT_2_PC[A1_tx_char] = '\0';
//    //tx_index = BEGINNING;
//    iot_rx_wr = A1_tx_char;

    UCA1IE |= UCTXIE;
}

// void ORIGINAL_Send_AT_Command(const char* cmd) {
//     unsigned int i = 0;
//     while (cmd[i] != '\0') {
//         PC_2_IOT[usb_rx_wr++] = cmd[i++];
//         if (usb_rx_wr >= sizeof(PC_2_IOT)) usb_rx_wr = BEGINNING;
//     }
//     // Add CRLF as required by AT commands
//     PC_2_IOT[usb_rx_wr++] = '\r';
//     if (usb_rx_wr >= sizeof(PC_2_IOT)) usb_rx_wr = BEGINNING;
//     PC_2_IOT[usb_rx_wr++] = '\n';
//     if (usb_rx_wr >= sizeof(PC_2_IOT)) usb_rx_wr = BEGINNING;

//     UCA0IE |= UCTXIE; // Enable UART TX interrupt to start sending
// 
//     // ALT MODE BELOW:::
//    if (!allow_comms) { return; }
//
//    unsigned int i = 0;
//
//    // Write command to PC_2_IOT buffer (UCA0 will send this to IOT)
//    while (cmd[i] != '\0') {
//        PC_2_IOT[usb_rx_wr++] = cmd[i++];
//        if (usb_rx_wr >= sizeof(PC_2_IOT)) {
//            usb_rx_wr = BEGINNING;
//        }
//    }
//
//    // Enable UCA0 TX interrupt to start sending to IOT module
//    UCA0IE |= UCTXIE;
//
//    GRN_TOGGLE();  // Debug: verify function is called
// }

void Set_Baud_115200(void)
{
	UCA0IE &= ~UCTXIE;		// stop A0 TX interrupts during reconfig
	UCA1IE &= ~UCTXIE;		// stop A1 TX interrupts during reconfig

	UCA0CTLW0 |= UCSWRST;			// Put eUSCI in reset
	UCA1CTLW0 |= UCSWRST;			// Put eUSCI in reset

	UCA0BRW = 4;					// UCA0
	UCA0MCTLW = 0x5551;				// 115200 baud
	UCA1BRW = 4;					// UCA1
	UCA1MCTLW = 0x5551;				// 115200 baud

	UCA0IFG = 0;					// Clear eUSCI flags
	UCA1IFG = 0;					// Clear eUSCI flags

	UCA0CTLW0 &= ~UCSWRST;			// Release eUSCI from reset
	UCA1CTLW0 &= ~UCSWRST;			// Release eUSCI from reset

	UCA0IE |= UCRXIE;				// Re-enable RX interrupts
	UCA1IE |= UCRXIE;				// Re-enable RX interrupts

//	BaudRate = BAUD_115200;
}

void Set_Baud_460800(void)
{
	UCA0IE &= ~UCTXIE;		// stop A0 TX interrupts during reconfig
	UCA1IE &= ~UCTXIE;		// stop A1 TX interrupts during reconfig

	UCA0CTLW0 |= UCSWRST;			// Put eUSCI in reset
	UCA1CTLW0 |= UCSWRST;			// Put eUSCI in reset

	UCA0BRW = 17;					// UCA0
	UCA0MCTLW = 0x4A00;				// 460,800 baud
	UCA1BRW = 17;					// UCA1
	UCA1MCTLW = 0x4A00;				// 460,800 baud

	UCA0IFG = 0;					// Clear eUSCI flags
	UCA1IFG = 0;					// Clear eUSCI flags

	UCA0CTLW0 &= ~UCSWRST;			// Release eUSCI from reset
	UCA1CTLW0 &= ~UCSWRST;			// Release eUSCI from reset

	UCA0IE |= UCRXIE;				// Re-enable RX interrupts
	UCA1IE |= UCRXIE;				// Re-enable RX interrupts

//	BaudRate = BAUD_460800;

}




void Init_SerialComms(void) {
	Init_Serial_UCA0();
	Init_Serial_UCA1();
}


void Init_Serial_UCA0(void) 
{

//	int i;
//	for (i = 0; i < SMALL_RING_SIZE; i++) {
//		USB_Char_Rx[i] = 0x00;
//	}
//	usb_rx_wr = BEGINNING;
//	usb_rx_rd = BEGINNING;
//	//for (i = 0; i < LARGE_RING_SIZE; i++) {		// May not use this
//	//	USB_Char_Tx[i] = 0x00;						// USB Tx Buffer
//	//}
//	usb_tx_ring_wr = BEGINNING;
//	usb_tx_ring_rd = BEGINNING;

	// Configure UART 0
	UCA0CTLW0 = 0;
	UCA0CTLW0 |=  UCSWRST;			    // Put eUSCI in reset
	UCA0CTLW0 |=  UCSSEL__SMCLK;	    // Set SMCLK as fBRCLK
	UCA0CTLW0 &= ~UCMSB;			    // MSB, LSB select
	UCA0CTLW0 &= ~UCSPB;			    // UCSPB = 0(1 stop bit) OR 1(2 stop bits)
	UCA0CTLW0 &= ~UCPEN;			    // No Parity
	UCA0CTLW0 &= ~UCSYNC;
	UCA0CTLW0 &= ~UC7BIT;
	UCA0CTLW0 |=  UCMODE_0;

//	Baud Rate: 9600
//	1. Calculate N = fBRCLK / Baudrate
//		N = SMCLK / 9,600 => 8,000,000 / 9,600 = 833.3333333
//		If N > 16 continue with step 3, otherwise with step 2
//	2. OS16 = 0, UCBRx = INT(N)
//		Continue with step 4
//	3. OS16 = 1,
//		UCx		= INT(N/16),
//				= INT(N/16) = 833.333/16 => 52
//		UCFx	= INT([(N/16) � INT(N/16)] � 16)
//				= ([833.333 / 16 - INT(833.333 / 16)] * 16)
//				= (52.08333333 - 52) * 16
//				= 0.083 * 16 = 1
//	4. UCSx is found by looking up the fractional part of N (= N-INT(N)) in table Table 18-4
//		Decimal of SMCLK / 8,000,000 / 9,600 = 833.3333333 => 0.333 yields 0x49 [Table]
//	5. If OS16 = 0 was chosen, a detailed error calculation is recommended to be performed
// 
//																 TX error (%)	 RX error (%)
//		    BRCLK   Baudrate  UCOS16	UCBRx	UCFx	UCSx	 neg	 pos	 neg     pos
//		8,000,000       9600	    1	   52     1 	0x49	-0.08	 0.04	-0.10	 0.14
// 
//		8,000,000	  115200		1	    4     5	    0x55	-0.80	0.64	-1.12	1.76
//		8,000,000	  460800		0      17     0	    0x4A	-2.72	2.56	-3.76	7.28

	// UCA0MCTLW = UCSx concatenate UCFx concatenate UCOS16;
	// UCA0MCTLW = 0X55 concatenate    5 concatenate      1;
	// UCA0MCTLW = 0x4A concatenate    0 concatenate      0;

	UCA0BRW = 4;					// 115200 baud
	UCA0MCTLW = 0x5551;				// 115200 baud
	// UCA0BRW = 17;				// 460,800 baud
	// UCA0MCTLW = 0x4A00;			// 460,800 baud

	UCA0CTLW0 &= ~UCSWRST;			// release from reset
//	UCA0TXBUF = 0x00;				// Prime the Pump
	UCA0IE |= UCRXIE;				// Enable RX interrupt
}


void Init_Serial_UCA1(void) {
	//------------------------------------------------------------------------------
	//												 TX error (%)	 RX error (%)
	// BRCLK  Baudrate UCOS16  UCBRx  UCFx	 UCSx	  neg	 pos	  neg	 pos
	// 8000000	  4800		1    104     2	 0xD6	-0.08	0.04	-0.10	0.14
	// 8000000	  9600		1	  52     1	 0x49	-0.08	0.04	-0.10	0.14
	// 8000000	 19200		1	  26     0	 0xB6	-0.08	0.16	-0.28	0.20
	// 8000000	 57600		1	   8    10	 0xF7	-0.32	0.32	-1.00	0.36
	// 8000000	115200		1	   4     5	 0x55	-0.80	0.64	-1.12	1.76
	// 8000000	460800		0     17     0	 0x4A	-2.72	2.56	-3.76	7.28
	//------------------------------------------------------------------------------
	// Configure eUSCI_A0 for UART mode
	UCA1CTLW0 = 0;
	UCA1CTLW0 |=  UCSWRST;			// Put eUSCI in reset
	UCA1CTLW0 |=  UCSSEL__SMCLK;		// Set SMCLK as fBRCLK
	UCA1CTLW0 &= ~UCMSB;			// MSB, LSB select
	UCA1CTLW0 &= ~UCSPB;			// UCSPB = 0(1 stop bit) OR 1(2 stop bits)
	UCA1CTLW0 &= ~UCPEN;			// No Parity
	UCA1CTLW0 &= ~UCSYNC;
	UCA1CTLW0 &= ~UC7BIT;
	UCA1CTLW0 |=  UCMODE_0;
	//   BRCLK  Baudrate   UCOS16    UCBRx    UCFx    UCSx     neg    pos     neg    pos
	// 8000000    115200      1        4        5     0x55   -0.80   0.64   -1.12   1.76
	// UCA?MCTLW = UCSx + UCFx + UCOS16
	UCA1BRW = 4;					// 115,200 baud
	UCA1MCTLW = 0x5551;

	UCA1CTLW0 &= ~UCSWRST;			// release from reset
	//UCA0TXBUF = 0x00;				// Prime the Pump
	UCA1IE |= UCRXIE;				// Enable RX interrupt
	//------------------------------------------------------------------------------
}



