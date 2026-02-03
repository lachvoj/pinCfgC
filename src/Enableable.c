#include "Enableable.h"

#include "Globals.h"
#include "Memory.h"

ENABLEABLE_RESULT_T Enableable_eInit(ENABLEABLE_T *psHandle, STRING_POINT_T *sName, uint8_t u8Id)
{
    if (psHandle == NULL || sName == NULL)
        return ENABLEABLE_NULLPTR_ERROR_E;

    char *pcName = (char *)Memory_vpAlloc(sName->szLen + 1 + 7); // +7 for "_enable"
    if (pcName == NULL)
        return ENABLEABLE_MEMORY_ALLOCATION_ERROR_E;

    // Copy name and append "_enable"
    memcpy((void *)pcName, sName->pcStrStart, sName->szLen);
    memcpy((void *)(pcName + sName->szLen), "_enable\0", 8);

    if (Presentable_eInitReuseName(&psHandle->sPresentable, pcName, u8Id) != PRESENTABLE_OK_E)
        return ENABLEABLE_ERROR_E;

    // Enableable uses the same vtab as Switch
    psHandle->sPresentable.psVtab = &psGlobals->sSwitchPrVTab;

    // Initialize enableable switch to ON state (enabled by default)
    psHandle->sPresentable.u8State = 1;

    return ENABLEABLE_OK_E;
}