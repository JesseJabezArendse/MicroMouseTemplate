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
uint32_t log_flash_write_addr = LOG_FLASH_START_ADDR;
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
    uint32_t StartPage = GetPage(StartPageAddress);
    uint32_t EndPageAddress = StartPageAddress + numBytes;
    uint32_t EndPage = GetPage(EndPageAddress);
    HAL_FLASH_Unlock();
    
    // Use fast programming for full 2K pages (much faster than doubleword)
    if (numBytes == STM32L476_FLASH_PAGE_SIZE && (StartPageAddress % STM32L476_FLASH_PAGE_SIZE) == 0)
    {
        // Fast programming: writes in 256-byte rows (8 rows = 2KB page)
        // Note: Flash page must be erased first (or already blank)
        for (uint32_t row = 0; row < 8; row++)
        {
            uint32_t row_addr = StartPageAddress + (row * 256);
            uint32_t row_data = (uint32_t)(Data + (row * 256));
            if (HAL_FLASH_Program(FLASH_TYPEPROGRAM_FAST, row_addr, row_data) != HAL_OK)
            {
                HAL_FLASH_Lock();
                return HAL_FLASH_GetError();
            }
        }
    }
    else
    {
        // Fall back to doubleword programming for partial pages
        for (uint32_t i = 0; i < numBytes; i += 8)
        {
            uint64_t data64 = 0;
            memcpy(&data64, &Data[i], 8);
            if (HAL_FLASH_Program(FLASH_TYPEPROGRAM_DOUBLEWORD, StartPageAddress + i, data64) != HAL_OK)
            {
                HAL_FLASH_Lock();
                return HAL_FLASH_GetError();
            }
        }
    }
    
    HAL_FLASH_Lock();
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


#define UID_BASE 0x1FFF7590

void initPreFormatedFlash(void) {
    uint8_t UID[12];
    memcpy(UID, (uint8_t*)UID_BASE, 12);

    // Read flash page containing LOG_FILE_UID into buffer
    uint32_t page_start = GetPage(LOG_FILE_UID);
    memcpy(USB_storage_buffer[active_usb_buffer], (uint8_t*)page_start, FLASH_PAGE_SIZE);

    // Find and replace 123456789ABC with UID
    uint8_t pattern[12] = "123456789ABC";
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

