#ifndef INPIN_H
#define INPIN_H

#include "Event.h"
#include "ILoopable.h"
#include "Presentable.h"
#include "Types.h"

typedef enum
{
    INPIN_DOWN_E = 0,
    INPIN_UP_E,
    INPIN_LONG_E,
    INPIN_MULTI_E,
    INPIN_DEBOUNCEDOWN_E,
    INPIN_DEBOUNCEUP_E
} PIN_STATE_T;

typedef enum
{
    INPIN_OK_E,
    INPIN_NULLPTR_ERROR_E,
    INPIN_SUBINIT_ERROR_E,
    INPIN_ERROR_E
} INPIN_RESULT_T;

typedef struct
{
    PRESENTABLE_T sPresentable;
    LOOPABLE_T sLoopable;
    IEVENTSUBSCRIBER_T *psFirstSubscriber;
    uint32_t u32TimerDebounceStarted;
    uint32_t u32TimerMultiStarted;
    uint8_t u8InPin;
    uint8_t u8PressCount;
    PIN_STATE_T ePinState;
    bool bLastPinState;
} INPIN_T;

void InPin_SetDebounceMs(uint32_t debounce);
void InPin_SetMulticlickMaxDelayMs(uint32_t multikMaxDelay);
void InPin_vInitType(PRESENTABLE_VTAB_T *psVtab);

INPIN_RESULT_T InPin_eInit(INPIN_T *psHandle, STRING_POINT_T *sName, uint8_t u8Id, uint8_t u8InPin);

// presentable IF
void InPin_vRcvMessage(PRESENTABLE_T *psBaseHandle, const MyMessage *pcMsg);

#endif // INPIN_H
