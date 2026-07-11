/**
 * Diagnostics_it.c
 *
 * Interrupt handlers for the standalone Diagnostics image. TIM6 drives the
 * HAL millisecond tick (see stm32l4xx_hal_timebase_tim.c), TIM4 delivers
 * the quadrature-encoder capture interrupts consumed by Motors.c's
 * HAL_TIM_IC_CaptureCallback(), and DMA2 Channel3 / ADC1 service the
 * circular ADC1 DMA transfer (see Diag_ADC1_Init()/HAL_ADC_MspInit() in
 * Diagnostics_main.c). The Cortex-M fault handlers are provided so a fault
 * halts predictably instead of falling through to the default handler.
 */
#include "main.h"

extern TIM_HandleTypeDef htim6;
extern TIM_HandleTypeDef htim4;
extern ADC_HandleTypeDef hadc1;
extern DMA_HandleTypeDef hdma_adc1;

void NMI_Handler(void)
{
  while (1)
  {
  }
}

void HardFault_Handler(void)
{
  while (1)
  {
  }
}

void MemManage_Handler(void)
{
  while (1)
  {
  }
}

void BusFault_Handler(void)
{
  while (1)
  {
  }
}

void UsageFault_Handler(void)
{
  while (1)
  {
  }
}

void DebugMon_Handler(void)
{
}

void TIM6_DAC_IRQHandler(void)
{
  HAL_TIM_IRQHandler(&htim6);
}

void TIM4_IRQHandler(void)
{
  HAL_TIM_IRQHandler(&htim4);
}

void DMA2_Channel3_IRQHandler(void)
{
  HAL_DMA_IRQHandler(&hdma_adc1);
}

void ADC1_2_IRQHandler(void)
{
  HAL_ADC_IRQHandler(&hadc1);
}
