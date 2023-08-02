#ifndef TYPES_H
#define TYPES_H

#include <errno.h>
#include <stdbool.h>
#include <stdint.h>

#ifndef PINCFG_CONFIG_MAX_SZ_D
#define PINCFG_CONFIG_MAX_SZ_D 512
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

#ifndef PINCFG_TIMED_ACTION_ADDITION_MS_D
#define PINCFG_TIMED_ACTION_ADDITION_MS_D 1966080 // 5 min
#endif

#ifndef PINCFG_LINE_SEPARATOR_D
#define PINCFG_LINE_SEPARATOR_D '/'
#endif

#ifndef PINCFG_VALUE_SEPARATOR_D
#define PINCFG_VALUE_SEPARATOR_D ','
#endif

#ifndef GATEWAY_ADDRESS
#define GATEWAY_ADDRESS 0
#endif

#ifndef S_DOOR
#define S_DOOR 0
#endif

#ifndef S_BINARY
#define S_BINARY 3
#endif

#ifndef S_INFO
#define S_INFO 36
#endif

#ifndef V_STATUS
#define V_STATUS 2
#endif

#ifndef V_TRIPPED
#define V_TRIPPED 16
#endif

#ifndef V_TEXT
#define V_TEXT 47
#endif

#endif // TYPES_H
