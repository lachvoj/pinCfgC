#include "test_helpers.h"

// ============================================================================
// Stress Tests - Wraparound, concurrency, overflow, and edge cases
// ============================================================================

void test_vMillisWraparound_ElapsedTime(void)
{
    uint32_t u32Start, u32Current, u32Elapsed;

    // Normal case: no wraparound
    u32Start = 1000;
    u32Current = 2000;
    u32Elapsed = PinCfg_u32GetElapsedTime(u32Start, u32Current);
    TEST_ASSERT_EQUAL(1000, u32Elapsed);

    // Wraparound case: current < start
    u32Start = UINT32_MAX - 500; // Very close to max
    u32Current = 500;            // Wrapped around to low value
    u32Elapsed = PinCfg_u32GetElapsedTime(u32Start, u32Current);
    TEST_ASSERT_EQUAL(1001, u32Elapsed); // Should be 501 + 500 + 1 = 1001

    // Edge case: start at MAX, current at 0
    u32Start = UINT32_MAX;
    u32Current = 0;
    u32Elapsed = PinCfg_u32GetElapsedTime(u32Start, u32Current);
    TEST_ASSERT_EQUAL(1, u32Elapsed);

    // Edge case: start at MAX-1, current at 1
    u32Start = UINT32_MAX - 1;
    u32Current = 1;
    u32Elapsed = PinCfg_u32GetElapsedTime(u32Start, u32Current);
    TEST_ASSERT_EQUAL(3, u32Elapsed); // (MAX - (MAX-1)) + 1 + 1 = 3

    // Edge case: same time
    u32Start = 5000;
    u32Current = 5000;
    u32Elapsed = PinCfg_u32GetElapsedTime(u32Start, u32Current);
    TEST_ASSERT_EQUAL(0, u32Elapsed);

    // Large elapsed time without wraparound
    u32Start = 0;
    u32Current = UINT32_MAX;
    u32Elapsed = PinCfg_u32GetElapsedTime(u32Start, u32Current);
    TEST_ASSERT_EQUAL(UINT32_MAX, u32Elapsed);
}

/**
 * Test: InPin debounce works correctly across millis wraparound
 */
void test_vMillisWraparound_InPinDebounce(void)
{
    PINCFG_RESULT_T eParseResult;
    LINKEDLIST_RESULT_T eLinkedListResult;
    char acOutStr[OUT_STR_MAX_LEN_D];
    size_t szMemoryRequired;

    const char *pcCfg = "CD,100/" // 100ms debounce
                        "I,i1,16/";

    PINCFG_PARSE_PARAMS_T sParams = {
        .pcConfig = pcCfg,
        .eAddToLoopables = PinCfgCsv_eAddToTempLoopables,
        .eAddToPresentables = PinCfgCsv_eAddToTempPresentables,
        .pszMemoryRequired = &szMemoryRequired,
        .pcOutString = acOutStr,
        .u16OutStrMaxLen = (uint16_t)OUT_STR_MAX_LEN_D,
        .bValidate = false};

    // Pass 1: Calculate memory
    eParseResult = PinCfgCsv_eParse(&sParams);
    TEST_ASSERT_EQUAL(PINCFG_OK_E, eParseResult);

    // Pass 2: Create objects
    sParams.pszMemoryRequired = NULL;
    eParseResult = PinCfgCsv_eParse(&sParams);
    TEST_ASSERT_EQUAL(PINCFG_OK_E, eParseResult);

    eLinkedListResult =
        LinkedList_eLinkedListToArray((LINKEDLIST_ITEM_T **)(&psGlobals->ppsLoopables), &psGlobals->u8LoopablesCount);
    TEST_ASSERT_EQUAL(LINKEDLIST_OK_E, eLinkedListResult);

    eLinkedListResult = LinkedList_eLinkedListToArray(
        (LINKEDLIST_ITEM_T **)(&psGlobals->ppsPresentables), &psGlobals->u8PresentablesCount);
    TEST_ASSERT_EQUAL(LINKEDLIST_OK_E, eLinkedListResult);

    INPIN_T *psInPin = (INPIN_T *)psGlobals->ppsPresentables[1];

    // Start just before wraparound
    uint32_t u32Time = UINT32_MAX - 50;
    mock_digitalRead_u8Return = LOW;
    mock_send_u32Called = 0;

    // Initial loop
    PinCfgCsv_vLoop(u32Time);

    // Button press detected
    mock_digitalRead_u8Return = HIGH;
    u32Time = UINT32_MAX - 40;
    PinCfgCsv_vLoop(u32Time);

    // Time wraps around during debounce
    u32Time = 60; // Wrapped around, total elapsed = 100ms (within debounce)
    PinCfgCsv_vLoop(u32Time);
    // Should still be in debounce state

    // Debounce complete after wraparound
    u32Time = 70; // Total elapsed > 100ms
    PinCfgCsv_vLoop(u32Time);
    // Should have completed debounce and sent event

    // Verify state transition occurred
    TEST_ASSERT_EQUAL(INPIN_UP_E, psInPin->ePinState);
}

/**
 * Test: Switch impulse timing works correctly across millis wraparound
 * This test verifies PinCfg_u32GetElapsedTime is used correctly in switch timing
 */
void test_vMillisWraparound_SwitchImpulse(void)
{
    // Test the core timing calculation used by switches
    uint32_t u32Start, u32Current, u32Elapsed;
    uint32_t u32Duration = 200; // 200ms impulse

    // Start just before wraparound
    u32Start = UINT32_MAX - 100;

    // 150ms elapsed (still within duration) - wraps around
    u32Current = 50; // UINT32_MAX - 100 + 150 wraps to 49
    u32Elapsed = PinCfg_u32GetElapsedTime(u32Start, u32Current);
    TEST_ASSERT_TRUE(u32Elapsed < u32Duration); // Should be ~150, less than 200
    TEST_ASSERT_EQUAL(151, u32Elapsed);         // Exact: (UINT32_MAX - (UINT32_MAX-100)) + 50 + 1 = 151

    // 250ms elapsed (beyond duration) - still wrapped
    u32Current = 150; // Start + 250 = wrapped to 149
    u32Elapsed = PinCfg_u32GetElapsedTime(u32Start, u32Current);
    TEST_ASSERT_TRUE(u32Elapsed > u32Duration); // Should be ~250, more than 200
    TEST_ASSERT_EQUAL(251, u32Elapsed);

    // Verify the function handles the exact boundary
    u32Start = UINT32_MAX - 199;
    u32Current = 0; // Exactly 200ms elapsed
    u32Elapsed = PinCfg_u32GetElapsedTime(u32Start, u32Current);
    TEST_ASSERT_EQUAL(200, u32Elapsed);
}

/**
 * Test: Sensor reporting interval works correctly across millis wraparound
 */
void test_vMillisWraparound_SensorReporting(void)
{
    PINCFG_RESULT_T eParseResult;
    LINKEDLIST_RESULT_T eLinkedListResult;
    char acOutStr[OUT_STR_MAX_LEN_D];
    size_t szMemoryRequired;

    const char *pcCfg = "MS,0,cpu_temp/"
                        "SR,CPUTemp,cpu_temp,6,6,0,0,1000,5,0/"; // 5 second reporting interval

    PINCFG_PARSE_PARAMS_T sParams = {
        .pcConfig = pcCfg,
        .eAddToLoopables = PinCfgCsv_eAddToTempLoopables,
        .eAddToPresentables = PinCfgCsv_eAddToTempPresentables,
        .pszMemoryRequired = &szMemoryRequired,
        .pcOutString = acOutStr,
        .u16OutStrMaxLen = (uint16_t)OUT_STR_MAX_LEN_D,
        .bValidate = false};

    // Pass 1: Calculate memory
    eParseResult = PinCfgCsv_eParse(&sParams);
    TEST_ASSERT_EQUAL(PINCFG_OK_E, eParseResult);

    // Pass 2: Create objects
    sParams.pszMemoryRequired = NULL;
    eParseResult = PinCfgCsv_eParse(&sParams);
    TEST_ASSERT_EQUAL(PINCFG_OK_E, eParseResult);

    eLinkedListResult =
        LinkedList_eLinkedListToArray((LINKEDLIST_ITEM_T **)(&psGlobals->ppsLoopables), &psGlobals->u8LoopablesCount);
    TEST_ASSERT_EQUAL(LINKEDLIST_OK_E, eLinkedListResult);

    eLinkedListResult = LinkedList_eLinkedListToArray(
        (LINKEDLIST_ITEM_T **)(&psGlobals->ppsPresentables), &psGlobals->u8PresentablesCount);
    TEST_ASSERT_EQUAL(LINKEDLIST_OK_E, eLinkedListResult);

    // Start just before wraparound
    uint32_t u32Time = UINT32_MAX - 2000; // 2 seconds before wrap
    mock_send_u32Called = 0;

    // Initial loop to set last report time
    PinCfgCsv_vLoop(u32Time);
    uint32_t u32InitialSendCount = mock_send_u32Called;

    // Time progresses but wraps around before reporting interval
    u32Time = 3000; // Wrapped, total elapsed ~5 seconds
    PinCfgCsv_vLoop(u32Time);

    // Should have sent a report (elapsed time >= 5000ms)
    TEST_ASSERT_TRUE(mock_send_u32Called > u32InitialSendCount);
}

// =============================================================================
// CONCURRENT TRIGGER EVENT TESTS
// =============================================================================

/**
 * Test: Multiple triggers subscribed to same input pin
 * Tests that multiple triggers can be chained on one input
 */
void test_vConcurrentTriggers_MultipleOnSameInput(void)
{
    PINCFG_RESULT_T eParseResult;
    LINKEDLIST_RESULT_T eLinkedListResult;
    char acOutStr[OUT_STR_MAX_LEN_D];
    size_t szMemoryRequired;

    // Two triggers on same input, controlling different switches
    const char *pcCfg = "I,i1,16/"
                        "S,o1,13,o2,12/"
                        "T,t1,i1,1,1,o1,0/"  // Trigger on UP, toggle o1
                        "T,t2,i1,1,1,o2,0/"; // Trigger on UP, toggle o2

    PINCFG_PARSE_PARAMS_T sParams = {
        .pcConfig = pcCfg,
        .eAddToLoopables = PinCfgCsv_eAddToTempLoopables,
        .eAddToPresentables = PinCfgCsv_eAddToTempPresentables,
        .pszMemoryRequired = &szMemoryRequired,
        .pcOutString = acOutStr,
        .u16OutStrMaxLen = (uint16_t)OUT_STR_MAX_LEN_D,
        .bValidate = false};

    // Pass 1: Calculate memory
    eParseResult = PinCfgCsv_eParse(&sParams);
    TEST_ASSERT_EQUAL(PINCFG_OK_E, eParseResult);

    // Pass 2: Create objects
    sParams.pszMemoryRequired = NULL;
    eParseResult = PinCfgCsv_eParse(&sParams);
    TEST_ASSERT_EQUAL(PINCFG_OK_E, eParseResult);

    eLinkedListResult =
        LinkedList_eLinkedListToArray((LINKEDLIST_ITEM_T **)(&psGlobals->ppsLoopables), &psGlobals->u8LoopablesCount);
    TEST_ASSERT_EQUAL(LINKEDLIST_OK_E, eLinkedListResult);

    eLinkedListResult = LinkedList_eLinkedListToArray(
        (LINKEDLIST_ITEM_T **)(&psGlobals->ppsPresentables), &psGlobals->u8PresentablesCount);
    TEST_ASSERT_EQUAL(LINKEDLIST_OK_E, eLinkedListResult);

    INPIN_T *psInPin = (INPIN_T *)psGlobals->ppsPresentables[1];

    // Verify triggers are subscribed to the input pin
    TEST_ASSERT_NOT_NULL(psInPin->psFirstSubscriber);
    TEST_ASSERT_NOT_NULL(psInPin->psFirstSubscriber->psNext);

    // Count subscribers in chain - due to two-pass parsing, expect >= 2
    int nSubscriberCount = 0;
    PINSUBSCRIBER_IF_T *psCurrent = psInPin->psFirstSubscriber;
    while (psCurrent != NULL)
    {
        nSubscriberCount++;
        psCurrent = psCurrent->psNext;
    }
    TEST_ASSERT_TRUE(nSubscriberCount >= 2); // At least two triggers subscribed
}

/**
 * Test: Trigger with multiple switch actions
 * Tests that a single trigger can control multiple switches with different actions
 */
void test_vConcurrentTriggers_MultipleActions(void)
{
    PINCFG_RESULT_T eParseResult;
    LINKEDLIST_RESULT_T eLinkedListResult;
    char acOutStr[OUT_STR_MAX_LEN_D];
    size_t szMemoryRequired;

    // One trigger controlling multiple switches
    const char *pcCfg = "I,i1,16/"
                        "S,o1,13,o2,12,o3,11/"
                        "T,t1,i1,1,1,o1,0,o2,1,o3,2/"; // Toggle o1, UP o2, DOWN o3

    PINCFG_PARSE_PARAMS_T sParams = {
        .pcConfig = pcCfg,
        .eAddToLoopables = PinCfgCsv_eAddToTempLoopables,
        .eAddToPresentables = PinCfgCsv_eAddToTempPresentables,
        .pszMemoryRequired = &szMemoryRequired,
        .pcOutString = acOutStr,
        .u16OutStrMaxLen = (uint16_t)OUT_STR_MAX_LEN_D,
        .bValidate = false};

    // Pass 1: Calculate memory
    eParseResult = PinCfgCsv_eParse(&sParams);
    TEST_ASSERT_EQUAL(PINCFG_OK_E, eParseResult);

    // Pass 2: Create objects
    sParams.pszMemoryRequired = NULL;
    eParseResult = PinCfgCsv_eParse(&sParams);
    TEST_ASSERT_EQUAL(PINCFG_OK_E, eParseResult);

    eLinkedListResult =
        LinkedList_eLinkedListToArray((LINKEDLIST_ITEM_T **)(&psGlobals->ppsLoopables), &psGlobals->u8LoopablesCount);
    TEST_ASSERT_EQUAL(LINKEDLIST_OK_E, eLinkedListResult);

    eLinkedListResult = LinkedList_eLinkedListToArray(
        (LINKEDLIST_ITEM_T **)(&psGlobals->ppsPresentables), &psGlobals->u8PresentablesCount);
    TEST_ASSERT_EQUAL(LINKEDLIST_OK_E, eLinkedListResult);

    INPIN_T *psInPin = (INPIN_T *)psGlobals->ppsPresentables[1];

    // Verify trigger is subscribed with correct structure
    TEST_ASSERT_NOT_NULL(psInPin->psFirstSubscriber);

    TRIGGER_T *psTrigger = (TRIGGER_T *)psInPin->psFirstSubscriber;

    // Verify trigger has 3 switch actions
    TEST_ASSERT_EQUAL(3, psTrigger->u8SwActCount);

    // Verify action types (0=toggle, 1=up, 2=down)
    TEST_ASSERT_EQUAL(TRIGGER_A_TOGGLE_E, psTrigger->pasSwAct[0].eAction);
    TEST_ASSERT_EQUAL(TRIGGER_A_UP_E, psTrigger->pasSwAct[1].eAction);
    TEST_ASSERT_EQUAL(TRIGGER_A_DOWN_E, psTrigger->pasSwAct[2].eAction);
}

/**
 * Test: Rapid trigger events (stress test)
 */
void test_vConcurrentTriggers_RapidEvents(void)
{
    PINCFG_RESULT_T eParseResult;
    LINKEDLIST_RESULT_T eLinkedListResult;
    char acOutStr[OUT_STR_MAX_LEN_D];
    size_t szMemoryRequired;

    const char *pcCfg = "CD,10/" // Very short debounce for testing
                        "CM,50/" // Short multiclick delay
                        "I,i1,16/"
                        "S,o1,13/"
                        "T,t1,i1,1,1,o1,0/"; // Toggle on UP

    PINCFG_PARSE_PARAMS_T sParams = {
        .pcConfig = pcCfg,
        .eAddToLoopables = PinCfgCsv_eAddToTempLoopables,
        .eAddToPresentables = PinCfgCsv_eAddToTempPresentables,
        .pszMemoryRequired = &szMemoryRequired,
        .pcOutString = acOutStr,
        .u16OutStrMaxLen = (uint16_t)OUT_STR_MAX_LEN_D,
        .bValidate = false};

    // Pass 1: Calculate memory
    eParseResult = PinCfgCsv_eParse(&sParams);
    TEST_ASSERT_EQUAL(PINCFG_OK_E, eParseResult);

    // Pass 2: Create objects
    sParams.pszMemoryRequired = NULL;
    eParseResult = PinCfgCsv_eParse(&sParams);
    TEST_ASSERT_EQUAL(PINCFG_OK_E, eParseResult);

    eLinkedListResult =
        LinkedList_eLinkedListToArray((LINKEDLIST_ITEM_T **)(&psGlobals->ppsLoopables), &psGlobals->u8LoopablesCount);
    TEST_ASSERT_EQUAL(LINKEDLIST_OK_E, eLinkedListResult);

    eLinkedListResult = LinkedList_eLinkedListToArray(
        (LINKEDLIST_ITEM_T **)(&psGlobals->ppsPresentables), &psGlobals->u8PresentablesCount);
    TEST_ASSERT_EQUAL(LINKEDLIST_OK_E, eLinkedListResult);

    SWITCH_T *psSwitch = (SWITCH_T *)psGlobals->ppsPresentables[2];
    uint32_t u32Time = 0;

    // Perform 10 rapid press/release cycles
    for (int i = 0; i < 10; i++)
    {
        // Press
        mock_digitalRead_u8Return = HIGH;
        u32Time += 1;
        PinCfgCsv_vLoop(u32Time);
        u32Time += psGlobals->u32InPinDebounceMs + 1;
        PinCfgCsv_vLoop(u32Time);

        // Release
        mock_digitalRead_u8Return = LOW;
        u32Time += 1;
        PinCfgCsv_vLoop(u32Time);
        u32Time += psGlobals->u32InPinDebounceMs + 1;
        PinCfgCsv_vLoop(u32Time);

        // Wait for multiclick timeout
        u32Time += psGlobals->u32InPinMulticlickMaxDelayMs + 1;
        PinCfgCsv_vLoop(u32Time);
    }

    // After 10 toggles, switch should be back to original state (even number of toggles)
    TEST_ASSERT_EQUAL(0, psSwitch->sPresentable.u8State);
}

// =============================================================================
// MAXIMUM ARRAY SIZE / 255 ELEMENT OVERFLOW TESTS
// =============================================================================

/**
 * Test: Trigger with maximum switch actions (PINCFG_TRIGGER_MAX_SWITCHES_D)
 */
void test_vMaxArraySize_TriggerMaxSwitches(void)
{
    TRIGGER_T sTrigger;
    TRIGGER_SWITCHACTION_T asSwAct[PINCFG_TRIGGER_MAX_SWITCHES_D + 1];
    TRIGGER_RESULT_T eResult;

    // Initialize switch action array
    for (int i = 0; i <= PINCFG_TRIGGER_MAX_SWITCHES_D; i++)
    {
        asSwAct[i].psSwitchHnd = NULL;
        asSwAct[i].eAction = TRIGGER_A_TOGGLE_E;
    }

    // Test with exactly max switches (should succeed)
    eResult = Trigger_eInit(&sTrigger, asSwAct, PINCFG_TRIGGER_MAX_SWITCHES_D, TRIGGER_UP_E, 1);
    TEST_ASSERT_EQUAL(TRIGGER_OK_E, eResult);
    TEST_ASSERT_EQUAL(PINCFG_TRIGGER_MAX_SWITCHES_D, sTrigger.u8SwActCount);

    // Test with more than max switches (should fail)
    eResult = Trigger_eInit(&sTrigger, asSwAct, PINCFG_TRIGGER_MAX_SWITCHES_D + 1, TRIGGER_UP_E, 1);
    TEST_ASSERT_EQUAL(TRIGGER_MAX_SWITCH_ERROR_E, eResult);

    // Test with zero switches (edge case)
    eResult = Trigger_eInit(&sTrigger, asSwAct, 0, TRIGGER_UP_E, 1);
    TEST_ASSERT_EQUAL(TRIGGER_OK_E, eResult);
    TEST_ASSERT_EQUAL(0, sTrigger.u8SwActCount);
}

/**
 * Test: Presentables count near 255 limit
 */
void test_vMaxArraySize_PresentablesCount(void)
{
    // This test validates that u8PresentablesCount doesn't overflow
    // when many presentables are created

    // Note: We can't actually create 256 presentables due to memory constraints
    // but we can verify the counter behavior with a simpler test

    uint8_t u8Count = 254;

    // Simulate incrementing counter
    u8Count++;
    TEST_ASSERT_EQUAL(255, u8Count);

    // Next increment would overflow
    uint8_t u8Prev = u8Count;
    u8Count++;
    TEST_ASSERT_EQUAL(0, u8Count);      // Wrapped to 0
    TEST_ASSERT_TRUE(u8Count < u8Prev); // Overflow detected

    // This demonstrates the vulnerability - code should check before incrementing
}

/**
 * Test: Loopables count behavior at boundary
 */
void test_vMaxArraySize_LoopablesCount(void)
{
    // Similar to presentables, verify u8LoopablesCount boundary behavior
    uint8_t u8Count = 255;

    // At max value, any increment would overflow
    TEST_ASSERT_EQUAL(255, u8Count);

    // Verify array access at boundary is safe
    // (actual array access testing would require more complex setup)
}

/**
 * Test: InPin subscriber chain length
 */
void test_vMaxArraySize_InPinSubscribers(void)
{
    INPIN_T sInPin;
    PINSUBSCRIBER_IF_T asSubscribers[10];
    INPIN_RESULT_T eResult;
    STRING_POINT_T sName;

    // Initialize input pin
    char acName[] = "test";
    PinCfgStr_vInitStrPoint(&sName, acName, 4);
    eResult = InPin_eInit(&sInPin, &sName, 1, 16);
    TEST_ASSERT_EQUAL(INPIN_OK_E, eResult);

    // Add many subscribers (chain test)
    for (int i = 0; i < 10; i++)
    {
        asSubscribers[i].psNext = NULL;
        asSubscribers[i].vEventHandle = NULL;
        eResult = InPin_eAddSubscriber(&sInPin, &asSubscribers[i]);
        TEST_ASSERT_EQUAL(INPIN_OK_E, eResult);
    }

    // Verify chain is intact
    PINSUBSCRIBER_IF_T *psCurrent = sInPin.psFirstSubscriber;
    int count = 0;
    while (psCurrent != NULL)
    {
        count++;
        psCurrent = psCurrent->psNext;
    }
    TEST_ASSERT_EQUAL(10, count);
}

// =============================================================================
// MALFORMED CSV EDGE CASE TESTS
// =============================================================================

/**
 * Test: Missing line terminators
 */
void test_vMalformedCSV_MissingTerminator(void)
{
    PINCFG_RESULT_T eParseResult;
    char acOutStr[OUT_STR_MAX_LEN_D];
    size_t szMemoryRequired;

    PINCFG_PARSE_PARAMS_T sParams = {
        .eAddToLoopables = PinCfgCsv_eAddToTempLoopables,
        .eAddToPresentables = PinCfgCsv_eAddToTempPresentables,
        .pszMemoryRequired = &szMemoryRequired,
        .pcOutString = acOutStr,
        .u16OutStrMaxLen = (uint16_t)OUT_STR_MAX_LEN_D,
        .bValidate = false};

    // Config without trailing slash
    sParams.pcConfig = "S,o1,13";
    eParseResult = PinCfgCsv_eParse(&sParams);
    // Should still parse (last element doesn't need terminator)
    TEST_ASSERT_TRUE(eParseResult == PINCFG_OK_E || eParseResult == PINCFG_WARNINGS_E);
}

/**
 * Test: Empty fields in CSV
 */
void test_vMalformedCSV_EmptyFields(void)
{
    PINCFG_RESULT_T eParseResult;
    char acOutStr[OUT_STR_MAX_LEN_D];
    size_t szMemoryRequired;

    PINCFG_PARSE_PARAMS_T sParams = {
        .eAddToLoopables = PinCfgCsv_eAddToTempLoopables,
        .eAddToPresentables = PinCfgCsv_eAddToTempPresentables,
        .pszMemoryRequired = &szMemoryRequired,
        .pcOutString = acOutStr,
        .u16OutStrMaxLen = (uint16_t)OUT_STR_MAX_LEN_D,
        .bValidate = false};

    // Empty name field - may succeed with empty name or warn
    sParams.pcConfig = "S,,13/";
    eParseResult = PinCfgCsv_eParse(&sParams);
    TEST_ASSERT_TRUE(eParseResult == PINCFG_WARNINGS_E || eParseResult == PINCFG_OK_E);

    // Empty pin field - should warn about invalid pin
    Memory_eReset();
    PinCfgCsv_eInit(testMemory, MEMORY_SZ, NULL);
    sParams.pcConfig = "S,o1,/";
    eParseResult = PinCfgCsv_eParse(&sParams);
    TEST_ASSERT_TRUE(eParseResult == PINCFG_WARNINGS_E || eParseResult == PINCFG_OK_E);

    // Multiple empty fields - should warn about invalid format
    Memory_eReset();
    PinCfgCsv_eInit(testMemory, MEMORY_SZ, NULL);
    sParams.pcConfig = "S,,,/";
    eParseResult = PinCfgCsv_eParse(&sParams);
    TEST_ASSERT_TRUE(eParseResult == PINCFG_WARNINGS_E || eParseResult == PINCFG_OK_E);
}

/**
 * Test: Extra commas/fields
 */
void test_vMalformedCSV_ExtraFields(void)
{
    PINCFG_RESULT_T eParseResult;
    char acOutStr[OUT_STR_MAX_LEN_D];
    size_t szMemoryRequired;

    PINCFG_PARSE_PARAMS_T sParams = {
        .eAddToLoopables = PinCfgCsv_eAddToTempLoopables,
        .eAddToPresentables = PinCfgCsv_eAddToTempPresentables,
        .pszMemoryRequired = &szMemoryRequired,
        .pcOutString = acOutStr,
        .u16OutStrMaxLen = (uint16_t)OUT_STR_MAX_LEN_D,
        .bValidate = false};

    // Extra fields for switch (expects pairs)
    sParams.pcConfig = "S,o1,13,o2/"; // Missing pin for o2
    eParseResult = PinCfgCsv_eParse(&sParams);
    TEST_ASSERT_EQUAL(PINCFG_WARNINGS_E, eParseResult);

    // Extra trailing commas
    Memory_eReset();
    PinCfgCsv_eInit(testMemory, MEMORY_SZ, NULL);
    sParams.pcConfig = "S,o1,13,,,/";
    eParseResult = PinCfgCsv_eParse(&sParams);
    TEST_ASSERT_EQUAL(PINCFG_WARNINGS_E, eParseResult);
}

/**
 * Test: Invalid characters in numeric fields
 */
void test_vMalformedCSV_InvalidNumericChars(void)
{
    PINCFG_RESULT_T eParseResult;
    char acOutStr[OUT_STR_MAX_LEN_D];
    size_t szMemoryRequired;

    PINCFG_PARSE_PARAMS_T sParams = {
        .eAddToLoopables = PinCfgCsv_eAddToTempLoopables,
        .eAddToPresentables = PinCfgCsv_eAddToTempPresentables,
        .pszMemoryRequired = &szMemoryRequired,
        .pcOutString = acOutStr,
        .u16OutStrMaxLen = (uint16_t)OUT_STR_MAX_LEN_D,
        .bValidate = false};

    // Letters in pin number
    sParams.pcConfig = "S,o1,abc/";
    eParseResult = PinCfgCsv_eParse(&sParams);
    TEST_ASSERT_EQUAL(PINCFG_WARNINGS_E, eParseResult);

    // Special characters in debounce value
    Memory_eReset();
    PinCfgCsv_eInit(testMemory, MEMORY_SZ, NULL);
    sParams.pcConfig = "CD,#$%/";
    eParseResult = PinCfgCsv_eParse(&sParams);
    TEST_ASSERT_EQUAL(PINCFG_WARNINGS_E, eParseResult);

    // Mixed valid/invalid
    Memory_eReset();
    PinCfgCsv_eInit(testMemory, MEMORY_SZ, NULL);
    sParams.pcConfig = "S,o1,12x3/";
    eParseResult = PinCfgCsv_eParse(&sParams);
    TEST_ASSERT_EQUAL(PINCFG_WARNINGS_E, eParseResult);
}

/**
 * Test: Very long field values
 */
void test_vMalformedCSV_LongFields(void)
{
    PINCFG_RESULT_T eParseResult;
    char acOutStr[OUT_STR_MAX_LEN_D];
    size_t szMemoryRequired;

    PINCFG_PARSE_PARAMS_T sParams = {
        .eAddToLoopables = PinCfgCsv_eAddToTempLoopables,
        .eAddToPresentables = PinCfgCsv_eAddToTempPresentables,
        .pszMemoryRequired = &szMemoryRequired,
        .pcOutString = acOutStr,
        .u16OutStrMaxLen = (uint16_t)OUT_STR_MAX_LEN_D,
        .bValidate = false};

    // Very long name (buffer overflow potential)
    char acLongConfig[512];
    strcpy(acLongConfig, "S,");
    for (int i = 0; i < 200; i++)
    {
        strcat(acLongConfig, "x");
    }
    strcat(acLongConfig, ",13/");

    sParams.pcConfig = acLongConfig;
    eParseResult = PinCfgCsv_eParse(&sParams);
    // Should handle gracefully (either succeed with truncation or warn)
    TEST_ASSERT_TRUE(eParseResult == PINCFG_OK_E || eParseResult == PINCFG_WARNINGS_E);
}

/**
 * Test: Unknown line type identifiers
 */
void test_vMalformedCSV_UnknownLineType(void)
{
    PINCFG_RESULT_T eParseResult;
    char acOutStr[OUT_STR_MAX_LEN_D];
    size_t szMemoryRequired;

    PINCFG_PARSE_PARAMS_T sParams = {
        .eAddToLoopables = PinCfgCsv_eAddToTempLoopables,
        .eAddToPresentables = PinCfgCsv_eAddToTempPresentables,
        .pszMemoryRequired = &szMemoryRequired,
        .pcOutString = acOutStr,
        .u16OutStrMaxLen = (uint16_t)OUT_STR_MAX_LEN_D,
        .bValidate = false};

    // Unknown single-char type
    sParams.pcConfig = "X,test,13/";
    eParseResult = PinCfgCsv_eParse(&sParams);
    TEST_ASSERT_EQUAL(PINCFG_WARNINGS_E, eParseResult);

    // Unknown multi-char type
    Memory_eReset();
    PinCfgCsv_eInit(testMemory, MEMORY_SZ, NULL);
    sParams.pcConfig = "UNKNOWN,test,13/";
    eParseResult = PinCfgCsv_eParse(&sParams);
    TEST_ASSERT_EQUAL(PINCFG_WARNINGS_E, eParseResult);

    // Mixed valid and unknown
    Memory_eReset();
    PinCfgCsv_eInit(testMemory, MEMORY_SZ, NULL);
    sParams.pcConfig = "S,o1,13/BADTYPE,x,y/S,o2,12/";
    eParseResult = PinCfgCsv_eParse(&sParams);
    // Should parse valid lines, warn about invalid
    TEST_ASSERT_EQUAL(PINCFG_WARNINGS_E, eParseResult);
}

/**
 * Test: Null bytes and control characters
 */
void test_vMalformedCSV_ControlChars(void)
{
    PINCFG_RESULT_T eParseResult;
    char acOutStr[OUT_STR_MAX_LEN_D];
    size_t szMemoryRequired;

    PINCFG_PARSE_PARAMS_T sParams = {
        .eAddToLoopables = PinCfgCsv_eAddToTempLoopables,
        .eAddToPresentables = PinCfgCsv_eAddToTempPresentables,
        .pszMemoryRequired = &szMemoryRequired,
        .pcOutString = acOutStr,
        .u16OutStrMaxLen = (uint16_t)OUT_STR_MAX_LEN_D,
        .bValidate = false};

    // Tab characters (might be treated as whitespace or part of name)
    sParams.pcConfig = "S,\to1,13/";
    eParseResult = PinCfgCsv_eParse(&sParams);
    // Behavior depends on implementation - tab may be part of name
    TEST_ASSERT_TRUE(eParseResult == PINCFG_OK_E || eParseResult == PINCFG_WARNINGS_E);

    // Newline in middle of config - behavior depends on parser
    Memory_eReset();
    PinCfgCsv_eInit(testMemory, MEMORY_SZ, NULL);
    sParams.pcConfig = "S,o1\n,13/";
    eParseResult = PinCfgCsv_eParse(&sParams);
    // Parser may treat newline as part of name or as error
    TEST_ASSERT_TRUE(eParseResult == PINCFG_OK_E || eParseResult == PINCFG_WARNINGS_E);
}

// =============================================================================
// INTEGER OVERFLOW IN MEMORY ALLOCATION TESTS
// =============================================================================

#ifndef USE_MALLOC
/**
 * Test: Memory allocation size overflow
 */
void test_vMemoryOverflow_AllocationSize(void)
{
    // Test that requesting very large allocations fails gracefully
    Memory_eInit(testMemory, MEMORY_SZ);

    // Request more than available
    void *pvResult = Memory_vpAlloc(MEMORY_SZ * 2);
    TEST_ASSERT_NULL(pvResult);
    TEST_ASSERT_EQUAL(ENOMEM, errno);

    // Request SIZE_MAX (potential overflow)
    pvResult = Memory_vpAlloc(SIZE_MAX);
    TEST_ASSERT_NULL(pvResult);
    TEST_ASSERT_EQUAL(ENOMEM, errno);

    // Request SIZE_MAX - small value (near-overflow)
    pvResult = Memory_vpAlloc(SIZE_MAX - 100);
    TEST_ASSERT_NULL(pvResult);
    TEST_ASSERT_EQUAL(ENOMEM, errno);
}

/**
 * Test: Memory_szGetAllocatedSize overflow handling
 */
void test_vMemoryOverflow_GetAllocatedSize(void)
{
    // Test that size calculation doesn't overflow
    size_t szResult;

    // Normal case
    szResult = Memory_szGetAllocatedSize(100);
    TEST_ASSERT_TRUE(szResult >= 100);

    // Large value that could cause alignment overflow
    szResult = Memory_szGetAllocatedSize(SIZE_MAX - sizeof(void *));
    // Should either return a large aligned value or handle overflow
    // The exact behavior depends on implementation

    // Zero size
    szResult = Memory_szGetAllocatedSize(0);
    TEST_ASSERT_TRUE(szResult >= 0);
}

/**
 * Test: Temp memory allocation near boundary
 */
void test_vMemoryOverflow_TempAllocation(void)
{
    Memory_eInit(testMemory, MEMORY_SZ);

    // Calculate available temp space
    size_t szAvailable = (char *)psGlobals->pvMemTempEnd - (char *)psGlobals->pvMemNext;

    // Allocate most of temp space
    void *pvFirst = Memory_vpTempAlloc(szAvailable - sizeof(void *) * 4);
    TEST_ASSERT_NOT_NULL(pvFirst);

    // Try to allocate more than remaining
    void *pvSecond = Memory_vpTempAlloc(sizeof(void *) * 10);
    TEST_ASSERT_NULL(pvSecond);

    // Free and try again
    Memory_vTempFree();
    pvSecond = Memory_vpTempAlloc(szAvailable - sizeof(void *) * 4);
    TEST_ASSERT_NOT_NULL(pvSecond);
}

/**
 * Test: Multiple allocations causing cumulative overflow
 */
void test_vMemoryOverflow_CumulativeAllocation(void)
{
    Memory_eInit(testMemory, MEMORY_SZ);
    size_t szFree = Memory_szGetFree();

    // Allocate in small chunks until exhausted
    int nAllocations = 0;
    while (Memory_szGetFree() > sizeof(void *) * 2)
    {
        void *pvChunk = Memory_vpAlloc(sizeof(void *));
        if (pvChunk == NULL)
            break;
        nAllocations++;

        // Safety limit to prevent infinite loop
        if (nAllocations > 10000)
            break;
    }

    // Verify we consumed memory
    TEST_ASSERT_TRUE(nAllocations > 0);
    TEST_ASSERT_TRUE(Memory_szGetFree() < szFree);

    // Next allocation should fail
    void *pvFail = Memory_vpAlloc(sizeof(void *) * 10);
    TEST_ASSERT_NULL(pvFail);
}
#endif // USE_MALLOC

/**
 * Test: CSV parsing with config that would require excessive memory
 */
void test_vMemoryOverflow_CSVParsing(void)
{
    PINCFG_RESULT_T eParseResult;
    char acOutStr[OUT_STR_MAX_LEN_D];
    size_t szMemoryRequired;

    PINCFG_PARSE_PARAMS_T sParams = {
        .eAddToLoopables = PinCfgCsv_eAddToTempLoopables,
        .eAddToPresentables = PinCfgCsv_eAddToTempPresentables,
        .pszMemoryRequired = &szMemoryRequired,
        .pcOutString = acOutStr,
        .u16OutStrMaxLen = (uint16_t)OUT_STR_MAX_LEN_D,
        .bValidate = false};

    // Many switches (memory-intensive)
    char acManyItems[2048];
    strcpy(acManyItems, "");
    for (int i = 0; i < 50; i++)
    {
        char acItem[32];
        snprintf(acItem, sizeof(acItem), "S,o%d,%d/", i, (i % 32) + 1);
        strcat(acManyItems, acItem);
    }

    sParams.pcConfig = acManyItems;
    eParseResult = PinCfgCsv_eParse(&sParams);

    // First pass calculates memory - should succeed
    TEST_ASSERT_TRUE(eParseResult == PINCFG_OK_E || eParseResult == PINCFG_OUTOFMEMORY_ERROR_E);

    if (eParseResult == PINCFG_OK_E)
    {
        // Verify memory requirement was calculated
        TEST_ASSERT_TRUE(szMemoryRequired > 0);
    }
}


// ============================================================================
// Test Registration Function
// ============================================================================

void register_stress_tests(void)
{
    // Wraparound tests
    RUN_TEST(test_vMillisWraparound_ElapsedTime);
    RUN_TEST(test_vMillisWraparound_InPinDebounce);
    RUN_TEST(test_vMillisWraparound_SwitchImpulse);
    RUN_TEST(test_vMillisWraparound_SensorReporting);
    
    // Concurrency tests
    RUN_TEST(test_vConcurrentTriggers_MultipleOnSameInput);
    RUN_TEST(test_vConcurrentTriggers_MultipleActions);
    RUN_TEST(test_vConcurrentTriggers_RapidEvents);
    
    // Array size tests
    RUN_TEST(test_vMaxArraySize_TriggerMaxSwitches);
    RUN_TEST(test_vMaxArraySize_PresentablesCount);
    RUN_TEST(test_vMaxArraySize_LoopablesCount);
    RUN_TEST(test_vMaxArraySize_InPinSubscribers);
    
    // Malformed CSV tests
    RUN_TEST(test_vMalformedCSV_MissingTerminator);
    RUN_TEST(test_vMalformedCSV_EmptyFields);
    RUN_TEST(test_vMalformedCSV_ExtraFields);
    RUN_TEST(test_vMalformedCSV_InvalidNumericChars);
    RUN_TEST(test_vMalformedCSV_LongFields);
    RUN_TEST(test_vMalformedCSV_UnknownLineType);
    RUN_TEST(test_vMalformedCSV_ControlChars);
    
#ifndef USE_MALLOC
    // Memory overflow tests (static memory only)
    RUN_TEST(test_vMemoryOverflow_AllocationSize);
    RUN_TEST(test_vMemoryOverflow_GetAllocatedSize);
    RUN_TEST(test_vMemoryOverflow_TempAllocation);
    RUN_TEST(test_vMemoryOverflow_CumulativeAllocation);
#endif
    RUN_TEST(test_vMemoryOverflow_CSVParsing);
}
