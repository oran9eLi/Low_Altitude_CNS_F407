#include "app_logger.h"
#include <stdio.h>
#include <string.h>
#include "queue.h"
#include "task.h"
#include "app_alarm.h"
#include "app_status.h"
#include "app_storage.h"
/***************DEBUG***************/
#include "debug_trace.h"
/*************DEBUG END*************/

#define APP_LOG_QUEUE_LENGTH 16U

static QueueHandle_t s_log_queue;

static void App_LoggerPostAlarm(App_AlarmOp_t op, App_ErrorCode_t code, uint32_t detail);

static const char *App_LogLevelToString(App_LogLevel_t level)
{
  switch (level) {
    case APP_LOG_LEVEL_INFO:
      return "INFO";
    case APP_LOG_LEVEL_WARN:
      return "WARN";
    case APP_LOG_LEVEL_ERROR:
      return "ERROR";
    default:
      return "UNKNOWN";
  }
}

BaseType_t App_LoggerInit(void)
{
  s_log_queue = xQueueCreate(APP_LOG_QUEUE_LENGTH, sizeof(App_LogRecord_t));
  if (s_log_queue == NULL) {
    return pdFAIL;
  }

  App_StorageInit();
  return pdPASS;
}

BaseType_t App_LogWrite(const App_LogRecord_t *record)
{
  if ((s_log_queue == NULL) || (record == NULL)) {
    return pdFAIL;
  }

  return xQueueSend(s_log_queue, record, 0U);
}

void App_LoggerTask(void *argument)
{
  App_LogRecord_t record;
  char line[APP_LOG_LINE_MAX_LEN];
  int len;

  (void)argument;
  App_StatusSet(APP_MODULE_LOGGER, APP_STATE_OK, ERR_OK);

  /***************DEBUG***************/
  DBG_TRACE_TASK_STARTED("logger");
  /*************DEBUG END*************/

  for (;;) {
    if (xQueueReceive(s_log_queue, &record, pdMS_TO_TICKS(1000)) == pdPASS) {
      len = snprintf(line, sizeof(line), "%lu,%s,%s,%s,%s,%s,%lu\r\n", (unsigned long)record.timestamp_ms, App_LogLevelToString(record.level), record.module, record.field, record.value, record.unit, (unsigned long)record.status);
      if ((len <= 0) || ((size_t)len >= sizeof(line)) ||
          (App_StorageWriteCsvLine(line, (size_t)len) != APP_STORAGE_OK)) {
        App_StatusSet(APP_MODULE_LOGGER, APP_STATE_ERROR, ERR_LOG_WRITE_FAILED);
        App_LoggerPostAlarm(APP_ALARM_OP_RAISE, ERR_LOG_WRITE_FAILED, 1U);
      } else {
        App_StatusSet(APP_MODULE_LOGGER, APP_STATE_OK, ERR_OK);
        App_LoggerPostAlarm(APP_ALARM_OP_CLEAR, ERR_LOG_WRITE_FAILED, 0U);
      }
    }

    App_StatusHeartbeat(APP_MODULE_LOGGER);
  }
}

/**
 * @brief 投递日志模块异常状态变化消息。
 */
static void App_LoggerPostAlarm(App_AlarmOp_t op, App_ErrorCode_t code, uint32_t detail)
{
  App_AlarmMsg_t msg;
  App_AlarmPayload_t payload;
  App_AlarmPayloadType_t payload_type = APP_ALARM_PAYLOAD_NONE;
  App_AlarmPayload_t *payload_ptr = NULL;

  (void)memset(&payload, 0, sizeof(payload));
  if (op == APP_ALARM_OP_RAISE)
  {
    payload.storage.target_id = 0U;
    payload.storage.operation = 1U;
    payload.storage.detail = detail;
    payload_type = APP_ALARM_PAYLOAD_STORAGE;
    payload_ptr = &payload;
  }

  if (App_AlarmBuildMsg(&msg, op, APP_MODULE_LOGGER, (op == APP_ALARM_OP_RAISE) ? APP_ALARM_SEVERITY_IMPORTANT : APP_ALARM_SEVERITY_NORMAL, code, 0U, payload_type, payload_ptr) == APP_ALARM_RESULT_OK)
  {
    (void)App_AlarmPost(&msg, 0U);
  }
}
