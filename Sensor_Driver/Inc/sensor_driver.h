#ifndef SENSOR_DRIVER_H
#define SENSOR_DRIVER_H

#include <stdint.h>
#include "sensor_data.h"

/**
 * @brief 传感器驱动检查结果。
 *
 * 该结果用于驱动初始化、自检、读取和状态查询。枚举值按严重程度递增，
 * 注册表会用数值更大的结果作为更严重结果。
 */
typedef enum {
  SENSOR_CHECK_OK = 0,    /**< 正常。 */
  SENSOR_CHECK_WARNING,   /**< 有警告但可继续运行，例如数据质量下降。 */
  SENSOR_CHECK_NOT_READY, /**< 尚未就绪，例如未收到首帧或总线未准备好。 */
  SENSOR_CHECK_ERROR      /**< 设备不可用或存在严重错误。 */
} Sensor_CheckResult_t;

/**
 * @brief 传感器初始化函数指针类型。
 *
 * @return 初始化结果。
 */
typedef Sensor_CheckResult_t (*Sensor_InitFn)(void);

/**
 * @brief 传感器自检函数指针类型。
 *
 * @param[out] error_code 可选错误码输出，可以为 NULL。
 * @return 自检结果。
 */
typedef Sensor_CheckResult_t (*Sensor_SelfCheckFn)(uint32_t *error_code);

/**
 * @brief 传感器采样读取函数指针类型。
 *
 * @param[out] samples 调用方提供的采样输出缓冲区。
 * @param[in] max_count samples 最多可容纳的采样记录数量。
 * @param[out] out_count 可选输出，表示实际写入的采样记录数量。
 * @return 读取结果。
 */
typedef Sensor_CheckResult_t (*Sensor_ReadFn)(Sensor_Sample_t *samples, uint16_t max_count, uint16_t *out_count);

/**
 * @brief 传感器状态查询函数指针类型。
 *
 * 与 self_check 的区别是：get_status 应尽量只返回当前缓存状态，不主动触发耗时总线读写。
 *
 * @param[out] error_code 可选错误码输出，可以为 NULL。
 * @return 当前状态结果。
 */
typedef Sensor_CheckResult_t (*Sensor_GetStatusFn)(uint32_t *error_code);

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
