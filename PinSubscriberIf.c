#include "PinSubscriberIf.h"
#include "Trigger.h"

void PinSubscriberIf_vEventHandle(PINSUBSCRIBER_IF_T *psHandle, uint8_t u8EventType, uint32_t u32Data)
{
    if (psHandle->ePinCfgType == PINCFG_TRIGGER_E)
    {
        Trigger_vEventHandle((TRIGGER_HANDLE_T *)psHandle, u8EventType, u32Data);
    }
}
