#include "PinCfgIfMock.h"

void vPinCfgIfMock_setup(void)
{
    mock_bRequest_u32Called = 0;
    mock_bRequest_bReturn = false;

    mock_bPresent_u32Called = 0;
    mock_bPresent_bReturn = false;

    mock_bSend_u32Called = 0;
    mock_bSend_bReturn = false;

    mock_u8SaveCfg_u32Called = 0;
    mock_u8SaveCfg_u8Return = false;
}

uint8_t mock_bRequest_u8Id;
uint8_t mock_bRequest_u8VariableType;
uint8_t mock_bRequest_u8Destination;
bool mock_bRequest_bReturn;
uint32_t mock_bRequest_u32Called;
bool bRequest(const uint8_t u8Id, const uint8_t u8VariableType, const uint8_t u8Destination)
{
    mock_bRequest_u8Id = u8Id;
    mock_bRequest_u8VariableType = u8VariableType;
    mock_bRequest_u8Destination = u8Destination;
    mock_bRequest_u32Called++;
    return mock_bRequest_bReturn;
}


uint8_t mock_bPresent_u8Id;
uint8_t mock_bPresent_sensorType;
const char *mock_bPresent_pcName;
bool mock_bPresent_bReturn;
uint32_t mock_bPresent_u32Called;
bool bPresent(const uint8_t u8Id, const uint8_t sensorType, const char *pcName)
{
    mock_bPresent_u8Id = u8Id;
    mock_bPresent_sensorType = sensorType;
    mock_bPresent_pcName = pcName;
    mock_bPresent_u32Called++;
    return mock_bPresent_bReturn;
}


uint8_t mock_bSend_u8Id;
uint8_t mock_bSend_variableType;
const void *mock_bSend_pvMessage;
bool mock_bSend_bReturn;
uint32_t mock_bSend_u32Called;
bool bSend(const uint8_t u8Id, const uint8_t variableType, const void *pvMessage)
{
    mock_bSend_u8Id = u8Id;
    mock_bSend_variableType = variableType;
    mock_bSend_pvMessage = pvMessage;
    mock_bSend_u32Called++;
    return mock_bSend_bReturn;
}

const char *mock_u8SaveCfg_pcCfg;
int8_t mock_u8SaveCfg_u8Return;
uint32_t mock_u8SaveCfg_u32Called;
uint8_t u8SaveCfg(const char *pcCfg)
{
    mock_u8SaveCfg_pcCfg = pcCfg;
    mock_u8SaveCfg_u32Called++;
    return mock_u8SaveCfg_u8Return;
}
