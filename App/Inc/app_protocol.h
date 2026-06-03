#ifndef APP_PROTOCOL_H
#define APP_PROTOCOL_H

#include <stddef.h>
#include <stdint.h>

#define APP_PROTOCOL_FRAME_MAX_LEN 64U

typedef enum {
  APP_PROTOCOL_OK = 0,
  APP_PROTOCOL_INVALID_LENGTH,
  APP_PROTOCOL_INVALID_HEADER,
  APP_PROTOCOL_INVALID_CHECKSUM
} App_ProtocolResult_t;

typedef struct
{
  uint8_t type;
  uint8_t payload[APP_PROTOCOL_FRAME_MAX_LEN];
  uint16_t payload_len;
} App_ProtocolMessage_t;

App_ProtocolResult_t App_ProtocolBuildFrame(uint8_t type, const uint8_t *payload, uint16_t payload_len, uint8_t *frame, uint16_t frame_max_len, uint16_t *frame_len);
App_ProtocolResult_t App_ProtocolParse(const uint8_t *frame, size_t length, App_ProtocolMessage_t *message);
void App_ProtocolTask(void *argument);

#endif
