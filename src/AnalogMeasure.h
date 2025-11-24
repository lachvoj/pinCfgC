#ifndef ANALOGMEASURE_H
#define ANALOGMEASURE_H

#ifdef PINCFG_FEATURE_ANALOG_MEASUREMENT

#include "ISensorMeasure.h"
#include "PinCfgStr.h"
#include "Types.h"
#include <stdint.h>

/**
 * @brief Analog measurement result codes
 */
typedef enum ANALOGMEASURE_RESULT_E
{
    ANALOGMEASURE_OK_E = 0,
    ANALOGMEASURE_NULLPTR_ERROR_E,
    ANALOGMEASURE_INVALID_PARAM_E,
    ANALOGMEASURE_ERROR_E
} ANALOGMEASURE_RESULT_T;

/**
 * @brief Analog measurement structure (platform-independent)
 * 
 * Reads raw ADC values from analog pins.
 * Platform abstraction handled by AnalogWrapper.
 * Returns raw uint16 ADC value as 2 bytes (big-endian).
 * All processing (averaging, voltage conversion) handled by Sensor layer.
 * 
 * Size: 12 bytes (32-bit) / 16 bytes (64-bit)
 */
typedef struct ANALOGMEASURE_S
{
    ISENSORMEASURE_T sSensorMeasure;  // Interface (includes eType and pcName) (12/16 bytes)
    uint8_t u8Pin;                    // Analog pin number (1 byte)
} ANALOGMEASURE_T;

/**
 * @brief Initialize analog measurement source
 * 
 * @param psHandle Pointer to ANALOGMEASURE_T structure (must be allocated by caller)
 * @param psName Pointer to STRING_POINT_T containing name (will be copied)
 * @param u8Pin Analog pin number
 *              - Arduino: 0-7 for A0-A7 (or direct pin number)
 *              - STM32: Pin number (mapped via AnalogWrapper configuration)
 * @return ANALOGMEASURE_RESULT_T status
 * 
 * @note Returns raw ADC values (0-1023 or 0-4095)
 * @note All processing handled by Sensor layer (averaging, voltage conversion, etc.)
 */
ANALOGMEASURE_RESULT_T AnalogMeasure_eInit(
    ANALOGMEASURE_T *psHandle,
    STRING_POINT_T *psName,
    uint8_t u8Pin
);

#endif // PINCFG_FEATURE_ANALOG_MEASUREMENT

#endif // ANALOGMEASURE_H
