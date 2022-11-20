#include <string.h>

#include "ExtCfgReceiver.h"
#include "Globals.h"
#include "Memory.h"

static void ExtCfgReceiver_vConfigurationReceived(EXTCFGRECEIVER_HANDLE_T *psHandle);

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
    case 0: strcpy(psHandle->acState, "OFF"); break;
    case 1: strcpy(psHandle->acState, "RECEIVING"); break;
    case 2: strcpy(psHandle->acState, "RECEIVED"); break;
    case 3: strcpy(psHandle->acState, "VALIDATING"); break;
    case 4: strcpy(psHandle->acState, "VALIDATION OK"); break;
    case 5: strcpy(psHandle->acState, "VALIDATION ERROR"); break;
    default: break;
    }
    psGlobals->sPinCfgIf.bSend(PINCFG_EXTCFGRECEIVER_E, psHandle->u8Id, (void *)psHandle->acState);
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
            psHandle->u16CfgNext = 0;
            strcpy(psGlobals->acCfgBuf + psHandle->u16CfgNext, pcStartString + strlen(PINCFG_CONF_START_STR));
            psHandle->eState = EXTCFGRECEIVER_RECEIVNG_E;
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
            ExtCfgReceiver_vConfigurationReceived(psHandle);
        }
        else
        {
            strcpy(psGlobals->acCfgBuf + psHandle->u16CfgNext, pcMessage);
        }
    }
}

void ExtCfgReceiver_vPresent(EXTCFGRECEIVER_HANDLE_T *psHandle)
{
    psGlobals->sPinCfgIf.bPresent(PINCFG_EXTCFGRECEIVER_E, psHandle->u8Id, psHandle->pcName);
}

void ExtCfgReceiver_vPresentState(EXTCFGRECEIVER_HANDLE_T *psHandle)
{
    psGlobals->sPinCfgIf.bSend(PINCFG_EXTCFGRECEIVER_E, psHandle->u8Id, (void *)psHandle->acState);
    psGlobals->sPinCfgIf.bRequest(PINCFG_EXTCFGRECEIVER_E, psHandle->u8Id);
}

// private
static void ExtCfgReceiver_vConfigurationReceived(EXTCFGRECEIVER_HANDLE_T *psHandle)
{
    // TODO:
    // validate cfg
    // apply cfg
    // save cfg
}
