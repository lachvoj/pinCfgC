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
static inline void PinCfgCsv_vAddToString(const char *pcMsgToBeAdded, char *pcOutMsg, const size_t szOutMsgMaxLen);

PINCFG_RESULT_T PinCfgCsv_eParse(
    const char *pcCfgBuf,
    const bool bValidate,
    const bool bRemoteConfigEnabled,
    LOOPRE_IF_T **apsLoopables,
    uint8_t *pu8LoopablesLen,
    const uint8_t u8LoopablesMaxLen,
    LOOPRE_IF_T **apsPresentables,
    uint8_t *pu8PresentablesLen,
    const uint8_t u8PresentablesMaxLen,
    MYSENOSRS_IF_T *psMySensorsIf,
    PIN_IF_T *psPinIf,
    char *pcOutString,
    const uint16_t u16OutStrMaxLen)
{
    if (pcCfgBuf == NULL || pcOutString == NULL ||
        (!bValidate && (apsLoopables == NULL || pu8LoopablesLen == NULL || apsPresentables == NULL ||
                        pu8PresentablesLen == NULL || psMySensorsIf == NULL || psMySensorsIf->bRequest == NULL ||
                        psMySensorsIf->bPresent == NULL || psMySensorsIf->bSend == NULL || psPinIf == NULL ||
                        psPinIf->vPinMode == NULL || psPinIf->u8ReadPin == NULL || psPinIf->vWritePin == NULL)))
    {
        return PINCFG_NULLPTR_ERROR_E;
    }
    // external interfaces init
    psPinCfg_MySensorsIf = psMySensorsIf;
    psPinCfg_PinIf = psPinIf;

    *pu8LoopablesLen = 0;
    *pu8PresentablesLen = 0;
    pcOutString[0] = '\0';
    if (!bValidate && bRemoteConfigEnabled)
    {
        EXTCFGRECEIVER_HANDLE_T *psExtCfgReciver =
            (EXTCFGRECEIVER_HANDLE_T *)Memory_vpAlloc(sizeof(EXTCFGRECEIVER_HANDLE_T));

        STRING_POINT_T sName = {extCfgName, sizeof(extCfgName)};
        ExtCfgReceiver_eInit(psExtCfgReciver, &sName, *pu8PresentablesLen, true);
        apsPresentables[*pu8PresentablesLen] = (LOOPRE_IF_T *)psExtCfgReciver;
        (*pu8PresentablesLen)++;
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

        // switches
        sTempStrPt = sLine;
        PinCfgStr_vGetSplitElemByIndex(&sTempStrPt, ',', 0);
        if (sTempStrPt.szLen == 1 && sTempStrPt.pcStrStart[0] == 'S')
        {
            sTempStrPt = sLine;
            PinCfgStr_vGetSplitElemByIndex(&sTempStrPt, ',', 1);
            uint8_t u8SwitchesCount;
            if (PinCfgStr_eAtoi(&sTempStrPt, (int *)&u8SwitchesCount) != PINCFG_STR_OK_E || u8SwitchesCount < 0)
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
                if (PinCfgStr_eAtoi(&sTempStrPt, (int *)&u8Pin) != PINCFG_STR_OK_E || u8Pin < 1)
                {
                    PinCfgCsv_vAddToString("W: Switch: Invalid pin number.\n", pcOutString, (size_t)u16OutStrMaxLen);
                    continue;
                }
                if (!bValidate)
                {
                    SWITCH_HANDLE_T *psSwHnd = (SWITCH_HANDLE_T *)Memory_vpAlloc(sizeof(SWITCH_HANDLE_T));
                    if (psSwHnd == NULL)
                    {
                        PinCfgCsv_vAddToString("W: Switch: Out of memory.\n", pcOutString, (size_t)u16OutStrMaxLen);
                        return PINCFG_OUTOFMEMORY_ERROR_E;
                    }
                    sTempStrPt = sLine;
                    PinCfgStr_vGetSplitElemByIndex(&sTempStrPt, ',', (u8NumberOffset - 1));
                    Switch_eInit(
                        psSwHnd, &sTempStrPt, *pu8PresentablesLen, true, 0U, SWITCH_CLASSIC_E, (uint8_t)u8Pin, 0U);
                    apsPresentables[*pu8PresentablesLen] = (LOOPRE_IF_T *)psSwHnd;
                    (*pu8PresentablesLen)++;
                    apsLoopables[*pu8LoopablesLen] = (LOOPRE_IF_T *)psSwHnd;
                    (*pu8LoopablesLen)++;
                }
            }
        }
        // inpins
        else if (sTempStrPt.szLen == 1 && sTempStrPt.pcStrStart[0] == 'I')
        {
            sTempStrPt = sLine;
            PinCfgStr_vGetSplitElemByIndex(&sTempStrPt, ',', 1);
            uint8_t u8InPinsCount;
            if (PinCfgStr_eAtoi(&sTempStrPt, (int *)&u8InPinsCount) != PINCFG_STR_OK_E || u8InPinsCount < 0)
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
                if (PinCfgStr_eAtoi(&sTempStrPt, (int *)&u8Pin) != PINCFG_STR_OK_E || u8Pin < 1)
                {
                    PinCfgCsv_vAddToString("W: InPin: Invalid pin number.\n", pcOutString, (size_t)u16OutStrMaxLen);
                    continue;
                }
                if (!bValidate)
                {
                    INPIN_HANDLE_T *psInPinHnd = (INPIN_HANDLE_T *)Memory_vpAlloc(sizeof(INPIN_HANDLE_T));
                    if (psInPinHnd == NULL)
                    {
                        PinCfgCsv_vAddToString("W: InPin: Out of memory.\n", pcOutString, (size_t)u16OutStrMaxLen);
                        return PINCFG_OUTOFMEMORY_ERROR_E;
                    }
                    sTempStrPt = sLine;
                    PinCfgStr_vGetSplitElemByIndex(&sTempStrPt, ',', (u8NumberOffset - 1));
                    InPin_eInit(psInPinHnd, &sTempStrPt, *pu8PresentablesLen, true, (uint8_t)u8Pin);
                    apsPresentables[*pu8PresentablesLen] = (LOOPRE_IF_T *)psInPinHnd;
                    (*pu8PresentablesLen)++;
                    apsLoopables[*pu8LoopablesLen] = (LOOPRE_IF_T *)psInPinHnd;
                    (*pu8LoopablesLen)++;
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
            for (uint8_t i = 0; i < *pu8LoopablesLen; i++)
            {
                if (strcmp(MySensorsPresent_pcGetName((MYSENSORSPRESENT_HANDLE_T *)apsLoopables[i]), cTempStr) == 0)
                {
                    psEmiter = (INPIN_HANDLE_T *)apsLoopables[i];
                    break;
                }
            }
            if (psEmiter == NULL)
            {
                PinCfgCsv_vAddToString("W: Trigger: Invalid InPin name.\n", pcOutString, (size_t)u16OutStrMaxLen);
                continue;
            }

            sTempStrPt = sLine;
            PinCfgStr_vGetSplitElemByIndex(&sTempStrPt, ',', 3);
            uint8_t u8EventType;
            if (PinCfgStr_eAtoi(&sTempStrPt, (int *)&u8EventType) != PINCFG_STR_OK_E || u8EventType < 0 ||
                u8EventType > 3)
            {
                PinCfgCsv_vAddToString(
                    "W: Trigger: Invalid format for event type.\n", pcOutString, (size_t)u16OutStrMaxLen);
                continue;
            }

            sTempStrPt = sLine;
            PinCfgStr_vGetSplitElemByIndex(&sTempStrPt, ',', 4);
            int32_t u8EventCount;
            if (PinCfgStr_eAtoi(&sTempStrPt, (int *)&u8EventCount) != PINCFG_STR_OK_E || u8EventCount < 0)
            {
                PinCfgCsv_vAddToString(
                    "W: Trigger: Invalid format for event count.\n", pcOutString, (size_t)u16OutStrMaxLen);
                continue;
            }

            sTempStrPt = sLine;
            PinCfgStr_vGetSplitElemByIndex(&sTempStrPt, ',', 5);
            int16_t s16DrivesCount;
            if (PinCfgStr_eAtoi(&sTempStrPt, (int *)&s16DrivesCount) != PINCFG_STR_OK_E || s16DrivesCount < 0)
            {
                PinCfgCsv_vAddToString(
                    "W: Trigger: Invalid format for drives count.\n", pcOutString, (size_t)u16OutStrMaxLen);
                continue;
            }

            TRIGGER_SWITCHACTION_T *pasSwActs = NULL;
            uint8_t u8DrivesCountReal = 0U;
            for (uint8_t i = 0; i < s16DrivesCount; i++)
            {
                uint8_t u8NameOffset = 6 + i * 2;
                SWITCH_HANDLE_T *psDriven = NULL;
                sTempStrPt = sLine;
                PinCfgStr_vGetSplitElemByIndex(&sTempStrPt, ',', u8NameOffset);
                memcpy(cTempStr, sTempStrPt.pcStrStart, sTempStrPt.szLen);
                cTempStr[sTempStrPt.szLen] = '\0';
                for (uint8_t j = 0; j < *pu8LoopablesLen; j++)
                {
                    if (strcmp(MySensorsPresent_pcGetName((LOOPRE_IF_T *)apsLoopables[j]), cTempStr) == 0)
                    {
                        psDriven = (SWITCH_HANDLE_T *)apsLoopables[j];
                        break;
                    }
                }
                if (psDriven == NULL)
                {
                    PinCfgCsv_vAddToString("W: Trigger: Invalid switch name.\n", pcOutString, (size_t)u16OutStrMaxLen);
                    continue;
                }

                sTempStrPt = sLine;
                PinCfgStr_vGetSplitElemByIndex(&sTempStrPt, ',', (u8NameOffset + 1));
                int8_t d8DrivenAction;
                if (PinCfgStr_eAtoi(&sTempStrPt, (int *)&d8DrivenAction) != PINCFG_STR_OK_E || d8DrivenAction < 0 ||
                    d8DrivenAction > 2)
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
                    psSwAct->psSwitchHnd = psDriven;
                    psSwAct->eAction = (TRIGGER_ACTION_T)d8DrivenAction;
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
                Trigger_eInit(
                    psTriggerHnd,
                    pasSwActs,
                    u8DrivesCountReal,
                    (TRIGGER_EVENTTYPE_T)u8EventType,
                    (uint8_t)u8EventCount);
            }
        }
        else
        {
            // TODO: string status
            return PINCFG_TYPE_ERROR_E;
        }
    }

    return PINCFG_OK_E;
}

PINCFG_RESULT_T PinCfgCsv_eValidate(const char *pcCfgBuf, char *pcOutString, const uint16_t u16OutStrMaxLen)
{
    return PinCfgCsv_eParse(
        pcCfgBuf, true, false, NULL, NULL, 0, NULL, NULL, 0, NULL, NULL, pcOutString, u16OutStrMaxLen);
}

// private
static inline void PinCfgCsv_vAddToString(const char *pcMsgToBeAdded, char *pcOutMsg, const size_t szOutMsgMaxLen)
{
    if ((strlen(pcMsgToBeAdded) + strlen(pcOutMsg)) > (szOutMsgMaxLen - 1))
        return;

    strcat(pcOutMsg, pcMsgToBeAdded);
}
