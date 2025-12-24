#include "Globals.h"
#include "MySensorsWrapper.h"
#include "PinCfgUtils.h"
#include "Switch.h"
#include "Trigger.h"

// loopable IF
void Switch_vLoopClassic(LOOPABLE_T *psLoopableHandle, uint32_t u32ms);
void Switch_vLoopClassicFeedback(LOOPABLE_T *psLoopableHandle, uint32_t u32ms);
void Switch_vLoopImpulse(LOOPABLE_T *psLoopableHandle, uint32_t u32ms);
void Switch_vLoopImpulseFeedback(LOOPABLE_T *psLoopableHandle, uint32_t u32ms);
void Switch_vLoopTimed(LOOPABLE_T *psLoopableHandle, uint32_t u32ms);
void Switch_vLoopTimedFeedback(LOOPABLE_T *psLoopableHandle, uint32_t u32ms);

static inline void Switch_vWritePin(SWITCH_T *psHandle, uint8_t u8Value)
{
    digitalWrite(psHandle->u8OutPin, u8Value);
}

void Switch_SetImpulseDurationMs(uint32_t u32ImpulseDuration)
{
    psGlobals->u32SwitchImpulseDurationMs = u32ImpulseDuration;
}

void Switch_SetFbOnDelayMs(uint32_t u32FbOnDelayMs)
{
    psGlobals->u32SwitchFbOnDelayMs = u32FbOnDelayMs;
}

void Switch_SetFbOffDelayMs(uint32_t u32FbOffDelayMs)
{
    psGlobals->u32SwitchFbOffDelayMs = u32FbOffDelayMs;
}

void Switch_vInitType(PRESENTABLE_VTAB_T *psVtab)
{
    psVtab->eVType = V_STATUS;
    psVtab->eSType = S_BINARY;
    psVtab->vReceive = Presentable_vRcvMessage;
    psVtab->vPresent = Presentable_vPresent;

    Switch_SetImpulseDurationMs(PINCFG_SWITCH_IMPULSE_DURATIN_MS_D);
    Switch_SetFbOnDelayMs(PINCFG_SWITCH_FB_ON_DELAY_MS_D);
    Switch_SetFbOffDelayMs(PINCFG_SWITCH_FB_OFF_DELAY_MS_D);
}

SWITCH_RESULT_T Switch_eInit(
    SWITCH_T *psHandle,
    STRING_POINT_T *sName,
    uint8_t u8Id,
    SWITCH_MODE_T eMode,
    uint8_t u8OutPin,
    uint8_t u8FbPin,
    uint32_t u32TimedAdidtionalDelayMs)
{
    if (psHandle == NULL)
        return SWITCH_NULLPTR_ERROR_E;

    if (Presentable_eInit(&psHandle->sPresentable, sName, u8Id) != PRESENTABLE_OK_E)
    {
        return SWITCH_SUBINIT_ERROR_E;
    }

    // vtab init
    psHandle->sPresentable.psVtab = &psGlobals->sSwitchPrVTab;

    // loopable init
    switch (eMode)
    {
    case SWITCH_CLASSIC_E:
        if (u8FbPin > 0)
            psHandle->sLoopable.vLoop = Switch_vLoopClassicFeedback;
        else
            psHandle->sLoopable.vLoop = Switch_vLoopClassic;
        break;

    case SWITCH_IMPULSE_E:
        if (u8FbPin > 0)
            psHandle->sLoopable.vLoop = Switch_vLoopImpulseFeedback;
        else
            psHandle->sLoopable.vLoop = Switch_vLoopImpulse;
        break;

    case SWITCH_TIMED_E:
        if (u8FbPin > 0)
            psHandle->sLoopable.vLoop = Switch_vLoopTimedFeedback;
        else
            psHandle->sLoopable.vLoop = Switch_vLoopTimed;
        break;

    default:
        return SWITCH_INIT_ERROR_E; // unsupported mode
        break;
    }

    psHandle->eMode = eMode;
    psHandle->u8OutPin = u8OutPin;
    psHandle->u8FbPin = u8FbPin;
    psHandle->u32ImpulseStarted = 0U;
    psHandle->u32ImpulseDuration = psGlobals->u32SwitchImpulseDurationMs;
    psHandle->u32FbReadStarted = 0U;
    psHandle->u32TimedAdidtionalDelayMs = u32TimedAdidtionalDelayMs;

    if (u8FbPin > 0)
    {
        pinMode(u8FbPin, INPUT_PULLUP);
        digitalWrite(u8FbPin, HIGH); // enabling pullup
    }
    pinMode(u8OutPin, OUTPUT);
    digitalWrite(u8OutPin, LOW);

    return SWITCH_OK_E;
}

static inline void Switch_vCheckFbPin(SWITCH_T *psHandle, uint32_t u32ms)
{
    uint8_t u8ActualPinState = (uint8_t)!digitalRead(psHandle->u8FbPin);
    if (psHandle->sPresentable.u8State != u8ActualPinState)
    {
        if (psHandle->u32FbReadStarted == 0U)
        {
            psHandle->u32FbReadStarted = u32ms;
        }
        else if (
            (psHandle->sPresentable.u8State == 1 &&
             PinCfg_u32GetElapsedTime(psHandle->u32FbReadStarted, u32ms) > psGlobals->u32SwitchFbOnDelayMs) ||
            (psHandle->sPresentable.u8State == 0 &&
             PinCfg_u32GetElapsedTime(psHandle->u32FbReadStarted, u32ms) > psGlobals->u32SwitchFbOffDelayMs))
        {
            psHandle->u32FbReadStarted = 0U;
            Presentable_vSetState((PRESENTABLE_T *)psHandle, (int32_t)u8ActualPinState, true);
        }
    }
    else
    {
        psHandle->u32FbReadStarted = 0U;
    }
}

static inline void Switch_vHandleClassic(SWITCH_T *psHandle)
{
    psHandle->sPresentable.bStateChanged = false;
    Switch_vWritePin(psHandle, psHandle->sPresentable.u8State);
}

static inline void Switch_vHandleImpulse(SWITCH_T *psHandle, uint32_t u32ms)
{
    if (psHandle->u32ImpulseStarted == 0U)
    {
        psHandle->u32ImpulseStarted = u32ms;
        Switch_vWritePin(psHandle, (uint8_t) true);
    }
    else if (PinCfg_u32GetElapsedTime(psHandle->u32ImpulseStarted, u32ms) >= psHandle->u32ImpulseDuration)
    {
        psHandle->u32ImpulseStarted = 0U;
        psHandle->sPresentable.bStateChanged = false;
        Switch_vWritePin(psHandle, (uint8_t) false);
    }
}

static inline void Switch_vHandleTimed(SWITCH_T *psHandle, uint32_t u32ms)
{
    if (psHandle->u32ImpulseStarted != 0U &&
        PinCfg_u32GetElapsedTime(psHandle->u32ImpulseStarted, u32ms) > psHandle->u32ImpulseDuration)
    {
        psHandle->u32ImpulseStarted = 0U;
        psHandle->u32ImpulseDuration = 0U;
        Switch_vWritePin(psHandle, (uint8_t) false);
        Presentable_vSetState((PRESENTABLE_T *)psHandle, (int32_t) false, true);
        psHandle->sPresentable.bStateChanged = false;
    }
}

void Switch_vLoopClassic(LOOPABLE_T *psLoopableHandle, uint32_t u32ms)
{
    (void)u32ms; // unused parameter

    SWITCH_T *psHandle = container_of(psLoopableHandle, SWITCH_T, sLoopable);

    if (psHandle->sPresentable.bStateChanged != true)
        return;

    Switch_vHandleClassic(psHandle);
}

void Switch_vLoopClassicFeedback(LOOPABLE_T *psLoopableHandle, uint32_t u32ms)
{
    SWITCH_T *psHandle = container_of(psLoopableHandle, SWITCH_T, sLoopable);

    Switch_vCheckFbPin(psHandle, u32ms);

    if (psHandle->sPresentable.bStateChanged != true)
        return;

    Switch_vHandleClassic(psHandle);
}

void Switch_vLoopImpulse(LOOPABLE_T *psLoopableHandle, uint32_t u32ms)
{
    SWITCH_T *psHandle = container_of(psLoopableHandle, SWITCH_T, sLoopable);

    if (psHandle->sPresentable.bStateChanged != true)
        return;

    Switch_vHandleImpulse(psHandle, u32ms);
}

void Switch_vLoopImpulseFeedback(LOOPABLE_T *psLoopableHandle, uint32_t u32ms)
{
    SWITCH_T *psHandle = container_of(psLoopableHandle, SWITCH_T, sLoopable);

    Switch_vCheckFbPin(psHandle, u32ms);

    if (psHandle->sPresentable.bStateChanged != true)
        return;

    Switch_vHandleImpulse(psHandle, u32ms);
}

void Switch_vLoopTimed(LOOPABLE_T *psLoopableHandle, uint32_t u32ms)
{
    SWITCH_T *psHandle = container_of(psLoopableHandle, SWITCH_T, sLoopable);

    // Always check timeout - timed switch must turn off even without state change
    // Turn-on is handled by Switch_vEventHandle, not here
    if (psHandle->u32ImpulseDuration > 0U)
    {
        Switch_vHandleTimed(psHandle, u32ms);
    }
}

void Switch_vLoopTimedFeedback(LOOPABLE_T *psLoopableHandle, uint32_t u32ms)
{
    SWITCH_T *psHandle = container_of(psLoopableHandle, SWITCH_T, sLoopable);

    Switch_vCheckFbPin(psHandle, u32ms);

    // Always check timeout - timed switch must turn off even without state change
    // Turn-on is handled by Switch_vEventHandle, not here
    if (psHandle->u32ImpulseDuration > 0U)
    {
        Switch_vHandleTimed(psHandle, u32ms);
    }
}

void Switch_vEventHandle(SWITCH_T *psHandle, uint8_t u8EventType, uint32_t u32Data, uint32_t u32ms)
{
    (void)u32Data; // unused parameter

    if (psHandle == NULL)
        return;

    if (psHandle->eMode != SWITCH_TIMED_E)
        return;

    TRIGGER_EVENTTYPE_T eEventType = (TRIGGER_EVENTTYPE_T)u8EventType;
    if (eEventType == TRIGGER_UP_E)
    {
        if (psHandle->sPresentable.u8State == 0U)
        {
            psHandle->u32ImpulseStarted = u32ms;
            psHandle->u32ImpulseDuration = psHandle->u32TimedAdidtionalDelayMs;
            Switch_vWritePin(psHandle, (uint8_t) true);
            Presentable_vSetState((PRESENTABLE_T *)psHandle, (int32_t) true, true);
        }
        else
        {
            psHandle->u32ImpulseDuration += psHandle->u32TimedAdidtionalDelayMs;
        }
    }
    else if (eEventType == TRIGGER_LONG_E)
    {
        if (psHandle->sPresentable.u8State == 1U)
        {
            psHandle->u32ImpulseStarted = 0U;
            psHandle->u32ImpulseDuration = 0U;
            Switch_vWritePin(psHandle, (uint8_t) false);
            Presentable_vSetState((PRESENTABLE_T *)psHandle, (int32_t) false, true);
            psHandle->sPresentable.bStateChanged = false;
        }
    }
}
