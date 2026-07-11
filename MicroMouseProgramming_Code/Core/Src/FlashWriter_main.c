/**
 * FlashWriter_main.c
 *
 * Standalone entry point for the "Flash Writer" CMake preset. Brings up
 * only SPI2 + the ZD25WQ80C external flash driver and USART1 (PB6/PB7),
 * mass-erases the chip, writes FLASH_WRITE_STRING below at address 0, then
 * reads it back and reports the result over UART. No robot peripherals
 * (motors, sensors, OLED, etc.) are touched.
 *
 * Edit FLASH_WRITE_STRING and rebuild this preset to write something else.
 */
#include "main.h"
#include "ZD25WQ80C.h"
#include <stdio.h>
#include <string.h>

/* What gets written to flash address 0 (including the NUL terminator). */
static const char FLASH_WRITE_STRING[] = "Hello MicroMouse 2026!";

SPI_HandleTypeDef  hspi2;
UART_HandleTypeDef huart1;

static void SystemClock_Config(void);
static void UART_Print(const char *msg);

int main(void)
{
  HAL_Init();
  SystemClock_Config();

  MX_GPIO_Init();
  MX_SPI2_Init();
  MX_USART1_UART_Init();

  UART_Print("\r\n=== ZD25WQ80C External Flash Writer ===\r\n");

  if (!initZD25WQ80C())
  {
    UART_Print("Flash init FAILED - check JEDEC ID / wiring\r\n");
    while (1)
    {
      HAL_Delay(1000);
    }
  }

  UART_Print("Mass erasing chip (can take up to ~200s)...\r\n");
  if (ZD25WQ80C_ChipErase() != HAL_OK)
  {
    UART_Print("Chip erase FAILED or timed out\r\n");
    while (1)
    {
      HAL_Delay(1000);
    }
  }
  UART_Print("Erase complete.\r\n");

  uint16_t len = (uint16_t)sizeof(FLASH_WRITE_STRING); /* includes NUL terminator */
  char msg[64];
  snprintf(msg, sizeof(msg), "Writing %u bytes at address 0x000000...\r\n", (unsigned int)len);
  UART_Print(msg);

  if (ZD25WQ80C_PageProgram(0, (const uint8_t *)FLASH_WRITE_STRING, len) != HAL_OK)
  {
    UART_Print("Page program FAILED or timed out\r\n");
    while (1)
    {
      HAL_Delay(1000);
    }
  }
  UART_Print("Write complete.\r\n");

  uint8_t readback[sizeof(FLASH_WRITE_STRING)] = {0};
  if (ZD25WQ80C_Read(0, readback, len) != HAL_OK)
  {
    UART_Print("Readback FAILED\r\n");
  }
  else if (memcmp(readback, FLASH_WRITE_STRING, len) == 0)
  {
    UART_Print("Verify OK: readback matches FLASH_WRITE_STRING.\r\n");
  }
  else
  {
    UART_Print("Verify FAILED: readback does not match.\r\n");
  }

  UART_Print("Readback: \"");
  HAL_UART_Transmit(&huart1, readback, (uint16_t)(len - 1), HAL_MAX_DELAY); /* drop NUL */
  UART_Print("\"\r\n");

  while (1)
  {
    UART_Print("--- idle, write cycle already complete ---\r\n");
    HAL_Delay(5000);
  }
}

static void UART_Print(const char *msg)
{
  HAL_UART_Transmit(&huart1, (uint8_t *)msg, (uint16_t)strlen(msg), HAL_MAX_DELAY);
}

/**
 * @brief System Clock Configuration (same PLL config as the main firmware:
 *        HSI -> PLL -> 80 MHz SYSCLK).
 */
static void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  if (HAL_PWREx_ControlVoltageScaling(PWR_REGULATOR_VOLTAGE_SCALE1) != HAL_OK)
  {
    Error_Handler();
  }

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

  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK
                               | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
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
 * @brief Configures FLASH_CS (PB12) as an output, deasserted high.
 *        SPI2 and USART1 pins are configured by their own MSP callbacks.
 */
void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};

  __HAL_RCC_GPIOB_CLK_ENABLE();

  HAL_GPIO_WritePin(FLASH_CS_GPIO_Port, FLASH_CS_Pin, GPIO_PIN_SET);
  GPIO_InitStruct.Pin   = FLASH_CS_Pin;
  GPIO_InitStruct.Mode  = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull  = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(FLASH_CS_GPIO_Port, &GPIO_InitStruct);
}

void MX_SPI2_Init(void)
{
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
}

/**
 * @brief SPI2 MSP init: PB13/14/15 as AF5 (SCK/MISO/MOSI).
 */
void HAL_SPI_MspInit(SPI_HandleTypeDef *hspi)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
  if (hspi->Instance != SPI2)
  {
    return;
  }

  __HAL_RCC_SPI2_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  GPIO_InitStruct.Pin       = GPIO_PIN_13 | GPIO_PIN_14 | GPIO_PIN_15;
  GPIO_InitStruct.Mode      = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Pull      = GPIO_NOPULL;
  GPIO_InitStruct.Speed     = GPIO_SPEED_FREQ_VERY_HIGH;
  GPIO_InitStruct.Alternate = GPIO_AF5_SPI2;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
}

void MX_USART1_UART_Init(void)
{
  huart1.Instance          = USART1;
  huart1.Init.BaudRate     = 115200;
  huart1.Init.WordLength   = UART_WORDLENGTH_8B;
  huart1.Init.StopBits     = UART_STOPBITS_1;
  huart1.Init.Parity       = UART_PARITY_NONE;
  huart1.Init.Mode         = UART_MODE_TX_RX;
  huart1.Init.HwFlowCtl    = UART_HWCONTROL_NONE;
  huart1.Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_UART_Init(&huart1) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
 * @brief USART1 MSP init: PB6 TX / PB7 RX as AF7.
 */
void HAL_UART_MspInit(UART_HandleTypeDef *huart)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
  if (huart->Instance != USART1)
  {
    return;
  }

  __HAL_RCC_USART1_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  GPIO_InitStruct.Pin       = GPIO_PIN_6 | GPIO_PIN_7;
  GPIO_InitStruct.Mode      = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Pull      = GPIO_NOPULL;
  GPIO_InitStruct.Speed     = GPIO_SPEED_FREQ_VERY_HIGH;
  GPIO_InitStruct.Alternate = GPIO_AF7_USART1;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
}

/**
 * @brief TIM6 drives the 1ms HAL tick (see stm32l4xx_hal_timebase_tim.c).
 */
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
  if (htim->Instance == TIM6)
  {
    HAL_IncTick();
  }
}

void Error_Handler(void)
{
  __disable_irq();
  while (1)
  {
  }
}
