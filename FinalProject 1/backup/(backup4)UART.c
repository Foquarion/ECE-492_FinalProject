/*
 * UART.c
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
#include "UART.h"


// DONT PRIME THE BUFFER!!!!

//#define BAUD_115200            (0)            // 115200 Baud Rate
//#define BAUD_460800            (1)            // 460800 Baud Rate

unsigned char BaudRate;


volatile char USB_Char_Rx[SMALL_RING_SIZE];		// USB Rx Ring Buffer

extern volatile unsigned char USB_Ring_Rx[SMALL_RING_SIZE];		// USB Rx Ring Buffer
extern volatile unsigned int usb_rx_wr;				// USB Rx Ring Buffer Write Index
extern volatile unsigned int usb_rx_rd;				// USB Rx Ring Buffer Read Index

char UART_ProcBuff[1][11];						// UART Process Buffer (10 chars + NULL)
unsigned int uart_char = 0;						// UART Process Buffer Character Index
//unsigned int uart_line = 0;					// UART Process Buffer Line Index (not used for HW8)

unsigned char process_buffer[PROCESS_BUFFER_SIZE];	// UART Transmit Process Buffer

volatile unsigned char IOT_TX_buf[SMALL_RING_SIZE];
extern volatile unsigned int iot_tx;

volatile unsigned char UCA1_TX_Buffer[PROCESS_BUFFER_SIZE];		// UCA1 TX Buffer
volatile unsigned int tx_index;

unsigned char usb_tx_ring_wr;
unsigned char usb_tx_ring_rd;

//unsigned char usb_rx_ring_wr;
//unsigned char usb_rx_ring_rd;

extern volatile unsigned char IOT_2_PC[SMALL_RING_SIZE];
extern volatile unsigned int iot_rx_wr;
extern volatile unsigned char allow_comms;

extern volatile unsigned char command_buffer[PROCESS_BUFFER_SIZE];
extern volatile unsigned char command_index;
extern volatile unsigned char command_ready;

extern volatile unsigned char reset_iot;

extern volatile unsigned char SEND_2_IOT[SMALL_RING_SIZE];
extern volatile unsigned int iot_rx_rd; // IOT Rx Ring Buffer Read Index

extern char display_line[4][11];                 // LCD display lines
extern volatile unsigned char display_changed;   // Flag to update display

volatile unsigned char SEND_2_IOT[PROCESS_BUFFER_SIZE];		// IOT Transmit Buffer
volatile unsigned char iot_tx_index = 0;			// IOT Transmit Buffer Index volatile unsigned char IOT_TX_Buffer[32];
volatile unsigned int iot_tx_len = 0;

volatile char IOT_Data[PROCESS_BUFFER_LINES][PROCESS_BUFFER_SIZE]; // Holds up to 4 lines of IOT messages
volatile unsigned int line = 0;        // Current line being written
volatile unsigned int character = 0;   // Current character index in line
volatile unsigned int nextline = 0;    // Next line index for processing

volatile unsigned char iot_parse = 0; // Flag to indicate IOT parsing state
volatile unsigned int test_Value = 0; // Test variable for IOT parsing
volatile unsigned char ip_address[16]; // Buffer to hold IP address string
volatile unsigned char boot_state = 0; // State variable for IOT boot process
volatile unsigned char ip_address_found = 0; // Flag to indicate if a NEW IP address has been assigned
volatile unsigned char iot_parse_ready = 0; // Flag to indicate IOT parse ready

// Flags to identify which response line has been parsed
volatile unsigned char ssid_found = 0;       // Set when +CWJAP response is identified
//volatile unsigned char ip_addr_found = 0;    // Set when +CIFSR response with IP is identified

// IOT Boot Sequencing Globals

extern volatile unsigned char init_iot_connection;  // Flag to initiate IOT connection sequence
extern volatile unsigned char boot_next;            // Flag to proceed to next boot state
volatile unsigned char iot_boot_state = 0;       // Current boot state (0-3)

char SET_HOME_WIFI[] = "AT+CWJAP=\"The LAN Before Time\",\"purpleduke1\"\r\n";
char SET_NCSU_WIFI[] = "AT+CWJAP=\"ncsu\",\"\"\r\n";

char SET_MUX_COMMAND[] ="AT+CIPMUX=1\r\n";
char SET_PORT_COMMAND[] ="AT+CIPSERVER=1,55155\r\n";
char REQUEST_SSID_COMMAND[] = "AT+CWJAP?\r\n";
char REQUEST_IP_COMMAND[] ="AT+CIFSR\r\n";

void Connect_IOT(void) {
    // Boot sequence triggered by iot_boot flag set in TB2 CCR1 ISR (100ms)
    // Each state transition occurs via boot_next flag set in TB2 CCR0 ISR (100ms intervals)
    // States: 0->1->2->3->complete, with ~100ms between each transition
    
    if (boot_next) {
        boot_next = 0;  // Consume the flag
        
        GRN_TOGGLE();

        switch (iot_boot_state) {
        case 0:  // State 0: Enable IOT module (released from reset)
            P3OUT |= IOT_EN;  // Set IOT EN pin HIGH
            iot_boot_state++;  // Move to next state
            break;

        case 1:  // State 1: Set Multiplex mode (allow multiple connections)
            iot_boot_state++;
            break;

        case 2:  // State 1: Set Multiplex mode (allow multiple connections)
            iot_boot_state++;
            break;

        case 3:  // State 1: Set Multiplex mode (allow multiple connections)
            iot_boot_state++;
            break;

        case 4: // Debugging with a pause break here
            Send_AT_Command(SET_MUX_COMMAND);
            iot_boot_state++;
            break;

        case 5:  // State 2: Set Server mode (using port 55155)
            Send_AT_Command(SET_PORT_COMMAND);
            iot_boot_state++;
            break;

        case 6: // State 3: Request SSID
            Send_AT_Command(REQUEST_SSID_COMMAND);
            iot_boot_state++;
            break;

        case 7:  // State 4: Request IP address and complete boot
            Send_AT_Command(REQUEST_IP_COMMAND);
            iot_boot_state = 0;  // Reset state machine
            init_iot_connection = 0;  // Clear the boot initiation flag
            TB2CCTL0 &= ~CCIE;  // Disable TB2 CCR0 interrupt (no longer need 100ms ticks)
            break;

        default:
            iot_boot_state = 0;
            init_iot_connection = 0;
            break;
        }
    }
}

void Boot_IOT(void) {
	P3OUT |= IOT_EN;
}

void Reset_IOT(void){
    GRN_ON();
    reset_iot = 1;
    P3OUT &= ~IOT_EN;

    TB2CCTL2 &= ~CCIFG;
    TB2CCR2 = TB2R + TB2CCR2_INTERVAL;      // Set the interrupt timer
    TB2CCTL2 |= CCIE;                       // Enable Reset timer in TB2-CCR2
}

void Enable_Comms(void) {
    static unsigned char temp;
    allow_comms = TRUE;

    temp = usb_rx_wr++;
    SEND_2_IOT[temp] = 'X';  // Simulate receiving a character
    if (usb_rx_wr >= sizeof(SEND_2_IOT)) {
        usb_rx_wr = BEGINNING;
    }
    UCA0IE |= UCTXIE;  // Enable transmission
}

void Process_Command(void) {
    if (!command_ready) return;
    
    command_ready = 0;  // Consume the command flag
    
    // TESTING command processing
    if (command_index >= 2) {
        if (command_buffer[0] == '^' && command_buffer[1] == '^') { 	// "^^" command - respond immediately
            Send_Response("I'm here\r\n");
        }

		// CHANGE LOGIC BELOW WHEN NEEDED BUT WORKS FOR NOW
        else if (command_buffer[0] == '^' && command_index >= 3) {		// Single '^' command, checks if index passed command char. 
            switch (command_buffer[1]) {
                case 'F':  												// ^F - Fast baud rate
                    Set_Baud_115200();
                    Send_Response("115,200\r\n");
                    break;
                    
                case 'S':  												// ^S - Slow baud rate  
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
    while (response[i] != 0 && i < sizeof(IOT_2_PC) - 1) {		// Copy response to IOT_2_PC buffer for transmission to PC
        IOT_2_PC[iot_rx_wr++] = response[i++];
        if (iot_rx_wr >= sizeof(IOT_2_PC)) {
            iot_rx_wr = BEGINNING;
        }
    }
        UCA1IE |= UCTXIE;  										// Enable transmission to PC
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
// }

void Send_AT_Command(const char* msg)
{
    if (!allow_comms) { return; }
    GRN_TOGGLE();
    // Copy up to 32 bytes into IOT_TX_Buffer
    unsigned int i = 0;
    while (msg[i] != '\0' && i < PROCESS_BUFFER_SIZE) {
        SEND_2_IOT[i] = msg[i];
        i++;
    }
    SEND_2_IOT[i] = '\0'; // Null terminate for safety
    iot_tx_len = i;
    iot_tx_index = 0;
    UCA0IE |= UCTXIE; // Enable UCA0 TX interrupt to start transmission

    // ALT MODE BELOW:::
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
}


void IOT_Process(void) {               // Process IOT messages
    int i;
    unsigned int iot_rx_wr_temp;

    // Process all available characters in IOT_2_PC ring buffer
    while (1) {
        iot_rx_wr_temp = iot_rx_wr;

        if (iot_rx_wr_temp == iot_rx_rd) {  // Ring buffer empty, exit loop
            break;
        }

        // Read one character from ring buffer
        IOT_Data[line][character] = IOT_2_PC[iot_rx_rd++];

        if (iot_rx_rd >= sizeof(IOT_2_PC)) {
            iot_rx_rd = BEGINNING;
        }

        if (IOT_Data[line][character] == 0x0A) {
			IOT_Data[line][character] = '\0'; // Null terminate line
            iot_parse = 0;
            iot_parse_ready = 1;  // Signal that a complete line is ready for parsing
            character = BEGINNING;
            line++;

            if (line >= PROCESS_BUFFER_LINES) {
                line = BEGINNING;
            }

            nextline = line + 1;

            if (nextline >= PROCESS_BUFFER_LINES) {
                nextline = BEGINNING;
            }
        }
        else {
            switch (character) 
            { // Check chars as they come in to pre-process response
            case  0:
                if (IOT_Data[line][character] == '+') {   // Got "+"                    
                    
                    test_Value++;
                    if (test_Value) {
                        RED_ON();
                    }
                    
                    iot_parse = 1;
                }
                break;

            case  1:
                // GRN_LED_ON;
                break;

            case  2:
                // Position 2: Check for 'W' (CWJAP) or 'I' (CIFSR)
//                if (IOT_Data[line][character] == 'W') {   // Got "+CW..." (CWJAP - SSID response)
//                    ssid_found = 1;
//                }
//                else if (IOT_Data[line][character] == 'I') {   // Got "+CI..." (CIFSR - IP response)
//                    ip_addr_found = 1;
//                }
                break;

            case  4:
//                if (IOT_Data[line][character] == 'y') {   // Got read"y"
//                    for (i = 0; i < sizeof(AT); i++) {
//                        IOT_TX_buf[i] = AT[i];
//                    }
//
//                    UCA0IE |= UCTXIE;
//                    boot_state = 1;
//                    // RED_LED_ON;
//                    GRN_LED_OFF;
//                }
                break;

            case  5:
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

            case  10:
                if (IOT_Data[line][character] == 'I') {
                    ip_address_found = 1;
                    strcpy(display_line[1], "IP Address");

                    for (i = 0; i < sizeof(ip_address); i++) {
                       ip_address[i] = 0;
                    }

                    display_changed = 1;
                    // iot_index = 0;
                }
                break;

            case 14:
                if (ip_address_found){
                    for (i = 0; i < sizeof(ip_address); i++){
                        if(IOT_Data[line][character + i] == '"'){
                            ip_address[i] = '/r';
                            ip_address[i+1] = '/n';
                            ip_address_found = FALSE;
                            break;
                        }
                        else{
                            ip_address[i] = IOT_Data[line][character + i];
                        }
                    }
                }

            
            default: break;
            }
            character++;
        }
    }  // End of while loop - process all available characters
}



// Helper function: Center text on a 10-character LCD line
// Input: src string, destination buffer (10 chars), length to use
void center_text(const unsigned char *src, unsigned char *dest, unsigned int len) {
    int i, padding;
    
    // Limit length to 10 chars max
    if (len > 10) len = 10;
    
    // Clear destination
    for (i = 0; i < 10; i++) {
        dest[i] = ' ';
    }
    
    // Calculate left padding to center the text
    padding = (10 - len) / 2;
    
    // Copy text to center position
    for (i = 0; i < len && src[i] != '\0'; i++) {
        dest[padding + i] = src[i];
    }
}


// Helper function: Extract SSID from +CWJAP response
// Format: "+CWJAP:"ssid","...
// Returns length of SSID extracted (max 10)
unsigned int extract_ssid(const unsigned char *line, unsigned char *ssid_buf) {
    unsigned int i = 0, ssid_len = 0;
    unsigned char in_quotes = 0;
    
    // Find first quote after +CWJAP:
    while (line[i] != '\0') {
        if (line[i] == '"') {
            in_quotes = 1;
            i++;
            break;
        }
        i++;
    }
    
    // Extract SSID until closing quote or max 10 chars
    while (line[i] != '\0' && line[i] != '"' && ssid_len < 10) {
        ssid_buf[ssid_len++] = line[i++];
    }
    ssid_buf[ssid_len] = '\0';
    
    return ssid_len;
}


// Helper function: Extract IP address from +CIFSR:APIP response
// Format: "+CIFSR:APIP,"192.168.4.1"
// Returns length of IP extracted
unsigned int extract_ip_address(const unsigned char *line, unsigned char *ip_buf) {
    unsigned int i = 0, ip_len = 0;
    unsigned char found_apip = 0;
    
    // Look for "APIP," in the line
    while (line[i] != '\0') {
        if (line[i] == 'A' && line[i+1] == 'P' && line[i+2] == 'I' && line[i+3] == 'P' && line[i+4] == ',') {
            found_apip = 1;
            i += 5;  // Move past "APIP,"
            break;
        }
        i++;
    }
    
    if (!found_apip) return 0;

    // Find the opening quote
    while (line[i] != '\0' && line[i] != '"') {
        i++;
    }
    if (line[i] == '"') i++;  // Move past the quote

    // Extract IP address until closing quote or end
    while (line[i] != '\0' && line[i] != '"' && ip_len < 15) {
        ip_buf[ip_len++] = line[i++];
    }
    ip_buf[ip_len] = '\0';
    
    return ip_len;
}

// Helper function: Parse IP address into groups and extract first 2 and last 2 groups
// Input: IP address string ("192.168.4.1")
// Output: first_two ("192.168"), last_two ("4.1")
void parse_ip_groups(const unsigned char *ip_addr, unsigned char *first_two, unsigned char *last_two) {
    unsigned int i = 0, dot_count = 0;
    unsigned int second_dot_pos = 0;
    
    // Find the position of the second dot
    for (i = 0; ip_addr[i] != '\0'; i++) {
        if (ip_addr[i] == '.') {
            dot_count++;
            if (dot_count == 2) {
                second_dot_pos = i;
                break;
            }
        }
    }
    
    // Copy first two groups NOT including second dot
    for (i = 0; i < second_dot_pos; i++) {
        first_two[i] = ip_addr[i];
    }
    first_two[i] = '\0';
    
    // Copy last two groups (character after second dot to end)
    if (ip_addr[second_dot_pos + 1] != '\0') {
        strcpy((char*)last_two, (char*)(ip_addr + second_dot_pos + 1));
    }
}


// Main parsing function: Gated in main by iot_parse_ready flag
// Parse IOT_Data buffer to extract SSID and IP, populate display_line[]
void Parse_IOT_Data(void) {
    unsigned int i, len;
    unsigned char ssid_buf[11] = {0};      // SSID (max 10 + null)
    unsigned char ip_buf[16] = {0};        // IP address (max 15 + null)
    unsigned char first_two[16] = {0};     // First two IP groups
    unsigned char last_two[16] = {0};      // Last two IP groups
    int updated = 0;

    // Scan through IOT_Data buffer to find +CWJAP and +CIFSR responses
    for (i = 0; i < PROCESS_BUFFER_LINES; i++) {
        // Only process non-empty lines
        if (IOT_Data[i][0] == '\0') continue;

        // Debug Testing:::raw data
        // strcpy(display_line[0], IOT_Data[i]); display_changed=1;

        // SSID: Check for +CWJAP
        if (strstr(IOT_Data[i], "+CWJAP:") != NULL) {
            len = extract_ssid(IOT_Data[i], ssid_buf);
            if (len > 0) {
                // Center SSID on line 0
                center_text(ssid_buf, (unsigned char*)display_line[0], len);
                display_line[0][10] = '\0';
                updated = 1;
                RED_ON();  // Debug: SSID found
            }
        }

        // IP Address: Check for +CIFSR:APIP
        if (strstr(IOT_Data[i], "APIP,") != NULL) {
            len = extract_ip_address(IOT_Data[i], ip_buf);
            if (len > 0) {
                // Parse IP into first two and last two groups
                parse_ip_groups(ip_buf, first_two, last_two);
                
                // Line 1: "IP address" centered
                center_text((const unsigned char*)"IP address", (unsigned char*)display_line[1], 10);
                display_line[1][10] = '\0';
                
                // Line 2: First two IP groups centered
                len = strlen((char*)first_two);
                center_text(first_two, (unsigned char*)display_line[2], len);
                display_line[2][10] = '\0';
                
                // Line 3: Last two IP groups centered
                len = strlen((char*)last_two);
                center_text(last_two, (unsigned char*)display_line[3], len);
                display_line[3][10] = '\0';

                updated = 1;
                GRN_ON();  // Debug: IP found
            }
        }
    }

    if (updated) {
        display_changed = 1;
    }
    
    // Reset parse ready flag
    iot_parse_ready = 0;
//    ssid_found = 0;
//    ip_addr_found = 0;
}


void A0_transmit(void) {
    unsigned int i = 0;
	// Contents must be in process_buffer
	// End of Transmission is identified by NULL character in process_buffer
	// process_buffer includes Carriage Return and Line Feed
	while (process_buffer[i] != '\0') {
		IOT_TX_buf[i] = process_buffer[i++];
		if (i >= sizeof(IOT_TX_buf)) {
			break;											// Prevent buffer overflow
		}
		if (i < sizeof(IOT_TX_buf)) {
			IOT_TX_buf[i] = '\0';							// Null terminate the buffer
		}
		else {
			IOT_TX_buf[sizeof(IOT_TX_buf) - 1] = '\0';		// Ensure buffer is null terminated
		}
	}
	iot_tx = BEGINNING;									// Set Array index to first location
	UCA0IE |= UCTXIE;									// Enable TX interrupt
}

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
//        SEND_BUFFER[iot_rx_wr-1] = NULL;      // Null Proc_Buffer
        if (iot_rx_wr >= sizeof(IOT_2_PC)){
            iot_rx_wr = BEGINNING;
        }
    }
//    IOT_2_PC[A1_tx_char] = '\0';
//    //tx_index = BEGINNING;
//    iot_rx_wr = A1_tx_char;

    UCA1IE |= UCTXIE;
}


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

	int i;
	for (i = 0; i < SMALL_RING_SIZE; i++) {
		USB_Char_Rx[i] = 0x00;
	}
	usb_rx_wr = BEGINNING;
	usb_rx_rd = BEGINNING;
	//for (i = 0; i < LARGE_RING_SIZE; i++) {		// May not use this
	//	USB_Char_Tx[i] = 0x00;						// USB Tx Buffer
	//}
	usb_tx_ring_wr = BEGINNING;
	usb_tx_ring_rd = BEGINNING;

	// Configure UART 0
	UCA0CTLW0 = 0;
	UCA0CTLW0 |=  UCSWRST;			// Put eUSCI in reset
	UCA0CTLW0 |=  UCSSEL__SMCLK;	// Set SMCLK as fBRCLK
	UCA0CTLW0 &= ~UCMSB;			// MSB, LSB select
	UCA0CTLW0 &= ~UCSPB;			// UCSPB = 0(1 stop bit) OR 1(2 stop bits)
	UCA0CTLW0 &= ~UCPEN;			// No Parity
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



