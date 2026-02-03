#include "CliAuth.h"

#include <string.h>

#include "PersistentConfiguration.h"

// No global variables - read password from EEPROM when needed

// Convert single hex character to nibble (4 bits)
static uint8_t hexCharToNibble(char c)
{
    if (c >= '0' && c <= '9')
        return c - '0';
    if (c >= 'a' && c <= 'f')
        return c - 'a' + 10;
    if (c >= 'A' && c <= 'F')
        return c - 'A' + 10;
    return 0xFF; // Invalid
}

// Convert hex string (64 chars) to binary hash (32 bytes)
bool CliAuth_bHexToBinary(const char *pcHexString, uint8_t *pu8Binary, size_t u32BinarySize)
{
    if (pcHexString == NULL || pu8Binary == NULL)
        return false;

    // Check if hex string is correct length (2 chars per byte)
    size_t hexLen = strlen(pcHexString);
    if (hexLen != u32BinarySize * 2)
        return false;

    for (size_t i = 0; i < u32BinarySize; i++)
    {
        uint8_t highNibble = hexCharToNibble(pcHexString[i * 2]);
        uint8_t lowNibble = hexCharToNibble(pcHexString[i * 2 + 1]);

        if (highNibble == 0xFF || lowNibble == 0xFF)
            return false; // Invalid hex character

        pu8Binary[i] = (highNibble << 4) | lowNibble;
    }

    return true;
}

// Default password as binary (SHA-256 hash of "admin123")
static const uint8_t _au8DefaultPasswordBinary[32] = {0x24, 0x0b, 0xe5, 0x18, 0xfa, 0xbd, 0x27, 0x24, 0xdd, 0xb6, 0xf0,
                                                      0x4e, 0xeb, 0x1d, 0xa5, 0x96, 0x74, 0x48, 0xd7, 0xe8, 0x31, 0xc0,
                                                      0x8c, 0x8f, 0xa8, 0x22, 0x80, 0x9f, 0x74, 0xc7, 0x20, 0xa9};

// Helper: Check if binary buffer is all zeros or all 0xFF (empty/erased EEPROM)
static bool isPasswordEmpty(const uint8_t *pu8Password)
{
    bool bAllZeros = true;
    bool bAllFF = true;

    for (size_t i = 0; i < PINCFG_AUTH_PASSWORD_LEN_D; i++)
    {
        if (pu8Password[i] != 0x00)
            bAllZeros = false;
        if (pu8Password[i] != 0xFF)
            bAllFF = false;
    }

    return bAllZeros || bAllFF;
}

CLI_AUTH_RESULT_T CliAuth_eVerifyPassword(const char *pcPassword)
{
    if (pcPassword == NULL)
        return CLI_AUTH_NULLPTR_ERROR_E;

    // Validate password length (SHA-256 hex = 64 chars)
    size_t passwordLen = strlen(pcPassword);
    if (passwordLen != PINCFG_AUTH_HEX_PASSWORD_LEN_D)
        return CLI_AUTH_WRONG_PASSWORD_E;

    // Convert incoming hex password to binary
    uint8_t au8IncomingBinary[32];
    if (!CliAuth_bHexToBinary(pcPassword, au8IncomingBinary, 32))
        return CLI_AUTH_WRONG_PASSWORD_E; // Invalid hex format

    // Read stored password from EEPROM (stored as 32-byte binary hash)
    uint8_t au8StoredBinary[PINCFG_AUTH_PASSWORD_LEN_D];
    PERCFG_RESULT_T eResult = PersistentCfg_eReadPassword(au8StoredBinary);

    if (eResult != PERCFG_OK_E || isPasswordEmpty(au8StoredBinary))
    {
        // If no password stored or empty/corrupted, use default hash and save it
        memcpy(au8StoredBinary, _au8DefaultPasswordBinary, PINCFG_AUTH_PASSWORD_LEN_D);
        PersistentCfg_eWritePassword(au8StoredBinary);
    }

    // Constant-time comparison to prevent timing attacks
    uint8_t diff = 0;
    for (size_t i = 0; i < 32; i++)
    {
        diff |= au8StoredBinary[i] ^ au8IncomingBinary[i];
    }

    return (diff == 0) ? CLI_AUTH_OK_E : CLI_AUTH_WRONG_PASSWORD_E;
}

CLI_AUTH_RESULT_T CliAuth_eSetPassword(const char *pcNewPassword)
{
    if (pcNewPassword == NULL)
        return CLI_AUTH_NULLPTR_ERROR_E;

    // Check new password is valid hex string (64 chars for 32 bytes)
    size_t newLen = strlen(pcNewPassword);
    if (newLen != PINCFG_AUTH_HEX_PASSWORD_LEN_D)
        return CLI_AUTH_ERROR_E;

    // Convert hex string to binary
    uint8_t au8NewPasswordBinary[PINCFG_AUTH_PASSWORD_LEN_D];
    if (!CliAuth_bHexToBinary(pcNewPassword, au8NewPasswordBinary, PINCFG_AUTH_PASSWORD_LEN_D))
        return CLI_AUTH_ERROR_E; // Invalid hex format

    // Write new password binary hash to EEPROM
    PERCFG_RESULT_T eResult = PersistentCfg_eWritePassword(au8NewPasswordBinary);
    if (eResult != PERCFG_OK_E)
        return CLI_AUTH_EEPROM_ERROR_E;

    return CLI_AUTH_OK_E;
}

CLI_AUTH_RESULT_T CliAuth_eChangePassword(const char *pcCurrentPassword, const char *pcNewPassword)
{
    if (pcCurrentPassword == NULL || pcNewPassword == NULL)
        return CLI_AUTH_NULLPTR_ERROR_E;

    // Verify current password
    if (CliAuth_eVerifyPassword(pcCurrentPassword) != CLI_AUTH_OK_E)
        return CLI_AUTH_WRONG_PASSWORD_E;

    // Check new password is valid hex string (64 chars for 32 bytes)
    size_t newLen = strlen(pcNewPassword);
    if (newLen != 64)
        return CLI_AUTH_ERROR_E;

    // Convert hex string to binary
    uint8_t au8NewPasswordBinary[32];
    if (!CliAuth_bHexToBinary(pcNewPassword, au8NewPasswordBinary, 32))
        return CLI_AUTH_ERROR_E; // Invalid hex format

    // Write new password binary hash to EEPROM
    PERCFG_RESULT_T eResult = PersistentCfg_eWritePassword(au8NewPasswordBinary);
    if (eResult != PERCFG_OK_E)
        return CLI_AUTH_EEPROM_ERROR_E;

    return CLI_AUTH_OK_E;
}

CLI_AUTH_RESULT_T CliAuth_eVerifyPasswordRateLimited(
    const char *pcPassword,
    uint32_t u32CurrentMs,
    uint8_t *pu8FailedAttempts,
    uint32_t *pu32LockoutUntilMs)
{
    if (pcPassword == NULL || pu8FailedAttempts == NULL || pu32LockoutUntilMs == NULL)
        return CLI_AUTH_NULLPTR_ERROR_E;

    // Check if currently locked out
    if (*pu32LockoutUntilMs != 0)
    {
        // Handle timestamp wraparound: if lockout is in the "future" relative to current time
        // by more than half the uint32 range, assume wraparound occurred
        uint32_t u32TimeDiff = *pu32LockoutUntilMs - u32CurrentMs;
        if (u32TimeDiff <= CLI_AUTH_MAX_LOCKOUT_MS)
        {
            // Still locked out
            return CLI_AUTH_LOCKED_OUT_E;
        }
        // Lockout expired (or wraparound), clear it
        *pu32LockoutUntilMs = 0;
    }

    // Verify password
    CLI_AUTH_RESULT_T eResult = CliAuth_eVerifyPassword(pcPassword);

    if (eResult == CLI_AUTH_OK_E)
    {
        // Success - reset rate limiting
        *pu8FailedAttempts = 0;
        *pu32LockoutUntilMs = 0;
        return CLI_AUTH_OK_E;
    }

    // Failed - increment counter and calculate lockout
    if (*pu8FailedAttempts < 255)
        (*pu8FailedAttempts)++;

    // Exponential backoff: 1s, 2s, 4s, 8s, 16s, 32s, 64s, 128s, 256s, then cap at 5 min
    uint32_t u32LockoutMs = CLI_AUTH_BASE_LOCKOUT_MS << (*pu8FailedAttempts - 1);
    if (u32LockoutMs > CLI_AUTH_MAX_LOCKOUT_MS)
        u32LockoutMs = CLI_AUTH_MAX_LOCKOUT_MS;

    *pu32LockoutUntilMs = u32CurrentMs + u32LockoutMs;

    // Handle case where addition wraps to 0 (unlikely but possible)
    if (*pu32LockoutUntilMs == 0)
        *pu32LockoutUntilMs = 1;

    return CLI_AUTH_WRONG_PASSWORD_E;
}

uint32_t CliAuth_u32GetRemainingLockoutMs(uint32_t u32CurrentMs, uint32_t u32LockoutUntilMs)
{
    if (u32LockoutUntilMs == 0)
        return 0;

    uint32_t u32TimeDiff = u32LockoutUntilMs - u32CurrentMs;

    // If difference is larger than max lockout, lockout has expired
    if (u32TimeDiff > CLI_AUTH_MAX_LOCKOUT_MS)
        return 0;

    return u32TimeDiff;
}
