#ifndef MYMESSAGEMOCK_H
#define MYMESSAGEMOCK_H

#include <stdint.h>
#include <stdbool.h>

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

extern uint32_t mock_MyMessage_getCustom_u32Called;

extern uint32_t mock_MyMessage_getPayloadType_u32Called;

extern uint32_t mock_MyMessage_getString_u32Called;

void init_MyMessageMock(void);

#endif // MYMESSAGEMOCK_H
