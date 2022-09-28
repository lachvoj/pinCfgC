#ifndef PINCFGCSV_H
#define PINCFGCSV_H

#include "LooPreIf.h"
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
    PINCFG_ERROR_E
} PINCFG_RESULT_T;

PINCFG_RESULT_T PinCfgCsv_eParse(
    const char *pcCfgBuf,
    const bool bValidate,
    const bool bRemoteConfigEnabled,
    LOOPRE_IF_T **apsLoopables,
    uint8_t *u8LoopablesLen,
    const uint8_t u8LoopablesMaxLen,
    LOOPRE_IF_T **apsPresentables,
    uint8_t *u8PresentablesLen,
    const uint8_t u8PresentablesMaxLen,
    MYSENOSRS_IF_T *psMySensorsIf,
    PIN_IF_T *psPinIf,
    char *pcOutString,
    const uint16_t u16OutStrMaxLen);

PINCFG_RESULT_T PinCfgCsv_eValidate(const char *pcCfgBuf, char *pcOutString, const uint16_t u16OutStrMaxLen);

#endif // PINCFGCSV_H
