#include <stdint.h>
#include <stdio.h>

#include "app_registry.h"
#include "app_alarm.h"
#include "app_logger.h"
#include "app_storage.h"
#include "display.h"
#include "sensor_registry.h"

static App_AlarmId_t s_last_alarm_id;
static App_ModuleId_t s_last_alarm_source;
static uint32_t s_last_alarm_detail;
static uint8_t s_default_alarm_called;
static uint8_t s_explicit_alarm_called;

static int expect_true(int condition, const char *message)
{
  if (!condition)
  {
    (void)printf("FAIL: %s\n", message);
    return 1;
  }

  return 0;
}

void App_StatusSet(App_ModuleId_t module, App_ModuleState_t state, App_ErrorCode_t error_code)
{
  (void)module;
  (void)state;
  (void)error_code;
}

App_AlarmResult_t App_AlarmRaise(App_AlarmId_t alarm_id, App_ModuleId_t source, App_ErrorCode_t error_code, uint32_t detail)
{
  s_last_alarm_id         = alarm_id;
  s_last_alarm_source     = source;
  s_last_alarm_detail     = detail;
  s_explicit_alarm_called = 1U;
  (void)error_code;
  return APP_ALARM_RESULT_OK;
}

App_AlarmResult_t App_AlarmRaiseDefault(App_AlarmId_t alarm_id, App_ModuleId_t source, uint32_t detail)
{
  s_last_alarm_id        = alarm_id;
  s_last_alarm_source    = source;
  s_last_alarm_detail    = detail;
  s_default_alarm_called = 1U;
  return APP_ALARM_RESULT_OK;
}

uint32_t App_AlarmGetActiveMask(void)
{
  return 0U;
}

App_ErrorCode_t App_AlarmGetCurrentError(void)
{
  return APP_ERROR_OK;
}

void App_StorageInit(void)
{
}

App_StorageResult_t App_StorageWriteCsvLine(const char *line, size_t length)
{
  (void)line;
  (void)length;
  return APP_STORAGE_OK;
}

App_StorageResult_t App_StorageFlush(void)
{
  return APP_STORAGE_OK;
}

BaseType_t App_LoggerInit(void)
{
  return pdFAIL;
}

BaseType_t App_LogWrite(const App_LogRecord_t *record)
{
  (void)record;
  return pdPASS;
}

void App_LoggerTask(void *argument)
{
  (void)argument;
}

Display_Result_t Display_Init(void)
{
  return DISPLAY_OK;
}

Display_Result_t Display_SelfCheck(App_ErrorCode_t *error_code)
{
  if (error_code != 0)
  {
    *error_code = APP_ERROR_OK;
  }

  return DISPLAY_OK;
}

Display_Result_t Display_Refresh(uint32_t now_ms)
{
  (void)now_ms;
  return DISPLAY_OK;
}

Sensor_CheckResult_t Sensor_RegistryInitAll(void)
{
  return SENSOR_CHECK_OK;
}

Sensor_CheckResult_t Sensor_RegistrySelfCheckAll(uint32_t *error_code)
{
  if (error_code != 0)
  {
    *error_code = 0U;
  }

  return SENSOR_CHECK_OK;
}

Sensor_CheckResult_t Sensor_RegistrySelfCheckAbnormal(Sensor_CheckStatus_t *status_list, uint16_t max_count, uint16_t *out_count)
{
  (void)status_list;
  (void)max_count;

  if (out_count != 0)
  {
    *out_count = 0U;
  }

  return SENSOR_CHECK_OK;
}

Sensor_CheckResult_t Sensor_RegistryReadAll(Sensor_Sample_t *samples, uint16_t max_count, uint16_t *out_count)
{
  (void)samples;
  (void)max_count;

  if (out_count != 0)
  {
    *out_count = 0U;
  }

  return SENSOR_CHECK_OK;
}

uint16_t Sensor_RegistryGetCount(void)
{
  return 0U;
}

const Sensor_Driver_t *Sensor_RegistryGetDriver(uint16_t index)
{
  (void)index;
  return 0;
}

int main(void)
{
  int failures = 0;

  failures += expect_true(App_RegistryInitAll() == APP_CHECK_ERROR,
                          "logger init failure is reported");
  failures += expect_true(s_last_alarm_id == APP_ALARM_LOG_STORAGE_FAILED,
                          "logger init failure raises logger storage alarm");
  failures += expect_true(s_last_alarm_source == APP_MODULE_LOGGER,
                          "alarm source is logger");
  failures += expect_true(s_default_alarm_called != 0U,
                          "registry uses default alarm API without explicit error code");
  failures += expect_true(s_explicit_alarm_called == 0U,
                          "registry does not pass OK as explicit alarm error code");
  failures += expect_true(s_last_alarm_detail == (uint32_t)APP_CHECK_ERROR,
                          "alarm detail carries check result");

  if (failures != 0)
  {
    (void)printf("%d failure(s)\n", failures);
    return 1;
  }

  (void)printf("app registry alarm boundary tests passed\n");
  return 0;
}
