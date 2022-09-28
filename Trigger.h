#ifndef TRIGGER_H
#define TRIGGER_H

#include "PinSubscriberIf.h"
#include "Switch.h"
#include "Types.h"

#ifndef PINCFG_TRIGGER_MAX_SWITCHES_D
#define PINCFG_TRIGGER_MAX_SWITCHES_D 5
#endif
#if (PINCFG_TRIGGER_MAX_SWITCHES_D > 255)
#error PINCFG_TRIGGER_MAX_SWITCHES_D is more then 255!
#endif

typedef enum
{
    TRIGGER_OK_E,
    TRIGGER_NULLPTR_ERROR_E,
    TRIGGER_MAX_SWITCH_ERROR_E,
    TRIGGER_ERROR_E
} TRIGGER_RESULT_T;

typedef enum
{
    TRIGGER_DOWN_E,
    TRIGGER_UP_E,
    TRIGGER_LONG
} TRIGGER_EVENTTYPE_T;

typedef enum
{
    TRIGGER_A_TOGGLE_E,
    TRIGGER_A_UP_E,
    TRIGGER_A_DOWN_E
} TRIGGER_ACTION_T;

typedef struct
{
    SWITCH_HANDLE_T *psSwitchHnd;
    TRIGGER_ACTION_T eAction;
} TRIGGER_SWITCHACTION_T;

typedef struct
{
    PINSUBSCRIBER_IF sPinSubscriber;
    TRIGGER_SWITCHACTION_T *pasSwAct;
    uint8_t u8SwActCount;
    TRIGGER_EVENTTYPE_T eEventType;
    uint8_t u8EventCount;
} TRIGGER_HANDLE_T;

TRIGGER_RESULT_T Trigger_eInit(
    TRIGGER_HANDLE_T *psHandle,
    TRIGGER_SWITCHACTION_T *pasSwAct,
    uint8_t u8SwActCount,
    TRIGGER_EVENTTYPE_T eEventType,
    uint8_t u8EventCount);

#endif // TRIGGER_H