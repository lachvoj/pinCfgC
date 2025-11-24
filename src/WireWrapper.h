/**
 * @file WireWrapper.h
 * @brief C wrapper for I2C operations
 * 
 * Provides C-compatible interface for I2C operations.
 * Supports three implementations:
 * 
 * 1. UNIT_TEST mode: Uses I2C mock for testing
 * 2. STM32 HAL mode: Uses STM32 HAL I2C functions (auto-detected)
 * 3. Default mode: Uses Arduino Wire library
 * 
 * STM32 HAL Mode:
 * Automatically enabled when building for STM32 platforms with these macros:
 * - ARDUINO_ARCH_STM32, ARDUINO_ARCH_STM32F1, ARDUINO_ARCH_STM32F4, etc.
 * Can also be manually enabled with USE_STM32_HAL_I2C flag.
 * 
 * Optional defines for STM32 HAL:
 * - WIRE_I2C_HANDLE: I2C handle to use (default: hi2c1)
 * - WIRE_I2C_TIMEOUT_MS: Timeout in milliseconds (default: 100)
 * 
 * Example platformio.ini (manual enable):
 * ```
 * build_flags = 
 *     -DPINCFG_FEATURE_I2C_MEASUREMENT
 *     -DUSE_STM32_HAL_I2C        # Optional, auto-detected for STM32
 *     -DWIRE_I2C_HANDLE=hi2c1    # Optional, defaults to hi2c1
 * ```
 */

#ifndef WIREWRAPPER_H
#define WIREWRAPPER_H

#ifdef PINCFG_FEATURE_I2C_MEASUREMENT

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C"
{
#endif

// Detect STM32 HAL mode (must match WireWrapper.cpp detection)
#if defined(ARDUINO_ARCH_STM32) || defined(ARDUINO_ARCH_STM32F1) || \
    defined(ARDUINO_ARCH_STM32F4) || defined(ARDUINO_ARCH_STM32L4) || \
    defined(USE_STM32_HAL_I2C)
    
/**
 * @brief Set I2C handle to use for Wire operations (STM32 HAL mode only)
 * 
 * Optional function to configure which I2C peripheral to use at runtime.
 * If not called, defaults to hi2c1.
 * 
 * Must be called before Wire_vBegin() if you want to use a different I2C peripheral.
 * 
 * Example:
 * @code
 * extern I2C_HandleTypeDef hi2c2;
 * Wire_vSetI2CHandle(&hi2c2);  // Use I2C2 instead of I2C1
 * Wire_vBegin();
 * @endcode
 * 
 * @param psHandle Pointer to initialized I2C_HandleTypeDef (e.g., &hi2c1, &hi2c2)
 */
void Wire_vSetI2CHandle(void *psHandle);
#endif

/**
 * @brief Initialize Wire library
 * 
 * Must be called once during initialization before any I2C operations.
 */
void Wire_vBegin(void);

/**
 * @brief Begin I2C transmission to device
 * 
 * @param u8Address 7-bit I2C device address
 */
void Wire_vBeginTransmission(uint8_t u8Address);

/**
 * @brief Write single byte to I2C device
 * 
 * Queues data for transmission. Data is actually sent when
 * Wire_u8EndTransmission() is called.
 * 
 * @param u8Data Byte to write
 * @return Number of bytes written (always 1 on success)
 */
uint8_t Wire_u8Write(uint8_t u8Data);

/**
 * @brief Write multiple bytes to I2C device
 * 
 * @param pu8Data Pointer to data buffer
 * @param u8Length Number of bytes to write
 * @return Number of bytes written
 */
uint8_t Wire_u8WriteBytes(const uint8_t *pu8Data, uint8_t u8Length);

/**
 * @brief End I2C transmission
 * 
 * Transmits queued data and releases the bus.
 * 
 * @param bSendStop If true, sends stop condition (releases bus).
 *                  If false, sends restart (keeps bus active).
 * @return 0 on success, error code otherwise:
 *         1 = data too long for transmit buffer
 *         2 = received NACK on transmit of address
 *         3 = received NACK on transmit of data
 *         4 = other error
 */
uint8_t Wire_u8EndTransmission(bool bSendStop);

/**
 * @brief Request bytes from I2C device
 * 
 * Sends request and receives data into internal buffer.
 * Use Wire_u8Available() and Wire_u8Read() to retrieve data.
 * 
 * @param u8Address 7-bit I2C device address
 * @param u8Quantity Number of bytes to request (max 32)
 * @return Number of bytes actually received
 */
uint8_t Wire_u8RequestFrom(uint8_t u8Address, uint8_t u8Quantity);

/**
 * @brief Check how many bytes are available in receive buffer
 * 
 * @return Number of bytes available to read
 */
uint8_t Wire_u8Available(void);

/**
 * @brief Read one byte from receive buffer
 * 
 * @return Next byte from buffer (or 0 if buffer empty)
 */
uint8_t Wire_u8Read(void);

#ifdef __cplusplus
}
#endif

#endif // PINCFG_FEATURE_I2C_MEASUREMENT

#endif // WIREWRAPPER_H
