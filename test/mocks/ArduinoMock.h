#ifndef ARDUINOMOCK_H
#define ARDUINOMOCK_H

#include <stddef.h>
#include <stdint.h>

#include "EEPROMMock.h"
#include "GPIOMock.h"
#include "MyMessageMock.h"
#include "MySensorsMock.h"

#define OUTPUT 1
#define INPUT 0
#define HIGH 1
#define LOW 0
#define INPUT_PULLUP 0x2

void _delay_milliseconds(unsigned int millis);

#ifndef delay
#define delay _delay_milliseconds
#endif

extern uint32_t mock_wait_u32ms;
extern uint32_t mock_wait_u32Called;
void wait(uint32_t u32ms);

void init_ArduinoMock(void);

#endif // ARDUINOMOCK_H
