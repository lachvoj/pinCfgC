#include "CPUTemp.h"
#include "Globals.h"
#include "InPin.h"
#include "Memory.h"
#include "MySensorsWrapper.h"
#include "PinCfgUtils.h"


static SENSOR_RESULT_T CPUTemp_eMeasure(SENSOR_T *psHandle, float *pfValue);

CPUTEMP_RESULT_T CPUTemp_eInit(
    CPUTEMP_T *psHandle,
    STRING_POINT_T *sName,
    uint8_t u8Id,
    uint8_t u8Enableable,
    SENSOR_MODE_T eMode,
    uint32_t u32SamplingInterval,
    uint32_t u32ReportInterval,
    float fOffset)
{
    if (psHandle == NULL || sName == NULL)
        return CPUTEMP_NULLPTR_ERROR_E;

    // sensor init
    if (Sensor_eInit(
            &psHandle->sSensor,
            sName,
            V_TEMP,
            S_TEMP,
            InPin_vRcvMessage,
            CPUTemp_eMeasure,
            u8Id,
            u8Enableable,
            u32SamplingInterval,
            u32ReportInterval,
            fOffset,
            eMode) != SENSOR_OK_E)
    {
        return CPUTEMP_SUBINIT_ERROR_E;
    }

    return CPUTEMP_OK_E;
}

static SENSOR_RESULT_T CPUTemp_eMeasure(SENSOR_T *psHandle, float *pfValue)
{
    *pfValue = i8HwCPUTemperature() + psHandle->fOffset;

    return SENSOR_OK_E;
}
