#include "main.h"
#include "FreeRTOS.h"
#include "task.h"
#include "app_tasks.h"
#include "bsp.h"
#include "debug_console.h"
#include "debug_trace.h"

static void SystemClock_Config(void);

int main(void)
{
  BaseType_t tasks_init_result;
  uint32_t boot_hold_index;

  HAL_Init();
  SystemClock_Config();

  /***************DEBUG***************/
  DebugConsole_Init();
#ifdef DEBUG_BOOT_UART_ONLY
  while (1) {
    static const char raw_loop[] = "[RAW] " DBG_TEXT_PROJECT " USART1 " DBG_TEXT_LOOP_SEND "\r\n";
    (void)BSP_UART_Send((const uint8_t *)raw_loop, (uint16_t)(sizeof(raw_loop) - 1U), 100U);
    printf("[DBG] " DBG_TEXT_PROJECT " printf " DBG_TEXT_LOOP_SEND "\r\n");
    HAL_Delay(1000U);
  }
#endif
  for (boot_hold_index = 0U; boot_hold_index < 5U; boot_hold_index++) {
    DBG_PRINT("main: " DBG_TEXT_BOOT_HOLD " %lu/5", (unsigned long)(boot_hold_index + 1U));
    HAL_Delay(1000U);
  }

  DBG_PRINT("main: BSP " DBG_TEXT_INIT_START);
  /*************DEBUG END*************/

  BSP_Init();

  /***************DEBUG***************/
  DBG_PRINT("main: BSP " DBG_TEXT_INIT_DONE);
  DBG_PRINT("main: " DBG_TEXT_APP_TASK_INIT_START);
  /*************DEBUG END*************/

  tasks_init_result = App_TasksInit();

  /***************DEBUG***************/
  DBG_PRINT("main: " DBG_TEXT_APP_TASK_INIT_RESULT "=%d", (int)tasks_init_result);
  /*************DEBUG END*************/

  if (tasks_init_result != pdPASS) {
    /***************DEBUG***************/
    DBG_PRINT("main: " DBG_TEXT_APP_TASK_INIT_FAILED);
    /*************DEBUG END*************/
    Error_Handler();
  }

  /***************DEBUG***************/
  DBG_PRINT("main: " DBG_TEXT_ENTER " FreeRTOS " DBG_TEXT_SCHEDULER);
  /*************DEBUG END*************/

  vTaskStartScheduler();

  /***************DEBUG***************/
  DBG_PRINT("main: FreeRTOS " DBG_TEXT_SCHEDULER DBG_TEXT_ABNORMAL_RETURN);
  /*************DEBUG END*************/

  Error_Handler();
}

static void SystemClock_Config(void)
{
  RCC_ClkInitTypeDef RCC_ClkInitStruct;
  RCC_OscInitTypeDef RCC_OscInitStruct;

  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState       = RCC_HSE_ON;
  RCC_OscInitStruct.PLL.PLLState   = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource  = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLM       = 8;
  RCC_OscInitStruct.PLL.PLLN       = 336;
  RCC_OscInitStruct.PLL.PLLP       = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ       = 7;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK) {
    Error_Handler();
  }

  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_SYSCLK |
                                RCC_CLOCKTYPE_HCLK |
                                RCC_CLOCKTYPE_PCLK1 |
                                RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource   = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider  = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;
  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_5) != HAL_OK) {
    Error_Handler();
  }

  if (HAL_GetREVID() >= 0x1001) {
    __HAL_FLASH_PREFETCH_BUFFER_ENABLE();
  }
}

void Error_Handler(void)
{
  __disable_irq();
  while (1) {
  }
}

#ifdef USE_FULL_ASSERT
void assert_failed(uint8_t *file, uint32_t line)
{
  (void)file;
  (void)line;
  Error_Handler();
}
#endif
