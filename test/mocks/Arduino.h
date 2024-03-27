#ifndef Arduino_h
#define Arduino_h

#ifndef UNIT_TEST
#include <PeripheralPins.h>
#include <pinmap.h>
#include <pins_arduino.h>
#include <wiring_constants.h>
#include <wiring_digital.h>
#endif

#ifndef __cplusplus
#include "ArduinoMock.h"
#else
#include <cstddef>
#endif

#endif /* Arduino_h */