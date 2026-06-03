#ifndef DEBUG_CONSOLE_H
#define DEBUG_CONSOLE_H

#include <stdint.h>
#include <stdio.h>
#include "debug_config.h"

/***************DEBUG***************/
#ifdef DEBUG_ENABLE

void DebugConsole_Init(void);
void DebugConsole_HexDump(const char *prefix, const uint8_t *data, uint16_t len);
void DebugConsole_Printf(const char *prefix, const char *fmt, ...);
void DebugConsole_TaskInfo(const char *name, const char *fmt, ...);

#define DBG_PRINT(fmt, ...) DebugConsole_Printf("[DBG] ", fmt, ##__VA_ARGS__)
#define DBG_TASK_INFO(name, fmt, ...) DebugConsole_TaskInfo((name), fmt, ##__VA_ARGS__)

#else

#define DebugConsole_Init()
#define DebugConsole_HexDump(p, d, l)
#define DebugConsole_Printf(prefix, fmt, ...)
#define DebugConsole_TaskInfo(name, fmt, ...)
#define DBG_PRINT(fmt, ...)
#define DBG_TASK_INFO(name, fmt, ...)

#endif
/*************DEBUG END*************/

#endif
