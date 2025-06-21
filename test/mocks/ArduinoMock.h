#ifndef ARDUINOMOCK_H
#define ARDUINOMOCK_H

#include <stddef.h>
#include <stdint.h>

#include "EEPROMMock.h"

#ifndef OUTPUT
#include "GPIOMock.h"
#endif


#ifndef OUTPUT
#define OUTPUT 1
#endif

#ifndef INPUT
#define INPUT 0
#endif

#ifndef HIGH
#define HIGH 1
#endif

#ifndef LOW
#define LOW 0
#endif

#ifndef INPUT_PULLUP
#define INPUT_PULLUP 0x2
#endif

void _delay_milliseconds(unsigned int millis);

#ifndef delay
#define delay _delay_milliseconds
#endif

extern uint32_t mock_wait_u32ms;
extern uint32_t mock_wait_u32Called;
void wait(uint32_t u32ms);

void init_ArduinoMock(void);

#endif // ARDUINOMOCK_H
