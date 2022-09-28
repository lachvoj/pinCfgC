#ifndef PINSUBSCRIBER_IF_H
#define PINSUBSCRIBER_IF_H

#include "Types.h"

typedef struct PINSUBSCRIBER_IF
{
    void (*eventHandle)(struct PINSUBSCRIBER_IF *psHandle, uint8_t u8EventType, uint32_t u32Data);
} PINSUBSCRIBER_IF;

#endif // PINSUBSCRIBER_IF_H
