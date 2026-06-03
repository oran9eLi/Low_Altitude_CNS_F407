#include "app_alarm.h"
#include "app_status.h"
#include "queue.h"
#include "task.h"
/***************DEBUG***************/
#include "debug_trace.h"
/*************DEBUG END*************/
#include <string.h>

#define APP_ALARM_QUEUE_LENGTH 8U
#define APP_ALARM_TASK_PERIOD_MS 200U

/**
 * @brief 当前激活告警位图。
 *
 * 当 `APP_ALARM_xxx == N` 的告警处于激活状态时，第 N 位被置 1。
 * 该位图只作为运行时快速索引使用；HMI、日志和上报必须使用
 * `s_alarm_records[]` 中保存的标准错误码。
 */
static uint32_t s_alarm_mask;

/**
 * @brief 按 App_AlarmId_t 索引的运行时告警记录表。
 *
 * 每个告警项固定对应一条记录。触发告警时更新记录内容并设置
 * `s_alarm_mask` 中的对应位；清除告警时只清除 active 标志和位图位，
 * 保留 count、detail 等历史信息用于后续诊断。
 */
static App_AlarmRecord_t s_alarm_records[APP_ALARM_COUNT];

/**
 * @brief 告警事件队列。
 *
 * 普通模块只向该队列投递 `App_AlarmMsg_t`，由告警任务统一消费并更新
 * `s_alarm_mask` 和 `s_alarm_records[]`。
 */
static QueueHandle_t s_alarm_queue;

static App_AlarmResult_t App_AlarmApplyRaise(App_AlarmId_t alarm_id, App_ModuleId_t source, App_ErrorCode_t error_code, App_AlarmPayloadType_t payload_type, const App_AlarmPayload_t *payload);
static App_AlarmResult_t App_AlarmApplyClear(App_AlarmId_t alarm_id);
static App_AlarmResult_t App_AlarmDispatchMsg(const App_AlarmMsg_t *msg);
static App_ErrorCode_t App_AlarmGetDefaultError(App_AlarmId_t alarm_id);
static App_AlarmSeverity_t App_AlarmGetDefaultSeverity(App_AlarmId_t alarm_id);
static uint32_t App_AlarmGetPayloadDetail(App_AlarmPayloadType_t payload_type, const App_AlarmPayload_t *payload);
static App_AlarmResult_t App_AlarmPostResult(BaseType_t post_result);
static uint8_t App_AlarmIsValidPayload(App_AlarmPayloadType_t payload_type, const App_AlarmPayload_t *payload);
static uint8_t App_AlarmIsValidMsg(const App_AlarmMsg_t *msg);

/**
 * @brief 检查告警 ID 是否可作为记录索引和位图位号使用。
 *
 * @param alarm_id 待检查的告警 ID。
 * @retval 1U 告警 ID 有效。
 * @retval 0U 告警 ID 无效。
 */
static uint8_t App_AlarmIsValidId(App_AlarmId_t alarm_id);

/**
 * @brief 初始化告警模块运行时状态。
 *
 * 清空激活告警位图，并将所有告警记录复位为未激活状态。
 * 创建告警事件队列。后续普通告警入口只负责投递消息，由告警任务消费。
 * 在触发或查询任何告警前必须先调用本函数。
 */
void App_AlarmInit(void)
{
  uint32_t i;

  s_alarm_mask = 0U;

  for (i = 0U; i < (uint32_t)APP_ALARM_COUNT; i++)
  {
    s_alarm_records[i].alarm_id = (App_AlarmId_t)i;
    s_alarm_records[i].source = APP_MODULE_SYSTEM;
    s_alarm_records[i].error_code = APP_ERROR_OK;
    s_alarm_records[i].severity = APP_ALARM_SEVERITY_NORMAL;
    s_alarm_records[i].payload_type = APP_ALARM_PAYLOAD_NONE;
    (void)memset(&s_alarm_records[i].payload, 0, sizeof(s_alarm_records[i].payload));
    s_alarm_records[i].detail = 0U;
    s_alarm_records[i].count = 0U;
    s_alarm_records[i].active = 0U;
  }

  s_alarm_queue = xQueueCreate(APP_ALARM_QUEUE_LENGTH, sizeof(App_AlarmMsg_t));
}

/**
 * @brief 构造携带显式错误码的告警触发消息。
 *
 * 所有普通告警事件都统一通过 `App_AlarmMsg_t` 进入队列。调用方可以
 * 使用本函数填充消息，避免手写消息头字段造成不一致。
 *
 * @param msg 调用方提供的消息输出缓冲区。
 * @param alarm_id 要激活的内部告警项。
 * @param source 触发告警的模块。
 * @param error_code 标准对外错误码，不能为 `APP_ERROR_OK`。
 * @param payload_type payload 类型。
 * @param payload payload 数据。`payload_type` 为 NONE 时可以为 NULL。
 * @retval APP_ALARM_RESULT_OK 消息已构造。
 * @retval APP_ALARM_RESULT_INVALID_ID 告警 ID 无效。
 * @retval APP_ALARM_RESULT_INVALID_PARAM 参数无效。
 */
App_AlarmResult_t App_AlarmBuildRaiseMsg(App_AlarmMsg_t *msg, App_AlarmId_t alarm_id, App_ModuleId_t source, App_ErrorCode_t error_code, App_AlarmPayloadType_t payload_type, const App_AlarmPayload_t *payload)
{
  if (msg == NULL)
  {
    return APP_ALARM_RESULT_INVALID_PARAM;
  }

  if (App_AlarmIsValidId(alarm_id) == 0U)
  {
    return APP_ALARM_RESULT_INVALID_ID;
  }

  if ((error_code == APP_ERROR_OK) || (App_AlarmIsValidPayload(payload_type, payload) == 0U))
  {
    return APP_ALARM_RESULT_INVALID_PARAM;
  }

  (void)memset(msg, 0, sizeof(*msg));
  msg->header.msg_type = APP_ALARM_MSG_RAISE;
  msg->header.alarm_id = alarm_id;
  msg->header.source = source;
  msg->header.error_code = error_code;
  msg->header.payload_type = payload_type;
  msg->header.timestamp_ms = 0U;

  if (payload != NULL)
  {
    msg->payload = *payload;
  }

  return APP_ALARM_RESULT_OK;
}

/**
 * @brief 构造使用默认错误码的告警触发消息。
 *
 * 当调用方只能确定告警项、没有更具体标准错误码时使用本函数。
 * 默认错误码由告警模块内部表统一维护。
 *
 * @param msg 调用方提供的消息输出缓冲区。
 * @param alarm_id 要激活的内部告警项。
 * @param source 触发告警的模块。
 * @param payload_type payload 类型。
 * @param payload payload 数据。`payload_type` 为 NONE 时可以为 NULL。
 * @retval APP_ALARM_RESULT_OK 消息已构造。
 * @retval APP_ALARM_RESULT_INVALID_ID 告警 ID 无效。
 * @retval APP_ALARM_RESULT_INVALID_PARAM 参数无效。
 */
App_AlarmResult_t App_AlarmBuildRaiseDefaultMsg(App_AlarmMsg_t *msg, App_AlarmId_t alarm_id, App_ModuleId_t source, App_AlarmPayloadType_t payload_type, const App_AlarmPayload_t *payload)
{
  if (App_AlarmIsValidId(alarm_id) == 0U)
  {
    return APP_ALARM_RESULT_INVALID_ID;
  }

  return App_AlarmBuildRaiseMsg(msg, alarm_id, source, App_AlarmGetDefaultError(alarm_id), payload_type, payload);
}

/**
 * @brief 构造告警清除消息。
 *
 * @param msg 调用方提供的消息输出缓冲区。
 * @param alarm_id 要清除的内部告警项。
 * @param source 发起清除的模块。
 * @retval APP_ALARM_RESULT_OK 消息已构造。
 * @retval APP_ALARM_RESULT_INVALID_ID 告警 ID 无效。
 * @retval APP_ALARM_RESULT_INVALID_PARAM 参数无效。
 */
App_AlarmResult_t App_AlarmBuildClearMsg(App_AlarmMsg_t *msg, App_AlarmId_t alarm_id, App_ModuleId_t source)
{
  if (msg == NULL)
  {
    return APP_ALARM_RESULT_INVALID_PARAM;
  }

  if (App_AlarmIsValidId(alarm_id) == 0U)
  {
    return APP_ALARM_RESULT_INVALID_ID;
  }

  (void)memset(msg, 0, sizeof(*msg));
  msg->header.msg_type = APP_ALARM_MSG_CLEAR;
  msg->header.alarm_id = alarm_id;
  msg->header.source = source;
  msg->header.error_code = APP_ERROR_OK;
  msg->header.payload_type = APP_ALARM_PAYLOAD_NONE;
  msg->header.timestamp_ms = 0U;

  return APP_ALARM_RESULT_OK;
}

/**
 * @brief 投递告警消息到告警队列。
 *
 * FreeRTOS 队列本身是线程安全的，本函数不额外加互斥锁。
 *
 * @param msg 待投递的告警消息。
 * @param timeout_ticks 队列满时等待的 tick 数。
 * @retval pdPASS 投递成功。
 * @retval pdFAIL 投递失败。
 */
BaseType_t App_AlarmPost(const App_AlarmMsg_t *msg, TickType_t timeout_ticks)
{
  if ((s_alarm_queue == NULL) || (App_AlarmIsValidMsg(msg) == 0U))
  {
    return pdFAIL;
  }

  return xQueueSend(s_alarm_queue, msg, timeout_ticks);
}

/**
 * @brief 在中断上下文投递告警消息到告警队列。
 *
 * @param msg 待投递的告警消息。
 * @param higher_priority_task_woken FreeRTOS 中断唤醒标志。
 * @retval pdPASS 投递成功。
 * @retval pdFAIL 投递失败。
 */
BaseType_t App_AlarmPostFromISR(const App_AlarmMsg_t *msg, BaseType_t *higher_priority_task_woken)
{
  if ((s_alarm_queue == NULL) || (App_AlarmIsValidMsg(msg) == 0U))
  {
    return pdFAIL;
  }

  return xQueueSendFromISR(s_alarm_queue, msg, higher_priority_task_woken);
}

/**
 * @brief 触发或更新一条告警记录。
 *
 * 告警 ID 用于内部状态管理和 active mask 位号索引。
 * 错误码是面向状态、HMI、日志和上报的稳定对外编码。
 * `APP_ERROR_OK` 只表示无错误，不能作为告警错误码传入。
 *
 * @param alarm_id 要激活的内部告警项。
 * @param source 触发告警的模块。
 * @param error_code 标准对外错误码。
 * @param detail 内部诊断细节，例如驱动错误、HAL 返回值或子状态。
 * @retval APP_ALARM_RESULT_OK 告警已记录。
 * @retval APP_ALARM_RESULT_INVALID_ID 告警 ID 无法表示。
 * @retval APP_ALARM_RESULT_INVALID_PARAM 错误码无效。
 */
App_AlarmResult_t App_AlarmRaise(App_AlarmId_t alarm_id, App_ModuleId_t source, App_ErrorCode_t error_code, uint32_t detail)
{
  App_AlarmMsg_t msg;
  App_AlarmPayload_t payload;
  App_AlarmResult_t result;

  payload.raw_detail = detail;
  result = App_AlarmBuildRaiseMsg(&msg, alarm_id, source, error_code, APP_ALARM_PAYLOAD_RAW, &payload);
  if (result != APP_ALARM_RESULT_OK)
  {
    return result;
  }

  return App_AlarmPostResult(App_AlarmPost(&msg, 0U));
}

/**
 * @brief 使用告警项默认错误码触发或更新一条告警记录。
 *
 * 当调用方只能确定告警项，但没有更具体的标准错误码时，使用本接口。
 * 默认错误码由告警模块内部表统一维护，避免调用方重复维护映射关系。
 *
 * @param alarm_id 要激活的内部告警项。
 * @param source 触发告警的模块。
 * @param detail 内部诊断细节，例如驱动错误、HAL 返回值或子状态。
 * @retval APP_ALARM_RESULT_OK 告警已记录。
 * @retval APP_ALARM_RESULT_INVALID_ID 告警 ID 无法表示。
 */
App_AlarmResult_t App_AlarmRaiseDefault(App_AlarmId_t alarm_id, App_ModuleId_t source, uint32_t detail)
{
  App_AlarmMsg_t msg;
  App_AlarmPayload_t payload;
  App_AlarmResult_t result;

  payload.raw_detail = detail;
  result = App_AlarmBuildRaiseDefaultMsg(&msg, alarm_id, source, APP_ALARM_PAYLOAD_RAW, &payload);
  if (result != APP_ALARM_RESULT_OK)
  {
    return result;
  }

  return App_AlarmPostResult(App_AlarmPost(&msg, 0U));
}

/**
 * @brief 立即写入告警记录。
 *
 * 本接口仅用于 malloc failed、stack overflow 等系统钩子。普通模块必须
 * 构造 `App_AlarmMsg_t` 并通过队列投递。
 *
 * @param alarm_id 要激活的内部告警项。
 * @param source 触发告警的模块。
 * @param error_code 标准对外错误码，不能为 `APP_ERROR_OK`。
 * @param detail 内部诊断细节。
 * @retval APP_ALARM_RESULT_OK 告警已记录。
 * @retval APP_ALARM_RESULT_INVALID_ID 告警 ID 无效。
 * @retval APP_ALARM_RESULT_INVALID_PARAM 错误码无效。
 */
App_AlarmResult_t App_AlarmRaiseImmediate(App_AlarmId_t alarm_id, App_ModuleId_t source, App_ErrorCode_t error_code, uint32_t detail)
{
  App_AlarmPayload_t payload;

  if (App_AlarmIsValidId(alarm_id) == 0U)
  {
    return APP_ALARM_RESULT_INVALID_ID;
  }

  if (error_code == APP_ERROR_OK)
  {
    return APP_ALARM_RESULT_INVALID_PARAM;
  }

  payload.raw_detail = detail;
  return App_AlarmApplyRaise(alarm_id, source, error_code, APP_ALARM_PAYLOAD_RAW, &payload);
}

/**
 * @brief 写入告警记录并更新模块状态。
 *
 * 本函数假定告警 ID 和错误码已经由公开 API 校验或解析完成。
 *
 * @param alarm_id 要激活的内部告警项。
 * @param source 触发告警的模块。
 * @param error_code 标准对外错误码。
 * @param payload_type payload 类型。
 * @param payload payload 数据。
 * @retval APP_ALARM_RESULT_OK 告警已记录。
 */
static App_AlarmResult_t App_AlarmApplyRaise(App_AlarmId_t alarm_id, App_ModuleId_t source, App_ErrorCode_t error_code, App_AlarmPayloadType_t payload_type, const App_AlarmPayload_t *payload)
{
  App_AlarmRecord_t *record;
  uint32_t detail;

  detail = App_AlarmGetPayloadDetail(payload_type, payload);

  taskENTER_CRITICAL();
  record = &s_alarm_records[(uint32_t)alarm_id];
  record->alarm_id = alarm_id;
  record->source = source;
  record->error_code = error_code;
  record->severity = App_AlarmGetDefaultSeverity(alarm_id);
  record->payload_type = payload_type;
  (void)memset(&record->payload, 0, sizeof(record->payload));
  if (payload != NULL)
  {
    record->payload = *payload;
  }
  record->detail = detail;
  record->count++;
  record->active = 1U;

  s_alarm_mask |= (1UL << (uint32_t)alarm_id);
  taskEXIT_CRITICAL();

  App_StatusSet(source, APP_STATE_ERROR, error_code);

  return APP_ALARM_RESULT_OK;
}

/**
 * @brief 清除告警记录的激活状态。
 *
 * @param alarm_id 要清除的内部告警项。
 * @retval APP_ALARM_RESULT_OK 告警激活状态已清除。
 * @retval APP_ALARM_RESULT_INVALID_ID 告警 ID 无效。
 */
static App_AlarmResult_t App_AlarmApplyClear(App_AlarmId_t alarm_id)
{
  if (App_AlarmIsValidId(alarm_id) == 0U)
  {
    return APP_ALARM_RESULT_INVALID_ID;
  }

  taskENTER_CRITICAL();
  s_alarm_mask &= ~(1UL << (uint32_t)alarm_id);
  s_alarm_records[(uint32_t)alarm_id].active = 0U;
  taskEXIT_CRITICAL();

  return APP_ALARM_RESULT_OK;
}

/**
 * @brief 分发并应用一条告警消息。
 *
 * @param msg 已从队列取出的告警消息。
 * @retval APP_ALARM_RESULT_OK 消息已处理。
 * @retval APP_ALARM_RESULT_INVALID_PARAM 消息无效。
 */
static App_AlarmResult_t App_AlarmDispatchMsg(const App_AlarmMsg_t *msg)
{
  if (App_AlarmIsValidMsg(msg) == 0U)
  {
    return APP_ALARM_RESULT_INVALID_PARAM;
  }

  switch (msg->header.msg_type)
  {
  case APP_ALARM_MSG_RAISE:
    return App_AlarmApplyRaise(msg->header.alarm_id, msg->header.source, msg->header.error_code, msg->header.payload_type, &msg->payload);
  case APP_ALARM_MSG_CLEAR:
    return App_AlarmApplyClear(msg->header.alarm_id);
  default:
    return APP_ALARM_RESULT_INVALID_PARAM;
  }
}

/**
 * @brief 清除一个激活告警项。
 *
 * 普通清除请求会先进入告警队列，实际 active mask 和记录状态由告警任务
 * 统一更新。count、detail 等历史字段在清除后仍保留。
 *
 * @param alarm_id 要清除的内部告警项。
 * @retval APP_ALARM_RESULT_OK 告警激活状态已清除。
 * @retval APP_ALARM_RESULT_INVALID_ID 告警 ID 无法表示。
 */
App_AlarmResult_t App_AlarmClear(App_AlarmId_t alarm_id)
{
  App_AlarmMsg_t msg;
  App_AlarmResult_t result = App_AlarmBuildClearMsg(&msg, alarm_id, APP_MODULE_ALARM);

  if (result != APP_ALARM_RESULT_OK)
  {
    return result;
  }

  return App_AlarmPostResult(App_AlarmPost(&msg, 0U));
}

/**
 * @brief 获取当前激活告警位图。
 *
 * @return 告警位图，第 N 位表示告警 ID N 是否处于激活状态。
 */
uint32_t App_AlarmGetActiveMask(void)
{
  uint32_t active_mask;

  taskENTER_CRITICAL();
  active_mask = s_alarm_mask;
  taskEXIT_CRITICAL();

  return active_mask;
}

/**
 * @brief 获取指定告警项的运行时记录副本。
 *
 * @param alarm_id 要查询的告警 ID。
 * @param record 调用方提供的记录输出缓冲区。
 * @retval APP_ALARM_RESULT_OK 记录已复制到输出缓冲区。
 * @retval APP_ALARM_RESULT_INVALID_ID 告警 ID 无效。
 * @retval APP_ALARM_RESULT_INVALID_PARAM 输出缓冲区为空。
 */
App_AlarmResult_t App_AlarmGetRecord(App_AlarmId_t alarm_id, App_AlarmRecord_t *record)
{
  if (record == 0)
  {
    return APP_ALARM_RESULT_INVALID_PARAM;
  }

  if (App_AlarmIsValidId(alarm_id) == 0U)
  {
    return APP_ALARM_RESULT_INVALID_ID;
  }

  taskENTER_CRITICAL();
  *record = s_alarm_records[(uint32_t)alarm_id];
  taskEXIT_CRITICAL();

  return APP_ALARM_RESULT_OK;
}

/**
 * @brief 从当前激活告警中选择一个对外错误码。
 *
 * 当前错误码按告警等级从激活记录中选择。多个激活告警等级相同时，
 * 因扫描条件使用 `>=`，编号靠后的告警会覆盖前一个结果。
 * 这样在 HMI/日志策略尚未复杂化前，仍能保持确定性。
 *
 * @return 当前优先级最高的标准错误码；无激活告警时返回 APP_ERROR_OK。
 */
App_ErrorCode_t App_AlarmGetCurrentError(void)
{
  App_AlarmSeverity_t highest_severity = APP_ALARM_SEVERITY_NORMAL;
  App_ErrorCode_t current_error = APP_ERROR_OK;
  uint32_t i;

  for (i = 1U; i < (uint32_t)APP_ALARM_COUNT; i++)
  {
    App_AlarmRecord_t record;

    taskENTER_CRITICAL();
    record = s_alarm_records[i];
    taskEXIT_CRITICAL();

    if ((record.active != 0U) && (record.severity >= highest_severity))
    {
      highest_severity = record.severity;
      current_error = record.error_code;
    }
  }

  return current_error;
}

/**
 * @brief 处理告警队列中等待的消息。
 *
 * @param max_messages 本次最多处理的消息数；传 0 表示处理到队列为空。
 * @return 实际处理的消息数量。
 */
uint32_t App_AlarmProcessPending(uint32_t max_messages)
{
  App_AlarmMsg_t msg;
  uint32_t processed = 0U;

  while ((max_messages == 0U) || (processed < max_messages))
  {
    if ((s_alarm_queue == NULL) || (xQueueReceive(s_alarm_queue, &msg, 0U) != pdPASS))
    {
      break;
    }

    (void)App_AlarmDispatchMsg(&msg);
    processed++;
  }

  return processed;
}

/**
 * @brief 告警任务入口。
 *
 * 周期性消费告警队列，并维护告警模块心跳。
 *
 * @param argument FreeRTOS 任务参数，当前未使用。
 */
void App_AlarmTask(void *argument)
{
  (void)argument;
  App_StatusSet(APP_MODULE_ALARM, APP_STATE_OK, APP_ERROR_OK);

  /***************DEBUG***************/
  DBG_TRACE_TASK_STARTED("alarm");
  /*************DEBUG END*************/

  for (;;)
  {
    (void)App_AlarmProcessPending(0U);
    App_StatusHeartbeat(APP_MODULE_ALARM);
    vTaskDelay(pdMS_TO_TICKS(APP_ALARM_TASK_PERIOD_MS));
  }
}

/**
 * @brief 获取告警项默认对应的标准错误码。
 *
 * @param alarm_id 内部告警项。
 * @return 告警项对应的标准对外错误码。
 */
static App_ErrorCode_t App_AlarmGetDefaultError(App_AlarmId_t alarm_id)
{
  switch (alarm_id)
  {
  case APP_ALARM_HMI_OFFLINE:
    return APP_ERROR_HMI_OFFLINE;
  case APP_ALARM_GNSS_OFFLINE:
    return APP_ERROR_GNSS_OFFLINE;
  case APP_ALARM_GNSS_NO_FIX:
    return APP_ERROR_GNSS_NO_FIX;
  case APP_ALARM_SENSOR_FAULT:
    return APP_ERROR_SENSOR_I2C;
  case APP_ALARM_LORA_OFFLINE:
    return APP_ERROR_LORA_OFFLINE;
  case APP_ALARM_STORAGE_FAILED:
  case APP_ALARM_LOG_STORAGE_FAILED:
    return APP_ERROR_SD_MOUNT;
  case APP_ALARM_POWER_LOW:
    return APP_ERROR_POWER_LOW;
  case APP_ALARM_MOTOR_FAULT:
    return APP_ERROR_MOTOR_FAULT;
  case APP_ALARM_HEAP_FAILED:
    return APP_ERROR_HEAP_FAILED;
  case APP_ALARM_STACK_OVERFLOW:
    return APP_ERROR_STACK_OVERFLOW;
  case APP_ALARM_SELF_CHECK_FAILED:
    return APP_ERROR_SELF_CHECK_FAILED;
  case APP_ALARM_NONE:
  case APP_ALARM_COUNT:
  default:
    return APP_ERROR_UNKNOWN;
  }
}

/**
 * @brief 获取告警项默认等级。
 *
 * @param alarm_id 内部告警项。
 * @return 触发该告警时默认使用的告警等级。
 */
static App_AlarmSeverity_t App_AlarmGetDefaultSeverity(App_AlarmId_t alarm_id)
{
  switch (alarm_id)
  {
  case APP_ALARM_POWER_LOW:
  case APP_ALARM_LORA_OFFLINE:
  case APP_ALARM_STORAGE_FAILED:
  case APP_ALARM_LOG_STORAGE_FAILED:
    return APP_ALARM_SEVERITY_IMPORTANT;
  case APP_ALARM_HEAP_FAILED:
  case APP_ALARM_STACK_OVERFLOW:
  case APP_ALARM_MOTOR_FAULT:
    return APP_ALARM_SEVERITY_CRITICAL;
  case APP_ALARM_HMI_OFFLINE:
  case APP_ALARM_GNSS_OFFLINE:
  case APP_ALARM_GNSS_NO_FIX:
  case APP_ALARM_SENSOR_FAULT:
  case APP_ALARM_SELF_CHECK_FAILED:
    return APP_ALARM_SEVERITY_GENERAL;
  case APP_ALARM_NONE:
  case APP_ALARM_COUNT:
  default:
    return APP_ALARM_SEVERITY_NORMAL;
  }
}

/**
 * @brief 从 payload 中提取兼容旧记录字段的 detail。
 *
 * @param payload_type payload 类型。
 * @param payload payload 数据。
 * @return 可用于日志和诊断的简化 detail。
 */
static uint32_t App_AlarmGetPayloadDetail(App_AlarmPayloadType_t payload_type, const App_AlarmPayload_t *payload)
{
  if (payload == NULL)
  {
    return 0U;
  }

  switch (payload_type)
  {
  case APP_ALARM_PAYLOAD_RAW:
    return payload->raw_detail;
  case APP_ALARM_PAYLOAD_SENSOR:
    return payload->sensor.driver_error;
  case APP_ALARM_PAYLOAD_COMM:
    return payload->comm.link_error;
  case APP_ALARM_PAYLOAD_STORAGE:
    return payload->storage.storage_error;
  case APP_ALARM_PAYLOAD_SYSTEM:
    return payload->system.hook_code;
  case APP_ALARM_PAYLOAD_NONE:
  default:
    return 0U;
  }
}

/**
 * @brief 将队列投递结果转换为告警 API 结果。
 *
 * @param post_result FreeRTOS 队列投递结果。
 * @retval APP_ALARM_RESULT_OK 投递成功。
 * @retval APP_ALARM_RESULT_QUEUE_FAILED 投递失败。
 */
static App_AlarmResult_t App_AlarmPostResult(BaseType_t post_result)
{
  return (post_result == pdPASS) ? APP_ALARM_RESULT_OK : APP_ALARM_RESULT_QUEUE_FAILED;
}

/**
 * @brief 校验 payload 类型和数据指针是否匹配。
 *
 * @param payload_type payload 类型。
 * @param payload payload 数据。
 * @retval 1U payload 有效。
 * @retval 0U payload 无效。
 */
static uint8_t App_AlarmIsValidPayload(App_AlarmPayloadType_t payload_type, const App_AlarmPayload_t *payload)
{
  switch (payload_type)
  {
  case APP_ALARM_PAYLOAD_NONE:
    return 1U;
  case APP_ALARM_PAYLOAD_RAW:
  case APP_ALARM_PAYLOAD_SENSOR:
  case APP_ALARM_PAYLOAD_COMM:
  case APP_ALARM_PAYLOAD_STORAGE:
  case APP_ALARM_PAYLOAD_SYSTEM:
    return (payload != NULL) ? 1U : 0U;
  default:
    return 0U;
  }
}

/**
 * @brief 校验告警消息是否可投递或消费。
 *
 * @param msg 告警消息。
 * @retval 1U 消息有效。
 * @retval 0U 消息无效。
 */
static uint8_t App_AlarmIsValidMsg(const App_AlarmMsg_t *msg)
{
  if ((msg == NULL) || (App_AlarmIsValidId(msg->header.alarm_id) == 0U))
  {
    return 0U;
  }

  switch (msg->header.msg_type)
  {
  case APP_ALARM_MSG_RAISE:
    return (msg->header.error_code != APP_ERROR_OK) ? App_AlarmIsValidPayload(msg->header.payload_type, &msg->payload) : 0U;
  case APP_ALARM_MSG_CLEAR:
    return 1U;
  default:
    return 0U;
  }
}

/**
 * @brief 校验告警 ID 是否可用于位图和记录表访问。
 *
 * `APP_ALARM_NONE` 不是可触发告警。当前 active mask 为 32 位，
 * 因此在位图存储方式调整前，告警 ID 必须小于 32。
 *
 * @param alarm_id 待校验的告警 ID。
 * @retval 1U 告警 ID 有效。
 * @retval 0U 告警 ID 无效。
 */
static uint8_t App_AlarmIsValidId(App_AlarmId_t alarm_id)
{
  return ((alarm_id > APP_ALARM_NONE) && (alarm_id < APP_ALARM_COUNT) && ((uint32_t)alarm_id < 32U)) ? 1U : 0U;
}
