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

Motor_t MOTOR_L = {0, 0};
Motor_t MOTOR_R = {0, 0};

// Legacy scalar aliases — kept so existing code still compiles
int8_t MOTOR_LS = 0;
int8_t MOTOR_RS = 0;


extern TIM_HandleTypeDef htim3;
extern TIM_HandleTypeDef htim4;


void initMotors() {

}

void refreshMotors() {

}