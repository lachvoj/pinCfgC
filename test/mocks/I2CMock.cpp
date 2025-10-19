/**
 * @file I2CMock.cpp
 * @brief C++ implementation of Wire class mock
 */

#include "I2CMock.h"

// =============================================================================
// WireMock Class Implementation
// =============================================================================

void WireMock::begin()
{
    // No-op for mock
}

void WireMock::beginTransmission(uint8_t address)
{
    mock_Wire_beginTransmission_called++;
    mock_Wire_beginTransmission_address = address;
    mock_Wire_write_bytes_len = 0;
}

uint8_t WireMock::endTransmission(bool stop)
{
    mock_Wire_endTransmission_called++;
    mock_Wire_endTransmission_stop = stop;
    return mock_Wire_endTransmission_return;
}

size_t WireMock::write(uint8_t data)
{
    mock_Wire_write_byte_called++;
    if (mock_Wire_write_bytes_len < sizeof(mock_Wire_write_bytes))
    {
        mock_Wire_write_bytes[mock_Wire_write_bytes_len++] = data;
    }
    return 1;
}

size_t WireMock::write(const uint8_t *data, size_t length)
{
    mock_Wire_write_buffer_called++;
    for (size_t i = 0; i < length && mock_Wire_write_bytes_len < sizeof(mock_Wire_write_bytes); i++)
    {
        mock_Wire_write_bytes[mock_Wire_write_bytes_len++] = data[i];
    }
    return length;
}

uint8_t WireMock::requestFrom(uint8_t address, uint8_t quantity, bool stop)
{
    mock_Wire_requestFrom_called++;
    mock_Wire_requestFrom_address = address;
    mock_Wire_requestFrom_quantity = quantity;
    mock_Wire_requestFrom_stop = stop;
    
    // Return number of bytes available
    return mock_Wire_available_return;
}

int WireMock::available()
{
    mock_Wire_available_called++;
    return mock_Wire_available_return;
}

int WireMock::read()
{
    mock_Wire_read_called++;
    
    if (mock_Wire_read_index < mock_Wire_read_sequence_len)
    {
        return mock_Wire_read_sequence[mock_Wire_read_index++];
    }
    
    return -1; // No data available
}

// Global Wire object
WireMock Wire;
