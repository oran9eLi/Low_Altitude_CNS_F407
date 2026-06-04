#include "app_registry.h"

#include "FreeRTOS.h"
#include "app_alarm.h"
#include "app_logger.h"
#include "app_storage.h"
#include "display.h"
#include "sensor_registry.h"

#define APP_REGISTRY_COUNT(array) (sizeof(array) / sizeof((array)[0]))
#define APP_REGISTRY_SENSOR_STATUS_MAX 4U

static App_CheckResult_t App_RegistryNoopInit(void);
static App_CheckResult_t App_RegistryOkSelfCheck(App_ErrorCode_t *error_code);
static App_CheckResult_t App_RegistryAlarmSelfCheck(App_ErrorCode_t *error_code);
static App_CheckResult_t App_RegistryStorageInit(void);
static App_CheckResult_t App_RegistryStorageSelfCheck(App_ErrorCode_t *error_code);
static App_CheckResult_t App_RegistryLoggerInit(void);
static App_CheckResult_t App_RegistryDisplayInit(void);
static App_CheckResult_t App_RegistryDisplaySelfCheck(App_ErrorCode_t *error_code);
static App_CheckResult_t App_RegistrySensorInit(void);
static App_CheckResult_t App_RegistrySensorSelfCheck(App_ErrorCode_t *error_code);

static App_CheckResult_t App_RegistryRunInit(const App_ModuleRegistryEntry_t *entry);
static App_CheckResult_t App_RegistryRunSelfCheck(const App_ModuleRegistryEntry_t *entry);
static App_AlarmId_t App_RegistryAlarmFromModule(App_ModuleId_t module);
static App_ModuleState_t App_RegistryStateFromCheck(App_CheckResult_t result);
static App_CheckResult_t App_RegistryWorst(App_CheckResult_t current, App_CheckResult_t next);
static App_CheckResult_t App_RegistryCheckFromDisplay(Display_Result_t result);
static App_CheckResult_t App_RegistryCheckFromSensor(Sensor_Severity_t severity);
static App_AlarmId_t App_RegistryAlarmFromSensorStatus(const Sensor_Status_t *status);
static App_ErrorCode_t App_RegistryErrorFromSensorStatus(const Sensor_Status_t *status);
static void App_RegistryPostSensorAlarm(const Sensor_Status_t *status);

/**
 * @brief 应用模块静态注册表。
 *
 * 注册表集中描述各模块的初始化、自检、周期服务入口和自检周期。
 * 应用层通过该表统一完成启动初始化、运行期巡检、状态汇总和告警触发。
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
 *
 * 下标与 @ref s_module_registry 保持一致，用于 @ref App_RegistryPoll
 * 判断模块是否达到下一次自检周期。
 */
static uint32_t s_last_check_ms[APP_REGISTRY_COUNT(s_module_registry)];

/**
 * @brief 初始化注册表中的所有模块。
 *
 * 按注册表顺序执行模块初始化，并清空对应的周期自检时间戳。
 *
 * @retval APP_CHECK_OK 所有模块初始化成功。
 * @retval 其他值 返回初始化过程中出现的最高严重程度结果。
 */
App_CheckResult_t App_RegistryInitAll(void)
{
  App_CheckResult_t result = APP_CHECK_OK;
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
 *
 * 用于启动后自检或需要全量健康检查的场景。
 *
 * @retval APP_CHECK_OK 所有模块自检正常。
 * @retval 其他值 返回自检过程中出现的最高严重程度结果。
 */
App_CheckResult_t App_RegistrySelfCheckAll(void)
{
  App_CheckResult_t result = APP_CHECK_OK;
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
 * 该函数由应用任务周期调用。模块若注册了 service 入口则每次轮询执行；
 * 模块若注册了 self_check 且达到 check_period_ms，则执行一次自检并更新时间戳。
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
 *
 * @return 模块注册项数量。
 */
size_t App_RegistryGetCount(void)
{
  return APP_REGISTRY_COUNT(s_module_registry);
}

/**
 * @brief 按索引获取模块注册项。
 *
 * @param index 模块注册项索引。
 * @return 指向注册项的只读指针；索引越界时返回 NULL。
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
 *
 * 用于尚无独立初始化动作、但需要纳入注册表管理的模块。
 *
 * @retval APP_CHECK_OK 固定返回初始化成功。
 */
static App_CheckResult_t App_RegistryNoopInit(void)
{
  return APP_CHECK_OK;
}

/**
 * @brief 默认正常自检入口。
 *
 * 用于尚无独立自检动作、但需要参与统一自检流程的模块。
 *
 * @param error_code 输出错误码指针，可为 NULL。
 * @retval APP_CHECK_OK 固定返回自检正常。
 */
static App_CheckResult_t App_RegistryOkSelfCheck(App_ErrorCode_t *error_code)
{
  if (error_code != NULL)
  {
    *error_code = APP_ERROR_OK;
  }

  return APP_CHECK_OK;
}

/**
 * @brief 告警模块自检入口。
 *
 * 通过告警活动位图判断当前是否存在未清除告警，并同步输出当前错误码。
 *
 * @param error_code 输出当前错误码指针，可为 NULL。
 * @retval APP_CHECK_OK 当前无活动告警。
 * @retval APP_CHECK_WARNING 当前存在活动告警。
 */
static App_CheckResult_t App_RegistryAlarmSelfCheck(App_ErrorCode_t *error_code)
{
  uint32_t active_mask = App_AlarmGetActiveMask();

  if (error_code != NULL)
  {
    *error_code = App_AlarmGetCurrentError();
  }

  if (active_mask != 0U)
  {
    return APP_CHECK_WARNING;
  }

  return APP_CHECK_OK;
}

/**
 * @brief 存储模块初始化入口。
 *
 * 初始化应用存储抽象层，使后续日志和数据保存流程具备统一存储入口。
 *
 * @retval APP_CHECK_OK 当前存储初始化流程未上报错误。
 */
static App_CheckResult_t App_RegistryStorageInit(void)
{
  App_StorageInit();
  return APP_CHECK_OK;
}

/**
 * @brief 存储模块自检入口。
 *
 * 通过刷新存储缓存验证存储链路是否可用。
 *
 * @param error_code 输出错误码指针，可为 NULL。
 * @retval APP_CHECK_OK 存储刷新正常。
 * @retval APP_CHECK_NOT_READY 存储刷新失败或介质未就绪。
 */
static App_CheckResult_t App_RegistryStorageSelfCheck(App_ErrorCode_t *error_code)
{
  if (App_StorageFlush() != APP_STORAGE_OK)
  {
    if (error_code != NULL)
    {
      *error_code = APP_ERROR_SD_MOUNT;
    }

    return APP_CHECK_NOT_READY;
  }

  if (error_code != NULL)
  {
    *error_code = APP_ERROR_OK;
  }

  return APP_CHECK_OK;
}

/**
 * @brief 日志模块初始化入口。
 *
 * 创建日志队列并初始化日志模块依赖的运行资源。
 *
 * @retval APP_CHECK_OK 日志模块初始化成功。
 * @retval APP_CHECK_ERROR 日志模块初始化失败。
 */
static App_CheckResult_t App_RegistryLoggerInit(void)
{
  if (App_LoggerInit() != pdPASS)
  {
    return APP_CHECK_ERROR;
  }

  return APP_CHECK_OK;
}

/**
 * @brief 显示模块初始化入口。
 *
 * 调用显示层初始化函数，并将显示层结果转换为应用自检结果。
 *
 * @return 应用层检查结果。
 */
static App_CheckResult_t App_RegistryDisplayInit(void)
{
  return App_RegistryCheckFromDisplay(Display_Init());
}

/**
 * @brief 显示模块自检入口。
 *
 * 调用显示层自检函数，并将显示层结果转换为应用自检结果。
 *
 * @param error_code 输出错误码指针，可为 NULL。
 * @return 应用层检查结果。
 */
static App_CheckResult_t App_RegistryDisplaySelfCheck(App_ErrorCode_t *error_code)
{
  return App_RegistryCheckFromDisplay(Display_SelfCheck(error_code));
}

/**
 * @brief 传感器模块初始化入口。
 *
 * 初始化传感器注册表中的所有传感器，并将传感器严重程度转换为应用自检结果。
 *
 * @return 应用层检查结果。
 */
static App_CheckResult_t App_RegistrySensorInit(void)
{
  return App_RegistryCheckFromSensor(Sensor_RegistryInitAll());
}

/**
 * @brief 传感器模块自检入口。
 *
 * 收集传感器异常状态，投递传感器告警消息，并输出最高严重程度对应的错误码。
 *
 * @param error_code 输出错误码指针，可为 NULL。
 * @return 应用层检查结果。
 */
static App_CheckResult_t App_RegistrySensorSelfCheck(App_ErrorCode_t *error_code)
{
  Sensor_Status_t status_list[APP_REGISTRY_SENSOR_STATUS_MAX];
  Sensor_Severity_t sensor_severity;
  Sensor_Severity_t highest_severity = SENSOR_SEVERITY_NORMAL;
  App_ErrorCode_t current_error = APP_ERROR_OK;
  uint16_t status_count = 0U;
  uint16_t i;

  sensor_severity = Sensor_RegistrySelfCheckAbnormal(status_list,
                                                     APP_REGISTRY_SENSOR_STATUS_MAX,
                                                     &status_count);

  for (i = 0U; (i < status_count) && (i < APP_REGISTRY_SENSOR_STATUS_MAX); i++)
  {
    const Sensor_Status_t *status = &status_list[i];
    App_ErrorCode_t mapped_error = App_RegistryErrorFromSensorStatus(status);

    App_RegistryPostSensorAlarm(status);

    if (status->severity >= highest_severity)
    {
      highest_severity = status->severity;
      current_error = mapped_error;
    }
  }

  if ((status_count == 0U) && (sensor_severity != SENSOR_SEVERITY_NORMAL))
  {
    current_error = APP_ERROR_SENSOR_I2C;
  }

  if (error_code != NULL)
  {
    *error_code = current_error;
  }

  return App_RegistryCheckFromSensor(sensor_severity);
}

/**
 * @brief 执行单个模块初始化并同步异常状态。
 *
 * 初始化失败时会更新模块状态，并按模块默认告警项投递初始化异常告警。
 *
 * @param entry 模块注册项指针。
 * @return 模块初始化检查结果。
 */
static App_CheckResult_t App_RegistryRunInit(const App_ModuleRegistryEntry_t *entry)
{
  App_CheckResult_t result = APP_CHECK_OK;

  if ((entry != NULL) && (entry->init != NULL))
  {
    result = entry->init();
  }

  if ((entry != NULL) && (result != APP_CHECK_OK))
  {
    App_AlarmId_t alarm_id = App_RegistryAlarmFromModule(entry->id);

    // 初始化入口没有模块自检输出的具体错误码，默认错误码由告警模块统一解析。
    App_StatusSet(entry->id, App_RegistryStateFromCheck(result), APP_ERROR_OK);
    (void)App_AlarmRaiseDefault(alarm_id, entry->id, (uint32_t)result);
  }

  return result;
}

/**
 * @brief 执行单个模块自检并同步状态。
 *
 * 非传感器模块自检异常时由注册表按模块默认告警项投递告警；传感器模块
 * 由传感器自检流程按具体设备状态投递更细粒度的告警。
 *
 * @param entry 模块注册项指针。
 * @return 模块自检结果。
 */
static App_CheckResult_t App_RegistryRunSelfCheck(const App_ModuleRegistryEntry_t *entry)
{
  App_CheckResult_t result = APP_CHECK_OK;
  App_ErrorCode_t error_code = APP_ERROR_OK;

  if ((entry != NULL) && (entry->self_check != NULL))
  {
    result = entry->self_check(&error_code);
  }

  if (entry != NULL)
  {
    App_StatusSet(entry->id, App_RegistryStateFromCheck(result), error_code);

    if ((entry->id != APP_MODULE_SENSOR) && (result >= APP_CHECK_NOT_READY))
    {
      App_AlarmId_t alarm_id = App_RegistryAlarmFromModule(entry->id);

      if (error_code == APP_ERROR_OK)
      {
        (void)App_AlarmRaiseDefault(alarm_id, entry->id, (uint32_t)result);
      }
      else
      {
        (void)App_AlarmRaise(alarm_id, entry->id, error_code, (uint32_t)result);
      }
    }
  }

  return result;
}

/**
 * @brief 获取模块对应的默认告警项。
 *
 * @param module 应用模块编号。
 * @return 模块默认告警项。
 */
static App_AlarmId_t App_RegistryAlarmFromModule(App_ModuleId_t module)
{
  switch (module)
  {
  case APP_MODULE_DISPLAY:
    return APP_ALARM_HMI_OFFLINE;
  case APP_MODULE_COMM:
    return APP_ALARM_LORA_OFFLINE;
  case APP_MODULE_LOGGER:
    return APP_ALARM_LOG_STORAGE_FAILED;
  case APP_MODULE_STORAGE:
    return APP_ALARM_STORAGE_FAILED;
  case APP_MODULE_SENSOR:
    return APP_ALARM_SENSOR_FAULT;
  case APP_MODULE_SYSTEM:
  case APP_MODULE_PROTOCOL:
  case APP_MODULE_ALARM:
  case APP_MODULE_COUNT:
  default:
    return APP_ALARM_SELF_CHECK_FAILED;
  }
}

/**
 * @brief 将检查结果转换为模块状态。
 *
 * @param result 应用检查结果。
 * @return 模块运行状态。
 */
static App_ModuleState_t App_RegistryStateFromCheck(App_CheckResult_t result)
{
  switch (result)
  {
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

/**
 * @brief 比较两个检查结果并返回较严重者。
 *
 * @param current 当前累计结果。
 * @param next 新的检查结果。
 * @return 严重程度更高的检查结果。
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
 * @brief 将显示层结果转换为应用检查结果。
 *
 * @param result 显示层返回结果。
 * @return 应用检查结果。
 */
static App_CheckResult_t App_RegistryCheckFromDisplay(Display_Result_t result)
{
  switch (result)
  {
  case DISPLAY_OK:
    return APP_CHECK_OK;
  case DISPLAY_NOT_READY:
    return APP_CHECK_NOT_READY;
  case DISPLAY_ERROR:
  default:
    return APP_CHECK_ERROR;
  }
}

/**
 * @brief 将传感器严重程度转换为应用检查结果。
 *
 * @param severity 传感器严重程度。
 * @return 应用检查结果。
 */
static App_CheckResult_t App_RegistryCheckFromSensor(Sensor_Severity_t severity)
{
  switch (severity)
  {
  case SENSOR_SEVERITY_NORMAL:
    return APP_CHECK_OK;
  case SENSOR_SEVERITY_GENERAL:
    return APP_CHECK_WARNING;
  case SENSOR_SEVERITY_IMPORTANT:
  case SENSOR_SEVERITY_CRITICAL:
  default:
    return APP_CHECK_ERROR;
  }
}

/**
 * @brief 根据传感器状态选择告警项。
 *
 * GNSS 相关异常会优先映射到 GNSS 专用告警；其他传感器异常统一映射为
 * 传感器故障告警。
 *
 * @param status 传感器状态指针。
 * @return 传感器状态对应的告警项。
 */
static App_AlarmId_t App_RegistryAlarmFromSensorStatus(const Sensor_Status_t *status)
{
  if (status == NULL)
  {
    return APP_ALARM_SENSOR_FAULT;
  }

  if (status->sensor_type == SENSOR_TYPE_GNSS)
  {
    if (status->reason == SENSOR_FAULT_NO_FIX)
    {
      return APP_ALARM_GNSS_NO_FIX;
    }

    if ((status->reason == SENSOR_FAULT_OFFLINE) ||
        (status->reason == SENSOR_FAULT_NO_DATA) ||
        (status->reason == SENSOR_FAULT_NOT_READY) ||
        (status->reason == SENSOR_FAULT_TIMEOUT))
    {
      return APP_ALARM_GNSS_OFFLINE;
    }
  }

  return APP_ALARM_SENSOR_FAULT;
}

/**
 * @brief 根据传感器状态选择应用错误码。
 *
 * GNSS 相关异常会优先映射到 GNSS 专用错误码；其他传感器异常统一映射为
 * 通用传感器链路错误码。
 *
 * @param status 传感器状态指针。
 * @return 传感器状态对应的应用错误码。
 */
static App_ErrorCode_t App_RegistryErrorFromSensorStatus(const Sensor_Status_t *status)
{
  if (status == NULL)
  {
    return APP_ERROR_SENSOR_I2C;
  }

  if (status->sensor_type == SENSOR_TYPE_GNSS)
  {
    if (status->reason == SENSOR_FAULT_NO_FIX)
    {
      return APP_ERROR_GNSS_NO_FIX;
    }

    if ((status->reason == SENSOR_FAULT_OFFLINE) ||
        (status->reason == SENSOR_FAULT_NO_DATA) ||
        (status->reason == SENSOR_FAULT_NOT_READY) ||
        (status->reason == SENSOR_FAULT_TIMEOUT))
    {
      return APP_ERROR_GNSS_OFFLINE;
    }
  }

  return APP_ERROR_SENSOR_I2C;
}

/**
 * @brief 投递传感器告警消息。
 *
 * 将传感器状态组装为告警 payload，并通过告警队列提交给告警模块处理。
 * 状态为空或严重程度为正常时不投递告警。
 *
 * @param status 传感器状态指针。
 */
static void App_RegistryPostSensorAlarm(const Sensor_Status_t *status)
{
  App_AlarmMsg_t msg;
  App_AlarmPayload_t payload;
  App_AlarmResult_t result;

  if ((status == NULL) || (status->severity == SENSOR_SEVERITY_NORMAL))
  {
    return;
  }

  payload.sensor.device_id = status->device_id;
  payload.sensor.sensor_type = (uint16_t)status->sensor_type;
  payload.sensor.severity = (uint16_t)status->severity;
  payload.sensor.fault_reason = (uint16_t)status->reason;
  payload.sensor.driver_error = status->driver_error;

  result = App_AlarmBuildRaiseMsg(&msg,
                                  App_RegistryAlarmFromSensorStatus(status),
                                  APP_MODULE_SENSOR,
                                  App_RegistryErrorFromSensorStatus(status),
                                  APP_ALARM_PAYLOAD_SENSOR,
                                  &payload);
  if (result == APP_ALARM_RESULT_OK)
  {
    (void)App_AlarmPost(&msg, 0U);
  }
}
