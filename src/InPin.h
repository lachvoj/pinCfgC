#ifndef INPIN_H
#define INPIN_H

#include "LooPreIf.h"
#include "MySensorsPresent.h"
#include "PinSubscriberIf.h"
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
    INPIN_MAXSUBSCRIBERS_ERROR_E,
    INPIN_SUBINIT_ERROR_E,
    INPIN_ERROR_E
} INPIN_RESULT_T;

typedef struct
{
    MYSENSORSPRESENT_HANDLE_T sMySenPresent;
    PINSUBSCRIBER_IF_T *psFirstSubscriber;
    uint32_t u32TimerDebounceStarted;
    uint32_t u32timerMultiStarted;
    uint8_t u8InPin;
    uint8_t u8PressCount;
    PIN_STATE_T ePinState;
    bool bLastPinState;
} INPIN_HANDLE_T;

void InPin_SetDebounceMs(uint32_t debounce);
void InPin_SetMulticlickMaxDelayMs(uint32_t multikMaxDelay);

INPIN_RESULT_T InPin_eInit(
    INPIN_HANDLE_T *psHandle,
    STRING_POINT_T *sName,
    uint8_t u8Id,
    uint8_t u8InPin);
INPIN_RESULT_T InPin_eAddSubscriber(INPIN_HANDLE_T *psHandle, PINSUBSCRIBER_IF_T *psSubscriber);
void InPin_vSendEvent(INPIN_HANDLE_T *psHandle, uint8_t u8EventType, uint32_t u32Data);

// loopable IF
void InPin_vLoop(INPIN_HANDLE_T *psHandle, uint32_t u32ms);

#endif // INPIN_H
