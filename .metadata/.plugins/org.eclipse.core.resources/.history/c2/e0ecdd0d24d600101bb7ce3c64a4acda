/*
 * menu.c
 *
 *  Created on: Nov 8, 2025
 *      Author: Dallas.Owens
 */

#include  "msp430.h"
#include  "macros.h"
#include  "functions.h"
#include  "ports.h"
#include  "timers.h"
#include  "LCD.h"
#include  "menu.h"
#include  "switches.h"
#include  "ADC.h"
#include  "calibration.h"
#include  "DAC.h"
#include  "LED.h"
#include  <string.h>
#include  "bootup.h"
#include  "UART.h"
#include "wheels.h"

extern volatile unsigned char display_changed;
extern unsigned char display_ssid;
extern unsigned char display_ip;
extern unsigned char command_mode;


menu_page_t menu_index = MAIN_MENU;
unsigned char display_menu = FALSE;
unsigned int prev_thumb = 0;

// Get resolution per page index
// can be used for varying number of menu/submenu items
unsigned int Determine_Index_Resolution(unsigned int num) {
    return (ADC_Thumb * num) / 1024;									// Update before use for current ADC value range
}


void Menu_ChangePage(void)
{
    if(prev_thumb == 0){
        prev_thumb = ADC_Thumb;
    }
    int diff = (int)ADC_Thumb - (int)prev_thumb;

    // Scroll down (decrease menu index)
    if ((diff < -MENU_SCROLL) && (menu_index > MAIN_MENU)) {
        prev_thumb = ADC_Thumb;
        menu_index--;
        display_menu = TRUE;
        lcd_4line();
    }

    // Scroll up (increase menu index)
    else if ((diff > MENU_SCROLL) && (menu_index < MENU_NUM)) {
        prev_thumb = ADC_Thumb;
        menu_index++;
        display_menu = TRUE;
        lcd_4line();
    }

}


void Main_Menu_Process(void) 
{
    static unsigned char switch_reset = 0;
	Menu_ChangePage();

	switch (menu_index)
	{
	case MAIN_MENU:
		if(display_menu){
			strcpy(display_line[0], "  NEW CAR ");
			strcpy(display_line[1], "  WHO DIS ");
			strcpy(display_line[2], "  Calib-->");
			strcpy(display_line[3], "<--LnFollw");
			display_changed = TRUE;
        	display_menu = FALSE;
		}
		if (SW1_pressed) {
			SW1_pressed = FALSE;
		    Sample_ADC();                                               // START ADC SAMPLING FOR LINE DETECTION
			Line_Follow_Start_Autonomous();
			display_default();
			display_menu = FALSE;
		}
        if (SW2_pressed) {
            SW2_pressed = FALSE;
            if(!calibrating && !process_line_follow){
                IR_Calibrate_Menu();
            }
        }
		break;

	case IOT_MENU:
		if (display_menu){
			strcpy(display_line[0], " IOT MENU ");
			strcpy(display_line[1], "==========");
			strcpy(display_line[2], "  Reset-->");
			strcpy(display_line[3], "<--Dsply  ");
			display_menu = FALSE;
			display_changed = TRUE;
		}
		if (SW1_pressed) {
			SW1_pressed = FALSE;
			display_ssid = TRUE;
			display_ip = TRUE;
			Display_IOT_Parse();
		}
		if (SW2_pressed) {
			SW2_pressed = FALSE;
			IOT_Reset();
			switch_reset = TRUE;
		}
		if (switch_reset){
		    IOT_Connect();
		    if (iot_boot_complete) {          // Check if IoT boot complete
		        strcpy(display_line[2], "IOT Ready ");
		        display_changed = TRUE;
		        iot_boot_complete = FALSE;
		        switch_reset = FALSE;
		    }
		}
		break;

	case CALIBRATION_MENU:
		if (display_menu){
			strcpy(display_line[0], "CALIBRATE ");
			strcpy(display_line[1], "==========");
			strcpy(display_line[2], "  Calib-->");
			strcpy(display_line[3], "<--Dsply  ");
			display_menu = FALSE;
			display_changed = TRUE;
		}
		if (SW1_pressed) {
			SW1_pressed = FALSE;
			if(!calibrating){
			    Display_Calibration();
			}
		}
		if (SW2_pressed) {
			SW2_pressed = FALSE;
			if(!calibrating){
			    IR_Calibrate_Menu();
			}
		}
		break;

	case ADC_MENU:
		if (display_menu){
  			display_ADC();
			display_menu = FALSE;
			display_changed = TRUE;
		}
		if (display_thumb) {
			display_thumb = FALSE;
			HEXtoBCD(ADC_Thumb);
			adc_line(2, 6);
			display_changed = TRUE;
		}
		if (display_left_detect) {
			display_left_detect = FALSE;
			HEXtoBCD(CAR_Right_Detect);
			adc_line(4, 0);                             // Display on line 4 - LEFT side
			display_changed = TRUE;
		}
		if (display_right_detect) {
			display_right_detect = FALSE;
			HEXtoBCD(CAR_Left_Detect);
			adc_line(4, 6);                             // Display on line 4 - RIGHT side
			display_changed = TRUE;
		}
		break;

	default:
		menu_index = MAIN_MENU;
		display_menu = TRUE;
		display_changed = TRUE;
		break;
	}
}


