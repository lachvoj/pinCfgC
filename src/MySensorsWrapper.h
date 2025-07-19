#ifndef MYSENSORSWRAPPER_H
#define MYSENSORSWRAPPER_H

#include "Types.h"

#ifndef GATEWAY_ADDRESS
#define GATEWAY_ADDRESS ((uint8_t)0)
#endif

#ifndef AUTO
#define AUTO (255u)
#endif

#ifdef UNIT_TEST
#include "MyMessageMock.h"
#include "MySensorsMock.h"
#include "ArduinoMock.h"
#else
#include <core/MyMessage.h>
#endif

#ifdef __cplusplus
extern "C"
{
#endif

    typedef enum
    {
        WRAP_OK_E,
        WRAP_NULLPTR_ERROR_E,
        WRAP_ERROR_E
    } WRAP_RESULT_T;

    // MyMessage
    WRAP_RESULT_T eMyMessageInit(MyMessage *message, const uint8_t _sensorId, const mysensors_data_t _dataType);
    WRAP_RESULT_T eMyMessageSetUInt8(MyMessage *message, const uint8_t value);
    WRAP_RESULT_T eMyMessageSetCStr(MyMessage *message, const char *value);

    // MySensorsCore
    WRAP_RESULT_T eSend(MyMessage *message, const bool requestEcho);
    WRAP_RESULT_T eRequest(const uint8_t childSensorId, const uint8_t variableType, const uint8_t destination);
    WRAP_RESULT_T ePresent(
        const uint8_t sensorId,
        const mysensors_sensor_t sensorType,
        const char *description,
        const bool requestEcho);
    void vWait(const uint32_t waitingMS);

    // HAL
    void vHwWriteConfigBlock(void *buf, void *addr, size_t length);
    void vHwReadConfigBlock(void *buf, void *addr, size_t length);
    uint8_t u8EEPROMRead(int idx);
    int8_t i8HwCPUTemperature(void);

#ifdef __cplusplus
}
#endif

#endif // MYSENSORSWRAPPER_H