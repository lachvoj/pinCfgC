#include "Event.h"

EVENTSUBSCRIBER_RESULT_T EventPublisher_eAddSubscriber(IEVENTPUBLISHER_T *psHandle, IEVENTSUBSCRIBER_T *psSubscriber)
{
    if (psHandle == NULL || psSubscriber == NULL)
        return EVENTSUBSCRIBER_NULLPTR_ERROR_E;

    if (psHandle->psFirstSubscriber != NULL)
    {
        IEVENTSUBSCRIBER_T *psCurrent = psHandle->psFirstSubscriber;
        while (psCurrent->psNext != NULL)
        {
            psCurrent = psCurrent->psNext;
        }
        psCurrent->psNext = psSubscriber;
    }
    else
    {
        psHandle->psFirstSubscriber = psSubscriber;
    }

    return EVENTSUBSCRIBER_OK_E;
}

void EventPublisher_vSendEvent(IEVENTPUBLISHER_T *psHandle, uint8_t u8EventType, int32_t i32Data, uint32_t u32ms)
{
    IEVENTSUBSCRIBER_T *psCurrent = psHandle->psFirstSubscriber;
    while (psCurrent != NULL)
    {
        psCurrent->vEventHandle(psCurrent, u8EventType, i32Data, u32ms);
        psCurrent = psCurrent->psNext;
    }
}