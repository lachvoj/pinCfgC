#ifdef PINCFG_FEATURE_I2C_MEASUREMENT

#include <string.h>

#include "I2CMeasure.h"
#include "PinCfgUtils.h"
#include "SensorMeasure.h"
#include "WireWrapper.h"


#ifdef UNIT_TEST
#include "ArduinoMock.h"
#else
#include <Arduino.h>
#endif

// Default timeout for I2C operations (milliseconds)
#define I2CMEASURE_DEFAULT_TIMEOUT_MS 100

PINCFG_RESULT_T I2CMeasure_eInit(
    I2CMEASURE_T *psHandle,
    STRING_POINT_T *psName,
    uint8_t u8DeviceAddress,
    const uint8_t *pu8CommandBytes,
    uint8_t u8CommandLength,
    uint8_t u8DataSize,
    uint16_t u16ConversionDelayMs)
{
    // Validate parameters
    if (psHandle == NULL || psName == NULL)
    {
        return PINCFG_NULLPTR_ERROR_E;
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

    // Initialize base interface (allocates and copies name)
    if (SensorMeasure_eInit(&psHandle->sInterface, psName, MEASUREMENT_TYPE_I2C_E) != SENSORMEASURE_OK_E)
    {
        return PINCFG_ERROR_E;
    }

    // Set measurement function pointer
    psHandle->sInterface.eMeasure = I2CMeasure_eMeasure;

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

    // Initialize Wire library (safe to call multiple times)
    Wire_vBegin();

    return PINCFG_OK_E;
}

ISENSORMEASURE_RESULT_T I2CMeasure_eMeasure(
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
            psHandle->u32RequestTime = u32ms;
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
            psHandle->u32RequestTime = u32ms;
            psHandle->eState = I2CMEASURE_STATE_COMMAND_SENT_E;
        }
        return ISENSORMEASURE_PENDING_E;

    case I2CMEASURE_STATE_COMMAND_SENT_E:
        // Wait for conversion delay
        if (PinCfg_u32GetElapsedTime(psHandle->u32RequestTime, u32ms) >= psHandle->u16ConversionDelayMs)
        {
            Wire_u8RequestFrom(psHandle->u8DeviceAddress, psHandle->u8DataSize);
            psHandle->u32RequestTime = u32ms;
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
        if (PinCfg_u32GetElapsedTime(psHandle->u32RequestTime, u32ms) > psHandle->u16TimeoutMs)
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
                u8CopySize = *pu8Size; // Limit to buffer size
            }

            for (uint8_t i = 0; i < u8CopySize; i++)
            {
                pu8Buffer[i] = psHandle->au8Buffer[i];
            }

            *pu8Size = u8CopySize; // Return actual bytes copied
        }

        // Reset for next read
        psHandle->eState = I2CMEASURE_STATE_IDLE_E;
        return ISENSORMEASURE_OK_E;

    case I2CMEASURE_STATE_ERROR_E:
    default: psHandle->eState = I2CMEASURE_STATE_IDLE_E; return ISENSORMEASURE_ERROR_E;
    }
}

#endif // PINCFG_FEATURE_I2C_MEASUREMENT
