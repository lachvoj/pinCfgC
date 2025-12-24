#ifndef PERSISTENTCONFIGURATION_H
#define PERSISTENTCONFIGURATION_H

#include "MySensorsWrapper.h"

typedef enum
{
    PERCFG_OK_E = 0,
    PERCFG_WARNINGS_E,
    PERCFG_CFG_BIGGER_THAN_MAX_SZ_E,
    PERCFG_WRITE_SIZE_FAILED_E,
    PERCFG_WRITE_CHECKSUM_FAILED_E,
    PERCFG_WRITE_CFG_FAILED_E,
    PERCFG_READ_SIZE_FAILED_E,
    PERCFG_READ_CHECKSUM_FAILED_E,
    PERCFG_READ_FAILED_E,
    PERCFG_READ_AND_BACKUP_FAILED_E,
    PERCFG_INVALID_E,
    PERCFG_OUT_OF_RANGE_E,
    PERCFG_ERROR_E
} PERCFG_RESULT_T;

PERCFG_RESULT_T PersistentCfg_eGetConfigSize(uint16_t *pu16CfgSize);

// Password read/write - work with 32-byte binary SHA-256 hash
PERCFG_RESULT_T PersistentCfg_eReadPassword(uint8_t *pu8PasswordOut);

PERCFG_RESULT_T PersistentCfg_eWritePassword(const uint8_t *pu8Password);

PERCFG_RESULT_T PersistentCfg_eLoadConfig(char *pcCfg);

PERCFG_RESULT_T PersistentCfg_eSaveConfig(const char *pcCfg);

// Internal functions exposed for unit testing only
// These work with full config including password - DO NOT use in application code
#ifdef UNIT_TEST
PERCFG_RESULT_T PersistentCfg_eGetSize(uint16_t *pu16CfgSize);
PERCFG_RESULT_T PersistentCfg_eSave(const char *pcCfg);
PERCFG_RESULT_T PersistentCfg_eLoad(char *pcCfg);
#endif

#endif // PERSISTENTCONFIGURATION_H