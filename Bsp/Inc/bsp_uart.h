#ifndef BSP_UART_H
#define BSP_UART_H

#include "stm32f4xx_hal.h"

HAL_StatusTypeDef BSP_UART_Init(void);
HAL_StatusTypeDef BSP_UART_Send(const uint8_t *data, uint16_t length, uint32_t timeout_ms);

#endif
