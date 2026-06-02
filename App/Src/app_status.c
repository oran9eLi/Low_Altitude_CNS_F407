#include "app_status.h"
#include "FreeRTOS.h"
#include "task.h"

static App_ModuleStatus_t s_module_status[APP_MODULE_COUNT];

void App_StatusInit(void)
{
  uint32_t i;

  for (i = 0; i < APP_MODULE_COUNT; i++) {
    s_module_status[i].state        = APP_STATE_INIT;
    s_module_status[i].heartbeat    = 0U;
    s_module_status[i].last_tick_ms = 0U;
    s_module_status[i].error_code   = 0U;
  }
}

void App_StatusSet(App_ModuleId_t module, App_ModuleState_t state, uint32_t error_code)
{
  if (module >= APP_MODULE_COUNT) {
    return;
  }

  s_module_status[module].state        = state;
  s_module_status[module].error_code   = error_code;
  s_module_status[module].last_tick_ms = xTaskGetTickCount();
}

void App_StatusHeartbeat(App_ModuleId_t module)
{
  if (module >= APP_MODULE_COUNT) {
    return;
  }

  s_module_status[module].heartbeat++;
  s_module_status[module].last_tick_ms = xTaskGetTickCount();
}

App_ModuleStatus_t App_StatusGet(App_ModuleId_t module)
{
  App_ModuleStatus_t empty_status = {APP_STATE_ERROR, 0U, 0U, 1U};

  if (module >= APP_MODULE_COUNT) {
    return empty_status;
  }

  return s_module_status[module];
}
