#include "stm32l4xx_hal.h"
#include <string.h>
#include <stdint.h>
#include "preformatted_flash.h"
#include "stm32l4xx_hal.h"
#include <string.h>
#include "main.h"

uint8_t STATE = 1;

// Buffer MUST be 8-byte aligned for FLASH_TYPEPROGRAM_FAST
__attribute__((aligned(8))) uint8_t USB_storage_buffer[2][USB_BUFFER_SIZE];
uint16_t usb_storage_buffer_index[2] = {0, 0};
uint8_t active_usb_buffer = 0;
uint32_t log_flash_write_addr = 0;  // Will be set dynamically by detectLogStartAddress()
uint32_t log_flash_start_addr = 0;  // Starting address for percentage calculation
uint8_t readyToLog;

uint32_t GetPage(uint32_t Address)
{
    for (int indx = 0; indx < STM32L476_NUM_PAGES; indx++)
    {
        uint32_t page_start = STM32L476_FLASH_BASE + STM32L476_FLASH_PAGE_SIZE * indx;
        uint32_t page_end = page_start + STM32L476_FLASH_PAGE_SIZE;
        if ((Address >= page_start) && (Address < page_end))
            return page_start;
    }
    return 0;
}

uint32_t detectLogStartAddress(void)
{
    // Dynamically find where to start logging by finding first page after application code
    // Strategy: Scan backwards from middle of flash to find last non-0xFF page (end of code)
    
    uint32_t scan_addr = FLASH_MID - STM32L476_FLASH_PAGE_SIZE;  // Start just before middle
    uint32_t last_code_page = STM32L476_FLASH_BASE;
    
    // Scan backwards from middle to find first non-0xFF page (this is end of application code)
    while (scan_addr > STM32L476_FLASH_BASE) {
        // Check if this page has any non-0xFF data
        uint8_t *page_ptr = (uint8_t*)scan_addr;
        uint32_t non_ff_count = 0;
        
        for (uint32_t i = 0; i < STM32L476_FLASH_PAGE_SIZE; i++) {
            if (page_ptr[i] != 0xFF) {
                non_ff_count++;
                break;  // Found at least one non-0xFF byte, that's enough
            }
        }
        
        if (non_ff_count > 0) {
            // Found non-0xFF page - this is the last page of application code
            last_code_page = scan_addr;
            break;
        }
        
        scan_addr -= STM32L476_FLASH_PAGE_SIZE;
    }
    
    // Log starts GAP_PAGES after the last code page (already aligned to 2KB)
    uint32_t log_start = last_code_page + ((GAP_PAGES + 1) * STM32L476_FLASH_PAGE_SIZE);
    
    // Ensure we don't go beyond flash and maintain minimum spacing
    if (log_start >= FLASH_END || log_start < last_code_page) {
        log_start = FLASH_MID;  // Fallback to middle
    }
    
    return log_start;
}

uint8_t calculateOptimalSamplingRate(uint32_t log_start_addr, uint8_t expected_minutes, uint16_t log_struct_size)
{
    // Calculate maximum sampling rate based on available flash and expected duration
    #define MAX_SAMPLING_RATE 100  // Hz - maximum allowed
#ifdef FLASH_END
#undef FLASH_END
#endif
    #define FLASH_END 0x08080000
    
    // Calculate available flash space for logging
    if (log_start_addr >= FLASH_END || expected_minutes == 0) {
        return 10;  // Fallback to 10Hz if invalid parameters
    }
    
    uint32_t available_flash = FLASH_END - log_start_addr;
    uint32_t max_samples = available_flash / log_struct_size;
    uint32_t expected_seconds = expected_minutes * 60;
    
    // Calculate optimal sampling rate (samples per second)
    uint32_t optimal_rate = max_samples / expected_seconds;
    
    // Cap at maximum rate
    if (optimal_rate > MAX_SAMPLING_RATE) {
        optimal_rate = MAX_SAMPLING_RATE;
    }
    
    // Ensure minimum rate of 1Hz
    if (optimal_rate < 1) {
        optimal_rate = 1;
    }
    
    return (uint8_t)optimal_rate;
}

uint8_t bytes_temp[4];

void float2Bytes(uint8_t *ftoa_bytes_temp, float float_variable)
{
    union {
        float a;
        uint8_t bytes[4];
    } thing;
    thing.a = float_variable;
    for (uint8_t i = 0; i < 4; i++) {
        ftoa_bytes_temp[i] = thing.bytes[i];
    }
}

float Bytes2float(uint8_t *ftoa_bytes_temp)
{
    union {
        float a;
        uint8_t bytes[4];
    } thing;
    for (uint8_t i = 0; i < 4; i++) {
        thing.bytes[i] = ftoa_bytes_temp[i];
    }
    return thing.a;
}

uint32_t Flash_Write_Data(uint32_t StartPageAddress, uint8_t *Data, uint32_t numBytes)
{
    // Disable instruction cache for flash operations
    __HAL_FLASH_INSTRUCTION_CACHE_DISABLE();
    __HAL_FLASH_DATA_CACHE_DISABLE();
    
    HAL_FLASH_Unlock();
    
    // Use doubleword programming (works on any bank, including executing bank)
    // Fast programming only works on non-executing bank, so we use doubleword for reliability
    for (uint32_t i = 0; i < numBytes; i += 8)
    {
        uint64_t data64 = 0;
        memcpy(&data64, &Data[i], 8);
        if (HAL_FLASH_Program(FLASH_TYPEPROGRAM_DOUBLEWORD, StartPageAddress + i, data64) != HAL_OK)
        {
            HAL_FLASH_Lock();
            __HAL_FLASH_INSTRUCTION_CACHE_RESET();
            __HAL_FLASH_DATA_CACHE_RESET();
            __HAL_FLASH_INSTRUCTION_CACHE_ENABLE();
            __HAL_FLASH_DATA_CACHE_ENABLE();
            return HAL_FLASH_GetError();
        }
    }
    
    HAL_FLASH_Lock();
    
    // Re-enable caches and invalidate them after flash operations
    __HAL_FLASH_INSTRUCTION_CACHE_RESET();
    __HAL_FLASH_DATA_CACHE_RESET();
    __HAL_FLASH_INSTRUCTION_CACHE_ENABLE();
    __HAL_FLASH_DATA_CACHE_ENABLE();
    
    return 0;
}

void Flash_Read_Data(uint32_t StartPageAddress, uint32_t *RxBuf, uint16_t numberofwords)
{
    while (numberofwords--)
    {
        *RxBuf = *(__IO uint32_t *)StartPageAddress;
        StartPageAddress += 4;
        RxBuf++;
    }
}

void Flash_Write_NUM(uint32_t StartSectorAddress, float Num)
{
    float2Bytes(bytes_temp, Num);
    Flash_Write_Data(StartSectorAddress, (uint8_t *)bytes_temp, 1);
}

float Flash_Read_NUM(uint32_t StartSectorAddress)
{
    uint8_t buffer[4];
    Flash_Read_Data(StartSectorAddress, (uint32_t *)buffer, 1);
    return Bytes2float(buffer);
}

#ifdef USE_RAM
uint8_t USB_storage_buffer[STORAGE_BLK_NBR*STORAGE_BLK_SIZ];
#endif
#ifdef USE_FLASH

uint8_t UID[12] = {0};


#ifndef UID_BASE
#define UID_BASE 0x1FFF7590
#endif

void initPreFormatedFlash(void) {
    uint8_t UID[12];
    memcpy(UID, (uint8_t*)UID_BASE, 12);

    // Read flash page containing LOG_FILE_UID into buffer
    uint32_t page_start = GetPage(LOG_FILE_UID);
    memcpy(USB_storage_buffer[active_usb_buffer], (uint8_t*)page_start, FLASH_PAGE_SIZE);

    // Find and replace 123456789ABC with UID
    uint8_t pattern[12] = {'1','2','3','4','5','6','7','8','9','A','B','C'};
    for (uint32_t i = 0; i <= FLASH_PAGE_SIZE - 12; ++i) {
        if (memcmp(&USB_storage_buffer[active_usb_buffer][i], pattern, 12) == 0) {
            memcpy(&USB_storage_buffer[active_usb_buffer][i], UID, 12);
            break;
        }
    }

    // Write buffer back to flash using new utility
    Flash_Write_Data(page_start, (uint8_t*)USB_storage_buffer[active_usb_buffer], FLASH_PAGE_SIZE);
}

// Write the buffer to flash at USB_LOG_DATA_ADDRESS
void write_usb_storage_buffer_to_flash(void) {
    HAL_FLASH_Unlock();
    // Erase the page if needed (optional, depends on your use case)
    FLASH_EraseInitTypeDef eraseInit;
    uint32_t pageError;
    eraseInit.TypeErase = FLASH_TYPEERASE_PAGES;
    eraseInit.Page = (USB_LOG_DATA_ADDRESS - FLASH_BASE) / FLASH_PAGE_SIZE;
    eraseInit.NbPages = 1;
    HAL_FLASHEx_Erase(&eraseInit, &pageError);

    // Write buffer to flash
    for (uint32_t i = 0; i < USB_BUFFER_SIZE; i += 8) {
        uint64_t data = 0;
        memcpy(&data, &USB_storage_buffer[active_usb_buffer][i], 8);
        HAL_FLASH_Program(FLASH_TYPEPROGRAM_DOUBLEWORD, USB_LOG_DATA_ADDRESS + i, data);
    }
    HAL_FLASH_Lock();
}
#endif


// Function to get a pointer to the preformatted data
uint8_t* get_preformatted_data(void) {
    return USB_PREFORMATED;
}

// Function to get the size of the preformatted data
uint32_t get_preformatted_data_size(void) {
    return USB_PREFORMATED_SIZE;
}
