/**
 * StandaloneFlash_it.c
 *
 * Minimal interrupt handlers shared by the standalone Flash Dumper and
 * Flash Writer images. Only TIM6 (HAL millisecond tick, see
 * stm32l4xx_hal_timebase_tim.c) is actually enabled; the Cortex-M fault
 * handlers are provided so a fault halts predictably instead of falling
 * through to the default handler.
 */
#include "main.h"

extern TIM_HandleTypeDef htim6;

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
