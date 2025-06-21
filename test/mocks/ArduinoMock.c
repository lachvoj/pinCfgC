#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "ArduinoMock.h"
#include "GPIOMock.h"
#include "MySensorsMock.h"

#ifndef __linux__
void nanosleep(struct timespec *time, void *something)
{
    (void)time;
    (void)something;
}
#endif // __linux__

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