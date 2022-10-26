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

#ifndef PINCFG_CONF_START_STR
#define PINCFG_CONF_START_STR "#{#"
#endif

#ifndef PINCFG_CONF_END_STR
#define PINCFG_CONF_END_STR "#}#"
#endif

typedef enum
{
    PINCFG_EXTCFGRECEIVER_E,
    PINCFG_INPIN_E,
    PINCFG_SWITCH_E,
    PINCFG_TRIGGER_E
} PINCFG_ELEMENT_TYPE_T;

#endif // TYPES_H
