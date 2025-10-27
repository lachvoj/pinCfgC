#ifdef FEATURE_SPI_MEASUREMENT

#ifdef UNIT_TEST
extern "C"
{
#include "SPIMock.h"
}
#else

// Detect STM32 HAL mode
#if defined(ARDUINO_ARCH_STM32) || defined(ARDUINO_ARCH_STM32F1) || \
    defined(ARDUINO_ARCH_STM32F4) || defined(ARDUINO_ARCH_STM32L4) || \
    defined(USE_STM32_HAL_SPI)
    #define USE_STM32_HAL_MODE
#endif

#ifdef USE_STM32_HAL_MODE

// STM32 HAL SPI support
#include "stm32_def.h"  // Platform definitions
#include <cstddef>      // For NULL definition

// Default SPI handle (can be overridden)
#ifndef SPI_HANDLE
    extern SPI_HandleTypeDef hspi1;
    #define SPI_HANDLE hspi1
#endif

#ifndef SPI_TIMEOUT_MS
    #define SPI_TIMEOUT_MS 100
#endif

// External SPI handle (must be defined in main.c or similar)
// Provide a weak default to avoid linker errors if not defined
extern SPI_HandleTypeDef SPI_HANDLE;

// Weak default definition - can be overridden by user code
__attribute__((weak)) SPI_HandleTypeDef hspi1;

// Global SPI handle pointer (can be changed via SPI_vSetSPIHandle)
static SPI_HandleTypeDef *g_psSPIHandle = &SPI_HANDLE;

#else
// Arduino SPI library
#include <SPI.h>
#endif

#endif // UNIT_TEST

#include "SPIWrapper.h"

// SPI configuration state (for non-HAL mode)
#ifndef USE_STM32_HAL_MODE
static uint8_t g_u8SPIMode = 0;  // SPI_MODE0
static uint32_t g_u32SPIClockHz = 1000000;  // 1 MHz default
#endif

extern "C"
{

#ifdef USE_STM32_HAL_MODE
/**
 * @brief Set SPI handle to use for SPI operations
 */
void SPI_vSetSPIHandle(void *psHandle)
{
    if (psHandle != NULL)
    {
        g_psSPIHandle = (SPI_HandleTypeDef *)psHandle;
    }
}
#endif

/**
 * @brief Initialize SPI library
 */
void SPI_vBegin(void)
{
#ifdef UNIT_TEST
    SPI_begin();
#elif defined(USE_STM32_HAL_MODE)
    // SPI is initialized via HAL_SPI_Init() in main.c
    // No additional initialization needed
#else
    // Arduino SPI library
    SPI.begin();
    SPI.setDataMode(g_u8SPIMode);
    SPI.setClockDivider(F_CPU / g_u32SPIClockHz);
    SPI.setBitOrder(MSBFIRST);
#endif
}

/**
 * @brief Transfer single byte via SPI
 */
uint8_t SPI_u8Transfer(uint8_t u8Data)
{
#ifdef UNIT_TEST
    return SPI_transfer(u8Data);
#elif defined(USE_STM32_HAL_MODE)
    uint8_t u8RxData = 0;
    HAL_SPI_TransmitReceive(g_psSPIHandle, &u8Data, &u8RxData, 1, SPI_TIMEOUT_MS);
    return u8RxData;
#else
    return SPI.transfer(u8Data);
#endif
}

/**
 * @brief Transfer multiple bytes via SPI
 */
void SPI_vTransferBytes(uint8_t *pu8TxBuffer, uint8_t *pu8RxBuffer, uint8_t u8Length)
{
#ifdef UNIT_TEST
    SPI_transferBytes(pu8TxBuffer, pu8RxBuffer, u8Length);
#elif defined(USE_STM32_HAL_MODE)
    if (pu8TxBuffer == pu8RxBuffer)
    {
        // In-place transfer
        uint8_t au8TempBuffer[32];  // Temporary buffer for TX data
        if (u8Length <= sizeof(au8TempBuffer))
        {
            for (uint8_t i = 0; i < u8Length; i++)
            {
                au8TempBuffer[i] = pu8TxBuffer[i];
            }
            HAL_SPI_TransmitReceive(g_psSPIHandle, au8TempBuffer, pu8RxBuffer, u8Length, SPI_TIMEOUT_MS);
        }
    }
    else
    {
        HAL_SPI_TransmitReceive(g_psSPIHandle, pu8TxBuffer, pu8RxBuffer, u8Length, SPI_TIMEOUT_MS);
    }
#else
    // Arduino SPI library doesn't have bulk transfer, do byte-by-byte
    for (uint8_t i = 0; i < u8Length; i++)
    {
        pu8RxBuffer[i] = SPI.transfer(pu8TxBuffer[i]);
    }
#endif
}

/**
 * @brief Set SPI configuration
 */
void SPI_vSetConfig(uint8_t u8Mode, uint32_t u32ClockHz)
{
#ifdef UNIT_TEST
    SPI_setConfig(u8Mode, u32ClockHz);
#elif defined(USE_STM32_HAL_MODE)
    // For STM32 HAL, SPI configuration is done in CubeMX/main.c
    // Runtime reconfiguration would require HAL_SPI_DeInit/Init
    // Not implemented for now - use compile-time configuration
    (void)u8Mode;
    (void)u32ClockHz;
#else
    g_u8SPIMode = u8Mode;
    g_u32SPIClockHz = u32ClockHz;
    
    SPI.setDataMode(u8Mode);
    SPI.setClockDivider(F_CPU / u32ClockHz);
#endif
}

/**
 * @brief End SPI operations
 */
void SPI_vEnd(void)
{
#ifdef UNIT_TEST
    SPI_end();
#elif defined(USE_STM32_HAL_MODE)
    // HAL doesn't have "end" - SPI peripheral stays initialized
#else
    SPI.end();
#endif
}

} // extern "C"

#endif // FEATURE_SPI_MEASUREMENT
