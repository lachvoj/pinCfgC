#ifndef GLOBALS_H
#define GLOBALS_H

#include "LooPreIf.h"
#include "PincfgIf.h"

typedef struct
{
    // memory
    char *pvMemEnd;
    char *pvMemNext;
    char *pvMemTempEnd;
    bool bMemIsInitialized;
    // pincfgcsv
    LOOPRE_IF_T *psLoopablesFirst;
    LOOPRE_IF_T *psPresentablesFirst;
    // InPin
    uint32_t u32InPinDebounceMs;
    uint32_t u32InPinMulticlickMaxDelayMs;
    // cfg buf
    char acCfgBuf[PINCFG_CONFIG_MAX_SZ_D];
} GLOBALS_HANDLE_T;

extern GLOBALS_HANDLE_T *psGlobals;

#endif // GLOBALS_H
