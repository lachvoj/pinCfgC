#include "SensorMeasure.h"
#include "Memory.h"
#include <string.h>

/**
 * @brief Initialize sensor measurement interface with name reuse
 * 
 * Similar to Presentable_eInitReuseName pattern.
 * Does NOT allocate or copy name - uses provided pointer directly.
 */
SENSORMEASURE_RESULT_T SensorMeasure_eInitReuseName(
    ISENSORMEASURE_T *psHandle,
    const char *pcName,
    MEASUREMENT_TYPE_T eType)
{
    if (psHandle == NULL || pcName == NULL)
        return SENSORMEASURE_NULLPTR_ERROR_E;

    psHandle->pcName = pcName;
    psHandle->eType = eType;
    psHandle->eMeasure = NULL;      // Must be set by concrete implementation

    return SENSORMEASURE_OK_E;
}

/**
 * @brief Initialize sensor measurement interface with name allocation
 * 
 * Similar to Presentable_eInit pattern.
 * Allocates memory and copies name string from STRING_POINT_T.
 */
SENSORMEASURE_RESULT_T SensorMeasure_eInit(
    ISENSORMEASURE_T *psHandle,
    STRING_POINT_T *psName,
    MEASUREMENT_TYPE_T eType)
{
    if (psHandle == NULL || psName == NULL)
        return SENSORMEASURE_NULLPTR_ERROR_E;

    // Allocate memory for name string
    char *pcName = (char *)Memory_vpAlloc(psName->szLen + 1);
    if (pcName == NULL)
    {
        return SENSORMEASURE_ALLOCATION_ERROR_E;
    }

    // Copy name string
    memcpy((void *)pcName, (const void *)psName->pcStrStart, (size_t)psName->szLen);
    pcName[psName->szLen] = '\0';

    // Call reuse-name version
    return SensorMeasure_eInitReuseName(psHandle, pcName, eType);
}

/**
 * @brief Get measurement source name
 */
const char *SensorMeasure_pcGetName(ISENSORMEASURE_T *psHandle)
{
    if (psHandle == NULL)
        return NULL;
    
    return psHandle->pcName;
}
