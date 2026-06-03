#ifndef TESTS_UNIT_QUEUE_H
#define TESTS_UNIT_QUEUE_H

#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include "FreeRTOS.h"

typedef struct
{
  UBaseType_t length;
  UBaseType_t item_size;
  UBaseType_t count;
  UBaseType_t head;
  UBaseType_t tail;
  unsigned char *buffer;
} TestQueue_t;

typedef TestQueue_t *QueueHandle_t;

static inline QueueHandle_t xQueueCreate(UBaseType_t queue_length, UBaseType_t item_size)
{
  QueueHandle_t queue = (QueueHandle_t)malloc(sizeof(TestQueue_t));

  if (queue == NULL)
  {
    return NULL;
  }

  queue->buffer = (unsigned char *)malloc((size_t)queue_length * (size_t)item_size);
  if (queue->buffer == NULL)
  {
    free(queue);
    return NULL;
  }

  queue->length = queue_length;
  queue->item_size = item_size;
  queue->count = 0U;
  queue->head = 0U;
  queue->tail = 0U;
  return queue;
}

static inline BaseType_t xQueueSend(QueueHandle_t queue, const void *item, TickType_t ticks_to_wait)
{
  (void)ticks_to_wait;

  if ((queue == NULL) || (item == NULL) || (queue->count >= queue->length))
  {
    return pdFAIL;
  }

  (void)memcpy(&queue->buffer[(size_t)queue->tail * (size_t)queue->item_size], item, queue->item_size);
  queue->tail = (queue->tail + 1U) % queue->length;
  queue->count++;
  return pdPASS;
}

static inline BaseType_t xQueueSendFromISR(QueueHandle_t queue, const void *item, BaseType_t *higher_priority_task_woken)
{
  if (higher_priority_task_woken != NULL)
  {
    *higher_priority_task_woken = pdFALSE;
  }

  return xQueueSend(queue, item, 0U);
}

static inline BaseType_t xQueueReceive(QueueHandle_t queue, void *item, TickType_t ticks_to_wait)
{
  (void)ticks_to_wait;

  if ((queue == NULL) || (item == NULL) || (queue->count == 0U))
  {
    return pdFAIL;
  }

  (void)memcpy(item, &queue->buffer[(size_t)queue->head * (size_t)queue->item_size], queue->item_size);
  queue->head = (queue->head + 1U) % queue->length;
  queue->count--;
  return pdPASS;
}

#endif
