#include <stdint.h>
#include <stdio.h>

#include "app_alarm.h"

static App_ModuleId_t s_last_module;
static App_ModuleState_t s_last_state;
static App_ErrorCode_t s_last_error_code;

void App_StatusSet(App_ModuleId_t module, App_ModuleState_t state, App_ErrorCode_t error_code)
{
  s_last_module     = module;
  s_last_state      = state;
  s_last_error_code = error_code;
}

void App_StatusHeartbeat(App_ModuleId_t module)
{
  (void)module;
}

static int expect_true(int condition, const char *message)
{
  if (!condition) {
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
  failures += expect_true(App_AlarmGetActiveMask() == 0U, "alarm mask starts empty");
  failures += expect_true(App_AlarmGetCurrentError() == APP_ERROR_OK, "current error starts OK");
  failures += expect_true(App_AlarmRaise(APP_ALARM_STORAGE_FAILED,
                                         APP_MODULE_STORAGE,
                                         APP_ERROR_OK,
                                         1U) == APP_ALARM_RESULT_INVALID_PARAM,
                           "explicit raise rejects OK error code");

  failures += expect_true(App_AlarmRaiseDefault(APP_ALARM_STORAGE_FAILED,
                                                APP_MODULE_STORAGE,
                                                7U) == APP_ALARM_RESULT_OK,
                           "raising storage alarm with default error succeeds");
  failures += expect_true(App_AlarmGetActiveMask() == 0U, "queued alarm does not update mask before processing");
  failures += expect_true(App_AlarmProcessPending(1U) == 1U, "one queued alarm is processed");

  failures += expect_true((App_AlarmGetActiveMask() & (1UL << APP_ALARM_STORAGE_FAILED)) != 0U,
                           "storage alarm bit is active");
  failures += expect_true(App_AlarmGetCurrentError() == APP_ERROR_SD_MOUNT,
                           "current error exposes standard error code");
  failures += expect_true(s_last_module == APP_MODULE_STORAGE, "status source is storage");
  failures += expect_true(s_last_state == APP_STATE_ERROR, "status state is error");
  failures += expect_true(s_last_error_code == APP_ERROR_SD_MOUNT, "status uses standard error code");

  failures += expect_true(App_AlarmGetRecord(APP_ALARM_STORAGE_FAILED, &record) == APP_ALARM_RESULT_OK,
                           "record is copied out");

  failures += expect_true(record.alarm_id == APP_ALARM_STORAGE_FAILED, "record alarm id is stored");
  failures += expect_true(record.source == APP_MODULE_STORAGE, "record source is stored");
  failures += expect_true(record.error_code == APP_ERROR_SD_MOUNT, "record error code is stored");
  failures += expect_true(record.detail == 7U, "record detail is stored");
  failures += expect_true(record.payload_type == APP_ALARM_PAYLOAD_RAW, "raw payload type is stored");
  failures += expect_true(record.payload.raw_detail == 7U, "raw payload is stored");
  failures += expect_true(record.count == 1U, "record count increments");
  failures += expect_true(record.active != 0U, "record is active");
  failures += expect_true(App_AlarmGetRecord(APP_ALARM_STORAGE_FAILED, 0) == APP_ALARM_RESULT_INVALID_PARAM,
                           "record output buffer is required");

  payload.storage.storage_error = 12U;
  payload.storage.file_index = 3U;
  failures += expect_true(App_AlarmBuildRaiseDefaultMsg(&msg,
                                                        APP_ALARM_STORAGE_FAILED,
                                                        APP_MODULE_STORAGE,
                                                        APP_ALARM_PAYLOAD_STORAGE,
                                                        &payload) == APP_ALARM_RESULT_OK,
                           "default raise message is built");
  failures += expect_true(App_AlarmPost(&msg, 0U) == pdPASS, "storage payload alarm is posted");
  failures += expect_true(App_AlarmProcessPending(1U) == 1U, "storage payload alarm is processed");
  failures += expect_true(App_AlarmGetRecord(APP_ALARM_STORAGE_FAILED, &record) == APP_ALARM_RESULT_OK,
                           "storage payload record is copied out");
  failures += expect_true(record.payload_type == APP_ALARM_PAYLOAD_STORAGE, "storage payload type is stored");
  failures += expect_true(record.payload.storage.storage_error == 12U, "storage error payload is stored");
  failures += expect_true(record.payload.storage.file_index == 3U, "storage file index payload is stored");
  failures += expect_true(record.detail == 12U, "storage payload detail is derived");

  failures += expect_true(App_AlarmClear(APP_ALARM_STORAGE_FAILED) == APP_ALARM_RESULT_OK,
                           "clearing storage alarm succeeds");
  failures += expect_true((App_AlarmGetActiveMask() & (1UL << APP_ALARM_STORAGE_FAILED)) != 0U,
                           "queued clear does not update mask before processing");
  failures += expect_true(App_AlarmProcessPending(1U) == 1U, "one queued clear is processed");
  failures += expect_true(App_AlarmGetActiveMask() == 0U, "alarm mask clears");
  failures += expect_true(App_AlarmGetCurrentError() == APP_ERROR_OK, "current error returns OK after clear");

  if (failures != 0) {
    (void)printf("%d failure(s)\n", failures);
    return 1;
  }

  (void)printf("app_alarm tests passed\n");
  return 0;
}
