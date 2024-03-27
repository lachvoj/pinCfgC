#ifndef PINCFGIFMOCK_H
#define PINCFGIFMOCK_H

#include <core/MyMessage.h>
#include <stdint.h>

extern uint8_t mock_bRequest_u8Id;
extern bool mock_bRequest_bReturn;
extern uint32_t mock_bRequest_u32Called;

extern uint8_t mock_bPresent_u8Id;
extern const char *mock_bPresent_pcName;
extern bool mock_bPresent_bReturn;
extern uint32_t mock_bPresent_u32Called;

extern MyMessage mock_send_msg;
extern bool mock_send_requestEcho;
extern char mock_send_message[MAX_PAYLOAD_SIZE * 30];
extern bool mock_send_bReturn;
extern uint32_t mock_send_u32Called;

extern uint32_t mock_wait_u32WaitMS;
extern uint32_t mock_wait_u32Called;

extern void *mock_hwWriteConfigBlock_buf;
extern void *mock_hwWriteConfigBlock_addr;
extern size_t mock_hwWriteConfigBlock_length;
extern uint32_t mock_hwWriteConfigBlock_u32Called;

extern void *mock_hwReadConfigBlock_buf;
extern void *mock_hwReadConfigBlock_addr;
extern size_t mock_hwReadConfigBlock_length;
extern uint32_t mock_hwReadConfigBlock_u32Called;

extern int8_t mock_hwCPUTemperature_i8Return;
extern uint32_t mock_hwCPUTemperature_u32Called;

void init_MySensorsMock(void);

#endif // PINCFGIFMOCK_H
