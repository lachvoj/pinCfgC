#ifndef ANALOGWRAPPER_H
#define ANALOGWRAPPER_H

#ifdef PINCFG_FEATURE_ANALOG_MEASUREMENT

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif

// Detect STM32 HAL mode (must match AnalogWrapper.cpp detection)
#if defined(ARDUINO_ARCH_STM32) || defined(ARDUINO_ARCH_STM32F1) || defined(ARDUINO_ARCH_STM32F4) ||                   \
    defined(ARDUINO_ARCH_STM32L4) || defined(USE_STM32_HAL_ADC)

    void Analog_vSetADCHandle(void *psHandle);

    void Analog_vSetPinToChannel(uint8_t u8Pin, uint32_t u32Channel);
#endif

    uint16_t Analog_u16Read(uint8_t u8Pin);

#ifdef __cplusplus
}
#endif

#endif // PINCFG_FEATURE_ANALOG_MEASUREMENT
#endif // ANALOGWRAPPER_H
