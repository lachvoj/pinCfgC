#ifdef PINCFG_FEATURE_SPI_MEASUREMENT

#include <string.h>

#include "PinCfgUtils.h"
#include "SPIMeasure.h"
#include "SPIWrapper.h"
#include "SensorMeasure.h"


#ifdef UNIT_TEST
#include "ArduinoMock.h"
#else
#include <Arduino.h>
#endif

// Default timeout for SPI operations (milliseconds)
#define SPIMEASURE_DEFAULT_TIMEOUT_MS 100

PINCFG_RESULT_T SPIMeasure_eInit(
    SPIMEASURE_T *psHandle,
    STRING_POINT_T *psName,
    uint8_t u8ChipSelectPin,
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

    if (u8DataSize < 1 || u8DataSize > 8)
    {
        return PINCFG_ERROR_E;
    }

    if (u8CommandLength > 4)
    {
        return PINCFG_ERROR_E;
    }

    // Command bytes required if command length > 0
    if (u8CommandLength > 0 && pu8CommandBytes == NULL)
    {
        return PINCFG_NULLPTR_ERROR_E;
    }

    // Initialize structure
    memset(psHandle, 0, sizeof(SPIMEASURE_T));

    // Initialize base interface (allocates and copies name)
    if (SensorMeasure_eInit(&psHandle->sInterface, psName, MEASUREMENT_TYPE_SPI_E) != SENSORMEASURE_OK_E)
    {
        return PINCFG_ERROR_E;
    }

    // Set measurement function pointer
    psHandle->sInterface.eMeasure = SPIMeasure_eMeasure;

    // Set SPI configuration
    psHandle->u8ChipSelectPin = u8ChipSelectPin;
    psHandle->u8DataSize = u8DataSize;

    // Copy command bytes if provided
    psHandle->u8CommandLength = u8CommandLength;
    if (u8CommandLength > 0)
    {
        for (uint8_t i = 0; i < u8CommandLength; i++)
        {
            psHandle->au8CommandBytes[i] = pu8CommandBytes[i];
        }
    }

    // Set conversion delay
    psHandle->u16ConversionDelayMs = u16ConversionDelayMs;

    // Initialize state machine
    psHandle->eState = SPIMEASURE_STATE_IDLE_E;
    psHandle->u16TimeoutMs = SPIMEASURE_DEFAULT_TIMEOUT_MS;
    psHandle->u32RequestTime = 0;

    // Initialize SPI library (safe to call multiple times)
    SPI_vBegin();

    // Configure CS pin as output and set HIGH (inactive)
    pinMode(psHandle->u8ChipSelectPin, OUTPUT);
    digitalWrite(psHandle->u8ChipSelectPin, HIGH);

    return PINCFG_OK_E;
}

ISENSORMEASURE_RESULT_T SPIMeasure_eMeasure(
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
    SPIMEASURE_T *psHandle = (SPIMEASURE_T *)pSelf;

    // Run state machine
    switch (psHandle->eState)
    {
    case SPIMEASURE_STATE_IDLE_E:
        // Start new transaction
        // Assert CS (active low)
        digitalWrite(psHandle->u8ChipSelectPin, LOW);

        if (psHandle->u8CommandLength == 0)
        {
            // Simple mode: directly read data
            for (uint8_t i = 0; i < psHandle->u8DataSize; i++)
            {
                psHandle->au8Buffer[i] = SPI_u8Transfer(0xFF); // Send dummy bytes to clock out data
            }

            // Deassert CS
            digitalWrite(psHandle->u8ChipSelectPin, HIGH);

            psHandle->eState = SPIMEASURE_STATE_DATA_READY_E;
        }
        else
        {
            // Command mode: write command first
            for (uint8_t i = 0; i < psHandle->u8CommandLength; i++)
            {
                SPI_u8Transfer(psHandle->au8CommandBytes[i]);
            }

            psHandle->u32RequestTime = u32ms;

            if (psHandle->u16ConversionDelayMs > 0)
            {
                // Need to wait for conversion
                // Deassert CS during wait (sensor-dependent, but common pattern)
                digitalWrite(psHandle->u8ChipSelectPin, HIGH);
                psHandle->eState = SPIMEASURE_STATE_COMMAND_SENT_E;
            }
            else
            {
                // Read immediately after command
                psHandle->eState = SPIMEASURE_STATE_READING_E;
            }
        }
        return ISENSORMEASURE_PENDING_E;

    case SPIMEASURE_STATE_COMMAND_SENT_E:
        // Wait for conversion delay
        if (PinCfg_u32GetElapsedTime(psHandle->u32RequestTime, u32ms) >= psHandle->u16ConversionDelayMs)
        {
            // Re-assert CS and read data
            digitalWrite(psHandle->u8ChipSelectPin, LOW);
            psHandle->eState = SPIMEASURE_STATE_READING_E;
        }

        // Check timeout
        if (PinCfg_u32GetElapsedTime(psHandle->u32RequestTime, u32ms) > psHandle->u16TimeoutMs)
        {
            digitalWrite(psHandle->u8ChipSelectPin, HIGH);
            psHandle->eState = SPIMEASURE_STATE_ERROR_E;
            return ISENSORMEASURE_ERROR_E;
        }

        return ISENSORMEASURE_PENDING_E;

    case SPIMEASURE_STATE_READING_E:
        // Read data bytes
        for (uint8_t i = 0; i < psHandle->u8DataSize; i++)
        {
            psHandle->au8Buffer[i] = SPI_u8Transfer(0xFF); // Send dummy bytes
        }

        // Deassert CS
        digitalWrite(psHandle->u8ChipSelectPin, HIGH);

        psHandle->eState = SPIMEASURE_STATE_DATA_READY_E;
        return ISENSORMEASURE_PENDING_E;

    case SPIMEASURE_STATE_DATA_READY_E:
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
        psHandle->eState = SPIMEASURE_STATE_IDLE_E;
        return ISENSORMEASURE_OK_E;

    case SPIMEASURE_STATE_ERROR_E:
    default:
        // Ensure CS is deasserted
        digitalWrite(psHandle->u8ChipSelectPin, HIGH);
        psHandle->eState = SPIMEASURE_STATE_IDLE_E;
        return ISENSORMEASURE_ERROR_E;
    }
}

#endif // PINCFG_FEATURE_SPI_MEASUREMENT
