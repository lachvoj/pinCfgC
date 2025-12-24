#ifndef SENSORMEASURE_H
#define SENSORMEASURE_H

#include "ISensorMeasure.h"
#include "PinCfgStr.h"
#include "Types.h"

typedef enum
{
    SENSORMEASURE_OK_E,
    SENSORMEASURE_ALLOCATION_ERROR_E,
    SENSORMEASURE_NULLPTR_ERROR_E,
    SENSORMEASURE_ERROR_E
} SENSORMEASURE_RESULT_T;

SENSORMEASURE_RESULT_T SensorMeasure_eInitReuseName(
    ISENSORMEASURE_T *psHandle,
    const char *pcName,
    MEASUREMENT_TYPE_T eType);

SENSORMEASURE_RESULT_T SensorMeasure_eInit(
    ISENSORMEASURE_T *psHandle,
    STRING_POINT_T *psName,
    MEASUREMENT_TYPE_T eType);

const char *SensorMeasure_pcGetName(ISENSORMEASURE_T *psHandle);

#endif // SENSORMEASURE_H
