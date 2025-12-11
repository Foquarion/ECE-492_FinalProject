/*
 * ports.c -    Initialize ports and configure functionality (Init.values, direction, sp.functions, pull up/down res., etc.)
 *
 *  Created on: Sep 9, 2025
 *      Author: Dallas.Owens
 */


/*
 * Complete the following functions
 */

#include "macros.h"
#include  "functions.h"
#include  "msp430.h"
#include "ports.h"
#include  "LED.h"


extern unsigned char SMCLK_SETTING;


void Init_LCD_Backlite(void){

}


void Init_Ports(void){
    Init_Port1();
    Init_Port2();
    Init_Port3(USE_GPIO);
    Init_Port4();
    Init_Port5();
    Init_Port6();
}


void Init_Port1(void){
    P1SELC = 0X00;          // Set all to gpio
    P1OUT = 0X00;           // P1 Set Low
    P1DIR = 0X00;           // Set P1 direction to output

//    P1SEL0 &= ~SENS0_R;           // (0)   RED_LED GPIO operation
//    P1SEL1 &= ~SENS0_R;           // (0)   RED_LED GPIO operation
//    P1OUT  &= ~SENS0_R;           // (0)   Initial Value = Low / Off
//    P1DIR  &= ~SENS0_R;           // (1)   Direction = output
    P1SELC |= SENS0_R;

    P1SELC |= SENS0_L;

    P1SELC |= SENS1_L;         // (11) ADC input

    P1SEL0 &= ~IOT_BOOT_CPU;     // (0)   IOT_BOOT_CPU GPIO operation
    P1SEL1 &= ~IOT_BOOT_CPU;     // (0)   IOT_BOOT_CPU GPIO operation
    P1DIR  |=  IOT_BOOT_CPU;     // (0)   Direction = output
    P1OUT  |=  IOT_BOOT_CPU;     // (0)   Initial Value = High / 1

    P1SEL0 &= ~IOT_RUN_RED;     // (0)   IOT_RUN_CPU GPIO operation
    P1SEL1 &= ~IOT_RUN_RED;     // (0)   IOT_RUN_CPU GPIO operation
    P1DIR  |=  IOT_RUN_RED;     // (1)   Direction = output
    P1OUT  &= ~IOT_RUN_RED;     // (0)   Initial Value = Low / Off

    P1SELC |= SENS1_R;         // (11) ADC input

    P1SEL0 |=  UCA0RXD;         // (1)   UCA0RXD FUNCTION operation
    P1SEL1 &= ~UCA0RXD;         // (0)   UCA0RXD FUNCTION operation
//    P1OUT  &= ~UCA0RXD;         // (0)   Initial Value = Low / Off
//    P1DIR  &= ~UCA0RXD;         // (0)   Direction = input

    P1SEL0 |=  UCA0TXD;         // (1)   UCA0TXD FUNCTION operation
    P1SEL1 &= ~UCA0TXD;         // (0)   UCA0TXD FUNCTION operation
//    P1OUT  &= ~UCA0TXD;         // (0)   Initial Value = Low / Off
//    P1DIR  &= ~UCA0TXD;         // (0)   Direction = input
}

void Init_Port2(void){ // Configure Port 2
//------------------------------------------------------------------------------
    P2OUT = 0x00;           // P2 set Low
    P2DIR = 0x00;           // Set P2 direction to output
    P2SELC = 0X00;          // Set all to gpio


    P2SEL0 &= ~IOT_EN;           // (0)   IOT_RN_CPU GPIO operation
    P2SEL1 &= ~IOT_EN;           // (0)   IOT_RN_CPU GPIO operation
    P2DIR  |=  IOT_EN;           // (0)   Direction = output
    P2OUT  &= ~IOT_EN;           // (0)   Initial Value = Low / Off

    P2SEL0 &= ~IOT_LINK_GRN;     // (0)   IOT_LINK_CPU GPIO operation
    P2SEL1 &= ~IOT_LINK_GRN;     // (0)   IOT_LINK_CPU GPIO operation
    P2DIR  |=  IOT_LINK_GRN;     // (0)   Direction = output
    P2OUT  &= ~IOT_LINK_GRN;     // (0)   Initial Value = Low / Off

//    P2SEL0 &= ~IR_LED;          // (0)   P2_2 GPIO operation
//    P2SEL1 &= ~IR_LED;          // (0)   P2_2 GPIO operation
//    P2DIR  |=  IR_LED;          // (1)   Direction = output
//    P2OUT  &= ~IR_LED;          // (0)   Initial Value = Low / Off

    P2SEL0 &= ~SW2;             // (0)   SW2 Operation
    P2SEL1 &= ~SW2;             // (0)   SW2 Operation
    P2DIR  &= ~SW2;             // (0)   Direction = input
    P2PUPD |= SW2;              // (1)   Configure pullup resistor
    P2REN  |= SW2;              // (1)   Enable pullup resistor
    P2IES  |= SW2;              // (1)   SW2 HI/LO EDGE INTERRUPT
    P2IFG  &= ~SW2;             // (0)   IFG SW2 CLEARED
    P2IE   |= SW2;              // (1)   SW2 INTERRUPT ENABLED

//    P2SEL0 &= ~IOT_RUN_RED;     // (0)   IOT_RUN_CPU GPIO operation
//    P2SEL1 &= ~IOT_RUN_RED;     // (0)   IOT_RUN_CPU GPIO operation
//    P2DIR  |=  IOT_RUN_RED;     // (1)   Direction = output
//    P2OUT  &= ~IOT_RUN_RED;     // (0)   Initial Value = Low / Off
//
//    P2SEL0 &= ~DAC_ENB;         // (0)   DAC_ENB GPIO operation
//    P2SEL1 &= ~DAC_ENB;         // (0)   DAC_ENB GPIO operation
//    P2DIR  |=  DAC_ENB;         // (1)   Direction = output
//    P2OUT  &= ~DAC_ENB;         // (1)   Initial Value = High
//
//    P2SEL0 &= ~LFXOUT;          // (0)   LFXOUT Clock operation
//    P2SEL1 |= LFXOUT;           // (1)   LFXOUT Clock operation
//
//    P2SEL0 &= ~LFXIN;           // (0)   LFXIN Clock operation
//    P2SEL1 |= LFXIN;            // (1)   LFXIN Clock operation
//------------------------------------------------------------------------------
}

void Init_Port3(char SMCLK_SETTING){
//------------------------------------------------------------------------------
    P3OUT = 0x00;           // P3 set Low
    P3DIR = 0x00;           // Set P3 direction to output
    P3SELC = 0X00;          // Set all to gpio


//    P3SEL0 &= ~TEST_PROBE;      // (0)   TEST_PROBE GPIO operation
//    P3SEL1 &= ~TEST_PROBE;      // (0)   TEST_PROBE GPIO operation
//    P3DIR  &= ~TEST_PROBE;      // (0)   Direction = input
//    P3OUT  &= ~TEST_PROBE;      // (0)   Initial Value = Low / Off
//
//    //P3SEL0 &= ~DAC_CTRL_2;      // (0)   DAC_CTRL_2 GPIO operation
//    //P3SEL1 &= ~DAC_CTRL_2;      // (0)   DAC_CTRL_2 GPIO operation
//	P3SELC |=  DAC_CTRL_2;      // (11)  DAC input
//    P3DIR  |=  DAC_CTRL_2;      // (0)   Direction = input
//    P3OUT  |=  DAC_CTRL_2;      // (0)   Initial Value = Low / Off
//
//    P3SEL0 &= ~OA2N;            // (0)   OA2N FUNCTION operation
//    P3SEL1 &= ~OA2N;            // (0)   OA2N FUNCTION operation
//    P3DIR  &= ~OA2N;            // (0)   Direction = input
//    P3OUT  &= ~OA2N;            // (0)   Initial Value = Low / Off
//
//    P3SEL0 &= ~OA2P;            // (0)   OA2P FUNCTION operation
//    P3SEL1 &= ~OA2P;            // (0)   OA2P FUNCTION operation
//    P3DIR  &= ~OA2P;            // (0)   Direction = input
//    P3OUT  &= ~OA2P;            // (0)   Initial Value = Low / Off

    P3SEL0 &= ~SW1;             // (0)   SW1 GPIO operation
    P3SEL1 &= ~SW1;             // (0)   SW1 GPIO operation
    P3DIR  &= ~SW1;             // (0)   Direction = input
    P3PUPD |= SW1;              // (1)   Configure pullup resistor
    P3REN  |= SW1;              // (1)   Enable pullup resistor
    P3IES  |= SW1;              // (1)   SW1 HI/LO EDGE INTERRUPT
    P3IFG  &= ~SW1;             // (0)   IFG SW1 CLEARED
    P3IE   |= SW1;              // (1)   SW1 INTERRUPT ENABLED


//    P3SEL0 &= ~DAC_CTRL_3;       // (0)   DAC_CTRL_3 GPIO operation
//    P3SEL1 &= ~DAC_CTRL_3;       // (0)   DAC_CTRL_3 GPIO operation
//    P3DIR  &= ~DAC_CTRL_3;       // (0)   Direction = input
//    P3OUT  &= ~DAC_CTRL_3;       // (0)   Initial Value = Low / Off
//
//    P3SEL0 &= ~IOT_LINK_GRN;     // (0)   IOT_LINK_CPU GPIO operation
//    P3SEL1 &= ~IOT_LINK_GRN;     // (0)   IOT_LINK_CPU GPIO operation
//    P3DIR  |=  IOT_LINK_GRN;     // (0)   Direction = output
//    P3OUT  &= ~IOT_LINK_GRN;     // (0)   Initial Value = Low / Off
//
//    P3SEL0 &= ~IOT_EN;           // (0)   IOT_RN_CPU GPIO operation
//    P3SEL1 &= ~IOT_EN;           // (0)   IOT_RN_CPU GPIO operation
//    P3DIR  |=  IOT_EN;           // (0)   Direction = output
//    P3OUT  &= ~IOT_EN;           // (0)   Initial Value = Low / Off

}

void Init_Port4(void){ // Configure PORT 4
//------------------------------------------------------------------------------
    P4OUT = 0x00;           // P4 set Low
    P4DIR = 0x00;           // Set P4 direction to output
    P4SELC = 0X00;          // Set all to gpio


//    P4SEL0 &= ~RESET_LCD;       // (0)   RESET_LCD GPIO operation
//    P4SEL1 &= ~RESET_LCD;       // (0)   RESET_LCD GPIO operation
//    P4OUT  &= ~RESET_LCD;       // (0)   Initial Value = Low / Off
//    P4DIR  |= RESET_LCD;        // (1)   Direction = output
//
//    P4SEL0 &= ~SW1;             // (0)   SW1 GPIO operation
//    P4SEL1 &= ~SW1;             // (0)   SW1 GPIO operation
//    P4DIR  &= ~SW1;             // (0)   Direction = input
//    P4PUPD |= SW1;              // (1)   Configure pullup resistor
//    P4REN  |= SW1;              // (1)   Enable pullup resistor
//    P4IES  |= SW1;              // (1)   SW1 HI/LO EDGE INTERRUPT
//    P4IFG  &= ~SW1;             // (0)   IFG SW1 CLEARED
//    P4IE   |= SW1;              // (1)   SW1 INTERRUPT ENABLED

    P4SEL0 |= UCA1TXD;          // (1)   USCI_A1 UART operation
    P4SEL1 &= ~UCA1TXD;         // (0)   USCI_A1 UART operation

    P4SEL0 |= UCA1RXD;          // (1)   USCI_A1 UART operation
    P4SEL1 &= ~UCA1RXD;         // (0)   USCI_A1 UART operation

    P4SEL0 &= ~LED_8;          // (0)   GRN_LED GPIO operation
    P4SEL1 &= ~LED_8;          // (0)   GRN_LED GPIO operation
    P4DIR  |=  LED_8;          // (1)   Direction = output
    P4OUT  &= ~LED_8;          // (0)   Initial Value = Low / Off

    P4SEL0 &= ~LED_9;          // (0)   GRN_LED GPIO operation
    P4SEL1 &= ~LED_9;          // (0)   GRN_LED GPIO operation
    P4DIR  |=  LED_9;          // (1)   Direction = output
    P4OUT  &= ~LED_9;          // (0)   Initial Value = Low / Off

//    P4SEL0 |= UCB1SIMO;         // (1)   UCB1SIMO SPI BUS operation
//    P4SEL1 &= ~UCB1SIMO;        // (0)   UCB1SIMO SPI BUS operation
//
//    P4SEL0 |= UCB1SOMI;         // (1)   UCB1SOMI SPI BUS operation
//    P4SEL1 &= ~UCB1SOMI;        // (0)   UCB1SOMI SPI BUS operation
//------------------------------------------------------------------------------
}




void Init_Port5(void){
//------------------------------------------------------------------------------
    P5OUT = 0x00;         // P5 set Low
    P5DIR = 0x00;         // Set P5 direction to output
    P5SELC = 0X00;          // Set all to gpio


    P5SELC |=  SENS2_L;

    P5SELC |=  SENS2_R;

//    P5SEL0 &= ~V_DAC;           // (0)   V_DAC GPIO operation
//    P5SEL1 &= ~V_DAC;           // (0)   V_DAC GPIO operation
//    P5OUT  &= ~V_DAC;           // (0)   Initial Value = Low / Off
//    P5DIR  &= ~V_DAC;           // (0)   Direction = input
//    P5SELC |=  V_DAC;

//    P5SEL0 &= ~V3_3;            // (0)   V3_3 GPIO operation
//    P5SEL1 &= ~V3_3;            // (0)   V3_3 GPIO operation
//    P5OUT  &= ~V3_3;            // (0)   Initial Value = Low / Off
//    P5DIR  &= ~V3_3;            // (0)   Direction = input
//    P5SELC |=  V3_3;

//    P5SEL0 &= ~IOT_BOOT_CPU;     // (0)   IOT_BOOT_CPU GPIO operation
//    P5SEL1 &= ~IOT_BOOT_CPU;     // (0)   IOT_BOOT_CPU GPIO operation
//    P5DIR  |=  IOT_BOOT_CPU;     // (0)   Direction = output
//    P5OUT  |=  IOT_BOOT_CPU;     // (0)   Initial Value = High / 1
}

void Init_Port6(void){
//------------------------------------------------------------------------------
    P6OUT = 0x00;           // P6 set Low
    P6DIR = 0x00;           // Set P6 direction to output
    P6SELC = 0X00;          // Set all to gpio


    P6SEL0 &= ~LED_2;          // (0)   GRN_LED GPIO operation
    P6SEL1 &= ~LED_2;          // (0)   GRN_LED GPIO operation
    P6DIR  |=  LED_2;          // (1)   Direction = output
    P6OUT  &= ~LED_2;          // (0)   Initial Value = Low / Off

    P6SEL0 &= ~LED_1;          // (0)   GRN_LED GPIO operation
    P6SEL1 &= ~LED_1;          // (0)   GRN_LED GPIO operation
    P6DIR  |=  LED_1;          // (1)   Direction = output
    P6OUT  &= ~LED_1;          // (0)   Initial Value = Low / Off

    P6SEL0 &= ~LED_3;          // (0)   GRN_LED GPIO operation
    P6SEL1 &= ~LED_3;          // (0)   GRN_LED GPIO operation
    P6DIR  |=  LED_3;          // (1)   Direction = output
    P6OUT  &= ~LED_3;          // (0)   Initial Value = Low / Off

    P6SEL0 &= ~LED_4;          // (0)   GRN_LED GPIO operation
    P6SEL1 &= ~LED_4;          // (0)   GRN_LED GPIO operation
    P6DIR  |=  LED_4;          // (1)   Direction = output
    P6OUT  &= ~LED_4;          // (0)   Initial Value = Low / Off

    P6SEL0 &= ~LED_5;          // (0)   GRN_LED GPIO operation
    P6SEL1 &= ~LED_5;          // (0)   GRN_LED GPIO operation
    P6DIR  |=  LED_5;          // (1)   Direction = output
    P6OUT  &= ~LED_5;          // (0)   Initial Value = Low / Off

    P6SEL0 &= ~LED_6;          // (0)   GRN_LED GPIO operation
    P6SEL1 &= ~LED_6;          // (0)   GRN_LED GPIO operation
    P6DIR  |=  LED_6;          // (1)   Direction = output
    P6OUT  &= ~LED_6;          // (0)   Initial Value = Low / Off

    P6SEL0 &= ~LED_7;          // (0)   GRN_LED GPIO operation
    P6SEL1 &= ~LED_7;          // (0)   GRN_LED GPIO operation
    P6DIR  |=  LED_7;          // (1)   Direction = output
    P6OUT  &= ~LED_7;          // (0)   Initial Value = Low / Off
}
