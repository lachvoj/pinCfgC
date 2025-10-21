/**
 * @file PinCfgParse.h
 * @brief Parser internal types and structures
 * 
 * This header contains types used internally by the parser subsystem.
 * Separated to avoid circular dependencies between modules.
 */

#ifndef PINCFG_PARSE_H
#define PINCFG_PARSE_H

#include <stdint.h>

// Forward declaration from PinCfgCsv.h
typedef struct PINCFG_PARSE_PARAMS_S PINCFG_PARSE_PARAMS_T;
typedef struct STRING_POINT_S STRING_POINT_T;
typedef struct LINKEDLIST_ITEM_S LINKEDLIST_ITEM_T;

// Subfunction parameter structure - used internally by parser subfunctions
typedef struct PINCFG_PARSE_SUBFN_PARAMS_S
{
    PINCFG_PARSE_PARAMS_T *psParsePrms;
    size_t pcOutStringLast;
    uint16_t u16LinesProcessed;
    uint8_t u8LineItemsLen;
    uint8_t u8PresentablesCount;
    size_t szNumberOfWarnings;
    STRING_POINT_T sLine;
    STRING_POINT_T sTempStrPt;
    LINKEDLIST_ITEM_T *psMeasurementsListHead;  // Measurements during parsing only
} PINCFG_PARSE_SUBFN_PARAMS_T;

#endif // PINCFG_PARSE_H
