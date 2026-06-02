#include "bsp.h"
#include "bsp_uart.h"
#include "bsp_can.h"
#include "bsp_spi.h"
#include "bsp_i2c.h"
#include "bsp_sd.h"

void BSP_Init(void)
{
  BSP_UART_Init();
  BSP_CAN_Init();
  BSP_SPI_Init();
  BSP_I2C_Init();
  BSP_SD_Init();
}
