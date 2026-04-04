//********************************************************************
//*                          Micro Mouse                             *
//*                          Motors Library                          *
//*==================================================================*
//* @author:    Jesse Jabez Arendse                                  *
//* @date:      24-10-2023                                           *
//*==================================================================*
//*                                                                  *
//* Description:                                                     *
//* This header file provides an interface for initializing and      *
//* refreshing motor control values for the Micro Mouse robot.       *
//* It includes functions for starting PWM and updating motor        *
//* speed and direction.                                             *
//*                                                                  *
//********************************************************************

#ifndef MOTORS_H
#define MOTORS_H

#include "stm32l4xx.h"
#include "main.h"

// GPIO pin defines
#define LED_MOT_LS_Pin          GPIO_PIN_9
#define LED_MOT_LS_GPIO_Port    GPIOE
#define LED_DOWN_LS_Pin         GPIO_PIN_11
#define LED_DOWN_LS_GPIO_Port   GPIOE
#define LED_MOT_RS_Pin          GPIO_PIN_13
#define LED_MOT_RS_GPIO_Port    GPIOE
#define LED_DOWN_RS_Pin         GPIO_PIN_14
#define LED_DOWN_RS_GPIO_Port   GPIOE
#define MOT_RIGHT_FWD_Pin       GPIO_PIN_12
#define MOT_RIGHT_FWD_GPIO_Port GPIOD
#define MOT_RIGHT_BWD_Pin       GPIO_PIN_13
#define MOT_RIGHT_BWD_GPIO_Port GPIOD
#define MOT_LEFT_FWD_Pin        GPIO_PIN_8
#define MOT_LEFT_FWD_GPIO_Port  GPIOC
#define MOT_LEFT_BWD_Pin        GPIO_PIN_9
#define MOT_LEFT_BWD_GPIO_Port  GPIOC
#define MOTOR_EN_Pin            GPIO_PIN_7
#define MOTOR_EN_GPIO_Port      GPIOD

typedef struct
{
	int16_t	magnitude;	// Speed: negative = reverse, positive = forward (-100 to +100)
	int16_t	encoderRate;	// Encoder tick rate (ticks/s)
} Motor_t;

// Motor instances
extern Motor_t MOTOR_L;
extern Motor_t MOTOR_R;

// Legacy scalar aliases (kept for compatibility)
extern int8_t MOTOR_LS;
extern int8_t MOTOR_RS;

// Function declarations
void initMotors();
void refreshMotors();

#endif
