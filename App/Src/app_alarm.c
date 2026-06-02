#include "app_alarm.h"
#include "app_status.h"

static uint32_t s_alarm_mask;

void App_AlarmInit(void)
{
  s_alarm_mask = 0U;
}

void App_AlarmRaise(App_AlarmCode_t code, App_ModuleId_t source, uint32_t detail)
{
  if ((code <= APP_ALARM_NONE) || (code >= 32)) {
    return;
  }

  s_alarm_mask |= (1UL << (uint32_t)code);
  App_StatusSet(source, APP_STATE_ERROR, detail);
}

void App_AlarmClear(App_AlarmCode_t code)
{
  if ((code <= APP_ALARM_NONE) || (code >= 32)) {
    return;
  }

  s_alarm_mask &= ~(1UL << (uint32_t)code);
}

uint32_t App_AlarmGetActiveMask(void)
{
  return s_alarm_mask;
}
