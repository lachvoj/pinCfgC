#ifndef PINCFGSTRING_H
#define PINCFGSTRING_H

#include <string.h>

#include "Types.h"

typedef struct
{
    const char *pcStrStart;
    size_t szLen;
} STRING_POINT_T;

void PinCfgStr_vInitStrPoint(STRING_POINT_T *psStrPoint, const char *pcStrStart, size_t szLen);

typedef enum
{
    PINCFG_STR_OK_E,
    PINCFG_STR_MAXLEN_ERROR_E,
    PINCFG_STR_INDEXOUTOFARRAY_ERROR_E,
    PINCFG_STR_INSUFFICIENT_BUFFER_E,
    PINCFG_STR_UNSUCCESSFULL_CONVERSION_E,
    PINCFG_STR_ERROR_E
} PINCFG_STR_RESULT_T;

size_t PinCfgStr_szGetSplitCount(const STRING_POINT_T *psInStrPt, const char cDelimiter);

void PinCfgStr_vGetSplitElemByIndex(STRING_POINT_T *psStrPt, const char cDelimiter, const size_t szIndex);

PINCFG_STR_RESULT_T PinCfgStr_eAtoU8(const STRING_POINT_T *psStrPt, uint8_t *pu8Out);

PINCFG_STR_RESULT_T PinCfgStr_eSplitStrPt(
    const STRING_POINT_T *psInStrPt,
    const char cDelimiter,
    STRING_POINT_T *pasOut,
    uint8_t *u8OutLen,
    const uint8_t u8OutMaxLen);

PINCFG_STR_RESULT_T PinCfgStr_eRemoveAt(STRING_POINT_T *pasStrPts, uint8_t *u8StrPtsLen, const uint8_t u8Index);

PINCFG_STR_RESULT_T PinCfgStr_eRemoveEmpty(STRING_POINT_T *pasStrPts, uint8_t *u8StrPtsLen);

PINCFG_STR_RESULT_T PinCfgStr_eRemoveStartingWith(STRING_POINT_T *pasStrPts, uint8_t *u8StrPtsLen, const char cStart);

#endif // PINCFGSTRING_H
