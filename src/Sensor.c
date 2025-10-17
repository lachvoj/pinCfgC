#include "Memory.h"
#include "PinCfgUtils.h"
#include "Sensor.h"

static void Sensor_vLoop(LOOPABLE_T *psLoopableHandle, uint32_t u32ms);

SENSOR_RESULT_T Sensor_eInit(
    SENSOR_T *psHandle,
    PINCFG_RESULT_T (*eAddToLoopables)(LOOPABLE_T *psLoopable),
    PINCFG_RESULT_T (*eAddToPresentables)(PRESENTABLE_T *psPresentable),
    uint8_t *pu8PresentablesCount,
    bool bCumulative,
    bool bEnableable,
    STRING_POINT_T *sName,
    mysensors_data_t eVType,
    mysensors_sensor_t eSType,
    void (*vReceive)(PRESENTABLE_T *psHandle, const void *pvMessage),
    ISENSORMEASURE_T *psSensorMeasure,
    uint16_t u16SamplingIntervalMs,
    uint16_t u16ReportIntervalSec,
    float fOffset)
{
    if (psHandle == NULL || sName == NULL || vReceive == NULL || psSensorMeasure == NULL)
        return SENSOR_NULLPTR_ERROR_E;

    // Initialize presentable
    if (Presentable_eInit(&psHandle->sPresentable, sName, *pu8PresentablesCount) != PRESENTABLE_OK_E)
        return SENSOR_SUBINIT_ERROR_E;

    // Setup vtab
    psHandle->psSensorMeasure = psSensorMeasure;
    psHandle->sVtab.eVType = eVType;
    psHandle->sVtab.eSType = eSType;
    psHandle->sVtab.vReceive = vReceive;
    psHandle->sPresentable.psVtab = &psHandle->sVtab;

    // Setup loop function (single function for all modes)
    psHandle->sLoopable.vLoop = Sensor_vLoop;

    // Initialize timing
    psHandle->u16ReportIntervalSec = u16ReportIntervalSec;
    psHandle->u16SamplingIntervalMs = u16SamplingIntervalMs;
    psHandle->u32LastReportMs = 0U;
    psHandle->u32LastSamplingMs = 0U;
    psHandle->fOffset = fOffset;

    // Initialize mode flags
    psHandle->u8Flags = 0U;
    if (bCumulative)
        psHandle->u8Flags |= SENSOR_FLAG_CUMULATIVE;
    if (bEnableable)
        psHandle->u8Flags |= SENSOR_FLAG_ENABLEABLE;
    psHandle->u8Flags |= SENSOR_FLAG_ENABLED;  // Start enabled

    // Initialize cumulative fields if needed
    if (bCumulative)
    {
        psHandle->u32SamplesCount = 0U;
        psHandle->fCumulatedValue = 0.0;
    }

    // Setup enableable if needed
    if (bEnableable)
    {
        psHandle->psEnableable = (ENABLEABLE_T *)Memory_vpAlloc(sizeof(ENABLEABLE_T));
        if (psHandle->psEnableable == NULL)
            return SENSOR_MEMORY_ALLOCATION_ERROR_E;
            
        if (Enableable_eInit(psHandle->psEnableable, sName, *pu8PresentablesCount + 1) != ENABLEABLE_OK_E)
            return SENSOR_SUBINIT_ERROR_E;
            
        if (eAddToPresentables != NULL)
        {
            if (eAddToPresentables((PRESENTABLE_T *)psHandle->psEnableable) != PINCFG_OK_E)
                return SENSOR_SUBINIT_ERROR_E;
            (*pu8PresentablesCount)++;
        }
    }
    else
    {
        psHandle->psEnableable = NULL;
    }

    // Add to arrays
    if (eAddToPresentables != NULL)
    {
        if (eAddToPresentables((PRESENTABLE_T *)psHandle) != PINCFG_OK_E)
            return SENSOR_SUBINIT_ERROR_E;
        (*pu8PresentablesCount)++;
    }

    if (eAddToLoopables != NULL)
    {
        if (eAddToLoopables((LOOPABLE_T *)(&psHandle->sLoopable)) != PINCFG_OK_E)
            return SENSOR_SUBINIT_ERROR_E;
    }

    return SENSOR_OK_E;
}

static void Sensor_vLoop(LOOPABLE_T *psLoopableHandle, uint32_t u32ms)
{
    SENSOR_T *psHandle = (SENSOR_T *)(((uint8_t *)psLoopableHandle) - sizeof(PRESENTABLE_T));

    // Handle enableable state changes
    if (psHandle->u8Flags & SENSOR_FLAG_ENABLEABLE)
    {
        if (psHandle->psEnableable->sPresentable.bStateChanged)
        {
            psHandle->psEnableable->sPresentable.bStateChanged = false;
            
            if (psHandle->psEnableable->sPresentable.u8State)
                psHandle->u8Flags |= SENSOR_FLAG_ENABLED;
            else
                psHandle->u8Flags &= ~SENSOR_FLAG_ENABLED;
            
            // Reset timers on state change
            psHandle->u32LastReportMs = 0U;
            if (psHandle->u8Flags & SENSOR_FLAG_CUMULATIVE)
            {
                psHandle->u32LastSamplingMs = 0U;
                psHandle->fCumulatedValue = 0.0;
                psHandle->u32SamplesCount = 0U;
            }
        }
        
        // Skip measurement if disabled
        if (!(psHandle->u8Flags & SENSOR_FLAG_ENABLED))
            return;
    }

    // Handle cumulative mode
    if (psHandle->u8Flags & SENSOR_FLAG_CUMULATIVE)
    {
        // Check sampling interval (in milliseconds)
        if (PinCfg_u32GetElapsedTime(psHandle->u32LastSamplingMs, u32ms) >= psHandle->u16SamplingIntervalMs)
        {
            psHandle->u32LastSamplingMs = u32ms;
            
            // Take a sample
            float fValue = 0.0f;
            if (psHandle->psSensorMeasure->eMeasure(psHandle->psSensorMeasure, &fValue, psHandle->fOffset, u32ms) == SENSOR_OK_E)
            {
                psHandle->fCumulatedValue += fValue;
                psHandle->u32SamplesCount++;
            }
        }
        
        // Check report interval (convert seconds to milliseconds)
        if (PinCfg_u32GetElapsedTime(psHandle->u32LastReportMs, u32ms) >= ((uint32_t)psHandle->u16ReportIntervalSec * 1000U))
        {
            psHandle->u32LastReportMs = u32ms;
            
            if (psHandle->u32SamplesCount > 0U)
            {
                float fAverage = (float)(psHandle->fCumulatedValue / psHandle->u32SamplesCount);
                Presentable_vSetState((PRESENTABLE_T *)psHandle, (uint8_t)fAverage, true);
                
                // Reset accumulator
                psHandle->fCumulatedValue = 0.0;
                psHandle->u32SamplesCount = 0U;
            }
        }
    }
    // Handle standard mode
    else
    {
        // Check report interval (convert seconds to milliseconds)
        if (PinCfg_u32GetElapsedTime(psHandle->u32LastReportMs, u32ms) >= ((uint32_t)psHandle->u16ReportIntervalSec * 1000U))
        {
            psHandle->u32LastReportMs = u32ms;
            
            float fValue = 0.0f;
            if (psHandle->psSensorMeasure->eMeasure(psHandle->psSensorMeasure, &fValue, psHandle->fOffset, u32ms) == SENSOR_OK_E)
            {
                Presentable_vSetState((PRESENTABLE_T *)psHandle, (uint8_t)fValue, true);
            }
        }
    }
}
