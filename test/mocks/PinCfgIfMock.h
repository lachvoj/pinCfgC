#ifndef PINCFGIFMOCK_H
#define PINCFGIFMOCK_H

#include <PincfgIf.h>
#include <Types.h>

extern uint8_t mock_bRequest_u8Id;
extern PINCFG_ELEMENT_TYPE_T mock_bRequest_eType;
extern bool mock_bRequest_bReturn;
extern uint32_t mock_bRequest_u32Called;

extern uint8_t mock_bPresent_u8Id;
extern uint8_t mock_bPresent_eType;
extern const char *mock_bPresent_pcName;
extern bool mock_bPresent_bReturn;
extern uint32_t mock_bPresent_u32Called;

extern uint8_t mock_bSend_u8Id;
extern uint8_t mock_bSend_eType;
extern const void *mock_bSend_pvMessage;
extern bool mock_bSend_bReturn;
extern uint32_t mock_bSend_u32Called;

extern const char *mock_u8SaveCfg_pcCfg;
extern int8_t mock_u8SaveCfg_u8Return;
extern uint32_t mock_u8SaveCfg_u32Called;

extern PINCFG_IF_T sPincfgIf;

void vPinCfgIfMock_setup(void);

#endif // PINCFGIFMOCK_H
