#include "sensor_driver.h"
#include "sensor_data.h"
#include "sensor_device_id.h"
#include <stddef.h>
#include <string.h>

static Sensor_Severity_t MockSensor_Init(Sensor_Status_t *status);
static Sensor_Severity_t MockSensor_SelfCheck(Sensor_Status_t *status);
static Sensor_Severity_t MockSensor_Read(Sensor_Sample_t *samples, uint16_t max_count, uint16_t *out_count, Sensor_Status_t *status);
static Sensor_Severity_t MockSensor_GetStatus(Sensor_Status_t *status);

static int32_t  s_lat_e7  = 399000000;
static int32_t  s_lon_e7  = 1164000000;
static int32_t  s_alt_mm  = 100000;
static uint32_t s_seed    = 0xA5A5U;
static uint8_t  s_cycle   = 0;

static uint32_t MockRand(void)
{
  s_seed = s_seed * 1103515245U + 12345U;
  return s_seed;
}

static Sensor_Severity_t MockSensor_Init(Sensor_Status_t *status)
{
  if (status != NULL) {
    status->device_id    = SENSOR_DEVICE_GNSS_1;
    status->sensor_type  = SENSOR_TYPE_GNSS;
    status->severity     = SENSOR_SEVERITY_NORMAL;
    status->reason       = SENSOR_FAULT_NONE;
    status->driver_error = 0U;
  }
  return SENSOR_SEVERITY_NORMAL;
}

static Sensor_Severity_t MockSensor_SelfCheck(Sensor_Status_t *status)
{
  uint8_t is_abnormal;

  s_cycle++;
  is_abnormal = ((s_cycle % 10U) == 0U) ? 1U : 0U;

  if (status != NULL) {
    status->device_id    = SENSOR_DEVICE_GNSS_1;
    status->sensor_type  = SENSOR_TYPE_GNSS;
    status->driver_error = 0U;

    if (is_abnormal != 0U) {
      status->severity = SENSOR_SEVERITY_GENERAL;
      status->reason   = SENSOR_FAULT_NO_FIX;
    } else {
      status->severity = SENSOR_SEVERITY_NORMAL;
      status->reason   = SENSOR_FAULT_NONE;
    }
  }
  return is_abnormal ? SENSOR_SEVERITY_GENERAL : SENSOR_SEVERITY_NORMAL;
}

static Sensor_Severity_t MockSensor_Read(Sensor_Sample_t *samples, uint16_t max_count, uint16_t *out_count, Sensor_Status_t *status)
{
  Sensor_Sample_t *sample;

  if ((samples == NULL) || (max_count == 0U)) {
    if (out_count != NULL) { *out_count = 0U; }
    return SENSOR_SEVERITY_IMPORTANT;
  }

  s_lat_e7 += (int32_t)((MockRand() % 2001U) - 1000U);
  s_lon_e7 += (int32_t)((MockRand() % 2001U) - 1000U);
  s_alt_mm += (int32_t)((MockRand() % 1001U) - 500U);

  if (s_lat_e7 < 399000000)  s_lat_e7 = 399000000;
  if (s_lat_e7 > 401000000)  s_lat_e7 = 401000000;
  if (s_lon_e7 < 1163000000) s_lon_e7 = 1163000000;
  if (s_lon_e7 > 1165000000) s_lon_e7 = 1165000000;
  if (s_alt_mm < 50000)      s_alt_mm = 50000;
  if (s_alt_mm > 150000)     s_alt_mm = 150000;

  sample = &samples[0];
  sample->device_id   = SENSOR_DEVICE_GNSS_1;
  sample->sensor_type = SENSOR_TYPE_GNSS;
  sample->channel     = 0U;
  sample->value_type  = SENSOR_VALUE_GEO;
  sample->timestamp_ms = 0U;
  sample->status      = 0U;
  strncpy(sample->unit, "geo", sizeof(sample->unit));
  sample->value.geo.latitude_deg_e7  = s_lat_e7;
  sample->value.geo.longitude_deg_e7 = s_lon_e7;
  sample->value.geo.altitude_mm      = s_alt_mm;

  if (out_count != NULL) { *out_count = 1U; }

  if (status != NULL) {
    status->device_id    = SENSOR_DEVICE_GNSS_1;
    status->sensor_type  = SENSOR_TYPE_GNSS;
    status->severity     = SENSOR_SEVERITY_NORMAL;
    status->reason       = SENSOR_FAULT_NONE;
    status->driver_error = 0U;
  }

  return SENSOR_SEVERITY_NORMAL;
}

static Sensor_Severity_t MockSensor_GetStatus(Sensor_Status_t *status)
{
  if (status != NULL) {
    status->device_id    = SENSOR_DEVICE_GNSS_1;
    status->sensor_type  = SENSOR_TYPE_GNSS;
    status->severity     = SENSOR_SEVERITY_NORMAL;
    status->reason       = SENSOR_FAULT_NONE;
    status->driver_error = 0U;
  }
  return SENSOR_SEVERITY_NORMAL;
}

const Sensor_Driver_t g_mock_sensor_driver =
{
  .device_id   = SENSOR_DEVICE_GNSS_1,
  .sensor_type = SENSOR_TYPE_GNSS,
  .name        = "MockGNSS",
  .init        = MockSensor_Init,
  .self_check  = MockSensor_SelfCheck,
  .read        = MockSensor_Read,
  .get_status  = MockSensor_GetStatus,
};
