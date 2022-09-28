#ifndef LOOPREIF_H
#define LOOPREIF_H

#include "Types.h"

typedef struct LOOPRE_IF_T
{
    PINCFG_ELEMENT_TYPE_T ePinCfgType;
} LOOPRE_IF_T;

// loopable
void loop(struct LOOPRE_IF_T *psHandle, uint64_t u64ms);
// presentable
uint8_t getId(struct LOOPRE_IF_T *psHandle);
const char *getName(struct LOOPRE_IF_T *psHandle);
void rcvMessage(struct LOOPRE_IF_T *psHandle, const void *pvMessage);
void present(struct LOOPRE_IF_T *psHandle);
void presentState(struct LOOPRE_IF_T *psHandle);

#endif // LOOPREIF_H
