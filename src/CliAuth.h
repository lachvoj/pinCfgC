#ifndef CLI_AUTH_H
#define CLI_AUTH_H

#include "Types.h"
#include <stdbool.h>

// Default password (change after first deployment!)
#define CLI_AUTH_DEFAULT_PASSWORD "admin123"

// Password is stored as the FIRST PINCFG_AUTH_PASSWORD_MAX_LEN_D bytes of the configuration
// This way it benefits from all PersistentConfiguration features:
// - CRC16 integrity checking
// - Triple metadata voting
// - Block checksums  
// - Dynamic backup
// - Fault-tolerant recovery

typedef enum CLI_AUTH_RESULT_E
{
    CLI_AUTH_OK_E = 0,
    CLI_AUTH_WRONG_PASSWORD_E,
    CLI_AUTH_NULLPTR_ERROR_E,
    CLI_AUTH_EEPROM_ERROR_E,
    CLI_AUTH_ERROR_E
} CLI_AUTH_RESULT_T;

/**
 * @brief Verify password against stored password in EEPROM
 * Reads password from persistent configuration each time (no caching)
 * @param pcPassword Password string to verify (null-terminated)
 * @return CLI_AUTH_OK_E if password matches, CLI_AUTH_WRONG_PASSWORD_E otherwise
 */
CLI_AUTH_RESULT_T CliAuth_eVerifyPassword(const char *pcPassword);

/**
 * @brief Change the stored password
 * Updates password in configuration using PersistentCfg_eWritePassword
 * @param pcCurrentPassword Current password (null-terminated)
 * @param pcNewPassword New password (null-terminated, max PINCFG_AUTH_PASSWORD_MAX_LEN_D-1 chars)
 * @return CLI_AUTH_OK_E on success, CLI_AUTH_WRONG_PASSWORD_E if current password incorrect
 */
CLI_AUTH_RESULT_T CliAuth_eChangePassword(const char *pcCurrentPassword, const char *pcNewPassword);

#endif // CLI_AUTH_H
