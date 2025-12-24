#ifndef I2CMOCK_H
#define I2CMOCK_H

#ifdef PINCFG_FEATURE_I2C_MEASUREMENT

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif

    typedef struct
    {
        uint8_t au8Buffer[32];  // Simulated I2C receive buffer
        uint8_t u8BufferSize;   // Number of bytes in buffer
        uint8_t u8ReadIndex;    // Current read position
        uint8_t u8LastAddress;  // Last device address used
        uint8_t u8LastRegister; // Last register written
        bool bSimulateTimeout;  // If true, never return data (test timeout)
        bool bSimulateError;    // If true, requestFrom returns 0
    } WIRE_MOCK_T;

    void Wire_begin(void);

    void Wire_beginTransmission(uint8_t address);

    uint8_t Wire_write(uint8_t data);

    uint8_t Wire_endTransmission(bool sendStop);

    uint8_t Wire_requestFrom(uint8_t address, uint8_t quantity);

    uint8_t Wire_available(void);

    uint8_t Wire_read(void);

    void WireMock_vSetResponse(const uint8_t *data, uint8_t size);

    void WireMock_vSimulateTimeout(bool enable);

    void WireMock_vSimulateError(bool enable);

    void WireMock_vReset(void);

    uint8_t WireMock_u8GetLastAddress(void);

    uint8_t WireMock_u8GetLastRegister(void);

#ifdef __cplusplus
}
#endif

#endif // PINCFG_FEATURE_I2C_MEASUREMENT
#endif // I2CMOCK_H
