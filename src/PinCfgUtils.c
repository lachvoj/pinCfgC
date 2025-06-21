#include "PinCfgUtils.h"

uint32_t PinCfg_u32GetElapsedTime(uint32_t u32Start, uint32_t u32Current)
{
    // Handle case where timestamp has wrapped around
    if (u32Current < u32Start) {
        return (UINT32_MAX - u32Start) + u32Current + 1;
    }
    return u32Current - u32Start;
}
