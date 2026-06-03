#ifndef APP_STATUS_H
#define APP_STATUS_H

#include <stdint.h>
#include "app_error.h"

typedef enum {
  APP_MODULE_SYSTEM = 0,
  APP_MODULE_COMM,
  APP_MODULE_PROTOCOL,
  APP_MODULE_LOGGER,
  APP_MODULE_DISPLAY,
  APP_MODULE_ALARM,
  APP_MODULE_STORAGE,
  APP_MODULE_SENSOR,
  APP_MODULE_COUNT
} App_ModuleId_t;

typedef enum {
  APP_STATE_INIT = 0,
  APP_STATE_OK,
  APP_STATE_WARNING,
  APP_STATE_ERROR,
  APP_STATE_TIMEOUT,
  APP_STATE_OFFLINE
} App_ModuleState_t;

typedef struct
{
  App_ModuleState_t state;
  uint32_t heartbeat;
  uint32_t last_tick_ms;
  App_ErrorCode_t error_code;
} App_ModuleStatus_t;

void App_StatusInit(void);
void App_StatusSet(App_ModuleId_t module, App_ModuleState_t state, App_ErrorCode_t error_code);
void App_StatusHeartbeat(App_ModuleId_t module);
App_ModuleStatus_t App_StatusGet(App_ModuleId_t module);

#endif
