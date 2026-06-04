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
static void App_DisplayPostAlarm(App_AlarmOp_t op, App_ErrorCode_t code, App_AlarmSeverity_t severity, uint32_t detail);

/**
 * @brief 初始化应用层状态、告警和 FreeRTOS 任务。
 *
 * 该函数在调度器启动前调用，先完成状态模块和告警模块初始化，再通过系统注册表
 * 执行模块初始化和启动自检。自检通过后创建系统、通信、协议、日志、显示和告警任务。
 *
 * @retval pdPASS 应用任务创建完成。
 * @retval pdFAIL 模块初始化、自检或任务创建失败。
 */
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

  if (init_result == APP_CHECK_CRITICAL)
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

  if (self_check_result == APP_CHECK_CRITICAL)
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

/**
 * @brief 系统任务入口。
 *
 * 周期执行系统心跳、注册表轮询和启动状态日志写入。注册表轮询会驱动各模块
 * 的周期服务和周期自检，是应用层运行期健康检查的集中入口。
 *
 * @param argument FreeRTOS 任务参数，当前未使用。
 */
static void App_SystemTask(void *argument)
{
  App_LogRecord_t log_record;

  (void)argument;
  App_StatusSet(APP_MODULE_SYSTEM, APP_STATE_OK, ERR_OK);

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

/**
 * @brief 通信任务入口。
 *
 * 当前通信任务维护通信模块状态和心跳，后续 LoRa 链路收发调度可在该任务中扩展。
 *
 * @param argument FreeRTOS 任务参数，当前未使用。
 */
static void App_CommTask(void *argument)
{
  (void)argument;
  App_StatusSet(APP_MODULE_COMM, APP_STATE_OK, ERR_OK);

  /***************DEBUG***************/
  DBG_TRACE_TASK_STARTED("comm");
  /*************DEBUG END*************/

  for (;;)
  {
    App_StatusHeartbeat(APP_MODULE_COMM);
    vTaskDelay(pdMS_TO_TICKS(500));
  }
}

/**
 * @brief 显示任务入口。
 *
 * 周期刷新显示模块，并根据刷新结果更新显示模块状态。显示异常时投递 HMI 离线告警。
 *
 * @param argument FreeRTOS 任务参数，当前未使用。
 */
static void App_DisplayTask(void *argument)
{
  Display_Result_t result;

  (void)argument;
  App_StatusSet(APP_MODULE_DISPLAY, APP_STATE_OK, ERR_OK);

  /***************DEBUG***************/
  DBG_TRACE_TASK_STARTED("display");
  /*************DEBUG END*************/

  for (;;)
  {
    result = Display_Refresh(xTaskGetTickCount());

    if (result == DISPLAY_OK)
    {
      App_StatusSet(APP_MODULE_DISPLAY, APP_STATE_OK, ERR_OK);
      App_StatusHeartbeat(APP_MODULE_DISPLAY);
      App_DisplayPostAlarm(APP_ALARM_OP_CLEAR, ERR_DISPLAY_OFFLINE, APP_ALARM_SEVERITY_NORMAL, 0U);
      App_DisplayPostAlarm(APP_ALARM_OP_CLEAR, ERR_DISPLAY_REFRESH_FAILED, APP_ALARM_SEVERITY_NORMAL, 0U);
    }
    else if (result == DISPLAY_NOT_READY)
    {
      App_StatusSet(APP_MODULE_DISPLAY, APP_STATE_OFFLINE, ERR_DISPLAY_OFFLINE);
      App_DisplayPostAlarm(APP_ALARM_OP_RAISE, ERR_DISPLAY_OFFLINE, APP_ALARM_SEVERITY_GENERAL, (uint32_t)result);
    }
    else
    {
      App_StatusSet(APP_MODULE_DISPLAY, APP_STATE_ERROR, ERR_DISPLAY_REFRESH_FAILED);
      App_DisplayPostAlarm(APP_ALARM_OP_RAISE, ERR_DISPLAY_REFRESH_FAILED, APP_ALARM_SEVERITY_IMPORTANT, (uint32_t)result);
    }

    vTaskDelay(pdMS_TO_TICKS(200));
  }
}

/**
 * @brief 投递显示模块异常状态变化消息。
 */
static void App_DisplayPostAlarm(App_AlarmOp_t op, App_ErrorCode_t code, App_AlarmSeverity_t severity, uint32_t detail)
{
  App_AlarmMsg_t msg;
  App_AlarmPayload_t payload;
  App_AlarmPayloadType_t payload_type = APP_ALARM_PAYLOAD_NONE;
  App_AlarmPayload_t *payload_ptr = NULL;

  (void)memset(&payload, 0, sizeof(payload));
  if (op == APP_ALARM_OP_RAISE)
  {
    payload.raw_detail = detail;
    payload_type = APP_ALARM_PAYLOAD_RAW;
    payload_ptr = &payload;
  }

  if (App_AlarmBuildMsg(&msg, op, APP_MODULE_DISPLAY, severity, code, 0U, payload_type, payload_ptr) == APP_ALARM_RESULT_OK)
  {
    (void)App_AlarmPost(&msg, 0U);
  }
}
