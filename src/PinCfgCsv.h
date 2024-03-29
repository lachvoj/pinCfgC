#ifndef PINCFGCSV_H
#define PINCFGCSV_H

#include "Types.h"

typedef enum
{
    PINCFG_OK_E,
    PINCFG_NULLPTR_ERROR_E,
    PINCFG_IF_NULLPTR_ERROR_E,
    PINCFG_INVALID_FORMAT_E,
    PINCFG_MAXLEN_ERROR_E,
    PINCFG_TYPE_ERROR_E,
    PINCFG_OUTOFMEMORY_ERROR_E,
    PINCFG_MEMORYINIT_ERROR_E,
    PINCFG_ERROR_E,
    PINCFG_WARNINGS_E
} PINCFG_RESULT_T;

PINCFG_RESULT_T PinCfgCsv_eInit(uint8_t *pu8Memory, size_t szMemorySize, const char *pcDefaultCfg);

char *PinCfgCsv_pcGetCfgBuf(void);

PINCFG_RESULT_T PinCfgCsv_eParse(
    size_t *pszMemoryRequired,
    char *pcOutString,
    const uint16_t u16OutStrMaxLen,
    const bool bValidate);

PINCFG_RESULT_T PinCfgCsv_eValidate(size_t *pszMemoryRequired, char *pcOutString, const uint16_t u16OutStrMaxLen);

void PinCfgCsv_vLoop(uint32_t u32ms);

void PinCfgCsv_vPresentation(void);

void PinCfgCfg_vReceive(const uint8_t u8Id, const void *pvMsgData);

#endif // PINCFGCSV_H
