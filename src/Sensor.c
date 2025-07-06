#include "Memory.h"
#include "PinCfgUtils.h"
#include "Sensor.h"

static SENSOR_RESULT_T Sensor_eInitStandard(SENSOR_T *psHandle, uint32_t u32ReportInterval, float fOffset);
static SENSOR_RESULT_T Sensor_eInitCumulative(
    SENSOR_T *psHandle,
    uint32_t u32ReportInterval,
    float fOffset,
    uint32_t u32SamplingInterval);
static SENSOR_RESULT_T Sensor_eInitEnableableStandard(
    SENSOR_T *psHandle,
    STRING_POINT_T *sName,
    uint32_t u32ReportInterval,
    float fOffset);
static SENSOR_RESULT_T Sensor_eInitEnableableCumulative(
    SENSOR_T *psHandle,
    STRING_POINT_T *sName,
    uint32_t u32ReportInterval,
    float fOffset,
    uint32_t u32SamplingInterval);

static void Sensor_vLoopStandard(LOOPABLE_T *psLoopableHandle, uint32_t u32ms);
static void Sensor_vLoopCumulative(LOOPABLE_T *psLoopableHandle, uint32_t u32ms);
static void Sensor_vLoopEnableableStandardDisabled(LOOPABLE_T *psLoopableHandle, uint32_t u32ms);
static void Sensor_vLoopEnableableCumulativeDisabled(LOOPABLE_T *psLoopableHandle, uint32_t u32ms);
static void Sensor_vLoopEnableableStandardEanbled(LOOPABLE_T *psLoopableHandle, uint32_t u32ms);
static void Sensor_vLoopEnableableCumulativeEnabled(LOOPABLE_T *psLoopableHandle, uint32_t u32ms);

SENSOR_RESULT_T Sensor_eInit(
    SENSOR_T *psHandle,
    PINCFG_RESULT_T (*eAddToLoopables)(LOOPABLE_T *psLoopable),
    PINCFG_RESULT_T (*eAddToPresentables)(PRESENTABLE_T *psPresentable),
    uint8_t *pu8PresentablesCount,
    SENSOR_MODE_T eMode,
    uint8_t u8Enableable,
    STRING_POINT_T *sName,
    mysensors_data_t eVType,
    mysensors_sensor_t eSType,
    void (*vReceive)(PRESENTABLE_T *psHandle, const void *pvMessage),
    ISENSORMEASURE_T *psSensorMeasure,
    uint32_t u32SamplingInterval,
    uint32_t u32ReportInterval,
    float fOffset)
{
    if (psHandle == NULL || sName == NULL || vReceive == NULL || psSensorMeasure == NULL)
        return SENSOR_NULLPTR_ERROR_E;

    // presentable init
    if (Presentable_eInit(&psHandle->sPresentable, sName, *pu8PresentablesCount) != PRESENTABLE_OK_E)
        return SENSOR_SUBINIT_ERROR_E;

    // IMeasure init
    psHandle->psSensorMeasure = psSensorMeasure;

    psHandle->sVtab.eVType = eVType;
    psHandle->sVtab.eSType = eSType;
    psHandle->sVtab.vReceive = vReceive;
    psHandle->sPresentable.psVtab = &psHandle->sVtab;

    // Add to presentables
    if (eAddToPresentables != NULL)
    {
        if (eAddToPresentables((PRESENTABLE_T *)psHandle) != PINCFG_OK_E)
            return SENSOR_SUBINIT_ERROR_E;
        (*pu8PresentablesCount)++;
    }

    // Add to loopables
    if (eAddToLoopables != NULL)
    {
        if (eAddToLoopables((LOOPABLE_T *)(&psHandle->sLoopable)) != PINCFG_OK_E)
            return SENSOR_SUBINIT_ERROR_E;
    }

    if (u8Enableable == 0U && eMode == SENSOR_MODE_STANDARD_E)
        return Sensor_eInitStandard(psHandle, u32ReportInterval, fOffset);

    if (u8Enableable == 0U && eMode == SENSOR_MODE_CUMULATIVE_E)
        return Sensor_eInitCumulative(psHandle, u32ReportInterval, fOffset, u32SamplingInterval);

    SENSOR_RESULT_T eResult = SENSOR_OK_E;
    if (u8Enableable != 0U && eMode == SENSOR_MODE_STANDARD_E)
    {
        eResult = Sensor_eInitEnableableStandard(psHandle, sName, u32ReportInterval, fOffset);
        if (eResult != SENSOR_OK_E)
            return eResult;
        
        // Add enableable to presentables
        SENSOR_ENABLEABLE_T *psSensorEnableableHandle = (SENSOR_ENABLEABLE_T *)psHandle;
        if (eAddToPresentables != NULL)
        {
            if (eAddToPresentables((PRESENTABLE_T *)(&psSensorEnableableHandle->sEnableable)) != PINCFG_OK_E)
                return SENSOR_SUBINIT_ERROR_E;
            (*pu8PresentablesCount)++;
        }

        return SENSOR_OK_E;
    }

    if (u8Enableable != 0U && eMode == SENSOR_MODE_CUMULATIVE_E)
    {
        eResult = Sensor_eInitEnableableCumulative(psHandle, sName, u32ReportInterval, fOffset, u32SamplingInterval);
        if (eResult != SENSOR_OK_E)
            return eResult;
        
        // Add enableable to presentables
        SENSOR_CUMULATIVE_ENABLEABLE_T *psSensorCumulativeEnableableHandle = (SENSOR_CUMULATIVE_ENABLEABLE_T *)psHandle;
        if (eAddToPresentables != NULL)
        {
            if (eAddToPresentables((PRESENTABLE_T *)(&psSensorCumulativeEnableableHandle->sEnableable)) != PINCFG_OK_E)
                return SENSOR_SUBINIT_ERROR_E;
            (*pu8PresentablesCount)++;
        }

        return SENSOR_OK_E;
    }

    return SENSOR_INIT_ERROR_E;
}

size_t Sensor_szGetTypeSize(SENSOR_MODE_T eMode, uint8_t u8Enableable)
{
    size_t szSize = sizeof(SENSOR_T);

    if (eMode == SENSOR_MODE_CUMULATIVE_E)
        szSize += sizeof(SENSOR_CUMULATIVE_T) - sizeof(SENSOR_T);

    if (u8Enableable != 0U)
        szSize += sizeof(SENSOR_ENABLEABLE_T) - sizeof(SENSOR_T);

    return szSize;
}

static SENSOR_RESULT_T Sensor_eInitStandard(SENSOR_T *psHandle, uint32_t u32ReportInterval, float fOffset)
{
    // loopable init
    psHandle->sLoopable.vLoop = Sensor_vLoopStandard;

    // values init
    psHandle->u32ReportInterval = u32ReportInterval;
    psHandle->u32LastReportMs = 0U;
    psHandle->fOffset = fOffset;

    return SENSOR_OK_E;
}

static SENSOR_RESULT_T Sensor_eInitCumulative(
    SENSOR_T *psHandle,
    uint32_t u32ReportInterval,
    float fOffset,
    uint32_t u32SamplingInterval)
{
    SENSOR_RESULT_T eResult = Sensor_eInitStandard(psHandle, u32ReportInterval, fOffset);
    if (eResult != SENSOR_OK_E)
        return eResult;

    // loopable init
    psHandle->sLoopable.vLoop = Sensor_vLoopCumulative;

    SENSOR_CUMULATIVE_T *psCumulativeHandle = (SENSOR_CUMULATIVE_T *)psHandle;
    psCumulativeHandle->u32SamplingInterval = u32SamplingInterval;
    psCumulativeHandle->u32LastSamplingMs = 0U;
    psCumulativeHandle->u32SamplesCount = 0U;
    psCumulativeHandle->fCumulatedValue = 0.0f;

    return SENSOR_OK_E;
}

static SENSOR_RESULT_T Sensor_eInitEnableableStandard(
    SENSOR_T *psHandle,
    STRING_POINT_T *sName,
    uint32_t u32ReportInterval,
    float fOffset)
{
    SENSOR_RESULT_T eResult = Sensor_eInitStandard(psHandle, u32ReportInterval, fOffset);
    if (eResult != SENSOR_OK_E)
        return eResult;

    // loopable init
    psHandle->sLoopable.vLoop = Sensor_vLoopEnableableStandardDisabled;

    SENSOR_ENABLEABLE_T *psSensorEnableableHandle = (SENSOR_ENABLEABLE_T *)psHandle;
    // enableable init
    if (Enableable_eInit(&psSensorEnableableHandle->sEnableable, sName, psHandle->sPresentable.u8Id + 1) !=
        ENABLEABLE_OK_E)
        return SENSOR_SUBINIT_ERROR_E;

    return SENSOR_OK_E;
}

static SENSOR_RESULT_T Sensor_eInitEnableableCumulative(
    SENSOR_T *psHandle,
    STRING_POINT_T *sName,
    uint32_t u32ReportInterval,
    float fOffset,
    uint32_t u32SamplingInterval)
{
    SENSOR_RESULT_T eResult = Sensor_eInitCumulative(psHandle, u32ReportInterval, fOffset, u32SamplingInterval);
    if (eResult != SENSOR_OK_E)
        return eResult;

    // loopable init
    psHandle->sLoopable.vLoop = Sensor_vLoopEnableableCumulativeDisabled;

    SENSOR_CUMULATIVE_ENABLEABLE_T *psSensorCumulativeEnableableHandle = (SENSOR_CUMULATIVE_ENABLEABLE_T *)psHandle;
    // enableable init
    if (Enableable_eInit(&psSensorCumulativeEnableableHandle->sEnableable, sName, psHandle->sPresentable.u8Id + 1) !=
        ENABLEABLE_OK_E)
        return SENSOR_SUBINIT_ERROR_E;

    return SENSOR_OK_E;
}

static void Sensor_vHandleStandard(SENSOR_T *psHandle, uint32_t u32ms)
{
    if (PinCfg_u32GetElapsedTime(psHandle->u32LastReportMs, u32ms) < psHandle->u32ReportInterval)
        return;

    psHandle->u32LastReportMs = u32ms;

    float fValue = 0.0f;
    if (psHandle->psSensorMeasure->eMeasure(psHandle->psSensorMeasure, &fValue, psHandle->fOffset, u32ms) !=
        SENSOR_OK_E)
        return;

    Presentable_vSetState((PRESENTABLE_T *)psHandle, (uint8_t)fValue, true);
}

static void Sensor_vHandleCumulative(SENSOR_T *psHandle, uint32_t u32ms)
{
    SENSOR_CUMULATIVE_T *psCumulativeHandle = (SENSOR_CUMULATIVE_T *)psHandle;
    if (PinCfg_u32GetElapsedTime(psCumulativeHandle->u32LastSamplingMs, u32ms) <
        psCumulativeHandle->u32SamplingInterval)
        return;

    psCumulativeHandle->u32LastSamplingMs = u32ms;

    float fValue = 0.0f;
    if (psHandle->psSensorMeasure->eMeasure(psHandle->psSensorMeasure, &fValue, psHandle->fOffset, u32ms) !=
        SENSOR_OK_E)
        return;

    psCumulativeHandle->fCumulatedValue += fValue;
    psCumulativeHandle->u32SamplesCount++;

    if (PinCfg_u32GetElapsedTime(psHandle->u32LastReportMs, u32ms) < psHandle->u32ReportInterval)
        return;

    psHandle->u32LastReportMs = u32ms;
    fValue = psCumulativeHandle->fCumulatedValue / psCumulativeHandle->u32SamplesCount;
    Presentable_vSetState((PRESENTABLE_T *)psHandle, (uint8_t)fValue, true);
    psCumulativeHandle->fCumulatedValue = 0.0f;
    psCumulativeHandle->u32SamplesCount = 0U;
}

static void Sensor_vLoopStandard(LOOPABLE_T *psLoopableHandle, uint32_t u32ms)
{
    SENSOR_T *psHandle = (SENSOR_T *)(((uint8_t *)psLoopableHandle) - sizeof(PRESENTABLE_T));
    Sensor_vHandleStandard(psHandle, u32ms);
}

static void Sensor_vLoopCumulative(LOOPABLE_T *psLoopableHandle, uint32_t u32ms)
{
    SENSOR_T *psHandle = (SENSOR_T *)(((uint8_t *)psLoopableHandle) - sizeof(PRESENTABLE_T));
    Sensor_vHandleCumulative(psHandle, u32ms);
}

static void Sensor_vLoopEnableableStandardDisabled(LOOPABLE_T *psLoopableHandle, uint32_t u32ms)
{
    (void)u32ms; // unused parameter

    SENSOR_T *psHandle = (SENSOR_T *)(((uint8_t *)psLoopableHandle) - sizeof(PRESENTABLE_T));
    SENSOR_ENABLEABLE_T *psEnableableHandle = (SENSOR_ENABLEABLE_T *)psHandle;

    if (!psEnableableHandle->sEnableable.sPresentable.bStateChanged)
        return;

    psEnableableHandle->sEnableable.sPresentable.bStateChanged = false;

    psHandle->u32LastReportMs = 0U;

    if (psEnableableHandle->sEnableable.sPresentable.u8State == (uint8_t) false)
        return;

    psHandle->sLoopable.vLoop = Sensor_vLoopEnableableStandardEanbled;
}

static void Sensor_vLoopEnableableCumulativeDisabled(LOOPABLE_T *psLoopableHandle, uint32_t u32ms)
{
    (void)u32ms; // unused parameter

    SENSOR_T *psHandle = (SENSOR_T *)(((uint8_t *)psLoopableHandle) - sizeof(PRESENTABLE_T));
    SENSOR_CUMULATIVE_T *psCumulativeHandle = (SENSOR_CUMULATIVE_T *)psHandle;
    SENSOR_CUMULATIVE_ENABLEABLE_T *psEnableableCumulativeHandle = (SENSOR_CUMULATIVE_ENABLEABLE_T *)psHandle;

    if (!psEnableableCumulativeHandle->sEnableable.sPresentable.bStateChanged)
        return;

    psEnableableCumulativeHandle->sEnableable.sPresentable.bStateChanged = false;

    psHandle->u32LastReportMs = 0U;
    psCumulativeHandle->u32LastSamplingMs = 0U;
    psCumulativeHandle->fCumulatedValue = 0.0f;
    psCumulativeHandle->u32SamplesCount = 0U;

    if (psEnableableCumulativeHandle->sEnableable.sPresentable.u8State == (uint8_t) false)
        return;

    psHandle->sLoopable.vLoop = Sensor_vLoopEnableableCumulativeEnabled;
}

static void Sensor_vLoopEnableableStandardEanbled(LOOPABLE_T *psLoopableHandle, uint32_t u32ms)
{
    SENSOR_T *psHandle = (SENSOR_T *)(((uint8_t *)psLoopableHandle) - sizeof(PRESENTABLE_T));
    SENSOR_ENABLEABLE_T *psEnableableHandle = (SENSOR_ENABLEABLE_T *)psHandle;

    if (psEnableableHandle->sEnableable.sPresentable.bStateChanged &&
        psEnableableHandle->sEnableable.sPresentable.u8State == (uint8_t) false)
        psHandle->sLoopable.vLoop = Sensor_vLoopEnableableStandardDisabled;
    else
        Sensor_vHandleStandard(psHandle, u32ms);
}

static void Sensor_vLoopEnableableCumulativeEnabled(LOOPABLE_T *psLoopableHandle, uint32_t u32ms)
{
    SENSOR_T *psHandle = (SENSOR_T *)(((uint8_t *)psLoopableHandle) - sizeof(PRESENTABLE_T));
    SENSOR_CUMULATIVE_ENABLEABLE_T *psEnableableCumulativeHandle = (SENSOR_CUMULATIVE_ENABLEABLE_T *)psHandle;

    if (psEnableableCumulativeHandle->sEnableable.sPresentable.bStateChanged &&
        psEnableableCumulativeHandle->sEnableable.sPresentable.u8State == (uint8_t) false)
        psHandle->sLoopable.vLoop = Sensor_vLoopEnableableCumulativeDisabled;
    else
        Sensor_vHandleCumulative(psHandle, u32ms);
}
