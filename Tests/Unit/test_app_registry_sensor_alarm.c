#include <stdint.h>
#include <stdio.h>

#include "app_registry.h"
#include "app_alarm.h"
#include "app_logger.h"
#include "app_storage.h"
#include "display.h"
#include "sensor_registry.h"

static App_AlarmMsg_t s_last_alarm_msg;
static uint8_t s_alarm_post_called;
static App_ModuleState_t s_sensor_status_state;
static App_ErrorCode_t s_sensor_status_error;
static uint8_t s_sensor_status_set;

static int expect_true(int condition, const char *message)
{
  if (!condition)
  {
    (void)printf("FAIL: %s\n", message);
    return 1;
  }

  return 0;
}

void App_StatusSet(App_ModuleId_t module, App_ModuleState_t state, App_ErrorCode_t error_code)
{
  if (module == APP_MODULE_SENSOR)
  {
    s_sensor_status_state = state;
    s_sensor_status_error = error_code;
    s_sensor_status_set = 1U;
  }
}

App_AlarmResult_t App_AlarmBuildRaiseMsg(App_AlarmMsg_t *msg,
                                         App_AlarmId_t alarm_id,
                                         App_ModuleId_t source,
                                         App_ErrorCode_t error_code,
                                         App_AlarmPayloadType_t payload_type,
                                         const App_AlarmPayload_t *payload)
{
  if ((msg == 0) || (payload == 0))
  {
    return APP_ALARM_RESULT_INVALID_PARAM;
  }

  msg->header.msg_type = APP_ALARM_MSG_RAISE;
  msg->header.alarm_id = alarm_id;
  msg->header.source = source;
  msg->header.error_code = error_code;
  msg->header.payload_type = payload_type;
  msg->header.timestamp_ms = 0U;
  msg->payload = *payload;
  return APP_ALARM_RESULT_OK;
}

BaseType_t App_AlarmPost(const App_AlarmMsg_t *msg, TickType_t timeout_ticks)
{
  (void)timeout_ticks;

  if (msg == 0)
  {
    return pdFAIL;
  }

  s_last_alarm_msg = *msg;
  s_alarm_post_called = 1U;
  return pdPASS;
}

App_AlarmResult_t App_AlarmRaise(App_AlarmId_t alarm_id, App_ModuleId_t source, App_ErrorCode_t error_code, uint32_t detail)
{
  (void)alarm_id;
  (void)source;
  (void)error_code;
  (void)detail;
  return APP_ALARM_RESULT_OK;
}

App_AlarmResult_t App_AlarmRaiseDefault(App_AlarmId_t alarm_id, App_ModuleId_t source, uint32_t detail)
{
  (void)alarm_id;
  (void)source;
  (void)detail;
  return APP_ALARM_RESULT_OK;
}

uint32_t App_AlarmGetActiveMask(void)
{
  return 0U;
}

App_ErrorCode_t App_AlarmGetCurrentError(void)
{
  return APP_ERROR_OK;
}

void App_StorageInit(void)
{
}

App_StorageResult_t App_StorageWriteCsvLine(const char *line, size_t length)
{
  (void)line;
  (void)length;
  return APP_STORAGE_OK;
}

App_StorageResult_t App_StorageFlush(void)
{
  return APP_STORAGE_OK;
}

BaseType_t App_LoggerInit(void)
{
  return pdPASS;
}

BaseType_t App_LogWrite(const App_LogRecord_t *record)
{
  (void)record;
  return pdPASS;
}

void App_LoggerTask(void *argument)
{
  (void)argument;
}

Display_Result_t Display_Init(void)
{
  return DISPLAY_OK;
}

Display_Result_t Display_SelfCheck(App_ErrorCode_t *error_code)
{
  if (error_code != 0)
  {
    *error_code = APP_ERROR_OK;
  }

  return DISPLAY_OK;
}

Display_Result_t Display_Refresh(uint32_t now_ms)
{
  (void)now_ms;
  return DISPLAY_OK;
}

Sensor_Severity_t Sensor_RegistryInitAll(void)
{
  return SENSOR_SEVERITY_NORMAL;
}

Sensor_Severity_t Sensor_RegistrySelfCheckAll(void)
{
  return SENSOR_SEVERITY_GENERAL;
}

Sensor_Severity_t Sensor_RegistrySelfCheckAbnormal(Sensor_Status_t *status_list, uint16_t max_count, uint16_t *out_count)
{
  if (out_count != 0)
  {
    *out_count = 1U;
  }

  if ((status_list != 0) && (max_count > 0U))
  {
    status_list[0].device_id = 1U;
    status_list[0].sensor_type = SENSOR_TYPE_GNSS;
    status_list[0].severity = SENSOR_SEVERITY_GENERAL;
    status_list[0].reason = SENSOR_FAULT_NO_FIX;
    status_list[0].driver_error = 0x33U;
  }

  return SENSOR_SEVERITY_GENERAL;
}

Sensor_Severity_t Sensor_RegistryReadAll(Sensor_Sample_t *samples,
                                         uint16_t max_sample_count,
                                         uint16_t *out_sample_count,
                                         Sensor_Status_t *status_list,
                                         uint16_t max_status_count,
                                         uint16_t *out_status_count)
{
  (void)samples;
  (void)max_sample_count;
  (void)status_list;
  (void)max_status_count;

  if (out_sample_count != 0)
  {
    *out_sample_count = 0U;
  }

  if (out_status_count != 0)
  {
    *out_status_count = 0U;
  }

  return SENSOR_SEVERITY_NORMAL;
}

uint16_t Sensor_RegistryGetCount(void)
{
  return 0U;
}

const Sensor_Driver_t *Sensor_RegistryGetDriver(uint16_t index)
{
  (void)index;
  return 0;
}

int main(void)
{
  int failures = 0;

  failures += expect_true(App_RegistrySelfCheckAll() == APP_CHECK_WARNING,
                          "sensor GNSS no-fix is reported as warning");
  failures += expect_true(s_sensor_status_set != 0U,
                          "sensor module status is updated");
  failures += expect_true(s_sensor_status_state == APP_STATE_WARNING,
                          "sensor module status is warning");
  failures += expect_true(s_sensor_status_error == APP_ERROR_GNSS_NO_FIX,
                          "sensor module status uses GNSS no-fix error");
  failures += expect_true(s_alarm_post_called != 0U,
                          "sensor alarm message is posted");
  failures += expect_true(s_last_alarm_msg.header.alarm_id == APP_ALARM_GNSS_NO_FIX,
                          "GNSS no-fix alarm is selected");
  failures += expect_true(s_last_alarm_msg.header.source == APP_MODULE_SENSOR,
                          "sensor module is alarm source");
  failures += expect_true(s_last_alarm_msg.header.error_code == APP_ERROR_GNSS_NO_FIX,
                          "GNSS no-fix error is selected");
  failures += expect_true(s_last_alarm_msg.header.payload_type == APP_ALARM_PAYLOAD_SENSOR,
                          "sensor payload type is used");
  failures += expect_true(s_last_alarm_msg.payload.sensor.device_id == 1U,
                          "sensor device id is in payload");
  failures += expect_true(s_last_alarm_msg.payload.sensor.sensor_type == SENSOR_TYPE_GNSS,
                          "sensor type is in payload");
  failures += expect_true(s_last_alarm_msg.payload.sensor.severity == SENSOR_SEVERITY_GENERAL,
                          "sensor severity is in payload");
  failures += expect_true(s_last_alarm_msg.payload.sensor.fault_reason == SENSOR_FAULT_NO_FIX,
                          "sensor fault reason is in payload");
  failures += expect_true(s_last_alarm_msg.payload.sensor.driver_error == 0x33U,
                          "sensor driver error is in payload");

  if (failures != 0)
  {
    (void)printf("%d failure(s)\n", failures);
    return 1;
  }

  (void)printf("app registry sensor alarm tests passed\n");
  return 0;
}
