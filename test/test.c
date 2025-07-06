#include <stdio.h>
#include <unity.h>

#include "ArduinoMock.h"
#include "CPUTemp.h"
#include "Cli.h"
#include "Globals.h"
#include "InPin.h"
#include "LinkedList.h"
#include "Memory.h"
#include "MySensorsMock.h"
#include "PinCfgCsv.h"
#include "PinCfgStr.h"
#include "Presentable.h"
#include "Sensor.h"
#include "Switch.h"
#include "Trigger.h"

#ifndef MEMORY_SZ
#define MEMORY_SZ 3003
#endif

#ifndef AS_OUT_MAX_LEN_D
#define AS_OUT_MAX_LEN_D 10
#endif

#ifndef LOOPRE_ARRAYS_MAX_LEN_D
#define LOOPRE_ARRAYS_MAX_LEN_D 30
#endif

#ifndef OUT_STR_MAX_LEN_D
#define OUT_STR_MAX_LEN_D 250
#endif

static uint8_t testMemory[MEMORY_SZ];

void setUp(void)
{
    PinCfgCsv_eInit(testMemory, MEMORY_SZ, NULL);

    init_MySensorsMock();
    init_ArduinoMock();
}

void tearDown(void)
{
    // clean stuff up here
}

void test_vMemory(void)
{
    MEMORY_RESULT_T eMemRes;

#ifndef USE_MALLOC
    eMemRes = Memory_eInit(NULL, sizeof(GLOBALS_T) - 1);
    TEST_ASSERT_EQUAL(MEMORY_ERROR_E, eMemRes);

    eMemRes = Memory_eInit(testMemory, sizeof(GLOBALS_T) - 1);
    TEST_ASSERT_EQUAL(MEMORY_INSUFFICIENT_SIZE_ERROR_E, eMemRes);

    eMemRes = Memory_eInit(testMemory, MEMORY_SZ);
    TEST_ASSERT_EQUAL(MEMORY_OK_E, eMemRes);
    // memory test
    char *a1 = (char *)Memory_vpAlloc(2);
    char *a2 = (char *)Memory_vpAlloc(3);
    char *a3 = (char *)Memory_vpAlloc(MEMORY_SZ);
    TEST_ASSERT_EQUAL(ENOMEM, errno);
    TEST_ASSERT_EQUAL(
        MEMORY_SZ - sizeof(GLOBALS_T) - (2 * sizeof(void *)) - (MEMORY_SZ % sizeof(void *)), Memory_szGetFree());

    TEST_ASSERT_EQUAL(a1, (char *)testMemory + sizeof(GLOBALS_T));
    TEST_ASSERT_EQUAL(a2, a1 + sizeof(void *));
    TEST_ASSERT_NULL(a3);

    // temp memory alloc test
    struct MEMORY_TEMP_ITEM_S
    {
        union
        {
            struct
            {
                uint16_t u16AlocatedSize;
                bool bFree;
            };
            void *pvAligment;
        };
    } memoryItemStruct;
    Memory_eInit(testMemory, MEMORY_SZ);
    char *pcTemp1 = (char *)Memory_vpTempAlloc((sizeof(void *) * 2) - 2);
    TEST_ASSERT_EQUAL(pcTemp1, (char *)psGlobals->pvMemEnd - (sizeof(void *) * 2));
    TEST_ASSERT_EQUAL(psGlobals->pvMemTempEnd, pcTemp1 - sizeof(memoryItemStruct));

    char *pcTemp2 = (char *)Memory_vpTempAlloc((sizeof(void *) * 2) - 2);
    TEST_ASSERT_EQUAL(pcTemp2, pcTemp1 - (sizeof(void *) * 2) - sizeof(memoryItemStruct));
    TEST_ASSERT_EQUAL(psGlobals->pvMemTempEnd, pcTemp2 - sizeof(memoryItemStruct));

    char *pcTemp3 = (char *)Memory_vpTempAlloc(MEMORY_SZ);
    TEST_ASSERT_NULL(pcTemp3);

    char str1[] = "Hello!";
    char str2[] = "Hi!";
    strcpy(pcTemp1, str1);
    strcpy(pcTemp2, str2);
    TEST_ASSERT_EQUAL_STRING("Hello!", pcTemp1);
    TEST_ASSERT_EQUAL_STRING("Hi!", pcTemp2);

    Memory_vTempFreePt(pcTemp2);
    TEST_ASSERT_EQUAL(psGlobals->pvMemTempEnd, pcTemp1 - sizeof(memoryItemStruct));
    TEST_ASSERT_EQUAL_STRING("", pcTemp2);

    Memory_vTempFree();
    TEST_ASSERT_EQUAL_STRING("", pcTemp1);
    TEST_ASSERT_EQUAL_STRING("", pcTemp2);

    pcTemp1 = (char *)Memory_vpTempAlloc(8);
    pcTemp1[0] = 0xff;
    pcTemp1[1] = 0xff;
    pcTemp1[2] = 0xff;
    pcTemp1[3] = 0xff;
    pcTemp1[4] = 0xff;
    pcTemp1[5] = 0xff;
    pcTemp1[6] = 0xff;
    pcTemp1[7] = 0xff;
    pcTemp1[8] = 0xff;
    uint8_t u32Exp1[] = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff};
    uint8_t u32Exp2[] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff};
    TEST_ASSERT_EQUAL_MEMORY(&u32Exp1, pcTemp1, 9);
    Memory_vTempFree();
    TEST_ASSERT_EQUAL_MEMORY(&u32Exp2, pcTemp1, 9);

    void *pvTmp1 = Memory_vpTempAlloc(1);
    void *pvTmp2 = Memory_vpTempAlloc(2);
    void *pvTmp3 = Memory_vpTempAlloc(3);

    TEST_ASSERT_NOT_NULL(pvTmp1);
    TEST_ASSERT_NOT_NULL(pvTmp2);
    TEST_ASSERT_NOT_NULL(pvTmp3);

    TEST_ASSERT_EQUAL(psGlobals->pvMemTempEnd, ((char *)pvTmp3) - sizeof(memoryItemStruct));

    Memory_vTempFreePt(pvTmp1);
    TEST_ASSERT_EQUAL(psGlobals->pvMemTempEnd, ((char *)pvTmp3) - sizeof(memoryItemStruct));

    Memory_vTempFreePt(pvTmp3);
    TEST_ASSERT_EQUAL(psGlobals->pvMemTempEnd, ((char *)pvTmp2) - sizeof(memoryItemStruct));

    Memory_vTempFreePt(pvTmp2);
    TEST_ASSERT_EQUAL(psGlobals->pvMemTempEnd, psGlobals->pvMemEnd);
#endif // USE_MALLOC
}

void test_vStringPoint(void)
{
    char acName[] = "Ahoj";
    STRING_POINT_T sName;
    PinCfgStr_vInitStrPoint(&sName, acName, sizeof(acName) - 1);
    TEST_ASSERT_EQUAL(acName, sName.pcStrStart);
    TEST_ASSERT_EQUAL(strlen(acName), 4);
}

void test_vLinkedList(void)
{
    LINKEDLIST_ITEM_T *pvFirst = NULL;
    PRESENTABLE_T *psPresentHandle = (PRESENTABLE_T *)Memory_vpAlloc(sizeof(PRESENTABLE_T));
    psPresentHandle->pcName = "item_1";

    LINKEDLIST_RESULT_T eRes = LinkedList_eAddToLinkedList(&pvFirst, (void *)psPresentHandle);
    TEST_ASSERT_EQUAL(eRes, LINKEDLIST_OK_E);

    psPresentHandle = (PRESENTABLE_T *)Memory_vpAlloc(sizeof(PRESENTABLE_T));
    psPresentHandle->pcName = "item_2";

    eRes = LinkedList_eAddToLinkedList(&pvFirst, (void *)psPresentHandle);
    TEST_ASSERT_EQUAL(eRes, LINKEDLIST_OK_E);

    psPresentHandle = (PRESENTABLE_T *)Memory_vpAlloc(sizeof(PRESENTABLE_T));
    psPresentHandle->pcName = "item_3";

    eRes = LinkedList_eAddToLinkedList(&pvFirst, (void *)psPresentHandle);
    TEST_ASSERT_EQUAL(eRes, LINKEDLIST_OK_E);

    size_t szLength;
    eRes = LinkedList_eGetLength(&pvFirst, &szLength);
    TEST_ASSERT_EQUAL(eRes, LINKEDLIST_OK_E);
    TEST_ASSERT_EQUAL(3, szLength);

    PRESENTABLE_T *psItem1 = (PRESENTABLE_T *)LinkedList_pvPopFront(&pvFirst);
    TEST_ASSERT_EQUAL_STRING("item_1", psItem1->pcName);
    TEST_ASSERT_EQUAL_STRING("item_2", (*((PRESENTABLE_T **)pvFirst))->pcName);

    PRESENTABLE_T *psItem2 = (PRESENTABLE_T *)LinkedList_pvPopFront(&pvFirst);
    TEST_ASSERT_EQUAL_STRING("item_2", psItem2->pcName);
    TEST_ASSERT_EQUAL_STRING("item_3", (*((PRESENTABLE_T **)pvFirst))->pcName);

    PRESENTABLE_T *psItem3 = (PRESENTABLE_T *)LinkedList_pvPopFront(&pvFirst);
    TEST_ASSERT_EQUAL_STRING("item_3", psPresentHandle->pcName);
    TEST_ASSERT_NULL(pvFirst);

    eRes = LinkedList_eAddToLinkedList(&pvFirst, (void *)psItem1);
    TEST_ASSERT_EQUAL(eRes, LINKEDLIST_OK_E);
    eRes = LinkedList_eAddToLinkedList(&pvFirst, (void *)psItem2);
    TEST_ASSERT_EQUAL(eRes, LINKEDLIST_OK_E);
    eRes = LinkedList_eAddToLinkedList(&pvFirst, (void *)psItem3);
    TEST_ASSERT_EQUAL(eRes, LINKEDLIST_OK_E);

    LINKEDLIST_ITEM_T *psLLItem = LinkedList_psGetNext(pvFirst);
    psPresentHandle = (PRESENTABLE_T *)LinkedList_pvGetStoredItem(psLLItem);
    TEST_ASSERT_EQUAL_STRING("item_2", psPresentHandle->pcName);
}

void test_vPinCfgStr(void)
{
    char pcTestString1[] = "This\nIs the b;est;257;22;testing\n\n\nstring ever made!\nbraka\n";
    STRING_POINT_T sTestStrPt1;

    // init
    PinCfgStr_vInitStrPoint(&sTestStrPt1, pcTestString1, sizeof(pcTestString1) - 1);
    TEST_ASSERT_EQUAL(sTestStrPt1.pcStrStart, &pcTestString1);
    TEST_ASSERT_EQUAL(sTestStrPt1.szLen, sizeof(pcTestString1) - 1);

    // split count
    size_t szReturn;
    szReturn = PinCfgStr_szGetSplitCount(&sTestStrPt1, '\n');
    TEST_ASSERT_EQUAL(6, szReturn);
    szReturn = PinCfgStr_szGetSplitCount(&sTestStrPt1, ' ');
    TEST_ASSERT_EQUAL(5, szReturn);
    szReturn = PinCfgStr_szGetSplitCount(&sTestStrPt1, 'T');
    TEST_ASSERT_EQUAL(2, szReturn);

    // split element by index
    STRING_POINT_T sTestStrPt2;
    sTestStrPt2 = sTestStrPt1;
    PinCfgStr_vGetSplitElemByIndex(&sTestStrPt2, '\n', 6);
    TEST_ASSERT_NULL(sTestStrPt2.pcStrStart);
    sTestStrPt2 = sTestStrPt1;
    PinCfgStr_vGetSplitElemByIndex(&sTestStrPt2, '\n', 5);
    TEST_ASSERT_NOT_NULL(sTestStrPt2.pcStrStart);
    TEST_ASSERT_EQUAL_STRING("braka\n", sTestStrPt2.pcStrStart);
    sTestStrPt2 = sTestStrPt1;
    PinCfgStr_vGetSplitElemByIndex(&sTestStrPt2, ' ', 4);
    TEST_ASSERT_EQUAL_STRING("made!\nbraka\n", sTestStrPt2.pcStrStart);

    // to U8
    PINCFG_STR_RESULT_T eResult;
    uint8_t u8Out = 0;
    sTestStrPt2 = sTestStrPt1;
    PinCfgStr_vGetSplitElemByIndex(&sTestStrPt2, ';', 0);
    eResult = PinCfgStr_eAtoU8(&sTestStrPt2, &u8Out);
    TEST_ASSERT_EQUAL(PINCFG_STR_INSUFFICIENT_BUFFER_E, eResult);
    TEST_ASSERT_EQUAL(0, u8Out);
    sTestStrPt2 = sTestStrPt1;
    PinCfgStr_vGetSplitElemByIndex(&sTestStrPt2, ';', 1);
    eResult = PinCfgStr_eAtoU8(&sTestStrPt2, &u8Out);
    TEST_ASSERT_EQUAL(PINCFG_STR_UNSUCCESSFULL_CONVERSION_E, eResult);
    TEST_ASSERT_EQUAL(0, u8Out);
    sTestStrPt2 = sTestStrPt1;
    PinCfgStr_vGetSplitElemByIndex(&sTestStrPt2, ';', 2);
    eResult = PinCfgStr_eAtoU8(&sTestStrPt2, &u8Out);
    TEST_ASSERT_EQUAL(PINCFG_STR_OK_E, eResult);
    TEST_ASSERT_EQUAL(1, u8Out);
    sTestStrPt2 = sTestStrPt1;
    PinCfgStr_vGetSplitElemByIndex(&sTestStrPt2, ';', 3);
    eResult = PinCfgStr_eAtoU8(&sTestStrPt2, &u8Out);
    TEST_ASSERT_EQUAL(PINCFG_STR_OK_E, eResult);
    TEST_ASSERT_EQUAL(22, u8Out);
}

void test_vMySenosrsPresent(void)
{
    // setup
    char acName[] = "MySenosrsPresent";
    STRING_POINT_T sName;
    PinCfgStr_vInitStrPoint(&sName, acName, sizeof(acName) - 1);
    PRESENTABLE_T *psPresentHandle = (PRESENTABLE_T *)Memory_vpAlloc(sizeof(PRESENTABLE_T));
    PRESENTABLE_VTAB_T sPresentableVTab;
    psPresentHandle->psVtab = &sPresentableVTab;

    // init test
    PRESENTABLE_RESULT_T eResult;
    eResult = Presentable_eInit(NULL, &sName, 1);
    TEST_ASSERT_EQUAL(PRESENTABLE_NULLPTR_ERROR_E, eResult);
    eResult = Presentable_eInit(psPresentHandle, NULL, 1);
    TEST_ASSERT_EQUAL(PRESENTABLE_NULLPTR_ERROR_E, eResult);
    eResult = Presentable_eInit(NULL, NULL, 1);
    TEST_ASSERT_EQUAL(PRESENTABLE_NULLPTR_ERROR_E, eResult);
#ifndef USE_MALLOC
    Memory_vpTempAlloc((size_t)(psGlobals->pvMemTempEnd - psGlobals->pvMemNext - sizeof(char *)));
    eResult = Presentable_eInit(psPresentHandle, &sName, 1);
    Memory_vTempFree();
    TEST_ASSERT_EQUAL(PRESENTABLE_ALLOCATION_ERROR_E, eResult);
#endif // USE_MALLOC
    eResult = Presentable_eInit(psPresentHandle, &sName, 1);
    TEST_ASSERT_EQUAL_STRING(acName, psPresentHandle->pcName);
    TEST_ASSERT_EQUAL_UINT8(1, psPresentHandle->u8Id);

    Presentable_vSendState(psPresentHandle);
    TEST_ASSERT_EQUAL_UINT8(0, mock_MyMessage_set_uint8_t_value);
    TEST_ASSERT_EQUAL(1, mock_send_u32Called);

    Presentable_vSetState(psPresentHandle, 1, true);
    TEST_ASSERT_EQUAL(2, mock_send_u32Called);

    Presentable_vToggle(psPresentHandle);
    TEST_ASSERT_EQUAL_UINT8(0, mock_MyMessage_set_uint8_t_value);
    TEST_ASSERT_EQUAL(3, mock_send_u32Called);

    TEST_ASSERT_EQUAL_UINT8(1U, Presentable_u8GetId(psPresentHandle));

    TEST_ASSERT_EQUAL_STRING(acName, Presentable_pcGetName(psPresentHandle));

    uint8_t u8Msg = 0x55U;
    Presentable_vRcvMessage(psPresentHandle, (const void *)&u8Msg);
    TEST_ASSERT_EQUAL(3, mock_send_u32Called);

    Presentable_vPresent(psPresentHandle);
    TEST_ASSERT_EQUAL(1, mock_bPresent_u8Id);
    TEST_ASSERT_EQUAL_STRING(acName, mock_bPresent_pcName);
    TEST_ASSERT_EQUAL(1, mock_bPresent_u32Called);

    Presentable_vPresentState(psPresentHandle);
    TEST_ASSERT_EQUAL(4, mock_send_u32Called);
    TEST_ASSERT_EQUAL_UINT8(1, mock_bRequest_u8Id);
    TEST_ASSERT_EQUAL(1, mock_bRequest_u32Called);
}

void test_vInPin(void)
{
    char acName[] = "InPin";
    STRING_POINT_T sName;
    PinCfgStr_vInitStrPoint(&sName, acName, sizeof(acName) - 1);
    INPIN_T *psInPinHandle = (INPIN_T *)Memory_vpAlloc(sizeof(INPIN_T));
    INPIN_RESULT_T eResult;

    // init
    eResult = InPin_eInit(NULL, &sName, 2, 16);
    TEST_ASSERT_EQUAL(INPIN_NULLPTR_ERROR_E, eResult);
    eResult = InPin_eInit(psInPinHandle, NULL, 2, 16);
    TEST_ASSERT_EQUAL(INPIN_SUBINIT_ERROR_E, eResult);
    eResult = InPin_eInit(psInPinHandle, &sName, 2, 16);
    TEST_ASSERT_EQUAL(INPIN_OK_E, eResult);
    TEST_ASSERT_EQUAL(16, mock_pinMode_u8Pin);
    TEST_ASSERT_EQUAL(INPUT_PULLUP, mock_pinMode_u8Mode);
    TEST_ASSERT_EQUAL(1, mock_pinMode_u32Called);
    TEST_ASSERT_EQUAL(16, mock_digitalWrite_u8Pin);
    TEST_ASSERT_EQUAL(HIGH, mock_digitalWrite_u8Value);
    TEST_ASSERT_EQUAL(1, mock_digitalWrite_u32Called);

    // add subscirber
    PINSUBSCRIBER_IF_T *psPinSubscriber1 = (PINSUBSCRIBER_IF_T *)Memory_vpAlloc(sizeof(PINSUBSCRIBER_IF_T));
    PINSUBSCRIBER_IF_T *psPinSubscriber2 = (PINSUBSCRIBER_IF_T *)Memory_vpAlloc(sizeof(PINSUBSCRIBER_IF_T));
    eResult = InPin_eAddSubscriber(NULL, psPinSubscriber1);
    TEST_ASSERT_EQUAL(INPIN_NULLPTR_ERROR_E, eResult);
    eResult = InPin_eAddSubscriber(psInPinHandle, NULL);
    TEST_ASSERT_EQUAL(INPIN_NULLPTR_ERROR_E, eResult);
    eResult = InPin_eAddSubscriber(NULL, NULL);
    TEST_ASSERT_EQUAL(INPIN_NULLPTR_ERROR_E, eResult);
    eResult = InPin_eAddSubscriber(psInPinHandle, psPinSubscriber1);
    TEST_ASSERT_EQUAL(INPIN_OK_E, eResult);
    TEST_ASSERT_EQUAL(psInPinHandle->psFirstSubscriber, psPinSubscriber1);
    eResult = InPin_eAddSubscriber(psInPinHandle, psPinSubscriber2);
    TEST_ASSERT_EQUAL(INPIN_OK_E, eResult);
    TEST_ASSERT_EQUAL(psInPinHandle->psFirstSubscriber, psPinSubscriber1);
    TEST_ASSERT_EQUAL(psPinSubscriber1->psNext, psPinSubscriber2);
}

void test_vSwitch(void)
{
    PINCFG_RESULT_T eParseResult;
    LINKEDLIST_RESULT_T eLinkedListResult;
    char acOutStr[OUT_STR_MAX_LEN_D];
    size_t szMemoryRequired;

    const char *pcCfg = "S,o1,13/"
                        "CR,120/"
                        "SI,o2,12/"
                        "CR,350/"
                        "SI,o5,9/"
                        "SF,o6,10,2/"
                        "SIF,o7,11,3/"
                        "ST,o8,11,300000/";

    PINCFG_PARSE_PARAMS_T sParams = {
        .pcConfig = pcCfg,
        .eAddToLoopables = PinCfgCsv_eAddToTempLoopables,
        .eAddToPresentables = PinCfgCsv_eAddToTempPresentables,
        .pszMemoryRequired = &szMemoryRequired,
        .pcOutString = acOutStr,
        .u16OutStrMaxLen = (uint16_t)OUT_STR_MAX_LEN_D,
        .bValidate = false};

    eParseResult = PinCfgCsv_eParse(&sParams);
    TEST_ASSERT_EQUAL(PINCFG_OK_E, eParseResult);

    eLinkedListResult =
        LinkedList_eLinkedListToArray((LINKEDLIST_ITEM_T **)(&psGlobals->ppsLoopables), &psGlobals->u8LoopablesCount);
    TEST_ASSERT_EQUAL(LINKEDLIST_OK_E, eLinkedListResult);
    TEST_ASSERT_EQUAL(6, psGlobals->u8LoopablesCount);

    eLinkedListResult = LinkedList_eLinkedListToArray(
        (LINKEDLIST_ITEM_T **)(&psGlobals->ppsPresentables), &psGlobals->u8PresentablesCount);
    TEST_ASSERT_EQUAL(LINKEDLIST_OK_E, eLinkedListResult);
    TEST_ASSERT_EQUAL(7, psGlobals->u8PresentablesCount);

    PRESENTABLE_T *psPresentable = (PRESENTABLE_T *)psGlobals->ppsPresentables[0];
    TEST_ASSERT_EQUAL_STRING("CLI", psPresentable->pcName);

    SWITCH_T *psSwitchHnd = (SWITCH_T *)psGlobals->ppsPresentables[1];
    TEST_ASSERT_EQUAL_STRING("o1", psSwitchHnd->sPresentable.pcName);
    TEST_ASSERT_EQUAL(SWITCH_CLASSIC_E, psSwitchHnd->eMode);
    TEST_ASSERT_EQUAL(13, psSwitchHnd->u8OutPin);

    psSwitchHnd = (SWITCH_T *)psGlobals->ppsPresentables[2];
    TEST_ASSERT_EQUAL_STRING("o2", psSwitchHnd->sPresentable.pcName);
    TEST_ASSERT_EQUAL(SWITCH_IMPULSE_E, psSwitchHnd->eMode);
    TEST_ASSERT_EQUAL(12, psSwitchHnd->u8OutPin);
    TEST_ASSERT_EQUAL(120, psSwitchHnd->u32ImpulseDuration);

    psSwitchHnd = (SWITCH_T *)psGlobals->ppsPresentables[3];
    TEST_ASSERT_EQUAL_STRING("o5", psSwitchHnd->sPresentable.pcName);
    TEST_ASSERT_EQUAL(SWITCH_IMPULSE_E, psSwitchHnd->eMode);
    TEST_ASSERT_EQUAL(9, psSwitchHnd->u8OutPin);
    TEST_ASSERT_EQUAL(350, psSwitchHnd->u32ImpulseDuration);

    psSwitchHnd = (SWITCH_T *)psGlobals->ppsPresentables[4];
    TEST_ASSERT_EQUAL_STRING("o6", psSwitchHnd->sPresentable.pcName);
    TEST_ASSERT_EQUAL(SWITCH_CLASSIC_E, psSwitchHnd->eMode);
    TEST_ASSERT_EQUAL(10, psSwitchHnd->u8OutPin);
    TEST_ASSERT_EQUAL(2, psSwitchHnd->u8FbPin);

    psSwitchHnd = (SWITCH_T *)psGlobals->ppsPresentables[5];
    TEST_ASSERT_EQUAL_STRING("o7", psSwitchHnd->sPresentable.pcName);
    TEST_ASSERT_EQUAL(SWITCH_IMPULSE_E, psSwitchHnd->eMode);
    TEST_ASSERT_EQUAL(11, psSwitchHnd->u8OutPin);
    TEST_ASSERT_EQUAL(3, psSwitchHnd->u8FbPin);

    TEST_ASSERT_EQUAL(350, psGlobals->u32SwitchImpulseDurationMs);

    psSwitchHnd = (SWITCH_T *)psGlobals->ppsPresentables[6];
    TEST_ASSERT_EQUAL_STRING("o8", psSwitchHnd->sPresentable.pcName);
    TEST_ASSERT_EQUAL(SWITCH_TIMED_E, psSwitchHnd->eMode);
    TEST_ASSERT_EQUAL(11, psSwitchHnd->u8OutPin);
    TEST_ASSERT_EQUAL(300000, psSwitchHnd->u32TimedAdidtionalDelayMs);
}

void test_vTrigger(void)
{
    PINCFG_RESULT_T eParseResult;
    LINKEDLIST_RESULT_T eLinkedListResult;
    char acOutStr[OUT_STR_MAX_LEN_D];
    size_t szMemoryRequired;

    const char *pcCfg = "I,i1,16/"
                        "S,o1,13/"
                        "T,t1,i1,0,1,o1,2/";

    // Setup parse params
    PINCFG_PARSE_PARAMS_T sParams = {
        .pcConfig = pcCfg,
        .eAddToLoopables = PinCfgCsv_eAddToTempLoopables,
        .eAddToPresentables = PinCfgCsv_eAddToTempPresentables,
        .pszMemoryRequired = &szMemoryRequired,
        .pcOutString = acOutStr,
        .u16OutStrMaxLen = (uint16_t)OUT_STR_MAX_LEN_D,
        .bValidate = false
    };

    eParseResult = PinCfgCsv_eParse(&sParams);
    TEST_ASSERT_EQUAL(PINCFG_OK_E, eParseResult);

    eLinkedListResult =
        LinkedList_eLinkedListToArray((LINKEDLIST_ITEM_T **)(&psGlobals->ppsLoopables), &psGlobals->u8LoopablesCount);
    TEST_ASSERT_EQUAL(LINKEDLIST_OK_E, eLinkedListResult);
    TEST_ASSERT_EQUAL(2, psGlobals->u8LoopablesCount);

    eLinkedListResult = LinkedList_eLinkedListToArray(
        (LINKEDLIST_ITEM_T **)(&psGlobals->ppsPresentables), &psGlobals->u8PresentablesCount);
    TEST_ASSERT_EQUAL(LINKEDLIST_OK_E, eLinkedListResult);
    TEST_ASSERT_EQUAL(3, psGlobals->u8PresentablesCount);

    INPIN_T *psInPin = (INPIN_T *)psGlobals->ppsPresentables[1];
    SWITCH_T *psSwitch = (SWITCH_T *)psGlobals->ppsPresentables[2];
    TRIGGER_T *psTrigger = (TRIGGER_T *)psInPin->psFirstSubscriber; // Assuming trigger is registered as first subscriber

    // Check trigger presentable
    TEST_ASSERT_NOT_NULL(psTrigger);
    TEST_ASSERT_EQUAL(1, psTrigger->u8SwActCount);
    TEST_ASSERT_EQUAL(TRIGGER_A_TOGGLE_E, psTrigger->eEventType);
    TEST_ASSERT_EQUAL(1, psTrigger->u8EventCount);

    // Check trigger actions
    TRIGGER_SWITCHACTION_T *psAction1 = &psTrigger->pasSwAct[0];
    TEST_ASSERT_EQUAL_STRING("o1", psAction1->psSwitchHnd->sPresentable.pcName);
    TEST_ASSERT_EQUAL(TRIGGER_LONG_E, psAction1->eAction);

    // Simulate trigger activation

    // // Set input pin to 0, trigger should set switch to 1
    // psInPin->u8LastValue = 0;
    // psTrigger->asActions[0].psInPin = psInPin;
    // psTrigger->asActions[0].psSwitch = psSwitch;
    // psTrigger->asActions[1].psInPin = psInPin;
    // psTrigger->asActions[1].psSwitch = psSwitch;

    // // Simulate trigger logic
    // psInPin->u8LastValue = 0;
    // psSwitch->u8State = 0;
    // psTrigger->asActions[0].psSwitch->u8State = 0;
    // psTrigger->asActions[1].psSwitch->u8State = 0;
    // psTrigger->asActions[0].psInPin->u8LastValue = 0;
    // psTrigger->asActions[1].psInPin->u8LastValue = 0;

    // // Call trigger logic (assuming function exists, e.g. Trigger_vProcess)
    // if (psTrigger->asActions[0].psInPin->u8LastValue == psTrigger->asActions[0].u8InValue) {
    //     psTrigger->asActions[0].psSwitch->u8State = psTrigger->asActions[0].u8OutValue;
    // }
    // TEST_ASSERT_EQUAL(1, psSwitch->u8State);

    // // Set input pin to 1, trigger should set switch to 2
    // psInPin->u8LastValue = 1;
    // if (psTrigger->asActions[1].psInPin->u8LastValue == psTrigger->asActions[1].u8InValue) {
    //     psTrigger->asActions[1].psSwitch->u8State = psTrigger->asActions[1].u8OutValue;
    // }
    // TEST_ASSERT_EQUAL(2, psSwitch->u8State);
}

void test_vPinCfgCsv(void)
{
    PINCFG_RESULT_T eParseResult;
    char acOutStr[OUT_STR_MAX_LEN_D];
    size_t szMemoryRequired;
    size_t szRequiredMem;

    PINCFG_PARSE_PARAMS_T sParams = {
        .eAddToLoopables = PinCfgCsv_eAddToTempLoopables,
        .eAddToPresentables = PinCfgCsv_eAddToTempPresentables,
        .pszMemoryRequired = &szMemoryRequired,
        .pcOutString = acOutStr,
        .u16OutStrMaxLen = (uint16_t)OUT_STR_MAX_LEN_D,
        .bValidate = false};

#ifndef USE_MALLOC
    Memory_vpAlloc((size_t)(psGlobals->pvMemTempEnd - psGlobals->pvMemNext - sizeof(char *)));
    eParseResult = PinCfgCsv_eParse(&sParams);
    szRequiredMem = Memory_szGetAllocatedSize(sizeof(GLOBALS_T));
    szRequiredMem += Memory_szGetAllocatedSize(sizeof(CLI_T));
    szRequiredMem += Memory_szGetAllocatedSize(sizeof(LINKEDLIST_ITEM_T) + sizeof(void *));
    TEST_ASSERT_EQUAL(szRequiredMem, szMemoryRequired);
    TEST_ASSERT_EQUAL(PINCFG_OUTOFMEMORY_ERROR_E, eParseResult);
    TEST_ASSERT_EQUAL_STRING("E:CLI:Out of memory.\n", acOutStr);
    Memory_eReset();
#endif // USE_MALLOC

    eParseResult = PinCfgCsv_eParse(&sParams);
    szRequiredMem = Memory_szGetAllocatedSize(sizeof(GLOBALS_T));
    szRequiredMem += Memory_szGetAllocatedSize(sizeof(CLI_T));
    szRequiredMem += Memory_szGetAllocatedSize(sizeof(LINKEDLIST_ITEM_T) + sizeof(void *));
    TEST_ASSERT_EQUAL(szRequiredMem, szMemoryRequired);
    TEST_ASSERT_EQUAL(PINCFG_NULLPTR_ERROR_E, eParseResult);
    TEST_ASSERT_EQUAL_STRING("E:Invalid format. NULL configuration.\n", acOutStr);
    Memory_eReset();

    sParams.pcConfig = "";
    eParseResult = PinCfgCsv_eParse(&sParams);
    szRequiredMem = Memory_szGetAllocatedSize(sizeof(GLOBALS_T));
    szRequiredMem += Memory_szGetAllocatedSize(sizeof(CLI_T));
    szRequiredMem += Memory_szGetAllocatedSize(sizeof(LINKEDLIST_ITEM_T) + sizeof(void *));
    TEST_ASSERT_EQUAL(szRequiredMem, szMemoryRequired);
    TEST_ASSERT_EQUAL(PINCFG_ERROR_E, eParseResult);
    TEST_ASSERT_EQUAL_STRING("E:Invalid format. Empty configuration.\n", acOutStr);
    Memory_eReset();

    sParams.pcConfig = "S";
    eParseResult = PinCfgCsv_eParse(&sParams);
    szRequiredMem = Memory_szGetAllocatedSize(sizeof(GLOBALS_T));
    szRequiredMem += Memory_szGetAllocatedSize(sizeof(CLI_T));
    szRequiredMem += Memory_szGetAllocatedSize(sizeof(LINKEDLIST_ITEM_T) + sizeof(void *));
    TEST_ASSERT_EQUAL(szRequiredMem, szMemoryRequired);
    TEST_ASSERT_EQUAL(PINCFG_WARNINGS_E, eParseResult);
    TEST_ASSERT_EQUAL_STRING("W:L:0:Not defined or invalid format.\nI: Configuration parsed.\n", acOutStr);
    Memory_eReset();

    sParams.pcConfig = "S,o1";
    eParseResult = PinCfgCsv_eParse(&sParams);
    szRequiredMem = Memory_szGetAllocatedSize(sizeof(GLOBALS_T));
    szRequiredMem += Memory_szGetAllocatedSize(sizeof(CLI_T));
    szRequiredMem += Memory_szGetAllocatedSize(sizeof(LINKEDLIST_ITEM_T) + sizeof(void *));
    TEST_ASSERT_EQUAL(szRequiredMem, szMemoryRequired);
    TEST_ASSERT_EQUAL(PINCFG_WARNINGS_E, eParseResult);
    TEST_ASSERT_EQUAL_STRING("W:L:0:Switch:Invalid number of arguments.\nI: Configuration parsed.\n", acOutStr);
    Memory_eReset();

    sParams.pcConfig = "S,o1,afsd";
    eParseResult = PinCfgCsv_eParse(&sParams);
    szRequiredMem = Memory_szGetAllocatedSize(sizeof(GLOBALS_T));
    szRequiredMem += Memory_szGetAllocatedSize(sizeof(CLI_T));
    szRequiredMem += Memory_szGetAllocatedSize(sizeof(LINKEDLIST_ITEM_T) + sizeof(void *));
    TEST_ASSERT_EQUAL(szRequiredMem, szMemoryRequired);
    TEST_ASSERT_EQUAL(PINCFG_WARNINGS_E, eParseResult);
    TEST_ASSERT_EQUAL_STRING("W:L:0:Switch:Invalid pin number.\nI: Configuration parsed.\n", acOutStr);
    Memory_eReset();

    sParams.pcConfig = "S,o1,afsd,o2";
    eParseResult = PinCfgCsv_eParse(&sParams);
    szRequiredMem = Memory_szGetAllocatedSize(sizeof(GLOBALS_T));
    szRequiredMem += Memory_szGetAllocatedSize(sizeof(CLI_T));
    szRequiredMem += Memory_szGetAllocatedSize(sizeof(LINKEDLIST_ITEM_T) + sizeof(void *));
    TEST_ASSERT_EQUAL(szRequiredMem, szMemoryRequired);
    TEST_ASSERT_EQUAL(PINCFG_WARNINGS_E, eParseResult);
    TEST_ASSERT_EQUAL_STRING(
        "W:L:0:Switch:Invalid number of items defining names and pins.\nI: Configuration parsed.\n", acOutStr);
    Memory_eReset();

#ifndef USE_MALLOC
    Memory_vpAlloc((size_t)(psGlobals->pvMemTempEnd - psGlobals->pvMemNext - sizeof(char *) - sizeof(CLI_T)));
    sParams.pcConfig = "S,o1,2";
    eParseResult = PinCfgCsv_eParse(&sParams);
    szRequiredMem = Memory_szGetAllocatedSize(sizeof(GLOBALS_T));
    szRequiredMem += Memory_szGetAllocatedSize(sizeof(CLI_T));
    szRequiredMem += Memory_szGetAllocatedSize(sizeof(LINKEDLIST_ITEM_T) + sizeof(void *));
    szRequiredMem += Memory_szGetAllocatedSize(sizeof(SWITCH_T));
    szRequiredMem += Memory_szGetAllocatedSize(sizeof(char) * 3);
    szRequiredMem +=
        Memory_szGetAllocatedSize(sizeof(LINKEDLIST_ITEM_T) + sizeof(void *)) * 2; /* loopables, presentables */
    TEST_ASSERT_EQUAL(szRequiredMem, szMemoryRequired);
    TEST_ASSERT_EQUAL(PINCFG_OUTOFMEMORY_ERROR_E, eParseResult);
    TEST_ASSERT_EQUAL_STRING("E:L:0:Switch:Out of memory.\n", acOutStr);
    Memory_eReset();

    TEST_ASSERT_EQUAL(
        (MEMORY_SZ - sizeof(GLOBALS_T) - (MEMORY_SZ - sizeof(GLOBALS_T)) % sizeof(char *)), Memory_szGetFree());
#endif // USE_MALLOC

    // pins o3,11,o4,10 removed due to emulator uses them for serial output

    sParams.pcConfig = "# (bluePill)/";
    eParseResult = PinCfgCsv_eParse(&sParams);
    szRequiredMem = Memory_szGetAllocatedSize(sizeof(GLOBALS_T));
    szRequiredMem += Memory_szGetAllocatedSize(sizeof(CLI_T));
    szRequiredMem += Memory_szGetAllocatedSize(sizeof(LINKEDLIST_ITEM_T) + sizeof(void *));
    TEST_ASSERT_EQUAL(szRequiredMem, szMemoryRequired);
    Memory_eReset();

    sParams.pcConfig = "S,o1,13,o2,12/";
    eParseResult = PinCfgCsv_eParse(&sParams);
    szRequiredMem = Memory_szGetAllocatedSize(sizeof(GLOBALS_T));
    szRequiredMem += Memory_szGetAllocatedSize(sizeof(CLI_T));
    szRequiredMem += Memory_szGetAllocatedSize(sizeof(LINKEDLIST_ITEM_T) + sizeof(void *));
    szRequiredMem += Memory_szGetAllocatedSize(sizeof(SWITCH_T)) * 2;
    szRequiredMem += Memory_szGetAllocatedSize(sizeof(char) * 3) * 2;
    szRequiredMem += Memory_szGetAllocatedSize(sizeof(LINKEDLIST_ITEM_T) + sizeof(void *)) * 4;
    TEST_ASSERT_EQUAL(szRequiredMem, szMemoryRequired);
    Memory_eReset();

    sParams.pcConfig = "I,i1,16/";
    eParseResult = PinCfgCsv_eParse(&sParams);
    szRequiredMem = Memory_szGetAllocatedSize(sizeof(GLOBALS_T));
    szRequiredMem += Memory_szGetAllocatedSize(sizeof(CLI_T));
    szRequiredMem += Memory_szGetAllocatedSize(sizeof(LINKEDLIST_ITEM_T) + sizeof(void *));
    szRequiredMem += Memory_szGetAllocatedSize(sizeof(INPIN_T));
    szRequiredMem += Memory_szGetAllocatedSize(sizeof(char) * 3);
    szRequiredMem += Memory_szGetAllocatedSize(sizeof(LINKEDLIST_ITEM_T) + sizeof(void *)) * 2;
    TEST_ASSERT_EQUAL(szRequiredMem, szMemoryRequired);
    Memory_eReset();

    sParams.pcConfig = "S,o1,13,o2,12/"
                       "I,i1,16/"
                       "T,vtl12,i1,0,1,o1,2,o2,2/";
    eParseResult = PinCfgCsv_eParse(&sParams);
    szRequiredMem = Memory_szGetAllocatedSize(sizeof(GLOBALS_T));
    szRequiredMem += Memory_szGetAllocatedSize(sizeof(CLI_T));
    szRequiredMem += Memory_szGetAllocatedSize(sizeof(LINKEDLIST_ITEM_T) + sizeof(void *));
    szRequiredMem += Memory_szGetAllocatedSize(sizeof(SWITCH_T)) * 2;
    szRequiredMem += Memory_szGetAllocatedSize(sizeof(char) * 3) * 2;
    szRequiredMem += Memory_szGetAllocatedSize(sizeof(LINKEDLIST_ITEM_T) + sizeof(void *)) * 4;
    szRequiredMem += Memory_szGetAllocatedSize(sizeof(INPIN_T));
    szRequiredMem += Memory_szGetAllocatedSize(sizeof(char) * 3);
    szRequiredMem += Memory_szGetAllocatedSize(sizeof(LINKEDLIST_ITEM_T) + sizeof(void *)) * 2;
    szRequiredMem += Memory_szGetAllocatedSize(sizeof(TRIGGER_T));
    szRequiredMem += Memory_szGetAllocatedSize(sizeof(TRIGGER_SWITCHACTION_T)) * 2;
    TEST_ASSERT_EQUAL(szRequiredMem, szMemoryRequired);
    Memory_eReset();

    sParams.pcConfig = "# (bluePill)/"
                       "S,o1,13,o2,12,o5,9,o6,8,o7,7,o8,6,o9,5,o10,4,o11,3,o12,2/"
                       "I,i1,16,i2,15,i3,14,i4,31,i5,30,i6,201,i7,195,i8,194,i9,193,i10,192,i11,19,i12,18/"
                       "#triggers/"
                       "T,vtl11,i1,0,1,o2,2/"
                       "T,vtl12,i1,0,1,o2,2,o5,2";
    eParseResult = PinCfgCsv_eParse(&sParams);

    szRequiredMem = Memory_szGetAllocatedSize(sizeof(GLOBALS_T));
    szRequiredMem += Memory_szGetAllocatedSize(sizeof(CLI_T));
    szRequiredMem += Memory_szGetAllocatedSize(sizeof(LINKEDLIST_ITEM_T) + sizeof(void *));
    szRequiredMem += Memory_szGetAllocatedSize(sizeof(SWITCH_T)) * 10;
    szRequiredMem += Memory_szGetAllocatedSize(sizeof(char) * 3) * 7;
    szRequiredMem += Memory_szGetAllocatedSize(sizeof(char) * 4) * 3;
    szRequiredMem += Memory_szGetAllocatedSize(sizeof(LINKEDLIST_ITEM_T) + sizeof(void *)) * 20;
    szRequiredMem += Memory_szGetAllocatedSize(sizeof(INPIN_T)) * 12;
    szRequiredMem += Memory_szGetAllocatedSize(sizeof(char) * 3) * 9;
    szRequiredMem += Memory_szGetAllocatedSize(sizeof(char) * 4) * 3;
    szRequiredMem += Memory_szGetAllocatedSize(sizeof(LINKEDLIST_ITEM_T) + sizeof(void *)) * 24;
    szRequiredMem += Memory_szGetAllocatedSize(sizeof(TRIGGER_T)) * 2;
    szRequiredMem += Memory_szGetAllocatedSize(sizeof(TRIGGER_SWITCHACTION_T)) * 3;
    TEST_ASSERT_EQUAL(szRequiredMem, szMemoryRequired);
    TEST_ASSERT_EQUAL(PINCFG_OK_E, eParseResult);
    TEST_ASSERT_EQUAL_STRING("I: Configuration parsed.\n", acOutStr);
    // Memory_eReset();
}

void test_vCLI(void)
{
    CLI_T *psCli = (CLI_T *)Memory_vpAlloc(sizeof(CLI_T));
    Cli_eInit(psCli, 0);

    Cli_vRcvMessage((PRESENTABLE_T *)psCli, "#[S,o1,13/");
    TEST_ASSERT_EQUAL(1, mock_send_u32Called);
    TEST_ASSERT_EQUAL_STRING("RECEIVING", mock_MyMessage_set_char_value);
    TEST_ASSERT_EQUAL_STRING("S,o1,13/", psCli->pcCfgBuf);

    memset(mock_send_message, 0, sizeof(mock_send_message));
    Cli_vRcvMessage((PRESENTABLE_T *)psCli, "I,i1,16/");
    TEST_ASSERT_EQUAL_STRING("S,o1,13/I,i1,16/", psCli->pcCfgBuf);
    Cli_vRcvMessage((PRESENTABLE_T *)psCli, "T,vtl11,i1,0,1,o2,2/]#");
    TEST_ASSERT_EQUAL_STRING("VALIDATION_OK;Save of cfg unsucessfull.;LISTENING;", mock_send_message);
    TEST_ASSERT_EQUAL(4, mock_send_u32Called);
    TEST_ASSERT_EQUAL_STRING("LISTENING", mock_MyMessage_set_char_value);

    memset(mock_send_message, 0, sizeof(mock_send_message));
    Cli_eInit(psCli, 0);
    Cli_vRcvMessage((PRESENTABLE_T *)psCli, "#[T,o1,13/]#");
    TEST_ASSERT_EQUAL_STRING(
        "RECEIVING;W:L:0:Trigger:Invalid num;ber of arguments.\nE:L:1:U;nknown type.\nI: Configura;tion "
        "parsed.\n;VALIDATION_ERROR;LISTENING;",
        mock_send_message);

    memset(mock_send_message, 0, sizeof(mock_send_message));
    Cli_eInit(psCli, 0);
    Cli_vRcvMessage((PRESENTABLE_T *)psCli, "#C:GET_CFG");
    TEST_ASSERT_EQUAL_STRING("Unable to read cfg size.;LISTENING;", mock_send_message);
}

void test_vGlobalConfig(void)
{
    PINCFG_RESULT_T eParseResult;
    char acOutStr[OUT_STR_MAX_LEN_D];
    size_t szMemoryRequired;
    PINCFG_PARSE_PARAMS_T sParams = {
        .eAddToLoopables = NULL,
        .eAddToPresentables = NULL,
        .pszMemoryRequired = &szMemoryRequired,
        .pcOutString = acOutStr,
        .u16OutStrMaxLen = (uint16_t)OUT_STR_MAX_LEN_D,
        .bValidate = false};

    sParams.pcConfig = "CD,330/"
                       "CM,620/"
                       "CR,150/"
                       "CN,1000/"
                       "CF,30000/";

    eParseResult = PinCfgCsv_eParse(&sParams);

    TEST_ASSERT_EQUAL(PINCFG_OK_E, eParseResult);

    TEST_ASSERT_EQUAL(330, psGlobals->u32InPinDebounceMs);
    TEST_ASSERT_EQUAL(620, psGlobals->u32InPinMulticlickMaxDelayMs);
    TEST_ASSERT_EQUAL(150, psGlobals->u32SwitchImpulseDurationMs);

    // InPin debounce
    memset(acOutStr, 0, OUT_STR_MAX_LEN_D);
    sParams.pcConfig = "D";

    eParseResult = PinCfgCsv_eParse(&sParams);

    TEST_ASSERT_EQUAL_STRING("W:L:0:Not defined or invalid format.\nI: Configuration parsed.\n", acOutStr);

    memset(acOutStr, 0, OUT_STR_MAX_LEN_D);
    sParams.pcConfig = "CD,abc";

    eParseResult = PinCfgCsv_eParse(&sParams);

    TEST_ASSERT_EQUAL_STRING("W:L:0:InPinDebounceMs:Invalid number.\nI: Configuration parsed.\n", acOutStr);

    // InPin multiclick
    memset(acOutStr, 0, OUT_STR_MAX_LEN_D);
    sParams.pcConfig = "CM";

    eParseResult = PinCfgCsv_eParse(&sParams);

    TEST_ASSERT_EQUAL_STRING("W:L:0:Not defined or invalid format.\nI: Configuration parsed.\n", acOutStr);

    memset(acOutStr, 0, OUT_STR_MAX_LEN_D);
    sParams.pcConfig = "CM,abc";

    eParseResult = PinCfgCsv_eParse(&sParams);

    TEST_ASSERT_EQUAL_STRING("W:L:0:InPinMulticlickMaxDelayMs:Invalid number.\nI: Configuration parsed.\n", acOutStr);

    // Switch impulse duration
    memset(acOutStr, 0, OUT_STR_MAX_LEN_D);
    sParams.pcConfig = "CR";

    eParseResult = PinCfgCsv_eParse(&sParams);

    TEST_ASSERT_EQUAL_STRING("W:L:0:Not defined or invalid format.\nI: Configuration parsed.\n", acOutStr);

    memset(acOutStr, 0, OUT_STR_MAX_LEN_D);
    sParams.pcConfig = "CR,abc";

    eParseResult = PinCfgCsv_eParse(&sParams);

    TEST_ASSERT_EQUAL_STRING("W:L:0:SwitchImpulseDurationMs:Invalid number.\nI: Configuration parsed.\n", acOutStr);

    // Switch feedback on delay
    memset(acOutStr, 0, OUT_STR_MAX_LEN_D);
    sParams.pcConfig = "CN";

    eParseResult = PinCfgCsv_eParse(&sParams);

    TEST_ASSERT_EQUAL_STRING("W:L:0:Not defined or invalid format.\nI: Configuration parsed.\n", acOutStr);

    memset(acOutStr, 0, OUT_STR_MAX_LEN_D);
    sParams.pcConfig = "CN,abc";

    eParseResult = PinCfgCsv_eParse(&sParams);

    TEST_ASSERT_EQUAL_STRING("W:L:0:SwitchFbOnDelayMs:Invalid number.\nI: Configuration parsed.\n", acOutStr);

    // Switch feedback off delay
    memset(acOutStr, 0, OUT_STR_MAX_LEN_D);
    sParams.pcConfig = "CF";

    eParseResult = PinCfgCsv_eParse(&sParams);

    TEST_ASSERT_EQUAL_STRING("W:L:0:Not defined or invalid format.\nI: Configuration parsed.\n", acOutStr);

    memset(acOutStr, 0, OUT_STR_MAX_LEN_D);
    sParams.pcConfig = "CF,abc";

    eParseResult = PinCfgCsv_eParse(&sParams);

    TEST_ASSERT_EQUAL_STRING("W:L:0:SwitchFbOffDelayMs:Invalid number.\nI: Configuration parsed.\n", acOutStr);
}

void test_vCPUTemp(void)
{
    PINCFG_RESULT_T eResult;
    LINKEDLIST_RESULT_T eLinkedListResult;
    char acOutStr[OUT_STR_MAX_LEN_D];
    size_t szMemoryRequired;
    size_t szRequiredMem;
    ENABLEABLE_T *psEnableableHnd;

    const char *pcCfg = "TI,CPUTemp0,0/"
                        "TI,CPUTemp1,0,1/"
                        "TI,CPUTemp2,1,0/"
                        "TI,CPUTemp3,1,1/"
                        "TI,CPUTemp4,1,1,1500/"
                        "TI,CPUTemp5,1,1,1500,200000/"
                        "TI,CPUTemp6,1,1,1500,200000,-2.1/";

    PINCFG_PARSE_PARAMS_T sParams = {
        .pcConfig = pcCfg,
        .eAddToLoopables = PinCfgCsv_eAddToTempLoopables,
        .eAddToPresentables = PinCfgCsv_eAddToTempPresentables,
        .pszMemoryRequired = &szMemoryRequired,
        .pcOutString = acOutStr,
        .u16OutStrMaxLen = (uint16_t)OUT_STR_MAX_LEN_D,
        .bValidate = false};

    eResult = PinCfgCsv_eParse(&sParams);
    TEST_ASSERT_EQUAL(PINCFG_OK_E, eResult);

#ifndef USE_MALLOC
    TEST_ASSERT_EQUAL(szMemoryRequired, MEMORY_SZ - (MEMORY_SZ % sizeof(void *)) - Memory_szGetFree());
#endif // USE_MALLOC

    eLinkedListResult =
        LinkedList_eLinkedListToArray((LINKEDLIST_ITEM_T **)(&psGlobals->ppsLoopables), &psGlobals->u8LoopablesCount);
    TEST_ASSERT_EQUAL(LINKEDLIST_OK_E, eLinkedListResult);
    TEST_ASSERT_EQUAL(7, psGlobals->u8LoopablesCount);

    eLinkedListResult = LinkedList_eLinkedListToArray(
        (LINKEDLIST_ITEM_T **)(&psGlobals->ppsPresentables), &psGlobals->u8PresentablesCount);
    TEST_ASSERT_EQUAL(LINKEDLIST_OK_E, eLinkedListResult);
    TEST_ASSERT_EQUAL(13, psGlobals->u8PresentablesCount);

    SENSOR_T *psSensor = (SENSOR_T *)psGlobals->ppsPresentables[1];
    TEST_ASSERT_EQUAL(V_TEMP, psSensor->sVtab.eVType);
    TEST_ASSERT_EQUAL(S_TEMP, psSensor->sVtab.eSType);
    TEST_ASSERT_EQUAL(InPin_vRcvMessage, psSensor->sVtab.vReceive);
    TEST_ASSERT_EQUAL(PINCFG_CPUTEMP_REPORTING_INTV_MS_D, psSensor->u32ReportInterval);
    TEST_ASSERT_EQUAL(0, psSensor->u32LastReportMs);
    TEST_ASSERT_EQUAL(PINCFG_CPUTEMP_OFFSET_D, psSensor->fOffset);
    TEST_ASSERT_EQUAL(1, psSensor->sPresentable.u8Id);
    TEST_ASSERT_EQUAL_STRING("CPUTemp0", psSensor->sPresentable.pcName);
    TEST_ASSERT_EQUAL(InPin_vRcvMessage, psSensor->sPresentable.psVtab->vReceive);

    SENSOR_CUMULATIVE_T *psSensorC = (SENSOR_CUMULATIVE_T *)psGlobals->ppsPresentables[2];
    TEST_ASSERT_EQUAL(V_TEMP, psSensorC->sSensor.sVtab.eVType);
    TEST_ASSERT_EQUAL(S_TEMP, psSensorC->sSensor.sVtab.eSType);
    TEST_ASSERT_EQUAL(InPin_vRcvMessage, psSensorC->sSensor.sVtab.vReceive);
    TEST_ASSERT_EQUAL(PINCFG_CPUTEMP_REPORTING_INTV_MS_D, psSensorC->sSensor.u32ReportInterval);
    TEST_ASSERT_EQUAL(0, psSensorC->sSensor.u32LastReportMs);
    TEST_ASSERT_EQUAL(PINCFG_CPUTEMP_OFFSET_D, psSensorC->sSensor.fOffset);
    TEST_ASSERT_EQUAL(PINCFG_CPUTEMP_SAMPLING_INTV_MS_D, psSensorC->u32SamplingInterval);
    TEST_ASSERT_EQUAL(0, psSensorC->u32LastSamplingMs);
    TEST_ASSERT_EQUAL(0, psSensorC->u32SamplesCount);
    TEST_ASSERT_EQUAL(0, psSensorC->fCumulatedValue);
    TEST_ASSERT_EQUAL_STRING("CPUTemp1", psSensorC->sSensor.sPresentable.pcName);
    TEST_ASSERT_EQUAL(InPin_vRcvMessage, psSensorC->sSensor.sPresentable.psVtab->vReceive);

    SENSOR_ENABLEABLE_T *psSensorE = (SENSOR_ENABLEABLE_T *)psGlobals->ppsPresentables[3];
    TEST_ASSERT_EQUAL(V_TEMP, psSensorE->sSensor.sVtab.eVType);
    TEST_ASSERT_EQUAL(S_TEMP, psSensorE->sSensor.sVtab.eSType);
    TEST_ASSERT_EQUAL(InPin_vRcvMessage, psSensorE->sSensor.sVtab.vReceive);
    TEST_ASSERT_EQUAL(PINCFG_CPUTEMP_REPORTING_INTV_MS_D, psSensorE->sSensor.u32ReportInterval);
    TEST_ASSERT_EQUAL(0, psSensorE->sSensor.u32LastReportMs);
    TEST_ASSERT_EQUAL(PINCFG_CPUTEMP_OFFSET_D, psSensorE->sSensor.fOffset);
    TEST_ASSERT_EQUAL(3, psSensorE->sSensor.sPresentable.u8Id);
    TEST_ASSERT_EQUAL_STRING("CPUTemp2", psSensorE->sSensor.sPresentable.pcName);
    TEST_ASSERT_EQUAL(InPin_vRcvMessage, psSensorE->sSensor.sPresentable.psVtab->vReceive);
    psEnableableHnd = &psSensorE->sEnableable;
    TEST_ASSERT_EQUAL(psGlobals->ppsPresentables[4], psEnableableHnd);
    TEST_ASSERT_EQUAL(4, psEnableableHnd->sPresentable.u8Id);
    TEST_ASSERT_EQUAL_STRING("CPUTemp2_enable", psEnableableHnd->sPresentable.pcName);

    SENSOR_CUMULATIVE_ENABLEABLE_T *psSensorCE = (SENSOR_CUMULATIVE_ENABLEABLE_T *)psGlobals->ppsPresentables[5];
    TEST_ASSERT_EQUAL(V_TEMP, psSensorCE->sSensorCumulative.sSensor.sVtab.eVType);
    TEST_ASSERT_EQUAL(S_TEMP, psSensorCE->sSensorCumulative.sSensor.sVtab.eSType);
    TEST_ASSERT_EQUAL(InPin_vRcvMessage, psSensorCE->sSensorCumulative.sSensor.sVtab.vReceive);
    TEST_ASSERT_EQUAL(PINCFG_CPUTEMP_REPORTING_INTV_MS_D, psSensorCE->sSensorCumulative.sSensor.u32ReportInterval);
    TEST_ASSERT_EQUAL(0, psSensorCE->sSensorCumulative.sSensor.u32LastReportMs);
    TEST_ASSERT_EQUAL(PINCFG_CPUTEMP_OFFSET_D, psSensorCE->sSensorCumulative.sSensor.fOffset);
    TEST_ASSERT_EQUAL(PINCFG_CPUTEMP_SAMPLING_INTV_MS_D, psSensorCE->sSensorCumulative.u32SamplingInterval);
    TEST_ASSERT_EQUAL(0, psSensorCE->sSensorCumulative.u32LastSamplingMs);
    TEST_ASSERT_EQUAL(0, psSensorCE->sSensorCumulative.u32SamplesCount);
    TEST_ASSERT_EQUAL(0, psSensorCE->sSensorCumulative.fCumulatedValue);
    TEST_ASSERT_EQUAL(5, psSensorCE->sSensorCumulative.sSensor.sPresentable.u8Id);
    TEST_ASSERT_EQUAL_STRING("CPUTemp3", psSensorCE->sSensorCumulative.sSensor.sPresentable.pcName);
    TEST_ASSERT_EQUAL(InPin_vRcvMessage, psSensorCE->sSensorCumulative.sSensor.sPresentable.psVtab->vReceive);
    psEnableableHnd = &psSensorCE->sEnableable;
    TEST_ASSERT_EQUAL(psGlobals->ppsPresentables[6], psEnableableHnd);
    TEST_ASSERT_EQUAL(6, psEnableableHnd->sPresentable.u8Id);
    TEST_ASSERT_EQUAL_STRING("CPUTemp3_enable", psEnableableHnd->sPresentable.pcName);

    psSensorCE = (SENSOR_CUMULATIVE_ENABLEABLE_T *)psGlobals->ppsPresentables[7];
    TEST_ASSERT_EQUAL(V_TEMP, psSensorCE->sSensorCumulative.sSensor.sVtab.eVType);
    TEST_ASSERT_EQUAL(S_TEMP, psSensorCE->sSensorCumulative.sSensor.sVtab.eSType);
    TEST_ASSERT_EQUAL(InPin_vRcvMessage, psSensorCE->sSensorCumulative.sSensor.sVtab.vReceive);
    TEST_ASSERT_EQUAL(PINCFG_CPUTEMP_REPORTING_INTV_MS_D, psSensorCE->sSensorCumulative.sSensor.u32ReportInterval);
    TEST_ASSERT_EQUAL(0, psSensorCE->sSensorCumulative.sSensor.u32LastReportMs);
    TEST_ASSERT_EQUAL(PINCFG_CPUTEMP_OFFSET_D, psSensorCE->sSensorCumulative.sSensor.fOffset);
    TEST_ASSERT_EQUAL(1500, psSensorCE->sSensorCumulative.u32SamplingInterval);
    TEST_ASSERT_EQUAL(0, psSensorCE->sSensorCumulative.u32LastSamplingMs);
    TEST_ASSERT_EQUAL(0, psSensorCE->sSensorCumulative.u32SamplesCount);
    TEST_ASSERT_EQUAL(0, psSensorCE->sSensorCumulative.fCumulatedValue);
    TEST_ASSERT_EQUAL(7, psSensorCE->sSensorCumulative.sSensor.sPresentable.u8Id);
    TEST_ASSERT_EQUAL_STRING("CPUTemp4", psSensorCE->sSensorCumulative.sSensor.sPresentable.pcName);
    TEST_ASSERT_EQUAL(InPin_vRcvMessage, psSensorCE->sSensorCumulative.sSensor.sPresentable.psVtab->vReceive);
    psEnableableHnd = &psSensorCE->sEnableable;
    TEST_ASSERT_EQUAL(psGlobals->ppsPresentables[8], psEnableableHnd);
    TEST_ASSERT_EQUAL(8, psEnableableHnd->sPresentable.u8Id);
    TEST_ASSERT_EQUAL_STRING("CPUTemp4_enable", psEnableableHnd->sPresentable.pcName);

    psSensorCE = (SENSOR_CUMULATIVE_ENABLEABLE_T *)psGlobals->ppsPresentables[9];
    TEST_ASSERT_EQUAL(V_TEMP, psSensorCE->sSensorCumulative.sSensor.sVtab.eVType);
    TEST_ASSERT_EQUAL(S_TEMP, psSensorCE->sSensorCumulative.sSensor.sVtab.eSType);
    TEST_ASSERT_EQUAL(InPin_vRcvMessage, psSensorCE->sSensorCumulative.sSensor.sVtab.vReceive);
    TEST_ASSERT_EQUAL(200000, psSensorCE->sSensorCumulative.sSensor.u32ReportInterval);
    TEST_ASSERT_EQUAL(0, psSensorCE->sSensorCumulative.sSensor.u32LastReportMs);
    TEST_ASSERT_EQUAL(PINCFG_CPUTEMP_OFFSET_D, psSensorCE->sSensorCumulative.sSensor.fOffset);
    TEST_ASSERT_EQUAL(1500, psSensorCE->sSensorCumulative.u32SamplingInterval);
    TEST_ASSERT_EQUAL(0, psSensorCE->sSensorCumulative.u32LastSamplingMs);
    TEST_ASSERT_EQUAL(0, psSensorCE->sSensorCumulative.u32SamplesCount);
    TEST_ASSERT_EQUAL(0, psSensorCE->sSensorCumulative.fCumulatedValue);
    TEST_ASSERT_EQUAL(9, psSensorCE->sSensorCumulative.sSensor.sPresentable.u8Id);
    TEST_ASSERT_EQUAL_STRING("CPUTemp5", psSensorCE->sSensorCumulative.sSensor.sPresentable.pcName);
    TEST_ASSERT_EQUAL(InPin_vRcvMessage, psSensorCE->sSensorCumulative.sSensor.sPresentable.psVtab->vReceive);
    psEnableableHnd = &psSensorCE->sEnableable;
    TEST_ASSERT_EQUAL(psGlobals->ppsPresentables[10], psEnableableHnd);
    TEST_ASSERT_EQUAL(10, psEnableableHnd->sPresentable.u8Id);
    TEST_ASSERT_EQUAL_STRING("CPUTemp5_enable", psEnableableHnd->sPresentable.pcName);

    psSensorCE = (SENSOR_CUMULATIVE_ENABLEABLE_T *)psGlobals->ppsPresentables[11];
    TEST_ASSERT_EQUAL(V_TEMP, psSensorCE->sSensorCumulative.sSensor.sVtab.eVType);
    TEST_ASSERT_EQUAL(S_TEMP, psSensorCE->sSensorCumulative.sSensor.sVtab.eSType);
    TEST_ASSERT_EQUAL(InPin_vRcvMessage, psSensorCE->sSensorCumulative.sSensor.sVtab.vReceive);
    TEST_ASSERT_EQUAL(200000, psSensorCE->sSensorCumulative.sSensor.u32ReportInterval);
    TEST_ASSERT_EQUAL(0, psSensorCE->sSensorCumulative.sSensor.u32LastReportMs);
    TEST_ASSERT_EQUAL_FLOAT(-2.1f, psSensorCE->sSensorCumulative.sSensor.fOffset);
    TEST_ASSERT_EQUAL(1500, psSensorCE->sSensorCumulative.u32SamplingInterval);
    TEST_ASSERT_EQUAL(0, psSensorCE->sSensorCumulative.u32LastSamplingMs);
    TEST_ASSERT_EQUAL(0, psSensorCE->sSensorCumulative.u32SamplesCount);
    TEST_ASSERT_EQUAL(0, psSensorCE->sSensorCumulative.fCumulatedValue);
    TEST_ASSERT_EQUAL(11, psSensorCE->sSensorCumulative.sSensor.sPresentable.u8Id);
    TEST_ASSERT_EQUAL_STRING("CPUTemp6", psSensorCE->sSensorCumulative.sSensor.sPresentable.pcName);
    TEST_ASSERT_EQUAL(InPin_vRcvMessage, psSensorCE->sSensorCumulative.sSensor.sPresentable.psVtab->vReceive);
    psEnableableHnd = &psSensorCE->sEnableable;
    TEST_ASSERT_EQUAL(psGlobals->ppsPresentables[12], psEnableableHnd);
    TEST_ASSERT_EQUAL(12, psEnableableHnd->sPresentable.u8Id);
    TEST_ASSERT_EQUAL_STRING("CPUTemp6_enable", psEnableableHnd->sPresentable.pcName);
}

void test_vFlow_timedSwitch(void)
{
    PINCFG_RESULT_T eResult;

    // Configuration string: input pin -> trigger -> timed switch
    const char *pcCfg = "I,i1,16/"            // Input pin on pin 16
                        "ST,sw1,13,5000/"     // Timed switch on pin 13 with 5s timeout
                        "T,t1,i1,4,1,sw1,3/"; // Trigger that connects i1 to sw1

    // To use this configuration disable loading stored one
    uint16_t u16OverMaxCfgLenght = PINCFG_CONFIG_MAX_SZ_D + 1;
    mock_hwReadConfigBlock_buf = (void *)&u16OverMaxCfgLenght;

    eResult = PinCfgCsv_eInit(testMemory, MEMORY_SZ, pcCfg);
    TEST_ASSERT_EQUAL(PINCFG_OK_E, eResult);
    TEST_ASSERT_EQUAL(2, psGlobals->u8LoopablesCount);

    // Verify CLI setup
    PRESENTABLE_T *psCLI = (PRESENTABLE_T *)psGlobals->ppsPresentables[0];
    TEST_ASSERT_EQUAL_STRING("CLI", psCLI->pcName);

    // Verify input pin setup
    INPIN_T *psInPin = (INPIN_T *)psGlobals->ppsPresentables[1];
    TEST_ASSERT_EQUAL_STRING("i1", psInPin->sPresentable.pcName);
    TEST_ASSERT_EQUAL(16, psInPin->u8InPin);

    // Verify switch setup
    SWITCH_T *psSwitchHnd = (SWITCH_T *)psGlobals->ppsPresentables[2];
    TEST_ASSERT_EQUAL_STRING("sw1", psSwitchHnd->sPresentable.pcName);
    TEST_ASSERT_EQUAL(SWITCH_TIMED_E, psSwitchHnd->eMode);
    TEST_ASSERT_EQUAL(13, psSwitchHnd->u8OutPin);
    TEST_ASSERT_EQUAL(5000, psSwitchHnd->u32TimedAdidtionalDelayMs);

    // One interval test
    // Reset mock counters
    mock_digitalWrite_u32Called = 0;
    mock_MyMessage_set_uint8_t_value = 0;
    mock_send_u32Called = 0;
    uint32_t u32Time = 0;

    // Simulate input pin activation
    mock_digitalRead_u8Return = HIGH;
    PinCfgCsv_vLoop(u32Time);
    u32Time += psGlobals->u32InPinDebounceMs;
    PinCfgCsv_vLoop(u32Time);

    // Verify switch was activated
    TEST_ASSERT_EQUAL(1, mock_digitalWrite_u32Called);
    TEST_ASSERT_EQUAL(HIGH, mock_digitalWrite_u8Value);
    TEST_ASSERT_EQUAL(13, mock_digitalWrite_u8Pin);
    TEST_ASSERT_EQUAL(2, mock_send_u32Called);

    // Simulate input pin deactivation also with debounce
    mock_digitalRead_u8Return = LOW;
    u32Time += 200;
    PinCfgCsv_vLoop(u32Time);
    u32Time += psGlobals->u32InPinDebounceMs;
    PinCfgCsv_vLoop(u32Time);

    // Input pin sent state change
    TEST_ASSERT_EQUAL(3, mock_send_u32Called);

    // Verify switch remains on until timeout
    TEST_ASSERT_EQUAL(1, mock_digitalWrite_u32Called);
    TEST_ASSERT_EQUAL(HIGH, mock_digitalWrite_u8Value);

    // Advance time to just before timeout
    PinCfgCsv_vLoop(4999);
    TEST_ASSERT_EQUAL(1, mock_digitalWrite_u32Called);
    TEST_ASSERT_EQUAL(HIGH, mock_digitalWrite_u8Value);
    TEST_ASSERT_EQUAL(3, mock_send_u32Called);

    // Advance time past timeout
    PinCfgCsv_vLoop(5101);
    TEST_ASSERT_EQUAL(2, mock_digitalWrite_u32Called); // Switch turned off
    TEST_ASSERT_EQUAL(LOW, mock_digitalWrite_u8Value);
    TEST_ASSERT_EQUAL(13, mock_digitalWrite_u8Pin);
    TEST_ASSERT_EQUAL(4, mock_send_u32Called); // State change message sent
    // End of one interval test

    // Three interval test
    mock_digitalWrite_u32Called = 0;
    mock_MyMessage_set_uint8_t_value = 0;
    mock_send_u32Called = 0;
    u32Time = 0;

    // Simulate input pin activation 1
    mock_digitalRead_u8Return = HIGH;
    PinCfgCsv_vLoop(u32Time);
    u32Time += psGlobals->u32InPinDebounceMs;
    PinCfgCsv_vLoop(u32Time);

    // Verify switch was activated
    TEST_ASSERT_EQUAL(1, mock_digitalWrite_u32Called);
    TEST_ASSERT_EQUAL(HIGH, mock_digitalWrite_u8Value);
    TEST_ASSERT_EQUAL(13, mock_digitalWrite_u8Pin);
    TEST_ASSERT_EQUAL(2, mock_send_u32Called);
    TEST_ASSERT_EQUAL(u32Time, psSwitchHnd->u32ImpulseStarted);
    TEST_ASSERT_EQUAL(5000, psSwitchHnd->u32ImpulseDuration);

    // Simulate input pin deactivation also with debounce 1
    mock_digitalRead_u8Return = LOW;
    u32Time += 200;
    PinCfgCsv_vLoop(u32Time);
    u32Time += psGlobals->u32InPinDebounceMs;
    PinCfgCsv_vLoop(u32Time);

    // Input pin sent state change
    TEST_ASSERT_EQUAL(3, mock_send_u32Called);

    // Simulate input pin activation 2
    mock_digitalRead_u8Return = HIGH;
    u32Time += 200;
    PinCfgCsv_vLoop(u32Time);
    u32Time += psGlobals->u32InPinDebounceMs;
    PinCfgCsv_vLoop(u32Time);

    // Input pin sent state change
    TEST_ASSERT_EQUAL(4, mock_send_u32Called);
    TEST_ASSERT_EQUAL(1, mock_digitalWrite_u32Called);
    TEST_ASSERT_EQUAL(HIGH, mock_digitalWrite_u8Value);
    TEST_ASSERT_EQUAL(10000, psSwitchHnd->u32ImpulseDuration);

    // Simulate input pin deactivation also with debounce 2
    mock_digitalRead_u8Return = LOW;
    u32Time += 200;
    PinCfgCsv_vLoop(u32Time);
    u32Time += psGlobals->u32InPinDebounceMs;
    PinCfgCsv_vLoop(u32Time);

    // Input pin sent state change
    TEST_ASSERT_EQUAL(5, mock_send_u32Called);

    // Simulate input pin activation 3
    mock_digitalRead_u8Return = HIGH;
    u32Time += 200;
    PinCfgCsv_vLoop(u32Time);
    u32Time += psGlobals->u32InPinDebounceMs;
    PinCfgCsv_vLoop(u32Time);

    // Input pin sent state change
    TEST_ASSERT_EQUAL(6, mock_send_u32Called);
    TEST_ASSERT_EQUAL(1, mock_digitalWrite_u32Called);
    TEST_ASSERT_EQUAL(HIGH, mock_digitalWrite_u8Value);
    TEST_ASSERT_EQUAL(15000, psSwitchHnd->u32ImpulseDuration);

    // Simulate input pin deactivation also with debounce 3
    mock_digitalRead_u8Return = LOW;
    u32Time += 200;
    PinCfgCsv_vLoop(u32Time);
    u32Time += psGlobals->u32InPinDebounceMs;
    PinCfgCsv_vLoop(u32Time);

    // Input pin sent state change
    TEST_ASSERT_EQUAL(7, mock_send_u32Called);

    // Advance to timeout
    u32Time += psSwitchHnd->u32ImpulseStarted + psSwitchHnd->u32ImpulseDuration + 1;
    PinCfgCsv_vLoop(20000);

    // Verify switch is off
    TEST_ASSERT_EQUAL(2, mock_digitalWrite_u32Called);
    TEST_ASSERT_EQUAL(LOW, mock_digitalWrite_u8Value);
    TEST_ASSERT_EQUAL(13, mock_digitalWrite_u8Pin);
    TEST_ASSERT_EQUAL(8, mock_send_u32Called);
    // End of three interval test

    // Verify canceling V1
    // reset mock counters
    mock_digitalWrite_u32Called = 0;
    mock_MyMessage_set_uint8_t_value = 0;
    mock_send_u32Called = 0;
    u32Time = 0;
    // Simulate input pin activation
    mock_digitalRead_u8Return = HIGH;
    PinCfgCsv_vLoop(u32Time);
    u32Time += psGlobals->u32InPinDebounceMs;
    PinCfgCsv_vLoop(u32Time);

    // Verify switch was activated
    TEST_ASSERT_EQUAL(1, mock_digitalWrite_u32Called);
    TEST_ASSERT_EQUAL(HIGH, mock_digitalWrite_u8Value);
    TEST_ASSERT_EQUAL(13, mock_digitalWrite_u8Pin);
    TEST_ASSERT_EQUAL(2, mock_send_u32Called);
    TEST_ASSERT_EQUAL(u32Time, psSwitchHnd->u32ImpulseStarted);
    TEST_ASSERT_EQUAL(5000, psSwitchHnd->u32ImpulseDuration);

    // Simulate input pin deactivation also with debounce
    mock_digitalRead_u8Return = LOW;
    u32Time += 200;
    PinCfgCsv_vLoop(u32Time);
    u32Time += psGlobals->u32InPinDebounceMs;
    PinCfgCsv_vLoop(u32Time);

    // Input pin sent state change
    TEST_ASSERT_EQUAL(3, mock_send_u32Called);

    // Simulate longpress
    mock_digitalRead_u8Return = HIGH;
    u32Time += 200;
    PinCfgCsv_vLoop(u32Time);
    u32Time += psGlobals->u32InPinDebounceMs;
    PinCfgCsv_vLoop(u32Time);

    // Input pin sent state change
    TEST_ASSERT_EQUAL(4, mock_send_u32Called);

    // No switch changes
    TEST_ASSERT_EQUAL(1, mock_digitalWrite_u32Called);
    TEST_ASSERT_EQUAL(HIGH, mock_digitalWrite_u8Value);

    u32Time += psInPin->u32timerMultiStarted + psGlobals->u32InPinMulticlickMaxDelayMs;
    PinCfgCsv_vLoop(u32Time);

    // Verify switch is off
    TEST_ASSERT_EQUAL(2, mock_digitalWrite_u32Called);
    TEST_ASSERT_EQUAL(LOW, mock_digitalWrite_u8Value);
    TEST_ASSERT_EQUAL(13, mock_digitalWrite_u8Pin);
    TEST_ASSERT_EQUAL(5, mock_send_u32Called);

    // Clean input pin state
    mock_digitalRead_u8Return = LOW;
    u32Time += psGlobals->u32InPinDebounceMs;
    PinCfgCsv_vLoop(u32Time);
    u32Time += psGlobals->u32InPinDebounceMs;
    PinCfgCsv_vLoop(u32Time);
    // End of verify canceling V1

    // Verify canceling V2
    // reset mock counters
    mock_digitalWrite_u32Called = 0;
    mock_MyMessage_set_uint8_t_value = 0;
    mock_send_u32Called = 0;
    u32Time = 0;

    // Simulate input pin activation
    mock_digitalRead_u8Return = HIGH;
    PinCfgCsv_vLoop(u32Time);
    u32Time += psGlobals->u32InPinDebounceMs;
    PinCfgCsv_vLoop(u32Time);

    // Verify switch was activated
    TEST_ASSERT_EQUAL(1, mock_digitalWrite_u32Called);
    TEST_ASSERT_EQUAL(HIGH, mock_digitalWrite_u8Value);
    TEST_ASSERT_EQUAL(13, mock_digitalWrite_u8Pin);
    TEST_ASSERT_EQUAL(2, mock_send_u32Called);
    TEST_ASSERT_EQUAL(u32Time, psSwitchHnd->u32ImpulseStarted);
    TEST_ASSERT_EQUAL(5000, psSwitchHnd->u32ImpulseDuration);
    TEST_ASSERT_EQUAL(u32Time, psInPin->u32timerMultiStarted);

    u32Time += psInPin->u32timerMultiStarted + psGlobals->u32InPinMulticlickMaxDelayMs;
    PinCfgCsv_vLoop(u32Time);

    // Verify switch is off
    TEST_ASSERT_EQUAL(2, mock_digitalWrite_u32Called);
    TEST_ASSERT_EQUAL(LOW, mock_digitalWrite_u8Value);
    TEST_ASSERT_EQUAL(13, mock_digitalWrite_u8Pin);
    TEST_ASSERT_EQUAL(3, mock_send_u32Called);
    // End of verify canceling V2
}

#ifndef ARDUINO
int main(void)
#else
int test_main(void)
#endif
{
    // sizes
    printf("sizeof(GLOBALS_T): %ld\n", sizeof(GLOBALS_T));
    printf("sizeof(PRESENTABLE_T): %ld\n", sizeof(PRESENTABLE_T));
    printf("sizeof(CLI_T): %ld\n", sizeof(CLI_T));
    printf("sizeof(SWITCH_T): %ld\n", sizeof(SWITCH_T));
    printf("sizeof(INPIN_T): %ld\n", sizeof(INPIN_T));
    printf("sizeof(TRIGGER_T): %ld\n", sizeof(TRIGGER_T));
    printf("sizeof(TRIGGER_SWITCHACTION_T): %ld\n", sizeof(TRIGGER_SWITCHACTION_T));
    printf("sizeof(SENSOR_T): %ld\n", sizeof(SENSOR_T));
    printf("sizeof(SENSOR_CUMULATIVE_T): %ld\n", sizeof(SENSOR_CUMULATIVE_T));
    printf("sizeof(SENSOR_ENABLEABLE_T): %ld\n", sizeof(SENSOR_ENABLEABLE_T));
    printf("sizeof(SENSOR_CUMULATIVE_ENABLEABLE_T): %ld\n", sizeof(SENSOR_CUMULATIVE_ENABLEABLE_T));
    printf("sizeof(CPUTEMP_T): %ld\n", sizeof(CPUTEMP_T));
    printf("\n\n");

    UNITY_BEGIN();
    RUN_TEST(test_vMemory);
    RUN_TEST(test_vStringPoint);
    RUN_TEST(test_vLinkedList);
    RUN_TEST(test_vPinCfgStr);
    RUN_TEST(test_vMySenosrsPresent);
    RUN_TEST(test_vInPin);
    RUN_TEST(test_vSwitch);
    RUN_TEST(test_vTrigger);
    RUN_TEST(test_vPinCfgCsv);
    RUN_TEST(test_vCLI);
    RUN_TEST(test_vGlobalConfig);
    RUN_TEST(test_vCPUTemp);

    RUN_TEST(test_vFlow_timedSwitch);

    return UNITY_END();
}
