#include "Memory.h"
#include "PinCfgUtils.h"
#include "Sensor.h"

static void Sensor_vLoopStandard(LOOPABLE_T *psLoopableHandle, uint32_t u32ms);
static void Sensor_vLoopCumulative(LOOPABLE_T *psLoopableHandle, uint32_t u32ms);
static void Sensor_vLoopEnableableDisabled(LOOPABLE_T *psLoopableHandle, uint32_t u32ms);
static void Sensor_vLoopEnableableEanbledStandard(LOOPABLE_T *psLoopableHandle, uint32_t u32ms);
static void Sensor_vLoopEnableableEnabledCumulative(LOOPABLE_T *psLoopableHandle, uint32_t u32ms);

SENSOR_RESULT_T Sensor_eInit(
    SENSOR_T *psHandle,
    STRING_POINT_T *sName,
    mysensors_data_t eVType,
    mysensors_sensor_t eSType,
    void (*vReceive)(PRESENTABLE_T *psHandle, const void *pvMessage),
    SENSOR_RESULT_T (*eMeasure)(SENSOR_T *psHandle, float *fValue),
    uint8_t u8Id,
    uint8_t u8Enableable,
    uint32_t u32SamplingInterval,
    uint32_t u32ReportInterval,
    float fOffset,
    SENSOR_MODE_T eMode)
{
    if (psHandle == NULL || sName == NULL || vReceive == NULL || eMeasure == NULL)
        return SENSOR_NULLPTR_ERROR_E;

    // presentable init
    if (Presentable_eInit(&psHandle->sPresentable, sName, u8Id) != PRESENTABLE_OK_E)
        return SENSOR_SUBINIT_ERROR_E;
    psHandle->sVtab.eVType = eVType;
    psHandle->sVtab.eSType = eSType;
    psHandle->sVtab.vReceive = vReceive;
    psHandle->sPresentable.psVtab = &psHandle->sVtab;

    // enableable init
    psHandle->psEnableable = NULL;
    if (u8Enableable != 0U)
    {
        psHandle->psEnableable = (ENABLEABLE_T *)Memory_vpAlloc(sizeof(ENABLEABLE_T));
        if (psHandle->psEnableable == NULL)
            return SENSOR_MEMORY_ALLOCATION_ERROR_E;

        if (Enableable_eInit(psHandle->psEnableable, sName, u8Id + 1) != ENABLEABLE_OK_E)
            return SENSOR_SUBINIT_ERROR_E;
    }

    // loopable init
    if (u8Enableable != 0U)
        psHandle->sLoopable.vLoop = Sensor_vLoopEnableableDisabled;
    else
    {
        psHandle->sLoopable.vLoop = Sensor_vLoopStandard;
        if (eMode == SENSOR_MODE_CUMULATIVE_E)
            psHandle->sLoopable.vLoop = Sensor_vLoopCumulative;
    }

    // values init
    psHandle->eMeasure = eMeasure;

    psHandle->u32SamplingInterval = u32SamplingInterval;
    psHandle->u32LastSamplingMs = 0U;
    psHandle->fCumulatedValue = 0.0f;
    psHandle->u32SamplesCount = 0U;

    psHandle->u32ReportInterval = u32ReportInterval;
    psHandle->u32LastReportMs = 0U;
    psHandle->fOffset = fOffset;
    psHandle->eMode = eMode;

    return SENSOR_OK_E;
}

static void Sensor_vHandleStandard(SENSOR_T *psHandle, uint32_t u32ms)
{
    if (PinCfg_u32GetElapsedTime(psHandle->u32LastReportMs, u32ms) < psHandle->u32ReportInterval)
        return;

    psHandle->u32LastReportMs = u32ms;

    float fValue = 0.0f;
    if (psHandle->eMeasure(psHandle, &fValue) != SENSOR_OK_E)
        return;

    Presentable_vSetState((PRESENTABLE_T *)psHandle, (uint8_t)fValue, true);
}

static void Sensor_vHandleCumulative(SENSOR_T *psHandle, uint32_t u32ms)
{
    if (PinCfg_u32GetElapsedTime(psHandle->u32LastSamplingMs, u32ms) < psHandle->u32SamplingInterval)
        return;

    psHandle->u32LastSamplingMs = u32ms;

    float fValue = 0.0f;
    if (psHandle->eMeasure(psHandle, &fValue) != SENSOR_OK_E)
        return;

    psHandle->fCumulatedValue += fValue;
    psHandle->u32SamplesCount++;

    if (PinCfg_u32GetElapsedTime(psHandle->u32LastReportMs, u32ms) < psHandle->u32ReportInterval)
        return;

    psHandle->u32LastReportMs = u32ms;
    fValue = psHandle->fCumulatedValue / psHandle->u32SamplesCount;
    Presentable_vSetState((PRESENTABLE_T *)psHandle, (uint8_t)fValue, true);
    psHandle->fCumulatedValue = 0.0f;
    psHandle->u32SamplesCount = 0U;
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

static void Sensor_vLoopEnableableDisabled(LOOPABLE_T *psLoopableHandle, uint32_t u32ms)
{
    (void)u32ms; // unused parameter

    SENSOR_T *psHandle = (SENSOR_T *)(((uint8_t *)psLoopableHandle) - sizeof(PRESENTABLE_T));

    if (!psHandle->psEnableable->sPresentable.bStateChanged)
        return;

    psHandle->psEnableable->sPresentable.bStateChanged = false;

    psHandle->u32LastSamplingMs = 0U;
    psHandle->fCumulatedValue = 0.0f;
    psHandle->u32SamplesCount = 0U;
    psHandle->u32LastReportMs = 0U;

    if (psHandle->psEnableable->sPresentable.u8State == (uint8_t) false)
        return;

    if (psHandle->eMode == SENSOR_MODE_CUMULATIVE_E)
        psHandle->sLoopable.vLoop = Sensor_vLoopEnableableEnabledCumulative;
    else
        psHandle->sLoopable.vLoop = Sensor_vLoopEnableableEanbledStandard;
}

static inline bool Sensor_vCheckDisabling(SENSOR_T *psHandle)
{
    if (psHandle->psEnableable->sPresentable.bStateChanged && psHandle->psEnableable->sPresentable.u8State == (uint8_t) false)
    {
        psHandle->sLoopable.vLoop = Sensor_vLoopEnableableDisabled;
        return true;
    }

    return false;
}

static void Sensor_vLoopEnableableEanbledStandard(LOOPABLE_T *psLoopableHandle, uint32_t u32ms)
{
    SENSOR_T *psHandle = (SENSOR_T *)(((uint8_t *)psLoopableHandle) - sizeof(PRESENTABLE_T));

    if (Sensor_vCheckDisabling(psHandle))
        return;

    Sensor_vHandleStandard(psHandle, u32ms);
}

static void Sensor_vLoopEnableableEnabledCumulative(LOOPABLE_T *psLoopableHandle, uint32_t u32ms)
{
    SENSOR_T *psHandle = (SENSOR_T *)(((uint8_t *)psLoopableHandle) - sizeof(PRESENTABLE_T));

    if (Sensor_vCheckDisabling(psHandle))
        return;

    Sensor_vHandleCumulative(psHandle, u32ms);
}
