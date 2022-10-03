#include <errno.h>
#include <string.h>

#include "Memory.h"
#include "Types.h"

typedef struct MEMORY_HANDLE
{
    uint8_t *pu8Memory;
    void *pvNext;
    void *pvTempEnd;
    size_t szSize;
    bool bIsInitialized;
} MEMORY_HANDLE_T;

static MEMORY_HANDLE_T gsMemory;

MEMORY_RESULT_T Memory_eInit(uint8_t *pu8Memory, size_t szSize)
{
    if (szSize == 0)
        return MEMORY_ERROR_E;

    memset(pu8Memory, 0x00U, szSize);

    gsMemory.pu8Memory = pu8Memory;
    gsMemory.pvNext = gsMemory.pu8Memory;
    gsMemory.szSize = szSize;
    gsMemory.bIsInitialized = true;

    Memory_vTempFree();

    return MEMORY_OK_E;
}

void *Memory_vpAlloc(size_t szSize)
{
    void *pvResult = NULL;

    if (!gsMemory.bIsInitialized)
    {
        errno = ENOMEM;
        return pvResult;
    }

    size_t szToAlloc = szSize / sizeof(void *);
    if (szSize % sizeof(void *) > 0)
        szToAlloc++;

    void *pvNextAfterAllocation = (void *)((void **)gsMemory.pvNext + szToAlloc);
    if (pvNextAfterAllocation >= gsMemory.pvTempEnd)
    {
        errno = ENOMEM;
        return pvResult;
    }

    pvResult = gsMemory.pvNext;
    gsMemory.pvNext = pvNextAfterAllocation;

    return pvResult;
}

void *Memory_vpTempAlloc(size_t szSize)
{
    void *pvResult = NULL;

    if (!gsMemory.bIsInitialized)
    {
        errno = ENOMEM;
        return pvResult;
    }

    size_t szToAlloc = szSize / sizeof(void *);
    if (szSize % sizeof(void *) > 0)
        szToAlloc++;

    void *pvTempEndAfterAllocation = (void *)((void **)gsMemory.pvTempEnd - szToAlloc);
    if (pvTempEndAfterAllocation <= gsMemory.pvNext)
    {
        errno = ENOMEM;
        return pvResult;
    }

    gsMemory.pvTempEnd = pvTempEndAfterAllocation;

    return pvTempEndAfterAllocation;
}

void Memory_vTempFree(void)
{
    if (gsMemory.bIsInitialized)
    {
        gsMemory.pvTempEnd = gsMemory.pu8Memory + (gsMemory.szSize / sizeof(void *)) * sizeof(void *);
    }
}
