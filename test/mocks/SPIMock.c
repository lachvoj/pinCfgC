#ifdef PINCFG_FEATURE_SPI_MEASUREMENT

#include "SPIMock.h"

#include <string.h>

// Global mock state
static SPI_MOCK_T g_sSPIMock;

void SPI_begin(void)
{
    SPIMock_vReset();
    g_sSPIMock.u8Mode = 0;           // Default to MODE0
    g_sSPIMock.u32ClockHz = 1000000; // Default to 1MHz
}

uint8_t SPI_transfer(uint8_t data)
{
    // Store transmitted data
    if (g_sSPIMock.u8LastTxSize < sizeof(g_sSPIMock.au8LastTxData))
    {
        g_sSPIMock.au8LastTxData[g_sSPIMock.u8LastTxSize++] = data;
    }

    g_sSPIMock.u32TransferCount++;

    if (g_sSPIMock.bSimulateError)
    {
        return 0x00; // Return error pattern
    }

    // Return next byte from response buffer
    if (g_sSPIMock.u8ResponseIndex < g_sSPIMock.u8ResponseSize)
    {
        return g_sSPIMock.au8ResponseBuffer[g_sSPIMock.u8ResponseIndex++];
    }

    return 0xFF; // Default SPI idle pattern
}

void SPI_transferBytes(uint8_t *txBuffer, uint8_t *rxBuffer, uint8_t length)
{
    for (uint8_t i = 0; i < length; i++)
    {
        rxBuffer[i] = SPI_transfer(txBuffer[i]);
    }
}

void SPI_setConfig(uint8_t mode, uint32_t clockHz)
{
    g_sSPIMock.u8Mode = mode;
    g_sSPIMock.u32ClockHz = clockHz;
}

void SPI_end(void)
{
    // Nothing to do in mock
}

void SPIMock_vSetResponse(const uint8_t *data, uint8_t size)
{
    if (size > sizeof(g_sSPIMock.au8ResponseBuffer))
    {
        size = sizeof(g_sSPIMock.au8ResponseBuffer);
    }

    memcpy(g_sSPIMock.au8ResponseBuffer, data, size);
    g_sSPIMock.u8ResponseSize = size;
    g_sSPIMock.u8ResponseIndex = 0;
}

void SPIMock_vSimulateError(bool enable)
{
    g_sSPIMock.bSimulateError = enable;
}

void SPIMock_vReset(void)
{
    memset(&g_sSPIMock, 0, sizeof(SPI_MOCK_T));
}

const uint8_t *SPIMock_pu8GetLastTxData(uint8_t *pSize)
{
    if (pSize != NULL)
    {
        *pSize = g_sSPIMock.u8LastTxSize;
    }
    return g_sSPIMock.au8LastTxData;
}

uint32_t SPIMock_u32GetTransferCount(void)
{
    return g_sSPIMock.u32TransferCount;
}

uint8_t SPIMock_u8GetMode(void)
{
    return g_sSPIMock.u8Mode;
}

uint32_t SPIMock_u32GetClockHz(void)
{
    return g_sSPIMock.u32ClockHz;
}

#endif // PINCFG_FEATURE_SPI_MEASUREMENT
