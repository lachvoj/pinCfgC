#ifndef EXTCFGRECEIVER_H
#define EXTCFGRECEIVER_H

#include "LooPreIf.h"
#include "PincfgIf.h"
#include "PinCfgStr.h"
#include "Types.h"

typedef enum
{
    EXTCFGRECEIVER_OFF_E,
    EXTCFGRECEIVER_RECEIVNG_E,
    EXTCFGRECEIVER_RECEIVED_E,
    EXTCFGRECEIVER_VALIDATING_E,
    EXTCFGRECEIVER_VALIDATION_OK_E,
    EXTCFGRECEIVER_VALIDATION_ERROR_E
} EXTCFGRECEIVER_STATE_T;

typedef struct
{
    LOOPRE_IF_T sLooPreIf;
    const char *pcName;
    char acState[PINCFG_TXTSTATE_MAX_SZ_D];
    uint16_t u16CfgNext;
    EXTCFGRECEIVER_STATE_T eState;
    uint8_t u8Id;
} EXTCFGRECEIVER_HANDLE_T;

typedef enum
{
    EXTCFGRECEIVER_OK_E,
    EXTCFGRECEIVER_ALLOCATION_ERROR_E,
    EXTCFGRECEIVER_NULLPTR_ERROR_E,
    EXTCFGRECEIVER_ERROR_E
} EXTCFGRECEIVER_RESULT_T;

EXTCFGRECEIVER_RESULT_T ExtCfgReceiver_eInit(
    EXTCFGRECEIVER_HANDLE_T *psHandle,
    uint8_t u8Id,
    bool bPresent);

void ExtCfgReceiver_vSetState(EXTCFGRECEIVER_HANDLE_T *psHandle, const char *psState);

// presentable IF
uint8_t ExtCfgReceiver_u8GetId(EXTCFGRECEIVER_HANDLE_T *psHandle);
const char *ExtCfgReceiver_pcGetName(EXTCFGRECEIVER_HANDLE_T *psHandle);
void ExtCfgReceiver_vRcvMessage(EXTCFGRECEIVER_HANDLE_T *psHandle, const void *pvMessage);
void ExtCfgReceiver_vPresent(EXTCFGRECEIVER_HANDLE_T *psHandle);
void ExtCfgReceiver_vPresentState(EXTCFGRECEIVER_HANDLE_T *psHandle);

#endif // EXTCFGRECEIVER_H
