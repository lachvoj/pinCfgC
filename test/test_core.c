#include "test_helpers.h"

// ============================================================================
// Core Infrastructure Tests
// Memory, StringPoint, LinkedList, PinCfgStr, FixedPointParser
// ============================================================================

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
    // Calculate expected free space: Memory_eInit aligns end down to void* boundary
    // pvMemEnd = pu8Memory + ((szSize - 1) / sizeof(void *)) * sizeof(void *)
    size_t szAlignedSize = ((MEMORY_SZ - 1) / sizeof(void *)) * sizeof(void *);
    size_t szExpectedFree = szAlignedSize - sizeof(GLOBALS_T) - (2 * sizeof(void *));
    TEST_ASSERT_EQUAL(szExpectedFree, Memory_szGetFree());

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

void test_vFixedPointParser(void)
{
    int32_t i32Result;
    PINCFG_STR_RESULT_T eResult;
    STRING_POINT_T sStrPt;

    // Test positive integer
    sStrPt.pcStrStart = "1";
    sStrPt.szLen = 1;
    eResult = PinCfgStr_eAtoFixedPoint(&sStrPt, &i32Result);
    TEST_ASSERT_EQUAL(PINCFG_STR_OK_E, eResult);
    TEST_ASSERT_EQUAL(1000000, i32Result);

    // Test decimal
    sStrPt.pcStrStart = "0.0625";
    sStrPt.szLen = 6;
    eResult = PinCfgStr_eAtoFixedPoint(&sStrPt, &i32Result);
    TEST_ASSERT_EQUAL(PINCFG_STR_OK_E, eResult);
    TEST_ASSERT_EQUAL(62500, i32Result);

    // Test negative
    sStrPt.pcStrStart = "-2.1";
    sStrPt.szLen = 4;
    eResult = PinCfgStr_eAtoFixedPoint(&sStrPt, &i32Result);
    TEST_ASSERT_EQUAL(PINCFG_STR_OK_E, eResult);
    TEST_ASSERT_EQUAL(-2100000, i32Result);

    // Test 6 decimals (AHT10 temperature)
    sStrPt.pcStrStart = "0.000191";
    sStrPt.szLen = 8;
    eResult = PinCfgStr_eAtoFixedPoint(&sStrPt, &i32Result);
    TEST_ASSERT_EQUAL(PINCFG_STR_OK_E, eResult);
    TEST_ASSERT_EQUAL(191, i32Result);

    // Test truncation (>6 decimals)
    sStrPt.pcStrStart = "0.0000504";
    sStrPt.szLen = 9;
    eResult = PinCfgStr_eAtoFixedPoint(&sStrPt, &i32Result);
    TEST_ASSERT_EQUAL(PINCFG_STR_OK_E, eResult);
    TEST_ASSERT_EQUAL(50, i32Result); // Truncates to 0.00005

    // Test large value (max safe: 2147.483647)
    sStrPt.pcStrStart = "2147.483647";
    sStrPt.szLen = 11;
    eResult = PinCfgStr_eAtoFixedPoint(&sStrPt, &i32Result);
    TEST_ASSERT_EQUAL(PINCFG_STR_OK_E, eResult);
    TEST_ASSERT_EQUAL(2147483647, i32Result); // INT32_MAX

    // Test overflow detection
    sStrPt.pcStrStart = "2148";
    sStrPt.szLen = 4;
    eResult = PinCfgStr_eAtoFixedPoint(&sStrPt, &i32Result);
    TEST_ASSERT_EQUAL(PINCFG_STR_UNSUCCESSFULL_CONVERSION_E, eResult);

    // Test overflow with large decimal
    sStrPt.pcStrStart = "2147.483648";
    sStrPt.szLen = 11;
    eResult = PinCfgStr_eAtoFixedPoint(&sStrPt, &i32Result);
    TEST_ASSERT_EQUAL(PINCFG_STR_UNSUCCESSFULL_CONVERSION_E, eResult);

    // Test invalid format
    sStrPt.pcStrStart = "abc";
    sStrPt.szLen = 3;
    eResult = PinCfgStr_eAtoFixedPoint(&sStrPt, &i32Result);
    TEST_ASSERT_EQUAL(PINCFG_STR_UNSUCCESSFULL_CONVERSION_E, eResult);

    // Test NULL pointer
    eResult = PinCfgStr_eAtoFixedPoint(NULL, &i32Result);
    TEST_ASSERT_EQUAL(PINCFG_STR_INSUFFICIENT_BUFFER_E, eResult);

    // Test edge case: only decimal point
    sStrPt.pcStrStart = ".5";
    sStrPt.szLen = 2;
    eResult = PinCfgStr_eAtoFixedPoint(&sStrPt, &i32Result);
    TEST_ASSERT_EQUAL(PINCFG_STR_OK_E, eResult);
    TEST_ASSERT_EQUAL(500000, i32Result);

    // Test edge case: trailing decimal
    sStrPt.pcStrStart = "2.";
    sStrPt.szLen = 2;
    eResult = PinCfgStr_eAtoFixedPoint(&sStrPt, &i32Result);
    TEST_ASSERT_EQUAL(PINCFG_STR_OK_E, eResult);
    TEST_ASSERT_EQUAL(2000000, i32Result);

    // Test negative decimal only
    sStrPt.pcStrStart = "-.5";
    sStrPt.szLen = 3;
    eResult = PinCfgStr_eAtoFixedPoint(&sStrPt, &i32Result);
    TEST_ASSERT_EQUAL(PINCFG_STR_OK_E, eResult);
    TEST_ASSERT_EQUAL(-500000, i32Result);

    // Test zero
    sStrPt.pcStrStart = "0";
    sStrPt.szLen = 1;
    eResult = PinCfgStr_eAtoFixedPoint(&sStrPt, &i32Result);
    TEST_ASSERT_EQUAL(PINCFG_STR_OK_E, eResult);
    TEST_ASSERT_EQUAL(0, i32Result);

    // Test negative zero
    sStrPt.pcStrStart = "-0.0";
    sStrPt.szLen = 4;
    eResult = PinCfgStr_eAtoFixedPoint(&sStrPt, &i32Result);
    TEST_ASSERT_EQUAL(PINCFG_STR_OK_E, eResult);
    TEST_ASSERT_EQUAL(0, i32Result);
}

// ============================================================================
// Test Registration Function
// ============================================================================

void register_core_tests(void)
{
    RUN_TEST(test_vMemory);
    RUN_TEST(test_vStringPoint);
    RUN_TEST(test_vLinkedList);
    RUN_TEST(test_vPinCfgStr);
    RUN_TEST(test_vFixedPointParser);
}
