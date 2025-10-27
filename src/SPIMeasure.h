#ifndef SPIMEASURE_H
#define SPIMEASURE_H

#ifdef FEATURE_SPI_MEASUREMENT

#include "ISensorMeasure.h"
#include "Types.h"
#include "PinCfgCsv.h"
#include <stdint.h>

/**
 * @brief SPI measurement state machine states
 */
typedef enum SPIMEASURE_STATE_E
{
    SPIMEASURE_STATE_IDLE_E = 0,         // No transaction in progress
    SPIMEASURE_STATE_COMMAND_SENT_E,     // Command written (command mode only)
    SPIMEASURE_STATE_WAITING_E,          // Waiting for conversion (command mode only)
    SPIMEASURE_STATE_READING_E,          // Reading data
    SPIMEASURE_STATE_DATA_READY_E,       // Data read, ready to return
    SPIMEASURE_STATE_ERROR_E             // Error occurred
} SPIMEASURE_STATE_T;

/**
 * @brief SPI measurement structure (non-blocking)
 * 
 * Implements state machine for non-blocking SPI reads.
 * Supports two modes:
 * - Simple mode: Assert CS → Read data → Deassert CS (cmdLen=0)
 * - Command mode: Assert CS → Write command → Wait → Read data → Deassert CS (cmdLen=1-4)
 * 
 * Returns raw bytes in big-endian format. Sensor layer handles byte extraction and conversion.
 * 
 * Common SPI sensors:
 * - MAX31855/MAX6675: Thermocouple (simple mode, 2-4 bytes)
 * - BME680: Environmental sensor (command mode)
 * - ADXL345: Accelerometer (command mode)
 * - MPU9250: IMU (command mode)
 * 
 * Total size: ~32 bytes per instance
 */
typedef struct SPIMEASURE_S
{
    ISENSORMEASURE_T sInterface;     // Interface (eMeasure function pointer + eType + pcName) - 12/16 bytes
    uint8_t u8ChipSelectPin;         // CS/SS pin number (e.g., 10) - 1 byte
    uint8_t au8CommandBytes[4];      // Command sequence: empty=simple mode, 1-4 bytes=command mode - 4 bytes
    uint8_t u8CommandLength;         // Command length: 0=simple mode, 1-4=command mode - 1 byte
    uint8_t u8DataSize;              // Number of bytes to read (1-8) - 1 byte
    SPIMEASURE_STATE_T eState;       // Current state machine state - 1 byte
    uint32_t u32RequestTime;         // millis() when request/command started - 4 bytes
    uint16_t u16TimeoutMs;           // Timeout duration (default: 100ms) - 2 bytes
    uint16_t u16ConversionDelayMs;   // Delay after command before read (0=none, 10=typical) - 2 bytes
    uint8_t au8Buffer[8];            // Raw SPI data buffer (up to 8 bytes for most sensors) - 8 bytes
} SPIMEASURE_T;

/**
 * @brief Initialize SPI measurement instance
 * 
 * @param psHandle Pointer to SPIMEASURE_T structure (must be allocated by caller)
 * @param psName Pointer to STRING_POINT_T containing name (will be copied)
 * @param u8ChipSelectPin GPIO pin number for chip select (CS/SS)
 * @param pu8CommandBytes Pointer to command bytes array (0-4 bytes, NULL for simple mode)
 * @param u8CommandLength Number of command bytes (0=simple, 1-4=command mode)
 * @param u8DataSize Number of bytes to read (1-8)
 * @param u16ConversionDelayMs Delay after command before read (0=none, 10=typical)
 * @return PINCFG_RESULT_OK_E on success, error code otherwise
 */
PINCFG_RESULT_T SPIMeasure_eInit(
    SPIMEASURE_T *psHandle,
    STRING_POINT_T *psName,
    uint8_t u8ChipSelectPin,
    const uint8_t *pu8CommandBytes,
    uint8_t u8CommandLength,
    uint8_t u8DataSize,
    uint16_t u16ConversionDelayMs
);

/**
 * @brief Read measurement from SPI device (non-blocking)
 * 
 * Returns raw bytes from SPI buffer in big-endian format.
 * Sensor layer handles byte extraction and conversion to float.
 * 
 * Implements state machine:
 * - IDLE: Start new request, return PENDING
 * - COMMAND_SENT: Wait for conversion delay
 * - READING: Read data from SPI
 * - DATA_READY: Return SUCCESS with raw bytes
 * - ERROR: Return ERROR
 * 
 * Must be called repeatedly until SUCCESS or ERROR returned.
 * 
 * @param pSelf Pointer to ISENSORMEASURE_T interface (actual type: SPIMEASURE_T*)
 * @param pu8Buffer Output buffer for raw bytes (must be at least 8 bytes)
 * @param pu8Size Input/Output: max buffer size in, actual bytes copied out
 * @param u32ms Current time in milliseconds (not used, kept for interface compatibility)
 * @return ISENSORMEASURE_OK_E when data ready
 *         ISENSORMEASURE_PENDING_E when operation in progress
 *         ISENSORMEASURE_ERROR_E on timeout or error
 *         ISENSORMEASURE_NULLPTR_ERROR_E if parameters NULL
 */
ISENSORMEASURE_RESULT_T SPIMeasure_eMeasure(
    ISENSORMEASURE_T *pSelf,
    uint8_t *pu8Buffer,
    uint8_t *pu8Size,
    uint32_t u32ms
);

#endif // FEATURE_SPI_MEASUREMENT

#endif // SPIMEASURE_H
