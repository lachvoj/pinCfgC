#ifndef GLOBALS_H
#define GLOBALS_H

#include "LooPre.h"
#include "PinSubscriberIf.h"

typedef struct
{
    // memory
    char *pvMemEnd;
    char *pvMemNext;
    char *pvMemTempEnd;
    bool bMemIsInitialized;
    // pincfgcsv
    LOOPRE_T *psLoopablesFirst;
    LOOPRE_T *psPresentablesFirst;
    LOOPRE_VTAB_T sSwitchVTab;
    LOOPRE_VTAB_T sInPinVTab;
    LOOPRE_VTAB_T sExtCfgReceiverVTab;
    LOOPRE_VTAB_T sCpuTempVTab;
    PINSUBSCRIBER_VTAB_T sTriggerVTab;
    // InPin
    uint32_t u32InPinDebounceMs;
    uint32_t u32InPinMulticlickMaxDelayMs;
    // Switch
    uint32_t u32SwitchImpulseDurationMs;
    uint32_t u32SwitchFbOnDelayMs;
    uint32_t u32SwitchFbOffDelayMs;
    uint32_t u32SwitchTimedActionAdditionMs;
    // cfg buf
    char acCfgBuf[PINCFG_CONFIG_MAX_SZ_D];
} GLOBALS_HANDLE_T;

extern GLOBALS_HANDLE_T *psGlobals;

#endif // GLOBALS_H
