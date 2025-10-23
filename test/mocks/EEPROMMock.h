#ifndef EEPROMMOCK_H
#define EEPROMMOCK_H

#include <stdint.h>

// EEPROM addresses
#define EEPROM_NODE_ID_ADDRESS 0x00
#define EEPROM_LOCAL_CONFIG_ADDRESS 413  // MySensors reserves 0-412

// STM32F103 EEPROM size (1024 bytes)
#define E2END 0x3FF  // Last address (0x3FF), size is 0x400 = 1024 bytes

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
