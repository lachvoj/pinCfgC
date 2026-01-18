#include "test_helpers.h"

// Global test memory buffer
uint8_t testMemory[MEMORY_SZ];

// Mock EEPROM storage (must be non-static for EEPROMMock.cpp to access)
uint8_t mock_EEPROM[1024];

// Default password binary hash (SHA-256 of "admin123")
static const uint8_t _au8TestDefaultPasswordBinary[32] = {
    0x24, 0x0b, 0xe5, 0x18, 0xfa, 0xbd, 0x27, 0x24, 0xdd, 0xb6, 0xf0,
    0x4e, 0xeb, 0x1d, 0xa5, 0x96, 0x74, 0x48, 0xd7, 0xe8, 0x31, 0xc0,
    0x8c, 0x8f, 0xa8, 0x22, 0x80, 0x9f, 0x74, 0xc7, 0x20, 0xa9
};

void setUp(void)
{
    init_mock_EEPROM(); // Initialize EEPROM to empty state (0xFF)

    PinCfgCsv_eInit(testMemory, MEMORY_SZ, NULL);

    init_MySensorsMock();
    init_ArduinoMock();
}

void tearDown(void)
{
    // clean stuff up here
}

void init_mock_EEPROM(void)
{
    memset(mock_EEPROM, 0xFF, sizeof(mock_EEPROM)); // 0xFF is typical EEPROM erased state
}

// Helper: Initialize EEPROM with default password (for CLI tests that need authentication)
void init_mock_EEPROM_with_default_password(void)
{
    init_mock_EEPROM();
    // Write default password binary hash to EEPROM
    PersistentCfg_eWritePassword(_au8TestDefaultPasswordBinary);
}

// Helper: Write password from hex string to EEPROM (for persistent config tests)
PERCFG_RESULT_T write_password_from_hex(const char *hex_password)
{
    uint8_t au8Binary[32];
    if (!CliAuth_bHexToBinary(hex_password, au8Binary, 32))
        return PERCFG_ERROR_E;
    return PersistentCfg_eWritePassword(au8Binary);
}

// Helper: Corrupt a specific byte in mock EEPROM
void corrupt_EEPROM_byte(uint16_t address, uint8_t bit_position)
{
    mock_EEPROM[address] ^= (1 << bit_position);
}

// Helper: Corrupt a specific address range
void corrupt_EEPROM_range(uint16_t start, uint16_t end)
{
    for (uint16_t i = start; i < end; i++)
    {
        mock_EEPROM[i] ^= 0x55; // Flip multiple bits
    }
}
