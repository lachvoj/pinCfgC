#include "Globals.h"
#include "MySensorsWrapper.h"
#include "Switch.h"

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

void Switch_vSetTimedActionAdditionMs(uint32_t u32TimedActionAdditionMs)
{
    psGlobals->u32SwitchTimedActionAdditionMs = u32TimedActionAdditionMs;
}

void Switch_vInitType(PRESENTABLE_VTAB_T *psVtab)
{
    psVtab->eVType = V_STATUS;
    psVtab->eSType = S_BINARY;
    psVtab->vReceive = Presentable_vRcvMessage;

    Switch_SetImpulseDurationMs(PINCFG_SWITCH_IMPULSE_DURATIN_MS_D);
    Switch_SetFbOnDelayMs(PINCFG_SWITCH_FB_ON_DELAY_MS_D);
    Switch_SetFbOffDelayMs(PINCFG_SWITCH_FB_OFF_DELAY_MS_D);
    Switch_vSetTimedActionAdditionMs(PINCFG_TIMED_ACTION_ADDITION_MS_D);
}

SWITCH_RESULT_T Switch_eInit(
    SWITCH_T *psHandle,
    STRING_POINT_T *sName,
    uint8_t u8Id,
    SWITCH_MODE_T eMode,
    uint8_t u8OutPin,
    uint8_t u8FbPin)
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
    psHandle->sLoopable.vLoop = Switch_vLoop;

    psHandle->eMode = eMode;
    psHandle->u8OutPin = u8OutPin;
    psHandle->u8FbPin = u8FbPin;
    psHandle->u32ImpulseStarted = 0U;
    psHandle->u32ImpulseDuration = psGlobals->u32SwitchImpulseDurationMs;
    psHandle->u32FbReadStarted = 0U;

    if (u8FbPin > 0)
    {
        pinMode(u8FbPin, INPUT_PULLUP);
        digitalWrite(u8FbPin, HIGH); // enabling pullup
    }
    pinMode(u8OutPin, OUTPUT);
    digitalWrite(u8OutPin, LOW);

    return SWITCH_OK_E;
}

void Switch_vLoop(LOOPABLE_T *psLoopableHandle, uint32_t u32ms)
{
    SWITCH_T *psHandle = (SWITCH_T *)(((uint8_t *)psLoopableHandle) - sizeof(PRESENTABLE_T));

    if (psHandle->u8FbPin != 0)
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
                 (u32ms - psHandle->u32FbReadStarted) > psGlobals->u32SwitchFbOnDelayMs) ||
                (psHandle->sPresentable.u8State == 0 &&
                 (u32ms - psHandle->u32FbReadStarted) > psGlobals->u32SwitchFbOffDelayMs))
            {
                psHandle->u32FbReadStarted = 0U;
                psHandle->sPresentable.u8State = u8ActualPinState;
                psHandle->sPresentable.bStateChanged = false;
                Presentable_vSendState((PRESENTABLE_T *)psHandle);
            }
        }
        else if (psHandle->u32FbReadStarted != 0)
            psHandle->u32FbReadStarted = 0U;
    }

    if (psHandle->sPresentable.bStateChanged != true)
        return;

    if (psHandle->eMode == SWITCH_CLASSIC_E)
    {
        psHandle->sPresentable.bStateChanged = false;
        Switch_vWritePin(psHandle, psHandle->sPresentable.u8State);
    }
    else if (psHandle->eMode == SWITCH_IMPULSE_E)
    {
        if (psHandle->u32ImpulseStarted == 0U)
        {
            psHandle->u32ImpulseStarted = u32ms;
            Switch_vWritePin(psHandle, (uint8_t) true);
        }
        else if ((u32ms - psHandle->u32ImpulseStarted) >= psHandle->u32ImpulseDuration)
        {
            psHandle->u32ImpulseStarted = 0U;
            psHandle->sPresentable.bStateChanged = false;
            Switch_vWritePin(psHandle, (uint8_t) false);
        }
    }
}

void Switch_vTimedAction(SWITCH_T *psHandle)
{
    psHandle->eMode = SWITCH_IMPULSE_E;
    if (psHandle->u32ImpulseStarted == 0)
    {
        psHandle->u32ImpulseDuration = psGlobals->u32SwitchTimedActionAdditionMs;
        Presentable_vSetState((PRESENTABLE_T *)psHandle, 1, true);
    }
    else
        psHandle->u32ImpulseDuration += psGlobals->u32SwitchTimedActionAdditionMs;
}
