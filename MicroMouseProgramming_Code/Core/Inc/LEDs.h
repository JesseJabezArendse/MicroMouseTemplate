//********************************************************************
//*                          Micro Mouse                             *
//*                          LED Library                             *
//*==================================================================*
//* @author:    Jesse Jabez Arendse                                  *
//* @date:      24-10-2023                                           *
//*==================================================================*
//*                                                                  *
//* Description:                                                     *
//* This header file provides an interface for controlling LEDs      *
//* connected to GPIO pins PC13, PC14, and PC15 on the Micro Mouse   *
//* robot. It includes functions for initializing the LEDs and       *
//* refreshing their states based on an array.                       *
//*                                                                  *
//********************************************************************

#ifndef LEDS_H
#define LEDS_H

#include "stm32l4xx.h"
#include "main.h"

// GPIO pin defines
#define LED0_Pin            GPIO_PIN_13
#define LED0_GPIO_Port      GPIOC
#define LED1_Pin            GPIO_PIN_14
#define LED1_GPIO_Port      GPIOC
#define LED2_Pin            GPIO_PIN_15
#define LED2_GPIO_Port      GPIOC
#define CTRL_LEDS_Pin       GPIO_PIN_3
#define CTRL_LEDS_GPIO_Port GPIOB

typedef struct
{
	uint8_t LED0;
	uint8_t LED1;
	uint8_t LED2;
} LED_t;

// LED array corresponding to PC13, PC14, PC15
extern uint8_t LED[3];
extern LED_t   LEDS;

// Function declarations
void initLEDs();
void refreshLEDs();

#endif
