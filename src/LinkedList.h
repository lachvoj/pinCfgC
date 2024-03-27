#ifndef LINKEDLIST_H
#define LINKEDLIST_H

#include "Types.h"

typedef enum LINKEDLIST_RESULT_E
{
    LINKEDLIST_OK_E,
    LINKEDLIST_NULLPTR_ERROR_E,
    LINKEDLIST_OUTOFMEMORY_ERROR_E,
} LINKEDLIST_RESULT_T;

typedef struct LINKEDLIST_ITEM_S LINKEDLIST_ITEM_T;
typedef struct LINKEDLIST_ITEM_S
{
    void *pvItem;
    LINKEDLIST_ITEM_T *pvNext;
} LINKEDLIST_ITEM_T;

LINKEDLIST_RESULT_T LinkedList_eAddToLinkedList(LINKEDLIST_ITEM_T **ppsFirst, void *pvNew);
LINKEDLIST_RESULT_T LinkedList_eGetLength(LINKEDLIST_ITEM_T **ppsFirst, size_t *szSize);
LINKEDLIST_ITEM_T *LinkedList_psGetNext(LINKEDLIST_ITEM_T* pvItem);
void *LinkedList_pvPopFront(LINKEDLIST_ITEM_T **ppsFirst);
void *LinkedList_pvGetStoredItem(LINKEDLIST_ITEM_T *psLLItem);
#endif // LINKEDLIST_H
