#ifndef MYSENSORSPRESENT_H
#define MYSENSORSPRESENT_H

#include "LooPreIf.h"
#include "PincfgIf.h"
#include "PinCfgStr.h"
#include "Types.h"

typedef struct
{
    LOOPRE_IF_T sLooPreIf;
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
    uint8_t u8Id,
    PINCFG_ELEMENT_TYPE_T eType);
void MySensorsPresent_vSendMySensorsStatus(MYSENSORSPRESENT_HANDLE_T *psHandle);
void MySensorsPresent_vSetState(MYSENSORSPRESENT_HANDLE_T *psHandle, uint8_t u8State, bool bSendStatus);
void MySensorsPresent_vToggle(MYSENSORSPRESENT_HANDLE_T *psHandle);

// presentable IF
void MySensorsPresent_vRcvMessage(MYSENSORSPRESENT_HANDLE_T *psHandle, uint8_t u8State);
void MySensorsPresent_vPresent(MYSENSORSPRESENT_HANDLE_T *psHandle);
void MySensorsPresent_vPresentState(MYSENSORSPRESENT_HANDLE_T *psHandle);

#endif // MYSENSORSPRESENT_H
