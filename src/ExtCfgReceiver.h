#ifndef EXTCFGRECEIVER_H
#define EXTCFGRECEIVER_H

#include "LooPre.h"
#include "PinCfgStr.h"
#include "Types.h"

typedef enum
{
    EXTCFGRECEIVER_LISTENING_E,
    EXTCFGRECEIVER_RECEIVNG_E,
    EXTCFGRECEIVER_RECEIVED_E,
    EXTCFGRECEIVER_VALIDATING_E,
    EXTCFGRECEIVER_VALIDATION_OK_E,
    EXTCFGRECEIVER_VALIDATION_ERROR_E,
    EXTCFGRECEIVER_CUSTOM_E
} EXTCFGRECEIVER_STATE_T;

typedef struct
{
    LOOPRE_T sLooPre;
    char acState[PINCFG_TXTSTATE_MAX_SZ_D];
    uint16_t u16CfgNext;
    EXTCFGRECEIVER_STATE_T eState;
} EXTCFGRECEIVER_HANDLE_T;

typedef enum
{
    EXTCFGRECEIVER_OK_E,
    EXTCFGRECEIVER_ALLOCATION_ERROR_E,
    EXTCFGRECEIVER_NULLPTR_ERROR_E,
    EXTCFGRECEIVER_ERROR_E
} EXTCFGRECEIVER_RESULT_T;

EXTCFGRECEIVER_RESULT_T ExtCfgReceiver_eInit(EXTCFGRECEIVER_HANDLE_T *psHandle, uint8_t u8Id);

void ExtCfgReceiver_vSetState(
    EXTCFGRECEIVER_HANDLE_T *psHandle,
    EXTCFGRECEIVER_STATE_T estate,
    const char *psState,
    bool bSendState);

// presentable IF
void ExtCfgReceiver_vRcvMessage(LOOPRE_T *psBaseHandle, const char *pcMessage);
void ExtCfgReceiver_vPresent(LOOPRE_T *psBaseHandle);
void ExtCfgReceiver_vPresentState(LOOPRE_T *psBaseHandle);

#endif // EXTCFGRECEIVER_H
