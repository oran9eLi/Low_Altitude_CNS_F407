/**
 * @file sensor_registry.c
 * @brief 传感器驱动注册表实现。
 *
 * 本模块维护产品中使用的传感器驱动静态列表。
 * 应用层通过本注册表，基于统一的 Sensor_Driver_t 接口完成传感器初始化、
 * 自检、查询和批量读取。
 */

#include "sensor_registry.h"

/***************DEBUG***************/
#include "debug_config.h"
#ifdef DEBUG_ENABLE
extern const Sensor_Driver_t g_mock_sensor_driver;
#endif
/*************DEBUG END*************/

#include <stddef.h>

/**
 * @brief 静态传感器驱动注册表的槽位数量。
 */
#define SENSOR_REGISTRY_SLOT_COUNT (sizeof(s_sensor_drivers) / sizeof((s_sensor_drivers)[0]))

/**
 * @brief 从两个传感器严重程度中返回更严重的结果。
 *
 * 枚举值按正常、一般、重要、严重递增，因此数值更大的结果被视为更严重。
 *
 * @param current 当前累计结果。
 * @param next 新增待合并的结果。
 * @return 更严重的结果。
 */
static Sensor_Severity_t Sensor_RegistryWorst(Sensor_Severity_t current, Sensor_Severity_t next)
{
  if ((uint32_t)next > (uint32_t)current) {
    return next;
  }

  return current;
}

/**
 * @brief 初始化一条传感器状态为正常。
 *
 * @param driver 传感器驱动句柄，可以为 NULL。
 * @param status 待初始化的状态结构，可以为 NULL。
 */
static void Sensor_RegistryInitStatus(const Sensor_Driver_t *driver, Sensor_Status_t *status)
{
  if (status == NULL) {
    return;
  }

  status->device_id    = (driver != NULL) ? driver->device_id : 0U;
  status->sensor_type  = (driver != NULL) ? driver->sensor_type : SENSOR_TYPE_UNKNOWN;
  status->severity     = SENSOR_SEVERITY_NORMAL;
  status->reason       = SENSOR_FAULT_NONE;
  status->driver_error = 0U;
}

/**
 * @brief 静态传感器驱动注册表。
 *
 * 具体传感器驱动文件加入工程后，在这里挂入对应的驱动句柄，
 * 例如：&g_gnss_driver、&g_weather_driver。
 *
 * 当前 NULL 占位用于在尚未接入具体传感器驱动时保持注册表合法。
 */
static const Sensor_Driver_t *const s_sensor_drivers[] =
  {
/***************DEBUG***************/
#ifdef DEBUG_ENABLE
    &g_mock_sensor_driver,
#endif
/*************DEBUG END*************/
    NULL,
  };

/**
 * @brief 初始化所有已注册的传感器驱动。
 *
 * 句柄为 NULL 或 init 回调为 NULL 的驱动会被跳过。
 * 函数返回所有驱动初始化结果中的最严重结果。
 *
 * @return 所有已注册驱动初始化成功时返回 SENSOR_SEVERITY_NORMAL；
 * 否则返回驱动上报的最严重程度。
 */
Sensor_Severity_t Sensor_RegistryInitAll(void)
{
  Sensor_Severity_t result = SENSOR_SEVERITY_NORMAL;
  uint16_t i;

  for (i = 0U; i < SENSOR_REGISTRY_SLOT_COUNT; i++) {
    const Sensor_Driver_t *driver = s_sensor_drivers[i];

    if ((driver != NULL) && (driver->init != NULL)) {
      Sensor_Status_t status;
      Sensor_Severity_t driver_result;

      Sensor_RegistryInitStatus(driver, &status);
      driver_result = driver->init(&status);
      result = Sensor_RegistryWorst(result, Sensor_RegistryWorst(status.severity, driver_result));
    }
  }

  return result;
}

/**
 * @brief 对所有已注册的传感器驱动执行自检。
 *
 * 句柄为 NULL 或 self_check 回调为 NULL 的驱动会被跳过。
 * @return 所有已注册驱动自检通过时返回 SENSOR_SEVERITY_NORMAL；
 * 否则返回驱动上报的最严重程度。
 */
Sensor_Severity_t Sensor_RegistrySelfCheckAll(void)
{
  Sensor_Severity_t result = SENSOR_SEVERITY_NORMAL;
  uint16_t i;

  for (i = 0U; i < SENSOR_REGISTRY_SLOT_COUNT; i++) {
    const Sensor_Driver_t *driver = s_sensor_drivers[i];

    if ((driver != NULL) && (driver->self_check != NULL)) {
      Sensor_Status_t status;
      Sensor_Severity_t driver_result;

      Sensor_RegistryInitStatus(driver, &status);
      driver_result = driver->self_check(&status);
      result = Sensor_RegistryWorst(result, Sensor_RegistryWorst(status.severity, driver_result));
    }
  }

  return result;
}

/**
 * @brief 对所有已注册传感器执行自检，并输出异常传感器明细。
 *
 * 本接口只把非 SENSOR_SEVERITY_NORMAL 的传感器写入 status_list。
 * out_count 返回本次发现的异常总数；如果 out_count 大于 max_count，
 * 表示调用方提供的 status_list 缓冲区不足，只有前 max_count 条异常被写入。
 *
 * @param[out] status_list 调用方提供的异常状态数组。
 * @param[in] max_count status_list 最多可容纳的异常状态数量。
 * @param[out] out_count 可选输出，表示本次发现的异常总数。
 * @return 没有异常时返回 SENSOR_SEVERITY_NORMAL；
 * 缓冲区无效时返回 SENSOR_SEVERITY_IMPORTANT；
 * 其他情况返回所有异常中的最严重程度。
 */
Sensor_Severity_t Sensor_RegistrySelfCheckAbnormal(Sensor_Status_t *status_list, uint16_t max_count, uint16_t *out_count)
{
  Sensor_Severity_t result = SENSOR_SEVERITY_NORMAL;
  uint16_t abnormal_count     = 0U;
  uint16_t i;

  if (out_count != NULL) {
    *out_count = 0U;
  }

  if ((status_list == NULL) || (max_count == 0U)) {
    return SENSOR_SEVERITY_IMPORTANT;
  }

  for (i = 0U; i < SENSOR_REGISTRY_SLOT_COUNT; i++) {
    const Sensor_Driver_t *driver = s_sensor_drivers[i];

    if ((driver != NULL) && (driver->self_check != NULL)) {
      Sensor_Status_t status;
      Sensor_Severity_t driver_result;

      Sensor_RegistryInitStatus(driver, &status);
      driver_result = driver->self_check(&status);

      if (status.severity < driver_result) {
        status.severity = driver_result;
      }

      if (status.severity != SENSOR_SEVERITY_NORMAL) {
        result = Sensor_RegistryWorst(result, status.severity);

        if (abnormal_count < max_count) {
          status_list[abnormal_count] = status;
        }

        abnormal_count++;
      }
    }
  }

  if (out_count != NULL) {
    *out_count = abnormal_count;
  }

  return result;
}

/**
 * @brief 从所有已注册的传感器驱动读取采样数据。
 *
 * 每个驱动向调用方提供的采样缓冲区追加 0 个或多个 Sensor_Sample_t 记录。
 * 当所有驱动遍历完成或缓冲区写满时停止读取。
 *
 * @param[out] samples 调用方提供的采样缓冲区。
 * @param[in] max_count samples 中最多可容纳的 Sensor_Sample_t 记录数量。
 * @param[out] out_count 可选输出，表示实际写入的采样数量。
 * @return 所有驱动读取成功时返回 SENSOR_SEVERITY_NORMAL；
 * 输出缓冲区无效时返回 SENSOR_SEVERITY_IMPORTANT；
 * 其他情况返回驱动上报的最严重程度。
 */
Sensor_Severity_t Sensor_RegistryReadAll(Sensor_Sample_t *samples,
                                         uint16_t max_sample_count,
                                         uint16_t *out_sample_count,
                                         Sensor_Status_t *status_list,
                                         uint16_t max_status_count,
                                         uint16_t *out_status_count)
{
  Sensor_Severity_t result = SENSOR_SEVERITY_NORMAL;
  uint16_t total_count        = 0U;
  uint16_t status_count       = 0U;
  uint16_t i;

  if (out_sample_count != NULL) {
    *out_sample_count = 0U;
  }

  if (out_status_count != NULL) {
    *out_status_count = 0U;
  }

  if ((samples == NULL) || (max_sample_count == 0U)) {
    return SENSOR_SEVERITY_IMPORTANT;
  }

  for (i = 0U; i < SENSOR_REGISTRY_SLOT_COUNT; i++) {
    const Sensor_Driver_t *driver = s_sensor_drivers[i];

    if ((driver != NULL) && (driver->read != NULL) && (total_count < max_sample_count)) {
      uint16_t driver_count = 0U;
      Sensor_Status_t status;
      Sensor_Severity_t driver_result;

      Sensor_RegistryInitStatus(driver, &status);
      driver_result = driver->read(&samples[total_count],
                                   (uint16_t)(max_sample_count - total_count),
                                   &driver_count,
                                   &status);

      if (status.severity < driver_result) {
        status.severity = driver_result;
      }

      result = Sensor_RegistryWorst(result, status.severity);
      total_count = (uint16_t)(total_count + driver_count);

      if (status.severity != SENSOR_SEVERITY_NORMAL) {
        if ((status_list != NULL) && (status_count < max_status_count)) {
          status_list[status_count] = status;
        }

        status_count++;
      }
    }
  }

  if (out_sample_count != NULL) {
    *out_sample_count = total_count;
  }

  if (out_status_count != NULL) {
    *out_status_count = status_count;
  }

  return result;
}

/**
 * @brief 获取当前已注册的具体传感器驱动数量。
 *
 * NULL 注册表槽位不会计入数量。
 *
 * @return 非 NULL 传感器驱动数量。
 */
uint16_t Sensor_RegistryGetCount(void)
{
  uint16_t count = 0U;
  uint16_t i;

  for (i = 0U; i < SENSOR_REGISTRY_SLOT_COUNT; i++) {
    if (s_sensor_drivers[i] != NULL) {
      count++;
    }
  }

  return count;
}

/**
 * @brief 按紧凑索引获取已注册的传感器驱动。
 *
 * 索引只针对非 NULL 注册项计数。例如，即使静态注册表中存在 NULL 占位，
 * index 0 仍然返回第一个具体传感器驱动。
 *
 * @param[in] index 非 NULL 驱动列表中的紧凑索引。
 * @return 指向目标驱动的指针；如果索引越界则返回 NULL。
 */
const Sensor_Driver_t *Sensor_RegistryGetDriver(uint16_t index)
{
  uint16_t count = 0U;
  uint16_t i;

  for (i = 0U; i < SENSOR_REGISTRY_SLOT_COUNT; i++) {
    if (s_sensor_drivers[i] != NULL) {
      if (count == index) {
        return s_sensor_drivers[i];
      }

      count++;
    }
  }

  return NULL;
}
