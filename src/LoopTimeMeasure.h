#ifndef LOOPTIMEMEASURE_H
#define LOOPTIMEMEASURE_H

#ifdef PINCFG_FEATURE_LOOPTIME_MEASUREMENT

#include <stdbool.h>
#include <stdint.h>

#include "ISensorMeasure.h"
#include "PinCfgStr.h"
#include "Types.h"

typedef enum LOOPTIMEMEASURE_RESULT_E
{
    LOOPTIMEMEASURE_OK_E = 0,
    LOOPTIMEMEASURE_NULLPTR_ERROR_E,
    LOOPTIMEMEASURE_ERROR_E
} LOOPTIMEMEASURE_RESULT_T;

typedef struct LOOPTIMEMEASURE_S
{
    ISENSORMEASURE_T sSensorMeasure; // Interface (includes eType and pcName) (12/16 bytes)
    uint32_t u32LastCallTime;        // Timestamp of previous eMeasure() call (4 bytes)
    bool bFirstCall;                 // True until first valid measurement (1 byte, 3 bytes padding)
} LOOPTIMEMEASURE_T;

LOOPTIMEMEASURE_RESULT_T LoopTimeMeasure_eInit(LOOPTIMEMEASURE_T *psHandle, STRING_POINT_T *psName);

#endif // PINCFG_FEATURE_LOOPTIME_MEASUREMENT

#endif // LOOPTIMEMEASURE_H
