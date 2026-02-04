#include "PinCfgCsv.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "CPUTempMeasure.h"
#include "Cli.h"
#include "Event.h"
#include "Globals.h"
#include "InPin.h"
#include "LoopTimeMeasure.h"
#include "Memory.h"
#include "MySensorsWrapper.h"
#include "PersistentConfiguration.h"
#include "PinCfgMessages.h"
#include "PinCfgParse.h"
#include "PinCfgStr.h"
#include "Sensor.h"
#include "SensorMeasure.h"
#include "Switch.h"
#include "Trigger.h"

#ifdef PINCFG_FEATURE_I2C_MEASUREMENT
#include "I2CMeasure.h"
#endif

#ifdef PINCFG_FEATURE_LOOPTIME_MEASUREMENT
#include "LoopTimeMeasure.h"
#endif

#ifdef PINCFG_FEATURE_ANALOG_MEASUREMENT
#include "AnalogMeasure.h"
#endif

#ifdef PINCFG_FEATURE_SPI_MEASUREMENT
#include "SPIMeasure.h"
#endif

// Forward declarations
static PINCFG_RESULT_T PinCfgCsv_CreateCli(PINCFG_PARSE_SUBFN_PARAMS_T *psPrms);
static PINCFG_RESULT_T PinCfgCsv_ParseSwitch(PINCFG_PARSE_SUBFN_PARAMS_T *psPrms);
static PINCFG_RESULT_T PinCfgCsv_ParseInpins(PINCFG_PARSE_SUBFN_PARAMS_T *psPrms);
static PINCFG_RESULT_T PinCfgCsv_ParseTriggers(PINCFG_PARSE_SUBFN_PARAMS_T *psPrms);

// Phase 2: Measurement source and sensor reporter parsers
static PINCFG_RESULT_T PinCfgCsv_ParseMeasurementSource(PINCFG_PARSE_SUBFN_PARAMS_T *psPrms);
static PINCFG_RESULT_T PinCfgCsv_ParseSensorReporter(PINCFG_PARSE_SUBFN_PARAMS_T *psPrms);
static PINCFG_RESULT_T PinCfgCsv_eAddToLinkedList(LINKEDLIST_ITEM_T **psFirst, void *pvNew);

static PINCFG_RESULT_T PinCfgCsv_ParseGlobalConfigItems(PINCFG_PARSE_SUBFN_PARAMS_T *psPrms);

static PRESENTABLE_T *PinCfgCsv_psFindInPresentablesById(uint8_t u8Id);
static PRESENTABLE_T *PinCfgCsv_psFindInTempPresentablesByName(const STRING_POINT_T *psName);
static ISENSORMEASURE_T *PinCfgCsv_psFindMeasurementByName(
    PINCFG_PARSE_SUBFN_PARAMS_T *psPrms,
    const STRING_POINT_T *psName);

// ============================================================================
// Measurement type size lookup table (ROM)
// ============================================================================
static const uint8_t _au8MeasurementSizes[MEASUREMENT_TYPE_COUNT_E] = {
    [MEASUREMENT_TYPE_CPUTEMP_E] = sizeof(CPUTEMPMEASURE_T),
#ifdef PINCFG_FEATURE_ANALOG_MEASUREMENT
    [MEASUREMENT_TYPE_ANALOG_E] = sizeof(ANALOGMEASURE_T),
#else
    [MEASUREMENT_TYPE_ANALOG_E] = 0,
#endif
    [MEASUREMENT_TYPE_DIGITAL_E] = 0, // Not implemented
#ifdef PINCFG_FEATURE_I2C_MEASUREMENT
    [MEASUREMENT_TYPE_I2C_E] = sizeof(I2CMEASURE_T),
#else
    [MEASUREMENT_TYPE_I2C_E] = 0,
#endif
    [MEASUREMENT_TYPE_CALCULATED_E] = 0, // Not implemented
#ifdef PINCFG_FEATURE_LOOPTIME_MEASUREMENT
    [MEASUREMENT_TYPE_LOOPTIME_E] = sizeof(LOOPTIMEMEASURE_T),
#else
    [MEASUREMENT_TYPE_LOOPTIME_E] = 0,
#endif
#ifdef PINCFG_FEATURE_SPI_MEASUREMENT
    [MEASUREMENT_TYPE_SPI_E] = sizeof(SPIMEASURE_T),
#else
    [MEASUREMENT_TYPE_SPI_E] = 0,
#endif
};

// ============================================================================
// Helper functions for code size optimization
// ============================================================================

// Get field at given index (sets psPrms->sTempStrPt)
static void vGetField(PINCFG_PARSE_SUBFN_PARAMS_T *psPrms, uint8_t u8Index)
{
    psPrms->sTempStrPt = psPrms->sLine;
    PinCfgStr_vGetSplitElemByIndex(&(psPrms->sTempStrPt), PINCFG_VALUE_SEPARATOR_D, u8Index);
}

// Parse a field at given index as uint8_t
static PINCFG_STR_RESULT_T eParseFieldU8(PINCFG_PARSE_SUBFN_PARAMS_T *psPrms, uint8_t u8Index, uint8_t *pu8Out)
{
    vGetField(psPrms, u8Index);
    return PinCfgStr_eAtoU8(&(psPrms->sTempStrPt), pu8Out);
}

// Parse a field at given index as uint32_t
static PINCFG_STR_RESULT_T eParseFieldU32(PINCFG_PARSE_SUBFN_PARAMS_T *psPrms, uint8_t u8Index, uint32_t *pu32Out)
{
    vGetField(psPrms, u8Index);
    return PinCfgStr_eAtoU32(&(psPrms->sTempStrPt), pu32Out);
}

// Get optional field at given index (returns true if field exists and is non-empty)
static bool bGetOptionalField(PINCFG_PARSE_SUBFN_PARAMS_T *psPrms, uint8_t u8Index)
{
    if (psPrms->u8LineItemsLen >= u8Index + 1)
    {
        vGetField(psPrms, u8Index);
        return (psPrms->sTempStrPt.szLen > 0);
    }
    return false;
}

// Allocate memory with OOM error handling
// NOTE: Does NOT call Memory_eReset() - caller must decide whether to reset
static void *pvAllocOrOOM(
    PINCFG_PARSE_SUBFN_PARAMS_T *psPrms,
    size_t szSize,
    PINCFG_PARSE_STRINGS_T eComponentType,
    PINCFG_RESULT_T *peResult)
{
    void *pvAlloc = Memory_vpAlloc(szSize);
    if (pvAlloc == NULL)
    {
        psPrms->pcOutStringLast += LOG_ERROR(psPrms, PinCfgMessages_getString(eComponentType), ERR_OOM);
        *peResult = PINCFG_OUTOFMEMORY_ERROR_E;
    }
    return pvAlloc;
}

// Register component to presentables and loopables lists
static bool bRegisterComponent(
    PINCFG_PARSE_SUBFN_PARAMS_T *psPrms,
    PRESENTABLE_T *psPresentable,
    LOOPABLE_T *psLoopable,
    PINCFG_PARSE_STRINGS_T eComponentType)
{
    bool bOk = true;

    if (psPresentable != NULL && psPrms->psParsePrms->eAddToPresentables != NULL)
    {
        if (psPrms->psParsePrms->eAddToPresentables(psPresentable) == PINCFG_OK_E)
        {
            psPrms->u8PresentablesCount++;
        }
        else
        {
            bOk = false;
        }
    }

    if (bOk && psLoopable != NULL && psPrms->psParsePrms->eAddToLoopables != NULL)
    {
        bOk = (psPrms->psParsePrms->eAddToLoopables(psLoopable) == PINCFG_OK_E);
    }

    if (!bOk)
    {
        psPrms->pcOutStringLast += LOG_WARNING(psPrms, PinCfgMessages_getString(eComponentType), ERR_INIT_FAILED);
    }

    return bOk;
}

// Extended warning log with custom suffix string
__attribute__((unused)) static size_t szLogWarningExt(
    PINCFG_PARSE_SUBFN_PARAMS_T *psPrms,
    PINCFG_PARSE_STRINGS_T eComponentType,
    PINCFG_ERROR_CODE_T eErrorCode,
    const char *pcSuffix)
{
    psPrms->szNumberOfWarnings++;
    return szSafeAppendFormat(
        psPrms->psParsePrms->pcOutString,
        psPrms->pcOutStringLast,
        psPrms->psParsePrms->u16OutStrMaxLen,
        PinCfgMessages_getString(FSDSS_E),
        PinCfgMessages_getString(EL_E),
        psPrms->u16LinesProcessed,
        PinCfgMessages_getString(eComponentType),
        PinCfgMessages_getString(eErrorCode == ERR_OOM ? OOM_E : IVLD_E),
        pcSuffix);
}

// Calculate and add memory requirement for a component
static void vAddMemoryRequirement(
    PINCFG_PARSE_SUBFN_PARAMS_T *psPrms,
    size_t szStructSize,
    size_t szNameLen,
    bool bAddPresentable,
    bool bAddLoopable)
{
    if (psPrms->psParsePrms->pszMemoryRequired == NULL)
        return;

    *(psPrms->psParsePrms->pszMemoryRequired) +=
        Memory_szGetAllocatedSize(szStructSize) + Memory_szGetAllocatedSize(szNameLen + 1);

    if (bAddPresentable && psPrms->psParsePrms->eAddToPresentables != NULL)
        *(psPrms->psParsePrms->pszMemoryRequired) +=
            Memory_szGetAllocatedSize(sizeof(LINKEDLIST_ITEM_T) + sizeof(void *));

    if (bAddLoopable && psPrms->psParsePrms->eAddToLoopables != NULL)
        *(psPrms->psParsePrms->pszMemoryRequired) +=
            Memory_szGetAllocatedSize(sizeof(LINKEDLIST_ITEM_T) + sizeof(void *));
}

// ============================================================================

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
            if (!(psCurrent->u8Flags & PRESENTABLE_FLAG_STATE_PRESENTED))
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
        psGlobals->ppsPresentables[i]->psVtab->vPresent(psGlobals->ppsPresentables[i]);
        vWait(50);
    }
}

void PinCfgCsv_vReceiveMessage(const MyMessage *message)
{
    if (psGlobals == NULL)
        return;

    // Access MyMessage struct fields directly (C-compatible)
    PRESENTABLE_T *psReceiver = PinCfgCsv_psFindInPresentablesById(message->sensor);
    if (psReceiver == NULL)
        return;

    // Validate message type matches receiver's expected type
    if (psReceiver->psVtab->eVType != message->type)
        return;

    // Pass the entire message to the receiver
    psReceiver->psVtab->vReceive(psReceiver, message);
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
#ifndef USE_MALLOC
    if (pu8Memory == NULL)
    {
        return PINCFG_NULLPTR_ERROR_E;
    }
#endif // USE_MALLOC

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
    size_t szCfg = 0;
    PERCFG_RESULT_T eLoadResult = PersistentCfg_eGetConfigSize((uint16_t *)&szCfg);
    if (eLoadResult == PERCFG_OK_E)
    {
        if (szCfg <= 1)
        {
            // Empty EEPROM - set result to error so it falls through to pcDefaultCfg
            eLoadResult = PERCFG_ERROR_E;
        }
        else
        {
            sParseParams.pcConfig = (char *)Memory_vpTempAlloc(szCfg + 1);
            if (sParseParams.pcConfig == NULL)
                return ePincfgResult;

            eLoadResult = PersistentCfg_eLoadConfig((char *)sParseParams.pcConfig);
            if (eLoadResult == PERCFG_OK_E)
                ePincfgResult = PinCfgCsv_eParse(&sParseParams);
        }
    }

    if (pcDefaultCfg != NULL &&
        (eLoadResult != PERCFG_OK_E || (ePincfgResult != PINCFG_OK_E && ePincfgResult != PINCFG_WARNINGS_E)))
    {
        eMemAndTypesInitRes = PinCfgCsv_eInitMemoryAndTypes(pu8Memory, szMemorySize);
        if (eMemAndTypesInitRes != PINCFG_OK_E)
            return eMemAndTypesInitRes;

        sParseParams.pcConfig = pcDefaultCfg;
        ePincfgResult = PinCfgCsv_eParse(&sParseParams);
    }

    // Only convert linked lists to arrays if they were actually created
    // Empty config doesn't create linked lists, so skip conversion
    // Check if the lists are still NULL (indicating no items were added)
    if (psGlobals->ppsLoopables != NULL || psGlobals->ppsPresentables != NULL)
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

        // Note: Measurements are NOT converted to array
        // They remain in linked list during parsing only
        // Sensors store direct pointers, so no array needed at runtime
    }

    if (ePincfgResult == PINCFG_OUTOFMEMORY_ERROR_E && psGlobals->u8PresentablesCount > 0)
    {
        CLI_T *psCli = (CLI_T *)psGlobals->ppsPresentables[0];
        Cli_vSetState(psCli, CLI_OUT_OF_MEM_ERR_E, NULL, true);
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
        .szNumberOfWarnings = 0,
        .psMeasurementsListHead = NULL}; // Initialize measurement list

    uint16_t u16LinesLen;
    PINCFG_RESULT_T eResult = PINCFG_ERROR_E;

    if (sPrms.psParsePrms->pszMemoryRequired != NULL)
    {
        // Initialize with base memory (GLOBALS_T)
        // CLI and linked list items will be added by CreateCli and other creation functions
        *(sPrms.psParsePrms->pszMemoryRequired) = Memory_szGetAllocatedSize(sizeof(GLOBALS_T));
    }

    if (sPrms.psParsePrms->pcOutString != NULL && sPrms.psParsePrms->u16OutStrMaxLen > 0)
        sPrms.psParsePrms->pcOutString[0] = '\0';

    // Try to create CLI first - this allows OOM detection even with invalid config
    // This is important for memory requirement validation in test scenarios
    eResult = PinCfgCsv_CreateCli(&sPrms);
    if (eResult != PINCFG_OK_E)
        return eResult;

    // Now check for NULL config after CLI creation attempt
    if (psParams->pcConfig == NULL)
    {
        sPrms.pcOutStringLast += LOG_SIMPLE_ERROR(
            sPrms.psParsePrms->pcOutString, sPrms.pcOutStringLast, sPrms.psParsePrms->u16OutStrMaxLen, ERR_NULL_CONFIG);
        return PINCFG_NULLPTR_ERROR_E;
    }

    // Check for empty config after CLI creation
    PinCfgStr_vInitStrPoint(&(sPrms.sTempStrPt), psParams->pcConfig, strlen(psParams->pcConfig));
    u16LinesLen = (uint8_t)PinCfgStr_szGetSplitCount(&(sPrms.sTempStrPt), PINCFG_LINE_SEPARATOR_D);
    if (u16LinesLen == 0 || (u16LinesLen == 1 && sPrms.sTempStrPt.szLen == 0))
    {
        // Empty config is an error - CLI already created but that's OK
        sPrms.pcOutStringLast += LOG_SIMPLE_ERROR(
            sPrms.psParsePrms->pcOutString,
            sPrms.pcOutStringLast,
            sPrms.psParsePrms->u16OutStrMaxLen,
            ERR_EMPTY_CONFIG);
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
            sPrms.pcOutStringLast += LOG_WARNING(&sPrms, "", ERR_UNDEFINED_FORMAT);
            continue;
        }

        sPrms.sTempStrPt = sPrms.sLine;
        PinCfgStr_vGetSplitElemByIndex(&(sPrms.sTempStrPt), PINCFG_VALUE_SEPARATOR_D, 0);

        // Phase 2: Check 2-character prefixes FIRST before 1-character ones
        // Measurement Source (MS)
        if (sPrms.sTempStrPt.szLen == 2 && sPrms.sTempStrPt.pcStrStart[0] == 'M' &&
            sPrms.sTempStrPt.pcStrStart[1] == 'S')
        {
            eResult = PinCfgCsv_ParseMeasurementSource(&sPrms);
            if (eResult != PINCFG_OK_E)
                return eResult;
        }
        // Phase 2: Sensor Reporter (SR)
        else if (
            sPrms.sTempStrPt.szLen == 2 && sPrms.sTempStrPt.pcStrStart[0] == 'S' &&
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
            sPrms.pcOutStringLast += LOG_WARNING(&sPrms, "", ERR_UNKNOWN_TYPE);
        }
    }

    // Print final summary
#ifdef PINCFG_USE_ERROR_MESSAGES
    sPrms.pcOutStringLast += szSafeAppendFormat(
        sPrms.psParsePrms->pcOutString,
        sPrms.pcOutStringLast,
        sPrms.psParsePrms->u16OutStrMaxLen,
        "I: Configuration parsed.\n");
#else
    sPrms.pcOutStringLast += szSafeAppendFormat(
        sPrms.psParsePrms->pcOutString,
        sPrms.pcOutStringLast,
        sPrms.psParsePrms->u16OutStrMaxLen,
        "W%zu\n",
        sPrms.szNumberOfWarnings);
#endif

    if (sPrms.szNumberOfWarnings > 0)
        return PINCFG_WARNINGS_E;

    return PINCFG_OK_E;
}

// private
static PINCFG_RESULT_T PinCfgCsv_CreateCli(PINCFG_PARSE_SUBFN_PARAMS_T *psPrms)
{
    if (psPrms == NULL)
        return PINCFG_NULLPTR_ERROR_E;

    if (psPrms->psParsePrms->pszMemoryRequired != NULL)
    {
        *(psPrms->psParsePrms->pszMemoryRequired) += Memory_szGetAllocatedSize(sizeof(CLI_T));
        if (psPrms->psParsePrms->eAddToPresentables != NULL)
            *(psPrms->psParsePrms->pszMemoryRequired) +=
                Memory_szGetAllocatedSize(sizeof(LINKEDLIST_ITEM_T) + sizeof(void *));

        if (psPrms->psParsePrms->eAddToLoopables != NULL)
            *(psPrms->psParsePrms->pszMemoryRequired) +=
                Memory_szGetAllocatedSize(sizeof(LINKEDLIST_ITEM_T) + sizeof(void *));
    }

    if (!psPrms->psParsePrms->bValidate)
    {
        PINCFG_RESULT_T eAllocResult = PINCFG_OK_E;
        CLI_T *psCfgRcvrHnd = (CLI_T *)pvAllocOrOOM(psPrms, sizeof(CLI_T), CLI_E, &eAllocResult);
        if (eAllocResult != PINCFG_OK_E)
        {
            return eAllocResult;
        }

        if (Cli_eInit(psCfgRcvrHnd, psPrms->u8PresentablesCount) == CLI_OK_E)
        {
            PinCfgCsv_eAddToTempPresentables((PRESENTABLE_T *)psCfgRcvrHnd);
            PinCfgCsv_eAddToTempLoopables(&psCfgRcvrHnd->sLoopable);
            psPrms->u8PresentablesCount++;
        }
        else
        {
            psPrms->pcOutStringLast += LOG_SIMPLE_WARNING(
                psPrms->psParsePrms->pcOutString,
                psPrms->pcOutStringLast,
                psPrms->psParsePrms->u16OutStrMaxLen,
                ERR_INIT_FAILED);
        }
    }

    return PINCFG_OK_E;
}

static PINCFG_RESULT_T PinCfgCsv_ParseSwitch(PINCFG_PARSE_SUBFN_PARAMS_T *psPrms)
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
        psPrms->pcOutStringLast += LOG_WARNING(psPrms, PinCfgMessages_getString(SW_E), ERR_INVALID_ARGS);
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
            psPrms->pcOutStringLast += LOG_WARNING(psPrms, PinCfgMessages_getString(SW_E), ERR_INVALID_DEFINITION);
            return PINCFG_OK_E;
        }
    }

    u8Count = (uint8_t)((psPrms->u8LineItemsLen - 1) % u8SwItems);
    if (u8Count != 0)
    {
        psPrms->pcOutStringLast += LOG_WARNING(psPrms, PinCfgMessages_getString(SW_E), ERR_INVALID_ITEMS);
        return PINCFG_OK_E;
    }

    u8Count = (uint8_t)((psPrms->u8LineItemsLen - 1) / u8SwItems);
    for (i = 0; i < u8Count; i++)
    {
        u8Offset = 1 + i * u8SwItems;

        if (eParseFieldU8(psPrms, (u8Offset + 1), &u8Pin) != PINCFG_STR_OK_E)
        {
            psPrms->pcOutStringLast += LOG_WARNING(psPrms, PinCfgMessages_getString(SW_E), ERR_INVALID_PIN);
            continue;
        }

        // feedback pin
        if ((eMode != SWITCH_TIMED_E && u8SwItems == 3) || (eMode == SWITCH_TIMED_E && u8SwItems == 4))
        {
            uint8_t u8FbPinOffset = 2;
            if (u8SwItems == 4)
                u8FbPinOffset++;

            if (eParseFieldU8(psPrms, (u8Offset + u8FbPinOffset), &u8FbPin) != PINCFG_STR_OK_E)
            {
                psPrms->pcOutStringLast += LOG_WARNING(psPrms, PinCfgMessages_getString(SW_E), ERR_INVALID_PIN);
                continue;
            }
        }

        // time period
        if (eMode == SWITCH_TIMED_E)
        {
            if (eParseFieldU32(psPrms, (u8Offset + 2), &u32TimedPeriodMs) != PINCFG_STR_OK_E ||
                u32TimedPeriodMs < PINCFG_TIMED_SWITCH_MIN_PERIOD_MS_D ||
                u32TimedPeriodMs > PINCFG_TIMED_SWITCH_MAX_PERIOD_MS_D)
            {
                psPrms->pcOutStringLast += LOG_WARNING(psPrms, PinCfgMessages_getString(SW_E), ERR_INVALID_TIME_PERIOD);
                continue;
            }
        }

        vGetField(psPrms, u8Offset);

        vAddMemoryRequirement(psPrms, sizeof(SWITCH_T), psPrms->sTempStrPt.szLen, true, true);

        if (psPrms->psParsePrms->bValidate)
            continue;

        PINCFG_RESULT_T eAllocResult = PINCFG_OK_E;
        SWITCH_T *psSwitchHnd = (SWITCH_T *)pvAllocOrOOM(psPrms, sizeof(SWITCH_T), SW_E, &eAllocResult);
        if (psSwitchHnd == NULL)
            return eAllocResult;

        bool bInitOk =
            (Switch_eInit(
                 psSwitchHnd,
                 &(psPrms->sTempStrPt),
                 psPrms->u8PresentablesCount,
                 eMode,
                 u8Pin,
                 u8FbPin,
                 u32TimedPeriodMs) == SWITCH_OK_E);

        if (bInitOk)
        {
            bRegisterComponent(psPrms, (PRESENTABLE_T *)psSwitchHnd, &(psSwitchHnd->sLoopable), SW_E);
        }
        else
        {
            psPrms->pcOutStringLast += LOG_WARNING(psPrms, PinCfgMessages_getString(SW_E), ERR_INIT_FAILED);
        }
    }

    return PINCFG_OK_E;
}

static PINCFG_RESULT_T PinCfgCsv_ParseInpins(PINCFG_PARSE_SUBFN_PARAMS_T *psPrms)
{
    uint8_t u8Pin, u8Count, u8Offset, i;

    if (psPrms == NULL)
        return PINCFG_NULLPTR_ERROR_E;

    if (psPrms->u8LineItemsLen < 3)
    {
        psPrms->pcOutStringLast += LOG_WARNING(psPrms, PinCfgMessages_getString(IP_E), ERR_INVALID_ARGS);

        return PINCFG_OK_E;
    }

    u8Count = ((psPrms->u8LineItemsLen - 1) % 2);
    if (u8Count != 0)
    {
        psPrms->pcOutStringLast += LOG_WARNING(psPrms, PinCfgMessages_getString(IP_E), ERR_INVALID_ITEMS);
        return PINCFG_OK_E;
    }

    u8Count = ((psPrms->u8LineItemsLen - 1) / 2);
    for (i = 0; i < u8Count; i++)
    {
        u8Offset = 1 + i * 2;
        if (eParseFieldU8(psPrms, (u8Offset + 1), &u8Pin) != PINCFG_STR_OK_E || u8Pin < 1)
        {
            psPrms->pcOutStringLast += LOG_WARNING(psPrms, PinCfgMessages_getString(IP_E), ERR_INVALID_PIN);
            continue;
        }

        vGetField(psPrms, u8Offset);

        vAddMemoryRequirement(psPrms, sizeof(INPIN_T), psPrms->sTempStrPt.szLen, true, true);

        if (psPrms->psParsePrms->bValidate)
            continue;

        PINCFG_RESULT_T eAllocResult = PINCFG_OK_E;
        INPIN_T *psInPinHnd = (INPIN_T *)pvAllocOrOOM(psPrms, sizeof(INPIN_T), IP_E, &eAllocResult);
        if (psInPinHnd == NULL)
            return eAllocResult;

        bool bInitOk =
            (InPin_eInit(psInPinHnd, &(psPrms->sTempStrPt), psPrms->u8PresentablesCount, u8Pin) == INPIN_OK_E);

        if (bInitOk)
        {
            bRegisterComponent(psPrms, (PRESENTABLE_T *)psInPinHnd, &(psInPinHnd->sLoopable), IP_E);
        }
        else
        {
            psPrms->pcOutStringLast += LOG_WARNING(psPrms, PinCfgMessages_getString(IP_E), ERR_INIT_FAILED);
        }
    }

    return PINCFG_OK_E;
}

static PINCFG_RESULT_T PinCfgCsv_ParseTriggers(PINCFG_PARSE_SUBFN_PARAMS_T *psPrms)
{
    uint8_t u8Count, u8EventType, u8DrivesCountReal, u8Offset, u8DrivenAction;
    int32_t i32EventData;
    IEVENTPUBLISHER_T *psEventPublisherHnd;
    SWITCH_T *psSwitchHnd;

    if (psPrms == NULL)
        return PINCFG_NULLPTR_ERROR_E;

    if (psPrms->u8LineItemsLen < 7)
    {
        psPrms->pcOutStringLast += LOG_WARNING(psPrms, PinCfgMessages_getString(TRG_E), ERR_INVALID_ARGS);

        return PINCFG_OK_E;
    }

    vGetField(psPrms, 2);

    if (psPrms->psParsePrms->bValidate)
    {
        if (PinCfgCsv_pcStrstrpt(psPrms->psParsePrms->pcConfig, &(psPrms->sTempStrPt)) == NULL)
        {
            psPrms->pcOutStringLast += LOG_WARNING(psPrms, PinCfgMessages_getString(TRG_E), ERR_SOURCE_NOT_FOUND);

            return PINCFG_OK_E;
        }
    }
    else
    {
        psEventPublisherHnd = (IEVENTPUBLISHER_T *)PinCfgCsv_psFindInTempPresentablesByName(&(psPrms->sTempStrPt));
        if (psEventPublisherHnd == NULL)
        {
            psPrms->pcOutStringLast +=
                LOG_WARNING(psPrms, PinCfgMessages_getString(TRG_E), ERR_EVENT_PUBLISHER_NOT_FOUND);

            return PINCFG_OK_E;
        }
    }

    if (eParseFieldU8(psPrms, 3, &u8EventType) != PINCFG_STR_OK_E || u8EventType > (uint8_t)TRIGGER_ALL_E)
    {
        psPrms->pcOutStringLast += LOG_WARNING(psPrms, PinCfgMessages_getString(TRG_E), ERR_INVALID_EVENT_TYPE);

        return PINCFG_OK_E;
    }

    vGetField(psPrms, 4);
    if (PinCfgStr_eAtoFixedPoint(&(psPrms->sTempStrPt), &i32EventData) != PINCFG_STR_OK_E)
    {
        psPrms->pcOutStringLast += LOG_WARNING(psPrms, PinCfgMessages_getString(TRG_E), ERR_INVALID_EVENT_DATA);

        return PINCFG_OK_E;
    }
    if (u8EventType == (uint8_t)TRIGGER_MULTI_E)
        i32EventData /= PINCFG_FIXED_POINT_SCALE;

    psPrms->sTempStrPt = psPrms->sLine;
    u8Count = (uint8_t)((psPrms->u8LineItemsLen - 5) / 2);
    u8DrivesCountReal = 0;

    if (psPrms->psParsePrms->pszMemoryRequired != NULL)
    {
        *(psPrms->psParsePrms->pszMemoryRequired) +=
            Memory_szGetAllocatedSize((size_t)sizeof(TRIGGER_SWITCHACTION_T) * (size_t)u8Count) +
            Memory_szGetAllocatedSize(sizeof(TRIGGER_T));
    }

    if (psPrms->psParsePrms->bValidate)
    {
        for (uint8_t i = 0; i < u8Count; i++)
        {
            u8Offset = 5 + i * 2;
            vGetField(psPrms, u8Offset);
            if (PinCfgCsv_pcStrstrpt(psPrms->psParsePrms->pcConfig, &(psPrms->sTempStrPt)) == NULL)
            {
                psPrms->pcOutStringLast += LOG_WARNING(psPrms, PinCfgMessages_getString(TRG_E), ERR_SOURCE_NOT_FOUND);
                continue;
            }
        }
        return PINCFG_OK_E;
    }

    // allocate memory for switch actions
    PINCFG_RESULT_T eAllocResult = PINCFG_OK_E;
    TRIGGER_SWITCHACTION_T *pasSwActs = (TRIGGER_SWITCHACTION_T *)pvAllocOrOOM(
        psPrms, (size_t)sizeof(TRIGGER_SWITCHACTION_T) * (size_t)u8Count, TRG_E, &eAllocResult);
    if (pasSwActs == NULL)
        return eAllocResult;

    for (uint8_t i = 0; i < u8Count; i++)
    {
        u8Offset = 5 + i * 2;
        vGetField(psPrms, u8Offset);
        psSwitchHnd = (SWITCH_T *)PinCfgCsv_psFindInTempPresentablesByName(&(psPrms->sTempStrPt));
        if (psSwitchHnd == NULL)
        {
            psPrms->pcOutStringLast += LOG_WARNING(psPrms, PinCfgMessages_getString(TRG_E), ERR_SWITCH_NOT_FOUND);
            continue;
        }

        if (eParseFieldU8(psPrms, (u8Offset + 1), &u8DrivenAction) != PINCFG_STR_OK_E ||
            u8DrivenAction > (uint8_t)TRIGGER_A_FORWARD_E)
        {
            psPrms->pcOutStringLast += LOG_WARNING(psPrms, PinCfgMessages_getString(TRG_E), ERR_INVALID_SWITCH_ACTION);
            continue;
        }

        pasSwActs[u8DrivesCountReal].psSwitchHnd = psSwitchHnd;
        pasSwActs[u8DrivesCountReal].eAction = (TRIGGER_ACTION_T)u8DrivenAction;

        u8DrivesCountReal++;
    }

    if (u8DrivesCountReal == 0U)
    {
        psPrms->pcOutStringLast += LOG_WARNING(psPrms, PinCfgMessages_getString(TRG_E), ERR_NOTHING_TO_DRIVE);

        return PINCFG_OK_E;
    }

    TRIGGER_T *psTriggerHnd = (TRIGGER_T *)pvAllocOrOOM(psPrms, sizeof(TRIGGER_T), TRG_E, &eAllocResult);
    if (psTriggerHnd == NULL)
        return eAllocResult;

    if ((Trigger_eInit(psTriggerHnd, pasSwActs, u8DrivesCountReal, (TRIGGER_EVENTTYPE_T)u8EventType, i32EventData) !=
         TRIGGER_OK_E) ||
        (EventPublisher_eAddSubscriber(psEventPublisherHnd, (IEVENTSUBSCRIBER_T *)psTriggerHnd) !=
         EVENTSUBSCRIBER_OK_E))
    {
        psPrms->pcOutStringLast += LOG_WARNING(psPrms, PinCfgMessages_getString(TRG_E), ERR_INIT_FAILED);
        return PINCFG_ERROR_E;
    }

    return PINCFG_OK_E;
}

// Phase 2: Parse Measurement Source (MS)
// Format: MS,<type_enum>,<name>[,additional_params]/
// Example: MS,0,temp0/  (0 = MEASUREMENT_TYPE_CPUTEMP_E)
// Example: MS,1,battery,0/ (1 = MEASUREMENT_TYPE_ANALOG_E, pin 0)
static PINCFG_RESULT_T PinCfgCsv_ParseMeasurementSource(PINCFG_PARSE_SUBFN_PARAMS_T *psPrms)
{
    // MS,<type_enum>,<name>[,additional_params]
    // Required: At least 3 items (MS, type, name)
    // Each measurement type validates its own parameter count
    if (psPrms->u8LineItemsLen < 3)
    {
        psPrms->pcOutStringLast += LOG_WARNING(psPrms, PinCfgMessages_getString(MS_E), ERR_INVALID_ARGS);
        return PINCFG_OK_E;
    }

    // Get type enum (index 1) - parse as uint8_t
    uint8_t u8Type = 0;
    if (eParseFieldU8(psPrms, 1, &u8Type) != PINCFG_STR_OK_E || u8Type >= MEASUREMENT_TYPE_COUNT_E)
    {
        psPrms->pcOutStringLast += LOG_WARNING(psPrms, PinCfgMessages_getString(MS_E), ERR_INVALID_TYPE_ENUM);
        return PINCFG_OK_E;
    }

    MEASUREMENT_TYPE_T eType = (MEASUREMENT_TYPE_T)u8Type;

    // Get name (index 2) - allocate string storage
    vGetField(psPrms, 2);

    // Calculate memory using lookup table
    if (psPrms->psParsePrms->pszMemoryRequired != NULL)
    {
        size_t szMeasurementSize = _au8MeasurementSizes[eType];
        if (szMeasurementSize == 0)
            szMeasurementSize = sizeof(CPUTEMPMEASURE_T); // Fallback for unsupported types

        *(psPrms->psParsePrms->pszMemoryRequired) +=
            Memory_szGetAllocatedSize(szMeasurementSize) + Memory_szGetAllocatedSize(psPrms->sTempStrPt.szLen + 1);
        return PINCFG_OK_E;
    }

    if (psPrms->psParsePrms->bValidate)
        return PINCFG_OK_E;

    // Create measurement source based on type
    ISENSORMEASURE_T *psGenericMeasurement = NULL;

    switch (eType)
    {
    case MEASUREMENT_TYPE_CPUTEMP_E:
    {
        PINCFG_RESULT_T eAllocResult = PINCFG_OK_E;
        CPUTEMPMEASURE_T *psMeasurement =
            (CPUTEMPMEASURE_T *)pvAllocOrOOM(psPrms, sizeof(CPUTEMPMEASURE_T), MS_E, &eAllocResult);
        if (psMeasurement == NULL)
            return eAllocResult;

        // Initialize measurement (name allocation happens inside)
        if (CPUTempMeasure_eInit(psMeasurement, &(psPrms->sTempStrPt)) != CPUTEMPMEASURE_OK_E)
        {
            psPrms->pcOutStringLast += LOG_WARNING(psPrms, PinCfgMessages_getString(MS_E), ERR_INIT_FAILED);
            return PINCFG_OK_E;
        }

        psGenericMeasurement = &(psMeasurement->sSensorMeasure);
        break;
    }

    // Phase 3: Add cases for other measurement types here
#ifdef PINCFG_FEATURE_I2C_MEASUREMENT
    case MEASUREMENT_TYPE_I2C_E:
    {
        // Parse I2C-specific parameters
        // Format: MS,3,name,addr,cmd1,size[,cache_ms][,cmd2,cmd3]/
        // Simple mode (6 params): MS,3,name,addr,register,size/
        // Simple with cache (7 params): MS,3,name,addr,register,size,cache_ms/
        // Command mode (8-9 params): MS,3,name,addr,cmd1,size,cache_ms,cmd2[,cmd3]/

        // Validate parameter count (need at least 6: MS, type, name, addr, cmd1, size)
        if (psPrms->u8LineItemsLen < 6)
        {
            psPrms->pcOutStringLast += LOG_WARNING(psPrms, PinCfgMessages_getString(MS_E), ERR_INVALID_I2C_PARAMS);
            return PINCFG_OK_E;
        }

        uint8_t au8CommandBytes[3] = {0};
        uint8_t u8CommandLength = 0;
        uint16_t u16ConversionDelayMs = 0;
        uint16_t u16CacheValidMs = PINCFG_I2CMEASURE_CACHE_DEFAULT_MS_D;
        uint32_t u32Temp = 0;

        // Parameter 3: I2C device address (hex or decimal)
        if (eParseFieldU32(psPrms, 3, &u32Temp) != PINCFG_STR_OK_E)
        {
            psPrms->pcOutStringLast += LOG_WARNING(psPrms, PinCfgMessages_getString(MS_E), ERR_INVALID_I2C_ADDRESS);
            return PINCFG_OK_E;
        }
        uint8_t u8DeviceAddress = (uint8_t)u32Temp;

        // Parameter 4: Command byte 1 (register or first command byte)
        u32Temp = 0;
        if (eParseFieldU32(psPrms, 4, &u32Temp) != PINCFG_STR_OK_E)
        {
            psPrms->pcOutStringLast += LOG_WARNING(psPrms, PinCfgMessages_getString(MS_E), ERR_INVALID_I2C_CMD);
            return PINCFG_OK_E;
        }
        au8CommandBytes[0] = (uint8_t)u32Temp;
        u8CommandLength = 1;

        // Parameter 5: Data size (1-6 bytes)
        uint8_t u8DataSize = 0;
        if (eParseFieldU8(psPrms, 5, &u8DataSize) != PINCFG_STR_OK_E)
        {
            psPrms->pcOutStringLast += LOG_WARNING(psPrms, PinCfgMessages_getString(MS_E), ERR_INVALID_DATA_SIZE);
            return PINCFG_OK_E;
        }

        // Validate data size (1-6 bytes)
        if (u8DataSize < 1 || u8DataSize > PINCFG_I2C_MAX_DATA_SIZE)
        {
            psPrms->pcOutStringLast += LOG_WARNING(psPrms, PinCfgMessages_getString(MS_E), ERR_INVALID_DATA_SIZE);
            return PINCFG_OK_E;
        }

        // Parameter 6 (optional): Cache duration in ms (empty value uses default)
        if (bGetOptionalField(psPrms, 6))
        {
            u32Temp = 0;
            if (eParseFieldU32(psPrms, 6, &u32Temp) == PINCFG_STR_OK_E)
            {
                u16CacheValidMs = (uint16_t)u32Temp;
                if (u16CacheValidMs > PINCFG_I2CMEASURE_CACHE_MAX_MS_D)
                    u16CacheValidMs = PINCFG_I2CMEASURE_CACHE_MAX_MS_D;
            }
        }

        // Parameters 7-8 (optional): Additional command bytes for command mode
        if (psPrms->u8LineItemsLen >= 8)
        {
            // Parameter 7: Command byte 2
            u32Temp = 0;
            if (eParseFieldU32(psPrms, 7, &u32Temp) != PINCFG_STR_OK_E)
            {
                psPrms->pcOutStringLast += LOG_WARNING(psPrms, PinCfgMessages_getString(MS_E), ERR_INVALID_I2C_CMD);
                return PINCFG_OK_E;
            }
            au8CommandBytes[1] = (uint8_t)u32Temp;
            u8CommandLength = 2;

            // Parameter 8 (optional): Command byte 3
            if (psPrms->u8LineItemsLen >= 9)
            {
                u32Temp = 0;
                if (eParseFieldU32(psPrms, 8, &u32Temp) != PINCFG_STR_OK_E)
                {
                    psPrms->pcOutStringLast += LOG_WARNING(psPrms, PinCfgMessages_getString(MS_E), ERR_INVALID_I2C_CMD);
                    return PINCFG_OK_E;
                }
                au8CommandBytes[2] = (uint8_t)u32Temp;
                u8CommandLength = 3;
            }

            // Determine conversion delay based on command pattern
            // AHT10: 0xAC trigger → 80ms delay
            // BME280: 0xF4 forced mode → 10ms delay
            if (au8CommandBytes[0] == 0xAC)
            {
                u16ConversionDelayMs = 80; // AHT10
            }
            else if (au8CommandBytes[0] == 0xF4)
            {
                u16ConversionDelayMs = 10; // BME280
            }
            else
            {
                u16ConversionDelayMs = 0; // Unknown, no delay
            }
        }

        // Allocate I2C measurement structure
        PINCFG_RESULT_T eAllocResult = PINCFG_OK_E;
        I2CMEASURE_T *psMeasurement = (I2CMEASURE_T *)pvAllocOrOOM(psPrms, sizeof(I2CMEASURE_T), MS_E, &eAllocResult);
        if (psMeasurement == NULL)
            return eAllocResult;

        // Get name back for init
        vGetField(psPrms, 2);

        // Initialize I2C measurement (name allocation happens inside)
        if (I2CMeasure_eInit(
                psMeasurement,
                &(psPrms->sTempStrPt),
                u8DeviceAddress,
                au8CommandBytes,
                u8CommandLength,
                u8DataSize,
                u16ConversionDelayMs,
                u16CacheValidMs) != PINCFG_OK_E)
        {
            psPrms->pcOutStringLast += LOG_WARNING(psPrms, PinCfgMessages_getString(MS_E), ERR_INIT_FAILED);
            return PINCFG_OK_E;
        }

        psGenericMeasurement = &(psMeasurement->sInterface);
        break;
    }
#endif // PINCFG_FEATURE_I2C_MEASUREMENT

#ifdef PINCFG_FEATURE_SPI_MEASUREMENT
    case MEASUREMENT_TYPE_SPI_E:
    {
        // Parse SPI-specific parameters
        // Format: MS,6,name,cs_pin,cmd1,size[,cmd2,cmd3,cmd4,delay]/
        // Simple mode (5 params): MS,6,name,cs_pin,size/
        // Command mode (6+ params): MS,6,name,cs_pin,cmd1,size[,cmd2,cmd3,cmd4,delay]/

        // Validate parameter count (need at least 5: MS, type, name, cs_pin, size)
        if (psPrms->u8LineItemsLen < 5)
        {
            psPrms->pcOutStringLast += szLogWarningExt(psPrms, MS_E, ERR_INVALID_ARGS, " SPI params\n");
            return PINCFG_OK_E;
        }

        uint8_t au8CommandBytes[4] = {0};
        uint8_t u8CommandLength = 0;
        uint16_t u16ConversionDelayMs = 0;

        // Parameter 3: CS pin number
        uint8_t u8CSPin = 0;
        if (eParseFieldU8(psPrms, 3, &u8CSPin) != PINCFG_STR_OK_E)
        {
            psPrms->pcOutStringLast += LOG_WARNING(psPrms, PinCfgMessages_getString(MS_E), ERR_INVALID_PIN);
            return PINCFG_OK_E;
        }

        uint8_t u8DataSize = 0;

        // Check if parameter 4 is size (simple mode) or command byte (command mode)
        // Simple mode: MS,6,name,cs,size/
        // Command mode: MS,6,name,cs,cmd1,size[,cmd2,cmd3,cmd4,delay]/
        if (psPrms->u8LineItemsLen == 5)
        {
            // Simple mode - parameter 4 is size
            if (eParseFieldU8(psPrms, 4, &u8DataSize) != PINCFG_STR_OK_E)
            {
                psPrms->pcOutStringLast += LOG_WARNING(psPrms, PinCfgMessages_getString(MS_E), ERR_INVALID_DATA_SIZE);
                return PINCFG_OK_E;
            }
            u8CommandLength = 0;
        }
        else
        {
            // Command mode - parse from the end backwards
            // Last param might be delay (if > 8) or size (if 1-8)
            // Format: MS,6,name,cs,cmd[1-4],size[,delay]/

            // Check if last parameter is a delay (> 8)
            uint8_t u8SizeParamIndex;
            uint32_t u32LastParam = 0;
            if (eParseFieldU32(psPrms, psPrms->u8LineItemsLen - 1, &u32LastParam) == PINCFG_STR_OK_E &&
                u32LastParam > 8)
            {
                // Last param is delay
                u16ConversionDelayMs = (uint16_t)u32LastParam;
                u8SizeParamIndex = psPrms->u8LineItemsLen - 2; // Size is second-to-last
            }
            else
            {
                // Last param is size
                u8SizeParamIndex = psPrms->u8LineItemsLen - 1;
            }

            // Parse size
            if (eParseFieldU8(psPrms, u8SizeParamIndex, &u8DataSize) != PINCFG_STR_OK_E)
            {
                psPrms->pcOutStringLast += LOG_WARNING(psPrms, PinCfgMessages_getString(MS_E), ERR_INVALID_DATA_SIZE);
                return PINCFG_OK_E;
            }

            // Parse command bytes (between index 4 and sizeParamIndex)
            u8CommandLength = u8SizeParamIndex - 4; // Number of params between cs and size
            if (u8CommandLength > PINCFG_SPI_MAX_CMD_BYTES)
            {
                psPrms->pcOutStringLast +=
                    szLogWarningExt(psPrms, MS_E, ERR_INVALID_ARGS, " too many command bytes (max 4)\n");
                return PINCFG_OK_E;
            }

            // Read command bytes
            for (uint8_t i = 0; i < u8CommandLength; i++)
            {
                uint32_t u32Cmd = 0;
                if (eParseFieldU32(psPrms, 4 + i, &u32Cmd) != PINCFG_STR_OK_E)
                {
                    psPrms->pcOutStringLast += LOG_WARNING(psPrms, PinCfgMessages_getString(MS_E), ERR_INVALID_ARGS);
                    return PINCFG_OK_E;
                }
                au8CommandBytes[i] = (uint8_t)u32Cmd;
            }
        }

        // Validate data size (1-8 bytes for SPI)
        if (u8DataSize < 1 || u8DataSize > PINCFG_SPI_MAX_DATA_SIZE)
        {
            psPrms->pcOutStringLast += LOG_WARNING(psPrms, PinCfgMessages_getString(MS_E), ERR_INVALID_DATA_SIZE);
            return PINCFG_OK_E;
        }

        // Allocate SPI measurement structure
        PINCFG_RESULT_T eAllocResult = PINCFG_OK_E;
        SPIMEASURE_T *psMeasurement = (SPIMEASURE_T *)pvAllocOrOOM(psPrms, sizeof(SPIMEASURE_T), MS_E, &eAllocResult);
        if (psMeasurement == NULL)
            return eAllocResult;

        // Get name from index 2
        vGetField(psPrms, 2);

        // Initialize SPI measurement
        if (SPIMeasure_eInit(
                psMeasurement,
                &(psPrms->sTempStrPt),
                u8CSPin,
                u8CommandLength > 0 ? au8CommandBytes : NULL,
                u8CommandLength,
                u8DataSize,
                u16ConversionDelayMs) != PINCFG_OK_E)
        {
            psPrms->pcOutStringLast += LOG_WARNING(psPrms, PinCfgMessages_getString(MS_E), ERR_INIT_FAILED);
            return PINCFG_OK_E;
        }

        psGenericMeasurement = &(psMeasurement->sInterface);
        break;
    }
#endif // PINCFG_FEATURE_SPI_MEASUREMENT

#ifdef PINCFG_FEATURE_LOOPTIME_MEASUREMENT
    case MEASUREMENT_TYPE_LOOPTIME_E:
    {
        PINCFG_RESULT_T eAllocResult = PINCFG_OK_E;
        LOOPTIMEMEASURE_T *psMeasurement =
            (LOOPTIMEMEASURE_T *)pvAllocOrOOM(psPrms, sizeof(LOOPTIMEMEASURE_T), MS_E, &eAllocResult);
        if (psMeasurement == NULL)
            return eAllocResult;

        // Initialize measurement (name allocation happens inside)
        if (LoopTimeMeasure_eInit(psMeasurement, &(psPrms->sTempStrPt)) != LOOPTIMEMEASURE_OK_E)
        {
            psPrms->pcOutStringLast += LOG_WARNING(psPrms, PinCfgMessages_getString(MS_E), ERR_INIT_FAILED);
            return PINCFG_OK_E;
        }

        psGenericMeasurement = &(psMeasurement->sSensorMeasure);
        break;
    }
#endif // PINCFG_FEATURE_LOOPTIME_MEASUREMENT

#ifdef PINCFG_FEATURE_ANALOG_MEASUREMENT
    case MEASUREMENT_TYPE_ANALOG_E:
    {
        // Parse analog-specific parameters
        // Format: MS,1,name,pin/
        // Required: MS, type(1), name, pin = 4 items
        if (psPrms->u8LineItemsLen != 4)
        {
            psPrms->pcOutStringLast += LOG_WARNING(psPrms, PinCfgMessages_getString(MS_E), ERR_INVALID_ARGS);
            return PINCFG_OK_E;
        }

        // Parameter 3: Analog pin number
        uint8_t u8Pin = 0;
        if (eParseFieldU8(psPrms, 3, &u8Pin) != PINCFG_STR_OK_E)
        {
            psPrms->pcOutStringLast += LOG_WARNING(psPrms, PinCfgMessages_getString(MS_E), ERR_INVALID_PIN);
            return PINCFG_OK_E;
        }

        // Allocate analog measurement structure
        PINCFG_RESULT_T eAllocResult = PINCFG_OK_E;
        ANALOGMEASURE_T *psMeasurement =
            (ANALOGMEASURE_T *)pvAllocOrOOM(psPrms, sizeof(ANALOGMEASURE_T), MS_E, &eAllocResult);
        if (psMeasurement == NULL)
            return eAllocResult;

        // Get name from index 2
        vGetField(psPrms, 2);

        // Initialize analog measurement (name allocation happens inside)
        if (AnalogMeasure_eInit(psMeasurement, &(psPrms->sTempStrPt), u8Pin) != ANALOGMEASURE_OK_E)
        {
            psPrms->pcOutStringLast += LOG_WARNING(psPrms, PinCfgMessages_getString(MS_E), ERR_INIT_FAILED);
            return PINCFG_OK_E;
        }

        psGenericMeasurement = &(psMeasurement->sSensorMeasure);
        break;
    }
#endif // PINCFG_FEATURE_ANALOG_MEASUREMENT

    case MEASUREMENT_TYPE_DIGITAL_E:
#ifndef PINCFG_FEATURE_I2C_MEASUREMENT
    case MEASUREMENT_TYPE_I2C_E:
#endif
#ifndef PINCFG_FEATURE_ANALOG_MEASUREMENT
    case MEASUREMENT_TYPE_ANALOG_E:
#endif
    case MEASUREMENT_TYPE_CALCULATED_E:
    default:
        psPrms->pcOutStringLast += LOG_WARNING(psPrms, PinCfgMessages_getString(MS_E), ERR_TYPE_NOT_IMPLEMENTED);
        return PINCFG_OK_E;
    }

    // Register measurement (add to linked list during parsing)
    // Note: List can be NULL at start, LinkedList_eAddToLinkedList handles this
    if (psGenericMeasurement != NULL)
    {
        PINCFG_RESULT_T eAddResult = PinCfgCsv_eAddToLinkedList(
            (LINKEDLIST_ITEM_T **)&(psPrms->psMeasurementsListHead), (void *)psGenericMeasurement);

        if (eAddResult != PINCFG_OK_E)
        {
            psPrms->pcOutStringLast += LOG_ERROR(psPrms, PinCfgMessages_getString(MS_E), ERR_OOM);
            return eAddResult;
        }
    }

    return PINCFG_OK_E;
}

// Phase 2: Parse Sensor Reporter (SR)
// Format: SR,<name>,<measurementName>,<vType>,<sType>,<enableable>,<cumulative>,<samplingMs>,<reportSec>,<scale>,<offset>,<precision>,<byteOffset>,<byteCount>,<unit>/
// Example: SR,sensor1,temp0,6,6,0,0,1000,300,1.0,0,0,0,0,°C/
static PINCFG_RESULT_T PinCfgCsv_ParseSensorReporter(PINCFG_PARSE_SUBFN_PARAMS_T *psPrms)
{
    // SR,<name>,<measurement>,<vType>,<sType>,<enableable>,<cumulative>,<sampMs>,<reportSec>,<scale>,<offset>,<precision>,<unit>,<byteOffset>,<byteCount>,<bitShift>,<bitMask>,<endianness>
    // Min: SR,name,meas,6,6,0,0,1000,300 = 9 items (scale, offset, precision, unit, byte extraction, transform
    // optional) Max: SR,name,meas,6,6,0,0,1000,300,0.0625,-2.1,2,°C,0,2,4,0x0FFFFF,0 = 18 items
    if (psPrms->u8LineItemsLen < 9 || psPrms->u8LineItemsLen > 18)
    {
        psPrms->pcOutStringLast += LOG_WARNING(psPrms, PinCfgMessages_getString(SR_E), ERR_INVALID_ARGS);
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
        psMeasurement = PinCfgCsv_psFindMeasurementByName(psPrms, &sMeasurementName);
    }

    if (psMeasurement == NULL && !psPrms->psParsePrms->bValidate && psPrms->psParsePrms->pszMemoryRequired == NULL)
    {
        psPrms->pcOutStringLast += LOG_WARNING(psPrms, PinCfgMessages_getString(SR_E), ERR_MEASUREMENT_NOT_FOUND);
        return PINCFG_OK_E;
    }

    // Get V_TYPE (MySensors variable type, index 3)
    uint8_t u8VType = 0U;
    psPrms->sTempStrPt = psPrms->sLine;
    PinCfgStr_vGetSplitElemByIndex(&(psPrms->sTempStrPt), PINCFG_VALUE_SEPARATOR_D, 3);
    if (PinCfgStr_eAtoU8(&(psPrms->sTempStrPt), &u8VType) != PINCFG_STR_OK_E)
    {
        psPrms->pcOutStringLast += LOG_WARNING(psPrms, PinCfgMessages_getString(SR_E), ERR_INVALID_VTYPE);
        return PINCFG_OK_E;
    }

    // Get S_TYPE (MySensors sensor type, index 4)
    uint8_t u8SType = 0U;
    psPrms->sTempStrPt = psPrms->sLine;
    PinCfgStr_vGetSplitElemByIndex(&(psPrms->sTempStrPt), PINCFG_VALUE_SEPARATOR_D, 4);
    if (PinCfgStr_eAtoU8(&(psPrms->sTempStrPt), &u8SType) != PINCFG_STR_OK_E)
    {
        psPrms->pcOutStringLast += LOG_WARNING(psPrms, PinCfgMessages_getString(SR_E), ERR_INVALID_STYPE);
        return PINCFG_OK_E;
    }

    // Get enableable (index 5)
    uint8_t u8Enableable = 0U;
    psPrms->sTempStrPt = psPrms->sLine;
    PinCfgStr_vGetSplitElemByIndex(&(psPrms->sTempStrPt), PINCFG_VALUE_SEPARATOR_D, 5);
    if (PinCfgStr_eAtoU8(&(psPrms->sTempStrPt), &u8Enableable) != PINCFG_STR_OK_E || u8Enableable > 1U)
    {
        psPrms->pcOutStringLast += LOG_WARNING(psPrms, PinCfgMessages_getString(SR_E), ERR_INVALID_ENABLEABLE);
        return PINCFG_OK_E;
    }

    // Get cumulative (index 6)
    uint8_t u8Cumulative = 0U;
    psPrms->sTempStrPt = psPrms->sLine;
    PinCfgStr_vGetSplitElemByIndex(&(psPrms->sTempStrPt), PINCFG_VALUE_SEPARATOR_D, 6);
    if (PinCfgStr_eAtoU8(&(psPrms->sTempStrPt), &u8Cumulative) != PINCFG_STR_OK_E || u8Cumulative > 1U)
    {
        psPrms->pcOutStringLast += LOG_WARNING(psPrms, PinCfgMessages_getString(SR_E), ERR_INVALID_CUMULATIVE);
        return PINCFG_OK_E;
    }

    // Get sampling interval (index 7)
    uint16_t u16SamplingIntervalMs = PINCFG_SENSOR_SAMPLING_INTV_MS_D;
    uint32_t u32Temp = 0;
    psPrms->sTempStrPt = psPrms->sLine;
    PinCfgStr_vGetSplitElemByIndex(&(psPrms->sTempStrPt), PINCFG_VALUE_SEPARATOR_D, 7);
    if (PinCfgStr_eAtoU32(&(psPrms->sTempStrPt), &u32Temp) != PINCFG_STR_OK_E ||
        u32Temp < PINCFG_SENSOR_SAMPLING_INTV_MIN_MS_D || u32Temp > PINCFG_SENSOR_SAMPLING_INTV_MAX_MS_D)
    {
        psPrms->pcOutStringLast += LOG_WARNING(psPrms, PinCfgMessages_getString(SR_E), ERR_INVALID_SAMPLING_INTV);
        return PINCFG_OK_E;
    }
    u16SamplingIntervalMs = (uint16_t)u32Temp;

    // Get report interval in SECONDS (index 8)
    uint16_t u16ReportIntervalSec = PINCFG_SENSOR_REPORTING_INTV_SEC_D;
    u32Temp = 0;
    psPrms->sTempStrPt = psPrms->sLine;
    PinCfgStr_vGetSplitElemByIndex(&(psPrms->sTempStrPt), PINCFG_VALUE_SEPARATOR_D, 8);
    if (PinCfgStr_eAtoU32(&(psPrms->sTempStrPt), &u32Temp) != PINCFG_STR_OK_E ||
        u32Temp < PINCFG_SENSOR_REPORTING_INTV_MIN_SEC_D || u32Temp > PINCFG_SENSOR_REPORTING_INTV_MAX_SEC_D)
    {
        psPrms->pcOutStringLast += LOG_WARNING(psPrms, PinCfgMessages_getString(SR_E), ERR_INVALID_REPORT_INTV);
        return PINCFG_OK_E;
    }
    u16ReportIntervalSec = (uint16_t)u32Temp;

    // Get scale (index 9, optional, multiplicative factor)
    int32_t i32Scale = (int32_t)(PINCFG_SENSOR_SCALE_D * PINCFG_FIXED_POINT_SCALE); // Default: 1.0
    if (bGetOptionalField(psPrms, 9))
    {
        int32_t i32MinScale = (int32_t)(PINCFG_SENSOR_SCALE_MIN_D * PINCFG_FIXED_POINT_SCALE);
        int32_t i32MaxScale = (int32_t)(PINCFG_SENSOR_SCALE_MAX_D * PINCFG_FIXED_POINT_SCALE);
        if (PinCfgStr_eAtoFixedPoint(&(psPrms->sTempStrPt), &i32Scale) != PINCFG_STR_OK_E || i32Scale < i32MinScale ||
            i32Scale > i32MaxScale)
        {
            psPrms->pcOutStringLast += szSafeAppendFormat(
                psPrms->psParsePrms->pcOutString,
                psPrms->pcOutStringLast,
                psPrms->psParsePrms->u16OutStrMaxLen,
                PinCfgMessages_getString(FSDSSS_E),
                PinCfgMessages_getString(EL_E),
                psPrms->u16LinesProcessed,
                PinCfgMessages_getString(SR_E),
                PinCfgMessages_getString(IVLD_E),
                PinCfgMessages_getString(SCALE_E));
            return PINCFG_OK_E;
        }
    }

    // Get offset (index 10, optional, additive adjustment)
    int32_t i32Offset = (int32_t)(PINCFG_SENSOR_OFFSET_D * PINCFG_FIXED_POINT_SCALE); // Default: 0.0
    if (bGetOptionalField(psPrms, 10))
    {
        int32_t i32MinOffset = (int32_t)(PINCFG_SENSOR_OFFSET_MIN_D * PINCFG_FIXED_POINT_SCALE);
        int32_t i32MaxOffset = (int32_t)(PINCFG_SENSOR_OFFSET_MAX_D * PINCFG_FIXED_POINT_SCALE);
        if (PinCfgStr_eAtoFixedPoint(&(psPrms->sTempStrPt), &i32Offset) != PINCFG_STR_OK_E ||
            i32Offset < i32MinOffset || i32Offset > i32MaxOffset)
        {
            psPrms->pcOutStringLast += szSafeAppendFormat(
                psPrms->psParsePrms->pcOutString,
                psPrms->pcOutStringLast,
                psPrms->psParsePrms->u16OutStrMaxLen,
                PinCfgMessages_getString(FSDSS_E),
                PinCfgMessages_getString(EL_E),
                psPrms->u16LinesProcessed,
                PinCfgMessages_getString(SR_E),
                PinCfgMessages_getString(IVLD_E),
                PinCfgMessages_getString(OFFSET_E));
            return PINCFG_OK_E;
        }
    }

    // Get precision (index 11, optional, decimal places for display/payload sizing)
    uint8_t u8Precision = PINCFG_SENSOR_PRECISION_D; // Default: 0 decimals
    if (bGetOptionalField(psPrms, 11))
    {
        if (PinCfgStr_eAtoU8(&(psPrms->sTempStrPt), &u8Precision) != PINCFG_STR_OK_E ||
            u8Precision > PINCFG_SENSOR_PRECISION_MAX_D)
        {
            psPrms->pcOutStringLast += szSafeAppendFormat(
                psPrms->psParsePrms->pcOutString,
                psPrms->pcOutStringLast,
                psPrms->psParsePrms->u16OutStrMaxLen,
                PinCfgMessages_getString(FSDSSS_E),
                PinCfgMessages_getString(EL_E),
                psPrms->u16LinesProcessed,
                PinCfgMessages_getString(SR_E),
                PinCfgMessages_getString(IVLD_E),
                PinCfgMessages_getString(PRECISION_E));
            return PINCFG_OK_E;
        }
    }

    // Get unit string (index 12, optional, moved before byte extraction)
    STRING_POINT_T sUnit = {NULL, 0};
    if (psPrms->u8LineItemsLen >= 13)
    {
        vGetField(psPrms, 12);
        if (psPrms->sTempStrPt.szLen > PINCFG_SENSOR_UNIT_MAX_LEN_D)
        {
            psPrms->pcOutStringLast += LOG_WARNING(psPrms, PinCfgMessages_getString(SR_E), ERR_INVALID_UNIT);
            return PINCFG_OK_E;
        }
        sUnit = psPrms->sTempStrPt;
    }

    // Get byte offset (index 13, optional, for multi-value sensors)
    uint8_t u8ByteOffset = 0U;
    if (bGetOptionalField(psPrms, 13))
    {
        if (PinCfgStr_eAtoU8(&(psPrms->sTempStrPt), &u8ByteOffset) != PINCFG_STR_OK_E || u8ByteOffset > 5U)
        {
            psPrms->pcOutStringLast += LOG_WARNING(psPrms, PinCfgMessages_getString(SR_E), ERR_INVALID_BYTE_OFFSET);
            return PINCFG_OK_E;
        }
    }

    // Get byte count (index 14, optional, for multi-value sensors)
    uint8_t u8ByteCount = 0U; // 0 means use all bytes from offset
    if (bGetOptionalField(psPrms, 14))
    {
        if (PinCfgStr_eAtoU8(&(psPrms->sTempStrPt), &u8ByteCount) != PINCFG_STR_OK_E || u8ByteCount > 6U)
        {
            psPrms->pcOutStringLast += LOG_WARNING(psPrms, PinCfgMessages_getString(SR_E), ERR_INVALID_BYTE_COUNT);
            return PINCFG_OK_E;
        }
    }

    // Get bit shift (index 15, optional, right shift after extraction)
    uint8_t u8BitShift = 0U; // Default: no shift
    if (bGetOptionalField(psPrms, 15))
    {
        if (PinCfgStr_eAtoU8(&(psPrms->sTempStrPt), &u8BitShift) != PINCFG_STR_OK_E || u8BitShift > 31U)
        {
            psPrms->pcOutStringLast += LOG_WARNING(psPrms, PinCfgMessages_getString(SR_E), ERR_INVALID_ARGS);
            return PINCFG_OK_E;
        }
    }

    // Get bit mask (index 16, optional, AND mask before shift, hex or decimal)
    uint32_t u32BitMask = 0xFFFFFFFF; // Default: no mask
    if (bGetOptionalField(psPrms, 16))
    {
        if (PinCfgStr_eAtoU32(&(psPrms->sTempStrPt), &u32BitMask) != PINCFG_STR_OK_E)
        {
            psPrms->pcOutStringLast += LOG_WARNING(psPrms, PinCfgMessages_getString(SR_E), ERR_INVALID_ARGS);
            return PINCFG_OK_E;
        }
    }

    // Get endianness (index 17, optional, 0=big-endian, 1=little-endian)
    uint8_t u8Endianness = 0U; // Default: big-endian
    if (bGetOptionalField(psPrms, 17))
    {
        if (PinCfgStr_eAtoU8(&(psPrms->sTempStrPt), &u8Endianness) != PINCFG_STR_OK_E || u8Endianness > 1U)
        {
            psPrms->pcOutStringLast += LOG_WARNING(psPrms, PinCfgMessages_getString(SR_E), ERR_INVALID_ARGS);
            return PINCFG_OK_E;
        }
    }

    // Calculate memory
    if (psPrms->psParsePrms->pszMemoryRequired != NULL)
    {
        *(psPrms->psParsePrms->pszMemoryRequired) +=
            Memory_szGetAllocatedSize(sizeof(SENSOR_T)) + Memory_szGetAllocatedSize(sSensorName.szLen + 1);

        // Add unit string memory if provided
        if (sUnit.szLen > 0)
            *(psPrms->psParsePrms->pszMemoryRequired) += Memory_szGetAllocatedSize(sUnit.szLen + 1);

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
    PINCFG_RESULT_T eAllocResult = PINCFG_OK_E;
    SENSOR_T *psSensorHandle = (SENSOR_T *)pvAllocOrOOM(psPrms, sizeof(SENSOR_T), SR_E, &eAllocResult);
    if (eAllocResult != PINCFG_OK_E)
    {
        return eAllocResult;
    }

    // Link sensor to measurement source
    bool bInitOk =
        (Sensor_eInit(
             psSensorHandle,
             &psPrms->u8PresentablesCount,
             (bool)u8Cumulative,
             (bool)u8Enableable,
             &sSensorName,
             (mysensors_data_t)u8VType,
             (mysensors_sensor_t)u8SType,
             psMeasurement, // Link to measurement
             u16SamplingIntervalMs,
             u16ReportIntervalSec,
             i32Scale,                          // Multiplicative scale factor (fixed-point)
             i32Offset,                         // Additive offset adjustment (fixed-point)
             u8Precision,                       // Decimal places (0-6)
             (sUnit.szLen > 0) ? &sUnit : NULL, // Unit string
             u8ByteOffset,                      // Byte extraction start
             u8ByteCount,                       // Byte extraction count
             u8BitShift,                        // Bit shift
             u32BitMask,                        // Bit mask
             u8Endianness) == SENSOR_OK_E);     // Endianness

    // Add main sensor to presentables
    if (bInitOk && psPrms->psParsePrms->eAddToPresentables != NULL)
    {
        bInitOk = (psPrms->psParsePrms->eAddToPresentables((PRESENTABLE_T *)psSensorHandle) == PINCFG_OK_E);
    }

    // Add enableable to presentables if created
    if (bInitOk && psSensorHandle->psEnableable != NULL && psPrms->psParsePrms->eAddToPresentables != NULL)
    {
        bInitOk =
            (psPrms->psParsePrms->eAddToPresentables((PRESENTABLE_T *)psSensorHandle->psEnableable) == PINCFG_OK_E);
    }

    // Add loopable
    if (bInitOk && psPrms->psParsePrms->eAddToLoopables != NULL)
    {
        bInitOk = (psPrms->psParsePrms->eAddToLoopables(&psSensorHandle->sLoopable) == PINCFG_OK_E);
    }

    if (!bInitOk)
    {
        psPrms->pcOutStringLast += szSafeAppendFormat(
            psPrms->psParsePrms->pcOutString,
            psPrms->pcOutStringLast,
            psPrms->psParsePrms->u16OutStrMaxLen,
            PinCfgMessages_getString(FSDSS_E),
            PinCfgMessages_getString(EL_E),
            psPrms->u16LinesProcessed,
            PinCfgMessages_getString(SR_E),
            PinCfgMessages_getString(INITF_E),
            PinCfgMessages_getString(NL_E));
    }

    // Set byte extraction and transformation parameters for multi-value sensors (e.g., AHT10 temp+humidity)
    psSensorHandle->u8DataByteOffset = u8ByteOffset;
    psSensorHandle->u8DataByteCount = u8ByteCount;
    psSensorHandle->u8BitShift = u8BitShift;
    psSensorHandle->u32BitMask = u32BitMask;
    psSensorHandle->u8Endianness = u8Endianness;

    return PINCFG_OK_E;
}

static PINCFG_RESULT_T PinCfgCsv_ParseGlobalConfigItems(PINCFG_PARSE_SUBFN_PARAMS_T *psPrms)
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
    default:
    {
        psPrms->pcOutStringLast += LOG_WARNING(psPrms, "", ERR_INVALID_GLOBAL_CFG);
        return PINCFG_OK_E;
    }
    break;
    }

    if (psPrms->u8LineItemsLen < 2)
    {
        psPrms->pcOutStringLast += LOG_WARNING(psPrms, PinCfgMessages_getString(eItem), ERR_INVALID_ARGS);
        return PINCFG_OK_E;
    }

    psPrms->sTempStrPt = psPrms->sLine;
    PinCfgStr_vGetSplitElemByIndex(&(psPrms->sTempStrPt), PINCFG_VALUE_SEPARATOR_D, 1);
    if (PinCfgStr_eAtoU32(&(psPrms->sTempStrPt), &u32ParsedNumber) != PINCFG_STR_OK_E || u32ParsedNumber < u32Min)
    {
        psPrms->pcOutStringLast += LOG_WARNING(psPrms, PinCfgMessages_getString(eItem), ERR_INVALID_NUMBER);
        return PINCFG_OK_E;
    }

    switch (eItem)
    {
    case IPDMS_E: InPin_SetDebounceMs(u32ParsedNumber); break;
    case IPMCDMS_E: InPin_SetMulticlickMaxDelayMs(u32ParsedNumber); break;
    case SWIDMS_E: Switch_SetImpulseDurationMs(u32ParsedNumber); break;
    case SWFNDMS_E: Switch_SetFbDelayMs(u32ParsedNumber); break;
    default: break;
    }

    return PINCFG_OK_E;
}

static PRESENTABLE_T *PinCfgCsv_psFindInPresentablesById(uint8_t u8Id)
{
    if (u8Id >= psGlobals->u8PresentablesCount)
        return NULL;

    return (psGlobals->ppsPresentables)[u8Id];
}

static ISENSORMEASURE_T *PinCfgCsv_psFindMeasurementByName(
    PINCFG_PARSE_SUBFN_PARAMS_T *psPrms,
    const STRING_POINT_T *psName)
{
    if (psName == NULL || psName->pcStrStart == NULL || psName->szLen == 0)
        return NULL;

    // During parsing: Search linked list only
    // No array conversion - sensors store direct pointers
    LINKEDLIST_ITEM_T *psCurrent = psPrms->psMeasurementsListHead;

    while (psCurrent != NULL)
    {
        // Get the actual measurement from the linked list item
        ISENSORMEASURE_T *psMeasure = (ISENSORMEASURE_T *)psCurrent->pvItem;
        if (psMeasure != NULL && psMeasure->pcName != NULL)
        {
            // Name is now in base interface - much simpler!
            size_t szStoredLen = strlen(psMeasure->pcName);
            if (szStoredLen == psName->szLen && strncmp(psMeasure->pcName, psName->pcStrStart, psName->szLen) == 0)
            {
                return psMeasure;
            }
        }

        psCurrent = (LINKEDLIST_ITEM_T *)psCurrent->pvNext;
    }

    return NULL; // Not found
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

        psLLItem = LinkedList_psGetNext(psLLItem);
        psReturn = NULL;
    }

    return psReturn;
}

size_t szGetSize(size_t a, size_t b)
{
    return (a > b) ? (a - b) : 0;
}

static PINCFG_RESULT_T PinCfgCsv_eAddToLinkedList(LINKEDLIST_ITEM_T **psFirst, void *pvNew)
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
