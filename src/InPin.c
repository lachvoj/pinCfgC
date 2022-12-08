#ifdef ARDUINO
#include <Arduino.h>
#else
#include <ArduinoMock.h>
#endif
#include <errno.h>

#include "Globals.h"
#include "InPin.h"
#include "PinSubscriberIf.h"

void InPin_SetDebounceMs(uint32_t u32Debounce)
{
    psGlobals->u32InPinDebounceMs = u32Debounce;
}

void InPin_SetMulticlickMaxDelayMs(uint32_t u32MultikMaxDelay)
{
    psGlobals->u32InPinMulticlickMaxDelayMs = u32MultikMaxDelay;
}

INPIN_RESULT_T InPin_eInit(
    INPIN_HANDLE_T *psHandle,
    STRING_POINT_T *sName,
    uint8_t u8Id,
    uint8_t u8InPin)
{
    if (psHandle == NULL)
        return INPIN_NULLPTR_ERROR_E;

    if (MySensorsPresent_eInit(&psHandle->sMySenPresent, sName, u8Id, PINCFG_INPIN_E) !=
        MYSENSORSPRESENT_OK_E)
    {
        return INPIN_SUBINIT_ERROR_E;
    }

    psHandle->u8InPin = u8InPin;
    psHandle->u32TimerDebounceStarted--;
    psHandle->u32timerMultiStarted--;

    pinMode(u8InPin, INPUT_PULLUP);
    digitalWrite(u8InPin, HIGH); // enabling pullup

    return INPIN_OK_E;
}

INPIN_RESULT_T InPin_eAddSubscriber(INPIN_HANDLE_T *psHandle, PINSUBSCRIBER_IF_T *psSubscriber)
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

void InPin_vSendEvent(INPIN_HANDLE_T *psHandle, uint8_t u8EventType, uint32_t u32Data)
{
    PINSUBSCRIBER_IF_T *psCurrent = psHandle->psFirstSubscriber;
    while (psCurrent != NULL)
    {
        PinSubscriberIf_vEventHandle(psCurrent, u8EventType, u32Data);
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

void InPin_vLoop(INPIN_HANDLE_T *psHandle, uint32_t u32ms)
{
    MYSENSORSPRESENT_HANDLE_T *psMySensorsPresentHnd = (MYSENSORSPRESENT_HANDLE_T *)psHandle;

    INPIN_CHANGE_T eChange = INPIN_CHANGE_NOCHANGE_E;
    if (psHandle->ePinState != INPIN_DEBOUNCEDOWN_E && psHandle->ePinState != INPIN_DEBOUNCEUP_E)
    {
        bool bPinState = (bool)digitalRead(psHandle->u8InPin);
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
        if (u32ms - psHandle->u32TimerDebounceStarted < psGlobals->u32InPinDebounceMs)
            break;

        psHandle->ePinState = INPIN_DOWN_E;
        InPin_vSendEvent(psHandle, (uint8_t)psHandle->ePinState, 0U);
        MySensorsPresent_vSetState(psMySensorsPresentHnd, (uint8_t) false, true);
    }
    break;
    case INPIN_DOWN_E:
    {
        if (psHandle->u8PressCount > 0 &&
            (u32ms - psHandle->u32timerMultiStarted) > psGlobals->u32InPinMulticlickMaxDelayMs)
        {
            InPin_vSendEvent(psHandle, (uint8_t)INPIN_MULTI_E, (uint32_t)psHandle->u8PressCount);
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
        if (u32ms - psHandle->u32TimerDebounceStarted < psGlobals->u32InPinDebounceMs)
            break;

        psHandle->ePinState = INPIN_UP_E;
        psHandle->u32timerMultiStarted = u32ms;
        psHandle->u8PressCount++;
        InPin_vSendEvent(psHandle, (uint8_t)psHandle->ePinState, (int)psHandle->u8PressCount);
        MySensorsPresent_vSetState(psMySensorsPresentHnd, (uint8_t) true, true);
    }
    break;
    case INPIN_UP_E:
    {
        if (u32ms - psHandle->u32timerMultiStarted > psGlobals->u32InPinMulticlickMaxDelayMs)
        {
            psHandle->ePinState = INPIN_LONG_E;
            InPin_vSendEvent(psHandle, (uint8_t)psHandle->ePinState, 0U);
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
