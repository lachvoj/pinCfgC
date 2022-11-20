#ifndef ARDUINOMOCK_H
#define ARDUINOMOCK_H

#include "Arduino.h"

extern uint8_t mock_pinMode_u8Pin;
extern uint8_t mock_pinMode_u8Mode;
extern uint32_t mock_pinMode_u32Called;

extern uint8_t mock_digitalRead_u8Pin;
extern uint8_t mock_digitalRead_u8Return;
extern uint32_t mock_digitalRead_u32Called;
void mock_digitalRead_SetReturnSequence(uint8_t *pu8RetSeq, size_t szRetSeqSize);

extern uint8_t mock_digitalWrite_u8Pin;
extern uint8_t mock_digitalWrite_u8Value;
extern uint32_t mock_digitalWrite_u32Called;

extern uint32_t mock_wait_u32ms;
extern uint32_t mock_pwait_u32Called;

void vArduinoMock_setup(void);

#endif // ARDUINOMOCK_H
