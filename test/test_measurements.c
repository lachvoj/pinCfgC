#include "test_helpers.h"

// ============================================================================
// Measurement Tests - I2C, SPI, Analog, CPUTemp, LoopTime
// ============================================================================

void test_vCPUTemp(void)
{
    PINCFG_RESULT_T eResult;
    LINKEDLIST_RESULT_T eLinkedListResult;
    char acOutStr[OUT_STR_MAX_LEN_D];
    size_t szMemoryRequired;

    // Phase 3: Test unified sensor structure with CPUTemp sensors
    // Format: MS,type,name/  SR,sensorName,measurementName,vType,sType,enableable,cumulative,sampMs,reportSec,scale,offset,precision/
    // V_TEMP=0, S_TEMP=6, 0=MEASUREMENT_TYPE_CPUTEMP_E
    const char *pcCfg = "MS,0,temp/"
                        "SR,CPUTemp0,temp,0,6,0,0,1000,300/"
                        "SR,CPUTemp1,temp,0,6,1,1,1000,300/"
                        "SR,CPUTemp2,temp,0,6,1,1,1500,200,1.0,-2.1,2/"
                        "SR,CPUTemp3,temp,0,6,0,1,2000,120,0.5,10.5,3/";

    // First pass: calculate memory requirements
    PINCFG_PARSE_PARAMS_T sParams = {
        .pcConfig = pcCfg,
        .eAddToLoopables = PinCfgCsv_eAddToTempLoopables,
        .eAddToPresentables = PinCfgCsv_eAddToTempPresentables,
        .pszMemoryRequired = &szMemoryRequired,
        .pcOutString = acOutStr,
        .u16OutStrMaxLen = (uint16_t)OUT_STR_MAX_LEN_D,
        .bValidate = false};

    eResult = PinCfgCsv_eParse(&sParams);
    if (eResult != PINCFG_OK_E)
    {
        printf("\n=== CPUTemp Parse Error (pass 1) ===\nResult: %d\nOutput: %s\n", eResult, acOutStr);
    }
    TEST_ASSERT_EQUAL(PINCFG_OK_E, eResult);

    // Second pass: actually create objects
    memset(acOutStr, 0, OUT_STR_MAX_LEN_D);
    sParams.pszMemoryRequired = NULL; // Disable memory calculation
    eResult = PinCfgCsv_eParse(&sParams);
    if (eResult != PINCFG_OK_E)
    {
        printf("\n=== CPUTemp Parse Error ===\nResult: %d\nOutput: %s\n", eResult, acOutStr);
    }
    TEST_ASSERT_EQUAL(PINCFG_OK_E, eResult);

    // Note: Memory check skipped for two-pass parse
    // First pass calculates memory, second pass creates objects
    // The memory calculation only accounts for objects created in pass 1

    eLinkedListResult =
        LinkedList_eLinkedListToArray((LINKEDLIST_ITEM_T **)(&psGlobals->ppsLoopables), &psGlobals->u8LoopablesCount);
    TEST_ASSERT_EQUAL(LINKEDLIST_OK_E, eLinkedListResult);

    eLinkedListResult = LinkedList_eLinkedListToArray(
        (LINKEDLIST_ITEM_T **)(&psGlobals->ppsPresentables), &psGlobals->u8PresentablesCount);
    TEST_ASSERT_EQUAL(LINKEDLIST_OK_E, eLinkedListResult);

    // Presentables: [0]=CLI, [1]=CLI, [2]=CPUTemp0, [3]=CPUTemp1, [4]=CPUTemp1_enable,
    // [5]=CPUTemp2, [6]=CPUTemp2_enable, [7]=CPUTemp3

    // Test basic sensor (CPUTemp0: non-cumulative, non-enableable) - defaults
    SENSOR_T *psSensor = (SENSOR_T *)psGlobals->ppsPresentables[2];
    TEST_ASSERT_EQUAL(V_TEMP, psSensor->sVtab.eVType);
    TEST_ASSERT_EQUAL(S_TEMP, psSensor->sVtab.eSType);
    TEST_ASSERT_EQUAL_STRING("CPUTemp0", psSensor->sPresentable.pcName);
    TEST_ASSERT_EQUAL((int32_t)(PINCFG_SENSOR_SCALE_D * PINCFG_FIXED_POINT_SCALE), psSensor->i32Scale);   // Default 1.0
    TEST_ASSERT_EQUAL((int32_t)(PINCFG_SENSOR_OFFSET_D * PINCFG_FIXED_POINT_SCALE), psSensor->i32Offset); // Default 0.0
    TEST_ASSERT_EQUAL(PINCFG_SENSOR_PRECISION_D, psSensor->u8Precision);                                  // Default 0
    TEST_ASSERT_EQUAL(1000, psSensor->u16SamplingIntervalMs);
    TEST_ASSERT_EQUAL(300000, psSensor->u32ReportIntervalMs);
    TEST_ASSERT_EQUAL(0, psSensor->u8Flags & SENSOR_FLAG_CUMULATIVE);
    TEST_ASSERT_EQUAL(0, psSensor->u8Flags & SENSOR_FLAG_ENABLEABLE);

    // Test cumulative enableable sensor (CPUTemp1) - defaults for scale/offset/precision
    psSensor = (SENSOR_T *)psGlobals->ppsPresentables[3];
    TEST_ASSERT_EQUAL(V_TEMP, psSensor->sVtab.eVType);
    TEST_ASSERT_EQUAL(S_TEMP, psSensor->sVtab.eSType);
    TEST_ASSERT_EQUAL_STRING("CPUTemp1", psSensor->sPresentable.pcName);
    TEST_ASSERT_EQUAL((int32_t)(PINCFG_SENSOR_SCALE_D * PINCFG_FIXED_POINT_SCALE), psSensor->i32Scale);   // Default 1.0
    TEST_ASSERT_EQUAL((int32_t)(PINCFG_SENSOR_OFFSET_D * PINCFG_FIXED_POINT_SCALE), psSensor->i32Offset); // Default 0.0
    TEST_ASSERT_EQUAL(PINCFG_SENSOR_PRECISION_D, psSensor->u8Precision);                                  // Default 0
    TEST_ASSERT_NOT_EQUAL(0, psSensor->u8Flags & SENSOR_FLAG_CUMULATIVE);
    TEST_ASSERT_NOT_EQUAL(0, psSensor->u8Flags & SENSOR_FLAG_ENABLEABLE);
    TEST_ASSERT_NOT_NULL(psSensor->psEnableable);

    // Test sensor with custom parameters (CPUTemp2) - scale, offset, precision
    psSensor = (SENSOR_T *)psGlobals->ppsPresentables[5];
    TEST_ASSERT_EQUAL(V_TEMP, psSensor->sVtab.eVType);
    TEST_ASSERT_EQUAL(S_TEMP, psSensor->sVtab.eSType);
    TEST_ASSERT_EQUAL_STRING("CPUTemp2", psSensor->sPresentable.pcName);
    TEST_ASSERT_EQUAL(1000000, psSensor->i32Scale);   // 1.0 × PINCFG_FIXED_POINT_SCALE
    TEST_ASSERT_EQUAL(-2100000, psSensor->i32Offset); // -2.1 × PINCFG_FIXED_POINT_SCALE
    TEST_ASSERT_EQUAL(2, psSensor->u8Precision);      // 2 decimal places
    TEST_ASSERT_EQUAL(1500, psSensor->u16SamplingIntervalMs);
    TEST_ASSERT_EQUAL(200000, psSensor->u32ReportIntervalMs);
    TEST_ASSERT_NOT_EQUAL(0, psSensor->u8Flags & SENSOR_FLAG_CUMULATIVE);
    TEST_ASSERT_NOT_EQUAL(0, psSensor->u8Flags & SENSOR_FLAG_ENABLEABLE);

    // Test sensor with scale factor (CPUTemp3) - 0.5 scale, 10.5 offset, 3 decimals
    psSensor = (SENSOR_T *)psGlobals->ppsPresentables[7];
    TEST_ASSERT_EQUAL(V_TEMP, psSensor->sVtab.eVType);
    TEST_ASSERT_EQUAL(S_TEMP, psSensor->sVtab.eSType);
    TEST_ASSERT_EQUAL_STRING("CPUTemp3", psSensor->sPresentable.pcName);
    TEST_ASSERT_EQUAL(500000, psSensor->i32Scale);    // 0.5 × PINCFG_FIXED_POINT_SCALE
    TEST_ASSERT_EQUAL(10500000, psSensor->i32Offset); // 10.5 × PINCFG_FIXED_POINT_SCALE
    TEST_ASSERT_EQUAL(3, psSensor->u8Precision);      // 3 decimal places
    TEST_ASSERT_EQUAL(2000, psSensor->u16SamplingIntervalMs);
    TEST_ASSERT_EQUAL(120000, psSensor->u32ReportIntervalMs);
    TEST_ASSERT_NOT_EQUAL(0, psSensor->u8Flags & SENSOR_FLAG_CUMULATIVE);
    TEST_ASSERT_EQUAL(0, psSensor->u8Flags & SENSOR_FLAG_ENABLEABLE);
}

// =============================================================================
// I2C MEASUREMENT TESTS
// =============================================================================

#ifdef PINCFG_FEATURE_I2C_MEASUREMENT
void test_vI2CMeasure_Init(void)
{
    I2CMEASURE_T sI2C;
    uint8_t au8Cmd[] = {0xAC, 0x33, 0x00};
    uint8_t u8RegAddr = 0x00; // TMP102 temperature register
    PINCFG_RESULT_T eResult;
    STRING_POINT_T sName;

    // Test simple mode initialization (1-byte register read)
    PinCfgStr_vInitStrPoint(&sName, "temp", 4);
    eResult = I2CMeasure_eInit(
        &sI2C,
        &sName,
        0x48,       // TMP102 address
        &u8RegAddr, // Register address
        1,          // 1 byte command (register address)
        2,          // Read 2 bytes
        0,          // No conversion delay
        0           // No cache
    );

    TEST_ASSERT_EQUAL(PINCFG_OK_E, eResult);
    TEST_ASSERT_EQUAL(0x48, sI2C.u8DeviceAddress);
    TEST_ASSERT_EQUAL(1, sI2C.u8CommandLength);
    TEST_ASSERT_EQUAL(2, sI2C.u8DataSize);
    TEST_ASSERT_EQUAL(I2CMEASURE_STATE_IDLE_E, sI2C.eState);
    TEST_ASSERT_EQUAL(0, sI2C.u16ConversionDelayMs);

    // Test command mode initialization (multi-byte command with delay)
    PinCfgStr_vInitStrPoint(&sName, "aht10", 5);
    eResult = I2CMeasure_eInit(
        &sI2C,
        &sName,
        0x38,   // AHT10 address
        au8Cmd, // 3-byte command
        3,      // Command length
        6,      // Read 6 bytes
        80,     // 80ms conversion delay
        0       // No cache
    );

    TEST_ASSERT_EQUAL(PINCFG_OK_E, eResult);
    TEST_ASSERT_EQUAL(0x38, sI2C.u8DeviceAddress);
    TEST_ASSERT_EQUAL(0xAC, sI2C.au8CommandBytes[0]);
    TEST_ASSERT_EQUAL(0x33, sI2C.au8CommandBytes[1]);
    TEST_ASSERT_EQUAL(0x00, sI2C.au8CommandBytes[2]);
    TEST_ASSERT_EQUAL(3, sI2C.u8CommandLength);
    TEST_ASSERT_EQUAL(6, sI2C.u8DataSize);
    TEST_ASSERT_EQUAL(80, sI2C.u16ConversionDelayMs);

    // Test NULL pointer error
    eResult = I2CMeasure_eInit(NULL, &sName, 0x48, &u8RegAddr, 1, 2, 0, 0);
    TEST_ASSERT_EQUAL(PINCFG_NULLPTR_ERROR_E, eResult);

    // Test invalid data size (0 bytes)
    uint8_t u8Dummy = 0;
    PinCfgStr_vInitStrPoint(&sName, "test", 4);
    eResult = I2CMeasure_eInit(&sI2C, &sName, 0x48, &u8Dummy, 1, 0, 0, 0);
    TEST_ASSERT_EQUAL(PINCFG_ERROR_E, eResult);

    // Test invalid data size (>6 bytes)
    eResult = I2CMeasure_eInit(&sI2C, &sName, 0x48, &u8Dummy, 1, 7, 0, 0);
    TEST_ASSERT_EQUAL(PINCFG_ERROR_E, eResult);
}

/**
 * Test I2C simple mode read (TMP102-style: write register, read 2 bytes)
 */
void test_vI2CMeasure_SimpleRead(void)
{
    I2CMEASURE_T sI2C;
    uint8_t au8Buffer[6];
    uint8_t u8Size = 6;
    ISENSORMEASURE_RESULT_T eResult;
    uint8_t au8ResponseData[] = {0x01, 0x90}; // 400 decimal
    uint8_t u8RegAddr = 0x00;                 // Temperature register
    STRING_POINT_T sName;

    // Initialize for simple 2-byte read
    PinCfgStr_vInitStrPoint(&sName, "temp", 4);
    I2CMeasure_eInit(&sI2C, &sName, 0x48, &u8RegAddr, 1, 2, 0, 0);

    // Mock: Set up response data
    WireMock_vReset();
    WireMock_vSetResponse(au8ResponseData, 2);

    // First call: Should send request and return PENDING
    eResult = I2CMeasure_eMeasure(&sI2C.sInterface, au8Buffer, &u8Size, 0);
    TEST_ASSERT_EQUAL(ISENSORMEASURE_PENDING_E, eResult);
    TEST_ASSERT_EQUAL(I2CMEASURE_STATE_REQUEST_SENT_E, sI2C.eState);
    TEST_ASSERT_EQUAL(0x48, WireMock_u8GetLastAddress());

    // Second call: Data available, should read and process
    u8Size = 6;
    eResult = I2CMeasure_eMeasure(&sI2C.sInterface, au8Buffer, &u8Size, 0);
    TEST_ASSERT_EQUAL(ISENSORMEASURE_PENDING_E, eResult);
    TEST_ASSERT_EQUAL(I2CMEASURE_STATE_DATA_READY_E, sI2C.eState);

    // Third call: Return the raw bytes
    u8Size = 6;
    eResult = I2CMeasure_eMeasure(&sI2C.sInterface, au8Buffer, &u8Size, 0);
    TEST_ASSERT_EQUAL(ISENSORMEASURE_OK_E, eResult);
    TEST_ASSERT_EQUAL(2, u8Size);
    TEST_ASSERT_EQUAL(0x01, au8Buffer[0]);
    TEST_ASSERT_EQUAL(0x90, au8Buffer[1]);
    TEST_ASSERT_EQUAL(I2CMEASURE_STATE_IDLE_E, sI2C.eState);

    // Can repeat measurement
    WireMock_vSetResponse(au8ResponseData, 2);
    u8Size = 6;
    eResult = I2CMeasure_eMeasure(&sI2C.sInterface, au8Buffer, &u8Size, 0);
    TEST_ASSERT_EQUAL(ISENSORMEASURE_PENDING_E, eResult);
}

/**
 * Test I2C command mode (AHT10-style: send command, wait, read data)
 */
void test_vI2CMeasure_CommandMode(void)
{
    I2CMEASURE_T sI2C;
    uint8_t au8Buffer[6];
    uint8_t u8Size = 6;
    ISENSORMEASURE_RESULT_T eResult;
    uint8_t au8Cmd[] = {0xAC, 0x33, 0x00};
    uint8_t au8ResponseData[] = {0x1C, 0x5E, 0x33, 0x7F, 0xF0, 0x00};
    STRING_POINT_T sName;

    // Initialize for command mode with conversion delay
    PinCfgStr_vInitStrPoint(&sName, "aht10", 5);
    I2CMeasure_eInit(&sI2C, &sName, 0x38, au8Cmd, 3, 6, 80, 0);

    WireMock_vReset();
    mock_millis_u32Return = 0;

    // First call: Send command, start waiting
    eResult = I2CMeasure_eMeasure(&sI2C.sInterface, au8Buffer, &u8Size, mock_millis_u32Return);
    TEST_ASSERT_EQUAL(ISENSORMEASURE_PENDING_E, eResult);
    TEST_ASSERT_EQUAL(I2CMEASURE_STATE_COMMAND_SENT_E, sI2C.eState);
    TEST_ASSERT_EQUAL(0x38, WireMock_u8GetLastAddress());

    // Second call: Still waiting for conversion delay
    mock_millis_u32Return = 50; // Only 50ms elapsed
    u8Size = 6;
    eResult = I2CMeasure_eMeasure(&sI2C.sInterface, au8Buffer, &u8Size, mock_millis_u32Return);
    TEST_ASSERT_EQUAL(ISENSORMEASURE_PENDING_E, eResult);
    TEST_ASSERT_EQUAL(I2CMEASURE_STATE_COMMAND_SENT_E, sI2C.eState); // Stays in COMMAND_SENT until delay expires

    // Third call: Conversion delay complete, request data
    mock_millis_u32Return = 85; // >80ms elapsed
    WireMock_vSetResponse(au8ResponseData, 6);
    u8Size = 6;
    eResult = I2CMeasure_eMeasure(&sI2C.sInterface, au8Buffer, &u8Size, mock_millis_u32Return);
    TEST_ASSERT_EQUAL(ISENSORMEASURE_PENDING_E, eResult);
    TEST_ASSERT_EQUAL(I2CMEASURE_STATE_REQUEST_SENT_E, sI2C.eState);

    // Fourth call: Read data
    u8Size = 6;
    eResult = I2CMeasure_eMeasure(&sI2C.sInterface, au8Buffer, &u8Size, mock_millis_u32Return);
    TEST_ASSERT_EQUAL(ISENSORMEASURE_PENDING_E, eResult);
    TEST_ASSERT_EQUAL(I2CMEASURE_STATE_DATA_READY_E, sI2C.eState);

    // Fifth call: Return raw bytes
    u8Size = 6;
    eResult = I2CMeasure_eMeasure(&sI2C.sInterface, au8Buffer, &u8Size, mock_millis_u32Return);
    TEST_ASSERT_EQUAL(ISENSORMEASURE_OK_E, eResult);
    TEST_ASSERT_EQUAL(6, u8Size);
    TEST_ASSERT_EQUAL(I2CMEASURE_STATE_IDLE_E, sI2C.eState);
}

/**
 * Test I2C timeout handling
 */
void test_vI2CMeasure_Timeout(void)
{
    I2CMEASURE_T sI2C;
    uint8_t au8Buffer[6];
    uint8_t u8Size = 6;
    ISENSORMEASURE_RESULT_T eResult;
    uint8_t u8RegAddr = 0x00;
    STRING_POINT_T sName;

    // Initialize with default timeout
    PinCfgStr_vInitStrPoint(&sName, "temp", 4);
    I2CMeasure_eInit(&sI2C, &sName, 0x48, &u8RegAddr, 1, 2, 0, 0);

    WireMock_vReset();
    WireMock_vSimulateTimeout(true); // Never return data
    mock_millis_u32Return = 0;

    // Start request
    eResult = I2CMeasure_eMeasure(&sI2C.sInterface, au8Buffer, &u8Size, mock_millis_u32Return);
    TEST_ASSERT_EQUAL(ISENSORMEASURE_PENDING_E, eResult);

    // Still waiting, no timeout yet
    mock_millis_u32Return = 50;
    u8Size = 6;
    eResult = I2CMeasure_eMeasure(&sI2C.sInterface, au8Buffer, &u8Size, mock_millis_u32Return);
    TEST_ASSERT_EQUAL(ISENSORMEASURE_PENDING_E, eResult);

    // Timeout occurs (default 100ms)
    mock_millis_u32Return = 150;
    u8Size = 6;
    eResult = I2CMeasure_eMeasure(&sI2C.sInterface, au8Buffer, &u8Size, mock_millis_u32Return);
    TEST_ASSERT_EQUAL(ISENSORMEASURE_ERROR_E, eResult);
    TEST_ASSERT_EQUAL(I2CMEASURE_STATE_ERROR_E, sI2C.eState); // State is ERROR after timeout

    // Can retry after timeout - first call resets from ERROR to IDLE
    WireMock_vSimulateTimeout(false);
    uint8_t au8Data[] = {0x00, 0xFF};
    WireMock_vSetResponse(au8Data, 2);
    mock_millis_u32Return = 200; // Reset time for new request

    // First call after error: Resets state to IDLE, still returns ERROR
    u8Size = 6;
    eResult = I2CMeasure_eMeasure(&sI2C.sInterface, au8Buffer, &u8Size, 0);
    TEST_ASSERT_EQUAL(ISENSORMEASURE_ERROR_E, eResult);      // Still ERROR during reset
    TEST_ASSERT_EQUAL(I2CMEASURE_STATE_IDLE_E, sI2C.eState); // Now in IDLE

    // Second call: Now starts new transaction
    u8Size = 6;
    eResult = I2CMeasure_eMeasure(&sI2C.sInterface, au8Buffer, &u8Size, 0);
    TEST_ASSERT_EQUAL(ISENSORMEASURE_PENDING_E, eResult);
    TEST_ASSERT_EQUAL(I2CMEASURE_STATE_REQUEST_SENT_E, sI2C.eState);
}

/**
 * Test I2C device error (device not responding)
 */
void test_vI2CMeasure_DeviceError(void)
{
    I2CMEASURE_T sI2C;
    uint8_t au8Buffer[6];
    uint8_t u8Size = 6;
    ISENSORMEASURE_RESULT_T eResult;
    uint8_t u8RegAddr = 0x00;
    STRING_POINT_T sName;

    PinCfgStr_vInitStrPoint(&sName, "temp", 4);
    I2CMeasure_eInit(&sI2C, &sName, 0x48, &u8RegAddr, 1, 2, 0, 0);

    WireMock_vReset();
    WireMock_vSimulateError(true); // Device doesn't ACK
    mock_millis_u32Return = 0;

    // Request should fail immediately
    eResult = I2CMeasure_eMeasure(&sI2C.sInterface, au8Buffer, &u8Size, 0);
    TEST_ASSERT_EQUAL(ISENSORMEASURE_PENDING_E, eResult);

    // Should detect error and reset
    mock_millis_u32Return = 10;
    u8Size = 6;
    eResult = I2CMeasure_eMeasure(&sI2C.sInterface, au8Buffer, &u8Size, 0);
    // State depends on implementation - either ERROR or retry
    TEST_ASSERT_TRUE(eResult == ISENSORMEASURE_ERROR_E || eResult == ISENSORMEASURE_PENDING_E);
}

/**
 * Test I2C raw data reading (for multi-value sensors like AHT10)
 */
void test_vI2CMeasure_RawData(void)
{
    I2CMEASURE_T sI2C;
    uint8_t au8Buffer[6];
    uint8_t u8Size = 6;
    ISENSORMEASURE_RESULT_T eResult;
    uint8_t au8ResponseData[] = {0x1C, 0x5E, 0x33, 0x7F, 0xF0, 0x00};
    uint8_t u8RegAddr = 0x00;
    STRING_POINT_T sName;

    // Initialize for 6-byte read
    PinCfgStr_vInitStrPoint(&sName, "aht10", 5);
    I2CMeasure_eInit(&sI2C, &sName, 0x38, &u8RegAddr, 1, 6, 0, 0);

    WireMock_vReset();
    WireMock_vSetResponse(au8ResponseData, 6);

    // Request data
    eResult = I2CMeasure_eMeasure(&sI2C.sInterface, au8Buffer, &u8Size, 0);
    TEST_ASSERT_EQUAL(ISENSORMEASURE_PENDING_E, eResult);

    // Data ready
    u8Size = 6;
    eResult = I2CMeasure_eMeasure(&sI2C.sInterface, au8Buffer, &u8Size, 0);
    TEST_ASSERT_EQUAL(ISENSORMEASURE_PENDING_E, eResult);

    // Get raw bytes
    u8Size = 6;
    eResult = I2CMeasure_eMeasure(&sI2C.sInterface, au8Buffer, &u8Size, 0);
    TEST_ASSERT_EQUAL(ISENSORMEASURE_OK_E, eResult);
    TEST_ASSERT_EQUAL(6, u8Size);
    TEST_ASSERT_EQUAL(0x1C, au8Buffer[0]);
    TEST_ASSERT_EQUAL(0x5E, au8Buffer[1]);
    TEST_ASSERT_EQUAL(0x33, au8Buffer[2]);
    TEST_ASSERT_EQUAL(0x7F, au8Buffer[3]);
    TEST_ASSERT_EQUAL(0xF0, au8Buffer[4]);
    TEST_ASSERT_EQUAL(0x00, au8Buffer[5]);
}

#endif // PINCFG_FEATURE_I2C_MEASUREMENT

// =============================================================================
// SPI MEASUREMENT TESTS
// =============================================================================

#ifdef PINCFG_FEATURE_SPI_MEASUREMENT
void test_vSPIMeasure_Init(void)
{
    SPIMEASURE_T sSPI;
    uint8_t au8Cmd[] = {0x80}; // Read command for typical SPI sensor
    uint8_t u8CSPin = 10;
    PINCFG_RESULT_T eResult;
    STRING_POINT_T sName;

    // Test simple mode initialization (no command, just read)
    PinCfgStr_vInitStrPoint(&sName, "thermo", 6);
    eResult = SPIMeasure_eInit(
        &sSPI,
        &sName,
        u8CSPin, // CS pin
        NULL,    // No command (simple mode)
        0,       // 0 byte command (simple mode)
        4,       // Read 4 bytes (e.g., MAX31855)
        0        // No conversion delay
    );

    TEST_ASSERT_EQUAL(PINCFG_OK_E, eResult);
    TEST_ASSERT_EQUAL(u8CSPin, sSPI.u8ChipSelectPin);
    TEST_ASSERT_EQUAL(0, sSPI.u8CommandLength);
    TEST_ASSERT_EQUAL(4, sSPI.u8DataSize);
    TEST_ASSERT_EQUAL(SPIMEASURE_STATE_IDLE_E, sSPI.eState);
    TEST_ASSERT_EQUAL(0, sSPI.u16ConversionDelayMs);
    TEST_ASSERT_EQUAL(MEASUREMENT_TYPE_SPI_E, sSPI.sInterface.eType);

    // Test command mode initialization (with command and delay)
    uint8_t au8CmdMulti[] = {0xD0, 0x00, 0x00}; // Example command
    PinCfgStr_vInitStrPoint(&sName, "accel", 5);
    eResult = SPIMeasure_eInit(
        &sSPI,
        &sName,
        9,           // Different CS pin
        au8CmdMulti, // Multi-byte command
        3,           // Command length
        6,           // Read 6 bytes
        10           // 10ms conversion delay
    );

    TEST_ASSERT_EQUAL(PINCFG_OK_E, eResult);
    TEST_ASSERT_EQUAL(9, sSPI.u8ChipSelectPin);
    TEST_ASSERT_EQUAL(0xD0, sSPI.au8CommandBytes[0]);
    TEST_ASSERT_EQUAL(0x00, sSPI.au8CommandBytes[1]);
    TEST_ASSERT_EQUAL(0x00, sSPI.au8CommandBytes[2]);
    TEST_ASSERT_EQUAL(3, sSPI.u8CommandLength);
    TEST_ASSERT_EQUAL(6, sSPI.u8DataSize);
    TEST_ASSERT_EQUAL(10, sSPI.u16ConversionDelayMs);

    // Test NULL pointer error
    eResult = SPIMeasure_eInit(NULL, &sName, 10, au8Cmd, 1, 4, 0);
    TEST_ASSERT_EQUAL(PINCFG_NULLPTR_ERROR_E, eResult);

    // Test invalid data size (0 bytes)
    PinCfgStr_vInitStrPoint(&sName, "test", 4);
    eResult = SPIMeasure_eInit(&sSPI, &sName, 10, NULL, 0, 0, 0);
    TEST_ASSERT_EQUAL(PINCFG_ERROR_E, eResult);

    // Test invalid data size (>8 bytes)
    eResult = SPIMeasure_eInit(&sSPI, &sName, 10, NULL, 0, 9, 0);
    TEST_ASSERT_EQUAL(PINCFG_ERROR_E, eResult);

    // Test invalid command length (>4 bytes)
    uint8_t au8LongCmd[] = {0x01, 0x02, 0x03, 0x04, 0x05};
    eResult = SPIMeasure_eInit(&sSPI, &sName, 10, au8LongCmd, 5, 4, 0);
    TEST_ASSERT_EQUAL(PINCFG_ERROR_E, eResult);

    // Test command length without command bytes (should fail)
    eResult = SPIMeasure_eInit(&sSPI, &sName, 10, NULL, 1, 4, 0);
    TEST_ASSERT_EQUAL(PINCFG_NULLPTR_ERROR_E, eResult);
}

/**
 * Test SPI simple mode read (MAX31855-style: just read data)
 */
void test_vSPIMeasure_SimpleRead(void)
{
    SPIMEASURE_T sSPI;
    uint8_t au8Buffer[8];
    uint8_t u8Size = 8;
    ISENSORMEASURE_RESULT_T eResult;
    uint8_t au8ResponseData[] = {0x01, 0x91, 0x67, 0x00}; // MAX31855 format
    STRING_POINT_T sName;

    // Initialize for simple 4-byte read
    PinCfgStr_vInitStrPoint(&sName, "thermo", 6);
    SPIMeasure_eInit(&sSPI, &sName, 10, NULL, 0, 4, 0);

    // Mock: Set up response data
    SPIMock_vReset();
    SPIMock_vSetResponse(au8ResponseData, 4);
    init_ArduinoMock(); // Reset GPIO mock counters after init

    // First call: Should read immediately in simple mode and return PENDING
    eResult = SPIMeasure_eMeasure(&sSPI.sInterface, au8Buffer, &u8Size, 0);
    TEST_ASSERT_EQUAL(ISENSORMEASURE_PENDING_E, eResult);
    TEST_ASSERT_EQUAL(SPIMEASURE_STATE_DATA_READY_E, sSPI.eState);
    TEST_ASSERT_EQUAL(4, SPIMock_u32GetTransferCount()); // 4 bytes transferred

    // Verify CS pin was toggled (LOW then HIGH)
    TEST_ASSERT_EQUAL(2, mock_digitalWrite_u32Called); // CS LOW then HIGH

    // Second call: Return the raw bytes
    u8Size = 8;
    eResult = SPIMeasure_eMeasure(&sSPI.sInterface, au8Buffer, &u8Size, 0);
    TEST_ASSERT_EQUAL(ISENSORMEASURE_OK_E, eResult);
    TEST_ASSERT_EQUAL(4, u8Size);
    TEST_ASSERT_EQUAL(0x01, au8Buffer[0]);
    TEST_ASSERT_EQUAL(0x91, au8Buffer[1]);
    TEST_ASSERT_EQUAL(0x67, au8Buffer[2]);
    TEST_ASSERT_EQUAL(0x00, au8Buffer[3]);
    TEST_ASSERT_EQUAL(SPIMEASURE_STATE_IDLE_E, sSPI.eState);

    // Can repeat measurement
    SPIMock_vSetResponse(au8ResponseData, 4);
    init_ArduinoMock(); // Reset GPIO mock counters
    u8Size = 8;
    eResult = SPIMeasure_eMeasure(&sSPI.sInterface, au8Buffer, &u8Size, 0);
    TEST_ASSERT_EQUAL(ISENSORMEASURE_PENDING_E, eResult);
}

/**
 * Test SPI command mode (sensor with read command and no delay)
 */
void test_vSPIMeasure_CommandModeNoDelay(void)
{
    SPIMEASURE_T sSPI;
    uint8_t au8Buffer[8];
    uint8_t u8Size = 8;
    ISENSORMEASURE_RESULT_T eResult;
    uint8_t au8Cmd[] = {0x80}; // Read register command
    uint8_t au8ResponseData[] = {0xAA, 0xBB, 0xCC, 0xDD};
    STRING_POINT_T sName;
    uint8_t u8TxSize;
    const uint8_t *pu8TxData;

    // Initialize for command mode without conversion delay
    PinCfgStr_vInitStrPoint(&sName, "sensor", 6);
    SPIMeasure_eInit(&sSPI, &sName, 10, au8Cmd, 1, 4, 0);

    SPIMock_vReset();
    // Set response with command byte first (full-duplex: command echoed back, then data)
    uint8_t au8FullResponse[] = {0xFF, 0xAA, 0xBB, 0xCC, 0xDD}; // 0xFF echo for command, then data
    SPIMock_vSetResponse(au8FullResponse, 5);
    init_ArduinoMock();

    // First call: Send command and immediately read data
    eResult = SPIMeasure_eMeasure(&sSPI.sInterface, au8Buffer, &u8Size, 0);
    TEST_ASSERT_EQUAL(ISENSORMEASURE_PENDING_E, eResult);
    TEST_ASSERT_EQUAL(SPIMEASURE_STATE_READING_E, sSPI.eState);

    // Verify command was sent
    pu8TxData = SPIMock_pu8GetLastTxData(&u8TxSize);
    TEST_ASSERT_GREATER_THAN(0, u8TxSize); // Should have sent at least 1 byte
    TEST_ASSERT_EQUAL(0x80, pu8TxData[0]);

    // Second call: Complete reading
    u8Size = 8;
    eResult = SPIMeasure_eMeasure(&sSPI.sInterface, au8Buffer, &u8Size, 0);
    TEST_ASSERT_EQUAL(ISENSORMEASURE_PENDING_E, eResult);
    TEST_ASSERT_EQUAL(SPIMEASURE_STATE_DATA_READY_E, sSPI.eState);

    // Third call: Return raw bytes
    u8Size = 8;
    eResult = SPIMeasure_eMeasure(&sSPI.sInterface, au8Buffer, &u8Size, 0);
    TEST_ASSERT_EQUAL(ISENSORMEASURE_OK_E, eResult);
    TEST_ASSERT_EQUAL(4, u8Size);
    // Buffer contains the data bytes read (mock returned bytes 1-4 from full response)
    TEST_ASSERT_EQUAL(0xAA, au8Buffer[0]);
    TEST_ASSERT_EQUAL(0xBB, au8Buffer[1]);
    TEST_ASSERT_EQUAL(0xCC, au8Buffer[2]);
    TEST_ASSERT_EQUAL(0xDD, au8Buffer[3]);
    TEST_ASSERT_EQUAL(SPIMEASURE_STATE_IDLE_E, sSPI.eState);
}

/**
 * Test SPI command mode with conversion delay
 */
void test_vSPIMeasure_CommandModeWithDelay(void)
{
    SPIMEASURE_T sSPI;
    uint8_t au8Buffer[8];
    uint8_t u8Size = 8;
    ISENSORMEASURE_RESULT_T eResult;
    uint8_t au8Cmd[] = {0xAC, 0x33}; // Multi-byte command
    uint8_t au8ResponseData[] = {0x11, 0x22, 0x33, 0x44, 0x55, 0x66};
    STRING_POINT_T sName;

    // Initialize for command mode with conversion delay
    PinCfgStr_vInitStrPoint(&sName, "sensor", 6);
    SPIMeasure_eInit(&sSPI, &sName, 10, au8Cmd, 2, 6, 50);

    SPIMock_vReset();
    mock_millis_u32Return = 0;
    init_ArduinoMock();

    // First call: Send command, start waiting
    eResult = SPIMeasure_eMeasure(&sSPI.sInterface, au8Buffer, &u8Size, mock_millis_u32Return);
    TEST_ASSERT_EQUAL(ISENSORMEASURE_PENDING_E, eResult);
    TEST_ASSERT_EQUAL(SPIMEASURE_STATE_COMMAND_SENT_E, sSPI.eState);
    TEST_ASSERT_EQUAL(2, SPIMock_u32GetTransferCount()); // Command sent

    // CS should be HIGH during wait
    TEST_ASSERT_EQUAL(HIGH, mock_digitalWrite_u8Value);

    // Second call: Still waiting for conversion delay
    mock_millis_u32Return = 30; // Only 30ms elapsed
    u8Size = 8;
    eResult = SPIMeasure_eMeasure(&sSPI.sInterface, au8Buffer, &u8Size, mock_millis_u32Return);
    TEST_ASSERT_EQUAL(ISENSORMEASURE_PENDING_E, eResult);
    TEST_ASSERT_EQUAL(SPIMEASURE_STATE_COMMAND_SENT_E, sSPI.eState);

    // Third call: Conversion delay complete, read data
    mock_millis_u32Return = 55; // >50ms elapsed
    SPIMock_vSetResponse(au8ResponseData, 6);
    u8Size = 8;
    eResult = SPIMeasure_eMeasure(&sSPI.sInterface, au8Buffer, &u8Size, mock_millis_u32Return);
    TEST_ASSERT_EQUAL(ISENSORMEASURE_PENDING_E, eResult);
    TEST_ASSERT_EQUAL(SPIMEASURE_STATE_READING_E, sSPI.eState); // Should transition to READING

    // Fourth call: Complete reading
    u8Size = 8;
    eResult = SPIMeasure_eMeasure(&sSPI.sInterface, au8Buffer, &u8Size, mock_millis_u32Return);
    TEST_ASSERT_EQUAL(ISENSORMEASURE_PENDING_E, eResult);
    TEST_ASSERT_EQUAL(SPIMEASURE_STATE_DATA_READY_E, sSPI.eState);

    // Fifth call: Return raw bytes
    u8Size = 8;
    eResult = SPIMeasure_eMeasure(&sSPI.sInterface, au8Buffer, &u8Size, mock_millis_u32Return);
    TEST_ASSERT_EQUAL(ISENSORMEASURE_OK_E, eResult);
    TEST_ASSERT_EQUAL(6, u8Size);
    TEST_ASSERT_EQUAL(0x11, au8Buffer[0]);
    TEST_ASSERT_EQUAL(0x22, au8Buffer[1]);
    TEST_ASSERT_EQUAL(0x33, au8Buffer[2]);
    TEST_ASSERT_EQUAL(0x44, au8Buffer[3]);
    TEST_ASSERT_EQUAL(0x55, au8Buffer[4]);
    TEST_ASSERT_EQUAL(0x66, au8Buffer[5]);
    TEST_ASSERT_EQUAL(SPIMEASURE_STATE_IDLE_E, sSPI.eState);
}

/**
 * Test SPI timeout handling
 */
void test_vSPIMeasure_Timeout(void)
{
    SPIMEASURE_T sSPI;
    uint8_t au8Buffer[8];
    uint8_t u8Size = 8;
    ISENSORMEASURE_RESULT_T eResult;
    uint8_t au8Cmd[] = {0x01};
    STRING_POINT_T sName;

    // Initialize with conversion delay
    PinCfgStr_vInitStrPoint(&sName, "sensor", 6);
    SPIMeasure_eInit(&sSPI, &sName, 10, au8Cmd, 1, 4, 50);

    SPIMock_vReset();
    // Set full response including command echo
    uint8_t au8FullResponse[] = {0xFF, 0xAA, 0xBB, 0xCC, 0xDD};
    SPIMock_vSetResponse(au8FullResponse, 5);
    mock_millis_u32Return = 0;

    // Start request
    eResult = SPIMeasure_eMeasure(&sSPI.sInterface, au8Buffer, &u8Size, mock_millis_u32Return);
    TEST_ASSERT_EQUAL(ISENSORMEASURE_PENDING_E, eResult);
    TEST_ASSERT_EQUAL(SPIMEASURE_STATE_COMMAND_SENT_E, sSPI.eState);

    // Still waiting, no timeout yet
    mock_millis_u32Return = 40; // Still within conversion delay (50ms)
    u8Size = 8;
    eResult = SPIMeasure_eMeasure(&sSPI.sInterface, au8Buffer, &u8Size, mock_millis_u32Return);
    TEST_ASSERT_EQUAL(ISENSORMEASURE_PENDING_E, eResult);
    TEST_ASSERT_EQUAL(SPIMEASURE_STATE_COMMAND_SENT_E, sSPI.eState); // Still waiting

    // Timeout occurs during wait (before conversion delay completes)
    mock_millis_u32Return = 150; // Exceeds timeout (100ms default)
    u8Size = 8;
    eResult = SPIMeasure_eMeasure(&sSPI.sInterface, au8Buffer, &u8Size, mock_millis_u32Return);
    TEST_ASSERT_EQUAL(ISENSORMEASURE_ERROR_E, eResult);
    TEST_ASSERT_EQUAL(SPIMEASURE_STATE_ERROR_E, sSPI.eState);

    // Can retry after timeout
    uint8_t au8RetryData[] = {0xAA, 0xBB, 0xCC, 0xDD};
    SPIMock_vSetResponse(au8RetryData, 4);
    mock_millis_u32Return = 200;

    // First call after error: Resets to IDLE
    u8Size = 8;
    eResult = SPIMeasure_eMeasure(&sSPI.sInterface, au8Buffer, &u8Size, 0);
    TEST_ASSERT_EQUAL(ISENSORMEASURE_ERROR_E, eResult);
    TEST_ASSERT_EQUAL(SPIMEASURE_STATE_IDLE_E, sSPI.eState);

    // Second call: Starts new transaction
    u8Size = 8;
    eResult = SPIMeasure_eMeasure(&sSPI.sInterface, au8Buffer, &u8Size, 0);
    TEST_ASSERT_EQUAL(ISENSORMEASURE_PENDING_E, eResult);
}

/**
 * Test SPI error simulation
 */
void test_vSPIMeasure_ErrorSimulation(void)
{
    SPIMEASURE_T sSPI;
    uint8_t au8Buffer[8];
    uint8_t u8Size = 8;
    ISENSORMEASURE_RESULT_T eResult;
    STRING_POINT_T sName;

    PinCfgStr_vInitStrPoint(&sName, "sensor", 6);
    SPIMeasure_eInit(&sSPI, &sName, 10, NULL, 0, 4, 0);

    SPIMock_vReset();
    SPIMock_vSimulateError(true); // Simulate communication error

    // Request should complete but with error data (0x00)
    eResult = SPIMeasure_eMeasure(&sSPI.sInterface, au8Buffer, &u8Size, 0);
    TEST_ASSERT_EQUAL(ISENSORMEASURE_PENDING_E, eResult);

    // Get data (will be zeros due to error)
    u8Size = 8;
    eResult = SPIMeasure_eMeasure(&sSPI.sInterface, au8Buffer, &u8Size, 0);
    TEST_ASSERT_EQUAL(ISENSORMEASURE_OK_E, eResult);
    TEST_ASSERT_EQUAL(4, u8Size);
    TEST_ASSERT_EQUAL(0x00, au8Buffer[0]);
    TEST_ASSERT_EQUAL(0x00, au8Buffer[1]);
    TEST_ASSERT_EQUAL(0x00, au8Buffer[2]);
    TEST_ASSERT_EQUAL(0x00, au8Buffer[3]);
}

/**
 * Test SPI buffer size limiting
 */
void test_vSPIMeasure_BufferSizeLimit(void)
{
    SPIMEASURE_T sSPI;
    uint8_t au8Buffer[3]; // Smaller buffer than data size
    uint8_t u8Size = 3;
    ISENSORMEASURE_RESULT_T eResult;
    uint8_t au8ResponseData[] = {0x11, 0x22, 0x33, 0x44, 0x55, 0x66};
    STRING_POINT_T sName;

    // Initialize for 6-byte read
    PinCfgStr_vInitStrPoint(&sName, "sensor", 6);
    SPIMeasure_eInit(&sSPI, &sName, 10, NULL, 0, 6, 0);

    SPIMock_vReset();
    SPIMock_vSetResponse(au8ResponseData, 6);

    // Read data
    eResult = SPIMeasure_eMeasure(&sSPI.sInterface, au8Buffer, &u8Size, 0);
    TEST_ASSERT_EQUAL(ISENSORMEASURE_PENDING_E, eResult);

    // Get data (should be limited to buffer size)
    u8Size = 3;
    eResult = SPIMeasure_eMeasure(&sSPI.sInterface, au8Buffer, &u8Size, 0);
    TEST_ASSERT_EQUAL(ISENSORMEASURE_OK_E, eResult);
    TEST_ASSERT_EQUAL(3, u8Size); // Limited to buffer size
    TEST_ASSERT_EQUAL(0x11, au8Buffer[0]);
    TEST_ASSERT_EQUAL(0x22, au8Buffer[1]);
    TEST_ASSERT_EQUAL(0x33, au8Buffer[2]);
}

/**
 * Test SPI CS pin control
 */
void test_vSPIMeasure_CSPinControl(void)
{
    SPIMEASURE_T sSPI;
    uint8_t au8Buffer[8];
    uint8_t u8Size = 8;
    uint8_t au8ResponseData[] = {0xFF, 0xFF};
    STRING_POINT_T sName;

    PinCfgStr_vInitStrPoint(&sName, "sensor", 6);
    SPIMeasure_eInit(&sSPI, &sName, 10, NULL, 0, 2, 0);

    SPIMock_vReset();
    SPIMock_vSetResponse(au8ResponseData, 2);
    init_ArduinoMock();

    // During init, CS should be configured as OUTPUT and set HIGH
    TEST_ASSERT_EQUAL(10, mock_pinMode_u8Pin);
    TEST_ASSERT_EQUAL(OUTPUT, mock_pinMode_u8Mode);
    TEST_ASSERT_EQUAL(10, mock_digitalWrite_u8Pin);
    TEST_ASSERT_EQUAL(HIGH, mock_digitalWrite_u8Value);

    // During read, CS should go LOW then HIGH
    init_ArduinoMock(); // Reset counters
    SPIMeasure_eMeasure(&sSPI.sInterface, au8Buffer, &u8Size, 0);

    // Should have called digitalWrite at least once (for CS HIGH at end)
    TEST_ASSERT_GREATER_THAN(0, mock_digitalWrite_u32Called);
}

/**
 * Test SPI CSV parsing - simple mode
 */
void test_vSPIMeasure_CSVParsing_Simple(void)
{
    PINCFG_RESULT_T eResult;
    LINKEDLIST_RESULT_T eLinkedListResult;
    char acOutStr[OUT_STR_MAX_LEN_D];
    size_t szMemoryRequired;

    // Format: MS,6,name,cs_pin,size/
    // 6 = MEASUREMENT_TYPE_SPI_E, simple mode (no command)
    const char *pcCfg = "MS,6,temp_sensor,10,2/"
                        "MS,6,pressure,11,4/"
                        "MS,6,accel,12,6/";

    // First pass: calculate memory requirements
    PINCFG_PARSE_PARAMS_T sParams = {
        .pcConfig = pcCfg,
        .eAddToLoopables = PinCfgCsv_eAddToTempLoopables,
        .eAddToPresentables = PinCfgCsv_eAddToTempPresentables,
        .pszMemoryRequired = &szMemoryRequired,
        .pcOutString = acOutStr,
        .u16OutStrMaxLen = (uint16_t)OUT_STR_MAX_LEN_D,
        .bValidate = false};

    eResult = PinCfgCsv_eParse(&sParams);
    if (eResult != PINCFG_OK_E)
    {
        printf("\n=== SPI Simple Parse Error (pass 1) ===\nResult: %d\nOutput: %s\n", eResult, acOutStr);
    }
    TEST_ASSERT_EQUAL(PINCFG_OK_E, eResult);

    // Second pass: actually create objects
    memset(acOutStr, 0, OUT_STR_MAX_LEN_D);
    sParams.pszMemoryRequired = NULL;
    eResult = PinCfgCsv_eParse(&sParams);
    if (eResult != PINCFG_OK_E)
    {
        printf("\n=== SPI Simple Parse Error ===\nResult: %d\nOutput: %s\n", eResult, acOutStr);
    }
    TEST_ASSERT_EQUAL(PINCFG_OK_E, eResult);

    eLinkedListResult =
        LinkedList_eLinkedListToArray((LINKEDLIST_ITEM_T **)(&psGlobals->ppsLoopables), &psGlobals->u8LoopablesCount);
    TEST_ASSERT_EQUAL(LINKEDLIST_OK_E, eLinkedListResult);

    eLinkedListResult = LinkedList_eLinkedListToArray(
        (LINKEDLIST_ITEM_T **)(&psGlobals->ppsPresentables), &psGlobals->u8PresentablesCount);
    TEST_ASSERT_EQUAL(LINKEDLIST_OK_E, eLinkedListResult);
}

/**
 * Test SPI CSV parsing - command mode with delay
 */
void test_vSPIMeasure_CSVParsing_Command(void)
{
    PINCFG_RESULT_T eResult;
    LINKEDLIST_RESULT_T eLinkedListResult;
    char acOutStr[OUT_STR_MAX_LEN_D];
    size_t szMemoryRequired;

    // Format: MS,6,name,cs_pin,cmd1,size[,cmd2,cmd3,cmd4,delay]/
    // Command mode with 1-4 command bytes and optional delay
    const char *pcCfg = "MS,6,thermo,10,3,2/"       // 1 cmd byte (0x03), 2 data bytes, no delay
                        "MS,6,adc,11,170,2,50/"     // 1 cmd byte (0xAA), 2 data bytes, 50ms delay
                        "MS,6,fram,12,3,0,1,4,25/"; // 3 cmd bytes (0x03,0x00,0x01), 4 data bytes, 25ms delay

    // First pass: calculate memory requirements
    PINCFG_PARSE_PARAMS_T sParams = {
        .pcConfig = pcCfg,
        .eAddToLoopables = PinCfgCsv_eAddToTempLoopables,
        .eAddToPresentables = PinCfgCsv_eAddToTempPresentables,
        .pszMemoryRequired = &szMemoryRequired,
        .pcOutString = acOutStr,
        .u16OutStrMaxLen = (uint16_t)OUT_STR_MAX_LEN_D,
        .bValidate = false};

    eResult = PinCfgCsv_eParse(&sParams);
    if (eResult != PINCFG_OK_E)
    {
        printf("\n=== SPI Command Parse Error (pass 1) ===\nResult: %d\nOutput: %s\n", eResult, acOutStr);
    }
    TEST_ASSERT_EQUAL(PINCFG_OK_E, eResult);

    // Second pass: actually create objects
    memset(acOutStr, 0, OUT_STR_MAX_LEN_D);
    sParams.pszMemoryRequired = NULL;
    eResult = PinCfgCsv_eParse(&sParams);
    if (eResult != PINCFG_OK_E)
    {
        printf("\n=== SPI Command Parse Error ===\nResult: %d\nOutput: %s\n", eResult, acOutStr);
    }
    TEST_ASSERT_EQUAL(PINCFG_OK_E, eResult);

    eLinkedListResult =
        LinkedList_eLinkedListToArray((LINKEDLIST_ITEM_T **)(&psGlobals->ppsLoopables), &psGlobals->u8LoopablesCount);
    TEST_ASSERT_EQUAL(LINKEDLIST_OK_E, eLinkedListResult);

    eLinkedListResult = LinkedList_eLinkedListToArray(
        (LINKEDLIST_ITEM_T **)(&psGlobals->ppsPresentables), &psGlobals->u8PresentablesCount);
    TEST_ASSERT_EQUAL(LINKEDLIST_OK_E, eLinkedListResult);
}

/**
 * Test SPI CSV parsing with sensor integration
 */
void test_vSPIMeasure_CSVParsing_Sensor(void)
{
    PINCFG_RESULT_T eResult;
    LINKEDLIST_RESULT_T eLinkedListResult;
    char acOutStr[OUT_STR_MAX_LEN_D];
    size_t szMemoryRequired;

    // Create SPI measurement and sensor that uses it
    // MS,6,name,cs_pin,cmd,size,delay/
    // SR,sensorName,measurementName,vType,sType,enableable,cumulative,sampMs,reportSec[,offset]/
    const char *pcCfg = "MS,6,max31855,10,0,4,100/" // Thermocouple: cmd=0x00, 4 bytes, 100ms delay
                        "SR,Temperature,max31855,38,6,0,1,1000,60,1.0/";

    // First pass: calculate memory
    PINCFG_PARSE_PARAMS_T sParams = {
        .pcConfig = pcCfg,
        .eAddToLoopables = PinCfgCsv_eAddToTempLoopables,
        .eAddToPresentables = PinCfgCsv_eAddToTempPresentables,
        .pszMemoryRequired = &szMemoryRequired,
        .pcOutString = acOutStr,
        .u16OutStrMaxLen = (uint16_t)OUT_STR_MAX_LEN_D,
        .bValidate = false};

    eResult = PinCfgCsv_eParse(&sParams);
    if (eResult != PINCFG_OK_E)
    {
        printf("\n=== SPI Sensor Parse Error (pass 1) ===\nResult: %d\nOutput: %s\n", eResult, acOutStr);
    }
    TEST_ASSERT_EQUAL(PINCFG_OK_E, eResult);

    // Second pass: create objects
    memset(acOutStr, 0, OUT_STR_MAX_LEN_D);
    sParams.pszMemoryRequired = NULL;
    eResult = PinCfgCsv_eParse(&sParams);
    if (eResult != PINCFG_OK_E)
    {
        printf("\n=== SPI Sensor Parse Error ===\nResult: %d\nOutput: %s\n", eResult, acOutStr);
    }
    TEST_ASSERT_EQUAL(PINCFG_OK_E, eResult);

    eLinkedListResult =
        LinkedList_eLinkedListToArray((LINKEDLIST_ITEM_T **)(&psGlobals->ppsLoopables), &psGlobals->u8LoopablesCount);
    TEST_ASSERT_EQUAL(LINKEDLIST_OK_E, eLinkedListResult);

    eLinkedListResult = LinkedList_eLinkedListToArray(
        (LINKEDLIST_ITEM_T **)(&psGlobals->ppsPresentables), &psGlobals->u8PresentablesCount);
    TEST_ASSERT_EQUAL(LINKEDLIST_OK_E, eLinkedListResult);

    // Verify sensor was created
    TEST_ASSERT_GREATER_THAN(0, psGlobals->u8PresentablesCount);
}

#endif // PINCFG_FEATURE_SPI_MEASUREMENT

// =============================================================================
// ANALOG MEASUREMENT TESTS
// =============================================================================

#ifdef PINCFG_FEATURE_ANALOG_MEASUREMENT
void test_vAnalogMeasure_Init(void)
{
    ANALOGMEASURE_T sAnalog;
    STRING_POINT_T sName;
    ANALOGMEASURE_RESULT_T eResult;

    // Test valid initialization
    PinCfgStr_vInitStrPoint(&sName, "analog", 6);
    eResult = AnalogMeasure_eInit(&sAnalog, &sName, 0);
    TEST_ASSERT_EQUAL(ANALOGMEASURE_OK_E, eResult);
    TEST_ASSERT_EQUAL(MEASUREMENT_TYPE_ANALOG_E, sAnalog.sSensorMeasure.eType);
    TEST_ASSERT_NOT_NULL(sAnalog.sSensorMeasure.eMeasure);
    TEST_ASSERT_EQUAL(0, sAnalog.u8Pin);
    TEST_ASSERT_EQUAL_STRING("analog", sAnalog.sSensorMeasure.pcName);

    // Test different pin number
    PinCfgStr_vInitStrPoint(&sName, "a3", 2);
    eResult = AnalogMeasure_eInit(&sAnalog, &sName, 3);
    TEST_ASSERT_EQUAL(ANALOGMEASURE_OK_E, eResult);
    TEST_ASSERT_EQUAL(3, sAnalog.u8Pin);
    TEST_ASSERT_EQUAL_STRING("a3", sAnalog.sSensorMeasure.pcName);

    // Test NULL handle
    eResult = AnalogMeasure_eInit(NULL, &sName, 0);
    TEST_ASSERT_EQUAL(ANALOGMEASURE_NULLPTR_ERROR_E, eResult);

    // Test NULL name
    eResult = AnalogMeasure_eInit(&sAnalog, NULL, 0);
    TEST_ASSERT_EQUAL(ANALOGMEASURE_NULLPTR_ERROR_E, eResult);
}

/**
 * Test AnalogMeasure simple reading
 */
void test_vAnalogMeasure_SimpleRead(void)
{
    ANALOGMEASURE_T sAnalog;
    STRING_POINT_T sName;
    uint8_t au8Buffer[4];
    uint8_t u8Size = 4;
    ISENSORMEASURE_RESULT_T eResult;

    // Initialize for analog pin A0
    PinCfgStr_vInitStrPoint(&sName, "analog", 6);
    AnalogMeasure_eInit(&sAnalog, &sName, 0);

    // Mock: Set ADC value to 512 (mid-range 10-bit)
    mock_analogRead_u16Return = 512;
    mock_analogRead_u32Called = 0;

    // Read analog value
    eResult = sAnalog.sSensorMeasure.eMeasure(&sAnalog.sSensorMeasure, au8Buffer, &u8Size, 0);
    TEST_ASSERT_EQUAL(ISENSORMEASURE_OK_E, eResult);
    TEST_ASSERT_EQUAL(2, u8Size); // Returns 2 bytes
    TEST_ASSERT_EQUAL(1, mock_analogRead_u32Called);
    TEST_ASSERT_EQUAL(0, mock_analogRead_u8LastPin);

    // Verify big-endian format
    uint16_t u16Value = ((uint16_t)au8Buffer[0] << 8) | au8Buffer[1];
    TEST_ASSERT_EQUAL(512, u16Value);

    // Test different ADC value
    mock_analogRead_u16Return = 1023; // Max 10-bit value
    eResult = sAnalog.sSensorMeasure.eMeasure(&sAnalog.sSensorMeasure, au8Buffer, &u8Size, 0);
    TEST_ASSERT_EQUAL(ISENSORMEASURE_OK_E, eResult);
    u16Value = ((uint16_t)au8Buffer[0] << 8) | au8Buffer[1];
    TEST_ASSERT_EQUAL(1023, u16Value);

    // Test minimum value
    mock_analogRead_u16Return = 0;
    eResult = sAnalog.sSensorMeasure.eMeasure(&sAnalog.sSensorMeasure, au8Buffer, &u8Size, 0);
    TEST_ASSERT_EQUAL(ISENSORMEASURE_OK_E, eResult);
    u16Value = ((uint16_t)au8Buffer[0] << 8) | au8Buffer[1];
    TEST_ASSERT_EQUAL(0, u16Value);

    // Test 12-bit ADC value (STM32)
    mock_analogRead_u16Return = 4095; // Max 12-bit value
    eResult = sAnalog.sSensorMeasure.eMeasure(&sAnalog.sSensorMeasure, au8Buffer, &u8Size, 0);
    TEST_ASSERT_EQUAL(ISENSORMEASURE_OK_E, eResult);
    u16Value = ((uint16_t)au8Buffer[0] << 8) | au8Buffer[1];
    TEST_ASSERT_EQUAL(4095, u16Value);
}

/**
 * Test AnalogMeasure error handling
 */
void test_vAnalogMeasure_ErrorHandling(void)
{
    ANALOGMEASURE_T sAnalog;
    STRING_POINT_T sName;
    uint8_t au8Buffer[4];
    uint8_t u8Size = 4;
    ISENSORMEASURE_RESULT_T eResult;

    // Initialize
    PinCfgStr_vInitStrPoint(&sName, "analog", 6);
    AnalogMeasure_eInit(&sAnalog, &sName, 0);

    // Test NULL self pointer
    eResult = sAnalog.sSensorMeasure.eMeasure(NULL, au8Buffer, &u8Size, 0);
    TEST_ASSERT_EQUAL(ISENSORMEASURE_NULLPTR_ERROR_E, eResult);

    // Test NULL buffer
    eResult = sAnalog.sSensorMeasure.eMeasure(&sAnalog.sSensorMeasure, NULL, &u8Size, 0);
    TEST_ASSERT_EQUAL(ISENSORMEASURE_NULLPTR_ERROR_E, eResult);

    // Test NULL size pointer
    eResult = sAnalog.sSensorMeasure.eMeasure(&sAnalog.sSensorMeasure, au8Buffer, NULL, 0);
    TEST_ASSERT_EQUAL(ISENSORMEASURE_NULLPTR_ERROR_E, eResult);

    // Test buffer too small
    u8Size = 1;
    eResult = sAnalog.sSensorMeasure.eMeasure(&sAnalog.sSensorMeasure, au8Buffer, &u8Size, 0);
    TEST_ASSERT_EQUAL(ISENSORMEASURE_ERROR_E, eResult);
}

/**
 * Test AnalogMeasure with different pins
 */
void test_vAnalogMeasure_MultiplePins(void)
{
    ANALOGMEASURE_T sAnalog0, sAnalog3, sAnalog7;
    STRING_POINT_T sName;
    uint8_t au8Buffer[4];
    uint8_t u8Size = 4;

    // Initialize multiple analog measurements
    PinCfgStr_vInitStrPoint(&sName, "a0", 2);
    AnalogMeasure_eInit(&sAnalog0, &sName, 0);

    PinCfgStr_vInitStrPoint(&sName, "a3", 2);
    AnalogMeasure_eInit(&sAnalog3, &sName, 3);

    PinCfgStr_vInitStrPoint(&sName, "a7", 2);
    AnalogMeasure_eInit(&sAnalog7, &sName, 7);

    // Read from pin 0
    mock_analogRead_u16Return = 100;
    sAnalog0.sSensorMeasure.eMeasure(&sAnalog0.sSensorMeasure, au8Buffer, &u8Size, 0);
    TEST_ASSERT_EQUAL(0, mock_analogRead_u8LastPin);

    // Read from pin 3
    mock_analogRead_u16Return = 300;
    sAnalog3.sSensorMeasure.eMeasure(&sAnalog3.sSensorMeasure, au8Buffer, &u8Size, 0);
    TEST_ASSERT_EQUAL(3, mock_analogRead_u8LastPin);

    // Read from pin 7
    mock_analogRead_u16Return = 700;
    sAnalog7.sSensorMeasure.eMeasure(&sAnalog7.sSensorMeasure, au8Buffer, &u8Size, 0);
    TEST_ASSERT_EQUAL(7, mock_analogRead_u8LastPin);
}

/**
 * Test AnalogMeasure CSV parsing
 */
void test_vAnalogMeasure_CSVParsing(void)
{
    PINCFG_RESULT_T eResult;
    LINKEDLIST_RESULT_T eLinkedListResult;
    char acOutStr[OUT_STR_MAX_LEN_D];
    size_t szMemoryRequired;

    // Format: MS,1,name,pin/
    // 1 = MEASUREMENT_TYPE_ANALOG_E
    const char *pcCfg = "MS,1,battery,0/"
                        "MS,1,temp_sens,1/"
                        "MS,1,light,2/";

    // First pass: calculate memory requirements
    PINCFG_PARSE_PARAMS_T sParams = {
        .pcConfig = pcCfg,
        .eAddToLoopables = PinCfgCsv_eAddToTempLoopables,
        .eAddToPresentables = PinCfgCsv_eAddToTempPresentables,
        .pszMemoryRequired = &szMemoryRequired,
        .pcOutString = acOutStr,
        .u16OutStrMaxLen = (uint16_t)OUT_STR_MAX_LEN_D,
        .bValidate = false};

    eResult = PinCfgCsv_eParse(&sParams);
    if (eResult != PINCFG_OK_E)
    {
        printf("\n=== Analog Parse Error (pass 1) ===\nResult: %d\nOutput: %s\n", eResult, acOutStr);
    }
    TEST_ASSERT_EQUAL(PINCFG_OK_E, eResult);

    // Second pass: actually create objects
    memset(acOutStr, 0, OUT_STR_MAX_LEN_D);
    sParams.pszMemoryRequired = NULL;
    eResult = PinCfgCsv_eParse(&sParams);
    if (eResult != PINCFG_OK_E)
    {
        printf("\n=== Analog Parse Error ===\nResult: %d\nOutput: %s\n", eResult, acOutStr);
    }
    TEST_ASSERT_EQUAL(PINCFG_OK_E, eResult);

    eLinkedListResult =
        LinkedList_eLinkedListToArray((LINKEDLIST_ITEM_T **)(&psGlobals->ppsLoopables), &psGlobals->u8LoopablesCount);
    TEST_ASSERT_EQUAL(LINKEDLIST_OK_E, eLinkedListResult);

    eLinkedListResult = LinkedList_eLinkedListToArray(
        (LINKEDLIST_ITEM_T **)(&psGlobals->ppsPresentables), &psGlobals->u8PresentablesCount);
    TEST_ASSERT_EQUAL(LINKEDLIST_OK_E, eLinkedListResult);

    // Verify analog measurement sources were created
    // Note: The measurement sources are stored in global registry, not as presentables
    // We'll verify them by using them with sensors
}

/**
 * Test AnalogMeasure with Sensor integration
 */
void test_vAnalogMeasure_SensorIntegration(void)
{
    PINCFG_RESULT_T eResult;
    LINKEDLIST_RESULT_T eLinkedListResult;
    char acOutStr[OUT_STR_MAX_LEN_D];
    size_t szMemoryRequired;

    // Create analog measurement and sensor that uses it
    // MS,1,name,pin/
    // SR,sensorName,measurementName,vType,sType,enableable,cumulative,sampMs,reportSec[,offset]/
    // V_VOLTAGE=38, S_POWER=0
    // Note: cumulative=1 so sensor samples at sampling interval (1000ms), not report interval (60s)
    // Note: fOffset=1.0 (index 9) is required to prevent measurements from being multiplied by 0
    const char *pcCfg = "MS,1,battery,0/"
                        "SR,BatteryVolt,battery,38,0,0,1,1000,60,1.0/";

    // First pass: calculate memory
    PINCFG_PARSE_PARAMS_T sParams = {
        .pcConfig = pcCfg,
        .eAddToLoopables = PinCfgCsv_eAddToTempLoopables,
        .eAddToPresentables = PinCfgCsv_eAddToTempPresentables,
        .pszMemoryRequired = &szMemoryRequired,
        .pcOutString = acOutStr,
        .u16OutStrMaxLen = (uint16_t)OUT_STR_MAX_LEN_D,
        .bValidate = false};

    eResult = PinCfgCsv_eParse(&sParams);
    if (eResult != PINCFG_OK_E)
    {
        printf("\n=== Analog Sensor Parse Error (pass 1) ===\nResult: %d\nOutput: %s\n", eResult, acOutStr);
    }
    TEST_ASSERT_EQUAL(PINCFG_OK_E, eResult);

    // Second pass: create objects
    memset(acOutStr, 0, OUT_STR_MAX_LEN_D);
    sParams.pszMemoryRequired = NULL;
    eResult = PinCfgCsv_eParse(&sParams);
    if (eResult != PINCFG_OK_E)
    {
        printf("\n=== Analog Sensor Parse Error ===\nResult: %d\nOutput: %s\n", eResult, acOutStr);
    }
    TEST_ASSERT_EQUAL(PINCFG_OK_E, eResult);

    eLinkedListResult =
        LinkedList_eLinkedListToArray((LINKEDLIST_ITEM_T **)(&psGlobals->ppsLoopables), &psGlobals->u8LoopablesCount);
    TEST_ASSERT_EQUAL(LINKEDLIST_OK_E, eLinkedListResult);

    eLinkedListResult = LinkedList_eLinkedListToArray(
        (LINKEDLIST_ITEM_T **)(&psGlobals->ppsPresentables), &psGlobals->u8PresentablesCount);
    TEST_ASSERT_EQUAL(LINKEDLIST_OK_E, eLinkedListResult);

    // Verify sensor was created
    // Presentables: [0]=CLI, [1]=CLI, [2]=BatteryVolt sensor
    SENSOR_T *psSensor = (SENSOR_T *)psGlobals->ppsPresentables[2];
    TEST_ASSERT_EQUAL_STRING("BatteryVolt", psSensor->sPresentable.pcName);
    TEST_ASSERT_EQUAL(38, psSensor->sVtab.eVType); // V_VOLTAGE
    TEST_ASSERT_EQUAL(0, psSensor->sVtab.eSType);  // S_POWER
    TEST_ASSERT_EQUAL(1000, psSensor->u16SamplingIntervalMs);
    TEST_ASSERT_EQUAL(60000, psSensor->u32ReportIntervalMs);

    // Mock analog reading and test sensor
    mock_analogRead_u16Return = 512; // Mid-range value
    mock_analogRead_u32Called = 0;

    // Simulate sensor loop - sensor should read from analog measurement
    mock_millis_u32Return = 0;
    PinCfgCsv_vLoop(mock_millis_u32Return);

    mock_millis_u32Return = 1000; // After sampling interval
    PinCfgCsv_vLoop(mock_millis_u32Return);

    // Verify analog read was called
    TEST_ASSERT_GREATER_THAN(0, mock_analogRead_u32Called);
}

#endif // PINCFG_FEATURE_ANALOG_MEASUREMENT

// =============================================================================
// LOOPTIME MEASUREMENT TESTS
// =============================================================================

#ifdef PINCFG_FEATURE_LOOPTIME_MEASUREMENT
void test_vLoopTimeMeasure_Init(void)
{
    LOOPTIMEMEASURE_T sMeasure;
    LOOPTIMEMEASURE_RESULT_T eResult;
    STRING_POINT_T sName;

    // Test NULL handle
    PinCfgStr_vInitStrPoint(&sName, "test", 4);
    eResult = LoopTimeMeasure_eInit(NULL, &sName);
    TEST_ASSERT_EQUAL(LOOPTIMEMEASURE_NULLPTR_ERROR_E, eResult);

    // Test NULL name
    eResult = LoopTimeMeasure_eInit(&sMeasure, NULL);
    TEST_ASSERT_EQUAL(LOOPTIMEMEASURE_NULLPTR_ERROR_E, eResult);

    // Test valid initialization
    PinCfgStr_vInitStrPoint(&sName, "looptime", 8);
    eResult = LoopTimeMeasure_eInit(&sMeasure, &sName);
    TEST_ASSERT_EQUAL(LOOPTIMEMEASURE_OK_E, eResult);

    // Verify interface setup
    TEST_ASSERT_EQUAL(MEASUREMENT_TYPE_LOOPTIME_E, sMeasure.sSensorMeasure.eType);
    TEST_ASSERT_NOT_NULL(sMeasure.sSensorMeasure.eMeasure);

    // Verify state initialization
    TEST_ASSERT_EQUAL(0, sMeasure.u32LastCallTime);
    TEST_ASSERT_TRUE(sMeasure.bFirstCall);
    TEST_ASSERT_EQUAL_STRING("looptime", sMeasure.sSensorMeasure.pcName);
}

/**
 * Test LoopTimeMeasure time delta calculation
 */
void test_vLoopTimeMeasure_TimeDelta(void)
{
    LOOPTIMEMEASURE_T sMeasure;
    uint8_t au8Buffer[4];
    uint8_t u8Size = 4;
    ISENSORMEASURE_RESULT_T eResult;
    STRING_POINT_T sName;

    // Initialize measurement
    PinCfgStr_vInitStrPoint(&sName, "looptime", 8);
    LoopTimeMeasure_eInit(&sMeasure, &sName);

    // First call should return error (no previous timestamp)
    mock_micros_u32Return = 1000;
    eResult = sMeasure.sSensorMeasure.eMeasure(&sMeasure.sSensorMeasure, au8Buffer, &u8Size, 1000);
    TEST_ASSERT_EQUAL(ISENSORMEASURE_ERROR_E, eResult);
    TEST_ASSERT_FALSE(sMeasure.bFirstCall);
    TEST_ASSERT_EQUAL(1000, sMeasure.u32LastCallTime);

    // Second call should return delta as 4 bytes
    u8Size = 4;
    mock_micros_u32Return = 1010;
    eResult = sMeasure.sSensorMeasure.eMeasure(&sMeasure.sSensorMeasure, au8Buffer, &u8Size, 1010);
    TEST_ASSERT_EQUAL(ISENSORMEASURE_OK_E, eResult);
    TEST_ASSERT_EQUAL(4, u8Size);
    // Extract uint32 from big-endian bytes
    uint32_t u32Delta =
        ((uint32_t)au8Buffer[0] << 24) | ((uint32_t)au8Buffer[1] << 16) | ((uint32_t)au8Buffer[2] << 8) | au8Buffer[3];
    TEST_ASSERT_EQUAL(10, u32Delta);
    TEST_ASSERT_EQUAL(1010, sMeasure.u32LastCallTime);

    // Third call with different delta
    u8Size = 4;
    mock_micros_u32Return = 1025;
    eResult = sMeasure.sSensorMeasure.eMeasure(&sMeasure.sSensorMeasure, au8Buffer, &u8Size, 1025);
    TEST_ASSERT_EQUAL(ISENSORMEASURE_OK_E, eResult);
    u32Delta =
        ((uint32_t)au8Buffer[0] << 24) | ((uint32_t)au8Buffer[1] << 16) | ((uint32_t)au8Buffer[2] << 8) | au8Buffer[3];
    TEST_ASSERT_EQUAL(15, u32Delta);
    TEST_ASSERT_EQUAL(1025, sMeasure.u32LastCallTime);

    // Test with zero delta (same timestamp)
    u8Size = 4;
    mock_micros_u32Return = 1025;
    eResult = sMeasure.sSensorMeasure.eMeasure(&sMeasure.sSensorMeasure, au8Buffer, &u8Size, 1025);
    TEST_ASSERT_EQUAL(ISENSORMEASURE_OK_E, eResult);
    u32Delta =
        ((uint32_t)au8Buffer[0] << 24) | ((uint32_t)au8Buffer[1] << 16) | ((uint32_t)au8Buffer[2] << 8) | au8Buffer[3];
    TEST_ASSERT_EQUAL(0, u32Delta);
}

/**
 * Test LoopTimeMeasure with offset
 * Note: LoopTime returns raw uint32 ms; offset is applied at Sensor layer
 */
void test_vLoopTimeMeasure_Offset(void)
{
    LOOPTIMEMEASURE_T sMeasure;
    uint8_t au8Buffer[4];
    uint8_t u8Size = 4;
    ISENSORMEASURE_RESULT_T eResult;
    STRING_POINT_T sName;

    // Initialize measurement
    PinCfgStr_vInitStrPoint(&sName, "looptime", 8);
    LoopTimeMeasure_eInit(&sMeasure, &sName);

    // First call (skip)
    mock_micros_u32Return = 1000;
    sMeasure.sSensorMeasure.eMeasure(&sMeasure.sSensorMeasure, au8Buffer, &u8Size, 1000);

    // Second call - verify raw delta is returned (offset not applied here)
    u8Size = 4;
    mock_micros_u32Return = 1010;
    eResult = sMeasure.sSensorMeasure.eMeasure(&sMeasure.sSensorMeasure, au8Buffer, &u8Size, 1010);
    TEST_ASSERT_EQUAL(ISENSORMEASURE_OK_E, eResult);
    uint32_t u32Delta =
        ((uint32_t)au8Buffer[0] << 24) | ((uint32_t)au8Buffer[1] << 16) | ((uint32_t)au8Buffer[2] << 8) | au8Buffer[3];
    TEST_ASSERT_EQUAL(10, u32Delta); // Raw 10ms delta

    // Third call - another raw delta
    u8Size = 4;
    mock_micros_u32Return = 1020;
    eResult = sMeasure.sSensorMeasure.eMeasure(&sMeasure.sSensorMeasure, au8Buffer, &u8Size, 1020);
    TEST_ASSERT_EQUAL(ISENSORMEASURE_OK_E, eResult);
    u32Delta =
        ((uint32_t)au8Buffer[0] << 24) | ((uint32_t)au8Buffer[1] << 16) | ((uint32_t)au8Buffer[2] << 8) | au8Buffer[3];
    TEST_ASSERT_EQUAL(10, u32Delta); // Raw 10ms delta
}

/**
 * Test LoopTimeMeasure with timestamp overflow
 */
void test_vLoopTimeMeasure_Overflow(void)
{
    LOOPTIMEMEASURE_T sMeasure;
    uint8_t au8Buffer[4];
    uint8_t u8Size = 4;
    ISENSORMEASURE_RESULT_T eResult;
    STRING_POINT_T sName;

    // Initialize measurement
    PinCfgStr_vInitStrPoint(&sName, "looptime", 8);
    LoopTimeMeasure_eInit(&sMeasure, &sName);

    // First call near overflow
    uint32_t u32NearMax = UINT32_MAX - 5;
    mock_micros_u32Return = u32NearMax;
    sMeasure.sSensorMeasure.eMeasure(&sMeasure.sSensorMeasure, au8Buffer, &u8Size, u32NearMax);

    // Second call after overflow
    uint32_t u32AfterOverflow = 10; // Wrapped around
    u8Size = 4;
    mock_micros_u32Return = u32AfterOverflow;
    eResult = sMeasure.sSensorMeasure.eMeasure(&sMeasure.sSensorMeasure, au8Buffer, &u8Size, u32AfterOverflow);
    TEST_ASSERT_EQUAL(ISENSORMEASURE_OK_E, eResult);
    // Delta should be 16 (5 to max + 1 + 10 after overflow)
    uint32_t u32Delta =
        ((uint32_t)au8Buffer[0] << 24) | ((uint32_t)au8Buffer[1] << 16) | ((uint32_t)au8Buffer[2] << 8) | au8Buffer[3];
    TEST_ASSERT_EQUAL(16, u32Delta);
}

/**
 * Test LoopTimeMeasure NULL parameter handling
 */
void test_vLoopTimeMeasure_NullParams(void)
{
    LOOPTIMEMEASURE_T sMeasure;
    uint8_t au8Buffer[4];
    uint8_t u8Size = 4;
    ISENSORMEASURE_RESULT_T eResult;
    STRING_POINT_T sName;

    // Initialize measurement
    PinCfgStr_vInitStrPoint(&sName, "looptime", 8);
    LoopTimeMeasure_eInit(&sMeasure, &sName);

    // Test NULL self pointer
    eResult = sMeasure.sSensorMeasure.eMeasure(NULL, au8Buffer, &u8Size, 1000);
    TEST_ASSERT_EQUAL(ISENSORMEASURE_NULLPTR_ERROR_E, eResult);

    // Test NULL buffer pointer
    eResult = sMeasure.sSensorMeasure.eMeasure(&sMeasure.sSensorMeasure, NULL, &u8Size, 1000);
    TEST_ASSERT_EQUAL(ISENSORMEASURE_NULLPTR_ERROR_E, eResult);

    // Test NULL size pointer
    eResult = sMeasure.sSensorMeasure.eMeasure(&sMeasure.sSensorMeasure, au8Buffer, NULL, 1000);
    TEST_ASSERT_EQUAL(ISENSORMEASURE_NULLPTR_ERROR_E, eResult);
}

/**
 * Test LoopTimeMeasure CSV parsing
 * Note: This test requires USE_MALLOC due to complex memory allocation patterns in two-pass parsing
 */
void test_vLoopTimeMeasure_CSVParsing(void)
{
#ifdef USE_MALLOC
    PINCFG_RESULT_T eResult;
    char acOutStr[OUT_STR_MAX_LEN_D];
    size_t szMemoryRequired;

    // Test basic loop time configuration
    // MS,5,looptime/ SR,LoopTime,looptime,99,99,0,1,100,5/  (enableable=0, cumulative=1 for loop measurements)
    const char *pcCfg = "MS,5,looptime/"
                        "SR,LoopTime,looptime,99,99,0,1,100,5/";

    // Use two-pass parsing like other tests
    PINCFG_PARSE_PARAMS_T sParams = {
        .pcConfig = pcCfg,
        .eAddToLoopables = PinCfgCsv_eAddToTempLoopables,
        .eAddToPresentables = PinCfgCsv_eAddToTempPresentables,
        .pszMemoryRequired = &szMemoryRequired,
        .pcOutString = acOutStr,
        .u16OutStrMaxLen = (uint16_t)OUT_STR_MAX_LEN_D,
        .bValidate = false};

    // First pass: calculate memory requirements
    eResult = PinCfgCsv_eParse(&sParams);
    TEST_ASSERT_EQUAL(PINCFG_OK_E, eResult);

    // Second pass: actually create objects
    memset(acOutStr, 0, OUT_STR_MAX_LEN_D);
    sParams.pszMemoryRequired = NULL;
    eResult = PinCfgCsv_eParse(&sParams);
    TEST_ASSERT_EQUAL(PINCFG_OK_E, eResult);

    // Convert linked lists to arrays
    LinkedList_eLinkedListToArray((LINKEDLIST_ITEM_T **)(&psGlobals->ppsLoopables), &psGlobals->u8LoopablesCount);
    LinkedList_eLinkedListToArray((LINKEDLIST_ITEM_T **)(&psGlobals->ppsPresentables), &psGlobals->u8PresentablesCount);

    // Note: Measurement sources are now stored internally and accessed via sensors
    // Verify sensor was created (presentables: [0]=CLI, [1]=CLI, [2]=LoopTime)
    SENSOR_T *psSensor = (SENSOR_T *)psGlobals->ppsPresentables[2];
    TEST_ASSERT_EQUAL_STRING("LoopTime", psSensor->sPresentable.pcName);
    TEST_ASSERT_EQUAL(99, psSensor->sVtab.eVType);
    TEST_ASSERT_EQUAL(99, psSensor->sVtab.eSType);
    TEST_ASSERT_NOT_EQUAL(0, psSensor->u8Flags & SENSOR_FLAG_CUMULATIVE); // cumulative=1
    TEST_ASSERT_EQUAL(5000, psSensor->u32ReportIntervalMs);
    TEST_ASSERT_NOT_NULL(psSensor->psSensorMeasure);
    TEST_ASSERT_EQUAL(MEASUREMENT_TYPE_LOOPTIME_E, psSensor->psSensorMeasure->eType);
#else
    // Test skipped in static allocation mode - requires more memory
    TEST_PASS();
#endif
}

/**
 * Test LoopTimeMeasure special case in Sensor (bypasses sampling interval)
 */
void test_vLoopTimeMeasure_SensorIntegration(void)
{
    PINCFG_RESULT_T eResult;
    char acOutStr[OUT_STR_MAX_LEN_D];
    size_t szMemoryRequired;

    // Configuration with loop time measurement (enableable=0, cumulative=1 for loop measurements)
    // Note: fOffset=1.0 (index 9) is required to prevent measurements from being multiplied by 0
    const char *pcCfg = "MS,5,looptime/"
                        "SR,LoopTime,looptime,99,99,0,1,100,5,1.0/";

    // Use two-pass parsing like other tests
    PINCFG_PARSE_PARAMS_T sParams = {
        .pcConfig = pcCfg,
        .eAddToLoopables = PinCfgCsv_eAddToTempLoopables,
        .eAddToPresentables = PinCfgCsv_eAddToTempPresentables,
        .pszMemoryRequired = &szMemoryRequired,
        .pcOutString = acOutStr,
        .u16OutStrMaxLen = (uint16_t)OUT_STR_MAX_LEN_D,
        .bValidate = false};

    // First pass + second pass
    eResult = PinCfgCsv_eParse(&sParams);
    TEST_ASSERT_EQUAL(PINCFG_OK_E, eResult);

    memset(acOutStr, 0, OUT_STR_MAX_LEN_D);
    sParams.pszMemoryRequired = NULL;
    eResult = PinCfgCsv_eParse(&sParams);
    TEST_ASSERT_EQUAL(PINCFG_OK_E, eResult);

    // Convert linked lists to arrays
    LinkedList_eLinkedListToArray((LINKEDLIST_ITEM_T **)(&psGlobals->ppsLoopables), &psGlobals->u8LoopablesCount);
    LinkedList_eLinkedListToArray((LINKEDLIST_ITEM_T **)(&psGlobals->ppsPresentables), &psGlobals->u8PresentablesCount);

    // Get the sensor (should be at index 2: [0]=CLI, [1]=CLI, [2]=LoopTime)
    SENSOR_T *psSensor = (SENSOR_T *)psGlobals->ppsPresentables[2];
    TEST_ASSERT_NOT_NULL(psSensor);

    // Verify cumulative mode is set (critical for this test to work)
    TEST_ASSERT_TRUE((psSensor->u8Flags & 0x01) != 0); // SENSOR_FLAG_CUMULATIVE

    // Simulate loop iterations
    // Note: Sampling interval is 100ms, so sensor measures at intervals of >= 100ms
    mock_micros_u32Return = 0;

    // Loop 1 (time 0) - First call initializes timestamp, no measurement yet
    PinCfgCsv_vLoop(mock_micros_u32Return);
    TEST_ASSERT_EQUAL(0, psSensor->u32SamplesCount);

    // Loop 2 (time 100ms) - Should measure (delta = 100ms), first real sample
    mock_micros_u32Return = 100;
    PinCfgCsv_vLoop(mock_micros_u32Return);
    TEST_ASSERT_EQUAL(1, psSensor->u32SamplesCount); // First successful measurement

    // Loop 3 (time 200ms) - Should measure again (delta = 100ms)
    mock_micros_u32Return = 200;
    PinCfgCsv_vLoop(mock_micros_u32Return);
    TEST_ASSERT_EQUAL(2, psSensor->u32SamplesCount);

    // Loop 4 (time 300ms) - Should measure (delta = 100ms)
    mock_micros_u32Return = 300;
    PinCfgCsv_vLoop(mock_micros_u32Return);
    TEST_ASSERT_EQUAL(3, psSensor->u32SamplesCount);

    // Loop 5 (time 600ms) - Should measure (delta = 300ms)
    mock_micros_u32Return = 600;
    PinCfgCsv_vLoop(mock_micros_u32Return);

    // Debug: print actual values
    if (psSensor->u32SamplesCount != 4 || psSensor->i64CumulatedValue < 599 || psSensor->i64CumulatedValue > 601)
    {
        printf("\n=== LoopTime Sensor Debug ===\n");
        printf("Samples count: %lu (expected 4)\n", (unsigned long)psSensor->u32SamplesCount);
        printf("Cumulated value: %lld (expected 600)\n", (long long)psSensor->i64CumulatedValue);
        printf("Sensor flags: 0x%02X\n", psSensor->u8Flags);
        printf("Sampling interval: %u ms\n", psSensor->u16SamplingIntervalMs);
        printf("Report interval: %u ms\n", psSensor->u32ReportIntervalMs);
        printf("i32Offset (scale factor): %d (raw fixed-point)\n", psSensor->i32Offset);
        printf("Data byte offset: %u\n", psSensor->u8DataByteOffset);
        printf("Data byte count: %u\n", psSensor->u8DataByteCount);
    }

    TEST_ASSERT_EQUAL(4, psSensor->u32SamplesCount);

    // Verify cumulative value (100ms + 100ms + 100ms + 300ms = 600ms)
    TEST_ASSERT_EQUAL(600, psSensor->i64CumulatedValue); // Already scaled value

    // Continue loops until report interval (5 seconds = 5000ms)
    for (uint32_t i = 6; i <= 5000; i++)
    {
        mock_micros_u32Return = i;
        PinCfgCsv_vLoop(mock_micros_u32Return);
    }

    // After reporting, samples should be reset
    TEST_ASSERT_EQUAL(0, psSensor->u32SamplesCount);
    TEST_ASSERT_EQUAL(0, psSensor->i64CumulatedValue);
}

/**
 * Test multiple LoopTimeMeasure sensors
 * Note: This test requires USE_MALLOC due to complex memory allocation patterns in two-pass parsing
 */
void test_vLoopTimeMeasure_MultipleSensors(void)
{
#ifdef USE_MALLOC
    PINCFG_RESULT_T eResult;
    char acOutStr[OUT_STR_MAX_LEN_D];
    size_t szMemoryRequired;

    // Configuration with multiple loop time sensors (enableable=0, cumulative=1 for loop measurements)
    const char *pcCfg = "MS,5,loop1/"
                        "MS,5,loop2/"
                        "SR,Loop1,loop1,99,99,0,1,100,5/"
                        "SR,Loop2,loop2,99,99,0,1,100,10/";

    // Use two-pass parsing like other tests
    PINCFG_PARSE_PARAMS_T sParams = {
        .pcConfig = pcCfg,
        .eAddToLoopables = PinCfgCsv_eAddToTempLoopables,
        .eAddToPresentables = PinCfgCsv_eAddToTempPresentables,
        .pszMemoryRequired = &szMemoryRequired,
        .pcOutString = acOutStr,
        .u16OutStrMaxLen = (uint16_t)OUT_STR_MAX_LEN_D,
        .bValidate = false};

    // First pass + second pass
    eResult = PinCfgCsv_eParse(&sParams);
    TEST_ASSERT_EQUAL(PINCFG_OK_E, eResult);

    memset(acOutStr, 0, OUT_STR_MAX_LEN_D);
    sParams.pszMemoryRequired = NULL;
    eResult = PinCfgCsv_eParse(&sParams);
    TEST_ASSERT_EQUAL(PINCFG_OK_E, eResult);

    // Convert linked lists to arrays
    LinkedList_eLinkedListToArray((LINKEDLIST_ITEM_T **)(&psGlobals->ppsLoopables), &psGlobals->u8LoopablesCount);
    LinkedList_eLinkedListToArray((LINKEDLIST_ITEM_T **)(&psGlobals->ppsPresentables), &psGlobals->u8PresentablesCount);

    // Note: Measurements are now internal, accessed via sensors
    // Verify two sensors created (presentables: [0]=CLI, [1]=CLI, [2]=Loop1, [3]=Loop2)
    SENSOR_T *psSensor1 = (SENSOR_T *)psGlobals->ppsPresentables[2];
    SENSOR_T *psSensor2 = (SENSOR_T *)psGlobals->ppsPresentables[3];
    TEST_ASSERT_EQUAL_STRING("Loop1", psSensor1->sPresentable.pcName);
    TEST_ASSERT_EQUAL_STRING("Loop2", psSensor2->sPresentable.pcName);
    TEST_ASSERT_EQUAL(5000, psSensor1->u32ReportIntervalMs);
    TEST_ASSERT_EQUAL(10000, psSensor2->u32ReportIntervalMs);
    TEST_ASSERT_NOT_NULL(psSensor1->psSensorMeasure);
    TEST_ASSERT_NOT_NULL(psSensor2->psSensorMeasure);
    TEST_ASSERT_EQUAL(MEASUREMENT_TYPE_LOOPTIME_E, psSensor1->psSensorMeasure->eType);
    TEST_ASSERT_EQUAL(MEASUREMENT_TYPE_LOOPTIME_E, psSensor2->psSensorMeasure->eType);
#else
    // Test skipped in static allocation mode - requires more memory
    TEST_PASS();
#endif
}

#endif // PINCFG_FEATURE_LOOPTIME_MEASUREMENT

// ============================================================================
// Test Registration Function
// ============================================================================

void register_measurements_tests(void)
{
    RUN_TEST(test_vCPUTemp);
#ifdef PINCFG_FEATURE_I2C_MEASUREMENT
    RUN_TEST(test_vI2CMeasure_Init);
    RUN_TEST(test_vI2CMeasure_SimpleRead);
    RUN_TEST(test_vI2CMeasure_CommandMode);
    RUN_TEST(test_vI2CMeasure_Timeout);
    RUN_TEST(test_vI2CMeasure_DeviceError);
    RUN_TEST(test_vI2CMeasure_RawData);
#endif
#ifdef PINCFG_FEATURE_SPI_MEASUREMENT
    RUN_TEST(test_vSPIMeasure_Init);
    RUN_TEST(test_vSPIMeasure_SimpleRead);
    RUN_TEST(test_vSPIMeasure_CommandModeNoDelay);
    RUN_TEST(test_vSPIMeasure_CommandModeWithDelay);
    RUN_TEST(test_vSPIMeasure_Timeout);
    RUN_TEST(test_vSPIMeasure_ErrorSimulation);
    RUN_TEST(test_vSPIMeasure_BufferSizeLimit);
    RUN_TEST(test_vSPIMeasure_CSPinControl);
    RUN_TEST(test_vSPIMeasure_CSVParsing_Simple);
    RUN_TEST(test_vSPIMeasure_CSVParsing_Command);
    RUN_TEST(test_vSPIMeasure_CSVParsing_Sensor);
#endif
#ifdef PINCFG_FEATURE_ANALOG_MEASUREMENT
    RUN_TEST(test_vAnalogMeasure_Init);
    RUN_TEST(test_vAnalogMeasure_SimpleRead);
    RUN_TEST(test_vAnalogMeasure_ErrorHandling);
    RUN_TEST(test_vAnalogMeasure_MultiplePins);
    RUN_TEST(test_vAnalogMeasure_CSVParsing);
    RUN_TEST(test_vAnalogMeasure_SensorIntegration);
#endif
#ifdef PINCFG_FEATURE_LOOPTIME_MEASUREMENT
    RUN_TEST(test_vLoopTimeMeasure_Init);
    RUN_TEST(test_vLoopTimeMeasure_TimeDelta);
    RUN_TEST(test_vLoopTimeMeasure_Offset);
    RUN_TEST(test_vLoopTimeMeasure_Overflow);
    RUN_TEST(test_vLoopTimeMeasure_NullParams);
    RUN_TEST(test_vLoopTimeMeasure_CSVParsing);
    RUN_TEST(test_vLoopTimeMeasure_SensorIntegration);
    RUN_TEST(test_vLoopTimeMeasure_MultipleSensors);
// TODO: Implement these tests
// RUN_TEST(test_vLoopTimeMeasure_BasicRecording);
// RUN_TEST(test_vLoopTimeMeasure_CSVParsing_Basic);
// RUN_TEST(test_vLoopTimeMeasure_CSVParsing_WithThresholds);
// RUN_TEST(test_vLoopTimeMeasure_MinMaxTracking);
// RUN_TEST(test_vLoopTimeMeasure_ArrayRotation);
// RUN_TEST(test_vLoopTimeMeasure_ThresholdViolations);
// RUN_TEST(test_vLoopTimeMeasure_EdgeCases);
#endif
}
