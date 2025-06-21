
// #include <MySensorsCore.h>
#include <string.h>

extern "C"
{
#include "MyMessageMock.h"
#include "MySensorsMock.h"

    uint8_t mock_bRequest_u8Id;
    uint8_t mock_bRequest_u8VariableType;
    uint8_t mock_bRequest_u8Destination;
    bool mock_bRequest_bReturn;
    uint32_t mock_bRequest_u32Called;
    bool request(const uint8_t u8Id, const uint8_t u8VariableType, const uint8_t u8Destination)
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
    bool mock_bPresent_requestEcho;
    bool mock_bPresent_bReturn;
    uint32_t mock_bPresent_u32Called;
    bool present(const uint8_t u8Id, const mysensors_sensor_t sensorType, const char *pcName, const bool requestEcho)
    {
        mock_bPresent_u8Id = u8Id;
        mock_bPresent_sensorType = sensorType;
        mock_bPresent_pcName = pcName;
        mock_bPresent_requestEcho = requestEcho;
        mock_bPresent_u32Called++;
        return mock_bPresent_bReturn;
    }

    MyMessage mock_send_msg;
    bool mock_send_requestEcho;
    char mock_send_message[MAX_PAYLOAD_SIZE * 30];
    bool mock_send_bReturn;
    uint32_t mock_send_u32Called;
    bool send(MyMessage msg, const bool requestEcho)
    {
        mock_send_msg = msg;
        mock_send_requestEcho = requestEcho;
        const char *pcStrMsg = msg.getString();
        if (pcStrMsg != NULL)
        {
            strcat(mock_send_message, msg.getString());
            strcat(mock_send_message, ";");
        }
        // mock_send_message = (MyMessageMock)msg;
        mock_send_u32Called++;
        return mock_send_bReturn;
    }

    uint32_t mock_wait_u32WaitMS;
    uint32_t mock_wait_u32Called;
    void wait(const uint32_t u32WaitMS)
    {
        mock_wait_u32WaitMS = u32WaitMS;
        mock_wait_u32Called++;
    }

    void *mock_hwWriteConfigBlock_buf;
    void *mock_hwWriteConfigBlock_addr;
    size_t mock_hwWriteConfigBlock_length;
    uint32_t mock_hwWriteConfigBlock_u32Called;
    void hwWriteConfigBlock(void *buf, void *addr, size_t length)
    {
        mock_hwWriteConfigBlock_buf = buf;
        mock_hwWriteConfigBlock_addr = addr;
        mock_hwWriteConfigBlock_length = length;
        mock_hwWriteConfigBlock_u32Called++;
    }

    void *mock_hwReadConfigBlock_buf;
    void *mock_hwReadConfigBlock_buf_addr;
    void *mock_hwReadConfigBlock_addr;
    size_t mock_hwReadConfigBlock_length;
    uint32_t mock_hwReadConfigBlock_u32Called;
    void hwReadConfigBlock(void *buf, void *addr, size_t length)
    {
        if (mock_hwReadConfigBlock_buf != NULL)
        {
            memcpy(buf, mock_hwReadConfigBlock_buf, length);
        }
        mock_hwReadConfigBlock_buf_addr = buf;
        mock_hwReadConfigBlock_addr = addr;
        mock_hwReadConfigBlock_length = length;
        mock_hwReadConfigBlock_u32Called++;
    }

    int8_t mock_hwCPUTemperature_i8Return;
    uint32_t mock_hwCPUTemperature_u32Called;
    int8_t hwCPUTemperature(void)
    {
        mock_hwCPUTemperature_u32Called++;
        return mock_hwCPUTemperature_i8Return;
    }

    void init_MySensorsMock(void)
    {
        mock_bRequest_u32Called = 0;
        mock_bRequest_bReturn = false;

        mock_bPresent_u32Called = 0;
        mock_bPresent_bReturn = false;

        mock_send_u32Called = 0;
        mock_send_bReturn = false;
        (void)memset((void *)mock_send_message, 0u, sizeof(mock_send_message));

        mock_wait_u32Called = 0;
        mock_hwWriteConfigBlock_u32Called = 0;
        mock_hwReadConfigBlock_u32Called = 0;

        mock_hwCPUTemperature_i8Return = 0;
        mock_hwCPUTemperature_u32Called = 0;

        init_MyMessageMock();
    }
}
