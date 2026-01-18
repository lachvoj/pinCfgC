#ifndef TEST_HELPERS_H
#define TEST_HELPERS_H

#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <unity.h>

// Include system headers
#include "ArduinoMock.h"
#include "CPUTempMeasure.h"
#include "Cli.h"
#include "CliAuth.h"
#include "Globals.h"
#include "InPin.h"
#include "LinkedList.h"
#ifdef PINCFG_FEATURE_LOOPTIME_MEASUREMENT
#include "LoopTimeMeasure.h"
#endif
#include "Memory.h"
#include "MySensorsMock.h"
#include "PersistentConfiguration.h"
#include "PinCfgCsv.h"
#include "PinCfgStr.h"
#include "PinCfgUtils.h"
#include "Presentable.h"
#include "Sensor.h"
#include "Switch.h"
#include "Trigger.h"

#ifdef PINCFG_FEATURE_I2C_MEASUREMENT
#include "I2CMeasure.h"
#include "I2CMock.h"
#endif

#ifdef PINCFG_FEATURE_SPI_MEASUREMENT
#include "SPIMeasure.h"
#include "SPIMock.h"
#endif

#ifdef PINCFG_FEATURE_ANALOG_MEASUREMENT
#include "AnalogMeasure.h"
#endif

// Memory size definitions
#ifndef MEMORY_SZ
#ifdef USE_MALLOC
#define MEMORY_SZ 3003
#else
// Need more memory for static allocation mode (more complex tests)
#define MEMORY_SZ 5000
#endif
#endif

#ifndef AS_OUT_MAX_LEN_D
#define AS_OUT_MAX_LEN_D 10
#endif

#ifndef LOOPRE_ARRAYS_MAX_LEN_D
#define LOOPRE_ARRAYS_MAX_LEN_D 30
#endif

#ifndef OUT_STR_MAX_LEN_D
#define OUT_STR_MAX_LEN_D 250
#endif

// Global test memory buffer
extern uint8_t testMemory[MEMORY_SZ];

// Mock EEPROM storage (extern, defined in test_helpers.c)
extern uint8_t mock_EEPROM[1024];

// Unity setup/teardown functions
void setUp(void);
void tearDown(void);

// Helper functions for EEPROM initialization
void init_mock_EEPROM(void);
void init_mock_EEPROM_with_default_password(void);
PERCFG_RESULT_T write_password_from_hex(const char *hex_password);

// Helper functions for EEPROM corruption (testing recovery)
void corrupt_EEPROM_byte(uint16_t address, uint8_t bit_position);
void corrupt_EEPROM_range(uint16_t start, uint16_t end);

#endif // TEST_HELPERS_H
