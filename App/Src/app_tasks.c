#include "app_tasks.h"
#include "app_alarm.h"
#include "app_logger.h"
#include "app_protocol.h"
#include "app_registry.h"
#include "app_status.h"
#include "display.h"
#include "task.h"
#include "debug_trace.h"
#include <string.h>

#define APP_TASK_STACK_SYSTEM (configMINIMAL_STACK_SIZE * 2U)
#define APP_TASK_STACK_COMM (configMINIMAL_STACK_SIZE * 2U)
#define APP_TASK_STACK_PROTOCOL (configMINIMAL_STACK_SIZE * 2U)
#define APP_TASK_STACK_LOGGER (configMINIMAL_STACK_SIZE * 3U)
#define APP_TASK_STACK_DISPLAY (configMINIMAL_STACK_SIZE * 2U)
#define APP_TASK_STACK_ALARM (configMINIMAL_STACK_SIZE * 2U)

static void App_SystemTask(void *argument);
static void App_CommTask(void *argument);
static void App_DisplayTask(void *argument);

BaseType_t App_TasksInit(void)
{
  App_CheckResult_t init_result;
  App_CheckResult_t self_check_result;

  App_StatusInit();
  App_AlarmInit();

  /***************DEBUG***************/
  DBG_TRACE_POST_INIT_START();
  /*************DEBUG END*************/

  init_result = App_RegistryInitAll();
  (void)App_AlarmProcessPending(0U);

  /***************DEBUG***************/
  DBG_TRACE_POST_INIT_DONE(init_result);
  /*************DEBUG END*************/

  if (init_result == APP_CHECK_ERROR)
  {
    return pdFAIL;
  }

  /***************DEBUG***************/
  DBG_TRACE_POST_SELF_CHECK_START();
  /*************DEBUG END*************/

  self_check_result = App_RegistrySelfCheckAll();
  (void)App_AlarmProcessPending(0U);

  /***************DEBUG***************/
  DBG_TRACE_POST_SELF_CHECK_DONE(self_check_result);
  /*************DEBUG END*************/

  if (self_check_result == APP_CHECK_ERROR)
  {
    return pdFAIL;
  }

  /***************DEBUG***************/
  DBG_TRACE_TASKS_READY();
  /*************DEBUG END*************/

  if (xTaskCreate(App_SystemTask, "system", APP_TASK_STACK_SYSTEM, NULL, tskIDLE_PRIORITY + 3U, NULL) != pdPASS)
  {
    return pdFAIL;
  }

  if (xTaskCreate(App_CommTask, "comm", APP_TASK_STACK_COMM, NULL, tskIDLE_PRIORITY + 2U, NULL) != pdPASS)
  {
    return pdFAIL;
  }

  if (xTaskCreate(App_ProtocolTask, "protocol", APP_TASK_STACK_PROTOCOL, NULL, tskIDLE_PRIORITY + 2U, NULL) != pdPASS)
  {
    return pdFAIL;
  }

  if (xTaskCreate(App_LoggerTask, "logger", APP_TASK_STACK_LOGGER, NULL, tskIDLE_PRIORITY + 2U, NULL) != pdPASS)
  {
    return pdFAIL;
  }

  if (xTaskCreate(App_DisplayTask, "display", APP_TASK_STACK_DISPLAY, NULL, tskIDLE_PRIORITY + 1U, NULL) != pdPASS)
  {
    return pdFAIL;
  }

  if (xTaskCreate(App_AlarmTask, "alarm", APP_TASK_STACK_ALARM, NULL, tskIDLE_PRIORITY + 1U, NULL) != pdPASS)
  {
    return pdFAIL;
  }

  return pdPASS;
}

static void App_SystemTask(void *argument)
{
  App_LogRecord_t log_record;

  (void)argument;
  App_StatusSet(APP_MODULE_SYSTEM, APP_STATE_OK, APP_ERROR_OK);

  /***************DEBUG***************/
  DBG_TRACE_TASK_STARTED("system");
  /*************DEBUG END*************/

  memset(&log_record, 0, sizeof(log_record));
  log_record.level = APP_LOG_LEVEL_INFO;
  strncpy(log_record.module, "SYSTEM", sizeof(log_record.module));
  strncpy(log_record.field, "startup", sizeof(log_record.field));
  strncpy(log_record.value, "ok", sizeof(log_record.value));
  strncpy(log_record.unit, "-", sizeof(log_record.unit));

  for (;;)
  {
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
  App_StatusSet(APP_MODULE_COMM, APP_STATE_OK, APP_ERROR_OK);

  /***************DEBUG***************/
  DBG_TRACE_TASK_STARTED("comm");
  /*************DEBUG END*************/

  for (;;)
  {
    App_StatusHeartbeat(APP_MODULE_COMM);
    vTaskDelay(pdMS_TO_TICKS(500));
  }
}

static void App_DisplayTask(void *argument)
{
  Display_Result_t result;

  (void)argument;
  App_StatusSet(APP_MODULE_DISPLAY, APP_STATE_OK, APP_ERROR_OK);

  /***************DEBUG***************/
  DBG_TRACE_TASK_STARTED("display");
  /*************DEBUG END*************/

  for (;;)
  {
    result = Display_Refresh(xTaskGetTickCount());

    if (result == DISPLAY_OK)
    {
      App_StatusSet(APP_MODULE_DISPLAY, APP_STATE_OK, APP_ERROR_OK);
      App_StatusHeartbeat(APP_MODULE_DISPLAY);
    }
    else if (result == DISPLAY_NOT_READY)
    {
      App_StatusSet(APP_MODULE_DISPLAY, APP_STATE_OFFLINE, APP_ERROR_HMI_OFFLINE);
    }
    else
    {
      App_StatusSet(APP_MODULE_DISPLAY, APP_STATE_ERROR, APP_ERROR_HMI_OFFLINE);
      (void)App_AlarmRaise(APP_ALARM_HMI_OFFLINE,
                           APP_MODULE_DISPLAY,
                           APP_ERROR_HMI_OFFLINE,
                           (uint32_t)result);
    }

    vTaskDelay(pdMS_TO_TICKS(200));
  }
}

void vApplicationMallocFailedHook(void)
{
  (void)App_AlarmRaiseImmediate(APP_ALARM_HEAP_FAILED, APP_MODULE_SYSTEM, APP_ERROR_HEAP_FAILED, 0U);
  taskDISABLE_INTERRUPTS();
  for (;;)
  {
  }
}

void vApplicationStackOverflowHook(TaskHandle_t task, char *task_name)
{
  (void)task;
  (void)task_name;
  (void)App_AlarmRaiseImmediate(APP_ALARM_STACK_OVERFLOW, APP_MODULE_SYSTEM, APP_ERROR_STACK_OVERFLOW, 0U);
  taskDISABLE_INTERRUPTS();
  for (;;)
  {
  }
}
