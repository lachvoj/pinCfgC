#ifndef LOOPRE_H
#define LOOPRE_H

#include "Types.h"

struct LOOPRE_T;

typedef struct
{
    // loopable
    void (*vLoop)(struct LOOPRE_T *psHandle, uint32_t u32ms);
    // presentable
    void (*vReceive)(struct LOOPRE_T *psHandle, const void *pvMessage);
    void (*vPresent)(struct LOOPRE_T *psHandle);
    void (*vPresentState)(struct LOOPRE_T *psHandle);
} LOOPRE_VTAB_T;

typedef struct LOOPRE_T
{
    LOOPRE_VTAB_T *psVtab;
    struct LOOPRE_T *psNextLoopable;
    struct LOOPRE_T *psNextPresentable;
    char *pcName;
    uint8_t u8Id;
#ifdef MY_CONTROLLER_HA
    bool bStatePresented;
#endif
} LOOPRE_T;

// presentable
uint8_t LooPre_u8GetId(LOOPRE_T *psHandle);
const char *LooPre_pcGetName(LOOPRE_T *psHandle);

#endif // LOOPRE_H
