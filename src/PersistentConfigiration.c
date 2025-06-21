#ifdef UNIT_TEST
#include "EEPROMMock.h"
#else
#include <core/MyEepromAddresses.h>
#endif

#include <string.h>

#include "PersistentConfigiration.h"

#define EEPROM_PINCFG_SIZE (EEPROM_LOCAL_CONFIG_ADDRESS + (10U))
#define EEPROM_PINCFG_SIZE_B1 (EEPROM_PINCFG_SIZE + (2U))
#define EEPROM_PINCFG_SIZE_B2 (EEPROM_PINCFG_SIZE_B1 + (2U))
#define EEPROM_PINCFG_CHKSUM (EEPROM_PINCFG_SIZE_B2 + (2U))
#define EEPROM_PINCFG_CHKSUM_B1 (EEPROM_PINCFG_CHKSUM + (1U))
#define EEPROM_PINCFG_CHKSUM_B2 (EEPROM_PINCFG_CHKSUM_B1 + (1U))
#define EEPROM_PINCFG (EEPROM_PINCFG_CHKSUM_B2 + (1U))
#define EEPROM_PINCFG_END 1024
#define EEPROM_PINCFG_BACKUP_MAX_POSSIBLE_SIZE ((EEPROM_PINCFG_END - EEPROM_PINCFG) / (2U))
#define EEPROM_PINCFG_BACKUP (EEPROM_PINCFG + EEPROM_PINCFG_BACKUP_MAX_POSSIBLE_SIZE)
#define DATA_LENGTH E2END

uint8_t crc8(const uint8_t *data, uint16_t dataSize)
{
    uint8_t ret = 0;
    for (uint16_t i = 0; i < dataSize; ++i)
    {
        ret ^= data[i];
    }

    return ret;
}

PERCFG_RESULT_T eWriteCfg(void* iEePos, uint16_t u16CfgSize, const char *pcCfg)
{
    vHwWriteConfigBlock((void *)pcCfg, iEePos, u16CfgSize);

    // TODO: verify
    uint8_t *u8EePos = (uint8_t *)iEePos;
    for (uint16_t u16pos = 0; u16pos < u16CfgSize; ++u16pos)
    {
        if (pcCfg[u16pos] != u8EEPROMRead((int)(u8EePos + u16pos)))
            return PERCFG_WRITE_CFG_FAILED_E;
    }

    return PERCFG_OK_E;
}

PERCFG_RESULT_T PersistentCfg_eSave(const char *pcCfg)
{
    const uint16_t u16CfgSize = (uint16_t)strlen(pcCfg);
    if (u16CfgSize == 0 || u16CfgSize > PINCFG_CONFIG_MAX_SZ_D)
        return PERCFG_CFG_BIGGER_THAN_MAX_SZ_E;

    // write size
    vHwWriteConfigBlock((void *)&u16CfgSize, (void *)EEPROM_PINCFG_SIZE, sizeof(uint16_t));
    vHwWriteConfigBlock((void *)&u16CfgSize, (void *)EEPROM_PINCFG_SIZE_B1, sizeof(uint16_t));
    vHwWriteConfigBlock((void *)&u16CfgSize, (void *)EEPROM_PINCFG_SIZE_B2, sizeof(uint16_t));

    // verify size
    uint16_t u16CfgSizeVerify;
    vHwReadConfigBlock((void *)&u16CfgSizeVerify, (void *)EEPROM_PINCFG_SIZE, sizeof(uint16_t));
    if (u16CfgSize != u16CfgSizeVerify)
        return PERCFG_WRITE_SIZE_FAILED_E;
    vHwReadConfigBlock((void *)&u16CfgSizeVerify, (void *)EEPROM_PINCFG_SIZE_B1, sizeof(uint16_t));
    if (u16CfgSize != u16CfgSizeVerify)
        return PERCFG_WRITE_SIZE_FAILED_E;
    vHwReadConfigBlock((void *)&u16CfgSizeVerify, (void *)EEPROM_PINCFG_SIZE_B2, sizeof(uint16_t));
    if (u16CfgSize != u16CfgSizeVerify)
        return PERCFG_WRITE_SIZE_FAILED_E;

    // write checksum
    uint8_t u8CfgCheckSum = crc8((const uint8_t *)pcCfg, u16CfgSize);
    vHwWriteConfigBlock((void *)&u8CfgCheckSum, (void *)EEPROM_PINCFG_CHKSUM, sizeof(uint8_t));
    vHwWriteConfigBlock((void *)&u8CfgCheckSum, (void *)EEPROM_PINCFG_CHKSUM_B1, sizeof(uint8_t));
    vHwWriteConfigBlock((void *)&u8CfgCheckSum, (void *)EEPROM_PINCFG_CHKSUM_B2, sizeof(uint8_t));

    uint8_t u8CfgCheckSumVerify;
    vHwReadConfigBlock((void *)&u8CfgCheckSumVerify, (void *)EEPROM_PINCFG_CHKSUM, sizeof(uint8_t));
    if (u8CfgCheckSum != u8CfgCheckSumVerify)
        return PERCFG_WRITE_CHECKSUM_FAILED_E;
    vHwReadConfigBlock((void *)&u8CfgCheckSumVerify, (void *)EEPROM_PINCFG_CHKSUM_B1, sizeof(uint8_t));
    if (u8CfgCheckSum != u8CfgCheckSumVerify)
        return PERCFG_WRITE_CHECKSUM_FAILED_E;
    vHwReadConfigBlock((void *)&u8CfgCheckSumVerify, (void *)EEPROM_PINCFG_CHKSUM_B2, sizeof(uint8_t));
    if (u8CfgCheckSum != u8CfgCheckSumVerify)
        return PERCFG_WRITE_CHECKSUM_FAILED_E;

    // overwrite node id to force obtain new one
    u16CfgSizeVerify = AUTO;
    vHwWriteConfigBlock((void *)&u16CfgSizeVerify, (void *)EEPROM_NODE_ID_ADDRESS, 1u);

    // write cfg
    PERCFG_RESULT_T eRet = eWriteCfg((void *)EEPROM_PINCFG, u16CfgSize, pcCfg);
    if (eRet == PERCFG_OK_E && u16CfgSize <= EEPROM_PINCFG_BACKUP_MAX_POSSIBLE_SIZE)
    {
        eWriteCfg((void *)EEPROM_PINCFG_BACKUP, u16CfgSize, pcCfg);
    }

    return eRet;
}

PERCFG_RESULT_T PersistentCfg_eLoad(const char *pcCfg)
{
#ifndef PINCFG_CONFIG_MAX_SZ_D
#error "Define PINCFG_CONFIG_MAX_SZ_D !"
#endif

    // read size
    uint16_t u16CfgSize;
    PERCFG_RESULT_T eGetSizeRet = PersistentCfg_eGetSize(&u16CfgSize);
    if (eGetSizeRet != PERCFG_OK_E)
        return eGetSizeRet;

    // read checksum
    uint8_t u8CfgCheckSum;
    uint8_t u8CfgCheckSumB1;
    uint8_t u8CfgCheckSumB2;

    vHwReadConfigBlock((void *)&u8CfgCheckSum, (void *)EEPROM_PINCFG_CHKSUM, sizeof(uint8_t));
    vHwReadConfigBlock((void *)&u8CfgCheckSumB1, (void *)EEPROM_PINCFG_CHKSUM_B1, sizeof(uint8_t));
    vHwReadConfigBlock((void *)&u8CfgCheckSumB2, (void *)EEPROM_PINCFG_CHKSUM_B2, sizeof(uint8_t));

    if (u8CfgCheckSum != u8CfgCheckSumB1 && u8CfgCheckSum == u8CfgCheckSumB2)
        vHwWriteConfigBlock((void *)&u8CfgCheckSum, (void *)EEPROM_PINCFG_CHKSUM_B1, sizeof(uint16_t));
    else if (u8CfgCheckSum == u8CfgCheckSumB1 && u8CfgCheckSum != u8CfgCheckSumB2)
        vHwWriteConfigBlock((void *)&u8CfgCheckSum, (void *)EEPROM_PINCFG_CHKSUM_B2, sizeof(uint16_t));
    else if (u8CfgCheckSumB1 == u8CfgCheckSumB2 && u8CfgCheckSumB1 != u8CfgCheckSum)
    {
        vHwWriteConfigBlock((void *)&u8CfgCheckSum, (void *)EEPROM_PINCFG_CHKSUM, sizeof(uint16_t));
        u8CfgCheckSum = u8CfgCheckSumB1;
    }
    else if (u8CfgCheckSum != u8CfgCheckSumB1 && u8CfgCheckSum != u8CfgCheckSumB2 && u8CfgCheckSumB1 != u8CfgCheckSumB2)
        return PERCFG_READ_CHECKSUM_FAILED_E;

    // read configuration
    uint8_t u8CfgCheckSumCalculated;
    vHwReadConfigBlock((void *)pcCfg, (void *)EEPROM_PINCFG, u16CfgSize);
    u8CfgCheckSumCalculated = crc8((const uint8_t *)pcCfg, u16CfgSize);
    bool bIsCfgOk = (u8CfgCheckSumCalculated == u8CfgCheckSum);

    if (u16CfgSize > EEPROM_PINCFG_BACKUP_MAX_POSSIBLE_SIZE && bIsCfgOk)
        return PERCFG_OK_E;
    else if (u16CfgSize > EEPROM_PINCFG_BACKUP_MAX_POSSIBLE_SIZE && !bIsCfgOk)
        return PERCFG_READ_FAILED_E;

    // read configuration backup
    vHwReadConfigBlock((void *)pcCfg, (void *)EEPROM_PINCFG_BACKUP, u16CfgSize);
    u8CfgCheckSumCalculated = crc8((const uint8_t *)pcCfg, u16CfgSize);
    bool bIsCfgBckOk = (u8CfgCheckSumCalculated == u8CfgCheckSum);

    if (!bIsCfgOk && bIsCfgBckOk)
    {
        eWriteCfg((void *)EEPROM_PINCFG, u16CfgSize, pcCfg);
        return PERCFG_OK_E;
    }

    if (bIsCfgOk && !bIsCfgBckOk)
    {
        vHwReadConfigBlock((void *)pcCfg, (void *)EEPROM_PINCFG, u16CfgSize);
        eWriteCfg((void *)EEPROM_PINCFG, u16CfgSize, pcCfg);
        return PERCFG_OK_E;
    }

    if (!bIsCfgOk && !bIsCfgBckOk)
        return PERCFG_READ_AND_BACKUP_FAILED_E;

    return PERCFG_OK_E;
}

PERCFG_RESULT_T PersistentCfg_eGetSize(uint16_t *pu16CfgSize)
{
    uint16_t u16CfgSize;
    uint16_t u16CfgSizeB1;
    uint16_t u16CfgSizeB2;

    vHwReadConfigBlock((void *)&u16CfgSize, (void *)EEPROM_PINCFG_SIZE, sizeof(uint16_t));
    vHwReadConfigBlock((void *)&u16CfgSizeB1, (void *)EEPROM_PINCFG_SIZE_B1, sizeof(uint16_t));
    vHwReadConfigBlock((void *)&u16CfgSizeB2, (void *)EEPROM_PINCFG_SIZE_B2, sizeof(uint16_t));

    if (u16CfgSize != u16CfgSizeB1 && u16CfgSize == u16CfgSizeB2)
        vHwWriteConfigBlock((void *)&u16CfgSize, (void *)EEPROM_PINCFG_SIZE_B1, sizeof(uint16_t));
    else if (u16CfgSize == u16CfgSizeB1 && u16CfgSize != u16CfgSizeB2)
        vHwWriteConfigBlock((void *)&u16CfgSize, (void *)EEPROM_PINCFG_SIZE_B2, sizeof(uint16_t));
    else if (u16CfgSizeB1 == u16CfgSizeB2 && u16CfgSizeB1 != u16CfgSize)
    {
        vHwWriteConfigBlock((void *)&u16CfgSize, (void *)EEPROM_PINCFG_SIZE, sizeof(uint16_t));
        u16CfgSize = u16CfgSizeB1;
    }
    else if (u16CfgSize != u16CfgSizeB1 && u16CfgSize != u16CfgSizeB2 && u16CfgSizeB1 != u16CfgSizeB2)
        return PERCFG_READ_SIZE_FAILED_E;

    if (u16CfgSize > PINCFG_CONFIG_MAX_SZ_D)
        return PERCFG_CFG_BIGGER_THAN_MAX_SZ_E;

    *pu16CfgSize = u16CfgSize;
    return PERCFG_OK_E;
}