#ifndef SPIMEASURE_H
#define SPIMEASURE_H

#ifdef PINCFG_FEATURE_SPI_MEASUREMENT

#include <stdint.h>

#include "ISensorMeasure.h"
#include "PinCfgCsv.h"
#include "Types.h"

typedef enum SPIMEASURE_STATE_E
{
    SPIMEASURE_STATE_IDLE_E = 0,     // No transaction in progress
    SPIMEASURE_STATE_COMMAND_SENT_E, // Command written (command mode only)
    SPIMEASURE_STATE_WAITING_E,      // Waiting for conversion (command mode only)
    SPIMEASURE_STATE_READING_E,      // Reading data
    SPIMEASURE_STATE_DATA_READY_E,   // Data read, ready to return
    SPIMEASURE_STATE_ERROR_E         // Error occurred
} SPIMEASURE_STATE_T;

typedef struct SPIMEASURE_S
{
    ISENSORMEASURE_T sInterface;   // Interface (eMeasure function pointer + eType + pcName) - 12/16 bytes
    uint8_t u8ChipSelectPin;       // CS/SS pin number (e.g., 10) - 1 byte
    uint8_t au8CommandBytes[4];    // Command sequence: empty=simple mode, 1-4 bytes=command mode - 4 bytes
    uint8_t u8CommandLength;       // Command length: 0=simple mode, 1-4=command mode - 1 byte
    uint8_t u8DataSize;            // Number of bytes to read (1-8) - 1 byte
    SPIMEASURE_STATE_T eState;     // Current state machine state - 1 byte
    uint32_t u32RequestTime;       // millis() when request/command started - 4 bytes
    uint16_t u16TimeoutMs;         // Timeout duration (default: 100ms) - 2 bytes
    uint16_t u16ConversionDelayMs; // Delay after command before read (0=none, 10=typical) - 2 bytes
    uint8_t au8Buffer[8];          // Raw SPI data buffer (up to 8 bytes for most sensors) - 8 bytes
} SPIMEASURE_T;

PINCFG_RESULT_T SPIMeasure_eInit(
    SPIMEASURE_T *psHandle,
    STRING_POINT_T *psName,
    uint8_t u8ChipSelectPin,
    const uint8_t *pu8CommandBytes,
    uint8_t u8CommandLength,
    uint8_t u8DataSize,
    uint16_t u16ConversionDelayMs);

ISENSORMEASURE_RESULT_T SPIMeasure_eMeasure(
    ISENSORMEASURE_T *pSelf,
    uint8_t *pu8Buffer,
    uint8_t *pu8Size,
    uint32_t u32ms);

#endif // PINCFG_FEATURE_SPI_MEASUREMENT
#endif // SPIMEASURE_H
