#include <EEPROM.h>

#if defined(UNIT_TEST)
extern "C"
{
#include "MySensorsMock.h"
}
#else
#include <core/MySensorsCore.h>
#include <hal/architecture/MyHwHAL.h>
#if defined(ARDUINO_ARCH_STM32)
#include <hal/architecture/STM32/MyHwSTM32.h>
#endif
#endif

#include "MySensorsWrapper.h"

#if defined(ARDUINO_ARCH_AVR)
#include <hal/architecture/AVR/MyHwAVR.h>
#else
extern void hwWriteConfigBlock(void *buf, void *addr, size_t length);
extern void hwReadConfigBlock(void *buf, void *addr, size_t length);
#endif

#ifdef __cplusplus
extern "C"
{
#endif
    WRAP_RESULT_T eMyMessageInit(
        MyMessage *pcMessage,
        const uint8_t u8SensorId,
        const mysensors_data_t eDataType,
        const mysensors_payload_t ePayloadType,
        const char *pcValue)
    {
        if (pcMessage == NULL)
            return WRAP_NULLPTR_ERROR_E;

        pcMessage->clear();
        pcMessage->setSensor(u8SensorId);
        pcMessage->setType(static_cast<uint8_t>(eDataType));
        switch (ePayloadType)
        {
        case P_STRING: pcMessage->set(pcValue); break;
        case P_BYTE: pcMessage->set((uint8_t)(uintptr_t)pcValue); break;
        case P_INT16: pcMessage->set((int16_t)(uintptr_t)pcValue); break;
        case P_UINT16: pcMessage->set((uint16_t)(uintptr_t)pcValue); break;
        case P_LONG32: pcMessage->set((int32_t)(uintptr_t)pcValue); break;
        case P_ULONG32: pcMessage->set((uint32_t)(uintptr_t)pcValue); break;
        }

        return WRAP_OK_E;
    }

    WRAP_RESULT_T eMyMessageSetCStr(MyMessage *message, const char *value)
    {
        if (message == NULL)
            return WRAP_NULLPTR_ERROR_E;

        message->set(value);

        return WRAP_OK_E;
    }

    uint8_t eMessageGetByte(const MyMessage *message)
    {
        if (message == NULL)
            return 0;

        return (uint8_t)message->getByte();
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

    uint32_t u32Millis()
    {
        return millis();
    }

    uint32_t u32Micros()
    {
        return micros();
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

#ifdef UNIT_TEST
    // Mock variables defined in ArduinoMock.c
    extern uint16_t mock_analogRead_u16Return;
    extern uint32_t mock_analogRead_u32Called;
    extern uint8_t mock_analogRead_u8LastPin;
#endif

    uint16_t u16HwAnalogRead(uint8_t u8Pin)
    {
#ifdef UNIT_TEST
        mock_analogRead_u32Called++;
        mock_analogRead_u8LastPin = u8Pin;
        return mock_analogRead_u16Return;
#elif defined(ARDUINO_ARCH_STM32)
    return hwAnalogRead(u8Pin);
#else
    // Fallback to Arduino analogRead for other platforms
    return (uint16_t)analogRead(u8Pin);
#endif
    }

#ifdef MY_TRANSPORT_ERROR_LOG
    uint8_t u8TransportGetErrorLogCount(void)
    {
        return transportGetErrorLogCount();
    }

    bool bTransportGetErrorLogEntry(uint8_t u8Index, TransportErrorLogEntry_t *psEntry)
    {
        return transportGetErrorLogEntry(u8Index, psEntry);
    }

    uint32_t u32TransportGetTotalErrorCount(void)
    {
        return transportGetTotalErrorCount();
    }

    void vTransportClearErrorLog(void)
    {
        transportClearErrorLog();
    }
#endif
#ifdef __cplusplus
}
#endif
