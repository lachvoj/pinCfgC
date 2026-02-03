#include "InPin.h"

#include <errno.h>

#include "Globals.h"
#include "MySensorsWrapper.h"
#include "PinCfgUtils.h"
#include "PinSubscriberIf.h"

void InPin_SetDebounceMs(uint32_t u32Debounce)
{
    psGlobals->u32InPinDebounceMs = u32Debounce;
}

void InPin_SetMulticlickMaxDelayMs(uint32_t u32MultikMaxDelay)
{
    psGlobals->u32InPinMulticlickMaxDelayMs = u32MultikMaxDelay;
}

void InPin_vInitType(PRESENTABLE_VTAB_T *psVtab)
{
    psVtab->eVType = V_TRIPPED;
    psVtab->eSType = S_DOOR;
    psVtab->vReceive = InPin_vRcvMessage;
    psVtab->vPresent = Presentable_vPresent;

    InPin_SetDebounceMs(PINCFG_DEBOUNCE_MS_D);
    InPin_SetMulticlickMaxDelayMs(PINCFG_MULTICLICK_MAX_DELAY_MS_D);
}

INPIN_RESULT_T InPin_eInit(INPIN_T *psHandle, STRING_POINT_T *sName, uint8_t u8Id, uint8_t u8InPin)
{
    if (psHandle == NULL)
        return INPIN_NULLPTR_ERROR_E;

    if (Presentable_eInit(&psHandle->sPresentable, sName, u8Id) != PRESENTABLE_OK_E)
    {
        return INPIN_SUBINIT_ERROR_E;
    }

    // vtab init
    psHandle->sPresentable.psVtab = &psGlobals->sInPinPrVTab;

    // loopable init
    psHandle->sLoopable.vLoop = InPin_vLoop;

    psHandle->u8InPin = u8InPin;
    psHandle->ePinState = INPIN_DOWN_E;
    psHandle->bLastPinState = false; // Start LOW so first HIGH is detected as change
    psHandle->u8PressCount = 0U;
    psHandle->u32TimerDebounceStarted = 0U;
    psHandle->u32timerMultiStarted = 0U;
    psHandle->psFirstSubscriber = NULL;

    pinMode(u8InPin, INPUT_PULLUP);
    digitalWrite(u8InPin, HIGH); // enabling pullup

    return INPIN_OK_E;
}

INPIN_RESULT_T InPin_eAddSubscriber(INPIN_T *psHandle, PINSUBSCRIBER_IF_T *psSubscriber)
{
    if (psHandle == NULL || psSubscriber == NULL)
        return INPIN_NULLPTR_ERROR_E;

    if (psHandle->psFirstSubscriber != NULL)
    {
        PINSUBSCRIBER_IF_T *psCurrent = psHandle->psFirstSubscriber;
        while (psCurrent->psNext != NULL)
        {
            psCurrent = psCurrent->psNext;
        }
        psCurrent->psNext = psSubscriber;
    }
    else
    {
        psHandle->psFirstSubscriber = psSubscriber;
    }

    return INPIN_OK_E;
}

void InPin_vSendEvent(INPIN_T *psHandle, uint8_t u8EventType, uint32_t u32Data, uint32_t u32ms)
{
    PINSUBSCRIBER_IF_T *psCurrent = psHandle->psFirstSubscriber;
    while (psCurrent != NULL)
    {
        psCurrent->vEventHandle(psCurrent, u8EventType, u32Data, u32ms);
        psCurrent = psCurrent->psNext;
    }
}

// loopable IF
typedef enum
{
    INPIN_CHANGE_NOCHANGE_E,
    INPIN_CHANGE_UP_E,
    INPIN_CHANGE_DOWN_E
} INPIN_CHANGE_T;

void InPin_vLoop(LOOPABLE_T *psLoopableHandle, uint32_t u32ms)
{
    INPIN_T *psHandle = container_of(psLoopableHandle, INPIN_T, sLoopable);

    INPIN_CHANGE_T eChange = INPIN_CHANGE_NOCHANGE_E;
    if (psHandle->ePinState != INPIN_DEBOUNCEDOWN_E && psHandle->ePinState != INPIN_DEBOUNCEUP_E)
    {
        bool bPinState = (bool)digitalRead(psHandle->u8InPin);
        if (bPinState != psHandle->bLastPinState)
        {
            if (bPinState)
                eChange = INPIN_CHANGE_UP_E;
            else
                eChange = INPIN_CHANGE_DOWN_E;

            psHandle->bLastPinState = bPinState;
        }
    }

    switch (psHandle->ePinState)
    {
    case INPIN_DEBOUNCEDOWN_E:
    {
        if (PinCfg_u32GetElapsedTime(psHandle->u32TimerDebounceStarted, u32ms) < psGlobals->u32InPinDebounceMs)
            break;

        psHandle->ePinState = INPIN_DOWN_E;
        InPin_vSendEvent(psHandle, (uint8_t)psHandle->ePinState, 0U, u32ms);
        Presentable_vSetState((PRESENTABLE_T *)psHandle, (int32_t) false, true);
    }
    break;
    case INPIN_DOWN_E:
    {
        if (psHandle->u8PressCount > 0 &&
            PinCfg_u32GetElapsedTime(psHandle->u32timerMultiStarted, u32ms) > psGlobals->u32InPinMulticlickMaxDelayMs)
        {
            InPin_vSendEvent(psHandle, (uint8_t)INPIN_MULTI_E, (uint32_t)psHandle->u8PressCount, u32ms);
            psHandle->u8PressCount = 0U;
        }

        if (eChange == INPIN_CHANGE_UP_E)
        {
            psHandle->ePinState = INPIN_DEBOUNCEUP_E;
            psHandle->u32TimerDebounceStarted = u32ms;
        }
    }
    break;
    case INPIN_DEBOUNCEUP_E:
    {
        if (PinCfg_u32GetElapsedTime(psHandle->u32TimerDebounceStarted, u32ms) < psGlobals->u32InPinDebounceMs)
            break;

        psHandle->ePinState = INPIN_UP_E;
        psHandle->u32timerMultiStarted = u32ms;
        psHandle->u8PressCount++;
        InPin_vSendEvent(psHandle, (uint8_t)psHandle->ePinState, (int)psHandle->u8PressCount, u32ms);
        Presentable_vSetState((PRESENTABLE_T *)psHandle, (int32_t) true, true);
    }
    break;
    case INPIN_UP_E:
    {
        if (PinCfg_u32GetElapsedTime(psHandle->u32timerMultiStarted, u32ms) > psGlobals->u32InPinMulticlickMaxDelayMs)
        {
            psHandle->ePinState = INPIN_LONG_E;
            InPin_vSendEvent(psHandle, (uint8_t)psHandle->ePinState, 0U, u32ms);
            psHandle->u8PressCount = 0U;
        }
        else if (eChange == INPIN_CHANGE_DOWN_E)
        {
            psHandle->ePinState = INPIN_DEBOUNCEDOWN_E;
            psHandle->u32TimerDebounceStarted = u32ms;
        }
    }
    break;
    case INPIN_LONG_E:
    {
        if (eChange == INPIN_CHANGE_DOWN_E)
        {
            psHandle->ePinState = INPIN_DEBOUNCEDOWN_E;
            psHandle->u32TimerDebounceStarted = u32ms;
        }
    }
    break;
    default: break;
    }
}

// presentable IF
void InPin_vRcvMessage(PRESENTABLE_T *psBaseHandle, const MyMessage *pcMsg)
{
    (void)pcMsg;
#ifdef MY_CONTROLLER_HA
    psBaseHandle->u8Flags |= PRESENTABLE_FLAG_STATE_PRESENTED;
#else
    (void)psBaseHandle;
#endif
}
