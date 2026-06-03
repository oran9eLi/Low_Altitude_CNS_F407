#ifndef DISPLAY_H
#define DISPLAY_H

#include <stdint.h>
#include "app_error.h"

typedef enum {
  DISPLAY_OK = 0,
  DISPLAY_NOT_READY,
  DISPLAY_ERROR
} Display_Result_t;

Display_Result_t Display_Init(void);
Display_Result_t Display_SelfCheck(App_ErrorCode_t *error_code);
Display_Result_t Display_Refresh(uint32_t now_ms);

#endif
