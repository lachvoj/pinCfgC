#ifndef PINCFGIF_H
#define PINCFGIF_H

#include "Types.h"

typedef struct
{
    bool (*bRequest)(const PINCFG_ELEMENT_TYPE_T eType, const uint8_t u8Id);
    bool (*bPresent)(const PINCFG_ELEMENT_TYPE_T eType, const uint8_t u8Id, const char *pcName);
    bool (*bSend)(const PINCFG_ELEMENT_TYPE_T eType, const uint8_t u8Id, const void *pvMessage);
    int8_t (*i8SaveCfg)(const char *pcCfg);
} PINCFG_IF_T;

#endif // PINCFGIF_H
