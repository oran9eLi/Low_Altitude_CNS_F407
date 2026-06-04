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
#define APP_ALARM_ACTIVE_MAX 16U
#define APP_ALARM_SYSTEM_INSTANCE_ID 0U
#define APP_ALARM_HOOK_MALLOC_FAILED 1U
#define APP_ALARM_HOOK_STACK_OVERFLOW 2U

/**
 * @brief 活动异常记录表。
 *
 * 表内只保存当前仍处于激活状态的异常。记录唯一键为
 * `source + code + instance_id`，用于区分不同模块、不同错误和不同设备实例。
 */
static App_AlarmRecord_t s_alarm_records[APP_ALARM_ACTIVE_MAX];

/**
 * @brief 告警消息队列。
 *
 * 普通模块统一构造 `App_AlarmMsg_t` 并投递到该队列，由告警任务集中更新
 * 活动异常记录表。
 */
static QueueHandle_t s_alarm_queue;

static App_AlarmResult_t App_AlarmDispatchMsg(const App_AlarmMsg_t *msg);
static App_AlarmResult_t App_AlarmApplyRaise(const App_AlarmMsg_t *msg);
static App_AlarmResult_t App_AlarmApplyClear(const App_AlarmMsg_t *msg);
static void App_AlarmRecordDirect(App_ModuleId_t source, App_AlarmSeverity_t severity, App_ErrorCode_t code, uint16_t instance_id, App_AlarmPayloadType_t payload_type, const App_AlarmPayload_t *payload);
static int16_t App_AlarmFindKeyLocked(App_ModuleId_t source, App_ErrorCode_t code, uint16_t instance_id);
static int16_t App_AlarmFindFreeLocked(void);
static int16_t App_AlarmFindReplacementLocked(App_AlarmSeverity_t severity);
static uint16_t App_AlarmCountActiveLocked(void);
static uint8_t App_AlarmSourceHasActiveLocked(App_ModuleId_t source, App_ErrorCode_t *code, App_AlarmSeverity_t *severity);
static void App_AlarmWriteRecordLocked(int16_t index, const App_AlarmMsg_t *msg, uint32_t now_ms, uint8_t is_new_record);
static uint8_t App_AlarmIsValidPayload(App_AlarmPayloadType_t payload_type, const App_AlarmPayload_t *payload);
static uint8_t App_AlarmIsValidMsg(const App_AlarmMsg_t *msg);

/**
 * @brief 初始化告警模块。
 *
 * 清空活动异常记录表并创建告警消息队列。后续普通异常必须通过
 * `App_AlarmPost()` 进入队列，由告警任务统一消费。
 */
void App_AlarmInit(void)
{
  (void)memset(s_alarm_records, 0, sizeof(s_alarm_records));
  s_alarm_queue = xQueueCreate(APP_ALARM_QUEUE_LENGTH, sizeof(App_AlarmMsg_t));
}

/**
 * @brief 构造统一异常状态变化消息。
 *
 * RAISE 表示异常进入或持续存在；CLEAR 表示指定异常键恢复。错误码放在消息头，
 * payload 只保存现场上下文。
 *
 * @param msg 输出消息缓冲区。
 * @param op 状态变化类型。
 * @param source 异常来源模块。
 * @param severity 严重程度。
 * @param code 统一错误码。
 * @param instance_id 来源模块内的实例编号。
 * @param payload_type payload 类型。
 * @param payload payload 数据，payload_type 为 NONE 时可为 NULL。
 * @return 构造结果。
 */
App_AlarmResult_t App_AlarmBuildMsg(App_AlarmMsg_t *msg, App_AlarmOp_t op, App_ModuleId_t source, App_AlarmSeverity_t severity, App_ErrorCode_t code, uint16_t instance_id, App_AlarmPayloadType_t payload_type, const App_AlarmPayload_t *payload)
{
  if (msg == NULL)
  {
    return APP_ALARM_RESULT_INVALID_PARAM;
  }

  (void)memset(msg, 0, sizeof(*msg));
  msg->header.op = op;
  msg->header.source = source;
  msg->header.severity = severity;
  msg->header.code = code;
  msg->header.payload_type = payload_type;
  msg->header.instance_id = instance_id;
  msg->header.timestamp_ms = 0U;

  if (payload != NULL)
  {
    msg->payload = *payload;
  }

  if (App_AlarmIsValidMsg(msg) == 0U)
  {
    return APP_ALARM_RESULT_INVALID_PARAM;
  }

  return APP_ALARM_RESULT_OK;
}

/**
 * @brief 投递异常消息到告警队列。
 *
 * 队列满时返回 pdFAIL，并尝试直接记录系统级队列溢出异常。
 *
 * @param msg 待投递消息。
 * @param timeout_ticks 队列满时等待的 tick 数。
 * @retval pdPASS 投递成功。
 * @retval pdFAIL 投递失败。
 */
BaseType_t App_AlarmPost(const App_AlarmMsg_t *msg, TickType_t timeout_ticks)
{
  BaseType_t result;

  if ((s_alarm_queue == NULL) || (App_AlarmIsValidMsg(msg) == 0U))
  {
    return pdFAIL;
  }

  result = xQueueSend(s_alarm_queue, msg, timeout_ticks);
  if (result != pdPASS)
  {
    App_AlarmPayload_t payload;

    (void)memset(&payload, 0, sizeof(payload));
    payload.system.hook_code = ERR_SYSTEM_ALARM_QUEUE_FULL;
    payload.system.detail = (uint32_t)msg->header.code;
    App_AlarmRecordDirect(APP_MODULE_SYSTEM, APP_ALARM_SEVERITY_IMPORTANT, ERR_SYSTEM_ALARM_QUEUE_FULL, APP_ALARM_SYSTEM_INSTANCE_ID, APP_ALARM_PAYLOAD_SYSTEM, &payload);
  }

  return result;
}

/**
 * @brief 在中断上下文投递异常消息到告警队列。
 *
 * @param msg 待投递消息。
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
 * 周期性消费异常消息队列，并维护告警模块心跳。
 *
 * @param argument FreeRTOS 任务参数，当前未使用。
 */
void App_AlarmTask(void *argument)
{
  (void)argument;
  App_StatusSet(APP_MODULE_ALARM, APP_STATE_OK, ERR_OK);

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
 * @brief 获取当前活动异常数量。
 *
 * @return 当前 active 记录数量。
 */
uint16_t App_AlarmGetActiveCount(void)
{
  uint16_t count;

  taskENTER_CRITICAL();
  count = App_AlarmCountActiveLocked();
  taskEXIT_CRITICAL();

  return count;
}

/**
 * @brief 按活动记录序号获取记录副本。
 *
 * index 只针对 active 记录计数，不是内部数组物理下标。
 *
 * @param index 活动记录序号。
 * @param record 输出记录副本。
 * @return 查询结果。
 */
App_AlarmResult_t App_AlarmGetRecord(uint16_t index, App_AlarmRecord_t *record)
{
  uint16_t active_index = 0U;
  uint16_t i;

  if (record == NULL)
  {
    return APP_ALARM_RESULT_INVALID_PARAM;
  }

  taskENTER_CRITICAL();
  for (i = 0U; i < APP_ALARM_ACTIVE_MAX; i++)
  {
    if (s_alarm_records[i].active != 0U)
    {
      if (active_index == index)
      {
        *record = s_alarm_records[i];
        taskEXIT_CRITICAL();
        return APP_ALARM_RESULT_OK;
      }

      active_index++;
    }
  }
  taskEXIT_CRITICAL();

  return APP_ALARM_RESULT_NOT_FOUND;
}

/**
 * @brief 按来源模块和实例编号查找活动异常。
 *
 * @param source 来源模块。
 * @param instance_id 实例编号。
 * @param records 输出记录数组。
 * @param max_count 输出数组容量。
 * @return 实际写入的记录数量。
 */
uint16_t App_AlarmFindBySourceInstance(App_ModuleId_t source, uint16_t instance_id, App_AlarmRecord_t *records, uint16_t max_count)
{
  uint16_t count = 0U;
  uint16_t i;

  if ((records == NULL) || (max_count == 0U))
  {
    return 0U;
  }

  taskENTER_CRITICAL();
  for (i = 0U; (i < APP_ALARM_ACTIVE_MAX) && (count < max_count); i++)
  {
    if ((s_alarm_records[i].active != 0U) &&
        (s_alarm_records[i].source == source) &&
        (s_alarm_records[i].instance_id == instance_id))
    {
      records[count] = s_alarm_records[i];
      count++;
    }
  }
  taskEXIT_CRITICAL();

  return count;
}

/**
 * @brief 获取当前最高优先级错误码。
 *
 * @return 无活动异常时返回 ERR_OK，否则返回最高严重程度记录的错误码。
 */
App_ErrorCode_t App_AlarmGetCurrentCode(void)
{
  App_AlarmSeverity_t highest_severity = APP_ALARM_SEVERITY_NORMAL;
  App_ErrorCode_t current_code = ERR_OK;
  uint16_t i;

  taskENTER_CRITICAL();
  for (i = 0U; i < APP_ALARM_ACTIVE_MAX; i++)
  {
    if ((s_alarm_records[i].active != 0U) && (s_alarm_records[i].severity >= highest_severity))
    {
      highest_severity = s_alarm_records[i].severity;
      current_code = s_alarm_records[i].code;
    }
  }
  taskEXIT_CRITICAL();

  return current_code;
}

/**
 * @brief 获取当前最高异常严重程度。
 *
 * @return 无活动异常时返回 APP_ALARM_SEVERITY_NORMAL。
 */
App_AlarmSeverity_t App_AlarmGetCurrentSeverity(void)
{
  App_AlarmSeverity_t highest_severity = APP_ALARM_SEVERITY_NORMAL;
  uint16_t i;

  taskENTER_CRITICAL();
  for (i = 0U; i < APP_ALARM_ACTIVE_MAX; i++)
  {
    if ((s_alarm_records[i].active != 0U) && (s_alarm_records[i].severity >= highest_severity))
    {
      highest_severity = s_alarm_records[i].severity;
    }
  }
  taskEXIT_CRITICAL();

  return highest_severity;
}

/**
 * @brief 分发并应用一条异常消息。
 *
 * @param msg 已从队列取出的消息。
 * @return 处理结果。
 */
static App_AlarmResult_t App_AlarmDispatchMsg(const App_AlarmMsg_t *msg)
{
  if (App_AlarmIsValidMsg(msg) == 0U)
  {
    return APP_ALARM_RESULT_INVALID_PARAM;
  }

  if (msg->header.op == APP_ALARM_OP_RAISE)
  {
    return App_AlarmApplyRaise(msg);
  }

  return App_AlarmApplyClear(msg);
}

/**
 * @brief 写入或刷新活动异常。
 *
 * @param msg RAISE 消息。
 * @return 写入结果。
 */
static App_AlarmResult_t App_AlarmApplyRaise(const App_AlarmMsg_t *msg)
{
  App_AlarmResult_t result = APP_ALARM_RESULT_OK;
  int16_t index;
  uint32_t now_ms;

  now_ms = (msg->header.timestamp_ms != 0U) ? msg->header.timestamp_ms : (uint32_t)xTaskGetTickCount();

  taskENTER_CRITICAL();
  index = App_AlarmFindKeyLocked(msg->header.source, msg->header.code, msg->header.instance_id);
  if (index >= 0)
  {
    App_AlarmWriteRecordLocked(index, msg, now_ms, 0U);
  }
  else
  {
    index = App_AlarmFindFreeLocked();
    if (index < 0)
    {
      index = App_AlarmFindReplacementLocked(msg->header.severity);
    }

    if (index >= 0)
    {
      App_AlarmWriteRecordLocked(index, msg, now_ms, 1U);
    }
    else
    {
      result = APP_ALARM_RESULT_NO_SLOT;
    }
  }
  taskEXIT_CRITICAL();

  if (result == APP_ALARM_RESULT_OK)
  {
    App_StatusSet(msg->header.source, APP_STATE_ERROR, msg->header.code);
  }
  else if (msg->header.code != ERR_SYSTEM_ALARM_RECORD_FULL)
  {
    App_AlarmPayload_t payload;

    (void)memset(&payload, 0, sizeof(payload));
    payload.system.hook_code = ERR_SYSTEM_ALARM_RECORD_FULL;
    payload.system.detail = (uint32_t)msg->header.code;
    App_AlarmRecordDirect(APP_MODULE_SYSTEM, APP_ALARM_SEVERITY_CRITICAL, ERR_SYSTEM_ALARM_RECORD_FULL, APP_ALARM_SYSTEM_INSTANCE_ID, APP_ALARM_PAYLOAD_SYSTEM, &payload);
  }

  return result;
}

/**
 * @brief 清除活动异常。
 *
 * @param msg CLEAR 消息。
 * @return 清除结果。
 */
static App_AlarmResult_t App_AlarmApplyClear(const App_AlarmMsg_t *msg)
{
  App_ErrorCode_t source_code = ERR_OK;
  App_AlarmSeverity_t source_severity = APP_ALARM_SEVERITY_NORMAL;
  App_AlarmResult_t result = APP_ALARM_RESULT_NOT_FOUND;
  uint8_t has_source_active;
  int16_t index;

  taskENTER_CRITICAL();
  index = App_AlarmFindKeyLocked(msg->header.source, msg->header.code, msg->header.instance_id);
  if (index >= 0)
  {
    (void)memset(&s_alarm_records[index], 0, sizeof(s_alarm_records[index]));
    result = APP_ALARM_RESULT_OK;
  }

  has_source_active = App_AlarmSourceHasActiveLocked(msg->header.source, &source_code, &source_severity);
  taskEXIT_CRITICAL();

  if (result == APP_ALARM_RESULT_OK)
  {
    if (has_source_active != 0U)
    {
      App_StatusSet(msg->header.source, APP_STATE_ERROR, source_code);
    }
    else
    {
      (void)source_severity;
      App_StatusSet(msg->header.source, APP_STATE_OK, ERR_OK);
    }
  }

  return result;
}

/**
 * @brief 直接写入系统级异常记录。
 *
 * 该函数只用于系统 hook 或队列/记录表溢出等异常路径，不作为普通模块 API。
 */
static void App_AlarmRecordDirect(App_ModuleId_t source, App_AlarmSeverity_t severity, App_ErrorCode_t code, uint16_t instance_id, App_AlarmPayloadType_t payload_type, const App_AlarmPayload_t *payload)
{
  App_AlarmMsg_t msg;
  int16_t index;
  uint32_t now_ms = (uint32_t)xTaskGetTickCount();

  (void)memset(&msg, 0, sizeof(msg));
  msg.header.op = APP_ALARM_OP_RAISE;
  msg.header.source = source;
  msg.header.severity = severity;
  msg.header.code = code;
  msg.header.instance_id = instance_id;
  msg.header.payload_type = payload_type;
  msg.header.timestamp_ms = now_ms;
  if (payload != NULL)
  {
    msg.payload = *payload;
  }

  taskENTER_CRITICAL();
  index = App_AlarmFindKeyLocked(source, code, instance_id);
  if (index < 0)
  {
    index = App_AlarmFindFreeLocked();
  }
  if (index < 0)
  {
    index = App_AlarmFindReplacementLocked(severity);
  }
  if (index >= 0)
  {
    App_AlarmWriteRecordLocked(index, &msg, now_ms, (s_alarm_records[index].active == 0U) ? 1U : 0U);
  }
  taskEXIT_CRITICAL();

  App_StatusSet(source, APP_STATE_ERROR, code);
}

/**
 * @brief 在活动表中查找指定异常键。
 */
static int16_t App_AlarmFindKeyLocked(App_ModuleId_t source, App_ErrorCode_t code, uint16_t instance_id)
{
  uint16_t i;

  for (i = 0U; i < APP_ALARM_ACTIVE_MAX; i++)
  {
    if ((s_alarm_records[i].active != 0U) &&
        (s_alarm_records[i].source == source) &&
        (s_alarm_records[i].code == code) &&
        (s_alarm_records[i].instance_id == instance_id))
    {
      return (int16_t)i;
    }
  }

  return -1;
}

/**
 * @brief 查找空闲记录槽。
 */
static int16_t App_AlarmFindFreeLocked(void)
{
  uint16_t i;

  for (i = 0U; i < APP_ALARM_ACTIVE_MAX; i++)
  {
    if (s_alarm_records[i].active == 0U)
    {
      return (int16_t)i;
    }
  }

  return -1;
}

/**
 * @brief 表满时查找可被更严重异常替换的记录。
 */
static int16_t App_AlarmFindReplacementLocked(App_AlarmSeverity_t severity)
{
  App_AlarmSeverity_t lowest_severity = APP_ALARM_SEVERITY_CRITICAL;
  int16_t lowest_index = -1;
  uint16_t i;

  for (i = 0U; i < APP_ALARM_ACTIVE_MAX; i++)
  {
    if (s_alarm_records[i].severity <= lowest_severity)
    {
      lowest_severity = s_alarm_records[i].severity;
      lowest_index = (int16_t)i;
    }
  }

  if ((lowest_index >= 0) && (severity > lowest_severity))
  {
    return lowest_index;
  }

  return -1;
}

/**
 * @brief 统计活动记录数量。
 */
static uint16_t App_AlarmCountActiveLocked(void)
{
  uint16_t count = 0U;
  uint16_t i;

  for (i = 0U; i < APP_ALARM_ACTIVE_MAX; i++)
  {
    if (s_alarm_records[i].active != 0U)
    {
      count++;
    }
  }

  return count;
}

/**
 * @brief 查询来源模块是否仍有活动异常。
 */
static uint8_t App_AlarmSourceHasActiveLocked(App_ModuleId_t source, App_ErrorCode_t *code, App_AlarmSeverity_t *severity)
{
  App_AlarmSeverity_t highest_severity = APP_ALARM_SEVERITY_NORMAL;
  App_ErrorCode_t current_code = ERR_OK;
  uint8_t found = 0U;
  uint16_t i;

  for (i = 0U; i < APP_ALARM_ACTIVE_MAX; i++)
  {
    if ((s_alarm_records[i].active != 0U) &&
        (s_alarm_records[i].source == source) &&
        (s_alarm_records[i].severity >= highest_severity))
    {
      highest_severity = s_alarm_records[i].severity;
      current_code = s_alarm_records[i].code;
      found = 1U;
    }
  }

  if (code != NULL)
  {
    *code = current_code;
  }
  if (severity != NULL)
  {
    *severity = highest_severity;
  }

  return found;
}

/**
 * @brief 写入活动记录槽。
 */
static void App_AlarmWriteRecordLocked(int16_t index, const App_AlarmMsg_t *msg, uint32_t now_ms, uint8_t is_new_record)
{
  App_AlarmRecord_t *record = &s_alarm_records[index];

  if (is_new_record != 0U)
  {
    (void)memset(record, 0, sizeof(*record));
    record->first_tick_ms = now_ms;
  }
  else if (record->first_tick_ms == 0U)
  {
    record->first_tick_ms = now_ms;
  }

  record->active = 1U;
  record->source = msg->header.source;
  record->severity = msg->header.severity;
  record->code = msg->header.code;
  record->payload_type = msg->header.payload_type;
  record->instance_id = msg->header.instance_id;
  record->last_tick_ms = now_ms;
  (void)memset(&record->payload, 0, sizeof(record->payload));
  if (msg->header.payload_type != APP_ALARM_PAYLOAD_NONE)
  {
    record->payload = msg->payload;
  }
}

/**
 * @brief 校验 payload 类型和数据指针是否匹配。
 */
static uint8_t App_AlarmIsValidPayload(App_AlarmPayloadType_t payload_type, const App_AlarmPayload_t *payload)
{
  switch (payload_type)
  {
  case APP_ALARM_PAYLOAD_NONE:
    return 1U;
  case APP_ALARM_PAYLOAD_SENSOR:
  case APP_ALARM_PAYLOAD_COMM:
  case APP_ALARM_PAYLOAD_STORAGE:
  case APP_ALARM_PAYLOAD_SYSTEM:
  case APP_ALARM_PAYLOAD_RAW:
    return (payload != NULL) ? 1U : 0U;
  default:
    return 0U;
  }
}

/**
 * @brief 校验异常消息是否可投递或消费。
 */
static uint8_t App_AlarmIsValidMsg(const App_AlarmMsg_t *msg)
{
  if ((msg == NULL) || (msg->header.source >= APP_MODULE_COUNT) || (msg->header.code == ERR_OK))
  {
    return 0U;
  }

  switch (msg->header.op)
  {
  case APP_ALARM_OP_RAISE:
    if (msg->header.severity == APP_ALARM_SEVERITY_NORMAL)
    {
      return 0U;
    }
    return App_AlarmIsValidPayload(msg->header.payload_type, &msg->payload);
  case APP_ALARM_OP_CLEAR:
    return 1U;
  default:
    return 0U;
  }
}

/**
 * @brief FreeRTOS 动态内存申请失败钩子。
 *
 * 触发后直接写入系统级异常记录并关闭中断。
 */
void vApplicationMallocFailedHook(void)
{
  App_AlarmPayload_t payload;

  (void)memset(&payload, 0, sizeof(payload));
  payload.system.hook_code = APP_ALARM_HOOK_MALLOC_FAILED;
  App_AlarmRecordDirect(APP_MODULE_SYSTEM, APP_ALARM_SEVERITY_CRITICAL, ERR_SYSTEM_HEAP_FAILED, APP_ALARM_SYSTEM_INSTANCE_ID, APP_ALARM_PAYLOAD_SYSTEM, &payload);
  taskDISABLE_INTERRUPTS();
  for (;;)
  {
  }
}

/**
 * @brief FreeRTOS 任务栈溢出钩子。
 *
 * 触发后直接写入系统级异常记录并关闭中断。
 *
 * @param task 发生栈溢出的任务句柄。
 * @param task_name 发生栈溢出的任务名称。
 */
void vApplicationStackOverflowHook(TaskHandle_t task, char *task_name)
{
  App_AlarmPayload_t payload;

  (void)task_name;
  (void)memset(&payload, 0, sizeof(payload));
  payload.system.hook_code = APP_ALARM_HOOK_STACK_OVERFLOW;
  payload.system.task_id = (uint32_t)(uintptr_t)task;
  App_AlarmRecordDirect(APP_MODULE_SYSTEM, APP_ALARM_SEVERITY_CRITICAL, ERR_SYSTEM_STACK_OVERFLOW, APP_ALARM_SYSTEM_INSTANCE_ID, APP_ALARM_PAYLOAD_SYSTEM, &payload);
  taskDISABLE_INTERRUPTS();
  for (;;)
  {
  }
}
