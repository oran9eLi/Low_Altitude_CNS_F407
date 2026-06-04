#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "app_alarm.h"

static App_ModuleId_t s_last_module;
static App_ModuleState_t s_last_state;
static App_ErrorCode_t s_last_code;

void App_StatusSet(App_ModuleId_t module, App_ModuleState_t state, App_ErrorCode_t code)
{
  s_last_module = module;
  s_last_state = state;
  s_last_code = code;
}

void App_StatusHeartbeat(App_ModuleId_t module)
{
  (void)module;
}

void DebugConsole_TaskInfo(const char *name, const char *fmt, ...)
{
  (void)name;
  (void)fmt;
}

static int expect_true(int condition, const char *message)
{
  if (!condition)
  {
    (void)printf("FAIL: %s\n", message);
    return 1;
  }

  return 0;
}

int main(void)
{
  App_AlarmRecord_t record;
  App_AlarmMsg_t msg;
  App_AlarmPayload_t payload;
  int failures = 0;

  App_AlarmInit();
  failures += expect_true(App_AlarmGetActiveCount() == 0U, "active table starts empty");
  failures += expect_true(App_AlarmGetCurrentCode() == ERR_OK, "current code starts OK");
  failures += expect_true(App_AlarmBuildMsg(&msg, APP_ALARM_OP_RAISE, APP_MODULE_STORAGE, APP_ALARM_SEVERITY_IMPORTANT, ERR_OK, 0U, APP_ALARM_PAYLOAD_NONE, NULL) == APP_ALARM_RESULT_INVALID_PARAM, "raise rejects OK code");

  payload.raw_detail = 7U;
  failures += expect_true(App_AlarmBuildMsg(&msg, APP_ALARM_OP_RAISE, APP_MODULE_STORAGE, APP_ALARM_SEVERITY_IMPORTANT, ERR_STORAGE_NOT_READY, 0U, APP_ALARM_PAYLOAD_RAW, &payload) == APP_ALARM_RESULT_OK, "storage raise message is built");
  failures += expect_true(App_AlarmPost(&msg, 0U) == pdPASS, "storage alarm is posted");
  failures += expect_true(App_AlarmGetActiveCount() == 0U, "queued alarm does not update active table before processing");
  failures += expect_true(App_AlarmProcessPending(1U) == 1U, "one queued alarm is processed");

  failures += expect_true(App_AlarmGetActiveCount() == 1U, "one active record exists");
  failures += expect_true(App_AlarmGetCurrentCode() == ERR_STORAGE_NOT_READY, "current code exposes storage error");
  failures += expect_true(s_last_module == APP_MODULE_STORAGE, "status source is storage");
  failures += expect_true(s_last_state == APP_STATE_ERROR, "status state is error");
  failures += expect_true(s_last_code == ERR_STORAGE_NOT_READY, "status uses standard error code");

  failures += expect_true(App_AlarmGetRecord(0U, &record) == APP_ALARM_RESULT_OK, "record is copied out");
  failures += expect_true(record.source == APP_MODULE_STORAGE, "record source is stored");
  failures += expect_true(record.code == ERR_STORAGE_NOT_READY, "record code is stored");
  failures += expect_true(record.instance_id == 0U, "record instance id is stored");
  failures += expect_true(record.payload_type == APP_ALARM_PAYLOAD_RAW, "raw payload type is stored");
  failures += expect_true(record.payload.raw_detail == 7U, "raw payload is stored");
  failures += expect_true(record.active != 0U, "record is active");
  failures += expect_true(App_AlarmGetRecord(0U, NULL) == APP_ALARM_RESULT_INVALID_PARAM, "record output buffer is required");

  (void)memset(&payload, 0, sizeof(payload));
  payload.storage.target_id = 3U;
  payload.storage.operation = 1U;
  payload.storage.detail = 12U;
  failures += expect_true(App_AlarmBuildMsg(&msg, APP_ALARM_OP_RAISE, APP_MODULE_STORAGE, APP_ALARM_SEVERITY_IMPORTANT, ERR_STORAGE_NOT_READY, 0U, APP_ALARM_PAYLOAD_STORAGE, &payload) == APP_ALARM_RESULT_OK, "storage payload message is built");
  failures += expect_true(App_AlarmPost(&msg, 0U) == pdPASS, "storage payload alarm is posted");
  failures += expect_true(App_AlarmProcessPending(1U) == 1U, "storage payload alarm is processed");
  failures += expect_true(App_AlarmGetRecord(0U, &record) == APP_ALARM_RESULT_OK, "storage payload record is copied out");
  failures += expect_true(record.payload_type == APP_ALARM_PAYLOAD_STORAGE, "storage payload type is stored");
  failures += expect_true(record.payload.storage.detail == 12U, "storage detail payload is stored");
  failures += expect_true(record.first_tick_ms == record.last_tick_ms, "refresh keeps first timestamp in this test tick");

  failures += expect_true(App_AlarmBuildMsg(&msg, APP_ALARM_OP_CLEAR, APP_MODULE_STORAGE, APP_ALARM_SEVERITY_NORMAL, ERR_STORAGE_NOT_READY, 0U, APP_ALARM_PAYLOAD_NONE, NULL) == APP_ALARM_RESULT_OK, "clear message is built");
  failures += expect_true(App_AlarmPost(&msg, 0U) == pdPASS, "clear is posted");
  failures += expect_true(App_AlarmGetActiveCount() == 1U, "queued clear does not update table before processing");
  failures += expect_true(App_AlarmProcessPending(1U) == 1U, "one queued clear is processed");
  failures += expect_true(App_AlarmGetActiveCount() == 0U, "active table clears");
  failures += expect_true(App_AlarmGetCurrentCode() == ERR_OK, "current code returns OK after clear");

  if (failures != 0)
  {
    (void)printf("%d failure(s)\n", failures);
    return 1;
  }

  (void)printf("app_alarm tests passed\n");
  return 0;
}
