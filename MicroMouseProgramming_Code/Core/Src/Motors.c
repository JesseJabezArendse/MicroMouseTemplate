//********************************************************************
//*                          Micro Mouse                             *
//*                          Motors Library                          *
//*==================================================================*
//* @author:    Jesse Jabez Arendse                                  *
//* @date:      04-04-2026                                           *
//*==================================================================*
//*                                                                  *
//* Description:                                                     *
//* This source file implements functions for initializing and       *
//* refreshing motor control values for the Micro Mouse robot.       *
//* It includes functions for starting PWM and updating motor        *
//* speed and direction.                                             *
//*                                                                  *
//********************************************************************

#include "Motors.h"
#include "main.h"
#include "math.h"
#include <stdlib.h>

int8_t MOTOR_LS = 0;
int8_t MOTOR_RS = 0;


extern TIM_HandleTypeDef htim3;
extern TIM_HandleTypeDef htim4;


void initMotors() {
    // Start PWM for motors
    HAL_TIM_PWM_Start(&htim4, TIM_CHANNEL_1);
    HAL_TIM_PWM_Start(&htim4, TIM_CHANNEL_2);
    HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_3);
    HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_4);
}

void refreshMotors() {
    // Update motor control values
    if (MOTOR_LS >= 0) {
        TIM3->CCR3 = abs(MOTOR_LS) * 1;
        TIM3->CCR4 = 0;
    } else {
        TIM3->CCR3 = 0;
        TIM3->CCR4 = abs(MOTOR_LS) * 1;
    }

    if (MOTOR_RS >= 0) {
        TIM4->CCR1 = abs(MOTOR_RS) * 1;
        TIM4->CCR2 = 0;
    } else {
        TIM4->CCR1 = 0;
        TIM4->CCR2 = abs(MOTOR_RS) * 1;
    }
}