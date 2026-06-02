#ifndef APP_STORAGE_H
#define APP_STORAGE_H

#include <stddef.h>
#include <stdint.h>

typedef enum {
  APP_STORAGE_OK = 0,
  APP_STORAGE_NOT_READY,
  APP_STORAGE_WRITE_FAILED
} App_StorageResult_t;

void App_StorageInit(void);
App_StorageResult_t App_StorageWriteCsvLine(const char *line, size_t length);
App_StorageResult_t App_StorageFlush(void);

#endif
