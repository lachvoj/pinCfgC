
#include "EEPROM.h"

extern "C"
{
#include "EEPROMMock.h"
}

// External mock EEPROM storage (defined in test.c)
extern uint8_t mock_EEPROM[1024];

int mock_EEPROM_read_idx;
uint32_t mock_EEPROM_read_u32Called;
uint8_t mock_EEPROM_read_return;
uint8_t EEPROMClass::read(int idx)
{
    mock_EEPROM_read_idx = idx;
    mock_EEPROM_read_u32Called++;

    // Return from mock EEPROM array if in range
    if (idx >= 0 && idx < 1024)
    {
        mock_EEPROM_read_return = mock_EEPROM[idx];
    }

    return mock_EEPROM_read_return;
}

int mock_EEPROM_write_idx;
int mock_EEPROM_write_val;
uint32_t mock_EEPROM_write_u32Called;
void EEPROMClass::write(int idx, uint8_t val)
{

    mock_EEPROM_write_idx = idx;
    mock_EEPROM_write_val = val;
    mock_EEPROM_write_u32Called++;

    // Write to mock EEPROM array if in range
    if (idx >= 0 && idx < 1024)
    {
        mock_EEPROM[idx] = val;
    }
}

int mock_EEPROM_update_idx;
int mock_EEPROM_update_val;
uint32_t mock_EEPROM_update_u32Called;
void EEPROMClass::update(int idx, uint8_t val)
{
    mock_EEPROM_update_idx = idx;
    mock_EEPROM_update_val = val;
    mock_EEPROM_update_u32Called++;

    // Update mock EEPROM array if in range (same as write for mock)
    if (idx >= 0 && idx < 1024)
    {
        mock_EEPROM[idx] = val;
    }
}

extern "C"
{
    void init_EEPROMMock()
    {
        mock_EEPROM_read_u32Called = 0;
        mock_EEPROM_read_return = 0;
        mock_EEPROM_write_u32Called = 0;
        mock_EEPROM_update_u32Called = 0;
    }
}
