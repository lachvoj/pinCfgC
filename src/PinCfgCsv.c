#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include "CPUTemp.h"
#include "Cli.h"
#include "Globals.h"
#include "InPin.h"
#include "Memory.h"
#include "MySensorsWrapper.h"
#include "PersistentConfigiration.h"
#include "PinCfgCsv.h"
#include "PinCfgStr.h"
#include "Switch.h"
#include "Trigger.h"

typedef struct PINCFG_PARSE_SUBFN_PARAMS_S
{
    PINCFG_PARSE_PARAMS_T *psParsePrms;
    size_t pcOutStringLast;
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
    "CLI:",                               // ECR_E
    "Switch:",                            // SW_E
    "InPin:",                             // IP_E
    "Trigger:",                           // TRG_E
    "CPUTemperature",                     // CPUTMP_E
    "InPinDebounceMs:",                   // IPDMS_E
    "InPinMulticlickMaxDelayMs:",         // IPMCDMS_E
    "SwitchImpulseDurationMs:",           // SWIDMS_E
    "SwitchFbOnDelayMs:",                 // SWFNDMS_E
    "SwitchFbOffDelayMs:",                // SWFFDMS_E
    "Out of memory.",                     // OOM_E
    "Init failed!",                       // INITF_E
    "Invalid pin number.",                // IPN_E
    "Invalid number",                     // IN_E
    " of arguments.",                     // OARGS_E
    " of items defining names and pins.", // OITMS_E
    "Switch name not found."              // SNNF_E
};

typedef enum PINCFG_PARSE_STRINGS_E
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
    CPUTMP_E,
    IPDMS_E,
    IPMCDMS_E,
    SWIDMS_E,
    SWFNDMS_E,
    SWFFDMS_E,
    OOM_E,
    INITF_E,
    IPN_E,
    IN_E,
    OARGS_E,
    OITMS_E,
    SNNF_E
} PINCFG_PARSE_STRINGS_T;

static inline PINCFG_RESULT_T PinCfgCsv_CreateCli(PINCFG_PARSE_SUBFN_PARAMS_T *psPrms);
static inline PINCFG_RESULT_T PinCfgCsv_ParseSwitch(PINCFG_PARSE_SUBFN_PARAMS_T *psPrms);
static inline PINCFG_RESULT_T PinCfgCsv_ParseInpins(PINCFG_PARSE_SUBFN_PARAMS_T *psPrms);
static inline PINCFG_RESULT_T PinCfgCsv_ParseTriggers(PINCFG_PARSE_SUBFN_PARAMS_T *psPrms);
static inline PINCFG_RESULT_T PinCfgCsv_ParseTemperatureSensors(PINCFG_PARSE_SUBFN_PARAMS_T *psPrms);
static inline PINCFG_RESULT_T PinCfgCsv_ParseCPUTemperatureSensor(PINCFG_PARSE_SUBFN_PARAMS_T *psPrms);

static inline PINCFG_RESULT_T PinCfgCsv_ParseGlobalConfigItems(PINCFG_PARSE_SUBFN_PARAMS_T *psPrms);

static inline PRESENTABLE_T *PinCfgCsv_psFindInPresentablesById(uint8_t u8Id);
static PRESENTABLE_T *PinCfgCsv_psFindInTempPresentablesByName(const STRING_POINT_T *psName);
static inline size_t szGetAllocatedSize(size_t szToAllocate);
static inline size_t szGetSize(size_t a, size_t b);

#ifdef MY_CONTROLLER_HA
static bool bInitialValueSent = false;
#endif

void PinCfgCsv_vLoop(uint32_t u32ms)
{
#ifdef MY_CONTROLLER_HA
    if (!bInitialValueSent)
    {
        bool bAllPresented = true;
        for (uint8_t i = 0; i < psGlobals->u8PresentablesCount; i++)
        {
            PRESENTABLE_T *psCurrent = psGlobals->ppsPresentables[i];
            if (!psCurrent->bStatePresented)
            {
                Presentable_vPresentState(psCurrent);
                vWait(50);
                bAllPresented = false;
            }
        }
        if (bAllPresented)
            bInitialValueSent = true;
    }
#endif

    for (uint8_t i = 0; i < psGlobals->u8LoopablesCount; i++)
    {
        LOOPABLE_T *psCurrent = psGlobals->ppsLoopables[i];
        psCurrent->vLoop(psCurrent, u32ms);
    }
}

void PinCfgCsv_vPresentation(void)
{
    for (uint8_t i = 0; i < psGlobals->u8PresentablesCount; i++)
    {
        Presentable_vPresent(psGlobals->ppsPresentables[i]);
        vWait(50);
    }
}

void PinCfgCfg_vReceive(const uint8_t u8Id, const void *pvMessage)
{
    if (psGlobals == NULL)
        return;

    PRESENTABLE_T *psReceiver = PinCfgCsv_psFindInPresentablesById(u8Id);
    if (psReceiver != NULL)
        psReceiver->psVtab->vReceive(psReceiver, pvMessage);
}

PINCFG_RESULT_T PinCfgCsv_eValidate(
    const char *pcConfig,
    size_t *pszMemoryRequired,
    char *pcOutString,
    const uint16_t u16OutStrMaxLen)
{
    PINCFG_PARSE_PARAMS_T sParseParams = {
        .pcConfig = pcConfig,
        .eAddToLoopables = NULL,
        .eAddToPresentables = NULL,
        .pszMemoryRequired = pszMemoryRequired,
        .pcOutString = pcOutString,
        .u16OutStrMaxLen = u16OutStrMaxLen,
        .bValidate = true};

    return PinCfgCsv_eParse(&sParseParams);
}

PINCFG_RESULT_T PinCfgCsv_eInitMemoryAndTypes(uint8_t *pu8Memory, size_t szMemorySize)
{
    if (Memory_eInit(pu8Memory, szMemorySize) != MEMORY_OK_E)
    {
        return PINCFG_MEMORYINIT_ERROR_E;
    }

    // V tabs init
    // extcfgreceiver
    Cli_vInitType(&(psGlobals->sCliPrVTab));
    // switch
    Switch_vInitType(&(psGlobals->sSwitchPrVTab));
    // inpin
    InPin_vInitType(&(psGlobals->sInPinPrVTab));
    // CPUTemp
    CPUTemp_vInitType(&(psGlobals->sCpuTempPrVTab));

    return PINCFG_OK_E;
}

PINCFG_RESULT_T PinCfgCsv_eInit(uint8_t *pu8Memory, size_t szMemorySize, const char *pcDefaultCfg)
{
    // Memory init
    if (pu8Memory == NULL)
    {
        return PINCFG_NULLPTR_ERROR_E;
    }

    PINCFG_RESULT_T eMemAndTypesInitRes = PinCfgCsv_eInitMemoryAndTypes(pu8Memory, szMemorySize);
    if (eMemAndTypesInitRes != PINCFG_OK_E)
        return eMemAndTypesInitRes;

    PINCFG_PARSE_PARAMS_T sParseParams = {
        .pcConfig = NULL,
        .eAddToLoopables = PinCfgCsv_eAddToTempLoopables,
        .eAddToPresentables = PinCfgCsv_eAddToTempPresentables,
        .pszMemoryRequired = NULL,
        .pcOutString = NULL,
        .u16OutStrMaxLen = 0,
        .bValidate = false};

    PINCFG_RESULT_T ePincfgResult = PINCFG_ERROR_E;
    size_t szCfg;
    PERCFG_RESULT_T eLoadResult = PersistentCfg_eGetSize((uint16_t *)&szCfg);
    if (eLoadResult == PERCFG_OK_E)
    {
        sParseParams.pcConfig = (char *)Memory_vpTempAlloc(szCfg);
        if (sParseParams.pcConfig == NULL)
            return ePincfgResult;

        eLoadResult = PersistentCfg_eLoad(sParseParams.pcConfig);
    }

    if (eLoadResult == PERCFG_OK_E)
        ePincfgResult = PinCfgCsv_eParse(&sParseParams);

    if (pcDefaultCfg != NULL &&
        (eLoadResult != PERCFG_OK_E || (ePincfgResult != PINCFG_OK_E && ePincfgResult != PINCFG_WARNINGS_E)))
    {
        eMemAndTypesInitRes = PinCfgCsv_eInitMemoryAndTypes(pu8Memory, szMemorySize);
        if (eMemAndTypesInitRes != PINCFG_OK_E)
            return eMemAndTypesInitRes;

        sParseParams.pcConfig = pcDefaultCfg;
        ePincfgResult = PinCfgCsv_eParse(&sParseParams);
        // char sOutStr[1000];
        // ePincfgResult = PinCfgCsv_eParse(NULL, sOutStr, sizeof(sOutStr), false);
    }

    if (ePincfgResult == PINCFG_OK_E)
    {
        // move loopables and presentables from linked list to array
        ePincfgResult = PinCfgCsv_eLinkedListToArray(
            (LINKEDLIST_ITEM_T **)&(psGlobals->ppsLoopables), &(psGlobals->u8LoopablesCount));
        if (ePincfgResult != PINCFG_OK_E)
            return ePincfgResult;

        ePincfgResult = PinCfgCsv_eLinkedListToArray(
            (LINKEDLIST_ITEM_T **)&(psGlobals->ppsPresentables), &(psGlobals->u8PresentablesCount));
        if (ePincfgResult != PINCFG_OK_E)
            return ePincfgResult;
    }

    Memory_vTempFree();

    return ePincfgResult;
}

PINCFG_RESULT_T PinCfgCsv_eParse(PINCFG_PARSE_PARAMS_T *psParams)
{
    PINCFG_PARSE_SUBFN_PARAMS_T sPrms = {
        .psParsePrms = psParams,
        .pcOutStringLast = 0,
        .u16LinesProcessed = 0,
        .u8LineItemsLen = 0,
        .u8PresentablesCount = 0,
        .szNumberOfWarnings = 0};

    uint16_t u16LinesLen;
    PINCFG_RESULT_T eResult = PINCFG_ERROR_E;

    if (sPrms.psParsePrms->pszMemoryRequired != NULL)
        *(sPrms.psParsePrms->pszMemoryRequired) = 0;

    if (sPrms.psParsePrms->pcOutString != NULL && sPrms.psParsePrms->u16OutStrMaxLen > 0)
        sPrms.psParsePrms->pcOutString[0] = '\0';

    eResult = PinCfgCsv_CreateCli(&sPrms);
    if (eResult != PINCFG_OK_E)
        return eResult;

    if (psParams->pcConfig == NULL)
    {
        sPrms.pcOutStringLast += snprintf(
            (char *)(sPrms.psParsePrms->pcOutString + sPrms.pcOutStringLast),
            szGetSize(sPrms.psParsePrms->u16OutStrMaxLen, sPrms.pcOutStringLast),
            "E:Invalid format. NULL configuration.\n");
        return PINCFG_NULLPTR_ERROR_E;
    }

    PinCfgStr_vInitStrPoint(&(sPrms.sTempStrPt), psParams->pcConfig, strlen(psParams->pcConfig));
    u16LinesLen = (uint8_t)PinCfgStr_szGetSplitCount(&(sPrms.sTempStrPt), PINCFG_LINE_SEPARATOR_D);
    if (u16LinesLen == 1 && sPrms.sTempStrPt.szLen == 0)
    {
        sPrms.pcOutStringLast += snprintf(
            (char *)(sPrms.psParsePrms->pcOutString + sPrms.pcOutStringLast),
            szGetSize(sPrms.psParsePrms->u16OutStrMaxLen, sPrms.pcOutStringLast),
            "E:Invalid format. Empty configuration.\n");
        return PINCFG_ERROR_E;
    }

    for (sPrms.u16LinesProcessed = 0; sPrms.u16LinesProcessed < u16LinesLen; sPrms.u16LinesProcessed++)
    {
        PinCfgStr_vInitStrPoint(&(sPrms.sLine), psParams->pcConfig, strlen(psParams->pcConfig));
        PinCfgStr_vGetSplitElemByIndex(&(sPrms.sLine), PINCFG_LINE_SEPARATOR_D, sPrms.u16LinesProcessed);
        if (sPrms.sLine.pcStrStart[0] == '#') // comment continue
            continue;

        sPrms.u8LineItemsLen = (uint8_t)PinCfgStr_szGetSplitCount(&(sPrms.sLine), PINCFG_VALUE_SEPARATOR_D);
        if (sPrms.u8LineItemsLen < 2)
        {
            sPrms.pcOutStringLast += snprintf(
                (char *)(sPrms.psParsePrms->pcOutString + sPrms.pcOutStringLast),
                szGetSize(sPrms.psParsePrms->u16OutStrMaxLen, sPrms.pcOutStringLast),
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
        // temperature
        else if (sPrms.sTempStrPt.szLen == 2 && sPrms.sTempStrPt.pcStrStart[0] == 'T')
        {
            eResult = PinCfgCsv_ParseTemperatureSensors(&sPrms);
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
                (char *)(sPrms.psParsePrms->pcOutString + sPrms.pcOutStringLast),
                szGetSize(sPrms.psParsePrms->u16OutStrMaxLen, sPrms.pcOutStringLast),
                _s[FSDS_E],
                _s[EL_E],
                sPrms.u16LinesProcessed,
                "Unknown type.");
            sPrms.szNumberOfWarnings++;
        }
    }
    sPrms.pcOutStringLast += snprintf(
        (char *)(sPrms.psParsePrms->pcOutString + sPrms.pcOutStringLast),
        szGetSize(sPrms.psParsePrms->u16OutStrMaxLen, sPrms.pcOutStringLast),
        "I: Configuration parsed.\n");

    if (sPrms.szNumberOfWarnings > 0)
        return PINCFG_WARNINGS_E;

    return PINCFG_OK_E;
}

// private
static inline PINCFG_RESULT_T PinCfgCsv_CreateCli(PINCFG_PARSE_SUBFN_PARAMS_T *psPrms)
{
    if (psPrms == NULL)
        return PINCFG_NULLPTR_ERROR_E;

    if (!psPrms->psParsePrms->bValidate)
    {
        CLI_T *psCfgRcvrHnd = (CLI_T *)Memory_vpAlloc(sizeof(CLI_T));
        if (psCfgRcvrHnd == NULL)
        {
            Memory_eReset();
            psPrms->pcOutStringLast += snprintf(
                (char *)(psPrms->psParsePrms->pcOutString + psPrms->pcOutStringLast),
                szGetSize(psPrms->psParsePrms->u16OutStrMaxLen, psPrms->pcOutStringLast),
                _s[FSSS_E],
                _s[E_E],
                _s[ECR_E],
                _s[OOM_E]);

            return PINCFG_OUTOFMEMORY_ERROR_E;
        }

        if (Cli_eInit(psCfgRcvrHnd, psPrms->u8PresentablesCount) == CLI_OK_E)
        {
            PinCfgCsv_eAddToTempPresentables((PRESENTABLE_T *)psCfgRcvrHnd);
            psPrms->u8PresentablesCount++;
        }
        else
        {
            psPrms->pcOutStringLast += snprintf(
                (char *)(psPrms->psParsePrms->pcOutString + psPrms->pcOutStringLast),
                szGetSize(psPrms->psParsePrms->u16OutStrMaxLen, psPrms->pcOutStringLast),
                _s[FSSS_E],
                _s[W_E],
                _s[ECR_E],
                _s[INITF_E]);
            psPrms->szNumberOfWarnings++;
        }
    }
    if (psPrms->psParsePrms->pszMemoryRequired != NULL)
        *(psPrms->psParsePrms->pszMemoryRequired) += szGetAllocatedSize(sizeof(CLI_T));

    return PINCFG_OK_E;
}

static inline PINCFG_RESULT_T PinCfgCsv_ParseSwitch(PINCFG_PARSE_SUBFN_PARAMS_T *psPrms)
{
    uint8_t u8Pin, u8FbPin, u8Count, u8Offset, u8SwItems, i;
    uint32_t u32TimedPeriodMs = 0U;
    SWITCH_MODE_T eMode = SWITCH_CLASSIC_E;
    bool bIsDefinitionValid = true;

    u8SwItems = 2; // switch name and pin
    u8FbPin = 0U;

    if (psPrms == NULL)
        return PINCFG_NULLPTR_ERROR_E;

    if (psPrms->u8LineItemsLen < 3)
    {
        psPrms->pcOutStringLast += snprintf(
            (char *)(psPrms->psParsePrms->pcOutString + psPrms->pcOutStringLast),
            szGetSize(psPrms->psParsePrms->u16OutStrMaxLen, psPrms->pcOutStringLast),
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
        else if (psPrms->sTempStrPt.pcStrStart[1] == 'T')
        {
            eMode = SWITCH_TIMED_E;
            u8SwItems = 3; // switch name, pin and delay
        }
        else if ((psPrms->sTempStrPt.pcStrStart[1] == 'F'))
            u8SwItems = 3; // switch name, pin and feedback pin
        else
            bIsDefinitionValid = false;

        if (psPrms->sTempStrPt.szLen >= 3)
        {
            if (psPrms->sTempStrPt.pcStrStart[2] == 'F')
                u8SwItems++;
            else
                bIsDefinitionValid = false;
        }

        if (!bIsDefinitionValid)
        {
            psPrms->pcOutStringLast += snprintf(
                (char *)(psPrms->psParsePrms->pcOutString + psPrms->pcOutStringLast),
                szGetSize(psPrms->psParsePrms->u16OutStrMaxLen, psPrms->pcOutStringLast),
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
            (char *)(psPrms->psParsePrms->pcOutString + psPrms->pcOutStringLast),
            szGetSize(psPrms->psParsePrms->u16OutStrMaxLen, psPrms->pcOutStringLast),
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
                (char *)(psPrms->psParsePrms->pcOutString + psPrms->pcOutStringLast),
                szGetSize(psPrms->psParsePrms->u16OutStrMaxLen, psPrms->pcOutStringLast),
                _s[FSDSS_E],
                _s[WL_E],
                psPrms->u16LinesProcessed,
                _s[SW_E],
                _s[IPN_E]);
            psPrms->szNumberOfWarnings++;
            continue;
        }

        // feedback pin
        if ((eMode != SWITCH_TIMED_E && u8SwItems == 3) || (eMode == SWITCH_TIMED_E && u8SwItems == 4))
        {
            uint8_t u8FbPinOffset = 2;
            if (u8SwItems == 4)
                u8FbPinOffset++;

            psPrms->sTempStrPt = psPrms->sLine;
            PinCfgStr_vGetSplitElemByIndex(&(psPrms->sTempStrPt), PINCFG_VALUE_SEPARATOR_D, (u8Offset + u8FbPinOffset));
            if (PinCfgStr_eAtoU8(&(psPrms->sTempStrPt), &u8FbPin) != PINCFG_STR_OK_E)
            {
                psPrms->pcOutStringLast += snprintf(
                    (char *)(psPrms->psParsePrms->pcOutString + psPrms->pcOutStringLast),
                    szGetSize(psPrms->psParsePrms->u16OutStrMaxLen, psPrms->pcOutStringLast),
                    _s[FSDSS_E],
                    _s[WL_E],
                    psPrms->u16LinesProcessed,
                    _s[SW_E],
                    _s[IPN_E]);
                psPrms->szNumberOfWarnings++;
                continue;
            }
        }

        // time period
        if (eMode == SWITCH_TIMED_E)
        {
            psPrms->sTempStrPt = psPrms->sLine;
            PinCfgStr_vGetSplitElemByIndex(&(psPrms->sTempStrPt), PINCFG_VALUE_SEPARATOR_D, (u8Offset + 2));
            if (PinCfgStr_eAtoU32(&(psPrms->sTempStrPt), &u32TimedPeriodMs) != PINCFG_STR_OK_E ||
                u32TimedPeriodMs < PINCFG_TIMED_SWITCH_MIN_PERIOD_MS_D ||
                u32TimedPeriodMs > PINCFG_TIMED_SWITCH_MAX_PERIOD_MS_D)
            {
                psPrms->pcOutStringLast += snprintf(
                    (char *)(psPrms->psParsePrms->pcOutString + psPrms->pcOutStringLast),
                    szGetSize(psPrms->psParsePrms->u16OutStrMaxLen, psPrms->pcOutStringLast),
                    _s[FSDSS_E],
                    _s[WL_E],
                    psPrms->u16LinesProcessed,
                    _s[SW_E],
                    "Invalid time period.");
                psPrms->szNumberOfWarnings++;
                continue;
            }
        }

        psPrms->sTempStrPt = psPrms->sLine;
        PinCfgStr_vGetSplitElemByIndex(&(psPrms->sTempStrPt), PINCFG_VALUE_SEPARATOR_D, u8Offset);

        SWITCH_T *psSwitchHnd = NULL;
        bool bInitOk = true;
        if (!psPrms->psParsePrms->bValidate)
        {
            psSwitchHnd = (SWITCH_T *)Memory_vpAlloc(sizeof(SWITCH_T));
            bInitOk = (psSwitchHnd != NULL);
            if (!bInitOk)
            {
                Memory_eReset();

                psPrms->pcOutStringLast += snprintf(
                    (char *)(psPrms->psParsePrms->pcOutString + psPrms->pcOutStringLast),
                    szGetSize(psPrms->psParsePrms->u16OutStrMaxLen, psPrms->pcOutStringLast),
                    _s[FSDSS_E],
                    _s[EL_E],
                    psPrms->u16LinesProcessed,
                    _s[SW_E],
                    _s[OOM_E]);

                return PINCFG_OUTOFMEMORY_ERROR_E;
            }

            bInitOk =
                (Switch_eInit(
                     psSwitchHnd,
                     &(psPrms->sTempStrPt),
                     psPrms->u8PresentablesCount,
                     eMode,
                     u8Pin,
                     u8FbPin,
                     u32TimedPeriodMs) == SWITCH_OK_E);
        }

        if (bInitOk && psPrms->psParsePrms->eAddToPresentables != NULL)
        {
            if (psPrms->psParsePrms->eAddToPresentables((PRESENTABLE_T *)psSwitchHnd) == PINCFG_OK_E)
            {
                bInitOk = true;
                psPrms->u8PresentablesCount++;
            }
        }
        if (bInitOk && psPrms->psParsePrms->eAddToLoopables != NULL)
        {
            bInitOk = (psPrms->psParsePrms->eAddToLoopables(&(psSwitchHnd->sLoopable)) == PINCFG_OK_E);
        }
        if (!bInitOk)
        {
            psPrms->pcOutStringLast += snprintf(
                (char *)(psPrms->psParsePrms->pcOutString + psPrms->pcOutStringLast),
                szGetSize(psPrms->psParsePrms->u16OutStrMaxLen, psPrms->pcOutStringLast),
                _s[FSDSS_E],
                _s[WL_E],
                psPrms->u16LinesProcessed,
                _s[SW_E],
                _s[INITF_E]);
            psPrms->szNumberOfWarnings++;
        }

        if (psPrms->psParsePrms->pszMemoryRequired != NULL)
            *(psPrms->psParsePrms->pszMemoryRequired) +=
                szGetAllocatedSize(sizeof(SWITCH_T)) + szGetAllocatedSize(psPrms->sTempStrPt.szLen + 1);
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
            (char *)(psPrms->psParsePrms->pcOutString + psPrms->pcOutStringLast),
            szGetSize(psPrms->psParsePrms->u16OutStrMaxLen, psPrms->pcOutStringLast),
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
            (char *)(psPrms->psParsePrms->pcOutString + psPrms->pcOutStringLast),
            szGetSize(psPrms->psParsePrms->u16OutStrMaxLen, psPrms->pcOutStringLast),
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
                (char *)(psPrms->psParsePrms->pcOutString + psPrms->pcOutStringLast),
                szGetSize(psPrms->psParsePrms->u16OutStrMaxLen, psPrms->pcOutStringLast),
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

        INPIN_T *psInPinHnd = NULL;
        bool bInitOk = true;
        if (!psPrms->psParsePrms->bValidate)
        {
            psInPinHnd = (INPIN_T *)Memory_vpAlloc(sizeof(INPIN_T));
            bInitOk = (psInPinHnd != NULL);
            if (!bInitOk)
            {
                Memory_eReset();
                psPrms->pcOutStringLast += snprintf(
                    (char *)(psPrms->psParsePrms->pcOutString + psPrms->pcOutStringLast),
                    szGetSize(psPrms->psParsePrms->u16OutStrMaxLen, psPrms->pcOutStringLast),
                    _s[FSDSS_E],
                    _s[EL_E],
                    psPrms->u16LinesProcessed,
                    _s[IP_E],
                    _s[OOM_E]);

                return PINCFG_OUTOFMEMORY_ERROR_E;
            }

            bInitOk =
                (InPin_eInit(psInPinHnd, &(psPrms->sTempStrPt), psPrms->u8PresentablesCount, u8Pin) == INPIN_OK_E);
        }

        if (bInitOk && psPrms->psParsePrms->eAddToPresentables != NULL)
        {
            if (psPrms->psParsePrms->eAddToPresentables((PRESENTABLE_T *)psInPinHnd) == PINCFG_OK_E)
            {
                bInitOk = true;
                psPrms->u8PresentablesCount++;
            }
        }
        if (bInitOk && psPrms->psParsePrms->eAddToLoopables != NULL)
        {
            bInitOk = (psPrms->psParsePrms->eAddToLoopables(&(psInPinHnd->sLoopable)) == PINCFG_OK_E);
        }
        if (!bInitOk)
        {
            psPrms->pcOutStringLast += snprintf(
                (char *)(psPrms->psParsePrms->pcOutString + psPrms->pcOutStringLast),
                szGetSize(psPrms->psParsePrms->u16OutStrMaxLen, psPrms->pcOutStringLast),
                _s[FSDSS_E],
                _s[WL_E],
                psPrms->u16LinesProcessed,
                _s[IP_E],
                _s[INITF_E]);
            psPrms->szNumberOfWarnings++;
        }

        if (psPrms->psParsePrms->pszMemoryRequired != NULL)
            *(psPrms->psParsePrms->pszMemoryRequired) +=
                szGetAllocatedSize(sizeof(INPIN_T)) + szGetAllocatedSize(psPrms->sTempStrPt.szLen + 1);
    }

    return PINCFG_OK_E;
}

static inline PINCFG_RESULT_T PinCfgCsv_ParseTriggers(PINCFG_PARSE_SUBFN_PARAMS_T *psPrms)
{
    uint8_t u8Count, u8EventType, u8EventCount, u8DrivesCountReal, u8Offset, u8DrivenAction, i;
    INPIN_T *psInPinEmiterHnd;
    SWITCH_T *psSwitchHnd;
    TRIGGER_SWITCHACTION_T *pasSwActs;
    TRIGGER_SWITCHACTION_T *psSwAct;

    if (psPrms == NULL)
        return PINCFG_NULLPTR_ERROR_E;

    if (psPrms->u8LineItemsLen < 7)
    {
        psPrms->pcOutStringLast += snprintf(
            (char *)(psPrms->psParsePrms->pcOutString + psPrms->pcOutStringLast),
            szGetSize(psPrms->psParsePrms->u16OutStrMaxLen, psPrms->pcOutStringLast),
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

    if (psPrms->psParsePrms->bValidate)
    {
        if (PinCfgCsv_pcStrstrpt(psPrms->psParsePrms->pcConfig, &(psPrms->sTempStrPt)) == NULL)
        {
            psPrms->pcOutStringLast += snprintf(
                (char *)(psPrms->psParsePrms->pcOutString + psPrms->pcOutStringLast),
                szGetSize(psPrms->psParsePrms->u16OutStrMaxLen, psPrms->pcOutStringLast),
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
        psInPinEmiterHnd = (INPIN_T *)PinCfgCsv_psFindInTempPresentablesByName(&(psPrms->sTempStrPt));
        if (psInPinEmiterHnd == NULL)
        {
            psPrms->pcOutStringLast += snprintf(
                (char *)(psPrms->psParsePrms->pcOutString + psPrms->pcOutStringLast),
                szGetSize(psPrms->psParsePrms->u16OutStrMaxLen, psPrms->pcOutStringLast),
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
    if (PinCfgStr_eAtoU8(&(psPrms->sTempStrPt), &u8EventType) != PINCFG_STR_OK_E ||
        u8EventType > (uint8_t)TRIGGER_ALL_E)
    {
        psPrms->pcOutStringLast += snprintf(
            (char *)(psPrms->psParsePrms->pcOutString + psPrms->pcOutStringLast),
            szGetSize(psPrms->psParsePrms->u16OutStrMaxLen, psPrms->pcOutStringLast),
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
            (char *)(psPrms->psParsePrms->pcOutString + psPrms->pcOutStringLast),
            szGetSize(psPrms->psParsePrms->u16OutStrMaxLen, psPrms->pcOutStringLast),
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
        if (psPrms->psParsePrms->bValidate)
        {
            if (PinCfgCsv_pcStrstrpt(psPrms->psParsePrms->pcConfig, &(psPrms->sTempStrPt)) == NULL)
            {
                psPrms->pcOutStringLast += snprintf(
                    (char *)(psPrms->psParsePrms->pcOutString + psPrms->pcOutStringLast),
                    szGetSize(psPrms->psParsePrms->u16OutStrMaxLen, psPrms->pcOutStringLast),
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
            psSwitchHnd = (SWITCH_T *)PinCfgCsv_psFindInTempPresentablesByName(&(psPrms->sTempStrPt));
            if (psSwitchHnd == NULL)
            {
                psPrms->pcOutStringLast += snprintf(
                    (char *)(psPrms->psParsePrms->pcOutString + psPrms->pcOutStringLast),
                    szGetSize(psPrms->psParsePrms->u16OutStrMaxLen, psPrms->pcOutStringLast),
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
        if (PinCfgStr_eAtoU8(&(psPrms->sTempStrPt), &u8DrivenAction) != PINCFG_STR_OK_E ||
            u8DrivenAction > (uint8_t)TRIGGER_A_FORWARD_E)
        {
            psPrms->pcOutStringLast += snprintf(
                (char *)(psPrms->psParsePrms->pcOutString + psPrms->pcOutStringLast),
                szGetSize(psPrms->psParsePrms->u16OutStrMaxLen, psPrms->pcOutStringLast),
                _s[FSDSS_E],
                _s[WL_E],
                psPrms->u16LinesProcessed,
                _s[TRG_E],
                "Invalid switch action.");
            psPrms->szNumberOfWarnings++;
            continue;
        }
        u8DrivesCountReal++;

        if (!psPrms->psParsePrms->bValidate)
        {
            psSwAct = (TRIGGER_SWITCHACTION_T *)Memory_vpAlloc(sizeof(TRIGGER_SWITCHACTION_T));
            if (psSwAct == NULL)
            {
                Memory_eReset();
                psPrms->pcOutStringLast += snprintf(
                    (char *)(psPrms->psParsePrms->pcOutString + psPrms->pcOutStringLast),
                    szGetSize(psPrms->psParsePrms->u16OutStrMaxLen, psPrms->pcOutStringLast),
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

        if (psPrms->psParsePrms->pszMemoryRequired != NULL)
            *(psPrms->psParsePrms->pszMemoryRequired) += szGetAllocatedSize(sizeof(TRIGGER_SWITCHACTION_T));
    }

    if (!psPrms->psParsePrms->bValidate)
    {
        if ((u8DrivesCountReal == 0U || pasSwActs == NULL))
        {
            psPrms->pcOutStringLast += snprintf(
                (char *)(psPrms->psParsePrms->pcOutString + psPrms->pcOutStringLast),
                szGetSize(psPrms->psParsePrms->u16OutStrMaxLen, psPrms->pcOutStringLast),
                _s[FSDSS_E],
                _s[WL_E],
                psPrms->u16LinesProcessed,
                _s[TRG_E],
                "Nothing to drive.");
            psPrms->szNumberOfWarnings++;

            return PINCFG_OK_E;
        }

        TRIGGER_T *psTriggerHnd = (TRIGGER_T *)Memory_vpAlloc(sizeof(TRIGGER_T));
        if (psTriggerHnd == NULL)
        {
            Memory_eReset();
            psPrms->pcOutStringLast += snprintf(
                (char *)(psPrms->psParsePrms->pcOutString + psPrms->pcOutStringLast),
                szGetSize(psPrms->psParsePrms->u16OutStrMaxLen, psPrms->pcOutStringLast),
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
                (char *)(psPrms->psParsePrms->pcOutString + psPrms->pcOutStringLast),
                szGetSize(psPrms->psParsePrms->u16OutStrMaxLen, psPrms->pcOutStringLast),
                _s[FSDSS_E],
                _s[WL_E],
                psPrms->u16LinesProcessed,
                _s[TRG_E],
                _s[INITF_E]);
            psPrms->szNumberOfWarnings++;
        }
    }

    if (psPrms->psParsePrms->pszMemoryRequired != NULL)
        *(psPrms->psParsePrms->pszMemoryRequired) += szGetAllocatedSize(sizeof(TRIGGER_T));

    return PINCFG_OK_E;
}

static inline PINCFG_RESULT_T PinCfgCsv_ParseTemperatureSensors(PINCFG_PARSE_SUBFN_PARAMS_T *psPrms)
{
    switch (psPrms->sTempStrPt.pcStrStart[1])
    {
    case 'I': return PinCfgCsv_ParseCPUTemperatureSensor(psPrms); break;
    default:
    {
        psPrms->pcOutStringLast += snprintf(
            (char *)(psPrms->psParsePrms->pcOutString + psPrms->pcOutStringLast),
            szGetSize(psPrms->psParsePrms->u16OutStrMaxLen, psPrms->pcOutStringLast),
            _s[FSDS_E],
            _s[WL_E],
            psPrms->u16LinesProcessed,
            " Unknown type of temperature sensor.");
        psPrms->szNumberOfWarnings++;

        return PINCFG_OK_E;
    }
    break;
    }

    return PINCFG_OK_E;
}

static inline PINCFG_RESULT_T PinCfgCsv_ParseCPUTemperatureSensor(PINCFG_PARSE_SUBFN_PARAMS_T *psPrms)
{
    if (psPrms->u8LineItemsLen < 2)
    {
        psPrms->pcOutStringLast += snprintf(
            (char *)(psPrms->psParsePrms->pcOutString + psPrms->pcOutStringLast),
            szGetSize(psPrms->psParsePrms->u16OutStrMaxLen, psPrms->pcOutStringLast),
            _s[FSDSSS_E],
            _s[WL_E],
            psPrms->u16LinesProcessed,
            _s[CPUTMP_E],
            _s[IN_E],
            _s[OARGS_E]);
        psPrms->szNumberOfWarnings++;

        return PINCFG_OK_E;
    }

    uint32_t u32ReportInterval = PINCFG_CPUTEMP_REPORTING_INTV_MS_D;
    if (psPrms->u8LineItemsLen >= 3)
    {
        psPrms->sTempStrPt = psPrms->sLine;
        PinCfgStr_vGetSplitElemByIndex(&(psPrms->sTempStrPt), PINCFG_VALUE_SEPARATOR_D, 2);
        if (PinCfgStr_eAtoU32(&(psPrms->sTempStrPt), &u32ReportInterval) != PINCFG_STR_OK_E)
        {
            psPrms->pcOutStringLast += snprintf(
                (char *)(psPrms->psParsePrms->pcOutString + psPrms->pcOutStringLast),
                szGetSize(psPrms->psParsePrms->u16OutStrMaxLen, psPrms->pcOutStringLast),
                _s[FSDSSS_E],
                _s[WL_E],
                psPrms->u16LinesProcessed,
                _s[CPUTMP_E],
                _s[IN_E],
                ".");
            psPrms->szNumberOfWarnings++;

            return PINCFG_OK_E;
        }
    }

    float fOffset = PINCFG_CPUTEMP_OFFSET_D;
    if (psPrms->u8LineItemsLen >= 4)
    {
        psPrms->sTempStrPt = psPrms->sLine;
        PinCfgStr_vGetSplitElemByIndex(&(psPrms->sTempStrPt), PINCFG_VALUE_SEPARATOR_D, 3);
        if (PinCfgStr_eAtoFloat(&(psPrms->sTempStrPt), &fOffset) != PINCFG_STR_OK_E)
        {
            psPrms->pcOutStringLast += snprintf(
                (char *)(psPrms->psParsePrms->pcOutString + psPrms->pcOutStringLast),
                szGetSize(psPrms->psParsePrms->u16OutStrMaxLen, psPrms->pcOutStringLast),
                _s[FSDSSS_E],
                _s[WL_E],
                psPrms->u16LinesProcessed,
                _s[CPUTMP_E],
                _s[IN_E],
                ".");
            psPrms->szNumberOfWarnings++;

            return PINCFG_OK_E;
        }
    }

    psPrms->sTempStrPt = psPrms->sLine;
    PinCfgStr_vGetSplitElemByIndex(&(psPrms->sTempStrPt), PINCFG_VALUE_SEPARATOR_D, 1);

    CPUTEMP_T *psTempHnd = NULL;
    bool bInitOk = true;
    if (!psPrms->psParsePrms->bValidate)
    {
        psTempHnd = (CPUTEMP_T *)Memory_vpAlloc(sizeof(CPUTEMP_T));
        bInitOk = (psTempHnd != NULL);
        if (!bInitOk)
        {
            Memory_eReset();
            psPrms->pcOutStringLast += snprintf(
                (char *)(psPrms->psParsePrms->pcOutString + psPrms->pcOutStringLast),
                szGetSize(psPrms->psParsePrms->u16OutStrMaxLen, psPrms->pcOutStringLast),
                _s[FSDSS_E],
                _s[EL_E],
                psPrms->u16LinesProcessed,
                _s[CPUTMP_E],
                _s[OOM_E]);

            return PINCFG_OUTOFMEMORY_ERROR_E;
        }

        bInitOk =
            (CPUTemp_eInit(psTempHnd, &(psPrms->sTempStrPt), psPrms->u8PresentablesCount, u32ReportInterval, fOffset) ==
             CPUTEMP_OK_E);
    }

    if (bInitOk && psPrms->psParsePrms->eAddToPresentables != NULL)
    {
        if (psPrms->psParsePrms->eAddToPresentables((PRESENTABLE_T *)psTempHnd) == PINCFG_OK_E)
        {
            bInitOk = true;
            psPrms->u8PresentablesCount++;
        }
    }
    if (bInitOk && psPrms->psParsePrms->eAddToLoopables != NULL)
    {
        bInitOk = (psPrms->psParsePrms->eAddToLoopables(&(psTempHnd->sLoopable)) == PINCFG_OK_E);
    }
    if (!bInitOk)
    {
        psPrms->pcOutStringLast += snprintf(
            (char *)(psPrms->psParsePrms->pcOutString + psPrms->pcOutStringLast),
            szGetSize(psPrms->psParsePrms->u16OutStrMaxLen, psPrms->pcOutStringLast),
            _s[FSDSS_E],
            _s[WL_E],
            psPrms->u16LinesProcessed,
            _s[CPUTMP_E],
            _s[INITF_E]);
        psPrms->szNumberOfWarnings++;
    }

    if (psPrms->psParsePrms->pszMemoryRequired != NULL)
        *(psPrms->psParsePrms->pszMemoryRequired) +=
            szGetAllocatedSize(sizeof(CPUTEMP_T)) + szGetAllocatedSize(psPrms->sTempStrPt.szLen + 1);

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
    default:
    {
        psPrms->pcOutStringLast += snprintf(
            (char *)(psPrms->psParsePrms->pcOutString + psPrms->pcOutStringLast),
            szGetSize(psPrms->psParsePrms->u16OutStrMaxLen, psPrms->pcOutStringLast),
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
            (char *)(psPrms->psParsePrms->pcOutString + psPrms->pcOutStringLast),
            szGetSize(psPrms->psParsePrms->u16OutStrMaxLen, psPrms->pcOutStringLast),
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
            (char *)(psPrms->psParsePrms->pcOutString + psPrms->pcOutStringLast),
            szGetSize(psPrms->psParsePrms->u16OutStrMaxLen, psPrms->pcOutStringLast),
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
    default: break;
    }

    return PINCFG_OK_E;
}

static inline PRESENTABLE_T *PinCfgCsv_psFindInPresentablesById(uint8_t u8Id)
{
    // for (uint8_t i = 0; i < psGlobals->u8PresentablesCount; i++)
    // {
    //     if (psGlobals->ppsPresentables[i]->u8Id == u8Id)
    //         return psGlobals->ppsPresentables[i];
    // }

    // TODO: check this
    if (u8Id >= psGlobals->u8PresentablesCount)
        return NULL;

    return (psGlobals->ppsPresentables)[u8Id];
}

static PRESENTABLE_T *PinCfgCsv_psFindInTempPresentablesByName(const STRING_POINT_T *psName)
{
    PRESENTABLE_T *psReturn = NULL;

    LINKEDLIST_ITEM_T *psLLItem = (LINKEDLIST_ITEM_T *)psGlobals->ppsPresentables;
    while (psLLItem != NULL)
    {
        psReturn = (PRESENTABLE_T *)LinkedList_pvGetStoredItem(psLLItem);
        size_t szPcHeyNameLength = strlen(psReturn->pcName);
        if (szPcHeyNameLength == psName->szLen && memcmp(psName->pcStrStart, psReturn->pcName, szPcHeyNameLength) == 0)
            break;

        psLLItem = LinkedList_psGetNext((void *)psLLItem);
        psReturn = NULL;
    }

    return psReturn;
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

static inline PINCFG_RESULT_T PinCfgCsv_eAddToLinkedList(LINKEDLIST_ITEM_T **psFirst, void *pvNew)
{
    LINKEDLIST_RESULT_T eAddRes = LinkedList_eAddToLinkedList(psFirst, pvNew);

    if (eAddRes == LINKEDLIST_OUTOFMEMORY_ERROR_E)
        return PINCFG_OUTOFMEMORY_ERROR_E;

    if (eAddRes == LINKEDLIST_NULLPTR_ERROR_E)
        return PINCFG_NULLPTR_ERROR_E;

    return PINCFG_OK_E;
}

PINCFG_RESULT_T PinCfgCsv_eAddToTempLoopables(LOOPABLE_T *psLoopable)
{
    return PinCfgCsv_eAddToLinkedList((LINKEDLIST_ITEM_T **)&(psGlobals->ppsLoopables), (void *)psLoopable);
}

PINCFG_RESULT_T PinCfgCsv_eAddToTempPresentables(PRESENTABLE_T *psPresentable)
{
    return PinCfgCsv_eAddToLinkedList((LINKEDLIST_ITEM_T **)&(psGlobals->ppsPresentables), (void *)psPresentable);
}

PINCFG_RESULT_T PinCfgCsv_eLinkedListToArray(LINKEDLIST_ITEM_T **ppsFirst, uint8_t *u8Count)
{
    void *pvNewFirst = NULL;
    void *pvItem = LinkedList_pvPopFront(ppsFirst);
    while (pvItem != NULL)
    {
        void **pvArrayItem = (void **)Memory_vpAlloc(sizeof(void **));
        if (pvArrayItem == NULL)
            return PINCFG_OUTOFMEMORY_ERROR_E;

        if (pvNewFirst == NULL)
            pvNewFirst = pvArrayItem;

        *pvArrayItem = pvItem;
        (*u8Count)++;
        pvItem = LinkedList_pvPopFront(ppsFirst);
    }
    *ppsFirst = pvNewFirst;

    return PINCFG_OK_E;
}
