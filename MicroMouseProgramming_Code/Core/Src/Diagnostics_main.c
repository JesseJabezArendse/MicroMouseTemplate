/**
 * Diagnostics_main.c
 *
 * Standalone entry point for the "Micromouse - Diagnostics" CMake preset.
 * Brings up every peripheral bus one at a time (ADC1, I2C1, I2C2, TIM1,
 * TIM3, TIM4, TIM5, TIM7, SPI2), then probes whatever hardware should be
 * behind each bus (INA219, IMU, SSD1306 OLED, 9x VL53L0X ToF, ZD25WQ80C
 * external flash, both drive motors) and prints a PASS/FAIL line per item
 * over USART1 (PB6/PB7, 115200 8N1). Nothing here calls Error_Handler() on
 * a bus or device failure — every step is attempted regardless of earlier
 * results, so a single missing part shows up as one FAIL line instead of
 * a hang.
 *
 * WARNING: the motor test briefly drives each wheel (TIM3 PWM) and checks
 * that TIM4's quadrature encoder capture actually sees edges as a result —
 * no encoder movement within the timeout is a FAIL. Make sure the mouse is
 * on a stand / wheels are clear before running this preset. Internal flash
 * is never touched.
 *
 * "Timeout on fail" is provided by each driver's own bounded HAL timeout
 * (I2C_TIMEOUT, ZD25WQ80C_SPI_TIMEOUT, the VL53L0X driver's internal
 * startTimeout()/checkTimeoutExpired() mechanism, HAL_ADC_PollForConversion's
 * timeout argument, and an explicit HAL_GetTick() bound around the motor/
 * encoder check) rather than a single bespoke watchdog here.
 *
 * After the report, pressing SW1 or SW2 (matching the exact
 * "SW1.state != SW2.state" trigger MicroMouse_main.c uses) starts a real
 * logging burst — but to the EXTERNAL ZD25WQ80C SPI flash only. Internal
 * STM32 flash is never erased or written by this build. Logging starts at
 * external address 0, sector-erasing (4KB) just ahead of the write pointer
 * as it advances, and page-programs (256B) MicroMouseLogHeader_t/
 * MicroMouseLog_t records (same layout as the main firmware, for
 * compatibility with existing log-decoding tools) at a fixed 25 Hz until
 * the entire 1MB ZD25WQ80C is filled. ADC1 runs the full 5-channel DMA-continuous
 * conversion (VBAT + 4 photo sensors), same as the main firmware, so
 * PHOTO_* fields carry real values.
 */
#include "main.h"
#include "ZD25WQ80C.h"
#include "IMU.h"
#include "INA219.h"
#include "SSD1306.h"
#include "VL53L0X.h"
#include "Motors.h"
#include "Buttons.h"
#include "ADCs.h"
#include <stdio.h>
#include <string.h>

ADC_HandleTypeDef  hadc1;
DMA_HandleTypeDef  hdma_adc1;
I2C_HandleTypeDef  hi2c1;
I2C_HandleTypeDef  hi2c2;
TIM_HandleTypeDef  htim1;
TIM_HandleTypeDef  htim3;
TIM_HandleTypeDef  htim4;
TIM_HandleTypeDef  htim5;
TIM_HandleTypeDef  htim7;
SPI_HandleTypeDef  hspi2;
UART_HandleTypeDef huart1;

/* ADCs.c defines these under different names than ADCs.h declares
   (V_BATT/V_PHOTO_* vs VBAT/PHOTO_*) — redeclared here to match what's
   actually defined, same workaround MicroMouse_main.c uses. */
extern uint16_t V_BATT;
extern uint16_t V_PHOTO_DOWN_LS;
extern uint16_t V_PHOTO_DOWN_RS;
extern uint16_t V_PHOTO_MOT_LS;
extern uint16_t V_PHOTO_MOT_RS;

static uint16_t pass_count = 0;
static uint16_t total_count = 0;

/* Same layout as MicroMouseLogHeader_t/MicroMouseLog_t in MicroMouse_main.c
   — kept in sync by hand, since those structs live in that file, not a
   shared header. UUID must stay first (used to detect log data pages). */
typedef struct __attribute__((packed)) {
    uint8_t uuid[12];
    uint8_t version;
    uint8_t sampling_rate_hz;
    uint8_t expected_minutes;
    uint8_t student_number[9];
} MicroMouseLogHeader_t;

typedef struct __attribute__((packed)) {
    uint16_t sample_count;
    uint8_t state;
    uint8_t LEDs;
    int8_t Motor_Left;
    int8_t Motor_Right;
    uint16_t Distance_Left;
    uint16_t Distance_Front_Left;
    uint16_t Distance_Centre;
    uint16_t Distance_Front_Right;
    uint16_t Distance_Right;
    uint16_t Distance_Back;
    uint8_t PHOTO_DOWN_LS;
    uint8_t PHOTO_DOWN_RS;
    uint8_t PHOTO_MOT_LS;
    uint8_t PHOTO_MOT_RS;
    int16_t IMU_Accel_X;
    int16_t IMU_Accel_Y;
    int16_t IMU_Accel_Z;
    int16_t IMU_Gyro_X;
    int16_t IMU_Gyro_Y;
    int16_t IMU_Gyro_Z;
} MicroMouseLog_t;

#define LOG_SAMPLE_RATE_MS  40 /* 25 Hz, matches production's default */

static uint8_t  LOG_VERSION = 1;
static uint8_t  LOG_SAMPLING_RATE_HZ = 25;
static uint8_t  EXPECTED_MINUTES = 1;
static uint8_t  STUDENT_NUMBER[9] = "DIAGNOST";
static uint8_t  STATE = 1;

static uint32_t log_write_addr = 0;      /* next external-flash address to program */
static uint32_t log_erased_sector = 0xFFFFFFFFU; /* sentinel: no sector erased yet */
static uint16_t log_sample_counter = 0;
static uint8_t  logging_full = 0;
static uint8_t  log_page_buf[ZD25WQ80C_PAGE_SIZE];
static uint16_t log_page_index = 0;

static void SystemClock_Config(void);
static void Diag_GPIO_Init(void);
static uint8_t Diag_USART1_Init(void);
static void Diag_DMA_Init(void);
static uint8_t Diag_ADC1_Init(void);
static uint8_t Diag_I2C_Init(I2C_HandleTypeDef *hi2c, I2C_TypeDef *inst, uint32_t timing);
static uint8_t Diag_TIM_Base(TIM_HandleTypeDef *htim, TIM_TypeDef *inst, uint32_t psc, uint32_t period);
static uint8_t Diag_TIM3_PWM_Init(void);
static uint8_t Diag_TIM4_IC_Init(void);
static uint8_t Diag_TestMotor(Motor_t *motor, int16_t magnitude, uint32_t timeout_ms);
static uint8_t Diag_SPI2_Init(void);
static void Diag_EnsureSectorErased(uint32_t addr);
static void Diag_InitLogs(void);
static void Diag_FlushLogPage(void);
static void Diag_LogSample(void);
static void UART_Print(const char *msg);
static void report(const char *name, uint8_t pass, const char *detail);

/**
 * @brief  VL53L0X.c calls this on a failed calibration step; it is only
 *         ever defined in MicroMouse_main.c/main.c in the full firmware.
 *         Reproduced here so the diagnostics build links standalone.
 */
void restartI2C(I2C_HandleTypeDef *hi2c)
{
  HAL_I2C_DeInit(hi2c);
  HAL_I2C_Init(hi2c);
}

int main(void)
{
  HAL_Init();
  SystemClock_Config();

  Diag_GPIO_Init();
  Diag_USART1_Init(); /* if this fails there is no way left to report it */

  UART_Print("\r\n=== MicroMouse Hardware Diagnostics ===\r\n");
  UART_Print("Bringing up each bus, then probing whatever should be behind it.\r\n");
  UART_Print("This WILL briefly spin both wheels to check the encoders. Internal flash is not touched.\r\n\r\n");

  Diag_DMA_Init();
  uint8_t adc_ok  = Diag_ADC1_Init();
  report("ADC1 bus", adc_ok, NULL);

  uint8_t i2c1_ok = Diag_I2C_Init(&hi2c1, I2C1, 0x00F01A72);
  report("I2C1 bus", i2c1_ok, NULL);

  uint8_t i2c2_ok = Diag_I2C_Init(&hi2c2, I2C2, 0x00801A80);
  report("I2C2 bus", i2c2_ok, NULL);

  report("TIM1 base", Diag_TIM_Base(&htim1, TIM1, 800 - 1, 1000 - 1), NULL);

  uint8_t tim3_ok = Diag_TIM3_PWM_Init();
  report("TIM3 PWM (motor drive)", tim3_ok, NULL);

  uint8_t tim4_ok = Diag_TIM4_IC_Init();
  report("TIM4 input capture (encoders)", tim4_ok, NULL);

  report("TIM5 base", Diag_TIM_Base(&htim5, TIM5, 1, 1),     NULL);
  report("TIM7 base", Diag_TIM_Base(&htim7, TIM7, 1, 65535), NULL);

  uint8_t spi2_ok = Diag_SPI2_Init();
  report("SPI2 bus", spi2_ok, NULL);

  UART_Print("\r\n--- Devices ---\r\n");

  if (adc_ok)
  {
    initADCs(); /* starts circular DMA over all 5 channels */
    HAL_Delay(50); /* let a few conversions land before reading */
    refreshADCs();
    char detail[80];
    snprintf(detail, sizeof(detail), "VBAT=%u DOWN_LS=%u DOWN_RS=%u MOT_LS=%u MOT_RS=%u",
             V_BATT, V_PHOTO_DOWN_LS, V_PHOTO_DOWN_RS, V_PHOTO_MOT_LS, V_PHOTO_MOT_RS);
    report("ADC1 VBAT + photo sensors", 1, detail);
  }

  if (i2c1_ok)
  {
    report("INA219 (I2C1)", initINA219(), NULL);
  }

  if (i2c2_ok)
  {
    uint8_t who_am_i = checkIMU();
    char detail[24];
    snprintf(detail, sizeof(detail), "WHO_AM_I=0x%02X", who_am_i);
    report("IMU (I2C2)", who_am_i != 0x00, detail);
    if (who_am_i != 0x00)
    {
      initIMU();
    }

    initScreen();
    report("SSD1306 OLED (I2C2)", SSD1306_Data.Initialized, NULL);
  }

  if (i2c1_ok && i2c2_ok)
  {
    initTOFs(200);
    report("ToF side-left (I2C2)",         TOF_sb_left_result.initialized,        NULL);
    report("ToF side-front-left (I2C2)",   TOF_sb_front_left_result.initialized,  NULL);
    report("ToF side-front (I2C2)",        TOF_sb_front_result.initialized,       NULL);
    report("ToF side-front-right (I2C2)",  TOF_sb_front_right_result.initialized, NULL);
    report("ToF side-right (I2C2)",        TOF_sb_right_result.initialized,       NULL);
    report("ToF main-back (I2C1)",         TOF_mb_back_result.initialized,        NULL);
    report("ToF main-front (I2C2)",        TOF_mb_front_result.initialized,       NULL);
    report("ToF main-front-left (I2C2)",   TOF_mb_front_left_result.initialized,  NULL);
    report("ToF main-front-right (I2C2)",  TOF_mb_front_right_result.initialized, NULL);
  }
  else
  {
    UART_Print("Skipping 9x VL53L0X ToF sensors (I2C1 and/or I2C2 bus unavailable)\r\n");
  }

  if (tim3_ok && tim4_ok)
  {
    initMotors();
    report("Motor R turns (encoder responds)", Diag_TestMotor(&MOTOR_R, 50, 1000), NULL);
    report("Motor L turns (encoder responds)", Diag_TestMotor(&MOTOR_L, 50, 1000), NULL);
  }
  else
  {
    UART_Print("Skipping motor/encoder test (TIM3 and/or TIM4 bus unavailable)\r\n");
  }

  uint8_t flash_ok = 0;
  if (spi2_ok)
  {
    flash_ok = initZD25WQ80C();
    if (flash_ok)
    {
      uint8_t mfr, id_hi, id_lo;
      ZD25WQ80C_ReadJEDECID(&mfr, &id_hi, &id_lo);
      char detail[48];
      snprintf(detail, sizeof(detail), "JEDEC MFR=0x%02X ID=0x%02X%02X", mfr, id_hi, id_lo);
      report("ZD25WQ80C ext. flash (SPI2)", 1, detail);
    }
    else
    {
      report("ZD25WQ80C ext. flash (SPI2)", 0, "JEDEC ID mismatch / no response");
    }
  }

  char summary[48];
  snprintf(summary, sizeof(summary), "\r\n%u / %u checks passed\r\n", pass_count, total_count);
  UART_Print(summary);

  initSW();
  if (spi2_ok && flash_ok)
  {
    UART_Print("Press SW1 or SW2 to log live sensor data to the EXTERNAL flash until it fills (1MB).\r\n");
  }
  else
  {
    UART_Print("External flash unavailable — button-triggered logging is disabled this run.\r\n");
  }

  uint8_t logging_started = 0;
  uint32_t last_status_ms = HAL_GetTick();

  while (1)
  {
    refreshSWValues();

    if (spi2_ok && flash_ok && !logging_started && (SW1.state != SW2.state))
    {
      logging_started = 1;
      UART_Print("\r\nButton pressed - starting log capture to EXTERNAL flash...\r\n");
      Diag_InitLogs();
    }

    if (logging_started && !logging_full)
    {
      Diag_LogSample();
      if ((log_sample_counter % 25) == 0)
      {
        char msg[40];
        snprintf(msg, sizeof(msg), "Logged %u samples...\r\n", log_sample_counter);
        UART_Print(msg);
      }
      HAL_Delay(LOG_SAMPLE_RATE_MS);
    }
    else if (logging_started && logging_full)
    {
      if ((HAL_GetTick() - last_status_ms) > 5000)
      {
        UART_Print("--- external flash log region full, capture stopped, reset to re-run ---\r\n");
        last_status_ms = HAL_GetTick();
      }
    }
    else
    {
      if ((HAL_GetTick() - last_status_ms) > 2000)
      {
        UART_Print("--- diagnostics complete, reset to re-run ---\r\n");
        last_status_ms = HAL_GetTick();
      }
    }
  }
}

static void report(const char *name, uint8_t pass, const char *detail)
{
  total_count++;
  if (pass)
  {
    pass_count++;
  }
  char line[100];
  snprintf(line, sizeof(line), "[%s] %-24s %s\r\n", pass ? " OK " : "FAIL", name, detail ? detail : "");
  UART_Print(line);
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
 * @brief Configures FLASH_CS (PB12) and MOTOR_EN (PD7) as outputs, both
 *        deasserted/low until their respective bus is actually tested.
 *        Every other bus configures its own pins via its MSP callback.
 */
static void Diag_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};

  __HAL_RCC_GPIOB_CLK_ENABLE();
  __HAL_RCC_GPIOD_CLK_ENABLE();

  HAL_GPIO_WritePin(FLASH_CS_GPIO_Port, FLASH_CS_Pin, GPIO_PIN_SET);
  GPIO_InitStruct.Pin   = FLASH_CS_Pin;
  GPIO_InitStruct.Mode  = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull  = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(FLASH_CS_GPIO_Port, &GPIO_InitStruct);

  HAL_GPIO_WritePin(MOTOR_EN_GPIO_Port, MOTOR_EN_Pin, GPIO_PIN_RESET);
  GPIO_InitStruct.Pin   = MOTOR_EN_Pin;
  GPIO_InitStruct.Mode  = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull  = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(MOTOR_EN_GPIO_Port, &GPIO_InitStruct);
}

static uint8_t Diag_USART1_Init(void)
{
  huart1.Instance          = USART1;
  huart1.Init.BaudRate     = 115200;
  huart1.Init.WordLength   = UART_WORDLENGTH_8B;
  huart1.Init.StopBits     = UART_STOPBITS_1;
  huart1.Init.Parity       = UART_PARITY_NONE;
  huart1.Init.Mode         = UART_MODE_TX_RX;
  huart1.Init.HwFlowCtl    = UART_HWCONTROL_NONE;
  huart1.Init.OverSampling = UART_OVERSAMPLING_16;
  return HAL_UART_Init(&huart1) == HAL_OK;
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
 * @brief DMA2 clock enable — required before HAL_DMA_Init() links hdma_adc1.
 */
static void Diag_DMA_Init(void)
{
  __HAL_RCC_DMA2_CLK_ENABLE();
}

/**
 * @brief Full 5-channel oversampled ADC1, circular DMA into ADCs[] — same
 *        configuration as MX_ADC1_Init() in the main firmware, so
 *        ADCs.c's initADCs()/refreshADCs() work unmodified and PHOTO_*
 *        readings are real.
 */
static uint8_t Diag_ADC1_Init(void)
{
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
    return 0;
  }

  ADC_MultiModeTypeDef multimode = {0};
  multimode.Mode = ADC_MODE_INDEPENDENT;
  if (HAL_ADCEx_MultiModeConfigChannel(&hadc1, &multimode) != HAL_OK)
  {
    return 0;
  }

  ADC_ChannelConfTypeDef sConfig = {0};
  sConfig.SamplingTime = ADC_SAMPLETIME_640CYCLES_5;
  sConfig.SingleDiff = ADC_SINGLE_ENDED;
  sConfig.OffsetNumber = ADC_OFFSET_NONE;
  sConfig.Offset = 0;

  sConfig.Channel = ADC_CHANNEL_VBAT;
  sConfig.Rank = ADC_REGULAR_RANK_1;
  if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK) return 0;

  sConfig.Channel = ADC_CHANNEL_8;
  sConfig.Rank = ADC_REGULAR_RANK_2;
  if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK) return 0;

  sConfig.Channel = ADC_CHANNEL_13;
  sConfig.Rank = ADC_REGULAR_RANK_3;
  if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK) return 0;

  sConfig.Channel = ADC_CHANNEL_14;
  sConfig.Rank = ADC_REGULAR_RANK_4;
  if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK) return 0;

  sConfig.Channel = ADC_CHANNEL_15;
  sConfig.Rank = ADC_REGULAR_RANK_5;
  return HAL_ADC_ConfigChannel(&hadc1, &sConfig) == HAL_OK;
}

/**
 * @brief PA3/PC4/PC5/PB0 as analog inputs (ADC_MOT_RS/DOWN_RS/DOWN_LS/MOT_LS,
 *        see ADCs.h) plus the DMA2 Channel3 link, matching HAL_ADC_MspInit
 *        in stm32l4xx_hal_msp.c.
 */
void HAL_ADC_MspInit(ADC_HandleTypeDef *hadc)
{
  if (hadc->Instance != ADC1)
  {
    return;
  }

  RCC_PeriphCLKInitTypeDef PeriphClkInit = {0};
  PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_ADC;
  PeriphClkInit.AdcClockSelection = RCC_ADCCLKSOURCE_SYSCLK;
  HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit);

  __HAL_RCC_ADC_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  GPIO_InitTypeDef GPIO_InitStruct = {0};
  GPIO_InitStruct.Pin = ADC_MOT_RS_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_ANALOG_ADC_CONTROL;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(ADC_MOT_RS_GPIO_Port, &GPIO_InitStruct);

  GPIO_InitStruct.Pin = ADC_DOWN_RS_Pin | ADC_DOWN_LS_Pin;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

  GPIO_InitStruct.Pin = ADC_MOT_LS_Pin;
  HAL_GPIO_Init(ADC_MOT_LS_GPIO_Port, &GPIO_InitStruct);

  hdma_adc1.Instance = DMA2_Channel3;
  hdma_adc1.Init.Request = DMA_REQUEST_0;
  hdma_adc1.Init.Direction = DMA_PERIPH_TO_MEMORY;
  hdma_adc1.Init.PeriphInc = DMA_PINC_DISABLE;
  hdma_adc1.Init.MemInc = DMA_MINC_ENABLE;
  hdma_adc1.Init.PeriphDataAlignment = DMA_PDATAALIGN_HALFWORD;
  hdma_adc1.Init.MemDataAlignment = DMA_MDATAALIGN_HALFWORD;
  hdma_adc1.Init.Mode = DMA_CIRCULAR;
  hdma_adc1.Init.Priority = DMA_PRIORITY_LOW;
  HAL_DMA_Init(&hdma_adc1);

  __HAL_LINKDMA(hadc, DMA_Handle, hdma_adc1);

  HAL_NVIC_SetPriority(DMA2_Channel3_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(DMA2_Channel3_IRQn);
  HAL_NVIC_SetPriority(ADC1_2_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(ADC1_2_IRQn);
}

static uint8_t Diag_I2C_Init(I2C_HandleTypeDef *hi2c, I2C_TypeDef *inst, uint32_t timing)
{
  hi2c->Instance = inst;
  hi2c->Init.Timing = timing;
  hi2c->Init.OwnAddress1 = 0;
  hi2c->Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
  hi2c->Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
  hi2c->Init.OwnAddress2 = 0;
  hi2c->Init.OwnAddress2Masks = I2C_OA2_NOMASK;
  hi2c->Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
  hi2c->Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
  if (HAL_I2C_Init(hi2c) != HAL_OK)
  {
    return 0;
  }
  if (HAL_I2CEx_ConfigAnalogFilter(hi2c, I2C_ANALOGFILTER_ENABLE) != HAL_OK)
  {
    return 0;
  }
  return HAL_I2CEx_ConfigDigitalFilter(hi2c, 15) == HAL_OK;
}

/**
 * @brief I2C1 -> PB8/PB9 (AF4), I2C2 -> PB10/PB11 (AF4). Same pin mapping
 *        as the full firmware's HAL_I2C_MspInit in stm32l4xx_hal_msp.c.
 */
void HAL_I2C_MspInit(I2C_HandleTypeDef *hi2c)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
  RCC_PeriphCLKInitTypeDef PeriphClkInit = {0};

  if (hi2c->Instance == I2C1)
  {
    PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_I2C1;
    PeriphClkInit.I2c1ClockSelection = RCC_I2C1CLKSOURCE_PCLK1;
    HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit);

    __HAL_RCC_GPIOB_CLK_ENABLE();
    GPIO_InitStruct.Pin       = GPIO_PIN_8 | GPIO_PIN_9;
    GPIO_InitStruct.Mode      = GPIO_MODE_AF_OD;
    GPIO_InitStruct.Pull      = GPIO_PULLUP;
    GPIO_InitStruct.Speed     = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF4_I2C1;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

    __HAL_SYSCFG_FASTMODEPLUS_ENABLE(SYSCFG_FASTMODEPLUS_PB8);
    __HAL_SYSCFG_FASTMODEPLUS_ENABLE(SYSCFG_FASTMODEPLUS_PB9);

    __HAL_RCC_I2C1_CLK_ENABLE();
  }
  else if (hi2c->Instance == I2C2)
  {
    PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_I2C2;
    PeriphClkInit.I2c2ClockSelection = RCC_I2C2CLKSOURCE_PCLK1;
    HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit);

    __HAL_RCC_GPIOB_CLK_ENABLE();
    GPIO_InitStruct.Pin       = GPIO_PIN_10 | GPIO_PIN_11;
    GPIO_InitStruct.Mode      = GPIO_MODE_AF_OD;
    GPIO_InitStruct.Pull      = GPIO_PULLUP;
    GPIO_InitStruct.Speed     = GPIO_SPEED_FREQ_MEDIUM;
    GPIO_InitStruct.Alternate = GPIO_AF4_I2C2;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

    __HAL_RCC_I2C2_CLK_ENABLE();
  }
}

/**
 * @brief Base-only timer bring-up (no PWM/IC channels, no output pins,
 *        nothing started) — just proves the peripheral itself configures.
 *        Motors are intentionally never driven by this build.
 */
static uint8_t Diag_TIM_Base(TIM_HandleTypeDef *htim, TIM_TypeDef *inst, uint32_t psc, uint32_t period)
{
  htim->Instance = inst;
  htim->Init.Prescaler = psc;
  htim->Init.CounterMode = TIM_COUNTERMODE_UP;
  htim->Init.Period = period;
  htim->Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim->Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_Base_Init(htim) != HAL_OK)
  {
    return 0;
  }

  TIM_ClockConfigTypeDef sClockSourceConfig = {0};
  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
  return HAL_TIM_ConfigClockSource(htim, &sClockSourceConfig) == HAL_OK;
}

void HAL_TIM_Base_MspInit(TIM_HandleTypeDef *htim)
{
  if (htim->Instance == TIM1)
  {
    __HAL_RCC_TIM1_CLK_ENABLE();
  }
  else if (htim->Instance == TIM3)
  {
    __HAL_RCC_TIM3_CLK_ENABLE();
  }
  else if (htim->Instance == TIM4)
  {
    __HAL_RCC_TIM4_CLK_ENABLE();
    __HAL_RCC_GPIOD_CLK_ENABLE();

    /* TIM4 CH1-4 <- encoder A/B phases (PD12-15), see Motors.h */
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin       = MOTORR_A_ENC_Pin | MOTORR_B_ENC_Pin | MOTORL_A_ENC_Pin | MOTORL_B_ENC_Pin;
    GPIO_InitStruct.Mode      = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull      = GPIO_NOPULL;
    GPIO_InitStruct.Speed     = GPIO_SPEED_FREQ_LOW;
    GPIO_InitStruct.Alternate = GPIO_AF2_TIM4;
    HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);
  }
  else if (htim->Instance == TIM5)
  {
    __HAL_RCC_TIM5_CLK_ENABLE();
  }
  else if (htim->Instance == TIM7)
  {
    __HAL_RCC_TIM7_CLK_ENABLE();
  }
}

/**
 * @brief TIM3 CH1-4 -> H-bridge PWM enable pins (PC6-9), see Motors.h.
 */
void HAL_TIM_MspPostInit(TIM_HandleTypeDef *htim)
{
  if (htim->Instance != TIM3)
  {
    return;
  }

  __HAL_RCC_GPIOC_CLK_ENABLE();
  GPIO_InitTypeDef GPIO_InitStruct = {0};
  GPIO_InitStruct.Pin       = MOTORR_A_EN_Pin | MOTORR_B_EN_Pin | MOTORL_A_EN_Pin | MOTORL_B_EN_Pin;
  GPIO_InitStruct.Mode      = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Pull      = GPIO_NOPULL;
  GPIO_InitStruct.Speed     = GPIO_SPEED_FREQ_LOW;
  GPIO_InitStruct.Alternate = GPIO_AF2_TIM3;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);
}

/**
 * @brief TIM3 PWM bring-up matching Motors.c's expectations (4 PWM
 *        channels driving the H-bridge, see initMotors()/refreshMotors()).
 */
static uint8_t Diag_TIM3_PWM_Init(void)
{
  htim3.Instance = TIM3;
  htim3.Init.Prescaler = 4 - 1;
  htim3.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim3.Init.Period = 1000 - 1;
  htim3.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim3.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_ENABLE;
  if (HAL_TIM_Base_Init(&htim3) != HAL_OK)
  {
    return 0;
  }

  TIM_ClockConfigTypeDef sClockSourceConfig = {0};
  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
  if (HAL_TIM_ConfigClockSource(&htim3, &sClockSourceConfig) != HAL_OK)
  {
    return 0;
  }

  if (HAL_TIM_PWM_Init(&htim3) != HAL_OK)
  {
    return 0;
  }

  TIM_MasterConfigTypeDef sMasterConfig = {0};
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim3, &sMasterConfig) != HAL_OK)
  {
    return 0;
  }

  TIM_OC_InitTypeDef sConfigOC = {0};
  sConfigOC.OCMode = TIM_OCMODE_PWM1;
  sConfigOC.Pulse = 0;
  sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
  sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
  if (HAL_TIM_PWM_ConfigChannel(&htim3, &sConfigOC, TIM_CHANNEL_1) != HAL_OK) return 0;
  if (HAL_TIM_PWM_ConfigChannel(&htim3, &sConfigOC, TIM_CHANNEL_2) != HAL_OK) return 0;
  if (HAL_TIM_PWM_ConfigChannel(&htim3, &sConfigOC, TIM_CHANNEL_3) != HAL_OK) return 0;
  if (HAL_TIM_PWM_ConfigChannel(&htim3, &sConfigOC, TIM_CHANNEL_4) != HAL_OK) return 0;

  HAL_TIM_MspPostInit(&htim3);
  return 1;
}

/**
 * @brief TIM4 input-capture bring-up matching Motors.c's expectations
 *        (quadrature encoder A/B phases on CH1-4, see initMotors() and
 *        HAL_TIM_IC_CaptureCallback()).
 */
static uint8_t Diag_TIM4_IC_Init(void)
{
  htim4.Instance = TIM4;
  htim4.Init.Prescaler = 80 - 1;
  htim4.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim4.Init.Period = 0xFFFF;
  htim4.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim4.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_ENABLE;
  if (HAL_TIM_Base_Init(&htim4) != HAL_OK)
  {
    return 0;
  }

  TIM_ClockConfigTypeDef sClockSourceConfig = {0};
  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
  if (HAL_TIM_ConfigClockSource(&htim4, &sClockSourceConfig) != HAL_OK)
  {
    return 0;
  }

  if (HAL_TIM_IC_Init(&htim4) != HAL_OK)
  {
    return 0;
  }

  TIM_MasterConfigTypeDef sMasterConfig = {0};
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim4, &sMasterConfig) != HAL_OK)
  {
    return 0;
  }

  TIM_IC_InitTypeDef sConfigIC = {0};
  sConfigIC.ICPolarity = TIM_INPUTCHANNELPOLARITY_RISING;
  sConfigIC.ICSelection = TIM_ICSELECTION_DIRECTTI;
  sConfigIC.ICPrescaler = TIM_ICPSC_DIV1;
  sConfigIC.ICFilter = 0;
  if (HAL_TIM_IC_ConfigChannel(&htim4, &sConfigIC, TIM_CHANNEL_1) != HAL_OK) return 0;
  if (HAL_TIM_IC_ConfigChannel(&htim4, &sConfigIC, TIM_CHANNEL_2) != HAL_OK) return 0;
  if (HAL_TIM_IC_ConfigChannel(&htim4, &sConfigIC, TIM_CHANNEL_3) != HAL_OK) return 0;
  if (HAL_TIM_IC_ConfigChannel(&htim4, &sConfigIC, TIM_CHANNEL_4) != HAL_OK) return 0;

  HAL_NVIC_SetPriority(TIM4_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(TIM4_IRQn);
  return 1;
}

/**
 * @brief Drives one motor at `magnitude` and waits up to `timeout_ms` for
 *        Motors.c's TIM4 capture callback to report a nonzero encoderRate.
 *        The motor is stopped before returning either way — this never
 *        leaves a wheel spinning after the check completes.
 */
static uint8_t Diag_TestMotor(Motor_t *motor, int16_t magnitude, uint32_t timeout_ms)
{
  motor->encoderRate = 0;
  motor->magnitude = magnitude;

  uint32_t start = HAL_GetTick();
  uint8_t moved = 0;
  while ((HAL_GetTick() - start) < timeout_ms)
  {
    refreshMotors();
    if (motor->encoderRate != 0)
    {
      moved = 1;
      break;
    }
    HAL_Delay(10);
  }

  motor->magnitude = 0;
  refreshMotors();
  HAL_Delay(50); /* let the wheel coast to a stop before the next test/step */
  return moved;
}

static uint8_t Diag_SPI2_Init(void)
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
  return HAL_SPI_Init(&hspi2) == HAL_OK;
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

/**
 * @brief Sector-erase (4KB) the external-flash sector containing `addr`,
 *        but only if it isn't the one already erased last time — the log
 *        write pointer only ever moves forward, so each sector needs
 *        erasing at most once per logging session.
 */
static void Diag_EnsureSectorErased(uint32_t addr)
{
  uint32_t sector = addr & ~((uint32_t)ZD25WQ80C_SECTOR_SIZE - 1);
  if (sector != log_erased_sector)
  {
    ZD25WQ80C_SectorErase(sector);
    log_erased_sector = sector;
  }
}

/**
 * @brief Start a fresh log at external address 0 and write the
 *        MicroMouseLogHeader_t as the first page.
 */
static void Diag_InitLogs(void)
{
  log_write_addr = 0;
  log_erased_sector = 0xFFFFFFFFU;
  log_sample_counter = 0;
  logging_full = 0;
  log_page_index = 0;

  Diag_EnsureSectorErased(log_write_addr);

  MicroMouseLogHeader_t header;
  uint8_t *uid_ptr = (uint8_t *)0x1FFF7590;
  memcpy(header.uuid, uid_ptr, 12);
  header.version = LOG_VERSION;
  header.sampling_rate_hz = LOG_SAMPLING_RATE_HZ;
  header.expected_minutes = EXPECTED_MINUTES;
  memcpy(header.student_number, STUDENT_NUMBER, 9);

  memcpy(log_page_buf, &header, sizeof(header));
  log_page_index = sizeof(header);
}

/**
 * @brief Program the buffered page (<= ZD25WQ80C_PAGE_SIZE bytes) at
 *        log_write_addr, erasing the containing sector first if needed,
 *        then advance the write pointer by one page.
 */
static void Diag_FlushLogPage(void)
{
  if (log_page_index == 0)
  {
    return;
  }

  Diag_EnsureSectorErased(log_write_addr);
  ZD25WQ80C_PageProgram(log_write_addr, log_page_buf, log_page_index);
  log_write_addr += ZD25WQ80C_PAGE_SIZE;
  log_page_index = 0;

  if (log_write_addr >= ZD25WQ80C_FLASH_SIZE)
  {
    logging_full = 1;
  }
}

/**
 * @brief Refresh live sensor values, pack one MicroMouseLog_t record, and
 *        buffer/flush it to the EXTERNAL flash — including PHOTO_* (same
 *        0-65535 -> 0-255 scaling formula as production, applied to the
 *        raw 12-bit ADC reading; kept identical for compatibility even
 *        though it means PHOTO_* only ever reaches the low end of the
 *        0-255 range).
 */
static void Diag_LogSample(void)
{
  refreshIMUValues();
  refreshTOFValues();
  refreshINA219Values();
  refreshMotors();
  refreshADCs();

  MicroMouseLog_t log;
  memset(&log, 0, sizeof(log));
  log.sample_count = log_sample_counter++;
  log.state = STATE;
  log.Motor_Left  = (int8_t)MOTOR_L.magnitude;
  log.Motor_Right = (int8_t)MOTOR_R.magnitude;
  log.Distance_Left        = (uint16_t)(TOF_sb_left_result.Distance        > 4095 ? 4095 : TOF_sb_left_result.Distance);
  log.Distance_Front_Left  = (uint16_t)(TOF_sb_front_left_result.Distance  > 4095 ? 4095 : TOF_sb_front_left_result.Distance);
  log.Distance_Centre      = (uint16_t)(TOF_sb_front_result.Distance       > 4095 ? 4095 : TOF_sb_front_result.Distance);
  log.Distance_Front_Right = (uint16_t)(TOF_sb_front_right_result.Distance > 4095 ? 4095 : TOF_sb_front_right_result.Distance);
  log.Distance_Right       = (uint16_t)(TOF_sb_right_result.Distance       > 4095 ? 4095 : TOF_sb_right_result.Distance);
  log.Distance_Back        = (uint16_t)(TOF_mb_back_result.Distance        > 4095 ? 4095 : TOF_mb_back_result.Distance);
  log.PHOTO_DOWN_LS = (uint8_t)((V_PHOTO_DOWN_LS * 255UL) / 65535UL);
  log.PHOTO_DOWN_RS = (uint8_t)((V_PHOTO_DOWN_RS * 255UL) / 65535UL);
  log.PHOTO_MOT_LS  = (uint8_t)((V_PHOTO_MOT_LS  * 255UL) / 65535UL);
  log.PHOTO_MOT_RS  = (uint8_t)((V_PHOTO_MOT_RS  * 255UL) / 65535UL);
  log.IMU_Accel_X = (int16_t)(IMU_Accel[0] * 1000.0f);
  log.IMU_Accel_Y = (int16_t)(IMU_Accel[1] * 1000.0f);
  log.IMU_Accel_Z = (int16_t)(IMU_Accel[2] * 1000.0f);
  log.IMU_Gyro_X  = (int16_t)(IMU_Gyro[0] * 1000.0f);
  log.IMU_Gyro_Y  = (int16_t)(IMU_Gyro[1] * 1000.0f);
  log.IMU_Gyro_Z  = (int16_t)(IMU_Gyro[2] * 1000.0f);

  uint8_t *log_bytes = (uint8_t *)&log;
  uint16_t log_offset = 0;

  while (log_offset < sizeof(log))
  {
    uint16_t page_space = ZD25WQ80C_PAGE_SIZE - log_page_index;
    uint16_t to_write = (sizeof(log) - log_offset < page_space) ? (uint16_t)(sizeof(log) - log_offset) : page_space;

    memcpy(&log_page_buf[log_page_index], log_bytes + log_offset, to_write);
    log_page_index += to_write;
    log_offset += to_write;

    if (log_page_index >= ZD25WQ80C_PAGE_SIZE)
    {
      Diag_FlushLogPage();
      if (logging_full)
      {
        return;
      }
    }
  }
}

void Error_Handler(void)
{
  __disable_irq();
  while (1)
  {
  }
}
