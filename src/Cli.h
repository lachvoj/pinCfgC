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
    CLI_RECEIVING_AUTH_E,     // Receiving fragmented password
    CLI_RECEIVING_TYPE_E,     // Receiving fragmented CFG:/CMD: prefix
    CLI_RECEIVING_CFG_DATA_E, // Receiving config data
    CLI_RECEIVING_CMD_DATA_E, // Receiving command data
    CLI_VALIDATION_OK_E,
#ifdef FWCHECK_ENABLED
    CLI_CRC_ERROR_E,
#endif
    CLI_VALIDATION_ERROR_E,
    CLI_CUSTOM_E
} CLI_STATE_T;

typedef struct CLI_S
{
    PRESENTABLE_T sPresentable;
    LOOPABLE_T sLoopable;
#ifdef FWCHECK_ENABLED
    uint32_t u32LastFwCrcCheckMs;
#endif
    uint32_t u32ReceivingStartedMs;
    char *pcCfgBuf;
    uint16_t u16CfgNext;
    CLI_STATE_T eState;
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
