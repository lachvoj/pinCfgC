#ifndef WIREWRAPPER_H
#define WIREWRAPPER_H

#ifdef PINCFG_FEATURE_I2C_MEASUREMENT

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif

    void Wire_vBegin(void);

    void Wire_vBeginTransmission(uint8_t u8Address);

    uint8_t Wire_u8Write(uint8_t u8Data);

    uint8_t Wire_u8WriteBytes(const uint8_t *pu8Data, uint8_t u8Length);

    uint8_t Wire_u8EndTransmission(bool bSendStop);

    uint8_t Wire_u8RequestFrom(uint8_t u8Address, uint8_t u8Quantity);

    uint8_t Wire_u8Available(void);

    uint8_t Wire_u8Read(void);

#ifdef __cplusplus
}
#endif

#endif // PINCFG_FEATURE_I2C_MEASUREMENT
#endif // WIREWRAPPER_H
