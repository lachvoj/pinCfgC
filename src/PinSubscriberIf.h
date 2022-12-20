#ifndef PINSUBSCRIBER_IF_H
#define PINSUBSCRIBER_IF_H

#include "Types.h"

struct PINSUBSCRIBER_IF;

typedef struct
{
    void (*vEventHandle)(struct PINSUBSCRIBER_IF *psHandle, uint8_t u8EventType, uint32_t u32Data);
} PINSUBSCRIBER_VTAB_T;

typedef struct PINSUBSCRIBER_IF
{
    PINSUBSCRIBER_VTAB_T *psVtab;
    struct PINSUBSCRIBER_IF *psNext;
} PINSUBSCRIBER_IF_T;

#endif // PINSUBSCRIBER_IF_H
