#ifndef VCP_H
#define VCP_H

#include "main.h"
#include <stdint.h>

/* Size of the circular DMA receive buffer: 3-byte header + payload + 3-byte terminator */
#define VCP_BUFFER_SIZE (3 + 5 + (18*5) + 1 + 3)

extern UART_HandleTypeDef huart1;

void VCP_Init(void);
void USART1_Transmit(const uint8_t *data, uint16_t size);

#endif /* VCP_H */
