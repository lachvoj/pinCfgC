#ifndef SENSOR_H
#define SENSOR_H

#include "Enableable.h"
#include "ILoopable.h"
#include "ISensorMeasure.h"
#include "MySensorsWrapper.h"
#include "PinCfgCsv.h"
#include "Presentable.h"
#include "Types.h"

typedef enum SENSOR_RESULT_E
{
    SENSOR_OK_E = 0,
    SENSOR_NULLPTR_ERROR_E,
    SENSOR_SUBINIT_ERROR_E,
    SENSOR_MEMORY_ALLOCATION_ERROR_E,
    SENSOR_INIT_ERROR_E,
    SENSOR_ERROR_E
} SENSOR_RESULT_T;

typedef struct SENSOR_S
{
    PRESENTABLE_T sPresentable;
    LOOPABLE_T sLoopable;
    PRESENTABLE_VTAB_T sVtab;
    ISENSORMEASURE_T *psSensorMeasure;
    // Enableable presentable (only used if bEnableable=true)
    ENABLEABLE_T *psEnableable; // NULL if not enableable

    // Timing (optimized with uint16_t intervals)
    uint16_t u16SamplingIntervalMs; // Sampling interval in milliseconds (100-5000)
    uint32_t u32ReportIntervalMs;   // Pre-calculated report interval in ms (avoids multiply in loop)
    uint32_t u32LastReportMs;       // Last report timestamp (millis())
    uint32_t u32LastSamplingMs;     // Last sampling timestamp (millis())

    // Cumulative mode fields (always present, only used if bCumulative=true)
    uint32_t u32SamplesCount;  // Sample counter (can exceed 65k)
    int64_t i64CumulatedValue; // Fixed-point accumulator (no scaling)

    // Calibration (fixed-point: value × PINCFG_FIXED_POINT_SCALE)
    int32_t i32Scale;    // Multiplicative scale factor (e.g., 0.0625 stored as 62500)
    int32_t i32Offset;   // Additive offset adjustment (e.g., -2.1 stored as -2100000)
    uint8_t u8Precision; // Decimal places (0-6), affects payload type sizing

    // Data extraction (for multi-value I2C sensors)
    uint8_t u8DataByteOffset; // Starting byte index in raw measurement buffer (0-5)
    uint8_t u8DataByteCount;  // Number of bytes to extract (1-6, 0=use all)

    // Feature flags (use SENSOR_FLAG_* masks to access)
    uint8_t u8Flags;

    // Unit string for MySensors V_UNIT_PREFIX message (optional)
    const char *pcUnit; // Unit string (e.g., "µs", "°C", "ppm"), NULL if not used

} SENSOR_T;

// Sensor flag bit masks
#define SENSOR_FLAG_CUMULATIVE 0x01 // Use cumulative averaging
#define SENSOR_FLAG_ENABLEABLE 0x02 // Can be toggled on/off
#define SENSOR_FLAG_ENABLED 0x04    // Current enabled state
#define SENSOR_FLAG_UNIT_SENT 0x08  // Unit prefix has been sent

SENSOR_RESULT_T Sensor_eInit(
    SENSOR_T *psHandle,
    uint8_t *pu8PresentablesCount,
    bool bCumulative,
    bool bEnableable,
    STRING_POINT_T *sName,
    mysensors_data_t eVType,
    mysensors_sensor_t eSType,
    ISENSORMEASURE_T *psSensorMeasure,
    uint16_t u16SamplingIntervalMs,
    uint16_t u16ReportIntervalSec,
    int32_t i32Scale,    // Fixed-point: scale × PINCFG_FIXED_POINT_SCALE
    int32_t i32Offset,   // Fixed-point: offset × PINCFG_FIXED_POINT_SCALE
    uint8_t u8Precision, // Decimal places (0-6)
    const char *pcUnit); // Unit string for V_UNIT_PREFIX (NULL if not used)

#endif // SENSOR_H
