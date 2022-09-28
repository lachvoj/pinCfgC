#include "Trigger.h"

static void Trigger_vEventHandle(PINSUBSCRIBER_IF *psHandle, uint8_t u8EventType, uint32_t u32Data);

TRIGGER_RESULT_T Trigger_eInit(
    TRIGGER_HANDLE_T *psHandle,
    TRIGGER_SWITCHACTION_T *pasSwAct,
    uint8_t u8SwActCount,
    TRIGGER_EVENTTYPE_T eEventType,
    uint8_t u8EventCount)
{
    if (psHandle == NULL || pasSwAct == NULL)
        return TRIGGER_NULLPTR_ERROR_E;

    if (u8SwActCount > PINCFG_TRIGGER_MAX_SWITCHES_D)
        return TRIGGER_MAX_SWITCH_ERROR_E;

    // pin subscriberIF
    psHandle->sPinSubscriber.eventHandle = Trigger_vEventHandle;

    // parameters init
    psHandle->pasSwAct = pasSwAct;
    psHandle->u8SwActCount = u8SwActCount;
    psHandle->eEventType = eEventType;
    psHandle->u8EventCount = u8EventCount;

    return TRIGGER_OK_E;
}

static void Trigger_vEventHandle(PINSUBSCRIBER_IF *psHandle, uint8_t u8EventType, uint32_t u32Data)
{
    TRIGGER_HANDLE_T *psTriggerHnd = (TRIGGER_HANDLE_T *)psHandle;

    if ((TRIGGER_EVENTTYPE_T)u8EventType != psTriggerHnd->eEventType || u32Data != psTriggerHnd->u8EventCount)
        return;

    for (uint8_t i = 0; i < psTriggerHnd->u8SwActCount; i++)
    {
        if (psTriggerHnd->pasSwAct[i].eAction == TRIGGER_A_TOGGLE_E)
        {
            MySensorsPresent_vToggle((MYSENSORSPRESENT_HANDLE_T *)(psTriggerHnd->pasSwAct[i].psSwitchHnd));
        }
        else if (psTriggerHnd->pasSwAct[i].eAction == TRIGGER_A_UP_E)
        {
            MySensorsPresent_vSetState(
                (MYSENSORSPRESENT_HANDLE_T *)(psTriggerHnd->pasSwAct[i].psSwitchHnd), (uint8_t) true);
        }
        else if (psTriggerHnd->pasSwAct[i].eAction == TRIGGER_A_DOWN_E)
        {
            MySensorsPresent_vSetState(
                (MYSENSORSPRESENT_HANDLE_T *)(psTriggerHnd->pasSwAct[i].psSwitchHnd), (uint8_t) false);
        }
    }
}
