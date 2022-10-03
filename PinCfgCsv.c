#include <string.h>

#include "ExtCfgReceiver.h"
#include "ExternalInterfaces.h"
#include "InPin.h"
#include "Memory.h"
#include "PinCfgCsv.h"
#include "PinCfgStr.h"
#include "Switch.h"
#include "Trigger.h"

#ifndef PINCFG_PARSE_MAX_STRPTS_D
#define PINCFG_PARSE_MAX_STRPTS_D 50
#endif

static const char *extCfgName = "cfgReceviver";
static LOOPRE_IF_T *psLoopablesFirst;
static LOOPRE_IF_T *psPresentablesFirst;

static inline void PinCfgCsv_vAddToLoopables(LOOPRE_IF_T *psLoopable);
static inline void PinCfgCsv_vAddToPresentables(LOOPRE_IF_T *psLoopable);
static inline void PinCfgCsv_vAddToString(const char *pcMsgToBeAdded, char *pcOutMsg, const size_t szOutMsgMaxLen);

PINCFG_RESULT_T PinCfgCsv_eParse(
    const char *pcCfgBuf,
    char *pcOutString,
    const uint16_t u16OutStrMaxLen,
    const bool bValidate,
    uint8_t *pcMemory,
    size_t szMemorySize,
    const bool bRemoteConfigEnabled,
    MYSENOSRS_IF_T *psMySensorsIf,
    PIN_IF_T *psPinIf)
{
    if (pcCfgBuf == NULL || pcOutString == NULL ||
        (!bValidate && (psMySensorsIf == NULL || psMySensorsIf->bRequest == NULL || psMySensorsIf->bPresent == NULL ||
                        psMySensorsIf->bSend == NULL || psPinIf == NULL || psPinIf->vPinMode == NULL ||
                        psPinIf->u8ReadPin == NULL || psPinIf->vWritePin == NULL)))
    {
        return PINCFG_NULLPTR_ERROR_E;
    }
    // external interfaces init
    psPinCfg_MySensorsIf = psMySensorsIf;
    psPinCfg_PinIf = psPinIf;

    // Memory init
    if (!bValidate)
    {
        if (pcMemory == NULL)
        {
            PinCfgCsv_vAddToString("E: Memory: Null pointer to memory!\n", pcOutString, (size_t)u16OutStrMaxLen);
            return PINCFG_NULLPTR_ERROR_E;
        }
        if (Memory_eInit((uint8_t *)pcMemory, szMemorySize) != MEMORY_OK_E)
        {
            PinCfgCsv_vAddToString("E: Memory: Init error!\n", pcOutString, (size_t)u16OutStrMaxLen);
            return PINCFG_MEMORYINIT_ERROR_E;
        }
    }

    uint8_t u8PresentablesCount = 0;
    pcOutString[0] = '\0';
    if (!bValidate && bRemoteConfigEnabled)
    {
        EXTCFGRECEIVER_HANDLE_T *psExtCfgReciver =
            (EXTCFGRECEIVER_HANDLE_T *)Memory_vpAlloc(sizeof(EXTCFGRECEIVER_HANDLE_T));
        if (psExtCfgReciver == NULL)
        {
            PinCfgCsv_vAddToString("E: ExtCfgReceiver: Out of memory.\n", pcOutString, (size_t)u16OutStrMaxLen);
            return PINCFG_OUTOFMEMORY_ERROR_E;
        }

        STRING_POINT_T sName = {extCfgName, sizeof(extCfgName)};
        if (ExtCfgReceiver_eInit(psExtCfgReciver, &sName, u8PresentablesCount, true) == EXTCFGRECEIVER_OK_E)
        {
            PinCfgCsv_vAddToPresentables((LOOPRE_IF_T *)psExtCfgReciver);
            u8PresentablesCount++;
        }
        else
        {
            PinCfgCsv_vAddToString("W: ExtCfgReceiver: Init failed!\n", pcOutString, (size_t)u16OutStrMaxLen);
        }
    }
    STRING_POINT_T sTempStrPt;
    PinCfgStr_vInitStrPoint(&sTempStrPt, pcCfgBuf, strlen(pcCfgBuf));
    uint8_t u8LinesLen = (uint8_t)PinCfgStr_szGetSplitCount(&sTempStrPt, '\n');
    if (u8LinesLen == 0)
    {
        PinCfgCsv_vAddToString("E: Invalid format.\n", pcOutString, (size_t)u16OutStrMaxLen);
        return PINCFG_ERROR_E;
    }
    STRING_POINT_T sLine;

    for (uint8_t u8LinesProcessed = 0; u8LinesProcessed < u8LinesLen; u8LinesProcessed++)
    {
        PinCfgStr_vInitStrPoint(&sLine, pcCfgBuf, strlen(pcCfgBuf));
        PinCfgStr_vGetSplitElemByIndex(&sLine, '\n', u8LinesProcessed);
        if (sLine.pcStrStart[0] == '#') // comment continue
            continue;

        uint8_t u8LineItemsLen = (uint8_t)PinCfgStr_szGetSplitCount(&sLine, ',');
        if (u8LineItemsLen < 2)
        {
            PinCfgCsv_vAddToString("W: Not defined or invalid format.\n", pcOutString, (size_t)u16OutStrMaxLen);
            continue;
        }

        sTempStrPt = sLine;
        PinCfgStr_vGetSplitElemByIndex(&sTempStrPt, ',', 0);
        // switches
        if (sTempStrPt.szLen == 1 && sTempStrPt.pcStrStart[0] == 'S')
        {
            sTempStrPt = sLine;
            PinCfgStr_vGetSplitElemByIndex(&sTempStrPt, ',', 1);
            uint8_t u8SwitchesCount;
            if (PinCfgStr_eAtoU8(&sTempStrPt, &u8SwitchesCount) != PINCFG_STR_OK_E || u8SwitchesCount < 0)
            {
                PinCfgCsv_vAddToString("W: Switch: Invalid format for count.\n", pcOutString, (size_t)u16OutStrMaxLen);
                continue;
            }
            if (((u8LineItemsLen - 2) / 2) != u8SwitchesCount)
            {
                PinCfgCsv_vAddToString(
                    "W: Switch: Invalid number of items defining names and pins.\n",
                    pcOutString,
                    (size_t)u16OutStrMaxLen);
                continue;
            }
            for (uint8_t i = 0; i < u8SwitchesCount; i++)
            {
                uint8_t u8NumberOffset = 2 + i * 2 + 1;

                sTempStrPt = sLine;
                PinCfgStr_vGetSplitElemByIndex(&sTempStrPt, ',', u8NumberOffset);
                uint8_t u8Pin;
                if (PinCfgStr_eAtoU8(&sTempStrPt, &u8Pin) != PINCFG_STR_OK_E || u8Pin < 1)
                {
                    PinCfgCsv_vAddToString("W: Switch: Invalid pin number.\n", pcOutString, (size_t)u16OutStrMaxLen);
                    continue;
                }
                if (!bValidate)
                {
                    SWITCH_HANDLE_T *psSwHnd = (SWITCH_HANDLE_T *)Memory_vpAlloc(sizeof(SWITCH_HANDLE_T));
                    if (psSwHnd == NULL)
                    {
                        PinCfgCsv_vAddToString("E: Switch: Out of memory.\n", pcOutString, (size_t)u16OutStrMaxLen);
                        return PINCFG_OUTOFMEMORY_ERROR_E;
                    }
                    sTempStrPt = sLine;
                    PinCfgStr_vGetSplitElemByIndex(&sTempStrPt, ',', (u8NumberOffset - 1));
                    if (Switch_eInit(
                            psSwHnd,
                            &sTempStrPt,
                            u8PresentablesCount,
                            true,
                            0U,
                            SWITCH_CLASSIC_E,
                            (uint8_t)u8Pin,
                            0U) == SWITCH_OK_E)
                    {
                        PinCfgCsv_vAddToPresentables((LOOPRE_IF_T *)psSwHnd);
                        u8PresentablesCount++;
                        PinCfgCsv_vAddToLoopables((LOOPRE_IF_T *)psSwHnd);
                    }
                    else
                    {
                        PinCfgCsv_vAddToString("W: Switch: Init failed!\n", pcOutString, (size_t)u16OutStrMaxLen);
                    }
                }
            }
        }
        // inpins
        else if (sTempStrPt.szLen == 1 && sTempStrPt.pcStrStart[0] == 'I')
        {
            sTempStrPt = sLine;
            PinCfgStr_vGetSplitElemByIndex(&sTempStrPt, ',', 1);
            uint8_t u8InPinsCount;
            if (PinCfgStr_eAtoU8(&sTempStrPt, &u8InPinsCount) != PINCFG_STR_OK_E || u8InPinsCount < 0)
            {
                PinCfgCsv_vAddToString("W: InPin: Invalid format for count.\n", pcOutString, (size_t)u16OutStrMaxLen);
                continue;
            }
            if (((u8LineItemsLen - 2) / 2) != u8InPinsCount)
            {
                PinCfgCsv_vAddToString(
                    "W: InPin: Invalid number of items defining names and pins.\n",
                    pcOutString,
                    (size_t)u16OutStrMaxLen);
                continue;
            }
            for (uint8_t i = 0; i < u8InPinsCount; i++)
            {
                uint8_t u8NumberOffset = 2 + i * 2 + 1;
                sTempStrPt = sLine;
                PinCfgStr_vGetSplitElemByIndex(&sTempStrPt, ',', u8NumberOffset);
                uint8_t u8Pin;
                if (PinCfgStr_eAtoU8(&sTempStrPt, &u8Pin) != PINCFG_STR_OK_E || u8Pin < 1)
                {
                    PinCfgCsv_vAddToString("W: InPin: Invalid pin number.\n", pcOutString, (size_t)u16OutStrMaxLen);
                    continue;
                }
                if (!bValidate)
                {
                    INPIN_HANDLE_T *psInPinHnd = (INPIN_HANDLE_T *)Memory_vpAlloc(sizeof(INPIN_HANDLE_T));
                    if (psInPinHnd == NULL)
                    {
                        PinCfgCsv_vAddToString("E: InPin: Out of memory.\n", pcOutString, (size_t)u16OutStrMaxLen);
                        return PINCFG_OUTOFMEMORY_ERROR_E;
                    }
                    sTempStrPt = sLine;
                    PinCfgStr_vGetSplitElemByIndex(&sTempStrPt, ',', (u8NumberOffset - 1));
                    if (InPin_eInit(psInPinHnd, &sTempStrPt, u8PresentablesCount, true, (uint8_t)u8Pin) == INPIN_OK_E)
                    {
                        PinCfgCsv_vAddToPresentables((LOOPRE_IF_T *)psInPinHnd);
                        u8PresentablesCount++;
                        PinCfgCsv_vAddToLoopables((LOOPRE_IF_T *)psInPinHnd);
                    }
                    else
                    {
                        PinCfgCsv_vAddToString("W: InPin: Init failed!\n", pcOutString, (size_t)u16OutStrMaxLen);
                    }
                }
            }
        }
        // triggers
        else if (sTempStrPt.szLen == 1 && sTempStrPt.pcStrStart[0] == 'T')
        {
            INPIN_HANDLE_T *psEmiter = NULL;
            char cTempStr[20];
            sTempStrPt = sLine;
            PinCfgStr_vGetSplitElemByIndex(&sTempStrPt, ',', 2);
            memcpy(cTempStr, sTempStrPt.pcStrStart, sTempStrPt.szLen);
            cTempStr[sTempStrPt.szLen] = '\0';
            LOOPRE_IF_T *psCurrent = psLoopablesFirst;
            while (psCurrent != NULL)
            {
                if (strcmp(MySensorsPresent_pcGetName((MYSENSORSPRESENT_HANDLE_T *)psCurrent), cTempStr) == 0)
                {
                    psEmiter = (INPIN_HANDLE_T *)psCurrent;
                    break;
                }
                psCurrent = psCurrent->psNextLoopable;
            }
            if (psEmiter == NULL)
            {
                PinCfgCsv_vAddToString("W: Trigger: Invalid InPin name.\n", pcOutString, (size_t)u16OutStrMaxLen);
                continue;
            }

            sTempStrPt = sLine;
            PinCfgStr_vGetSplitElemByIndex(&sTempStrPt, ',', 3);
            uint8_t u8EventType;
            if (PinCfgStr_eAtoU8(&sTempStrPt, &u8EventType) != PINCFG_STR_OK_E || u8EventType > 3)
            {
                PinCfgCsv_vAddToString(
                    "W: Trigger: Invalid format for event type.\n", pcOutString, (size_t)u16OutStrMaxLen);
                continue;
            }

            sTempStrPt = sLine;
            PinCfgStr_vGetSplitElemByIndex(&sTempStrPt, ',', 4);
            uint8_t u8EventCount;
            if (PinCfgStr_eAtoU8(&sTempStrPt, &u8EventCount) != PINCFG_STR_OK_E)
            {
                PinCfgCsv_vAddToString(
                    "W: Trigger: Invalid format for event count.\n", pcOutString, (size_t)u16OutStrMaxLen);
                continue;
            }

            sTempStrPt = sLine;
            PinCfgStr_vGetSplitElemByIndex(&sTempStrPt, ',', 5);
            uint8_t u8DrivesCount;
            if (PinCfgStr_eAtoU8(&sTempStrPt, &u8DrivesCount) != PINCFG_STR_OK_E)
            {
                PinCfgCsv_vAddToString(
                    "W: Trigger: Invalid format for drives count.\n", pcOutString, (size_t)u16OutStrMaxLen);
                continue;
            }

            TRIGGER_SWITCHACTION_T *pasSwActs = NULL;
            uint8_t u8DrivesCountReal = 0U;
            for (uint8_t i = 0; i < u8DrivesCount; i++)
            {
                uint8_t u8NameOffset = 6 + i * 2;
                SWITCH_HANDLE_T *psDriven = NULL;
                sTempStrPt = sLine;
                PinCfgStr_vGetSplitElemByIndex(&sTempStrPt, ',', u8NameOffset);
                memcpy(cTempStr, sTempStrPt.pcStrStart, sTempStrPt.szLen);
                cTempStr[sTempStrPt.szLen] = '\0';
                psCurrent = psLoopablesFirst;
                while (psCurrent != NULL)
                {
                    if (strcmp(MySensorsPresent_pcGetName((MYSENSORSPRESENT_HANDLE_T *)psCurrent), cTempStr) == 0)
                    {
                        psDriven = (SWITCH_HANDLE_T *)psCurrent;
                        break;
                    }
                    psCurrent = psCurrent->psNextLoopable;
                }
                if (psDriven == NULL)
                {
                    PinCfgCsv_vAddToString("W: Trigger: Invalid switch name.\n", pcOutString, (size_t)u16OutStrMaxLen);
                    continue;
                }

                sTempStrPt = sLine;
                PinCfgStr_vGetSplitElemByIndex(&sTempStrPt, ',', (u8NameOffset + 1));
                uint8_t u8DrivenAction;
                if (PinCfgStr_eAtoU8(&sTempStrPt, &u8DrivenAction) != PINCFG_STR_OK_E || u8DrivenAction > 2)
                {
                    PinCfgCsv_vAddToString(
                        "W: Trigger: Invalid switch action.\n", pcOutString, (size_t)u16OutStrMaxLen);
                    continue;
                }
                u8DrivesCountReal++;

                if (!bValidate)
                {
                    TRIGGER_SWITCHACTION_T *psSwAct =
                        (TRIGGER_SWITCHACTION_T *)Memory_vpAlloc(sizeof(TRIGGER_SWITCHACTION_T));
                    if (psSwAct == NULL)
                    {
                        PinCfgCsv_vAddToString(
                            "E: Trigger: Switchaction: Out of memory.\n", pcOutString, (size_t)u16OutStrMaxLen);
                        return PINCFG_OUTOFMEMORY_ERROR_E;
                    }
                    psSwAct->psSwitchHnd = psDriven;
                    psSwAct->eAction = (TRIGGER_ACTION_T)u8DrivenAction;
                    if (pasSwActs == NULL)
                    {
                        pasSwActs = psSwAct;
                    }
                }
            }
            if (u8DrivesCountReal == 0U || pasSwActs == NULL)
            {
                PinCfgCsv_vAddToString("W: Trigger: Nothing to drive.\n", pcOutString, (size_t)u16OutStrMaxLen);
                continue;
            }
            if (!bValidate)
            {
                TRIGGER_HANDLE_T *psTriggerHnd = (TRIGGER_HANDLE_T *)Memory_vpAlloc(sizeof(TRIGGER_HANDLE_T));
                if (psTriggerHnd == NULL)
                {
                    PinCfgCsv_vAddToString(
                        "E: Trigger: Out of memory.\n", pcOutString, (size_t)u16OutStrMaxLen);
                    return PINCFG_OUTOFMEMORY_ERROR_E;
                }
                if (Trigger_eInit(
                    psTriggerHnd,
                    pasSwActs,
                    u8DrivesCountReal,
                    (TRIGGER_EVENTTYPE_T)u8EventType,
                    (uint8_t)u8EventCount) != TRIGGER_OK_E)
                {
                    PinCfgCsv_vAddToString("W: Trigger: Init failed!\n", pcOutString, (size_t)u16OutStrMaxLen);
                }
            }
        }
        else
        {
            PinCfgCsv_vAddToString("W: Unknown type.\n", pcOutString, (size_t)u16OutStrMaxLen);
        }
    }

    return PINCFG_OK_E;
}

// private
static inline void PinCfgCsv_vAddToLoopables(LOOPRE_IF_T *psLoopable)
{
    if (psLoopablesFirst == NULL)
        psLoopablesFirst = psLoopable;
    else
    {
        LOOPRE_IF_T *psCurrent = psLoopablesFirst;
        while (psCurrent->psNextLoopable != NULL)
        {
            psCurrent = psCurrent->psNextLoopable;
        }
        psCurrent->psNextLoopable = psLoopable;
    }
}

static inline void PinCfgCsv_vAddToPresentables(LOOPRE_IF_T *psPresentable)
{
    if (psPresentablesFirst == NULL)
        psPresentablesFirst = psPresentable;
    else
    {
        LOOPRE_IF_T *psCurrent = psPresentablesFirst;
        while (psCurrent->psNextPresentable != NULL)
        {
            psCurrent = psCurrent->psNextPresentable;
        }
        psCurrent->psNextPresentable = psPresentable;
    }
}

static inline void PinCfgCsv_vAddToString(const char *pcMsgToBeAdded, char *pcOutMsg, const size_t szOutMsgMaxLen)
{
    if ((strlen(pcMsgToBeAdded) + strlen(pcOutMsg)) > (szOutMsgMaxLen - 1))
        return;

    strcat(pcOutMsg, pcMsgToBeAdded);
}
