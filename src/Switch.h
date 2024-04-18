#ifndef SWITCH_H
#define SWITCH_H

#include "ILoopable.h"
#include "Presentable.h"
#include "Types.h"

typedef enum SWITCH_MODE_E
{
    SWITCH_CLASSIC_E = 0,
    SWITCH_IMPULSE_E
} SWITCH_MODE_T;

typedef enum SWITCH_RESULT_E
{
    SWITCH_OK_E = 0,
    SWITCH_NULLPTR_ERROR_E,
    SWITCH_SUBINIT_ERROR_E,
    SWITCH_ERROR_E
} SWITCH_RESULT_T;

typedef struct SWITCH_S
{
    PRESENTABLE_T sPresentable;
    LOOPABLE_T sLoopable;
    uint32_t u32ImpulseDuration;
    uint32_t u32ImpulseStarted;
    uint32_t u32FbReadStarted;
    uint8_t u8OutPin;
    uint8_t u8FbPin;
    SWITCH_MODE_T eMode;
} SWITCH_T;

// static
void Switch_SetImpulseDurationMs(uint32_t u32ImpulseDuration);
void Switch_SetFbOnDelayMs(uint32_t u32FbOnDelayMs);
void Switch_SetFbOffDelayMs(uint32_t u32FbOffDelayMs);
void Switch_vSetTimedActionAdditionMs(uint32_t u32TimedActionAdditionMs);

// type init
void Switch_vInitType(PRESENTABLE_VTAB_T *psVtab);

// ctor
SWITCH_RESULT_T Switch_eInit(
    SWITCH_T *psHandle,
    STRING_POINT_T *sName,
    uint8_t u8Id,
    SWITCH_MODE_T eMode,
    uint8_t u8OutPin,
    uint8_t u8FbPin);

void Switch_vTimedAction(SWITCH_T *psHandle);

// loopable IF
void Switch_vLoop(LOOPABLE_T *psLoopableHandle, uint32_t u32ms);


#endif // SWITCH_H
