//********************************************************************
//*                          Micro Mouse                             *
//*                          ADC Library                             *
//*==================================================================*
//* @author:    Jesse Jabez Arendse                                  *
//* @date:      24-10-2023                                           *
//*==================================================================*
//*                                                                  *
//* Description:                                                     *
//* This header file provides an interface for initializing and      *
//* refreshing ADC values for various sensors on the Micro Mouse     *
//* robot. It includes functions for starting DMA and updating       *
//* sensor values.                                                   *
//*                                                                  *
//********************************************************************

#ifndef ADCs_H
#define ADCs_H

#include "stm32l4xx.h"
#include "main.h"

// GPIO pin defines
#define ADC_MOT_RS_Pin        GPIO_PIN_3
#define ADC_MOT_RS_GPIO_Port  GPIOA
#define ADC_DOWN_RS_Pin       GPIO_PIN_4
#define ADC_DOWN_RS_GPIO_Port GPIOC
#define ADC_DOWN_LS_Pin       GPIO_PIN_5
#define ADC_DOWN_LS_GPIO_Port GPIOC
#define ADC_MOT_LS_Pin        GPIO_PIN_0
#define ADC_MOT_LS_GPIO_Port  GPIOB

// ADC variables
extern uint16_t VBAT;
extern uint16_t PHOTO_DOWN_LS;
extern uint16_t PHOTO_DOWN_RS;
extern uint16_t PHOTO_MOT_LS;
extern uint16_t PHOTO_MOT_RS;

// Function declarations
void initADCs();
void refreshADCs();

#endif
