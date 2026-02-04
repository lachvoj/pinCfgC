#ifndef GLOBALS_H
#define GLOBALS_H

// ============================================================================
// Compile-Time Feature Flags
// ============================================================================

// Uncomment to enable I2C measurement support (Phase 3)
// Adds ~600-800 bytes to binary when enabled
// #define  PINCFG_FEATURE_I2C_MEASUREMENT

// Uncomment to enable loop time measurement support (for debugging/profiling)
// Adds ~200-300 bytes to binary when enabled
// #define  PINCFG_FEATURE_LOOPTIME_MEASUREMENT

// ============================================================================

#include "Event.h"
#include "ILoopable.h"
#include "ISensorMeasure.h"
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
    LOOPABLE_T **ppsLoopables;
    PRESENTABLE_T **ppsPresentables;
    PRESENTABLE_VTAB_T sSwitchPrVTab;
    PRESENTABLE_VTAB_T sInPinPrVTab;
    PRESENTABLE_VTAB_T sCliPrVTab;
    PRESENTABLE_VTAB_T sSensorReporterPrVTab;
    // InPin
    uint32_t u32InPinDebounceMs;
    uint32_t u32InPinMulticlickMaxDelayMs;
    // Switch
    uint32_t u32SwitchImpulseDurationMs;
    uint32_t u32SwitchFbDelayMs;
} GLOBALS_T;

extern GLOBALS_T *psGlobals;

#endif // GLOBALS_H
