#include <stdio.h>

#include "PinCfgCsv.h"
#include "PinCfgMessages.h"
#include "PinCfgParse.h"
#include "PinCfgStr.h"


// Error message strings - conditional compilation for size optimization
#ifdef PINCFG_USE_ERROR_MESSAGES
static const char *_errorMessages[] = {
    "",                              // 0 - unused
    "NULL configuration",            // 1 - ERR_NULL_CONFIG
    "Empty configuration",           // 2 - ERR_EMPTY_CONFIG
    "Not defined or invalid format", // 3 - ERR_UNDEFINED_FORMAT
    "Unknown type",                  // 4 - ERR_UNKNOWN_TYPE
    "Invalid number of arguments",   // 5 - ERR_INVALID_ARGS
    "Invalid number of items",       // 6 - ERR_INVALID_ITEMS
    "Invalid definition",            // 7 - ERR_INVALID_DEFINITION
    "Invalid pin number",            // 8 - ERR_INVALID_PIN
    "Invalid time period",           // 9 - ERR_INVALID_TIME_PERIOD
    "OOM",                           // 10 - ERR_OOM
    "Init failed",                   // 11 - ERR_INIT_FAILED
    "Switch not found",              // 12 - ERR_SWITCH_NOT_FOUND
    "InPin not found",               // 13 - ERR_INPIN_NOT_FOUND
    "Source not found",              // 14 - ERR_SOURCE_NOT_FOUND
    "Invalid event type",            // 15 - ERR_INVALID_EVENT_TYPE
    "Invalid event count",           // 16 - ERR_INVALID_EVENT_COUNT
    "Invalid switch action",         // 17 - ERR_INVALID_SWITCH_ACTION
    "Nothing to drive",              // 18 - ERR_NOTHING_TO_DRIVE
    "Invalid type enum",             // 19 - ERR_INVALID_TYPE_ENUM
    "Invalid number",                // 20 - ERR_INVALID_NUMBER
    "Measurement not found",         // 21 - ERR_MEASUREMENT_NOT_FOUND
    "Invalid V_TYPE",                // 22 - ERR_INVALID_VTYPE
    "Invalid S_TYPE",                // 23 - ERR_INVALID_STYPE
    "Invalid enableable",            // 24 - ERR_INVALID_ENABLEABLE
    "Invalid cumulative",            // 25 - ERR_INVALID_CUMULATIVE
    "Invalid sampling interval",     // 26 - ERR_INVALID_SAMPLING_INTV
    "Invalid report interval",       // 27 - ERR_INVALID_REPORT_INTV
    "Invalid sensor scale",          // 28 - ERR_INVALID_SCALE
    "Invalid sensor offset",         // 29 - ERR_INVALID_OFFSET
    "Invalid sensor precision",      // 30 - ERR_INVALID_PRECISION
    "Invalid byte offset",           // 31 - ERR_INVALID_BYTE_OFFSET
    "Invalid byte count",            // 32 - ERR_INVALID_BYTE_COUNT
    "Type not implemented",          // 33 - ERR_TYPE_NOT_IMPLEMENTED
    "Invalid I2C params",            // 32 - ERR_INVALID_I2C_PARAMS
    "Invalid I2C address",           // 33 - ERR_INVALID_I2C_ADDRESS
    "Invalid I2C command",           // 34 - ERR_INVALID_I2C_CMD
    "Invalid data size",             // 35 - ERR_INVALID_DATA_SIZE
    "Invalid global config"          // 36 - ERR_INVALID_GLOBAL_CFG
};
#endif

// Parse strings - format strings and common text
static const char *_parseStrings[] = {
    "%s%s\n",             // FSS_E
    "%s%s%s\n",           // FSSS_E
    "%s%d:%s\n",          // FSDS_E
    "%s%d:%s%s\n",        // FSDSS_E
    "%s%d:%s%s%s\n",      // FSDSSS_E
    "%s%d:%s%s (0-%d)\n", // FSDSSD_E
    "E:",                 // E_E
    "W:",                 // W_E
    "E:L:",               // EL_E
    "W:L:",               // WL_E
    "I:",                 // I_E
#ifdef PINCFG_USE_ERROR_MESSAGES
    "CLI:",                               // ECR_E
    "Switch:",                            // SW_E
    "InPin:",                             // IP_E
    "Trigger:",                           // TRG_E
    "CPUTemperature",                     // CPUTMP_E
    "InPinDebounceMs:",                   // IPDMS_E
    "InPinMulticlickMaxDelayMs:",         // IPMCDMS_E
    "SwitchImpulseDurationMs:",           // SWIDMS_E
    "SwitchFbDelayMs:",                 // SWFNDMS_E
    "SwitchFbOffDelayMs:",                // SWFFDMS_E
    "OOM",                                // OOM_E
    "init failed",                        // INITF_E
    "Invalid pin number.",                // IPN_E
    "invalid",                            // IVLD_E
    "Invalid number",                     // IN_E
    " of arguments.",                     // OARGS_E
    " of items defining names and pins.", // OITMS_E
    "Switch name not found.",             // SNNF_E
    "MS",                                 // MS_E
    "SR",                                 // SR_E
    " type enum",                         // TE_E
    " args\n",                            // ARGS_E
    " (name)\n",                          // NAME_E
    " (cputemp)\n",                       // CPUTEMP_E
    " type not implemented\n",            // TNI_E
    " (sensor)\n",                        // SENSOR_E
    " measurement not found: ",           // MNF_E
    "\n",                                 // NL_E
    " scale\n",                           // SCALE_E
    " offset\n",                          // OFFSET_E
    " precision\n",                       // PRECISION_E
    " enableable\n",                      // ENABLEABLE_E
    " cumulative\n",                      // CUMULATIVE_E
    " sampling interval\n",               // SAMPINT_E
    " report interval\n"                  // REPINT_E
#endif
};

const char *PinCfgMessages_getString(PINCFG_PARSE_STRINGS_T eStr)
{
#ifdef PINCFG_USE_ERROR_MESSAGES
    return _parseStrings[eStr];
#else
    // Compact mode - return base strings or empty string
    if (eStr <= I_E)
    {
        return _parseStrings[eStr];
    }
    else
    {
        return ""; // All extended strings return empty string in compact mode
    }
#endif
}

// Unified error logging - automatically handles PINCFG_USE_ERROR_MESSAGES mode
// Also increments warning counter automatically
size_t PinCfgMessages_logParseError(
    PINCFG_PARSE_SUBFN_PARAMS_T *psPrms,
    const char *prefix,
    PINCFG_ERROR_CODE_T errorCode,
    bool isFatal)
{
    // Increment warning counter if this is a warning (not fatal)
    if (!isFatal)
    {
        psPrms->szNumberOfWarnings++;
    }

    size_t result;
#ifdef PINCFG_USE_ERROR_MESSAGES
    result = snprintf(
        (char *)(psPrms->psParsePrms->pcOutString + psPrms->pcOutStringLast),
        szGetSize(psPrms->psParsePrms->u16OutStrMaxLen, psPrms->pcOutStringLast),
        PinCfgMessages_getString(FSDSS_E),
        isFatal ? PinCfgMessages_getString(EL_E) : PinCfgMessages_getString(WL_E),
        psPrms->u16LinesProcessed,
        prefix,
        _errorMessages[errorCode]);
#else
    (void)prefix; // Unused in compact mode
    result = snprintf(
        (char *)(psPrms->psParsePrms->pcOutString + psPrms->pcOutStringLast),
        szGetSize(psPrms->psParsePrms->u16OutStrMaxLen, psPrms->pcOutStringLast),
        isFatal ? "L%d:E%d;" : "L%d:W%d;",
        psPrms->u16LinesProcessed,
        errorCode);
#endif
    return result;
}

// Helper for simple error without line number (E: or W:)
size_t PinCfgMessages_logSimpleError(
    char *pcOutString,
    size_t pcOutStringLast,
    uint16_t u16OutStrMaxLen,
    PINCFG_ERROR_CODE_T errorCode,
    bool isFatal)
{
#ifdef PINCFG_USE_ERROR_MESSAGES
    return snprintf(
        (char *)(pcOutString + pcOutStringLast),
        szGetSize(u16OutStrMaxLen, pcOutStringLast),
        "%s%s\n",
        isFatal ? PinCfgMessages_getString(E_E) : PinCfgMessages_getString(W_E),
        _errorMessages[errorCode]);
#else
    return snprintf(
        (char *)(pcOutString + pcOutStringLast),
        szGetSize(u16OutStrMaxLen, pcOutStringLast),
        isFatal ? "E%d\n" : "W%d\n",
        errorCode);
#endif
}
