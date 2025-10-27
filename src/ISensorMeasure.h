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
    MEASUREMENT_TYPE_I2C_E = 3,      // I2C sensor
    MEASUREMENT_TYPE_CALCULATED_E = 4, // Calculated/formula (reserved for Phase 3)
    MEASUREMENT_TYPE_LOOPTIME_E = 5, // Loop execution time (debug measurement, bypasses sampling interval)
    MEASUREMENT_TYPE_SPI_E = 6,      // SPI sensor
    MEASUREMENT_TYPE_COUNT_E         // Number of types (for validation)
} MEASUREMENT_TYPE_T;

typedef enum ISENSORMEASURE_RESULT_E
{
    ISENSORMEASURE_OK_E = 0,
    ISENSORMEASURE_NULLPTR_ERROR_E,
    ISENSORMEASURE_ERROR_E,
    ISENSORMEASURE_PENDING_E  // Operation in progress, call again (non-blocking operations)
} ISENSORMEASURE_RESULT_T;

typedef struct ISENSORMEASURE_S ISENSORMEASURE_T;

/**
 * @brief Sensor measurement interface structure
 * 
 * Base interface for all measurement sources.
 * Similar to PRESENTABLE_T pattern - contains common fields.
 * 
 * Universal raw bytes interface:
 * - All measurements return raw bytes in big-endian format
 * - Sensor layer handles byte extraction and conversion to float
 * - Consistent processing for all measurement types
 * 
 * Size: ~12 bytes (32-bit) / ~16 bytes (64-bit)
 */
typedef struct ISENSORMEASURE_S
{
    ISENSORMEASURE_RESULT_T (*eMeasure)(ISENSORMEASURE_T *pSelf, uint8_t *pu8Buffer, uint8_t *pu8Size, uint32_t u32ms);
    MEASUREMENT_TYPE_T eType;  // Measurement type (part of interface contract)
    const char *pcName;        // Measurement source name for lookup (allocated/copied during init)
} ISENSORMEASURE_T;

#endif // ISENSORMEASURE_H