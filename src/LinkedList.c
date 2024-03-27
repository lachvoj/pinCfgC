#include "LinkedList.h"
#include "Memory.h"

LINKEDLIST_RESULT_T LinkedList_eAddToLinkedList(LINKEDLIST_ITEM_T **ppsFirst, void *pvNew)
{
    if (ppsFirst == NULL || pvNew == NULL)
        return LINKEDLIST_NULLPTR_ERROR_E;

    LINKEDLIST_ITEM_T *psLl = NULL;
    if (*ppsFirst == NULL)
    {
        psLl = (LINKEDLIST_ITEM_T *)Memory_vpTempAlloc(sizeof(LINKEDLIST_ITEM_T));
        if (psLl == NULL)
            return LINKEDLIST_OUTOFMEMORY_ERROR_E;

        psLl->pvItem = pvNew;
        psLl->pvNext = NULL;
        *ppsFirst = psLl;
    }
    else
    {
        psLl = *ppsFirst;
        while (psLl->pvNext != NULL)
        {
            psLl = psLl->pvNext;
        }

        LINKEDLIST_ITEM_T *psLlNew = NULL;
        psLlNew = (LINKEDLIST_ITEM_T *)Memory_vpTempAlloc(sizeof(LINKEDLIST_ITEM_T));
        if (psLlNew == NULL)
            return LINKEDLIST_OUTOFMEMORY_ERROR_E;
        psLlNew->pvItem = pvNew;
        psLlNew->pvNext = NULL;
        psLl->pvNext = psLlNew;
    }

    return LINKEDLIST_OK_E;
}

LINKEDLIST_RESULT_T LinkedList_eGetLength(LINKEDLIST_ITEM_T **ppsFirst, size_t *szSize)
{
    if (ppsFirst == NULL)
        return LINKEDLIST_NULLPTR_ERROR_E;

    *szSize = 0;
    if (*ppsFirst == NULL)
        return LINKEDLIST_OK_E;

    (*szSize)++;
    LINKEDLIST_ITEM_T *psLl = *ppsFirst;
    while (psLl->pvNext != NULL)
    {
        psLl = psLl->pvNext;
        (*szSize)++;
    }

    return LINKEDLIST_OK_E;
}

LINKEDLIST_ITEM_T *LinkedList_psGetNext(LINKEDLIST_ITEM_T *pvItem)
{
    if (pvItem == NULL)
        return NULL;

    return pvItem->pvNext;
}

void *LinkedList_pvPopFront(LINKEDLIST_ITEM_T **ppsFirst)
{
    if (ppsFirst == NULL || *ppsFirst == NULL)
        return NULL;

    LINKEDLIST_ITEM_T *psLl = *ppsFirst;
    *ppsFirst = psLl->pvNext;

    void *pvItem = psLl->pvItem;
    Memory_vTempFreePt(psLl);

    return pvItem;
}

void *LinkedList_pvGetStoredItem(LINKEDLIST_ITEM_T *psLLItem)
{
    if (psLLItem == NULL)
        return NULL;

    return psLLItem->pvItem;
}
