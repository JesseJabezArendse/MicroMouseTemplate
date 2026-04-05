//********************************************************************
//*                          Micro Mouse                             *
//*                          Button Library                          *
//*==================================================================*
//* @author:    Jesse Jabez Arendse                                  *
//* @date:      09-06-2025                                           *
//*==================================================================*

#include "main.h"
#include "Buttons.h"

// Individual switch control structures
SW_t SW1;
SW_t SW2;

void initSW() {
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    SW1.port = SW1_GPIO_Port;
    SW1.pin = SW1_Pin;
    SW1.state = 0;

    SW2.port = SW2_GPIO_Port;
    SW2.pin = SW2_Pin;
    SW2.state = 0;
    
    // Configure SW1 (PE6) as input
    GPIO_InitStruct.Pin = SW1.pin;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_PULLUP;  // Active-low buttons
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(SW1.port, &GPIO_InitStruct);
    
    // Configure SW2 (PB2) as input
    GPIO_InitStruct.Pin = SW2.pin;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_PULLUP;  // Active-low buttons
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(SW2.port, &GPIO_InitStruct);
}

// Read button states from GPIO
void refreshSWValues() {
    // Read the current state of the GPIO pins
    // Logic: pin reads LOW (reset) when button is pressed
    SW1.state = HAL_GPIO_ReadPin(SW1.port, SW1.pin) == GPIO_PIN_RESET ? BUTTON_PRESSED_STATE : BUTTON_RELEASED_STATE;
    SW2.state = HAL_GPIO_ReadPin(SW2.port, SW2.pin) == GPIO_PIN_RESET ? BUTTON_PRESSED_STATE : BUTTON_RELEASED_STATE;
}

//********************************************************************
// END OF PROGRAM
//********************************************************************