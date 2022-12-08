#include <string.h>

#include "Globals.h"
#include "Memory.h"
#include "MySensorsPresent.h"

MYSENSORSPRESENT_RESULT_T MySensorsPresent_eInit(
    MYSENSORSPRESENT_HANDLE_T *psHandle,
    STRING_POINT_T *psName,
    uint8_t u8Id,
    PINCFG_ELEMENT_TYPE_T eType)
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
    memcpy((void *)pcName, (const void *)psName->pcStrStart, (size_t)psName->szLen);
    pcName[psName->szLen] = '\0';

    psHandle->sLooPreIf.pcName = pcName;
    psHandle->sLooPreIf.u8Id = u8Id;
#ifdef MY_CONTROLLER_HA
    psHandle->sLooPreIf.bStatePresented = false;
#endif

    return MYSENSORSPRESENT_OK_E;
}

void MySensorsPresent_vSendMySensorsStatus(MYSENSORSPRESENT_HANDLE_T *psHandle)
{
    bSendStatus(psHandle->sLooPreIf.u8Id, psHandle->u8State);
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
void MySensorsPresent_vRcvMessage(MYSENSORSPRESENT_HANDLE_T *psHandle, uint8_t u8State)
{
#ifdef MY_CONTROLLER_HA
    psHandle->sLooPreIf.bStatePresented = true;
#endif
    MySensorsPresent_vSetState(psHandle, u8State, false);
}

void MySensorsPresent_vPresent(MYSENSORSPRESENT_HANDLE_T *psHandle)
{
    bPresentBinary(psHandle->sLooPreIf.u8Id, psHandle->sLooPreIf.pcName);
}

void MySensorsPresent_vPresentState(MYSENSORSPRESENT_HANDLE_T *psHandle)
{
    MySensorsPresent_vSendMySensorsStatus(psHandle);
    bRequestStatus(psHandle->sLooPreIf.u8Id);
}
