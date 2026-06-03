#include "app_logger.h"
#include <stdio.h>
#include <string.h>
#include "queue.h"
#include "task.h"
#include "app_alarm.h"
#include "app_status.h"
#include "app_storage.h"

#define APP_LOG_QUEUE_LENGTH 16U

static QueueHandle_t s_log_queue;

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
  App_StatusSet(APP_MODULE_LOGGER, APP_STATE_OK, APP_ERROR_OK);

  for (;;) {
    if (xQueueReceive(s_log_queue, &record, pdMS_TO_TICKS(1000)) == pdPASS) {
      len = snprintf(line,
                     sizeof(line),
                     "%lu,%s,%s,%s,%s,%s,%lu\r\n",
                     (unsigned long)record.timestamp_ms,
                     App_LogLevelToString(record.level),
                     record.module,
                     record.field,
                     record.value,
                     record.unit,
                     (unsigned long)record.status);
      if ((len <= 0) || ((size_t)len >= sizeof(line)) ||
          (App_StorageWriteCsvLine(line, (size_t)len) != APP_STORAGE_OK)) {
        (void)App_AlarmRaise(APP_ALARM_LOG_STORAGE_FAILED,
                             APP_MODULE_LOGGER,
                             APP_ERROR_SD_MOUNT,
                             1U);
      }
    }

    App_StatusHeartbeat(APP_MODULE_LOGGER);
  }
}
