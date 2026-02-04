#ifndef EVENTSUBSCRIBER_IF_H
#define EVENTSUBSCRIBER_IF_H

#include "ILoopable.h"
#include "Presentable.h"
#include "Types.h"

typedef struct IEVENTSUBSCRIBER_S IEVENTSUBSCRIBER_T;

typedef struct IEVENTPUBLISHER_S
{
    PRESENTABLE_T sPresentable;
    LOOPABLE_T sLoopable;
    IEVENTSUBSCRIBER_T *psFirstSubscriber;
} IEVENTPUBLISHER_T;

typedef enum
{
    EVENTSUBSCRIBER_OK_E,
    EVENTSUBSCRIBER_NULLPTR_ERROR_E,
    EVENTSUBSCRIBER_MAXSUBSCRIBERS_ERROR_E,
    EVENTSUBSCRIBER_ERROR_E
} EVENTSUBSCRIBER_RESULT_T;

typedef struct IEVENTSUBSCRIBER_S
{
    void (*vEventHandle)(IEVENTSUBSCRIBER_T *psHandle, uint8_t u8EventType, uint32_t u32Data, uint32_t u32ms);
    IEVENTSUBSCRIBER_T *psNext;
} IEVENTSUBSCRIBER_T;

EVENTSUBSCRIBER_RESULT_T EventPublisher_eAddSubscriber(IEVENTPUBLISHER_T *psHandle, IEVENTSUBSCRIBER_T *psSubscriber);
void EventPublisher_vSendEvent(IEVENTPUBLISHER_T *psHandle, uint8_t u8EventType, int32_t i32Data, uint32_t u32ms);

#endif // EVENTSUBSCRIBER_IF_H
