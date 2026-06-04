#ifndef APP_ALARM_H
#define APP_ALARM_H

#include <stdint.h>
#include "FreeRTOS.h"
#include "app_error.h"
#include "app_status.h"

#define APP_ALARM_NAME_LEN 16U

typedef enum {
  APP_ALARM_RESULT_OK = 0,
  APP_ALARM_RESULT_INVALID_PARAM,
  APP_ALARM_RESULT_QUEUE_FAILED,
  APP_ALARM_RESULT_NOT_FOUND,
  APP_ALARM_RESULT_NO_SLOT
} App_AlarmResult_t;

typedef enum {
  APP_ALARM_SEVERITY_NORMAL = 0,
  APP_ALARM_SEVERITY_GENERAL,
  APP_ALARM_SEVERITY_IMPORTANT,
  APP_ALARM_SEVERITY_CRITICAL
} App_AlarmSeverity_t;

typedef enum {
  APP_ALARM_OP_RAISE = 0,
  APP_ALARM_OP_CLEAR
} App_AlarmOp_t;

typedef enum {
  APP_ALARM_PAYLOAD_NONE = 0,
  APP_ALARM_PAYLOAD_SENSOR,
  APP_ALARM_PAYLOAD_COMM,
  APP_ALARM_PAYLOAD_STORAGE,
  APP_ALARM_PAYLOAD_SYSTEM,
  APP_ALARM_PAYLOAD_RAW
} App_AlarmPayloadType_t;

typedef struct
{
  uint16_t sensor_id;
  uint16_t sensor_type;
  char name[APP_ALARM_NAME_LEN];
  uint32_t detail;
} App_AlarmSensorPayload_t;

typedef struct
{
  uint16_t link_id;
  uint32_t timeout_ms;
  uint32_t detail;
} App_AlarmCommPayload_t;

typedef struct
{
  uint16_t target_id;
  uint32_t operation;
  uint32_t detail;
} App_AlarmStoragePayload_t;

typedef struct
{
  uint32_t hook_code;
  uint32_t task_id;
  uint32_t detail;
} App_AlarmSystemPayload_t;

typedef union {
  App_AlarmSensorPayload_t sensor;
  App_AlarmCommPayload_t comm;
  App_AlarmStoragePayload_t storage;
  App_AlarmSystemPayload_t system;
  uint32_t raw_detail;
} App_AlarmPayload_t;

typedef struct
{
  App_AlarmOp_t op;
  App_ModuleId_t source;
  App_AlarmSeverity_t severity;
  App_ErrorCode_t code;
  App_AlarmPayloadType_t payload_type;
  uint16_t instance_id;
  uint32_t timestamp_ms;
} App_AlarmMsgHeader_t;

typedef struct
{
  App_AlarmMsgHeader_t header;
  App_AlarmPayload_t payload;
} App_AlarmMsg_t;

typedef struct
{
  uint8_t active;
  App_ModuleId_t source;
  App_AlarmSeverity_t severity;
  App_ErrorCode_t code;
  App_AlarmPayloadType_t payload_type;
  uint16_t instance_id;
  uint32_t first_tick_ms;
  uint32_t last_tick_ms;
  App_AlarmPayload_t payload;
} App_AlarmRecord_t;

void App_AlarmInit(void);
App_AlarmResult_t App_AlarmBuildMsg(App_AlarmMsg_t *msg, App_AlarmOp_t op, App_ModuleId_t source, App_AlarmSeverity_t severity, App_ErrorCode_t code, uint16_t instance_id, App_AlarmPayloadType_t payload_type, const App_AlarmPayload_t *payload);
BaseType_t App_AlarmPost(const App_AlarmMsg_t *msg, TickType_t timeout_ticks);
BaseType_t App_AlarmPostFromISR(const App_AlarmMsg_t *msg, BaseType_t *higher_priority_task_woken);
uint32_t App_AlarmProcessPending(uint32_t max_messages);
void App_AlarmTask(void *argument);

uint16_t App_AlarmGetActiveCount(void);
App_AlarmResult_t App_AlarmGetRecord(uint16_t index, App_AlarmRecord_t *record);
uint16_t App_AlarmFindBySourceInstance(App_ModuleId_t source, uint16_t instance_id, App_AlarmRecord_t *records, uint16_t max_count);
App_ErrorCode_t App_AlarmGetCurrentCode(void);
App_AlarmSeverity_t App_AlarmGetCurrentSeverity(void);

#endif
