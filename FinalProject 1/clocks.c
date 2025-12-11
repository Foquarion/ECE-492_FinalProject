// ------------------------------------------------------------------------------
//
//  Description: This file contains the Clock Initialization
//
//  Jim Carlson
//  Jan 2021
//  Built with IAR Embedded Workbench Version: V7.12.1
//
//  ***********************************
//  *   CLONED FOR USE IN ECE 306     *
//  *   BY: DALLAS OWENS - 9/9/2025   *
//  ***********************************
//
//  MODIFIED: Dec 2025
//  - Simplified clock configuration without FLL
//  - Uses DCO in free-running mode for maximum compatibility
//------------------------------------------------------------------------------
#include  "functions.h"
#include  "msp430.h"
#include  "macros.h"

// MACROS========================================================================
#define MCLK_FREQ_MHZ           (8) // MCLK = 8MHz
#define CSKEY                   (0xA5) // CS register unlock key

void Init_Clocks(void){
// -----------------------------------------------------------------------------
// Clock Configurations - Simplified (No FLL)
//
// This configuration uses the DCO in free-running mode without FLL.
// Less accurate than FLL-locked, but very reliable for initial bring-up.
//
//   ACLK  = VLO (~10 kHz internal, varies by device)
//   MCLK  = DCO = ~8MHz (free-running, no FLL)
//   SMCLK = DCO = ~8MHz
// -----------------------------------------------------------------------------

    WDTCTL = WDTPW | WDTHOLD;       // Disable watchdog

    // Unlock CS registers for writing
    // (CS registers are password protected, key = 0xA5)
    CSCTL0_H = CSKEY;               // Unlock CS registers with password

    // Configure DCO for ~8MHz (free-running, no FLL)
    // DCORSEL_3 selects the 8MHz range
    CSCTL1 = DCORSEL_3;             // Set DCO to 8MHz range

    // Disable FLL (we're running DCO free)
    __bis_SR_register(SCG0);        // Disable FLL

    // Set FLL reference divider (not used, but set to safe value)
    CSCTL2 = FLLD_0 + 243;          // FLL settings (inactive)
    
    // Set FLL reference to REFO (not used since FLL is disabled)
    CSCTL3 = SELREF__REFOCLK;       // REFO as FLL reference

    // Configure clock sources
    // ACLK  = VLO (internal ~10kHz, no external crystal needed)
    // MCLK  = DCOCLKDIV (DCO divided clock)
    // SMCLK = DCOCLKDIV (DCO divided clock)
    CSCTL4 = SELA__VLOCLK |         // ACLK = VLO (~10kHz)
             SELMS__DCOCLKDIV;      // MCLK and SMCLK = DCO

    // Set clock dividers
    // MCLK  = DCO / 1 = ~8MHz
    // SMCLK = DCO / 1 = ~8MHz
    CSCTL5 = DIVM__1 |              // MCLK divider = 1
             DIVS__1;               // SMCLK divider = 1

    // Lock CS registers
    CSCTL0_H = 0;                   // Lock CS registers

    // Disable the GPIO power-on default high-impedance mode
    PM5CTL0 &= ~LOCKLPM5;
}


// Software_Trim is not used in this simplified configuration
void Software_Trim(void){
    // Empty - not needed when FLL is disabled
}
