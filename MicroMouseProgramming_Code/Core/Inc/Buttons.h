//********************************************************************
//*                          Micro Mouse                             *
//*                          Button Library                          *
//*==================================================================*
//* @author:    Jesse Jabez Arendse                                  *
//* @date:      09-06-2025                                           *
//*==================================================================*
//*                                                                  *
//* Description:                                                     *
//* This header file provides an interface for managing buttons on   *
//* the Micro Mouse robot. It includes initialization, state         *
//* refreshing, and handling button presses for user interaction.    *
//*                                                                  *
//********************************************************************

#ifndef BUTTONS_H
#define BUTTONS_H

#include "stm32l4xx.h"
#include "main.h"

// GPIO pin defines are provided by main.h

//====================================================================
// GLOBAL CONSTANTS
//====================================================================
#define BUTTON_DEBOUNCE_DELAY_MS 50 // Debounce delay in milliseconds
#define BUTTON_PRESSED_STATE     1  // State indicating button is pressed
#define BUTTON_RELEASED_STATE    0  // State indicating button is released
//====================================================================

// Switch structure: each switch has port, pin, and state
typedef struct {
	GPIO_TypeDef *port;  // GPIO port (e.g., GPIOE)
	uint16_t pin;        // GPIO pin (e.g., GPIO_PIN_6)
	uint8_t state;       // Switch state (0 = released, 1 = pressed)
} SW_t;

// Individual switch control structures
extern SW_t SW1;  // PE6
extern SW_t SW2;  // PB2

//====================================================================
// FUNCTION DECLARATIONS
//====================================================================
void initSW();               // Initialize the buttons and GPIO pins
void refreshSWValues();      // Read button states from GPIO
//====================================================================
#endif

//********************************************************************
// END OF PROGRAM
//********************************************************************