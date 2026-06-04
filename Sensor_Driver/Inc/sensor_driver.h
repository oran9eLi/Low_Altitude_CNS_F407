#ifndef SENSOR_DRIVER_H
#define SENSOR_DRIVER_H

#include <stdint.h>
#include "app_error.h"
#include "sensor_data.h"

/**
 * @brief 传感器异常严重程度。
 *
 * 该枚举只表达异常影响级别，不表达具体故障原因。枚举值按严重程度递增，
 * 注册表会用数值更大的结果作为更严重结果。
 */
typedef enum {
  SENSOR_SEVERITY_NORMAL = 0, /**< 正常。 */
  SENSOR_SEVERITY_GENERAL,    /**< 一般异常，不影响主系统运行。 */
  SENSOR_SEVERITY_IMPORTANT,  /**< 重要异常，影响实验或数据可信度。 */
  SENSOR_SEVERITY_CRITICAL    /**< 严重异常，需要停机或人工处理。 */
} Sensor_Severity_t;

/**
 * @brief 传感器状态事实。
 *
 * 驱动通过该结构向注册表输出严重程度、统一错误码和原始细节码。
 * `code` 必须使用 ERR_SENSOR_* 范围内的错误码；正常时填写 ERR_OK。
 */
typedef struct
{
  uint16_t device_id;              /**< 设备编号，由项目设备编号表统一分配。 */
  Sensor_Type_t sensor_type;       /**< 传感器类型。 */
  Sensor_Severity_t severity;      /**< 异常严重程度。 */
  App_ErrorCode_t code;            /**< 全系统统一错误码，传感器驱动只填写 ERR_SENSOR_* 或 ERR_OK。 */
  uint32_t driver_error;           /**< 可选驱动细节码，例如 HAL 返回值、芯片状态或阶段码；没有细节时填 0。 */
} Sensor_Status_t;

/**
 * @brief 传感器初始化函数指针类型。
 *
 * @param[out] status 可选状态输出，可以为 NULL。
 * @return 初始化严重程度。
 */
typedef Sensor_Severity_t (*Sensor_InitFn)(Sensor_Status_t *status);

/**
 * @brief 传感器自检函数指针类型。
 *
 * @param[out] status 可选状态输出，可以为 NULL。
 * @return 自检严重程度。
 */
typedef Sensor_Severity_t (*Sensor_SelfCheckFn)(Sensor_Status_t *status);

/**
 * @brief 传感器采样读取函数指针类型。
 *
 * @param[out] samples 调用方提供的采样输出缓冲区。
 * @param[in] max_count samples 最多可容纳的采样记录数量。
 * @param[out] out_count 可选输出，表示实际写入的采样记录数量。
 * @param[out] status 可选状态输出，可以为 NULL。
 * @return 读取严重程度。
 */
typedef Sensor_Severity_t (*Sensor_ReadFn)(Sensor_Sample_t *samples, uint16_t max_count, uint16_t *out_count, Sensor_Status_t *status);

/**
 * @brief 传感器状态查询函数指针类型。
 *
 * 与 self_check 的区别是：get_status 应尽量只返回当前缓存状态，不主动触发耗时总线读写。
 *
 * @param[out] status 可选状态输出，可以为 NULL。
 * @return 当前状态严重程度。
 */
typedef Sensor_Severity_t (*Sensor_GetStatusFn)(Sensor_Status_t *status);

/**
 * @brief 传感器驱动句柄。
 *
 * 每个具体传感器驱动都应提供一个 const Sensor_Driver_t 实例。注册表通过该句柄
 * 统一完成初始化、自检、读取和状态查询。
 */
typedef struct
{
  uint16_t device_id;            /**< 设备编号，由项目设备编号表统一分配。 */
  Sensor_Type_t sensor_type;     /**< 传感器类型。 */
  const char *name;              /**< 设备名称，用于日志、显示和调试。 */
  Sensor_InitFn init;            /**< 初始化回调。 */
  Sensor_SelfCheckFn self_check; /**< 自检回调。 */
  Sensor_ReadFn read;            /**< 采样读取回调。 */
  Sensor_GetStatusFn get_status; /**< 状态查询回调。 */
} Sensor_Driver_t;

#endif
