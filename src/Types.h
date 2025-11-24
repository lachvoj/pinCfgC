#ifndef TYPES_H
#define TYPES_H

#include <errno.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef UNIT_TEST
#include "ArduinoMock.h"
#else
#include <wiring_constants.h>
#include <wiring_digital.h>
#endif

// Fixed-point arithmetic scaling factor (6 decimal places)
// Sensor offset values are stored as: value × PINCFG_FIXED_POINT_SCALE
// Example: 0.0625 is stored as 62500 (0.0625 × 1000000)
#ifndef PINCFG_FIXED_POINT_SCALE
#define PINCFG_FIXED_POINT_SCALE 1000000LL
#endif

// Password section size
#ifndef PINCFG_AUTH_PASSWORD_MAX_LEN_D
#define PINCFG_AUTH_PASSWORD_MAX_LEN_D 32
#endif

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

#ifndef PINCFG_SWITCH_FB_ON_DELAY_MS_D
#define PINCFG_SWITCH_FB_ON_DELAY_MS_D 1000
#endif

#ifndef PINCFG_SWITCH_FB_OFF_DELAY_MS_D
#define PINCFG_SWITCH_FB_OFF_DELAY_MS_D 30000
#endif

#ifndef PINCFG_LONG_MESSAGE_DELAY_MS_D
#define PINCFG_LONG_MESSAGE_DELAY_MS_D 500
#endif

#ifndef PINCFG_LINE_SEPARATOR_D
#define PINCFG_LINE_SEPARATOR_D '/'
#endif

#ifndef PINCFG_VALUE_SEPARATOR_D
#define PINCFG_VALUE_SEPARATOR_D ','
#endif

// Sampling interval in milliseconds (needs ms precision)
#define PINCFG_CPUTEMP_SAMPLING_INTV_MIN_MS_D 100
#define PINCFG_CPUTEMP_SAMPLING_INTV_MAX_MS_D 5000
#ifndef PINCFG_CPUTEMP_SAMPLING_INTV_MS_D
#define PINCFG_CPUTEMP_SAMPLING_INTV_MS_D 1000 /* 1s */
#endif

// Reporting interval in SECONDS (more intuitive, allows uint16_t optimization)
#define PINCFG_CPUTEMP_REPORTING_INTV_MIN_SEC_D 1       /* 1 second */
#define PINCFG_CPUTEMP_REPORTING_INTV_MAX_SEC_D 3600    /* 1 hour */
#ifndef PINCFG_CPUTEMP_REPORTING_INTV_SEC_D
#define PINCFG_CPUTEMP_REPORTING_INTV_SEC_D 300         /* 5 minutes */
#endif

#ifndef PINCFG_CPUTEMP_OFFSET_D
#define PINCFG_CPUTEMP_OFFSET_D 0.0
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

#endif // TYPES_H
