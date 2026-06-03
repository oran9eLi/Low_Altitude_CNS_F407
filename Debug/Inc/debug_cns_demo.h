#ifndef DEBUG_CNS_DEMO_H
#define DEBUG_CNS_DEMO_H

#include "debug_config.h"

/***************DEBUG***************/
#ifdef DEBUG_ENABLE

void DebugCnsDemo_RunOnce(void);

#else

#define DebugCnsDemo_RunOnce()

#endif
/*************DEBUG END*************/

#endif
