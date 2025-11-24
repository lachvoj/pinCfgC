/**
 * @file AnalogWrapper.h
 * @brief C wrapper for Analog ADC operations
 * 
 * Provides C-compatible interface for analog ADC operations.
 * Supports three implementations:
 * 
 * 1. UNIT_TEST mode: Uses mock for testing
 * 2. STM32 HAL mode: Uses STM32 HAL ADC functions (auto-detected)
 * 3. Default mode: Uses Arduino analogRead() API
 * 
 * STM32 HAL Mode:
 * Automatically enabled when building for STM32 platforms with these macros:
 * - ARDUINO_ARCH_STM32, ARDUINO_ARCH_STM32F1, ARDUINO_ARCH_STM32F4, etc.
 * Can also be manually enabled with USE_STM32_HAL_ADC flag.
 * 
 * Optional defines for STM32 HAL:
 * - ANALOG_ADC_HANDLE: ADC handle to use (default: hadc1)
 * - ANALOG_ADC_TIMEOUT_MS: Timeout in milliseconds (default: 10)
 * 
 * Example platformio.ini (manual enable):
 * ```
 * build_flags = 
 *     -DPINCFG_FEATURE_ANALOG_MEASUREMENT
 *     -DUSE_STM32_HAL_ADC        # Optional, auto-detected for STM32
 *     -DANALOG_ADC_HANDLE=hadc1  # Optional, defaults to hadc1
 * ```
 */

#ifndef ANALOGWRAPPER_H
#define ANALOGWRAPPER_H

#ifdef PINCFG_FEATURE_ANALOG_MEASUREMENT

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C"
{
#endif

// Detect STM32 HAL mode (must match AnalogWrapper.cpp detection)
#if defined(ARDUINO_ARCH_STM32) || defined(ARDUINO_ARCH_STM32F1) || \
    defined(ARDUINO_ARCH_STM32F4) || defined(ARDUINO_ARCH_STM32L4) || \
    defined(USE_STM32_HAL_ADC)
    
/**
 * @brief Set ADC handle to use for Analog operations (STM32 HAL mode only)
 * 
 * Optional function to configure which ADC peripheral to use at runtime.
 * If not called, defaults to hadc1.
 * 
 * Must be called before Analog_vBegin() if you want to use a different ADC peripheral.
 * 
 * Example:
 * @code
 * extern ADC_HandleTypeDef hadc2;
 * Analog_vSetADCHandle(&hadc2);  // Use ADC2 instead of ADC1
 * Analog_vBegin();
 * @endcode
 * 
 * @param psHandle Pointer to initialized ADC_HandleTypeDef (e.g., &hadc1, &hadc2)
 */
void Analog_vSetADCHandle(void *psHandle);

/**
 * @brief Configure pin-to-channel mapping for STM32 (HAL mode only)
 * 
 * Maps Arduino-style pin numbers to STM32 ADC channels.
 * Must be called after Analog_vSetADCHandle() and before first analogRead().
 * 
 * If not called, a default mapping will be used (PA0=CH0, PA1=CH1, etc.)
 * 
 * Example:
 * @code
 * // Map pin 0 to ADC_CHANNEL_5 (PA5)
 * Analog_vSetPinToChannel(0, ADC_CHANNEL_5);
 * @endcode
 * 
 * @param u8Pin Pin number (0-15)
 * @param u32Channel STM32 ADC channel (e.g., ADC_CHANNEL_0, ADC_CHANNEL_1)
 */
void Analog_vSetPinToChannel(uint8_t u8Pin, uint32_t u32Channel);
#endif

/**
 * @brief Read analog value from pin (Arduino-compatible)
 * 
 * Direct equivalent to Arduino's analogRead() function.
 * Performs blocking ADC read and returns raw ADC value.
 * 
 * Arduino mode:
 * - Uses analogRead() directly
 * - Returns 0-1023 (10-bit) or 0-4095 (12-bit depending on board)
 * - Pin: Use 0-7 for A0-A7, or direct analog pin number
 * 
 * STM32 HAL mode:
 * - Uses HAL_ADC_Start() + HAL_ADC_PollForConversion()
 * - Returns 0-4095 (12-bit for STM32F1)
 * - Pin: Maps to ADC channel via Analog_vSetPinToChannel()
 * 
 * @param u8Pin Analog pin number
 * @return Raw ADC value (0-1023 or 0-4095 depending on platform)
 *         Returns 0 on error
 */
uint16_t Analog_u16Read(uint8_t u8Pin);

#ifdef __cplusplus
}
#endif

#endif // PINCFG_FEATURE_ANALOG_MEASUREMENT

#endif // ANALOGWRAPPER_H
