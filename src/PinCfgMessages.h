/**
 * @file PinCfgMessages.h
 * @brief Error codes and message strings for PinCfg parser
 * 
 * This module centralizes all error codes and message strings used in the parser,
 * supporting both compact and verbose error message modes via
 * conditional compilation (USE_ERROR_MESSAGES flag).
 */

#ifndef PINCFG_MESSAGES_H
#define PINCFG_MESSAGES_H

#include <stdint.h>
#include <stdbool.h>

// Forward declarations to avoid circular dependencies
typedef struct PINCFG_PARSE_SUBFN_PARAMS_S PINCFG_PARSE_SUBFN_PARAMS_T;

// Error codes for parser diagnostics
typedef enum PINCFG_ERROR_CODE_E
{
    ERR_NULL_CONFIG = 1,
    ERR_EMPTY_CONFIG,
    ERR_UNDEFINED_FORMAT,
    ERR_UNKNOWN_TYPE,
    ERR_INVALID_ARGS,
    ERR_INVALID_ITEMS,
    ERR_INVALID_DEFINITION,
    ERR_INVALID_PIN,
    ERR_INVALID_TIME_PERIOD,
    ERR_OOM,
    ERR_INIT_FAILED,
    ERR_SWITCH_NOT_FOUND,
    ERR_INPIN_NOT_FOUND,
    ERR_SOURCE_NOT_FOUND,
    ERR_INVALID_EVENT_TYPE,
    ERR_INVALID_EVENT_COUNT,
    ERR_INVALID_SWITCH_ACTION,
    ERR_NOTHING_TO_DRIVE,
    ERR_INVALID_TYPE_ENUM,
    ERR_INVALID_NUMBER,
    ERR_MEASUREMENT_NOT_FOUND,
    ERR_INVALID_VTYPE,
    ERR_INVALID_STYPE,
    ERR_INVALID_ENABLEABLE,
    ERR_INVALID_CUMULATIVE,
    ERR_INVALID_SAMPLING_INTV,
    ERR_INVALID_REPORT_INTV,
    ERR_INVALID_BYTE_OFFSET,
    ERR_INVALID_BYTE_COUNT,
    ERR_TYPE_NOT_IMPLEMENTED,
    ERR_INVALID_I2C_PARAMS,
    ERR_INVALID_I2C_ADDRESS,
    ERR_INVALID_I2C_CMD,
    ERR_INVALID_DATA_SIZE,
    ERR_INVALID_GLOBAL_CFG
} PINCFG_ERROR_CODE_T;

// Parse string indices - used for accessing common strings
// NOTE: ALL enum values are always defined (enums have no binary footprint)
// Only the actual string array is conditionally compiled
typedef enum PINCFG_PARSE_STRINGS_E
{
    // Base strings - always available
    FSS_E = 0,      // "%s%s\n"
    FSSS_E,         // "%s%s%s\n"
    FSDS_E,         // "%s%d:%s\n"
    FSDSS_E,        // "%s%d:%s%s\n"
    FSDSSS_E,       // "%s%d:%s%s%s\n"
    FSDSSD_E,       // "%s%d:%s%s (0-%d)\n"
    E_E,            // "E:"
    W_E,            // "W:"
    EL_E,           // "E:L:"
    WL_E,           // "W:L:"
    I_E,            // "I:"
    // Extended strings - returned as empty string in compact mode
    ECR_E,          // "CLI:"
    SW_E,           // "Switch:"
    IP_E,           // "InPin:"
    TRG_E,          // "Trigger:"
    CPUTMP_E,       // "CPUTemperature"
    IPDMS_E,        // "InPinDebounceMs:"
    IPMCDMS_E,      // "InPinMulticlickMaxDelayMs:"
    SWIDMS_E,       // "SwitchImpulseDurationMs:"
    SWFNDMS_E,      // "SwitchFbOnDelayMs:"
    SWFFDMS_E,      // "SwitchFbOffDelayMs:"
    OOM_E,          // "OOM"
    INITF_E,        // "init failed"
    IPN_E,          // "Invalid pin number."
    IVLD_E,         // "invalid"
    IN_E,           // "Invalid number"
    OARGS_E,        // " of arguments."
    OITMS_E,        // " of items defining names and pins."
    SNNF_E,         // "Switch name not found."
    MS_E,           // "MS"
    SR_E,           // "SR"
    TE_E,           // " type enum"
    ARGS_E,         // " args\n"
    NAME_E,         // " (name)\n"
    CPUTEMP_E,      // " (cputemp)\n"
    TNI_E,          // " type not implemented\n"
    SENSOR_E,       // " (sensor)\n"
    MNF_E,          // " measurement not found: "
    NL_E,           // "\n"
    OFFSET_E,       // " offset\n"
    ENABLEABLE_E,   // " enableable\n"
    CUMULATIVE_E,   // " cumulative\n"
    SAMPINT_E,      // " sampling interval\n"
    REPINT_E        // " report interval\n"
} PINCFG_PARSE_STRINGS_T;

/**
 * @brief Get a parser string by index
 * @param eStr String index from PINCFG_PARSE_STRINGS_T
 * @return Pointer to the string (may be empty string in compact mode)
 */
const char* PinCfgMessages_getString(PINCFG_PARSE_STRINGS_T eStr);

/**
 * @brief Log a parse error with line number and context
 * @param psPrms Parser subfunction parameters
 * @param prefix Context prefix string (e.g., "Switch:", "InPin:")
 * @param errorCode Error code from PINCFG_ERROR_CODE_T
 * @param isFatal True for errors (E:), false for warnings (W:)
 * @return Number of characters written
 */
size_t PinCfgMessages_logParseError(PINCFG_PARSE_SUBFN_PARAMS_T *psPrms, 
                                    const char *prefix, 
                                    PINCFG_ERROR_CODE_T errorCode,
                                    bool isFatal);

/**
 * @brief Log a simple error without line number
 * @param pcOutString Output string buffer
 * @param pcOutStringLast Current position in buffer
 * @param u16OutStrMaxLen Maximum buffer length
 * @param errorCode Error code from PINCFG_ERROR_CODE_T
 * @param isFatal True for errors (E:), false for warnings (W:)
 * @return Number of characters written
 */
size_t PinCfgMessages_logSimpleError(char *pcOutString, 
                                     size_t pcOutStringLast,
                                     uint16_t u16OutStrMaxLen,
                                     PINCFG_ERROR_CODE_T errorCode,
                                     bool isFatal);

// Convenience macros for logging
#define LOG_WARNING(psPrms, prefix, errorCode) \
    PinCfgMessages_logParseError(psPrms, prefix, errorCode, false)

#define LOG_ERROR(psPrms, prefix, errorCode) \
    PinCfgMessages_logParseError(psPrms, prefix, errorCode, true)

#define LOG_SIMPLE_ERROR(pcOutString, pcOutStringLast, u16MaxLen, errorCode) \
    PinCfgMessages_logSimpleError(pcOutString, pcOutStringLast, u16MaxLen, errorCode, true)

#define LOG_SIMPLE_WARNING(pcOutString, pcOutStringLast, u16MaxLen, errorCode) \
    PinCfgMessages_logSimpleError(pcOutString, pcOutStringLast, u16MaxLen, errorCode, false)

#endif // PINCFG_MESSAGES_H
