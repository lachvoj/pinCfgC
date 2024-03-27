#include "CPUTemp.h"
#include "Globals.h"
#include "InPin.h"
#include "MySensorsWrapper.h"

void CPUTemp_vInitType(PRESENTABLE_VTAB_T *psVtab)
{
    psVtab->eVType = V_TEMP;
    psVtab->eSType = S_TEMP;
    psVtab->vReceive = InPin_vRcvMessage;
}

CPUTEMP_RESULT_T CPUTemp_eInit(
    CPUTEMP_T *psHandle,
    STRING_POINT_T *sName,
    uint8_t u8Id,
    uint32_t u32ReportInterval,
    float fOffset)
{
    if (psHandle == NULL)
        return CPUTEMP_NULLPTR_ERROR_E;

    if (Presentable_eInit(&psHandle->sPresentable, sName, u8Id) != PRESENTABLE_OK_E)
    {
        return CPUTEMP_SUBINIT_ERROR_E;
    }

    // vtab init
    psHandle->sPresentable.psVtab = &psGlobals->sCpuTempPrVTab;

    // loopable init
    psHandle->sLoopable.vLoop = CPUTemp_vLoop;

    psHandle->u32ReportInterval = u32ReportInterval;
    psHandle->fOffset = fOffset;
    psHandle->u32MillisLast = 0;

    return CPUTEMP_OK_E;
}

// loopable IF
void CPUTemp_vLoop(LOOPABLE_T *psLoopableHandle, uint32_t u32ms)
{
    CPUTEMP_T *psHandle = (CPUTEMP_T *)(((uint8_t *)psLoopableHandle) - sizeof(PRESENTABLE_T));
    if ((u32ms - psHandle->u32MillisLast) > psHandle->u32ReportInterval)
    {
        Presentable_vSetState((PRESENTABLE_T *)psHandle, (uint8_t)(i8HwCPUTemperature() + psHandle->fOffset), true);
    }
}