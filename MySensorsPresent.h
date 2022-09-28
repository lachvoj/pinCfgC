#ifndef MYSENSORSPRESENT_H
#define MYSENSORSPRESENT_H

#include "LooPreIf.h"
#include "MysensorsIf.h"
#include "PinCfgStr.h"
#include "Types.h"

typedef struct
{
    LOOPRE_IF_T sLooPreIf;
    char *pcName;
    uint8_t u8Id;
    uint8_t u8State;
    bool bStateChanged;
    bool bPresent;
} MYSENSORSPRESENT_HANDLE_T;

typedef enum
{
    MYSENSORSPRESENT_OK_E,
    MYSENSORSPRESENT_ALLOCATION_ERROR_E,
    MYSENSORSPRESENT_NULLPTR_ERROR_E,
    MYSENSORSPRESENT_ERROR_E
} MYSENSORSPRESENT_RESULT_T;

MYSENSORSPRESENT_RESULT_T MySensorsPresent_eInit(
    MYSENSORSPRESENT_HANDLE_T *psHandle,
    STRING_POINT_T *psName,
    uint8_t u8Id,
    PINCFG_ELEMENT_TYPE_T eType,
    bool bPresent);
void MySensorsPresent_vSendMySensorsStatus(MYSENSORSPRESENT_HANDLE_T *psHandle);
void MySensorsPresent_vSetState(MYSENSORSPRESENT_HANDLE_T *psHandle, uint8_t u8State);
void MySensorsPresent_vToggle(MYSENSORSPRESENT_HANDLE_T *psHandle);

// presentable IF
uint8_t MySensorsPresent_u8GetId(MYSENSORSPRESENT_HANDLE_T *psHandle);
const char *MySensorsPresent_pcGetName(MYSENSORSPRESENT_HANDLE_T *psHandle);
void MySensorsPresent_vRcvMessage(MYSENSORSPRESENT_HANDLE_T *psHandle, const void *pvMessage);
void MySensorsPresent_vPresent(MYSENSORSPRESENT_HANDLE_T *psHandle);
void MySensorsPresent_vPresentState(MYSENSORSPRESENT_HANDLE_T *psHandle);

#endif // MYSENSORSPRESENT_H
