#include "CPUTemp.h"
#include "Globals.h"

CPUTEMP_RESULT_T CPUTemp_eInit(
    CPUTEMP_HANDLE_T *psHandle,
    STRING_POINT_T *sName,
    uint8_t u8Id,
    uint32_t u32ReportInterval,
    float fOffset)
{
    if (psHandle == NULL)
        return CPUTEMP_NULLPTR_ERROR_E;

    if (MySensorsPresent_eInit(&psHandle->sMySenPresent, sName, u8Id) != MYSENSORSPRESENT_OK_E)
    {
        return CPUTEMP_SUBINIT_ERROR_E;
    }

    // vtab init
    psHandle->sMySenPresent.sLooPre.psVtab = &psGlobals->sCpuTempVTab;

    psHandle->u32ReportInterval = u32ReportInterval;
    psHandle->fOffset = fOffset;
    psHandle->u32MillisLast = 0;

    return CPUTEMP_OK_E;
}

// loopable IF
void CPUTemp_vLoop(LOOPRE_T *psBaseHandle, uint32_t u32ms)
{
    CPUTEMP_HANDLE_T *psHandle = (CPUTEMP_HANDLE_T *)psBaseHandle;
    if ((u32ms - psHandle->u32MillisLast) > psHandle->u32ReportInterval)
    {
        MySensorsPresent_vSetState(
            (MYSENSORSPRESENT_HANDLE_T *)psBaseHandle, (uint8_t)(i8HwCPUTemperature() + psHandle->fOffset), true);
    }
}