#ifndef I2CMOCK_H
#define I2CMOCK_H

#ifdef FEATURE_I2C_MEASUREMENT

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief I2C Wire library mock for testing
 * 
 * Simulates Arduino Wire library behavior for unit testing
 * without requiring actual I2C hardware.
 */

/**
 * @brief Mock state structure
 */
typedef struct
{
    uint8_t au8Buffer[32];      // Simulated I2C receive buffer
    uint8_t u8BufferSize;       // Number of bytes in buffer
    uint8_t u8ReadIndex;        // Current read position
    uint8_t u8LastAddress;      // Last device address used
    uint8_t u8LastRegister;     // Last register written
    bool bSimulateTimeout;      // If true, never return data (test timeout)
    bool bSimulateError;        // If true, requestFrom returns 0
} WIRE_MOCK_T;

/**
 * @brief Initialize Wire library (mock)
 */
void Wire_begin(void);

/**
 * @brief Begin I2C transmission (mock)
 * 
 * @param address 7-bit I2C device address
 */
void Wire_beginTransmission(uint8_t address);

/**
 * @brief Write byte to I2C (mock)
 * 
 * In real Wire, this queues data for transmission.
 * In mock, we capture the register address.
 * 
 * @param data Byte to write
 * @return Number of bytes written (always 1)
 */
uint8_t Wire_write(uint8_t data);

/**
 * @brief End I2C transmission (mock)
 * 
 * @param sendStop If false, keeps bus active (repeated start)
 * @return 0 on success, error code otherwise
 */
uint8_t Wire_endTransmission(bool sendStop);

/**
 * @brief Request bytes from I2C device (mock)
 * 
 * @param address 7-bit I2C device address
 * @param quantity Number of bytes to request
 * @return Number of bytes actually received
 */
uint8_t Wire_requestFrom(uint8_t address, uint8_t quantity);

/**
 * @brief Check how many bytes available in receive buffer (mock)
 * 
 * @return Number of bytes available to read
 */
uint8_t Wire_available(void);

/**
 * @brief Read one byte from receive buffer (mock)
 * 
 * @return Next byte from buffer
 */
uint8_t Wire_read(void);

/**
 * @brief Set mock response data
 * 
 * Preload the mock buffer with data that will be returned
 * by Wire_read() calls.
 * 
 * @param data Pointer to data to return
 * @param size Number of bytes
 */
void WireMock_vSetResponse(const uint8_t *data, uint8_t size);

/**
 * @brief Simulate I2C timeout behavior
 * 
 * When enabled, Wire_available() will always return 0
 * (no data available) to test timeout handling.
 * 
 * @param enable true = simulate timeout, false = normal
 */
void WireMock_vSimulateTimeout(bool enable);

/**
 * @brief Simulate I2C error (device not responding)
 * 
 * When enabled, Wire_requestFrom() returns 0 bytes.
 * 
 * @param enable true = simulate error, false = normal
 */
void WireMock_vSimulateError(bool enable);

/**
 * @brief Reset mock to initial state
 * 
 * Clears buffer, resets pointers, disables error simulation.
 */
void WireMock_vReset(void);

/**
 * @brief Get last device address accessed
 * 
 * @return Last address used in Wire_requestFrom or Wire_beginTransmission
 */
uint8_t WireMock_u8GetLastAddress(void);

/**
 * @brief Get last register address written
 * 
 * @return Last byte written via Wire_write (typically register address)
 */
uint8_t WireMock_u8GetLastRegister(void);

#ifdef __cplusplus
}
#endif

#endif // FEATURE_I2C_MEASUREMENT

#endif // I2CMOCK_H
