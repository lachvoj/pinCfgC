#ifndef SENSORMEASURE_H
#define SENSORMEASURE_H

#include "ISensorMeasure.h"
#include "PinCfgStr.h"
#include "Types.h"

typedef enum
{
    SENSORMEASURE_OK_E,
    SENSORMEASURE_ALLOCATION_ERROR_E,
    SENSORMEASURE_NULLPTR_ERROR_E,
    SENSORMEASURE_ERROR_E
} SENSORMEASURE_RESULT_T;

/**
 * @brief Initialize sensor measurement interface with name reuse
 * 
 * Similar to Presentable_eInitReuseName - reuses an already allocated name string.
 * Used when name is already allocated (e.g., from PersistentConfig).
 * 
 * @param psHandle Pointer to ISensorMeasure structure
 * @param pcName Pointer to already allocated name string (will NOT be copied)
 * @param eType Measurement type enum
 * @return SENSORMEASURE_RESULT_T status
 */
SENSORMEASURE_RESULT_T SensorMeasure_eInitReuseName(
    ISENSORMEASURE_T *psHandle,
    const char *pcName,
    MEASUREMENT_TYPE_T eType);

/**
 * @brief Initialize sensor measurement interface with name allocation
 * 
 * Similar to Presentable_eInit - allocates memory and copies name string.
 * Used during CSV parsing when name comes from STRING_POINT_T.
 * 
 * @param psHandle Pointer to ISensorMeasure structure
 * @param psName Pointer to STRING_POINT_T containing name (will be copied)
 * @param eType Measurement type enum
 * @return SENSORMEASURE_RESULT_T status
 */
SENSORMEASURE_RESULT_T SensorMeasure_eInit(
    ISENSORMEASURE_T *psHandle,
    STRING_POINT_T *psName,
    MEASUREMENT_TYPE_T eType);

/**
 * @brief Get measurement source name
 * 
 * @param psHandle Pointer to ISensorMeasure structure
 * @return Pointer to name string (NULL if not initialized)
 */
const char *SensorMeasure_pcGetName(ISENSORMEASURE_T *psHandle);

#endif // SENSORMEASURE_H
