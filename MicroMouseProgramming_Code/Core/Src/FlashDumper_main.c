/**
 * FlashDumper_main.c
 *
 * Standalone entry point for the "Flash Dumper" CMake preset. Brings up
 * only SPI2 + the ZD25WQ80C external flash driver and USART1 (PB6/PB7),
 * then streams the entire chip contents as a hex dump over UART on
 * repeat. No robot peripherals (motors, sensors, OLED, etc.) are touched.
 */
#include "main.h"
#include "ZD25WQ80C.h"
#include <stdio.h>
#include <string.h>

SPI_HandleTypeDef  hspi2;
UART_HandleTypeDef huart1;

static void SystemClock_Config(void);
static void UART_Print(const char *msg);
static void UART_DumpLine(uint32_t address, const uint8_t *data, uint8_t len);

int main(void)
{
  HAL_Init();
  SystemClock_Config();

  MX_GPIO_Init();
  MX_SPI2_Init();
  MX_USART1_UART_Init();

  UART_Print("\r\n=== ZD25WQ80C External Flash Dumper ===\r\n");

  if (!initZD25WQ80C())
  {
    UART_Print("Flash init FAILED - check JEDEC ID / wiring\r\n");
    while (1)
    {
      HAL_Delay(1000);
    }
  }

  uint8_t mfr, id_hi, id_lo;
  ZD25WQ80C_ReadJEDECID(&mfr, &id_hi, &id_lo);
  char line[64];
  snprintf(line, sizeof(line), "JEDEC ID: MFR=0x%02X ID=0x%02X%02X\r\n", mfr, id_hi, id_lo);
  UART_Print(line);

  static uint8_t page[ZD25WQ80C_PAGE_SIZE];

  while (1)
  {
    UART_Print("\r\n--- Dumping full flash contents ---\r\n");

    for (uint32_t address = 0; address < ZD25WQ80C_FLASH_SIZE; address += ZD25WQ80C_PAGE_SIZE)
    {
      if (ZD25WQ80C_Read(address, page, sizeof(page)) != HAL_OK)
      {
        UART_Print("Read error\r\n");
        continue;
      }

      for (uint16_t offset = 0; offset < sizeof(page); offset += 16)
      {
        UART_DumpLine(address + offset, &page[offset], 16);
      }
    }

    UART_Print("--- dump complete, restarting in 5s ---\r\n");
    HAL_Delay(5000);
  }
}

static void UART_Print(const char *msg)
{
  HAL_UART_Transmit(&huart1, (uint8_t *)msg, (uint16_t)strlen(msg), HAL_MAX_DELAY);
}

static void UART_DumpLine(uint32_t address, const uint8_t *data, uint8_t len)
{
  char line[80];
  int pos = snprintf(line, sizeof(line), "%08lX: ", (unsigned long)address);
  for (uint8_t i = 0; i < len; i++)
  {
    pos += snprintf(line + pos, sizeof(line) - (size_t)pos, "%02X ", data[i]);
  }
  pos += snprintf(line + pos, sizeof(line) - (size_t)pos, "\r\n");
  HAL_UART_Transmit(&huart1, (uint8_t *)line, (uint16_t)pos, HAL_MAX_DELAY);
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
