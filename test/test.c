#include <stdio.h>
#include <unity.h>

#ifndef ARDUINO
#include "ArduinoMock.h"
#endif
#include "ExtCfgReceiver.h"
#include "Globals.h"
#include "InPin.h"
#include "Memory.h"
#include "MySensorsPresent.h"
#include "PinCfgCsv.h"
#include "PinCfgStr.h"
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

#ifndef ARDUINO
    init_ArduinoMock();
#endif
}

void tearDown(void)
{
    // clean stuff up here
}

void test_vMemory(void)
{
    MEMORY_RESULT_T eMemRes = Memory_eInit(NULL, sizeof(GLOBALS_HANDLE_T) - 1);
    TEST_ASSERT_EQUAL(MEMORY_ERROR_E, eMemRes);

    eMemRes = Memory_eInit(testMemory, sizeof(GLOBALS_HANDLE_T) - 1);
    TEST_ASSERT_EQUAL(MEMORY_INSUFFICIENT_SIZE_ERROR_E, eMemRes);

    eMemRes = Memory_eInit(testMemory, MEMORY_SZ);
    TEST_ASSERT_EQUAL(MEMORY_OK_E, eMemRes);
    // memory test
    char *a1 = (char *)Memory_vpAlloc(2);
    char *a2 = (char *)Memory_vpAlloc(3);
    char *a3 = (char *)Memory_vpAlloc(MEMORY_SZ);
    TEST_ASSERT_EQUAL(ENOMEM, errno);
    TEST_ASSERT_EQUAL(
        MEMORY_SZ - sizeof(GLOBALS_HANDLE_T) - (2 * sizeof(char *)) - (MEMORY_SZ % sizeof(char *)), Memory_szGetFree());

    TEST_ASSERT_EQUAL(a1, (char *)testMemory + sizeof(GLOBALS_HANDLE_T));
    TEST_ASSERT_EQUAL(a2, a1 + sizeof(char *));
    TEST_ASSERT_NULL(a3);

    // temp memory alloc test
    Memory_eInit(testMemory, MEMORY_SZ);
    char *vpTemp1 = (char *)Memory_vpTempAlloc((sizeof(char *) * 2) - 2);
    char *vpTemp2 = (char *)Memory_vpTempAlloc((sizeof(char *) * 2) - 2);
    char *vpTemp3 = (char *)Memory_vpTempAlloc(MEMORY_SZ);

    TEST_ASSERT_EQUAL(vpTemp1, (char *)psGlobals->pvMemEnd - (sizeof(char *) * 2));
    TEST_ASSERT_EQUAL(vpTemp2, vpTemp1 - (sizeof(char *) * 2));
    TEST_ASSERT_NULL(vpTemp3);

    char str1[] = "Hello!";
    char str2[] = "Hi!";
    strcpy(vpTemp1, str1);
    strcpy(vpTemp2, str2);
    TEST_ASSERT_EQUAL_STRING("Hello!", vpTemp1);
    TEST_ASSERT_EQUAL_STRING("Hi!", vpTemp2);
    Memory_vTempFree();
    TEST_ASSERT_EQUAL_STRING("", vpTemp1);
    TEST_ASSERT_EQUAL_STRING("", vpTemp2);

    vpTemp1 = (char *)Memory_vpTempAlloc(8);
    vpTemp1[0] = 0xff;
    vpTemp1[1] = 0xff;
    vpTemp1[2] = 0xff;
    vpTemp1[3] = 0xff;
    vpTemp1[4] = 0xff;
    vpTemp1[5] = 0xff;
    vpTemp1[6] = 0xff;
    vpTemp1[7] = 0xff;
    vpTemp1[8] = 0xff;
    uint8_t u32Exp1[] = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff};
    uint8_t u32Exp2[] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff};
    TEST_ASSERT_EQUAL_MEMORY(&u32Exp1, vpTemp1, 9);
    Memory_vTempFree();
    TEST_ASSERT_EQUAL_MEMORY(&u32Exp2, vpTemp1, 9);
}

void test_vStringPoint(void)
{
    char acName[] = "Ahoj";
    STRING_POINT_T sName;
    PinCfgStr_vInitStrPoint(&sName, acName, sizeof(acName) - 1);
    TEST_ASSERT_EQUAL(acName, sName.pcStrStart);
    TEST_ASSERT_EQUAL(strlen(acName), 4);
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
    MYSENSORSPRESENT_HANDLE_T *psPresentHandle =
        (MYSENSORSPRESENT_HANDLE_T *)Memory_vpAlloc(sizeof(MYSENSORSPRESENT_HANDLE_T));
    LOOPRE_VTAB_T sLooPreVTab;
    psPresentHandle->sLooPre.psVtab = &sLooPreVTab;

    // init test
    MYSENSORSPRESENT_RESULT_T eResult;
    eResult = MySensorsPresent_eInit(NULL, &sName, 1);
    TEST_ASSERT_EQUAL(MYSENSORSPRESENT_NULLPTR_ERROR_E, eResult);
    eResult = MySensorsPresent_eInit(psPresentHandle, NULL, 1);
    TEST_ASSERT_EQUAL(MYSENSORSPRESENT_NULLPTR_ERROR_E, eResult);
    eResult = MySensorsPresent_eInit(NULL, NULL, 1);
    TEST_ASSERT_EQUAL(MYSENSORSPRESENT_NULLPTR_ERROR_E, eResult);
    Memory_vpTempAlloc((size_t)(psGlobals->pvMemTempEnd - psGlobals->pvMemNext - sizeof(char *)));
    eResult = MySensorsPresent_eInit(psPresentHandle, &sName, 1);
    Memory_vTempFree();
    TEST_ASSERT_EQUAL(MYSENSORSPRESENT_ALLOCATION_ERROR_E, eResult);
    eResult = MySensorsPresent_eInit(psPresentHandle, &sName, 1);
    TEST_ASSERT_EQUAL_STRING(acName, psPresentHandle->sLooPre.pcName);
    TEST_ASSERT_EQUAL_UINT8(1, psPresentHandle->sLooPre.u8Id);

    MySensorsPresent_vSendState((LOOPRE_T *)psPresentHandle);
    TEST_ASSERT_EQUAL_UINT8(0, mock_MyMessage_set_uint8_t_value);
    TEST_ASSERT_EQUAL(1, mock_send_u32Called);

    MySensorsPresent_vSetState(psPresentHandle, 1, true);
    TEST_ASSERT_EQUAL(2, mock_send_u32Called);

    MySensorsPresent_vToggle(psPresentHandle);
    TEST_ASSERT_EQUAL_UINT8(0, mock_MyMessage_set_uint8_t_value);
    TEST_ASSERT_EQUAL(3, mock_send_u32Called);

    TEST_ASSERT_EQUAL_UINT8(1U, LooPre_u8GetId((LOOPRE_T *)psPresentHandle));

    TEST_ASSERT_EQUAL_STRING(acName, LooPre_pcGetName((LOOPRE_T *)psPresentHandle));

    uint8_t u8Msg = 0x55U;
    MySensorsPresent_vRcvMessage((LOOPRE_T *)psPresentHandle, (const void *)&u8Msg);
    TEST_ASSERT_EQUAL(3, mock_send_u32Called);

    MySensorsPresent_vPresent((LOOPRE_T *)psPresentHandle);
    TEST_ASSERT_EQUAL(1, mock_bPresent_u8Id);
    TEST_ASSERT_EQUAL_STRING(acName, mock_bPresent_pcName);
    TEST_ASSERT_EQUAL(1, mock_bPresent_u32Called);

    MySensorsPresent_vPresentState((LOOPRE_T *)psPresentHandle);
    TEST_ASSERT_EQUAL(4, mock_send_u32Called);
    TEST_ASSERT_EQUAL_UINT8(1, mock_bRequest_u8Id);
    TEST_ASSERT_EQUAL(1, mock_bRequest_u32Called);
}

void test_vInPin(void)
{
    char acName[] = "InPin";
    STRING_POINT_T sName;
    PinCfgStr_vInitStrPoint(&sName, acName, sizeof(acName) - 1);
    INPIN_HANDLE_T *psInPinHandle = (INPIN_HANDLE_T *)Memory_vpAlloc(sizeof(INPIN_HANDLE_T));
    INPIN_RESULT_T eResult;

    // init
    eResult = InPin_eInit(NULL, &sName, 2, 16);
    TEST_ASSERT_EQUAL(INPIN_NULLPTR_ERROR_E, eResult);
    eResult = InPin_eInit(psInPinHandle, NULL, 2, 16);
    TEST_ASSERT_EQUAL(INPIN_SUBINIT_ERROR_E, eResult);
    eResult = InPin_eInit(psInPinHandle, &sName, 2, 16);
    TEST_ASSERT_EQUAL(INPIN_OK_E, eResult);
#ifndef ARDUINO
    TEST_ASSERT_EQUAL(16, mock_pinMode_u8Pin);
    TEST_ASSERT_EQUAL(INPUT_PULLUP, mock_pinMode_u8Mode);
    TEST_ASSERT_EQUAL(1, mock_pinMode_u32Called);
    TEST_ASSERT_EQUAL(16, mock_digitalWrite_u8Pin);
    TEST_ASSERT_EQUAL(HIGH, mock_digitalWrite_u8Value);
    TEST_ASSERT_EQUAL(1, mock_digitalWrite_u32Called);
#endif

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
    char acOutStr[OUT_STR_MAX_LEN_D];
    size_t szMemoryRequired;
    GLOBALS_HANDLE_T *psGlobals = (GLOBALS_HANDLE_T *)testMemory;

    strncpy(
        PinCfgCsv_pcGetCfgBuf(),
        "S,o1,13/"
        "CR,120/"
        "SI,o2,12/"
        "CR,350/"
        "SI,o5,9/"
        "SF,o6,10,2/"
        "SIF,o7,11,3/",
        PINCFG_CONFIG_MAX_SZ_D);

    eParseResult = PinCfgCsv_eParse(&szMemoryRequired, acOutStr, (uint16_t)OUT_STR_MAX_LEN_D, false);

    TEST_ASSERT_EQUAL(PINCFG_OK_E, eParseResult);

    SWITCH_HANDLE_T *psSwitchHnd = (SWITCH_HANDLE_T *)psGlobals->psLoopablesFirst;
    TEST_ASSERT_EQUAL_STRING("o1", psSwitchHnd->sMySenPresent.sLooPre.pcName);
    TEST_ASSERT_EQUAL(SWITCH_CLASSIC_E, psSwitchHnd->eMode);
    TEST_ASSERT_EQUAL(13, psSwitchHnd->u8OutPin);

    psSwitchHnd = (SWITCH_HANDLE_T *)psGlobals->psLoopablesFirst->psNextLoopable;
    TEST_ASSERT_EQUAL_STRING("o2", psSwitchHnd->sMySenPresent.sLooPre.pcName);
    TEST_ASSERT_EQUAL(SWITCH_IMPULSE_E, psSwitchHnd->eMode);
    TEST_ASSERT_EQUAL(12, psSwitchHnd->u8OutPin);
    TEST_ASSERT_EQUAL(120, psSwitchHnd->u32ImpulseDuration);

    psSwitchHnd = (SWITCH_HANDLE_T *)psGlobals->psLoopablesFirst->psNextLoopable->psNextLoopable;
    TEST_ASSERT_EQUAL_STRING("o5", psSwitchHnd->sMySenPresent.sLooPre.pcName);
    TEST_ASSERT_EQUAL(SWITCH_IMPULSE_E, psSwitchHnd->eMode);
    TEST_ASSERT_EQUAL(9, psSwitchHnd->u8OutPin);
    TEST_ASSERT_EQUAL(350, psSwitchHnd->u32ImpulseDuration);

    psSwitchHnd = (SWITCH_HANDLE_T *)psGlobals->psLoopablesFirst->psNextLoopable->psNextLoopable->psNextLoopable;
    TEST_ASSERT_EQUAL_STRING("o6", psSwitchHnd->sMySenPresent.sLooPre.pcName);
    TEST_ASSERT_EQUAL(SWITCH_CLASSIC_E, psSwitchHnd->eMode);
    TEST_ASSERT_EQUAL(10, psSwitchHnd->u8OutPin);
    TEST_ASSERT_EQUAL(2, psSwitchHnd->u8FbPin);

    psSwitchHnd =
        (SWITCH_HANDLE_T *)psGlobals->psLoopablesFirst->psNextLoopable->psNextLoopable->psNextLoopable->psNextLoopable;
    TEST_ASSERT_EQUAL_STRING("o7", psSwitchHnd->sMySenPresent.sLooPre.pcName);
    TEST_ASSERT_EQUAL(SWITCH_IMPULSE_E, psSwitchHnd->eMode);
    TEST_ASSERT_EQUAL(11, psSwitchHnd->u8OutPin);
    TEST_ASSERT_EQUAL(3, psSwitchHnd->u8FbPin);

    TEST_ASSERT_EQUAL(350, psGlobals->u32SwitchImpulseDurationMs);
}

void test_vPinCfgCsv(void)
{
    PINCFG_RESULT_T eParseResult;
    char acOutStr[OUT_STR_MAX_LEN_D];
    size_t szMemoryRequired;

    Memory_vpAlloc((size_t)(psGlobals->pvMemTempEnd - psGlobals->pvMemNext - sizeof(char *)));
    eParseResult = PinCfgCsv_eParse(&szMemoryRequired, acOutStr, (uint16_t)OUT_STR_MAX_LEN_D, false);
    TEST_ASSERT_EQUAL(0, szMemoryRequired);
    TEST_ASSERT_EQUAL(PINCFG_OUTOFMEMORY_ERROR_E, eParseResult);
    TEST_ASSERT_EQUAL_STRING("E:ExtCfgReceiver:Out of memory.\n", acOutStr);
    Memory_eReset();

    eParseResult = PinCfgCsv_eParse(&szMemoryRequired, acOutStr, (uint16_t)OUT_STR_MAX_LEN_D, false);
    TEST_ASSERT_EQUAL(sizeof(EXTCFGRECEIVER_HANDLE_T), szMemoryRequired);
    TEST_ASSERT_EQUAL(PINCFG_ERROR_E, eParseResult);
    TEST_ASSERT_EQUAL_STRING("E:Invalid format. Empty configuration.\n", acOutStr);
    Memory_eReset();

    strncpy(PinCfgCsv_pcGetCfgBuf(), "S", PINCFG_CONFIG_MAX_SZ_D);
    eParseResult = PinCfgCsv_eParse(&szMemoryRequired, acOutStr, (uint16_t)OUT_STR_MAX_LEN_D, false);
    TEST_ASSERT_EQUAL(sizeof(EXTCFGRECEIVER_HANDLE_T), szMemoryRequired);
    TEST_ASSERT_EQUAL(PINCFG_WARNINGS_E, eParseResult);
    TEST_ASSERT_EQUAL_STRING("W:L:0:Not defined or invalid format.\nI: Configuration parsed.\n", acOutStr);
    Memory_eReset();

    strncpy(PinCfgCsv_pcGetCfgBuf(), "S,o1", PINCFG_CONFIG_MAX_SZ_D);
    eParseResult = PinCfgCsv_eParse(&szMemoryRequired, acOutStr, (uint16_t)OUT_STR_MAX_LEN_D, false);
    TEST_ASSERT_EQUAL(sizeof(EXTCFGRECEIVER_HANDLE_T), szMemoryRequired);
    TEST_ASSERT_EQUAL(PINCFG_WARNINGS_E, eParseResult);
    TEST_ASSERT_EQUAL_STRING("W:L:0:Switch:Invalid number of arguments.\nI: Configuration parsed.\n", acOutStr);
    Memory_eReset();

    strncpy(PinCfgCsv_pcGetCfgBuf(), "S,o1,afsd", PINCFG_CONFIG_MAX_SZ_D);
    eParseResult = PinCfgCsv_eParse(&szMemoryRequired, acOutStr, (uint16_t)OUT_STR_MAX_LEN_D, false);
    TEST_ASSERT_EQUAL(sizeof(EXTCFGRECEIVER_HANDLE_T), szMemoryRequired);
    TEST_ASSERT_EQUAL(PINCFG_WARNINGS_E, eParseResult);
    TEST_ASSERT_EQUAL_STRING("W:L:0:Switch:Invalid pin number.\nI: Configuration parsed.\n", acOutStr);
    Memory_eReset();

    strncpy(PinCfgCsv_pcGetCfgBuf(), "S,o1,afsd,o2", PINCFG_CONFIG_MAX_SZ_D);
    eParseResult = PinCfgCsv_eParse(&szMemoryRequired, acOutStr, (uint16_t)OUT_STR_MAX_LEN_D, false);
    TEST_ASSERT_EQUAL(sizeof(EXTCFGRECEIVER_HANDLE_T), szMemoryRequired);
    TEST_ASSERT_EQUAL(PINCFG_WARNINGS_E, eParseResult);
    TEST_ASSERT_EQUAL_STRING("W:L:0:Switch:Invalid number of items defining names and pins.\nI: Configuration parsed.\n", acOutStr);
    Memory_eReset();

    Memory_vpAlloc(
        (size_t)(psGlobals->pvMemTempEnd - psGlobals->pvMemNext - sizeof(char *) - sizeof(EXTCFGRECEIVER_HANDLE_T)));
    strncpy(PinCfgCsv_pcGetCfgBuf(), "S,o1,2", PINCFG_CONFIG_MAX_SZ_D);
    eParseResult = PinCfgCsv_eParse(&szMemoryRequired, acOutStr, (uint16_t)OUT_STR_MAX_LEN_D, false);
    TEST_ASSERT_EQUAL(sizeof(EXTCFGRECEIVER_HANDLE_T), szMemoryRequired);
    TEST_ASSERT_EQUAL(PINCFG_OUTOFMEMORY_ERROR_E, eParseResult);
    TEST_ASSERT_EQUAL_STRING("E:L:0:Switch:Out of memory.\n", acOutStr);
    Memory_eReset();

    TEST_ASSERT_EQUAL(
        (MEMORY_SZ - sizeof(GLOBALS_HANDLE_T) - (MEMORY_SZ - sizeof(GLOBALS_HANDLE_T)) % sizeof(char *)),
        Memory_szGetFree());

    // pins o3,11,o4,10 removed due to emulator uses them for serial output
    strncpy(
        PinCfgCsv_pcGetCfgBuf(),
        "# (bluePill)/"
        "S,o1,13,o2,12,o5,9,o6,8,o7,7,o8,6,o9,5,o10,4,o11,3,o12,2/"
        "I,i1,16,i2,15,i3,14,i4,31,i5,30,i6,201,i7,195,i8,194,i9,193,i10,192,i11,19,i12,18/"
        "#triggers/"
        "T,vtl11,i1,0,1,o2,2/"
        "T,vtl12,i1,0,1,o2,2,o5,2",
        PINCFG_CONFIG_MAX_SZ_D);
    eParseResult = PinCfgCsv_eParse(&szMemoryRequired, acOutStr, (uint16_t)OUT_STR_MAX_LEN_D, false);
    TEST_ASSERT_EQUAL(
        sizeof(EXTCFGRECEIVER_HANDLE_T) + (10 * sizeof(SWITCH_HANDLE_T) + 10 * sizeof(char *)) +
            (12 * sizeof(INPIN_HANDLE_T) + 12 * sizeof(char *)) + (2 * sizeof(TRIGGER_HANDLE_T)) +
            (3 * sizeof(TRIGGER_SWITCHACTION_T)),
        szMemoryRequired);
    TEST_ASSERT_EQUAL(PINCFG_OK_E, eParseResult);
    TEST_ASSERT_EQUAL_STRING("I: Configuration parsed.\n", acOutStr);
}

void test_vExtCfgReceiver(void)
{
    EXTCFGRECEIVER_HANDLE_T *psExtReceiver = (EXTCFGRECEIVER_HANDLE_T *)Memory_vpAlloc(sizeof(EXTCFGRECEIVER_HANDLE_T));
    ExtCfgReceiver_eInit(psExtReceiver, 0);

    ExtCfgReceiver_vRcvMessage((LOOPRE_T *)psExtReceiver, "#[#S,o1,13/");
    TEST_ASSERT_EQUAL(1, mock_send_u32Called);
    TEST_ASSERT_EQUAL_STRING("RECEIVING", mock_MyMessage_set_char_value);
    TEST_ASSERT_EQUAL_STRING("S,o1,13/", psGlobals->acCfgBuf);


    memset(mock_MyMessage_set_char_value, 0, SEND_MESSAGE_MAX_LENGTH_D);
    ExtCfgReceiver_vRcvMessage((LOOPRE_T *)psExtReceiver, "I,i1,16/");
    TEST_ASSERT_EQUAL_STRING("S,o1,13/I,i1,16/", psGlobals->acCfgBuf);
    ExtCfgReceiver_vRcvMessage((LOOPRE_T *)psExtReceiver, "T,vtl11,i1,0,1,o2,2/#]#");
    TEST_ASSERT_EQUAL_STRING("S,o1,13/I,i1,16/T,vtl11,i1,0,1,o2,2/", psGlobals->acCfgBuf);
    TEST_ASSERT_EQUAL(2, mock_send_u32Called);
    TEST_ASSERT_EQUAL_STRING("VALIDATION OK", mock_MyMessage_set_char_value);

    memset(mock_MyMessage_set_char_value, 0, SEND_MESSAGE_MAX_LENGTH_D);
    ExtCfgReceiver_eInit(psExtReceiver, 0);
    ExtCfgReceiver_vRcvMessage((LOOPRE_T *)psExtReceiver, "#[#T,o1,13/#]#");
    TEST_ASSERT_EQUAL_STRING("RECEIVINGW:L:0:Trigger:Invalid number of arguments.\nI: Configuration parsed.\nVALIDATION ERROR", mock_MyMessage_set_char_value);

    memset(mock_MyMessage_set_char_value, 0, SEND_MESSAGE_MAX_LENGTH_D);
    strcpy(psGlobals->acCfgBuf, "This is test message which should be longer than max one-shot message.");
    ExtCfgReceiver_eInit(psExtReceiver, 0);
    ExtCfgReceiver_vRcvMessage((LOOPRE_T *)psExtReceiver, "#C:GET_CFG");
    TEST_ASSERT_EQUAL_STRING("This is test message which should be longer than max one-shot message.LISTENING", mock_MyMessage_set_char_value);
}

void test_vGlobalConfig(void)
{
    PINCFG_RESULT_T eParseResult;
    char acOutStr[OUT_STR_MAX_LEN_D];
    size_t szMemoryRequired;

    strncpy(
        PinCfgCsv_pcGetCfgBuf(),
        "CD,330/"
        "CM,620/"
        "CR,150/"
        "CN,1000/"
        "CF,30000/"
        "CA,1966080/",
        PINCFG_CONFIG_MAX_SZ_D);

    eParseResult = PinCfgCsv_eParse(&szMemoryRequired, acOutStr, (uint16_t)OUT_STR_MAX_LEN_D, false);

    TEST_ASSERT_EQUAL(PINCFG_OK_E, eParseResult);

    GLOBALS_HANDLE_T *psGlobals = (GLOBALS_HANDLE_T *)testMemory;
    TEST_ASSERT_EQUAL(330, psGlobals->u32InPinDebounceMs);
    TEST_ASSERT_EQUAL(620, psGlobals->u32InPinMulticlickMaxDelayMs);
    TEST_ASSERT_EQUAL(150, psGlobals->u32SwitchImpulseDurationMs);

    // InPin debounce
    memset(acOutStr, 0, OUT_STR_MAX_LEN_D);
    strncpy(PinCfgCsv_pcGetCfgBuf(), "D", PINCFG_CONFIG_MAX_SZ_D);

    eParseResult = PinCfgCsv_eParse(&szMemoryRequired, acOutStr, (uint16_t)OUT_STR_MAX_LEN_D, false);

    TEST_ASSERT_EQUAL_STRING("W:L:0:Not defined or invalid format.\nI: Configuration parsed.\n", acOutStr);

    memset(acOutStr, 0, OUT_STR_MAX_LEN_D);
    strncpy(PinCfgCsv_pcGetCfgBuf(), "CD,abc", PINCFG_CONFIG_MAX_SZ_D);

    eParseResult = PinCfgCsv_eParse(&szMemoryRequired, acOutStr, (uint16_t)OUT_STR_MAX_LEN_D, false);

    TEST_ASSERT_EQUAL_STRING("W:L:0:InPinDebounceMs:Invalid number.\nI: Configuration parsed.\n", acOutStr);

    // InPin multiclick
    memset(acOutStr, 0, OUT_STR_MAX_LEN_D);
    strncpy(PinCfgCsv_pcGetCfgBuf(), "CM", PINCFG_CONFIG_MAX_SZ_D);

    eParseResult = PinCfgCsv_eParse(&szMemoryRequired, acOutStr, (uint16_t)OUT_STR_MAX_LEN_D, false);

    TEST_ASSERT_EQUAL_STRING("W:L:0:Not defined or invalid format.\nI: Configuration parsed.\n", acOutStr);

    memset(acOutStr, 0, OUT_STR_MAX_LEN_D);
    strncpy(PinCfgCsv_pcGetCfgBuf(), "CM,abc", PINCFG_CONFIG_MAX_SZ_D);

    eParseResult = PinCfgCsv_eParse(&szMemoryRequired, acOutStr, (uint16_t)OUT_STR_MAX_LEN_D, false);

    TEST_ASSERT_EQUAL_STRING("W:L:0:InPinMulticlickMaxDelayMs:Invalid number.\nI: Configuration parsed.\n", acOutStr);

    // Switch impulse duration
    memset(acOutStr, 0, OUT_STR_MAX_LEN_D);
    strncpy(PinCfgCsv_pcGetCfgBuf(), "CR", PINCFG_CONFIG_MAX_SZ_D);

    eParseResult = PinCfgCsv_eParse(&szMemoryRequired, acOutStr, (uint16_t)OUT_STR_MAX_LEN_D, false);

    TEST_ASSERT_EQUAL_STRING("W:L:0:Not defined or invalid format.\nI: Configuration parsed.\n", acOutStr);

    memset(acOutStr, 0, OUT_STR_MAX_LEN_D);
    strncpy(PinCfgCsv_pcGetCfgBuf(), "CR,abc", PINCFG_CONFIG_MAX_SZ_D);

    eParseResult = PinCfgCsv_eParse(&szMemoryRequired, acOutStr, (uint16_t)OUT_STR_MAX_LEN_D, false);

    TEST_ASSERT_EQUAL_STRING("W:L:0:SwitchImpulseDurationMs:Invalid number.\nI: Configuration parsed.\n", acOutStr);

    // Switch feedback on delay
    memset(acOutStr, 0, OUT_STR_MAX_LEN_D);
    strncpy(PinCfgCsv_pcGetCfgBuf(), "CN", PINCFG_CONFIG_MAX_SZ_D);

    eParseResult = PinCfgCsv_eParse(&szMemoryRequired, acOutStr, (uint16_t)OUT_STR_MAX_LEN_D, false);

    TEST_ASSERT_EQUAL_STRING("W:L:0:Not defined or invalid format.\nI: Configuration parsed.\n", acOutStr);

    memset(acOutStr, 0, OUT_STR_MAX_LEN_D);
    strncpy(PinCfgCsv_pcGetCfgBuf(), "CN,abc", PINCFG_CONFIG_MAX_SZ_D);

    eParseResult = PinCfgCsv_eParse(&szMemoryRequired, acOutStr, (uint16_t)OUT_STR_MAX_LEN_D, false);

    TEST_ASSERT_EQUAL_STRING("W:L:0:SwitchFbOnDelayMs:Invalid number.\nI: Configuration parsed.\n", acOutStr);

    // Switch feedback off delay
    memset(acOutStr, 0, OUT_STR_MAX_LEN_D);
    strncpy(PinCfgCsv_pcGetCfgBuf(), "CF", PINCFG_CONFIG_MAX_SZ_D);

    eParseResult = PinCfgCsv_eParse(&szMemoryRequired, acOutStr, (uint16_t)OUT_STR_MAX_LEN_D, false);

    TEST_ASSERT_EQUAL_STRING("W:L:0:Not defined or invalid format.\nI: Configuration parsed.\n", acOutStr);

    memset(acOutStr, 0, OUT_STR_MAX_LEN_D);
    strncpy(PinCfgCsv_pcGetCfgBuf(), "CF,abc", PINCFG_CONFIG_MAX_SZ_D);

    eParseResult = PinCfgCsv_eParse(&szMemoryRequired, acOutStr, (uint16_t)OUT_STR_MAX_LEN_D, false);

    TEST_ASSERT_EQUAL_STRING("W:L:0:SwitchFbOffDelayMs:Invalid number.\nI: Configuration parsed.\n", acOutStr);

    // Switch timed action time addition
    memset(acOutStr, 0, OUT_STR_MAX_LEN_D);
    strncpy(PinCfgCsv_pcGetCfgBuf(), "CA", PINCFG_CONFIG_MAX_SZ_D);

    eParseResult = PinCfgCsv_eParse(&szMemoryRequired, acOutStr, (uint16_t)OUT_STR_MAX_LEN_D, false);

    TEST_ASSERT_EQUAL_STRING("W:L:0:Not defined or invalid format.\nI: Configuration parsed.\n", acOutStr);

    memset(acOutStr, 0, OUT_STR_MAX_LEN_D);
    strncpy(PinCfgCsv_pcGetCfgBuf(), "CA,abc", PINCFG_CONFIG_MAX_SZ_D);

    eParseResult = PinCfgCsv_eParse(&szMemoryRequired, acOutStr, (uint16_t)OUT_STR_MAX_LEN_D, false);

    TEST_ASSERT_EQUAL_STRING("W:L:0:SwitchTimedActionAdditionMs:Invalid number.\nI: Configuration parsed.\n", acOutStr);
}

#ifndef ARDUINO
int main(void)
#else
int test_main(void)
#endif
{
    // sizes
    printf("sizeof(GLOBALS_HANDLE_T): %ld\n", sizeof(GLOBALS_HANDLE_T));
    printf("sizeof(EXTCFGRECEIVER_HANDLE_T): %ld\n", sizeof(EXTCFGRECEIVER_HANDLE_T));
    printf("sizeof(SWITCH_HANDLE_T): %ld\n", sizeof(SWITCH_HANDLE_T));
    printf("sizeof(INPIN_HANDLE_T): %ld\n", sizeof(INPIN_HANDLE_T));
    printf("sizeof(TRIGGER_HANDLE_T): %ld\n", sizeof(TRIGGER_HANDLE_T));
    printf("sizeof(TRIGGER_SWITCHACTION_T): %ld\n", sizeof(TRIGGER_SWITCHACTION_T));
    printf("\n\n");

    UNITY_BEGIN();
    RUN_TEST(test_vMemory);
    RUN_TEST(test_vStringPoint);
    RUN_TEST(test_vPinCfgStr);
    RUN_TEST(test_vMySenosrsPresent);
    RUN_TEST(test_vInPin);
    RUN_TEST(test_vSwitch);
    RUN_TEST(test_vPinCfgCsv);
    RUN_TEST(test_vExtCfgReceiver);
    RUN_TEST(test_vGlobalConfig);

    return UNITY_END();
}
