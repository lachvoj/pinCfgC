#ifndef PINCFGIF_H
#define PINCFGIF_H

#include "Types.h"

bool bRequest(const PINCFG_ELEMENT_TYPE_T eType, const uint8_t u8Id);
bool bPresent(const PINCFG_ELEMENT_TYPE_T eType, const uint8_t u8Id, const char *pcName);
bool bSend(const PINCFG_ELEMENT_TYPE_T eType, const uint8_t u8Id, const void *pvMessage);
uint8_t u8SaveCfg(const char *pcCfg);

#endif // PINCFGIF_H
