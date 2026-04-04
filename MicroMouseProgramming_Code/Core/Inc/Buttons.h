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

// GPIO pin defines
#define SW1_Pin          GPIO_PIN_6
#define SW1_GPIO_Port    GPIOE
#define SW2_Pin          GPIO_PIN_2
#define SW2_GPIO_Port    GPIOB

//====================================================================
// GLOBAL CONSTANTS
//====================================================================
#define BUTTON_DEBOUNCE_DELAY_MS 50 // Debounce delay in milliseconds
#define BUTTON_PRESSED_STATE     1  // State indicating button is pressed
#define BUTTON_RELEASED_STATE    0  // State indicating button is released
//====================================================================

typedef struct
{
	uint8_t SW0;
	uint8_t SW1;
} SW_t;

extern SW_t SWS;

//====================================================================
// FUNCTION DECLARATIONS
//====================================================================
void initSW();               // Initialize the buttons
void refreshSWValues();      // Refresh button states
//====================================================================

#endif

//********************************************************************
// END OF PROGRAM
//********************************************************************