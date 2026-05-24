#ifndef PREFORMATTED_FLASH_H
#define PREFORMATTED_FLASH_H

#include <stdint.h>
#include "main.h"

#undef STORAGE_LUN_NBR
#undef STORAGE_BLK_NBR
#undef STORAGE_BLK_SIZ  
// #define USE_RAM
#define USE_FLASH
#define USB_FLASH_256KB_FILE

#define USB_FLASH_START_ADDRESS   0x08040000  
#define USB_LOG_DATA_ADDRESS      0x08040000  
#ifndef FLASH_PAGE_SIZE
#define FLASH_PAGE_SIZE 0x800
#endif
#define TOTAL_USB_DEVICE_SIZE   ( STORAGE_BLK_NBR * STORAGE_BLK_SIZ )

#define GAP_PAGES 2  // 3 pages (6KB) gap between code and log for safety
#ifndef FLASH_END
#define FLASH_END 0x08080000  // 512KB flash end
#endif
#define FLASH_MID 0x08040000  // Start scanning from middle

// Utility: Get flash page start address for STM32L476VE (FLASH_PAGE_SIZE = 2048)
#define STM32L476_FLASH_BASE 0x08000000
#define STM32L476_FLASH_PAGE_SIZE 2048
#define STM32L476_NUM_PAGES 383

#ifdef USB_FLASH_72KB_FILE
    #define STORAGE_LUN_NBR                  1
    #define STORAGE_BLK_NBR                  72*2  // enter twice the size of the Memory that you want to use
    #define STORAGE_BLK_SIZ                  0x200
    #define USB_PREFORMATED_SIZE             72*1024
#endif

uint32_t GetPage(uint32_t Address);
void float2Bytes(uint8_t *ftoa_bytes_temp, float float_variable);
float Bytes2float(uint8_t *ftoa_bytes_temp);
uint32_t Flash_Write_Data(uint32_t StartPageAddress, uint8_t *Data, uint32_t numBytes);
void Flash_Read_Data(uint32_t StartPageAddress, uint32_t *RxBuf, uint16_t numberofwords);
void Flash_Write_NUM(uint32_t StartSectorAddress, float Num);
float Flash_Read_NUM(uint32_t StartSectorAddress);

#ifdef USB_FLASH_80KB_FILE
    #define STORAGE_LUN_NBR                  1
    #define STORAGE_BLK_NBR                  80*2  // enter twice the size of the Memory that you want to use
    #define STORAGE_BLK_SIZ                  0x200
    #define USB_PREFORMATED_SIZE             80*1024
#endif

#ifdef USB_FLASH_128KB_FILE
    #define STORAGE_LUN_NBR                  1
    #define STORAGE_BLK_NBR                  128*2  // enter twice the size of the Memory that you want to use
    #define STORAGE_BLK_SIZ                  0x200
    #define USB_PREFORMATED_SIZE             128*1024
#endif

#ifdef USB_FLASH_256KB_FILE
    #define STORAGE_LUN_NBR                  1
    #define STORAGE_BLK_NBR                  256*2  // enter twice the size of the Memory that you want to use
    #define STORAGE_BLK_SIZ                  0x200
    #define USB_PREFORMATED_SIZE             256*1024

    #define LOG_FILE_START                   0x08045C00
    #define LOG_FILE_UID                     0x08045C17
    #define LOG_FILE_DATA                    0x08045C25
#endif

#define USB_BUFFER_SIZE (2*1024)
#define LOG_FLASH_PAGE_SIZE  0x800

#define USB_PREFORMATED (((uint8_t*)USB_FLASH_START_ADDRESS))

// Function to get a pointer to the preformatted data
uint8_t* get_preformatted_data(void);

// Function to get the size of the preformatted data
uint32_t get_preformatted_data_size(void);

// Initialize preformatted flash and update UID
void initPreFormatedFlash(void);

// Dynamically find where to start logging (first page after application code)
uint32_t detectLogStartAddress(void);

// Calculate optimal sampling rate based on available flash and expected duration
uint8_t calculateOptimalSamplingRate(uint32_t log_start_addr, uint8_t expected_minutes, uint16_t log_struct_size);

#endif /* PREFORMATTED_FLASH_H */
