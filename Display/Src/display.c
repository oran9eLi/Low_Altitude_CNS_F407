#include "display.h"

Display_Result_t Display_Init(void)
{
  return DISPLAY_OK;
}

Display_Result_t Display_SelfCheck(App_ErrorCode_t *error_code)
{
  if (error_code != 0) {
    *error_code = APP_ERROR_OK;
  }

  return DISPLAY_OK;
}

Display_Result_t Display_Refresh(uint32_t now_ms)
{
  (void)now_ms;
  return DISPLAY_OK;
}
