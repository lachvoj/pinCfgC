#include "Trigger.h"

#include "Globals.h"
#include "Switch.h"

static void Trigger_vEventHandle(
    IEVENTSUBSCRIBER_T *psBaseHandle,
    uint8_t u8EventType,
    int32_t i32Data,
    uint32_t u32ms);

TRIGGER_RESULT_T Trigger_eInit(
    TRIGGER_T *psHandle,
    TRIGGER_SWITCHACTION_T *pasSwAct,
    uint8_t u8SwActCount,
    TRIGGER_EVENTTYPE_T eEventType,
    int32_t i32EventData)
{
    if (psHandle == NULL || pasSwAct == NULL)
        return TRIGGER_NULLPTR_ERROR_E;

    if (u8SwActCount > PINCFG_TRIGGER_MAX_SWITCHES_D)
        return TRIGGER_MAX_SWITCH_ERROR_E;

    psHandle->sEventSubscriber.psNext = NULL;
    psHandle->sEventSubscriber.vEventHandle = Trigger_vEventHandle;

    // parameters init
    psHandle->pasSwAct = pasSwAct;
    psHandle->u8SwActCount = u8SwActCount;
    psHandle->eEventType = eEventType;
    psHandle->i32EventData = i32EventData;

    return TRIGGER_OK_E;
}

static void Trigger_vEventHandle(IEVENTSUBSCRIBER_T *psBaseHandle, uint8_t u8EventType, int32_t i32Data, uint32_t u32ms)
{
    TRIGGER_T *psHandle = (TRIGGER_T *)psBaseHandle;

    if ((TRIGGER_EVENTTYPE_T)u8EventType == TRIGGER_VALUE_E)
    {
        if (psHandle->eEventType == TRIGGER_HIGHER_E && i32Data <= psHandle->i32EventData)
            return;

        if (psHandle->eEventType == TRIGGER_LOWER_E && i32Data >= psHandle->i32EventData)
            return;

        if (psHandle->eEventType == TRIGGER_EXACT_E && i32Data != psHandle->i32EventData)
            return;
    }
    else if (psHandle->eEventType != TRIGGER_ALL_E && (TRIGGER_EVENTTYPE_T)u8EventType != psHandle->eEventType)
        return;

    if (psHandle->eEventType == TRIGGER_MULTI_E && i32Data != psHandle->i32EventData)
        return;

    for (uint8_t i = 0; i < psHandle->u8SwActCount; i++)
    {
        PRESENTABLE_T *psMyPresentHnd = (PRESENTABLE_T *)psHandle->pasSwAct[i].psSwitchHnd;
        switch (psHandle->pasSwAct[i].eAction)
        {
        case TRIGGER_A_TOGGLE_E: Presentable_vToggle(psMyPresentHnd); break;
        case TRIGGER_A_UP_E: Presentable_vSetState(psMyPresentHnd, (int32_t) true, true); break;
        case TRIGGER_A_DOWN_E: Presentable_vSetState(psMyPresentHnd, (int32_t) false, true); break;
        case TRIGGER_A_FORWARD_E: Switch_vEventHandle(((SWITCH_T *)psMyPresentHnd), u8EventType, i32Data, u32ms); break;
        default: break;
        }
    }
}
