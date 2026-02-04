#ifndef TRIGGER_H
#define TRIGGER_H

#include "Event.h"
#include "Switch.h"
#include "Types.h"

typedef enum TRIGGER_RESULT_E
{
    TRIGGER_OK_E,
    TRIGGER_NULLPTR_ERROR_E,
    TRIGGER_MAX_SWITCH_ERROR_E,
    TRIGGER_ERROR_E
} TRIGGER_RESULT_T;

typedef enum TRIGGER_EVENTTYPE_E
{
    TRIGGER_DOWN_E = 0,
    TRIGGER_UP_E,
    TRIGGER_LONG_E,
    TRIGGER_MULTI_E,
    TRIGGER_VALUE_E,
    TRIGGER_HIGHER_E,
    TRIGGER_LOWER_E,
    TRIGGER_EXACT_E,
    TRIGGER_ALL_E
} TRIGGER_EVENTTYPE_T;

typedef enum TRIGGER_ACTION_E
{
    TRIGGER_A_TOGGLE_E = 0,
    TRIGGER_A_UP_E,
    TRIGGER_A_DOWN_E,
    TRIGGER_A_FORWARD_E
} TRIGGER_ACTION_T;

typedef struct TRIGGER_SWITCHACTION_S
{
    SWITCH_T *psSwitchHnd;
    TRIGGER_ACTION_T eAction;
} TRIGGER_SWITCHACTION_T;

typedef struct TRIGGER_S
{
    IEVENTSUBSCRIBER_T sEventSubscriber;
    TRIGGER_SWITCHACTION_T *pasSwAct;
    uint8_t u8SwActCount;
    TRIGGER_EVENTTYPE_T eEventType;
    int32_t i32EventData;
} TRIGGER_T;

TRIGGER_RESULT_T Trigger_eInit(
    TRIGGER_T *psHandle,
    TRIGGER_SWITCHACTION_T *pasSwAct,
    uint8_t u8SwActCount,
    TRIGGER_EVENTTYPE_T eEventType,
    int32_t i32EventData);

#endif // TRIGGER_H