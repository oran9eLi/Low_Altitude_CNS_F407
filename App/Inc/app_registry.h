#ifndef APP_REGISTRY_H
#define APP_REGISTRY_H

#include <stddef.h>
#include <stdint.h>
#include "app_error.h"
#include "app_status.h"

typedef enum {
  APP_CHECK_NORMAL = 0,
  APP_CHECK_GENERAL,
  APP_CHECK_IMPORTANT,
  APP_CHECK_CRITICAL
} App_CheckResult_t;

typedef App_CheckResult_t (*App_ModuleInitFn)(void);
typedef App_CheckResult_t (*App_ModuleSelfCheckFn)(App_ErrorCode_t *error_code);
typedef void (*App_ModuleServiceFn)(void);

typedef struct
{
  App_ModuleId_t id;
  const char *name;
  App_ModuleInitFn init;
  App_ModuleSelfCheckFn self_check;
  App_ModuleServiceFn service;
  uint32_t check_period_ms;
} App_ModuleRegistryEntry_t;

App_CheckResult_t App_RegistryInitAll(void);
App_CheckResult_t App_RegistrySelfCheckAll(void);
void App_RegistryPoll(uint32_t now_ms);

size_t App_RegistryGetCount(void);
const App_ModuleRegistryEntry_t *App_RegistryGetEntry(size_t index);

#endif
