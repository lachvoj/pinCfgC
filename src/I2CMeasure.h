#ifndef I2CMEASURE_H
#define I2CMEASURE_H

#ifdef PINCFG_FEATURE_I2C_MEASUREMENT

#include <stdint.h>

#include "ISensorMeasure.h"
#include "PinCfgCsv.h"
#include "Types.h"

typedef enum I2CMEASURE_STATE_E
{
    I2CMEASURE_STATE_IDLE_E = 0,     // No transaction in progress
    I2CMEASURE_STATE_COMMAND_SENT_E, // Command written (command mode only)
    I2CMEASURE_STATE_WAITING_E,      // Waiting for conversion (command mode only)
    I2CMEASURE_STATE_REQUEST_SENT_E, // Wire.requestFrom() called
    I2CMEASURE_STATE_READING_E,      // Waiting for data available
    I2CMEASURE_STATE_DATA_READY_E,   // Data read, ready to return
    I2CMEASURE_STATE_ERROR_E         // Error occurred
} I2CMEASURE_STATE_T;

typedef struct I2CMEASURE_S
{
    ISENSORMEASURE_T sInterface;   // Interface (eMeasure function pointer + eType + pcName) - 12/16 bytes
    uint8_t u8DeviceAddress;       // I2C device address (7-bit, e.g., 0x48) - 1 byte
    uint8_t au8CommandBytes[3];    // Command sequence: [0]=register OR [0,1,2]=command - 3 bytes
    uint8_t u8CommandLength;       // Command length: 1=simple mode, 2-3=command mode - 1 byte
    uint8_t u8DataSize;            // Number of bytes to read (1-6) - 1 byte
    I2CMEASURE_STATE_T eState;     // Current state machine state - 1 byte
    uint32_t u32RequestTime;       // millis() when request/command started - 4 bytes
    uint16_t u16TimeoutMs;         // Timeout duration (default: 100ms) - 2 bytes
    uint16_t u16ConversionDelayMs; // Delay after command before read (0=none, 80=AHT10) - 2 bytes
    uint8_t au8Buffer[6];          // Raw I2C data buffer (increased for AHT10) - 6 bytes
} I2CMEASURE_T;

PINCFG_RESULT_T I2CMeasure_eInit(
    I2CMEASURE_T *psHandle,
    STRING_POINT_T *psName,
    uint8_t u8DeviceAddress,
    const uint8_t *pu8CommandBytes,
    uint8_t u8CommandLength,
    uint8_t u8DataSize,
    uint16_t u16ConversionDelayMs);

ISENSORMEASURE_RESULT_T I2CMeasure_eMeasure(
    ISENSORMEASURE_T *pSelf,
    uint8_t *pu8Buffer,
    uint8_t *pu8Size,
    uint32_t u32ms);

#endif // PINCFG_FEATURE_I2C_MEASUREMENT
#endif // I2CMEASURE_H
