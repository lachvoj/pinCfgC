#ifndef SENSOR_H
#define SENSOR_H

#include "Enableable.h"
#include "ILoopable.h"
#include "ISensorMeasure.h"
#include "MySensorsWrapper.h"
#include "PinCfgCsv.h"
#include "Presentable.h"
#include "Types.h"

typedef enum SENSOR_RESULT_E
{
    SENSOR_OK_E = 0,
    SENSOR_NULLPTR_ERROR_E,
    SENSOR_SUBINIT_ERROR_E,
    SENSOR_MEMORY_ALLOCATION_ERROR_E,
    SENSOR_INIT_ERROR_E,
    SENSOR_ERROR_E
} SENSOR_RESULT_T;

typedef enum SENSOR_MODE_E
{
    SENSOR_MODE_STANDARD_E = 0,
    SENSOR_MODE_CUMULATIVE_E
} SENSOR_MODE_T;

typedef struct SENSOR_S
{
    PRESENTABLE_T sPresentable;
    LOOPABLE_T sLoopable;
    ISENSORMEASURE_T *psSensorMeasure;
    PRESENTABLE_VTAB_T sVtab;
    uint32_t u32ReportInterval;
    uint32_t u32LastReportMs;
    float fOffset;
} SENSOR_T;

typedef struct SENSOR_CUMULATIVE_S
{
    SENSOR_T sSensor;
    uint32_t u32SamplingInterval;
    uint32_t u32LastSamplingMs;
    uint32_t u32SamplesCount;
    float fCumulatedValue;
} SENSOR_CUMULATIVE_T;

typedef struct SENSOR_ENABLEABLE_S
{
    SENSOR_T sSensor;
    ENABLEABLE_T sEnableable;
} SENSOR_ENABLEABLE_T;

typedef struct SENSOR_CUMULATIVE_ENABLEABLE_S
{
    SENSOR_CUMULATIVE_T sSensorCumulative;
    ENABLEABLE_T sEnableable;
} SENSOR_CUMULATIVE_ENABLEABLE_T;

SENSOR_RESULT_T Sensor_eInit(
    SENSOR_T *psHandle,
    PINCFG_RESULT_T (*eAddToLoopables)(LOOPABLE_T *psLoopable),
    PINCFG_RESULT_T (*eAddToPresentables)(PRESENTABLE_T *psPresentable),
    uint8_t *pu8PresentablesCount,
    SENSOR_MODE_T eMode,
    uint8_t u8Enableable,
    STRING_POINT_T *sName,
    mysensors_data_t eVType,
    mysensors_sensor_t eSType,
    void (*vReceive)(PRESENTABLE_T *psHandle, const void *pvMessage),
    ISENSORMEASURE_T *psSensorMeasure,
    uint32_t u32SamplingInterval,
    uint32_t u32ReportInterval,
    float fOffset);

size_t Sensor_szGetTypeSize(SENSOR_MODE_T eMode, uint8_t u8Enableable);

#endif // SENSOR_H
