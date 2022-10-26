#ifndef ARDUINO_H
#define ARDUINO_H

#define OUTPUT 1
#define INPUT 0
#define HIGH 1
#define LOW 0
#define INPUT_PULLUP 0x2

#include "Types.h"

void pinMode(uint8_t u8Pin, uint8_t u8Mode);

uint8_t digitalRead(uint8_t u8Pin);

void digitalWrite(uint8_t u8Pin, uint8_t u8Value);

void wait(uint32_t u32ms);

#endif // ARDUINO_H
