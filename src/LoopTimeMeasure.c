// Include Globals.h first to get feature flags
#include "Globals.h"

#ifdef PINCFG_FEATURE_LOOPTIME_MEASUREMENT

#include "LoopTimeMeasure.h"
#include "MySensorsWrapper.h"
#include "PinCfgUtils.h"
#include "SensorMeasure.h"

// Forward declaration of measurement function
static ISENSORMEASURE_RESULT_T LoopTimeMeasure_eMeasure(
    ISENSORMEASURE_T *pSelf,
    uint8_t *pu8Buffer,
    uint8_t *pu8Size,
    uint32_t u32ms);

LOOPTIMEMEASURE_RESULT_T LoopTimeMeasure_eInit(LOOPTIMEMEASURE_T *psHandle, STRING_POINT_T *psName)
{
    if (psHandle == NULL || psName == NULL)
        return LOOPTIMEMEASURE_NULLPTR_ERROR_E;

    // Initialize base interface (allocates and copies name)
    if (SensorMeasure_eInit(&psHandle->sSensorMeasure, psName, MEASUREMENT_TYPE_LOOPTIME_E) != SENSORMEASURE_OK_E)
        return LOOPTIMEMEASURE_ERROR_E;

    // Set measurement function pointer
    psHandle->sSensorMeasure.eMeasure = LoopTimeMeasure_eMeasure;

    // Initialize measurement-specific state
    psHandle->u32LastCallTime = 0U;
    psHandle->bFirstCall = true;

    return LOOPTIMEMEASURE_OK_E;
}

static ISENSORMEASURE_RESULT_T LoopTimeMeasure_eMeasure(
    ISENSORMEASURE_T *pSelf,
    uint8_t *pu8Buffer,
    uint8_t *pu8Size,
    uint32_t u32ms)
{
    if (pSelf == NULL || pu8Buffer == NULL || pu8Size == NULL)
        return ISENSORMEASURE_NULLPTR_ERROR_E;

    if (*pu8Size < 4)
        return ISENSORMEASURE_ERROR_E; // Buffer too small

    LOOPTIMEMEASURE_T *psHandle = (LOOPTIMEMEASURE_T *)pSelf;

    // Get current timestamp in microseconds (ignore u32ms parameter for high-resolution measurement)
    uint32_t u32CurrentMicros = u32Micros();

    // First call - just store timestamp, no measurement yet
    if (psHandle->bFirstCall)
    {
        psHandle->u32LastCallTime = u32CurrentMicros;
        psHandle->bFirstCall = false;
        return ISENSORMEASURE_ERROR_E; // Skip this sample (no delta yet)
    }

    // Calculate time since last call = loop execution time (in microseconds)
    uint32_t u32LoopDuration = PinCfg_u32GetElapsedTime(psHandle->u32LastCallTime, u32CurrentMicros);

    // Return loop duration in microseconds in big-endian format (4 bytes)
    psHandle->u32LastCallTime = u32CurrentMicros;

    // Return loop duration in big-endian format (4 bytes)
    pu8Buffer[0] = (uint8_t)(u32LoopDuration >> 24);
    pu8Buffer[1] = (uint8_t)(u32LoopDuration >> 16);
    pu8Buffer[2] = (uint8_t)(u32LoopDuration >> 8);
    pu8Buffer[3] = (uint8_t)(u32LoopDuration & 0xFF);
    *pu8Size = 4;

    return ISENSORMEASURE_OK_E;
}

#endif // PINCFG_FEATURE_LOOPTIME_MEASUREMENT
