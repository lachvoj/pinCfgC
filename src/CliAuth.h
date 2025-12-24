#ifndef CLI_AUTH_H
#define CLI_AUTH_H

#include <stdbool.h>

#include "Types.h"

// Default password (SHA-256 hash of "admin123" as hex string)
// CHANGE AFTER FIRST DEPLOYMENT!
#define CLI_AUTH_DEFAULT_PASSWORD "240be518fabd2724ddb6f04eeb1da5967448d7e831c08c8fa822809f74c720a9"

// Password is stored as 32-byte binary SHA-256 hash in EEPROM
// CLI receives password as 64-char hex string, which is converted to binary for storage/comparison
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
    CLI_AUTH_LOCKED_OUT_E,
    CLI_AUTH_NULLPTR_ERROR_E,
    CLI_AUTH_EEPROM_ERROR_E,
    CLI_AUTH_ERROR_E
} CLI_AUTH_RESULT_T;

// Rate limiting configuration
#define CLI_AUTH_MAX_LOCKOUT_MS (5UL * 60UL * 1000UL) // 5 minutes max lockout
#define CLI_AUTH_BASE_LOCKOUT_MS 1000UL               // 1 second initial lockout

CLI_AUTH_RESULT_T CliAuth_eVerifyPassword(const char *pcPassword);

// Set password directly (use when caller has already verified authentication)
CLI_AUTH_RESULT_T CliAuth_eSetPassword(const char *pcNewPassword);

CLI_AUTH_RESULT_T CliAuth_eChangePassword(const char *pcCurrentPassword, const char *pcNewPassword);

// Convert hex string (64 chars) to binary hash (32 bytes)
// Returns true on success, false if invalid format
bool CliAuth_bHexToBinary(const char *pcHexString, uint8_t *pu8Binary, size_t u32BinarySize);

CLI_AUTH_RESULT_T CliAuth_eVerifyPasswordRateLimited(
    const char *pcPassword,
    uint32_t u32CurrentMs,
    uint8_t *pu8FailedAttempts,
    uint32_t *pu32LockoutUntilMs);

uint32_t CliAuth_u32GetRemainingLockoutMs(uint32_t u32CurrentMs, uint32_t u32LockoutUntilMs);

#endif // CLI_AUTH_H
