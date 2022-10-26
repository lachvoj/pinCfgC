#include <errno.h>
#include <string.h>

#include "Globals.h"
#include "Memory.h"
#include "Types.h"

GLOBALS_HANDLE_T *psGlobals;

MEMORY_RESULT_T Memory_eInit(uint8_t *pu8Memory, size_t szSize)
{
    if (pu8Memory == NULL)
        return MEMORY_ERROR_E;

    if (szSize < sizeof(GLOBALS_HANDLE_T))
        return MEMORY_INSUFFICIENT_SIZE_ERROR_E;

    psGlobals = (GLOBALS_HANDLE_T *)pu8Memory;
    psGlobals->pvMemEnd = pu8Memory + ((szSize - 1) / sizeof(void *)) * sizeof(void *);
    psGlobals->pvMemNext = pu8Memory + sizeof(GLOBALS_HANDLE_T);
    psGlobals->pvMemTempEnd = psGlobals->pvMemNext;
    psGlobals->bMemIsInitialized = true;

    Memory_vTempFree();

    return MEMORY_OK_E;
}

MEMORY_RESULT_T Memory_eReset(void)
{
    psGlobals->pvMemNext = ((uint8_t *)psGlobals) + sizeof(GLOBALS_HANDLE_T);
    psGlobals->pvMemTempEnd = psGlobals->pvMemNext;
    Memory_vTempFree();

    return MEMORY_OK_E;
}

void *Memory_vpAlloc(size_t szSize)
{
    void *pvResult = NULL;

    if (!psGlobals->bMemIsInitialized)
    {
        errno = ENOMEM;
        return pvResult;
    }

    size_t szToAlloc = szSize / sizeof(void *);
    if (szSize % sizeof(void *) > 0)
        szToAlloc++;

    void *pvNextAfterAllocation = (void *)((void **)psGlobals->pvMemNext + szToAlloc);
    if (pvNextAfterAllocation >= psGlobals->pvMemTempEnd)
    {
        errno = ENOMEM;
        return pvResult;
    }

    pvResult = psGlobals->pvMemNext;
    psGlobals->pvMemNext = pvNextAfterAllocation;

    return pvResult;
}

void *Memory_vpTempAlloc(size_t szSize)
{
    if (!psGlobals->bMemIsInitialized)
    {
        errno = ENOMEM;
        return NULL;
    }

    size_t szToAlloc = szSize / sizeof(void *);
    if (szSize % sizeof(void *) > 0)
        szToAlloc++;

    void *pvTempEndAfterAllocation = (void *)((void **)psGlobals->pvMemTempEnd - szToAlloc);
    if (pvTempEndAfterAllocation <= psGlobals->pvMemNext)
    {
        errno = ENOMEM;
        return NULL;
    }

    psGlobals->pvMemTempEnd = pvTempEndAfterAllocation;

    return pvTempEndAfterAllocation;
}

void Memory_vTempFree(void)
{
    if (psGlobals->bMemIsInitialized)
    {
        if (psGlobals->pvMemEnd > psGlobals->pvMemTempEnd)
        {
            memset(psGlobals->pvMemTempEnd, 0x00U, (size_t)(psGlobals->pvMemEnd - psGlobals->pvMemTempEnd));
        }
        psGlobals->pvMemTempEnd = psGlobals->pvMemEnd;
    }
}
