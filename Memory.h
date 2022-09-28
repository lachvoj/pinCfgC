#ifndef MEMORY_H
#define MEMORY_H

#include "Types.h"

bool Memory_bInit(uint8_t *pu8Memory, size_t szSize);
void *Memory_vpAlloc(size_t szSize);
void *Memory_vpTempAlloc(size_t szSize);
void Memory_vTempFree(void);

#endif // MEMORY_H
