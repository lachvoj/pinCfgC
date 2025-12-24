#ifndef ISENSOR_H
#define ISENSOR_H

#include "Presentable.h"

typedef enum SENSOR_RESULT_E
{
    SENSOR_OK_E = 0,
    SENSOR_NULLPTR_ERROR_E,
    SENSOR_SUBINIT_ERROR_E,
    SENSOR_MEMORY_ALLOCATION_ERROR_E,
    SENSOR_INIT_ERROR_E,
    SENSOR_ERROR_E
} SENSOR_RESULT_T;

typedef struct SENSOR_S SENSOR_T;
typedef struct SENSOR_S
{
    PRESENTABLE_T sPresentable;
    LOOPABLE_T sLoopable;
    SENSOR_RESULT_T (*eMeasure)(SENSOR_T *pContext, float *pfValue, const float fOffset, uint32_t u32ms);
} SENSOR_T;

#endif // ISENSOR_H