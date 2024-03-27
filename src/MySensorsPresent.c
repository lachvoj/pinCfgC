#include <string.h>

#include "Globals.h"
#include "Memory.h"
#include "MySensorsPresent.h"
#include "MySensorsWrapper.h"

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
        MySensorsPresent_vSendState((LOOPRE_T *)psHandle);
}

uint8_t MySensorsPresent_u8GetState(MYSENSORSPRESENT_HANDLE_T *psHandle)
{
    return psHandle->u8State;
}

void MySensorsPresent_vToggle(MYSENSORSPRESENT_HANDLE_T *psHandle)
{
    if (psHandle->u8State == (uint8_t) false)
        psHandle->u8State = (uint8_t) true;
    else
        psHandle->u8State = (uint8_t) false;

    psHandle->bStateChanged = true;
    MySensorsPresent_vSendState((LOOPRE_T *)psHandle);
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
    ePresent(psBaseHandle->u8Id, psBaseHandle->psVtab->eSType, psBaseHandle->pcName, false);
}

void MySensorsPresent_vPresentState(LOOPRE_T *psBaseHandle)
{
    MySensorsPresent_vSendState(psBaseHandle);
    eRequest(psBaseHandle->u8Id, psBaseHandle->psVtab->eVType, GATEWAY_ADDRESS);
}

void MySensorsPresent_vSendState(LOOPRE_T *psBaseHandle)
{
    MyMessage msg;
    if (eMyMessageInit(&msg, psBaseHandle->u8Id, psBaseHandle->psVtab->eVType) != WRAP_OK_E)
        return;

    if (eMyMessageSetUInt8(&msg, ((MYSENSORSPRESENT_HANDLE_T *)psBaseHandle)->u8State) != WRAP_OK_E)
        return;

    eSend(&msg, false);
}
