// Include Globals.h first to get feature flags
#include "Globals.h"

#ifdef FEATURE_LOOPTIME_MEASUREMENT

#include "LoopTimeMeasure.h"
#include "PinCfgUtils.h"

// Forward declaration of measurement function
static ISENSORMEASURE_RESULT_T LoopTimeMeasure_eMeasure(
    ISENSORMEASURE_T *pSelf,
    float *pfValue,
    const float fOffset,
    uint32_t u32ms);

LOOPTIMEMEASURE_RESULT_T LoopTimeMeasure_eInit(
    LOOPTIMEMEASURE_T *psHandle,
    MEASUREMENT_TYPE_T eType,
    const char *pcName)
{
    if (psHandle == NULL || pcName == NULL)
        return LOOPTIMEMEASURE_NULLPTR_ERROR_E;

    // Initialize ISensorMeasure interface
    psHandle->sSensorMeasure.eType = eType;
    psHandle->sSensorMeasure.eMeasure = LoopTimeMeasure_eMeasure;
    psHandle->sSensorMeasure.eMeasureRaw = NULL;  // Loop time doesn't support raw byte access
    
    // Store name for lookup
    psHandle->pcName = pcName;
    
    // Initialize state
    psHandle->u32LastCallTime = 0U;
    psHandle->bFirstCall = true;

    return LOOPTIMEMEASURE_OK_E;
}

static ISENSORMEASURE_RESULT_T LoopTimeMeasure_eMeasure(
    ISENSORMEASURE_T *pSelf,
    float *pfValue,
    const float fOffset,
    uint32_t u32ms)
{
    if (pSelf == NULL || pfValue == NULL)
        return ISENSORMEASURE_NULLPTR_ERROR_E;

    LOOPTIMEMEASURE_T *psHandle = (LOOPTIMEMEASURE_T *)pSelf;
    
    // First call - just store timestamp, no measurement yet
    if (psHandle->bFirstCall)
    {
        psHandle->u32LastCallTime = u32ms;
        psHandle->bFirstCall = false;
        return ISENSORMEASURE_ERROR_E;  // Skip this sample (no delta yet)
    }
    
    // Calculate time since last call = loop execution time
    uint32_t u32LoopDuration = PinCfg_u32GetElapsedTime(psHandle->u32LastCallTime, u32ms);
    
    // Store timestamp for next iteration
    psHandle->u32LastCallTime = u32ms;
    
    // Return loop duration in milliseconds, with offset applied
    *pfValue = (float)u32LoopDuration + fOffset;
    
    return ISENSORMEASURE_OK_E;
}

#endif // FEATURE_LOOPTIME_MEASUREMENT
