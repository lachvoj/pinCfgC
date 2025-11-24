#ifndef LOOPTIMEMEASURE_H
#define LOOPTIMEMEASURE_H

#ifdef PINCFG_FEATURE_LOOPTIME_MEASUREMENT

#include "ISensorMeasure.h"
#include "PinCfgStr.h"
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
 * Returns raw uint32 time in milliseconds as 4 bytes (big-endian).
 * 
 * Size: 16 bytes (32-bit) / 20 bytes (64-bit)
 */
typedef struct LOOPTIMEMEASURE_S
{
    ISENSORMEASURE_T sSensorMeasure;  // Interface (includes eType and pcName) (12/16 bytes)
    uint32_t u32LastCallTime;         // Timestamp of previous eMeasure() call (4 bytes)
    bool bFirstCall;                  // True until first valid measurement (1 byte, 3 bytes padding)
} LOOPTIMEMEASURE_T;

/**
 * @brief Initialize loop time measurement source
 * 
 * @param psHandle Pointer to LOOPTIMEMEASURE_T structure (must be allocated by caller)
 * @param psName Pointer to STRING_POINT_T containing name (will be copied)
 * @return LOOPTIMEMEASURE_RESULT_T status
 */
LOOPTIMEMEASURE_RESULT_T LoopTimeMeasure_eInit(
    LOOPTIMEMEASURE_T *psHandle,
    STRING_POINT_T *psName);

#endif // PINCFG_FEATURE_LOOPTIME_MEASUREMENT

#endif // LOOPTIMEMEASURE_H
