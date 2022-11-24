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

static inline void PinCfgCsv_vAddToLoopables(LOOPRE_IF_T *psLoopable);
static LOOPRE_IF_T *PinCfgCsv_psFindInLoopablesByName(const STRING_POINT_T *psName);
static inline void PinCfgCsv_vAddToPresentables(LOOPRE_IF_T *psLoopable);
static void PinCfgCsv_vAddToString(
    char *pcOutMsg,
    const uint16_t szOutMsgMaxLen,
    const char *pcMsgToBeAdded,
    int16_t i16Line);
static inline size_t szGetAllocatedSize(size_t szToAllocate);

PINCFG_RESULT_T PinCfgCsv_eInit(uint8_t *pu8Memory, size_t szMemorySize)
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
    LOOPRE_IF_T *psLooPreIfElement;
    TRIGGER_SWITCHACTION_T *pasSwActs = NULL;
    TRIGGER_SWITCHACTION_T *psSwAct;
    TRIGGER_HANDLE_T *psTriggerHnd;
    INPIN_HANDLE_T *psEmiter = NULL;
    STRING_POINT_T sLine;
    STRING_POINT_T sTempStrPt;
    uint8_t u8PresentablesCount = 0;
    uint8_t u8LinesLen;
    uint8_t u8LinesProcessed;
    uint8_t u8Offset;
    uint8_t u8Count;
    uint8_t u8Pin;
    uint8_t i;

    uint8_t u8LineItemsLen;
    uint8_t u8EventType;
    uint8_t u8EventCount;

    uint8_t u8DrivenAction;
    uint8_t u8DrivesCountReal = 0U;

    if (pszMemoryRequired != NULL)
        *pszMemoryRequired = 0;

    if (pcOutString != NULL && u16OutStrMaxLen > 0)
        pcOutString[0] = '\0';

    if (!bValidate && bRemoteConfigEnabled)
    {
        psLooPreIfElement = (LOOPRE_IF_T *)Memory_vpAlloc(sizeof(EXTCFGRECEIVER_HANDLE_T));
        if (psLooPreIfElement == NULL)
        {
            Memory_eReset();
            PinCfgCsv_vAddToString(pcOutString, u16OutStrMaxLen, "E:ExtCfgReceiver: Out of memory.\n", -1);
            return PINCFG_OUTOFMEMORY_ERROR_E;
        }

        if (ExtCfgReceiver_eInit((EXTCFGRECEIVER_HANDLE_T *)psLooPreIfElement, u8PresentablesCount, true) ==
            EXTCFGRECEIVER_OK_E)
        {
            PinCfgCsv_vAddToPresentables(psLooPreIfElement);
            u8PresentablesCount++;
        }
        else
        {
            PinCfgCsv_vAddToString(pcOutString, u16OutStrMaxLen, "W:ExtCfgReceiver: Init failed!\n", -1);
        }
    }
    if (pszMemoryRequired != NULL)
        *pszMemoryRequired += szGetAllocatedSize(sizeof(EXTCFGRECEIVER_HANDLE_T));

    PinCfgStr_vInitStrPoint(&sTempStrPt, psGlobals->acCfgBuf, strlen(psGlobals->acCfgBuf));
    u8LinesLen = (uint8_t)PinCfgStr_szGetSplitCount(&sTempStrPt, '\n');
    if (u8LinesLen == 1 && sTempStrPt.szLen == 0)
    {
        PinCfgCsv_vAddToString(pcOutString, u16OutStrMaxLen, "E:Invalid format. Empty configuration.\n", -1);
        return PINCFG_ERROR_E;
    }

    for (u8LinesProcessed = 0; u8LinesProcessed < u8LinesLen; u8LinesProcessed++)
    {
        PinCfgStr_vInitStrPoint(&sLine, psGlobals->acCfgBuf, strlen(psGlobals->acCfgBuf));
        PinCfgStr_vGetSplitElemByIndex(&sLine, '\n', u8LinesProcessed);
        if (sLine.pcStrStart[0] == '#') // comment continue
            continue;

        u8LineItemsLen = (uint8_t)PinCfgStr_szGetSplitCount(&sLine, ',');
        if (u8LineItemsLen < 2)
        {
            PinCfgCsv_vAddToString(
                pcOutString, u16OutStrMaxLen, "W:L:%d:Not defined or invalid format.\n", (int16_t)u8LinesProcessed);
            continue;
        }

        sTempStrPt = sLine;
        PinCfgStr_vGetSplitElemByIndex(&sTempStrPt, ',', 0);
        // switches
        if (sTempStrPt.szLen == 1 && sTempStrPt.pcStrStart[0] == 'S')
        {
            if (u8LineItemsLen < 3)
            {
                PinCfgCsv_vAddToString(
                    pcOutString,
                    u16OutStrMaxLen,
                    "W:L:%d:Switch: Invalid number of arguments.\n",
                    (int16_t)u8LinesProcessed);
                continue;
            }

            u8Count = (uint8_t)((u8LineItemsLen - 1) % 2);
            if (u8Count != 0)
            {
                PinCfgCsv_vAddToString(
                    pcOutString,
                    u16OutStrMaxLen,
                    "W:L:%d:Switch: Invalid number of items defining names and pins.\n",
                    (int16_t)u8LinesProcessed);
                continue;
            }
            u8Count = (uint8_t)((u8LineItemsLen - 1) / 2);
            for (i = 0; i < u8Count; i++)
            {
                u8Offset = 1 + i * 2;

                sTempStrPt = sLine;
                PinCfgStr_vGetSplitElemByIndex(&sTempStrPt, ',', (u8Offset + 1));
                if (PinCfgStr_eAtoU8(&sTempStrPt, &u8Pin) != PINCFG_STR_OK_E || u8Pin < 1)
                {
                    PinCfgCsv_vAddToString(
                        pcOutString,
                        u16OutStrMaxLen,
                        "W:L:%d:Switch: Invalid pin number.\n",
                        (int16_t)u8LinesProcessed);
                    continue;
                }
                sTempStrPt = sLine;
                PinCfgStr_vGetSplitElemByIndex(&sTempStrPt, ',', u8Offset);
                if (!bValidate)
                {
                    psLooPreIfElement = (LOOPRE_IF_T *)Memory_vpAlloc(sizeof(SWITCH_HANDLE_T));
                    if (psLooPreIfElement == NULL)
                    {
                        Memory_eReset();
                        PinCfgCsv_vAddToString(
                            pcOutString, u16OutStrMaxLen, "E:L:%d:Switch: Out of memory.\n", (int16_t)u8LinesProcessed);
                        return PINCFG_OUTOFMEMORY_ERROR_E;
                    }
                    if (Switch_eInit(
                            (SWITCH_HANDLE_T *)psLooPreIfElement,
                            &sTempStrPt,
                            u8PresentablesCount,
                            true,
                            0U,
                            SWITCH_CLASSIC_E,
                            (uint8_t)u8Pin,
                            0U) == SWITCH_OK_E)
                    {
                        PinCfgCsv_vAddToPresentables(psLooPreIfElement);
                        u8PresentablesCount++;
                        PinCfgCsv_vAddToLoopables(psLooPreIfElement);
                    }
                    else
                    {
                        PinCfgCsv_vAddToString(
                            pcOutString, u16OutStrMaxLen, "W:L:%d:Switch: Init failed!\n", (int16_t)u8LinesProcessed);
                    }
                }
                if (pszMemoryRequired != NULL)
                    *pszMemoryRequired +=
                        szGetAllocatedSize(sizeof(SWITCH_HANDLE_T)) + szGetAllocatedSize(sTempStrPt.szLen + 1);
            }
        }
        // inpins
        else if (sTempStrPt.szLen == 1 && sTempStrPt.pcStrStart[0] == 'I')
        {
            if (u8LineItemsLen < 3)
            {
                PinCfgCsv_vAddToString(
                    pcOutString,
                    u16OutStrMaxLen,
                    "W:L:%d:InPin: Invalid number of arguments.\n",
                    (int16_t)u8LinesProcessed);
                continue;
            }

            u8Count = ((u8LineItemsLen - 1) % 2);
            if (u8Count != 0)
            {
                PinCfgCsv_vAddToString(
                    pcOutString,
                    u16OutStrMaxLen,
                    "W:L:%d:InPin: Invalid number of items defining names and pins.\n",
                    (int16_t)u8LinesProcessed);
                continue;
            }
            u8Count = ((u8LineItemsLen - 1) / 2);
            for (i = 0; i < u8Count; i++)
            {
                u8Offset = 1 + i * 2;
                sTempStrPt = sLine;
                PinCfgStr_vGetSplitElemByIndex(&sTempStrPt, ',', (u8Offset + 1));
                if (PinCfgStr_eAtoU8(&sTempStrPt, &u8Pin) != PINCFG_STR_OK_E || u8Pin < 1)
                {
                    PinCfgCsv_vAddToString(
                        pcOutString, u16OutStrMaxLen, "W:L:%d:InPin: Invalid pin number.\n", (int16_t)u8LinesProcessed);
                    continue;
                }
                sTempStrPt = sLine;
                PinCfgStr_vGetSplitElemByIndex(&sTempStrPt, ',', u8Offset);
                if (!bValidate)
                {
                    psLooPreIfElement = (LOOPRE_IF_T *)Memory_vpAlloc(sizeof(INPIN_HANDLE_T));
                    if (psLooPreIfElement == NULL)
                    {
                        Memory_eReset();
                        PinCfgCsv_vAddToString(
                            pcOutString, u16OutStrMaxLen, "E:L:%d:InPin: Out of memory.\n", (int16_t)u8LinesProcessed);
                        return PINCFG_OUTOFMEMORY_ERROR_E;
                    }
                    if (InPin_eInit(
                            (INPIN_HANDLE_T *)psLooPreIfElement,
                            &sTempStrPt,
                            u8PresentablesCount,
                            true,
                            (uint8_t)u8Pin) == INPIN_OK_E)
                    {
                        PinCfgCsv_vAddToPresentables(psLooPreIfElement);
                        u8PresentablesCount++;
                        PinCfgCsv_vAddToLoopables(psLooPreIfElement);
                    }
                    else
                    {
                        PinCfgCsv_vAddToString(
                            pcOutString, u16OutStrMaxLen, "W:L:%d:InPin: Init failed!\n", (int16_t)u8LinesProcessed);
                    }
                }
                if (pszMemoryRequired != NULL)
                    *pszMemoryRequired +=
                        szGetAllocatedSize(sizeof(INPIN_HANDLE_T)) + szGetAllocatedSize(sTempStrPt.szLen + 1);
            }
        }
        // triggers
        else if (sTempStrPt.szLen == 1 && sTempStrPt.pcStrStart[0] == 'T')
        {
            if (u8LineItemsLen < 7)
            {
                PinCfgCsv_vAddToString(
                    pcOutString,
                    u16OutStrMaxLen,
                    "W:L:%d:Trigger: Invalid number of arguments.\n",
                    (int16_t)u8LinesProcessed);
                continue;
            }

            sTempStrPt = sLine;
            PinCfgStr_vGetSplitElemByIndex(&sTempStrPt, ',', 2);
            psEmiter = (INPIN_HANDLE_T *)PinCfgCsv_psFindInLoopablesByName(&sTempStrPt);
            if (psEmiter == NULL)
            {
                PinCfgCsv_vAddToString(
                    pcOutString, u16OutStrMaxLen, "W:L:%d:Trigger: Invalid InPin name.\n", (int16_t)u8LinesProcessed);
                continue;
            }

            sTempStrPt = sLine;
            PinCfgStr_vGetSplitElemByIndex(&sTempStrPt, ',', 3);
            if (PinCfgStr_eAtoU8(&sTempStrPt, &u8EventType) != PINCFG_STR_OK_E || u8EventType > 3)
            {
                PinCfgCsv_vAddToString(
                    pcOutString,
                    u16OutStrMaxLen,
                    "W:L:%d:Trigger: Invalid format for event type.\n",
                    (int16_t)u8LinesProcessed);
                continue;
            }

            sTempStrPt = sLine;
            PinCfgStr_vGetSplitElemByIndex(&sTempStrPt, ',', 4);
            if (PinCfgStr_eAtoU8(&sTempStrPt, &u8EventCount) != PINCFG_STR_OK_E)
            {
                PinCfgCsv_vAddToString(
                    pcOutString,
                    u16OutStrMaxLen,
                    "W:L:%d:Trigger: Invalid format for event count.\n",
                    u8LinesProcessed);
                continue;
            }

            sTempStrPt = sLine;
            u8Count = (uint8_t)((u8LineItemsLen - 5) / 2);
            for (i = 0; i < u8Count; i++)
            {
                u8Offset = 5 + i * 2;
                sTempStrPt = sLine;
                PinCfgStr_vGetSplitElemByIndex(&sTempStrPt, ',', u8Offset);
                psLooPreIfElement = PinCfgCsv_psFindInLoopablesByName(&sTempStrPt);
                if (psLooPreIfElement == NULL)
                {
                    PinCfgCsv_vAddToString(
                        pcOutString,
                        u16OutStrMaxLen,
                        "W:L:%d:Trigger: Invalid switch name.\n",
                        (int16_t)u8LinesProcessed);
                    continue;
                }

                sTempStrPt = sLine;
                PinCfgStr_vGetSplitElemByIndex(&sTempStrPt, ',', (u8Offset + 1));
                if (PinCfgStr_eAtoU8(&sTempStrPt, &u8DrivenAction) != PINCFG_STR_OK_E || u8DrivenAction > 2)
                {
                    PinCfgCsv_vAddToString(
                        pcOutString,
                        u16OutStrMaxLen,
                        "W:L:%d:Trigger: Invalid switch action.\n",
                        (int16_t)u8LinesProcessed);
                    continue;
                }
                u8DrivesCountReal++;

                if (!bValidate)
                {
                    psSwAct = (TRIGGER_SWITCHACTION_T *)Memory_vpAlloc(sizeof(TRIGGER_SWITCHACTION_T));
                    if (psSwAct == NULL)
                    {
                        Memory_eReset();
                        PinCfgCsv_vAddToString(
                            pcOutString,
                            u16OutStrMaxLen,
                            "E:L:%d:Trigger: Switchaction: Out of memory.\n",
                            u8LinesProcessed);
                        return PINCFG_OUTOFMEMORY_ERROR_E;
                    }
                    psSwAct->psSwitchHnd = (SWITCH_HANDLE_T *)psLooPreIfElement;
                    psSwAct->eAction = (TRIGGER_ACTION_T)u8DrivenAction;
                    if (pasSwActs == NULL)
                    {
                        pasSwActs = psSwAct;
                    }
                }
                if (pszMemoryRequired != NULL)
                    *pszMemoryRequired += szGetAllocatedSize(sizeof(TRIGGER_SWITCHACTION_T));
            }
            if (u8DrivesCountReal == 0U || pasSwActs == NULL)
            {
                PinCfgCsv_vAddToString(
                    pcOutString, u16OutStrMaxLen, "W:L:%d:Trigger: Nothing to drive.\n", (int16_t)u8LinesProcessed);
                continue;
            }

            if (!bValidate)
            {
                psTriggerHnd = (TRIGGER_HANDLE_T *)Memory_vpAlloc(sizeof(TRIGGER_HANDLE_T));
                if (psTriggerHnd == NULL)
                {
                    Memory_eReset();
                    PinCfgCsv_vAddToString(
                        pcOutString, u16OutStrMaxLen, "E:L:%d:Trigger: Out of memory.\n", (int16_t)u8LinesProcessed);
                    return PINCFG_OUTOFMEMORY_ERROR_E;
                }
                if (Trigger_eInit(
                        psTriggerHnd,
                        pasSwActs,
                        u8DrivesCountReal,
                        (TRIGGER_EVENTTYPE_T)u8EventType,
                        (uint8_t)u8EventCount) == TRIGGER_OK_E)
                {
                    InPin_eAddSubscriber(psEmiter, (PINSUBSCRIBER_IF_T *)psTriggerHnd);
                }
                else
                {
                    PinCfgCsv_vAddToString(
                        pcOutString, u16OutStrMaxLen, "W:L:%d:Trigger: Init failed!\n", (int16_t)u8LinesProcessed);
                }
            }
            if (pszMemoryRequired != NULL)
                *pszMemoryRequired += szGetAllocatedSize(sizeof(TRIGGER_HANDLE_T));
        }
        else
        {
            PinCfgCsv_vAddToString(pcOutString, u16OutStrMaxLen, "W:L:%d:Unknown type.\n", (int16_t)u8LinesProcessed);
        }
    }

    PinCfgCsv_vAddToString(pcOutString, u16OutStrMaxLen, "I: Configuration parsed.\n", -1);

    return PINCFG_OK_E;
}

PINCFG_RESULT_T PinCfgCsv_eValidate(size_t *pszMemoryRequired, char *pcOutString, const uint16_t u16OutStrMaxLen)
{
    return PinCfgCsv_eParse(pszMemoryRequired, pcOutString, u16OutStrMaxLen, true, false);
}

#ifdef MY_CONTROLLER_HA
static bool bInitialValueSent;
#endif

void PinCfgCsv_vLoop(uint32_t u32ms)
{
    if (psGlobals == NULL)
        return;

#ifdef MY_CONTROLLER_HA
    if (!bInitialValueSent)
    {
        LOOPRE_IF_T *psCurrentPresentable = psGlobals->psPresentablesFirst;
        while (psCurrentPresentable->psNextPresentable != NULL)
        {
            LooPreIf_vPresentState(psCurrentPresentable);
            psCurrentPresentable = psCurrentPresentable->psNextPresentable;
        }
    }
#endif

    LOOPRE_IF_T *psCurrent = psGlobals->psLoopablesFirst;
    while (psCurrent->psNextLoopable != NULL)
    {
        LooPreIf_vLoop(psCurrent, u32ms);
        psCurrent = psCurrent->psNextLoopable;
    }
}

void PinCfgCsv_vPresentation(void)
{
    if (psGlobals == NULL)
        return;

    LOOPRE_IF_T *psCurrent = psGlobals->psPresentablesFirst;
    while (psCurrent->psNextPresentable != NULL)
    {
        LooPreIf_vPresent(psCurrent);
        psCurrent = psCurrent->psNextPresentable;
    }
}

void PinCfgCfg_vReceive(const uint8_t u8Id, void *pvMsgData)
{
    if (psGlobals == NULL)
        return;

#ifdef MY_CONTROLLER_HA
    if (!bInitialValueSent)
    {
        bInitialValueSent = true;
    }
#endif

    LOOPRE_IF_T *psCurrent = psGlobals->psPresentablesFirst;
    while (psCurrent->psNextPresentable != NULL)
    {
        if (LooPreIf_u8GetId(psCurrent) == u8Id)
        {
            LooPreIf_vRcvMessage(psCurrent, pvMsgData);
            break;
        }
        psCurrent = psCurrent->psNextPresentable;
    }
}

// private
static inline void PinCfgCsv_vAddToLoopables(LOOPRE_IF_T *psLoopable)
{
    if (psGlobals->psLoopablesFirst == NULL)
        psGlobals->psLoopablesFirst = psLoopable;
    else
    {
        LOOPRE_IF_T *psCurrent = psGlobals->psLoopablesFirst;
        while (psCurrent->psNextLoopable != NULL)
        {
            psCurrent = psCurrent->psNextLoopable;
        }
        psCurrent->psNextLoopable = psLoopable;
    }
}

static LOOPRE_IF_T *PinCfgCsv_psFindInLoopablesByName(const STRING_POINT_T *psName)
{
    LOOPRE_IF_T *psReturn = NULL;
    char *pcTempStr = (char *)Memory_vpTempAlloc(psName->szLen + 1);

    if (pcTempStr != NULL)
    {
        memcpy(pcTempStr, psName->pcStrStart, psName->szLen);
        pcTempStr[psName->szLen] = '\0';
        psReturn = psGlobals->psLoopablesFirst;
        while (psReturn != NULL &&
               strcmp(MySensorsPresent_pcGetName((MYSENSORSPRESENT_HANDLE_T *)psReturn), pcTempStr) != 0)
        {
            psReturn = psReturn->psNextLoopable;
        }
    }
    Memory_vTempFree();

    return psReturn;
}

static inline void PinCfgCsv_vAddToPresentables(LOOPRE_IF_T *psPresentable)
{
    if (psGlobals->psPresentablesFirst == NULL)
        psGlobals->psPresentablesFirst = psPresentable;
    else
    {
        LOOPRE_IF_T *psCurrent = psGlobals->psPresentablesFirst;
        while (psCurrent->psNextPresentable != NULL)
        {
            psCurrent = psCurrent->psNextPresentable;
        }
        psCurrent->psNextPresentable = psPresentable;
    }
}

static void PinCfgCsv_vAddToString(
    char *pcOutMsg,
    const uint16_t szOutMsgMaxLen,
    const char *pcMsgToBeAdded,
    int16_t i16Line)
{
    if (pcOutMsg == NULL || szOutMsgMaxLen == 0)
        return;

    size_t szBufNeeded = 0;
    if (i16Line >= 0)
        szBufNeeded = snprintf(NULL, 0, pcMsgToBeAdded, i16Line) + 1;

    if ((int)(szBufNeeded + strlen(pcOutMsg)) > (szOutMsgMaxLen - 1))
        return;

    if (i16Line >= 0)
    {
        char *pcFormattedToBeAdded = Memory_vpTempAlloc(szBufNeeded);
        if (pcFormattedToBeAdded == NULL)
            return;
        snprintf(pcFormattedToBeAdded, szBufNeeded, pcMsgToBeAdded, i16Line);

        strcat(pcOutMsg, pcFormattedToBeAdded);
        Memory_vTempFree();
    }
    else
        strcat(pcOutMsg, pcMsgToBeAdded);
}

static inline size_t szGetAllocatedSize(size_t szToAllocate)
{
    size_t szReturn = szToAllocate / sizeof(char *);
    if (szToAllocate % sizeof(char *) > 0)
        szReturn++;
    szReturn *= sizeof(char *);

    return szReturn;
}
