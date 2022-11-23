#ifndef PINCFGCSV_H
#define PINCFGCSV_H

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

PINCFG_RESULT_T PinCfgCsv_eInit(uint8_t *pu8Memory, size_t szMemorySize);

char *PinCfgCsv_pcGetCfgBuf(void);

PINCFG_RESULT_T PinCfgCsv_eParse(
    size_t *szMemoryRequired,
    char *pcOutString,
    const uint16_t u16OutStrMaxLen,
    const bool bValidate,
    const bool bRemoteConfigEnabled);

PINCFG_RESULT_T PinCfgCsv_eValidate(size_t *szMemoryRequired, char *pcOutString, const uint16_t u16OutStrMaxLen);

void PinCfgCsv_vLoop(uint32_t u32ms);

void PinCfgCsv_vPresentation(void);

void PinCfgCfg_vReceive(const uint8_t u8Id, void *pvMsgData);

#endif // PINCFGCSV_H
