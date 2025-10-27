#ifndef SPIMOCK_H
#define SPIMOCK_H

#ifdef FEATURE_SPI_MEASUREMENT

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief SPI library mock for testing
 * 
 * Simulates Arduino SPI library behavior for unit testing
 * without requiring actual SPI hardware.
 */

/**
 * @brief Mock state structure
 */
typedef struct
{
    uint8_t au8ResponseBuffer[32];  // Simulated SPI response data
    uint8_t u8ResponseSize;         // Number of bytes in response buffer
    uint8_t u8ResponseIndex;        // Current read position in response
    uint8_t au8LastTxData[32];      // Last transmitted data (for verification)
    uint8_t u8LastTxSize;           // Size of last transmission
    uint8_t u8Mode;                 // Current SPI mode
    uint32_t u32ClockHz;            // Current clock speed
    bool bSimulateError;            // If true, simulate transfer errors
    uint32_t u32TransferCount;      // Number of transfers performed
} SPI_MOCK_T;

/**
 * @brief Initialize SPI library (mock)
 */
void SPI_begin(void);

/**
 * @brief Transfer single byte via SPI (mock)
 * 
 * Returns next byte from response buffer, or 0xFF if buffer exhausted.
 * Stores transmitted byte in TX history.
 * 
 * @param data Byte to send
 * @return Simulated byte received
 */
uint8_t SPI_transfer(uint8_t data);

/**
 * @brief Transfer multiple bytes via SPI (mock)
 * 
 * @param txBuffer Pointer to data to send
 * @param rxBuffer Pointer to buffer for received data
 * @param length Number of bytes to transfer
 */
void SPI_transferBytes(uint8_t *txBuffer, uint8_t *rxBuffer, uint8_t length);

/**
 * @brief Set SPI configuration (mock)
 * 
 * @param mode SPI mode (0-3)
 * @param clockHz Clock speed in Hz
 */
void SPI_setConfig(uint8_t mode, uint32_t clockHz);

/**
 * @brief End SPI operations (mock)
 */
void SPI_end(void);

/**
 * @brief Set mock response data
 * 
 * Preload the mock buffer with data that will be returned
 * by SPI_transfer() calls.
 * 
 * Example:
 * @code
 * uint8_t response[] = {0x12, 0x34, 0x56, 0x78};
 * SPIMock_vSetResponse(response, 4);
 * @endcode
 * 
 * @param data Pointer to data to return
 * @param size Number of bytes
 */
void SPIMock_vSetResponse(const uint8_t *data, uint8_t size);

/**
 * @brief Simulate SPI error behavior
 * 
 * When enabled, SPI_transfer() returns 0x00 instead of response data.
 * 
 * @param enable true = simulate error, false = normal
 */
void SPIMock_vSimulateError(bool enable);

/**
 * @brief Reset mock to initial state
 * 
 * Clears buffers, resets counters, disables error simulation.
 */
void SPIMock_vReset(void);

/**
 * @brief Get last transmitted data
 * 
 * Returns pointer to internal buffer containing last TX data.
 * Valid until next SPI operation.
 * 
 * @param pSize Output: size of TX data
 * @return Pointer to TX data buffer
 */
const uint8_t* SPIMock_pu8GetLastTxData(uint8_t *pSize);

/**
 * @brief Get number of SPI transfers performed
 * 
 * Useful for verifying expected number of SPI operations.
 * 
 * @return Total transfer count since last reset
 */
uint32_t SPIMock_u32GetTransferCount(void);

/**
 * @brief Get current SPI mode setting
 * 
 * @return Current mode (0-3)
 */
uint8_t SPIMock_u8GetMode(void);

/**
 * @brief Get current SPI clock speed setting
 * 
 * @return Current clock in Hz
 */
uint32_t SPIMock_u32GetClockHz(void);

#ifdef __cplusplus
}
#endif

#endif // FEATURE_SPI_MEASUREMENT

#endif // SPIMOCK_H
