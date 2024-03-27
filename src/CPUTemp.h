#ifndef CPUTEMP_H
#define CPUTEMP_H

#include "ILoopable.h"
#include "Presentable.h"
#include "Types.h"

typedef enum CPUTEMP_RESULT_E
{
    CPUTEMP_OK_E,
    CPUTEMP_NULLPTR_ERROR_E,
    CPUTEMP_SUBINIT_ERROR_E,
    CPUTEMP_ERROR_E
} CPUTEMP_RESULT_T;

typedef struct CPUTEMP_S
{
    PRESENTABLE_T sPresentable;
    LOOPABLE_T sLoopable;
    uint32_t u32ReportInterval;
    float fOffset;
    uint32_t u32MillisLast;
} CPUTEMP_T;

void CPUTemp_vInitType(PRESENTABLE_VTAB_T *psVtab);

CPUTEMP_RESULT_T CPUTemp_eInit(
    CPUTEMP_T *psHandle,
    STRING_POINT_T *sName,
    uint8_t u8Id,
    uint32_t u32ReportInterval,
    float fOffset);

// loopable IF
void CPUTemp_vLoop(LOOPABLE_T *psLoopableHandle, uint32_t u32ms);

#endif // CPUTEMP_H
