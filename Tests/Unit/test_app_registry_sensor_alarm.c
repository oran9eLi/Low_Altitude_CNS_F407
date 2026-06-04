#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "app_registry.h"
#include "app_alarm.h"
#include "app_logger.h"
#include "app_storage.h"
#include "display.h"
#include "sensor_registry.h"

static App_AlarmMsg_t s_sensor_alarm_msg;
static uint8_t s_sensor_alarm_posted;
static App_ModuleState_t s_sensor_status_state;
static App_ErrorCode_t s_sensor_status_code;
static uint8_t s_sensor_status_set;

static const Sensor_Driver_t s_test_sensor_driver =
{
  .device_id = 1U,
  .sensor_type = SENSOR_TYPE_GNSS,
  .name = "GNSS-1",
  .init = 0,
  .self_check = 0,
  .read = 0,
  .get_status = 0,
};

static int expect_true(int condition, const char *message)
{
  if (!condition)
  {
    (void)printf("FAIL: %s\n", message);
    return 1;
  }

  return 0;
}

void App_StatusSet(App_ModuleId_t module, App_ModuleState_t state, App_ErrorCode_t code)
{
  if (module == APP_MODULE_SENSOR)
  {
    s_sensor_status_state = state;
    s_sensor_status_code = code;
    s_sensor_status_set = 1U;
  }
}

App_AlarmResult_t App_AlarmBuildMsg(App_AlarmMsg_t *msg, App_AlarmOp_t op, App_ModuleId_t source, App_AlarmSeverity_t severity, App_ErrorCode_t code, uint16_t instance_id, App_AlarmPayloadType_t payload_type, const App_AlarmPayload_t *payload)
{
  if (msg == 0)
  {
    return APP_ALARM_RESULT_INVALID_PARAM;
  }

  (void)memset(msg, 0, sizeof(*msg));
  msg->header.op = op;
  msg->header.source = source;
  msg->header.severity = severity;
  msg->header.code = code;
  msg->header.instance_id = instance_id;
  msg->header.payload_type = payload_type;
  if (payload != 0)
  {
    msg->payload = *payload;
  }
  return APP_ALARM_RESULT_OK;
}

BaseType_t App_AlarmPost(const App_AlarmMsg_t *msg, TickType_t timeout_ticks)
{
  (void)timeout_ticks;

  if (msg == 0)
  {
    return pdFAIL;
  }

  if ((msg->header.source == APP_MODULE_SENSOR) && (msg->header.op == APP_ALARM_OP_RAISE))
  {
    s_sensor_alarm_msg = *msg;
    s_sensor_alarm_posted = 1U;
  }

  return pdPASS;
}

uint16_t App_AlarmGetActiveCount(void)
{
  return 0U;
}

App_AlarmResult_t App_AlarmGetRecord(uint16_t index, App_AlarmRecord_t *record)
{
  (void)index;
  (void)record;
  return APP_ALARM_RESULT_NOT_FOUND;
}

App_ErrorCode_t App_AlarmGetCurrentCode(void)
{
  return ERR_OK;
}

App_AlarmSeverity_t App_AlarmGetCurrentSeverity(void)
{
  return APP_ALARM_SEVERITY_NORMAL;
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

Display_Result_t Display_SelfCheck(App_ErrorCode_t *code)
{
  if (code != 0)
  {
    *code = ERR_OK;
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
    status_list[0].code = ERR_SENSOR_NO_FIX;
    status_list[0].driver_error = 0x33U;
  }

  return SENSOR_SEVERITY_GENERAL;
}

Sensor_Severity_t Sensor_RegistryReadAll(Sensor_Sample_t *samples, uint16_t max_sample_count, uint16_t *out_sample_count, Sensor_Status_t *status_list, uint16_t max_status_count, uint16_t *out_status_count)
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
  return 1U;
}

const Sensor_Driver_t *Sensor_RegistryGetDriver(uint16_t index)
{
  return (index == 0U) ? &s_test_sensor_driver : 0;
}

int main(void)
{
  int failures = 0;

  failures += expect_true(App_RegistrySelfCheckAll() == APP_CHECK_GENERAL,
                          "sensor GNSS no-fix is reported as general");
  failures += expect_true(s_sensor_status_set != 0U, "sensor module status is updated");
  failures += expect_true(s_sensor_status_state == APP_STATE_WARNING, "sensor module status is warning");
  failures += expect_true(s_sensor_status_code == ERR_SENSOR_NO_FIX, "sensor module status uses unified sensor error");
  failures += expect_true(s_sensor_alarm_posted != 0U, "sensor alarm message is posted");
  failures += expect_true(s_sensor_alarm_msg.header.op == APP_ALARM_OP_RAISE, "sensor alarm is raise op");
  failures += expect_true(s_sensor_alarm_msg.header.source == APP_MODULE_SENSOR, "sensor module is alarm source");
  failures += expect_true(s_sensor_alarm_msg.header.code == ERR_SENSOR_NO_FIX, "sensor error code is passed through");
  failures += expect_true(s_sensor_alarm_msg.header.instance_id == 1U, "sensor id is the alarm instance id");
  failures += expect_true(s_sensor_alarm_msg.header.payload_type == APP_ALARM_PAYLOAD_SENSOR, "sensor payload type is used");
  failures += expect_true(s_sensor_alarm_msg.payload.sensor.sensor_id == 1U, "sensor id is in payload");
  failures += expect_true(s_sensor_alarm_msg.payload.sensor.sensor_type == SENSOR_TYPE_GNSS, "sensor type is in payload");
  failures += expect_true(strcmp(s_sensor_alarm_msg.payload.sensor.name, "GNSS-1") == 0,
                          "sensor name is in payload");
  failures += expect_true(s_sensor_alarm_msg.payload.sensor.detail == 0x33U, "sensor driver detail is in payload");

  if (failures != 0)
  {
    (void)printf("%d failure(s)\n", failures);
    return 1;
  }

  (void)printf("app registry sensor alarm tests passed\n");
  return 0;
}
