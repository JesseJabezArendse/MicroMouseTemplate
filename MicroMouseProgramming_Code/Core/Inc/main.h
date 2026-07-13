/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2023 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __MAIN_H
#define __MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32l4xx_hal.h"
#include "stm32l4xx_ll_dma.h"
#include "stm32l4xx_ll_usart.h"
#include "stm32l4xx_ll_rcc.h"
#include "stm32l4xx_ll_bus.h"
#include "stm32l4xx_ll_cortex.h"
#include "stm32l4xx_ll_system.h"
#include "stm32l4xx_ll_utils.h"
#include "stm32l4xx_ll_pwr.h"
#include "stm32l4xx_ll_gpio.h"

#include "stm32l4xx_ll_exti.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#define I2C_TIMEOUT 10

/* USER CODE END Includes */

/* Exported types ------------------------------------------------------------*/
/* USER CODE BEGIN ET */

/* USER CODE END ET */

/* Exported constants --------------------------------------------------------*/
/* USER CODE BEGIN EC */

/* USER CODE END EC */

/* Exported macro ------------------------------------------------------------*/
/* USER CODE BEGIN EM */

/* USER CODE END EM */

void HAL_TIM_MspPostInit(TIM_HandleTypeDef *htim);

/* Exported functions prototypes ---------------------------------------------*/
void Error_Handler(void);
void MX_GPIO_Init(void);
void MX_DMA_Init(void);
void MX_I2C1_Init(void);
void MX_I2C2_Init(void);
void MX_SPI2_Init(void);
void MX_ADC1_Init(void);
void MX_TIM1_Init(void);
void MX_TIM3_Init(void);
void MX_TIM4_Init(void);
void MX_TIM5_Init(void);
void MX_TIM7_Init(void);
void MX_USART1_UART_Init(void);
void MX_NVIC_Init(void);

/* USER CODE BEGIN EFP */
void initMicroMouse(void);
void configureTimer(float desired_frequency, TIM_TypeDef* tim);
void sendToSimulink(void);
void recievedFromSimulink(void);
uint8_t I2C_Scan(I2C_HandleTypeDef *hi2c, uint8_t *foundAddresses, uint8_t maxAddresses);
void initUSB(void);
void refreshUSB(void);
void logSelectedVariables(void);
void updateMicroMouse(void);
void restartI2C(I2C_HandleTypeDef *hi2c);
/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/
#define XSHUT5_Pin GPIO_PIN_2
#define XSHUT5_GPIO_Port GPIOE
#define XSHUT3_Pin GPIO_PIN_3
#define XSHUT3_GPIO_Port GPIOE
#define SW1_Pin GPIO_PIN_6
#define SW1_GPIO_Port GPIOE
#define LED0_Pin GPIO_PIN_13
#define LED0_GPIO_Port GPIOC
#define LED1_Pin GPIO_PIN_14
#define LED1_GPIO_Port GPIOC
#define LED2_Pin GPIO_PIN_15
#define LED2_GPIO_Port GPIOC
#define ADC_MOT_RS_Pin GPIO_PIN_3
#define ADC_MOT_RS_GPIO_Port GPIOA
#define ADC_DOWN_RS_Pin GPIO_PIN_4
#define ADC_DOWN_RS_GPIO_Port GPIOC
#define ADC_DOWN_LS_Pin GPIO_PIN_5
#define ADC_DOWN_LS_GPIO_Port GPIOC
#define ADC_MOT_LS_Pin GPIO_PIN_0
#define ADC_MOT_LS_GPIO_Port GPIOB
#define SW2_Pin GPIO_PIN_2
#define SW2_GPIO_Port GPIOB
#define XSHUT1_Pin GPIO_PIN_8
#define XSHUT1_GPIO_Port GPIOE
#define LED_MOT_LS_Pin GPIO_PIN_9
#define LED_MOT_LS_GPIO_Port GPIOE
#define XSHUT4_Pin GPIO_PIN_10
#define XSHUT4_GPIO_Port GPIOE
#define LED_DOWN_LS_Pin GPIO_PIN_11
#define LED_DOWN_LS_GPIO_Port GPIOE
#define LED_MOT_RS_Pin GPIO_PIN_13
#define LED_MOT_RS_GPIO_Port GPIOE
#define LED_DOWN_RS_Pin GPIO_PIN_14
#define LED_DOWN_RS_GPIO_Port GPIOE
#define XSHUT2_Pin GPIO_PIN_15
#define XSHUT2_GPIO_Port GPIOE
#define MOTORR_A_ENC_Pin GPIO_PIN_12
#define MOTORR_A_ENC_GPIO_Port GPIOD
#define MOTORR_B_ENC_Pin GPIO_PIN_13
#define MOTORR_B_ENC_GPIO_Port GPIOD
#define MOTORL_A_ENC_Pin GPIO_PIN_14
#define MOTORL_A_ENC_GPIO_Port GPIOD
#define MOTORL_B_ENC_Pin GPIO_PIN_15
#define MOTORL_B_ENC_GPIO_Port GPIOD
#define MOTORR_A_EN_Pin GPIO_PIN_6
#define MOTORR_A_EN_GPIO_Port GPIOC
#define MOTORR_B_EN_Pin GPIO_PIN_7
#define MOTORR_B_EN_GPIO_Port GPIOC
#define MOTORL_A_EN_Pin GPIO_PIN_8
#define MOTORL_A_EN_GPIO_Port GPIOC
#define MOTORL_B_EN_Pin GPIO_PIN_9
#define MOTORL_B_EN_GPIO_Port GPIOC
#define XSHUT8_Pin GPIO_PIN_8
#define XSHUT8_GPIO_Port GPIOA
#define XSHUT7_Pin GPIO_PIN_3
#define XSHUT7_GPIO_Port GPIOD
#define MOTOR_EN_Pin GPIO_PIN_7
#define MOTOR_EN_GPIO_Port GPIOD
#define CTRL_LEDS_Pin GPIO_PIN_3
#define CTRL_LEDS_GPIO_Port GPIOB
#define IMU_INT_Pin GPIO_PIN_5
#define IMU_INT_GPIO_Port GPIOB
#define FLASH_CS_Pin GPIO_PIN_12
#define FLASH_CS_GPIO_Port GPIOB
#define XSHUT9_Pin GPIO_PIN_0
#define XSHUT9_GPIO_Port GPIOE

/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
