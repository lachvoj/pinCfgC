#ifndef CLI_H
#define CLI_H

#include "ILoopable.h"
#include "PinCfgStr.h"
#include "Presentable.h"
#include "Types.h"

typedef enum CLI_RESULT_E
{
    CLI_OK_E = 0,
    CLI_ALLOCATION_ERROR_E,
    CLI_NULLPTR_ERROR_E,
    CLI_ERROR_E
} CLI_RESULT_T;

typedef enum CLI_STATE_E
{
    CLI_READY_E = 0,
    CLI_OUT_OF_MEM_ERR_E,
    CLI_RECEIVING_AUTH_E, // Receiving fragmented password
    CLI_RECEIVING_TYPE_E, // Receiving fragmented CFG:/CMD: prefix
    CLI_RECEIVING_DATA_E, // Receiving config/command data (after type determined)
    CLI_RECEIVED_E,
    CLI_VALIDATING_E,
    CLI_VALIDATION_OK_E,
    CLI_VALIDATION_ERROR_E,
    CLI_CUSTOM_E
} CLI_STATE_T;

typedef enum CLI_MODE_E
{
    CLI_LINE_E = 0,
    CLI_FULL_E
} CLI_MODE_T;

typedef struct CLI_S
{
    PRESENTABLE_T sPresentable;
    char *pcCfgBuf;
    uint16_t u16CfgNext;
    CLI_STATE_T eState;
    CLI_MODE_T eMode;
    bool bIsConfig;      // true = CFG, false = CMD (determined after pwd)
    bool bAuthenticated; // Authentication status
    // Rate limiting for brute-force protection
    uint8_t u8FailedAttempts;   // Consecutive failed attempts
    uint32_t u32LockoutUntilMs; // Lockout end timestamp (0 = not locked)
} CLI_T;

void Cli_vInitType(PRESENTABLE_VTAB_T *psVtab);

CLI_RESULT_T Cli_eInit(CLI_T *psHandle, uint8_t u8Id);

void Cli_vSetState(CLI_T *psHandle, CLI_STATE_T estate, const char *psState, bool bSendState);

// presentable IF
void Cli_vRcvMessage(PRESENTABLE_T *psBaseHandle, const MyMessage *pcMsg);

#endif // CLI_H
