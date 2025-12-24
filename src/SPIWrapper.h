#ifndef SPIWRAPPER_H
#define SPIWRAPPER_H

#ifdef PINCFG_FEATURE_SPI_MEASUREMENT

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif

// Detect STM32 HAL mode (must match SPIWrapper.cpp detection)
#if defined(ARDUINO_ARCH_STM32) || defined(ARDUINO_ARCH_STM32F1) || defined(ARDUINO_ARCH_STM32F4) ||                   \
    defined(ARDUINO_ARCH_STM32L4) || defined(USE_STM32_HAL_SPI)

    void SPI_vSetSPIHandle(void *psHandle);
#endif

    void SPI_vBegin(void);

    uint8_t SPI_u8Transfer(uint8_t u8Data);

    void SPI_vTransferBytes(uint8_t *pu8TxBuffer, uint8_t *pu8RxBuffer, uint8_t u8Length);

    void SPI_vSetConfig(uint8_t u8Mode, uint32_t u32ClockHz);

    void SPI_vEnd(void);

#ifdef __cplusplus
}
#endif

#endif // PINCFG_FEATURE_SPI_MEASUREMENT
#endif // SPIWRAPPER_H
