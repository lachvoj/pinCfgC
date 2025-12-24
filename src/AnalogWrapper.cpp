#ifdef PINCFG_FEATURE_ANALOG_MEASUREMENT

#include <stddef.h> // For NULL

#include "AnalogWrapper.h"


// Detect STM32 HAL mode
#if defined(ARDUINO_ARCH_STM32) || defined(ARDUINO_ARCH_STM32F1) || defined(ARDUINO_ARCH_STM32F4) ||                   \
    defined(ARDUINO_ARCH_STM32L4) || defined(USE_STM32_HAL_ADC)
#define USE_STM32_HAL_MODE
#endif

// Include appropriate ADC implementation
#ifdef UNIT_TEST
#include "ArduinoMock.h"
#elif defined(USE_STM32_HAL_MODE)
// STM32 HAL ADC support
#include "stm32_def.h" // Platform definitions

// Default ADC handle (can be overridden by defining ANALOG_ADC_HANDLE)
#ifndef ANALOG_ADC_HANDLE
#define ANALOG_ADC_HANDLE hadc1
#endif

// Default ADC timeout in milliseconds
#ifndef ANALOG_ADC_TIMEOUT_MS
#define ANALOG_ADC_TIMEOUT_MS 10
#endif

// External ADC handle (must be defined in main.c or similar)
// Provide a weak default to avoid linker errors if not defined
extern ADC_HandleTypeDef ANALOG_ADC_HANDLE;

// Weak default definition - can be overridden by user code
__attribute__((weak)) ADC_HandleTypeDef hadc1;

// Pointer to the actual ADC handle to use (allows runtime configuration)
static ADC_HandleTypeDef *g_psADCHandle = &ANALOG_ADC_HANDLE;

// Pin to ADC channel mapping table (supports up to 16 pins)
// Default mapping: Pin N -> ADC_CHANNEL_N
#define ANALOG_PIN_MAP_SIZE 16
static uint32_t g_au32PinToChannel[ANALOG_PIN_MAP_SIZE] = {
    ADC_CHANNEL_0,
    ADC_CHANNEL_1,
    ADC_CHANNEL_2,
    ADC_CHANNEL_3,
    ADC_CHANNEL_4,
    ADC_CHANNEL_5,
    ADC_CHANNEL_6,
    ADC_CHANNEL_7,
    ADC_CHANNEL_8,
    ADC_CHANNEL_9,
    ADC_CHANNEL_10,
    ADC_CHANNEL_11,
    ADC_CHANNEL_12,
    ADC_CHANNEL_13,
    ADC_CHANNEL_14,
    ADC_CHANNEL_15};
#else
#include <Arduino.h>
#endif

extern "C"
{

#ifdef USE_STM32_HAL_MODE
    void Analog_vSetADCHandle(void *psHandle)
    {
        if (psHandle != NULL)
        {
            g_psADCHandle = (ADC_HandleTypeDef *)psHandle;
        }
    }

    void Analog_vSetPinToChannel(uint8_t u8Pin, uint32_t u32Channel)
    {
        if (u8Pin < ANALOG_PIN_MAP_SIZE)
        {
            g_au32PinToChannel[u8Pin] = u32Channel;
        }
    }
#endif

    uint16_t Analog_u16Read(uint8_t u8Pin)
    {
#ifdef UNIT_TEST
        // Use mock implementation
        extern uint16_t mock_analogRead_u16Return;
        extern uint32_t mock_analogRead_u32Called;
        extern uint8_t mock_analogRead_u8LastPin;
        mock_analogRead_u32Called++;
        mock_analogRead_u8LastPin = u8Pin;
        return mock_analogRead_u16Return;

#elif defined(USE_STM32_HAL_MODE)
        // STM32 HAL implementation
        if (g_psADCHandle == NULL)
        {
            return 0; // Error: ADC not configured
        }

        // Validate pin number
        if (u8Pin >= ANALOG_PIN_MAP_SIZE)
        {
            return 0; // Error: Invalid pin
        }

        // Get ADC channel for this pin
        uint32_t u32Channel = g_au32PinToChannel[u8Pin];

        // Configure ADC channel
        ADC_ChannelConfTypeDef sConfig = {0};
        sConfig.Channel = u32Channel;
        sConfig.Rank = ADC_REGULAR_RANK_1;

// Set sampling time based on STM32 family
#if defined(STM32F1xx)
        sConfig.SamplingTime = ADC_SAMPLETIME_55CYCLES_5;
#elif defined(STM32F4xx) || defined(STM32L4xx)
        sConfig.SamplingTime = ADC_SAMPLETIME_84CYCLES;
#else
        sConfig.SamplingTime = ADC_SAMPLETIME_55CYCLES_5; // Default
#endif

        if (HAL_ADC_ConfigChannel(g_psADCHandle, &sConfig) != HAL_OK)
        {
            return 0; // Error: Channel configuration failed
        }

        // Start ADC conversion
        if (HAL_ADC_Start(g_psADCHandle) != HAL_OK)
        {
            return 0; // Error: Start failed
        }

        // Poll for conversion with timeout
        if (HAL_ADC_PollForConversion(g_psADCHandle, ANALOG_ADC_TIMEOUT_MS) != HAL_OK)
        {
            HAL_ADC_Stop(g_psADCHandle);
            return 0; // Error: Timeout
        }

        // Get ADC value
        uint16_t u16Value = (uint16_t)HAL_ADC_GetValue(g_psADCHandle);

        // Stop ADC
        HAL_ADC_Stop(g_psADCHandle);

        return u16Value;

#else
        // Arduino implementation
        return (uint16_t)analogRead(u8Pin);
#endif
    }

} // extern "C"

#endif // PINCFG_FEATURE_ANALOG_MEASUREMENT
