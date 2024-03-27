#ifndef PINSUBSCRIBER_IF_H
#define PINSUBSCRIBER_IF_H

#include "Types.h"

typedef struct PINSUBSCRIBER_IF_S PINSUBSCRIBER_IF_T;

typedef struct PINSUBSCRIBER_IF_S
{
    void (*vEventHandle)(PINSUBSCRIBER_IF_T *psHandle, uint8_t u8EventType, uint32_t u32Data);
    PINSUBSCRIBER_IF_T *psNext;
} PINSUBSCRIBER_IF_T;

#endif // PINSUBSCRIBER_IF_H
