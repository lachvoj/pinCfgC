#include <string.h>

#include "Cli.h"
#include "CliAuth.h"
#include "Globals.h"
#include "Memory.h"
#include "MySensorsWrapper.h"
#include "PersistentConfiguration.h"
#include "PinCfgCsv.h"

static const char *_apcStateStrings[] = {
    "READY",
    "OUT_OF_MEMORY_ERROR",
    "RECEIVING_AUTH",
    "RECEIVING_DATA",
    "RECEIVED",
    "VALIDATING",
    "VALIDATION_OK",
    "VALIDATION_ERROR"};

static const char *_pcMsgBegin = "#[";
static const char *_pcMsgEND = "]#";
static const char *_pcCfgPrefix = "CFG:";
static const char *_pcCmdPrefix = "CMD:";

static const char *_pcLockedOutMsg = "Locked out. Try again later.";
static const char *_pcAuthFailedMsg = "Authentication failed.";
static const char *_pcAutnRequiredMsg = "Authentication required.";
static const char *_pcInvalidPwdLenMsg = "Invalid pwd length.";
static const char *_pcDataTooLargeMsg = "Data too large.";
static const char *_pcInvalidMsgTypeMsg = "Invalid message type.";

static void Cli_vConfigurationReceived(CLI_T *psHandle);
static void Cli_vExecuteCommand(CLI_T *psHandle, char *pcCmd);
static void Cli_vSendBigMessage(CLI_T *psHandle, char *pcBigMsg);
static void Cli_vResetState(CLI_T *psHandle);
static bool Cli_bHandleAuthResult(CLI_T *psHandle, CLI_AUTH_RESULT_T eResult);
static void Cli_vProcessAfterAuth(
    CLI_T *psHandle,
    PRESENTABLE_T *psBaseHandle,
    char *pcDataStart,
    const char *pcMessage);
static void Cli_vFinalizeData(CLI_T *psHandle);

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
    psVtab->vPresent = Presentable_vPresent;
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
    psHandle->sPresentable.ePayloadType = P_STRING;
    psHandle->sPresentable.pcName = "CLI";
    psHandle->sPresentable.u8Id = u8Id;
#ifdef MY_CONTROLLER_HA
    psHandle->sPresentable.bStatePresented = false;
#endif

    // Initialize handle items
    psHandle->pcCfgBuf = NULL;
    psHandle->eMode = CLI_LINE_E;
    psHandle->eState = CLI_READY_E;
    psHandle->sPresentable.pcState = _apcStateStrings[CLI_READY_E];
    psHandle->bIsConfig = false;
    psHandle->bAuthenticated = false;

    // Initialize rate limiting
    psHandle->u8FailedAttempts = 0;
    psHandle->u32LockoutUntilMs = 0;

    return CLI_OK_E;
}

void Cli_vSetState(CLI_T *psHandle, CLI_STATE_T eState, const char *psState, bool bSendState)
{
    psHandle->eState = eState;
    if (eState == CLI_CUSTOM_E)
    {
        if (psState != NULL)
            psHandle->sPresentable.pcState = psState;
        else
        {
            psHandle->sPresentable.pcState = NULL;
            bSendState = false;
        }
    }
    else
        psHandle->sPresentable.pcState = _apcStateStrings[eState];

    if (bSendState)
        Presentable_vSendState((PRESENTABLE_T *)psHandle);
}

// Helper: Reset CLI state and optionally free buffer
static void Cli_vResetState(CLI_T *psHandle)
{
    psHandle->eState = CLI_READY_E;
    psHandle->u16CfgNext = 0;
    psHandle->bIsConfig = false;
    psHandle->bAuthenticated = false;
}

// Helper: Handle auth result, returns true if auth succeeded
static bool Cli_bHandleAuthResult(CLI_T *psHandle, CLI_AUTH_RESULT_T eResult)
{
    if (eResult == CLI_AUTH_OK_E)
    {
        psHandle->bAuthenticated = true;
        return true;
    }
    else if (eResult == CLI_AUTH_LOCKED_OUT_E)
    {
        Cli_vSetState(psHandle, CLI_CUSTOM_E, _pcLockedOutMsg, true);
    }
    else
    {
        Cli_vSetState(psHandle, CLI_CUSTOM_E, _pcAuthFailedMsg, true);
    }
    Cli_vResetState(psHandle);
    return false;
}

// Helper: Finalize received data (call appropriate handler based on bIsConfig)
static void Cli_vFinalizeData(CLI_T *psHandle)
{
    if (psHandle->bIsConfig)
    {
        psHandle->eState = CLI_RECEIVED_E;
        Cli_vConfigurationReceived(psHandle);
        Memory_vTempFreePt(psHandle->pcCfgBuf);
    }
    else
    {
        Cli_vExecuteCommand(psHandle, psHandle->pcCfgBuf);
    }
    Cli_vResetState(psHandle);
}

// Helper: Process message type (CFG:/CMD:) after successful authentication
// pcDataStart points to position after '/' (where CFG: or CMD: should be)
static void Cli_vProcessAfterAuth(
    CLI_T *psHandle,
    PRESENTABLE_T *psBaseHandle,
    char *pcDataStart,
    const char *pcMessage)
{
    // Check for CFG: or CMD: prefix
    if (strncmp(pcDataStart, _pcCfgPrefix, strlen(_pcCfgPrefix)) == 0)
    {
        psHandle->bIsConfig = true;
        pcDataStart += strlen(_pcCfgPrefix);
    }
    else if (strncmp(pcDataStart, _pcCmdPrefix, strlen(_pcCmdPrefix)) == 0)
    {
        psHandle->bIsConfig = false;
        pcDataStart += strlen(_pcCmdPrefix);
    }
    else
    {
        Cli_vSetState(psHandle, CLI_CUSTOM_E, _pcInvalidMsgTypeMsg, true);
        Cli_vResetState(psHandle);
        return;
    }

    // Check if complete message in same fragment
    char *pcMsgEnd = strstr(pcDataStart, _pcMsgEND);
    if (pcMsgEnd != NULL)
    {
        // Complete data in same fragment
        size_t szLen = pcMsgEnd - pcDataStart;
        memcpy(psHandle->pcCfgBuf, pcDataStart, szLen);
        psHandle->pcCfgBuf[szLen] = '\0';
        Cli_vFinalizeData(psHandle);
    }
    else
    {
        // Data continues in more fragments
        size_t szCopy = strlen(pcDataStart);
        if (szCopy >= PINCFG_CONFIG_MAX_SZ_D)
        {
            Cli_vSetState(psHandle, CLI_CUSTOM_E, _pcDataTooLargeMsg, true);
            Cli_vResetState(psHandle);
            return;
        }
        memcpy(psHandle->pcCfgBuf, pcDataStart, szCopy);
        psHandle->pcCfgBuf[szCopy] = '\0';
        psHandle->u16CfgNext = szCopy;
        Cli_vSetState(psHandle, CLI_RECEIVING_DATA_E, NULL, false);
        psBaseHandle->pcState = pcMessage;
        Presentable_vSendState(psBaseHandle);
    }
}

// presentable IF
void Cli_vRcvMessage(PRESENTABLE_T *psBaseHandle, const MyMessage *pcMsg)
{
    CLI_T *psHandle = (CLI_T *)psBaseHandle;
    const char *pcMessage = pcMsg->data;

    // Allocate buffer if needed
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
    }

#ifdef MY_CONTROLLER_HA
    psHandle->sPresentable.bStatePresented = true;
#endif

    // STATE: READY - Detect message type and start auth
    if (psHandle->eState == CLI_READY_E)
    {
        // Check for unified message format: #[<password>/<type>:...]#
        char *pcStartString = strstr((char *)pcMessage, _pcMsgBegin);
        if (pcStartString == NULL)
        {
            return;
        }

        // Password starts right after #[
        char *pcPwdStart = pcStartString + strlen(_pcMsgBegin);
        char *pcPwdEnd = strchr(pcPwdStart, '/');

        if (pcPwdEnd != NULL)
        {
            // Full pwd in first fragment
            size_t pwdLen = pcPwdEnd - pcPwdStart;
            if (pwdLen == 0 || pwdLen > PINCFG_AUTH_HEX_PASSWORD_LEN_D)
            {
                Cli_vSetState(psHandle, CLI_CUSTOM_E, _pcInvalidPwdLenMsg, true);
                Cli_vResetState(psHandle);
                return;
            }

            // Copy and verify pwd
            char acPassword[PINCFG_AUTH_HEX_PASSWORD_LEN_D + 1];
            memcpy(acPassword, pcPwdStart, pwdLen);
            acPassword[pwdLen] = '\0';

            CLI_AUTH_RESULT_T eAuthResult = CliAuth_eVerifyPasswordRateLimited(
                acPassword, u32Millis(), &psHandle->u8FailedAttempts, &psHandle->u32LockoutUntilMs);

            if (!Cli_bHandleAuthResult(psHandle, eAuthResult))
                return;

            // Auth successful - process message type
            Cli_vProcessAfterAuth(psHandle, psBaseHandle, pcPwdEnd + 1, pcMessage);
        }
        else
        {
            // Password continues in more fragments
            size_t szPartialLen = strlen(pcPwdStart);
            if (szPartialLen > PINCFG_AUTH_HEX_PASSWORD_LEN_D)
            {
                Cli_vSetState(psHandle, CLI_CUSTOM_E, _pcInvalidPwdLenMsg, true);
                Cli_vResetState(psHandle);
                return;
            }
            memcpy(psHandle->pcCfgBuf, pcPwdStart, szPartialLen);
            psHandle->u16CfgNext = szPartialLen;
            Cli_vSetState(psHandle, CLI_RECEIVING_AUTH_E, NULL, false);
            psBaseHandle->pcState = pcMessage;
            Presentable_vSendState(psBaseHandle);
        }
        return;
    }

    // STATE: RECEIVING_AUTH - Continue receiving fragmented pwd
    if (psHandle->eState == CLI_RECEIVING_AUTH_E)
    {
        char *pcPwdEnd = strchr(pcMessage, '/');

        if (pcPwdEnd != NULL)
        {
            // Found pwd terminator
            size_t szFragmentLen = pcPwdEnd - pcMessage;
            size_t szTotalPwdLen = psHandle->u16CfgNext + szFragmentLen;

            if (szTotalPwdLen > PINCFG_AUTH_HEX_PASSWORD_LEN_D)
            {
                Cli_vSetState(psHandle, CLI_CUSTOM_E, _pcInvalidPwdLenMsg, true);
                Cli_vResetState(psHandle);
                return;
            }

            // Complete pwd in buffer
            memcpy(psHandle->pcCfgBuf + psHandle->u16CfgNext, pcMessage, szFragmentLen);
            psHandle->pcCfgBuf[szTotalPwdLen] = '\0';

            CLI_AUTH_RESULT_T eAuthResult = CliAuth_eVerifyPasswordRateLimited(
                psHandle->pcCfgBuf, u32Millis(), &psHandle->u8FailedAttempts, &psHandle->u32LockoutUntilMs);

            if (!Cli_bHandleAuthResult(psHandle, eAuthResult))
                return;

            // Auth successful - process message type
            Cli_vProcessAfterAuth(psHandle, psBaseHandle, pcPwdEnd + 1, pcMessage);
        }
        else
        {
            // Password continues
            size_t szFragmentLen = strlen(pcMessage);
            size_t szNewTotal = psHandle->u16CfgNext + szFragmentLen;

            if (szNewTotal > PINCFG_AUTH_HEX_PASSWORD_LEN_D)
            {
                Cli_vSetState(psHandle, CLI_CUSTOM_E, _pcInvalidPwdLenMsg, true);
                Cli_vResetState(psHandle);
                return;
            }

            memcpy(psHandle->pcCfgBuf + psHandle->u16CfgNext, pcMessage, szFragmentLen);
            psHandle->u16CfgNext = szNewTotal;
            psBaseHandle->pcState = pcMessage;
            Presentable_vSendState(psBaseHandle);
        }
        return;
    }

    // STATE: RECEIVING_DATA - Continue receiving config or command data
    if (psHandle->eState == CLI_RECEIVING_DATA_E)
    {
        char *pcEndString = strstr((char *)pcMessage, _pcMsgEND);
        if (pcEndString != NULL)
        {
            // End marker found - finalize
            size_t szLen = pcEndString - pcMessage;
            if (psHandle->u16CfgNext + szLen >= PINCFG_CONFIG_MAX_SZ_D)
            {
                Cli_vSetState(psHandle, CLI_CUSTOM_E, _pcDataTooLargeMsg, true);
                Cli_vResetState(psHandle);
                return;
            }
            memcpy(psHandle->pcCfgBuf + psHandle->u16CfgNext, pcMessage, szLen);
            psHandle->pcCfgBuf[psHandle->u16CfgNext + szLen] = '\0';
            Cli_vFinalizeData(psHandle);
        }
        else
        {
            // Continue accumulating
            size_t szCopy = strlen(pcMessage);
            if (psHandle->u16CfgNext + szCopy >= PINCFG_CONFIG_MAX_SZ_D)
            {
                Cli_vSetState(psHandle, CLI_CUSTOM_E, _pcDataTooLargeMsg, true);
                Cli_vResetState(psHandle);
                return;
            }
            memcpy(psHandle->pcCfgBuf + psHandle->u16CfgNext, pcMessage, szCopy);
            psHandle->u16CfgNext += szCopy;
            psHandle->pcCfgBuf[psHandle->u16CfgNext] = '\0';
            psBaseHandle->pcState = pcMessage;
            Presentable_vSendState(psBaseHandle);
        }
        return;
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
    const size_t szFreeMemory = Memory_szGetFree() - (10 * sizeof(char *)); // leave some space for other allocations
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
        // Save config (PersistentCfg will preserve existing pwd)
        if (PersistentCfg_eSaveConfig(psHandle->pcCfgBuf) == PERCFG_OK_E)
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

    Cli_vSetState(psHandle, CLI_READY_E, NULL, true);
}

// Execute command after authentication (pcCmd is command string without pwd)
static void Cli_vExecuteCommand(CLI_T *psHandle, char *pcCmd)
{
    if (strncmp(pcCmd, "CHANGE_PWD:", 11) == 0)
    {
        // Change pwd: CHANGE_PWD:newpassword
        char *pcNewPwdStart = pcCmd + 11;
        size_t newPwdLen = strlen(pcNewPwdStart);

        if (newPwdLen == 0 || newPwdLen > PINCFG_AUTH_HEX_PASSWORD_LEN_D)
        {
            Cli_vSetState(psHandle, CLI_CUSTOM_E, "Invalid new pwd length.", true);
            return;
        }

        // For pwd change, we need the current pwd which is already verified
        // Use the verified pwd from buffer (stored during auth)
        CLI_AUTH_RESULT_T eResult = CliAuth_eSetPassword(pcNewPwdStart);

        if (eResult == CLI_AUTH_OK_E)
        {
            Cli_vSetState(psHandle, CLI_CUSTOM_E, "Pwd changed OK.", true);
        }
        else
        {
            Cli_vSetState(psHandle, CLI_CUSTOM_E, "Pwd change failed.", true);
        }
    }
    else if (strcmp(pcCmd, "GET_CFG") == 0)
    {
        uint16_t u16CfgSize = 0;
        if (PersistentCfg_eGetConfigSize(&u16CfgSize) != PERCFG_OK_E)
        {
            Cli_vSetState(psHandle, CLI_CUSTOM_E, "Unable to read cfg size.", true);
            return;
        }

        if (u16CfgSize == 0)
        {
            Cli_vSetState(psHandle, CLI_CUSTOM_E, "No config data.", true);
            return;
        }

        char *pcCfgBuf = Memory_vpTempAlloc(u16CfgSize + 1);
        if (pcCfgBuf == NULL)
        {
            Cli_vSetState(psHandle, CLI_OUT_OF_MEM_ERR_E, NULL, true);
            return;
        }

        if (PersistentCfg_eLoadConfig(pcCfgBuf) != PERCFG_OK_E)
        {
            Cli_vSetState(psHandle, CLI_CUSTOM_E, "Unable to load cfg.", true);
            Memory_vTempFreePt(pcCfgBuf);
            return;
        }

        Cli_vSendBigMessage(psHandle, pcCfgBuf);
        Memory_vTempFreePt(pcCfgBuf);
        Cli_vSetState(psHandle, CLI_READY_E, NULL, true);
    }
    else if (strcmp(pcCmd, "RESET") == 0)
    {
        resetFunc();
    }
    else
    {
        Cli_vSetState(psHandle, CLI_CUSTOM_E, "Unknown command.", true);
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
        psHandle->sPresentable.pcState = pcBigMsg + (u8i * PINCFG_TXTSTATE_MAX_SZ_D);
        Presentable_vSendState((PRESENTABLE_T *)psHandle);
        vWait(PINCFG_LONG_MESSAGE_DELAY_MS_D);
    }
}
