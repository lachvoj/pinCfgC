#ifndef CPUTEMP_H
#define CPUTEMP_H

#include "Enableable.h"
#include "ILoopable.h"
#include "ISensorMeasure.h"
#include "Presentable.h"
#include "Sensor.h"
#include "Types.h"

typedef enum CPUTEMP_RESULT_E
{
    CPUTEMP_OK_E = 0,
    CPUTEMP_NULLPTR_ERROR_E,
    CPUTEMP_SUBINIT_ERROR_E,
    CPUTEMP_MEMORY_ALLOCATION_ERROR_E,
    CPUTEMP_ERROR_E
} CPUTEMP_RESULT_T;

typedef struct CPUTEMP_S
{
    ISENSORMEASURE_T sSensorMeasure;
    // SENSOR_T *psSensor;
} CPUTEMP_T;

CPUTEMP_RESULT_T CPUTemp_eInit(
    CPUTEMP_T *psHandle,
    PINCFG_RESULT_T (*eAddToLoopables)(LOOPABLE_T *psLoopable),
    PINCFG_RESULT_T (*eAddToPresentables)(PRESENTABLE_T *psPresentable),
    uint8_t *u8PresentablesCount,
    SENSOR_MODE_T eMode,
    uint8_t u8Enableable,
    STRING_POINT_T *sName,
    uint32_t u32SamplingInterval,
    uint32_t u32ReportInterval,
    float fOffset);

#endif // CPUTEMP_H
