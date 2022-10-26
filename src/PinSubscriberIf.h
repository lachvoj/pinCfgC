#ifndef PINSUBSCRIBER_IF_H
#define PINSUBSCRIBER_IF_H

#include "Types.h"

typedef struct PINSUBSCRIBER_IF
{
    PINCFG_ELEMENT_TYPE_T ePinCfgType;
    struct PINSUBSCRIBER_IF *psNext;
} PINSUBSCRIBER_IF_T;

void PinSubscriberIf_vEventHandle(PINSUBSCRIBER_IF_T *psHandle, uint8_t u8EventType, uint32_t u32Data);

#endif // PINSUBSCRIBER_IF_H
