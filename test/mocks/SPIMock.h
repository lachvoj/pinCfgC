#ifndef SPIMOCK_H
#define SPIMOCK_H

#ifdef PINCFG_FEATURE_SPI_MEASUREMENT

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif
    typedef struct
    {
        uint8_t au8ResponseBuffer[32]; // Simulated SPI response data
        uint8_t u8ResponseSize;        // Number of bytes in response buffer
        uint8_t u8ResponseIndex;       // Current read position in response
        uint8_t au8LastTxData[32];     // Last transmitted data (for verification)
        uint8_t u8LastTxSize;          // Size of last transmission
        uint8_t u8Mode;                // Current SPI mode
        uint32_t u32ClockHz;           // Current clock speed
        bool bSimulateError;           // If true, simulate transfer errors
        uint32_t u32TransferCount;     // Number of transfers performed
    } SPI_MOCK_T;

    void SPI_begin(void);

    uint8_t SPI_transfer(uint8_t data);

    void SPI_transferBytes(uint8_t *txBuffer, uint8_t *rxBuffer, uint8_t length);

    void SPI_setConfig(uint8_t mode, uint32_t clockHz);

    void SPI_end(void);

    void SPIMock_vSetResponse(const uint8_t *data, uint8_t size);

    void SPIMock_vSimulateError(bool enable);

    void SPIMock_vReset(void);

    const uint8_t *SPIMock_pu8GetLastTxData(uint8_t *pSize);

    uint32_t SPIMock_u32GetTransferCount(void);

    uint8_t SPIMock_u8GetMode(void);

    uint32_t SPIMock_u32GetClockHz(void);

#ifdef __cplusplus
}
#endif

#endif // PINCFG_FEATURE_SPI_MEASUREMENT
#endif // SPIMOCK_H
