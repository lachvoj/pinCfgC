
// #include <MySensorsCore.h>
#include <string.h>

extern "C"
{
#include "MyMessageMock.h"
#include "MySensorsMock.h"

    char mock_sendSketchInfo_name[50];
    char mock_sendSketchInfo_version[50];
    uint32_t mock_sendSketchInfo_u32Called;
    bool mock_endSketchInfo_bReturn;
    bool sendSketchInfo(const char *name, const char *version)
    {
        (void)strcpy(mock_sendSketchInfo_name, name);
        (void)strcpy(mock_sendSketchInfo_version, version);
        mock_sendSketchInfo_u32Called++;

        return mock_endSketchInfo_bReturn;
    }

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

    uint8_t mock_transportGetNodeId_u8Return;
    uint8_t transportGetNodeId(void)
    {
        return mock_transportGetNodeId_u8Return;
    }

    uint32_t mock_wait_u32WaitMS;
    uint32_t mock_wait_u32Called;
    void wait(const uint32_t u32WaitMS)
    {
        mock_wait_u32WaitMS = u32WaitMS;
        mock_wait_u32Called++;
    }

    // Access the EEPROM mock from test.c
    extern uint8_t mock_EEPROM[1024];

    void *mock_hwWriteConfigBlock_buf;
    void *mock_hwWriteConfigBlock_addr;
    size_t mock_hwWriteConfigBlock_length;
    uint32_t mock_hwWriteConfigBlock_u32Called;
    void hwWriteConfigBlock(void *buf, void *addr, size_t length)
    {
        // addr is actually an integer address cast to pointer
        size_t address = (size_t)addr;
        memcpy(&mock_EEPROM[address], buf, length);

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
        // addr is actually an integer address cast to pointer
        size_t address = (size_t)addr;
        memcpy(buf, &mock_EEPROM[address], length);

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

        mock_sendSketchInfo_u32Called = 0;
        mock_endSketchInfo_bReturn = true;

        mock_send_u32Called = 0;
        mock_send_bReturn = false;
        (void)memset((void *)mock_send_message, 0u, sizeof(mock_send_message));

        mock_transportGetNodeId_u8Return = 0;

        mock_wait_u32Called = 0;
        mock_hwWriteConfigBlock_u32Called = 0;
        mock_hwReadConfigBlock_u32Called = 0;
        mock_hwReadConfigBlock_buf = NULL;

        mock_hwCPUTemperature_i8Return = 0;
        mock_hwCPUTemperature_u32Called = 0;

        init_MyMessageMock();

#ifdef MY_TRANSPORT_ERROR_LOG
        init_TransportErrorLogMock();
#endif
    }

#ifdef MY_TRANSPORT_ERROR_LOG
    TransportErrorLogEntry_t mock_transportErrorLog[16];
    uint8_t mock_transportErrorLogCount = 0;
    uint32_t mock_transportTotalErrorCount = 0;
    static uint8_t mock_transportErrorLogHead = 0;

    void transportLogError(uint8_t errorCode, uint8_t channel, uint8_t extra)
    {
        mock_transportErrorLog[mock_transportErrorLogHead].timestamp = 1000 + mock_transportTotalErrorCount * 100;
        mock_transportErrorLog[mock_transportErrorLogHead].errorCode = errorCode;
        mock_transportErrorLog[mock_transportErrorLogHead].channel = channel;
        mock_transportErrorLog[mock_transportErrorLogHead].extra = extra;
        mock_transportErrorLog[mock_transportErrorLogHead].reserved = 0;

        mock_transportErrorLogHead = (mock_transportErrorLogHead + 1) % 16;
        if (mock_transportErrorLogCount < 16) {
            mock_transportErrorLogCount++;
        }
        mock_transportTotalErrorCount++;
    }

    uint8_t transportGetErrorLogCount(void)
    {
        return mock_transportErrorLogCount;
    }

    bool transportGetErrorLogEntry(uint8_t index, TransportErrorLogEntry_t *entry)
    {
        if (index >= mock_transportErrorLogCount || entry == NULL) {
            return false;
        }
        uint8_t pos;
        if (mock_transportErrorLogCount < 16) {
            pos = index;
        } else {
            pos = (mock_transportErrorLogHead + index) % 16;
        }
        *entry = mock_transportErrorLog[pos];
        return true;
    }

    void transportClearErrorLog(void)
    {
        mock_transportErrorLogHead = 0;
        mock_transportErrorLogCount = 0;
    }

    uint32_t transportGetTotalErrorCount(void)
    {
        return mock_transportTotalErrorCount;
    }

    void init_TransportErrorLogMock(void)
    {
        mock_transportErrorLogHead = 0;
        mock_transportErrorLogCount = 0;
        mock_transportTotalErrorCount = 0;
        memset(mock_transportErrorLog, 0, sizeof(mock_transportErrorLog));
    }
#endif // MY_TRANSPORT_ERROR_LOG
}
