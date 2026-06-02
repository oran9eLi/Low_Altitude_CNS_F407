#include "app_storage.h"

static uint32_t s_storage_ready;
static uint32_t s_written_lines;

void App_StorageInit(void)
{
  s_storage_ready = 1U;
  s_written_lines = 0U;
}

App_StorageResult_t App_StorageWriteCsvLine(const char *line, size_t length)
{
  if ((s_storage_ready == 0U) || (line == 0) || (length == 0U)) {
    return APP_STORAGE_NOT_READY;
  }

  /* SD/FatFs backend will be connected here after SDIO/SPI is confirmed. */
  s_written_lines++;
  (void)s_written_lines;

  return APP_STORAGE_OK;
}

App_StorageResult_t App_StorageFlush(void)
{
  if (s_storage_ready == 0U) {
    return APP_STORAGE_NOT_READY;
  }

  return APP_STORAGE_OK;
}
