#include "test_helpers.h"

// Persistent Configuration Tests - EEPROM password and config storage

void test_vPersistentCfg_PasswordOnly(void)
{
    init_mock_EEPROM();

    const char *test_password_hex = "240be518fabd2724ddb6f04eeb1da5967448d7e831c08c8fa822809f74c720a9";
    uint8_t au8PasswordBinary[32];
    uint8_t au8PasswordReadBack[32];
    char config_buffer[PINCFG_CONFIG_MAX_SZ_D];

    // Convert hex to binary and write password
    CliAuth_bHexToBinary(test_password_hex, au8PasswordBinary, 32);
    PERCFG_RESULT_T result = PersistentCfg_eWritePassword(au8PasswordBinary);
    TEST_ASSERT_EQUAL(PERCFG_OK_E, result);

    // Read password back (as binary)
    memset(au8PasswordReadBack, 0, sizeof(au8PasswordReadBack));
    result = PersistentCfg_eReadPassword(au8PasswordReadBack);
    TEST_ASSERT_EQUAL(PERCFG_OK_E, result);
    TEST_ASSERT_EQUAL_MEMORY(au8PasswordBinary, au8PasswordReadBack, 32);

    // Config should be empty
    memset(config_buffer, 0, sizeof(config_buffer));
    result = PersistentCfg_eLoadConfig(config_buffer);
    TEST_ASSERT_EQUAL(PERCFG_OK_E, result);
    TEST_ASSERT_EQUAL_STRING("", config_buffer);

    // Config size should be 0
    uint16_t config_size = 99;
    result = PersistentCfg_eGetConfigSize(&config_size);
    TEST_ASSERT_EQUAL(PERCFG_OK_E, result);
    TEST_ASSERT_EQUAL(0, config_size);
}

/**
 * Test: Config-only operations (with zero password)
 */
void test_vPersistentCfg_ConfigOnly(void)
{
    init_mock_EEPROM();

    const char *test_config = "pin1=A0,sensor,temp/pin2=A1,switch";
    uint8_t au8PasswordBuffer[PINCFG_AUTH_PASSWORD_LEN_D];
    uint8_t au8ZeroPassword[PINCFG_AUTH_PASSWORD_LEN_D];
    char config_buffer[PINCFG_CONFIG_MAX_SZ_D];

    // Write zero password first (required for memory layout)
    memset(au8ZeroPassword, 0, sizeof(au8ZeroPassword));
    PERCFG_RESULT_T result = PersistentCfg_eWritePassword(au8ZeroPassword);
    TEST_ASSERT_EQUAL(PERCFG_OK_E, result);

    // Write config
    result = PersistentCfg_eSaveConfig(test_config);
    TEST_ASSERT_EQUAL(PERCFG_OK_E, result);

    // Read config back
    memset(config_buffer, 0, sizeof(config_buffer));
    result = PersistentCfg_eLoadConfig(config_buffer);
    TEST_ASSERT_EQUAL(PERCFG_OK_E, result);
    TEST_ASSERT_EQUAL_STRING(test_config, config_buffer);

    // Password should be all zeros
    memset(au8PasswordBuffer, 0xFF, sizeof(au8PasswordBuffer));
    result = PersistentCfg_eReadPassword(au8PasswordBuffer);
    TEST_ASSERT_EQUAL(PERCFG_OK_E, result);
    TEST_ASSERT_EQUAL_MEMORY(au8ZeroPassword, au8PasswordBuffer, PINCFG_AUTH_PASSWORD_LEN_D);

    // Config size should match (returns data length, not including null terminator)
    uint16_t config_size = 0;
    result = PersistentCfg_eGetConfigSize(&config_size);
    TEST_ASSERT_EQUAL(PERCFG_OK_E, result);
    TEST_ASSERT_EQUAL(strlen(test_config), config_size);
}

/**
 * Test: Password + Config together
 * Password stored as 32-byte binary hash
 */
void test_vPersistentCfg_PasswordAndConfig(void)
{
    init_mock_EEPROM();

    // Use a valid 64-char hex password (SHA-256 hash)
    const char *test_password_hex = "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855";
    uint8_t au8PasswordBinary[32];
    uint8_t au8PasswordReadBack[32];
    const char *test_config = "pin1=A0,sensor/pin2=D2,switch";
    char config_buffer[PINCFG_CONFIG_MAX_SZ_D];

    // Convert hex to binary and write password
    CliAuth_bHexToBinary(test_password_hex, au8PasswordBinary, 32);
    PERCFG_RESULT_T result = PersistentCfg_eWritePassword(au8PasswordBinary);
    TEST_ASSERT_EQUAL(PERCFG_OK_E, result);

    // Write config
    result = PersistentCfg_eSaveConfig(test_config);
    TEST_ASSERT_EQUAL(PERCFG_OK_E, result);

    // Read both back
    memset(au8PasswordReadBack, 0, sizeof(au8PasswordReadBack));
    result = PersistentCfg_eReadPassword(au8PasswordReadBack);
    TEST_ASSERT_EQUAL(PERCFG_OK_E, result);
    TEST_ASSERT_EQUAL_MEMORY(au8PasswordBinary, au8PasswordReadBack, 32);

    memset(config_buffer, 0, sizeof(config_buffer));
    result = PersistentCfg_eLoadConfig(config_buffer);
    TEST_ASSERT_EQUAL(PERCFG_OK_E, result);
    TEST_ASSERT_EQUAL_STRING(test_config, config_buffer);
}

/**
 * Test: Update password while preserving config
 * Uses 32-byte binary hashes for password
 */
void test_vPersistentCfg_UpdatePasswordKeepConfig(void)
{
    init_mock_EEPROM();

    // Two different 64-char hex passwords (SHA-256 hashes)
    const char *original_password_hex = "0000000000000000000000000000000000000000000000000000000000000001";
    const char *new_password_hex = "0000000000000000000000000000000000000000000000000000000000000002";
    uint8_t au8OriginalPassword[32];
    uint8_t au8NewPassword[32];
    uint8_t au8PasswordReadBack[32];
    const char *test_config = "pin1=A0/pin2=A1";
    char config_buffer[PINCFG_CONFIG_MAX_SZ_D];

    // Convert hex passwords to binary
    CliAuth_bHexToBinary(original_password_hex, au8OriginalPassword, 32);
    CliAuth_bHexToBinary(new_password_hex, au8NewPassword, 32);

    // Set initial password and config
    PERCFG_RESULT_T result = PersistentCfg_eWritePassword(au8OriginalPassword);
    TEST_ASSERT_EQUAL(PERCFG_OK_E, result);
    result = PersistentCfg_eSaveConfig(test_config);
    TEST_ASSERT_EQUAL(PERCFG_OK_E, result);

    // Update password
    result = PersistentCfg_eWritePassword(au8NewPassword);
    TEST_ASSERT_EQUAL(PERCFG_OK_E, result);

    // Verify new password
    memset(au8PasswordReadBack, 0, sizeof(au8PasswordReadBack));
    result = PersistentCfg_eReadPassword(au8PasswordReadBack);
    TEST_ASSERT_EQUAL(PERCFG_OK_E, result);
    TEST_ASSERT_EQUAL_MEMORY(au8NewPassword, au8PasswordReadBack, 32);

    // Verify config unchanged
    memset(config_buffer, 0, sizeof(config_buffer));
    result = PersistentCfg_eLoadConfig(config_buffer);
    TEST_ASSERT_EQUAL(PERCFG_OK_E, result);
    TEST_ASSERT_EQUAL_STRING(test_config, config_buffer);
}

/**
 * Test: Update config while preserving password
 * Uses 32-byte binary hash for password
 */
void test_vPersistentCfg_UpdateConfigKeepPassword(void)
{
    init_mock_EEPROM();

    const char *test_password_hex = "abcdef0123456789abcdef0123456789abcdef0123456789abcdef0123456789";
    uint8_t au8PasswordBinary[32];
    uint8_t au8PasswordReadBack[32];
    const char *original_config = "pin1=A0";
    const char *new_config = "pin1=A0/pin2=A1/pin3=D2";
    char config_buffer[PINCFG_CONFIG_MAX_SZ_D];

    // Convert hex password to binary
    CliAuth_bHexToBinary(test_password_hex, au8PasswordBinary, 32);

    // Set initial password and config
    PERCFG_RESULT_T result = PersistentCfg_eWritePassword(au8PasswordBinary);
    TEST_ASSERT_EQUAL(PERCFG_OK_E, result);
    result = PersistentCfg_eSaveConfig(original_config);
    TEST_ASSERT_EQUAL(PERCFG_OK_E, result);

    // Update config
    result = PersistentCfg_eSaveConfig(new_config);
    TEST_ASSERT_EQUAL(PERCFG_OK_E, result);

    // Verify new config
    memset(config_buffer, 0, sizeof(config_buffer));
    result = PersistentCfg_eLoadConfig(config_buffer);
    TEST_ASSERT_EQUAL(PERCFG_OK_E, result);
    TEST_ASSERT_EQUAL_STRING(new_config, config_buffer);

    // Verify password unchanged
    memset(au8PasswordReadBack, 0, sizeof(au8PasswordReadBack));
    result = PersistentCfg_eReadPassword(au8PasswordReadBack);
    TEST_ASSERT_EQUAL(PERCFG_OK_E, result);
    TEST_ASSERT_EQUAL_MEMORY(au8PasswordBinary, au8PasswordReadBack, 32);
}

/**
 * Test: Password corruption - MUST be 100% recoverable!
 * With dynamic backup, full password is always backed up.
 */
void test_vPersistentCfg_PasswordCorruption_FullRecovery(void)
{
    init_mock_EEPROM();

    const char *test_password_hex = "1234567890abcdef1234567890abcdef1234567890abcdef1234567890abcdef";
    uint8_t au8PasswordBinary[32];
    uint8_t au8PasswordReadBack[32];
    const char *test_config = "some_config_data";

    // Convert hex password to binary
    CliAuth_bHexToBinary(test_password_hex, au8PasswordBinary, 32);

    // Save password and config
    PERCFG_RESULT_T result = PersistentCfg_eWritePassword(au8PasswordBinary);
    TEST_ASSERT_EQUAL(PERCFG_OK_E, result);
    result = PersistentCfg_eSaveConfig(test_config);
    TEST_ASSERT_EQUAL(PERCFG_OK_E, result);

    // Corrupt password section
    uint16_t password_start = EEPROM_LOCAL_CONFIG_ADDRESS + 6; // After CRC16×3 = 419
    corrupt_EEPROM_byte(password_start + 5, 3);
    corrupt_EEPROM_byte(password_start + 10, 7);
    corrupt_EEPROM_byte(password_start + 20, 2);

    // Read password - MUST succeed (100% recovery guarantee!)
    memset(au8PasswordReadBack, 0, sizeof(au8PasswordReadBack));
    result = PersistentCfg_eReadPassword(au8PasswordReadBack);
    TEST_ASSERT_EQUAL(PERCFG_OK_E, result);
    TEST_ASSERT_EQUAL_MEMORY(au8PasswordBinary, au8PasswordReadBack, 32);
}

/**
 * Test: Config corruption in first 29 bytes (backed up region)
 */
void test_vPersistentCfg_ConfigCorruption_BackedUpRegion(void)
{
    init_mock_EEPROM();

    const char *test_password_hex = "fedcba9876543210fedcba9876543210fedcba9876543210fedcba9876543210";
    uint8_t au8PasswordBinary[32];
    const char *test_config = "pin1=A0,sensor,temp/pin2=A1"; // ~28 bytes
    char config_buffer[PINCFG_CONFIG_MAX_SZ_D];

    // Convert hex password to binary
    CliAuth_bHexToBinary(test_password_hex, au8PasswordBinary, 32);

    // Save password and config
    PERCFG_RESULT_T result = PersistentCfg_eWritePassword(au8PasswordBinary);
    TEST_ASSERT_EQUAL(PERCFG_OK_E, result);
    result = PersistentCfg_eSaveConfig(test_config);
    TEST_ASSERT_EQUAL(PERCFG_OK_E, result);

    // Corrupt config in first 29 bytes (backed up)
    uint16_t config_start = EEPROM_LOCAL_CONFIG_ADDRESS + 6 + PINCFG_AUTH_PASSWORD_LEN_D;
    corrupt_EEPROM_byte(config_start + 10, 4);
    corrupt_EEPROM_byte(config_start + 20, 5);

    // Read config - should succeed (within backup)
    memset(config_buffer, 0, sizeof(config_buffer));
    result = PersistentCfg_eLoadConfig(config_buffer);
    TEST_ASSERT_EQUAL(PERCFG_OK_E, result);
    TEST_ASSERT_EQUAL_STRING(test_config, config_buffer);
}

/**
 * Test: Config corruption beyond backup region (unrecoverable)
 * With dynamic backup, smaller configs are fully backed up.
 * We need a large config (300+ bytes) to exceed backup capacity.
 */
void test_vPersistentCfg_ConfigCorruption_BeyondBackup(void)
{
    init_mock_EEPROM();

    const char *test_password_hex = "0011223344556677889900aabbccddeeff0011223344556677889900aabbccdd";
    uint8_t au8PasswordBinary[32];
    // Large config: 300 bytes to exceed dynamic backup capacity
    // Total = 32 + 300 = 332 bytes
    // NumBlocks = 11, Checksums = 22 bytes
    // Backup start = 451 + 300 + 22 = 773
    // Backup max = 1024 - 773 = 251 bytes (less than 332 needed)
    // Backup covers: 32 (pwd) + 219 (partial cfg) = 251 bytes
    char test_config[350];
    memset(test_config, 'X', 300);
    test_config[300] = '\0';

    // Convert hex password to binary
    CliAuth_bHexToBinary(test_password_hex, au8PasswordBinary, 32);

    // Save password and config
    PERCFG_RESULT_T result = PersistentCfg_eWritePassword(au8PasswordBinary);
    TEST_ASSERT_EQUAL(PERCFG_OK_E, result);
    result = PersistentCfg_eSaveConfig(test_config);
    TEST_ASSERT_EQUAL(PERCFG_OK_E, result);

    // Corrupt config at position 250 (beyond backup coverage of 219 bytes of config)
    // Config backup covers bytes 0-218, so corruption at 250 is unrecoverable
    uint16_t config_start = EEPROM_LOCAL_CONFIG_ADDRESS + 6 + PINCFG_AUTH_PASSWORD_LEN_D;
    corrupt_EEPROM_byte(config_start + 250, 3);

    // Read config - should fail (beyond backup)
    char config_buffer[PINCFG_CONFIG_MAX_SZ_D];
    memset(config_buffer, 0, sizeof(config_buffer));
    result = PersistentCfg_eLoadConfig(config_buffer);
    TEST_ASSERT_EQUAL(PERCFG_READ_FAILED_E, result);
}

/**
 * Test: Password with various binary patterns
 * Password is now stored as 32-byte binary hash
 */
void test_vPersistentCfg_PasswordBinaryPatterns(void)
{
    init_mock_EEPROM();

    // Test various binary password patterns (as hex strings)
    const char *passwords_hex[] = {
        "0000000000000000000000000000000000000000000000000000000000000001", // mostly zeros
        "ffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff", // all ones
        "0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef", // mixed
        "deadbeefdeadbeefdeadbeefdeadbeefdeadbeefdeadbeefdeadbeefdeadbeef"  // pattern
    };

    for (int i = 0; i < 4; i++)
    {
        init_mock_EEPROM();

        uint8_t au8PasswordBinary[32];
        uint8_t au8PasswordReadBack[32];
        CliAuth_bHexToBinary(passwords_hex[i], au8PasswordBinary, 32);

        PERCFG_RESULT_T result = PersistentCfg_eWritePassword(au8PasswordBinary);
        TEST_ASSERT_EQUAL(PERCFG_OK_E, result);

        memset(au8PasswordReadBack, 0, sizeof(au8PasswordReadBack));
        result = PersistentCfg_eReadPassword(au8PasswordReadBack);
        TEST_ASSERT_EQUAL(PERCFG_OK_E, result);
        TEST_ASSERT_EQUAL_MEMORY(au8PasswordBinary, au8PasswordReadBack, 32);
    }
}

/**
 * Test: Config with null terminator handling
 */
void test_vPersistentCfg_ConfigNullTerminator(void)
{
    init_mock_EEPROM();

    uint8_t au8PasswordBinary[32];
    memset(au8PasswordBinary, 0x55, 32); // arbitrary pattern

    // Config with explicit null
    const char *config_with_null = "pin1=A0\0extra_data";
    char config_buffer[PINCFG_CONFIG_MAX_SZ_D];

    PERCFG_RESULT_T result = PersistentCfg_eWritePassword(au8PasswordBinary);
    TEST_ASSERT_EQUAL(PERCFG_OK_E, result);

    result = PersistentCfg_eSaveConfig(config_with_null);
    TEST_ASSERT_EQUAL(PERCFG_OK_E, result);

    memset(config_buffer, 0, sizeof(config_buffer));
    result = PersistentCfg_eLoadConfig(config_buffer);
    TEST_ASSERT_EQUAL(PERCFG_OK_E, result);
    TEST_ASSERT_EQUAL_STRING("pin1=A0", config_buffer); // Should stop at null
}

/**
 * Test: Password is exactly 32 bytes (binary hash)
 */
void test_vPersistentCfg_PasswordFixedSize(void)
{
    init_mock_EEPROM();

    // All 32 bytes set to 'P' (0x50)
    uint8_t au8PasswordBinary[32];
    memset(au8PasswordBinary, 'P', 32);

    uint8_t au8PasswordReadBack[32];

    PERCFG_RESULT_T result = PersistentCfg_eWritePassword(au8PasswordBinary);
    TEST_ASSERT_EQUAL(PERCFG_OK_E, result);

    memset(au8PasswordReadBack, 0, sizeof(au8PasswordReadBack));
    result = PersistentCfg_eReadPassword(au8PasswordReadBack);
    TEST_ASSERT_EQUAL(PERCFG_OK_E, result);
    TEST_ASSERT_EQUAL_MEMORY(au8PasswordBinary, au8PasswordReadBack, 32);
}

/**
 * Test: Config maximum length (480 bytes including null)
 */
void test_vPersistentCfg_ConfigMaxLength(void)
{
    init_mock_EEPROM();

    uint8_t au8PasswordBinary[32];
    memset(au8PasswordBinary, 0xAA, 32); // arbitrary pattern

    // 479 characters + null terminator = 480 bytes (max)
    char max_config[PINCFG_CONFIG_MAX_SZ_D];
    memset(max_config, 'C', PINCFG_CONFIG_MAX_SZ_D - 1);
    max_config[PINCFG_CONFIG_MAX_SZ_D - 1] = '\0';

    char config_buffer[PINCFG_CONFIG_MAX_SZ_D];

    PERCFG_RESULT_T result = PersistentCfg_eWritePassword(au8PasswordBinary);
    TEST_ASSERT_EQUAL(PERCFG_OK_E, result);

    result = PersistentCfg_eSaveConfig(max_config);
    TEST_ASSERT_EQUAL(PERCFG_OK_E, result);

    memset(config_buffer, 0, sizeof(config_buffer));
    result = PersistentCfg_eLoadConfig(config_buffer);
    TEST_ASSERT_EQUAL(PERCFG_OK_E, result);
    TEST_ASSERT_EQUAL_STRING(max_config, config_buffer);
}

/**
 * Test: Config size exceeds maximum (should fail)
 */
void test_vPersistentCfg_ConfigOversized(void)
{
    init_mock_EEPROM();

    uint8_t au8PasswordBinary[32];
    memset(au8PasswordBinary, 0x11, 32); // arbitrary pattern
    char oversized_config[600];
    memset(oversized_config, 'X', 599);
    oversized_config[599] = '\0';

    PERCFG_RESULT_T result = PersistentCfg_eWritePassword(au8PasswordBinary);
    TEST_ASSERT_EQUAL(PERCFG_OK_E, result);

    // Try to save oversized config - should fail
    result = PersistentCfg_eSaveConfig(oversized_config);
    TEST_ASSERT_EQUAL(PERCFG_CFG_BIGGER_THAN_MAX_SZ_E, result);
}

/**
 * Test: CRC16 3-way voting - one corrupted copy
 */
void test_vPersistentCfg_CRC16_ThreeWayVoting(void)
{
    init_mock_EEPROM();

    const char *test_password_hex = "aabbccdd00112233aabbccdd00112233aabbccdd00112233aabbccdd00112233";
    uint8_t au8PasswordBinary[32];
    uint8_t au8PasswordReadBack[32];
    const char *test_config = "config_data";

    // Convert hex password to binary
    CliAuth_bHexToBinary(test_password_hex, au8PasswordBinary, 32);

    // Save data
    PERCFG_RESULT_T result = PersistentCfg_eWritePassword(au8PasswordBinary);
    TEST_ASSERT_EQUAL(PERCFG_OK_E, result);
    result = PersistentCfg_eSaveConfig(test_config);
    TEST_ASSERT_EQUAL(PERCFG_OK_E, result);

    // Corrupt one CRC16 copy (other 2 should vote it out)
    uint16_t crc_addr = EEPROM_LOCAL_CONFIG_ADDRESS;
    corrupt_EEPROM_byte(crc_addr, 0);
    corrupt_EEPROM_byte(crc_addr + 1, 0);

    // Read - should auto-repair via voting
    memset(au8PasswordReadBack, 0, sizeof(au8PasswordReadBack));
    result = PersistentCfg_eReadPassword(au8PasswordReadBack);
    TEST_ASSERT_EQUAL(PERCFG_OK_E, result);
    TEST_ASSERT_EQUAL_MEMORY(au8PasswordBinary, au8PasswordReadBack, 32);
}

/**
 * Test: CRC16 3-way voting - all three different (should fail)
 */
void test_vPersistentCfg_CRC16_AllDifferent(void)
{
    init_mock_EEPROM();

    const char *test_password_hex = "eeff00112233445566778899aabbccddeeff00112233445566778899aabbccdd";
    uint8_t au8PasswordBinary[32];
    uint8_t au8PasswordReadBack[32];

    // Convert hex password to binary
    CliAuth_bHexToBinary(test_password_hex, au8PasswordBinary, 32);

    // Save data
    PERCFG_RESULT_T result = PersistentCfg_eWritePassword(au8PasswordBinary);
    TEST_ASSERT_EQUAL(PERCFG_OK_E, result);

    // Corrupt all three CRC16 copies differently
    uint16_t crc_addr1 = EEPROM_LOCAL_CONFIG_ADDRESS;
    uint16_t crc_addr2 = EEPROM_LOCAL_CONFIG_ADDRESS + 2;
    uint16_t crc_addr3 = EEPROM_LOCAL_CONFIG_ADDRESS + 4;

    corrupt_EEPROM_byte(crc_addr1, 0);
    corrupt_EEPROM_byte(crc_addr2, 1);
    corrupt_EEPROM_byte(crc_addr3, 2);

    // Read - should fail (no majority vote possible)
    memset(au8PasswordReadBack, 0, sizeof(au8PasswordReadBack));
    result = PersistentCfg_eReadPassword(au8PasswordReadBack);
    TEST_ASSERT_EQUAL(PERCFG_READ_CHECKSUM_FAILED_E, result);
}

/**
 * Test: Block CRC double redundancy repair
 */
void test_vPersistentCfg_BlockCRC_DoubleRedundancy(void)
{
    init_mock_EEPROM();

    uint8_t au8PasswordBinary[32];
    memset(au8PasswordBinary, 0x22, 32); // arbitrary pattern
    const char *test_config = "config_with_multiple_blocks_of_data";
    char config_buffer[PINCFG_CONFIG_MAX_SZ_D];

    // Save data
    PERCFG_RESULT_T result = PersistentCfg_eWritePassword(au8PasswordBinary);
    TEST_ASSERT_EQUAL(PERCFG_OK_E, result);
    result = PersistentCfg_eSaveConfig(test_config);
    TEST_ASSERT_EQUAL(PERCFG_OK_E, result);

    // Corrupt first block CRC copy (should repair from second)
    uint16_t block_crc_start = EEPROM_LOCAL_CONFIG_ADDRESS + 6 + PINCFG_AUTH_PASSWORD_LEN_D + PINCFG_CONFIG_MAX_SZ_D;
    corrupt_EEPROM_byte(block_crc_start, 3);

    // Read - should detect and repair
    memset(config_buffer, 0, sizeof(config_buffer));
    result = PersistentCfg_eLoadConfig(config_buffer);
    TEST_ASSERT_EQUAL(PERCFG_OK_E, result);
    TEST_ASSERT_EQUAL_STRING(test_config, config_buffer);
}

/**
 * Test: Config size query accuracy
 */
void test_vPersistentCfg_ConfigSizeQuery(void)
{
    init_mock_EEPROM();

    uint8_t au8PasswordBinary[32];
    memset(au8PasswordBinary, 0x33, 32); // arbitrary pattern
    const char *configs[] = {
        "short", "medium_length_config_data", "this_is_a_much_longer_configuration_with_many_pins_and_sensors"};

    for (int i = 0; i < 3; i++)
    {
        init_mock_EEPROM();

        PERCFG_RESULT_T result = PersistentCfg_eWritePassword(au8PasswordBinary);
        TEST_ASSERT_EQUAL(PERCFG_OK_E, result);

        uint16_t expected_size = strlen(configs[i]); // Data length without null terminator
        result = PersistentCfg_eSaveConfig(configs[i]);
        TEST_ASSERT_EQUAL(PERCFG_OK_E, result);

        uint16_t actual_size = 0;
        result = PersistentCfg_eGetConfigSize(&actual_size);
        TEST_ASSERT_EQUAL(PERCFG_OK_E, result);
        TEST_ASSERT_EQUAL(expected_size, actual_size); // Returns data length, not including null
    }
}

// ============================================================================
// Test Registration Function
// ============================================================================

void register_persistent_config_tests(void)
{
    RUN_TEST(test_vPersistentCfg_PasswordOnly);
    RUN_TEST(test_vPersistentCfg_ConfigOnly);
    RUN_TEST(test_vPersistentCfg_PasswordAndConfig);
    RUN_TEST(test_vPersistentCfg_UpdatePasswordKeepConfig);
    RUN_TEST(test_vPersistentCfg_UpdateConfigKeepPassword);
    RUN_TEST(test_vPersistentCfg_PasswordCorruption_FullRecovery);
    RUN_TEST(test_vPersistentCfg_ConfigCorruption_BackedUpRegion);
    RUN_TEST(test_vPersistentCfg_ConfigCorruption_BeyondBackup);
    RUN_TEST(test_vPersistentCfg_PasswordBinaryPatterns);
    RUN_TEST(test_vPersistentCfg_ConfigNullTerminator);
    RUN_TEST(test_vPersistentCfg_PasswordFixedSize);
    RUN_TEST(test_vPersistentCfg_ConfigMaxLength);
    RUN_TEST(test_vPersistentCfg_ConfigOversized);
    RUN_TEST(test_vPersistentCfg_CRC16_ThreeWayVoting);
    RUN_TEST(test_vPersistentCfg_CRC16_AllDifferent);
    RUN_TEST(test_vPersistentCfg_BlockCRC_DoubleRedundancy);
    RUN_TEST(test_vPersistentCfg_ConfigSizeQuery);
}
