//********************************************************************
//*                          Micro Mouse                             *
//*                          LED Library                             *
//*==================================================================*
//* @author:    Jesse Jabez Arendse                                  *
//* @date:      24-10-2023                                           *
//*==================================================================*
//*                                                                  *
//* Description:                                                     *
//* This source file implements functions for controlling LEDs       *
//* connected to GPIO pins PC13, PC14, and PC15 on the Micro Mouse   *
//* robot. It includes functions for refreshing the LED states       *
//* based on an array.                                               *
//*                                                                  *
//********************************************************************

#include "LEDs.h"

uint8_t LED[3] = {0, 0, 0}; // Initialize LED array
LED_t   LEDS   = {0, 0, 0};

void initLEDs(){
    HAL_GPIO_WritePin(CTRL_LEDS_GPIO_Port, CTRL_LEDS_Pin , GPIO_PIN_SET);
    HAL_GPIO_WritePin(LED0_GPIO_Port, LED0_Pin , GPIO_PIN_RESET);
    HAL_GPIO_WritePin(LED1_GPIO_Port, LED1_Pin , GPIO_PIN_RESET);
    HAL_GPIO_WritePin(LED2_GPIO_Port, LED2_Pin , GPIO_PIN_RESET);
}

void refreshLEDs() {
    // Sync struct into legacy array so either interface works
    LED[0] = LEDS.LED0;
    LED[1] = LEDS.LED1;
    LED[2] = LEDS.LED2;
    // Update the state of LEDs based on the LED array
    HAL_GPIO_WritePin(LED0_GPIO_Port, LED0_Pin , LED[0] ? GPIO_PIN_SET : GPIO_PIN_RESET);
    HAL_GPIO_WritePin(LED1_GPIO_Port, LED1_Pin , LED[1] ? GPIO_PIN_SET : GPIO_PIN_RESET);
    HAL_GPIO_WritePin(LED2_GPIO_Port, LED2_Pin , LED[2] ? GPIO_PIN_SET : GPIO_PIN_RESET);
}
