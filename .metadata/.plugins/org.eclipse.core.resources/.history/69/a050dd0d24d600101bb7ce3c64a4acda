/*
 * Display.c
 *
 *  Created on: Sep 9, 2025
 *      Author: Dallas.Owens
 */

#include  "timers.h"
#include  "msp430.h"
#include  "macros.h"
#include  "functions.h"
#include  "ports.h"
#include  "switches.h"
#include  "LCD.h"
#include  "UART.h"
#include  <string.h>
#include  "ADC.h"
#include  "wheels.h"
#include  "DAC.h"
#include  "calibration.h"
#include  "LED.h"


extern unsigned char SMCLK_SETTING;
extern unsigned char state;
extern unsigned char step;

extern volatile unsigned char SW1_display,  SW2_display;
extern volatile unsigned char default_display;

extern volatile unsigned char ADC_Channel;

extern volatile unsigned char display_changed;
extern volatile unsigned char backlite_timer;

extern unsigned char drive_enabled;
extern unsigned char display_ADC_values;

extern unsigned char BaudRate;


// _____________ DISPLAY GOVERNING FUNCTION (Backup) _________
// if (update_display) {
//    update_display = 0;
//    if (display_changed) {
//        display_changed = 0;
//        Display_Update(0, 0, 0, 0);
//    }
//}
// ___________________________________________________________


void Display_Process(void)
{
    if (update_display)
    { // Update display if ready
        update_display = 0;
        if (display_changed) {
            display_changed = 0;
            Display_Update(0, 0, 0, 0);
            Backlite_Control();
        }
    }
}

void Backlite_Control(void){
    backlite_timer = LCD_BACKLITE_DURATION;
    LCD_BACKLITE_DIMING = LCD_BACKLITE_DIM;
    TB2CTL &= ~TBIFG;                       // Clear Overflow Interrupt flag
    TB2CTL |= TBIE;                          // Enable Overflow Interrupt
}


void display_default(void){
    strcpy(display_line[0], "Arrived 00");
    strcpy(display_line[1], "  Dallas  ");
    strcpy(display_line[2], "   OWENS  ");
    strcpy(display_line[3], "          ");
    display_changed = TRUE;
}

//void display_default(void) {
//    strcpy(display_line[0], "  Waiting ");
//    strcpy(display_line[1], " for input");
//    strcpy(display_line[2], " DESTROY ");
//    strcpy(display_line[3], "ALL HUMANS");
//    display_changed = TRUE;
//}


// *********** DISPLAY FUNCTIONS ********************
// _____ Homework 9 ________________
void display_song(unsigned int idx, char* str) {
    unsigned int i;
    for (i = 0; i < 10; i++) {
		display_line[2][i] = str[idx + i];
		if (str[i] == '\0') {
			break;
		}
    }
	display_changed = TRUE;
}


void display_banner(unsigned char line, unsigned int idx, const char* str) 
{
    display_line[line][0] = display_line[line][1];
	display_line[line][1] = display_line[line][2];
	display_line[line][2] = display_line[line][3];
	display_line[line][3] = display_line[line][4];
	display_line[line][4] = display_line[line][5];
	display_line[line][5] = display_line[line][6];
	display_line[line][6] = display_line[line][7];
	display_line[line][7] = display_line[line][8];
	display_line[line][8] = display_line[line][9];
	display_line[line][9] = str[idx];
	display_line[line][10] = '\0';
	display_changed = TRUE;
}


// _____ Homework 8 __________________
void Display_Baud(void) 
{
    strcpy(display_line[0], "          ");
    strcpy(display_line[1], "         ");
    strcpy(display_line[2], "   Baud   ");

    switch (BaudRate) {
	case BAUD_115200:
		strcpy(display_line[3], "  115200  ");
		break;

	case BAUD_460800:
		strcpy(display_line[3], "  460800  ");
		break;

	default:
		strcpy(display_line[3], "  ERROR   ");
		break;
    }
	display_changed = TRUE;
}


//_____ Project 6 ____________________
void Display_Process_Project6(void)
{ // Check if new ADC results
    if(drive_enabled) {
        Display_State();
    }
    if(display_ADC_values) {

        if (display_thumb) {
            display_thumb = FALSE;
            HEXtoBCD(ADC_Thumb);
            adc_line(2, 6);
            display_changed = TRUE;
        }
        if (display_left_detect) {
            display_left_detect = FALSE;
            HEXtoBCD(ADC_Left_Detect);                  // Convert to BCD for display
            adc_line(4, 0);                             // Display on line 4 - LEFT side
            display_changed = TRUE;
        }
        if (display_right_detect) {
            display_right_detect = FALSE;
            HEXtoBCD(ADC_Right_Detect);
            adc_line(4, 6);                             // Display on line 4 - RIGHT side
            display_changed = TRUE;
        }
    }
    if (update_display)
    { // Update display if ready
        update_display = 0;
        if (display_changed) {
            display_changed = 0;
            Display_Update(0, 0, 0, 0);
        }
    }
}


//_____ BLACK LINE HELPERS ____________

void display_black_line_detected(void) {
    strcpy(display_line[2], "Black Line");
    strcpy(display_line[3], " Detected ");
    display_changed = TRUE;
}

void display_detect_values(void) {    // Display both sensor values when over black line
    HEXtoBCD(ADC_Left_Detect);
    adc_line(2, 0);  // Left value on line 2, left side

    HEXtoBCD(ADC_Right_Detect);
    adc_line(2, 6);  // Right value on line 2, right side

    strcpy(display_line[3], "  ONLINE  ");
    display_changed = TRUE;
}


void display_detect_difference(void) {    // Show which sensor is detecting stronger signal
    int difference = (int)ADC_Left_Detect - (int)ADC_Right_Detect;

    if (difference > HORSE_GRENADES) {
        strcpy(display_line[3], "LeftStrong");
    } else if (difference < -HORSE_GRENADES) {
        strcpy(display_line[3], "RightStrng");
    } else {
        strcpy(display_line[3], " CENTERED ");
    }
    display_changed = TRUE;
}


//_____ HEX DISPLAY HELPERS ____________
void display_hex(char hex[4]) {                  // "---3456---"
	//strcpy(display_line[3], "  ADC VAL ");
    display_line[3][3] = hex[0];                 
    display_line[3][4] = hex[1];
    display_line[3][5] = hex[2];
    display_line[3][6] = hex[3];
	display_changed = TRUE;
}

void display_emitter(void){
    strcpy(display_line[0], "IR:       ");

}

void display_ADC(void) {
    strcpy(display_line[0], "IR:    ON ");
    strcpy(display_line[1], "THUMB:    ");
    strcpy(display_line[2], "RIGHT LEFT");
    strcpy(display_line[3], "          ");
    display_changed = TRUE;
}


void set_display(const char* text, unsigned char line) {
    if (strlen(text) > 10) {
        strcpy(display_line[1], "TXT 2 LONG");
    }
    else {
        strcpy(display_line[line], text);
        display_line[line][10] = '\0';         // Weirdness possibly due to no termination of string?
    }
    display_changed = TRUE;
}

void set_display_line4(const char* text) {
    if (strlen(text) > 10) {
        strcpy(display_line[3], "TXT 2 LONG");
    }
    else {
        strcpy(display_line[1], text);
        display_line[3][10] = '\0';         // Weirdness possibly due to no termination of string?
    }
    display_changed = TRUE;
}







