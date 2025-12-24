#ifndef MYMESSAGEMOCK_H
#define MYMESSAGEMOCK_H

#include <stdbool.h>
#include <stdint.h>

#define SEND_MESSAGE_MAX_LENGTH_D 3000

extern uint32_t mock_MyMessage_clear_u32Called;

extern uint8_t mock_MyMessage_setSensor_sensorId;
extern uint32_t mock_MyMessage_setSensor_u32Called;

extern uint8_t mock_MyMessage_setType_messageType;
extern uint32_t mock_MyMessage_setType_u32Called;

extern uint8_t mock_MyMessage_set_uint8_t_value;
extern uint32_t mock_MyMessage_set_uint8_t_u32Called;

extern char mock_MyMessage_set_char_value[SEND_MESSAGE_MAX_LENGTH_D];
extern uint32_t mock_MyMessage_set_pChar_u32Called;

extern bool mock_MyMessage_set_bool_value;
extern uint32_t mock_MyMessage_set_bool_u32Called;

extern int16_t mock_MyMessage_set_int16_t_value;
extern uint32_t mock_MyMessage_set_int16_t_u32Called;

extern uint16_t mock_MyMessage_set_uint16_t_value;
extern uint32_t mock_MyMessage_set_uint16_t_u32Called;

extern int32_t mock_MyMessage_set_int32_t_value;
extern uint32_t mock_MyMessage_set_int32_t_u32Called;

extern uint32_t mock_MyMessage_set_uint32_t_value;
extern uint32_t mock_MyMessage_set_uint32_t_u32Called;

extern uint8_t mock_MyMessage_getByte_returnValue;
extern uint32_t mock_MyMessage_getByte_u32Called;

extern uint32_t mock_MyMessage_getCustom_u32Called;

extern uint32_t mock_MyMessage_getPayloadType_u32Called;

extern uint32_t mock_MyMessage_getString_u32Called;

void init_MyMessageMock(void);

// from MyMessage.h

#define GATEWAY_ADDRESS ((uint8_t)0) //!< Node ID for GW sketch

#define V2_MYS_HEADER_PROTOCOL_VERSION (2u)
#define V2_MYS_HEADER_SIZE (7u)
#define V2_MYS_HEADER_MAX_MESSAGE_SIZE (32u)

#define V2_MYS_HEADER_VSL_VERSION_POS (0)
#define V2_MYS_HEADER_VSL_VERSION_SIZE (2u)
#define V2_MYS_HEADER_VSL_SIGNED_POS (2u)
#define V2_MYS_HEADER_VSL_SIGNED_SIZE (1u)
#define V2_MYS_HEADER_VSL_LENGTH_POS (3u)
#define V2_MYS_HEADER_VSL_LENGTH_SIZE (5u)

#define V2_MYS_HEADER_CEP_COMMAND_POS (0)
#define V2_MYS_HEADER_CEP_COMMAND_SIZE (3u)
#define V2_MYS_HEADER_CEP_ECHOREQUEST_POS (3u)
#define V2_MYS_HEADER_CEP_ECHOREQUEST_SIZE (1u)
#define V2_MYS_HEADER_CEP_ECHO_POS (4u)
#define V2_MYS_HEADER_CEP_ECHO_SIZE (1u)
#define V2_MYS_HEADER_CEP_PAYLOADTYPE_POS (5u)
#define V2_MYS_HEADER_CEP_PAYLOADTYPE_SIZE (3u)

#define MAX_MESSAGE_SIZE V2_MYS_HEADER_MAX_MESSAGE_SIZE
#define HEADER_SIZE V2_MYS_HEADER_SIZE
#define MAX_PAYLOAD_SIZE (MAX_MESSAGE_SIZE - HEADER_SIZE)

#define MAX_PAYLOAD MAX_PAYLOAD_SIZE

typedef enum
{
    C_PRESENTATION = 0,
    C_SET = 1,
    C_REQ = 2,
    C_INTERNAL = 3,
    C_STREAM = 4,
    C_RESERVED_5 = 5,
    C_RESERVED_6 = 6,
    C_INVALID_7 = 7
} mysensors_command_t;

typedef enum
{
    S_DOOR = 0,
    S_MOTION = 1,
    S_SMOKE = 2,
    S_BINARY = 3,
    S_LIGHT = 3,
    S_DIMMER = 4,
    S_COVER = 5,
    S_TEMP = 6,
    S_HUM = 7,
    S_BARO = 8,
    S_WIND = 9,
    S_RAIN = 10,
    S_UV = 11,
    S_WEIGHT = 12,
    S_POWER = 13,
    S_HEATER = 14,
    S_DISTANCE = 15,
    S_LIGHT_LEVEL = 16,
    S_ARDUINO_NODE = 17,
    S_ARDUINO_REPEATER_NODE = 18,
    S_LOCK = 19,
    S_IR = 20,
    S_WATER = 21,
    S_AIR_QUALITY = 22,
    S_CUSTOM = 23,
    S_DUST = 24,
    S_SCENE_CONTROLLER = 25,
    S_RGB_LIGHT = 26,
    S_RGBW_LIGHT = 27,
    S_COLOR_SENSOR = 28,
    S_HVAC = 29,
    S_MULTIMETER = 30,
    S_SPRINKLER = 31,
    S_WATER_LEAK = 32,
    S_SOUND = 33,
    S_VIBRATION = 34,
    S_MOISTURE = 35,
    S_INFO = 36,
    S_GAS = 37,
    S_GPS = 38,
    S_WATER_QUALITY = 39
} mysensors_sensor_t;

typedef enum
{
    V_TEMP = 0,
    V_HUM = 1,
    V_STATUS = 2,
    V_LIGHT = 2,
    V_PERCENTAGE = 3,
    V_DIMMER = 3,
    V_PRESSURE = 4,
    V_FORECAST = 5,
    V_RAIN = 6,
    V_RAINRATE = 7,
    V_WIND = 8,
    V_GUST = 9,
    V_DIRECTION = 10,
    V_UV = 11,
    V_WEIGHT = 12,
    V_DISTANCE = 13,
    V_IMPEDANCE = 14,
    V_ARMED = 15,
    V_TRIPPED = 16,
    V_WATT = 17,
    V_KWH = 18,
    V_SCENE_ON = 19,
    V_SCENE_OFF = 20,
    V_HVAC_FLOW_STATE = 21,
    V_HEATER = 21,
    V_HVAC_SPEED = 22,
    V_LIGHT_LEVEL = 23,
    V_VAR1 = 24,
    V_VAR2 = 25,
    V_VAR3 = 26,
    V_VAR4 = 27,
    V_VAR5 = 28,
    V_UP = 29,
    V_DOWN = 30,
    V_STOP = 31,
    V_IR_SEND = 32,
    V_IR_RECEIVE = 33,
    V_FLOW = 34,
    V_VOLUME = 35,
    V_LOCK_STATUS = 36,
    V_LEVEL = 37,
    V_VOLTAGE = 38,
    V_CURRENT = 39,
    V_RGB = 40,
    V_RGBW = 41,
    V_ID = 42,
    V_UNIT_PREFIX = 43,
    V_HVAC_SETPOINT_COOL = 44,
    V_HVAC_SETPOINT_HEAT = 45,
    V_HVAC_FLOW_MODE = 46,
    V_TEXT = 47,
    V_CUSTOM = 48,
    V_POSITION = 49,
    V_IR_RECORD = 50,
    V_PH = 51,
    V_ORP = 52,
    V_EC = 53,
    V_VAR = 54,
    V_VA = 55,
    V_POWER_FACTOR = 56,
    V_MULTI_MESSAGE = 57
} mysensors_data_t;

typedef enum
{
    I_BATTERY_LEVEL = 0,
    I_TIME = 1,
    I_VERSION = 2,
    I_ID_REQUEST = 3,
    I_ID_RESPONSE = 4,
    I_INCLUSION_MODE = 5,
    I_CONFIG = 6,
    I_FIND_PARENT_REQUEST = 7,
    I_FIND_PARENT_RESPONSE = 8,
    I_LOG_MESSAGE = 9,
    I_CHILDREN = 10,
    I_SKETCH_NAME = 11,
    I_SKETCH_VERSION = 12,
    I_REBOOT = 13,
    I_GATEWAY_READY = 14,
    I_SIGNING_PRESENTATION = 15,
    I_NONCE_REQUEST = 16,
    I_NONCE_RESPONSE = 17,
    I_HEARTBEAT_REQUEST = 18,
    I_PRESENTATION = 19,
    I_DISCOVER_REQUEST = 20,
    I_DISCOVER_RESPONSE = 21,
    I_HEARTBEAT_RESPONSE = 22,
    I_LOCKED = 23,
    I_PING = 24,
    I_PONG = 25,
    I_REGISTRATION_REQUEST = 26,
    I_REGISTRATION_RESPONSE = 27,
    I_DEBUG = 28,
    I_SIGNAL_REPORT_REQUEST = 29,
    I_SIGNAL_REPORT_REVERSE = 30,
    I_SIGNAL_REPORT_RESPONSE = 31,
    I_PRE_SLEEP_NOTIFICATION = 32,
    I_POST_SLEEP_NOTIFICATION = 33
} mysensors_internal_t;

typedef enum
{
    ST_FIRMWARE_CONFIG_REQUEST = 0,
    ST_FIRMWARE_CONFIG_RESPONSE = 1,
    ST_FIRMWARE_REQUEST = 2,
    ST_FIRMWARE_RESPONSE = 3,
    ST_SOUND = 4,
    ST_IMAGE = 5,
    ST_FIRMWARE_CONFIRM = 6,
    ST_FIRMWARE_RESPONSE_RLE = 7
} mysensors_stream_t;

typedef enum
{
    P_STRING = 0,
    P_BYTE = 1,
    P_INT16 = 2,
    P_UINT16 = 3,
    P_LONG32 = 4,
    P_ULONG32 = 5,
    P_CUSTOM = 6,
    P_FLOAT32 = 7
} mysensors_payload_t;

#ifndef BIT
#define BIT(n) (1 << (n))
#endif
#define BIT_MASK(len) (BIT(len) - 1)
#define BF_MASK(start, len) (BIT_MASK(len) << (start))

#define BF_PREP(x, start, len) (((x) & BIT_MASK(len)) << (start))
#define BF_GET(y, start, len) (((y) >> (start)) & BIT_MASK(len))
#define BF_SET(y, x, start, len) (y = ((y) & ~BF_MASK(start, len)) | BF_PREP(x, start, len))

#define mSetVersion(_message, _version) _message.setVersion(_version)
#define mGetVersion(_message) _message.getVersion()

#define mSetSigned(_message, _signed) _message.setSigned(_signed)
#define mGetSigned(_message) _message.getSigned()

#define mSetLength(_message, _length) _message.setLength(_length)
#define mGetLength(_message) _message.getLength()

#define mSetCommand(_message, _command) _message.setCommand(_command)
#define mGetCommand(_message) _message.getCommand()

#define mSetRequestEcho(_message, _requestEcho) _message.setRequestEcho(_requestEcho)
#define mGetRequestEcho(_message) _message.getRequestEcho()

#define mSetEcho(_message, _echo) _message.setEcho(_echo)
#define mGetEcho(_message) _message.getEcho()

#define mSetPayloadType(_message, _payloadType) _message.setPayloadType(_payloadType)
#define mGetPayloadType(_message) _message.getPayloadType()

#if defined(__cplusplus)

#include <cstddef>
class MyMessage
{
  private:
    char *getCustomString(char *buffer) const;

  public:
    MyMessage(void);
    MyMessage(const uint8_t sensorId, const mysensors_data_t dataType);
    void clear(void);
    char *getStream(char *buffer) const;
    char *getString(char *buffer) const;
    const char *getString(void) const;
    void *getCustom(void) const;
    bool getBool(void) const;
    uint8_t getByte(void) const;
    float getFloat(void) const;
    int16_t getInt(void) const;
    uint16_t getUInt(void) const;
    int32_t getLong(void) const;
    uint32_t getULong(void) const;
    uint8_t getHeaderSize(void) const;
    uint8_t getMaxPayloadSize(void) const;
    uint8_t getExpectedMessageSize(void) const;
    bool isProtocolVersionValid(void) const;
    bool getRequestEcho(void) const;
    MyMessage &setRequestEcho(const bool requestEcho);
    uint8_t getVersion(void) const;
    MyMessage &setVersion(void);
    uint8_t getLength(void) const;
    MyMessage &setLength(const uint8_t length);
    mysensors_command_t getCommand(void) const;
    MyMessage &setCommand(const mysensors_command_t command);
    mysensors_payload_t getPayloadType(void) const;
    MyMessage &setPayloadType(const mysensors_payload_t payloadType);
    bool getSigned(void) const;
    MyMessage &setSigned(const bool signedFlag);
    bool isAck(void) const;
    bool isEcho(void) const;
    MyMessage &setEcho(const bool echo);
    uint8_t getType(void) const;
    MyMessage &setType(const uint8_t messageType);
    uint8_t getLast(void) const;
    MyMessage &setLast(const uint8_t lastId);
    uint8_t getSender(void) const;
    MyMessage &setSender(const uint8_t senderId);
    uint8_t getSensor(void) const;
    MyMessage &setSensor(const uint8_t sensorId);
    uint8_t getDestination(void) const;
    MyMessage &setDestination(const uint8_t destinationId);
    MyMessage &set(const void *payload, const size_t length);
    MyMessage &set(const char *value);
    MyMessage &set(const float value, const uint8_t decimals);
    MyMessage &set(const bool value);
    MyMessage &set(const uint8_t value);
    MyMessage &set(const uint32_t value);
    MyMessage &set(const int32_t value);
    MyMessage &set(const uint16_t value);
    MyMessage &set(const int16_t value);

#else

typedef union
{
    struct
    {

#endif
    uint8_t last;
    uint8_t sender;
    uint8_t destination;
    uint8_t version_length;
    uint8_t command_echo_payload;
    uint8_t type;
    uint8_t sensor;
    union
    {
        uint8_t bValue;
        uint16_t uiValue;
        int16_t iValue;
        uint32_t ulValue;
        int32_t lValue;
        struct
        {
            float fValue;
            uint8_t fPrecision;
        };
        char data[MAX_PAYLOAD_SIZE + 1];
    } __attribute__((packed));
#if defined(__cplusplus)
} __attribute__((packed));
#else
    };
    uint8_t array[HEADER_SIZE + MAX_PAYLOAD_SIZE + 1];
} __attribute__((packed)) MyMessage;
#endif

#endif // MYMESSAGEMOCK_H
