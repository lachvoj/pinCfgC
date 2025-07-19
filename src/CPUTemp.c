#include "CPUTemp.h"
#include "Globals.h"
#include "InPin.h"
#include "Memory.h"
#include "MySensorsWrapper.h"
#include "PinCfgUtils.h"

static SENSOR_RESULT_T CPUTemp_eMeasure(float *pfValue, const float fOffset);

CPUTEMP_RESULT_T CPUTemp_eInit(
    CPUTEMP_T *psHandle,
    PINCFG_RESULT_T (*eAddToLoopables)(LOOPABLE_T *psLoopable),
    PINCFG_RESULT_T (*eAddToPresentables)(PRESENTABLE_T *psPresentable),
    uint8_t *pu8PresentablesCount,
    SENSOR_MODE_T eMode,
    uint8_t u8Enableable,
    STRING_POINT_T *sName,
    uint32_t u32SamplingInterval,
    uint32_t u32ReportInterval,
    float fOffset)
{
    if (psHandle == NULL || sName == NULL)
        return CPUTEMP_NULLPTR_ERROR_E;

    // Allocate memory for the sensor handle
    size_t szSensorSize = Sensor_szGetTypeSize(eMode, u8Enableable);
    SENSOR_T *psSensorHandle = (SENSOR_T *)Memory_vpAlloc(szSensorSize);
    if (psSensorHandle == NULL)
        return CPUTEMP_MEMORY_ALLOCATION_ERROR_E;

    // sensor init
    if (Sensor_eInit(
            psSensorHandle,
            eAddToLoopables,
            eAddToPresentables,
            pu8PresentablesCount,
            eMode,
            u8Enableable,
            sName,
            V_TEMP,
            S_TEMP,
            InPin_vRcvMessage,
            &(psHandle->sSensorMeasure),
            u32SamplingInterval,
            u32ReportInterval,
            fOffset) != SENSOR_OK_E)
    {
        return CPUTEMP_SUBINIT_ERROR_E;
    }

    return CPUTEMP_OK_E;
}

static SENSOR_RESULT_T CPUTemp_eMeasure(float *pfValue, const float fOffset)
{
    *pfValue = i8HwCPUTemperature() + fOffset;

    return SENSOR_OK_E;
}
