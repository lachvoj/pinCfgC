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
    bool bCumulative,
    bool bEnableable,
    STRING_POINT_T *sName,
    uint16_t u16SamplingIntervalMs,
    uint16_t u16ReportIntervalSec,
    float fOffset)
{
    if (psHandle == NULL || sName == NULL)
        return CPUTEMP_NULLPTR_ERROR_E;

    // Allocate memory for the sensor handle (now fixed size)
    SENSOR_T *psSensorHandle = (SENSOR_T *)Memory_vpAlloc(sizeof(SENSOR_T));
    if (psSensorHandle == NULL)
        return CPUTEMP_MEMORY_ALLOCATION_ERROR_E;

    // sensor init
    if (Sensor_eInit(
            psSensorHandle,
            eAddToLoopables,
            eAddToPresentables,
            pu8PresentablesCount,
            bCumulative,
            bEnableable,
            sName,
            V_TEMP,
            S_TEMP,
            InPin_vRcvMessage,
            &(psHandle->sSensorMeasure),
            u16SamplingIntervalMs,
            u16ReportIntervalSec,
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
