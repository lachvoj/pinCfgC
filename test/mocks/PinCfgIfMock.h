#ifndef PINCFGIFMOCK_H
#define PINCFGIFMOCK_H

#include <PincfgIf.h>
#include <Types.h>

#define SEND_MESSAGE_MAX_LENGTH_D 3000

extern uint8_t mock_bRequest_u8Id;
extern bool mock_bRequest_bReturn;
extern uint32_t mock_bRequest_u32Called;

extern uint8_t mock_bRequestText_u8Id;
extern bool mock_bRequestText_bReturn;
extern uint32_t mock_bRequestText_u32Called;


extern uint8_t mock_bPresent_u8Id;
extern const char *mock_bPresent_pcName;
extern bool mock_bPresent_bReturn;
extern uint32_t mock_bPresent_u32Called;

extern uint8_t mock_bPresentInfo_u8Id;
extern const char *mock_bPresentInfo_pcName;
extern bool mock_bPresentInfo_bReturn;
extern uint32_t mock_bPresentInfo_u32Called;

extern uint8_t mock_bSend_u8Id;
extern const void *mock_bSend_pvMessage;
extern char mock_bSend_acMessage[SEND_MESSAGE_MAX_LENGTH_D];
extern bool mock_bSend_bReturn;
extern uint32_t mock_bSend_u32Called;

extern uint8_t mock_bSendText_u8Id;
extern const void *mock_bSendText_pvMessage;
extern bool mock_bSendText_bReturn;
extern uint32_t mock_bSendText_u32Called;

extern const char *mock_u8SaveCfg_pcCfg;
extern int8_t mock_u8SaveCfg_u8Return;
extern uint32_t mock_u8SaveCfg_u32Called;

extern uint32_t mock_vWait_u32Called;

void vPinCfgIfMock_setup(void);

bool bRequest(const uint8_t u8Id, const uint8_t u8VariableType, const uint8_t u8Destination);
bool bPresent(const uint8_t u8Id, const uint8_t sensorType, const char *pcName);
bool bSend(const uint8_t u8Id, const uint8_t variableType, const void *pvMessage);
uint8_t u8SaveCfg(const char *pcCfg);
void vWait(const uint32_t u32WaitMS);

#endif // PINCFGIFMOCK_H
