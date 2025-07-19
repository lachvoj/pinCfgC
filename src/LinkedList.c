#include "LinkedList.h"
#include "Memory.h"

LINKEDLIST_RESULT_T LinkedList_eAddToLinkedList(LINKEDLIST_ITEM_T **ppsFirst, void *pvNew)
{
    if (ppsFirst == NULL || pvNew == NULL)
        return LINKEDLIST_NULLPTR_ERROR_E;

    LINKEDLIST_ITEM_T *psNew = (LINKEDLIST_ITEM_T *)Memory_vpTempAlloc(sizeof(LINKEDLIST_ITEM_T));
    if (psNew == NULL)
        return LINKEDLIST_OUTOFMEMORY_ERROR_E;

    psNew->pvItem = pvNew;
    psNew->pvNext = NULL;

    if (*ppsFirst == NULL)
        *ppsFirst = psNew;
    else
     {
        LINKEDLIST_ITEM_T *psLl = *ppsFirst;
        while (psLl->pvNext != NULL)
            psLl = psLl->pvNext;
        psLl->pvNext = psNew;
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

LINKEDLIST_RESULT_T LinkedList_eLinkedListToArray(LINKEDLIST_ITEM_T **ppsFirst, uint8_t *u8Count)
{
    if (ppsFirst == NULL || u8Count == NULL)
        return LINKEDLIST_NULLPTR_ERROR_E;

    size_t szCount = 0;
    LINKEDLIST_RESULT_T eResult = LinkedList_eGetLength(ppsFirst, &szCount);
    if (eResult != LINKEDLIST_OK_E)
        return eResult;

    void **pvArray = (void **)Memory_vpAlloc(sizeof(void *) * szCount);
    if (pvArray == NULL)
            return LINKEDLIST_OUTOFMEMORY_ERROR_E;

    *u8Count = 0U;
    void *pvItem = LinkedList_pvPopFront(ppsFirst);
    while (pvItem != NULL)
    {
        pvArray[*u8Count] = pvItem;
        (*u8Count)++;
        pvItem = LinkedList_pvPopFront(ppsFirst);
    }
    *ppsFirst = (LINKEDLIST_ITEM_T *)pvArray;

    return LINKEDLIST_OK_E;
}