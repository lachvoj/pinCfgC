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
    PERCFG_ERROR_E
} PERCFG_RESULT_T;

PERCFG_RESULT_T PersistentCfg_eSave(const char *pcCfg);
PERCFG_RESULT_T PersistentCfg_eLoad(const char *pcCfg);
PERCFG_RESULT_T PersistentCfg_eGetSize(uint16_t *pu16CfgSize);
// PERCFG_RESULT_T PersistentCfg_eSaveLine(const char *pcCfg, const size_t szOffset);


#endif // PERSISTENTCONFIGURATION_H