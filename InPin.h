#ifndef INPIN_H
#define INPIN_H

#include "LooPreIf.h"
#include "MySensorsPresent.h"
#include "PinIf.h"
#include "PinSubscriberIf.h"
#include "Types.h"

#ifndef PINCFG_INPIN_MAX_SUBSCRIBERS_D
#define PINCFG_INPIN_MAX_SUBSCRIBERS_D 5
#endif
#if (PINCFG_INPIN_MAX_SUBSCRIBERS_D > 255)
#error PINCFG_INPIN_MAX_SUBSCRIBERS_D is more then 255!
#endif

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
    uint64_t u64TimerDebounceStarted;
    uint64_t u64timerMultiStarted;
    PINSUBSCRIBER_IF *apsSubscribers[PINCFG_INPIN_MAX_SUBSCRIBERS_D];
    uint8_t u8SubscribersCount;
    uint8_t u8InPin;
    uint8_t u8PressCount;
    PIN_STATE_T ePinState;
    bool bLastPinState;
} INPIN_HANDLE_T;

void InPin_SetDebounceMs(uint64_t debounce);
void InPin_SetMulticlickMaxDelayMs(uint64_t multikMaxDelay);

INPIN_RESULT_T InPin_eInit(
    INPIN_HANDLE_T *psHandle,
    STRING_POINT_T *sName,
    uint8_t u8Id,
    bool bPresent,
    uint8_t u8InPin);
INPIN_RESULT_T InPin_eAddSubscriber(INPIN_HANDLE_T *psHandle, PINSUBSCRIBER_IF *psSubscriber);
void InPin_vSendEvent(INPIN_HANDLE_T *psHandle, uint8_t u8EventType, uint32_t u32Data);
bool InPin_bReadPin(INPIN_HANDLE_T *psHandle);

// loopable IF
void InPin_vLoop(INPIN_HANDLE_T *psHandle, uint64_t u64ms);

#endif // INPIN_H
