#ifndef PINCFGIF_H
#define PINCFGIF_H

#include "Types.h"

typedef struct
{
    bool (*bRequest)(const uint8_t u8Id, const uint8_t variableType, const uint8_t u8Destination);
    bool (*bPresent)(const uint8_t u8Id, const uint8_t sensorType, const char *pcName);
    bool (*bSend)(const uint8_t u8Id, const uint8_t variableType, const void *pvMessage);
    uint8_t (*u8SaveCfg)(const char *pcCfg);
} PINCFG_IF_T;

#endif // PINCFGIF_H
