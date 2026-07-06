#include "main.h"
#include "DMA.h"

DMA_HandleTypeDef hdma_usart1_rx;

/**
 * @brief  Initialise the HAL DMA handle for USART1 RX (DMA2 Channel 7).
 * @note   The DMA channel registers are already configured by
 *         MX_USART1_UART_Init() via LL. This function only fills in
 *         the HAL handle struct so HAL_DMA_IRQHandler() and
 *         HAL_UART_Receive_DMA() can use it.
 */
void DMA_USART1_Init(void)
{
    hdma_usart1_rx.Instance                 = DMA2_Channel7;
    hdma_usart1_rx.Init.Request             = DMA_REQUEST_2;
    hdma_usart1_rx.Init.Direction           = DMA_PERIPH_TO_MEMORY;
    hdma_usart1_rx.Init.PeriphInc           = DMA_PINC_DISABLE;
    hdma_usart1_rx.Init.MemInc              = DMA_MINC_ENABLE;
    hdma_usart1_rx.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
    hdma_usart1_rx.Init.MemDataAlignment    = DMA_MDATAALIGN_BYTE;
    hdma_usart1_rx.Init.Mode                = DMA_CIRCULAR;
    hdma_usart1_rx.Init.Priority            = DMA_PRIORITY_LOW;
    hdma_usart1_rx.State                    = HAL_DMA_STATE_READY;
}

