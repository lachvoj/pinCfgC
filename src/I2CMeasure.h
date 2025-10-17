#ifndef I2CMEASURE_H
#define I2CMEASURE_H

#ifdef FEATURE_I2C_MEASUREMENT

#include "ISensorMeasure.h"
#include "Types.h"
#include <stdint.h>

/**
 * @brief I2C measurement state machine states
 */
typedef enum I2CMEASURE_STATE_E
{
    I2CMEASURE_STATE_IDLE_E = 0,         // No transaction in progress
    I2CMEASURE_STATE_COMMAND_SENT_E,     // Command written (command mode only)
    I2CMEASURE_STATE_WAITING_E,          // Waiting for conversion (command mode only)
    I2CMEASURE_STATE_REQUEST_SENT_E,     // Wire.requestFrom() called
    I2CMEASURE_STATE_READING_E,          // Waiting for data available
    I2CMEASURE_STATE_DATA_READY_E,       // Data read, ready to return
    I2CMEASURE_STATE_ERROR_E             // Error occurred
} I2CMEASURE_STATE_T;

/**
 * @brief I2C measurement structure (non-blocking)
 * 
 * Implements state machine for non-blocking I2C reads.
 * Supports two modes:
 * - Simple mode: Write register address → Read data (cmdLen=1)
 * - Command mode: Write multi-byte command → Wait → Read data (cmdLen=2-3)
 * 
 * Total size: ~32 bytes per instance
 */
typedef struct I2CMEASURE_S
{
    ISENSORMEASURE_T sInterface;     // Interface (eMeasure function pointer + eType) - 12 bytes
    uint8_t u8DeviceAddress;         // I2C device address (7-bit, e.g., 0x48) - 1 byte
    uint8_t au8CommandBytes[3];      // Command sequence: [0]=register OR [0,1,2]=command - 3 bytes
    uint8_t u8CommandLength;         // Command length: 1=simple mode, 2-3=command mode - 1 byte
    uint8_t u8DataSize;              // Number of bytes to read (1-6) - 1 byte
    I2CMEASURE_STATE_T eState;       // Current state machine state - 1 byte
    uint32_t u32RequestTime;         // millis() when request/command started - 4 bytes
    uint16_t u16TimeoutMs;           // Timeout duration (default: 100ms) - 2 bytes
    uint16_t u16ConversionDelayMs;   // Delay after command before read (0=none, 80=AHT10) - 2 bytes
    float fLastValue;                // Cached value when DATA_READY - 4 bytes
    uint8_t au8Buffer[6];            // Raw I2C data buffer (increased for AHT10) - 6 bytes
} I2CMEASURE_T;

/**
 * @brief Initialize I2C measurement instance
 * 
 * @param psHandle Pointer to I2CMEASURE_T structure (must be allocated by caller)
 * @param eType Measurement type (must be MEASUREMENT_TYPE_I2C_E)
 * @param pcName Measurement name (for logging, can be NULL)
 * @param u8DeviceAddress I2C device address (7-bit)
 * @param pu8CommandBytes Pointer to command bytes array (1-3 bytes)
 * @param u8CommandLength Number of command bytes (1=simple, 2-3=command mode)
 * @param u8DataSize Number of bytes to read (1-6)
 * @param u16ConversionDelayMs Delay after command before read (0=none, 80=AHT10)
 * @return PINCFG_RESULT_OK_E on success, error code otherwise
 */
PINCFG_RESULT_T I2CMeasure_eInit(
    I2CMEASURE_T *psHandle,
    MEASUREMENT_TYPE_T eType,
    const char *pcName,
    uint8_t u8DeviceAddress,
    const uint8_t *pu8CommandBytes,
    uint8_t u8CommandLength,
    uint8_t u8DataSize,
    uint16_t u16ConversionDelayMs
);

/**
 * @brief Read measurement from I2C device (non-blocking)
 * 
 * Implements state machine:
 * - IDLE: Start new request, return PENDING
 * - REQUEST_SENT/READING: Check if data available, return PENDING or ERROR
 * - DATA_READY: Return SUCCESS with value
 * - ERROR: Return ERROR
 * 
 * Must be called repeatedly until SUCCESS or ERROR returned.
 * 
 * @param pSelf Pointer to ISENSORMEASURE_T interface (actual type: I2CMEASURE_T*)
 * @param pfValue Output: measurement value (only valid when SUCCESS)
 * @param fOffset Offset to apply to measurement (applied by Sensor, not used here)
 * @param u32ms Current time in milliseconds (not used, kept for interface compatibility)
 * @return ISENSORMEASURE_OK_E when value ready
 *         ISENSORMEASURE_PENDING_E when operation in progress
 *         ISENSORMEASURE_ERROR_E on timeout or error
 *         ISENSORMEASURE_NULLPTR_ERROR_E if parameters NULL
 */
ISENSORMEASURE_RESULT_T I2CMeasure_eMeasure(
    ISENSORMEASURE_T *pSelf,
    float *pfValue,
    const float fOffset,
    uint32_t u32ms
);

/**
 * @brief Get raw measurement bytes (non-blocking, for multi-value sensors)
 * 
 * Returns raw bytes from I2C buffer. Allows sensors to extract specific byte
 * ranges (e.g., temperature vs humidity from AHT10).
 * 
 * @param pSelf Pointer to ISENSORMEASURE_T interface
 * @param pu8Buffer Output buffer for raw bytes (must be at least 6 bytes)
 * @param pu8Size Input/Output: max buffer size in, actual bytes copied out
 * @param u32ms Current time in milliseconds
 * @return ISENSORMEASURE_OK_E when data ready
 *         ISENSORMEASURE_PENDING_E when operation in progress
 *         ISENSORMEASURE_ERROR_E on timeout or error
 */
ISENSORMEASURE_RESULT_T I2CMeasure_eMeasureRaw(
    ISENSORMEASURE_T *pSelf,
    uint8_t *pu8Buffer,
    uint8_t *pu8Size,
    uint32_t u32ms
);

#endif // FEATURE_I2C_MEASUREMENT

#endif // I2CMEASURE_H
