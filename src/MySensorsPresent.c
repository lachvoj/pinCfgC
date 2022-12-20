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

void MySensorsPresent_vSendMySensorsStatus(MYSENSORSPRESENT_HANDLE_T *psHandle)
{
    bSendStatus(psHandle->sLooPre.u8Id, psHandle->u8State);
}

void MySensorsPresent_vSetState(MYSENSORSPRESENT_HANDLE_T *psHandle, uint8_t u8State, bool bSendStatus)
{
    psHandle->bStateChanged = (psHandle->u8State != u8State);
    psHandle->u8State = u8State;
    if (psHandle->bStateChanged && bSendStatus)
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
void MySensorsPresent_vRcvMessage(LOOPRE_T *psBaseHandle, uint8_t u8State)
{
#ifdef MY_CONTROLLER_HA
    psBaseHandle->bStatePresented = true;
#endif
    MySensorsPresent_vSetState((MYSENSORSPRESENT_HANDLE_T *)psBaseHandle, u8State, false);
}

void MySensorsPresent_vPresent(LOOPRE_T *psBaseHandle)
{
    bPresentBinary(psBaseHandle->u8Id, psBaseHandle->pcName);
}

void MySensorsPresent_vPresentState(LOOPRE_T *psBaseHandle)
{
    MySensorsPresent_vSendMySensorsStatus((MYSENSORSPRESENT_HANDLE_T *)psBaseHandle);
    bRequestStatus(psBaseHandle->u8Id);
}
