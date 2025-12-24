#include <string.h>

#include "Globals.h"
#include "Memory.h"
#include "MySensorsWrapper.h"
#include "Presentable.h"

PRESENTABLE_RESULT_T Presentable_eInitReuseName(PRESENTABLE_T *psHandle, const char *pcName, uint8_t u8Id)
{
    if (psHandle == NULL || pcName == NULL)
        return PRESENTABLE_NULLPTR_ERROR_E;

    psHandle->pcName = pcName;
    psHandle->u8Id = u8Id;
    psHandle->u32State = 0UL;
    psHandle->bStateChanged = false;
    psHandle->ePayloadType = P_BYTE;
#ifdef MY_CONTROLLER_HA
    psHandle->bStatePresented = false;
#endif

    return PRESENTABLE_OK_E;
}

PRESENTABLE_RESULT_T Presentable_eInit(PRESENTABLE_T *psHandle, STRING_POINT_T *psName, uint8_t u8Id)
{
    if (psHandle == NULL || psName == NULL)
        return PRESENTABLE_NULLPTR_ERROR_E;

    char *pcName = (char *)Memory_vpAlloc(psName->szLen + 1);
    if (pcName == NULL)
        return PRESENTABLE_ALLOCATION_ERROR_E;

    memcpy((void *)pcName, (const void *)psName->pcStrStart, (size_t)psName->szLen);
    pcName[psName->szLen] = '\0';

    return Presentable_eInitReuseName(psHandle, pcName, u8Id);
}

uint8_t Presentable_u8GetId(PRESENTABLE_T *psHandle)
{
    return psHandle->u8Id;
}

const char *Presentable_pcGetName(PRESENTABLE_T *psHandle)
{
    return psHandle->pcName;
}

void Presentable_vSetState(PRESENTABLE_T *psHandle, int32_t i32State, bool bSendStatus)
{
    uint32_t u32Mask = 0xFFFFFFFF;

    switch (psHandle->ePayloadType)
    {
    case P_BYTE: u32Mask = 0xFF; break;
    case P_INT16: u32Mask = 0xFFFF; break;
    default: break;
    }

    int32_t i32OldState = (psHandle->i32State & u32Mask);
    psHandle->i32State = (i32State & u32Mask);

    psHandle->bStateChanged = (i32OldState != psHandle->i32State);
#ifdef MY_CONTROLLER_HA
    if (bSendStatus && psHandle->bStatePresented)
#else
    if (bSendStatus)
#endif
        Presentable_vSendState(psHandle);
}

uint8_t Presentable_u8GetState(PRESENTABLE_T *psHandle)
{
    return psHandle->u8State;
}

uint32_t Presentable_u32GetState(PRESENTABLE_T *psHandle)
{
    return psHandle->u32State;
}

void Presentable_vToggle(PRESENTABLE_T *psHandle)
{
    if (psHandle->u8State == (uint8_t) false)
        psHandle->u8State = (uint8_t) true;
    else
        psHandle->u8State = (uint8_t) false;

    psHandle->bStateChanged = true;
    Presentable_vSendState(psHandle);
}

void Presentable_vRcvMessage(PRESENTABLE_T *psHandle, const MyMessage *pcMsg)
{
#ifdef MY_CONTROLLER_HA
    psHandle->bStatePresented = true;
#endif
    int32_t i32NewState = 0UL | eMessageGetByte(pcMsg);
    Presentable_vSetState(psHandle, i32NewState, false);
}

void Presentable_vPresent(PRESENTABLE_T *psHandle)
{
    ePresent(psHandle->u8Id, psHandle->psVtab->eSType, psHandle->pcName, false);
}

void Presentable_vPresentState(PRESENTABLE_T *psHandle)
{
    Presentable_vSendState(psHandle);
    eRequest(psHandle->u8Id, psHandle->psVtab->eVType, GATEWAY_ADDRESS);
}

void Presentable_vSendState(PRESENTABLE_T *psHandle)
{
    MyMessage msg;
    if (eMyMessageInit(&msg, psHandle->u8Id, psHandle->psVtab->eVType, psHandle->ePayloadType, psHandle->pcState) !=
        WRAP_OK_E)
        return;

    eSend(&msg, false);
}
