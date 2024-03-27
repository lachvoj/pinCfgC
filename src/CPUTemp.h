#ifndef CPUTEMP_H
#define CPUTEMP_H

#include "LooPre.h"
#include "MySensorsPresent.h"
#include "Types.h"

typedef enum
{
    CPUTEMP_OK_E,
    CPUTEMP_NULLPTR_ERROR_E,
    CPUTEMP_SUBINIT_ERROR_E,
    CPUTEMP_ERROR_E
} CPUTEMP_RESULT_T;

typedef struct
{
    MYSENSORSPRESENT_HANDLE_T sMySenPresent;
    uint32_t u32ReportInterval;
    float fOffset;
    uint32_t u32MillisLast;
} CPUTEMP_HANDLE_T;

CPUTEMP_RESULT_T CPUTemp_eInit(
    CPUTEMP_HANDLE_T *psHandle,
    STRING_POINT_T *sName,
    uint8_t u8Id,
    uint32_t u32ReportInterval,
    float fOffset);

// loopable IF
void CPUTemp_vLoop(LOOPRE_T *psBaseHandle, uint32_t u32ms);

#endif // CPUTEMP_H
