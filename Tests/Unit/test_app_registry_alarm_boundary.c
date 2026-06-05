#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "app_registry.h"
#include "app_alarm.h"
#include "app_logger.h"
#include "app_storage.h"
#include "display.h"
#include "sensor_registry.h"

static App_AlarmMsg_t s_last_alarm_msg;
static uint8_t s_alarm_posted;

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
  (void)module;
  (void)state;
  (void)code;
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

  if ((msg->header.source == APP_MODULE_LOGGER) && (msg->header.op == APP_ALARM_OP_RAISE))
  {
    s_last_alarm_msg = *msg;
    s_alarm_posted = 1U;
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
  return pdFAIL;
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

Sensor_Severity_t Sensor_RegistrySelfCheckAll(Sensor_Status_t *status_list, uint16_t max_count, uint16_t *out_count)
{
  (void)status_list;
  (void)max_count;

  if (out_count != 0)
  {
    *out_count = 0U;
  }

  return SENSOR_SEVERITY_NORMAL;
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

  failures += expect_true(App_RegistryInitAll() == APP_CHECK_CRITICAL,
                          "logger init failure is critical");
  failures += expect_true(s_alarm_posted != 0U, "logger init failure posts alarm message");
  failures += expect_true(s_last_alarm_msg.header.op == APP_ALARM_OP_RAISE, "logger init failure is raise op");
  failures += expect_true(s_last_alarm_msg.header.source == APP_MODULE_LOGGER, "alarm source is logger");
  failures += expect_true(s_last_alarm_msg.header.code == ERR_LOG_WRITE_FAILED, "logger init failure uses logger error code");
  failures += expect_true(s_last_alarm_msg.header.severity == APP_ALARM_SEVERITY_CRITICAL, "logger init failure severity is critical");
  failures += expect_true(s_last_alarm_msg.payload.raw_detail == (uint32_t)APP_CHECK_CRITICAL,
                          "alarm detail carries check result");

  if (failures != 0)
  {
    (void)printf("%d failure(s)\n", failures);
    return 1;
  }

  (void)printf("app registry alarm boundary tests passed\n");
  return 0;
}
