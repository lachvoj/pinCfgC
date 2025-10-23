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

/**
 * @brief Get configuration data size (excluding password)
 * Returns size of config data only, without password section
 * @param pu16CfgSize Output parameter for config data size (total size - password length)
 * @return PERCFG_OK_E on success
 */
PERCFG_RESULT_T PersistentCfg_eGetConfigSize(uint16_t *pu16CfgSize);

/**
 * @brief Read password from persistent configuration
 * Password is stored at PINCFG_PASSWORD_OFFSET (first PINCFG_AUTH_PASSWORD_MAX_LEN_D bytes)
 * @param pcPasswordOut Output buffer (PINCFG_AUTH_PASSWORD_MAX_LEN_D bytes)
 * @return PERCFG_OK_E on success
 */
PERCFG_RESULT_T PersistentCfg_eReadPassword(char *pcPasswordOut);

/**
 * @brief Write password to persistent configuration
 * Updates password at PINCFG_PASSWORD_OFFSET and recalculates all checksums
 * @param pcPassword New password (null-terminated, max PINCFG_AUTH_PASSWORD_MAX_LEN_D-1 chars)
 * @return PERCFG_OK_E on success
 */
PERCFG_RESULT_T PersistentCfg_eWritePassword(const char *pcPassword);

/**
 * @brief Load configuration data without password section
 * Skips password section (PINCFG_CONFIG_OFFSET bytes) and reads config data
 * @param pcCfg Output buffer for configuration (size = PINCFG_CONFIG_DATA_MAX_SZ_D)
 * @return PERCFG_OK_E on success
 */
PERCFG_RESULT_T PersistentCfg_eLoadConfig(char *pcCfg);

/**
 * @brief Save configuration data preserving existing password
 * Reads current password, prepends it to config, and saves
 * @param pcCfg Configuration data (without password)
 * @param u16CfgSize Size of configuration data (max PINCFG_CONFIG_DATA_MAX_SZ_D)
 * @return PERCFG_OK_E on success
 */
PERCFG_RESULT_T PersistentCfg_eSaveConfig(const char *pcCfg, uint16_t u16CfgSize);

// Internal functions exposed for unit testing only
// These work with full config including password - DO NOT use in application code
#ifdef UNIT_TEST
PERCFG_RESULT_T PersistentCfg_eGetSize(uint16_t *pu16CfgSize);
PERCFG_RESULT_T PersistentCfg_eSave(const char *pcCfg);
PERCFG_RESULT_T PersistentCfg_eLoad(char *pcCfg);
#endif

#endif // PERSISTENTCONFIGURATION_H