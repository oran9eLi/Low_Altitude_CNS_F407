#ifndef SENSOR_DEVICE_ID_H
#define SENSOR_DEVICE_ID_H

/**
 * @brief V1.0 传感器设备编号。
 *
 * 设备编号由框架侧统一分配，具体驱动只引用这里的枚举值。
 */
typedef enum {
  SENSOR_DEVICE_INVALID = 0,
  SENSOR_DEVICE_GNSS_1 = 1,
  SENSOR_DEVICE_MPU6050_1 = 2,
  SENSOR_DEVICE_BME280_1 = 3
} Sensor_DeviceId_t;

#endif
