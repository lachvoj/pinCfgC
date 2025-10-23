#include <cstring>

extern "C"
{
#include "MyMessageMock.h"
}

MyMessage::MyMessage(void)
{
    this->clear();
}

MyMessage::MyMessage(const uint8_t _sensorId, const mysensors_data_t _dataType)
{
    this->clear();
    (void)this->setSensor(_sensorId);
    (void)this->setType(static_cast<uint8_t>(_dataType));
}

uint32_t mock_MyMessage_clear_u32Called;
void MyMessage::clear(void)
{
    mock_MyMessage_clear_u32Called++;
    this->last = 0u;
    this->sender = 0u;
    this->destination = GATEWAY_ADDRESS; // Gateway is default destination
    this->version_length = 0u;
    this->command_echo_payload = 0u;
    this->type = 0u;
    this->sensor = 0u;
    // clear data buffer
    (void)memset((void *)this->data, 0u, sizeof(this->data));

    // set message protocol version
    (void)this->setVersion();
}

MyMessage &MyMessage::setVersion(void)
{
    BF_SET(
        this->version_length,
        V2_MYS_HEADER_PROTOCOL_VERSION,
        V2_MYS_HEADER_VSL_VERSION_POS,
        V2_MYS_HEADER_VSL_VERSION_SIZE);
    return *this;
}

uint8_t mock_MyMessage_setSensor_sensorId;
uint32_t mock_MyMessage_setSensor_u32Called;
MyMessage &MyMessage::setSensor(const uint8_t sensorId)
{
    mock_MyMessage_setSensor_sensorId = sensorId;
    mock_MyMessage_setSensor_u32Called++;
    return *this;
}

uint8_t mock_MyMessage_setType_messageType;
uint32_t mock_MyMessage_setType_u32Called;
MyMessage &MyMessage::setType(const uint8_t messageType)
{
    mock_MyMessage_setType_messageType = messageType;
    mock_MyMessage_setType_u32Called++;
    return *this;
}

uint8_t mock_MyMessage_set_uint8_t_value;
uint32_t mock_MyMessage_set_uint8_t_u32Called;
MyMessage &MyMessage::set(const uint8_t value)
{
    mock_MyMessage_set_uint8_t_value = value;
    mock_MyMessage_set_uint8_t_u32Called++;
    return *this;
}

char mock_MyMessage_set_char_value[SEND_MESSAGE_MAX_LENGTH_D];
uint32_t mock_MyMessage_set_pChar_u32Called;
MyMessage &MyMessage::set(const char *value)
{
    strcpy(mock_MyMessage_set_char_value, value);
    size_t szCharsToCopy = strlen(value);
    if (szCharsToCopy > MAX_PAYLOAD_SIZE)
        szCharsToCopy = MAX_PAYLOAD_SIZE;
    (void)strncpy(this->data, value, szCharsToCopy);
	// null terminate string
	this->data[szCharsToCopy] = 0;
    mock_MyMessage_set_pChar_u32Called++;
    return *this;
}

bool mock_MyMessage_set_bool_value;
uint32_t mock_MyMessage_set_bool_u32Called;
MyMessage &MyMessage::set(const bool value)
{
    mock_MyMessage_set_bool_value = value;
    mock_MyMessage_set_bool_u32Called++;
    return *this;
}

uint32_t mock_MyMessage_getCustom_u32Called;
void *MyMessage::getCustom(void) const
{
    mock_MyMessage_getCustom_u32Called++;
    return (void *)this->data;
}

uint32_t mock_MyMessage_getPayloadType_u32Called;
mysensors_payload_t MyMessage::getPayloadType(void) const
{
    mock_MyMessage_getPayloadType_u32Called++;
    return static_cast<mysensors_payload_t>(
        BF_GET(this->command_echo_payload, V2_MYS_HEADER_CEP_PAYLOADTYPE_POS, V2_MYS_HEADER_CEP_PAYLOADTYPE_SIZE));
}

uint32_t mock_MyMessage_getString_u32Called;
const char *MyMessage::getString(void) const
{
    mock_MyMessage_getString_u32Called++;
    if (this->getPayloadType() == P_STRING)
    {
        return this->data;
    }
    else
    {
        return NULL;
    }
}

extern "C"
{
    void init_MyMessageMock(void)
    {
        mock_MyMessage_setSensor_u32Called = 0;
        mock_MyMessage_clear_u32Called = 0;
        mock_MyMessage_setType_u32Called = 0;
        mock_MyMessage_set_uint8_t_u32Called = 0;
        mock_MyMessage_set_uint8_t_value = 0;
        mock_MyMessage_set_pChar_u32Called = 0;
        mock_MyMessage_set_bool_u32Called = 0;
        mock_MyMessage_set_bool_value = false;
        mock_MyMessage_getCustom_u32Called = 0;
        mock_MyMessage_getPayloadType_u32Called = 0;
        mock_MyMessage_getString_u32Called = 0;
        memset(mock_MyMessage_set_char_value, 0, sizeof(mock_MyMessage_set_char_value));
    }
}
