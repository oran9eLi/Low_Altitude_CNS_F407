#include "app_registry.h"

#include "FreeRTOS.h"
#include "app_alarm.h"
#include "app_logger.h"
#include "app_storage.h"
#include "display.h"
#include "sensor_registry.h"
#include <string.h>

#define APP_REGISTRY_COUNT(array) (sizeof(array) / sizeof((array)[0]))
#define APP_REGISTRY_SENSOR_STATUS_MAX 4U
#define APP_REGISTRY_SENSOR_ACTIVE_MAX 8U

typedef struct
{
  App_ErrorCode_t code;
  uint16_t instance_id;
} App_RegistryAlarmKey_t;

static App_CheckResult_t App_RegistryNoopInit(void);
static App_CheckResult_t App_RegistryOkSelfCheck(App_ErrorCode_t *code);
static App_CheckResult_t App_RegistryAlarmSelfCheck(App_ErrorCode_t *code);
static App_CheckResult_t App_RegistryStorageInit(void);
static App_CheckResult_t App_RegistryStorageSelfCheck(App_ErrorCode_t *code);
static App_CheckResult_t App_RegistryLoggerInit(void);
static App_CheckResult_t App_RegistryDisplayInit(void);
static App_CheckResult_t App_RegistryDisplaySelfCheck(App_ErrorCode_t *code);
static App_CheckResult_t App_RegistrySensorInit(void);
static App_CheckResult_t App_RegistrySensorSelfCheck(App_ErrorCode_t *code);

static App_CheckResult_t App_RegistryRunInit(const App_ModuleRegistryEntry_t *entry);
static App_CheckResult_t App_RegistryRunSelfCheck(const App_ModuleRegistryEntry_t *entry);
static App_ModuleState_t App_RegistryStateFromCheck(App_CheckResult_t result);
static App_AlarmSeverity_t App_RegistrySeverityFromCheck(App_CheckResult_t result);
static App_CheckResult_t App_RegistryWorst(App_CheckResult_t current, App_CheckResult_t next);
static App_CheckResult_t App_RegistryCheckFromAlarmSeverity(App_AlarmSeverity_t severity);
static App_CheckResult_t App_RegistryCheckFromDisplay(Display_Result_t result);
static App_CheckResult_t App_RegistryCheckFromSensor(Sensor_Severity_t severity);
static App_ErrorCode_t App_RegistryDefaultCodeFromModule(App_ModuleId_t module);
static App_ErrorCode_t App_RegistryNormalizeSensorCode(const Sensor_Status_t *status);
static const char *App_RegistryFindSensorName(uint16_t device_id);
static void App_RegistryPostModuleAlarm(App_ModuleId_t module, App_AlarmOp_t op, App_AlarmSeverity_t severity, App_ErrorCode_t code, uint32_t detail);
static void App_RegistryPostSensorAlarm(const Sensor_Status_t *status, App_ErrorCode_t code);
static void App_RegistryClearRecoveredSensorAlarms(const App_RegistryAlarmKey_t *current_keys, uint16_t current_count);
static uint8_t App_RegistryKeyExists(const App_RegistryAlarmKey_t *keys, uint16_t key_count, App_ErrorCode_t code, uint16_t instance_id);

/**
 * @brief 应用模块静态注册表。
 *
 * 注册表集中描述各模块初始化、自检、周期服务入口和自检周期。应用层通过该表
 * 统一完成启动初始化、运行期巡检、状态汇总和异常上报。
 */
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

/**
 * @brief 各模块上一次执行周期自检的时间戳。
 */
static uint32_t s_last_check_ms[APP_REGISTRY_COUNT(s_module_registry)];

/**
 * @brief 初始化注册表中的所有模块。
 */
App_CheckResult_t App_RegistryInitAll(void)
{
  App_CheckResult_t result = APP_CHECK_NORMAL;
  size_t i;

  for (i = 0U; i < APP_REGISTRY_COUNT(s_module_registry); i++)
  {
    result = App_RegistryWorst(result, App_RegistryRunInit(&s_module_registry[i]));
    s_last_check_ms[i] = 0U;
  }

  return result;
}

/**
 * @brief 执行注册表中所有模块的自检。
 */
App_CheckResult_t App_RegistrySelfCheckAll(void)
{
  App_CheckResult_t result = APP_CHECK_NORMAL;
  size_t i;

  for (i = 0U; i < APP_REGISTRY_COUNT(s_module_registry); i++)
  {
    result = App_RegistryWorst(result, App_RegistryRunSelfCheck(&s_module_registry[i]));
  }

  return result;
}

/**
 * @brief 执行模块周期服务和周期自检。
 *
 * @param now_ms 当前系统运行时间，单位为毫秒。
 */
void App_RegistryPoll(uint32_t now_ms)
{
  size_t i;

  for (i = 0U; i < APP_REGISTRY_COUNT(s_module_registry); i++)
  {
    const App_ModuleRegistryEntry_t *entry = &s_module_registry[i];

    if (entry->service != NULL)
    {
      entry->service();
    }

    if ((entry->self_check != NULL) && (entry->check_period_ms > 0U) && ((now_ms - s_last_check_ms[i]) >= entry->check_period_ms))
    {
      (void)App_RegistryRunSelfCheck(entry);
      s_last_check_ms[i] = now_ms;
    }
  }
}

/**
 * @brief 获取注册表中的模块数量。
 */
size_t App_RegistryGetCount(void)
{
  return APP_REGISTRY_COUNT(s_module_registry);
}

/**
 * @brief 按索引获取模块注册项。
 */
const App_ModuleRegistryEntry_t *App_RegistryGetEntry(size_t index)
{
  if (index >= APP_REGISTRY_COUNT(s_module_registry))
  {
    return NULL;
  }

  return &s_module_registry[index];
}

/**
 * @brief 空初始化入口。
 */
static App_CheckResult_t App_RegistryNoopInit(void)
{
  return APP_CHECK_NORMAL;
}

/**
 * @brief 默认正常自检入口。
 */
static App_CheckResult_t App_RegistryOkSelfCheck(App_ErrorCode_t *code)
{
  if (code != NULL)
  {
    *code = ERR_OK;
  }

  return APP_CHECK_NORMAL;
}

/**
 * @brief 告警模块自检入口。
 */
static App_CheckResult_t App_RegistryAlarmSelfCheck(App_ErrorCode_t *code)
{
  App_AlarmSeverity_t severity = App_AlarmGetCurrentSeverity();

  if (code != NULL)
  {
    *code = App_AlarmGetCurrentCode();
  }

  return App_RegistryCheckFromAlarmSeverity(severity);
}

/**
 * @brief 存储模块初始化入口。
 */
static App_CheckResult_t App_RegistryStorageInit(void)
{
  App_StorageInit();
  return APP_CHECK_NORMAL;
}

/**
 * @brief 存储模块自检入口。
 */
static App_CheckResult_t App_RegistryStorageSelfCheck(App_ErrorCode_t *code)
{
  if (App_StorageFlush() != APP_STORAGE_OK)
  {
    if (code != NULL)
    {
      *code = ERR_STORAGE_NOT_READY;
    }

    return APP_CHECK_IMPORTANT;
  }

  if (code != NULL)
  {
    *code = ERR_OK;
  }

  return APP_CHECK_NORMAL;
}

/**
 * @brief 日志模块初始化入口。
 */
static App_CheckResult_t App_RegistryLoggerInit(void)
{
  if (App_LoggerInit() != pdPASS)
  {
    return APP_CHECK_CRITICAL;
  }

  return APP_CHECK_NORMAL;
}

/**
 * @brief 显示模块初始化入口。
 */
static App_CheckResult_t App_RegistryDisplayInit(void)
{
  return App_RegistryCheckFromDisplay(Display_Init());
}

/**
 * @brief 显示模块自检入口。
 */
static App_CheckResult_t App_RegistryDisplaySelfCheck(App_ErrorCode_t *code)
{
  return App_RegistryCheckFromDisplay(Display_SelfCheck(code));
}

/**
 * @brief 传感器模块初始化入口。
 */
static App_CheckResult_t App_RegistrySensorInit(void)
{
  return App_RegistryCheckFromSensor(Sensor_RegistryInitAll());
}

/**
 * @brief 传感器模块自检入口。
 *
 * 本函数负责把传感器当前异常状态转换为统一异常消息，并根据本轮异常 key
 * 与活动异常表的差异投递恢复 CLEAR 消息。
 */
static App_CheckResult_t App_RegistrySensorSelfCheck(App_ErrorCode_t *code)
{
  Sensor_Status_t status_list[APP_REGISTRY_SENSOR_STATUS_MAX];
  App_RegistryAlarmKey_t current_keys[APP_REGISTRY_SENSOR_STATUS_MAX];
  Sensor_Severity_t sensor_severity;
  Sensor_Severity_t highest_severity = SENSOR_SEVERITY_NORMAL;
  App_ErrorCode_t current_code = ERR_OK;
  uint16_t status_count = 0U;
  uint16_t current_count = 0U;
  uint16_t i;

  sensor_severity = Sensor_RegistrySelfCheckAbnormal(status_list, APP_REGISTRY_SENSOR_STATUS_MAX, &status_count);

  for (i = 0U; (i < status_count) && (i < APP_REGISTRY_SENSOR_STATUS_MAX); i++)
  {
    const Sensor_Status_t *status = &status_list[i];
    App_ErrorCode_t normalized_code = App_RegistryNormalizeSensorCode(status);

    App_RegistryPostSensorAlarm(status, normalized_code);

    current_keys[current_count].code = normalized_code;
    current_keys[current_count].instance_id = status->device_id;
    current_count++;

    if (status->severity >= highest_severity)
    {
      highest_severity = status->severity;
      current_code = normalized_code;
    }
  }

  App_RegistryClearRecoveredSensorAlarms(current_keys, current_count);

  if ((status_count == 0U) && (sensor_severity != SENSOR_SEVERITY_NORMAL))
  {
    current_code = ERR_SENSOR_DATA_INVALID;
  }

  if (code != NULL)
  {
    *code = current_code;
  }

  return App_RegistryCheckFromSensor(sensor_severity);
}

/**
 * @brief 执行单个模块初始化并同步异常状态。
 */
static App_CheckResult_t App_RegistryRunInit(const App_ModuleRegistryEntry_t *entry)
{
  App_CheckResult_t result = APP_CHECK_NORMAL;

  if ((entry != NULL) && (entry->init != NULL))
  {
    result = entry->init();
  }

  if (entry != NULL)
  {
    App_ErrorCode_t code = (result == APP_CHECK_NORMAL) ? ERR_OK : App_RegistryDefaultCodeFromModule(entry->id);

    App_StatusSet(entry->id, App_RegistryStateFromCheck(result), code);
    if (result != APP_CHECK_NORMAL)
    {
      App_RegistryPostModuleAlarm(entry->id, APP_ALARM_OP_RAISE, App_RegistrySeverityFromCheck(result), code, (uint32_t)result);
    }
  }

  return result;
}

/**
 * @brief 执行单个模块自检并同步状态。
 */
static App_CheckResult_t App_RegistryRunSelfCheck(const App_ModuleRegistryEntry_t *entry)
{
  App_CheckResult_t result = APP_CHECK_NORMAL;
  App_ErrorCode_t code = ERR_OK;

  if ((entry != NULL) && (entry->self_check != NULL))
  {
    result = entry->self_check(&code);
  }

  if (entry != NULL)
  {
    if ((result != APP_CHECK_NORMAL) && (code == ERR_OK))
    {
      code = App_RegistryDefaultCodeFromModule(entry->id);
    }

    App_StatusSet(entry->id, App_RegistryStateFromCheck(result), code);

    if ((entry->id != APP_MODULE_SENSOR) && (entry->id != APP_MODULE_ALARM))
    {
      if (result == APP_CHECK_NORMAL)
      {
        App_RegistryPostModuleAlarm(entry->id, APP_ALARM_OP_CLEAR, APP_ALARM_SEVERITY_NORMAL, App_RegistryDefaultCodeFromModule(entry->id), 0U);
      }
      else
      {
        App_RegistryPostModuleAlarm(entry->id, APP_ALARM_OP_RAISE, App_RegistrySeverityFromCheck(result), code, (uint32_t)result);
      }
    }
  }

  return result;
}

/**
 * @brief 将检查结果转换为模块状态。
 */
static App_ModuleState_t App_RegistryStateFromCheck(App_CheckResult_t result)
{
  switch (result)
  {
  case APP_CHECK_NORMAL:
    return APP_STATE_OK;
  case APP_CHECK_GENERAL:
    return APP_STATE_WARNING;
  case APP_CHECK_IMPORTANT:
    return APP_STATE_OFFLINE;
  case APP_CHECK_CRITICAL:
  default:
    return APP_STATE_ERROR;
  }
}

/**
 * @brief 将检查结果转换为异常严重程度。
 */
static App_AlarmSeverity_t App_RegistrySeverityFromCheck(App_CheckResult_t result)
{
  switch (result)
  {
  case APP_CHECK_NORMAL:
    return APP_ALARM_SEVERITY_NORMAL;
  case APP_CHECK_GENERAL:
    return APP_ALARM_SEVERITY_GENERAL;
  case APP_CHECK_IMPORTANT:
    return APP_ALARM_SEVERITY_IMPORTANT;
  case APP_CHECK_CRITICAL:
  default:
    return APP_ALARM_SEVERITY_CRITICAL;
  }
}

/**
 * @brief 比较两个检查结果并返回较严重者。
 */
static App_CheckResult_t App_RegistryWorst(App_CheckResult_t current, App_CheckResult_t next)
{
  if ((uint32_t)next > (uint32_t)current)
  {
    return next;
  }

  return current;
}

/**
 * @brief 将告警严重程度转换为注册表检查结果。
 */
static App_CheckResult_t App_RegistryCheckFromAlarmSeverity(App_AlarmSeverity_t severity)
{
  switch (severity)
  {
  case APP_ALARM_SEVERITY_NORMAL:
    return APP_CHECK_NORMAL;
  case APP_ALARM_SEVERITY_GENERAL:
    return APP_CHECK_GENERAL;
  case APP_ALARM_SEVERITY_IMPORTANT:
    return APP_CHECK_IMPORTANT;
  case APP_ALARM_SEVERITY_CRITICAL:
  default:
    return APP_CHECK_CRITICAL;
  }
}

/**
 * @brief 将显示层结果转换为应用检查结果。
 */
static App_CheckResult_t App_RegistryCheckFromDisplay(Display_Result_t result)
{
  switch (result)
  {
  case DISPLAY_OK:
    return APP_CHECK_NORMAL;
  case DISPLAY_NOT_READY:
    return APP_CHECK_GENERAL;
  case DISPLAY_ERROR:
  default:
    return APP_CHECK_IMPORTANT;
  }
}

/**
 * @brief 将传感器严重程度转换为应用检查结果。
 */
static App_CheckResult_t App_RegistryCheckFromSensor(Sensor_Severity_t severity)
{
  switch (severity)
  {
  case SENSOR_SEVERITY_NORMAL:
    return APP_CHECK_NORMAL;
  case SENSOR_SEVERITY_GENERAL:
    return APP_CHECK_GENERAL;
  case SENSOR_SEVERITY_IMPORTANT:
    return APP_CHECK_IMPORTANT;
  case SENSOR_SEVERITY_CRITICAL:
  default:
    return APP_CHECK_CRITICAL;
  }
}

/**
 * @brief 获取模块默认错误码。
 */
static App_ErrorCode_t App_RegistryDefaultCodeFromModule(App_ModuleId_t module)
{
  switch (module)
  {
  case APP_MODULE_COMM:
    return ERR_COMM_LORA_OFFLINE;
  case APP_MODULE_PROTOCOL:
    return ERR_PROTOCOL_PARSE_FAILED;
  case APP_MODULE_LOGGER:
    return ERR_LOG_WRITE_FAILED;
  case APP_MODULE_DISPLAY:
    return ERR_DISPLAY_OFFLINE;
  case APP_MODULE_STORAGE:
    return ERR_STORAGE_NOT_READY;
  case APP_MODULE_SENSOR:
    return ERR_SENSOR_DATA_INVALID;
  case APP_MODULE_ALARM:
  case APP_MODULE_SYSTEM:
  case APP_MODULE_COUNT:
  default:
    return ERR_SYSTEM_SELF_CHECK_FAILED;
  }
}

/**
 * @brief 校正传感器状态中的统一错误码。
 */
static App_ErrorCode_t App_RegistryNormalizeSensorCode(const Sensor_Status_t *status)
{
  if (status == NULL)
  {
    return ERR_SENSOR_DATA_INVALID;
  }

  if (status->severity == SENSOR_SEVERITY_NORMAL)
  {
    return ERR_OK;
  }

  switch (status->code)
  {
  case ERR_SENSOR_OFFLINE:
  case ERR_SENSOR_NO_DATA:
  case ERR_SENSOR_DATA_INVALID:
  case ERR_SENSOR_NO_FIX:
  case ERR_SENSOR_INIT_FAILED:
  case ERR_SENSOR_TIMEOUT:
    return status->code;
  case ERR_OK:
  default:
    return ERR_SENSOR_DATA_INVALID;
  }
}

/**
 * @brief 根据传感器设备 ID 查找驱动名称。
 */
static const char *App_RegistryFindSensorName(uint16_t device_id)
{
  uint16_t i;
  uint16_t count = Sensor_RegistryGetCount();

  for (i = 0U; i < count; i++)
  {
    const Sensor_Driver_t *driver = Sensor_RegistryGetDriver(i);

    if ((driver != NULL) && (driver->device_id == device_id) && (driver->name != NULL))
    {
      return driver->name;
    }
  }

  return "";
}

/**
 * @brief 投递模块级异常状态变化消息。
 */
static void App_RegistryPostModuleAlarm(App_ModuleId_t module, App_AlarmOp_t op, App_AlarmSeverity_t severity, App_ErrorCode_t code, uint32_t detail)
{
  App_AlarmMsg_t msg;
  App_AlarmPayload_t payload;
  App_AlarmPayload_t *payload_ptr = NULL;
  App_AlarmPayloadType_t payload_type = APP_ALARM_PAYLOAD_NONE;

  if (code == ERR_OK)
  {
    return;
  }

  (void)memset(&payload, 0, sizeof(payload));
  if (op == APP_ALARM_OP_RAISE)
  {
    payload.raw_detail = detail;
    payload_type = APP_ALARM_PAYLOAD_RAW;
    payload_ptr = &payload;
  }

  if (App_AlarmBuildMsg(&msg, op, module, severity, code, 0U, payload_type, payload_ptr) == APP_ALARM_RESULT_OK)
  {
    (void)App_AlarmPost(&msg, 0U);
  }
}

/**
 * @brief 投递传感器异常 RAISE 消息。
 */
static void App_RegistryPostSensorAlarm(const Sensor_Status_t *status, App_ErrorCode_t code)
{
  App_AlarmMsg_t msg;
  App_AlarmPayload_t payload;
  const char *name;

  if ((status == NULL) || (status->severity == SENSOR_SEVERITY_NORMAL) || (code == ERR_OK))
  {
    return;
  }

  (void)memset(&payload, 0, sizeof(payload));
  payload.sensor.sensor_id = status->device_id;
  payload.sensor.sensor_type = (uint16_t)status->sensor_type;
  payload.sensor.detail = status->driver_error;
  name = App_RegistryFindSensorName(status->device_id);
  (void)strncpy(payload.sensor.name, name, sizeof(payload.sensor.name) - 1U);
  payload.sensor.name[sizeof(payload.sensor.name) - 1U] = '\0';

  if (App_AlarmBuildMsg(&msg, APP_ALARM_OP_RAISE, APP_MODULE_SENSOR, App_RegistryCheckFromSensor(status->severity) == APP_CHECK_GENERAL ? APP_ALARM_SEVERITY_GENERAL : App_RegistrySeverityFromCheck(App_RegistryCheckFromSensor(status->severity)), code, status->device_id, APP_ALARM_PAYLOAD_SENSOR, &payload) == APP_ALARM_RESULT_OK)
  {
    (void)App_AlarmPost(&msg, 0U);
  }
}

/**
 * @brief 清除本轮已经恢复的传感器异常。
 */
static void App_RegistryClearRecoveredSensorAlarms(const App_RegistryAlarmKey_t *current_keys, uint16_t current_count)
{
  uint16_t active_count = App_AlarmGetActiveCount();
  uint16_t i;

  for (i = 0U; (i < active_count) && (i < APP_REGISTRY_SENSOR_ACTIVE_MAX); i++)
  {
    App_AlarmRecord_t record;

    if ((App_AlarmGetRecord(i, &record) == APP_ALARM_RESULT_OK) &&
        (record.source == APP_MODULE_SENSOR) &&
        (App_RegistryKeyExists(current_keys, current_count, record.code, record.instance_id) == 0U))
    {
      App_AlarmMsg_t msg;

      if (App_AlarmBuildMsg(&msg, APP_ALARM_OP_CLEAR, APP_MODULE_SENSOR, APP_ALARM_SEVERITY_NORMAL, record.code, record.instance_id, APP_ALARM_PAYLOAD_NONE, NULL) == APP_ALARM_RESULT_OK)
      {
        (void)App_AlarmPost(&msg, 0U);
      }
    }
  }
}

/**
 * @brief 判断异常 key 是否存在于当前自检结果。
 */
static uint8_t App_RegistryKeyExists(const App_RegistryAlarmKey_t *keys, uint16_t key_count, App_ErrorCode_t code, uint16_t instance_id)
{
  uint16_t i;

  for (i = 0U; i < key_count; i++)
  {
    if ((keys[i].code == code) && (keys[i].instance_id == instance_id))
    {
      return 1U;
    }
  }

  return 0U;
}
