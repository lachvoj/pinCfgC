#ifdef ARDUINO
#include <Arduino.h>
#else
#include <ArduinoMock.h>
#endif
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include "ExtCfgReceiver.h"
#include "Globals.h"
#include "InPin.h"
#include "Memory.h"
#include "PinCfgCsv.h"
#include "PinCfgStr.h"
#include "Switch.h"
#include "Trigger.h"

typedef struct
{
    size_t *pszMemoryRequired;
    size_t pcOutStringLast;
    char *pcOutString;
    const uint16_t u16OutStrMaxLen;
    const bool bValidate;
    uint16_t u16LinesProcessed;
    uint8_t u8LineItemsLen;
    uint8_t u8PresentablesCount;
    size_t szNumberOfWarnings;
    STRING_POINT_T sLine;
    STRING_POINT_T sTempStrPt;
} PINCFG_PARSE_SUBFN_PARAMS_T;

static const char *_s[] = {
    "%s%s\n",                             // FSS_E
    "%s%s%s\n",                           // FSSS_E
    "%s%d:%s\n",                          // FSDS_E
    "%s%d:%s%s\n",                        // FSDSS_E
    "%s%d:%s%s%s\n",                      // FSDSSS_E
    "E:",                                 // E_E
    "W:",                                 // W_E
    "E:L:",                               // EL_E
    "W:L:",                               // WL_E
    "I:",                                 // I_E
    "ExtCfgReceiver:",                    // ECR_E
    "Switch:",                            // SW_E
    "InPin:",                             // IP_E
    "Trigger:",                           // TRG_E
    "InPinDebounceMs:",                   // IPDMS_E
    "InPinMulticlickMaxDelayMs:",         // IPMCDMS_E
    "SwitchImpulseDurationMs:",           // SWIDMS_E
    "SwitchFbOnDelayMs:",                 // SWFNDMS_E
    "SwitchFbOffDelayMs:",                // SWFFDMS_E
    "SwitchTimedActionAdditionMs:",       // SWTAAMS_E
    "Out of memory.",                     // OOM_E
    "Init failed!",                       // INITF_E
    "Invalid pin number.",                // IPN_E
    "Invalid number",                     // IN_E
    " of arguments.",                     // OARGS_E
    " of items defining names and pins.", // OITMS_E
    "Switch name not found."              // SNNF_E
};

typedef enum
{
    FSS_E = 0,
    FSSS_E,
    FSDS_E,
    FSDSS_E,
    FSDSSS_E,
    E_E,
    W_E,
    EL_E,
    WL_E,
    I_E,
    ECR_E,
    SW_E,
    IP_E,
    TRG_E,
    IPDMS_E,
    IPMCDMS_E,
    SWIDMS_E,
    SWFNDMS_E,
    SWFFDMS_E,
    SWTAAMS_E,
    OOM_E,
    INITF_E,
    IPN_E,
    IN_E,
    OARGS_E,
    OITMS_E,
    SNNF_E
} PINCFG_PARSE_STRINGS_T;

static inline PINCFG_RESULT_T PinCfgCsv_CreateExternalCfgReceiver(
    PINCFG_PARSE_SUBFN_PARAMS_T *psPrms,
    bool bRemoteConfigEnabled);
static inline PINCFG_RESULT_T PinCfgCsv_ParseSwitch(PINCFG_PARSE_SUBFN_PARAMS_T *psPrms);
static inline PINCFG_RESULT_T PinCfgCsv_ParseInpins(PINCFG_PARSE_SUBFN_PARAMS_T *psPrms);
static inline PINCFG_RESULT_T PinCfgCsv_ParseTriggers(PINCFG_PARSE_SUBFN_PARAMS_T *psPrms);

static inline PINCFG_RESULT_T PinCfgCsv_ParseGlobalConfigItems(PINCFG_PARSE_SUBFN_PARAMS_T *psPrms);

static inline LOOPRE_T *PinCfgCsv_psFindInPresentablesById(uint8_t u8Id);
static inline void PinCfgCsv_vAddToLoopables(LOOPRE_T *psLoopable);
static LOOPRE_T *PinCfgCsv_psFindInLoopablesByName(const STRING_POINT_T *psName);
static inline void PinCfgCsv_vAddToPresentables(LOOPRE_T *psLoopable);
static inline size_t szGetAllocatedSize(size_t szToAllocate);
static inline size_t szGetSize(size_t a, size_t b);

PINCFG_RESULT_T PinCfgCsv_eInit(uint8_t *pu8Memory, size_t szMemorySize, PINCFG_IF_T *psPincfgIf)
{
    // Memory init
    if (pu8Memory == NULL)
    {
        return PINCFG_NULLPTR_ERROR_E;
    }

    if (Memory_eInit(pu8Memory, szMemorySize) != MEMORY_OK_E)
    {
        return PINCFG_MEMORYINIT_ERROR_E;
    }

    memset((void *)psGlobals->acCfgBuf, 0, PINCFG_CONFIG_MAX_SZ_D);

    // pincfg if init
    if (psPincfgIf == NULL || psPincfgIf->bRequest == NULL || psPincfgIf->bPresent == NULL ||
        psPincfgIf->bSend == NULL || psPincfgIf->u8SaveCfg == NULL)
    {
        return PINCFG_IF_NULLPTR_ERROR_E;
    }
    psGlobals->sPincfgIf = *psPincfgIf;

    // V tabs init
    // extcfgreceiver
    psGlobals->sExtCfgReceiverVTab.u8VType = V_TEXT;
    psGlobals->sExtCfgReceiverVTab.u8SType = S_INFO;
    psGlobals->sExtCfgReceiverVTab.vLoop = NULL;
    psGlobals->sExtCfgReceiverVTab.vReceive = ExtCfgReceiver_vRcvMessage;
    psGlobals->sExtCfgReceiverVTab.vPresent = MySensorsPresent_vPresent;
    psGlobals->sExtCfgReceiverVTab.vPresentState = MySensorsPresent_vPresentState;
    psGlobals->sExtCfgReceiverVTab.vSendState = MySensorsPresent_vSendState;
    // switch
    psGlobals->sSwitchVTab.u8VType = V_STATUS;
    psGlobals->sSwitchVTab.u8SType = S_BINARY;
    psGlobals->sSwitchVTab.vLoop = Switch_vLoop;
    psGlobals->sSwitchVTab.vReceive = MySensorsPresent_vRcvMessage;
    psGlobals->sSwitchVTab.vPresent = MySensorsPresent_vPresent;
    psGlobals->sSwitchVTab.vPresentState = MySensorsPresent_vPresentState;
    psGlobals->sSwitchVTab.vSendState = MySensorsPresent_vSendState;
    // inpin
    psGlobals->sInPinVTab.u8VType = V_TRIPPED;
    psGlobals->sInPinVTab.u8SType = S_DOOR;
    psGlobals->sInPinVTab.vLoop = InPin_vLoop;
    psGlobals->sInPinVTab.vReceive = InPin_vRcvMessage;
    psGlobals->sInPinVTab.vPresent = MySensorsPresent_vPresent;
    psGlobals->sInPinVTab.vPresentState = MySensorsPresent_vPresentState;
    psGlobals->sInPinVTab.vSendState = MySensorsPresent_vSendState;
    // trigger
    psGlobals->sTriggerVTab.vEventHandle = Trigger_vEventHandle;

    InPin_SetDebounceMs(PINCFG_DEBOUNCE_MS_D);
    InPin_SetMulticlickMaxDelayMs(PINCFG_MULTICLICK_MAX_DELAY_MS_D);
    Switch_SetImpulseDurationMs(PINCFG_SWITCH_IMPULSE_DURATIN_MS_D);
    Switch_SetFbOnDelayMs(PINCFG_SWITCH_FB_ON_DELAY_MS_D);
    Switch_SetFbOffDelayMs(PINCFG_SWITCH_FB_OFF_DELAY_MS_D);
    Switch_vSetTimedActionAdditionMs(PINCFG_TIMED_ACTION_ADDITION_MS_D);

    return PINCFG_OK_E;
}

char *PinCfgCsv_pcGetCfgBuf(void)
{
    if (psGlobals == NULL)
        return NULL;

    return psGlobals->acCfgBuf;
}

PINCFG_RESULT_T PinCfgCsv_eParse(
    size_t *pszMemoryRequired,
    char *pcOutString,
    const uint16_t u16OutStrMaxLen,
    const bool bValidate,
    const bool bRemoteConfigEnabled)
{
    PINCFG_PARSE_SUBFN_PARAMS_T sPrms = {
        .pszMemoryRequired = pszMemoryRequired,
        .pcOutStringLast = 0,
        .pcOutString = pcOutString,
        .u16OutStrMaxLen = u16OutStrMaxLen,
        .bValidate = bValidate,
        .u16LinesProcessed = 0,
        .u8LineItemsLen = 0,
        .u8PresentablesCount = 0,
        .szNumberOfWarnings = 0};

    uint16_t u16LinesLen;
    PINCFG_RESULT_T eResult = PINCFG_ERROR_E;

    if (sPrms.pszMemoryRequired != NULL)
        *(sPrms.pszMemoryRequired) = 0;

    if (pcOutString != NULL && u16OutStrMaxLen > 0)
        pcOutString[0] = '\0';

    eResult = PinCfgCsv_CreateExternalCfgReceiver(&sPrms, bRemoteConfigEnabled);
    if (eResult != PINCFG_OK_E)
        return eResult;

    PinCfgStr_vInitStrPoint(&(sPrms.sTempStrPt), psGlobals->acCfgBuf, strlen(psGlobals->acCfgBuf));
    u16LinesLen = (uint8_t)PinCfgStr_szGetSplitCount(&(sPrms.sTempStrPt), PINCFG_LINE_SEPARATOR_D);
    if (u16LinesLen == 1 && sPrms.sTempStrPt.szLen == 0)
    {
        sPrms.pcOutStringLast += snprintf(
            (char *)(pcOutString + sPrms.pcOutStringLast),
            szGetSize(u16OutStrMaxLen, sPrms.pcOutStringLast),
            "E:Invalid format. Empty configuration.\n");
        return PINCFG_ERROR_E;
    }

    for (sPrms.u16LinesProcessed = 0; sPrms.u16LinesProcessed < u16LinesLen; sPrms.u16LinesProcessed++)
    {
        PinCfgStr_vInitStrPoint(&(sPrms.sLine), psGlobals->acCfgBuf, strlen(psGlobals->acCfgBuf));
        PinCfgStr_vGetSplitElemByIndex(&(sPrms.sLine), PINCFG_LINE_SEPARATOR_D, sPrms.u16LinesProcessed);
        if (sPrms.sLine.pcStrStart[0] == '#') // comment continue
            continue;

        sPrms.u8LineItemsLen = (uint8_t)PinCfgStr_szGetSplitCount(&(sPrms.sLine), PINCFG_VALUE_SEPARATOR_D);
        if (sPrms.u8LineItemsLen < 2)
        {
            sPrms.pcOutStringLast += snprintf(
                (char *)(pcOutString + sPrms.pcOutStringLast),
                szGetSize(u16OutStrMaxLen, sPrms.pcOutStringLast),
                _s[FSDS_E],
                _s[WL_E],
                sPrms.u16LinesProcessed,
                "Not defined or invalid format.");
            sPrms.szNumberOfWarnings++;
            continue;
        }

        sPrms.sTempStrPt = sPrms.sLine;
        PinCfgStr_vGetSplitElemByIndex(&(sPrms.sTempStrPt), PINCFG_VALUE_SEPARATOR_D, 0);
        // switches
        if (sPrms.sTempStrPt.szLen >= 1 && sPrms.sTempStrPt.pcStrStart[0] == 'S')
        {
            eResult = PinCfgCsv_ParseSwitch(&sPrms);
            if (eResult != PINCFG_OK_E)
                return eResult;
        }
        // inpins
        else if (sPrms.sTempStrPt.szLen == 1 && sPrms.sTempStrPt.pcStrStart[0] == 'I')
        {
            eResult = PinCfgCsv_ParseInpins(&sPrms);
            if (eResult != PINCFG_OK_E)
                return eResult;
        }
        // triggers
        else if (sPrms.sTempStrPt.szLen == 1 && sPrms.sTempStrPt.pcStrStart[0] == 'T')
        {
            eResult = PinCfgCsv_ParseTriggers(&sPrms);
            if (eResult != PINCFG_OK_E)
                return eResult;
        }
        // global config items
        else if (sPrms.sTempStrPt.szLen == 2 && sPrms.sTempStrPt.pcStrStart[0] == 'C')
        {
            eResult = PinCfgCsv_ParseGlobalConfigItems(&sPrms);
            if (eResult != PINCFG_OK_E)
                return eResult;
        }
        else
        {
            sPrms.pcOutStringLast += snprintf(
                (char *)(pcOutString + sPrms.pcOutStringLast),
                szGetSize(u16OutStrMaxLen, sPrms.pcOutStringLast),
                _s[FSDS_E],
                _s[EL_E],
                sPrms.u16LinesProcessed,
                "Unknown type.");
            sPrms.szNumberOfWarnings++;
        }
    }
    sPrms.pcOutStringLast += snprintf(
        (char *)(pcOutString + sPrms.pcOutStringLast),
        szGetSize(u16OutStrMaxLen, sPrms.pcOutStringLast),
        "I: Configuration parsed.\n");

    if (sPrms.szNumberOfWarnings > 0)
        return PINCFG_WARNINGS_E;

    return PINCFG_OK_E;
}

PINCFG_RESULT_T PinCfgCsv_eValidate(size_t *pszMemoryRequired, char *pcOutString, const uint16_t u16OutStrMaxLen)
{
    return PinCfgCsv_eParse(pszMemoryRequired, pcOutString, u16OutStrMaxLen, true, false);
}

#ifdef MY_CONTROLLER_HA
static bool bInitialValueSent = false;
#endif

void PinCfgCsv_vLoop(uint32_t u32ms)
{
    if (psGlobals == NULL)
        return;

#ifdef MY_CONTROLLER_HA
    if (!bInitialValueSent)
    {
        LOOPRE_T *psCurrent = psGlobals->psPresentablesFirst;
        bool bAllPresented = true;
        while (psCurrent != NULL)
        {
            if (!psCurrent->bStatePresented)
            {
                psCurrent->psVtab->vPresentState(psCurrent);
                psGlobals->sPincfgIf.vWait(50);
                bAllPresented = false;
            }
            psCurrent = psCurrent->psNextPresentable;
        }
        if (bAllPresented)
            bInitialValueSent = true;
    }
#endif

    LOOPRE_T *psCurrent = psGlobals->psLoopablesFirst;
    while (psCurrent != NULL)
    {
        psCurrent->psVtab->vLoop(psCurrent, u32ms);
        psCurrent = psCurrent->psNextLoopable;
    }
}

void PinCfgCsv_vPresentation(void)
{
    if (psGlobals == NULL)
        return;

    LOOPRE_T *psCurrent = psGlobals->psPresentablesFirst;
    while (psCurrent != NULL)
    {
        psCurrent->psVtab->vPresent(psCurrent);
        psCurrent = psCurrent->psNextPresentable;
        psGlobals->sPincfgIf.vWait(50);
    }
}

void PinCfgCfg_vReceive(const uint8_t u8Id, const void *pvMessage)
{
    if (psGlobals == NULL)
        return;

    LOOPRE_T *psReceiver = PinCfgCsv_psFindInPresentablesById(u8Id);
    if (psReceiver != NULL)
    {
        psReceiver->psVtab->vReceive(psReceiver, pvMessage);
    }
}

// private
static inline PINCFG_RESULT_T PinCfgCsv_CreateExternalCfgReceiver(
    PINCFG_PARSE_SUBFN_PARAMS_T *psPrms,
    bool bRemoteConfigEnabled)
{
    if (psPrms == NULL)
        return PINCFG_NULLPTR_ERROR_E;

    if (!psPrms->bValidate && bRemoteConfigEnabled)
    {
        EXTCFGRECEIVER_HANDLE_T *psCfgRcvrHnd =
            (EXTCFGRECEIVER_HANDLE_T *)Memory_vpAlloc(sizeof(EXTCFGRECEIVER_HANDLE_T));
        if (psCfgRcvrHnd == NULL)
        {
            Memory_eReset();
            psPrms->pcOutStringLast += snprintf(
                (char *)(psPrms->pcOutString + psPrms->pcOutStringLast),
                szGetSize(psPrms->u16OutStrMaxLen, psPrms->pcOutStringLast),
                _s[FSSS_E],
                _s[E_E],
                _s[ECR_E],
                _s[OOM_E]);

            return PINCFG_OUTOFMEMORY_ERROR_E;
        }

        if (ExtCfgReceiver_eInit(psCfgRcvrHnd, psPrms->u8PresentablesCount) == EXTCFGRECEIVER_OK_E)
        {
            PinCfgCsv_vAddToPresentables((LOOPRE_T *)psCfgRcvrHnd);
            psPrms->u8PresentablesCount++;
        }
        else
        {
            psPrms->pcOutStringLast += snprintf(
                (char *)(psPrms->pcOutString + psPrms->pcOutStringLast),
                szGetSize(psPrms->u16OutStrMaxLen, psPrms->pcOutStringLast),
                _s[FSSS_E],
                _s[W_E],
                _s[ECR_E],
                _s[INITF_E]);
            psPrms->szNumberOfWarnings++;
        }
    }
    if (psPrms->pszMemoryRequired != NULL)
        *psPrms->pszMemoryRequired += szGetAllocatedSize(sizeof(EXTCFGRECEIVER_HANDLE_T));

    return PINCFG_OK_E;
}

static inline PINCFG_RESULT_T PinCfgCsv_ParseSwitch(PINCFG_PARSE_SUBFN_PARAMS_T *psPrms)
{
    uint8_t u8Pin, u8FbPin, u8Count, u8Offset, u8SwItems, i;
    SWITCH_MODE_T eMode = SWITCH_CLASSIC_E;
    bool bIsDefinitionValid = true;

    u8SwItems = 2;
    u8FbPin = 0U;

    if (psPrms == NULL)
        return PINCFG_NULLPTR_ERROR_E;

    if (psPrms->u8LineItemsLen < 3)
    {
        psPrms->pcOutStringLast += snprintf(
            (char *)(psPrms->pcOutString + psPrms->pcOutStringLast),
            szGetSize(psPrms->u16OutStrMaxLen, psPrms->pcOutStringLast),
            _s[FSDSSS_E],
            _s[WL_E],
            psPrms->u16LinesProcessed,
            _s[SW_E],
            _s[IN_E],
            _s[OARGS_E]);
        psPrms->szNumberOfWarnings++;
        return PINCFG_OK_E;
    }

    if (psPrms->sTempStrPt.szLen >= 2)
    {
        if (psPrms->sTempStrPt.pcStrStart[1] == 'I')
            eMode = SWITCH_IMPULSE_E;
        else if ((psPrms->sTempStrPt.pcStrStart[1] == 'F'))
            u8SwItems = 3;
        else
            bIsDefinitionValid = false;

        if (psPrms->sTempStrPt.szLen >= 3)
        {
            if (psPrms->sTempStrPt.pcStrStart[2] == 'F')
                u8SwItems = 3;
            else
                bIsDefinitionValid = false;
        }

        if (!bIsDefinitionValid)
        {
            psPrms->pcOutStringLast += snprintf(
                (char *)(psPrms->pcOutString + psPrms->pcOutStringLast),
                szGetSize(psPrms->u16OutStrMaxLen, psPrms->pcOutStringLast),
                _s[FSDSS_E],
                _s[WL_E],
                psPrms->u16LinesProcessed,
                _s[SW_E],
                "Invalid definition.");
            psPrms->szNumberOfWarnings++;
            return PINCFG_OK_E;
        }
    }

    u8Count = (uint8_t)((psPrms->u8LineItemsLen - 1) % u8SwItems);
    if (u8Count != 0)
    {
        psPrms->pcOutStringLast += snprintf(
            (char *)(psPrms->pcOutString + psPrms->pcOutStringLast),
            szGetSize(psPrms->u16OutStrMaxLen, psPrms->pcOutStringLast),
            _s[FSDSSS_E],
            _s[WL_E],
            psPrms->u16LinesProcessed,
            _s[SW_E],
            _s[IN_E],
            _s[OITMS_E]);
        psPrms->szNumberOfWarnings++;
        return PINCFG_OK_E;
    }

    u8Count = (uint8_t)((psPrms->u8LineItemsLen - 1) / u8SwItems);
    for (i = 0; i < u8Count; i++)
    {
        u8Offset = 1 + i * u8SwItems;

        psPrms->sTempStrPt = psPrms->sLine;
        PinCfgStr_vGetSplitElemByIndex(&(psPrms->sTempStrPt), PINCFG_VALUE_SEPARATOR_D, (u8Offset + 1));
        if (PinCfgStr_eAtoU8(&(psPrms->sTempStrPt), &u8Pin) != PINCFG_STR_OK_E)
        {
            psPrms->pcOutStringLast += snprintf(
                (char *)(psPrms->pcOutString + psPrms->pcOutStringLast),
                szGetSize(psPrms->u16OutStrMaxLen, psPrms->pcOutStringLast),
                _s[FSDSS_E],
                _s[WL_E],
                psPrms->u16LinesProcessed,
                _s[SW_E],
                _s[IPN_E]);
            psPrms->szNumberOfWarnings++;
            continue;
        }

        if (u8SwItems == 3)
        {
            psPrms->sTempStrPt = psPrms->sLine;
            PinCfgStr_vGetSplitElemByIndex(&(psPrms->sTempStrPt), PINCFG_VALUE_SEPARATOR_D, (u8Offset + 2));
            if (PinCfgStr_eAtoU8(&(psPrms->sTempStrPt), &u8FbPin) != PINCFG_STR_OK_E)
            {
                psPrms->pcOutStringLast += snprintf(
                    (char *)(psPrms->pcOutString + psPrms->pcOutStringLast),
                    szGetSize(psPrms->u16OutStrMaxLen, psPrms->pcOutStringLast),
                    _s[FSDSS_E],
                    _s[WL_E],
                    psPrms->u16LinesProcessed,
                    _s[SW_E],
                    _s[IPN_E]);
                psPrms->szNumberOfWarnings++;
                continue;
            }
        }

        psPrms->sTempStrPt = psPrms->sLine;
        PinCfgStr_vGetSplitElemByIndex(&(psPrms->sTempStrPt), PINCFG_VALUE_SEPARATOR_D, u8Offset);
        if (!psPrms->bValidate)
        {
            SWITCH_HANDLE_T *psSwitchHnd = (SWITCH_HANDLE_T *)Memory_vpAlloc(sizeof(SWITCH_HANDLE_T));
            if (psSwitchHnd == NULL)
            {
                Memory_eReset();

                psPrms->pcOutStringLast += snprintf(
                    (char *)(psPrms->pcOutString + psPrms->pcOutStringLast),
                    szGetSize(psPrms->u16OutStrMaxLen, psPrms->pcOutStringLast),
                    _s[FSDSS_E],
                    _s[EL_E],
                    psPrms->u16LinesProcessed,
                    _s[SW_E],
                    _s[OOM_E]);

                return PINCFG_OUTOFMEMORY_ERROR_E;
            }

            if (Switch_eInit(psSwitchHnd, &(psPrms->sTempStrPt), psPrms->u8PresentablesCount, eMode, u8Pin, u8FbPin) ==
                SWITCH_OK_E)
            {
                PinCfgCsv_vAddToPresentables((LOOPRE_T *)psSwitchHnd);
                psPrms->u8PresentablesCount++;
                PinCfgCsv_vAddToLoopables((LOOPRE_T *)psSwitchHnd);
            }
            else
            {
                psPrms->pcOutStringLast += snprintf(
                    (char *)(psPrms->pcOutString + psPrms->pcOutStringLast),
                    szGetSize(psPrms->u16OutStrMaxLen, psPrms->pcOutStringLast),
                    _s[FSDSS_E],
                    _s[WL_E],
                    psPrms->u16LinesProcessed,
                    _s[SW_E],
                    _s[INITF_E]);
                psPrms->szNumberOfWarnings++;
            }
        }

        if (psPrms->pszMemoryRequired != NULL)
            *(psPrms->pszMemoryRequired) +=
                szGetAllocatedSize(sizeof(SWITCH_HANDLE_T)) + szGetAllocatedSize(psPrms->sTempStrPt.szLen + 1);
    }

    return PINCFG_OK_E;
}

static inline PINCFG_RESULT_T PinCfgCsv_ParseInpins(PINCFG_PARSE_SUBFN_PARAMS_T *psPrms)
{
    uint8_t u8Pin, u8Count, u8Offset, i;

    if (psPrms == NULL)
        return PINCFG_NULLPTR_ERROR_E;

    if (psPrms->u8LineItemsLen < 3)
    {
        psPrms->pcOutStringLast += snprintf(
            (char *)(psPrms->pcOutString + psPrms->pcOutStringLast),
            szGetSize(psPrms->u16OutStrMaxLen, psPrms->pcOutStringLast),
            _s[FSDSSS_E],
            _s[WL_E],
            psPrms->u16LinesProcessed,
            _s[IP_E],
            _s[IN_E],
            _s[OARGS_E]);
        psPrms->szNumberOfWarnings++;

        return PINCFG_OK_E;
    }

    u8Count = ((psPrms->u8LineItemsLen - 1) % 2);
    if (u8Count != 0)
    {
        psPrms->pcOutStringLast += snprintf(
            (char *)(psPrms->pcOutString + psPrms->pcOutStringLast),
            szGetSize(psPrms->u16OutStrMaxLen, psPrms->pcOutStringLast),
            _s[FSDSSS_E],
            _s[WL_E],
            psPrms->u16LinesProcessed,
            _s[IP_E],
            _s[IN_E],
            _s[OITMS_E]);
        psPrms->szNumberOfWarnings++;
        return PINCFG_OK_E;
    }

    u8Count = ((psPrms->u8LineItemsLen - 1) / 2);
    for (i = 0; i < u8Count; i++)
    {
        u8Offset = 1 + i * 2;
        psPrms->sTempStrPt = psPrms->sLine;
        PinCfgStr_vGetSplitElemByIndex(&(psPrms->sTempStrPt), PINCFG_VALUE_SEPARATOR_D, (u8Offset + 1));
        if (PinCfgStr_eAtoU8(&(psPrms->sTempStrPt), &u8Pin) != PINCFG_STR_OK_E || u8Pin < 1)
        {
            psPrms->pcOutStringLast += snprintf(
                (char *)(psPrms->pcOutString + psPrms->pcOutStringLast),
                szGetSize(psPrms->u16OutStrMaxLen, psPrms->pcOutStringLast),
                _s[FSDSS_E],
                _s[WL_E],
                psPrms->u16LinesProcessed,
                _s[IP_E],
                _s[IPN_E]);
            psPrms->szNumberOfWarnings++;
            continue;
        }

        psPrms->sTempStrPt = psPrms->sLine;
        PinCfgStr_vGetSplitElemByIndex(&(psPrms->sTempStrPt), PINCFG_VALUE_SEPARATOR_D, u8Offset);
        if (!psPrms->bValidate)
        {
            INPIN_HANDLE_T *psInPinHnd = (INPIN_HANDLE_T *)Memory_vpAlloc(sizeof(INPIN_HANDLE_T));
            if (psInPinHnd == NULL)
            {
                Memory_eReset();
                psPrms->pcOutStringLast += snprintf(
                    (char *)(psPrms->pcOutString + psPrms->pcOutStringLast),
                    szGetSize(psPrms->u16OutStrMaxLen, psPrms->pcOutStringLast),
                    _s[FSDSS_E],
                    _s[EL_E],
                    psPrms->u16LinesProcessed,
                    _s[IP_E],
                    _s[OOM_E]);

                return PINCFG_OUTOFMEMORY_ERROR_E;
            }

            if (InPin_eInit(psInPinHnd, &(psPrms->sTempStrPt), psPrms->u8PresentablesCount, u8Pin) == INPIN_OK_E)
            {
                PinCfgCsv_vAddToPresentables((LOOPRE_T *)psInPinHnd);
                psPrms->u8PresentablesCount++;
                PinCfgCsv_vAddToLoopables((LOOPRE_T *)psInPinHnd);
            }
            else
            {
                psPrms->pcOutStringLast += snprintf(
                    (char *)(psPrms->pcOutString + psPrms->pcOutStringLast),
                    szGetSize(psPrms->u16OutStrMaxLen, psPrms->pcOutStringLast),
                    _s[FSDSS_E],
                    _s[WL_E],
                    psPrms->u16LinesProcessed,
                    _s[IP_E],
                    _s[INITF_E]);
                psPrms->szNumberOfWarnings++;
            }
        }

        if (psPrms->pszMemoryRequired != NULL)
            *(psPrms->pszMemoryRequired) +=
                szGetAllocatedSize(sizeof(INPIN_HANDLE_T)) + szGetAllocatedSize(psPrms->sTempStrPt.szLen + 1);
    }

    return PINCFG_OK_E;
}

static inline PINCFG_RESULT_T PinCfgCsv_ParseTriggers(PINCFG_PARSE_SUBFN_PARAMS_T *psPrms)
{
    uint8_t u8Count, u8EventType, u8EventCount, u8DrivesCountReal, u8Offset, u8DrivenAction, i;
    INPIN_HANDLE_T *psInPinEmiterHnd;
    SWITCH_HANDLE_T *psSwitchHnd;
    TRIGGER_SWITCHACTION_T *pasSwActs;
    TRIGGER_SWITCHACTION_T *psSwAct;

    if (psPrms == NULL)
        return PINCFG_NULLPTR_ERROR_E;

    if (psPrms->u8LineItemsLen < 7)
    {
        psPrms->pcOutStringLast += snprintf(
            (char *)(psPrms->pcOutString + psPrms->pcOutStringLast),
            szGetSize(psPrms->u16OutStrMaxLen, psPrms->pcOutStringLast),
            _s[FSDSSS_E],
            _s[WL_E],
            psPrms->u16LinesProcessed,
            _s[TRG_E],
            _s[IN_E],
            _s[OARGS_E]);
        psPrms->szNumberOfWarnings++;

        return PINCFG_OK_E;
    }

    psPrms->sTempStrPt = psPrms->sLine;
    PinCfgStr_vGetSplitElemByIndex(&(psPrms->sTempStrPt), PINCFG_VALUE_SEPARATOR_D, 2);

    if (psPrms->bValidate)
    {
        if (PinCfgCsv_pcStrstrpt(psGlobals->acCfgBuf, &(psPrms->sTempStrPt)) == NULL)
        {
            psPrms->pcOutStringLast += snprintf(
                (char *)(psPrms->pcOutString + psPrms->pcOutStringLast),
                szGetSize(psPrms->u16OutStrMaxLen, psPrms->pcOutStringLast),
                _s[FSDSS_E],
                _s[WL_E],
                psPrms->u16LinesProcessed,
                _s[TRG_E],
                _s[SNNF_E]);
            psPrms->szNumberOfWarnings++;

            return PINCFG_OK_E;
        }
    }
    else
    {
        psInPinEmiterHnd = (INPIN_HANDLE_T *)PinCfgCsv_psFindInLoopablesByName(&(psPrms->sTempStrPt));
        if (psInPinEmiterHnd == NULL)
        {
            psPrms->pcOutStringLast += snprintf(
                (char *)(psPrms->pcOutString + psPrms->pcOutStringLast),
                szGetSize(psPrms->u16OutStrMaxLen, psPrms->pcOutStringLast),
                _s[FSDSS_E],
                _s[WL_E],
                psPrms->u16LinesProcessed,
                _s[TRG_E],
                "Invalid InPin name.");
            psPrms->szNumberOfWarnings++;

            return PINCFG_OK_E;
        }
    }

    psPrms->sTempStrPt = psPrms->sLine;
    PinCfgStr_vGetSplitElemByIndex(&(psPrms->sTempStrPt), PINCFG_VALUE_SEPARATOR_D, 3);
    if (PinCfgStr_eAtoU8(&(psPrms->sTempStrPt), &u8EventType) != PINCFG_STR_OK_E || u8EventType > (uint8_t)TRIGGER_LONG)
    {
        psPrms->pcOutStringLast += snprintf(
            (char *)(psPrms->pcOutString + psPrms->pcOutStringLast),
            szGetSize(psPrms->u16OutStrMaxLen, psPrms->pcOutStringLast),
            _s[FSDSS_E],
            _s[WL_E],
            psPrms->u16LinesProcessed,
            _s[TRG_E],
            "Invalid format for event type.");
        psPrms->szNumberOfWarnings++;

        return PINCFG_OK_E;
    }

    psPrms->sTempStrPt = psPrms->sLine;
    PinCfgStr_vGetSplitElemByIndex(&(psPrms->sTempStrPt), PINCFG_VALUE_SEPARATOR_D, 4);
    if (PinCfgStr_eAtoU8(&(psPrms->sTempStrPt), &u8EventCount) != PINCFG_STR_OK_E)
    {
        psPrms->pcOutStringLast += snprintf(
            (char *)(psPrms->pcOutString + psPrms->pcOutStringLast),
            szGetSize(psPrms->u16OutStrMaxLen, psPrms->pcOutStringLast),
            _s[FSDSS_E],
            _s[WL_E],
            psPrms->u16LinesProcessed,
            _s[TRG_E],
            "Invalid format for event count.");
        psPrms->szNumberOfWarnings++;

        return PINCFG_OK_E;
    }

    psPrms->sTempStrPt = psPrms->sLine;
    u8Count = (uint8_t)((psPrms->u8LineItemsLen - 5) / 2);
    u8DrivesCountReal = 0;
    pasSwActs = NULL;
    for (i = 0; i < u8Count; i++)
    {
        u8Offset = 5 + i * 2;
        psPrms->sTempStrPt = psPrms->sLine;
        PinCfgStr_vGetSplitElemByIndex(&(psPrms->sTempStrPt), PINCFG_VALUE_SEPARATOR_D, u8Offset);
        if (psPrms->bValidate)
        {
            if (PinCfgCsv_pcStrstrpt(psGlobals->acCfgBuf, &(psPrms->sTempStrPt)) == NULL)
            {
                psPrms->pcOutStringLast += snprintf(
                    (char *)(psPrms->pcOutString + psPrms->pcOutStringLast),
                    szGetSize(psPrms->u16OutStrMaxLen, psPrms->pcOutStringLast),
                    _s[FSDSS_E],
                    _s[WL_E],
                    psPrms->u16LinesProcessed,
                    _s[TRG_E],
                    _s[SNNF_E]);
                psPrms->szNumberOfWarnings++;
                continue;
            }
        }
        else
        {
            psSwitchHnd = (SWITCH_HANDLE_T *)PinCfgCsv_psFindInLoopablesByName(&(psPrms->sTempStrPt));
            if (psSwitchHnd == NULL)
            {
                psPrms->pcOutStringLast += snprintf(
                    (char *)(psPrms->pcOutString + psPrms->pcOutStringLast),
                    szGetSize(psPrms->u16OutStrMaxLen, psPrms->pcOutStringLast),
                    _s[FSDSS_E],
                    _s[WL_E],
                    psPrms->u16LinesProcessed,
                    _s[TRG_E],
                    "Invalid switch name.");
                psPrms->szNumberOfWarnings++;
                continue;
            }
        }

        psPrms->sTempStrPt = psPrms->sLine;
        PinCfgStr_vGetSplitElemByIndex(&(psPrms->sTempStrPt), PINCFG_VALUE_SEPARATOR_D, (u8Offset + 1));
        if (PinCfgStr_eAtoU8(&(psPrms->sTempStrPt), &u8DrivenAction) != PINCFG_STR_OK_E || u8DrivenAction > 2)
        {
            psPrms->pcOutStringLast += snprintf(
                (char *)(psPrms->pcOutString + psPrms->pcOutStringLast),
                szGetSize(psPrms->u16OutStrMaxLen, psPrms->pcOutStringLast),
                _s[FSDSS_E],
                _s[WL_E],
                psPrms->u16LinesProcessed,
                _s[TRG_E],
                "Invalid switch action.");
            psPrms->szNumberOfWarnings++;
            continue;
        }
        u8DrivesCountReal++;

        if (!psPrms->bValidate)
        {
            psSwAct = (TRIGGER_SWITCHACTION_T *)Memory_vpAlloc(sizeof(TRIGGER_SWITCHACTION_T));
            if (psSwAct == NULL)
            {
                Memory_eReset();
                psPrms->pcOutStringLast += snprintf(
                    (char *)(psPrms->pcOutString + psPrms->pcOutStringLast),
                    szGetSize(psPrms->u16OutStrMaxLen, psPrms->pcOutStringLast),
                    _s[FSDSSS_E],
                    _s[EL_E],
                    psPrms->u16LinesProcessed,
                    _s[TRG_E],
                    "Switchaction:",
                    _s[OOM_E]);

                return PINCFG_OUTOFMEMORY_ERROR_E;
            }
            psSwAct->psSwitchHnd = psSwitchHnd;
            psSwAct->eAction = (TRIGGER_ACTION_T)u8DrivenAction;
            if (pasSwActs == NULL)
            {
                pasSwActs = psSwAct;
            }
        }

        if (psPrms->pszMemoryRequired != NULL)
            *(psPrms->pszMemoryRequired) += szGetAllocatedSize(sizeof(TRIGGER_SWITCHACTION_T));
    }

    if (!psPrms->bValidate)
    {
        if ((u8DrivesCountReal == 0U || pasSwActs == NULL))
        {
            psPrms->pcOutStringLast += snprintf(
                (char *)(psPrms->pcOutString + psPrms->pcOutStringLast),
                szGetSize(psPrms->u16OutStrMaxLen, psPrms->pcOutStringLast),
                _s[FSDSS_E],
                _s[WL_E],
                psPrms->u16LinesProcessed,
                _s[TRG_E],
                "Nothing to drive.");
            psPrms->szNumberOfWarnings++;

            return PINCFG_OK_E;
        }

        TRIGGER_HANDLE_T *psTriggerHnd = (TRIGGER_HANDLE_T *)Memory_vpAlloc(sizeof(TRIGGER_HANDLE_T));
        if (psTriggerHnd == NULL)
        {
            Memory_eReset();
            psPrms->pcOutStringLast += snprintf(
                (char *)(psPrms->pcOutString + psPrms->pcOutStringLast),
                szGetSize(psPrms->u16OutStrMaxLen, psPrms->pcOutStringLast),
                _s[FSDSS_E],
                _s[EL_E],
                psPrms->u16LinesProcessed,
                _s[TRG_E],
                _s[OOM_E]);

            return PINCFG_OUTOFMEMORY_ERROR_E;
        }
        if (Trigger_eInit(psTriggerHnd, pasSwActs, u8DrivesCountReal, (TRIGGER_EVENTTYPE_T)u8EventType, u8EventCount) ==
            TRIGGER_OK_E)
        {
            InPin_eAddSubscriber(psInPinEmiterHnd, (PINSUBSCRIBER_IF_T *)psTriggerHnd);
        }
        else
        {
            Memory_eReset();
            psPrms->pcOutStringLast += snprintf(
                (char *)(psPrms->pcOutString + psPrms->pcOutStringLast),
                szGetSize(psPrms->u16OutStrMaxLen, psPrms->pcOutStringLast),
                _s[FSDSS_E],
                _s[WL_E],
                psPrms->u16LinesProcessed,
                _s[TRG_E],
                _s[INITF_E]);
            psPrms->szNumberOfWarnings++;
        }
    }

    if (psPrms->pszMemoryRequired != NULL)
        *(psPrms->pszMemoryRequired) += szGetAllocatedSize(sizeof(TRIGGER_HANDLE_T));

    return PINCFG_OK_E;
}

static inline PINCFG_RESULT_T PinCfgCsv_ParseGlobalConfigItems(PINCFG_PARSE_SUBFN_PARAMS_T *psPrms)
{
    PINCFG_PARSE_STRINGS_T eItem;
    uint32_t u32ParsedNumber = 0;
    uint32_t u32Min = 0;

    switch (psPrms->sTempStrPt.pcStrStart[1])
    {
    case 'D': eItem = IPDMS_E; break;
    case 'M': eItem = IPMCDMS_E; break;
    case 'R':
    {
        eItem = SWIDMS_E;
        u32Min = 50;
    }
    break;
    case 'N': eItem = SWFNDMS_E; break;
    case 'F': eItem = SWFFDMS_E; break;
    case 'A': eItem = SWTAAMS_E; break;
    default:
    {
        psPrms->pcOutStringLast += snprintf(
            (char *)(psPrms->pcOutString + psPrms->pcOutStringLast),
            szGetSize(psPrms->u16OutStrMaxLen, psPrms->pcOutStringLast),
            _s[FSDS_E],
            _s[WL_E],
            psPrms->u16LinesProcessed,
            " Unknown global configuration item.");
        psPrms->szNumberOfWarnings++;

        return PINCFG_OK_E;
    }
    break;
    }

    if (psPrms->u8LineItemsLen < 2)
    {
        psPrms->pcOutStringLast += snprintf(
            (char *)(psPrms->pcOutString + psPrms->pcOutStringLast),
            szGetSize(psPrms->u16OutStrMaxLen, psPrms->pcOutStringLast),
            _s[FSDSSS_E],
            _s[WL_E],
            psPrms->u16LinesProcessed,
            _s[eItem],
            _s[IN_E],
            _s[OARGS_E]);
        psPrms->szNumberOfWarnings++;

        return PINCFG_OK_E;
    }

    psPrms->sTempStrPt = psPrms->sLine;
    PinCfgStr_vGetSplitElemByIndex(&(psPrms->sTempStrPt), PINCFG_VALUE_SEPARATOR_D, 1);
    if (PinCfgStr_eAtoU32(&(psPrms->sTempStrPt), &u32ParsedNumber) != PINCFG_STR_OK_E || u32ParsedNumber < u32Min)
    {
        psPrms->pcOutStringLast += snprintf(
            (char *)(psPrms->pcOutString + psPrms->pcOutStringLast),
            szGetSize(psPrms->u16OutStrMaxLen, psPrms->pcOutStringLast),
            _s[FSDSSS_E],
            _s[WL_E],
            psPrms->u16LinesProcessed,
            _s[eItem],
            _s[IN_E],
            ".");
        psPrms->szNumberOfWarnings++;

        return PINCFG_OK_E;
    }

    switch (eItem)
    {
    case IPDMS_E: InPin_SetDebounceMs(u32ParsedNumber); break;
    case IPMCDMS_E: InPin_SetMulticlickMaxDelayMs(u32ParsedNumber); break;
    case SWIDMS_E: Switch_SetImpulseDurationMs(u32ParsedNumber); break;
    case SWFNDMS_E: Switch_SetFbOnDelayMs(u32ParsedNumber); break;
    case SWFFDMS_E: Switch_SetFbOffDelayMs(u32ParsedNumber); break;
    case SWTAAMS_E: Switch_vSetTimedActionAdditionMs(u32ParsedNumber); break;
    default: break;
    }

    return PINCFG_OK_E;
}

static inline LOOPRE_T *PinCfgCsv_psFindInPresentablesById(uint8_t u8Id)
{
    LOOPRE_T *psCurrent = psGlobals->psPresentablesFirst;
    while (psCurrent != NULL && (LooPre_u8GetId(psCurrent) != u8Id))
    {
        psCurrent = psCurrent->psNextPresentable;
    }

    return psCurrent;
}

static inline void PinCfgCsv_vAddToLoopables(LOOPRE_T *psLoopable)
{
    if (psGlobals->psLoopablesFirst == NULL)
        psGlobals->psLoopablesFirst = psLoopable;
    else
    {
        LOOPRE_T *psCurrent = psGlobals->psLoopablesFirst;
        while (psCurrent->psNextLoopable != NULL)
        {
            psCurrent = psCurrent->psNextLoopable;
        }
        psCurrent->psNextLoopable = psLoopable;
    }
}

static LOOPRE_T *PinCfgCsv_psFindInLoopablesByName(const STRING_POINT_T *psName)
{
    LOOPRE_T *psReturn = psGlobals->psLoopablesFirst;
    char *pcTempStr = (char *)Memory_vpTempAlloc(psName->szLen + 1);

    if (pcTempStr != NULL)
    {
        memcpy(pcTempStr, psName->pcStrStart, psName->szLen);
        pcTempStr[psName->szLen] = '\0';
        while (psReturn != NULL && strcmp(LooPre_pcGetName(psReturn), pcTempStr) != 0)
        {
            psReturn = psReturn->psNextLoopable;
        }
    }
    Memory_vTempFreeSize(psName->szLen + 1);

    return psReturn;
}

static inline void PinCfgCsv_vAddToPresentables(LOOPRE_T *psPresentable)
{
    if (psGlobals->psPresentablesFirst == NULL)
        psGlobals->psPresentablesFirst = psPresentable;
    else
    {
        LOOPRE_T *psCurrent = psGlobals->psPresentablesFirst;
        while (psCurrent->psNextPresentable != NULL)
        {
            psCurrent = psCurrent->psNextPresentable;
        }
        psCurrent->psNextPresentable = psPresentable;
    }
}

static inline size_t szGetAllocatedSize(size_t szToAllocate)
{
    size_t szReturn = szToAllocate / sizeof(char *);
    if (szToAllocate % sizeof(char *) > 0)
        szReturn++;
    szReturn *= sizeof(char *);

    return szReturn;
}

static inline size_t szGetSize(size_t a, size_t b)
{
    if (b >= a)
        return 0;

    return a - b;
}
