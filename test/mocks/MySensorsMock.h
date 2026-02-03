#ifndef PINCFGIFMOCK_H
#define PINCFGIFMOCK_H

#include <stdint.h>

#include "MyMessageMock.h"

extern uint8_t mock_bRequest_u8Id;
extern bool mock_bRequest_bReturn;
extern uint32_t mock_bRequest_u32Called;

extern uint8_t mock_bPresent_u8Id;
extern const char *mock_bPresent_pcName;
extern bool mock_bPresent_bReturn;
extern uint32_t mock_bPresent_u32Called;

extern uint8_t mock_transportGetNodeId_u8Return;

extern char mock_sendSketchInfo_name[50];
extern char mock_sendSketchInfo_version[50];
extern uint32_t mock_sendSketchInfo_u32Called;
extern bool mock_endSketchInfo_bReturn;

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
extern void *mock_hwReadConfigBlock_buf_addr;
extern void *mock_hwReadConfigBlock_addr;
extern size_t mock_hwReadConfigBlock_length;
extern uint32_t mock_hwReadConfigBlock_u32Called;

extern int8_t mock_hwCPUTemperature_i8Return;
extern uint32_t mock_hwCPUTemperature_u32Called;

void init_MySensorsMock(void);

bool sendSketchInfo(const char *name, const char *version);
bool send(MyMessage msg, const bool requestEcho);
bool request(const uint8_t u8Id, const uint8_t u8VariableType, const uint8_t u8Destination);
bool present(
    const uint8_t sensorId,
    const mysensors_sensor_t sensorType,
    const char *description,
    const bool requestEcho);
uint8_t transportGetNodeId(void);

void wait(const uint32_t u32WaitMS);
void hwWriteConfigBlock(void *buf, void *addr, size_t length);
void hwReadConfigBlock(void *buf, void *addr, size_t length);
int8_t hwCPUTemperature(void);

// Transport error log mock
#ifdef MY_TRANSPORT_ERROR_LOG

typedef struct
{
    uint32_t timestamp;
    uint8_t errorCode;
    uint8_t channel;
    uint8_t extra;
    uint8_t reserved;
} TransportErrorLogEntry_t;

extern TransportErrorLogEntry_t mock_transportErrorLog[16];
extern uint8_t mock_transportErrorLogCount;
extern uint32_t mock_transportTotalErrorCount;

void transportLogError(uint8_t errorCode, uint8_t channel, uint8_t extra);
uint8_t transportGetErrorLogCount(void);
bool transportGetErrorLogEntry(uint8_t index, TransportErrorLogEntry_t *entry);
void transportClearErrorLog(void);
uint32_t transportGetTotalErrorCount(void);
void init_TransportErrorLogMock(void);

#endif // MY_TRANSPORT_ERROR_LOG

#endif // PINCFGIFMOCK_H
