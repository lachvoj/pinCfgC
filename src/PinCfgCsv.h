#ifndef PINCFGCSV_H
#define PINCFGCSV_H

#include "ILoopable.h"
#include "Presentable.h"
#include "Types.h"

typedef enum PINCFG_RESULT_E
{
    PINCFG_OK_E = 0,
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

typedef struct PINCFG_PARSE_PARAMS_S
{
    const char *pcConfig;
    PINCFG_RESULT_T (*eAddToLoopables)(LOOPABLE_T *psLoopable);
    PINCFG_RESULT_T (*eAddToPresentables)(PRESENTABLE_T *psPresentable);
    size_t *pszMemoryRequired;
    char *pcOutString;
    const uint16_t u16OutStrMaxLen;
    const bool bValidate;
} PINCFG_PARSE_PARAMS_T;

void PinCfgCsv_vLoop(uint32_t u32ms);

void PinCfgCsv_vPresentation(void);

void PinCfgCfg_vReceive(const uint8_t u8Id, const void *pvMsgData);

PINCFG_RESULT_T PinCfgCsv_eValidate(
    const char *pcConfig,
    size_t *pszMemoryRequired,
    char *pcOutString,
    const uint16_t u16OutStrMaxLen);

PINCFG_RESULT_T PinCfgCsv_eInit(uint8_t *pu8Memory, size_t szMemorySize, const char *pcDefaultCfg);

PINCFG_RESULT_T PinCfgCsv_eParse(PINCFG_PARSE_PARAMS_T *psParams);

#endif // PINCFGCSV_H
