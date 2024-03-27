#pragma once

#include <stdint.h>

struct EEPROMClass
{
    uint8_t read(int idx);
    void write(int idx, uint8_t val);
    void update(int idx, uint8_t val);
};

static EEPROMClass EEPROM;