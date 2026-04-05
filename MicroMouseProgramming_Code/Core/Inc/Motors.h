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
#define MOTOR_EN_Pin            GPIO_PIN_7
#define MOTOR_EN_GPIO_Port      GPIOD

// Encoder input pins (TIM4 input capture)
#define MOTORR_A_ENC_Pin        GPIO_PIN_12
#define MOTORR_A_ENC_GPIO_Port  GPIOD
#define MOTORR_B_ENC_Pin        GPIO_PIN_13
#define MOTORR_B_ENC_GPIO_Port  GPIOD
#define MOTORL_A_ENC_Pin        GPIO_PIN_14
#define MOTORL_A_ENC_GPIO_Port  GPIOD
#define MOTORL_B_ENC_Pin        GPIO_PIN_15
#define MOTORL_B_ENC_GPIO_Port  GPIOD

typedef struct
{
	int16_t	magnitude;	// Speed: negative = reverse, positive = forward (-100 to +100)
	int16_t	encoderRate;	// Encoder tick rate (ticks/s)
} Motor_t;


// Function declarations
void initMotors();
void refreshMotors();

extern Motor_t MOTOR_L;
extern Motor_t MOTOR_R; 

#endif
