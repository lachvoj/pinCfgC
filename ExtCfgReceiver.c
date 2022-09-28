#include <string.h>

#include "ExtCfgReceiver.h"
#include "ExternalInterfaces.h"
#include "Memory.h"

static char *pcStartCfgStr = "#{#";
static char *pcEndCfgStr = "#}#";
static char *ppcStatusStr[] = {"OFF", "RECEIVING", "RECEIVED", "VALIDATING", "VALIDATION OK", "VALIDATION ERROR"};

static void ExtCfgReceiver_vConfigurationReceived(EXTCFGRECEIVER_HANDLE_T *psHandle);

EXTCFGRECEIVER_RESULT_T ExtCfgReceiver_eInit(
    EXTCFGRECEIVER_HANDLE_T *psHandle,
    STRING_POINT_T *psName,
    uint8_t u8Id,
    bool bPresent)
{
    if (psHandle == NULL || psName == NULL)
    {
        return EXTCFGRECEIVER_NULLPTR_ERROR_E;
    }

    char *pcName = (char *)Memory_vpAlloc(psName->szLen + 1);
    if (pcName == NULL)
    {
        return EXTCFGRECEIVER_ALLOCATION_ERROR_E;
    }
    // LOOPRE init
    psHandle->sLooPreIf.ePinCfgType = PINCFG_EXTCFGRECEIVER_E;
    // Initialize handle items
    memcpy((void *)pcName, (const void *)psName->pcStrStart, (size_t)psName->szLen);
    pcName[psName->szLen] = '\0';

    psHandle->pcName = pcName;
    psHandle->u8Id = u8Id;
    psHandle->eState = EXTCFGRECEIVER_OFF_E;

    return EXTCFGRECEIVER_OK_E;
}

void ExtCfgReceiver_vSetState(EXTCFGRECEIVER_HANDLE_T *psHandle, const char *psState)
{
    strcpy(psHandle->acState, ppcStatusStr[psHandle->eState]);
    psPinCfg_MySensorsIf->bSend(psHandle->u8Id, PINCFG_EXTCFGRECEIVER_E, (void *)psHandle->acState);
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
    EXTCFGRECEIVER_STATE_T eOldState = psHandle->eState;

    if (psHandle->eState == EXTCFGRECEIVER_OFF_E)
    {
        char *pcStartString = strstr(pcMessage, pcStartCfgStr);
        if (pcStartString != NULL)
        {
            psHandle->u16CfgNext = 0;
            strcpy(psHandle->acConfiguration + psHandle->u16CfgNext, pcStartString + strlen(pcStartCfgStr));
            psHandle->eState = EXTCFGRECEIVER_RECEIVNG_E;
        }
    }
    else if (psHandle->eState == EXTCFGRECEIVER_RECEIVNG_E)
    {
        char *pcEndString = strstr(pcMessage, pcEndCfgStr);
        if (pcEndString != NULL)
        {
            size_t szLen = pcEndString - pcMessage;
            psHandle->eState = EXTCFGRECEIVER_RECEIVED_E;
            memcpy(psHandle->acConfiguration + psHandle->u16CfgNext, pcMessage, szLen);
            ExtCfgReceiver_vConfigurationReceived(psHandle);
        }
        else
        {
            strcpy(psHandle->acConfiguration + psHandle->u16CfgNext, pcMessage);
        }
    }
}

void ExtCfgReceiver_vPresent(EXTCFGRECEIVER_HANDLE_T *psHandle)
{
    psPinCfg_MySensorsIf->bPresent(psHandle->u8Id, PINCFG_EXTCFGRECEIVER_E, psHandle->pcName);
}

void ExtCfgReceiver_vPresentState(EXTCFGRECEIVER_HANDLE_T *psHandle)
{
    psPinCfg_MySensorsIf->bSend(psHandle->u8Id, PINCFG_EXTCFGRECEIVER_E, (void *)psHandle->acState);
    psPinCfg_MySensorsIf->bRequest(psHandle->u8Id, PINCFG_EXTCFGRECEIVER_E);
}

// private
static void ExtCfgReceiver_vConfigurationReceived(EXTCFGRECEIVER_HANDLE_T *psHandle)
{
    // TODO:
    // validate cfg
    // apply cfg
    // save cfg
}
