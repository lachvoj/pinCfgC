#include "test_helpers.h"

// Component Tests - MySensorsPresent, InPin, Switch, Trigger

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
    Presentable_vRcvMessage(psPresentHandle, (const MyMessage *)&u8Msg);
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
    psPinSubscriber1->psNext = NULL;
    psPinSubscriber2->psNext = NULL;
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
    TEST_ASSERT_EQUAL(7, psGlobals->u8LoopablesCount);

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
        .bValidate = false};

    eParseResult = PinCfgCsv_eParse(&sParams);
    TEST_ASSERT_EQUAL(PINCFG_OK_E, eParseResult);

    eLinkedListResult =
        LinkedList_eLinkedListToArray((LINKEDLIST_ITEM_T **)(&psGlobals->ppsLoopables), &psGlobals->u8LoopablesCount);
    TEST_ASSERT_EQUAL(LINKEDLIST_OK_E, eLinkedListResult);
    TEST_ASSERT_EQUAL(3, psGlobals->u8LoopablesCount);

    eLinkedListResult = LinkedList_eLinkedListToArray(
        (LINKEDLIST_ITEM_T **)(&psGlobals->ppsPresentables), &psGlobals->u8PresentablesCount);
    TEST_ASSERT_EQUAL(LINKEDLIST_OK_E, eLinkedListResult);
    TEST_ASSERT_EQUAL(3, psGlobals->u8PresentablesCount);

    INPIN_T *psInPin = (INPIN_T *)psGlobals->ppsPresentables[1];
    SWITCH_T *psSwitch = (SWITCH_T *)psGlobals->ppsPresentables[2];
    TRIGGER_T *psTrigger =
        (TRIGGER_T *)psInPin->psFirstSubscriber; // Assuming trigger is registered as first subscriber

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

void register_components_tests(void)
{
    RUN_TEST(test_vMySenosrsPresent);
    RUN_TEST(test_vInPin);
    RUN_TEST(test_vSwitch);
    RUN_TEST(test_vTrigger);
}
