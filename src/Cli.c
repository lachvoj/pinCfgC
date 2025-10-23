#include <string.h>

#include "Cli.h"
#include "CliAuth.h"
#include "Globals.h"
#include "Memory.h"
#include "MySensorsWrapper.h"
#include "PersistentConfigiration.h"
#include "PinCfgCsv.h"

static const char *_apcStateStrings[] =
    {"LISTENING", "OUT_OF_MEMORY_ERROR", "RECEIVING", "RECEIVED", "VALIDATING", "VALIDATION_OK", "VALIDATION_ERROR"};

static const char *_pcCfgBegin = "#[";
static const char *_pcCfgEND = "]#";
static const char *_pcCmdBegin = "#C:";
static const char *_pcAuthPrefix = "PWD:";  // Password prefix for commands

static void Cli_vConfigurationReceived(CLI_T *psHandle);
static void Cli_vCommandFunction(CLI_T *psHandle, char *cmd);
static void Cli_vSendBigMessage(CLI_T *psHandle, char *pcBigMsg);
static void Cli_vSendMessage(CLI_T *psHandle, const char *pvMessage);

#if defined(ARDUINO) && !defined(TEST)
#ifdef ARDUINO_ARCH_STM32F1
void resetFunc(void)
{
    asm("b Reset_Handler"); 
}
#else
void (*resetFunc)(void) = 0; // declare reset fuction at address 0
#endif
#else
void resetFunc(void)
{
}
#endif

void Cli_vInitType(PRESENTABLE_VTAB_T *psVtab)
{
    psVtab->eVType = V_TEXT;
    psVtab->eSType = S_INFO;
    psVtab->vReceive = Cli_vRcvMessage;
}

CLI_RESULT_T Cli_eInit(CLI_T *psHandle, uint8_t u8Id)
{
    if (psHandle == NULL)
    {
        return CLI_NULLPTR_ERROR_E;
    }

    // vtab init
    psHandle->sPresentable.psVtab = &psGlobals->sCliPrVTab;

    // LOOPRE init
    psHandle->sPresentable.pcName = "CLI";
    psHandle->sPresentable.u8Id = u8Id;
#ifdef MY_CONTROLLER_HA
    psHandle->sPresentable.bStatePresented = false;
#endif

    // Initialize handle items
    psHandle->pcCfgBuf = NULL;
    psHandle->eMode = CLI_LINE_E;
    psHandle->eState = CLI_LISTENING_E;
    psHandle->pcState = _apcStateStrings[CLI_LISTENING_E];
    psHandle->bAuthenticated = false;

    return CLI_OK_E;
}

void Cli_vSetState(
    CLI_T *psHandle,
    CLI_STATE_T eState,
    const char *psState,
    bool bSendState)
{
    psHandle->eState = eState;
    if (eState == CLI_CUSTOM_E)
    {
        if (psState != NULL)
            psHandle->pcState = psState;
        else
        {
            psHandle->pcState = NULL;
            bSendState = false;
        }
    }
    else
        psHandle->pcState = _apcStateStrings[eState];

    if (bSendState)
        Cli_vSendMessage(psHandle, psHandle->pcState);
}

// presentable IF
void Cli_vRcvMessage(PRESENTABLE_T *psBaseHandle, const void *pvMessage)
{
    CLI_T *psHandle = (CLI_T *)psBaseHandle;
    const char *pcMessage = (const char *)pvMessage;
    bool bCopied = false;

    if (psHandle->pcCfgBuf == NULL)
    {
        size_t szFreeMemory = Memory_szGetFree();
        if (szFreeMemory >= PINCFG_CONFIG_MAX_SZ_D)
        {
            psHandle->eMode = CLI_FULL_E;
            psHandle->pcCfgBuf = (char *)Memory_vpTempAlloc(PINCFG_CONFIG_MAX_SZ_D);
            if (psHandle->pcCfgBuf == NULL)
            {
                Cli_vSetState(psHandle, CLI_OUT_OF_MEM_ERR_E, NULL, true);
                return;
            }
        }
        // TODO: Implement line mode
        // else
        // {
        //     if (szFreeMemory < PINCFG_CLI_MAX_LINE_SZ_D)
        //     {
        //         Cli_vSetState(psHandle, CLI_OUT_OF_MEM_ERR_E, NULL, true);
        //         return;
        //     }
        //     psHandle->eMode = CLI_LINE_E;
        //     psHandle->pcCfgBuf = (char *)Memory_vpTempAlloc(PINCFG_CLI_MAX_LINE_SZ_D);
        //     if (psHandle->pcCfgBuf == NULL)
        //     {
        //         Cli_vSetState(psHandle, CLI_OUT_OF_MEM_ERR_E, NULL, true);
        //         return;
        //     }
        // }
    }

#ifdef MY_CONTROLLER_HA
    psHandle->sPresentable.bStatePresented = true;
#endif
    // CLI_STATE_T eOldState = psHandle->eState;

    if (psHandle->eState == CLI_LISTENING_E)
    {
        char *pcStartString = strstr((char *)pcMessage, _pcCfgBegin);
        if (pcStartString != NULL)
        {
            // Check for authentication in format: #[PWD:password/config...]#
            char *pcAuthStart = strstr(pcStartString, _pcAuthPrefix);
            if (pcAuthStart == pcStartString + strlen(_pcCfgBegin))
            {
                // Extract password (terminated by '/')
                char *pcPwdStart = pcAuthStart + strlen(_pcAuthPrefix);
                char *pcPwdEnd = strchr(pcPwdStart, '/');
                
                if (pcPwdEnd != NULL && (pcPwdEnd - pcPwdStart) < PINCFG_AUTH_PASSWORD_MAX_LEN_D)
                {
                    // Parse password
                    char acPassword[PINCFG_AUTH_PASSWORD_MAX_LEN_D];
                    size_t pwdLen = pcPwdEnd - pcPwdStart;
                    memcpy(acPassword, pcPwdStart, pwdLen);
                    acPassword[pwdLen] = '\0';
                    
                    if (CliAuth_eVerifyPassword(acPassword) == CLI_AUTH_OK_E)
                    {
                        // Authentication successful
                        psHandle->bAuthenticated = true;
                        
                        // Copy config data (without password) to buffer
                        strcpy(psHandle->pcCfgBuf, pcPwdEnd + 1);
                        psHandle->u16CfgNext = strlen(pcPwdEnd + 1);
                        bCopied = true;
                        Cli_vSetState(psHandle, CLI_RECEIVING_E, NULL, true);
                    }
                    else
                    {
                        Cli_vSetState(psHandle, CLI_CUSTOM_E, "Authentication failed.", true);
                        Cli_vSetState(psHandle, CLI_LISTENING_E, NULL, true);
                        return;
                    }
                }
                else
                {
                    Cli_vSetState(psHandle, CLI_CUSTOM_E, "Invalid auth format. Use: #[PWD:password/config...]#", true);
                    Cli_vSetState(psHandle, CLI_LISTENING_E, NULL, true);
                    return;
                }
            }
            else
            {
                // No authentication provided
                Cli_vSetState(psHandle, CLI_CUSTOM_E, "Authentication required.", true);
                Cli_vSetState(psHandle, CLI_LISTENING_E, NULL, true);
                return;
            }
        }
        else
        {
            pcStartString = strstr((char *)pcMessage, _pcCmdBegin);
            if (pcStartString != NULL)
                Cli_vCommandFunction(psHandle, pcStartString + strlen(_pcCmdBegin));
        }
    }
    if (psHandle->eState == CLI_RECEIVING_E)
    {
        char *pcEndString = strstr((char *)pcMessage, _pcCfgEND);
        if (pcEndString != NULL)
        {
            size_t szLen = pcEndString - pcMessage;
            psHandle->eState = CLI_RECEIVED_E;
            memcpy(psHandle->pcCfgBuf + psHandle->u16CfgNext, pcMessage, szLen);
            bCopied = true;
            psHandle->pcCfgBuf[psHandle->u16CfgNext + szLen] = '\0';
            Cli_vConfigurationReceived(psHandle);
            Memory_vTempFreePt(psHandle->pcCfgBuf);
            psHandle->bAuthenticated = false;  // Reset authentication after use
        }
        if (!bCopied)
        {
            strcpy(psHandle->pcCfgBuf + psHandle->u16CfgNext, pcMessage);
            psHandle->u16CfgNext += strlen(pcMessage);
        }
    }
}

// private
static void Cli_vConfigurationReceived(CLI_T *psHandle)
{
    PINCFG_RESULT_T eValidationResult;
    size_t szMemoryRequired;
#ifdef USE_MALLOC
    const size_t szFreeMemory = PINCFG_CONFIG_MAX_SZ_D;
#else
    const size_t szFreeMemory =  Memory_szGetFree() - (10 * sizeof(char *)); // leave some space for other allocations
#endif // USE_MALLOC
    char *pcOut = (char *)Memory_vpTempAlloc(szFreeMemory);
    if (pcOut != NULL)
    {
        pcOut[0] = '\0';
        eValidationResult = PinCfgCsv_eValidate(psHandle->pcCfgBuf, &szMemoryRequired, pcOut, szFreeMemory);
    }
    else
    {
        eValidationResult = PinCfgCsv_eValidate(psHandle->pcCfgBuf, &szMemoryRequired, NULL, 0);
    }

    if (eValidationResult == PINCFG_OK_E)
    {
        Cli_vSetState(psHandle, CLI_VALIDATION_OK_E, NULL, true);
        // Save config without password (PersistentCfg will preserve existing password)
        if (PersistentCfg_eSaveConfig(psHandle->pcCfgBuf, strlen(psHandle->pcCfgBuf)) == PERCFG_OK_E)
            resetFunc();
        else
            Cli_vSetState(psHandle, CLI_CUSTOM_E, "Save of cfg unsucessfull.", true);
    }
    else
    {
        if (pcOut != NULL && strlen(pcOut) > 0)
        {
            Cli_vSendBigMessage(psHandle, pcOut);
        }

        Cli_vSetState(psHandle, CLI_VALIDATION_ERROR_E, NULL, true);
    }

    if (pcOut != NULL)
        Memory_vTempFreePt(pcOut);
    
    Cli_vSetState(psHandle, CLI_LISTENING_E, NULL, true);
}

static void Cli_vCommandFunction(CLI_T *psHandle, char *cmd)
{
    // Commands format: #C:PWD:password:COMMAND or #C:PWD:password:CHANGE_PWD:newpassword
    
    // Check if command starts with PWD:
    if (strncmp(cmd, _pcAuthPrefix, strlen(_pcAuthPrefix)) == 0)
    {
        char *pcPwdStart = cmd + strlen(_pcAuthPrefix);
        char *pcCmdStart = strchr(pcPwdStart, ':');
        
        if (pcCmdStart == NULL)
        {
            Cli_vSetState(psHandle, CLI_CUSTOM_E, "Invalid command format. Use: #C:PWD:password:COMMAND", true);
            Cli_vSetState(psHandle, CLI_LISTENING_E, NULL, true);
            return;
        }
        
        // Parse authentication password
        size_t pwdLen = pcCmdStart - pcPwdStart;
        if (pwdLen == 0 || pwdLen >= PINCFG_AUTH_PASSWORD_MAX_LEN_D)
        {
            Cli_vSetState(psHandle, CLI_CUSTOM_E, "Invalid password length.", true);
            Cli_vSetState(psHandle, CLI_LISTENING_E, NULL, true);
            return;
        }
        
        char acPassword[PINCFG_AUTH_PASSWORD_MAX_LEN_D];
        memcpy(acPassword, pcPwdStart, pwdLen);
        acPassword[pwdLen] = '\0';
        
        // Verify authentication
        if (CliAuth_eVerifyPassword(acPassword) != CLI_AUTH_OK_E)
        {
            Cli_vSetState(psHandle, CLI_CUSTOM_E, "Authentication failed.", true);
            Cli_vSetState(psHandle, CLI_LISTENING_E, NULL, true);
            return;
        }
        
        // Authentication successful, process command
        pcCmdStart++; // Skip ':'
        
        if (strncmp(pcCmdStart, "CHANGE_PWD:", 11) == 0)
        {
            // Change password: #C:PWD:currentpassword:CHANGE_PWD:newpassword
            char *pcNewPwdStart = pcCmdStart + 11;
            size_t newPwdLen = strlen(pcNewPwdStart);
            
            if (newPwdLen == 0 || newPwdLen >= PINCFG_AUTH_PASSWORD_MAX_LEN_D)
            {
                Cli_vSetState(psHandle, CLI_CUSTOM_E, "Invalid new password length.", true);
                Cli_vSetState(psHandle, CLI_LISTENING_E, NULL, true);
                return;
            }
            
            CLI_AUTH_RESULT_T eResult = CliAuth_eChangePassword(acPassword, pcNewPwdStart);
            
            if (eResult == CLI_AUTH_OK_E)
            {
                Cli_vSetState(psHandle, CLI_CUSTOM_E, "Password changed successfully.", true);
            }
            else
            {
                Cli_vSetState(psHandle, CLI_CUSTOM_E, "Password change failed.", true);
            }
            Cli_vSetState(psHandle, CLI_LISTENING_E, NULL, true);
            return;
        }
        else if (strcmp(pcCmdStart, "GET_CFG") == 0)
        {
            uint16_t u16CfgSize = 0;
            if (PersistentCfg_eGetConfigSize(&u16CfgSize) != PERCFG_OK_E)
            {
                Cli_vSetState(psHandle, CLI_CUSTOM_E, "Unable to read cfg size.", true);
                Cli_vSetState(psHandle, CLI_LISTENING_E, NULL, true);
                return;
            }

            if (u16CfgSize == 0)
            {
                Cli_vSetState(psHandle, CLI_CUSTOM_E, "No config data.", true);
                Cli_vSetState(psHandle, CLI_LISTENING_E, NULL, true);
                return;
            }
            
            char *pcCfgBuf = Memory_vpTempAlloc(u16CfgSize + 1);
            if (pcCfgBuf == NULL)
            {
                Cli_vSetState(psHandle, CLI_OUT_OF_MEM_ERR_E, NULL, true);
                Cli_vSetState(psHandle, CLI_LISTENING_E, NULL, true);
                return;
            }

            if (PersistentCfg_eLoadConfig(pcCfgBuf) != PERCFG_OK_E)
            {
                Cli_vSetState(psHandle, CLI_CUSTOM_E, "Unable to load cfg.", true);
                Memory_vTempFreePt(pcCfgBuf);
                Cli_vSetState(psHandle, CLI_LISTENING_E, NULL, true);
                return;
            }

            // Send config data (password already excluded by LoadConfigOnly)
            Cli_vSendBigMessage(psHandle, pcCfgBuf);
            Memory_vTempFreePt(pcCfgBuf);
            Cli_vSetState(psHandle, CLI_LISTENING_E, NULL, true);
        }
        else if (strcmp(pcCmdStart, "RESET") == 0)
        {
            resetFunc();
        }
        else
        {
            Cli_vSetState(psHandle, CLI_CUSTOM_E, "Unknown command.", true);
            Cli_vSetState(psHandle, CLI_LISTENING_E, NULL, true);
        }
    }
    else
    {
        // No authentication provided
        Cli_vSetState(psHandle, CLI_CUSTOM_E, "Authentication required for commands.", true);
        Cli_vSetState(psHandle, CLI_LISTENING_E, NULL, true);
    }
}

static void Cli_vSendBigMessage(CLI_T *psHandle, char *pcBigMsg)
{
    size_t szMsgSize = strlen(pcBigMsg);
    uint8_t u8MyMsgsCount = szMsgSize / PINCFG_TXTSTATE_MAX_SZ_D;
    if ((szMsgSize % PINCFG_TXTSTATE_MAX_SZ_D) > 0)
        u8MyMsgsCount++;

    for (uint8_t u8i = 0; u8i < u8MyMsgsCount; u8i++)
    {
        Cli_vSendMessage(psHandle, pcBigMsg + (u8i * PINCFG_TXTSTATE_MAX_SZ_D));
        vWait(PINCFG_LONG_MESSAGE_DELAY_MS_D);
    }
}

static void Cli_vSendMessage(CLI_T *psHandle, const char *pcMessage)
{
    PRESENTABLE_T *psBaseHandle = (PRESENTABLE_T *)psHandle;
    MyMessage msg;

    if (eMyMessageInit(&msg, psBaseHandle->u8Id, psBaseHandle->psVtab->eVType) != WRAP_OK_E)
        return;

    if (eMyMessageSetCStr(&msg, pcMessage) != WRAP_OK_E)
        return;

    eSend(&msg, false);
}
