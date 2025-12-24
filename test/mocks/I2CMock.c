#ifdef PINCFG_FEATURE_I2C_MEASUREMENT

#include "I2CMock.h"

#include <string.h>

// Global mock state
static WIRE_MOCK_T g_sWireMock;

void Wire_begin(void)
{
    WireMock_vReset();
}

void Wire_beginTransmission(uint8_t address)
{
    g_sWireMock.u8LastAddress = address;
}

uint8_t Wire_write(uint8_t data)
{
    // Capture register address (typically first write after beginTransmission)
    g_sWireMock.u8LastRegister = data;
    return 1; // 1 byte written
}

uint8_t Wire_endTransmission(bool sendStop)
{
    (void)sendStop; // Not used in mock

    if (g_sWireMock.bSimulateError)
    {
        return 1; // Non-zero = error
    }

    return 0; // Success
}

uint8_t Wire_requestFrom(uint8_t address, uint8_t quantity)
{
    g_sWireMock.u8LastAddress = address;

    if (g_sWireMock.bSimulateError)
    {
        return 0; // Device not responding
    }

    if (g_sWireMock.bSimulateTimeout)
    {
        // Don't load buffer, so Wire_available() returns 0
        return 0;
    }

    // Normal case: return how many bytes are available (up to quantity requested)
    if (g_sWireMock.u8BufferSize < quantity)
    {
        return g_sWireMock.u8BufferSize;
    }

    return quantity;
}

uint8_t Wire_available(void)
{
    if (g_sWireMock.bSimulateTimeout)
    {
        return 0; // Simulate no data available (timeout)
    }

    return g_sWireMock.u8BufferSize - g_sWireMock.u8ReadIndex;
}

uint8_t Wire_read(void)
{
    if (g_sWireMock.u8ReadIndex < g_sWireMock.u8BufferSize)
    {
        return g_sWireMock.au8Buffer[g_sWireMock.u8ReadIndex++];
    }

    return 0; // No more data
}

void WireMock_vSetResponse(const uint8_t *data, uint8_t size)
{
    if (size > sizeof(g_sWireMock.au8Buffer))
    {
        size = sizeof(g_sWireMock.au8Buffer);
    }

    memcpy(g_sWireMock.au8Buffer, data, size);
    g_sWireMock.u8BufferSize = size;
    g_sWireMock.u8ReadIndex = 0;
}

void WireMock_vSimulateTimeout(bool enable)
{
    g_sWireMock.bSimulateTimeout = enable;
}

void WireMock_vSimulateError(bool enable)
{
    g_sWireMock.bSimulateError = enable;
}

void WireMock_vReset(void)
{
    memset(&g_sWireMock, 0, sizeof(WIRE_MOCK_T));
}

uint8_t WireMock_u8GetLastAddress(void)
{
    return g_sWireMock.u8LastAddress;
}

uint8_t WireMock_u8GetLastRegister(void)
{
    return g_sWireMock.u8LastRegister;
}

#endif // PINCFG_FEATURE_I2C_MEASUREMENT
