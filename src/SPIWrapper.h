/**
 * @file SPIWrapper.h
 * @brief C wrapper for SPI operations
 * 
 * Provides C-compatible interface for SPI operations.
 * Supports three implementations:
 * 
 * 1. UNIT_TEST mode: Uses SPI mock for testing
 * 2. STM32 HAL mode: Uses STM32 HAL SPI functions (auto-detected)
 * 3. Default mode: Uses Arduino SPI library
 * 
 * STM32 HAL Mode:
 * Automatically enabled when building for STM32 platforms with these macros:
 * - ARDUINO_ARCH_STM32, ARDUINO_ARCH_STM32F1, ARDUINO_ARCH_STM32F4, etc.
 * Can also be manually enabled with USE_STM32_HAL_SPI flag.
 * 
 * Optional defines for STM32 HAL:
 * - SPI_HANDLE: SPI handle to use (default: hspi1)
 * - SPI_TIMEOUT_MS: Timeout in milliseconds (default: 100)
 * 
 * Example platformio.ini (manual enable):
 * ```
 * build_flags = 
 *     -DPINCFG_FEATURE_SPI_MEASUREMENT
 *     -DUSE_STM32_HAL_SPI        # Optional, auto-detected for STM32
 *     -DSPI_HANDLE=hspi1         # Optional, defaults to hspi1
 * ```
 * 
 * SPI Configuration:
 * - Mode: SPI_MODE0 (CPOL=0, CPHA=0) - most common for sensors
 * - Bit Order: MSB first
 * - Clock Speed: 1 MHz default (safe for most sensors)
 * 
 * Note: Clock speed and mode can be changed via SPI_vSetConfig() if needed.
 */

#ifndef SPIWRAPPER_H
#define SPIWRAPPER_H

#ifdef PINCFG_FEATURE_SPI_MEASUREMENT

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C"
{
#endif

// Detect STM32 HAL mode (must match SPIWrapper.cpp detection)
#if defined(ARDUINO_ARCH_STM32) || defined(ARDUINO_ARCH_STM32F1) || \
    defined(ARDUINO_ARCH_STM32F4) || defined(ARDUINO_ARCH_STM32L4) || \
    defined(USE_STM32_HAL_SPI)
    
/**
 * @brief Set SPI handle to use for SPI operations (STM32 HAL mode only)
 * 
 * Optional function to configure which SPI peripheral to use at runtime.
 * If not called, defaults to hspi1.
 * 
 * Must be called before SPI_vBegin() if you want to use a different SPI peripheral.
 * 
 * Example:
 * @code
 * extern SPI_HandleTypeDef hspi2;
 * SPI_vSetSPIHandle(&hspi2);  // Use SPI2 instead of SPI1
 * SPI_vBegin();
 * @endcode
 * 
 * @param psHandle Pointer to initialized SPI_HandleTypeDef (e.g., &hspi1, &hspi2)
 */
void SPI_vSetSPIHandle(void *psHandle);
#endif

/**
 * @brief Initialize SPI library
 * 
 * Must be called once during initialization before any SPI operations.
 * Configures SPI with default settings:
 * - Mode: SPI_MODE0 (CPOL=0, CPHA=0)
 * - Bit Order: MSB first
 * - Clock: 1 MHz
 */
void SPI_vBegin(void);

/**
 * @brief Transfer single byte via SPI (full-duplex)
 * 
 * Sends one byte and simultaneously receives one byte.
 * 
 * @param u8Data Byte to send
 * @return Byte received during transfer
 */
uint8_t SPI_u8Transfer(uint8_t u8Data);

/**
 * @brief Transfer multiple bytes via SPI (full-duplex)
 * 
 * Sends and receives multiple bytes. Input and output buffers can be the same.
 * 
 * @param pu8TxBuffer Pointer to data to send
 * @param pu8RxBuffer Pointer to buffer for received data (can be same as pu8TxBuffer)
 * @param u8Length Number of bytes to transfer
 */
void SPI_vTransferBytes(uint8_t *pu8TxBuffer, uint8_t *pu8RxBuffer, uint8_t u8Length);

/**
 * @brief Set SPI configuration
 * 
 * Allows changing SPI mode and clock speed if needed.
 * Most sensors work with default settings (MODE0, 1MHz).
 * 
 * @param u8Mode SPI mode (0-3):
 *               - 0: CPOL=0, CPHA=0 (most common)
 *               - 1: CPOL=0, CPHA=1
 *               - 2: CPOL=1, CPHA=0
 *               - 3: CPOL=1, CPHA=1
 * @param u32ClockHz Clock speed in Hz (e.g., 1000000 = 1MHz)
 */
void SPI_vSetConfig(uint8_t u8Mode, uint32_t u32ClockHz);

/**
 * @brief End SPI operations (optional cleanup)
 * 
 * Releases SPI peripheral resources. Only needed if you want to
 * free up the SPI peripheral for other use.
 */
void SPI_vEnd(void);

#ifdef __cplusplus
}
#endif

#endif // PINCFG_FEATURE_SPI_MEASUREMENT

#endif // SPIWRAPPER_H
