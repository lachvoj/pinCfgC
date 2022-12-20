#include <string.h>

#include "ExtCfgReceiver.h"
#include "Globals.h"
#include "Memory.h"
#include "PinCfgCsv.h"
#include "PincfgIf.h"

static void ExtCfgReceiver_vConfigurationReceived(EXTCFGRECEIVER_HANDLE_T *psHandle);
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
        if (psState == NULL)
            psHandle->acState[0] = '\0';
        else
            strcpy(psHandle->acState, psState);
    }
    break;
    default: break;
    }

    if (bSendState)
        bSendText(psHandle->sLooPre.u8Id, psHandle->acState);
}

// presentable IF
void ExtCfgReceiver_vRcvMessage(EXTCFGRECEIVER_HANDLE_T *psHandle, const char *pcMessage)
{
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
            ExtCfgReceiver_vSetState(psHandle, EXTCFGRECEIVER_RECEIVNG_E, NULL, true);
        }
    }
    else if (psHandle->eState == EXTCFGRECEIVER_RECEIVNG_E)
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
        else
        {
            strcpy(psGlobals->acCfgBuf + psHandle->u16CfgNext, pcMessage);
            psHandle->u16CfgNext += strlen(pcMessage);
        }
    }
}

void ExtCfgReceiver_vPresent(EXTCFGRECEIVER_HANDLE_T *psHandle)
{
    bPresentInfo(psHandle->sLooPre.u8Id, psHandle->sLooPre.pcName);
}

void ExtCfgReceiver_vPresentState(EXTCFGRECEIVER_HANDLE_T *psHandle)
{
    bSendText(psHandle->sLooPre.u8Id, psHandle->acState);
    bRequestText(psHandle->sLooPre.u8Id);
}

// private
static void ExtCfgReceiver_vConfigurationReceived(EXTCFGRECEIVER_HANDLE_T *psHandle)
{
    size_t szMemoryRequired;
    // size_t szFreeMemory = Memory_szGetFree() - 30U;
    PINCFG_RESULT_T eValidationResult = PinCfgCsv_eValidate(&szMemoryRequired, NULL, 0);
    if (eValidationResult == PINCFG_OK_E)
    {
        ExtCfgReceiver_vSetState(psHandle, EXTCFGRECEIVER_VALIDATION_OK_E, NULL, true);
        if (u8SaveCfg(psGlobals->acCfgBuf) == 0U)
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
        psHandle->eState = EXTCFGRECEIVER_VALIDATION_ERROR_E;
        ExtCfgReceiver_vSetState(psHandle, EXTCFGRECEIVER_VALIDATION_ERROR_E, NULL, true);
    }
    psHandle->eState = EXTCFGRECEIVER_LISTENING_E;
}
