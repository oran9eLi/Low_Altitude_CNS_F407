#include "bsp_uart.h"

void BSP_UART_Init(void)
{
  /* Configure concrete GNSS/Remote-ID/diagnostic UART ports after pin allocation is confirmed. */
}

HAL_StatusTypeDef BSP_UART_Send(const uint8_t *data, uint16_t length, uint32_t timeout_ms)
{
  (void)data;
  (void)length;
  (void)timeout_ms;
  return HAL_OK;
}
