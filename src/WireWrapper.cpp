/**
 * @file WireWrapper.cpp
 * @brief Simple wrapper for Arduino Wire library (for non-STM32 platforms and unit tests)
 *
 * On STM32 platforms, I2CMeasure uses STM32I2CDriver directly instead of this wrapper.
 */

#ifdef PINCFG_FEATURE_I2C_MEASUREMENT

#include "WireWrapper.h"

#include <stddef.h> // For NULL

#ifdef UNIT_TEST
#include "I2CMock.h"
#else
#include <Wire.h>
#endif

extern "C"
{

    void Wire_vBegin(void)
    {
#ifdef UNIT_TEST
        Wire_begin();
#else
        Wire.begin();
#endif
    }

    void Wire_vBeginTransmission(uint8_t u8Address)
    {
#ifdef UNIT_TEST
        Wire_beginTransmission(u8Address);
#else
        Wire.beginTransmission(u8Address);
#endif
    }

    uint8_t Wire_u8Write(uint8_t u8Data)
    {
#ifdef UNIT_TEST
        return Wire_write(u8Data);
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
#else
        return (uint8_t)Wire.write(pu8Data, u8Length);
#endif
    }

    uint8_t Wire_u8EndTransmission(bool bSendStop)
    {
#ifdef UNIT_TEST
        return Wire_endTransmission(bSendStop);
#else
        return (uint8_t)Wire.endTransmission(bSendStop);
#endif
    }

    uint8_t Wire_u8RequestFrom(uint8_t u8Address, uint8_t u8Quantity)
    {
#ifdef UNIT_TEST
        return Wire_requestFrom(u8Address, u8Quantity);
#else
        return (uint8_t)Wire.requestFrom(u8Address, u8Quantity);
#endif
    }

    uint8_t Wire_u8Available(void)
    {
#ifdef UNIT_TEST
        return Wire_available();
#else
        return (uint8_t)Wire.available();
#endif
    }

    uint8_t Wire_u8Read(void)
    {
#ifdef UNIT_TEST
        return Wire_read();
#else
        return (uint8_t)Wire.read();
#endif
    }

} // extern "C"

#endif // PINCFG_FEATURE_I2C_MEASUREMENT
