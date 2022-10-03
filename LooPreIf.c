#include "ExtCfgReceiver.h"
#include "InPin.h"
#include "LooPreIf.h"
#include "Switch.h"

// loopable
void loop(struct LOOPRE_IF_T *psHandle, uint32_t u32ms)
{
    if (psHandle->ePinCfgType == PINCFG_INPIN_E)
        InPin_vLoop((INPIN_HANDLE_T *)psHandle, u32ms);
    else if (psHandle->ePinCfgType == PINCFG_SWITCH_E)
        Switch_vLoop((SWITCH_HANDLE_T *)psHandle, u32ms);
}

// presentable
uint8_t getId(struct LOOPRE_IF_T *psHandle)
{
    if (psHandle->ePinCfgType == PINCFG_INPIN_E || psHandle->ePinCfgType == PINCFG_SWITCH_E)
        return MySensorsPresent_u8GetId((MYSENSORSPRESENT_HANDLE_T *)psHandle);
    else if (psHandle->ePinCfgType == PINCFG_EXTCFGRECEIVER_E)
        return ExtCfgReceiver_u8GetId((EXTCFGRECEIVER_HANDLE_T *)psHandle);

    return 0x00U;
}

const char *getName(struct LOOPRE_IF_T *psHandle)
{
    if (psHandle->ePinCfgType == PINCFG_INPIN_E || psHandle->ePinCfgType == PINCFG_SWITCH_E)
        return MySensorsPresent_pcGetName((MYSENSORSPRESENT_HANDLE_T *)psHandle);
    else if (psHandle->ePinCfgType == PINCFG_EXTCFGRECEIVER_E)
        return ExtCfgReceiver_pcGetName((EXTCFGRECEIVER_HANDLE_T *)psHandle);

    return NULL;
}

void rcvMessage(struct LOOPRE_IF_T *psHandle, const void *pvMessage)
{
    if (psHandle->ePinCfgType == PINCFG_INPIN_E || psHandle->ePinCfgType == PINCFG_SWITCH_E)
        MySensorsPresent_vRcvMessage((MYSENSORSPRESENT_HANDLE_T *)psHandle, pvMessage);
    else if (psHandle->ePinCfgType == PINCFG_EXTCFGRECEIVER_E)
        ExtCfgReceiver_vRcvMessage((EXTCFGRECEIVER_HANDLE_T *)psHandle, pvMessage);
}

void present(struct LOOPRE_IF_T *psHandle)
{
    if (psHandle->ePinCfgType == PINCFG_INPIN_E || psHandle->ePinCfgType == PINCFG_SWITCH_E)
        MySensorsPresent_vPresent((MYSENSORSPRESENT_HANDLE_T *)psHandle);
    else if (psHandle->ePinCfgType == PINCFG_EXTCFGRECEIVER_E)
        ExtCfgReceiver_vPresent((EXTCFGRECEIVER_HANDLE_T *)psHandle);
}

void presentState(struct LOOPRE_IF_T *psHandle)
{
    if (psHandle->ePinCfgType == PINCFG_INPIN_E || psHandle->ePinCfgType == PINCFG_SWITCH_E)
        MySensorsPresent_vPresentState((MYSENSORSPRESENT_HANDLE_T *)psHandle);
    else if (psHandle->ePinCfgType == PINCFG_EXTCFGRECEIVER_E)
        ExtCfgReceiver_vPresentState((EXTCFGRECEIVER_HANDLE_T *)psHandle);
}
