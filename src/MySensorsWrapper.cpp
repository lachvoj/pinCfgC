#include <EEPROM.h>

#ifdef UNIT_TEST
extern "C"
{
#include "MySensorsMock.h"
}
#else
#include <core/MySensorsCore.h>
#include <hal/architecture/MyHwHAL.h>
#endif

#include "MySensorsWrapper.h"

extern void hwWriteConfigBlock(void *buf, void *addr, size_t length);
extern void hwReadConfigBlock(void *buf, void *addr, size_t length);

extern "C"
{
    WRAP_RESULT_T eMyMessageInit(MyMessage *message, const uint8_t _sensorId, const mysensors_data_t _dataType)
    {
        if (message == NULL)
            return WRAP_NULLPTR_ERROR_E;

        message->clear();
        message->setSensor(_sensorId);
        message->setType(static_cast<uint8_t>(_dataType));

        return WRAP_OK_E;
    }

    WRAP_RESULT_T eMyMessageSetUInt8(MyMessage *message, const uint8_t value)
    {
        if (message == NULL)
            return WRAP_NULLPTR_ERROR_E;

        message->set(value);

        return WRAP_OK_E;
    }

    WRAP_RESULT_T eMyMessageSetCStr(MyMessage *message, const char *value)
    {
        if (message == NULL)
            return WRAP_NULLPTR_ERROR_E;

        message->set(value);

        return WRAP_OK_E;
    }

    WRAP_RESULT_T eSend(MyMessage *message, const bool requestEcho)
    {
        if (message == NULL)
            return WRAP_NULLPTR_ERROR_E;

        if (send(*message, requestEcho))
            return WRAP_OK_E;

        return WRAP_ERROR_E;
    }

    WRAP_RESULT_T eRequest(const uint8_t childSensorId, const uint8_t variableType, const uint8_t destination)
    {
        if (request(childSensorId, variableType, destination))
            return WRAP_OK_E;

        return WRAP_ERROR_E;
    }

    WRAP_RESULT_T ePresent(
        const uint8_t sensorId,
        const mysensors_sensor_t sensorType,
        const char *description,
        const bool requestEcho)
    {
        if (present(sensorId, sensorType, description, requestEcho))
            return WRAP_OK_E;

        return WRAP_ERROR_E;
    }

    void vWait(const uint32_t waitingMS)
    {
        wait(waitingMS);
    }

    void vHwWriteConfigBlock(void *buf, void *addr, size_t length)
    {
        hwWriteConfigBlock(buf, addr, length);
    }

    void vHwReadConfigBlock(void *buf, void *addr, size_t length)
    {
        hwReadConfigBlock(buf, addr, length);
    }

    uint8_t u8EEPROMRead(int idx)
    {
        return EEPROM.read(idx);
    }

    int8_t i8HwCPUTemperature(void)
    {
        return hwCPUTemperature();
    }
}
