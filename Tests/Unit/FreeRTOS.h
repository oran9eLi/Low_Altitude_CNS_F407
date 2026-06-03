#ifndef TESTS_UNIT_FREERTOS_H
#define TESTS_UNIT_FREERTOS_H

typedef int BaseType_t;
typedef unsigned int UBaseType_t;
typedef unsigned int TickType_t;

#ifndef pdPASS
#define pdPASS 1
#endif

#ifndef pdFAIL
#define pdFAIL 0
#endif

#ifndef pdTRUE
#define pdTRUE 1
#endif

#ifndef pdFALSE
#define pdFALSE 0
#endif

#ifndef portMAX_DELAY
#define portMAX_DELAY ((TickType_t)0xFFFFFFFFU)
#endif

#ifndef pdMS_TO_TICKS
#define pdMS_TO_TICKS(ms) ((TickType_t)(ms))
#endif

#ifndef taskENTER_CRITICAL
#define taskENTER_CRITICAL() do { } while (0)
#endif

#ifndef taskEXIT_CRITICAL
#define taskEXIT_CRITICAL() do { } while (0)
#endif

#endif
