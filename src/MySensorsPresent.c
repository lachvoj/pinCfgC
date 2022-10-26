#include <string.h>

#include "Globals.h"
#include "Memory.h"
#include "MySensorsPresent.h"

MYSENSORSPRESENT_RESULT_T MySensorsPresent_eInit(
    MYSENSORSPRESENT_HANDLE_T *psHandle,
    STRING_POINT_T *psName,
    uint8_t u8Id,
    PINCFG_ELEMENT_TYPE_T eType,
    bool bPresent)
{
    if (psHandle == NULL || psName == NULL)
        return MYSENSORSPRESENT_NULLPTR_ERROR_E;

    char *pcName = (char *)Memory_vpAlloc(psName->szLen + 1);
    if (pcName == NULL)
    {
        return MYSENSORSPRESENT_ALLOCATION_ERROR_E;
    }
    // LOOPRE init
    psHandle->sLooPreIf.ePinCfgType = eType;

    // Initialize handle items
    memcpy((void *)pcName, (const void *)psName->pcStrStart, (size_t)psName->szLen);
    pcName[psName->szLen] = '\0';

    psHandle->pcName = pcName;
    psHandle->u8Id = u8Id;
    psHandle->bPresent = bPresent;

    return MYSENSORSPRESENT_OK_E;
}

void MySensorsPresent_vSendMySensorsStatus(MYSENSORSPRESENT_HANDLE_T *psHandle)
{
    psGlobals->sPinCfgIf.bSend(psHandle->u8Id, psHandle->sLooPreIf.ePinCfgType, (void *)(&psHandle->u8State));
}

void MySensorsPresent_vSetState(MYSENSORSPRESENT_HANDLE_T *psHandle, uint8_t u8State)
{
    psHandle->bStateChanged = (psHandle->u8State != u8State);
    psHandle->u8State = u8State;
    if (psHandle->bStateChanged)
        MySensorsPresent_vSendMySensorsStatus(psHandle);
}

void MySensorsPresent_vToggle(MYSENSORSPRESENT_HANDLE_T *psHandle)
{
    if (psHandle->u8State == (uint8_t) false)
        psHandle->u8State = (uint8_t) true;
    else
        psHandle->u8State = (uint8_t) false;

    psHandle->bStateChanged = true;
    MySensorsPresent_vSendMySensorsStatus(psHandle);
}

// IMySensorsPresentable
uint8_t MySensorsPresent_u8GetId(MYSENSORSPRESENT_HANDLE_T *psHandle)
{
    return psHandle->u8Id;
}

const char *MySensorsPresent_pcGetName(MYSENSORSPRESENT_HANDLE_T *psHandle)
{
    return psHandle->pcName;
}

void MySensorsPresent_vRcvMessage(MYSENSORSPRESENT_HANDLE_T *psHandle, const void *pvMessage)
{
    MySensorsPresent_vSetState(psHandle, *((uint16_t *)pvMessage));
}

void MySensorsPresent_vPresent(MYSENSORSPRESENT_HANDLE_T *psHandle)
{
    psGlobals->sPinCfgIf.bPresent(psHandle->u8Id, psHandle->sLooPreIf.ePinCfgType, psHandle->pcName);
}

void MySensorsPresent_vPresentState(MYSENSORSPRESENT_HANDLE_T *psHandle)
{
    MySensorsPresent_vSendMySensorsStatus(psHandle);
    psGlobals->sPinCfgIf.bRequest(psHandle->sLooPreIf.ePinCfgType, psHandle->u8Id);
}
