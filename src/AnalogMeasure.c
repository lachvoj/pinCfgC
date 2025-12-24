// Include Globals.h first to get feature flags
#include "Globals.h"

#ifdef PINCFG_FEATURE_ANALOG_MEASUREMENT

#include "AnalogMeasure.h"
#include "AnalogWrapper.h"
#include "SensorMeasure.h"

// Forward declaration of measurement function
static ISENSORMEASURE_RESULT_T AnalogMeasure_eMeasure(
    ISENSORMEASURE_T *pSelf,
    uint8_t *pu8Buffer,
    uint8_t *pu8Size,
    uint32_t u32ms);

ANALOGMEASURE_RESULT_T AnalogMeasure_eInit(ANALOGMEASURE_T *psHandle, STRING_POINT_T *psName, uint8_t u8Pin)
{
    // Validate parameters
    if (psHandle == NULL || psName == NULL)
        return ANALOGMEASURE_NULLPTR_ERROR_E;

    // Initialize base interface (allocates and copies name)
    if (SensorMeasure_eInit(&psHandle->sSensorMeasure, psName, MEASUREMENT_TYPE_ANALOG_E) != SENSORMEASURE_OK_E)
        return ANALOGMEASURE_ERROR_E;

    // Set measurement function pointer
    psHandle->sSensorMeasure.eMeasure = AnalogMeasure_eMeasure;

    // Store measurement-specific configuration
    psHandle->u8Pin = u8Pin;

    return ANALOGMEASURE_OK_E;
}

static ISENSORMEASURE_RESULT_T AnalogMeasure_eMeasure(
    ISENSORMEASURE_T *pSelf,
    uint8_t *pu8Buffer,
    uint8_t *pu8Size,
    uint32_t u32ms)
{
    if (pSelf == NULL || pu8Buffer == NULL || pu8Size == NULL)
        return ISENSORMEASURE_NULLPTR_ERROR_E;

    if (*pu8Size < 2)
        return ISENSORMEASURE_ERROR_E; // Buffer too small

    ANALOGMEASURE_T *psHandle = (ANALOGMEASURE_T *)pSelf;

    // Avoid unused parameter warning
    (void)u32ms;

    // Read raw ADC value (uint16: 0-1023 or 0-4095 depending on platform)
    uint16_t u16RawADC = Analog_u16Read(psHandle->u8Pin);

    // Convert to big-endian format (MSB first)
    pu8Buffer[0] = (uint8_t)(u16RawADC >> 8);   // High byte
    pu8Buffer[1] = (uint8_t)(u16RawADC & 0xFF); // Low byte
    *pu8Size = 2;

    return ISENSORMEASURE_OK_E;
}

#endif // PINCFG_FEATURE_ANALOG_MEASUREMENT
