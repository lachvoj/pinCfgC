#ifndef WIREWRAPPER_H
#define WIREWRAPPER_H

#ifdef PINCFG_FEATURE_I2C_MEASUREMENT

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif

// Detect STM32 HAL mode (must match WireWrapper.cpp detection)
#if defined(ARDUINO_ARCH_STM32) || defined(ARDUINO_ARCH_STM32F1) || defined(ARDUINO_ARCH_STM32F4) ||                   \
    defined(ARDUINO_ARCH_STM32L4) || defined(USE_STM32_HAL_I2C)

    void Wire_vSetI2CHandle(void *psHandle);
#endif

    void Wire_vBegin(void);

    void Wire_vBeginTransmission(uint8_t u8Address);

    uint8_t Wire_u8Write(uint8_t u8Data);

    uint8_t Wire_u8WriteBytes(const uint8_t *pu8Data, uint8_t u8Length);

    uint8_t Wire_u8EndTransmission(bool bSendStop);

    uint8_t Wire_u8RequestFrom(uint8_t u8Address, uint8_t u8Quantity);

    uint8_t Wire_u8Available(void);

    uint8_t Wire_u8Read(void);

#ifdef __cplusplus
}
#endif

#endif // PINCFG_FEATURE_I2C_MEASUREMENT
#endif // WIREWRAPPER_H
