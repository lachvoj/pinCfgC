#ifndef MEMORY_H
#define MEMORY_H

#include "Types.h"

typedef enum
{
    MEMORY_OK_E,
    MEMORY_INSUFFICIENT_SIZE_ERROR_E,
    MEMORY_ERROR_E
} MEMORY_RESULT_T;

MEMORY_RESULT_T Memory_eInit(uint8_t *pu8Memory, size_t szSize);
MEMORY_RESULT_T Memory_eReset(void);
void *Memory_vpAlloc(size_t szSize);
void *Memory_vpTempAlloc(size_t szSize);
void Memory_vTempFree(void);
size_t Memory_szGetFree(void);

#endif // MEMORY_H
