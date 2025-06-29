#include <errno.h>
#include <string.h>

#include "Globals.h"
#include "Memory.h"
#include "Types.h"

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

GLOBALS_T *psGlobals;

MEMORY_RESULT_T Memory_eInit(uint8_t *pu8Memory, size_t szSize)
{
    if (pu8Memory == NULL)
        return MEMORY_ERROR_E;

    if (szSize < sizeof(GLOBALS_T))
        return MEMORY_INSUFFICIENT_SIZE_ERROR_E;

    psGlobals = (GLOBALS_T *)pu8Memory;
    psGlobals->pvMemEnd = (char *)(pu8Memory + ((szSize - 1) / sizeof(void *)) * sizeof(void *));
    psGlobals->pvMemNext = (char *)(pu8Memory + sizeof(GLOBALS_T));
    psGlobals->pvMemTempEnd = psGlobals->pvMemNext;
    psGlobals->u8LoopablesCount = 0;
    psGlobals->u8PresentablesCount = 0;
    psGlobals->ppsLoopables = NULL;
    psGlobals->ppsPresentables = NULL;
    psGlobals->bMemIsInitialized = true;

    Memory_vTempFree();

    return MEMORY_OK_E;
}

MEMORY_RESULT_T Memory_eReset(void)
{
    psGlobals->pvMemNext = ((char *)psGlobals) + sizeof(GLOBALS_T);
    psGlobals->pvMemTempEnd = psGlobals->pvMemNext;
    psGlobals->ppsLoopables = NULL;
    psGlobals->ppsPresentables = NULL;
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
    if (psGlobals->bMemIsInitialized)
    {
        MEMORY_TEMP_ITEM_T *psTempItem = (MEMORY_TEMP_ITEM_T *)((char *)pvToFree - sizeof(MEMORY_TEMP_ITEM_T));
        // memset(pvToFree, 0x00U, psTempItem->u16AlocatedSize - sizeof(MEMORY_TEMP_ITEM_T));
        psTempItem->bFree = true;
        if (psGlobals->pvMemTempEnd == (char *)psTempItem)
        {
            while (psTempItem->bFree == true)
            {
                size_t szTmp = (size_t)(0x0000FFFFU & psTempItem->u16AlocatedSize);
                memset(psGlobals->pvMemTempEnd, 0x00U, szTmp);
                psGlobals->pvMemTempEnd = psGlobals->pvMemTempEnd + szTmp;
                if (psGlobals->pvMemTempEnd >= psGlobals->pvMemEnd)
                {
                    psGlobals->pvMemTempEnd = psGlobals->pvMemEnd;
                    break;
                }
                psTempItem = (MEMORY_TEMP_ITEM_T *)psGlobals->pvMemTempEnd;
            }
        }
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
