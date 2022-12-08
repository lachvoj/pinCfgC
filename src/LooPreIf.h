#ifndef LOOPREIF_H
#define LOOPREIF_H

#include "Types.h"

typedef struct LOOPRE_IF_T
{
    PINCFG_ELEMENT_TYPE_T ePinCfgType;
    struct LOOPRE_IF_T *psNextLoopable;
    struct LOOPRE_IF_T *psNextPresentable;
    char *pcName;
    uint8_t u8Id;
#ifdef MY_CONTROLLER_HA
    bool bStatePresented;
#endif
} LOOPRE_IF_T;

// loopable
void LooPreIf_vLoop(LOOPRE_IF_T *psHandle, uint32_t u32ms);
// presentable
uint8_t LooPreIf_u8GetId(LOOPRE_IF_T *psHandle);
const char *LooPreIf_pcGetName(LOOPRE_IF_T *psHandle);
void LooPreIf_vRcvStatusMessage(LOOPRE_IF_T *psHandle, uint8_t u8Status);
void LooPreIf_vRcvTextMessage(LOOPRE_IF_T *psHandle, const char *pcMessage);
void LooPreIf_vPresent(LOOPRE_IF_T *psHandle);
void LooPreIf_vPresentState(LOOPRE_IF_T *psHandle);

#endif // LOOPREIF_H

