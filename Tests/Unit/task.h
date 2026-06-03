#ifndef TESTS_UNIT_TASK_H
#define TESTS_UNIT_TASK_H

#include "FreeRTOS.h"

typedef void *TaskHandle_t;

static inline TickType_t xTaskGetTickCount(void)
{
  return 0U;
}

static inline void vTaskDelay(TickType_t ticks_to_delay)
{
  (void)ticks_to_delay;
}

#endif
