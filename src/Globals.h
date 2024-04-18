#ifndef GLOBALS_H
#define GLOBALS_H

#include "ILoopable.h"
#include "PinSubscriberIf.h"
#include "Presentable.h"

typedef struct
{
    // memory
    char *pvMemEnd;
    char *pvMemNext;
    char *pvMemTempEnd;
    bool bMemIsInitialized;
    // pincfgcsv
    uint8_t u8LoopablesCount;
    uint8_t u8PresentablesCount;
    LOOPABLE_T **ppsLoopables;
    PRESENTABLE_T **ppsPresentables;
    PRESENTABLE_VTAB_T sSwitchPrVTab;
    PRESENTABLE_VTAB_T sInPinPrVTab;
    PRESENTABLE_VTAB_T sCliPrVTab;
    PRESENTABLE_VTAB_T sCpuTempPrVTab;
    // InPin
    uint32_t u32InPinDebounceMs;
    uint32_t u32InPinMulticlickMaxDelayMs;
    // Switch
    uint32_t u32SwitchImpulseDurationMs;
    uint32_t u32SwitchFbOnDelayMs;
    uint32_t u32SwitchFbOffDelayMs;
    uint32_t u32SwitchTimedActionAdditionMs;
} GLOBALS_T;

extern GLOBALS_T *psGlobals;

#endif // GLOBALS_H
