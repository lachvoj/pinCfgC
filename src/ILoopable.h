#ifndef ILOOPABLE_H
#define ILOOPABLE_H

#include "Types.h"

/* comented out due to optimalisation (one pointer) */
// typedef struct
// {
//     void (*vLoop)(struct LOOPABLE_T *psHandle, uint32_t u32ms);
// } LOOPABLE_VTAB_T;
typedef struct LOOPABLE_S LOOPABLE_T;

typedef struct LOOPABLE_S
{
    // LOOPABLE_VTAB_T *psVtab;
    void (*vLoop)(LOOPABLE_T *psHandle, uint32_t u32ms);
} LOOPABLE_T;

#endif // ILOOPABLE_H