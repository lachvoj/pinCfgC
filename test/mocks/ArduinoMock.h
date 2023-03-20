#ifndef ARDUINOMOCK_H
#define ARDUINOMOCK_H

#include <stddef.h>

#include "Types.h"

#define OUTPUT 1
#define INPUT 0
#define HIGH 1
#define LOW 0
#define INPUT_PULLUP 0x2

void _delay_milliseconds(unsigned int millis);

#ifndef delay
#define delay _delay_milliseconds
#endif

void pinMode(uint8_t u8Pin, uint8_t u8Mode);

uint8_t digitalRead(uint8_t u8Pin);

void digitalWrite(uint8_t u8Pin, uint8_t u8Value);

void wait(uint32_t u32ms);

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
