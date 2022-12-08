#ifndef PINCFGIF_H
#define PINCFGIF_H

#include "Types.h"

// !!! implement those !!!

bool bRequestStatus(const uint8_t u8Id);
bool bRequestText(const uint8_t u8Id);

bool bPresentBinary(const uint8_t u8Id, const char *pcName);
bool bPresentInfo(const uint8_t u8Id, const char *pcName);

bool bSendStatus(const uint8_t u8Id, uint8_t u8Status);
bool bSendText(const uint8_t u8Id, const char *pcMessage);

uint8_t u8SaveCfg(const char *pcCfg);

#endif // PINCFGIF_H
