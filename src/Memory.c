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
    psGlobals->pvMemEnd = (char *)(pu8Memory + ((szSize - 1) / sizeof(char *)) * sizeof(char *));
    psGlobals->pvMemNext = (char *)(pu8Memory + sizeof(GLOBALS_HANDLE_T));
    psGlobals->pvMemTempEnd = psGlobals->pvMemNext;
    psGlobals->psLoopablesFirst = NULL;
    psGlobals->psPresentablesFirst = NULL;
    psGlobals->bMemIsInitialized = true;

    Memory_vTempFree();

    return MEMORY_OK_E;
}

MEMORY_RESULT_T Memory_eReset(void)
{
    psGlobals->pvMemNext = ((char *)psGlobals) + sizeof(GLOBALS_HANDLE_T);
    psGlobals->pvMemTempEnd = psGlobals->pvMemNext;
    psGlobals->psLoopablesFirst = NULL;
    psGlobals->psPresentablesFirst = NULL;
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

    size_t szToAlloc = (szSize + sizeof(char *) - 1) / sizeof(char *);

    char *pvNextAfterAllocation = (char *)((char **)psGlobals->pvMemNext + szToAlloc);
    if (pvNextAfterAllocation > psGlobals->pvMemTempEnd)
    {
        errno = ENOMEM;
        return pvResult;
    }

    pvResult = (void *)psGlobals->pvMemNext;
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

    size_t szToAlloc = (szSize + sizeof(char *) - 1) / sizeof(char *);

    char *pvTempEndAfterAllocation = (char *)((char **)psGlobals->pvMemTempEnd - szToAlloc);
    if (pvTempEndAfterAllocation < psGlobals->pvMemNext)
    {
        errno = ENOMEM;
        return NULL;
    }

    psGlobals->pvMemTempEnd = pvTempEndAfterAllocation;

    return (void *)pvTempEndAfterAllocation;
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

void Memory_vTempFreeSize(size_t szSize)
{
    if (psGlobals->bMemIsInitialized)
    {
        size_t szToFree = (szSize + sizeof(char *) - 1) / sizeof(char *);
        char *pvTempEndAfterFree = (char *)((char **)psGlobals->pvMemTempEnd + szToFree);
        if (pvTempEndAfterFree > psGlobals->pvMemEnd)
            psGlobals->pvMemTempEnd = psGlobals->pvMemEnd;
        else
            psGlobals->pvMemTempEnd = pvTempEndAfterFree;
    }
}

size_t Memory_szGetFree(void)
{
    return (size_t)(psGlobals->pvMemTempEnd - psGlobals->pvMemNext);
}
