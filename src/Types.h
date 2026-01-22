#ifndef TYPES_H
#define TYPES_H

#include <errno.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#if defined(UNIT_TEST)
#include "ArduinoMock.h"
#elif defined(ARDUINO_ARCH_AVR)
#include <Arduino.h>
#else
#include <wiring_constants.h>
#include <wiring_digital.h>
#endif

#define container_of(ptr, type, member) ((type *)((uint8_t *)(ptr) - offsetof(type, member)))

// Fixed-point arithmetic scaling factor (6 decimal places)
// Sensor offset values are stored as: value × PINCFG_FIXED_POINT_SCALE
// Example: 0.0625 is stored as 62500 (0.0625 × 1000000)
#ifndef PINCFG_FIXED_POINT_SCALE
#define PINCFG_FIXED_POINT_SCALE 1000000LL
#endif

// Password section size (binary SHA-256 hash)
#ifdef PINCFG_AUTH_PASSWORD_LEN_D
#error "PINCFG_AUTH_PASSWORD_LEN_D must not be redefined! It is fixed at 32 bytes (SHA-256 hash)."
#endif
#define PINCFG_AUTH_PASSWORD_LEN_D 32 // SHA-256 binary hash (32 bytes)

// Hex representation of password for CLI (64 hex chars for 32 bytes)
#ifdef PINCFG_AUTH_HEX_PASSWORD_LEN_D
#error "PINCFG_AUTH_HEX_PASSWORD_LEN_D must not be redefined! It is fixed at 64 chars (hex representation of 32 bytes)."
#endif
#define PINCFG_AUTH_HEX_PASSWORD_LEN_D 64

// Config data section size
#ifndef PINCFG_CONFIG_MAX_SZ_D
#define PINCFG_CONFIG_MAX_SZ_D 480
#endif

#ifndef PINCFG_TXTSTATE_MAX_SZ_D
#define PINCFG_TXTSTATE_MAX_SZ_D 25
#endif

#ifndef PINCFG_TRIGGER_MAX_SWITCHES_D
#define PINCFG_TRIGGER_MAX_SWITCHES_D 5
#endif
#if (PINCFG_TRIGGER_MAX_SWITCHES_D > 255)
#error PINCFG_TRIGGER_MAX_SWITCHES_D is more then 255!
#endif

#ifndef PINCFG_DEBOUNCE_MS_D
#define PINCFG_DEBOUNCE_MS_D 100
#endif

#ifndef PINCFG_MULTICLICK_MAX_DELAY_MS_D
#define PINCFG_MULTICLICK_MAX_DELAY_MS_D 500
#endif

#ifndef PINCFG_SWITCH_IMPULSE_DURATIN_MS_D
#define PINCFG_SWITCH_IMPULSE_DURATIN_MS_D 300
#endif

#ifndef PINCFG_SWITCH_FB_DELAY_MS_D
#define PINCFG_SWITCH_FB_DELAY_MS_D 1000
#endif

#ifndef PINCFG_LONG_MESSAGE_DELAY_MS_D
#define PINCFG_LONG_MESSAGE_DELAY_MS_D 50
#endif

#ifndef PINCFG_LINE_SEPARATOR_D
#define PINCFG_LINE_SEPARATOR_D '/'
#endif

#ifndef PINCFG_VALUE_SEPARATOR_D
#define PINCFG_VALUE_SEPARATOR_D ','
#endif

// Sampling interval in milliseconds (needs ms precision)
#define PINCFG_SENSOR_SAMPLING_INTV_MIN_MS_D 100
#define PINCFG_SENSOR_SAMPLING_INTV_MAX_MS_D 5000
#ifndef PINCFG_SENSOR_SAMPLING_INTV_MS_D
#define PINCFG_SENSOR_SAMPLING_INTV_MS_D 1000 /* 1s */
#endif

// Reporting interval in SECONDS (more intuitive, allows uint16_t optimization)
#define PINCFG_SENSOR_REPORTING_INTV_MIN_SEC_D 1    /* 1 second */
#define PINCFG_SENSOR_REPORTING_INTV_MAX_SEC_D 3600 /* 1 hour */
#ifndef PINCFG_SENSOR_REPORTING_INTV_SEC_D
#define PINCFG_SENSOR_REPORTING_INTV_SEC_D 300 /* 5 minutes */
#endif

// Sensor scale boundaries (multiplicative, stored as fixed-point: value × PINCFG_FIXED_POINT_SCALE)
// NOTE: With int32_t storage, safe range is ±2147, but ±1000 is practical for most sensors
#define PINCFG_SENSOR_SCALE_MIN_D -1000.0
#define PINCFG_SENSOR_SCALE_MAX_D 1000.0
#ifndef PINCFG_SENSOR_SCALE_D
#define PINCFG_SENSOR_SCALE_D 1.0 // Default scale factor (identity)
#endif

// Sensor offset boundaries (additive, stored as fixed-point: value × PINCFG_FIXED_POINT_SCALE)
#define PINCFG_SENSOR_OFFSET_MIN_D -1000.0
#define PINCFG_SENSOR_OFFSET_MAX_D 1000.0
#ifndef PINCFG_SENSOR_OFFSET_D
#define PINCFG_SENSOR_OFFSET_D 0.0 // Default offset (no adjustment)
#endif

// Sensor precision (decimal places for display/payload sizing)
#define PINCFG_SENSOR_PRECISION_MIN_D 0
#define PINCFG_SENSOR_PRECISION_MAX_D 6
#ifndef PINCFG_SENSOR_PRECISION_D
#define PINCFG_SENSOR_PRECISION_D 0 // No decimals by default
#endif

#ifndef PINCFG_CLI_MAX_LINE_SZ_D
#define PINCFG_CLI_MAX_LINE_SZ_D 30
#endif

#ifndef PINCFG_TIMED_SWITCH_MIN_PERIOD_MS_D
#define PINCFG_TIMED_SWITCH_MIN_PERIOD_MS_D 50
#endif

#ifndef PINCFG_TIMED_SWITCH_MAX_PERIOD_MS_D
#define PINCFG_TIMED_SWITCH_MAX_PERIOD_MS_D 600000 /* 10 min */
#endif

#ifndef PINCFG_FW_CRC_CHECK_INTERVAL_MS_D
#define PINCFG_FW_CRC_CHECK_INTERVAL_MS_D 86400000 /* 24 hours */
#endif

#ifndef PINCFG_CLI_RECEIVE_TIMEOUT_MS_D
#define PINCFG_CLI_RECEIVE_TIMEOUT_MS_D 30000 /* 30 seconds */
#endif

#endif // TYPES_H
