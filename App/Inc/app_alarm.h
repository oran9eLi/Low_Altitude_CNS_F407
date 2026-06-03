#ifndef APP_ALARM_H
#define APP_ALARM_H

#include <stdint.h>
#include "FreeRTOS.h"
#include "app_error.h"
#include "app_status.h"

typedef enum {
  APP_ALARM_RESULT_OK = 0,
  APP_ALARM_RESULT_INVALID_ID,
  APP_ALARM_RESULT_INVALID_PARAM,
  APP_ALARM_RESULT_QUEUE_FAILED
} App_AlarmResult_t;

typedef enum {
  APP_ALARM_SEVERITY_NORMAL = 0,
  APP_ALARM_SEVERITY_GENERAL,
  APP_ALARM_SEVERITY_IMPORTANT,
  APP_ALARM_SEVERITY_CRITICAL
} App_AlarmSeverity_t;

typedef enum {
  APP_ALARM_MSG_RAISE = 0,
  APP_ALARM_MSG_CLEAR
} App_AlarmMsgType_t;

typedef enum {
  APP_ALARM_PAYLOAD_NONE = 0,
  APP_ALARM_PAYLOAD_RAW,
  APP_ALARM_PAYLOAD_SENSOR,
  APP_ALARM_PAYLOAD_COMM,
  APP_ALARM_PAYLOAD_STORAGE,
  APP_ALARM_PAYLOAD_SYSTEM
} App_AlarmPayloadType_t;

typedef enum {
  APP_ALARM_NONE = 0,
  APP_ALARM_SELF_CHECK_FAILED,
  APP_ALARM_HMI_OFFLINE,
  APP_ALARM_GNSS_OFFLINE,
  APP_ALARM_GNSS_NO_FIX,
  APP_ALARM_SENSOR_FAULT,
  APP_ALARM_LORA_OFFLINE,
  APP_ALARM_STORAGE_FAILED,
  APP_ALARM_POWER_LOW,
  APP_ALARM_MOTOR_FAULT,
  APP_ALARM_LOG_STORAGE_FAILED,
  APP_ALARM_HEAP_FAILED,
  APP_ALARM_STACK_OVERFLOW,
  APP_ALARM_COUNT
} App_AlarmId_t;

typedef struct
{
  App_AlarmMsgType_t msg_type;
  App_AlarmId_t alarm_id;
  App_ModuleId_t source;
  App_ErrorCode_t error_code;
  App_AlarmPayloadType_t payload_type;
  uint32_t timestamp_ms;
} App_AlarmMsgHeader_t;

typedef struct
{
  uint16_t device_id;
  uint16_t sensor_type;
  uint32_t driver_error;
} App_AlarmSensorPayload_t;

typedef struct
{
  uint8_t link_id;
  uint32_t timeout_ms;
  uint32_t link_error;
} App_AlarmCommPayload_t;

typedef struct
{
  uint32_t storage_error;
  uint32_t file_index;
} App_AlarmStoragePayload_t;

typedef struct
{
  uint32_t hook_code;
  uint32_t task_id;
} App_AlarmSystemPayload_t;

typedef union
{
  uint32_t raw_detail;
  App_AlarmSensorPayload_t sensor;
  App_AlarmCommPayload_t comm;
  App_AlarmStoragePayload_t storage;
  App_AlarmSystemPayload_t system;
} App_AlarmPayload_t;

typedef struct
{
  App_AlarmMsgHeader_t header;
  App_AlarmPayload_t payload;
} App_AlarmMsg_t;

typedef struct
{
  App_AlarmId_t alarm_id;
  App_ModuleId_t source;
  App_ErrorCode_t error_code;
  App_AlarmSeverity_t severity;
  App_AlarmPayloadType_t payload_type;
  App_AlarmPayload_t payload;
  uint32_t detail;
  uint32_t count;
  uint8_t active;
} App_AlarmRecord_t;

void App_AlarmInit(void);
App_AlarmResult_t App_AlarmBuildRaiseMsg(App_AlarmMsg_t *msg,
                                         App_AlarmId_t alarm_id,
                                         App_ModuleId_t source,
                                         App_ErrorCode_t error_code,
                                         App_AlarmPayloadType_t payload_type,
                                         const App_AlarmPayload_t *payload);
App_AlarmResult_t App_AlarmBuildRaiseDefaultMsg(App_AlarmMsg_t *msg,
                                                App_AlarmId_t alarm_id,
                                                App_ModuleId_t source,
                                                App_AlarmPayloadType_t payload_type,
                                                const App_AlarmPayload_t *payload);
App_AlarmResult_t App_AlarmBuildClearMsg(App_AlarmMsg_t *msg, App_AlarmId_t alarm_id, App_ModuleId_t source);
BaseType_t App_AlarmPost(const App_AlarmMsg_t *msg, TickType_t timeout_ticks);
BaseType_t App_AlarmPostFromISR(const App_AlarmMsg_t *msg, BaseType_t *higher_priority_task_woken);
App_AlarmResult_t App_AlarmRaise(App_AlarmId_t alarm_id, App_ModuleId_t source, App_ErrorCode_t error_code, uint32_t detail);
App_AlarmResult_t App_AlarmRaiseDefault(App_AlarmId_t alarm_id, App_ModuleId_t source, uint32_t detail);
App_AlarmResult_t App_AlarmRaiseImmediate(App_AlarmId_t alarm_id, App_ModuleId_t source, App_ErrorCode_t error_code, uint32_t detail);
App_AlarmResult_t App_AlarmClear(App_AlarmId_t alarm_id);
uint32_t App_AlarmGetActiveMask(void);
App_AlarmResult_t App_AlarmGetRecord(App_AlarmId_t alarm_id, App_AlarmRecord_t *record);
App_ErrorCode_t App_AlarmGetCurrentError(void);
uint32_t App_AlarmProcessPending(uint32_t max_messages);
void App_AlarmTask(void *argument);

#endif
