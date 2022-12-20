#ifndef SWITCH_H
#define SWITCH_H

#include "LooPre.h"
#include "MySensorsPresent.h"
#include "Types.h"

typedef enum
{
    SWITCH_CLASSIC_E,
    SWITCH_IMPULSE_E
} SWITCH_MODE_T;

typedef enum
{
    SWITCH_OK_E,
    SWITCH_NULLPTR_ERROR_E,
    SWITCH_SUBINIT_ERROR_E,
    SWITCH_ERROR_E
} SWITCH_RESULT_T;

typedef struct
{
    MYSENSORSPRESENT_HANDLE_T sMySenPresent;
    SWITCH_MODE_T eMode;
    uint32_t u32ImpulseDuration;
    uint32_t u32ImpulseStarted;
    uint8_t u8OutPin;
    uint8_t u8FbPin;
} SWITCH_HANDLE_T;

SWITCH_RESULT_T Switch_eInit(
    SWITCH_HANDLE_T *psHandle,
    STRING_POINT_T *sName,
    uint8_t u8Id,
    uint32_t u32ImpulseDuration,
    SWITCH_MODE_T eMode,
    uint8_t u8OutPin,
    uint8_t u8FbPin);

void Switch_vLoop(LOOPRE_T *psBaseHandle, uint32_t u32ms);

#endif // SWITCH_H
