#include <string.h>

#include "ExtCfgReceiver.h"
#include "Globals.h"
#include "Memory.h"
#include "PinCfgCsv.h"
#include "PincfgIf.h"

static void ExtCfgReceiver_vConfigurationReceived(EXTCFGRECEIVER_HANDLE_T *psHandle);
static void ExtCfgReceiver_vCommandFunction(EXTCFGRECEIVER_HANDLE_T *psHandle, char *cmd);
static void ExtCfgReceiver_vSendBigMessage(EXTCFGRECEIVER_HANDLE_T *psHandle, char *pcBigMsg, size_t szMsgSize);

#if defined(ARDUINO) && !defined(TEST)
void (*resetFunc)(void) = 0; // declare reset fuction at address 0
#else
void resetFunc(void)
{
}
#endif

EXTCFGRECEIVER_RESULT_T ExtCfgReceiver_eInit(EXTCFGRECEIVER_HANDLE_T *psHandle, uint8_t u8Id)
{
    if (psHandle == NULL)
    {
        return EXTCFGRECEIVER_NULLPTR_ERROR_E;
    }

    // vtab init
    psHandle->sLooPre.psVtab = &psGlobals->sExtCfgReceiverVTab;

    // LOOPRE init
    psHandle->sLooPre.pcName = "cfgReceviver";
    psHandle->sLooPre.u8Id = u8Id;
#ifdef MY_CONTROLLER_HA
    psHandle->sLooPre.bStatePresented = false;
#endif

    // Initialize handle items
    psHandle->eState = EXTCFGRECEIVER_LISTENING_E;
    strcpy(psHandle->acState, "LISTENING");

    return EXTCFGRECEIVER_OK_E;
}
void ExtCfgReceiver_vSetState(
    EXTCFGRECEIVER_HANDLE_T *psHandle,
    EXTCFGRECEIVER_STATE_T eState,
    const char *psState,
    bool bSendState)
{
    psHandle->eState = eState;
    switch (psHandle->eState)
    {
    case EXTCFGRECEIVER_LISTENING_E: strcpy(psHandle->acState, "LISTENING"); break;
    case EXTCFGRECEIVER_RECEIVNG_E: strcpy(psHandle->acState, "RECEIVING"); break;
    case EXTCFGRECEIVER_RECEIVED_E: strcpy(psHandle->acState, "RECEIVED"); break;
    case EXTCFGRECEIVER_VALIDATING_E: strcpy(psHandle->acState, "VALIDATING"); break;
    case EXTCFGRECEIVER_VALIDATION_OK_E: strcpy(psHandle->acState, "VALIDATION OK"); break;
    case EXTCFGRECEIVER_VALIDATION_ERROR_E: strcpy(psHandle->acState, "VALIDATION ERROR"); break;
    case EXTCFGRECEIVER_CUSTOM_E:
    {
        if (psState != NULL)
            strcpy(psHandle->acState, psState);
    }
    break;
    default: break;
    }

    if (bSendState)
        psGlobals->sPincfgIf.bSend(psHandle->sLooPre.u8Id, V_TEXT, psHandle->acState);
}

// presentable IF
void ExtCfgReceiver_vRcvMessage(LOOPRE_T *psBaseHandle, const void *pvMessage)
{
    EXTCFGRECEIVER_HANDLE_T *psHandle = (EXTCFGRECEIVER_HANDLE_T *)psBaseHandle;
    const char *pcMessage = (const char *)pvMessage;
    bool bCopied = false;

#ifdef MY_CONTROLLER_HA
    psHandle->sLooPre.bStatePresented = true;
#endif
    // EXTCFGRECEIVER_STATE_T eOldState = psHandle->eState;

    if (psHandle->eState == EXTCFGRECEIVER_LISTENING_E)
    {
        char *pcStartString = strstr(pcMessage, "#[#");
        if (pcStartString != NULL)
        {
            strcpy(psGlobals->acCfgBuf, pcStartString + strlen("#[#"));
            psHandle->u16CfgNext = strlen(pcMessage) - strlen("#[#");
            bCopied = true;
            ExtCfgReceiver_vSetState(psHandle, EXTCFGRECEIVER_RECEIVNG_E, NULL, true);
        }
        else
        {
            pcStartString = strstr(pcMessage, "#C:");
            if (pcStartString != NULL)
                ExtCfgReceiver_vCommandFunction(psHandle, pcStartString + strlen("#C:"));
        }
    }
    if (psHandle->eState == EXTCFGRECEIVER_RECEIVNG_E)
    {
        char *pcEndString = strstr(pcMessage, "#]#");
        if (pcEndString != NULL)
        {
            size_t szLen = pcEndString - pcMessage;
            psHandle->eState = EXTCFGRECEIVER_RECEIVED_E;
            memcpy(psGlobals->acCfgBuf + psHandle->u16CfgNext, pcMessage, szLen);
            psGlobals->acCfgBuf[psHandle->u16CfgNext + szLen] = '\0';
            ExtCfgReceiver_vConfigurationReceived(psHandle);
        }
        else if (!bCopied)
        {
            strcpy(psGlobals->acCfgBuf + psHandle->u16CfgNext, pcMessage);
            psHandle->u16CfgNext += strlen(pcMessage);
        }
    }
}

// private
static void ExtCfgReceiver_vConfigurationReceived(EXTCFGRECEIVER_HANDLE_T *psHandle)
{
    PINCFG_RESULT_T eValidationResult;
    size_t szMemoryRequired;
    const size_t szFreeMemory = Memory_szGetFree() - (10 * sizeof(char *)); // leave some space for
    char *pcOut = (char *)Memory_vpTempAlloc(szFreeMemory);
    if (pcOut != NULL)
    {
        pcOut[0] = '\0';
        eValidationResult = PinCfgCsv_eValidate(&szMemoryRequired, pcOut, szFreeMemory);
    }
    else
    {
        eValidationResult = PinCfgCsv_eValidate(&szMemoryRequired, NULL, 0);
    }

    if (eValidationResult == PINCFG_OK_E)
    {
        ExtCfgReceiver_vSetState(psHandle, EXTCFGRECEIVER_VALIDATION_OK_E, NULL, true);
        if (psGlobals->sPincfgIf.u8SaveCfg(psGlobals->acCfgBuf) == 0U)
        {
#ifdef ARDUINO_ARCH_STM32F1
            asm("b Reset_Handler");
#else
            resetFunc();
#endif
        }
    }
    else
    {
        size_t szOutSize = strlen(pcOut);
        if (szOutSize > 0)
        {
            ExtCfgReceiver_vSendBigMessage(psHandle, pcOut, strlen(pcOut));
        }

        ExtCfgReceiver_vSetState(psHandle, EXTCFGRECEIVER_VALIDATION_ERROR_E, NULL, true);
    }

    if (pcOut != NULL)
        Memory_vTempFreeSize(szFreeMemory);
    psHandle->eState = EXTCFGRECEIVER_LISTENING_E;
}

static void ExtCfgReceiver_vCommandFunction(EXTCFGRECEIVER_HANDLE_T *psHandle, char *cmd)
{
    if (strcmp("GET_CFG", cmd) == 0)
    {
        ExtCfgReceiver_vSendBigMessage(psHandle, psGlobals->acCfgBuf, strlen(psGlobals->acCfgBuf));
        ExtCfgReceiver_vSetState(psHandle, EXTCFGRECEIVER_LISTENING_E, NULL, true);
    }
}

static void ExtCfgReceiver_vSendBigMessage(EXTCFGRECEIVER_HANDLE_T *psHandle, char *pcBigMsg, size_t szMsgSize)
{
    uint8_t u8MyMsgsCount = szMsgSize / PINCFG_TXTSTATE_MAX_SZ_D;
    if ((szMsgSize % PINCFG_TXTSTATE_MAX_SZ_D) > 0)
        u8MyMsgsCount++;

    for (uint8_t u8i = 0; u8i < u8MyMsgsCount; u8i++)
    {
        psGlobals->sPincfgIf.bSend(psHandle->sLooPre.u8Id, V_TEXT, pcBigMsg + (u8i * PINCFG_TXTSTATE_MAX_SZ_D));
        psGlobals->sPincfgIf.vWait(PINCFG_LONG_MESSAGE_DELAY_MS_D);
    }
}