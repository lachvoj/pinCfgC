#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include "Cli.h"
#include "CPUTempMeasure.h"
#include "Globals.h"
#include "InPin.h"
#include "Memory.h"
#include "MySensorsWrapper.h"
#include "PersistentConfigiration.h"
#include "PinCfgCsv.h"
#include "PinCfgStr.h"
#include "Sensor.h"
#include "Switch.h"
#include "Trigger.h"

#ifdef FEATURE_I2C_MEASUREMENT
#include "I2CMeasure.h"
#endif

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
    "%s%d:%s%s (0-%d)\n",                 // FSDSSD_E - for type enum errors
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
    "OOM",                                // OOM_E
    "init failed",                        // INITF_E
    "Invalid pin number.",                // IPN_E
    "invalid",                            // IVLD_E
    "Invalid number",                     // IN_E
    " of arguments.",                     // OARGS_E
    " of items defining names and pins.", // OITMS_E
    "Switch name not found.",             // SNNF_E
    "MS",                                 // MS_E
    "SR",                                 // SR_E
    " type enum",                         // TE_E
    " args\n",                            // ARGS_E
    " (name)\n",                          // NAME_E
    " (cputemp)\n",                       // CPUTEMP_E
    " type not implemented\n",            // TNI_E
    " (sensor)\n",                        // SENSOR_E
    " measurement not found: ",           // MNF_E
    "\n",                                 // NL_E
    " offset\n",                          // OFFSET_E
    " enableable\n",                      // ENABLEABLE_E
    " cumulative\n",                      // CUMULATIVE_E
    " sampling interval\n",               // SAMPINT_E
    " report interval\n"                  // REPINT_E
};

typedef enum PINCFG_PARSE_STRINGS_E
{
    FSS_E = 0,
    FSSS_E,
    FSDS_E,
    FSDSS_E,
    FSDSSS_E,
    FSDSSD_E,
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
    IVLD_E,
    IN_E,
    OARGS_E,
    OITMS_E,
    SNNF_E,
    MS_E,
    SR_E,
    TE_E,
    ARGS_E,
    NAME_E,
    CPUTEMP_E,
    TNI_E,
    SENSOR_E,
    MNF_E,
    NL_E,
    OFFSET_E,
    ENABLEABLE_E,
    CUMULATIVE_E,
    SAMPINT_E,
    REPINT_E
} PINCFG_PARSE_STRINGS_T;

static inline PINCFG_RESULT_T PinCfgCsv_CreateCli(PINCFG_PARSE_SUBFN_PARAMS_T *psPrms);
static inline PINCFG_RESULT_T PinCfgCsv_ParseSwitch(PINCFG_PARSE_SUBFN_PARAMS_T *psPrms);
static inline PINCFG_RESULT_T PinCfgCsv_ParseInpins(PINCFG_PARSE_SUBFN_PARAMS_T *psPrms);
static inline PINCFG_RESULT_T PinCfgCsv_ParseTriggers(PINCFG_PARSE_SUBFN_PARAMS_T *psPrms);

// Phase 2: Measurement source and sensor reporter parsers
static inline PINCFG_RESULT_T PinCfgCsv_ParseMeasurementSource(PINCFG_PARSE_SUBFN_PARAMS_T *psPrms);
static inline PINCFG_RESULT_T PinCfgCsv_ParseSensorReporter(PINCFG_PARSE_SUBFN_PARAMS_T *psPrms);
static inline PINCFG_RESULT_T PinCfgCsv_eAddToLinkedList(LINKEDLIST_ITEM_T **psFirst, void *pvNew);

static inline PINCFG_RESULT_T PinCfgCsv_ParseGlobalConfigItems(PINCFG_PARSE_SUBFN_PARAMS_T *psPrms);

static inline PRESENTABLE_T *PinCfgCsv_psFindInPresentablesById(uint8_t u8Id);
static PRESENTABLE_T *PinCfgCsv_psFindInTempPresentablesByName(const STRING_POINT_T *psName);
static inline ISENSORMEASURE_T *PinCfgCsv_psFindMeasurementByName(const char *pcName, size_t szNameLen);
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
        // move loopables, presentables, and measurements from linked list to array
        LINKEDLIST_RESULT_T eLinkedListResult = LinkedList_eLinkedListToArray(
            (LINKEDLIST_ITEM_T **)&(psGlobals->ppsLoopables), &(psGlobals->u8LoopablesCount));
        if (eLinkedListResult != LINKEDLIST_OK_E)
            return PINCFG_OUTOFMEMORY_ERROR_E;

        eLinkedListResult = LinkedList_eLinkedListToArray(
            (LINKEDLIST_ITEM_T **)&(psGlobals->ppsPresentables), &(psGlobals->u8PresentablesCount));
        if (eLinkedListResult != LINKEDLIST_OK_E)
            return PINCFG_OUTOFMEMORY_ERROR_E;
        
        eLinkedListResult = LinkedList_eLinkedListToArray(
            (LINKEDLIST_ITEM_T **)&(psGlobals->ppsMeasurements), &(psGlobals->u8MeasurementsCount));
        if (eLinkedListResult != LINKEDLIST_OK_E)
            return PINCFG_OUTOFMEMORY_ERROR_E;
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
        *(sPrms.psParsePrms->pszMemoryRequired) = Memory_szGetAllocatedSize(sizeof(GLOBALS_T));

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
        
        // Phase 2: Check 2-character prefixes FIRST before 1-character ones
        // Measurement Source (MS)
        if (sPrms.sTempStrPt.szLen == 2 && 
            sPrms.sTempStrPt.pcStrStart[0] == 'M' &&
            sPrms.sTempStrPt.pcStrStart[1] == 'S')
        {
            eResult = PinCfgCsv_ParseMeasurementSource(&sPrms);
            if (eResult != PINCFG_OK_E)
                return eResult;
        }
        // Phase 2: Sensor Reporter (SR)
        else if (sPrms.sTempStrPt.szLen == 2 && 
                 sPrms.sTempStrPt.pcStrStart[0] == 'S' &&
                 sPrms.sTempStrPt.pcStrStart[1] == 'R')
        {
            eResult = PinCfgCsv_ParseSensorReporter(&sPrms);
            if (eResult != PINCFG_OK_E)
                return eResult;
        }
        // switches (check after SR to avoid conflict)
        else if (sPrms.sTempStrPt.szLen >= 1 && sPrms.sTempStrPt.pcStrStart[0] == 'S')
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

    if (psPrms->psParsePrms->pszMemoryRequired != NULL)
    {
        *(psPrms->psParsePrms->pszMemoryRequired) += Memory_szGetAllocatedSize(sizeof(CLI_T));
        if (psPrms->psParsePrms->eAddToPresentables != NULL)
            *(psPrms->psParsePrms->pszMemoryRequired) +=
                Memory_szGetAllocatedSize(sizeof(LINKEDLIST_ITEM_T) + sizeof(void *));
    }

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

        if (psPrms->psParsePrms->pszMemoryRequired != NULL)
        {
            *(psPrms->psParsePrms->pszMemoryRequired) +=
                Memory_szGetAllocatedSize(sizeof(SWITCH_T)) + Memory_szGetAllocatedSize(psPrms->sTempStrPt.szLen + 1);
            if (psPrms->psParsePrms->eAddToPresentables != NULL)
                *(psPrms->psParsePrms->pszMemoryRequired) +=
                    Memory_szGetAllocatedSize(sizeof(LINKEDLIST_ITEM_T) + sizeof(void *));
            if (psPrms->psParsePrms->eAddToLoopables != NULL)
                *(psPrms->psParsePrms->pszMemoryRequired) +=
                    Memory_szGetAllocatedSize(sizeof(LINKEDLIST_ITEM_T) + sizeof(void *));
        }

        if (psPrms->psParsePrms->bValidate)
            continue;

        SWITCH_T *psSwitchHnd = (SWITCH_T *)Memory_vpAlloc(sizeof(SWITCH_T));
        bool bInitOk = (psSwitchHnd != NULL);
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

        if (psPrms->psParsePrms->pszMemoryRequired != NULL)
        {
            *(psPrms->psParsePrms->pszMemoryRequired) +=
                Memory_szGetAllocatedSize(sizeof(INPIN_T)) + Memory_szGetAllocatedSize(psPrms->sTempStrPt.szLen + 1);
            if (psPrms->psParsePrms->eAddToPresentables != NULL)
                *(psPrms->psParsePrms->pszMemoryRequired) +=
                    Memory_szGetAllocatedSize(sizeof(LINKEDLIST_ITEM_T) + sizeof(void *));
            if (psPrms->psParsePrms->eAddToLoopables != NULL)
                *(psPrms->psParsePrms->pszMemoryRequired) +=
                    Memory_szGetAllocatedSize(sizeof(LINKEDLIST_ITEM_T) + sizeof(void *));
        }

        if (psPrms->psParsePrms->bValidate)
            continue;

        INPIN_T *psInPinHnd = (INPIN_T *)Memory_vpAlloc(sizeof(INPIN_T));
        bool bInitOk = (psInPinHnd != NULL);
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

        bInitOk = (InPin_eInit(psInPinHnd, &(psPrms->sTempStrPt), psPrms->u8PresentablesCount, u8Pin) == INPIN_OK_E);

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
    }

    return PINCFG_OK_E;
}

static inline PINCFG_RESULT_T PinCfgCsv_ParseTriggers(PINCFG_PARSE_SUBFN_PARAMS_T *psPrms)
{
    uint8_t u8Count, u8EventType, u8EventCount, u8DrivesCountReal, u8Offset, u8DrivenAction;
    INPIN_T *psInPinEmiterHnd;
    SWITCH_T *psSwitchHnd;

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

    if (psPrms->psParsePrms->pszMemoryRequired != NULL)
    {
        *(psPrms->psParsePrms->pszMemoryRequired) +=
            Memory_szGetAllocatedSize(sizeof(TRIGGER_SWITCHACTION_T) * u8Count) +
            Memory_szGetAllocatedSize(sizeof(TRIGGER_T));
    }

    if (psPrms->psParsePrms->bValidate)
    {
        for (uint8_t i = 0; i < u8Count; i++)
        {
            u8Offset = 5 + i * 2;
            psPrms->sTempStrPt = psPrms->sLine;
            PinCfgStr_vGetSplitElemByIndex(&(psPrms->sTempStrPt), PINCFG_VALUE_SEPARATOR_D, u8Offset);
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
        return PINCFG_OK_E;
    }

    // allocate memory for switch actions
    TRIGGER_SWITCHACTION_T *pasSwActs =
        (TRIGGER_SWITCHACTION_T *)Memory_vpAlloc(sizeof(TRIGGER_SWITCHACTION_T) * u8Count);
    if (pasSwActs == NULL)
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
    for (uint8_t i = 0; i < u8Count; i++)
    {
        u8Offset = 5 + i * 2;
        psPrms->sTempStrPt = psPrms->sLine;
        PinCfgStr_vGetSplitElemByIndex(&(psPrms->sTempStrPt), PINCFG_VALUE_SEPARATOR_D, u8Offset);
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

        pasSwActs[u8DrivesCountReal].psSwitchHnd = psSwitchHnd;
        pasSwActs[u8DrivesCountReal].eAction = (TRIGGER_ACTION_T)u8DrivenAction;

        u8DrivesCountReal++;
    }

    if (u8DrivesCountReal == 0U)
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

    return PINCFG_OK_E;
}

// Phase 2: Parse Measurement Source (MS)
// Format: MS,<type_enum>,<name>/
// Example: MS,0,temp0/  (0 = MEASUREMENT_TYPE_CPUTEMP_E)
static inline PINCFG_RESULT_T PinCfgCsv_ParseMeasurementSource(PINCFG_PARSE_SUBFN_PARAMS_T *psPrms)
{
    // MS,<type_enum>,<name>
    // Required: MS,0,name = 3 items (0 = MEASUREMENT_TYPE_CPUTEMP_E)
    if (psPrms->u8LineItemsLen != 3)
    {
        psPrms->pcOutStringLast += snprintf(
            (char *)(psPrms->psParsePrms->pcOutString + psPrms->pcOutStringLast),
            szGetSize(psPrms->psParsePrms->u16OutStrMaxLen, psPrms->pcOutStringLast),
            _s[FSDSS_E], _s[EL_E], psPrms->u16LinesProcessed, _s[MS_E], _s[IVLD_E], _s[ARGS_E]);
        psPrms->szNumberOfWarnings++;
        return PINCFG_OK_E;
    }

    // Get type enum (index 1) - parse as uint8_t
    uint8_t u8Type = 0;
    psPrms->sTempStrPt = psPrms->sLine;
    PinCfgStr_vGetSplitElemByIndex(&(psPrms->sTempStrPt), PINCFG_VALUE_SEPARATOR_D, 1);
    
    if (PinCfgStr_eAtoU8(&(psPrms->sTempStrPt), &u8Type) != PINCFG_STR_OK_E ||
        u8Type >= MEASUREMENT_TYPE_COUNT_E)
    {
        psPrms->pcOutStringLast += snprintf(
            (char *)(psPrms->psParsePrms->pcOutString + psPrms->pcOutStringLast),
            szGetSize(psPrms->psParsePrms->u16OutStrMaxLen, psPrms->pcOutStringLast),
            _s[FSDSSD_E], _s[EL_E], psPrms->u16LinesProcessed,
            _s[MS_E], _s[IVLD_E], _s[TE_E], MEASUREMENT_TYPE_COUNT_E - 1);
        psPrms->szNumberOfWarnings++;
        return PINCFG_OK_E;
    }
    
    MEASUREMENT_TYPE_T eType = (MEASUREMENT_TYPE_T)u8Type;

    // Get name (index 2) - allocate string storage
    psPrms->sTempStrPt = psPrms->sLine;
    PinCfgStr_vGetSplitElemByIndex(&(psPrms->sTempStrPt), PINCFG_VALUE_SEPARATOR_D, 2);
    
    // Calculate memory
    if (psPrms->psParsePrms->pszMemoryRequired != NULL)
    {
        *(psPrms->psParsePrms->pszMemoryRequired) +=
            Memory_szGetAllocatedSize(sizeof(CPUTEMPMEASURE_T)) +
            Memory_szGetAllocatedSize(psPrms->sTempStrPt.szLen + 1);
        return PINCFG_OK_E;
    }

    if (psPrms->psParsePrms->bValidate)
        return PINCFG_OK_E;

    // Allocate and copy name string
    char *pcName = (char *)Memory_vpAlloc(psPrms->sTempStrPt.szLen + 1);
    if (pcName == NULL)
    {
        Memory_eReset();
        psPrms->pcOutStringLast += snprintf(
            (char *)(psPrms->psParsePrms->pcOutString + psPrms->pcOutStringLast),
            szGetSize(psPrms->psParsePrms->u16OutStrMaxLen, psPrms->pcOutStringLast),
            _s[FSDSS_E], _s[EL_E], psPrms->u16LinesProcessed, _s[MS_E], _s[OOM_E], _s[NAME_E]);
        return PINCFG_OUTOFMEMORY_ERROR_E;
    }
    strncpy(pcName, psPrms->sTempStrPt.pcStrStart, psPrms->sTempStrPt.szLen);
    pcName[psPrms->sTempStrPt.szLen] = '\0';

    // Create measurement source based on type
    ISENSORMEASURE_T *psGenericMeasurement = NULL;
    
    switch (eType)
    {
    case MEASUREMENT_TYPE_CPUTEMP_E:
    {
        CPUTEMPMEASURE_T *psMeasurement = (CPUTEMPMEASURE_T *)Memory_vpAlloc(sizeof(CPUTEMPMEASURE_T));
        if (psMeasurement == NULL)
        {
            Memory_eReset();
            psPrms->pcOutStringLast += snprintf(
                (char *)(psPrms->psParsePrms->pcOutString + psPrms->pcOutStringLast),
                szGetSize(psPrms->psParsePrms->u16OutStrMaxLen, psPrms->pcOutStringLast),
                _s[FSDSS_E], _s[EL_E], psPrms->u16LinesProcessed, _s[MS_E], _s[OOM_E], _s[CPUTEMP_E]);
            return PINCFG_OUTOFMEMORY_ERROR_E;
        }

        if (CPUTempMeasure_eInit(psMeasurement, eType, pcName) != CPUTEMPMEASURE_OK_E)
        {
            psPrms->pcOutStringLast += snprintf(
                (char *)(psPrms->psParsePrms->pcOutString + psPrms->pcOutStringLast),
                szGetSize(psPrms->psParsePrms->u16OutStrMaxLen, psPrms->pcOutStringLast),
                _s[FSDSS_E], _s[EL_E], psPrms->u16LinesProcessed, _s[MS_E], _s[INITF_E], _s[NL_E]);
            psPrms->szNumberOfWarnings++;
            return PINCFG_OK_E;
        }
        
        psGenericMeasurement = &(psMeasurement->sSensorMeasure);
        break;
    }
    
    // Phase 3: Add cases for other measurement types here
#ifdef FEATURE_I2C_MEASUREMENT
    case MEASUREMENT_TYPE_I2C_E:
    {
        // Parse I2C-specific parameters
        // Format: MS,3,name,addr,cmd1,size[,cmd2,cmd3]/
        // Simple mode (6 params): MS,3,name,addr,register,size/
        // Command mode (7-8 params): MS,3,name,addr,cmd1,size,cmd2[,cmd3]/
        
        // Validate parameter count (need at least 6: MS, type, name, addr, cmd1, size)
        if (psPrms->u8LineItemsLen < 6)
        {
            psPrms->pcOutStringLast += snprintf(
                (char *)(psPrms->psParsePrms->pcOutString + psPrms->pcOutStringLast),
                szGetSize(psPrms->psParsePrms->u16OutStrMaxLen, psPrms->pcOutStringLast),
                _s[FSDSS_E], _s[EL_E], psPrms->u16LinesProcessed, _s[MS_E], _s[IVLD_E], " I2C params\n");
            psPrms->szNumberOfWarnings++;
            return PINCFG_OK_E;
        }
        
        uint8_t au8CommandBytes[3] = {0};
        uint8_t u8CommandLength = 0;
        uint16_t u16ConversionDelayMs = 0;
        
        // Parameter 3: I2C device address (hex or decimal)
        psPrms->sTempStrPt = psPrms->sLine;
        PinCfgStr_vGetSplitElemByIndex(&(psPrms->sTempStrPt), PINCFG_VALUE_SEPARATOR_D, 3);
        uint32_t u32Address = 0;
        if (PinCfgStr_eAtoU32(&(psPrms->sTempStrPt), &u32Address) != PINCFG_STR_OK_E)
        {
            psPrms->pcOutStringLast += snprintf(
                (char *)(psPrms->psParsePrms->pcOutString + psPrms->pcOutStringLast),
                szGetSize(psPrms->psParsePrms->u16OutStrMaxLen, psPrms->pcOutStringLast),
                _s[FSDSS_E], _s[EL_E], psPrms->u16LinesProcessed, _s[MS_E], _s[IVLD_E], " I2C address\n");
            psPrms->szNumberOfWarnings++;
            return PINCFG_OK_E;
        }
        uint8_t u8DeviceAddress = (uint8_t)u32Address;
        
        // Parameter 4: Command byte 1 (register or first command byte)
        psPrms->sTempStrPt = psPrms->sLine;
        PinCfgStr_vGetSplitElemByIndex(&(psPrms->sTempStrPt), PINCFG_VALUE_SEPARATOR_D, 4);
        uint32_t u32Cmd1 = 0;
        if (PinCfgStr_eAtoU32(&(psPrms->sTempStrPt), &u32Cmd1) != PINCFG_STR_OK_E)
        {
            psPrms->pcOutStringLast += snprintf(
                (char *)(psPrms->psParsePrms->pcOutString + psPrms->pcOutStringLast),
                szGetSize(psPrms->psParsePrms->u16OutStrMaxLen, psPrms->pcOutStringLast),
                _s[FSDSS_E], _s[EL_E], psPrms->u16LinesProcessed, _s[MS_E], _s[IVLD_E], " command byte\n");
            psPrms->szNumberOfWarnings++;
            return PINCFG_OK_E;
        }
        au8CommandBytes[0] = (uint8_t)u32Cmd1;
        u8CommandLength = 1;
        
        // Parameter 5: Data size (1-6 bytes)
        psPrms->sTempStrPt = psPrms->sLine;
        PinCfgStr_vGetSplitElemByIndex(&(psPrms->sTempStrPt), PINCFG_VALUE_SEPARATOR_D, 5);
        uint8_t u8DataSize = 0;
        if (PinCfgStr_eAtoU8(&(psPrms->sTempStrPt), &u8DataSize) != PINCFG_STR_OK_E)
        {
            psPrms->pcOutStringLast += snprintf(
                (char *)(psPrms->psParsePrms->pcOutString + psPrms->pcOutStringLast),
                szGetSize(psPrms->psParsePrms->u16OutStrMaxLen, psPrms->pcOutStringLast),
                _s[FSDSS_E], _s[EL_E], psPrms->u16LinesProcessed, _s[MS_E], _s[IVLD_E], " data size\n");
            psPrms->szNumberOfWarnings++;
            return PINCFG_OK_E;
        }
        
        // Validate data size (1-6 bytes)
        if (u8DataSize < 1 || u8DataSize > 6)
        {
            psPrms->pcOutStringLast += snprintf(
                (char *)(psPrms->psParsePrms->pcOutString + psPrms->pcOutStringLast),
                szGetSize(psPrms->psParsePrms->u16OutStrMaxLen, psPrms->pcOutStringLast),
                _s[FSDSS_E], _s[EL_E], psPrms->u16LinesProcessed, _s[MS_E], _s[IVLD_E], " data size (1-6)\n");
            psPrms->szNumberOfWarnings++;
            return PINCFG_OK_E;
        }
        
        // Check for optional command bytes (parameter 6 and 7)
        if (psPrms->u8LineItemsLen >= 7)
        {
            // Parameter 6 exists - command mode
            psPrms->sTempStrPt = psPrms->sLine;
            PinCfgStr_vGetSplitElemByIndex(&(psPrms->sTempStrPt), PINCFG_VALUE_SEPARATOR_D, 6);
            uint32_t u32Cmd2 = 0;
            if (PinCfgStr_eAtoU32(&(psPrms->sTempStrPt), &u32Cmd2) != PINCFG_STR_OK_E)
            {
                psPrms->pcOutStringLast += snprintf(
                    (char *)(psPrms->psParsePrms->pcOutString + psPrms->pcOutStringLast),
                    szGetSize(psPrms->psParsePrms->u16OutStrMaxLen, psPrms->pcOutStringLast),
                    _s[FSDSS_E], _s[EL_E], psPrms->u16LinesProcessed, _s[MS_E], _s[IVLD_E], " cmd2\n");
                psPrms->szNumberOfWarnings++;
                return PINCFG_OK_E;
            }
            au8CommandBytes[1] = (uint8_t)u32Cmd2;
            u8CommandLength = 2;
            
            // Try to parse parameter 7 (command byte 3)
            if (psPrms->u8LineItemsLen >= 8)
            {
                psPrms->sTempStrPt = psPrms->sLine;
                PinCfgStr_vGetSplitElemByIndex(&(psPrms->sTempStrPt), PINCFG_VALUE_SEPARATOR_D, 7);
                uint32_t u32Cmd3 = 0;
                if (PinCfgStr_eAtoU32(&(psPrms->sTempStrPt), &u32Cmd3) != PINCFG_STR_OK_E)
                {
                    psPrms->pcOutStringLast += snprintf(
                        (char *)(psPrms->psParsePrms->pcOutString + psPrms->pcOutStringLast),
                        szGetSize(psPrms->psParsePrms->u16OutStrMaxLen, psPrms->pcOutStringLast),
                        _s[FSDSS_E], _s[EL_E], psPrms->u16LinesProcessed, _s[MS_E], _s[IVLD_E], " cmd3\n");
                    psPrms->szNumberOfWarnings++;
                    return PINCFG_OK_E;
                }
                au8CommandBytes[2] = (uint8_t)u32Cmd3;
                u8CommandLength = 3;
            }
            
            // Determine conversion delay based on command pattern
            // AHT10: 0xAC trigger → 80ms delay
            // BME280: 0xF4 forced mode → 10ms delay
            if (au8CommandBytes[0] == 0xAC)
            {
                u16ConversionDelayMs = 80;  // AHT10
            }
            else if (au8CommandBytes[0] == 0xF4)
            {
                u16ConversionDelayMs = 10;  // BME280
            }
            else
            {
                u16ConversionDelayMs = 0;   // Unknown, no delay
            }
        }
        // else: Simple mode (6 params), u8CommandLength=1, u16ConversionDelayMs=0
        
        // Allocate I2C measurement structure
        I2CMEASURE_T *psMeasurement = (I2CMEASURE_T *)Memory_vpAlloc(sizeof(I2CMEASURE_T));
        if (psMeasurement == NULL)
        {
            Memory_eReset();
            psPrms->pcOutStringLast += snprintf(
                (char *)(psPrms->psParsePrms->pcOutString + psPrms->pcOutStringLast),
                szGetSize(psPrms->psParsePrms->u16OutStrMaxLen, psPrms->pcOutStringLast),
                _s[FSDSS_E], _s[EL_E], psPrms->u16LinesProcessed, _s[MS_E], _s[OOM_E], " (I2C)\n");
            return PINCFG_OUTOFMEMORY_ERROR_E;
        }
        
        // Initialize I2C measurement
        if (I2CMeasure_eInit(psMeasurement, eType, pcName, u8DeviceAddress, 
                             au8CommandBytes, u8CommandLength, u8DataSize, 
                             u16ConversionDelayMs) != PINCFG_OK_E)
        {
            psPrms->pcOutStringLast += snprintf(
                (char *)(psPrms->psParsePrms->pcOutString + psPrms->pcOutStringLast),
                szGetSize(psPrms->psParsePrms->u16OutStrMaxLen, psPrms->pcOutStringLast),
                _s[FSDSS_E], _s[EL_E], psPrms->u16LinesProcessed, _s[MS_E], _s[INITF_E], " (I2C)\n");
            psPrms->szNumberOfWarnings++;
            return PINCFG_OK_E;
        }
        
        psGenericMeasurement = &(psMeasurement->sInterface);
        break;
    }
#endif // FEATURE_I2C_MEASUREMENT
    
    case MEASUREMENT_TYPE_ANALOG_E:
    case MEASUREMENT_TYPE_DIGITAL_E:
#ifndef FEATURE_I2C_MEASUREMENT
    case MEASUREMENT_TYPE_I2C_E:
#endif
    case MEASUREMENT_TYPE_CALCULATED_E:
    default:
        psPrms->pcOutStringLast += snprintf(
            (char *)(psPrms->psParsePrms->pcOutString + psPrms->pcOutStringLast),
            szGetSize(psPrms->psParsePrms->u16OutStrMaxLen, psPrms->pcOutStringLast),
            _s[FSDSS_E], _s[EL_E], psPrms->u16LinesProcessed, _s[MS_E], _s[TNI_E]);
        psPrms->szNumberOfWarnings++;
        return PINCFG_OK_E;
    }

    // Register measurement (add to linked list during parsing)
    // Note: List can be NULL at start, LinkedList_eAddToLinkedList handles this
    if (psGenericMeasurement != NULL)
    {
        PINCFG_RESULT_T eAddResult = PinCfgCsv_eAddToLinkedList(
            (LINKEDLIST_ITEM_T **)&(psGlobals->ppsMeasurements), 
            (void *)psGenericMeasurement);
        
        if (eAddResult != PINCFG_OK_E)
        {
            psPrms->pcOutStringLast += snprintf(
                (char *)(psPrms->psParsePrms->pcOutString + psPrms->pcOutStringLast),
                szGetSize(psPrms->psParsePrms->u16OutStrMaxLen, psPrms->pcOutStringLast),
                _s[FSDSS_E], _s[EL_E], psPrms->u16LinesProcessed, _s[MS_E], _s[OOM_E], _s[NL_E]);
            return eAddResult;
        }
    }

    return PINCFG_OK_E;
}

// Phase 2: Parse Sensor Reporter (SR)
// Format: SR,<name>,<measurementName>,<vType>,<sType>,<enableable>,<cumulative>,<samplingMs>,<reportSec>,<offset>,<byteOffset>,<byteCount>/
// Example: SR,sensor1,temp0,6,6,0,0,1000,300,0.0/
static inline PINCFG_RESULT_T PinCfgCsv_ParseSensorReporter(PINCFG_PARSE_SUBFN_PARAMS_T *psPrms)
{
    // SR,<name>,<measurement>,<vType>,<sType>,<enableable>,<cumulative>,<sampMs>,<reportSec>,<offset>,<byteOffset>,<byteCount>
    // Min: SR,name,meas,6,6,0,0,1000,300 = 9 items (offset, byte offset/count optional)
    // Max: SR,name,meas,6,6,0,0,1000,300,0.0,0,0 = 12 items
    if (psPrms->u8LineItemsLen < 9 || psPrms->u8LineItemsLen > 12)
    {
        psPrms->pcOutStringLast += snprintf(
            (char *)(psPrms->psParsePrms->pcOutString + psPrms->pcOutStringLast),
            szGetSize(psPrms->psParsePrms->u16OutStrMaxLen, psPrms->pcOutStringLast),
            _s[FSDSS_E], _s[EL_E], psPrms->u16LinesProcessed, _s[SR_E], _s[IVLD_E], _s[ARGS_E]);
        psPrms->szNumberOfWarnings++;
        return PINCFG_OK_E;
    }

    // Get sensor name (index 1)
    psPrms->sTempStrPt = psPrms->sLine;
    PinCfgStr_vGetSplitElemByIndex(&(psPrms->sTempStrPt), PINCFG_VALUE_SEPARATOR_D, 1);
    STRING_POINT_T sSensorName = psPrms->sTempStrPt;

    // Get measurement name (index 2)  
    psPrms->sTempStrPt = psPrms->sLine;
    PinCfgStr_vGetSplitElemByIndex(&(psPrms->sTempStrPt), PINCFG_VALUE_SEPARATOR_D, 2);
    STRING_POINT_T sMeasurementName = psPrms->sTempStrPt;
    
    // Lookup measurement by name (skip during memory calculation)
    ISENSORMEASURE_T *psMeasurement = NULL;
    if (psPrms->psParsePrms->pszMemoryRequired == NULL)
    {
        psMeasurement = PinCfgCsv_psFindMeasurementByName(
            sMeasurementName.pcStrStart, 
            sMeasurementName.szLen);
    }
    
    if (psMeasurement == NULL && 
        !psPrms->psParsePrms->bValidate &&
        psPrms->psParsePrms->pszMemoryRequired == NULL)
    {
        psPrms->pcOutStringLast += snprintf(
            (char *)(psPrms->psParsePrms->pcOutString + psPrms->pcOutStringLast),
            szGetSize(psPrms->psParsePrms->u16OutStrMaxLen, psPrms->pcOutStringLast),
            "%s%d:%s%s%.*s\n",
            _s[EL_E], psPrms->u16LinesProcessed,
            _s[SR_E], _s[MNF_E],
            (int)psPrms->sTempStrPt.szLen,
            psPrms->sTempStrPt.pcStrStart);
        psPrms->szNumberOfWarnings++;
        return PINCFG_OK_E;
    }

    // Get V_TYPE (MySensors variable type, index 3)
    uint8_t u8VType = 0U;
    psPrms->sTempStrPt = psPrms->sLine;
    PinCfgStr_vGetSplitElemByIndex(&(psPrms->sTempStrPt), PINCFG_VALUE_SEPARATOR_D, 3);
    if (PinCfgStr_eAtoU8(&(psPrms->sTempStrPt), &u8VType) != PINCFG_STR_OK_E)
    {
        psPrms->pcOutStringLast += snprintf(
            (char *)(psPrms->psParsePrms->pcOutString + psPrms->pcOutStringLast),
            szGetSize(psPrms->psParsePrms->u16OutStrMaxLen, psPrms->pcOutStringLast),
            _s[FSDSS_E], _s[EL_E], psPrms->u16LinesProcessed, _s[SR_E], _s[IVLD_E], " V_TYPE\n");
        psPrms->szNumberOfWarnings++;
        return PINCFG_OK_E;
    }

    // Get S_TYPE (MySensors sensor type, index 4)
    uint8_t u8SType = 0U;
    psPrms->sTempStrPt = psPrms->sLine;
    PinCfgStr_vGetSplitElemByIndex(&(psPrms->sTempStrPt), PINCFG_VALUE_SEPARATOR_D, 4);
    if (PinCfgStr_eAtoU8(&(psPrms->sTempStrPt), &u8SType) != PINCFG_STR_OK_E)
    {
        psPrms->pcOutStringLast += snprintf(
            (char *)(psPrms->psParsePrms->pcOutString + psPrms->pcOutStringLast),
            szGetSize(psPrms->psParsePrms->u16OutStrMaxLen, psPrms->pcOutStringLast),
            _s[FSDSS_E], _s[EL_E], psPrms->u16LinesProcessed, _s[SR_E], _s[IVLD_E], " S_TYPE\n");
        psPrms->szNumberOfWarnings++;
        return PINCFG_OK_E;
    }

    // Get enableable (index 5)
    uint8_t u8Enableable = 0U;
    psPrms->sTempStrPt = psPrms->sLine;
    PinCfgStr_vGetSplitElemByIndex(&(psPrms->sTempStrPt), PINCFG_VALUE_SEPARATOR_D, 5);
    if (PinCfgStr_eAtoU8(&(psPrms->sTempStrPt), &u8Enableable) != PINCFG_STR_OK_E || u8Enableable > 1U)
    {
        psPrms->pcOutStringLast += snprintf(
            (char *)(psPrms->psParsePrms->pcOutString + psPrms->pcOutStringLast),
            szGetSize(psPrms->psParsePrms->u16OutStrMaxLen, psPrms->pcOutStringLast),
            _s[FSDSS_E], _s[EL_E], psPrms->u16LinesProcessed, _s[SR_E], _s[IVLD_E], _s[ENABLEABLE_E]);
        psPrms->szNumberOfWarnings++;
        return PINCFG_OK_E;
    }

    // Get cumulative (index 6)
    uint8_t u8Cumulative = 0U;
    psPrms->sTempStrPt = psPrms->sLine;
    PinCfgStr_vGetSplitElemByIndex(&(psPrms->sTempStrPt), PINCFG_VALUE_SEPARATOR_D, 6);
    if (PinCfgStr_eAtoU8(&(psPrms->sTempStrPt), &u8Cumulative) != PINCFG_STR_OK_E || u8Cumulative > 1U)
    {
        psPrms->pcOutStringLast += snprintf(
            (char *)(psPrms->psParsePrms->pcOutString + psPrms->pcOutStringLast),
            szGetSize(psPrms->psParsePrms->u16OutStrMaxLen, psPrms->pcOutStringLast),
            _s[FSDSS_E], _s[EL_E], psPrms->u16LinesProcessed, _s[SR_E], _s[IVLD_E], _s[CUMULATIVE_E]);
        psPrms->szNumberOfWarnings++;
        return PINCFG_OK_E;
    }

    // Get sampling interval (index 7)
    uint16_t u16SamplingIntervalMs = PINCFG_CPUTEMP_SAMPLING_INTV_MS_D;
    uint32_t u32Temp = 0;
    psPrms->sTempStrPt = psPrms->sLine;
    PinCfgStr_vGetSplitElemByIndex(&(psPrms->sTempStrPt), PINCFG_VALUE_SEPARATOR_D, 7);
    if (PinCfgStr_eAtoU32(&(psPrms->sTempStrPt), &u32Temp) != PINCFG_STR_OK_E ||
        u32Temp < PINCFG_CPUTEMP_SAMPLING_INTV_MIN_MS_D ||
        u32Temp > PINCFG_CPUTEMP_SAMPLING_INTV_MAX_MS_D)
    {
        psPrms->pcOutStringLast += snprintf(
            (char *)(psPrms->psParsePrms->pcOutString + psPrms->pcOutStringLast),
            szGetSize(psPrms->psParsePrms->u16OutStrMaxLen, psPrms->pcOutStringLast),
            _s[FSDSS_E], _s[EL_E], psPrms->u16LinesProcessed, _s[SR_E], _s[IVLD_E], _s[SAMPINT_E]);
        psPrms->szNumberOfWarnings++;
        return PINCFG_OK_E;
    }
    u16SamplingIntervalMs = (uint16_t)u32Temp;

    // Get report interval in SECONDS (index 8)
    uint16_t u16ReportIntervalSec = PINCFG_CPUTEMP_REPORTING_INTV_SEC_D;
    u32Temp = 0;
    psPrms->sTempStrPt = psPrms->sLine;
    PinCfgStr_vGetSplitElemByIndex(&(psPrms->sTempStrPt), PINCFG_VALUE_SEPARATOR_D, 8);
    if (PinCfgStr_eAtoU32(&(psPrms->sTempStrPt), &u32Temp) != PINCFG_STR_OK_E ||
        u32Temp < PINCFG_CPUTEMP_REPORTING_INTV_MIN_SEC_D ||
        u32Temp > PINCFG_CPUTEMP_REPORTING_INTV_MAX_SEC_D)
    {
        psPrms->pcOutStringLast += snprintf(
            (char *)(psPrms->psParsePrms->pcOutString + psPrms->pcOutStringLast),
            szGetSize(psPrms->psParsePrms->u16OutStrMaxLen, psPrms->pcOutStringLast),
            _s[FSDSS_E], _s[EL_E], psPrms->u16LinesProcessed, _s[SR_E], _s[IVLD_E], _s[REPINT_E]);
        psPrms->szNumberOfWarnings++;
        return PINCFG_OK_E;
    }
    u16ReportIntervalSec = (uint16_t)u32Temp;

    // Get offset (index 9, optional)
    float fOffset = PINCFG_CPUTEMP_OFFSET_D;
    if (psPrms->u8LineItemsLen >= 10)
    {
        psPrms->sTempStrPt = psPrms->sLine;
        PinCfgStr_vGetSplitElemByIndex(&(psPrms->sTempStrPt), PINCFG_VALUE_SEPARATOR_D, 9);
        if (PinCfgStr_eAtoFloat(&(psPrms->sTempStrPt), &fOffset) != PINCFG_STR_OK_E)
        {
            psPrms->pcOutStringLast += snprintf(
                (char *)(psPrms->psParsePrms->pcOutString + psPrms->pcOutStringLast),
                szGetSize(psPrms->psParsePrms->u16OutStrMaxLen, psPrms->pcOutStringLast),
                _s[FSDSS_E], _s[EL_E], psPrms->u16LinesProcessed, _s[SR_E], _s[IVLD_E], _s[OFFSET_E]);
            psPrms->szNumberOfWarnings++;
            return PINCFG_OK_E;
        }
    }

    // Get byte offset (index 10, optional, for multi-value sensors)
    uint8_t u8ByteOffset = 0U;
    if (psPrms->u8LineItemsLen >= 11)
    {
        psPrms->sTempStrPt = psPrms->sLine;
        PinCfgStr_vGetSplitElemByIndex(&(psPrms->sTempStrPt), PINCFG_VALUE_SEPARATOR_D, 10);
        if (PinCfgStr_eAtoU8(&(psPrms->sTempStrPt), &u8ByteOffset) != PINCFG_STR_OK_E || u8ByteOffset > 5U)
        {
            psPrms->pcOutStringLast += snprintf(
                (char *)(psPrms->psParsePrms->pcOutString + psPrms->pcOutStringLast),
                szGetSize(psPrms->psParsePrms->u16OutStrMaxLen, psPrms->pcOutStringLast),
                _s[FSDSS_E], _s[EL_E], psPrms->u16LinesProcessed, _s[SR_E], _s[IVLD_E], " byte offset (0-5)\n");
            psPrms->szNumberOfWarnings++;
            return PINCFG_OK_E;
        }
    }

    // Get byte count (index 11, optional, for multi-value sensors)
    uint8_t u8ByteCount = 0U;  // 0 means use all bytes from offset
    if (psPrms->u8LineItemsLen >= 12)
    {
        psPrms->sTempStrPt = psPrms->sLine;
        PinCfgStr_vGetSplitElemByIndex(&(psPrms->sTempStrPt), PINCFG_VALUE_SEPARATOR_D, 11);
        if (PinCfgStr_eAtoU8(&(psPrms->sTempStrPt), &u8ByteCount) != PINCFG_STR_OK_E || u8ByteCount > 6U)
        {
            psPrms->pcOutStringLast += snprintf(
                (char *)(psPrms->psParsePrms->pcOutString + psPrms->pcOutStringLast),
                szGetSize(psPrms->psParsePrms->u16OutStrMaxLen, psPrms->pcOutStringLast),
                _s[FSDSS_E], _s[EL_E], psPrms->u16LinesProcessed, _s[SR_E], _s[IVLD_E], " byte count (0-6)\n");
            psPrms->szNumberOfWarnings++;
            return PINCFG_OK_E;
        }
    }

    // Calculate memory
    if (psPrms->psParsePrms->pszMemoryRequired != NULL)
    {
        *(psPrms->psParsePrms->pszMemoryRequired) +=
            Memory_szGetAllocatedSize(sizeof(SENSOR_T)) +
            Memory_szGetAllocatedSize(sSensorName.szLen + 1);
        
        if (psPrms->psParsePrms->eAddToPresentables != NULL)
            *(psPrms->psParsePrms->pszMemoryRequired) +=
                Memory_szGetAllocatedSize(sizeof(LINKEDLIST_ITEM_T) + sizeof(void *));
        if (psPrms->psParsePrms->eAddToLoopables != NULL)
            *(psPrms->psParsePrms->pszMemoryRequired) +=
                Memory_szGetAllocatedSize(sizeof(LINKEDLIST_ITEM_T) + sizeof(void *));
        return PINCFG_OK_E;
    }

    if (psPrms->psParsePrms->bValidate)
        return PINCFG_OK_E;

    // Create sensor reporter
    SENSOR_T *psSensorHandle = (SENSOR_T *)Memory_vpAlloc(sizeof(SENSOR_T));
    if (psSensorHandle == NULL)
    {
        Memory_eReset();
        psPrms->pcOutStringLast += snprintf(
            (char *)(psPrms->psParsePrms->pcOutString + psPrms->pcOutStringLast),
            szGetSize(psPrms->psParsePrms->u16OutStrMaxLen, psPrms->pcOutStringLast),
            _s[FSDSS_E], _s[EL_E], psPrms->u16LinesProcessed, _s[SR_E], _s[OOM_E], _s[SENSOR_E]);
        return PINCFG_OUTOFMEMORY_ERROR_E;
    }

    // Link sensor to measurement source
    if (Sensor_eInit(
            psSensorHandle,
            psPrms->psParsePrms->eAddToLoopables,
            psPrms->psParsePrms->eAddToPresentables,
            &psPrms->u8PresentablesCount,
            (bool)u8Cumulative,
            (bool)u8Enableable,
            &sSensorName,
            (mysensors_data_t)u8VType,
            (mysensors_sensor_t)u8SType,
            InPin_vRcvMessage,
            psMeasurement,  // Link to measurement
            u16SamplingIntervalMs,
            u16ReportIntervalSec,
            fOffset) != SENSOR_OK_E)  // Sensor-specific calibration offset
    {
        psPrms->pcOutStringLast += snprintf(
            (char *)(psPrms->psParsePrms->pcOutString + psPrms->pcOutStringLast),
            szGetSize(psPrms->psParsePrms->u16OutStrMaxLen, psPrms->pcOutStringLast),
            _s[FSDSS_E], _s[EL_E], psPrms->u16LinesProcessed, _s[SR_E], _s[INITF_E], _s[NL_E]);
        psPrms->szNumberOfWarnings++;
    }

    // Set byte extraction parameters for multi-value sensors (e.g., AHT10 temp+humidity)
    psSensorHandle->u8DataByteOffset = u8ByteOffset;
    psSensorHandle->u8DataByteCount = u8ByteCount;

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

static inline ISENSORMEASURE_T *PinCfgCsv_psFindMeasurementByName(const char *pcName, size_t szNameLen)
{
    if (pcName == NULL || szNameLen == 0)
        return NULL;

    // During parsing, measurements are in a linked list
    // After parsing, they're in an array
    // Try array first (post-parse), then linked list (during parse)
    
    if (psGlobals->ppsMeasurements != NULL && psGlobals->u8MeasurementsCount > 0)
    {
        // Post-parse: Search array
        for (uint8_t i = 0; i < psGlobals->u8MeasurementsCount; i++)
        {
            ISENSORMEASURE_T *psMeasure = psGlobals->ppsMeasurements[i];
            if (psMeasure == NULL)
                continue;

            const char *pcStoredName = NULL;
            switch (psMeasure->eType)
            {
            case MEASUREMENT_TYPE_CPUTEMP_E:
                pcStoredName = ((CPUTEMPMEASURE_T *)psMeasure)->pcName;
                break;
            case MEASUREMENT_TYPE_ANALOG_E:
            case MEASUREMENT_TYPE_DIGITAL_E:
            case MEASUREMENT_TYPE_I2C_E:
            case MEASUREMENT_TYPE_CALCULATED_E:
            default:
                continue;
            }
            
            if (pcStoredName != NULL)
            {
                size_t szStoredLen = strlen(pcStoredName);
                if (szStoredLen == szNameLen && 
                    strncmp(pcStoredName, pcName, szNameLen) == 0)
                {
                    return psMeasure;
                }
            }
        }
    }
    else
    {
        // During parse: Search linked list
        LINKEDLIST_ITEM_T *psCurrent = (LINKEDLIST_ITEM_T *)psGlobals->ppsMeasurements;
        
        while (psCurrent != NULL)
        {
            // Get the actual measurement from the linked list item
            ISENSORMEASURE_T *psMeasure = (ISENSORMEASURE_T *)psCurrent->pvItem;
            if (psMeasure == NULL)
            {
                psCurrent = (LINKEDLIST_ITEM_T *)psCurrent->pvNext;
                continue;
            }
            
            const char *pcStoredName = NULL;
            switch (psMeasure->eType)
            {
            case MEASUREMENT_TYPE_CPUTEMP_E:
                pcStoredName = ((CPUTEMPMEASURE_T *)psMeasure)->pcName;
                break;
            case MEASUREMENT_TYPE_ANALOG_E:
            case MEASUREMENT_TYPE_DIGITAL_E:
            case MEASUREMENT_TYPE_I2C_E:
            case MEASUREMENT_TYPE_CALCULATED_E:
            default:
                psCurrent = (LINKEDLIST_ITEM_T *)psCurrent->pvNext;
                continue;
            }
            
            if (pcStoredName != NULL)
            {
                size_t szStoredLen = strlen(pcStoredName);
                if (szStoredLen == szNameLen && 
                    strncmp(pcStoredName, pcName, szNameLen) == 0)
                {
                    return psMeasure;
                }
            }
            
            psCurrent = (LINKEDLIST_ITEM_T *)psCurrent->pvNext;
        }
    }

    return NULL;  // Not found
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
