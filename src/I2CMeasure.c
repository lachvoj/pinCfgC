#ifdef FEATURE_I2C_MEASUREMENT

#include "I2CMeasure.h"
#include "WireWrapper.h"
#include <string.h>

#ifdef UNIT_TEST
#include "ArduinoMock.h"
#else
#include <Arduino.h>
#endif

// Default timeout for I2C operations (milliseconds)
#define I2CMEASURE_DEFAULT_TIMEOUT_MS 100

/**
 * @brief Convert big-endian byte array to float
 * 
 * Handles 1-4 bytes, sign-extends for signed values.
 * 
 * @param pu8Data Pointer to byte array (big-endian)
 * @param u8Size Number of bytes (1-4)
 * @return Converted float value
 */
static float I2CMeasure_fBytesToFloat(const uint8_t *pu8Data, uint8_t u8Size)
{
    int32_t i32Value = 0;

    // Combine bytes (big-endian: MSB first)
    for (uint8_t i = 0; i < u8Size; i++)
    {
        i32Value = (i32Value << 8) | pu8Data[i];
    }

    // Sign-extend for signed values (if MSB is set)
    if (u8Size == 1 && (i32Value & 0x80))
    {
        i32Value |= 0xFFFFFF00; // Sign extend 8-bit
    }
    else if (u8Size == 2 && (i32Value & 0x8000))
    {
        i32Value |= 0xFFFF0000; // Sign extend 16-bit
    }
    else if (u8Size == 3 && (i32Value & 0x800000))
    {
        i32Value |= 0xFF000000; // Sign extend 24-bit
    }
    // 4-byte values don't need sign extension (already 32-bit)

    return (float)i32Value;
}

/**
 * @brief Initialize I2C measurement instance
 */
PINCFG_RESULT_T I2CMeasure_eInit(
    I2CMEASURE_T *psHandle,
    MEASUREMENT_TYPE_T eType,
    const char *pcName,
    uint8_t u8DeviceAddress,
    const uint8_t *pu8CommandBytes,
    uint8_t u8CommandLength,
    uint8_t u8DataSize,
    uint16_t u16ConversionDelayMs)
{
    // Validate parameters
    if (psHandle == NULL)
    {
        return PINCFG_NULLPTR_ERROR_E;
    }

    if (eType != MEASUREMENT_TYPE_I2C_E)
    {
        return PINCFG_ERROR_E;
    }

    if (u8DataSize < 1 || u8DataSize > 6)
    {
        return PINCFG_ERROR_E;
    }

    if (u8CommandLength < 1 || u8CommandLength > 3)
    {
        return PINCFG_ERROR_E;
    }

    if (pu8CommandBytes == NULL)
    {
        return PINCFG_NULLPTR_ERROR_E;
    }

    // Initialize structure
    memset(psHandle, 0, sizeof(I2CMEASURE_T));

    // Set interface
    psHandle->sInterface.eMeasure = I2CMeasure_eMeasure;
    psHandle->sInterface.eMeasureRaw = I2CMeasure_eMeasureRaw;
    psHandle->sInterface.eType = eType;

    // Set I2C configuration
    psHandle->u8DeviceAddress = u8DeviceAddress;
    psHandle->u8DataSize = u8DataSize;
    
    // Copy command bytes
    psHandle->u8CommandLength = u8CommandLength;
    for (uint8_t i = 0; i < u8CommandLength; i++)
    {
        psHandle->au8CommandBytes[i] = pu8CommandBytes[i];
    }

    // Set conversion delay
    psHandle->u16ConversionDelayMs = u16ConversionDelayMs;

    // Initialize state machine
    psHandle->eState = I2CMEASURE_STATE_IDLE_E;
    psHandle->u16TimeoutMs = I2CMEASURE_DEFAULT_TIMEOUT_MS;
    psHandle->u32RequestTime = 0;
    psHandle->fLastValue = 0.0f;

    // Initialize Wire library (safe to call multiple times)
    Wire_vBegin();

    // Note: pcName is not used (no logging in measurement layer)
    (void)pcName;

    return PINCFG_OK_E;
}

/**
 * @brief Get raw measurement bytes (for multi-value sensors)
 * 
 * Returns raw bytes from I2C buffer. Allows sensors to extract
 * specific byte ranges (e.g., temperature vs humidity from AHT10).
 * 
 * @param pSelf Pointer to ISENSORMEASURE_T interface
 * @param pu8Buffer Output buffer for raw bytes (must be at least 6 bytes)
 * @param pu8Size Input/Output: max buffer size in, actual bytes out
 * @param u32ms Current time in milliseconds
 * @return ISENSORMEASURE_OK_E when data ready
 *         ISENSORMEASURE_PENDING_E when operation in progress
 *         ISENSORMEASURE_ERROR_E on timeout or error
 */
ISENSORMEASURE_RESULT_T I2CMeasure_eMeasureRaw(
    ISENSORMEASURE_T *pSelf,
    uint8_t *pu8Buffer,
    uint8_t *pu8Size,
    uint32_t u32ms)
{
    // Validate parameters
    if (pSelf == NULL || pu8Buffer == NULL || pu8Size == NULL)
    {
        return ISENSORMEASURE_NULLPTR_ERROR_E;
    }

    // Cast to actual type
    I2CMEASURE_T *psHandle = (I2CMEASURE_T *)pSelf;

    (void)u32ms;  // Not used

    // Run state machine (same as eMeasure, but don't convert to float)
    // We'll copy the buffer when in DATA_READY state
    
    switch (psHandle->eState)
    {
    case I2CMEASURE_STATE_IDLE_E:
        // Start new transaction (same logic as eMeasure)
        if (psHandle->u8CommandLength == 1)
        {
            // Simple mode
            Wire_vBeginTransmission(psHandle->u8DeviceAddress);
            Wire_u8Write(psHandle->au8CommandBytes[0]);
            Wire_u8EndTransmission(false);
            Wire_u8RequestFrom(psHandle->u8DeviceAddress, psHandle->u8DataSize);
            psHandle->u32RequestTime = millis();
            psHandle->eState = I2CMEASURE_STATE_REQUEST_SENT_E;
        }
        else
        {
            // Command mode
            Wire_vBeginTransmission(psHandle->u8DeviceAddress);
            for (uint8_t i = 0; i < psHandle->u8CommandLength; i++)
            {
                Wire_u8Write(psHandle->au8CommandBytes[i]);
            }
            Wire_u8EndTransmission(true);
            psHandle->u32RequestTime = millis();
            psHandle->eState = I2CMEASURE_STATE_COMMAND_SENT_E;
        }
        return ISENSORMEASURE_PENDING_E;

    case I2CMEASURE_STATE_COMMAND_SENT_E:
        // Wait for conversion delay
        if ((millis() - psHandle->u32RequestTime) >= psHandle->u16ConversionDelayMs)
        {
            Wire_u8RequestFrom(psHandle->u8DeviceAddress, psHandle->u8DataSize);
            psHandle->u32RequestTime = millis();
            psHandle->eState = I2CMEASURE_STATE_REQUEST_SENT_E;
        }
        return ISENSORMEASURE_PENDING_E;

    case I2CMEASURE_STATE_REQUEST_SENT_E:
    case I2CMEASURE_STATE_READING_E:
        // Check if data available
        if (Wire_u8Available() >= psHandle->u8DataSize)
        {
            // Read all bytes
            for (uint8_t i = 0; i < psHandle->u8DataSize; i++)
            {
                psHandle->au8Buffer[i] = Wire_u8Read();
            }
            psHandle->eState = I2CMEASURE_STATE_DATA_READY_E;
            return ISENSORMEASURE_PENDING_E;
        }

        // Check timeout
        if ((millis() - psHandle->u32RequestTime) > psHandle->u16TimeoutMs)
        {
            psHandle->eState = I2CMEASURE_STATE_ERROR_E;
            return ISENSORMEASURE_ERROR_E;
        }

        psHandle->eState = I2CMEASURE_STATE_READING_E;
        return ISENSORMEASURE_PENDING_E;

    case I2CMEASURE_STATE_DATA_READY_E:
        // Copy raw bytes to output buffer
        {
            uint8_t u8CopySize = psHandle->u8DataSize;
            if (u8CopySize > *pu8Size)
            {
                u8CopySize = *pu8Size;  // Limit to buffer size
            }

            for (uint8_t i = 0; i < u8CopySize; i++)
            {
                pu8Buffer[i] = psHandle->au8Buffer[i];
            }

            *pu8Size = u8CopySize;  // Return actual bytes copied
        }

        // Reset for next read
        psHandle->eState = I2CMEASURE_STATE_IDLE_E;
        return ISENSORMEASURE_OK_E;

    case I2CMEASURE_STATE_ERROR_E:
    default:
        psHandle->eState = I2CMEASURE_STATE_IDLE_E;
        return ISENSORMEASURE_ERROR_E;
    }
}

/**
 * @brief Read measurement from I2C device (non-blocking state machine)
 */
ISENSORMEASURE_RESULT_T I2CMeasure_eMeasure(
    ISENSORMEASURE_T *pSelf,
    float *pfValue,
    const float fOffset,
    uint32_t u32ms)
{
    // Validate parameters
    if (pSelf == NULL || pfValue == NULL)
    {
        return ISENSORMEASURE_NULLPTR_ERROR_E;
    }

    // Cast to actual type
    I2CMEASURE_T *psHandle = (I2CMEASURE_T *)pSelf;

    // Note: fOffset is applied by Sensor layer, not here
    // Note: u32ms not needed (using millis() directly)
    (void)fOffset;
    (void)u32ms;

    // State machine
    switch (psHandle->eState)
    {
    case I2CMEASURE_STATE_IDLE_E:
        // Check mode: simple (cmdLen=1) or command (cmdLen=2-3)
        if (psHandle->u8CommandLength == 1)
        {
            // Simple mode: Write register address, then read
            Wire_vBeginTransmission(psHandle->u8DeviceAddress);
            Wire_u8Write(psHandle->au8CommandBytes[0]);  // Register address
            Wire_u8EndTransmission(false); // Keep bus active (repeated start)

            // Request data bytes immediately
            Wire_u8RequestFrom(psHandle->u8DeviceAddress, psHandle->u8DataSize);

            // Record start time for timeout
            psHandle->u32RequestTime = millis();

            // Transition to REQUEST_SENT (skip COMMAND_SENT and WAITING)
            psHandle->eState = I2CMEASURE_STATE_REQUEST_SENT_E;
        }
        else
        {
            // Command mode: Write multi-byte command
            Wire_vBeginTransmission(psHandle->u8DeviceAddress);
            for (uint8_t i = 0; i < psHandle->u8CommandLength; i++)
            {
                Wire_u8Write(psHandle->au8CommandBytes[i]);
            }
            Wire_u8EndTransmission(true); // Release bus after command

            // Record start time for conversion delay
            psHandle->u32RequestTime = millis();

            // Transition to COMMAND_SENT
            psHandle->eState = I2CMEASURE_STATE_COMMAND_SENT_E;
        }

        // Not ready yet
        return ISENSORMEASURE_PENDING_E;

    case I2CMEASURE_STATE_COMMAND_SENT_E:
        // Wait for conversion delay (command mode only)
        if ((millis() - psHandle->u32RequestTime) >= psHandle->u16ConversionDelayMs)
        {
            // Delay complete, now request data
            Wire_u8RequestFrom(psHandle->u8DeviceAddress, psHandle->u8DataSize);

            // Update time for read timeout
            psHandle->u32RequestTime = millis();

            // Transition to REQUEST_SENT
            psHandle->eState = I2CMEASURE_STATE_REQUEST_SENT_E;
        }

        // Still waiting for conversion
        return ISENSORMEASURE_PENDING_E;

    case I2CMEASURE_STATE_REQUEST_SENT_E:
    case I2CMEASURE_STATE_READING_E:
        // Check if all bytes available
        if (Wire_u8Available() >= psHandle->u8DataSize)
        {
            // Read all bytes into buffer
            for (uint8_t i = 0; i < psHandle->u8DataSize; i++)
            {
                psHandle->au8Buffer[i] = Wire_u8Read();
            }

            // Convert bytes to float (big-endian)
            psHandle->fLastValue = I2CMeasure_fBytesToFloat(
                psHandle->au8Buffer,
                psHandle->u8DataSize);

            // Transition to data ready state
            psHandle->eState = I2CMEASURE_STATE_DATA_READY_E;

            // Still need one more call to return the value
            return ISENSORMEASURE_PENDING_E;
        }

        // Check for timeout
        if ((millis() - psHandle->u32RequestTime) > psHandle->u16TimeoutMs)
        {
            // Timeout - reset state and return error
            psHandle->eState = I2CMEASURE_STATE_ERROR_E;
            return ISENSORMEASURE_ERROR_E;
        }

        // Still waiting for data
        psHandle->eState = I2CMEASURE_STATE_READING_E;
        return ISENSORMEASURE_PENDING_E;

    case I2CMEASURE_STATE_DATA_READY_E:
        // Return cached value
        *pfValue = psHandle->fLastValue;

        // Reset state machine for next request
        psHandle->eState = I2CMEASURE_STATE_IDLE_E;

        // Success!
        return ISENSORMEASURE_OK_E;

    case I2CMEASURE_STATE_ERROR_E:
    default:
        // Reset state and return error
        psHandle->eState = I2CMEASURE_STATE_IDLE_E;
        return ISENSORMEASURE_ERROR_E;
    }
}

#endif // FEATURE_I2C_MEASUREMENT
