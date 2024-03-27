#ifndef EEPROMMOCK_H
#define EEPROMMOCK_H

#include <stdint.h>

extern int mock_EEPROM_read_idx;
extern uint32_t mock_EEPROM_read_u32Called;
extern uint8_t mock_EEPROM_read_return;

extern int mock_EEPROM_write_idx;
extern int mock_EEPROM_write_val;
extern uint32_t mock_EEPROM_write_u32Called;

extern int mock_EEPROM_update_idx;
extern int mock_EEPROM_update_val;
extern uint32_t mock_EEPROM_update_u32Called;

void init_EEPROMMock(void);

#endif // EEPROMMOCK_H
