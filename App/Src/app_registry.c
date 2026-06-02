#include "app_registry.h"

#include "FreeRTOS.h"
#include "app_alarm.h"
#include "app_logger.h"
#include "app_storage.h"
#include "display.h"
#include "sensor_registry.h"

#define APP_REGISTRY_COUNT(array) (sizeof(array) / sizeof((array)[0]))

static App_CheckResult_t App_RegistryNoopInit(void);
static App_CheckResult_t App_RegistryOkSelfCheck(uint32_t *error_code);
static App_CheckResult_t App_RegistryAlarmSelfCheck(uint32_t *error_code);
static App_CheckResult_t App_RegistryStorageInit(void);
static App_CheckResult_t App_RegistryStorageSelfCheck(uint32_t *error_code);
static App_CheckResult_t App_RegistryLoggerInit(void);
static App_CheckResult_t App_RegistryDisplayInit(void);
static App_CheckResult_t App_RegistryDisplaySelfCheck(uint32_t *error_code);
static App_CheckResult_t App_RegistrySensorInit(void);
static App_CheckResult_t App_RegistrySensorSelfCheck(uint32_t *error_code);

static App_CheckResult_t App_RegistryRunInit(const App_ModuleRegistryEntry_t *entry);
static App_CheckResult_t App_RegistryRunSelfCheck(const App_ModuleRegistryEntry_t *entry);
static App_ModuleState_t App_RegistryStateFromCheck(App_CheckResult_t result);
static App_CheckResult_t App_RegistryWorst(App_CheckResult_t current, App_CheckResult_t next);
static App_CheckResult_t App_RegistryCheckFromDisplay(Display_Result_t result);
static App_CheckResult_t App_RegistryCheckFromSensor(Sensor_CheckResult_t result);

// 模块注册表
static const App_ModuleRegistryEntry_t s_module_registry[] =
  {
    {APP_MODULE_STORAGE,
     "storage",
     App_RegistryStorageInit,
     App_RegistryStorageSelfCheck,
     NULL,
     5000U},
    {APP_MODULE_LOGGER,
     "logger",
     App_RegistryLoggerInit,
     App_RegistryOkSelfCheck,
     NULL,
     5000U},
    {APP_MODULE_SENSOR,
     "sensor",
     App_RegistrySensorInit,
     App_RegistrySensorSelfCheck,
     NULL,
     1000U},
    {APP_MODULE_COMM,
     "comm",
     App_RegistryNoopInit,
     App_RegistryOkSelfCheck,
     NULL,
     1000U},
    {APP_MODULE_PROTOCOL,
     "protocol",
     App_RegistryNoopInit,
     App_RegistryOkSelfCheck,
     NULL,
     1000U},
    {APP_MODULE_DISPLAY,
     "display",
     App_RegistryDisplayInit,
     App_RegistryDisplaySelfCheck,
     NULL,
     1000U},
    {APP_MODULE_ALARM,
     "alarm",
     App_RegistryNoopInit,
     App_RegistryAlarmSelfCheck,
     NULL,
     1000U},
    {APP_MODULE_SYSTEM,
     "system",
     App_RegistryNoopInit,
     App_RegistryOkSelfCheck,
     NULL,
     1000U}};

static uint32_t s_last_check_ms[APP_REGISTRY_COUNT(s_module_registry)];

/**
 * @brief 初始化所有模块
 * @retval App_CheckResult_t
 */
App_CheckResult_t App_RegistryInitAll(void)
{
  App_CheckResult_t result = APP_CHECK_OK;
  size_t i;

  for (i = 0U; i < APP_REGISTRY_COUNT(s_module_registry); i++) {
    result             = App_RegistryWorst(result, App_RegistryRunInit(&s_module_registry[i]));
    s_last_check_ms[i] = 0U;
  }

  return result;
}

App_CheckResult_t App_RegistrySelfCheckAll(void)
{
  App_CheckResult_t result = APP_CHECK_OK;
  size_t i;

  for (i = 0U; i < APP_REGISTRY_COUNT(s_module_registry); i++) {
    result = App_RegistryWorst(result, App_RegistryRunSelfCheck(&s_module_registry[i]));
  }

  return result;
}

void App_RegistryPoll(uint32_t now_ms)
{
  size_t i;

  for (i = 0U; i < APP_REGISTRY_COUNT(s_module_registry); i++) {
    const App_ModuleRegistryEntry_t *entry = &s_module_registry[i];

    if (entry->service != NULL) {
      entry->service();
    }

    if ((entry->self_check != NULL) &&
        (entry->check_period_ms > 0U) &&
        ((now_ms - s_last_check_ms[i]) >= entry->check_period_ms)) {
      (void)App_RegistryRunSelfCheck(entry);
      s_last_check_ms[i] = now_ms;
    }
  }
}

size_t App_RegistryGetCount(void)
{
  return APP_REGISTRY_COUNT(s_module_registry);
}

const App_ModuleRegistryEntry_t *App_RegistryGetEntry(size_t index)
{
  if (index >= APP_REGISTRY_COUNT(s_module_registry)) {
    return NULL;
  }

  return &s_module_registry[index];
}

static App_CheckResult_t App_RegistryNoopInit(void)
{
  return APP_CHECK_OK;
}

static App_CheckResult_t App_RegistryOkSelfCheck(uint32_t *error_code)
{
  if (error_code != NULL) {
    *error_code = 0U;
  }

  return APP_CHECK_OK;
}

static App_CheckResult_t App_RegistryAlarmSelfCheck(uint32_t *error_code)
{
  uint32_t active_mask = App_AlarmGetActiveMask();

  if (error_code != NULL) {
    *error_code = active_mask;
  }

  if (active_mask != 0U) {
    return APP_CHECK_WARNING;
  }

  return APP_CHECK_OK;
}

static App_CheckResult_t App_RegistryStorageInit(void)
{
  App_StorageInit();
  return APP_CHECK_OK;
}

static App_CheckResult_t App_RegistryStorageSelfCheck(uint32_t *error_code)
{
  if (App_StorageFlush() != APP_STORAGE_OK) {
    if (error_code != NULL) {
      *error_code = 1U;
    }

    return APP_CHECK_NOT_READY;
  }

  if (error_code != NULL) {
    *error_code = 0U;
  }

  return APP_CHECK_OK;
}

static App_CheckResult_t App_RegistryLoggerInit(void)
{
  if (App_LoggerInit() != pdPASS) {
    return APP_CHECK_ERROR;
  }

  return APP_CHECK_OK;
}

static App_CheckResult_t App_RegistryDisplayInit(void)
{
  return App_RegistryCheckFromDisplay(Display_Init());
}

static App_CheckResult_t App_RegistryDisplaySelfCheck(uint32_t *error_code)
{
  return App_RegistryCheckFromDisplay(Display_SelfCheck(error_code));
}

static App_CheckResult_t App_RegistrySensorInit(void)
{
  return App_RegistryCheckFromSensor(Sensor_RegistryInitAll());
}

static App_CheckResult_t App_RegistrySensorSelfCheck(uint32_t *error_code)
{
  return App_RegistryCheckFromSensor(Sensor_RegistrySelfCheckAll(error_code));
}

static App_CheckResult_t App_RegistryRunInit(const App_ModuleRegistryEntry_t *entry)
{
  App_CheckResult_t result = APP_CHECK_OK;

  if ((entry != NULL) && (entry->init != NULL)) {
    result = entry->init();
  }

  if ((entry != NULL) && (result != APP_CHECK_OK)) {
    App_StatusSet(entry->id, App_RegistryStateFromCheck(result), (uint32_t)result);
    App_AlarmRaise(APP_ALARM_SELF_CHECK_FAILED, entry->id, (uint32_t)result);
  }

  return result;
}

static App_CheckResult_t App_RegistryRunSelfCheck(const App_ModuleRegistryEntry_t *entry)
{
  App_CheckResult_t result = APP_CHECK_OK;
  uint32_t error_code      = 0U;

  if ((entry != NULL) && (entry->self_check != NULL)) {
    result = entry->self_check(&error_code);
  }

  if (entry != NULL) {
    App_StatusSet(entry->id, App_RegistryStateFromCheck(result), error_code);

    if (result == APP_CHECK_ERROR) {
      App_AlarmRaise(APP_ALARM_SELF_CHECK_FAILED, entry->id, error_code);
    }
  }

  return result;
}

static App_ModuleState_t App_RegistryStateFromCheck(App_CheckResult_t result)
{
  switch (result) {
    case APP_CHECK_OK:
      return APP_STATE_OK;
    case APP_CHECK_WARNING:
      return APP_STATE_WARNING;
    case APP_CHECK_NOT_READY:
      return APP_STATE_OFFLINE;
    case APP_CHECK_ERROR:
    default:
      return APP_STATE_ERROR;
  }
}

static App_CheckResult_t App_RegistryWorst(App_CheckResult_t current, App_CheckResult_t next)
{
  if ((uint32_t)next > (uint32_t)current) {
    return next;
  }

  return current;
}

static App_CheckResult_t App_RegistryCheckFromDisplay(Display_Result_t result)
{
  switch (result) {
    case DISPLAY_OK:
      return APP_CHECK_OK;
    case DISPLAY_NOT_READY:
      return APP_CHECK_NOT_READY;
    case DISPLAY_ERROR:
    default:
      return APP_CHECK_ERROR;
  }
}

static App_CheckResult_t App_RegistryCheckFromSensor(Sensor_CheckResult_t result)
{
  switch (result) {
    case SENSOR_CHECK_OK:
      return APP_CHECK_OK;
    case SENSOR_CHECK_WARNING:
      return APP_CHECK_WARNING;
    case SENSOR_CHECK_NOT_READY:
      return APP_CHECK_NOT_READY;
    case SENSOR_CHECK_ERROR:
    default:
      return APP_CHECK_ERROR;
  }
}
