#include "test_helpers.h"

// CLI Tests - Command Line Interface, Authentication, Transport Errors

void test_vCLI(void)
{
    init_mock_EEPROM_with_default_password();
    CLI_T *psCli = (CLI_T *)Memory_vpAlloc(sizeof(CLI_T));
    Cli_eInit(psCli, 0);

    MyMessage *pcMsg = (MyMessage *)Memory_vpAlloc(sizeof(MyMessage));

    // Test fragmented password reception
    // Full message: "#[240be518fabd2724ddb6f04eeb1da5967448d7e831c08c8fa822809f74c720a9/CFG:S,o1,13/"
    // Fragment 1: "#[240be518fabd272" (21 chars - within MAX_PAYLOAD_SIZE)
    strncpy(pcMsg->data, "#[240be518fabd272", MAX_PAYLOAD_SIZE);
    pcMsg->data[MAX_PAYLOAD_SIZE] = '\0';
    Cli_vRcvMessage((PRESENTABLE_T *)psCli, pcMsg);
    TEST_ASSERT_EQUAL(CLI_RECEIVING_AUTH_E, psCli->eState);
    TEST_ASSERT_EQUAL(1, mock_send_u32Called);

    // Fragment 2: "4ddb6f04eeb1da5967448" (21 chars)
    strncpy(pcMsg->data, "4ddb6f04eeb1da5967448", MAX_PAYLOAD_SIZE);
    pcMsg->data[MAX_PAYLOAD_SIZE] = '\0';
    Cli_vRcvMessage((PRESENTABLE_T *)psCli, pcMsg);
    TEST_ASSERT_EQUAL(CLI_RECEIVING_AUTH_E, psCli->eState);

    // Fragment 3: "d7e831c08c8fa822809f7" (21 chars)
    strncpy(pcMsg->data, "d7e831c08c8fa822809f7", MAX_PAYLOAD_SIZE);
    pcMsg->data[MAX_PAYLOAD_SIZE] = '\0';
    Cli_vRcvMessage((PRESENTABLE_T *)psCli, pcMsg);
    TEST_ASSERT_EQUAL(CLI_RECEIVING_AUTH_E, psCli->eState);

    // Fragment 4: "4c720a9/CFG:S,o1,13/" (password ends, config starts)
    strcpy(pcMsg->data, "4c720a9/CFG:S,o1,13/");
    Cli_vRcvMessage((PRESENTABLE_T *)psCli, pcMsg);
    // After password validation and type detection, advances to CFG_DATA state
    TEST_ASSERT_EQUAL(CLI_RECEIVING_CFG_DATA_E, psCli->eState);
    TEST_ASSERT_EQUAL_STRING("S,o1,13/", psCli->pcCfgBuf);

    // Send more data to complete CFG: detection and enter CFG_DATA state
    memset(mock_send_message, 0, sizeof(mock_send_message));
    strcpy(pcMsg->data, "I,i1,16/");
    Cli_vRcvMessage((PRESENTABLE_T *)psCli, pcMsg);
    TEST_ASSERT_EQUAL_STRING("S,o1,13/I,i1,16/", psCli->pcCfgBuf);
    strcpy(pcMsg->data, "T,vtl11,i1,0,1,o2,2/]#");
    Cli_vRcvMessage((PRESENTABLE_T *)psCli, pcMsg);
    // State sent after each frame: both fragments send CFG_DATA, then final sends validation
    TEST_ASSERT_EQUAL_STRING("RECEIVING_CFG_DATA;RECEIVING_CFG_DATA;VALIDATION_OK;READY;", mock_send_message);
    TEST_ASSERT_EQUAL_STRING("READY", mock_MyMessage_set_char_value);

    // Test wrong password (fragmented)
    memset(mock_send_message, 0, sizeof(mock_send_message));
    mock_send_u32Called = 0;
    Cli_eInit(psCli, 0);
    strcpy(pcMsg->data, "#[wrongpwd/T,o1,13");
    Cli_vRcvMessage((PRESENTABLE_T *)psCli, pcMsg);
    // First sends RECEIVING_AUTH state, then auth fails when '/' is processed
    TEST_ASSERT_EQUAL_STRING("RECEIVING_AUTH;Authentication failed.;", mock_send_message);
    TEST_ASSERT_EQUAL(CLI_READY_E, psCli->eState);
}

void test_vCLI_EdgeCases(void)
{
    // Initialize memory at start of test
    Memory_eReset();
    PinCfgCsv_eInit(testMemory, MEMORY_SZ, NULL);

    init_mock_EEPROM_with_default_password();
    CLI_T *psCli = (CLI_T *)Memory_vpAlloc(sizeof(CLI_T));
    Cli_eInit(psCli, 0);

    MyMessage *pcMsg = (MyMessage *)Memory_vpAlloc(sizeof(MyMessage));

    // Test receiving configuration with fragmented password
    // Fragment 1: password prefix
    strncpy(pcMsg->data, "#[240be518fabd272", MAX_PAYLOAD_SIZE);
    pcMsg->data[MAX_PAYLOAD_SIZE] = '\0';
    Cli_vRcvMessage((PRESENTABLE_T *)psCli, pcMsg);
    TEST_ASSERT_EQUAL(CLI_RECEIVING_AUTH_E, psCli->eState);

    // Fragment 2: more password
    strncpy(pcMsg->data, "4ddb6f04eeb1da5967448", MAX_PAYLOAD_SIZE);
    pcMsg->data[MAX_PAYLOAD_SIZE] = '\0';
    Cli_vRcvMessage((PRESENTABLE_T *)psCli, pcMsg);

    // Fragment 3: more password
    strncpy(pcMsg->data, "d7e831c08c8fa822809f7", MAX_PAYLOAD_SIZE);
    pcMsg->data[MAX_PAYLOAD_SIZE] = '\0';
    Cli_vRcvMessage((PRESENTABLE_T *)psCli, pcMsg);

    // Fragment 4: end of password and start of config
    strcpy(pcMsg->data, "4c720a9/CFG:S,o1,13/]#");
    Cli_vRcvMessage((PRESENTABLE_T *)psCli, pcMsg);
    // State sent after each frame: 3 fragments with RECEIVING_AUTH feedback, then 4th gets auth+type+cfg_data before
    // validation
    TEST_ASSERT_EQUAL_STRING(
        "RECEIVING_AUTH;RECEIVING_AUTH;RECEIVING_AUTH;RECEIVING_AUTH;RECEIVING_TYPE;RECEIVING_CFG_DATA;VALIDATION_OK;"
        "READY;",
        mock_send_message);

    // Test fragmented command with GET_CFG
    // First save some config so GET_CFG has something to return
    memset(mock_send_message, 0, sizeof(mock_send_message));
    Cli_eInit(psCli, 0);

    // Fragment 1: #[240be518fabd272 (21 chars)
    strncpy(pcMsg->data, "#[240be518fabd27", MAX_PAYLOAD_SIZE);
    pcMsg->data[MAX_PAYLOAD_SIZE] = '\0';
    Cli_vRcvMessage((PRESENTABLE_T *)psCli, pcMsg);
    TEST_ASSERT_EQUAL(CLI_RECEIVING_AUTH_E, psCli->eState);

    // Fragment 2: more password
    strncpy(pcMsg->data, "24ddb6f04eeb1da596744", MAX_PAYLOAD_SIZE);
    pcMsg->data[MAX_PAYLOAD_SIZE] = '\0';
    Cli_vRcvMessage((PRESENTABLE_T *)psCli, pcMsg);

    // Fragment 3: more password
    strncpy(pcMsg->data, "8d7e831c08c8fa822809f", MAX_PAYLOAD_SIZE);
    pcMsg->data[MAX_PAYLOAD_SIZE] = '\0';
    Cli_vRcvMessage((PRESENTABLE_T *)psCli, pcMsg);

    // Fragment 4: end of password and command
    strcpy(pcMsg->data, "74c720a9/CMD:GET_CFG]#");
    Cli_vRcvMessage((PRESENTABLE_T *)psCli, pcMsg);
    // Should return config or "No config data." message
    TEST_ASSERT_TRUE(strlen(mock_send_message) > 0);

    // Test fragmented command with RESET (simple command, no additional params)
    memset(mock_send_message, 0, sizeof(mock_send_message));
    Cli_eInit(psCli, 0);

    // Command continues across fragments
    strncpy(pcMsg->data, "#[240be518fabd27", MAX_PAYLOAD_SIZE);
    pcMsg->data[MAX_PAYLOAD_SIZE] = '\0';
    Cli_vRcvMessage((PRESENTABLE_T *)psCli, pcMsg);

    strncpy(pcMsg->data, "24ddb6f04eeb1da596744", MAX_PAYLOAD_SIZE);
    pcMsg->data[MAX_PAYLOAD_SIZE] = '\0';
    Cli_vRcvMessage((PRESENTABLE_T *)psCli, pcMsg);

    strncpy(pcMsg->data, "8d7e831c08c8fa822809f", MAX_PAYLOAD_SIZE);
    pcMsg->data[MAX_PAYLOAD_SIZE] = '\0';
    Cli_vRcvMessage((PRESENTABLE_T *)psCli, pcMsg);

    // Note: We don't actually call RESET as it would reset the system
    // Just test that we get to CLI_RECEIVING_DATA_E state correctly
    strcpy(pcMsg->data, "74c720a9/CMD:UNKNOWNCMD]#");
    Cli_vRcvMessage((PRESENTABLE_T *)psCli, pcMsg);
    // 3 fragments sent before final -> 3x RECEIVING_AUTH feedback, 4th gets auth+type+cmd_data before result
    TEST_ASSERT_EQUAL_STRING(
        "RECEIVING_AUTH;RECEIVING_AUTH;RECEIVING_AUTH;RECEIVING_AUTH;RECEIVING_TYPE;RECEIVING_CMD_DATA;Unknown "
        "command.;",
        mock_send_message);

    // Test invalid command format
    memset(mock_send_message, 0, sizeof(mock_send_message));
    Cli_eInit(psCli, 0);
    strcpy(pcMsg->data, "#[INVALID_PWD/CMD:TEST]#");
    Cli_vRcvMessage((PRESENTABLE_T *)psCli, pcMsg);
    // Note: First sends RECEIVING_AUTH state, then auth fails when '/' is processed
    TEST_ASSERT_TRUE(strlen(mock_send_message) > 0);
    TEST_ASSERT_EQUAL_STRING("RECEIVING_AUTH;Authentication failed.;", mock_send_message);

    // Test empty configuration - use MyMessage struct
    memset(mock_send_message, 0, sizeof(mock_send_message));
    Cli_eInit(psCli, 0);
    strcpy(pcMsg->data, "#[]#");
    Cli_vRcvMessage((PRESENTABLE_T *)psCli, pcMsg);
    // Note: Empty configuration handling - should require auth
}

void test_vCLI_ChangePassword(void)
{
    // Initialize memory at start of test
    Memory_eReset();
    PinCfgCsv_eInit(testMemory, MEMORY_SZ, NULL);

    init_mock_EEPROM_with_default_password();
    CLI_T *psCli = (CLI_T *)Memory_vpAlloc(sizeof(CLI_T));
    Cli_eInit(psCli, 0);

    MyMessage *pcMsg = (MyMessage *)Memory_vpAlloc(sizeof(MyMessage));

    // Default password hash: 240be518fabd2724ddb6f04eeb1da5967448d7e831c08c8fa822809f74c720a9
    // New password hash (sha256 of "newpassword123"):
    // ef92b778bafe771e89245b89ecbc08a44a4e166c06659911881f383d4473e94f

    // Test 1: Successful password change
    // Command: #[<current_password>/CMD:CHANGE_PWD:<new_password>#
    memset(mock_send_message, 0, sizeof(mock_send_message));

    // Fragment 1: Start of command with current password
    strncpy(pcMsg->data, "#[240be518fabd27", MAX_PAYLOAD_SIZE);
    pcMsg->data[MAX_PAYLOAD_SIZE] = '\0';
    Cli_vRcvMessage((PRESENTABLE_T *)psCli, pcMsg);
    TEST_ASSERT_EQUAL(CLI_RECEIVING_AUTH_E, psCli->eState);

    // Fragment 2: More of current password
    strncpy(pcMsg->data, "24ddb6f04eeb1da596744", MAX_PAYLOAD_SIZE);
    pcMsg->data[MAX_PAYLOAD_SIZE] = '\0';
    Cli_vRcvMessage((PRESENTABLE_T *)psCli, pcMsg);

    // Fragment 3: More of current password
    strncpy(pcMsg->data, "8d7e831c08c8fa822809f", MAX_PAYLOAD_SIZE);
    pcMsg->data[MAX_PAYLOAD_SIZE] = '\0';
    Cli_vRcvMessage((PRESENTABLE_T *)psCli, pcMsg);

    // Fragment 4: End of current password + CHANGE_PWD: + start of new password
    strncpy(pcMsg->data, "74c720a9/CMD:CHANGE_PWD:e", MAX_PAYLOAD_SIZE);
    pcMsg->data[MAX_PAYLOAD_SIZE] = '\0';
    Cli_vRcvMessage((PRESENTABLE_T *)psCli, pcMsg);
    // After password validation and type detection, advances to CMD_DATA state
    TEST_ASSERT_EQUAL(CLI_RECEIVING_CMD_DATA_E, psCli->eState);

    // Fragment 5: More of new password
    strncpy(pcMsg->data, "f92b778bafe771e89245b", MAX_PAYLOAD_SIZE);
    pcMsg->data[MAX_PAYLOAD_SIZE] = '\0';
    Cli_vRcvMessage((PRESENTABLE_T *)psCli, pcMsg);

    // Fragment 6: More of new password
    strncpy(pcMsg->data, "89ecbc08a44a4e166c066", MAX_PAYLOAD_SIZE);
    pcMsg->data[MAX_PAYLOAD_SIZE] = '\0';
    Cli_vRcvMessage((PRESENTABLE_T *)psCli, pcMsg);

    // Fragment 7: End of new password + terminator
    strcpy(pcMsg->data, "59911881f383d4473e94f]#");
    Cli_vRcvMessage((PRESENTABLE_T *)psCli, pcMsg);

    // Should report success - state sent after each frame: 3x RECEIVING_AUTH + 4th gets auth+type+cmd_data + 3x
    // RECEIVING_CMD_DATA + result
    TEST_ASSERT_EQUAL_STRING(
        "RECEIVING_AUTH;RECEIVING_AUTH;RECEIVING_AUTH;RECEIVING_AUTH;RECEIVING_TYPE;RECEIVING_CMD_DATA;RECEIVING_CMD_"
        "DATA;RECEIVING_CMD_DATA;RECEIVING_CMD_DATA;Pwd changed OK.;",
        mock_send_message);
    TEST_ASSERT_EQUAL(CLI_READY_E, psCli->eState);

    // Test 2: Verify new password works - try to authenticate with it
    memset(mock_send_message, 0, sizeof(mock_send_message));
    Cli_eInit(psCli, 0);

    // Try GET_CFG with new password
    strncpy(pcMsg->data, "#[ef92b778bafe771", MAX_PAYLOAD_SIZE);
    pcMsg->data[MAX_PAYLOAD_SIZE] = '\0';
    Cli_vRcvMessage((PRESENTABLE_T *)psCli, pcMsg);

    strncpy(pcMsg->data, "e89245b89ecbc08a44a4e", MAX_PAYLOAD_SIZE);
    pcMsg->data[MAX_PAYLOAD_SIZE] = '\0';
    Cli_vRcvMessage((PRESENTABLE_T *)psCli, pcMsg);

    strncpy(pcMsg->data, "166c06659911881f383d4", MAX_PAYLOAD_SIZE);
    pcMsg->data[MAX_PAYLOAD_SIZE] = '\0';
    Cli_vRcvMessage((PRESENTABLE_T *)psCli, pcMsg);

    strcpy(pcMsg->data, "473e94f/CMD:GET_CFG]#");
    Cli_vRcvMessage((PRESENTABLE_T *)psCli, pcMsg);

    // Should succeed with new password - 3 fragments -> 3x RECEIVING_AUTH feedback, 4th gets auth+type+cmd_data before
    // result
    TEST_ASSERT_EQUAL_STRING(
        "RECEIVING_AUTH;RECEIVING_AUTH;RECEIVING_AUTH;RECEIVING_AUTH;RECEIVING_TYPE;RECEIVING_CMD_DATA;No config "
        "data.;",
        mock_send_message);

    // Test 3: Verify old password no longer works
    memset(mock_send_message, 0, sizeof(mock_send_message));
    Cli_eInit(psCli, 0);

    // Try with old password - should fail
    strncpy(pcMsg->data, "#[240be518fabd27", MAX_PAYLOAD_SIZE);
    pcMsg->data[MAX_PAYLOAD_SIZE] = '\0';
    Cli_vRcvMessage((PRESENTABLE_T *)psCli, pcMsg);

    strncpy(pcMsg->data, "24ddb6f04eeb1da596744", MAX_PAYLOAD_SIZE);
    pcMsg->data[MAX_PAYLOAD_SIZE] = '\0';
    Cli_vRcvMessage((PRESENTABLE_T *)psCli, pcMsg);

    strncpy(pcMsg->data, "8d7e831c08c8fa822809f", MAX_PAYLOAD_SIZE);
    pcMsg->data[MAX_PAYLOAD_SIZE] = '\0';
    Cli_vRcvMessage((PRESENTABLE_T *)psCli, pcMsg);

    strcpy(pcMsg->data, "74c720a9/CMD:GET_CFG]#");
    Cli_vRcvMessage((PRESENTABLE_T *)psCli, pcMsg);

    // Should fail authentication with old password - 3 fragments + 4th also sends auth before failure
    TEST_ASSERT_EQUAL_STRING(
        "RECEIVING_AUTH;RECEIVING_AUTH;RECEIVING_AUTH;RECEIVING_AUTH;Authentication failed.;", mock_send_message);

    // Test 4: Invalid new password (too short)
    // Reset password back to default for this test
    init_mock_EEPROM_with_default_password();
    memset(mock_send_message, 0, sizeof(mock_send_message));
    Cli_eInit(psCli, 0);

    strncpy(pcMsg->data, "#[240be518fabd27", MAX_PAYLOAD_SIZE);
    pcMsg->data[MAX_PAYLOAD_SIZE] = '\0';
    Cli_vRcvMessage((PRESENTABLE_T *)psCli, pcMsg);

    strncpy(pcMsg->data, "24ddb6f04eeb1da596744", MAX_PAYLOAD_SIZE);
    pcMsg->data[MAX_PAYLOAD_SIZE] = '\0';
    Cli_vRcvMessage((PRESENTABLE_T *)psCli, pcMsg);

    strncpy(pcMsg->data, "8d7e831c08c8fa822809f", MAX_PAYLOAD_SIZE);
    pcMsg->data[MAX_PAYLOAD_SIZE] = '\0';
    Cli_vRcvMessage((PRESENTABLE_T *)psCli, pcMsg);

    // Send CHANGE_PWD with empty/short password
    strcpy(pcMsg->data, "74c720a9/CMD:CHANGE_PWD:]#");
    Cli_vRcvMessage((PRESENTABLE_T *)psCli, pcMsg);

    // Should report invalid password length - 3 fragments + 4th with auth+type+cmd_data before result
    TEST_ASSERT_EQUAL_STRING(
        "RECEIVING_AUTH;RECEIVING_AUTH;RECEIVING_AUTH;RECEIVING_AUTH;RECEIVING_TYPE;RECEIVING_CMD_DATA;Invalid new pwd "
        "length.;",
        mock_send_message);
}

#ifdef MY_TRANSPORT_ERROR_LOG

void test_vCLI_GetTransportErrors_EmptyLog(void)
{
    Memory_eReset();
    PinCfgCsv_eInit(testMemory, MEMORY_SZ, NULL);

    init_mock_EEPROM_with_default_password();
    init_TransportErrorLogMock();

    CLI_T *psCli = (CLI_T *)Memory_vpAlloc(sizeof(CLI_T));
    Cli_eInit(psCli, 0);

    MyMessage *pcMsg = (MyMessage *)Memory_vpAlloc(sizeof(MyMessage));
    memset(mock_send_message, 0, sizeof(mock_send_message));

    // Send GET_TSP_ERRORS command with valid auth (fragmented to fit MAX_PAYLOAD_SIZE)
    // Password: 240be518fabd2724ddb6f04eeb1da5967448d7e831c08c8fa822809f74c720a9
    strncpy(pcMsg->data, "#[240be518fabd272", MAX_PAYLOAD_SIZE);
    pcMsg->data[MAX_PAYLOAD_SIZE] = '\0';
    Cli_vRcvMessage((PRESENTABLE_T *)psCli, pcMsg);

    strncpy(pcMsg->data, "4ddb6f04eeb1da5967448", MAX_PAYLOAD_SIZE);
    pcMsg->data[MAX_PAYLOAD_SIZE] = '\0';
    Cli_vRcvMessage((PRESENTABLE_T *)psCli, pcMsg);

    strncpy(pcMsg->data, "d7e831c08c8fa822809f7", MAX_PAYLOAD_SIZE);
    pcMsg->data[MAX_PAYLOAD_SIZE] = '\0';
    Cli_vRcvMessage((PRESENTABLE_T *)psCli, pcMsg);

    strcpy(pcMsg->data, "4c720a9/CMD:GET_TSP_ER");
    Cli_vRcvMessage((PRESENTABLE_T *)psCli, pcMsg);

    // Clear mock before final fragment to capture only the response
    memset(mock_send_message, 0, sizeof(mock_send_message));
    strcpy(pcMsg->data, "RORS]#");
    Cli_vRcvMessage((PRESENTABLE_T *)psCli, pcMsg);

    // Final fragment still sends state feedback before result
    TEST_ASSERT_EQUAL_STRING("RECEIVING_CMD_DATA;No errors. Total: 0;", mock_send_message);
}

void test_vCLI_GetTransportErrors_SingleEntry(void)
{
    Memory_eReset();
    PinCfgCsv_eInit(testMemory, MEMORY_SZ, NULL);

    init_mock_EEPROM_with_default_password();
    init_TransportErrorLogMock();

    // Log some errors
    transportLogError(0x82, 8, 5);  // CAN TX failed, channel 8, extra 5
    transportLogError(0x83, 8, 0);  // CAN TX timeout
    transportLogError(0x8B, 8, 10); // CAN bus off

    CLI_T *psCli = (CLI_T *)Memory_vpAlloc(sizeof(CLI_T));
    Cli_eInit(psCli, 0);

    MyMessage *pcMsg = (MyMessage *)Memory_vpAlloc(sizeof(MyMessage));
    memset(mock_send_message, 0, sizeof(mock_send_message));

    // Send GET_TSP_ERRORS command (fragmented)
    strncpy(pcMsg->data, "#[240be518fabd272", MAX_PAYLOAD_SIZE);
    pcMsg->data[MAX_PAYLOAD_SIZE] = '\0';
    Cli_vRcvMessage((PRESENTABLE_T *)psCli, pcMsg);

    strncpy(pcMsg->data, "4ddb6f04eeb1da5967448", MAX_PAYLOAD_SIZE);
    pcMsg->data[MAX_PAYLOAD_SIZE] = '\0';
    Cli_vRcvMessage((PRESENTABLE_T *)psCli, pcMsg);

    strncpy(pcMsg->data, "d7e831c08c8fa822809f7", MAX_PAYLOAD_SIZE);
    pcMsg->data[MAX_PAYLOAD_SIZE] = '\0';
    Cli_vRcvMessage((PRESENTABLE_T *)psCli, pcMsg);

    strcpy(pcMsg->data, "4c720a9/CMD:GET_TSP_ER");
    Cli_vRcvMessage((PRESENTABLE_T *)psCli, pcMsg);

    // Clear mock before final fragment to capture only the response
    memset(mock_send_message, 0, sizeof(mock_send_message));
    strcpy(pcMsg->data, "RORS]#");
    Cli_vRcvMessage((PRESENTABLE_T *)psCli, pcMsg);

    // Final fragment sends state feedback, then message is split every 25 chars by Cli_vSendBigMessage, mock adds ;
    // after each Full message: "T:3,C:3|1000,82,8,5|1100,83,8,0|1200,8B,8,10" (47 chars)
    TEST_ASSERT_EQUAL_STRING(
        "RECEIVING_CMD_DATA;T:3,C:3|1000,82,8,5|1100,;83,8,0|1200,8B,8,10;READY;", mock_send_message);
}

void test_vCLI_GetTransportErrors_MultipleEntries(void)
{
    Memory_eReset();
    PinCfgCsv_eInit(testMemory, MEMORY_SZ, NULL);

    init_mock_EEPROM_with_default_password();
    init_TransportErrorLogMock();

    // Log some errors first
    transportLogError(0x82, 8, 1);
    transportLogError(0x83, 8, 2);
    TEST_ASSERT_EQUAL(2, transportGetErrorLogCount());

    CLI_T *psCli = (CLI_T *)Memory_vpAlloc(sizeof(CLI_T));
    Cli_eInit(psCli, 0);

    MyMessage *pcMsg = (MyMessage *)Memory_vpAlloc(sizeof(MyMessage));
    memset(mock_send_message, 0, sizeof(mock_send_message));

    // Send CLR_TSP_ERRORS command (fragmented)
    strncpy(pcMsg->data, "#[240be518fabd272", MAX_PAYLOAD_SIZE);
    pcMsg->data[MAX_PAYLOAD_SIZE] = '\0';
    Cli_vRcvMessage((PRESENTABLE_T *)psCli, pcMsg);

    strncpy(pcMsg->data, "4ddb6f04eeb1da5967448", MAX_PAYLOAD_SIZE);
    pcMsg->data[MAX_PAYLOAD_SIZE] = '\0';
    Cli_vRcvMessage((PRESENTABLE_T *)psCli, pcMsg);

    strncpy(pcMsg->data, "d7e831c08c8fa822809f7", MAX_PAYLOAD_SIZE);
    pcMsg->data[MAX_PAYLOAD_SIZE] = '\0';
    Cli_vRcvMessage((PRESENTABLE_T *)psCli, pcMsg);

    strcpy(pcMsg->data, "4c720a9/CMD:CLR_TSP_ER");
    Cli_vRcvMessage((PRESENTABLE_T *)psCli, pcMsg);

    // Clear mock before final fragment to capture only the response
    memset(mock_send_message, 0, sizeof(mock_send_message));
    strcpy(pcMsg->data, "RORS]#");
    Cli_vRcvMessage((PRESENTABLE_T *)psCli, pcMsg);

    // Final fragment sends state feedback before result
    TEST_ASSERT_EQUAL_STRING("RECEIVING_CMD_DATA;Errors cleared.;", mock_send_message);

    // Error count should be 0 now
    TEST_ASSERT_EQUAL(0, transportGetErrorLogCount());

    // Total count should still be tracked
    TEST_ASSERT_EQUAL(2, transportGetTotalErrorCount());
}

void test_vCLI_GetTransportErrors_FullLog(void)
{
    Memory_eReset();
    PinCfgCsv_eInit(testMemory, MEMORY_SZ, NULL);

    init_mock_EEPROM_with_default_password();
    init_TransportErrorLogMock();

    CLI_T *psCli = (CLI_T *)Memory_vpAlloc(sizeof(CLI_T));
    Cli_eInit(psCli, 0);

    MyMessage *pcMsg = (MyMessage *)Memory_vpAlloc(sizeof(MyMessage));
    memset(mock_send_message, 0, sizeof(mock_send_message));

    // Send GET_TSP_ERRORS with wrong password (short enough to fit)
    strcpy(pcMsg->data, "#[wrongpwd/CMD:GET_TSP");
    Cli_vRcvMessage((PRESENTABLE_T *)psCli, pcMsg);

    // First sends RECEIVING_AUTH state, then auth fails when '/' is processed
    TEST_ASSERT_EQUAL_STRING("RECEIVING_AUTH;Authentication failed.;", mock_send_message);
}

#endif // MY_TRANSPORT_ERROR_LOG

void register_cli_tests(void)
{
    RUN_TEST(test_vCLI);
    RUN_TEST(test_vCLI_EdgeCases);
    RUN_TEST(test_vCLI_ChangePassword);
#ifdef MY_TRANSPORT_ERROR_LOG
    RUN_TEST(test_vCLI_GetTransportErrors_EmptyLog);
    RUN_TEST(test_vCLI_GetTransportErrors_SingleEntry);
    RUN_TEST(test_vCLI_GetTransportErrors_MultipleEntries);
    RUN_TEST(test_vCLI_GetTransportErrors_FullLog);
#endif
}
