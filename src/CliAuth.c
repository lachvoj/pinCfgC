#include "CliAuth.h"
#include "PersistentConfigiration.h"
#include <string.h>

// No global variables - read password from EEPROM when needed

CLI_AUTH_RESULT_T CliAuth_eVerifyPassword(const char *pcPassword)
{
    if (pcPassword == NULL)
        return CLI_AUTH_NULLPTR_ERROR_E;
    
    // Read stored password from EEPROM (no caching)
    char acStoredPassword[PINCFG_AUTH_PASSWORD_MAX_LEN_D];
    PERCFG_RESULT_T eResult = PersistentCfg_eReadPassword(acStoredPassword);
    
    if (eResult != PERCFG_OK_E)
    {
        // If no password stored, compare against default
        strncpy(acStoredPassword, CLI_AUTH_DEFAULT_PASSWORD, PINCFG_AUTH_PASSWORD_MAX_LEN_D - 1);
        acStoredPassword[PINCFG_AUTH_PASSWORD_MAX_LEN_D - 1] = '\0';
    }
    
    // Constant-time comparison to prevent timing attacks
    size_t len1 = strlen(acStoredPassword);
    size_t len2 = strlen(pcPassword);
    
    // If lengths differ, still do comparison to prevent timing leak
    size_t maxLen = (len1 > len2) ? len1 : len2;
    
    uint8_t diff = (len1 != len2) ? 1 : 0;
    for (size_t i = 0; i < maxLen && i < PINCFG_AUTH_PASSWORD_MAX_LEN_D; i++)
    {
        char c1 = (i < len1) ? acStoredPassword[i] : 0;
        char c2 = (i < len2) ? pcPassword[i] : 0;
        diff |= c1 ^ c2;
    }
    
    return (diff == 0) ? CLI_AUTH_OK_E : CLI_AUTH_WRONG_PASSWORD_E;
}

CLI_AUTH_RESULT_T CliAuth_eChangePassword(const char *pcCurrentPassword, const char *pcNewPassword)
{
    if (pcCurrentPassword == NULL || pcNewPassword == NULL)
        return CLI_AUTH_NULLPTR_ERROR_E;
    
    // Verify current password
    if (CliAuth_eVerifyPassword(pcCurrentPassword) != CLI_AUTH_OK_E)
        return CLI_AUTH_WRONG_PASSWORD_E;
    
    // Check new password length
    size_t newLen = strlen(pcNewPassword);
    if (newLen == 0 || newLen >= PINCFG_AUTH_PASSWORD_MAX_LEN_D)
        return CLI_AUTH_ERROR_E;
    
    // Write new password using PersistentConfig function (recalculates all checksums)
    PERCFG_RESULT_T eResult = PersistentCfg_eWritePassword(pcNewPassword);
    if (eResult != PERCFG_OK_E)
        return CLI_AUTH_EEPROM_ERROR_E;
    
    return CLI_AUTH_OK_E;
}
