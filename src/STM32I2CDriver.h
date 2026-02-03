/**
 * @file STM32I2CDriver.h
 * @brief STM32 I2C driver using LL (Low-Level) library with interrupt support
 *
 * This driver provides non-blocking I2C operations for STM32F1 using interrupts.
 * It's designed to be used by I2CMeasure on STM32 platforms instead of Arduino Wire.
 */

#ifndef STM32_I2C_DRIVER_H
#define STM32_I2C_DRIVER_H

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdbool.h>
#include <stdint.h>

    /**
     * @brief I2C operation result codes
     */
    typedef enum STM32_I2C_RESULT_E
    {
        STM32_I2C_OK_E = 0,       ///< Operation successful
        STM32_I2C_BUSY_E,         ///< Bus is busy with another operation
        STM32_I2C_NACK_E,         ///< Device did not acknowledge
        STM32_I2C_BUS_ERROR_E,    ///< Bus error detected
        STM32_I2C_ARB_LOST_E,     ///< Arbitration lost
        STM32_I2C_TIMEOUT_E,      ///< Operation timed out
        STM32_I2C_INVALID_PARAM_E ///< Invalid parameter
    } STM32_I2C_RESULT_T;

    /**
     * @brief Initialize I2C1 peripheral
     *
     * Configures I2C1 on PB6 (SCL) and PB7 (SDA) at 100kHz standard mode.
     * Sets up NVIC for interrupt-driven operation.
     */
    void STM32_I2C_vInit(void);

    /**
     * @brief Check if I2C is ready for a new operation
     * @return true if idle/complete/error, false if operation in progress
     */
    bool STM32_I2C_bIsIdle(void);

    /**
     * @brief Get the result of the last operation
     * @return Result code from last completed operation
     */
    STM32_I2C_RESULT_T STM32_I2C_eGetResult(void);

    /**
     * @brief Start a write operation (non-blocking)
     *
     * Initiates I2C write to specified address. Returns immediately.
     * Poll STM32_I2C_bIsIdle() for completion.
     *
     * @param u8Address 7-bit I2C device address
     * @param pu8Data Pointer to data buffer (must remain valid until complete)
     * @param u8Length Number of bytes to write
     * @param bSendStop Whether to send STOP condition after transfer
     * @return STM32_I2C_OK_E if started, STM32_I2C_BUSY_E if busy
     */
    STM32_I2C_RESULT_T STM32_I2C_eWriteAsync(
        uint8_t u8Address,
        const uint8_t *pu8Data,
        uint8_t u8Length,
        bool bSendStop);

    /**
     * @brief Start a read operation (non-blocking)
     *
     * Initiates I2C read from specified address. Returns immediately.
     * Poll STM32_I2C_bIsIdle() for completion, then use STM32_I2C_u8GetRxCount()
     * and STM32_I2C_vReadData() to retrieve received data.
     *
     * @param u8Address 7-bit I2C device address
     * @param u8Length Number of bytes to read (max 32)
     * @return STM32_I2C_OK_E if started, STM32_I2C_BUSY_E if busy
     */
    STM32_I2C_RESULT_T STM32_I2C_eReadAsync(uint8_t u8Address, uint8_t u8Length);

    /**
     * @brief Get number of bytes received after read operation
     * @return Number of bytes available in receive buffer
     */
    uint8_t STM32_I2C_u8GetRxCount(void);

    /**
     * @brief Copy received data to user buffer
     * @param pu8Buffer Destination buffer
     * @param u8MaxLength Maximum bytes to copy
     * @return Number of bytes actually copied
     */
    uint8_t STM32_I2C_u8ReadData(uint8_t *pu8Buffer, uint8_t u8MaxLength);

    /**
     * @brief Reset driver state for next operation
     *
     * Call this after handling a completed or error state before starting
     * a new operation.
     */
    void STM32_I2C_vReset(void);

#ifdef __cplusplus
}
#endif

#endif // STM32_I2C_DRIVER_H
