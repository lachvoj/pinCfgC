#include "Globals.h"
#include "Switch.h"
#include "Trigger.h"

TRIGGER_RESULT_T Trigger_eInit(
    TRIGGER_T *psHandle,
    TRIGGER_SWITCHACTION_T *pasSwAct,
    uint8_t u8SwActCount,
    TRIGGER_EVENTTYPE_T eEventType,
    uint8_t u8EventCount)
{
    if (psHandle == NULL || pasSwAct == NULL)
        return TRIGGER_NULLPTR_ERROR_E;

    if (u8SwActCount > PINCFG_TRIGGER_MAX_SWITCHES_D)
        return TRIGGER_MAX_SWITCH_ERROR_E;

    psHandle->sPinSubscriber.vEventHandle = Trigger_vEventHandle;

    // parameters init
    psHandle->pasSwAct = pasSwAct;
    psHandle->u8SwActCount = u8SwActCount;
    psHandle->eEventType = eEventType;
    psHandle->u8EventCount = u8EventCount;

    return TRIGGER_OK_E;
}

void Trigger_vEventHandle(PINSUBSCRIBER_IF_T *psBaseHandle, uint8_t u8EventType, uint32_t u32Data)
{
    TRIGGER_T *psHandle = (TRIGGER_T *)psBaseHandle;

    if ((TRIGGER_EVENTTYPE_T)u8EventType != psHandle->eEventType || u32Data != psHandle->u8EventCount)
        return;

    for (uint8_t i = 0; i < psHandle->u8SwActCount; i++)
    {
        PRESENTABLE_T *psMyPresentHnd = (PRESENTABLE_T *)psHandle->pasSwAct[i].psSwitchHnd;
        switch (psHandle->pasSwAct[i].eAction)
        {
        case TRIGGER_A_TOGGLE_E: Presentable_vToggle(psMyPresentHnd); break;
        case TRIGGER_A_UP_E: Presentable_vSetState(psMyPresentHnd, (uint8_t) true, true); break;
        case TRIGGER_A_DOWN_E: Presentable_vSetState(psMyPresentHnd, (uint8_t) false, true); break;
        case TRIGGER_A_TIMED: Switch_vTimedAction(psHandle->pasSwAct[i].psSwitchHnd); break;
        default: break;
        }
    }
}
