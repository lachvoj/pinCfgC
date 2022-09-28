#ifndef PINIF_H
#define PINIF_H

#include "Types.h"

typedef struct
{
    void (*vPinMode)(uint8_t u8Pin, uint8_t u8Mode);
    uint8_t (*u8ReadPin)(uint8_t u8Pin);
    void (*vWritePin)(uint8_t u8Pin, uint8_t u8Value);
} PIN_IF_T;

#endif // PINIF_H
