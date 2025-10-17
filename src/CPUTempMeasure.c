#include "CPUTempMeasure.h"
#include "Globals.h"
#include "MySensorsWrapper.h"

static ISENSORMEASURE_RESULT_T CPUTempMeasure_eMeasure(
    ISENSORMEASURE_T *pSelf, 
    float *pfValue, 
    const float fOffset, 
    uint32_t u32ms);

CPUTEMPMEASURE_RESULT_T CPUTempMeasure_eInit(
    CPUTEMPMEASURE_T *psHandle,
    MEASUREMENT_TYPE_T eType,
    const char *pcName)
{
    if (psHandle == NULL || pcName == NULL)
        return CPUTEMPMEASURE_NULLPTR_ERROR_E;

    // Initialize ISensorMeasure interface
    psHandle->sSensorMeasure.eType = eType;  // Set type in interface
    psHandle->sSensorMeasure.eMeasure = CPUTempMeasure_eMeasure;
    psHandle->sSensorMeasure.eMeasureRaw = NULL;  // CPU temp doesn't support raw byte access
    
    // Store name for lookup
    psHandle->pcName = pcName;

    return CPUTEMPMEASURE_OK_E;
}

static ISENSORMEASURE_RESULT_T CPUTempMeasure_eMeasure(
    ISENSORMEASURE_T *pSelf, 
    float *pfValue, 
    const float fOffset, 
    uint32_t u32ms)
{
    if (pSelf == NULL || pfValue == NULL)
        return ISENSORMEASURE_NULLPTR_ERROR_E;

    // Read raw CPU temperature and apply sensor-provided offset
    *pfValue = i8HwCPUTemperature() + fOffset;

    return ISENSORMEASURE_OK_E;
}
