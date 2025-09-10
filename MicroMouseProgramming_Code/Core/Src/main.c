/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
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
/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <math.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include "IMU.h"
#include "VL53L0X.h"
#include "SSD1306.h"
#include "Motors.h"
#include "ADCs.h"
#include "LEDs.h"
#include "Buttons.h"
#include "INA219.h"
#include "preformatted_flash.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
ADC_HandleTypeDef hadc1;
DMA_HandleTypeDef hdma_adc1;

I2C_HandleTypeDef hi2c1;
I2C_HandleTypeDef hi2c2;

TIM_HandleTypeDef htim1;
TIM_HandleTypeDef htim3;
TIM_HandleTypeDef htim4;
TIM_HandleTypeDef htim5;
TIM_HandleTypeDef htim7;

UART_HandleTypeDef huart1;
DMA_HandleTypeDef hdma_usart1_rx;
DMA_HandleTypeDef hdma_usart1_tx;

/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_NVIC_Init(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
int8_t expectedHeader[3] = {'J', '_', 'A'};
int8_t expectedTerminator[3] = {'A', '_', 'J'};

int32_t counter = 0;

// from this file
float SW1;
float SW2;

int8_t bigBuffer[3+5+(18*5)+1+3]; // 5 bytes for mouse control + headers and terminators
uint8_t headerBuffer[3];  // Buffer to receive header bytes
uint8_t header[3];
uint8_t terminator[3];
int8_t buffer[5];
uint8_t headerIndex = 0;  // Index for tracking header reception progress
bool expecting_packet = false;  // Flag to track if we're expecting a full packet

bool LED0 = 0;
bool LED1 = 0;
bool LED2 = 0;

// ADCs
extern uint16_t ADCs[5];

extern int8_t MOTOR_LS;
extern int8_t MOTOR_RS;

// from other files
extern float IMU_Accel[3];
extern float IMU_Gyro[3];
extern float IMU_Temp;

extern VL53L0_t TOF_left_result;
extern VL53L0_t TOF_centre_result;
extern VL53L0_t TOF_right_result;

extern char oled_string1[18];
extern char oled_string2[18];
extern char oled_string3[18];
extern char oled_string4[18];
extern char oled_string5[18];

extern uint8_t LED[3];
extern uint8_t SW[2];

extern uint8_t batteryLife;
extern int16_t Vbattery, Vshunt, Current, config, Power;
extern float miliwattAVG,miliWattTime,totalPowerUsed;
extern uint8_t STATE;
// functions

void configureTimer(float desired_frequency, TIM_TypeDef* tim) {
    // Assuming the clock frequency driving the timer is 80 MHz
    float clock_frequency = SystemCoreClock; // 80 MHz

    // Calculate the required total timer period in timer clock cycles
    float timer_period = clock_frequency / desired_frequency;

    // Choose a suitable prescaler (PSC) to fit the period within ARR's range
    uint32_t prescaler = (uint32_t)(timer_period / 65536.0f); // PSC ensures ARR <= 65535
    if (prescaler > 65535) {
        prescaler = 65535; // Cap PSC if it exceeds 16-bit value
    }

    // Calculate the ARR based on the chosen PSC
    uint64_t arr = (uint64_t)(timer_period / (prescaler + 1));



    // Update the timer registers
    tim->PSC = prescaler;   // Set the prescaler
    tim->ARR = arr;         // Set the auto-reload register

    // Reload the timer settings to apply the changes immediately
    tim->EGR = TIM_EGR_UG;  // Generate an update event to reload PSC and ARR
}

void sendToSimulink(){
    HAL_UART_Transmit(&huart1, (uint8_t *) "J_A"           ,3 , HAL_MAX_DELAY);

    // Switches
    HAL_UART_Transmit(&huart1,(float_t*)  &SW1     ,4 , HAL_MAX_DELAY);
    HAL_UART_Transmit(&huart1,(float_t *) &SW2     ,4 , HAL_MAX_DELAY);

  // IMU
    HAL_UART_Transmit(&huart1, &IMU_Accel[0]     ,4 , HAL_MAX_DELAY);
    HAL_UART_Transmit(&huart1, &IMU_Accel[1]     ,4 , HAL_MAX_DELAY);
    HAL_UART_Transmit(&huart1, &IMU_Accel[2]     ,4 , HAL_MAX_DELAY);
    HAL_UART_Transmit(&huart1, &IMU_Gyro[0]     ,4 , HAL_MAX_DELAY);
    HAL_UART_Transmit(&huart1, &IMU_Gyro[1]     ,4 , HAL_MAX_DELAY);
    HAL_UART_Transmit(&huart1, &IMU_Gyro[2]     ,4 , HAL_MAX_DELAY);
    HAL_UART_Transmit(&huart1, &IMU_Temp         ,4 , HAL_MAX_DELAY);


  // TOF
    HAL_UART_Transmit(&huart1, &(TOF_left_result.Distance)  , 4 , HAL_MAX_DELAY);
    HAL_UART_Transmit(&huart1, &(TOF_left_result.Ambient )  , 4 , HAL_MAX_DELAY);
    HAL_UART_Transmit(&huart1, &(TOF_left_result.Signal  )  , 4 , HAL_MAX_DELAY);
    HAL_UART_Transmit(&huart1, &(TOF_left_result.Status  )  , 4 , HAL_MAX_DELAY);
    HAL_UART_Transmit(&huart1,  &(TOF_centre_result.Distance)  , 4 , HAL_MAX_DELAY);
    HAL_UART_Transmit(&huart1,  &(TOF_centre_result.Ambient )  , 4 , HAL_MAX_DELAY);
    HAL_UART_Transmit(&huart1,  &(TOF_centre_result.Signal  )  , 4 , HAL_MAX_DELAY);
    HAL_UART_Transmit(&huart1,  &(TOF_centre_result.Status  )  , 4 , HAL_MAX_DELAY);
    HAL_UART_Transmit(&huart1,  &(TOF_right_result.Distance)  , 4 , HAL_MAX_DELAY);
    HAL_UART_Transmit(&huart1,  &(TOF_right_result.Ambient )  , 4 , HAL_MAX_DELAY);
    HAL_UART_Transmit(&huart1,  &(TOF_right_result.Signal  )  , 4 , HAL_MAX_DELAY);
    HAL_UART_Transmit(&huart1,  &(TOF_right_result.Status  )  , 4 , HAL_MAX_DELAY);

    // VBAT, MOT_RS , DOWN_RS , DOWN_LS , MOT_LS
    HAL_UART_Transmit(&huart1,  &(ADCs[0]  )  , 2 , HAL_MAX_DELAY);
    HAL_UART_Transmit(&huart1,  &(ADCs[0]  )  , 2 , HAL_MAX_DELAY);

    HAL_UART_Transmit(&huart1,  &(ADCs[1]  )  , 2 , HAL_MAX_DELAY);
    HAL_UART_Transmit(&huart1,  &(ADCs[1]  )  , 2 , HAL_MAX_DELAY);

    HAL_UART_Transmit(&huart1,  &(ADCs[2]  )  , 2 , HAL_MAX_DELAY);
    HAL_UART_Transmit(&huart1,  &(ADCs[2]  )  , 2 , HAL_MAX_DELAY);

    HAL_UART_Transmit(&huart1,  &(ADCs[3]  )  , 2 , HAL_MAX_DELAY);
    HAL_UART_Transmit(&huart1,  &(ADCs[3]  )  , 2 , HAL_MAX_DELAY);

    HAL_UART_Transmit(&huart1,  &(ADCs[4]  )  , 2 , HAL_MAX_DELAY);
    HAL_UART_Transmit(&huart1,  &(ADCs[4]  )  , 2 , HAL_MAX_DELAY);

    // INA219
    HAL_UART_Transmit(&huart1, (uint8_t *) &Vbattery     ,2 , HAL_MAX_DELAY);
    HAL_UART_Transmit(&huart1, (uint8_t *) &Vshunt       ,2 , HAL_MAX_DELAY);
    HAL_UART_Transmit(&huart1, (uint8_t *) &Current      ,2 , HAL_MAX_DELAY);
    HAL_UART_Transmit(&huart1, (uint8_t *) &Power        ,2 , HAL_MAX_DELAY); 
    HAL_UART_Transmit(&huart1, (uint8_t *) &batteryLife  ,1 , HAL_MAX_DELAY);
    HAL_UART_Transmit(&huart1, (uint8_t *) &batteryLife  ,1 , HAL_MAX_DELAY);
    HAL_UART_Transmit(&huart1, (uint8_t *) &batteryLife  ,1 , HAL_MAX_DELAY);
    HAL_UART_Transmit(&huart1, (uint8_t *) &batteryLife  ,1 , HAL_MAX_DELAY);



    HAL_UART_Transmit(&huart1, (uint32_t *) &counter         ,4 , HAL_MAX_DELAY);
    // HAL_UART_Transmit(&huart1, (uint8_t *) "A_J"      ,3 , HAL_MAX_DELAY);
}

void recievedFromSimulink(){
    // Data has been received via DMA into bigBuffer
    // We only reach here if the header was already verified
    expecting_packet = false; // Reset flag as packet has been received
    
    // Extract and verify terminator bytes
    memcpy(terminator, bigBuffer + (sizeof(bigBuffer) - 3), 3);
    
    if (terminator[0] == expectedTerminator[0] & terminator[1] == expectedTerminator[1] && terminator[2] == expectedTerminator[2]){
        LED[0] = bigBuffer[3];
        LED[1] = bigBuffer[4];
        LED[2] = bigBuffer[5];

        MOTOR_LS = bigBuffer[6];
        MOTOR_RS = bigBuffer[7];

        for (int i = 0; i < 18; i++) {
            oled_string1[i] = bigBuffer[8 + i];
            oled_string2[i] = bigBuffer[26 + i];
            oled_string3[i] = bigBuffer[44 + i];
            oled_string4[i] = bigBuffer[62 + i];
            oled_string5[i] = bigBuffer[80 + i];
        }

        STATE = bigBuffer[98];
    }
    
    // Restart header reception for next packet
    headerIndex = 0;
    HAL_UART_Receive_IT(&huart1, &headerBuffer[headerIndex], 1);
}

uint8_t I2C_Scan(I2C_HandleTypeDef *hi2c, uint8_t *foundAddresses, uint8_t maxAddresses) {
    uint8_t found = 0;
    for (uint8_t addr = 1; addr < 128; addr++) {
        if (HAL_I2C_IsDeviceReady(hi2c, addr << 1, 1, 10) == HAL_OK) {
            if (found < maxAddresses) {
                foundAddresses[found++] = addr;
            }
        }
    }
    return found;
}

void restartI2C(){
  #ifdef I2C_SAFE_RESTART
    TIM3->CCR4 = 0;
    TIM3->CCR3 = 0;
    TIM4->CCR2 = 0;
    TIM4->CCR1 = 0;
  #endif
  HAL_I2C_DeInit(&hi2c1);
  HAL_I2C_Init(&hi2c1);
  HAL_I2C_DeInit(&hi2c2);
  HAL_I2C_Init(&hi2c2);
  // Attempt to clear I2C bus error (BERR)}
}
// Logging



extern uint8_t USB_storage_buffer[2][USB_BUFFER_SIZE];
extern uint16_t usb_storage_buffer_index[2];
extern uint8_t active_usb_buffer;
extern uint8_t readyToLog;
extern uint32_t log_flash_write_addr;
uint8_t log_time_counter = 0;
 
typedef struct __attribute__((packed)) {
    uint8_t state;
    uint8_t LEDs[3];
    int8_t Motor_Left;
    int8_t Motor_Right;
    uint16_t Distance_Left;
    uint16_t Distance_Centre;
    uint16_t Distance_Right;
    uint16_t IMU_Accel_X;
    uint16_t IMU_Accel_Y;
    uint16_t IMU_Accel_Z;
    uint16_t IMU_Gyro_X;
    uint16_t IMU_Gyro_Y;
    uint16_t IMU_Gyro_Z;
} MicroMouseLog_t;

void initLogs() {
    // Configure TIM7 for 30Hz
    configureTimer(25, TIM7);
    HAL_TIM_Base_Start_IT(&htim7);
    readyToLog = false;
    #ifndef COMPILED_BY_SIMULINK 
    HAL_DBGMCU_EnableDBGSleepMode();
    HAL_DBGMCU_EnableDBGStandbyMode();
    HAL_DBGMCU_EnableDBGStopMode();
    #endif
}

bool first_buffer = true;
bool logging_enabled = false;
void refreshLoggedData() {
      #ifdef COMPILED_BY_SIMULINK
      log_time_counter++;
      if (log_time_counter >= 100/25) { // 1ms/Hz
        readyToLog = true;
        log_time_counter = 0;
      } 
      #endif

    if (!readyToLog) return;
    readyToLog = false;
    // Enable logging if any button is pressed (active low)
    if (!logging_enabled && (SW[0] != SW[1])) {
        logging_enabled = true;
        HAL_GPIO_WritePin(MOTOR_EN_GPIO_Port , MOTOR_EN_Pin , GPIO_PIN_SET);
    }
    if (!logging_enabled) return;

    if (first_buffer) {
        // Place UID only at the start of the very first buffer
        uint8_t *uid_ptr = (uint8_t*)0x1FFF7590;
        memcpy(USB_storage_buffer[active_usb_buffer], uid_ptr, 12);
        usb_storage_buffer_index[active_usb_buffer] = 12;
        first_buffer = false;

        FLASH_EraseInitTypeDef EraseInitStruct;
 
        uint32_t PAGEError;
        HAL_FLASH_Unlock();
        EraseInitStruct.TypeErase = FLASH_TYPEERASE_MASSERASE;
        EraseInitStruct.Banks = FLASH_BANK_2;
        if (HAL_FLASHEx_Erase(&EraseInitStruct, &PAGEError) != HAL_OK){
          return HAL_FLASH_GetError();
        }
        HAL_FLASH_Lock();
    }

    MicroMouseLog_t log;
    log.state = STATE;
    log.LEDs[0] = LED[0];
    log.LEDs[1] = LED[1];
    log.LEDs[2] = LED[2];
    log.Distance_Left = (uint16_t)(TOF_left_result.Distance > 4095 ? 4095 : TOF_left_result.Distance);
    log.Distance_Centre = (uint16_t)(TOF_centre_result.Distance > 4095 ? 4095 : TOF_centre_result.Distance);
    log.Distance_Right = (uint16_t)(TOF_right_result.Distance > 4095 ? 4095 : TOF_right_result.Distance);
    log.Motor_Left = MOTOR_LS;
    log.Motor_Right = MOTOR_RS;
    // Add IMU accel x, accel y, and gyro z
    log.IMU_Accel_X = (uint16_t)(IMU_Accel[0] * 1000.0f);
    log.IMU_Accel_Y = (uint16_t)(IMU_Accel[1] * 1000.0f);
    log.IMU_Accel_Z = (uint16_t)(IMU_Accel[2] * 1000.0f);
    log.IMU_Gyro_X = (uint16_t)(IMU_Gyro[0] * 1000.0f);
    log.IMU_Gyro_Y = (uint16_t)(IMU_Gyro[1] * 1000.0f);
    log.IMU_Gyro_Z = (uint16_t)(IMU_Gyro[2] * 1000.0f);
    // Check if flash at 0x807FFFF is not 0xFF, stop logging and indicate full
    if (*((uint8_t*)0x807FFFF) != 0xFF) {
        logging_enabled = false;
        LED[0] = 1;
        LED[1] = 1;
        LED[2] = 1;
        MOTOR_LS = 0;
        MOTOR_RS = 0;
        snprintf(oled_string2, sizeof(oled_string2), "MicroMouseLog Full");
        refreshScreen();
        return;
    }
    // Calculate CRC as bitwise AND of UID, motors, and state
    // Use last 3 bytes of UID and bitwise AND with Motor_Left, Motor_Right, and state, each shifted to fill 24 bits
    uint8_t *uid_ptr = (uint8_t*)0x1FFF7590;
    uint32_t uid24 = (uid_ptr[9] << 16) | (uid_ptr[10] << 8) | uid_ptr[11];
    uint32_t log24 = ((uint8_t)log.Motor_Left << 16) | ((uint8_t)log.Motor_Right << 8) | ((uint8_t)log.state);
    // log.crc = uid24 & log24;
    memcpy(&USB_storage_buffer[active_usb_buffer][usb_storage_buffer_index[active_usb_buffer]], &log, sizeof(MicroMouseLog_t));
    usb_storage_buffer_index[active_usb_buffer] += sizeof(log);

    if (usb_storage_buffer_index[active_usb_buffer] + sizeof(log) > USB_BUFFER_SIZE) {
        // Offload current buffer to flash
        Flash_Write_Data(log_flash_write_addr, USB_storage_buffer[active_usb_buffer], USB_BUFFER_SIZE);
        log_flash_write_addr += LOG_FLASH_PAGE_SIZE;
        // Switch buffer
        active_usb_buffer ^= 1;
        usb_storage_buffer_index[active_usb_buffer] = 0;
        first_buffer = false;
    }
    readyToLog = false;
}

#ifndef COMPILED_BY_SIMULINK

void initMicroMouse(){
  TIM3->CCR4 = 0;
  TIM3->CCR3 = 0;
  TIM4->CCR2 = 0;
  TIM4->CCR1 = 0;

  initTOFs(1);

  // Scan both I2C buses for devices
  uint8_t found1[1];
  uint8_t found2[5];
  uint8_t num1 = I2C_Scan(&hi2c1, found1, 1);
  uint8_t num2 = I2C_Scan(&hi2c2, found2, 5);

  initScreen();
  initINA219();
  initIMU();
  initADCs();
  initMotors();
  initLEDs();
  initSW();
  initLogs();
}

bool simulink_talking = false;

void updateMicroMouse(){
  // Motor Control
  // TIM4->CCR1 = 0;
  // TIM4->CCR2 = 0;
  // TIM3->CCR3 = 0;
  // TIM3->CCR4 = 0;

  // update screen
  refreshADCs();
  refreshScreen();

  // Show sensor data on screen as integers, each digit explicit
  int left_mm = (int)(TOF_left_result.Distance);
  int centre_mm = (int)(TOF_centre_result.Distance);
  int right_mm = (int)(TOF_right_result.Distance);
  int accel_x = (int)(IMU_Accel[0] * 1000); // scale to show 2 decimals as int
  int accel_y = (int)(IMU_Accel[1] * 1000);
  int accel_z = (int)(IMU_Accel[2] * 1000);
  int gyro_x = (int)(IMU_Gyro[0] * 1000);
  int gyro_y = (int)(IMU_Gyro[1] * 1000);
  int gyro_z = (int)(IMU_Gyro[2] * 1000);
  int vbatt_mv = (int)Vbattery;
  int current_ma = (int)Current;
  int batt_pct = (int)batteryLife;


  refreshLEDs();
  refreshSWValues();
  refreshTOFValues();
  refreshIMUValues();
  refreshINA219Values();
  refreshMotors();
  refreshLoggedData();
}

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart){
  if(huart->Instance == USART1){
    simulink_talking = true;
    recievedFromSimulink();
  }
}


void main(void)
{
  // Initialize the HAL Library; it must be the first function to be executed
  HAL_Init();

  // Configure the system clock
  SystemClock_Config();

  // Initialize all configured peripherals
  MX_DMA_Init();
  MX_GPIO_Init();
  MX_ADC1_Init();
  MX_I2C1_Init();
  MX_I2C2_Init();
  MX_TIM1_Init();
  MX_TIM3_Init();
  MX_TIM4_Init();
  MX_TIM5_Init();
  MX_TIM7_Init();
  MX_USART1_UART_Init();
  MX_NVIC_Init();


  initMicroMouse();
  

  // Configure timers for desired frequencies
  configureTimer(100, TIM5); // Example: configure TIM1 for a frequency of 1000 Hz

  HAL_TIM_Base_Start_IT(&htim5);

  HAL_UART_Receive_DMA(&huart1,(uint8_t *) &bigBuffer, sizeof(bigBuffer));

  HAL_Delay(5000);

  while (1)
  {

    // Process any pending flash writes from USB storage
    #ifdef USE_FLASH

    #endif
    
    // Main loop code here
    updateMicroMouse();
    // sendToSimulink();
    // HAL_Delay(100);
    // counter++;
  }
}
#endif /* COMPILED_BY_SIMULINK */
/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Configure the main internal regulator output voltage
  */
  if (HAL_PWREx_ControlVoltageScaling(PWR_REGULATOR_VOLTAGE_SCALE1) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
  RCC_OscInitStruct.PLL.PLLM = 1;
  RCC_OscInitStruct.PLL.PLLN = 10;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV7;
  RCC_OscInitStruct.PLL.PLLQ = RCC_PLLQ_DIV4;
  RCC_OscInitStruct.PLL.PLLR = RCC_PLLR_DIV2;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_4) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief NVIC Configuration.
  * @retval None
  */
static void MX_NVIC_Init(void)
{
  /* FLASH_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(FLASH_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(FLASH_IRQn);
  /* RCC_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(RCC_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(RCC_IRQn);
  /* DMA1_Channel1_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(DMA1_Channel1_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(DMA1_Channel1_IRQn);
  /* ADC1_2_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(ADC1_2_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(ADC1_2_IRQn);
  /* TIM4_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(TIM4_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(TIM4_IRQn);
  /* TIM5_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(TIM5_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(TIM5_IRQn);
  /* DMA2_Channel6_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(DMA2_Channel6_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(DMA2_Channel6_IRQn);
  /* DMA2_Channel7_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(DMA2_Channel7_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(DMA2_Channel7_IRQn);
  /* TIM7_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(TIM7_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(TIM7_IRQn);
}

/**
  * @brief ADC1 Initialization Function
  * @param None
  * @retval None
  */
void MX_ADC1_Init(void)
{

  /* USER CODE BEGIN ADC1_Init 0 */

  /* USER CODE END ADC1_Init 0 */

  ADC_MultiModeTypeDef multimode = {0};
  ADC_ChannelConfTypeDef sConfig = {0};

  /* USER CODE BEGIN ADC1_Init 1 */

  /* USER CODE END ADC1_Init 1 */

  /** Common config
  */
  hadc1.Instance = ADC1;
  hadc1.Init.ClockPrescaler = ADC_CLOCK_ASYNC_DIV1;
  hadc1.Init.Resolution = ADC_RESOLUTION_12B;
  hadc1.Init.DataAlign = ADC_DATAALIGN_RIGHT;
  hadc1.Init.ScanConvMode = ADC_SCAN_ENABLE;
  hadc1.Init.EOCSelection = ADC_EOC_SEQ_CONV;
  hadc1.Init.LowPowerAutoWait = DISABLE;
  hadc1.Init.ContinuousConvMode = ENABLE;
  hadc1.Init.NbrOfConversion = 5;
  hadc1.Init.DiscontinuousConvMode = DISABLE;
  hadc1.Init.ExternalTrigConv = ADC_SOFTWARE_START;
  hadc1.Init.ExternalTrigConvEdge = ADC_EXTERNALTRIGCONVEDGE_NONE;
  hadc1.Init.DMAContinuousRequests = ENABLE;
  hadc1.Init.Overrun = ADC_OVR_DATA_PRESERVED;
  hadc1.Init.OversamplingMode = ENABLE;
  hadc1.Init.Oversampling.Ratio = ADC_OVERSAMPLING_RATIO_256;
  hadc1.Init.Oversampling.RightBitShift = ADC_RIGHTBITSHIFT_4;
  hadc1.Init.Oversampling.TriggeredMode = ADC_TRIGGEREDMODE_SINGLE_TRIGGER;
  hadc1.Init.Oversampling.OversamplingStopReset = ADC_REGOVERSAMPLING_CONTINUED_MODE;
  if (HAL_ADC_Init(&hadc1) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure the ADC multi-mode
  */
  multimode.Mode = ADC_MODE_INDEPENDENT;
  if (HAL_ADCEx_MultiModeConfigChannel(&hadc1, &multimode) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure Regular Channel
  */
  sConfig.Channel = ADC_CHANNEL_VBAT;
  sConfig.Rank = ADC_REGULAR_RANK_1;
  sConfig.SamplingTime = ADC_SAMPLETIME_640CYCLES_5;
  sConfig.SingleDiff = ADC_SINGLE_ENDED;
  sConfig.OffsetNumber = ADC_OFFSET_NONE;
  sConfig.Offset = 0;
  if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure Regular Channel
  */
  sConfig.Channel = ADC_CHANNEL_8;
  sConfig.Rank = ADC_REGULAR_RANK_2;
  if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure Regular Channel
  */
  sConfig.Channel = ADC_CHANNEL_13;
  sConfig.Rank = ADC_REGULAR_RANK_3;
  if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure Regular Channel
  */
  sConfig.Channel = ADC_CHANNEL_14;
  sConfig.Rank = ADC_REGULAR_RANK_4;
  if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure Regular Channel
  */
  sConfig.Channel = ADC_CHANNEL_15;
  sConfig.Rank = ADC_REGULAR_RANK_5;
  if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN ADC1_Init 2 */

  /* USER CODE END ADC1_Init 2 */

}

/**
  * @brief I2C1 Initialization Function
  * @param None
  * @retval None
  */
void MX_I2C1_Init(void)
{

  /* USER CODE BEGIN I2C1_Init 0 */

  /* USER CODE END I2C1_Init 0 */

  /* USER CODE BEGIN I2C1_Init 1 */

  /* USER CODE END I2C1_Init 1 */
  hi2c1.Instance = I2C1;
  hi2c1.Init.Timing = 0x00801A80;
  hi2c1.Init.OwnAddress1 = 0;
  hi2c1.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
  hi2c1.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
  hi2c1.Init.OwnAddress2 = 0;
  hi2c1.Init.OwnAddress2Masks = I2C_OA2_NOMASK;
  hi2c1.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
  hi2c1.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
  if (HAL_I2C_Init(&hi2c1) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure Analogue filter
  */
  if (HAL_I2CEx_ConfigAnalogFilter(&hi2c1, I2C_ANALOGFILTER_ENABLE) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure Digital filter
  */
  if (HAL_I2CEx_ConfigDigitalFilter(&hi2c1, 15) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN I2C1_Init 2 */

  /* USER CODE END I2C1_Init 2 */

}

/**
  * @brief I2C2 Initialization Function
  * @param None
  * @retval None
  */
void MX_I2C2_Init(void)
{

  /* USER CODE BEGIN I2C2_Init 0 */

  /* USER CODE END I2C2_Init 0 */

  /* USER CODE BEGIN I2C2_Init 1 */

  /* USER CODE END I2C2_Init 1 */
  hi2c2.Instance = I2C2;
  hi2c2.Init.Timing = 0x00F01A72;
  hi2c2.Init.OwnAddress1 = 0;
  hi2c2.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
  hi2c2.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
  hi2c2.Init.OwnAddress2 = 0;
  hi2c2.Init.OwnAddress2Masks = I2C_OA2_NOMASK;
  hi2c2.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
  hi2c2.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
  if (HAL_I2C_Init(&hi2c2) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure Analogue filter
  */
  if (HAL_I2CEx_ConfigAnalogFilter(&hi2c2, I2C_ANALOGFILTER_ENABLE) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure Digital filter
  */
  if (HAL_I2CEx_ConfigDigitalFilter(&hi2c2, 15) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN I2C2_Init 2 */

  /* USER CODE END I2C2_Init 2 */

}

/**
  * @brief TIM1 Initialization Function
  * @param None
  * @retval None
  */
void MX_TIM1_Init(void)
{

  /* USER CODE BEGIN TIM1_Init 0 */

  /* USER CODE END TIM1_Init 0 */

  TIM_ClockConfigTypeDef sClockSourceConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};
  TIM_OC_InitTypeDef sConfigOC = {0};
  TIM_BreakDeadTimeConfigTypeDef sBreakDeadTimeConfig = {0};

  /* USER CODE BEGIN TIM1_Init 1 */

  /* USER CODE END TIM1_Init 1 */
  htim1.Instance = TIM1;
  htim1.Init.Prescaler = 800-1;
  htim1.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim1.Init.Period = 1000-1;
  htim1.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim1.Init.RepetitionCounter = 0;
  htim1.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_Base_Init(&htim1) != HAL_OK)
  {
    Error_Handler();
  }
  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
  if (HAL_TIM_ConfigClockSource(&htim1, &sClockSourceConfig) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_TIM_PWM_Init(&htim1) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterOutputTrigger2 = TIM_TRGO2_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim1, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sConfigOC.OCMode = TIM_OCMODE_PWM1;
  sConfigOC.Pulse = 1000-1;
  sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
  sConfigOC.OCNPolarity = TIM_OCNPOLARITY_HIGH;
  sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
  sConfigOC.OCIdleState = TIM_OCIDLESTATE_RESET;
  sConfigOC.OCNIdleState = TIM_OCNIDLESTATE_RESET;
  if (HAL_TIM_PWM_ConfigChannel(&htim1, &sConfigOC, TIM_CHANNEL_1) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_TIM_PWM_ConfigChannel(&htim1, &sConfigOC, TIM_CHANNEL_2) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_TIM_PWM_ConfigChannel(&htim1, &sConfigOC, TIM_CHANNEL_3) != HAL_OK)
  {
    Error_Handler();
  }
  sConfigOC.Pulse = 500-1;
  sConfigOC.OCPolarity = TIM_OCPOLARITY_LOW;
  if (HAL_TIM_PWM_ConfigChannel(&htim1, &sConfigOC, TIM_CHANNEL_4) != HAL_OK)
  {
    Error_Handler();
  }
  sBreakDeadTimeConfig.OffStateRunMode = TIM_OSSR_DISABLE;
  sBreakDeadTimeConfig.OffStateIDLEMode = TIM_OSSI_DISABLE;
  sBreakDeadTimeConfig.LockLevel = TIM_LOCKLEVEL_OFF;
  sBreakDeadTimeConfig.DeadTime = 0;
  sBreakDeadTimeConfig.BreakState = TIM_BREAK_DISABLE;
  sBreakDeadTimeConfig.BreakPolarity = TIM_BREAKPOLARITY_HIGH;
  sBreakDeadTimeConfig.BreakFilter = 0;
  sBreakDeadTimeConfig.Break2State = TIM_BREAK2_DISABLE;
  sBreakDeadTimeConfig.Break2Polarity = TIM_BREAK2POLARITY_HIGH;
  sBreakDeadTimeConfig.Break2Filter = 0;
  sBreakDeadTimeConfig.AutomaticOutput = TIM_AUTOMATICOUTPUT_DISABLE;
  if (HAL_TIMEx_ConfigBreakDeadTime(&htim1, &sBreakDeadTimeConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM1_Init 2 */
  HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_1);
  HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_2);
  HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_3);
  HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_4);
  /* USER CODE END TIM1_Init 2 */
  HAL_TIM_MspPostInit(&htim1);

}

/**
  * @brief TIM3 Initialization Function
  * @param None
  * @retval None
  */
void MX_TIM3_Init(void)
{

  /* USER CODE BEGIN TIM3_Init 0 */

  /* USER CODE END TIM3_Init 0 */

  TIM_ClockConfigTypeDef sClockSourceConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};
  TIM_OC_InitTypeDef sConfigOC = {0};

  /* USER CODE BEGIN TIM3_Init 1 */

  /* USER CODE END TIM3_Init 1 */
  htim3.Instance = TIM3;
  htim3.Init.Prescaler = 8000-1;
  htim3.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim3.Init.Period = 100-1;
  htim3.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim3.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_ENABLE;
  if (HAL_TIM_Base_Init(&htim3) != HAL_OK)
  {
    Error_Handler();
  }
  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
  if (HAL_TIM_ConfigClockSource(&htim3, &sClockSourceConfig) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_TIM_PWM_Init(&htim3) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim3, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sConfigOC.OCMode = TIM_OCMODE_PWM1;
  sConfigOC.Pulse = 0;
  sConfigOC.OCPolarity = TIM_OCPOLARITY_LOW;
  sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
  if (HAL_TIM_PWM_ConfigChannel(&htim3, &sConfigOC, TIM_CHANNEL_3) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_TIM_PWM_ConfigChannel(&htim3, &sConfigOC, TIM_CHANNEL_4) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM3_Init 2 */

  /* USER CODE END TIM3_Init 2 */
  HAL_TIM_MspPostInit(&htim3);

}

/**
  * @brief TIM4 Initialization Function
  * @param None
  * @retval None
  */
void MX_TIM4_Init(void)
{

  /* USER CODE BEGIN TIM4_Init 0 */

  /* USER CODE END TIM4_Init 0 */

  TIM_ClockConfigTypeDef sClockSourceConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};
  TIM_OC_InitTypeDef sConfigOC = {0};

  /* USER CODE BEGIN TIM4_Init 1 */

  /* USER CODE END TIM4_Init 1 */
  htim4.Instance = TIM4;
  htim4.Init.Prescaler = 8000-1;
  htim4.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim4.Init.Period = 100-1;
  htim4.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim4.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_ENABLE;
  if (HAL_TIM_Base_Init(&htim4) != HAL_OK)
  {
    Error_Handler();
  }
  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
  if (HAL_TIM_ConfigClockSource(&htim4, &sClockSourceConfig) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_TIM_PWM_Init(&htim4) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim4, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sConfigOC.OCMode = TIM_OCMODE_PWM1;
  sConfigOC.Pulse = 0;
  sConfigOC.OCPolarity = TIM_OCPOLARITY_LOW;
  sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
  if (HAL_TIM_PWM_ConfigChannel(&htim4, &sConfigOC, TIM_CHANNEL_1) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_TIM_PWM_ConfigChannel(&htim4, &sConfigOC, TIM_CHANNEL_2) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM4_Init 2 */

  /* USER CODE END TIM4_Init 2 */
  HAL_TIM_MspPostInit(&htim4);

}

/**
  * @brief TIM5 Initialization Function
  * @param None
  * @retval None
  */
void MX_TIM5_Init(void)
{

  /* USER CODE BEGIN TIM5_Init 0 */

  /* USER CODE END TIM5_Init 0 */

  TIM_ClockConfigTypeDef sClockSourceConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};

  /* USER CODE BEGIN TIM5_Init 1 */

  /* USER CODE END TIM5_Init 1 */
  htim5.Instance = TIM5;
  htim5.Init.Prescaler = 1;
  htim5.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim5.Init.Period = 1;
  htim5.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim5.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_ENABLE;
  if (HAL_TIM_Base_Init(&htim5) != HAL_OK)
  {
    Error_Handler();
  }
  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
  if (HAL_TIM_ConfigClockSource(&htim5, &sClockSourceConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim5, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM5_Init 2 */

  /* USER CODE END TIM5_Init 2 */

}

/**
  * @brief TIM7 Initialization Function
  * @param None
  * @retval None
  */
void MX_TIM7_Init(void)
{

  /* USER CODE BEGIN TIM7_Init 0 */

  /* USER CODE END TIM7_Init 0 */

  TIM_MasterConfigTypeDef sMasterConfig = {0};

  /* USER CODE BEGIN TIM7_Init 1 */

  /* USER CODE END TIM7_Init 1 */
  htim7.Instance = TIM7;
  htim7.Init.Prescaler = 1;
  htim7.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim7.Init.Period = 65535;
  htim7.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_ENABLE;
  if (HAL_TIM_Base_Init(&htim7) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim7, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM7_Init 2 */

  /* USER CODE END TIM7_Init 2 */

}

/**
  * @brief USART1 Initialization Function
  * @param None
  * @retval None
  */
void MX_USART1_UART_Init(void)
{

  /* USER CODE BEGIN USART1_Init 0 */

  /* USER CODE END USART1_Init 0 */

  /* USER CODE BEGIN USART1_Init 1 */

  /* USER CODE END USART1_Init 1 */
  huart1.Instance = USART1;
  huart1.Init.BaudRate = 1843200;
  huart1.Init.WordLength = UART_WORDLENGTH_8B;
  huart1.Init.StopBits = UART_STOPBITS_1;
  huart1.Init.Parity = UART_PARITY_NONE;
  huart1.Init.Mode = UART_MODE_TX_RX;
  huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart1.Init.OverSampling = UART_OVERSAMPLING_16;
  huart1.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
  huart1.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
  if (HAL_UART_Init(&huart1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART1_Init 2 */

  /* USER CODE END USART1_Init 2 */

}

/**
  * Enable DMA controller clock
  */
void MX_DMA_Init(void)
{

  /* DMA controller clock enable */
  __HAL_RCC_DMA2_CLK_ENABLE();
  __HAL_RCC_DMA1_CLK_ENABLE();

}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
/* USER CODE BEGIN MX_GPIO_Init_1 */
/* USER CODE END MX_GPIO_Init_1 */

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOE_CLK_ENABLE();
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOH_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();
  __HAL_RCC_GPIOD_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOE, XSHUT3_Pin|XSHUT1_Pin|XSHUT2_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(LED0_GPIO_Port, LED0_Pin, GPIO_PIN_SET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOC, LED1_Pin|LED2_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(MOTOR_EN_GPIO_Port, MOTOR_EN_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(CTRL_LEDS_GPIO_Port, CTRL_LEDS_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pins : PE2 PE4 PE5 PE7
                           PE8 PE12 PE0 PE1 */
  GPIO_InitStruct.Pin = GPIO_PIN_2|GPIO_PIN_4|GPIO_PIN_5|GPIO_PIN_7
                          |GPIO_PIN_8|GPIO_PIN_12|GPIO_PIN_0|GPIO_PIN_1;
  GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOE, &GPIO_InitStruct);

  /*Configure GPIO pins : XSHUT3_Pin XSHUT1_Pin XSHUT2_Pin */
  GPIO_InitStruct.Pin = XSHUT3_Pin|XSHUT1_Pin|XSHUT2_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
  HAL_GPIO_Init(GPIOE, &GPIO_InitStruct);

  /*Configure GPIO pin : SW1_Pin */
  GPIO_InitStruct.Pin = SW1_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(SW1_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pins : LED0_Pin LED1_Pin LED2_Pin */
  GPIO_InitStruct.Pin = LED0_Pin|LED1_Pin|LED2_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

  /*Configure GPIO pins : PC0 PC1 PC2 PC3
                           PC6 PC7 PC10 PC11
                           PC12 */
  GPIO_InitStruct.Pin = GPIO_PIN_0|GPIO_PIN_1|GPIO_PIN_2|GPIO_PIN_3
                          |GPIO_PIN_6|GPIO_PIN_7|GPIO_PIN_10|GPIO_PIN_11
                          |GPIO_PIN_12;
  GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

  /*Configure GPIO pins : PA0 PA1 PA2 PA4
                           PA5 PA6 PA7 PA8
                           PA9 PA10 PA11 PA12
                           PA15 */
  GPIO_InitStruct.Pin = GPIO_PIN_0|GPIO_PIN_1|GPIO_PIN_2|GPIO_PIN_4
                          |GPIO_PIN_5|GPIO_PIN_6|GPIO_PIN_7|GPIO_PIN_8
                          |GPIO_PIN_9|GPIO_PIN_10|GPIO_PIN_11|GPIO_PIN_12
                          |GPIO_PIN_15;
  GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pins : PB1 PB12 PB13 PB14
                           PB15 PB4 */
  GPIO_InitStruct.Pin = GPIO_PIN_1|GPIO_PIN_12|GPIO_PIN_13|GPIO_PIN_14
                          |GPIO_PIN_15|GPIO_PIN_4;
  GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /*Configure GPIO pin : SW2_Pin */
  GPIO_InitStruct.Pin = SW2_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(SW2_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pins : PD8 PD9 PD10 PD11
                           PD14 PD15 PD0 PD1
                           PD2 PD3 PD4 PD5
                           PD6 */
  GPIO_InitStruct.Pin = GPIO_PIN_8|GPIO_PIN_9|GPIO_PIN_10|GPIO_PIN_11
                          |GPIO_PIN_14|GPIO_PIN_15|GPIO_PIN_0|GPIO_PIN_1
                          |GPIO_PIN_2|GPIO_PIN_3|GPIO_PIN_4|GPIO_PIN_5
                          |GPIO_PIN_6;
  GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);

  /*Configure GPIO pin : MOTOR_EN_Pin */
  GPIO_InitStruct.Pin = MOTOR_EN_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(MOTOR_EN_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : CTRL_LEDS_Pin */
  GPIO_InitStruct.Pin = CTRL_LEDS_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(CTRL_LEDS_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : MPU6050_INT_Pin */
  GPIO_InitStruct.Pin = MPU6050_INT_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(MPU6050_INT_GPIO_Port, &GPIO_InitStruct);

/* USER CODE BEGIN MX_GPIO_Init_2 */
/* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/**
  * @brief  Period elapsed callback in non blocking mode
  * @note   This function is called  when TIM6 interrupt took place, inside
  * HAL_TIM_IRQHandler(). It makes a direct call to HAL_IncTick() to increment
  * a global variable "uwTick" used as application time base.
  * @param  htim : TIM handle
  * @retval None
  */
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
  /* USER CODE BEGIN Callback 0 */

  /* USER CODE END Callback 0 */
  if (htim->Instance == TIM6) {
    HAL_IncTick();
  }
  /* USER CODE BEGIN Callback 1 */

  /* USER CODE END Callback 1 */
}

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  // __disable_irq();
  // initMicroMouse();
  // __enable_irq();
  while (1)
  {
    /* code */
  }
  
  /* USER CODE END Error_Handler_Debug */
}

#ifdef  USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
