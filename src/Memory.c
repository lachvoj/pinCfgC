#include "Memory.h"

#include <errno.h>
#include <string.h>

#include "Globals.h"
#include "Types.h"

GLOBALS_T *psGlobals = NULL;

#ifndef USE_MALLOC
typedef struct MEMORY_TEMP_ITEM_S
{
    union
    {
        struct
        {
            uint16_t u16AlocatedSize;
            bool bFree;
        };
        void *pvAligment;
    };
} MEMORY_TEMP_ITEM_T;

static void Memory_vResetState(void)
{
    psGlobals->pvMemNext = (char *)psGlobals + sizeof(GLOBALS_T);
    psGlobals->pvMemTempEnd = psGlobals->pvMemNext;
    psGlobals->u8LoopablesCount = 0;
    psGlobals->u8PresentablesCount = 0;
    psGlobals->ppsLoopables = NULL;
    psGlobals->ppsPresentables = NULL;

    memset(psGlobals->pvMemNext, 0x00U, (size_t)(psGlobals->pvMemEnd - psGlobals->pvMemNext));

    Memory_vTempFree();
}

MEMORY_RESULT_T Memory_eInit(uint8_t *pu8Memory, size_t szSize)
{
    if (pu8Memory == NULL)
        return MEMORY_ERROR_E;

    if (szSize < sizeof(GLOBALS_T))
        return MEMORY_INSUFFICIENT_SIZE_ERROR_E;

    psGlobals = (GLOBALS_T *)pu8Memory;
    psGlobals->pvMemEnd = (char *)(pu8Memory + ((szSize - 1) / sizeof(void *)) * sizeof(void *));
    psGlobals->bMemIsInitialized = true;

    Memory_vResetState();

    return MEMORY_OK_E;
}

MEMORY_RESULT_T Memory_eReset(void)
{
    if (!psGlobals || !psGlobals->bMemIsInitialized)
        return MEMORY_ERROR_E;

    // Reset pointers without recalculating pvMemEnd (which would cause shrinking)
    // bMemIsInitialized stays true, pvMemEnd stays unchanged (already aligned from init)
    Memory_vResetState();

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

    // Check for integer overflow before calculating aligned size
    const size_t szOverheadMax = sizeof(void *) - 1;
    if (szSize > SIZE_MAX - szOverheadMax)
    {
        errno = ENOMEM;
        return pvResult;
    }

    size_t szToAlloc = ((szSize + sizeof(void *) - 1) / sizeof(void *)) * sizeof(void *);

    char *pvNextAfterAllocation = psGlobals->pvMemNext + szToAlloc;
    if (pvNextAfterAllocation > psGlobals->pvMemTempEnd || pvNextAfterAllocation < psGlobals->pvMemNext)
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

    // Check for integer overflow before calculating aligned size
    // Maximum safe size before addition would overflow
    const size_t szOverheadMax = sizeof(MEMORY_TEMP_ITEM_T) + sizeof(void *) - 1;
    if (szSize > SIZE_MAX - szOverheadMax)
    {
        errno = ENOMEM;
        return NULL;
    }

    size_t szToAlloc = ((szSize + sizeof(MEMORY_TEMP_ITEM_T) + sizeof(void *) - 1) / sizeof(void *)) * sizeof(void *);

    char *pvTempEndAfterAllocation = psGlobals->pvMemTempEnd - szToAlloc;
    if (pvTempEndAfterAllocation < psGlobals->pvMemNext || pvTempEndAfterAllocation >= psGlobals->pvMemTempEnd)
    {
        errno = ENOMEM;
        return NULL;
    }

    psGlobals->pvMemTempEnd = pvTempEndAfterAllocation;
    ((MEMORY_TEMP_ITEM_T *)pvTempEndAfterAllocation)->u16AlocatedSize = szToAlloc;
    ((MEMORY_TEMP_ITEM_T *)pvTempEndAfterAllocation)->bFree = false;

    return (void *)((char *)pvTempEndAfterAllocation + sizeof(MEMORY_TEMP_ITEM_T));
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

void Memory_vTempFreePt(void *pvToFree)
{
    if (!psGlobals || !psGlobals->bMemIsInitialized || pvToFree == NULL)
        return;

    MEMORY_TEMP_ITEM_T *psTempItem = (MEMORY_TEMP_ITEM_T *)((char *)pvToFree - sizeof(MEMORY_TEMP_ITEM_T));
    psTempItem->bFree = true;

    // Only coalesce/free from the temp end if this block is at the temp end
    while ((char *)psTempItem == psGlobals->pvMemTempEnd && psTempItem->bFree)
    {
        size_t szTmp = (size_t)(psTempItem->u16AlocatedSize & 0xFFFFU);
        memset(psGlobals->pvMemTempEnd, 0x00U, szTmp);
        psGlobals->pvMemTempEnd += szTmp;

        if (psGlobals->pvMemTempEnd >= psGlobals->pvMemEnd)
        {
            psGlobals->pvMemTempEnd = psGlobals->pvMemEnd;
            break;
        }
        psTempItem = (MEMORY_TEMP_ITEM_T *)psGlobals->pvMemTempEnd;
    }
}

size_t Memory_szGetFree(void)
{
    return (size_t)(psGlobals->pvMemTempEnd - psGlobals->pvMemNext);
}

size_t Memory_szGetAllocatedSize(size_t szSize)
{
    return (size_t)(((szSize + sizeof(void *) - 1) / sizeof(void *)) * sizeof(void *));
}
#else // USE_MALLOC
#include <stdlib.h>

#define MEMORY_SZ SIZE_MAX // Use a large value to indicate "unlimited" memory in malloc mode

MEMORY_RESULT_T Memory_eInit(uint8_t *pu8Memory, size_t szSize)
{
    (void)pu8Memory; // Unused in malloc mode
    (void)szSize;    // Unused in malloc mode
    if (psGlobals != NULL)
        memset(psGlobals, 0, sizeof(GLOBALS_T));
    else
    {
        psGlobals = (GLOBALS_T *)malloc(sizeof(GLOBALS_T));
        if (psGlobals == NULL)
            return MEMORY_INSUFFICIENT_SIZE_ERROR_E;
        memset(psGlobals, 0, sizeof(GLOBALS_T)); // Initialize newly allocated memory
    }

    return MEMORY_OK_E;
}

MEMORY_RESULT_T Memory_eReset(void)
{
    if (psGlobals == NULL)
    {
        return MEMORY_ERROR_E;
    }
    memset(psGlobals, 0, sizeof(GLOBALS_T));

    return MEMORY_OK_E;
}

void *Memory_vpAlloc(size_t szSize)
{
    void *pvResult = malloc(szSize);
    if (pvResult == NULL)
    {
        errno = ENOMEM;
    }
    return pvResult;
}

void *Memory_vpTempAlloc(size_t szSize)
{
    return Memory_vpAlloc(szSize);
}

void Memory_vTempFree(void)
{
}

void Memory_vTempFreePt(void *pvToFree)
{
    if (pvToFree != NULL)
        free(pvToFree);
}

size_t Memory_szGetFree(void)
{
    // In malloc mode, we cannot determine free memory size reliably
    return SIZE_MAX; // Return a large value to indicate "unlimited" free memory
}

size_t Memory_szGetAllocatedSize(size_t szSize)
{
    // In malloc mode, we return the size as is
    return szSize;
}
#endif                     // USE_MALLOC
