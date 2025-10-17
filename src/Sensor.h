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
    ENABLEABLE_T *psEnableable;  // NULL if not enableable
    
    // Timing (optimized with uint16_t intervals)
    uint16_t u16ReportIntervalSec;      // Report interval in seconds (1-3600)
    uint16_t u16SamplingIntervalMs;     // Sampling interval in milliseconds (100-5000)
    uint32_t u32LastReportMs;           // Last report timestamp (millis())
    uint32_t u32LastSamplingMs;         // Last sampling timestamp (millis())
    
    // Cumulative mode fields (always present, only used if bCumulative=true)
    uint32_t u32SamplesCount;           // Sample counter (can exceed 65k)
    double fCumulatedValue;             // Double precision to prevent precision loss in long cumulative periods
    
    // Calibration
    float fOffset;
    
    // Data extraction (for multi-value I2C sensors)
    uint8_t u8DataByteOffset;           // Starting byte index in raw measurement buffer (0-5)
    uint8_t u8DataByteCount;            // Number of bytes to extract (1-6, 0=use all)
    
    // Feature flags (use SENSOR_FLAG_* masks to access)
    uint8_t u8Flags;
    
} SENSOR_T;

// Sensor flag bit masks
#define SENSOR_FLAG_CUMULATIVE  0x01  // Use cumulative averaging
#define SENSOR_FLAG_ENABLEABLE  0x02  // Can be toggled on/off
#define SENSOR_FLAG_ENABLED     0x04  // Current enabled state

SENSOR_RESULT_T Sensor_eInit(
    SENSOR_T *psHandle,
    PINCFG_RESULT_T (*eAddToLoopables)(LOOPABLE_T *psLoopable),
    PINCFG_RESULT_T (*eAddToPresentables)(PRESENTABLE_T *psPresentable),
    uint8_t *pu8PresentablesCount,
    bool bCumulative,
    bool bEnableable,
    STRING_POINT_T *sName,
    mysensors_data_t eVType,
    mysensors_sensor_t eSType,
    void (*vReceive)(PRESENTABLE_T *psHandle, const void *pvMessage),
    ISENSORMEASURE_T *psSensorMeasure,
    uint16_t u16SamplingIntervalMs,
    uint16_t u16ReportIntervalSec,
    float fOffset);

#endif // SENSOR_H
