#ifndef SENSOR_REGISTRY_H
#define SENSOR_REGISTRY_H

#include <stdint.h>
#include "sensor_driver.h"

/**
 * @brief 传感器异常自检状态记录。
 *
 * 用于 Sensor_RegistrySelfCheckAbnormal() 输出具体异常传感器明细。
 * 正常传感器不会写入该结构数组。
 */
typedef struct
{
  uint16_t device_id;          /**< 异常传感器设备编号。 */
  Sensor_Type_t sensor_type;   /**< 异常传感器类型。 */
  Sensor_CheckResult_t result; /**< 自检结果。 */
  uint32_t error_code;         /**< 具体驱动返回的错误码。 */
} Sensor_CheckStatus_t;

Sensor_CheckResult_t Sensor_RegistryInitAll(void);
Sensor_CheckResult_t Sensor_RegistrySelfCheckAll(uint32_t *error_code);
Sensor_CheckResult_t Sensor_RegistrySelfCheckAbnormal(Sensor_CheckStatus_t *status_list, uint16_t max_count, uint16_t *out_count);
Sensor_CheckResult_t Sensor_RegistryReadAll(Sensor_Sample_t *samples, uint16_t max_count, uint16_t *out_count);

uint16_t Sensor_RegistryGetCount(void);
const Sensor_Driver_t *Sensor_RegistryGetDriver(uint16_t index);

#endif
