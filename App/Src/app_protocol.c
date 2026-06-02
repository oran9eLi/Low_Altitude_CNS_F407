#include "app_protocol.h"
#include <stdio.h>
#include <string.h>
#include "FreeRTOS.h"
#include "task.h"
#include "app_logger.h"
#include "app_status.h"

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
  static const uint8_t demo_frame[] = {0xAAU, 0x55U, 0x01U, 0x01U, 0x2AU, 0x28U};
  App_ProtocolMessage_t message;
  App_LogRecord_t log_record;

  (void)argument;
  App_StatusSet(APP_MODULE_PROTOCOL, APP_STATE_OK, 0U);

  for (;;) {
    if (App_ProtocolParse(demo_frame, sizeof(demo_frame), &message) == APP_PROTOCOL_OK) {
      log_record.timestamp_ms = xTaskGetTickCount();
      log_record.level        = APP_LOG_LEVEL_INFO;
      strncpy(log_record.module, "PROTOCOL", sizeof(log_record.module));
      strncpy(log_record.field, "demo_type", sizeof(log_record.field));
      snprintf(log_record.value, sizeof(log_record.value), "%u", (unsigned int)message.type);
      strncpy(log_record.unit, "-", sizeof(log_record.unit));
      log_record.status = 0U;
      (void)App_LogWrite(&log_record);
      App_StatusHeartbeat(APP_MODULE_PROTOCOL);
    }

    vTaskDelay(pdMS_TO_TICKS(1000));
  }
}
