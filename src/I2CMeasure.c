#ifdef PINCFG_FEATURE_I2C_MEASUREMENT

#include <string.h>

#include "I2CMeasure.h"
#include "PinCfgUtils.h"
#include "SensorMeasure.h"

// Platform detection: use STM32I2CDriver on STM32, WireWrapper elsewhere
#if !defined(UNIT_TEST) && \
    (defined(ARDUINO_ARCH_STM32) || defined(ARDUINO_ARCH_STM32F1) || \
     defined(ARDUINO_ARCH_STM32F4) || defined(ARDUINO_ARCH_STM32L4) || \
     defined(STM32F1) || defined(STM32F103xB))
#define USE_STM32_I2C_DRIVER
#include "STM32I2CDriver.h"
#else
#include "WireWrapper.h"
#endif


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
    uint16_t u16ConversionDelayMs,
    uint16_t u16CacheValidMs)
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

    // Initialize cache
    psHandle->u32CacheTimestamp = 0;       // 0 = cache invalid
    psHandle->u16CacheValidMs = u16CacheValidMs; // 0 = disabled

    // Initialize I2C (safe to call multiple times)
#ifdef USE_STM32_I2C_DRIVER
    STM32_I2C_vInit();
#else
    Wire_vBegin();
#endif

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

    // Check cache validity (if caching enabled)
    if (psHandle->u16CacheValidMs > 0 &&
        psHandle->u32CacheTimestamp != 0 &&
        PinCfg_u32GetElapsedTime(psHandle->u32CacheTimestamp, u32ms) < psHandle->u16CacheValidMs)
    {
        // Return cached data
        uint8_t u8CopySize = psHandle->u8DataSize;
        if (u8CopySize > *pu8Size)
        {
            u8CopySize = *pu8Size;
        }
        for (uint8_t i = 0; i < u8CopySize; i++)
        {
            pu8Buffer[i] = psHandle->au8Buffer[i];
        }
        *pu8Size = u8CopySize;
        return ISENSORMEASURE_OK_E;
    }

#ifdef USE_STM32_I2C_DRIVER
    // STM32 async state machine using STM32I2CDriver
    switch (psHandle->eState)
    {
    case I2CMEASURE_STATE_IDLE_E:
        // Start new transaction - first check if driver is available
        if (!STM32_I2C_bIsIdle())
        {
            // Driver busy with another measurement, retry next loop
            return ISENSORMEASURE_PENDING_E;
        }
        STM32_I2C_vReset();
        {
            STM32_I2C_RESULT_T eResult;
            if (psHandle->u8CommandLength == 1)
            {
                // Simple mode: write register address, no stop (repeated start follows)
                eResult = STM32_I2C_eWriteAsync(psHandle->u8DeviceAddress, 
                                                 psHandle->au8CommandBytes, 1, false);
            }
            else
            {
                // Command mode: write full command sequence with stop
                eResult = STM32_I2C_eWriteAsync(psHandle->u8DeviceAddress,
                                                 psHandle->au8CommandBytes,
                                                 psHandle->u8CommandLength, true);
            }
            
            if (eResult == STM32_I2C_BUSY_E)
            {
                // Driver became busy between check and call, retry next loop
                return ISENSORMEASURE_PENDING_E;
            }
            else if (eResult != STM32_I2C_OK_E)
            {
                psHandle->eState = I2CMEASURE_STATE_ERROR_E;
                return ISENSORMEASURE_ERROR_E;
            }
        }
        psHandle->u32RequestTime = u32ms;
        psHandle->eState = I2CMEASURE_STATE_COMMAND_SENT_E;
        return ISENSORMEASURE_PENDING_E;

    case I2CMEASURE_STATE_COMMAND_SENT_E:
        // Wait for TX to complete
        if (!STM32_I2C_bIsIdle())
        {
            if (PinCfg_u32GetElapsedTime(psHandle->u32RequestTime, u32ms) > psHandle->u16TimeoutMs)
            {
                psHandle->eState = I2CMEASURE_STATE_ERROR_E;
                return ISENSORMEASURE_ERROR_E;
            }
            return ISENSORMEASURE_PENDING_E;
        }

        // Check for TX errors
        if (STM32_I2C_eGetResult() != STM32_I2C_OK_E)
        {
            psHandle->eState = I2CMEASURE_STATE_ERROR_E;
            return ISENSORMEASURE_ERROR_E;
        }

        // Simple mode: immediately request data
        // Command mode: wait for conversion delay first
        if (psHandle->u8CommandLength == 1)
        {
            STM32_I2C_vReset();
            STM32_I2C_RESULT_T eReadResult = STM32_I2C_eReadAsync(psHandle->u8DeviceAddress, psHandle->u8DataSize);
            if (eReadResult == STM32_I2C_BUSY_E)
            {
                // Driver busy, stay in current state and retry
                return ISENSORMEASURE_PENDING_E;
            }
            else if (eReadResult != STM32_I2C_OK_E)
            {
                psHandle->eState = I2CMEASURE_STATE_ERROR_E;
                return ISENSORMEASURE_ERROR_E;
            }
            psHandle->u32RequestTime = u32ms;
            psHandle->eState = I2CMEASURE_STATE_REQUEST_SENT_E;
        }
        else
        {
            psHandle->eState = I2CMEASURE_STATE_WAITING_E;
        }
        return ISENSORMEASURE_PENDING_E;

    case I2CMEASURE_STATE_WAITING_E:
        // Wait for conversion delay (command mode only)
        if (PinCfg_u32GetElapsedTime(psHandle->u32RequestTime, u32ms) >= psHandle->u16ConversionDelayMs)
        {
            STM32_I2C_vReset();
            STM32_I2C_RESULT_T eReadResult = STM32_I2C_eReadAsync(psHandle->u8DeviceAddress, psHandle->u8DataSize);
            if (eReadResult == STM32_I2C_BUSY_E)
            {
                // Driver busy with another transaction, stay in WAITING and retry
                return ISENSORMEASURE_PENDING_E;
            }
            else if (eReadResult != STM32_I2C_OK_E)
            {
                psHandle->eState = I2CMEASURE_STATE_ERROR_E;
                return ISENSORMEASURE_ERROR_E;
            }
            psHandle->u32RequestTime = u32ms;
            psHandle->eState = I2CMEASURE_STATE_REQUEST_SENT_E;
        }
        return ISENSORMEASURE_PENDING_E;

    case I2CMEASURE_STATE_REQUEST_SENT_E:
    case I2CMEASURE_STATE_READING_E:
        // Wait for RX to complete
        if (!STM32_I2C_bIsIdle())
        {
            if (PinCfg_u32GetElapsedTime(psHandle->u32RequestTime, u32ms) > psHandle->u16TimeoutMs)
            {
                psHandle->eState = I2CMEASURE_STATE_ERROR_E;
                return ISENSORMEASURE_ERROR_E;
            }
            return ISENSORMEASURE_PENDING_E;
        }

        // Check for RX errors
        if (STM32_I2C_eGetResult() != STM32_I2C_OK_E)
        {
            psHandle->eState = I2CMEASURE_STATE_ERROR_E;
            return ISENSORMEASURE_ERROR_E;
        }

        // Check if data is available
        if (STM32_I2C_u8GetRxCount() >= psHandle->u8DataSize)
        {
            STM32_I2C_u8ReadData(psHandle->au8Buffer, psHandle->u8DataSize);
            psHandle->eState = I2CMEASURE_STATE_DATA_READY_E;
            return ISENSORMEASURE_PENDING_E;
        }

        psHandle->eState = I2CMEASURE_STATE_ERROR_E;
        return ISENSORMEASURE_ERROR_E;

    case I2CMEASURE_STATE_DATA_READY_E:
        {
            uint8_t u8CopySize = psHandle->u8DataSize;
            if (u8CopySize > *pu8Size)
            {
                u8CopySize = *pu8Size;
            }
            for (uint8_t i = 0; i < u8CopySize; i++)
            {
                pu8Buffer[i] = psHandle->au8Buffer[i];
            }
            *pu8Size = u8CopySize;

            // Update cache timestamp if caching enabled
            if (psHandle->u16CacheValidMs > 0)
            {
                psHandle->u32CacheTimestamp = u32ms;
            }
        }
        psHandle->eState = I2CMEASURE_STATE_IDLE_E;
        return ISENSORMEASURE_OK_E;

    case I2CMEASURE_STATE_ERROR_E:
    default:
        STM32_I2C_vReset();
        psHandle->eState = I2CMEASURE_STATE_IDLE_E;
        return ISENSORMEASURE_ERROR_E;
    }

#else
    // Non-STM32: use WireWrapper (Arduino Wire) with state machine for testability
    switch (psHandle->eState)
    {
    case I2CMEASURE_STATE_IDLE_E:
        // Start new transaction
        if (psHandle->u8CommandLength == 1)
        {
            // Simple mode: write register address
            Wire_vBeginTransmission(psHandle->u8DeviceAddress);
            Wire_u8Write(psHandle->au8CommandBytes[0]);
            Wire_u8EndTransmission(false); // No stop, repeated start follows
            
            // Request data (blocking)
            Wire_u8RequestFrom(psHandle->u8DeviceAddress, psHandle->u8DataSize);
            psHandle->u32RequestTime = u32ms;
            psHandle->eState = I2CMEASURE_STATE_REQUEST_SENT_E;
        }
        else
        {
            // Command mode: write full command sequence
            Wire_vBeginTransmission(psHandle->u8DeviceAddress);
            for (uint8_t i = 0; i < psHandle->u8CommandLength; i++)
            {
                Wire_u8Write(psHandle->au8CommandBytes[i]);
            }
            Wire_u8EndTransmission(true); // With stop
            psHandle->u32RequestTime = u32ms;
            psHandle->eState = I2CMEASURE_STATE_COMMAND_SENT_E;
        }
        return ISENSORMEASURE_PENDING_E;

    case I2CMEASURE_STATE_COMMAND_SENT_E:
        // Command mode: wait for conversion delay
        if (PinCfg_u32GetElapsedTime(psHandle->u32RequestTime, u32ms) >= psHandle->u16ConversionDelayMs)
        {
            // Request data
            Wire_u8RequestFrom(psHandle->u8DeviceAddress, psHandle->u8DataSize);
            psHandle->u32RequestTime = u32ms;
            psHandle->eState = I2CMEASURE_STATE_REQUEST_SENT_E;
        }
        return ISENSORMEASURE_PENDING_E;

    case I2CMEASURE_STATE_REQUEST_SENT_E:
    case I2CMEASURE_STATE_READING_E:
        // Check for timeout
        if (PinCfg_u32GetElapsedTime(psHandle->u32RequestTime, u32ms) > psHandle->u16TimeoutMs)
        {
            psHandle->eState = I2CMEASURE_STATE_ERROR_E;
            return ISENSORMEASURE_ERROR_E;
        }
        
        // Check if data is available
        if (Wire_u8Available() >= psHandle->u8DataSize)
        {
            for (uint8_t i = 0; i < psHandle->u8DataSize; i++)
            {
                psHandle->au8Buffer[i] = Wire_u8Read();
            }
            psHandle->eState = I2CMEASURE_STATE_DATA_READY_E;
            return ISENSORMEASURE_PENDING_E;
        }
        return ISENSORMEASURE_PENDING_E;

    case I2CMEASURE_STATE_DATA_READY_E:
        {
            uint8_t u8CopySize = psHandle->u8DataSize;
            if (u8CopySize > *pu8Size)
            {
                u8CopySize = *pu8Size;
            }
            for (uint8_t i = 0; i < u8CopySize; i++)
            {
                pu8Buffer[i] = psHandle->au8Buffer[i];
            }
            *pu8Size = u8CopySize;

            // Update cache timestamp if caching enabled
            if (psHandle->u16CacheValidMs > 0)
            {
                psHandle->u32CacheTimestamp = u32ms;
            }
        }
        psHandle->eState = I2CMEASURE_STATE_IDLE_E;
        return ISENSORMEASURE_OK_E;

    case I2CMEASURE_STATE_ERROR_E:
    default:
        psHandle->eState = I2CMEASURE_STATE_IDLE_E;
        return ISENSORMEASURE_ERROR_E;
    }
#endif
}

#endif // PINCFG_FEATURE_I2C_MEASUREMENT
