#ifndef PINCFG_UTILS_H
#define PINCFG_UTILS_H

#include <stdint.h>

/**
 * @brief Calculates elapsed time between two timestamps, handling 32-bit overflow
 * 
 * @param u32Start Start timestamp
 * @param u32Current Current timestamp
 * @return uint32_t Elapsed time in milliseconds
 */
uint32_t PinCfg_u32GetElapsedTime(uint32_t u32Start, uint32_t u32Current);

#endif // PINCFG_UTILS_H
