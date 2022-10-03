#include <stdio.h>

#include "InPin.h"
#include "Memory.h"
#include "MySensorsPresent.h"
#include "PinCfgCsv.h"
#include "PinCfgStr.h"
#include "Switch.h"
#include "Trigger.h"

#ifndef MEMORY_SZ
#define MEMORY_SZ 3000
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

// MYSENSORS_IF mock
uint8_t mock_bRequest_u8Id;
PINCFG_ELEMENT_TYPE_T mock_bRequest_eType;
bool mock_bRequest_bReturn;
bool bRequest(const uint8_t u8Id, const PINCFG_ELEMENT_TYPE_T eType)
{
    mock_bRequest_u8Id = u8Id;
    mock_bRequest_eType = eType;
    return mock_bRequest_bReturn;
}

uint8_t mock_bPresent_u8Id;
uint8_t mock_bPresent_eType;
const char *mock_bPresent_pcName;
bool mock_bPresent_bReturn;
bool bPresent(const uint8_t u8Id, const PINCFG_ELEMENT_TYPE_T eType, const char *pcName)
{
    mock_bPresent_u8Id = u8Id;
    mock_bPresent_eType = eType;
    mock_bPresent_pcName = pcName;
    return mock_bPresent_bReturn;
}

uint8_t mock_bSend_u8Id;
uint8_t mock_bSend_eType;
const void *mock_bSend_pvMessage;
bool mock_bSend_bReturn;
bool bSend(const uint8_t u8Id, const PINCFG_ELEMENT_TYPE_T eType, const void *pvMessage)
{
    mock_bSend_u8Id = u8Id;
    mock_bSend_eType = eType;
    mock_bSend_pvMessage = pvMessage;
    return mock_bSend_bReturn;
}
// end MYSENSORS_IF mock

// PIN_IF_T mock
uint8_t mock_vPinMode_u8Pin;
uint8_t mock_vPinMode_u8Mode;
void vPinMode(uint8_t u8Pin, uint8_t u8Mode)
{
    mock_vPinMode_u8Pin = u8Pin;
    mock_vPinMode_u8Mode = u8Mode;
}

uint8_t mock_u8ReadPin_u8Pin;
uint8_t mock_u8ReadPin_u8return;
uint8_t u8ReadPin(uint8_t u8Pin)
{
    mock_u8ReadPin_u8Pin = u8Pin;
    return mock_u8ReadPin_u8return;
}

uint8_t mock_vWritePin_u8Pin;
uint8_t mock_vWritePin_u8Value;
void vWritePin(uint8_t u8Pin, uint8_t u8Value)
{
    mock_vWritePin_u8Pin = u8Pin;
    mock_vWritePin_u8Value = u8Value;
}
// end PIN_IF_T mock

// utils
void printStringPointArray(STRING_POINT_T *asStrPts, uint8_t u8StrPtsLen)
{
    for (uint8_t i = 0; i < u8StrPtsLen; i++)
    {
        for (size_t j = 0; j < asStrPts[i].szLen; j++)
            printf("%c", asStrPts[i].pcStrStart[j]);
        printf("\n");
    }
    printf("\n");
}
// end utils

static uint8_t testMemory[MEMORY_SZ];
int main()
{
    // sizes
    printf("sizeof(SWITCH_HANDLE_T): %ld\n", sizeof(SWITCH_HANDLE_T));
    printf("sizeof(INPIN_HANDLE_T): %ld\n", sizeof(INPIN_HANDLE_T));
    printf("sizeof(TRIGGER_HANDLE_T): %ld\n", sizeof(TRIGGER_HANDLE_T));
    printf("sizeof(TRIGGER_SWITCHACTION_T): %ld\n", sizeof(TRIGGER_SWITCHACTION_T));

    Memory_eInit(testMemory, MEMORY_SZ);

    // memory test
    void *a1 = Memory_vpAlloc(2);
    void *a2 = Memory_vpAlloc(3);
    void *a3 = Memory_vpAlloc(MEMORY_SZ);

    printf("Mem addr: %ld\n", (long unsigned int)&testMemory);
    printf("allocated1 addr: %ld\n", (long unsigned int)a1);
    printf("allocated2 addr: %ld\n", (long unsigned int)a2);
    printf("allocated3 addr: %ld\n", (long unsigned int)a3);

    // mysensor if mock setup
    MYSENOSRS_IF_T *psMySensorsIf = (MYSENOSRS_IF_T *)Memory_vpAlloc(sizeof(MYSENOSRS_IF_T));
    psMySensorsIf->bRequest = bRequest;
    psMySensorsIf->bPresent = bPresent;
    psMySensorsIf->bSend = bSend;

    // pin if mock setup
    PIN_IF_T *psPinIf = (PIN_IF_T *)Memory_vpAlloc(sizeof(PIN_IF_T));
    psPinIf->vPinMode = vPinMode;
    psPinIf->u8ReadPin = u8ReadPin;
    psPinIf->vWritePin = vWritePin;

    // name mock setup
    char pcName[] = "Ahoj";
    STRING_POINT_T psName;
    PinCfgStr_vInitStrPoint(&psName, pcName, sizeof(pcName) - 1);

    // mysensorspresent test
    MYSENSORSPRESENT_HANDLE_T *psPresentHandle =
        (MYSENSORSPRESENT_HANDLE_T *)Memory_vpAlloc(sizeof(MYSENSORSPRESENT_HANDLE_T));
    MySensorsPresent_eInit(psPresentHandle, &psName, 1, 1, true);

    // inpin test
    INPIN_HANDLE_T *sInPinHandle = (INPIN_HANDLE_T *)Memory_vpAlloc(sizeof(INPIN_HANDLE_T));
    InPin_eInit(sInPinHandle, &psName, 2, false, 1);

    // string
    char pcTestString[] = "This\nIs the best;testing\n\n\nstring ever made!\nbraka\n";
    STRING_POINT_T psTestStrPt;
    PinCfgStr_vInitStrPoint(&psTestStrPt, pcTestString, sizeof(pcTestString) - 1);
    char cDelimiter = '\n';
    STRING_POINT_T asOut[AS_OUT_MAX_LEN_D];
    uint8_t u8OutLen;
    PinCfgStr_eSplitStrPt(&psTestStrPt, cDelimiter, asOut, &u8OutLen, AS_OUT_MAX_LEN_D);
    printStringPointArray(asOut, u8OutLen);

    PinCfgStr_eRemoveAt(asOut, &u8OutLen, 1);
    printStringPointArray(asOut, u8OutLen);

    PinCfgStr_eRemoveEmpty(asOut, &u8OutLen);
    printStringPointArray(asOut, u8OutLen);

    PinCfgStr_eRemoveStartingWith(asOut, &u8OutLen, 'b');
    printStringPointArray(asOut, u8OutLen);

    size_t szSplitCount = PinCfgStr_szGetSplitCount(&psTestStrPt, '\n');
    printf("szSplitCount %ld\n", szSplitCount);

    asOut[0] = psTestStrPt;
    PinCfgStr_vGetSplitElemByIndex(&asOut[0], '\n', 0);
    printStringPointArray(asOut, 1);
    asOut[1] = psTestStrPt;
    PinCfgStr_vGetSplitElemByIndex(&asOut[1], '\n', 5);
    printStringPointArray(&asOut[1], 1);
    asOut[2] = psTestStrPt;
    PinCfgStr_vGetSplitElemByIndex(&asOut[2], '\n', 2);
    printStringPointArray(&asOut[2], 1);
    asOut[3] = psTestStrPt;
    PinCfgStr_vGetSplitElemByIndex(&asOut[3], '\n', 6);
    printStringPointArray(&asOut[3], 1);

    // PinCfgCsv
    const char *pcConfig = "# (bluePill)\n"
                           "S,12,o1,13,o2,12,o3,11,o4,10,o5,9,o6,8,o7,7,o8,6,o9,5,o10,4,o11,3,o12,2\n"
                           "I,12,i1,16,i2,15,i3,14,i4,31,i5,30,i6,201,i7,195,i8,194,i9,193,i10,192,i11,19,i12,18\n"
                           "#triggers\n"
                           "T,vtl11,i1,3,1,1,o2,2\n"
                           "T,vtl12,i1,3,1,2,o2,2,o3,2";

    char acOutStr[OUT_STR_MAX_LEN_D];
    PinCfgCsv_eParse(
        pcConfig,
        acOutStr,
        (uint16_t)OUT_STR_MAX_LEN_D,
        false,
        testMemory,
        (size_t)MEMORY_SZ,
        true,
        psMySensorsIf,
        psPinIf);

    return 0;
}
