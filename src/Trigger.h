#ifndef TRIGGER_H
#define TRIGGER_H

#include "PinSubscriberIf.h"
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
    PINSUBSCRIBER_IF_T sPinSubscriber;
    TRIGGER_SWITCHACTION_T *pasSwAct;
    uint8_t u8SwActCount;
    TRIGGER_EVENTTYPE_T eEventType;
    uint8_t u8EventCount;
} TRIGGER_T;

TRIGGER_RESULT_T Trigger_eInit(
    TRIGGER_T *psHandle,
    TRIGGER_SWITCHACTION_T *pasSwAct,
    uint8_t u8SwActCount,
    TRIGGER_EVENTTYPE_T eEventType,
    uint8_t u8EventCount);

void Trigger_vEventHandle(PINSUBSCRIBER_IF_T *psBaseHandle, uint8_t u8EventType, uint32_t u32Data, uint32_t u32ms);

#endif // TRIGGER_H