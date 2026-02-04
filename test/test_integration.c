#include "test_helpers.h"

// Integration Tests - Full system and flow tests

void test_vFlow_timedSwitch(void)
{
    init_mock_EEPROM_with_default_password();
    PINCFG_RESULT_T eResult;

    // Configuration string: input pin -> trigger -> timed switch
    const char *pcCfg = "I,i1,16/"            // Input pin on pin 16
                        "ST,sw1,13,5000/"     // Timed switch on pin 13 with 5s timeout
                        "T,t1,i1,8,1,sw1,3/"; // Trigger that connects i1 to sw1

    // To use this configuration, make EEPROM config size return invalid (too large)
    // This ensures PinCfgCsv_eInit uses the provided pcCfg instead of loading from EEPROM
    static uint8_t mockConfigBuffer[512]; // Large enough for password (32) + oversized config indicator
    memset(mockConfigBuffer, 0, sizeof(mockConfigBuffer));
    uint16_t *pu16Size = (uint16_t *)&mockConfigBuffer[32]; // Put size after password area
    *pu16Size = PINCFG_CONFIG_MAX_SZ_D + 1;                 // Oversized config
    mock_hwReadConfigBlock_buf = mockConfigBuffer;

    eResult = PinCfgCsv_eInit(testMemory, MEMORY_SZ, pcCfg);
    TEST_ASSERT_EQUAL(PINCFG_OK_E, eResult);
    TEST_ASSERT_EQUAL(3, psGlobals->u8LoopablesCount);

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
    mock_millis_u32Return = 0;
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

    u32Time += psInPin->u32TimerMultiStarted + psGlobals->u32InPinMulticlickMaxDelayMs;
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
    TEST_ASSERT_EQUAL(u32Time, psInPin->u32TimerMultiStarted);

    u32Time += psInPin->u32TimerMultiStarted + psGlobals->u32InPinMulticlickMaxDelayMs;
    PinCfgCsv_vLoop(u32Time);

    // Verify switch is off
    TEST_ASSERT_EQUAL(2, mock_digitalWrite_u32Called);
    TEST_ASSERT_EQUAL(LOW, mock_digitalWrite_u8Value);
    TEST_ASSERT_EQUAL(13, mock_digitalWrite_u8Pin);
    TEST_ASSERT_EQUAL(3, mock_send_u32Called);
    // End of verify canceling V2
}

void test_vIntegration_CompleteSystem(void)
{
#ifdef USE_MALLOC
    // Initialize clean EEPROM to ensure we use the test config, not stored config
    init_mock_EEPROM_with_default_password();

    const char *pcCfg = "# Complete system test/"
                        "CD,50/"                        // Config: 50ms debounce
                        "CM,1000/"                      // Config: 1s multiclick
                        "CR,120/"                       // Config: 120ms impulse
                        "S,relay1,13,relay2,12/"        // 2 classic switches
                        "SI,impulse1,11/"               // 1 impulse switch
                        "ST,timed1,10,5000/"            // 1 timed switch
                        "I,button1,16,button2,15/"      // 2 input pins
                        "T,t1,button1,0,1,relay1,1/"    // Toggle trigger
                        "T,t2,button2,2,1,impulse1,1/"; // Long press trigger

    PINCFG_RESULT_T eResult = PinCfgCsv_eInit(testMemory, MEMORY_SZ, pcCfg);
    // Note: May have warnings for deprecated formats or edge cases
    TEST_ASSERT_TRUE(eResult == PINCFG_OK_E || eResult == PINCFG_WARNINGS_E);

    // Verify configuration applied
    TEST_ASSERT_EQUAL(50, psGlobals->u32InPinDebounceMs);
    TEST_ASSERT_EQUAL(1000, psGlobals->u32InPinMulticlickMaxDelayMs);
    TEST_ASSERT_EQUAL(120, psGlobals->u32SwitchImpulseDurationMs);

    // Verify components created (should have CLI + switches + inputs)
    TEST_ASSERT_TRUE(psGlobals->u8PresentablesCount >= 6); // At least CLI + some components
    TEST_ASSERT_TRUE(psGlobals->u8LoopablesCount >= 1);    // At least one loopable

    // Run few loop iterations to ensure no crashes
    mock_millis_u32Return = 0;
    for (int i = 0; i < 100; i++)
    {
        mock_millis_u32Return += 10;
        PinCfgCsv_vLoop(mock_millis_u32Return);
    }
#else
    // Test skipped in static allocation mode - requires more memory
    TEST_PASS();
#endif
}

void test_vIntegration_MemoryExhaustion(void)
{
#ifndef USE_MALLOC
    // Test that the system correctly detects when memory is exhausted
    // The system should fail gracefully with PINCFG_OUTOFMEMORY_ERROR_E

    char acLargeCfg[5000];
    strcpy(acLargeCfg, "");

    // Create a configuration that should exhaust the 8KB memory
    // With 80 bytes per switch, ~100 switches should fill 8KB
    for (int i = 0; i < 100; i++)
    {
        char acSwitch[50];
        sprintf(acSwitch, "S,o%d,%d/", i + 10, i + 20);
        strcat(acLargeCfg, acSwitch);
    }

    // Reinit EEPROM to ensure config is parsed (not loaded from EEPROM)
    init_mock_EEPROM();
    PINCFG_RESULT_T eResult = PinCfgCsv_eInit(testMemory, MEMORY_SZ, acLargeCfg);

    // Should fail with out of memory error
    TEST_ASSERT_EQUAL(PINCFG_OUTOFMEMORY_ERROR_E, eResult);
#else
    // Test skipped in malloc mode - requires static allocation for precise control
    TEST_PASS();
#endif
}

void register_integration_tests(void)
{
    RUN_TEST(test_vFlow_timedSwitch);
    RUN_TEST(test_vIntegration_CompleteSystem);
    RUN_TEST(test_vIntegration_MemoryExhaustion);
}
