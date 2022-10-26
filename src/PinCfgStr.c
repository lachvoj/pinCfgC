#include <errno.h>
#include <stdlib.h>

#include "PinCfgStr.h"

void PinCfgStr_vInitStrPoint(STRING_POINT_T *psStrPoint, const char *pcStrStart, size_t szLen)
{
    psStrPoint->pcStrStart = pcStrStart;
    psStrPoint->szLen = szLen;
}

size_t PinCfgStr_szGetSplitCount(const STRING_POINT_T *psInStrPt, const char cDelimiter)
{
    size_t szCount = 0;
    for (size_t i = 0; i < psInStrPt->szLen; i++)
    {
        if (psInStrPt->pcStrStart[i] == cDelimiter)
        {
            szCount++;
        }
    }
    if (psInStrPt->pcStrStart[psInStrPt->szLen - 1] != cDelimiter)
        szCount++;

    return szCount;
}

void PinCfgStr_vGetSplitElemByIndex(STRING_POINT_T *psStrPt, const char cDelimiter, const size_t szIndex)
{
    const char *pcInput = psStrPt->pcStrStart;
    size_t szActualIndex = 0;
    size_t szStrLen = 0;

    if (szIndex == 0 && pcInput[0] == cDelimiter)
    {
        psStrPt->pcStrStart = NULL;
        return;
    }
    for (size_t i = 0; i < psStrPt->szLen; i++)
    {
        if (pcInput[i] == cDelimiter)
        {
            if (szActualIndex == szIndex)
                break;
            if ((i + 1) == psStrPt->szLen)
            {
                psStrPt->pcStrStart = NULL;
                psStrPt->szLen = 0;
                return;
            }
            psStrPt->pcStrStart = &pcInput[i + 1];
            szActualIndex++;
            szStrLen = 0;
        }
        else
            szStrLen++;
    }
    psStrPt->szLen = szStrLen;
    if (szActualIndex < szIndex)
    {
        psStrPt->pcStrStart = NULL;
        psStrPt->szLen = 0;
    }
}

PINCFG_STR_RESULT_T PinCfgStr_eAtoU8(const STRING_POINT_T *psStrPt, uint8_t *pu8Out)
{
    if ((psStrPt->szLen + 1) > 4)
        return PINCFG_STR_INSUFFICIENT_BUFFER_E;

    char cTempStr[4];
    memcpy(cTempStr, psStrPt->pcStrStart, psStrPt->szLen);
    cTempStr[psStrPt->szLen] = '\0';
    errno = 0;
    *pu8Out = (uint8_t)atoi(cTempStr);
    if (errno)
        return PINCFG_STR_UNSUCCESSFULL_CONVERSION_E;

    return PINCFG_STR_OK_E;
}

PINCFG_STR_RESULT_T PinCfgStr_eSplitStrPt(
    const STRING_POINT_T *psInStrPt,
    const char cDelimiter,
    STRING_POINT_T *pasOut,
    uint8_t *u8OutLen,
    const uint8_t u8OutMaxLen)
{
    *u8OutLen = 0;
    size_t szStrStart = 0;
    pasOut[*u8OutLen].pcStrStart = psInStrPt->pcStrStart;
    for (size_t i = 0; i < psInStrPt->szLen; i++)
    {
        szStrStart++;
        if (psInStrPt->pcStrStart[i] == cDelimiter)
        {
            pasOut[*u8OutLen].szLen = szStrStart - 1;
            if (*u8OutLen == u8OutMaxLen)
            {
                return PINCFG_STR_MAXLEN_ERROR_E;
            }
            (*u8OutLen)++;
            pasOut[*u8OutLen].pcStrStart = &(psInStrPt->pcStrStart[i + 1]);
            szStrStart = 0;
        }
    }
    pasOut[*u8OutLen].szLen = szStrStart;
    (*u8OutLen)++;

    return PINCFG_STR_OK_E;
}
PINCFG_STR_RESULT_T PinCfgStr_eRemoveAt(STRING_POINT_T *pasStrPts, uint8_t *u8StrPtsLen, const uint8_t u8Index)
{
    if (u8Index >= *u8StrPtsLen)
    {
        return PINCFG_STR_INDEXOUTOFARRAY_ERROR_E;
    }

    for (uint8_t i = u8Index; i < (*u8StrPtsLen) - 1; i++)
    {
        pasStrPts[i] = pasStrPts[i + 1];
    }
    (*u8StrPtsLen)--;
    memset(&pasStrPts[*u8StrPtsLen], 0x00U, sizeof(STRING_POINT_T));

    return PINCFG_STR_OK_E;
}

PINCFG_STR_RESULT_T PinCfgStr_eRemoveEmpty(STRING_POINT_T *pasStrPts, uint8_t *u8StrPtsLen)
{
    uint8_t i = 0;
    while (true)
    {
        if (pasStrPts[i].szLen == 0)
        {
            PinCfgStr_eRemoveAt(pasStrPts, u8StrPtsLen, i);
            i--;
        }
        i++;
        if (i >= *u8StrPtsLen)
        {
            break;
        }
    }

    return PINCFG_STR_OK_E;
}

PINCFG_STR_RESULT_T PinCfgStr_eRemoveStartingWith(STRING_POINT_T *pasStrPts, uint8_t *u8StrPtsLen, const char cStart)
{
    uint8_t i = 0;
    while (true)
    {
        if (*(pasStrPts[i].pcStrStart) == cStart)
        {
            PinCfgStr_eRemoveAt(pasStrPts, u8StrPtsLen, i);
            i--;
        }
        i++;
        if (i >= *u8StrPtsLen)
        {
            break;
        }
    }

    return PINCFG_STR_OK_E;
}
