#ifndef ISENSORMEASURE_H
#define ISENSORMEASURE_H

#include <stdint.h>

/**
 * @brief Measurement source types
 * Used in CSV as numeric value for compact format
 */
typedef enum MEASUREMENT_TYPE_E
{
    MEASUREMENT_TYPE_CPUTEMP_E = 0,  // CPU temperature sensor
    MEASUREMENT_TYPE_ANALOG_E = 1,   // Analog input (reserved for Phase 3)
    MEASUREMENT_TYPE_DIGITAL_E = 2,  // Digital input (reserved for Phase 3)
    MEASUREMENT_TYPE_I2C_E = 3,      // I2C sensor (reserved for Phase 3)
    MEASUREMENT_TYPE_CALCULATED_E = 4, // Calculated/formula (reserved for Phase 3)
    MEASUREMENT_TYPE_COUNT_E         // Number of types (for validation)
} MEASUREMENT_TYPE_T;

typedef enum ISENSORMEASURE_RESULT_E
{
    ISENSORMEASURE_OK_E = 0,
    ISENSORMEASURE_NULLPTR_ERROR_E,
    ISENSORMEASURE_ERROR_E
} ISENSORMEASURE_RESULT_T;

typedef struct ISENSORMEASURE_S ISENSORMEASURE_T;
typedef struct ISENSORMEASURE_S
{
    ISENSORMEASURE_RESULT_T (*eMeasure)(ISENSORMEASURE_T *pSelf, float *pfValue, const float fOffset, uint32_t u32ms);
    MEASUREMENT_TYPE_T eType;  // Measurement type (part of interface contract)
} ISENSORMEASURE_T;

#endif // ISENSORMEASURE_H