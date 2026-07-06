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

// Individual LED control structures
LED_t LED0 = {LED0_GPIO_Port, LED0_Pin, 0};
LED_t LED1 = {LED1_GPIO_Port, LED1_Pin, 0};
LED_t LED2 = {LED2_GPIO_Port, LED2_Pin, 0};
LED_t IR_LED_LEFT_MOTOR = {LEFT_MOTOR_GPIO_Port, LEFT_MOTOR_Pin, 0};
LED_t IR_LED_RIGHT_MOTOR = {RIGHT_MOTOR_GPIO_Port, RIGHT_MOTOR_Pin, 0};
LED_t IR_LED_DOWN_LEFT = {DOWN_LEFT_GPIO_Port, DOWN_LEFT_Pin, 0};
LED_t IR_LED_DOWN_RIGHT = {DOWN_RIGHT_GPIO_Port, DOWN_RIGHT_Pin, 0};

void initLEDs() {
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    
    // Configure LED0 pin as output
    GPIO_InitStruct.Pin = LED0.pin;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(LED0.port, &GPIO_InitStruct);

    // Configure LED1 pin as output
    GPIO_InitStruct.Pin = LED1.pin;
    HAL_GPIO_Init(LED1.port, &GPIO_InitStruct);

    // Configure LED2 pin as output
    GPIO_InitStruct.Pin = LED2.pin;
    HAL_GPIO_Init(LED2.port, &GPIO_InitStruct);

    // Configure IR_LED_LEFT_MOTOR pin as output
    GPIO_InitStruct.Pin = IR_LED_LEFT_MOTOR.pin;
    HAL_GPIO_Init(IR_LED_LEFT_MOTOR.port, &GPIO_InitStruct);

    // Configure IR_LED_RIGHT_MOTOR pin as output
    GPIO_InitStruct.Pin = IR_LED_RIGHT_MOTOR.pin;
    HAL_GPIO_Init(IR_LED_RIGHT_MOTOR.port, &GPIO_InitStruct);

    // Configure IR_LED_DOWN_LEFT pin as output
    GPIO_InitStruct.Pin = IR_LED_DOWN_LEFT.pin;
    HAL_GPIO_Init(IR_LED_DOWN_LEFT.port, &GPIO_InitStruct);

    // Configure IR_LED_DOWN_RIGHT pin as output
    GPIO_InitStruct.Pin = IR_LED_DOWN_RIGHT.pin;
    HAL_GPIO_Init(IR_LED_DOWN_RIGHT.port, &GPIO_InitStruct);
    
    // Configure CTRL_LEDS pin (PB3) as output
    GPIO_InitStruct.Pin = CTRL_LEDS_Pin;
    HAL_GPIO_Init(CTRL_LEDS_GPIO_Port, &GPIO_InitStruct);
    
    // Set initial LED states: all OFF
    HAL_GPIO_WritePin(LED0_GPIO_Port, LED0_Pin, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(LED1_GPIO_Port, LED1_Pin, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(LED2_GPIO_Port, LED2_Pin, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(LEFT_MOTOR_GPIO_Port, LEFT_MOTOR_Pin, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(RIGHT_MOTOR_GPIO_Port, RIGHT_MOTOR_Pin, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(DOWN_LEFT_GPIO_Port, DOWN_LEFT_Pin, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(DOWN_RIGHT_GPIO_Port, DOWN_RIGHT_Pin, GPIO_PIN_RESET);
    
    // Enable LED control
    HAL_GPIO_WritePin(CTRL_LEDS_GPIO_Port, CTRL_LEDS_Pin, GPIO_PIN_SET);
}

void refreshLEDs() {
    // Write LED struct state to GPIO pins
    HAL_GPIO_WritePin(LED0_GPIO_Port, LED0_Pin, LED0.state ? GPIO_PIN_SET : GPIO_PIN_RESET);
    HAL_GPIO_WritePin(LED1_GPIO_Port, LED1_Pin, LED1.state ? GPIO_PIN_SET : GPIO_PIN_RESET);
    HAL_GPIO_WritePin(LED2_GPIO_Port, LED2_Pin, LED2.state ? GPIO_PIN_SET : GPIO_PIN_RESET);
    HAL_GPIO_WritePin(LEFT_MOTOR_GPIO_Port, LEFT_MOTOR_Pin, IR_LED_LEFT_MOTOR.state ? GPIO_PIN_SET : GPIO_PIN_RESET);
    HAL_GPIO_WritePin(RIGHT_MOTOR_GPIO_Port, RIGHT_MOTOR_Pin, IR_LED_RIGHT_MOTOR.state ? GPIO_PIN_SET : GPIO_PIN_RESET);
    HAL_GPIO_WritePin(DOWN_LEFT_GPIO_Port, DOWN_LEFT_Pin, IR_LED_DOWN_LEFT.state ? GPIO_PIN_SET : GPIO_PIN_RESET);
    HAL_GPIO_WritePin(DOWN_RIGHT_GPIO_Port, DOWN_RIGHT_Pin, IR_LED_DOWN_RIGHT.state ? GPIO_PIN_SET : GPIO_PIN_RESET);
}
