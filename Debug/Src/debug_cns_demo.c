#include "debug_cns_demo.h"

/***************DEBUG***************/
#ifdef DEBUG_ENABLE

#include "FreeRTOS.h"
#include "task.h"
#include "app_protocol.h"
#include "debug_trace.h"
#include "sensor_registry.h"
#include <string.h>

#define DEBUG_CNS_FRAME_TYPE_GNSS 0x02U
#define DEBUG_CNS_GNSS_PAYLOAD_LEN 25U

static void DebugCnsDemo_WriteLe32(uint8_t *buffer, uint16_t *offset, uint32_t value)
{
  buffer[(*offset)++] = (uint8_t)( value        & 0xFFU);
  buffer[(*offset)++] = (uint8_t)((value >> 8)  & 0xFFU);
  buffer[(*offset)++] = (uint8_t)((value >> 16) & 0xFFU);
  buffer[(*offset)++] = (uint8_t)((value >> 24) & 0xFFU);
}

static void DebugCnsDemo_BuildGnssPayload(const Sensor_Sample_t *sample, uint8_t *payload)
{
  uint16_t offset = 0U;
  uint8_t i;

  payload[offset++] = ((sample->value.geo.latitude_deg_e7 != 0) ? 2U : 0U);
  DebugCnsDemo_WriteLe32(payload, &offset, (uint32_t)sample->value.geo.latitude_deg_e7);
  DebugCnsDemo_WriteLe32(payload, &offset, (uint32_t)sample->value.geo.longitude_deg_e7);
  DebugCnsDemo_WriteLe32(payload, &offset, (uint32_t)sample->value.geo.altitude_mm);
  DebugCnsDemo_WriteLe32(payload, &offset, sample->timestamp_ms);

  for (i = 0U; i < 8U; i++) {
    payload[offset++] = 0x00U;
  }
}

void DebugCnsDemo_RunOnce(void)
{
  Sensor_Sample_t sample;
  Sensor_Status_t sensor_status;
  uint8_t payload[DEBUG_CNS_GNSS_PAYLOAD_LEN];
  uint8_t cns_frame[64U];
  uint16_t cns_frame_len = 0U;
  uint16_t sample_count = 0U;
  uint16_t status_count = 0U;
  Sensor_Severity_t read_result;
  static uint32_t s_frame_count = 0U;

  memset(&sample, 0, sizeof(sample));
  memset(&sensor_status, 0, sizeof(sensor_status));
  read_result = Sensor_RegistryReadAll(&sample,
                                       1U,
                                       &sample_count,
                                       &sensor_status,
                                       1U,
                                       &status_count);

  if ((read_result == SENSOR_SEVERITY_NORMAL) &&
      (sample_count > 0U) &&
      (sample.value_type == SENSOR_VALUE_GEO)) {
    sample.timestamp_ms = xTaskGetTickCount();

    DBG_PRINT("GNSS " DBG_TEXT_SAMPLE ": t=%lu " DBG_TEXT_LATITUDE "E7=%ld " DBG_TEXT_LONGITUDE "E7=%ld " DBG_TEXT_ALTITUDE "mm=%ld",
              (unsigned long)sample.timestamp_ms,
              (long)sample.value.geo.latitude_deg_e7,
              (long)sample.value.geo.longitude_deg_e7,
              (long)sample.value.geo.altitude_mm);

    DebugCnsDemo_BuildGnssPayload(&sample, payload);
    (void)App_ProtocolBuildFrame(DEBUG_CNS_FRAME_TYPE_GNSS,
                                 payload,
                                 DEBUG_CNS_GNSS_PAYLOAD_LEN,
                                 cns_frame,
                                 (uint16_t)sizeof(cns_frame),
                                 &cns_frame_len);

    s_frame_count++;
    DBG_PRINT("CNS " DBG_TEXT_FRAME " #%lu", (unsigned long)s_frame_count);
    DebugConsole_HexDump("CNS", cns_frame, cns_frame_len);
  } else {
    DBG_PRINT(DBG_TEXT_SENSOR_READ ": " DBG_TEXT_RESULT "=%u " DBG_TEXT_SAMPLE_COUNT "=%u " DBG_TEXT_STATUS "=%u",
              (unsigned int)read_result,
              (unsigned int)sample_count,
              (unsigned int)status_count);
  }
}

#endif
/*************DEBUG END*************/
