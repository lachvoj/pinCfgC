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
#include "ArduinoMock.h"
#include "MyMessageMock.h"
#include "MySensorsMock.h"
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
    WRAP_RESULT_T eMyMessageInit(
        MyMessage *pcMessage,
        const uint8_t u8SensorId,
        const mysensors_data_t eDataType,
        const mysensors_payload_t ePayloadType,
        const char *pcValue);
    uint8_t eMessageGetByte(const MyMessage *message);

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
    uint32_t u32Millis();
    void vHwWriteConfigBlock(void *buf, void *addr, size_t length);
    void vHwReadConfigBlock(void *buf, void *addr, size_t length);
    uint8_t u8EEPROMRead(int idx);
    int8_t i8HwCPUTemperature(void);

#ifdef __cplusplus
}
#endif

#endif // MYSENSORSWRAPPER_H