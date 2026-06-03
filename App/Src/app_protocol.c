#include "app_protocol.h"
#include <stdio.h>
#include <string.h>
#include "FreeRTOS.h"
#include "task.h"
#include "app_logger.h"
#include "app_status.h"
/***************DEBUG***************/
#include "debug_cns_demo.h"
#include "debug_trace.h"
/*************DEBUG END*************/

#define APP_FRAME_HEADER_0 0xAAU
#define APP_FRAME_HEADER_1 0x55U

static uint8_t App_ProtocolChecksum(const uint8_t *data, size_t length)
{
  uint8_t checksum = 0U;
  size_t i;

  for (i = 0U; i < length; i++) {
    checksum ^= data[i];
  }

  return checksum;
}

App_ProtocolResult_t App_ProtocolBuildFrame(uint8_t type, const uint8_t *payload, uint16_t payload_len, uint8_t *frame, uint16_t frame_max_len, uint16_t *frame_len)
{
  uint16_t len = 0U;

  if ((frame == NULL) || (frame_len == NULL)) {
    return APP_PROTOCOL_INVALID_LENGTH;
  }

  if ((payload_len > 0U) && (payload == NULL)) {
    return APP_PROTOCOL_INVALID_LENGTH;
  }

  if ((payload_len > APP_PROTOCOL_FRAME_MAX_LEN) || (frame_max_len < ((uint16_t)payload_len + 5U))) {
    return APP_PROTOCOL_INVALID_LENGTH;
  }

  frame[len++] = APP_FRAME_HEADER_0;
  frame[len++] = APP_FRAME_HEADER_1;
  frame[len++] = type;
  frame[len++] = (uint8_t)payload_len;

  if (payload_len > 0U) {
    memcpy(&frame[len], payload, payload_len);
    len = (uint16_t)(len + payload_len);
  }

  frame[len++] = App_ProtocolChecksum(&frame[2], (size_t)payload_len + 2U);
  *frame_len = len;

  return APP_PROTOCOL_OK;
}

App_ProtocolResult_t App_ProtocolParse(const uint8_t *frame, size_t length, App_ProtocolMessage_t *message)
{
  uint8_t payload_len;
  uint8_t expected_checksum;

  if ((frame == NULL) || (message == NULL) || (length < 5U)) {
    return APP_PROTOCOL_INVALID_LENGTH;
  }

  if ((frame[0] != APP_FRAME_HEADER_0) || (frame[1] != APP_FRAME_HEADER_1)) {
    return APP_PROTOCOL_INVALID_HEADER;
  }

  payload_len = frame[3];
  if (((size_t)payload_len + 5U) != length) {
    return APP_PROTOCOL_INVALID_LENGTH;
  }

  expected_checksum = App_ProtocolChecksum(&frame[2], (size_t)payload_len + 2U);
  if (expected_checksum != frame[length - 1U]) {
    return APP_PROTOCOL_INVALID_CHECKSUM;
  }

  message->type        = frame[2];
  message->payload_len = payload_len;
  if (payload_len > 0U) {
    memcpy(message->payload, &frame[4], payload_len);
  }

  return APP_PROTOCOL_OK;
}

void App_ProtocolTask(void *argument)
{
  App_LogRecord_t log_record;

  (void)argument;
  App_StatusSet(APP_MODULE_PROTOCOL, APP_STATE_OK, APP_ERROR_OK);

  /***************DEBUG***************/
  DBG_TRACE_TASK_STARTED("protocol");
  /*************DEBUG END*************/

  for (;;) {
    /***************DEBUG***************/
    DebugCnsDemo_RunOnce();
    /*************DEBUG END*************/

    log_record.timestamp_ms = xTaskGetTickCount();
    log_record.level        = APP_LOG_LEVEL_INFO;
    strncpy(log_record.module, "PROTOCOL", sizeof(log_record.module));
    strncpy(log_record.field, "alive", sizeof(log_record.field));
    snprintf(log_record.value, sizeof(log_record.value), "%lu", (unsigned long)log_record.timestamp_ms);
    strncpy(log_record.unit, "ms", sizeof(log_record.unit));
    log_record.status = 0U;
    (void)App_LogWrite(&log_record);
    App_StatusHeartbeat(APP_MODULE_PROTOCOL);

    vTaskDelay(pdMS_TO_TICKS(1000));
  }
}
