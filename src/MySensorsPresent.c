#include <string.h>

#include "Globals.h"
#include "Memory.h"
#include "MySensorsPresent.h"

MYSENSORSPRESENT_RESULT_T MySensorsPresent_eInit(
    MYSENSORSPRESENT_HANDLE_T *psHandle,
    STRING_POINT_T *psName,
    uint8_t u8Id)
{
    if (psHandle == NULL || psName == NULL)
        return MYSENSORSPRESENT_NULLPTR_ERROR_E;

    char *pcName = (char *)Memory_vpAlloc(psName->szLen + 1);
    if (pcName == NULL)
    {
        return MYSENSORSPRESENT_ALLOCATION_ERROR_E;
    }
    // LOOPRE init
    memcpy((void *)pcName, (const void *)psName->pcStrStart, (size_t)psName->szLen);
    pcName[psName->szLen] = '\0';

    psHandle->sLooPre.pcName = pcName;
    psHandle->sLooPre.u8Id = u8Id;
#ifdef MY_CONTROLLER_HA
    psHandle->sLooPre.bStatePresented = false;
#endif

    return MYSENSORSPRESENT_OK_E;
}

void MySensorsPresent_vSetState(MYSENSORSPRESENT_HANDLE_T *psHandle, uint8_t u8State, bool bSendStatus)
{
    psHandle->bStateChanged = (psHandle->u8State != u8State);
    psHandle->u8State = u8State;
    if (psHandle->bStateChanged && bSendStatus)
        ((LOOPRE_T *)psHandle)->psVtab->vSendState((LOOPRE_T *)psHandle);
}

void MySensorsPresent_vToggle(MYSENSORSPRESENT_HANDLE_T *psHandle)
{
    if (psHandle->u8State == (uint8_t) false)
        psHandle->u8State = (uint8_t) true;
    else
        psHandle->u8State = (uint8_t) false;

    psHandle->bStateChanged = true;
    ((LOOPRE_T *)psHandle)->psVtab->vSendState((LOOPRE_T *)psHandle);
}

// IMySensorsPresentable
void MySensorsPresent_vRcvMessage(LOOPRE_T *psBaseHandle, const void *pvMessage)
{
#ifdef MY_CONTROLLER_HA
    psBaseHandle->bStatePresented = true;
#endif
    MySensorsPresent_vSetState((MYSENSORSPRESENT_HANDLE_T *)psBaseHandle, *((const uint8_t *)pvMessage), false);
}

void MySensorsPresent_vPresent(LOOPRE_T *psBaseHandle)
{
    psGlobals->sPincfgIf.bPresent(psBaseHandle->u8Id, psBaseHandle->psVtab->u8SType, psBaseHandle->pcName);
}

void MySensorsPresent_vPresentState(LOOPRE_T *psBaseHandle)
{
    psBaseHandle->psVtab->vSendState(psBaseHandle);
    psGlobals->sPincfgIf.bRequest(psBaseHandle->u8Id, psBaseHandle->psVtab->u8VType, GATEWAY_ADDRESS);
}

void MySensorsPresent_vSendState(LOOPRE_T *psBaseHandle)
{
    psGlobals->sPincfgIf.bSend(
        psBaseHandle->u8Id,
        psBaseHandle->psVtab->u8VType,
        (const void *)&(((MYSENSORSPRESENT_HANDLE_T *)psBaseHandle)->u8State));
}
