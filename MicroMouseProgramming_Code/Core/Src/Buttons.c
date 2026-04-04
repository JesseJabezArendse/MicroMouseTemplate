//********************************************************************
//*                          Micro Mouse                             *
//*                          Button Library                          *
//*==================================================================*
//* @author:    Jesse Jabez Arendse                                  *
//* @date:      09-06-2025                                           *
//*==================================================================*

#include "main.h"
#include "Buttons.h"

// Define the button states array
uint8_t SW[2]; // Array to store button states: SW[0] for PE6, SW[1] for PB2
SW_t    SWS = {0, 0};

void initSW(){}

// Refresh button states
void refreshSWValues() {
    // Read the current state of PE6 and PB2
    SW[0] = HAL_GPIO_ReadPin(GPIOE, GPIO_PIN_6) == GPIO_PIN_RESET ? BUTTON_PRESSED_STATE : BUTTON_RELEASED_STATE;
    SW[1] = HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_2) == GPIO_PIN_RESET ? BUTTON_PRESSED_STATE : BUTTON_RELEASED_STATE;
    SWS.SW0 = SW[0];
    SWS.SW1 = SW[1];
}

//********************************************************************
// END OF PROGRAM
//********************************************************************