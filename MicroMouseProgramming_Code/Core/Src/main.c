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
#ifdef COMPILED_BY_SIMULINK
  #ifdef RUN_STUDENT_TEMPLATE
    #include "StudentTemplate.h"
  #else
    #include "MicroMouse_Deploy.h"
  #endif
#endif
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

SPI_HandleTypeDef hspi2;

TIM_HandleTypeDef htim1;
TIM_HandleTypeDef htim3;
TIM_HandleTypeDef htim4;
TIM_HandleTypeDef htim5;
TIM_HandleTypeDef htim7;

/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
#ifndef COMPILING_FOR_MICROPYTHON
void SystemClock_Config(void);
#endif
 void MX_NVIC_Init(void);
 
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
int32_t counter = 0;
volatile int32_t leftEncoderCount = 0;
volatile int32_t rightEncoderCount = 0;

// ADCs
extern uint16_t ADCs[5];

extern uint16_t V_BATT;
extern uint16_t V_PHOTO_DOWN_LS;
extern uint16_t V_PHOTO_DOWN_RS;
extern uint16_t V_PHOTO_MOT_LS;
extern uint16_t V_PHOTO_MOT_RS;

// from other files
extern float IMU_Accel[3];
extern float IMU_Gyro[3];
extern float IMU_Temp;

extern VL53L0_t TOF_sb_left_result;
extern VL53L0_t TOF_sb_front_result;
extern VL53L0_t TOF_sb_right_result;
extern VL53L0_t TOF_sb_front_left_result;
extern VL53L0_t TOF_sb_front_right_result;
extern VL53L0_t TOF_mb_back_result;
extern VL53L0_t TOF_mb_front_result;
extern VL53L0_t TOF_mb_front_left_result;
extern VL53L0_t TOF_mb_front_right_result;

extern LED_t LED0;
extern LED_t LED1;
extern LED_t LED2;
extern SW_t SW1;
extern SW_t SW2;

extern uint8_t batteryLife;
extern int16_t Vbattery, Vshunt, Current, config, Power;
extern float miliwattAVG,miliWattTime,totalPowerUsed;
extern uint8_t STATE;

extern uint8_t USB_storage_buffer[2][USB_BUFFER_SIZE];
extern uint16_t usb_storage_buffer_index[2];
extern uint8_t active_usb_buffer;
extern uint8_t readyToLog;
extern uint32_t log_flash_write_addr;
extern uint32_t log_flash_start_addr;
uint8_t log_time_counter = 0;
uint16_t log_sample_counter = 0;  // Wraps at 65535
 

// Global header configuration - modify these values as needed
uint8_t LOG_VERSION = 1;
uint8_t LOG_SAMPLING_RATE_HZ = 25;  // Will be dynamically calculated in initLogs()
uint8_t EXPECTED_MINUTES = 5;
uint8_t STUDENT_NUMBER[9] = "ABCDEF123";  // 9 characters

// Flash region detection structures (populated during initLogs)
#ifdef FLASH_END
#undef FLASH_END
#endif
#define FLASH_END 0x08080000
#define FLASH_START 0x08000000
#define PAGE_SIZE 2048
#define MAX_FLASH_REGIONS 32

typedef enum {
    REGION_EMPTY = 0,
    REGION_APP = 1,
    REGION_LOG = 2
} RegionType_t;

typedef struct {
    uint32_t start;
    uint32_t end;
    RegionType_t type;
} FlashRegion_t;

FlashRegion_t flash_regions[MAX_FLASH_REGIONS];
uint8_t flash_region_count = 0;
uint8_t log_pages_to_erase[256];  // Pages containing old logs
uint16_t log_erase_page_count = 0;

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
    // UART removed — use MicroMouse_main.c for UART-based Simulink comms

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

void restartI2C(I2C_HandleTypeDef *hi2c){
  hi2c->State = HAL_I2C_STATE_READY;
  hi2c->ErrorCode = HAL_I2C_ERROR_NONE;
}
// Logging

// Metadata header written once at start of log
// UUID MUST be first for log detection!
typedef struct __attribute__((packed)) {
    uint8_t uuid[12];           // FIRST: Used to detect log data pages
    uint8_t version;
    uint8_t sampling_rate_hz;
    uint8_t expected_minutes;
    uint8_t student_number[9];
} MicroMouseLogHeader_t;
 
typedef struct __attribute__((packed)) {
    uint16_t sample_count;
    uint8_t state;
    uint8_t LEDs;              // Packed: bit0=LED0, bit1=LED1, bit2=LED2
    int8_t Motor_Left;
    int8_t Motor_Right;
    uint16_t Distance_Left;
    uint16_t Distance_Front_Left;
    uint16_t Distance_Centre;
    uint16_t Distance_Front_Right;
    uint16_t Distance_Right;
    uint16_t Distance_Back;
    uint8_t PHOTO_DOWN_LS;     // Voltage 0-3.3V mapped to 0-255
    uint8_t PHOTO_DOWN_RS;
    uint8_t PHOTO_MOT_LS;
    uint8_t PHOTO_MOT_RS;
    int16_t IMU_Accel_X;       // Scaled by 1000 (e.g. -9.8 → -9800)
    int16_t IMU_Accel_Y;
    int16_t IMU_Accel_Z;
    int16_t IMU_Gyro_X;
    int16_t IMU_Gyro_Y;
    int16_t IMU_Gyro_Z;
} MicroMouseLog_t;

void initLogs() {
    readyToLog = false;
    #ifndef COMPILED_BY_SIMULINK 
    HAL_DBGMCU_EnableDBGSleepMode();
    HAL_DBGMCU_EnableDBGStandbyMode();
    HAL_DBGMCU_EnableDBGStopMode();
    #endif
    
    // Scan entire flash and classify regions as EMPTY, APP, or LOG
    uint8_t *uid_ptr = (uint8_t*)0x1FFF7590;
    
    flash_region_count = 0;
    log_erase_page_count = 0;
    
    // Step 1: Map flash into 2KB page-aligned regions
    uint32_t region_start = FLASH_START;
    RegionType_t prev_type = REGION_EMPTY;
    uint8_t first_region = 1;
    uint8_t log_region_found = 0;  // Once true, all non-empty pages are logs
    
    for (uint32_t scan_addr = FLASH_START; scan_addr < FLASH_END; scan_addr += PAGE_SIZE) {
        uint8_t *page_ptr = (uint8_t*)scan_addr;
        RegionType_t page_type = REGION_EMPTY;
        
        // Check if page is empty (all 0xFF)
        uint8_t is_empty = 1;
        for (uint32_t i = 0; i < PAGE_SIZE; i += 32) {
            if (page_ptr[i] != 0xFF) {
                is_empty = 0;
                break;
            }
        }
        
        if (!is_empty) {
            // Page has data - determine if it's APP or LOG
            uint8_t is_log = 0;
            
            // If we've already found a log region, all subsequent non-empty pages are logs
            if (log_region_found) {
                is_log = 1;
            } else {
                // Check 1: Does page start with UID? (log header signature)
                uint8_t uid_match = 1;
                for (uint32_t j = 0; j < 12; j++) {
                    if (page_ptr[j] != uid_ptr[j]) {
                        uid_match = 0;
                        break;
                    }
                }
                if (uid_match) {
                    is_log = 1;
                    log_region_found = 1;  // Mark that we've found logs
                }
                
                // Check 2: Scan in 512-byte chunks within page for log patterns
                if (!is_log) {
                    for (uint32_t offset = 0; offset < PAGE_SIZE && !is_log; offset += 512) {
                        uint8_t *check_ptr = page_ptr + offset;
                        
                        // Check for UID at this offset
                        uid_match = 1;
                        for (uint32_t j = 0; j < 12; j++) {
                            if (check_ptr[j] != uid_ptr[j]) {
                                uid_match = 0;
                                break;
                            }
                        }
                        if (uid_match) {
                            is_log = 1;
                            log_region_found = 1;  // Mark that we've found logs
                            break;
                        }
                        
                        // Check for incrementing sample_count pattern
                        if ((PAGE_SIZE - offset) >= sizeof(MicroMouseLogHeader_t) + sizeof(MicroMouseLog_t) * 10) {
                            uint32_t data_start = offset + sizeof(MicroMouseLogHeader_t);
                            uint16_t *sample_ptr = (uint16_t*)(page_ptr + data_start);
                            
                            uint8_t incrementing = 1;
                            for (uint8_t j = 0; j < 5; j++) {
                                uint16_t curr = sample_ptr[j * (sizeof(MicroMouseLog_t)/2)];
                                uint16_t next = sample_ptr[(j+1) * (sizeof(MicroMouseLog_t)/2)];
                                
                                if (next != curr + 1 && !(curr == 0xFFFF && next == 0)) {
                                    incrementing = 0;
                                    break;
                                }
                            }
                            if (incrementing) {
                                is_log = 1;
                                log_region_found = 1;  // Mark that we've found logs
                                break;
                            }
                        }
                    }
                }
            }
            
            page_type = is_log ? REGION_LOG : REGION_APP;
            
            // If this is a log page, add to erase list
            if (is_log) {
                uint8_t page_num = (scan_addr - FLASH_START) / PAGE_SIZE;
                log_pages_to_erase[log_erase_page_count++] = page_num;
            }
        }
        
        // Save region when type changes
        if (first_region || page_type != prev_type) {
            if (!first_region && flash_region_count < MAX_FLASH_REGIONS) {
                flash_regions[flash_region_count].start = region_start;
                flash_regions[flash_region_count].end = scan_addr;
                flash_regions[flash_region_count].type = prev_type;
                flash_region_count++;
                region_start = scan_addr;
            }
            prev_type = page_type;
            first_region = 0;
        }
    }
    
    // Add final region
    if (flash_region_count < MAX_FLASH_REGIONS) {
        flash_regions[flash_region_count].start = region_start;
        flash_regions[flash_region_count].end = FLASH_END;
        flash_regions[flash_region_count].type = prev_type;
        flash_region_count++;
    }
}

bool first_buffer = true;
bool logging_enabled = false;
void refreshLoggedData() {
      // Simulink timer runs at 100Hz (10ms ticks), so divide by sampling rate
      // Example: 100/25 = 4 ticks for 25Hz sampling (every 40ms)
      log_time_counter++;
      uint8_t ticks_per_sample = (LOG_SAMPLING_RATE_HZ > 0) ? (100 / LOG_SAMPLING_RATE_HZ) : 4;
      if (log_time_counter >= ticks_per_sample) {
        readyToLog = true;
        log_time_counter = 0;
      } 

    if (!readyToLog) return;
    readyToLog = false;
    // Enable logging if any button is pressed (active low)
    if (!logging_enabled && (SW1.state != SW2.state)) {
        logging_enabled = true;
        HAL_GPIO_WritePin(MOTOR_EN_GPIO_Port , MOTOR_EN_Pin , GPIO_PIN_SET);
    }
    if (!logging_enabled) return;

    if (first_buffer) {

        // Get our device UID (used for log header)
        uint8_t *uid_ptr = (uint8_t*)0x1FFF7590;
        
        // Step 1: Erase all old log pages (detected during initLogs)
        HAL_FLASH_Unlock();
        
        for (uint16_t i = 0; i < log_erase_page_count; i++) {
            uint32_t erase_addr = FLASH_START + (log_pages_to_erase[i] * PAGE_SIZE);
            
            FLASH_EraseInitTypeDef EraseInitStruct;
            uint32_t PageError;
            
            EraseInitStruct.TypeErase = FLASH_TYPEERASE_PAGES;
            EraseInitStruct.Page = log_pages_to_erase[i];
            EraseInitStruct.NbPages = 1;
            EraseInitStruct.Banks = (erase_addr < 0x08040000) ? FLASH_BANK_1 : FLASH_BANK_2;
            
            HAL_FLASHEx_Erase(&EraseInitStruct, &PageError);
        }
        
        HAL_FLASH_Lock();
        
        // Step 2: Update flash_regions - convert all LOG regions to EMPTY and merge
        for (uint8_t i = 0; i < flash_region_count; i++) {
            if (flash_regions[i].type == REGION_LOG) {
                flash_regions[i].type = REGION_EMPTY;
            }
        }
        
        // Merge adjacent EMPTY regions
        uint8_t write_idx = 0;
        for (uint8_t read_idx = 0; read_idx < flash_region_count; read_idx++) {
            if (write_idx > 0 && 
                flash_regions[write_idx - 1].type == REGION_EMPTY && 
                flash_regions[read_idx].type == REGION_EMPTY) {
                // Merge with previous region
                flash_regions[write_idx - 1].end = flash_regions[read_idx].end;
            } else {
                // Copy region
                if (write_idx != read_idx) {
                    flash_regions[write_idx] = flash_regions[read_idx];
                }
                write_idx++;
            }
        }
        flash_region_count = write_idx;
        
        // Step 3: Find best log start address (first EMPTY region that goes to end of flash)
        log_flash_write_addr = 0;
        for (uint8_t i = 0; i < flash_region_count; i++) {
            if (flash_regions[i].type == REGION_EMPTY && flash_regions[i].end == FLASH_END) {
                log_flash_write_addr = flash_regions[i].start;
                break;
            }
        }
        
        // If no empty region to end of flash, find first empty region after app code
        if (log_flash_write_addr == 0) {
            for (uint8_t i = 0; i < flash_region_count; i++) {
                if (flash_regions[i].type == REGION_EMPTY) {
                    log_flash_write_addr = flash_regions[i].start;
                    break;
                }
            }
        }
        
        // Fallback if no free region found
        if (log_flash_write_addr == 0) {
            log_flash_write_addr = 0x08040000;  // Use midpoint as fallback
        }
        
        // Store the starting address for percentage calculation
        log_flash_start_addr = log_flash_write_addr;
        
        // Calculate optimal sampling rate based on available flash and expected duration
        uint8_t optimal_rate = calculateOptimalSamplingRate(log_flash_write_addr, 
                                                            EXPECTED_MINUTES, 
                                                            sizeof(MicroMouseLog_t));
        
        // Update the global sampling rate (used in header and Simulink timing)
        LOG_SAMPLING_RATE_HZ = optimal_rate;
        
        // Configure TIM7 with calculated rate
        configureTimer(optimal_rate, TIM7);
        HAL_TIM_Base_Start_IT(&htim7);

        // Step 3: Write metadata header at the start of the very first buffer
        MicroMouseLogHeader_t header;
        
        // Populate header from global variables (UUID first for detection!)
        memcpy(header.uuid, uid_ptr, 12);
        header.version = LOG_VERSION;
        header.sampling_rate_hz = LOG_SAMPLING_RATE_HZ;
        header.expected_minutes = EXPECTED_MINUTES;
        memcpy(header.student_number, STUDENT_NUMBER, 9);
        
        // Write header to buffer
        memcpy(USB_storage_buffer[active_usb_buffer], &header, sizeof(MicroMouseLogHeader_t));
        usb_storage_buffer_index[active_usb_buffer] = sizeof(MicroMouseLogHeader_t);
        
        // Change OLED to show logging started: "LOGGING RUN :   0%" (18 chars)
        snprintf(SSD1306_Data.oled_string1, 19, "LOGGING RUN :   0%%");
        
        first_buffer = false;
    }

    MicroMouseLog_t log;
    log.sample_count = log_sample_counter++;  // Auto-wraps at 65535
    log.state = STATE;
    // Pack LEDs into single byte: bit0=LED0, bit1=LED1, bit2=LED2
    log.LEDs = (LED0.state & 0x01) | ((LED1.state & 0x01) << 1) | ((LED2.state & 0x01) << 2);
    log.Motor_Left = (int8_t)MOTOR_L.magnitude;
    log.Motor_Right = (int8_t)MOTOR_R.magnitude;
    log.Distance_Left = (uint16_t)(TOF_sb_left_result.Distance > 4095 ? 4095 : TOF_sb_left_result.Distance);
    log.Distance_Front_Left = (uint16_t)(TOF_sb_front_left_result.Distance > 4095 ? 4095 : TOF_sb_front_left_result.Distance);
    log.Distance_Centre = (uint16_t)(TOF_sb_front_result.Distance > 4095 ? 4095 : TOF_sb_front_result.Distance);
    log.Distance_Front_Right = (uint16_t)(TOF_sb_front_right_result.Distance > 4095 ? 4095 : TOF_sb_front_right_result.Distance);
    log.Distance_Right = (uint16_t)(TOF_sb_right_result.Distance > 4095 ? 4095 : TOF_sb_right_result.Distance);
    log.Distance_Back = (uint16_t)(TOF_mb_back_result.Distance > 4095 ? 4095 : TOF_mb_back_result.Distance);
    // Convert ADC (0-65535) to voltage uint8 (0-255 representing 0-3.3V)
    log.PHOTO_DOWN_LS = (uint8_t)((V_PHOTO_DOWN_LS * 255UL) / 65535UL);
    log.PHOTO_DOWN_RS = (uint8_t)((V_PHOTO_DOWN_RS * 255UL) / 65535UL);
    log.PHOTO_MOT_LS = (uint8_t)((V_PHOTO_MOT_LS * 255UL) / 65535UL);
    log.PHOTO_MOT_RS = (uint8_t)((V_PHOTO_MOT_RS * 255UL) / 65535UL);
    // IMU data scaled by 1000 and stored as signed int16 (e.g. -9.8 m/s² → -9800)
    log.IMU_Accel_X = (int16_t)(IMU_Accel[0] * 1000.0f);
    log.IMU_Accel_Y = (int16_t)(IMU_Accel[1] * 1000.0f);
    log.IMU_Accel_Z = (int16_t)(IMU_Accel[2] * 1000.0f);
    log.IMU_Gyro_X = (int16_t)(IMU_Gyro[0] * 1000.0f);
    log.IMU_Gyro_Y = (int16_t)(IMU_Gyro[1] * 1000.0f);
    log.IMU_Gyro_Z = (int16_t)(IMU_Gyro[2] * 1000.0f);
    
    // Write log to buffer, handling 2KB boundary with partial writes and flushing
    uint8_t *log_bytes = (uint8_t*)&log;
    uint16_t log_offset = 0;
    
    while (log_offset < sizeof(MicroMouseLog_t)) {
        // Calculate how much space is left in current buffer
        uint16_t buffer_space = USB_BUFFER_SIZE - usb_storage_buffer_index[active_usb_buffer];
        
        // Write as much as we can (either remaining log bytes or remaining buffer space)
        uint16_t to_write = (sizeof(MicroMouseLog_t) - log_offset < buffer_space) ? 
                            (sizeof(MicroMouseLog_t) - log_offset) : buffer_space;
        
        memcpy(&USB_storage_buffer[active_usb_buffer][usb_storage_buffer_index[active_usb_buffer]], 
               log_bytes + log_offset, to_write);
        
        usb_storage_buffer_index[active_usb_buffer] += to_write;
        log_offset += to_write;
        
        // If buffer is full (reached 2KB), flush it
        if (usb_storage_buffer_index[active_usb_buffer] >= USB_BUFFER_SIZE) {
            Flash_Write_Data(log_flash_write_addr, USB_storage_buffer[active_usb_buffer], USB_BUFFER_SIZE);
            log_flash_write_addr += USB_BUFFER_SIZE;
            
            // Update OLED with logging percentage: "LOGGING RUN : xxx%" (18 chars)
            #ifdef FLASH_END
#undef FLASH_END
#endif
#define FLASH_END 0x08080000
            uint32_t flash_used = log_flash_write_addr - log_flash_start_addr;
            uint32_t flash_available = FLASH_END - log_flash_start_addr;
            uint8_t percentage = (uint8_t)((flash_used * 100) / flash_available);
            if (percentage > 100) percentage = 100;
            
            // Update percentage digits at positions 14, 15, 16 (% stays at position 17)
            if (percentage < 10) {
                SSD1306_Data.oled_string1[14] = ' ';
                SSD1306_Data.oled_string1[15] = ' ';
                SSD1306_Data.oled_string1[16] = '0' + percentage;
            } else if (percentage < 100) {
                SSD1306_Data.oled_string1[14] = ' ';
                SSD1306_Data.oled_string1[15] = '0' + (percentage / 10);
                SSD1306_Data.oled_string1[16] = '0' + (percentage % 10);
            } else {
                SSD1306_Data.oled_string1[14] = '1';
                SSD1306_Data.oled_string1[15] = '0';
                SSD1306_Data.oled_string1[16] = '0';
            }
            
            // Check if we just wrote to the last page (0x0807F800 to 0x0807FFFF)
            #define FLASH_LAST_PAGE 0x0807F800
            if ((log_flash_write_addr - USB_BUFFER_SIZE) >= FLASH_LAST_PAGE) {
                // Flash log region is full - enter safe mode
                __disable_irq();  // Disable all interrupts
                
                // Turn off motor control FET
                HAL_GPIO_WritePin(MOTOR_EN_GPIO_Port, MOTOR_EN_Pin, GPIO_PIN_RESET);
        MOTOR_L.magnitude = 0;
        MOTOR_R.magnitude = 0;
                
                // Display on OLED
                snprintf(SSD1306_Data.oled_string1, sizeof(SSD1306_Data.oled_string1), "LOGS FULL!");
                snprintf(SSD1306_Data.oled_string2, sizeof(SSD1306_Data.oled_string2), "Safe Mode");
        refreshScreen();
                
                // Infinite loop with LED blinking (1 second on/off using NOP delay)
                while(1) {
                    // Turn LEDs on
                    LED0.state = 1;
                    LED1.state = 1;
                    LED2.state = 1;
                    refreshLEDs();
                    
                    // ~1 second delay using NOPs (80MHz clock, ~80M NOPs = 1 sec)
                    for(volatile uint32_t i = 0; i < 20000000; i++) {
                        __NOP();
                    }
                    
                    // Turn LEDs off
                    LED0.state = 0;
                    LED1.state = 0;
                    LED2.state = 0;
                    refreshLEDs();
                    
                    // ~1 second delay using NOPs
                    for(volatile uint32_t i = 0; i < 20000000; i++) {
                        __NOP();
                    }
                }
            }
            
        active_usb_buffer ^= 1;
        usb_storage_buffer_index[active_usb_buffer] = 0;
        }
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

#define STARTUP_HOLD_MS 5000

void updateMicroMouse(){
  // Motor Control
  // TIM4->CCR1 = 0;
  // TIM4->CCR2 = 0;
  // TIM3->CCR3 = 0;
  // TIM3->CCR4 = 0;

  // update screen
  refreshADCs();
  // refreshScreen();

  // Show sensor data on screen as integers, each digit explicit
  int left_mm = (int)(TOF_sb_left_result.Distance);
  int centre_mm = (int)(TOF_sb_front_result.Distance);
  int right_mm = (int)(TOF_sb_right_result.Distance);
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

  // Block motors for the first STARTUP_HOLD_MS milliseconds after boot
  if (HAL_GetTick() < STARTUP_HOLD_MS) {
    MOTOR_L.magnitude = 0;
    MOTOR_R.magnitude = 0;
  }

  refreshMotors();
  refreshLoggedData();
}

#endif /* COMPILED_BY_SIMULINK */

#define STARTUP_HOLD_MS 5000

static void raw_uart_init(uint32_t baud_divider) {
    RCC->AHB2ENR |= RCC_AHB2ENR_GPIOBEN;
    GPIOB->MODER = (GPIOB->MODER & ~GPIO_MODER_MODE6) | GPIO_MODER_MODE6_1;
    GPIOB->AFR[0] = (GPIOB->AFR[0] & ~GPIO_AFRL_AFSEL6) | (7 << GPIO_AFRL_AFSEL6_Pos);
    RCC->APB2ENR |= RCC_APB2ENR_USART1EN;
    USART1->CR1 &= ~USART_CR1_UE;
    USART1->BRR = baud_divider;
    USART1->CR1 |= USART_CR1_TE | USART_CR1_UE;
}

void raw_uart_print(const char *str) {
    if (USART1 != NULL && (RCC->APB2ENR & RCC_APB2ENR_USART1EN)) {
        for (const char *p = str; *p; p++) {
            while (!(USART1->ISR & USART_ISR_TXE));
            USART1->TDR = (uint8_t)*p;
        }
    }
}

void main(void)
{
  raw_uart_init(35); // 4 MHz MSI clock divider for 115200 baud
  raw_uart_print("\r\n--- STM32 main() Started ---\r\n");

  // Force Backup Domain reset to release LSE clock from PC14/PC15 GPIO pins
  __HAL_RCC_PWR_CLK_ENABLE();
  HAL_PWR_EnableBkUpAccess();
  __HAL_RCC_BACKUPRESET_FORCE();
  __HAL_RCC_BACKUPRESET_RELEASE();

  // Initialize the HAL Library; it must be the first function to be executed
  HAL_Init();
  raw_uart_print("HAL_Init completed.\r\n");

  // Configure the system clock
  SystemClock_Config();
  SystemCoreClockUpdate();
  raw_uart_init(694); // 80 MHz SYSCLK divider for 115200 baud
  raw_uart_print("SystemClock_Config completed (80 MHz).\r\n");

  // Initialize all configured peripherals
  MX_DMA_Init();
  raw_uart_print("DMA initialized.\r\n");
  MX_GPIO_Init();
  raw_uart_print("GPIO initialized.\r\n");
  MX_ADC1_Init();
  raw_uart_print("ADC1 initialized.\r\n");
  MX_I2C1_Init();
  raw_uart_print("I2C1 initialized.\r\n");
  MX_I2C2_Init();
  raw_uart_print("I2C2 initialized.\r\n");
  MX_SPI2_Init();
  raw_uart_print("SPI2 initialized.\r\n");
  MX_TIM1_Init();
  raw_uart_print("TIM1 initialized.\r\n");
  MX_TIM3_Init();
  raw_uart_print("TIM3 initialized.\r\n");
  MX_TIM4_Init();
  raw_uart_print("TIM4 initialized.\r\n");
  MX_TIM5_Init();
  raw_uart_print("TIM5 initialized.\r\n");
  MX_TIM7_Init();
  raw_uart_print("TIM7 initialized.\r\n");
  MX_NVIC_Init();
  raw_uart_print("NVIC initialized.\r\n");

#ifdef COMPILED_BY_SIMULINK
  #ifdef RUN_STUDENT_TEMPLATE
    raw_uart_print("Initializing StudentTemplate model...\r\n");
    StudentTemplate_initialize();
    raw_uart_print("StudentTemplate initialized.\r\n");
  #else
    raw_uart_print("Initializing MicroMouse_Deploy model...\r\n");
    MicroMouse_Deploy_initialize();
    raw_uart_print("MicroMouse_Deploy initialized.\r\n");
  #endif

  // Onboard LED diagnostic check:
  // Turn on LED control gating pin (PB3) first
  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_3, GPIO_PIN_SET);
  if (SSD1306_Data.Initialized) {
      HAL_GPIO_WritePin(GPIOC, GPIO_PIN_14, GPIO_PIN_SET); // LED1 (Green/Success) ON
      HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_RESET);
  } else {
      HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_SET); // LED0 (Red/Failure) ON
      HAL_GPIO_WritePin(GPIOC, GPIO_PIN_14, GPIO_PIN_RESET);
  }
#else
  initMicroMouse();
#endif

  // Configure timers for desired frequencies
  configureTimer(100, TIM5); // Example: configure TIM1 for a frequency of 1000 Hz

  HAL_TIM_Base_Start_IT(&htim5);

  while (1)
  {

    // Process any pending flash writes from USB storage
    #ifdef USE_FLASH

    #endif
    
    // Main loop code here
#ifdef COMPILED_BY_SIMULINK
    static uint32_t last_control_tick = 0;
    if (HAL_GetTick() - last_control_tick >= 10) { // 100Hz (10ms)
        last_control_tick = HAL_GetTick();
        if (HAL_GetTick() > STARTUP_HOLD_MS) {
            #ifdef RUN_STUDENT_TEMPLATE
              StudentTemplate_step();
            #else
              MicroMouse_Deploy_step();
            #endif
        }
    }
#else
    updateMicroMouse();
#endif
    // sendToSimulink();
    // HAL_Delay(100);
    // counter++;
  }
}
/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */

/**
  * @brief System Clock Configuration
  * @retval None
  */
#ifndef COMPILING_FOR_MICROPYTHON
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

  RCC_PeriphCLKInitTypeDef PeriphClkInitStruct = {0};
  PeriphClkInitStruct.PeriphClockSelection = RCC_PERIPHCLK_I2C1 | RCC_PERIPHCLK_I2C2;
  PeriphClkInitStruct.I2c1ClockSelection = RCC_I2C1CLKSOURCE_PCLK1;
  PeriphClkInitStruct.I2c2ClockSelection = RCC_I2C2CLKSOURCE_PCLK1;
  if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInitStruct) != HAL_OK)
  {
    Error_Handler();
  }
}
#endif

/**
  * @brief NVIC Configuration.
  * @retval None
  */
 void MX_NVIC_Init(void)
 
{
  /* FLASH_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(FLASH_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(FLASH_IRQn);
  /* RCC_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(RCC_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(RCC_IRQn);
  /* ADC1_2_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(ADC1_2_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(ADC1_2_IRQn);
  /* TIM4_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(TIM4_IRQn, 0, 0);
  // HAL_NVIC_EnableIRQ(TIM4_IRQn); // Disabled to prevent touch interrupt storms
  /* TIM5_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(TIM5_IRQn, 0, 0);
  // HAL_NVIC_EnableIRQ(TIM5_IRQn); // Disabled to prevent unused interrupt storms
  /* DMA2_Channel6_IRQn interrupt configuration */
  NVIC_SetPriority(DMA2_Channel6_IRQn, NVIC_EncodePriority(NVIC_GetPriorityGrouping(),0, 0));
  NVIC_EnableIRQ(DMA2_Channel6_IRQn);
  /* DMA2_Channel7_IRQn interrupt configuration */
  NVIC_SetPriority(DMA2_Channel7_IRQn, NVIC_EncodePriority(NVIC_GetPriorityGrouping(),0, 0));
  NVIC_EnableIRQ(DMA2_Channel7_IRQn);
  /* TIM7_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(TIM7_IRQn, 0, 0);
  // HAL_NVIC_EnableIRQ(TIM7_IRQn); // Disabled to prevent unused interrupt storms
  /* DMA2_Channel3_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(DMA2_Channel3_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(DMA2_Channel3_IRQn);
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
  hi2c1.Init.Timing = 0x00F01A72;
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
  hi2c2.Init.Timing = 0x00801A80;
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
  * @brief SPI2 Initialization Function
  * @param None
  * @retval None
  */
void MX_SPI2_Init(void)
{

  /* USER CODE BEGIN SPI2_Init 0 */

  /* USER CODE END SPI2_Init 0 */

  /* USER CODE BEGIN SPI2_Init 1 */

  /* USER CODE END SPI2_Init 1 */

  hspi2.Instance               = SPI2;
  hspi2.Init.Mode              = SPI_MODE_MASTER;
  hspi2.Init.Direction         = SPI_DIRECTION_2LINES;
  hspi2.Init.DataSize          = SPI_DATASIZE_8BIT;
  hspi2.Init.CLKPolarity       = SPI_POLARITY_LOW;
  hspi2.Init.CLKPhase          = SPI_PHASE_1EDGE;
  hspi2.Init.NSS               = SPI_NSS_SOFT;
  hspi2.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_8;  /* 80 MHz / 8 = 10 MHz */
  hspi2.Init.FirstBit          = SPI_FIRSTBIT_MSB;
  hspi2.Init.TIMode            = SPI_TIMODE_DISABLE;
  hspi2.Init.CRCCalculation    = SPI_CRCCALCULATION_DISABLE;
  hspi2.Init.CRCPolynomial     = 7;
  hspi2.Init.CRCLength         = SPI_CRC_LENGTH_DATASIZE;
  hspi2.Init.NSSPMode          = SPI_NSS_PULSE_DISABLE;
  if (HAL_SPI_Init(&hspi2) != HAL_OK)
  {
    Error_Handler();
  }

  /* USER CODE BEGIN SPI2_Init 2 */

  /* USER CODE END SPI2_Init 2 */

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
  htim3.Init.Prescaler = 4-1;
  htim3.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim3.Init.Period = 1000-1;
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
  sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
  sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
  if (HAL_TIM_PWM_ConfigChannel(&htim3, &sConfigOC, TIM_CHANNEL_1) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_TIM_PWM_ConfigChannel(&htim3, &sConfigOC, TIM_CHANNEL_2) != HAL_OK)
  {
    Error_Handler();
  }
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
  TIM_IC_InitTypeDef sConfigIC = {0};

  /* USER CODE BEGIN TIM4_Init 1 */

  /* USER CODE END TIM4_Init 1 */
  htim4.Instance = TIM4;
  htim4.Init.Prescaler = 80-1;
  htim4.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim4.Init.Period = 0xFFFF;
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
  if (HAL_TIM_IC_Init(&htim4) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim4, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sConfigIC.ICPolarity = TIM_INPUTCHANNELPOLARITY_RISING;
  sConfigIC.ICSelection = TIM_ICSELECTION_DIRECTTI;
  sConfigIC.ICPrescaler = TIM_ICPSC_DIV1;
  sConfigIC.ICFilter = 0;
  if (HAL_TIM_IC_ConfigChannel(&htim4, &sConfigIC, TIM_CHANNEL_1) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_TIM_IC_ConfigChannel(&htim4, &sConfigIC, TIM_CHANNEL_2) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_TIM_IC_ConfigChannel(&htim4, &sConfigIC, TIM_CHANNEL_3) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_TIM_IC_ConfigChannel(&htim4, &sConfigIC, TIM_CHANNEL_4) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM4_Init 2 */

  /* USER CODE END TIM4_Init 2 */

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

  LL_USART_InitTypeDef USART_InitStruct = {0};

  LL_GPIO_InitTypeDef GPIO_InitStruct = {0};
  RCC_PeriphCLKInitTypeDef PeriphClkInit = {0};

  /** Initializes the peripherals clock
  */
  PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_USART1;
  PeriphClkInit.Usart1ClockSelection = RCC_USART1CLKSOURCE_PCLK2;
  if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK)
  {
    Error_Handler();
  }

  /* Peripheral clock enable */
  LL_APB2_GRP1_EnableClock(LL_APB2_GRP1_PERIPH_USART1);

  LL_AHB2_GRP1_EnableClock(LL_AHB2_GRP1_PERIPH_GPIOB);
  /**USART1 GPIO Configuration
  PB6   ------> USART1_TX
  PB7   ------> USART1_RX
  */
  GPIO_InitStruct.Pin = LL_GPIO_PIN_6|LL_GPIO_PIN_7;
  GPIO_InitStruct.Mode = LL_GPIO_MODE_ALTERNATE;
  GPIO_InitStruct.Speed = LL_GPIO_SPEED_FREQ_VERY_HIGH;
  GPIO_InitStruct.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
  GPIO_InitStruct.Pull = LL_GPIO_PULL_NO;
  GPIO_InitStruct.Alternate = LL_GPIO_AF_7;
  LL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /* USART1 DMA Init */

  /* USART1_RX Init */
  LL_DMA_SetPeriphRequest(DMA2, LL_DMA_CHANNEL_7, LL_DMA_REQUEST_2);

  LL_DMA_SetDataTransferDirection(DMA2, LL_DMA_CHANNEL_7, LL_DMA_DIRECTION_PERIPH_TO_MEMORY);

  LL_DMA_SetChannelPriorityLevel(DMA2, LL_DMA_CHANNEL_7, LL_DMA_PRIORITY_LOW);

  LL_DMA_SetMode(DMA2, LL_DMA_CHANNEL_7, LL_DMA_MODE_CIRCULAR);

  LL_DMA_SetPeriphIncMode(DMA2, LL_DMA_CHANNEL_7, LL_DMA_PERIPH_NOINCREMENT);

  LL_DMA_SetMemoryIncMode(DMA2, LL_DMA_CHANNEL_7, LL_DMA_MEMORY_INCREMENT);

  LL_DMA_SetPeriphSize(DMA2, LL_DMA_CHANNEL_7, LL_DMA_PDATAALIGN_BYTE);

  LL_DMA_SetMemorySize(DMA2, LL_DMA_CHANNEL_7, LL_DMA_MDATAALIGN_BYTE);

  /* USART1_TX Init */
  LL_DMA_SetPeriphRequest(DMA2, LL_DMA_CHANNEL_6, LL_DMA_REQUEST_2);

  LL_DMA_SetDataTransferDirection(DMA2, LL_DMA_CHANNEL_6, LL_DMA_DIRECTION_MEMORY_TO_PERIPH);

  LL_DMA_SetChannelPriorityLevel(DMA2, LL_DMA_CHANNEL_6, LL_DMA_PRIORITY_LOW);

  LL_DMA_SetMode(DMA2, LL_DMA_CHANNEL_6, LL_DMA_MODE_NORMAL);

  LL_DMA_SetPeriphIncMode(DMA2, LL_DMA_CHANNEL_6, LL_DMA_PERIPH_NOINCREMENT);

  LL_DMA_SetMemoryIncMode(DMA2, LL_DMA_CHANNEL_6, LL_DMA_MEMORY_INCREMENT);

  LL_DMA_SetPeriphSize(DMA2, LL_DMA_CHANNEL_6, LL_DMA_PDATAALIGN_BYTE);

  LL_DMA_SetMemorySize(DMA2, LL_DMA_CHANNEL_6, LL_DMA_MDATAALIGN_BYTE);

  /* USER CODE BEGIN USART1_Init 1 */

  /* USER CODE END USART1_Init 1 */
  USART_InitStruct.BaudRate = 1843200;
  USART_InitStruct.DataWidth = LL_USART_DATAWIDTH_8B;
  USART_InitStruct.StopBits = LL_USART_STOPBITS_1;
  USART_InitStruct.Parity = LL_USART_PARITY_NONE;
  USART_InitStruct.TransferDirection = LL_USART_DIRECTION_TX_RX;
  USART_InitStruct.HardwareFlowControl = LL_USART_HWCONTROL_NONE;
  USART_InitStruct.OverSampling = LL_USART_OVERSAMPLING_16;
  LL_USART_Init(USART1, &USART_InitStruct);
  LL_USART_ConfigAsyncMode(USART1);
  LL_USART_Enable(USART1);
  /* USER CODE BEGIN USART1_Init 2 */

  /* USER CODE END USART1_Init 2 */

}

/**
  * Enable DMA controller clock
  */
void MX_DMA_Init(void)
{

  /* Init with LL driver */
  /* DMA controller clock enable */
  LL_AHB1_GRP1_EnableClock(LL_AHB1_GRP1_PERIPH_DMA2);

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
  HAL_GPIO_WritePin(GPIOE, XSHUT5_Pin|XSHUT3_Pin|XSHUT1_Pin|XSHUT4_Pin
                          |XSHUT2_Pin|XSHUT9_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(LED0_GPIO_Port, LED0_Pin, GPIO_PIN_SET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOC, LED1_Pin|LED2_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(XSHUT8_GPIO_Port, XSHUT8_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOD, XSHUT7_Pin|MOTOR_EN_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(CTRL_LEDS_GPIO_Port, CTRL_LEDS_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pins : XSHUT5_Pin XSHUT1_Pin XSHUT9_Pin */
  GPIO_InitStruct.Pin = XSHUT5_Pin|XSHUT1_Pin|XSHUT9_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOE, &GPIO_InitStruct);

  /*Configure GPIO pins : XSHUT3_Pin XSHUT4_Pin XSHUT2_Pin */
  GPIO_InitStruct.Pin = XSHUT3_Pin|XSHUT4_Pin|XSHUT2_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
  HAL_GPIO_Init(GPIOE, &GPIO_InitStruct);

  /*Configure GPIO pins : PE4 PE5 PE7 PE12
                           PE1 */
  GPIO_InitStruct.Pin = GPIO_PIN_4|GPIO_PIN_5|GPIO_PIN_7|GPIO_PIN_12
                          |GPIO_PIN_1;
  GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOE, &GPIO_InitStruct);

  /*Configure GPIO pin : SW1_Pin */
  GPIO_InitStruct.Pin = SW1_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(SW1_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pins : LED0_Pin LED1_Pin LED2_Pin */
  GPIO_InitStruct.Pin = LED0_Pin|LED1_Pin|LED2_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

  /*Configure GPIO pins : PC0 PC1 PC2 PC3
                           PC10 PC11 PC12 */
  GPIO_InitStruct.Pin = GPIO_PIN_0|GPIO_PIN_1|GPIO_PIN_2|GPIO_PIN_3
                          |GPIO_PIN_10|GPIO_PIN_11|GPIO_PIN_12;
  GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

  /*Configure GPIO pins : PA0 PA1 PA2 PA4
                           PA5 PA6 PA7 PA9
                           PA10 PA11 PA12 PA15 */
#ifdef COMPILING_FOR_MICROPYTHON
  GPIO_InitStruct.Pin = GPIO_PIN_0|GPIO_PIN_1|GPIO_PIN_2|GPIO_PIN_4
                          |GPIO_PIN_5|GPIO_PIN_6|GPIO_PIN_7|GPIO_PIN_9
                          |GPIO_PIN_10|GPIO_PIN_15;
#else
  GPIO_InitStruct.Pin = GPIO_PIN_0|GPIO_PIN_1|GPIO_PIN_2|GPIO_PIN_4
                          |GPIO_PIN_5|GPIO_PIN_6|GPIO_PIN_7|GPIO_PIN_9
                          |GPIO_PIN_10|GPIO_PIN_11|GPIO_PIN_12|GPIO_PIN_15;
#endif
  GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pins : PB1 PB4 */
  GPIO_InitStruct.Pin = GPIO_PIN_1|GPIO_PIN_4;
  GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /*Configure GPIO pin : FLASH_CS_Pin (PB12) — deasserted high at boot */
  GPIO_InitStruct.Pin = FLASH_CS_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
  HAL_GPIO_Init(FLASH_CS_GPIO_Port, &GPIO_InitStruct);
  HAL_GPIO_WritePin(FLASH_CS_GPIO_Port, FLASH_CS_Pin, GPIO_PIN_SET);

  /*Configure GPIO pins : SW2_Pin IMU_INT_Pin */
  GPIO_InitStruct.Pin = SW2_Pin|IMU_INT_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /*Configure GPIO pins : PD8 PD9 PD10 PD11
                           PD0 PD1 PD2 PD4
                           PD5 PD6 */
  GPIO_InitStruct.Pin = GPIO_PIN_8|GPIO_PIN_9|GPIO_PIN_10|GPIO_PIN_11
                          |GPIO_PIN_0|GPIO_PIN_1|GPIO_PIN_2|GPIO_PIN_4
                          |GPIO_PIN_5|GPIO_PIN_6;
  GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);

  /*Configure GPIO pin : XSHUT8_Pin */
  GPIO_InitStruct.Pin = XSHUT8_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(XSHUT8_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pins : XSHUT7_Pin MOTOR_EN_Pin */
  GPIO_InitStruct.Pin = XSHUT7_Pin|MOTOR_EN_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);

  /*Configure GPIO pin : CTRL_LEDS_Pin */
  GPIO_InitStruct.Pin = CTRL_LEDS_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(CTRL_LEDS_GPIO_Port, &GPIO_InitStruct);

  /* USER CODE BEGIN MX_GPIO_Init_2 */
  /*Configure GPIO pin Output Level - MB ToF XSHUT pins */
  HAL_GPIO_WritePin(XSHUT7_GPIO_Port, XSHUT7_Pin, GPIO_PIN_RESET);
  HAL_GPIO_WritePin(XSHUT8_GPIO_Port, XSHUT8_Pin, GPIO_PIN_RESET);
  HAL_GPIO_WritePin(XSHUT9_GPIO_Port, XSHUT9_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin : XSHUT7_Pin (PD3 - MB Front ToF) */
  GPIO_InitStruct.Pin = XSHUT7_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
  HAL_GPIO_Init(XSHUT7_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : XSHUT8_Pin (PA8 - MB Front Left ToF) */
  GPIO_InitStruct.Pin = XSHUT8_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
  HAL_GPIO_Init(XSHUT8_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : XSHUT9_Pin (PE0 - MB Front Right ToF) */
  GPIO_InitStruct.Pin = XSHUT9_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
  HAL_GPIO_Init(XSHUT9_GPIO_Port, &GPIO_InitStruct);
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
#ifdef COMPILING_FOR_MICROPYTHON
void jesse_legacy_period_elapsed_callback(TIM_HandleTypeDef *htim)
#else
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
#endif
{
  /* USER CODE BEGIN Callback 0 */

  /* USER CODE END Callback 0 */
  if (htim->Instance == TIM6)
  {
    HAL_IncTick();
  }
  /* USER CODE BEGIN Callback 1 */

  /* USER CODE END Callback 1 */
}

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
#ifndef COMPILING_FOR_MICROPYTHON
void Error_Handler(void)
{
  __disable_irq();
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  GPIO_InitTypeDef GPIO_InitStruct = {0};
  GPIO_InitStruct.Pin = GPIO_PIN_3;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_3, GPIO_PIN_SET);

  GPIO_InitStruct.Pin = GPIO_PIN_13;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

  while (1)
  {
    HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_13);
    for (volatile int i = 0; i < 1000000; i++);
  }
}
#endif
#ifdef USE_FULL_ASSERT
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

/*SimulinkGeneratedCode*/
