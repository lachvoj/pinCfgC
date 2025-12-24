#ifndef ANALOGMEASURE_H
#define ANALOGMEASURE_H

#ifdef PINCFG_FEATURE_ANALOG_MEASUREMENT

#include <stdint.h>

#include "ISensorMeasure.h"
#include "PinCfgStr.h"
#include "Types.h"

typedef enum ANALOGMEASURE_RESULT_E
{
    ANALOGMEASURE_OK_E = 0,
    ANALOGMEASURE_NULLPTR_ERROR_E,
    ANALOGMEASURE_INVALID_PARAM_E,
    ANALOGMEASURE_ERROR_E
} ANALOGMEASURE_RESULT_T;
typedef struct ANALOGMEASURE_S
{
    ISENSORMEASURE_T sSensorMeasure; // Interface (includes eType and pcName) (12/16 bytes)
    uint8_t u8Pin;                   // Analog pin number (1 byte)
} ANALOGMEASURE_T;

ANALOGMEASURE_RESULT_T AnalogMeasure_eInit(ANALOGMEASURE_T *psHandle, STRING_POINT_T *psName, uint8_t u8Pin);

#endif // PINCFG_FEATURE_ANALOG_MEASUREMENT
#endif // ANALOGMEASURE_H
