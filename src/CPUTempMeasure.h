#ifndef CPUTEMPMEASURE_H
#define CPUTEMPMEASURE_H

#include "ISensorMeasure.h"
#include "PinCfgStr.h"
#include "Types.h"

typedef enum CPUTEMPMEASURE_RESULT_E
{
    CPUTEMPMEASURE_OK_E = 0,
    CPUTEMPMEASURE_NULLPTR_ERROR_E,
    CPUTEMPMEASURE_ERROR_E
} CPUTEMPMEASURE_RESULT_T;

typedef struct CPUTEMPMEASURE_S
{
    ISENSORMEASURE_T sSensorMeasure;  // Interface (includes eType and pcName)
} CPUTEMPMEASURE_T;

/**
 * @brief Initialize CPU temperature measurement source
 * 
 * @param psHandle Pointer to CPUTEMPMEASURE_T structure
 * @param psName Pointer to STRING_POINT_T containing name (will be copied)
 * @return CPUTEMPMEASURE_RESULT_T status
 */
CPUTEMPMEASURE_RESULT_T CPUTempMeasure_eInit(
    CPUTEMPMEASURE_T *psHandle,
    STRING_POINT_T *psName);

#endif // CPUTEMPMEASURE_H
