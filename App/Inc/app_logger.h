#ifndef APP_LOGGER_H
#define APP_LOGGER_H

#include <stdint.h>
#include "FreeRTOS.h"

#define APP_LOG_FIELD_LEN    16U
#define APP_LOG_VALUE_LEN    24U
#define APP_LOG_LINE_MAX_LEN 96U

typedef enum {
  APP_LOG_LEVEL_INFO = 0,
  APP_LOG_LEVEL_WARN,
  APP_LOG_LEVEL_ERROR
} App_LogLevel_t;

typedef struct
{
  uint32_t timestamp_ms;
  App_LogLevel_t level;
  char module[APP_LOG_FIELD_LEN];
  char field[APP_LOG_FIELD_LEN];
  char value[APP_LOG_VALUE_LEN];
  char unit[APP_LOG_FIELD_LEN];
  uint32_t status;
} App_LogRecord_t;

BaseType_t App_LoggerInit(void);
BaseType_t App_LogWrite(const App_LogRecord_t *record);
void App_LoggerTask(void *argument);

#endif
