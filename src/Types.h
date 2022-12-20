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

#ifndef PINCFG_LINE_SEPARATOR_D
#define PINCFG_LINE_SEPARATOR_D '/'
#endif

#ifndef PINCFG_VALUE_SEPARATOR_D
#define PINCFG_VALUE_SEPARATOR_D ','
#endif

#endif // TYPES_H
