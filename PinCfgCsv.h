#ifndef PINCFGCSV_H
#define PINCFGCSV_H

#include "MysensorsIf.h"
#include "PinIf.h"
#include "Types.h"

typedef enum
{
    PINCFG_OK_E,
    PINCFG_NULLPTR_ERROR_E,
    PINCFG_INVALID_FORMAT_E,
    PINCFG_MAXLEN_ERROR_E,
    PINCFG_TYPE_ERROR_E,
    PINCFG_OUTOFMEMORY_ERROR_E,
    PINCFG_MEMORYINIT_ERROR_E,
    PINCFG_ERROR_E
} PINCFG_RESULT_T;

PINCFG_RESULT_T PinCfgCsv_eParse(
    const char *pcCfgBuf,
    char *pcOutString,
    const uint16_t u16OutStrMaxLen,
    const bool bValidate,
    uint8_t *pcMemory,
    size_t szMemorySize,
    const bool bRemoteConfigEnabled,
    MYSENOSRS_IF_T *psMySensorsIf,
    PIN_IF_T *psPinIf);

inline PINCFG_RESULT_T PinCfgCsv_eValidate(const char *pcCfgBuf, char *pcOutString, const uint16_t u16OutStrMaxLen)
{
    return PinCfgCsv_eParse(pcCfgBuf, pcOutString, u16OutStrMaxLen, true, NULL, 0, false, NULL, NULL);
}

// inline PINCFG_RESULT_T PinCfgCsv_eLoop();

#endif // PINCFGCSV_H
