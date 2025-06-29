#ifndef SENSOR_H
#define SENSOR_H

#include "Enableable.h"
#include "ILoopable.h"
#include "MySensorsWrapper.h"
#include "Presentable.h"
#include "Types.h"

typedef enum SENSOR_RESULT_E
{
    SENSOR_OK_E = 0,
    SENSOR_NULLPTR_ERROR_E,
    SENSOR_SUBINIT_ERROR_E,
    SENSOR_MEMORY_ALLOCATION_ERROR_E,
    SENSOR_ERROR_E
} SENSOR_RESULT_T;

typedef enum SENSOR_MODE_E
{
    SENSOR_MODE_STANDARD_E = 0,
    SENSOR_MODE_CUMULATIVE_E
} SENSOR_MODE_T;

typedef struct SENSOR_S SENSOR_T;
typedef struct SENSOR_S
{
    PRESENTABLE_T sPresentable;
    LOOPABLE_T sLoopable;
    ENABLEABLE_T *psEnableable;
    PRESENTABLE_VTAB_T sVtab;
    SENSOR_RESULT_T (*eMeasure)(SENSOR_T *psHandle, float *pfValue);
    uint32_t u32SamplingInterval;
    uint32_t u32LastSamplingMs;
    float fCumulatedValue;
    uint32_t u32SamplesCount;
    uint32_t u32ReportInterval;
    uint32_t u32LastReportMs;
    float fOffset;
    SENSOR_MODE_T eMode;
} SENSOR_T;

SENSOR_RESULT_T Sensor_eInit(
    SENSOR_T *psHandle,
    STRING_POINT_T *sName,
    mysensors_data_t eVType,
    mysensors_sensor_t eSType,
    void (*vReceive)(PRESENTABLE_T *psHandle, const void *pvMessage),
    SENSOR_RESULT_T (*eMeasure)(SENSOR_T *psHandle, float *pfValue),
    uint8_t u8Id,
    uint8_t u8Enableable,
    uint32_t u32SamplingInterval,
    uint32_t u32ReportInterval,
    float fOffset,
    SENSOR_MODE_T eMode);

#endif // SENSOR_H
