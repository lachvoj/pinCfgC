#ifndef ARDUINOMOCK_H
#define ARDUINOMOCK_H

#ifdef __cplusplus
extern "C"
{
#endif

#include <stddef.h>
#include <stdint.h>

#include "EEPROMMock.h"

#ifndef OUTPUT
#include "GPIOMock.h"
#endif

// HAL
#define PROGMEM

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

    extern uint32_t mock_millis_u32Called;
    extern uint32_t mock_millis_u32Return;
    uint32_t millis();

    // Analog functions
    extern uint16_t mock_analogRead_u16Return;
    extern uint32_t mock_analogRead_u32Called;
    extern uint8_t mock_analogRead_u8LastPin;
    uint16_t analogRead(uint8_t pin);

    void init_ArduinoMock(void);

#ifdef __cplusplus
}
#endif

#endif // ARDUINOMOCK_H
