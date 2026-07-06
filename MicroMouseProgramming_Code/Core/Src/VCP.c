#include "main.h"
#include "VCP.h"
#include "DMA.h"
#include <stdbool.h>

/* References to app-layer symbols defined in MicroMouse_main.c */
extern int8_t  bigBuffer[];
extern bool    simulink_talking;
extern void    recievedFromSimulink(void);

UART_HandleTypeDef huart1;

/**
 * @brief  Transmit bytes over USART1 via HAL (blocking).
 */
void USART1_Transmit(const uint8_t *data, uint16_t size)
{
    HAL_UART_Transmit(&huart1, (uint8_t *)data, size, HAL_MAX_DELAY);
}

/**
 * @brief  Initialise HAL UART + DMA handles and start circular DMA RX.
 * @note   Hardware (GPIO, DMA channel config, USART registers) is already
 *         set up by MX_USART1_UART_Init() using LL before this is called.
 *         We build the HAL handles on top of the live hardware without
 *         calling HAL_UART_Init() a second time.
 */
void VCP_Init(void)
{
    /* 1. Build the DMA handle */
    DMA_USART1_Init();

    /* 2. Build the UART handle and link the DMA RX channel */
    huart1.Instance  = USART1;
    huart1.gState    = HAL_UART_STATE_READY;
    huart1.RxState   = HAL_UART_STATE_READY;
    huart1.ErrorCode = HAL_UART_ERROR_NONE;
    __HAL_LINKDMA(&huart1, hdmarx, hdma_usart1_rx);

    /* 3. Enable DMA2 Ch7 interrupt */
    HAL_NVIC_SetPriority(DMA2_Channel7_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(DMA2_Channel7_IRQn);

    /* 4. Arm circular DMA RX into bigBuffer */
    HAL_UART_Receive_DMA(&huart1, (uint8_t *)bigBuffer, VCP_BUFFER_SIZE);
}

/**
 * @brief  HAL UART RX-complete callback – not used; bigBuffer is polled in the main loop.
 */
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *phuart)
{
    if (phuart->Instance == USART1)
    {
        simulink_talking = true;
    }
}
