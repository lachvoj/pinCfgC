#ifndef TYPES_H
#define TYPES_H

#include <errno.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

typedef enum
{
    PINCFG_EXTCFGRECEIVER_E,
    PINCFG_INPIN_E,
    PINCFG_SWITCH_E,
    PINCFG_TRIGGER_E
} PINCFG_ELEMENT_TYPE_T;

#endif // TYPES_H
