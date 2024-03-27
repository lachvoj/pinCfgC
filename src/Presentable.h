#ifndef PRESENTABLE_H
#define PRESENTABLE_H

#include "MySensorsWrapper.h"
#include "PinCfgStr.h"
#include "Types.h"

typedef struct PRESENTABLE_S PRESENTABLE_T;

typedef struct PRESENTABLE_VTAB_S
{
    void (*vReceive)(PRESENTABLE_T *psHandle, const void *pvMessage);
    mysensors_data_t eVType;
    mysensors_sensor_t eSType;
} PRESENTABLE_VTAB_T;

typedef struct PRESENTABLE_S
{
    PRESENTABLE_VTAB_T *psVtab;
    const char *pcName;
    uint8_t u8Id;
    uint8_t u8State;
    bool bStateChanged;
#ifdef MY_CONTROLLER_HA
    bool bStatePresented;
#endif
} PRESENTABLE_T;

typedef enum
{
    PRESENTABLE_OK_E,
    PRESENTABLE_ALLOCATION_ERROR_E,
    PRESENTABLE_NULLPTR_ERROR_E,
    PRESENTABLE_SUBINIT_ERROR_E,
    PRESENTABLE_ERROR_E
} PRESENTABLE_RESULT_T;

PRESENTABLE_RESULT_T Presentable_eInit(PRESENTABLE_T *psHandle, STRING_POINT_T *psName, uint8_t u8Id);

uint8_t Presentable_u8GetId(PRESENTABLE_T *psHandle);
const char *Presentable_pcGetName(PRESENTABLE_T *psHandle);
void Presentable_vSetState(PRESENTABLE_T *psHandle, uint8_t u8State, bool bSendStatus);
uint8_t Presentable_u8GetState(PRESENTABLE_T *psHandle);
void Presentable_vToggle(PRESENTABLE_T *psHandle);
void Presentable_vRcvMessage(PRESENTABLE_T *psHandle, const void *pvMessage);
void Presentable_vPresent(PRESENTABLE_T *psHandle);
void Presentable_vPresentState(PRESENTABLE_T *psHandle);
void Presentable_vSendState(PRESENTABLE_T *psHandle);

#endif // PRESENTABLE_H
