/*
 * queue.c
 *
 *  Created on: Nov 23, 2025
 *      Author: Dallas.Owens
 */


#include  "timers.h"
#include "msp430.h"
#include "ports.h"
#include "macros.h"
#include "functions.h"
#include  <string.h>
#include <stdio.h>
#include "UART.h"
#include "queue.h"
#include "wheels.h"
#include "calibration.h"
#include  "LED.h"
#include "PWM.h"

 // COMMAND QUEUE GLOBALS_______________________________________________________________________________
unsigned char Command_Buffer[MAX_COMMANDS][COMMAND_BUFFER_SIZE];
unsigned char command_index = BEGINNING;
volatile unsigned char command_ready = FALSE;

extern const unsigned char Command_PIN[];
unsigned char current_command[COMMAND_BUFFER_SIZE];

unsigned char cmd_queue_write = BEGINNING;
unsigned char cmd_queue_read = BEGINNING;
unsigned char cmd_queue_count = 0;

volatile unsigned int command_timer = 0;
volatile unsigned char command_active = FALSE;
volatile unsigned char command_complete = FALSE;
unsigned char command_waiting = FALSE;
unsigned char command_enabled = TRUE;

// WiFi PAD variable
unsigned char wifi_pad_value = 0;  // Holds current PAD value


void Process_Queue(void) {
    if (!command_enabled) return;
	if (!command_waiting) { return; }               // RETURN if no commands waiting
	// COMMAND ACTIVE: Check if command is active - wait for completion
    if(command_active){
		if (command_complete) {                 // COMMAND COMPLETE LOGIC
            command_complete = FALSE;                   // Consume flags
            command_active = FALSE;                     
			PWM_FULLSTOP();                             // Always stop PWM on command complete
		    if(cmd_queue_count == 0){ command_waiting = FALSE; }

            //lcd_4line();
			strcpy(display_line[1], "   IDLE   ");      // Display idle status
            display_changed = TRUE;
		}
		return;                                         // RETURN if command still active (dont process next)
    }

	ParsedCommand cmd;
	if (Get_Command()) {                                // COMMAND RETRIEVED AND DEQUEUED
		cmd = Parse_Command();                               // Parse the command
		Display_CurrentCommand();                               // Display the current command
		
        if (cmd.valid) {                                        // If command is valid, execute it
            command_active = TRUE;

            // Only initialize timer if not already running
            if (!(TB1CCTL2 & CCIE))
            {
                TB1CCR2 = TB1R + TB1CCR2_INTERVAL;
                TB1CCTL2 |= CCIE;
            }

            switch (cmd.direction) 
            {
                // IoT MOVEMENT
			case 'F':
				Send_Response("Moving Forward\r\n");
                PWM_FORWARD();
				command_timer = COMMAND_DELAY(cmd.duration);
				break;
			case 'B':
				Send_Response("Moving Backward\r\n");
                PWM_REVERSE();
                command_timer = COMMAND_DELAY(cmd.duration);
                break;
			case 'R':
				Send_Response("Turning Right\r\n");
                PWM_ROTATE_RIGHT();
                command_timer = COMMAND_DELAY(cmd.duration);
                break;
			case 'L':
				Send_Response("Turning Left\r\n");
                PWM_ROTATE_LEFT();
                command_timer = COMMAND_DELAY(cmd.duration);
                break;

                // LINE FOLLOWING
//            case 'S':
//                Send_Response("Skynet Protocol Invoked\r\n");
//                Line_Follow_Setup();
//                command_active = FALSE;  // Line follow handles its own state
//                break;

            case 'T':
				Send_Response("Skynet Protocol Invoked\r\n");
                Line_Follow_Start_Autonomous();
                command_active = FALSE;  // Line follow handles its own state
                break;
            
            case 'I':
                // TURN LEFT FIRST
                Send_Response("Skynet Protocol Invoked\r\n");
                Line_Follow_Start_LEFT_TURN();
                command_active = FALSE;  // Line follow handles its own state
                break;
            case 'D':
				// TURN RIGHT FIRST
                Send_Response("Skynet Protocol Invoked\r\n");
                Line_Follow_Start_RIGHT_TURN();
                command_active = FALSE;  // Line follow handles its own state
                break;

			case 'X':
                Send_Response("Exiting Circle\r\n");
                Line_Follow_Exit_Circle();
                command_active = FALSE;  // Don't use timer for this command
                break;
			case 'C':
				Send_Response("Calibrating IR Sensors\r\n");
                IR_Calibrate_Menu();
                command_active = FALSE;  // Calibration handles its own state
                break;

			case 'P':
				// WiFi PAD increment command - format D0000
				// Increments wifi_pad_value and displays it
				Send_Response("WiFi PAD Increment\r\n");
				wifi_pad_value++;

                strcpy(display_line[1], "Arrived 0 ");
                display_line[0][9] = (wifi_pad_value + '0'); // Convert to ASCII
                display_line[0][10] = '\0';

                display_changed = TRUE;
				command_active = FALSE;  // No timing needed, just display update
				break;

			case 'E':
				// WiFi PAD set command - format E000x where x is the value to set
				// Sets wifi_pad_value to cmd.duration and displays it
				Send_Response("WiFi PAD Set\r\n");
				wifi_pad_value = cmd.duration;

                strcpy(display_line[1], "Arrived 0 ");
                display_line[0][9] = (wifi_pad_value + '0'); // Convert to ASCII
                display_line[0][10] = '\0';

                display_changed = TRUE;
				command_active = FALSE;  // No timing needed, just display update
				break;

            case 'Y':
                // Setup LEFT - Drive forward 2s, rotate left
                Send_Response("Setup Left\r\n");
                Line_Follow_Setup_LEFT();
                command_active = FALSE;
                break;

            case 'Z':
                // Setup RIGHT - Drive forward 2s, rotate right
                Send_Response("Setup Right\r\n");
                Line_Follow_Setup_RIGHT();
                command_active = FALSE;
                break;


			default:
				Send_Response("Bad Command (Letter)\r\n");
				break;
			}
		}
		else {
			Send_Response("Bad Command\r\n");                               // Notify invalid command
		}
	}
}


void Queue_AddCommand(const char* cmd_string) {
    if (cmd_string[0] < 'A' || cmd_string[0] > 'Z') {
            Send_Response("Invalid Cmd Format\r\n");
            return;  // Reject malformed commands
        }

        // Check minimum length (letter + 4 digits = 5 chars minimum)
        unsigned int len = 0;
        while (cmd_string[len] != '\0' && len < COMMAND_BUFFER_SIZE) {
            len++;
        }

        if (len < 5) {
            Send_Response("Cmd Too Short\r\n");
            return;  // Reject partial commands
        }

        if (cmd_queue_count >= MAX_COMMANDS) {
            Send_Response("Queue Full\r\n");
            return;
        }

    if (cmd_queue_count >= MAX_COMMANDS) {    // Check if queue is full
        Send_Response("Queue Full\r\n");
        return;
    }

    // Copy to queue
    unsigned int i;
    for (i = 0; cmd_string[i] != '\0' && i < COMMAND_BUFFER_SIZE - 1; i++) {    // Write command str argument to Command Buffer
        Command_Buffer[cmd_queue_write][i] = cmd_string[i];                     //      for actionable processing

    }
    Command_Buffer[cmd_queue_write][i] = '\0';                                  // Null terminate

    cmd_queue_write++;                                      // inc. write pointer
    if (cmd_queue_write >= MAX_COMMANDS) {
        cmd_queue_write = BEGINNING;                        // Wrap around on overflow
    }
    cmd_queue_count++;                                      // Increase queue count tracker
    command_waiting = TRUE;                                 // Let main know there is a command waiting for action

    
    GRN_TOGGLE();                                           // Visual feedback
}

unsigned char Get_Command(void) {                           // Returns TRUE if a command was dequeued, FALSE if queue is empty
                                                            // Logic from previous debugging but works fine with implementation. ONLY CHANGE IF NEEDED BUT REMOVE RETURNED LOGIC ASSIGNMENT REFERENCES!!
    if (cmd_queue_count == 0) {                             // Queue is empty
        command_waiting = FALSE;                                    // No commands waiting for main
        return FALSE;                                               // No commands retrieved
    }
    
    unsigned int i;
    for (i = 0; i < COMMAND_BUFFER_SIZE; i++) {                     // Copy command from queue to "current_command" buffer
        current_command[i] = Command_Buffer[cmd_queue_read][i];     // Assign command for main to process now
        if (current_command[i] == '\0') {
            break;
        }
    }
    for(i = 0; i < COMMAND_BUFFER_SIZE; i++){
        Command_Buffer[cmd_queue_read][i] = '\0';           // Clear the command from the queue after retrieved to avoid multiple calls
    }

    cmd_queue_read++;                                       // Advance read pointer
    if (cmd_queue_read >= MAX_COMMANDS) {
        cmd_queue_read = 0;                                 // Wrap around to first queue position if reached last
    }                                               
    cmd_queue_count--;                                      // Decrement count
    return TRUE;                                            // Command removed from queue for processing
}

ParsedCommand Parse_Command(void)
{                                                           // Struct to hold relevant portions of parsed commands for easy access
    ParsedCommand cmd;
    cmd.valid = 0;
    cmd.direction = 0;
    cmd.duration = 0;

    // Command format: F1000 (direction + 4 digits)
    // Index 0: Direction letter
    cmd.direction = current_command[0];

    // Validate direction
    if (cmd.direction != 'F' && cmd.direction != 'B' &&
        cmd.direction != 'R' && cmd.direction != 'L' &&
        cmd.direction != 'T' && cmd.direction != 'X' &&
        cmd.direction != 'C' && cmd.direction != 'P' && 
        cmd.direction != 'E' && cmd.direction != 'I' &&  
        cmd.direction != 'D' && cmd.direction != 'S' &&
        cmd.direction != 'Y' && cmd.direction != 'Z')   
    {
        return cmd;                                                 // Invalid direction
    }

    // Extract duration (up to 4 digits)
    // increase extraction if needed, but handle display of characters from command too
    unsigned int i;
    for (i = 1; i < 5 && current_command[i] != '\0'; i++) {
        if (current_command[i] >= '0' && current_command[i] <= '9') {
            cmd.duration = (cmd.duration * 10) + (current_command[i] - '0');        // Shift and add ascii digit function from 309
        }
        else {
            return cmd;                                             // Non-digit found, not valid flag
        }
    }
    cmd.valid = 1;                                                  // Dont use bad commands - undefined behavior
    return cmd;
}

void Display_CurrentCommand(void) {
    unsigned int i = 0;
    strcpy(display_line[3], "          ");                              // Clear old data
    for (i = 0; i < 10 && current_command[i] != '\0'; i++)    {         // Because otherwise it wont clear the line...
        display_line[3][i] = current_command[i];                                // Resolution: 'current_command' ended in NULL (\0) and pasted. update display stopped at first NULL... Duh...
    }                                                                           // This version stops at NULL on copy, but does not copy it, leaving spaces above in place and recognized
    display_line[3][10] = '\0';                                         // Ensure null termination
    //lcd_BIG_mid();                                                      // Display on enlarged middle line
    display_changed = TRUE;                                             // Flag for update
}
