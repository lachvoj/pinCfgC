#include "Globals.h"
#include "InPin.h"
#include "Memory.h"
#include "PinCfgUtils.h"
#include "Sensor.h"

static void Sensor_vLoop(LOOPABLE_T *psLoopableHandle, uint32_t u32ms);
static void Sensor_vSendUnitPrefix(SENSOR_T *psHandle);
static mysensors_payload_t Sensor_eGetPayloadType(mysensors_data_t eVType, uint8_t u8Precision);
static int32_t Sensor_i32ExtractBytes(const uint8_t *pu8Buffer, uint8_t u8TotalSize, uint8_t u8Offset, uint8_t u8Count);
static const int32_t ai32PrecisionDivisors[7] = {1, 10, 100, 1000, 10000, 100000, 1000000};
static const char *Sensor_pcGetDefaultUnit(MEASUREMENT_TYPE_T eType);

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
    int32_t i32Scale,     // Fixed-point: scale × PINCFG_FIXED_POINT_SCALE
    int32_t i32Offset,    // Fixed-point: offset × PINCFG_FIXED_POINT_SCALE
    uint8_t u8Precision,  // Decimal places (0-6)
    STRING_POINT_T *psUnit) // Unit string for V_UNIT_PREFIX (NULL if not used)
{
    if (psHandle == NULL || sName == NULL || psSensorMeasure == NULL)
        return SENSOR_NULLPTR_ERROR_E;

    // Initialize presentable
    if (Presentable_eInit(&psHandle->sPresentable, sName, *pu8PresentablesCount) != PRESENTABLE_OK_E)
        return SENSOR_SUBINIT_ERROR_E;
    (*pu8PresentablesCount)++;

    // Setup vtab
    psHandle->psSensorMeasure = psSensorMeasure;
    psHandle->sVtab.eVType = eVType;
    psHandle->sVtab.eSType = eSType;
    psHandle->sVtab.vReceive = InPin_vRcvMessage;
    psHandle->sVtab.vPresent = Presentable_vPresent;
    psHandle->sPresentable.psVtab = &psHandle->sVtab;

    // Setup loop function (single function for all modes)
    psHandle->sLoopable.vLoop = Sensor_vLoop;

    // Initialize timing
    psHandle->u16SamplingIntervalMs = u16SamplingIntervalMs;
    psHandle->u32ReportIntervalMs = (uint32_t)u16ReportIntervalSec * 1000U; // Pre-calculate to avoid multiply in loop
    psHandle->u32LastReportMs = 0U;
    psHandle->u32LastSamplingMs = 0U;

    // Initialize calibration parameters
    psHandle->i32Scale = (i32Scale == 0) ? PINCFG_FIXED_POINT_SCALE : i32Scale; // Default: 1.0
    psHandle->i32Offset = i32Offset;                                            // Default: 0.0 (no offset)

    // Clamp precision to valid range (0-6)
    if (u8Precision > PINCFG_SENSOR_PRECISION_MAX_D)
    {
        psHandle->u8Precision = PINCFG_SENSOR_PRECISION_MAX_D;
        // Note: Warning logged during CSV parsing
    }
    else
    {
        psHandle->u8Precision = u8Precision;
    }

    // Determine appropriate payload type based on precision and V_TYPE
    psHandle->sPresentable.ePayloadType = Sensor_eGetPayloadType(eVType, psHandle->u8Precision);

    // Initialize byte extraction (defaults: use all bytes)
    psHandle->u8DataByteOffset = 0;
    psHandle->u8DataByteCount = 0; // 0 = use all bytes

    // Force cumulative mode for looptime measurements
    if (psSensorMeasure->eType == MEASUREMENT_TYPE_LOOPTIME_E)
        bCumulative = true;

    // Allocate and store unit string (can be NULL)
    if (psUnit != NULL && psUnit->szLen > 0)
    {
        char *pcUnitBuf = (char *)Memory_vpAlloc(psUnit->szLen + 1);
        if (pcUnitBuf == NULL)
            return SENSOR_MEMORY_ALLOCATION_ERROR_E;
        memcpy(pcUnitBuf, psUnit->pcStrStart, psUnit->szLen);
        pcUnitBuf[psUnit->szLen] = '\0';
        psHandle->pcUnit = pcUnitBuf;
    }
    else
    {
        psHandle->pcUnit = NULL;
    }

    // Initialize mode flags
    psHandle->u8Flags = 0U;
    if (bCumulative)
        psHandle->u8Flags |= SENSOR_FLAG_CUMULATIVE;
    if (bEnableable)
        psHandle->u8Flags |= SENSOR_FLAG_ENABLEABLE;
    psHandle->u8Flags |= SENSOR_FLAG_ENABLED; // Start enabled

    // Initialize cumulative fields if needed
    if (bCumulative)
    {
        psHandle->u32SamplesCount = 0U;
        psHandle->i64CumulatedValue = 0;
    }

    // Setup enableable if needed
    if (bEnableable)
    {
        psHandle->psEnableable = (ENABLEABLE_T *)Memory_vpAlloc(sizeof(ENABLEABLE_T));
        if (psHandle->psEnableable == NULL)
            return SENSOR_MEMORY_ALLOCATION_ERROR_E;

        // Enableable switch gets current count as ID (one more than main sensor)
        if (Enableable_eInit(psHandle->psEnableable, sName, *pu8PresentablesCount) != ENABLEABLE_OK_E)
            return SENSOR_SUBINIT_ERROR_E;
        (*pu8PresentablesCount)++; // Increment for next presentable
    }
    else
    {
        psHandle->psEnableable = NULL;
    }

    return SENSOR_OK_E;
}

static void Sensor_vLoop(LOOPABLE_T *psLoopableHandle, uint32_t u32ms)
{
    SENSOR_T *psHandle = container_of(psLoopableHandle, SENSOR_T, sLoopable);

    // Handle enableable state changes
    if (psHandle->u8Flags & SENSOR_FLAG_ENABLEABLE)
    {
        if (psHandle->psEnableable->sPresentable.bStateChanged)
        {
            psHandle->psEnableable->sPresentable.bStateChanged = false;

            if (psHandle->psEnableable->sPresentable.u8State)
                psHandle->u8Flags |= SENSOR_FLAG_ENABLED;
            else
                psHandle->u8Flags &= ~SENSOR_FLAG_ENABLED;

            // Reset timers on state change
            psHandle->u32LastReportMs = 0U;
            if (psHandle->u8Flags & SENSOR_FLAG_CUMULATIVE)
            {
                psHandle->u32LastSamplingMs = 0U;
                psHandle->i64CumulatedValue = 0;
                psHandle->u32SamplesCount = 0U;
            }
        }

        // Skip measurement if disabled
        if (!(psHandle->u8Flags & SENSOR_FLAG_ENABLED))
            return;
    }

    // Handle cumulative mode
    if (psHandle->u8Flags & SENSOR_FLAG_CUMULATIVE)
    {
        // Determine if we should measure this loop
        bool bShouldMeasure = false;

        // Special case: LOOPTIME always measures every loop (bypasses sampling interval)
#ifdef PINCFG_FEATURE_LOOPTIME_MEASUREMENT
        if (psHandle->psSensorMeasure->eType == MEASUREMENT_TYPE_LOOPTIME_E)
        {
            bShouldMeasure = true;
        }
        // Normal case: check sampling interval
        else
#endif
            if (PinCfg_u32GetElapsedTime(psHandle->u32LastSamplingMs, u32ms) >= psHandle->u16SamplingIntervalMs)
        {
            psHandle->u32LastSamplingMs = u32ms;
            bShouldMeasure = true;
        }

        if (bShouldMeasure)
        {
            // Universal measurement path - all measurements return raw bytes
            uint8_t au8Buffer[6];
            uint8_t u8Size = sizeof(au8Buffer);

            ISENSORMEASURE_RESULT_T eResult =
                psHandle->psSensorMeasure->eMeasure(psHandle->psSensorMeasure, au8Buffer, &u8Size, u32ms);

            // Handle non-blocking measurements
            if (eResult == ISENSORMEASURE_PENDING_E)
            {
                return; // Operation in progress, try again next loop
            }

            if (eResult == ISENSORMEASURE_OK_E)
            {
                // Extract raw integer value (no conversion)
                int32_t i32Value =
                    Sensor_i32ExtractBytes(au8Buffer, u8Size, psHandle->u8DataByteOffset, psHandle->u8DataByteCount);

                // Accumulate (no scaling - accumulates final values)
                psHandle->i64CumulatedValue += i32Value;
                psHandle->u32SamplesCount++;
            }
            // Note: On error, skip this sample (no increment)
        }

        // Check report interval (pre-calculated to avoid multiply each loop)
        if (PinCfg_u32GetElapsedTime(psHandle->u32LastReportMs, u32ms) >= psHandle->u32ReportIntervalMs)
        {
            psHandle->u32LastReportMs = u32ms;

            if (psHandle->u32SamplesCount > 0U)
            {
                // Send unit prefix on first report (if not already sent)
                if (!(psHandle->u8Flags & SENSOR_FLAG_UNIT_SENT))
                {
                    Sensor_vSendUnitPrefix(psHandle);
                    psHandle->u8Flags |= SENSOR_FLAG_UNIT_SENT;
                }

                int32_t i32Scaled = (((psHandle->i64CumulatedValue * psHandle->i32Scale) / psHandle->u32SamplesCount) +
                                     psHandle->i32Offset) /
                                    (PINCFG_FIXED_POINT_SCALE / ai32PrecisionDivisors[psHandle->u8Precision]);
                Presentable_vSetState((PRESENTABLE_T *)psHandle, i32Scaled, true);

                // Reset accumulator
                psHandle->i64CumulatedValue = 0;
                psHandle->u32SamplesCount = 0U;
            }
        }
    }
    // Handle standard mode
    else
    {
        // Check report interval (pre-calculated to avoid multiply each loop)
        if (PinCfg_u32GetElapsedTime(psHandle->u32LastReportMs, u32ms) >= psHandle->u32ReportIntervalMs)
        {
            psHandle->u32LastReportMs = u32ms;

            // Universal measurement path - all measurements return raw bytes
            uint8_t au8Buffer[6];
            uint8_t u8Size = sizeof(au8Buffer);

            ISENSORMEASURE_RESULT_T eResult =
                psHandle->psSensorMeasure->eMeasure(psHandle->psSensorMeasure, au8Buffer, &u8Size, u32ms);

            // Handle non-blocking measurements
            if (eResult == ISENSORMEASURE_PENDING_E)
            {
                return; // Operation in progress, try again next loop
            }

            if (eResult == ISENSORMEASURE_OK_E)
            {
                // Send unit prefix on first report (if not already sent)
                if (!(psHandle->u8Flags & SENSOR_FLAG_UNIT_SENT))
                {
                    Sensor_vSendUnitPrefix(psHandle);
                    psHandle->u8Flags |= SENSOR_FLAG_UNIT_SENT;
                }

                // Extract raw integer value (no conversion)
                int32_t i32Value =
                    Sensor_i32ExtractBytes(au8Buffer, u8Size, psHandle->u8DataByteOffset, psHandle->u8DataByteCount);

                int32_t i32Scaled = (((int64_t)i32Value * (int64_t)psHandle->i32Scale) + (int64_t)psHandle->i32Offset) /
                                    (PINCFG_FIXED_POINT_SCALE / ai32PrecisionDivisors[psHandle->u8Precision]);
                // Report value
                Presentable_vSetState((PRESENTABLE_T *)psHandle, i32Scaled, true);
            }
            // Note: On error, skip this reading (no state update)
        }
    }
}

static mysensors_payload_t Sensor_eGetPayloadType(mysensors_data_t eVType, uint8_t u8Precision)
{
    // Most sensor types are signed (temperature, etc.)
    // Only a few types are unsigned (percentage, light level, etc.)
    bool bSigned = true;

    // Determine signedness based on common V_TYPE usage
    switch (eVType)
    {
    case V_PERCENTAGE:  // 0-100%
    case V_LIGHT_LEVEL: // 0-100%
    case V_UV:          // UV index (positive)
        bSigned = false;
        break;
    default:
        bSigned = true; // Most sensors (temp, humidity, etc.) are signed
        break;
    }

    // Select payload type based on precision and signedness
    if (u8Precision == 0)
    {
        // No decimals: use smallest type
        return bSigned ? P_INT16 : P_BYTE;
    }
    else if (u8Precision <= 2)
    {
        // 1-2 decimals: INT16 fits ±32.76 (range -32768 to 32767)
        return P_INT16;
    }
    else
    {
        // 3-6 decimals: Need INT32 (e.g., ±9999.999 = ±9999999 in INT32)
        return P_LONG32;
    }
}

static int32_t Sensor_i32ExtractBytes(const uint8_t *pu8Buffer, uint8_t u8TotalSize, uint8_t u8Offset, uint8_t u8Count)
{
    int32_t i32Value = 0;
    uint8_t u8ExtractSize = u8Count;

    // If count is 0, use all bytes from offset
    if (u8ExtractSize == 0 || (u8Offset + u8ExtractSize) > u8TotalSize)
    {
        u8ExtractSize = u8TotalSize - u8Offset;
    }

    // Combine bytes (big-endian: MSB first)
    for (uint8_t i = 0; i < u8ExtractSize; i++)
    {
        i32Value = (i32Value << 8) | pu8Buffer[u8Offset + i];
    }

    // Sign-extend for signed values (if MSB is set)
    if (u8ExtractSize == 1 && (i32Value & 0x80))
    {
        i32Value |= 0xFFFFFF00;
    }
    else if (u8ExtractSize == 2 && (i32Value & 0x8000))
    {
        i32Value |= 0xFFFF0000;
    }
    else if (u8ExtractSize == 3 && (i32Value & 0x800000))
    {
        i32Value |= 0xFF000000;
    }

    return i32Value;
}

static const char *Sensor_pcGetDefaultUnit(MEASUREMENT_TYPE_T eType)
{
    switch (eType)
    {
#ifdef PINCFG_FEATURE_LOOPTIME_MEASUREMENT
    case MEASUREMENT_TYPE_LOOPTIME_E: return "ms";
#endif
    default: return NULL; // No default unit
    }
}

static void Sensor_vSendUnitPrefix(SENSOR_T *psHandle)
{
    // Determine unit string (explicit or default)
    const char *pcUnit = psHandle->pcUnit;
    if (pcUnit == NULL)
        pcUnit = Sensor_pcGetDefaultUnit(psHandle->psSensorMeasure->eType);

    // Send unit prefix if available
    if (pcUnit == NULL)
        return;

    MyMessage msg;
    if (eMyMessageInit(&msg, psHandle->sPresentable.u8Id, V_UNIT_PREFIX, P_STRING, pcUnit) != WRAP_OK_E)
        return;

    eSend(&msg, false);
}
