#include <errno.h>

#include "ExternalInterfaces.h"
#include "InPin.h"

static uint64_t gu64InPinDebounceMs;
static uint64_t gu64InPinMulticlickMaxDelayMs;

void InPin_SetDebounceMs(uint64_t u64Debounce)
{
    gu64InPinDebounceMs = u64Debounce;
}

void InPin_SetMulticlickMaxDelayMs(uint64_t u64MultikMaxDelay)
{
    gu64InPinMulticlickMaxDelayMs = u64MultikMaxDelay;
}

INPIN_RESULT_T InPin_eInit(
    INPIN_HANDLE_T *psHandle,
    STRING_POINT_T *sName,
    uint8_t u8Id,
    bool bPresent,
    uint8_t u8InPin)
{
    if (psHandle == NULL)
        return INPIN_NULLPTR_ERROR_E;

    if (MySensorsPresent_eInit(&psHandle->sMySenPresent, sName, u8Id, PINCFG_INPIN_E, bPresent) !=
        MYSENSORSPRESENT_OK_E)
    {
        return INPIN_SUBINIT_ERROR_E;
    }

    psHandle->u8InPin = u8InPin;
    psHandle->u64TimerDebounceStarted--;
    psHandle->u64timerMultiStarted--;

    return INPIN_OK_E;
}

INPIN_RESULT_T InPin_eAddSubscriber(INPIN_HANDLE_T *psHandle, PINSUBSCRIBER_IF *psSubscriber)
{
    if (psHandle->u8SubscribersCount >= PINCFG_INPIN_MAX_SUBSCRIBERS_D)
    {
        return INPIN_MAXSUBSCRIBERS_ERROR_E;
    }

    if (psSubscriber == NULL || psSubscriber->eventHandle == NULL)
        return INPIN_NULLPTR_ERROR_E;

    psHandle->apsSubscribers[psHandle->u8SubscribersCount] = psSubscriber;
    psHandle->u8SubscribersCount++;

    return INPIN_OK_E;
}

void InPin_vSendEvent(INPIN_HANDLE_T *psHandle, uint8_t u8EventType, uint32_t u32Data)
{
    for (uint8_t i = 0; i < psHandle->u8SubscribersCount; i++)
    {
        psHandle->apsSubscribers[i]->eventHandle(psHandle->apsSubscribers[i], u8EventType, u32Data);
    }
}

bool InPin_bReadPin(INPIN_HANDLE_T *psHandle)
{
    return (bool)psPinCfg_PinIf->u8ReadPin(psHandle->u8InPin);
}

// loopable IF
typedef enum
{
    INPIN_CHANGE_NOCHANGE_E,
    INPIN_CHANGE_UP_E,
    INPIN_CHANGE_DOWN_E
} INPIN_CHANGE_T;

void InPin_vLoop(INPIN_HANDLE_T *psHandle, uint64_t u64ms)
{
    MYSENSORSPRESENT_HANDLE_T *psMySensorsPresentHnd = (MYSENSORSPRESENT_HANDLE_T *)psHandle;

    INPIN_CHANGE_T eChange = INPIN_CHANGE_NOCHANGE_E;
    if (psHandle->ePinState != INPIN_DEBOUNCEDOWN_E && psHandle->ePinState != INPIN_DEBOUNCEUP_E)
    {
        bool bPinState = InPin_bReadPin(psHandle);
        if (bPinState != psHandle->bLastPinState)
        {
            if (bPinState)
                eChange = INPIN_CHANGE_DOWN_E;
            else
                eChange = INPIN_CHANGE_UP_E;

            psHandle->bLastPinState = bPinState;
        }
    }

    switch (psHandle->ePinState)
    {
    case INPIN_DEBOUNCEDOWN_E:
    {
        if (u64ms - psHandle->u64TimerDebounceStarted < gu64InPinDebounceMs)
            break;

        psHandle->ePinState = INPIN_DOWN_E;
        InPin_vSendEvent(psHandle, (uint8_t)psHandle->ePinState, 0U);
        MySensorsPresent_vSetState(psMySensorsPresentHnd, (uint8_t) false);
    }
    break;
    case INPIN_DOWN_E:
    {
        if (psHandle->u8PressCount > 0 && (u64ms - psHandle->u64timerMultiStarted) > gu64InPinMulticlickMaxDelayMs)
        {
            InPin_vSendEvent(psHandle, (uint8_t)INPIN_MULTI_E, (uint32_t)psHandle->u8PressCount);
            psHandle->u8PressCount = 0U;
        }
        if (eChange == INPIN_CHANGE_UP_E)
        {
            psHandle->ePinState = INPIN_DEBOUNCEUP_E;
            psHandle->u64TimerDebounceStarted = u64ms;
        }
    }
    break;
    case INPIN_DEBOUNCEUP_E:
    {
        if (u64ms - psHandle->u64TimerDebounceStarted < gu64InPinDebounceMs)
            break;

        psHandle->ePinState = INPIN_UP_E;
        psHandle->u64timerMultiStarted = u64ms;
        psHandle->u8PressCount++;
        InPin_vSendEvent(psHandle, (uint8_t)psHandle->ePinState, (int)psHandle->u8PressCount);
        MySensorsPresent_vSetState(psMySensorsPresentHnd, (uint8_t) true);
    }
    break;
    case INPIN_UP_E:
    {
        if (u64ms - psHandle->u64timerMultiStarted > gu64InPinMulticlickMaxDelayMs)
        {
            psHandle->ePinState = INPIN_LONG_E;
            InPin_vSendEvent(psHandle, (uint8_t)psHandle->ePinState, 0U);
            psHandle->u8PressCount = 0U;
        }
        else if (eChange == INPIN_CHANGE_DOWN_E)
        {
            psHandle->ePinState = INPIN_DEBOUNCEDOWN_E;
            psHandle->u64TimerDebounceStarted = u64ms;
        }
    }
    break;
    case INPIN_LONG_E:
    {
        if (eChange == INPIN_CHANGE_DOWN_E)
        {
            psHandle->ePinState = INPIN_DEBOUNCEDOWN_E;
            psHandle->u64TimerDebounceStarted = u64ms;
        }
    }
    break;
    default: break;
    }
}
