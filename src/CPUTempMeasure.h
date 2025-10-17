#ifndef CPUTEMPMEASURE_H
#define CPUTEMPMEASURE_H

#include "ISensorMeasure.h"
#include "Types.h"

typedef enum CPUTEMPMEASURE_RESULT_E
{
    CPUTEMPMEASURE_OK_E = 0,
    CPUTEMPMEASURE_NULLPTR_ERROR_E,
    CPUTEMPMEASURE_ERROR_E
} CPUTEMPMEASURE_RESULT_T;

typedef struct CPUTEMPMEASURE_S
{
    ISENSORMEASURE_T sSensorMeasure;  // Interface (includes eType)
    const char *pcName;               // Measurement source name for lookup
} CPUTEMPMEASURE_T;

/**
 * @brief Initialize CPU temperature measurement source
 * @param psHandle Pointer to CPUTEMPMEASURE_T structure
 * @param eType Measurement type (should be MEASUREMENT_TYPE_CPUTEMP_E)
 * @param pcName Name of this measurement source (for lookup)
 * @return CPUTEMPMEASURE_RESULT_T status
 */
CPUTEMPMEASURE_RESULT_T CPUTempMeasure_eInit(
    CPUTEMPMEASURE_T *psHandle,
    MEASUREMENT_TYPE_T eType,
    const char *pcName);

#endif // CPUTEMPMEASURE_H
