#ifndef LOOPTIMEMEASURE_H
#define LOOPTIMEMEASURE_H

#ifdef FEATURE_LOOPTIME_MEASUREMENT

#include "ISensorMeasure.h"
#include "Types.h"
#include <stdint.h>
#include <stdbool.h>

/**
 * @brief Loop time measurement result codes
 */
typedef enum LOOPTIMEMEASURE_RESULT_E
{
    LOOPTIMEMEASURE_OK_E = 0,
    LOOPTIMEMEASURE_NULLPTR_ERROR_E,
    LOOPTIMEMEASURE_ERROR_E
} LOOPTIMEMEASURE_RESULT_T;

/**
 * @brief Loop execution time measurement structure
 * 
 * Measures time between consecutive eMeasure() calls, which represents
 * the main loop execution time. Designed to be called every loop iteration
 * (bypasses normal Sensor sampling interval).
 * 
 * Size: 20 bytes (32-bit) / 24 bytes (64-bit)
 */
typedef struct LOOPTIMEMEASURE_S
{
    ISENSORMEASURE_T sSensorMeasure;  // Interface (12 bytes)
    const char *pcName;               // Measurement source name for lookup (4/8 bytes)
    uint32_t u32LastCallTime;         // Timestamp of previous eMeasure() call (4 bytes)
    bool bFirstCall;                  // True until first valid measurement (1 byte, 3 bytes padding)
} LOOPTIMEMEASURE_T;

/**
 * @brief Initialize loop time measurement source
 * 
 * @param psHandle Pointer to LOOPTIMEMEASURE_T structure (must be allocated by caller)
 * @param eType Measurement type (should be MEASUREMENT_TYPE_LOOPTIME_E)
 * @param pcName Name of this measurement source (for lookup, can be NULL)
 * @return LOOPTIMEMEASURE_RESULT_T status
 */
LOOPTIMEMEASURE_RESULT_T LoopTimeMeasure_eInit(
    LOOPTIMEMEASURE_T *psHandle,
    MEASUREMENT_TYPE_T eType,
    const char *pcName);

#endif // FEATURE_LOOPTIME_MEASUREMENT

#endif // LOOPTIMEMEASURE_H
