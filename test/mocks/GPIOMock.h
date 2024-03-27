#ifndef GPIOMOCK_H
#define GPIOMOCK_H

#include <stddef.h>
#include <stdint.h>

extern uint8_t mock_pinMode_u8Pin;
extern uint8_t mock_pinMode_u8Mode;
extern uint32_t mock_pinMode_u32Called;
void pinMode(uint8_t u8Pin, uint8_t u8Mode);

extern uint8_t mock_digitalRead_u8Pin;
extern uint8_t mock_digitalRead_u8Return;
extern uint32_t mock_digitalRead_u32Called;
void mock_digitalRead_SetReturnSequence(uint8_t *pu8RetSeq, size_t szRetSeqSize);
uint8_t digitalRead(uint8_t u8Pin);

extern uint8_t mock_digitalWrite_u8Pin;
extern uint8_t mock_digitalWrite_u8Value;
extern uint32_t mock_digitalWrite_u32Called;
void digitalWrite(uint8_t u8Pin, uint8_t u8Value);

void init_GPIOMock(void);

#endif /* GPIOMOCK_H */
