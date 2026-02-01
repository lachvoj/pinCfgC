#include <string.h>

#include "Globals.h"
#include "Memory.h"
#include "MySensorsWrapper.h"
#include "Presentable.h"

// Formatting
static const char *Presentable_pcFormatDecimal(int32_t i32Value, uint8_t u8Precision, char *pcBuffer, size_t szBufSize);

PRESENTABLE_RESULT_T Presentable_eInitReuseName(PRESENTABLE_T *psHandle, const char *pcName, uint8_t u8Id)
{
    if (psHandle == NULL || pcName == NULL)
        return PRESENTABLE_NULLPTR_ERROR_E;

    psHandle->pcName = pcName;
    psHandle->u8Id = u8Id;
    psHandle->u32State = 0UL;
    psHandle->u8Flags = 0U;
    psHandle->ePayloadType = P_BYTE;
    psHandle->u8Precision = 0U;

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

    if (i32OldState != psHandle->i32State)
        psHandle->u8Flags |= PRESENTABLE_FLAG_STATE_CHANGED;
#ifdef MY_CONTROLLER_HA
    if (bSendStatus && (psHandle->u8Flags & PRESENTABLE_FLAG_STATE_PRESENTED))
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

    psHandle->u8Flags |= PRESENTABLE_FLAG_STATE_CHANGED;
    Presentable_vSendState(psHandle);
}

void Presentable_vRcvMessage(PRESENTABLE_T *psHandle, const MyMessage *pcMsg)
{
#ifdef MY_CONTROLLER_HA
    psHandle->u8Flags |= PRESENTABLE_FLAG_STATE_PRESENTED;
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
    mysensors_payload_t ePayloadType = psHandle->ePayloadType;
    const char *pcValue = psHandle->pcState;
    char acBuffer[16];

    // Format as decimal string when precision > 0
    if (psHandle->u8Precision > 0U)
    {
        pcValue = Presentable_pcFormatDecimal(psHandle->i32State, psHandle->u8Precision, acBuffer, sizeof(acBuffer));
        ePayloadType = P_STRING;
    }

    if (eMyMessageInit(&msg, psHandle->u8Id, psHandle->psVtab->eVType, ePayloadType, pcValue) != WRAP_OK_E)
        return;

    eSend(&msg, false);
}

const char *Presentable_pcFormatDecimal(int32_t i32Value, uint8_t u8Precision, char *pcBuffer, size_t szBufSize)
{
    static const int32_t ai32Divisors[7] = {1, 10, 100, 1000, 10000, 100000, 1000000};

    if (u8Precision > 6)
        u8Precision = 6;

    if (u8Precision == 0)
    {
        snprintf(pcBuffer, szBufSize, "%ld", (long)i32Value);
        return pcBuffer;
    }

    int32_t i32Divisor = ai32Divisors[u8Precision];
    int32_t i32IntPart = i32Value / i32Divisor;
    int32_t i32FracPart = i32Value % i32Divisor;

    // Handle negative values: -0.5 should be "-0.5" not "0.-5"
    if (i32Value < 0 && i32IntPart == 0)
    {
        i32FracPart = -i32FracPart;
        snprintf(pcBuffer, szBufSize, "-%ld.%0*ld", (long)i32IntPart, (int)u8Precision, (long)i32FracPart);
    }
    else
    {
        if (i32FracPart < 0)
            i32FracPart = -i32FracPart;
        snprintf(pcBuffer, szBufSize, "%ld.%0*ld", (long)i32IntPart, (int)u8Precision, (long)i32FracPart);
    }

    return pcBuffer;
}
