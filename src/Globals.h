#ifndef GLOBALS_H
#define GLOBALS_H

#include "ILoopable.h"
#include "ISensorMeasure.h"
#include "PinSubscriberIf.h"
#include "Presentable.h"

typedef struct
{
#ifndef USE_MALLOC
    // memory
    char *pvMemEnd;
    char *pvMemNext;
    char *pvMemTempEnd;
    bool bMemIsInitialized;
#endif // USE_MALLOC

    // pincfgcsv
    uint8_t u8LoopablesCount;
    uint8_t u8PresentablesCount;
    uint8_t u8MeasurementsCount;
    LOOPABLE_T **ppsLoopables;
    PRESENTABLE_T **ppsPresentables;
    ISENSORMEASURE_T **ppsMeasurements;  // Registry of measurement sources
    PRESENTABLE_VTAB_T sSwitchPrVTab;
    PRESENTABLE_VTAB_T sInPinPrVTab;
    PRESENTABLE_VTAB_T sCliPrVTab;
    // InPin
    uint32_t u32InPinDebounceMs;
    uint32_t u32InPinMulticlickMaxDelayMs;
    // Switch
    uint32_t u32SwitchImpulseDurationMs;
    uint32_t u32SwitchFbOnDelayMs;
    uint32_t u32SwitchFbOffDelayMs;
} GLOBALS_T;

extern GLOBALS_T *psGlobals;

#endif // GLOBALS_H
