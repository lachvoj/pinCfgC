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

EXTCFGRECEIVER_RESULT_T ExtCfgReceiver_eInit(EXTCFGRECEIVER_HANDLE_T *psHandle, uint8_t u8Id, bool bPresent)
{
    if (psHandle == NULL)
    {
        return EXTCFGRECEIVER_NULLPTR_ERROR_E;
    }

    // LOOPRE init
    psHandle->sLooPreIf.ePinCfgType = PINCFG_EXTCFGRECEIVER_E;

    // Initialize handle items
    psHandle->pcName = "cfgReceviver";
    psHandle->u8Id = u8Id;
    psHandle->eState = EXTCFGRECEIVER_OFF_E;

    return EXTCFGRECEIVER_OK_E;
}

void ExtCfgReceiver_vSetState(EXTCFGRECEIVER_HANDLE_T *psHandle, const char *psState)
{
    switch (psHandle->eState)
    {
    case EXTCFGRECEIVER_OFF_E: strcpy(psHandle->acState, "OFF"); break;
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
    bSend(PINCFG_EXTCFGRECEIVER_E, psHandle->u8Id, (void *)psHandle->acState);
}

// presentable IF
uint8_t ExtCfgReceiver_u8GetId(EXTCFGRECEIVER_HANDLE_T *psHandle)
{
    return psHandle->u8Id;
}

const char *ExtCfgReceiver_pcGetName(EXTCFGRECEIVER_HANDLE_T *psHandle)
{
    return psHandle->pcName;
}

void ExtCfgReceiver_vRcvMessage(EXTCFGRECEIVER_HANDLE_T *psHandle, const void *pvMessage)
{
    const char *pcMessage = (const char *)pvMessage;
    // EXTCFGRECEIVER_STATE_T eOldState = psHandle->eState;

    if (psHandle->eState == EXTCFGRECEIVER_OFF_E)
    {
        char *pcStartString = strstr(pcMessage, PINCFG_CONF_START_STR);
        if (pcStartString != NULL)
        {
            strcpy(psGlobals->acCfgBuf, pcStartString + strlen(PINCFG_CONF_START_STR));
            psHandle->u16CfgNext = strlen(pcMessage) - strlen(PINCFG_CONF_START_STR);
            psHandle->eState = EXTCFGRECEIVER_RECEIVNG_E;
            ExtCfgReceiver_vSetState(psHandle, NULL);
        }
    }
    else if (psHandle->eState == EXTCFGRECEIVER_RECEIVNG_E)
    {
        char *pcEndString = strstr(pcMessage, PINCFG_CONF_END_STR);
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
    bPresent(PINCFG_EXTCFGRECEIVER_E, psHandle->u8Id, psHandle->pcName);
}

void ExtCfgReceiver_vPresentState(EXTCFGRECEIVER_HANDLE_T *psHandle)
{
    bSend(PINCFG_EXTCFGRECEIVER_E, psHandle->u8Id, (void *)psHandle->acState);
    bRequest(PINCFG_EXTCFGRECEIVER_E, psHandle->u8Id);
}

// private
static void ExtCfgReceiver_vConfigurationReceived(EXTCFGRECEIVER_HANDLE_T *psHandle)
{
    // TODO:
    // validate cfg
    // apply cfg
    // save cfg
    size_t szMemoryRequired;
    PINCFG_RESULT_T eValidationResult = PinCfgCsv_eValidate(&szMemoryRequired, NULL, 0);
    if (eValidationResult == PINCFG_OK_E)
    {
        psHandle->eState = EXTCFGRECEIVER_VALIDATION_OK_E;
        ExtCfgReceiver_vSetState(psHandle, NULL);
        if (u8SaveCfg(psGlobals->acCfgBuf) == 0U)
        {
            resetFunc();
        }
    }
    else
    {
        psHandle->eState = EXTCFGRECEIVER_VALIDATION_ERROR_E;
        ExtCfgReceiver_vSetState(psHandle, NULL);
    }
    psHandle->eState = EXTCFGRECEIVER_OFF_E;
}
