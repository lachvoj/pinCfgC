/**
 * @file WireWrapper.cpp
 * @brief C++ implementation of Wire wrapper for C code
 * 
 * Supports three modes:
 * 1. UNIT_TEST - Uses I2C mock for testing
 * 2. STM32 HAL - Uses STM32 HAL I2C functions (auto-detected via ARDUINO_ARCH_STM32*)
 * 3. Default - Uses Arduino Wire library
 * 
 * STM32 HAL mode is automatically enabled when building for STM32 platforms:
 * - ARDUINO_ARCH_STM32F1, ARDUINO_ARCH_STM32F4, etc.
 * - Or manually with USE_STM32_HAL_I2C flag
 */

#ifdef FEATURE_I2C_MEASUREMENT

#include "WireWrapper.h"
#include <stddef.h>  // For NULL

// Detect STM32 HAL mode (but not in unit test mode)
#if !defined(UNIT_TEST) && ( \
    defined(ARDUINO_ARCH_STM32) || defined(ARDUINO_ARCH_STM32F1) || \
    defined(ARDUINO_ARCH_STM32F4) || defined(ARDUINO_ARCH_STM32L4) || \
    defined(USE_STM32_HAL_I2C) \
)
    #define USE_STM32_HAL_MODE
#endif

// Include appropriate I2C implementation
#ifdef UNIT_TEST
    #include "I2CMock.h"
#elif defined(USE_STM32_HAL_MODE)
    // STM32 HAL I2C support
    #include "stm32_def.h"  // Platform definitions
    
    // Default I2C handle (can be overridden by defining WIRE_I2C_HANDLE)
    #ifndef WIRE_I2C_HANDLE
        #define WIRE_I2C_HANDLE hi2c1
    #endif
    
    // Default I2C timeout in milliseconds
    #ifndef WIRE_I2C_TIMEOUT_MS
        #define WIRE_I2C_TIMEOUT_MS 100
    #endif
    
    // External I2C handle (must be defined in main.c or similar)
    // Provide a weak default to avoid linker errors if not defined
    extern I2C_HandleTypeDef WIRE_I2C_HANDLE;
    
    // Weak default definition - can be overridden by user code
    __attribute__((weak)) I2C_HandleTypeDef hi2c1;
    
    // Pointer to the actual I2C handle to use (allows runtime configuration)
    static I2C_HandleTypeDef *g_psI2CHandle = &WIRE_I2C_HANDLE;
    
    // HAL state tracking for write operations
    static uint8_t g_u8CurrentAddress = 0;
    static uint8_t g_au8TxBuffer[32];
    static uint8_t g_u8TxBufferLen = 0;
    static uint8_t g_au8RxBuffer[32];
    static uint8_t g_u8RxBufferLen = 0;
    static uint8_t g_u8RxReadIndex = 0;
#else
    #include <Wire.h>
#endif

extern "C"
{

#ifdef USE_STM32_HAL_MODE
/**
 * @brief Set I2C handle to use for Wire operations
 * 
 * Call this function to configure which I2C peripheral to use.
 * Must be called before Wire_vBegin() if not using the default hi2c1.
 * 
 * @param psHandle Pointer to initialized I2C_HandleTypeDef
 */
void Wire_vSetI2CHandle(void *psHandle)
{
    if (psHandle != NULL)
    {
        g_psI2CHandle = (I2C_HandleTypeDef *)psHandle;
    }
}
#endif

void Wire_vBegin(void)
{
#ifdef UNIT_TEST
    Wire_begin();
#elif defined(USE_STM32_HAL_MODE)
    // I2C is initialized via HAL_I2C_Init() in main.c
    // Reset state variables
    g_u8CurrentAddress = 0;
    g_u8TxBufferLen = 0;
    g_u8RxBufferLen = 0;
    g_u8RxReadIndex = 0;
#else
    Wire.begin();
#endif
}

void Wire_vBeginTransmission(uint8_t u8Address)
{
#ifdef UNIT_TEST
    Wire_beginTransmission(u8Address);
#elif defined(USE_STM32_HAL_MODE)
    // Store address and reset transmit buffer
    g_u8CurrentAddress = u8Address;
    g_u8TxBufferLen = 0;
#else
    Wire.beginTransmission(u8Address);
#endif
}

uint8_t Wire_u8Write(uint8_t u8Data)
{
#ifdef UNIT_TEST
    return Wire_write(u8Data);
#elif defined(USE_STM32_HAL_MODE)
    // Buffer data for transmission
    if (g_u8TxBufferLen < sizeof(g_au8TxBuffer))
    {
        g_au8TxBuffer[g_u8TxBufferLen++] = u8Data;
        return 1;
    }
    return 0;  // Buffer full
#else
    return (uint8_t)Wire.write(u8Data);
#endif
}

uint8_t Wire_u8WriteBytes(const uint8_t *pu8Data, uint8_t u8Length)
{
    if (pu8Data == NULL || u8Length == 0)
    {
        return 0;
    }

#ifdef UNIT_TEST
    uint8_t u8Written = 0;
    for (uint8_t i = 0; i < u8Length; i++)
    {
        u8Written += Wire_write(pu8Data[i]);
    }
    return u8Written;
#elif defined(USE_STM32_HAL_MODE)
    // Buffer multiple bytes
    uint8_t u8Written = 0;
    for (uint8_t i = 0; i < u8Length; i++)
    {
        if (g_u8TxBufferLen < sizeof(g_au8TxBuffer))
        {
            g_au8TxBuffer[g_u8TxBufferLen++] = pu8Data[i];
            u8Written++;
        }
        else
        {
            break;  // Buffer full
        }
    }
    return u8Written;
#else
    return (uint8_t)Wire.write(pu8Data, u8Length);
#endif
}

uint8_t Wire_u8EndTransmission(bool bSendStop)
{
#ifdef UNIT_TEST
    return Wire_endTransmission(bSendStop);
#elif defined(USE_STM32_HAL_MODE)
    // Transmit buffered data using HAL
    if (g_u8TxBufferLen == 0)
    {
        return 0;  // Nothing to send, success
    }
    
    HAL_StatusTypeDef eStatus = HAL_I2C_Master_Transmit(
        g_psI2CHandle,
        g_u8CurrentAddress << 1,  // HAL uses 8-bit address
        g_au8TxBuffer,
        g_u8TxBufferLen,
        WIRE_I2C_TIMEOUT_MS
    );
    
    // Reset buffer
    g_u8TxBufferLen = 0;
    
    // Map HAL status to Wire-compatible error codes
    switch (eStatus)
    {
        case HAL_OK:
            return 0;  // Success
        case HAL_ERROR:
            return 4;  // Other error
        case HAL_TIMEOUT:
            return 4;  // Timeout (treated as other error)
        case HAL_BUSY:
            return 4;  // Bus busy
        default:
            return 4;
    }
#else
    return (uint8_t)Wire.endTransmission(bSendStop);
#endif
}

uint8_t Wire_u8RequestFrom(uint8_t u8Address, uint8_t u8Quantity)
{
#ifdef UNIT_TEST
    return Wire_requestFrom(u8Address, u8Quantity);
#elif defined(USE_STM32_HAL_MODE)
    // Receive data using HAL
    if (u8Quantity == 0 || u8Quantity > sizeof(g_au8RxBuffer))
    {
        return 0;
    }
    
    HAL_StatusTypeDef eStatus = HAL_I2C_Master_Receive(
        g_psI2CHandle,
        u8Address << 1,  // HAL uses 8-bit address
        g_au8RxBuffer,
        u8Quantity,
        WIRE_I2C_TIMEOUT_MS
    );
    
    if (eStatus == HAL_OK)
    {
        g_u8RxBufferLen = u8Quantity;
        g_u8RxReadIndex = 0;
        return u8Quantity;
    }
    else
    {
        g_u8RxBufferLen = 0;
        g_u8RxReadIndex = 0;
        return 0;  // Error
    }
#else
    return (uint8_t)Wire.requestFrom(u8Address, u8Quantity);
#endif
}

uint8_t Wire_u8Available(void)
{
#ifdef UNIT_TEST
    return Wire_available();
#elif defined(USE_STM32_HAL_MODE)
    // Return remaining bytes in receive buffer
    if (g_u8RxReadIndex < g_u8RxBufferLen)
    {
        return g_u8RxBufferLen - g_u8RxReadIndex;
    }
    return 0;
#else
    return (uint8_t)Wire.available();
#endif
}

uint8_t Wire_u8Read(void)
{
#ifdef UNIT_TEST
    return Wire_read();
#elif defined(USE_STM32_HAL_MODE)
    // Read next byte from receive buffer
    if (g_u8RxReadIndex < g_u8RxBufferLen)
    {
        return g_au8RxBuffer[g_u8RxReadIndex++];
    }
    return 0;  // No data available
#else
    return (uint8_t)Wire.read();
#endif
}

} // extern "C"

#endif // FEATURE_I2C_MEASUREMENT
