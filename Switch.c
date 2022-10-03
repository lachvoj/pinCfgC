#include "ExternalInterfaces.h"
#include "Switch.h"

static inline void Switch_vWritePin(SWITCH_HANDLE_T *psHandle, uint8_t u8Value);

// loopable IF
void Switch_vLoop_withFb(LOOPRE_IF_T *psHandle, uint32_t u32ms);
void Switch_vLoop_noFb(LOOPRE_IF_T *psHandle, uint32_t u32ms);

SWITCH_RESULT_T Switch_eInit(
    SWITCH_HANDLE_T *psHandle,
    STRING_POINT_T *sName,
    uint8_t u8Id,
    bool bPresent,
    uint32_t u32ImpulseDuration,
    SWITCH_MODE_T eMode,
    uint8_t u8OutPin,
    uint8_t u8FbPin)
{
    if (psHandle == NULL)
        return SWITCH_NULLPTR_ERROR_E;

    if (MySensorsPresent_eInit(&psHandle->sMySenPresent, sName, u8Id, PINCFG_SWITCH_E, bPresent) !=
        MYSENSORSPRESENT_OK_E)
    {
        return SWITCH_SUBINIT_ERROR_E;
    }

    psHandle->eMode = eMode;
    psHandle->u8OutPin = u8OutPin;
    psHandle->u8FbPin = u8FbPin;
    psHandle->u32ImpulseStarted = 0U;
    psHandle->u32ImpulseDuration = 300U;
    if (u32ImpulseDuration != 0U)
        psHandle->u32ImpulseDuration = u32ImpulseDuration;

    return SWITCH_OK_E;
}

void Switch_vLoop(SWITCH_HANDLE_T *psHandle, uint32_t u32ms)
{
    MYSENSORSPRESENT_HANDLE_T *psPresentHnd = (MYSENSORSPRESENT_HANDLE_T *)psHandle;

    if (psHandle->u8FbPin > 0)
    {
        uint8_t u8ActualPinState = psPinCfg_PinIf->u8ReadPin(psHandle->u8FbPin);
        if (psPresentHnd->u8State != u8ActualPinState)
        {
            psPresentHnd->u8State = u8ActualPinState;
            psPresentHnd->bStateChanged = true;
        }
    }

    if (psPresentHnd->bStateChanged != true)
        return;

    if (psHandle->eMode == SWITCH_CLASSIC_E)
    {
        psPresentHnd->bStateChanged = false;
        Switch_vWritePin(psHandle, psPresentHnd->u8State);
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
            psPresentHnd->bStateChanged = false;
            Switch_vWritePin(psHandle, (uint8_t) false);
        }
    }
}

// private
static inline void Switch_vWritePin(SWITCH_HANDLE_T *psHandle, uint8_t u8Value)
{
    return psPinCfg_PinIf->vWritePin(psHandle->u8OutPin, u8Value);
}
