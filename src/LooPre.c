#include "ExtCfgReceiver.h"
#include "InPin.h"
#include "LooPre.h"
#include "Switch.h"

uint8_t LooPre_u8GetId(LOOPRE_T *psHandle)
{
    return psHandle->u8Id;
}

const char *LooPre_pcGetName(LOOPRE_T *psHandle)
{
    return psHandle->pcName;
}
