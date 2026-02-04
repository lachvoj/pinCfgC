#include "ArduinoMock.h"

#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "GPIOMock.h"
#include "MySensorsMock.h"

#ifndef __linux__
void nanosleep(struct timespec *time, void *something)
{
    (void)time;
    (void)something;
}
#endif // __linux__

uint32_t mock_millis_u32Called;
uint32_t mock_millis_u32Return;
uint32_t millis()
{
    mock_millis_u32Called++;
    return mock_millis_u32Return;
}

uint32_t mock_micros_u32Called;
uint32_t mock_micros_u32Return;
uint32_t micros()
{
    mock_micros_u32Called++;
    return mock_micros_u32Return;
}

// Analog mock functions
uint16_t mock_analogRead_u16Return = 512; // Default mid-range (10-bit)
uint32_t mock_analogRead_u32Called = 0;
uint8_t mock_analogRead_u8LastPin = 0;

uint16_t analogRead(uint8_t pin)
{
    mock_analogRead_u32Called++;
    mock_analogRead_u8LastPin = pin;
    return mock_analogRead_u16Return;
}

void _delay_milliseconds(unsigned int millis)
{
    struct timespec sleeper;

    sleeper.tv_sec = (time_t)(millis / 1000);
    sleeper.tv_nsec = (long)(millis % 1000) * 1000000;
    nanosleep(&sleeper, NULL);
}

void init_ArduinoMock(void)
{
    init_EEPROMMock();
    init_GPIOMock();
    init_MySensorsMock();
}