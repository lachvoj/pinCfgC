#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include "ArduinoMock.h"

uint8_t mock_pinMode_u8Pin;
uint8_t mock_pinMode_u8Mode;
uint32_t mock_pinMode_u32Called;
void pinMode(uint8_t u8Pin, uint8_t u8Mode)
{
    mock_pinMode_u8Pin = u8Pin;
    mock_pinMode_u8Mode = u8Mode;
    mock_pinMode_u32Called++;
}

uint8_t mock_digitalRead_u8Pin;
uint32_t mock_digitalRead_u32Called;
uint8_t mock_digitalRead_u8Return;
uint8_t *mock_digitalRead_u8ReturnSeq;
size_t mock_digigtalRead_szReturnSeqSize;
size_t mock_digitalRead_szReturnSeqIndex;
void mock_digitalRead_SetReturnSequence(uint8_t *pu8RetSeq, size_t szRetSeqSize)
{
    if (pu8RetSeq == NULL || szRetSeqSize == 0)
        return;

    mock_digitalRead_u8ReturnSeq = malloc(szRetSeqSize);
    if (mock_digitalRead_u8ReturnSeq == NULL)
        return;

    memcpy(mock_digitalRead_u8ReturnSeq, pu8RetSeq, szRetSeqSize);
    mock_digigtalRead_szReturnSeqSize = szRetSeqSize;
    mock_digitalRead_szReturnSeqIndex = 0;
}
uint8_t digitalRead(uint8_t u8Pin)
{
    mock_digitalRead_u32Called++;
    mock_digitalRead_u8Pin = u8Pin;
    if (mock_digitalRead_u8ReturnSeq == NULL || mock_digigtalRead_szReturnSeqSize == 0)
        return mock_digitalRead_u8Return;

    mock_digitalRead_szReturnSeqIndex++;
    if (mock_digitalRead_szReturnSeqIndex > mock_digigtalRead_szReturnSeqSize)
        mock_digitalRead_szReturnSeqIndex = 1;

    return mock_digitalRead_u8ReturnSeq[mock_digitalRead_szReturnSeqIndex - 1];
}

uint8_t mock_digitalWrite_u8Pin;
uint8_t mock_digitalWrite_u8Value;
uint32_t mock_digitalWrite_u32Called;
void digitalWrite(uint8_t u8Pin, uint8_t u8Value)
{
    mock_digitalWrite_u8Pin = u8Pin;
    mock_digitalWrite_u8Value = u8Value;
    mock_digitalWrite_u32Called++;
}

uint32_t mock_wait_u32ms;
uint32_t mock_pwait_u32Called;
void wait(uint32_t u32ms)
{
    mock_wait_u32ms = u32ms;
    mock_pwait_u32Called++;
}

void vArduinoMock_setup(void)
{
    mock_pinMode_u32Called = 0;
    mock_digitalRead_u32Called = 0;
    mock_digitalRead_u8Return = 0;
    mock_digitalWrite_u32Called = 0;
    mock_digitalRead_u8ReturnSeq = NULL;
    mock_digigtalRead_szReturnSeqSize = 0;
    mock_pwait_u32Called = 0;
}
