#ifndef MYSENOSRSIF_H
#define MYSENOSRSIF_H

#include "Types.h"

typedef struct
{
    bool (*bRequest)(const uint8_t u8Id, const PINCFG_ELEMENT_TYPE_T eType);
    bool (*bPresent)(const uint8_t u8Id, const PINCFG_ELEMENT_TYPE_T eType, const char *pcName);
    bool (*bSend)(const uint8_t u8Id, const PINCFG_ELEMENT_TYPE_T eType, const void *pvMessage);
} MYSENOSRS_IF_T;

#endif // MYSENOSRSIF_H
