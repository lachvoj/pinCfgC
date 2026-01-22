#include <stdio.h>
#include <string.h>

#include "Cli.h"
#include "CliAuth.h"
#include "Globals.h"
#include "Memory.h"
#include "MySensorsWrapper.h"
#include "PersistentConfiguration.h"
#include "PinCfgCsv.h"
#include "PinCfgUtils.h"
#ifdef FWCHECK_ENABLED
#include "FWCheck.h"
#endif

static const char *_apcStateStrings[] = {
    "READY",
    "OUT_OF_MEMORY_ERROR",
    "RECEIVING_AUTH",
    "RECEIVING_TYPE",
    "RECEIVING_CFG_DATA",
    "RECEIVING_CMD_DATA",
    "VALIDATION_OK",
#ifdef FWCHECK_ENABLED
    "FW_CRC_ERROR",
#endif
    "VALIDATION_ERROR"

};

static const char *_pcMsgBegin = "#[";
static const char *_pcMsgEND = "]#";
static const char *_pcCfgPrefix = "CFG:";
static const char *_pcCmdPrefix = "CMD:";

static const char *_pcLockedOutMsg = "Locked out. Try again later.";
static const char *_pcAuthFailedMsg = "Authentication failed.";
static const char *_pcAutnRequiredMsg = "Authentication required.";
static const char *_pcInvalidPwdLenMsg = "Invalid pwd length.";
static const char *_pcDataTooLargeMsg = "Data too large.";
static const char *_pcInvalidMsgType = "Invalid message type.";
static const char *_pcInvalidMsgFormat = "Invalid message format.";

static void Cli_vConfigurationReceived(CLI_T *psHandle);
static void Cli_vExecuteCommand(CLI_T *psHandle, char *pcCmd);
static void Cli_vSendBigMessage(CLI_T *psHandle, char *pcBigMsg);
static void Cli_vResetState(CLI_T *psHandle);

#if defined(ARDUINO) && !defined(TEST)
#if defined(ARDUINO_ARCH_STM32) || defined(ARDUINO_ARCH_STM32F1)
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

void Cli_vLoop(LOOPABLE_T *psLoopableHandle, uint32_t u32ms);

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
    psHandle->eState = CLI_READY_E;
    psHandle->sPresentable.pcState = _apcStateStrings[CLI_READY_E];
    psHandle->u16CfgNext = 0;

    // Initialize rate limiting
    psHandle->u8FailedAttempts = 0;
    psHandle->u32LockoutUntilMs = 0;

    // Loopable init (always needed for receive timeout)
    psHandle->sLoopable.vLoop = Cli_vLoop;
    psHandle->u32ReceivingStartedMs = 0;

#ifdef FWCHECK_ENABLED
    // Initialize firmware CRC check module and verify firmware integrity
    FWCheck_Init();
    if (FWCheck_Verify() != FWCHECK_OK)
    {
        // Firmware CRC mismatch - enter unrecoverable error state
        psHandle->eState = CLI_CRC_ERROR_E;
        psHandle->sPresentable.pcState = _apcStateStrings[CLI_CRC_ERROR_E];
    }
    psHandle->u32LastFwCrcCheckMs = 0;
#endif

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
    if (psHandle->pcCfgBuf != NULL)
    {
        Memory_vTempFreePt(psHandle->pcCfgBuf);
        psHandle->pcCfgBuf = NULL;
    }
    psHandle->eState = CLI_READY_E;
    psHandle->sPresentable.pcState = _apcStateStrings[CLI_READY_E];
    psHandle->u16CfgNext = 0;
}

// Helper: Shift buffer left by removing everything before position
static void Cli_vShiftBuffer(CLI_T *psHandle, size_t szStartPos)
{
    size_t szCurrentLen = strlen(psHandle->pcCfgBuf);
    if (szStartPos >= szCurrentLen)
    {
        psHandle->pcCfgBuf[0] = '\0';
        psHandle->u16CfgNext = 0;
        return;
    }

    size_t szNewLen = szCurrentLen - szStartPos;
    memmove(psHandle->pcCfgBuf, psHandle->pcCfgBuf + szStartPos, szNewLen + 1);
    psHandle->u16CfgNext = szNewLen;
}

// Helper: Append data to buffer, returns false if too large
static bool Cli_bAppendToBuffer(CLI_T *psHandle, const char *pcData)
{
    size_t szLen = strlen(pcData);
    if (psHandle->u16CfgNext + szLen >= PINCFG_CONFIG_MAX_SZ_D)
    {
        Cli_vSetState(psHandle, CLI_CUSTOM_E, _pcDataTooLargeMsg, true);
        Cli_vResetState(psHandle);
        return false;
    }
    memcpy(psHandle->pcCfgBuf + psHandle->u16CfgNext, pcData, szLen);
    psHandle->u16CfgNext += szLen;
    psHandle->pcCfgBuf[psHandle->u16CfgNext] = '\0';

    return true;
}

// presentable IF
void Cli_vRcvMessage(PRESENTABLE_T *psBaseHandle, const MyMessage *pcMsg)
{
    CLI_T *psHandle = (CLI_T *)psBaseHandle;
    const char *pcMessage = pcMsg->data;
    char *pcBeginPos = NULL;
    bool bAlreadyAppended = false;

#ifdef MY_CONTROLLER_HA
    psHandle->sPresentable.bStatePresented = true;
#endif

#ifdef FWCHECK_ENABLED
    // CRC_ERROR is unrecoverable - reject all messages
    if (psHandle->eState == CLI_CRC_ERROR_E)
    {
        Presentable_vSendState((PRESENTABLE_T *)psHandle);
        return;
    }
#endif

    // Handle terminal/error states: reset to READY if message contains begin marker
    // This covers: CLI_OUT_OF_MEM_ERR_E, CLI_VALIDATION_OK_E, CLI_VALIDATION_ERROR_E, CLI_CUSTOM_E
    if (psHandle->eState != CLI_READY_E && psHandle->eState != CLI_RECEIVING_AUTH_E &&
        psHandle->eState != CLI_RECEIVING_TYPE_E && psHandle->eState != CLI_RECEIVING_CFG_DATA_E &&
        psHandle->eState != CLI_RECEIVING_CMD_DATA_E)
    {
        // Search for begin marker in the message
        pcBeginPos = strchr(pcMessage, _pcMsgBegin[0]);
        if (pcBeginPos != NULL)
        {
            // Reset state and shift message to start from begin marker
            Cli_vResetState(psHandle);
        }
        else
        {
            // Ignore messages in terminal states that don't contain a new transaction start
            return;
        }
    }

    // STATE: READY - Detect message type and start auth
    if (psHandle->eState == CLI_READY_E)
    {
        pcBeginPos = strchr(pcMessage, _pcMsgBegin[0]);
        if (pcBeginPos == NULL)
            return;

        pcMessage = pcBeginPos;

        // Allocate buffer if needed
        if (psHandle->pcCfgBuf == NULL)
        {
            size_t szFreeMemory = Memory_szGetFree();
            if (szFreeMemory >= PINCFG_CONFIG_MAX_SZ_D)
            {
                psHandle->pcCfgBuf = (char *)Memory_vpTempAlloc(PINCFG_CONFIG_MAX_SZ_D);
                if (psHandle->pcCfgBuf == NULL)
                {
                    Cli_vSetState(psHandle, CLI_OUT_OF_MEM_ERR_E, NULL, true);
                    return;
                }
            }
        }

        if (!Cli_bAppendToBuffer(psHandle, pcMessage))
            return;
        bAlreadyAppended = true;

        if (strlen(psHandle->pcCfgBuf) < 2)
        {
            Presentable_vSendState((PRESENTABLE_T *)psHandle);
            return;
        }

        // Check for unified message format: #[<password>/<type>:...]#
        char *pcStartString = strstr(psHandle->pcCfgBuf, _pcMsgBegin);
        if (pcStartString == NULL || pcStartString != psHandle->pcCfgBuf)
        {
            Cli_vSetState(psHandle, CLI_CUSTOM_E, _pcInvalidMsgFormat, true);
            Cli_vResetState(psHandle);
            return;
        }

        // Starting string confirmed shift buffer and go to next state
        Cli_vShiftBuffer(psHandle, strlen(_pcMsgBegin));
        psHandle->u32ReceivingStartedMs = u32Millis(); // Start timeout tracking
        Cli_vSetState(psHandle, CLI_RECEIVING_AUTH_E, NULL, true);
    }

    // STATE: RECEIVING_AUTH - Continue receiving fragmented pwd
    if (psHandle->eState == CLI_RECEIVING_AUTH_E)
    {
        if (!bAlreadyAppended)
        {
            if (!Cli_bAppendToBuffer(psHandle, pcMessage))
                return;
            else
            {
                bAlreadyAppended = true;
                Presentable_vSendState((PRESENTABLE_T *)psHandle);
            }
        }

        char *pcPwdEnd = strchr(psHandle->pcCfgBuf, '/');
        size_t szFragmentLen = pcPwdEnd ? (size_t)(pcPwdEnd - psHandle->pcCfgBuf) : strlen(psHandle->pcCfgBuf);

        if (szFragmentLen > PINCFG_AUTH_HEX_PASSWORD_LEN_D)
        {
            Cli_vSetState(psHandle, CLI_CUSTOM_E, _pcInvalidPwdLenMsg, true);
            Cli_vResetState(psHandle);
            return;
        }

        if (pcPwdEnd == NULL)
            return; // Password fragment not complete yet

        // Password complete - verify and continue
        pcPwdEnd[0] = '\0'; // Terminate password string
        CLI_AUTH_RESULT_T eAuthResult = CliAuth_eVerifyPasswordRateLimited(
            psHandle->pcCfgBuf, u32Millis(), &psHandle->u8FailedAttempts, &psHandle->u32LockoutUntilMs);

        if (eAuthResult != CLI_AUTH_OK_E)
        {
            const char *pcFailMsg = (eAuthResult == CLI_AUTH_LOCKED_OUT_E) ? _pcLockedOutMsg : _pcAuthFailedMsg;
            Cli_vSetState(psHandle, CLI_CUSTOM_E, pcFailMsg, true);
            Cli_vResetState(psHandle);
            return;
        }

        pcPwdEnd[0] = '/'; // Restore '/' character
        Cli_vShiftBuffer(psHandle, (size_t)(pcPwdEnd + 1 - psHandle->pcCfgBuf));
        Cli_vSetState(psHandle, CLI_RECEIVING_TYPE_E, NULL, true);
        bAlreadyAppended = true;
        // Continue to RECEIVING_TYPE state processing
    }

    // STATE: RECEIVING_TYPE - Continue receiving fragmented CFG:/CMD: prefix
    if (psHandle->eState == CLI_RECEIVING_TYPE_E)
    {
        if (!bAlreadyAppended)
        {
            if (!Cli_bAppendToBuffer(psHandle, pcMessage))
                return;
            else
            {
                bAlreadyAppended = true;
                Presentable_vSendState((PRESENTABLE_T *)psHandle);
            }
        }

        if (psHandle->u16CfgNext < 4)
            return;

        // Determine message type
        CLI_STATE_T eNextState;
        if (strncmp(psHandle->pcCfgBuf, _pcCfgPrefix, 4) == 0)
            eNextState = CLI_RECEIVING_CFG_DATA_E;
        else if (strncmp(psHandle->pcCfgBuf, _pcCmdPrefix, 4) == 0)
            eNextState = CLI_RECEIVING_CMD_DATA_E;
        else
        {
            Cli_vSetState(psHandle, CLI_CUSTOM_E, _pcInvalidMsgType, true);
            Cli_vResetState(psHandle);
            return;
        }

        // Move data after prefix to start of buffer
        Cli_vShiftBuffer(psHandle, 4);
        Cli_vSetState(psHandle, eNextState, NULL, true);
        bAlreadyAppended = true;
        // Continue to data receiving state processing
    }

    // STATE: RECEIVING_CFG_DATA or RECEIVING_CMD_DATA - Continue receiving data
    if (psHandle->eState == CLI_RECEIVING_CFG_DATA_E || psHandle->eState == CLI_RECEIVING_CMD_DATA_E)
    {
        if (!bAlreadyAppended)
        {
            if (!Cli_bAppendToBuffer(psHandle, pcMessage))
                return;
            else
            {
                bAlreadyAppended = true;
                Presentable_vSendState((PRESENTABLE_T *)psHandle);
            }
        }

        char *pcEndString = strstr(psHandle->pcCfgBuf, _pcMsgEND);

        if (pcEndString == NULL)
            return;

        pcEndString[0] = '\0'; // Terminate data string

        if (psHandle->eState == CLI_RECEIVING_CFG_DATA_E)
        {
            Cli_vConfigurationReceived(psHandle);
        }
        else // CLI_RECEIVING_CMD_DATA_E
        {
            Cli_vExecuteCommand(psHandle, psHandle->pcCfgBuf);
        }
        Cli_vResetState(psHandle);

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
        vWait(PINCFG_LONG_MESSAGE_DELAY_MS_D);
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

        char *pcCfgBuf = (char *)Memory_vpTempAlloc(u16CfgSize + 1);
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
#ifdef MY_TRANSPORT_ERROR_LOG
    else if (strcmp(pcCmd, "GET_TSP_ERRORS") == 0)
    {
        uint8_t u8Count = u8TransportGetErrorLogCount();
        uint32_t u32Total = u32TransportGetTotalErrorCount();

        if (u8Count == 0)
        {
            char acMsg[32];
            snprintf(acMsg, sizeof(acMsg), "No errors. Total: %lu", (unsigned long)u32Total);
            Cli_vSetState(psHandle, CLI_CUSTOM_E, acMsg, true);
        }
        else
        {
            // Format: "T:<total>,C:<count>|<ts>,<code>,<ch>,<ex>|..."
            // Each entry ~20 chars, estimate buffer size
            size_t szBufSize = 32 + (u8Count * 24);
            char *pcBuf = (char *)Memory_vpTempAlloc(szBufSize);
            if (pcBuf == NULL)
            {
                Cli_vSetState(psHandle, CLI_OUT_OF_MEM_ERR_E, NULL, true);
                return;
            }

            int iPos = snprintf(pcBuf, szBufSize, "T:%lu,C:%u", (unsigned long)u32Total, u8Count);

            TransportErrorLogEntry_t sEntry;
            for (uint8_t u8i = 0; u8i < u8Count && iPos < (int)(szBufSize - 24); u8i++)
            {
                if (bTransportGetErrorLogEntry(u8i, &sEntry))
                {
                    iPos += snprintf(
                        pcBuf + iPos,
                        szBufSize - iPos,
                        "|%lu,%02X,%u,%u",
                        (unsigned long)sEntry.timestamp,
                        sEntry.errorCode,
                        sEntry.channel,
                        sEntry.extra);
                }
            }

            Cli_vSendBigMessage(psHandle, pcBuf);
            Memory_vTempFreePt(pcBuf);
            Cli_vSetState(psHandle, CLI_READY_E, NULL, true);
        }
    }
    else if (strcmp(pcCmd, "CLR_TSP_ERRORS") == 0)
    {
        vTransportClearErrorLog();
        Cli_vSetState(psHandle, CLI_CUSTOM_E, "Errors cleared.", true);
    }
#endif
#ifdef FWCHECK_ENABLED
    else if (strcmp(pcCmd, "FW_CHCK") == 0)
    {
        // Firmware CRC verification command
        FWCheck_Result_t eResult = FWCheck_Verify();

        if (eResult == FWCHECK_OK)
        {
            Cli_vSetState(psHandle, CLI_CUSTOM_E, "FW_CRC_OK", true);
        }
        else
        {
            // CRC mismatch or error - enter unrecoverable error state
            Cli_vSetState(psHandle, CLI_CRC_ERROR_E, NULL, true);
        }
    }
#ifdef FWCHECK_INCLUDE_FW_VERSION
    else if (strcmp(pcCmd, "GET_FW_VERSION") == 0)
    {
        const char *pcFwVersion = FWCheck_GetFwVersion();
        if (pcFwVersion != NULL)
        {
            Cli_vSetState(psHandle, CLI_CUSTOM_E, pcFwVersion, true);
        }
        else
        {
            Cli_vSetState(psHandle, CLI_CUSTOM_E, "FW version not available.", true);
        }
    }
#endif // FWCHECK_INCLUDE_FW_VERSION
#endif // FWCHECK_ENABLED
    else
    {
        Cli_vSetState(psHandle, CLI_CUSTOM_E, "Unknown command.", true);
    }
}

void Cli_vLoop(LOOPABLE_T *psLoopableHandle, uint32_t u32ms)
{
    CLI_T *psHandle = container_of(psLoopableHandle, CLI_T, sLoopable);

    // Check for fragmented message receive timeout
    if (psHandle->u32ReceivingStartedMs != 0)
    {
        bool bIsReceiving =
            (psHandle->eState == CLI_RECEIVING_AUTH_E || psHandle->eState == CLI_RECEIVING_TYPE_E ||
             psHandle->eState == CLI_RECEIVING_CFG_DATA_E || psHandle->eState == CLI_RECEIVING_CMD_DATA_E);

        if (bIsReceiving &&
            PinCfg_u32GetElapsedTime(psHandle->u32ReceivingStartedMs, u32ms) >= PINCFG_CLI_RECEIVE_TIMEOUT_MS_D)
        {
            // Timeout - reset state and free buffer
            Cli_vResetState(psHandle);
            psHandle->u32ReceivingStartedMs = 0;
        }
        else if (!bIsReceiving)
        {
            // Not in receiving state anymore, clear timeout tracking
            psHandle->u32ReceivingStartedMs = 0;
        }
    }

#ifdef FWCHECK_ENABLED
    // Periodic firmware CRC check (once per day)
    if (PinCfg_u32GetElapsedTime(psHandle->u32LastFwCrcCheckMs, u32ms) >= PINCFG_FW_CRC_CHECK_INTERVAL_MS_D)
    {
        psHandle->u32LastFwCrcCheckMs = u32ms;

        if (FWCheck_Verify() != FWCHECK_OK)
        {
            // Firmware CRC mismatch - enter unrecoverable error state
            Cli_vSetState(psHandle, CLI_CRC_ERROR_E, NULL, true);
        }
    }
#endif
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
