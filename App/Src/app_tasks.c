#include "app_tasks.h"
#include <string.h>
#include "task.h"
#include "app_alarm.h"
#include "app_logger.h"
#include "app_protocol.h"
#include "app_registry.h"
#include "app_status.h"

#define APP_TASK_STACK_SYSTEM   (configMINIMAL_STACK_SIZE * 2U)
#define APP_TASK_STACK_COMM     (configMINIMAL_STACK_SIZE * 2U)
#define APP_TASK_STACK_PROTOCOL (configMINIMAL_STACK_SIZE * 2U)
#define APP_TASK_STACK_LOGGER   (configMINIMAL_STACK_SIZE * 3U)
#define APP_TASK_STACK_DISPLAY  (configMINIMAL_STACK_SIZE * 2U)
#define APP_TASK_STACK_ALARM    (configMINIMAL_STACK_SIZE * 2U)

static void App_SystemTask(void *argument);
static void App_CommTask(void *argument);
static void App_DisplayTask(void *argument);
static void App_AlarmTask(void *argument);

BaseType_t App_TasksInit(void)
{
  App_StatusInit();
  App_AlarmInit();

  if (App_RegistryInitAll() == APP_CHECK_ERROR) {
    return pdFAIL;
  }

  if (App_RegistrySelfCheckAll() == APP_CHECK_ERROR) {
    return pdFAIL;
  }

  if (xTaskCreate(App_SystemTask, "system", APP_TASK_STACK_SYSTEM, NULL, tskIDLE_PRIORITY + 3U, NULL) != pdPASS) {
    return pdFAIL;
  }

  if (xTaskCreate(App_CommTask, "comm", APP_TASK_STACK_COMM, NULL, tskIDLE_PRIORITY + 2U, NULL) != pdPASS) {
    return pdFAIL;
  }

  if (xTaskCreate(App_ProtocolTask, "protocol", APP_TASK_STACK_PROTOCOL, NULL, tskIDLE_PRIORITY + 2U, NULL) != pdPASS) {
    return pdFAIL;
  }

  if (xTaskCreate(App_LoggerTask, "logger", APP_TASK_STACK_LOGGER, NULL, tskIDLE_PRIORITY + 2U, NULL) != pdPASS) {
    return pdFAIL;
  }

  if (xTaskCreate(App_DisplayTask, "display", APP_TASK_STACK_DISPLAY, NULL, tskIDLE_PRIORITY + 1U, NULL) != pdPASS) {
    return pdFAIL;
  }

  if (xTaskCreate(App_AlarmTask, "alarm", APP_TASK_STACK_ALARM, NULL, tskIDLE_PRIORITY + 1U, NULL) != pdPASS) {
    return pdFAIL;
  }

  return pdPASS;
}

static void App_SystemTask(void *argument)
{
  App_LogRecord_t log_record;

  (void)argument;
  App_StatusSet(APP_MODULE_SYSTEM, APP_STATE_OK, 0U);

  memset(&log_record, 0, sizeof(log_record));
  log_record.level = APP_LOG_LEVEL_INFO;
  strncpy(log_record.module, "SYSTEM", sizeof(log_record.module));
  strncpy(log_record.field, "startup", sizeof(log_record.field));
  strncpy(log_record.value, "ok", sizeof(log_record.value));
  strncpy(log_record.unit, "-", sizeof(log_record.unit));

  for (;;) {
    log_record.timestamp_ms = xTaskGetTickCount();
    App_RegistryPoll(log_record.timestamp_ms);
    (void)App_LogWrite(&log_record);
    App_StatusHeartbeat(APP_MODULE_SYSTEM);
    vTaskDelay(pdMS_TO_TICKS(1000));
  }
}

static void App_CommTask(void *argument)
{
  (void)argument;
  App_StatusSet(APP_MODULE_COMM, APP_STATE_OK, 0U);

  for (;;) {
    App_StatusHeartbeat(APP_MODULE_COMM);
    vTaskDelay(pdMS_TO_TICKS(500));
  }
}

static void App_DisplayTask(void *argument)
{
  (void)argument;
  App_StatusSet(APP_MODULE_DISPLAY, APP_STATE_OK, 0U);

  for (;;) {
    App_StatusHeartbeat(APP_MODULE_DISPLAY);
    vTaskDelay(pdMS_TO_TICKS(200));
  }
}

static void App_AlarmTask(void *argument)
{
  (void)argument;
  App_StatusSet(APP_MODULE_ALARM, APP_STATE_OK, 0U);

  for (;;) {
    (void)App_AlarmGetActiveMask();
    App_StatusHeartbeat(APP_MODULE_ALARM);
    vTaskDelay(pdMS_TO_TICKS(200));
  }
}

void vApplicationMallocFailedHook(void)
{
  App_AlarmRaise(APP_ALARM_HEAP_FAILED, APP_MODULE_SYSTEM, 0U);
  taskDISABLE_INTERRUPTS();
  for (;;) {
  }
}

void vApplicationStackOverflowHook(TaskHandle_t task, char *task_name)
{
  (void)task;
  (void)task_name;
  App_AlarmRaise(APP_ALARM_STACK_OVERFLOW, APP_MODULE_SYSTEM, 0U);
  taskDISABLE_INTERRUPTS();
  for (;;) {
  }
}
