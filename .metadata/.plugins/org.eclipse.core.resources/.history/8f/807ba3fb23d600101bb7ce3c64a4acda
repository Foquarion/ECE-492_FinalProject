/*
 * menu.h
 *
 *  Created on: Nov 8, 2025
 *      Author: Dallas.Owens
 */

#ifndef MENU_H_
#define MENU_H_

#define ADC_MAX_RESOLUTION	(1024)

#define MENU_SCROLL		    (100)
#define SCROLL_RESOLUTION	(20)

#define MENU_NUM			(4)

typedef enum {
    MAIN_MENU = 1,
    IOT_MENU = 2,
    CALIBRATION_MENU = 3,
    ADC_MENU = 4
} menu_page_t;

extern menu_page_t menu_index;


unsigned int Determine_Index_Resolution(unsigned int num);
void Menu_ChangePage(void);
void Main_Menu_Process(void);

#endif /* MENU_H_ */
