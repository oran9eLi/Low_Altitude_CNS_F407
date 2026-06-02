#ifndef SENSOR_DATA_H
#define SENSOR_DATA_H

#include <stdint.h>

/**
 * @brief 采样单位字符串最大长度，包含字符串结束符。
 */
#define SENSOR_UNIT_LEN 8U

/**
 * @brief 传感器或设备类型。
 *
 * 用于描述驱动句柄和采样数据所属的设备类别，便于上层显示、日志和协议上报。
 */
typedef enum {
  SENSOR_TYPE_UNKNOWN = 0, /**< 未分类或暂未明确的设备类型。 */
  SENSOR_TYPE_GNSS,        /**< 导航定位类设备，例如 GNSS 模块。 */
  SENSOR_TYPE_REMOTE_ID,   /**< 监视类设备，例如 Remote ID 模块。 */
  SENSOR_TYPE_WEATHER,     /**< 气象或环境类传感器，例如 BME280。 */
  SENSOR_TYPE_POWER,       /**< 电源检测类设备，例如电压、电流检测模块。 */
  SENSOR_TYPE_RELAY_IO     /**< 继电器、开关量或普通 IO 类设备。 */
} Sensor_Type_t;

/**
 * @brief 传感器采样值的数据类型。
 *
 * 该类型用于说明 Sensor_Value_t 联合体中当前有效的字段。
 */
typedef enum {
  SENSOR_VALUE_NONE = 0, /**< 无有效采样值。 */
  SENSOR_VALUE_FLOAT,    /**< 单精度浮点采样值，对应 Sensor_Value_t::f32。 */
  SENSOR_VALUE_INT32,    /**< 有符号 32 位整型采样值，对应 Sensor_Value_t::i32。 */
  SENSOR_VALUE_UINT32,   /**< 无符号 32 位整型采样值，对应 Sensor_Value_t::u32。 */
  SENSOR_VALUE_BOOL,     /**< 布尔或开关量采样值，对应 Sensor_Value_t::boolean。 */
  SENSOR_VALUE_GEO       /**< 经纬高组合采样值，对应 Sensor_Value_t::geo。 */
} Sensor_ValueType_t;

/**
 * @brief 经纬高采样值。
 *
 * 使用整数定点格式，避免对外传输或存储时依赖 double。
 */
typedef struct
{
  int32_t latitude_deg_e7;  /**< 纬度，单位为 degree * 1e7。 */
  int32_t longitude_deg_e7; /**< 经度，单位为 degree * 1e7。 */
  int32_t altitude_mm;      /**< 高度，单位为 mm。 */
} Sensor_GeoValue_t;

/**
 * @brief 统一传感器采样值联合体。
 *
 * 具体哪个字段有效由 Sensor_Sample_t::value_type 决定。
 */
typedef union {
  float f32;             /**< 单精度浮点值。 */
  int32_t i32;           /**< 有符号 32 位整型值。 */
  uint32_t u32;          /**< 无符号 32 位整型值。 */
  uint8_t boolean;       /**< 布尔或开关量，0 表示 false，非 0 表示 true。 */
  Sensor_GeoValue_t geo; /**< 经纬高组合值。 */
} Sensor_Value_t;

/**
 * @brief 统一传感器采样记录。
 *
 * 具体传感器驱动通过该结构向上层输出采样数据。一个驱动可以一次输出多条记录，
 * 例如气象模块可以分别输出温度、湿度、气压。
 */
typedef struct
{
  uint16_t device_id;            /**< 设备编号，由项目设备编号表统一分配。 */
  Sensor_Type_t sensor_type;     /**< 传感器类型，通常与驱动句柄中的类型一致。 */
  uint16_t channel;              /**< 通道号，用于区分同一设备输出的多个采样项。 */
  Sensor_ValueType_t value_type; /**< 采样值类型，说明 value 联合体中哪个字段有效。 */
  uint32_t timestamp_ms;         /**< 采样时间戳，单位 ms。 */
  uint32_t status;               /**< 采样状态或数据质量标志，0 表示正常。 */
  char unit[SENSOR_UNIT_LEN];    /**< 单位字符串，例如 "V"、"degC"、"geo"、"-"。 */
  Sensor_Value_t value;          /**< 采样值。 */
} Sensor_Sample_t;

#endif
