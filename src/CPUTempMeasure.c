#include "CPUTempMeasure.h"

#include "Globals.h"
#include "MySensorsWrapper.h"
#include "SensorMeasure.h"

static ISENSORMEASURE_RESULT_T CPUTempMeasure_eMeasure(
    ISENSORMEASURE_T *pSelf,
    uint8_t *pu8Buffer,
    uint8_t *pu8Size,
    uint32_t u32ms);

CPUTEMPMEASURE_RESULT_T CPUTempMeasure_eInit(CPUTEMPMEASURE_T *psHandle, STRING_POINT_T *psName)
{
    if (psHandle == NULL || psName == NULL)
        return CPUTEMPMEASURE_NULLPTR_ERROR_E;

    // Initialize base interface (allocates and copies name)
    if (SensorMeasure_eInit(&psHandle->sSensorMeasure, psName, MEASUREMENT_TYPE_CPUTEMP_E) != SENSORMEASURE_OK_E)
        return CPUTEMPMEASURE_ERROR_E;

    // Set measurement function pointer
    psHandle->sSensorMeasure.eMeasure = CPUTempMeasure_eMeasure;

    return CPUTEMPMEASURE_OK_E;
}

static ISENSORMEASURE_RESULT_T CPUTempMeasure_eMeasure(
    ISENSORMEASURE_T *pSelf,
    uint8_t *pu8Buffer,
    uint8_t *pu8Size,
    uint32_t u32ms)
{
    if (pSelf == NULL || pu8Buffer == NULL || pu8Size == NULL)
        return ISENSORMEASURE_NULLPTR_ERROR_E;

    if (*pu8Size < 1)
        return ISENSORMEASURE_ERROR_E; // Buffer too small

    (void)u32ms; // Not used

    // Read raw CPU temperature (int8: -128 to +127°C)
    int8_t i8RawTemp = i8HwCPUTemperature();

    // Return as single byte (signed value)
    pu8Buffer[0] = (uint8_t)i8RawTemp;
    *pu8Size = 1;

    return ISENSORMEASURE_OK_E;
}
