#include <errno.h>
#include <stdlib.h>

#include "Memory.h"
#include "PinCfgStr.h"

void PinCfgStr_vInitStrPoint(STRING_POINT_T *psStrPoint, const char *pcStrStart, size_t szLen)
{
    psStrPoint->pcStrStart = pcStrStart;
    psStrPoint->szLen = szLen;
}

size_t PinCfgStr_szGetSplitCount(const STRING_POINT_T *psInStrPt, const char cDelimiter)
{
    size_t szCount = 0;

    // Handle empty string
    if (psInStrPt->szLen == 0)
        return 0;

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
    if (psStrPt->szLen > 4) // Allow "0xFF" (4 chars)
        return PINCFG_STR_INSUFFICIENT_BUFFER_E;

    char cTempStr[5];
    char *endptr = NULL;
    memcpy(cTempStr, psStrPt->pcStrStart, psStrPt->szLen);
    cTempStr[psStrPt->szLen] = '\0';
    errno = 0;
    *pu8Out = (uint8_t)strtoul(cTempStr, &endptr, 0); // Base 0 = auto-detect (0x=hex, else decimal)
    if (cTempStr == endptr || errno != 0 || *endptr != '\0')
        return PINCFG_STR_UNSUCCESSFULL_CONVERSION_E;

    return PINCFG_STR_OK_E;
}

PINCFG_STR_RESULT_T PinCfgStr_eAtoU32(const STRING_POINT_T *psStrPt, uint32_t *pu32Out)
{
    if (psStrPt->szLen > 10) // "0xFFFFFFFF" = 10 chars (already fits)
        return PINCFG_STR_INSUFFICIENT_BUFFER_E;

    char cTempStr[11];
    char *endptr = NULL;
    memcpy(cTempStr, psStrPt->pcStrStart, psStrPt->szLen);
    cTempStr[psStrPt->szLen] = '\0';
    errno = 0;
    *pu32Out = strtoul(cTempStr, &endptr, 0); // Base 0 = auto-detect (0x=hex, else decimal)
    if (cTempStr == endptr || errno != 0 || *endptr != '\0')
        return PINCFG_STR_UNSUCCESSFULL_CONVERSION_E;

    return PINCFG_STR_OK_E;
}

PINCFG_STR_RESULT_T PinCfgStr_eAtoFixedPoint(const STRING_POINT_T *psStrPt, int32_t *i32Out)
{
    if (psStrPt == NULL || i32Out == NULL)
        return PINCFG_STR_INSUFFICIENT_BUFFER_E;

    if (psStrPt->szLen == 0 || psStrPt->szLen > 15)
        return PINCFG_STR_INSUFFICIENT_BUFFER_E;

    // Copy to null-terminated buffer (larger for "2147.999999")
    char cTempStr[16];
    memcpy(cTempStr, psStrPt->pcStrStart, psStrPt->szLen);
    cTempStr[psStrPt->szLen] = '\0';

    // Find decimal point
    char *pcDecimal = strchr(cTempStr, '.');

    if (pcDecimal == NULL)
    {
        // No decimal point - parse as integer and multiply by 1e6
        char *endptr = NULL;
        errno = 0;
        int32_t i32Integer = (int32_t)strtol(cTempStr, &endptr, 10);

        if (cTempStr == endptr || errno != 0)
            return PINCFG_STR_UNSUCCESSFULL_CONVERSION_E;

        // Check overflow before multiply
        if (i32Integer > 2147 || i32Integer < -2147)
            return PINCFG_STR_UNSUCCESSFULL_CONVERSION_E;

        *i32Out = i32Integer * PINCFG_FIXED_POINT_SCALE;
        return PINCFG_STR_OK_E;
    }

    // Has decimal point - parse integer and fractional parts separately
    *pcDecimal = '\0'; // Temporarily null-terminate integer part

    // Parse integer part
    char *endptr = NULL;
    errno = 0;
    int32_t i32Integer = 0;
    bool bNegative = (cTempStr[0] == '-');

    if (cTempStr[0] != '\0' && !(cTempStr[0] == '-' && cTempStr[1] == '\0'))
    {
        i32Integer = (int32_t)strtol(cTempStr, &endptr, 10);
        if (cTempStr == endptr || errno != 0)
        {
            return PINCFG_STR_UNSUCCESSFULL_CONVERSION_E;
        }

        // Check range
        if (i32Integer > 2147 || i32Integer < -2147)
        {
            return PINCFG_STR_UNSUCCESSFULL_CONVERSION_E;
        }
    }

    // Parse fractional part (up to 6 digits)
    char *pcFraction = pcDecimal + 1;
    uint8_t u8FracLen = (uint8_t)strlen(pcFraction);

    if (u8FracLen > 6)
    {
        u8FracLen = 6; // Truncate to 6 digits
    }

    // Extract up to 6 digits from fraction
    char cFracStr[7];
    memcpy(cFracStr, pcFraction, u8FracLen);
    cFracStr[u8FracLen] = '\0';

    // Parse fraction as integer
    errno = 0;
    int32_t i32Fraction = 0;
    if (u8FracLen > 0)
    {
        i32Fraction = (int32_t)strtol(cFracStr, &endptr, 10);
        if (cFracStr == endptr || errno != 0)
        {
            return PINCFG_STR_UNSUCCESSFULL_CONVERSION_E;
        }
    }

    // Scale fraction to 6 decimal places
    while (u8FracLen < 6)
    {
        i32Fraction *= 10;
        u8FracLen++;
    }

    // Combine parts using int64_t to avoid overflow
    int64_t i64Result = (int64_t)i32Integer * PINCFG_FIXED_POINT_SCALE;
    if (bNegative)
    {
        i64Result -= i32Fraction; // Negative numbers
    }
    else
    {
        i64Result += i32Fraction; // Positive numbers
    }

    // Check if result fits in int32_t
    if (i64Result > 2147483647LL || i64Result < -2147483648LL)
    {
        return PINCFG_STR_UNSUCCESSFULL_CONVERSION_E;
    }

    *i32Out = (int32_t)i64Result;
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
    while (i < *u8StrPtsLen)
    {
        if (pasStrPts[i].szLen == 0)
        {
            PinCfgStr_eRemoveAt(pasStrPts, u8StrPtsLen, i);
            // Don't increment i - retry same index after removal
        }
        else
        {
            i++;
        }
    }

    return PINCFG_STR_OK_E;
}

PINCFG_STR_RESULT_T PinCfgStr_eRemoveStartingWith(STRING_POINT_T *pasStrPts, uint8_t *u8StrPtsLen, const char cStart)
{
    uint8_t i = 0;
    while (i < *u8StrPtsLen)
    {
        if (*(pasStrPts[i].pcStrStart) == cStart)
        {
            PinCfgStr_eRemoveAt(pasStrPts, u8StrPtsLen, i);
            // Don't increment i - retry same index after removal
        }
        else
        {
            i++;
        }
    }

    return PINCFG_STR_OK_E;
}

const char *PinCfgCsv_pcStrstrpt(const char *pcHaystack, const STRING_POINT_T *psNeedle)
{
    size_t szHaystackLength = strlen(pcHaystack);
    for (size_t i = 0; i < szHaystackLength; i++)
    {
        if (i + psNeedle->szLen > szHaystackLength)
        {
            return NULL;
        }
        if (strncmp(&pcHaystack[i], psNeedle->pcStrStart, psNeedle->szLen) == 0)
        {
            return &pcHaystack[i];
        }
    }

    return NULL;
}
