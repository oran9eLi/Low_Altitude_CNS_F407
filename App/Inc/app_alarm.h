#ifndef APP_ALARM_H
#define APP_ALARM_H

#include <stdint.h>
#include "app_status.h"

// 告警代码定义
typedef enum {
  APP_ALARM_NONE = 0,           // 无告警
  APP_ALARM_SELF_CHECK_FAILED,  // 自检失败
  APP_ALARM_COMM_TIMEOUT,       // 通信超时
  APP_ALARM_LOG_STORAGE_FAILED, // 日志存储失败
  APP_ALARM_STACK_OVERFLOW,     // 栈溢出
  APP_ALARM_HEAP_FAILED         // 堆内存失败
} App_AlarmCode_t;

void App_AlarmInit(void);
void App_AlarmRaise(App_AlarmCode_t code, App_ModuleId_t source, uint32_t detail);
void App_AlarmClear(App_AlarmCode_t code);
uint32_t App_AlarmGetActiveMask(void);

#endif
