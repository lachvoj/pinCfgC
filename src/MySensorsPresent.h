#ifndef MYSENSORSPRESENT_H
#define MYSENSORSPRESENT_H

#include "LooPre.h"
#include "PinCfgStr.h"
#include "PincfgIf.h"
#include "Types.h"

typedef struct
{
    LOOPRE_T sLooPre;
    uint8_t u8State;
    bool bStateChanged;
} MYSENSORSPRESENT_HANDLE_T;

typedef enum
{
    MYSENSORSPRESENT_OK_E,
    MYSENSORSPRESENT_ALLOCATION_ERROR_E,
    MYSENSORSPRESENT_NULLPTR_ERROR_E,
    MYSENSORSPRESENT_SUBINIT_ERROR_E,
    MYSENSORSPRESENT_ERROR_E
} MYSENSORSPRESENT_RESULT_T;

MYSENSORSPRESENT_RESULT_T MySensorsPresent_eInit(
    MYSENSORSPRESENT_HANDLE_T *psHandle,
    STRING_POINT_T *psName,
    uint8_t u8Id);
void MySensorsPresent_vSetState(MYSENSORSPRESENT_HANDLE_T *psHandle, uint8_t u8State, bool bSendStatus);
void MySensorsPresent_vToggle(MYSENSORSPRESENT_HANDLE_T *psHandle);

// presentable IF
void MySensorsPresent_vRcvMessage(LOOPRE_T *psBaseHandle, const void *pvMessage);
void MySensorsPresent_vPresent(LOOPRE_T *psBaseHandle);
void MySensorsPresent_vPresentState(LOOPRE_T *psBaseHandle);
void MySensorsPresent_vSendState(LOOPRE_T *psBaseHandle);

#endif // MYSENSORSPRESENT_H
