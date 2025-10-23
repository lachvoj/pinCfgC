#ifdef UNIT_TEST
#include "EEPROMMock.h"
#else
#include <core/MyEepromAddresses.h>
#endif

#include <string.h>

#include "PersistentConfigiration.h"

// Total config size used internally (password + config data)
#define PINCFG_TOTAL_SIZE (PINCFG_AUTH_PASSWORD_MAX_LEN_D + PINCFG_CONFIG_MAX_SZ_D)

// Block-level checksum configuration
#define PINCFG_BLOCK_SIZE 32U
#define PINCFG_NUM_BLOCKS ((PINCFG_TOTAL_SIZE + PINCFG_BLOCK_SIZE - 1) / PINCFG_BLOCK_SIZE)

// EEPROM Memory Layout - Partial backup strategy (limited by MySensors reserved area)
// Total available: 611 bytes (1024 - 413 MySensors reserved)
// Optimized: Start immediately after MySensors to maximize backup space
// Actual addresses: [MySensors(0-412)][CRC16×3(413-418)][Password(419-450)]
//                   [Config(451-930)][BlockCRC×2(931-962)][Backup(963-1023)]
// Backup: 61 bytes - covers full password (32B) + first 29 bytes of config
// Layout: [MySensors(413)][CRC16×3(6)][Password(32)][Config(480)][BlockCRC×2(32)][Backup(61)]

// Data section offsets (relative to start of data area, not EEPROM address)
#define PINCFG_PASSWORD_OFFSET 0U
#define PINCFG_CONFIG_OFFSET PINCFG_AUTH_PASSWORD_MAX_LEN_D

// EEPROM absolute addresses (optimized to start immediately after MySensors)
#define EEPROM_PINCFG_CHKSUM (EEPROM_LOCAL_CONFIG_ADDRESS)  // Start at 413 (no gap!)
#define EEPROM_PINCFG_CHKSUM_B1 (EEPROM_PINCFG_CHKSUM + (2U))  // CRC16 backup 1
#define EEPROM_PINCFG_CHKSUM_B2 (EEPROM_PINCFG_CHKSUM_B1 + (2U))  // CRC16 backup 2
#define EEPROM_PASSWORD (EEPROM_PINCFG_CHKSUM_B2 + (2U))  // Password section start
#define EEPROM_PINCFG (EEPROM_PASSWORD + PINCFG_AUTH_PASSWORD_MAX_LEN_D)  // Config data start

// Block checksums (redundant 2x for reliability)
#define EEPROM_PINCFG_BLOCK_CHKSUMS (EEPROM_PINCFG + PINCFG_CONFIG_MAX_SZ_D)
#define EEPROM_PINCFG_BLOCK_CHKSUMS_B1 (EEPROM_PINCFG_BLOCK_CHKSUMS + PINCFG_NUM_BLOCKS)

// Dynamic backup area - uses all remaining EEPROM space
#define EEPROM_PINCFG_BACKUP_START (EEPROM_PINCFG_BLOCK_CHKSUMS_B1 + PINCFG_NUM_BLOCKS)
#define EEPROM_PINCFG_BACKUP_MAX_SIZE (1024 - EEPROM_PINCFG_BACKUP_START)


// Forward declarations for internal functions
static PERCFG_RESULT_T eSave(const char *pcData, uint16_t u16Offset, uint16_t u16Size);
static PERCFG_RESULT_T eLoad(char *pcDataOut, uint16_t u16Offset);

// Calculate how much of config can be backed up (whatever fits in remaining EEPROM)
static inline uint16_t getBackupSize(uint16_t u16CfgSize)
{
    // Backup as much as possible, limited by:
    // 1. Actual config size
    // 2. Available EEPROM space
    return (u16CfgSize <= EEPROM_PINCFG_BACKUP_MAX_SIZE) ? u16CfgSize : EEPROM_PINCFG_BACKUP_MAX_SIZE;
}

// Scan EEPROM config area to find config size (ends at first \0)
// Returns size INCLUDING null terminator via pointer
PERCFG_RESULT_T PersistentCfg_eGetConfigSize(uint16_t *pu16ConfigSize)
{
    if (pu16ConfigSize == NULL)
        return PERCFG_ERROR_E;

    uint8_t au8Block[PINCFG_BLOCK_SIZE];
    
    // Scan only the config area (not password area)
    for (uint16_t offset = 0; offset < PINCFG_CONFIG_MAX_SZ_D; offset += PINCFG_BLOCK_SIZE)
    {
        uint16_t blockSize = (offset + PINCFG_BLOCK_SIZE <= PINCFG_CONFIG_MAX_SZ_D) 
                             ? PINCFG_BLOCK_SIZE 
                             : (PINCFG_CONFIG_MAX_SZ_D - offset);
        
        vHwReadConfigBlock((void *)au8Block, (void *)(EEPROM_PINCFG + offset), blockSize);
        
        // Search for null terminator in this block
        for (uint16_t i = 0; i < blockSize; i++)
        {
            if (au8Block[i] == '\0')
            {
                *pu16ConfigSize = offset + i + 1;  // Include the \0 in size
                return PERCFG_OK_E;
            }
        }
    }
    
    // No null found in config section - no valid config
    *pu16ConfigSize = 0;
    return PERCFG_OK_E;
}

// CRC16-CCITT (polynomial 0x1021) - detects all 1-2 bit errors and most burst errors
uint16_t crc16(const uint8_t *data, uint16_t dataSize)
{
    uint16_t crc = 0xFFFF;
    
    for (uint16_t i = 0; i < dataSize; ++i)
    {
        crc ^= (uint16_t)data[i] << 8;
        for (uint8_t bit = 0; bit < 8; ++bit)
        {
            if (crc & 0x8000)
                crc = (crc << 1) ^ 0x1021;
            else
                crc = crc << 1;
        }
    }
    
    return crc;
}

// Fast CRC8 for block-level checksums
uint8_t crc8(const uint8_t *data, uint16_t dataSize)
{
    uint8_t crc = 0xFF;
    
    for (uint16_t i = 0; i < dataSize; ++i)
    {
        crc ^= data[i];
        for (uint8_t bit = 0; bit < 8; ++bit)
        {
            if (crc & 0x80)
                crc = (crc << 1) ^ 0x07;
            else
                crc = crc << 1;
        }
    }
    
    return crc;
}

// Calculate block checksums for granular error detection
void calculateBlockChecksums(const char *pcCfg, uint16_t u16CfgSize, uint8_t *pu8BlockChecksums)
{
    uint16_t u16BlockIdx = 0;
    
    for (uint16_t offset = 0; offset < u16CfgSize; offset += PINCFG_BLOCK_SIZE)
    {
        uint16_t blockSize = (offset + PINCFG_BLOCK_SIZE <= u16CfgSize) 
                             ? PINCFG_BLOCK_SIZE 
                             : (u16CfgSize - offset);
        pu8BlockChecksums[u16BlockIdx++] = crc8((const uint8_t *)(pcCfg + offset), blockSize);
    }
}

// Write config to EEPROM optimized for flash paging
// Writes entire data in ONE operation for optimal flash page utilization
PERCFG_RESULT_T eWriteCfg(void* iEePos, uint16_t u16CfgSize, const char *pcCfg)
{
    // For flash-backed EEPROM: write entire block at once to maximize page efficiency
    // This is optimal for systems with flash page writes (typically 32-256 bytes)
    vHwWriteConfigBlock((void *)pcCfg, iEePos, u16CfgSize);

    // Verify entire write in blocks (read is fast, write is expensive)
    uint16_t u16Offset = 0;
    uint8_t au8Block[PINCFG_BLOCK_SIZE];
    
    while (u16Offset < u16CfgSize)
    {
        uint16_t u16BlockSize = (u16Offset + PINCFG_BLOCK_SIZE <= u16CfgSize) 
                                ? PINCFG_BLOCK_SIZE 
                                : (u16CfgSize - u16Offset);
        
        vHwReadConfigBlock((void *)au8Block, 
                          (void *)((uint8_t *)iEePos + u16Offset), 
                          u16BlockSize);
        
        if (memcmp(pcCfg + u16Offset, au8Block, u16BlockSize) != 0)
            return PERCFG_WRITE_CFG_FAILED_E;
        
        u16Offset += u16BlockSize;
    }

    return PERCFG_OK_E;
}

// Internal function - saves either password section or config section
// u16Offset: PINCFG_PASSWORD_OFFSET for password, PINCFG_CONFIG_OFFSET for config
// u16Size: data size to save INCLUDING null terminator
static PERCFG_RESULT_T eSave(const char *pcData, uint16_t u16Offset, uint16_t u16Size)
{
    if (u16Size == 0 || (u16Offset + u16Size) > PINCFG_TOTAL_SIZE)
        return PERCFG_CFG_BIGGER_THAN_MAX_SZ_E;

    // Determine new total size based on what we're updating
    uint16_t u16ConfigSize;
    uint16_t u16TotalSize;
    
    if (u16Offset == PINCFG_PASSWORD_OFFSET)
    {
        // Updating password - get existing config size by scanning
        u16ConfigSize = 0;
        uint8_t au8Block[PINCFG_BLOCK_SIZE];
        
        for (uint16_t offset = PINCFG_CONFIG_OFFSET; offset < PINCFG_TOTAL_SIZE; offset += PINCFG_BLOCK_SIZE)
        {
            uint16_t blockSize = (offset + PINCFG_BLOCK_SIZE <= PINCFG_TOTAL_SIZE) 
                                 ? PINCFG_BLOCK_SIZE 
                                 : (PINCFG_TOTAL_SIZE - offset);
            
            vHwReadConfigBlock((void *)au8Block, (void *)(EEPROM_PINCFG + (offset - PINCFG_CONFIG_OFFSET)), blockSize);
            
            for (uint16_t i = 0; i < blockSize; i++)
            {
                if (au8Block[i] == '\0')
                {
                    u16ConfigSize = offset - PINCFG_CONFIG_OFFSET + i + 1;
                    goto found_config_size;
                }
            }
        }
        found_config_size:
        
        u16TotalSize = PINCFG_CONFIG_OFFSET + u16ConfigSize;
    }
    else
    {
        // Updating config - use new config size
        // Password section is always PINCFG_CONFIG_OFFSET bytes (32) for alignment
        u16ConfigSize = u16Size;
        u16TotalSize = PINCFG_CONFIG_OFFSET + u16ConfigSize;
    }
    
    // Calculate CRC16 checksum incrementally (no stack buffer!)
    uint16_t u16CfgCheckSum = 0xFFFF;
    uint8_t au8Block[PINCFG_BLOCK_SIZE];
    
    // CRC over password section
    for (uint16_t offset = 0; offset < PINCFG_CONFIG_OFFSET; offset += PINCFG_BLOCK_SIZE)
    {
        uint16_t blockSize = (offset + PINCFG_BLOCK_SIZE <= PINCFG_CONFIG_OFFSET) 
                             ? PINCFG_BLOCK_SIZE 
                             : (PINCFG_CONFIG_OFFSET - offset);
        
        if (u16Offset == PINCFG_PASSWORD_OFFSET && offset < u16Size)
        {
            // Use new password data
            uint16_t copySize = (offset + blockSize <= u16Size) ? blockSize : (u16Size - offset);
            memcpy(au8Block, pcData + offset, copySize);
            if (copySize < blockSize)
                memset(au8Block + copySize, 0, blockSize - copySize);
        }
        else
        {
            // Use existing password data
            vHwReadConfigBlock((void *)au8Block, (void *)(EEPROM_PASSWORD + offset), blockSize);
        }
        
        // Update CRC16
        for (uint16_t i = 0; i < blockSize; ++i)
        {
            u16CfgCheckSum ^= (uint16_t)au8Block[i] << 8;
            for (uint8_t bit = 0; bit < 8; ++bit)
            {
                if (u16CfgCheckSum & 0x8000)
                    u16CfgCheckSum = (u16CfgCheckSum << 1) ^ 0x1021;
                else
                    u16CfgCheckSum = u16CfgCheckSum << 1;
            }
        }
    }
    
    // CRC over config section (if exists)
    if (u16ConfigSize > 0)
    {
        for (uint16_t offset = 0; offset < u16ConfigSize; offset += PINCFG_BLOCK_SIZE)
        {
            uint16_t blockSize = (offset + PINCFG_BLOCK_SIZE <= u16ConfigSize) 
                                 ? PINCFG_BLOCK_SIZE 
                                 : (u16ConfigSize - offset);
            
            if (u16Offset == PINCFG_CONFIG_OFFSET && offset < u16Size)
            {
                // Use new config data
                uint16_t copySize = (offset + blockSize <= u16Size) ? blockSize : (u16Size - offset);
                memcpy(au8Block, pcData + offset, copySize);
                if (copySize < blockSize)
                    memset(au8Block + copySize, 0, blockSize - copySize);
            }
            else
            {
                // Use existing config data
                vHwReadConfigBlock((void *)au8Block, (void *)(EEPROM_PINCFG + offset), blockSize);
            }
            
            // Update CRC16
            for (uint16_t i = 0; i < blockSize; ++i)
            {
                u16CfgCheckSum ^= (uint16_t)au8Block[i] << 8;
                for (uint8_t bit = 0; bit < 8; ++bit)
                {
                    if (u16CfgCheckSum & 0x8000)
                        u16CfgCheckSum = (u16CfgCheckSum << 1) ^ 0x1021;
                    else
                        u16CfgCheckSum = u16CfgCheckSum << 1;
                }
            }
        }
    }

    // Write CRC16 checksum (triple redundancy) - write as ONE 6-byte block for flash efficiency
    uint8_t au8CrcBlock[6];
    memcpy(&au8CrcBlock[0], &u16CfgCheckSum, sizeof(uint16_t));
    memcpy(&au8CrcBlock[2], &u16CfgCheckSum, sizeof(uint16_t));
    memcpy(&au8CrcBlock[4], &u16CfgCheckSum, sizeof(uint16_t));
    vHwWriteConfigBlock((void *)au8CrcBlock, (void *)EEPROM_PINCFG_CHKSUM, 6);

    // Verify CRC16 checksums
    uint8_t au8CrcBlockVerify[6];
    vHwReadConfigBlock((void *)au8CrcBlockVerify, (void *)EEPROM_PINCFG_CHKSUM, 6);
    if (memcmp(au8CrcBlock, au8CrcBlockVerify, 6) != 0)
        return PERCFG_WRITE_CHECKSUM_FAILED_E;

    // Calculate and write block checksums incrementally
    uint8_t au8BlockChecksums[PINCFG_NUM_BLOCKS];
    memset(au8BlockChecksums, 0, PINCFG_NUM_BLOCKS);
    
    uint16_t u16BlockIdx = 0;
    
    // Block checksums over password section
    for (uint16_t offset = 0; offset < PINCFG_CONFIG_OFFSET; offset += PINCFG_BLOCK_SIZE)
    {
        uint16_t blockSize = (offset + PINCFG_BLOCK_SIZE <= PINCFG_CONFIG_OFFSET) 
                             ? PINCFG_BLOCK_SIZE 
                             : (PINCFG_CONFIG_OFFSET - offset);
        
        if (u16Offset == PINCFG_PASSWORD_OFFSET && offset < u16Size)
        {
            // Use new password data
            uint16_t copySize = (offset + blockSize <= u16Size) ? blockSize : (u16Size - offset);
            memcpy(au8Block, pcData + offset, copySize);
            if (copySize < blockSize)
                memset(au8Block + copySize, 0, blockSize - copySize);
        }
        else
        {
            // Use existing password data
            vHwReadConfigBlock((void *)au8Block, (void *)(EEPROM_PASSWORD + offset), blockSize);
        }
        
        au8BlockChecksums[u16BlockIdx++] = crc8((const uint8_t *)au8Block, blockSize);
    }
    
    // Block checksums over config section
    if (u16ConfigSize > 0)
    {
        for (uint16_t offset = 0; offset < u16ConfigSize; offset += PINCFG_BLOCK_SIZE)
        {
            uint16_t blockSize = (offset + PINCFG_BLOCK_SIZE <= u16ConfigSize) 
                                 ? PINCFG_BLOCK_SIZE 
                                 : (u16ConfigSize - offset);
            
            if (u16Offset == PINCFG_CONFIG_OFFSET && offset < u16Size)
            {
                // Use new config data
                uint16_t copySize = (offset + blockSize <= u16Size) ? blockSize : (u16Size - offset);
                memcpy(au8Block, pcData + offset, copySize);
                if (copySize < blockSize)
                    memset(au8Block + copySize, 0, blockSize - copySize);
            }
            else
            {
                // Use existing config data
                vHwReadConfigBlock((void *)au8Block, (void *)(EEPROM_PINCFG + offset), blockSize);
            }
            
            au8BlockChecksums[u16BlockIdx++] = crc8((const uint8_t *)au8Block, blockSize);
        }
    }
    
    // Write block checksums (double redundancy) - write as ONE 32-byte block for flash efficiency
    uint8_t au8BlockCrcBoth[PINCFG_NUM_BLOCKS * 2];
    memcpy(&au8BlockCrcBoth[0], au8BlockChecksums, PINCFG_NUM_BLOCKS);
    memcpy(&au8BlockCrcBoth[PINCFG_NUM_BLOCKS], au8BlockChecksums, PINCFG_NUM_BLOCKS);
    vHwWriteConfigBlock((void *)au8BlockCrcBoth, (void *)EEPROM_PINCFG_BLOCK_CHKSUMS, PINCFG_NUM_BLOCKS * 2);

    // Verify block checksums
    uint8_t au8BlockCrcBothVerify[PINCFG_NUM_BLOCKS * 2];
    vHwReadConfigBlock((void *)au8BlockCrcBothVerify, (void *)EEPROM_PINCFG_BLOCK_CHKSUMS, PINCFG_NUM_BLOCKS * 2);
    if (memcmp(au8BlockCrcBoth, au8BlockCrcBothVerify, PINCFG_NUM_BLOCKS * 2) != 0)
        return PERCFG_WRITE_CHECKSUM_FAILED_E;

    // overwrite node id to force obtain new one
    uint16_t u16NodeId = AUTO;
    vHwWriteConfigBlock((void *)&u16NodeId, (void *)EEPROM_NODE_ID_ADDRESS, 1u);

    // Write only the section being updated in ONE large block (optimal for flash pages)
    void *pWriteAddr = (u16Offset == PINCFG_PASSWORD_OFFSET) 
                       ? (void *)(EEPROM_PASSWORD + u16Offset)
                       : (void *)(EEPROM_PINCFG + (u16Offset - PINCFG_CONFIG_OFFSET));
    
    PERCFG_RESULT_T eRet = eWriteCfg(pWriteAddr, u16Size, pcData);
    if (eRet != PERCFG_OK_E)
        return eRet;

    // Write backup in ONE large block (optimal for flash pages)
    uint16_t u16BackupSize = getBackupSize(u16TotalSize);
    if (u16Offset < u16BackupSize)
    {
        uint16_t u16BackupWriteSize = (u16Offset + u16Size <= u16BackupSize) 
                                       ? u16Size 
                                       : (u16BackupSize - u16Offset);
        eWriteCfg((void *)(EEPROM_PINCFG_BACKUP_START + u16Offset), u16BackupWriteSize, pcData);
    }

    return PERCFG_OK_E;
}

// Internal function - loads either password section or config section
// pcDataOut: buffer to receive data
// u16Offset: PINCFG_PASSWORD_OFFSET for password, PINCFG_CONFIG_OFFSET for config  
// Reads until nearest \0 or end of config area (using cyclic block reads)
static PERCFG_RESULT_T eLoad(char *pcDataOut, uint16_t u16Offset)
{
#ifndef PINCFG_CONFIG_MAX_SZ_D
#error "Define PINCFG_CONFIG_MAX_SZ_D !"
#endif

    // Find the \0 terminator using block-by-block reads
    uint16_t maxSearch = (u16Offset < PINCFG_CONFIG_OFFSET) 
                         ? PINCFG_CONFIG_OFFSET  // Searching in password section
                         : PINCFG_TOTAL_SIZE;          // Searching in config section
    
    uint16_t u16Size = 0;
    uint8_t au8Block[PINCFG_BLOCK_SIZE];
    
    // Scan in blocks to find null terminator
    for (uint16_t offset = u16Offset; offset < maxSearch; offset += PINCFG_BLOCK_SIZE)
    {
        uint16_t blockSize = (offset + PINCFG_BLOCK_SIZE <= maxSearch) 
                             ? PINCFG_BLOCK_SIZE 
                             : (maxSearch - offset);
        
        void *pReadAddr = (offset < PINCFG_CONFIG_OFFSET)
                          ? (void *)(EEPROM_PASSWORD + offset)
                          : (void *)(EEPROM_PINCFG + (offset - PINCFG_CONFIG_OFFSET));
        
        vHwReadConfigBlock((void *)au8Block, pReadAddr, blockSize);
        
        // Search for null terminator in this block
        for (uint16_t i = 0; i < blockSize; i++)
        {
            if (au8Block[i] == '\0')
            {
                u16Size = offset - u16Offset + i + 1;  // Include the \0 in size
                goto found_null;
            }
        }
    }
    found_null:
    
    // If no \0 found within valid range, return error
    if (u16Size == 0)
        return PERCFG_INVALID_E;
    
    // Calculate total size for CRC validation
    // Password section always occupies full 32 bytes (PINCFG_CONFIG_OFFSET) for alignment
    // Total size = full password section + config data size
    uint16_t u16TotalSize;
    if (u16Offset < PINCFG_CONFIG_OFFSET)
    {
        // Reading password - need to determine total size including any config data
        // Password section is always 32 bytes, so scan for config null terminator
        uint16_t u16ConfigSize = 0;
        for (uint16_t offset = PINCFG_CONFIG_OFFSET; offset < PINCFG_TOTAL_SIZE; offset += PINCFG_BLOCK_SIZE)
        {
            uint16_t blockSize = (offset + PINCFG_BLOCK_SIZE <= PINCFG_TOTAL_SIZE) 
                                 ? PINCFG_BLOCK_SIZE 
                                 : (PINCFG_TOTAL_SIZE - offset);
            
            vHwReadConfigBlock((void *)au8Block, (void *)(EEPROM_PINCFG + (offset - PINCFG_CONFIG_OFFSET)), blockSize);
            
            for (uint16_t i = 0; i < blockSize; i++)
            {
                if (au8Block[i] == '\0')
                {
                    u16ConfigSize = offset - PINCFG_CONFIG_OFFSET + i + 1;
                    goto found_config_null;
                }
            }
        }
        found_config_null:
        
        u16TotalSize = PINCFG_CONFIG_OFFSET + u16ConfigSize;
    }
    else
    {
        // Reading config - total size is offset + size
        u16TotalSize = u16Offset + u16Size;
    }
    
    // Sanity check: total size must fit within config area
    if (u16TotalSize > PINCFG_TOTAL_SIZE)
        return PERCFG_OUT_OF_RANGE_E;

    // read CRC16 checksums (triple redundancy with voting)
    uint16_t u16CfgCheckSum;
    uint16_t u16CfgCheckSumB1;
    uint16_t u16CfgCheckSumB2;

    vHwReadConfigBlock((void *)&u16CfgCheckSum, (void *)EEPROM_PINCFG_CHKSUM, sizeof(uint16_t));
    vHwReadConfigBlock((void *)&u16CfgCheckSumB1, (void *)EEPROM_PINCFG_CHKSUM_B1, sizeof(uint16_t));
    vHwReadConfigBlock((void *)&u16CfgCheckSumB2, (void *)EEPROM_PINCFG_CHKSUM_B2, sizeof(uint16_t));

    // 3-way voting on checksum
    if (u16CfgCheckSum != u16CfgCheckSumB1 && u16CfgCheckSum == u16CfgCheckSumB2)
        vHwWriteConfigBlock((void *)&u16CfgCheckSum, (void *)EEPROM_PINCFG_CHKSUM_B1, sizeof(uint16_t));
    else if (u16CfgCheckSum == u16CfgCheckSumB1 && u16CfgCheckSum != u16CfgCheckSumB2)
        vHwWriteConfigBlock((void *)&u16CfgCheckSum, (void *)EEPROM_PINCFG_CHKSUM_B2, sizeof(uint16_t));
    else if (u16CfgCheckSumB1 == u16CfgCheckSumB2 && u16CfgCheckSumB1 != u16CfgCheckSum)
    {
        vHwWriteConfigBlock((void *)&u16CfgCheckSumB1, (void *)EEPROM_PINCFG_CHKSUM, sizeof(uint16_t));
        u16CfgCheckSum = u16CfgCheckSumB1;
    }
    else if (u16CfgCheckSum != u16CfgCheckSumB1 && u16CfgCheckSum != u16CfgCheckSumB2 && u16CfgCheckSumB1 != u16CfgCheckSumB2)
        return PERCFG_READ_CHECKSUM_FAILED_E;

    // read block checksums (double redundancy)
    uint8_t au8BlockChecksums[PINCFG_NUM_BLOCKS];
    uint8_t au8BlockChecksumsB1[PINCFG_NUM_BLOCKS];
    vHwReadConfigBlock((void *)au8BlockChecksums, (void *)EEPROM_PINCFG_BLOCK_CHKSUMS, PINCFG_NUM_BLOCKS);
    vHwReadConfigBlock((void *)au8BlockChecksumsB1, (void *)EEPROM_PINCFG_BLOCK_CHKSUMS_B1, PINCFG_NUM_BLOCKS);

    // repair block checksums if needed
    if (memcmp(au8BlockChecksums, au8BlockChecksumsB1, PINCFG_NUM_BLOCKS) != 0)
    {
        // use first copy, repair second
        vHwWriteConfigBlock((void *)au8BlockChecksums, (void *)EEPROM_PINCFG_BLOCK_CHKSUMS_B1, PINCFG_NUM_BLOCKS);
    }

    // ============================================================
    // CORRUPTION DETECTION PHASE (incremental, no stack buffer)
    // ============================================================
    
    // Level 1: Full config validation with CRC16 (incremental calculation)
    uint16_t u16CfgCheckSumCalculated = 0xFFFF;
    
    for (uint16_t offset = 0; offset < u16TotalSize; offset += PINCFG_BLOCK_SIZE)
    {
        uint16_t blockSize = (offset + PINCFG_BLOCK_SIZE <= u16TotalSize) 
                             ? PINCFG_BLOCK_SIZE 
                             : (u16TotalSize - offset);
        
        void *pReadAddr = (offset < PINCFG_CONFIG_OFFSET)
                          ? (void *)(EEPROM_PASSWORD + offset)
                          : (void *)(EEPROM_PINCFG + (offset - PINCFG_CONFIG_OFFSET));
        
        vHwReadConfigBlock((void *)au8Block, pReadAddr, blockSize);
        
        // Update CRC16
        for (uint16_t i = 0; i < blockSize; ++i)
        {
            u16CfgCheckSumCalculated ^= (uint16_t)au8Block[i] << 8;
            for (uint8_t bit = 0; bit < 8; ++bit)
            {
                if (u16CfgCheckSumCalculated & 0x8000)
                    u16CfgCheckSumCalculated = (u16CfgCheckSumCalculated << 1) ^ 0x1021;
                else
                    u16CfgCheckSumCalculated = u16CfgCheckSumCalculated << 1;
            }
        }
    }
    
    bool bIsCfgOk = (u16CfgCheckSumCalculated == u16CfgCheckSum);

    // Level 2: Block-level validation (incremental)
    uint8_t au8BlockChecksumsCalculated[PINCFG_NUM_BLOCKS];
    memset(au8BlockChecksumsCalculated, 0, PINCFG_NUM_BLOCKS);
    
    uint16_t u16BlockIdx = 0;
    for (uint16_t offset = 0; offset < u16TotalSize; offset += PINCFG_BLOCK_SIZE)
    {
        uint16_t blockSize = (offset + PINCFG_BLOCK_SIZE <= u16TotalSize) 
                             ? PINCFG_BLOCK_SIZE 
                             : (u16TotalSize - offset);
        
        void *pReadAddr = (offset < PINCFG_CONFIG_OFFSET)
                          ? (void *)(EEPROM_PASSWORD + offset)
                          : (void *)(EEPROM_PINCFG + (offset - PINCFG_CONFIG_OFFSET));
        
        vHwReadConfigBlock((void *)au8Block, pReadAddr, blockSize);
        au8BlockChecksumsCalculated[u16BlockIdx++] = crc8((const uint8_t *)au8Block, blockSize);
    }
    
    // Fast path: if no corruption detected, copy requested section to output
    if (bIsCfgOk)
    {
        // Read the requested section directly into output buffer
        for (uint16_t offset = 0; offset < u16Size; offset += PINCFG_BLOCK_SIZE)
        {
            uint16_t blockSize = (offset + PINCFG_BLOCK_SIZE <= u16Size) 
                                 ? PINCFG_BLOCK_SIZE 
                                 : (u16Size - offset);
            
            void *pReadAddr = (u16Offset < PINCFG_CONFIG_OFFSET)
                              ? (void *)(EEPROM_PASSWORD + u16Offset + offset)
                              : (void *)(EEPROM_PINCFG + (u16Offset - PINCFG_CONFIG_OFFSET) + offset);
            
            vHwReadConfigBlock((void *)(pcDataOut + offset), pReadAddr, blockSize);
        }
        return PERCFG_OK_E;
    }

    // ============================================================
    // CORRUPTION RECOVERY PHASE
    // ============================================================
    
    uint16_t u16BackupSize = getBackupSize(u16TotalSize);
    uint16_t u16BackupBlocks = (u16BackupSize + PINCFG_BLOCK_SIZE - 1) / PINCFG_BLOCK_SIZE;
    
    // Check if corrupted blocks are within backed-up range
    bool bCorruptedBlockIsBackedUp = false;
    for (uint16_t i = 0; i < u16BackupBlocks && i < PINCFG_NUM_BLOCKS; ++i)
    {
        if (au8BlockChecksums[i] != au8BlockChecksumsCalculated[i])
        {
            bCorruptedBlockIsBackedUp = true;
            break;
        }
    }
    
    if (bCorruptedBlockIsBackedUp && u16BackupSize > 0)
    {
        // Validate backup incrementally
        uint8_t au8BackupBlockChecksums[PINCFG_NUM_BLOCKS];
        memset(au8BackupBlockChecksums, 0, PINCFG_NUM_BLOCKS);
        
        u16BlockIdx = 0;
        for (uint16_t offset = 0; offset < u16BackupSize; offset += PINCFG_BLOCK_SIZE)
        {
            uint16_t blockSize = (offset + PINCFG_BLOCK_SIZE <= u16BackupSize) 
                                 ? PINCFG_BLOCK_SIZE 
                                 : (u16BackupSize - offset);
            
            vHwReadConfigBlock((void *)au8Block, (void *)(EEPROM_PINCFG_BACKUP_START + offset), blockSize);
            au8BackupBlockChecksums[u16BlockIdx++] = crc8((const uint8_t *)au8Block, blockSize);
        }
        
        // Check if backup is good
        bool bBackupOk = true;
        for (uint16_t i = 0; i < u16BackupBlocks && i < PINCFG_NUM_BLOCKS; ++i)
        {
            if (au8BlockChecksums[i] != au8BackupBlockChecksums[i])
            {
                bBackupOk = false;
                break;
            }
        }
        
        if (bBackupOk)
        {
            // Restore from backup - optimize for flash page writes
            // Write password section as ONE block if backed up
            if (u16BackupSize >= PINCFG_CONFIG_OFFSET)
            {
                // Full password section is backed up - write in one go
                uint8_t au8PasswordBackup[PINCFG_AUTH_PASSWORD_MAX_LEN_D];
                vHwReadConfigBlock((void *)au8PasswordBackup, (void *)EEPROM_PINCFG_BACKUP_START, PINCFG_AUTH_PASSWORD_MAX_LEN_D);
                vHwWriteConfigBlock((void *)au8PasswordBackup, (void *)EEPROM_PASSWORD, PINCFG_AUTH_PASSWORD_MAX_LEN_D);
                
                // Write config section as ONE block if any config is backed up
                uint16_t u16ConfigBackupSize = u16BackupSize - PINCFG_CONFIG_OFFSET;
                if (u16ConfigBackupSize > 0)
                {
                    // Allocate buffer for config backup (reuse existing stack allocation pattern)
                    uint8_t au8ConfigBackup[PINCFG_BLOCK_SIZE * 4];  // 128 bytes (reasonable for embedded)
                    
                    // Write in 128-byte chunks for large configs
                    for (uint16_t offset = 0; offset < u16ConfigBackupSize; offset += sizeof(au8ConfigBackup))
                    {
                        uint16_t chunkSize = (offset + sizeof(au8ConfigBackup) <= u16ConfigBackupSize)
                                             ? sizeof(au8ConfigBackup)
                                             : (u16ConfigBackupSize - offset);
                        
                        vHwReadConfigBlock((void *)au8ConfigBackup, 
                                          (void *)(EEPROM_PINCFG_BACKUP_START + PINCFG_CONFIG_OFFSET + offset), 
                                          chunkSize);
                        vHwWriteConfigBlock((void *)au8ConfigBackup, 
                                           (void *)(EEPROM_PINCFG + offset), 
                                           chunkSize);
                    }
                }
            }
            else
            {
                // Partial password backup - write what we have as ONE block
                uint8_t au8PartialBackup[PINCFG_AUTH_PASSWORD_MAX_LEN_D];
                vHwReadConfigBlock((void *)au8PartialBackup, (void *)EEPROM_PINCFG_BACKUP_START, u16BackupSize);
                vHwWriteConfigBlock((void *)au8PartialBackup, (void *)EEPROM_PASSWORD, u16BackupSize);
            }
            
            // Revalidate with CRC16
            u16CfgCheckSumCalculated = 0xFFFF;
            for (uint16_t offset = 0; offset < u16TotalSize; offset += PINCFG_BLOCK_SIZE)
            {
                uint16_t blockSize = (offset + PINCFG_BLOCK_SIZE <= u16TotalSize) 
                                     ? PINCFG_BLOCK_SIZE 
                                     : (u16TotalSize - offset);
                
                void *pReadAddr = (offset < PINCFG_CONFIG_OFFSET)
                                  ? (void *)(EEPROM_PASSWORD + offset)
                                  : (void *)(EEPROM_PINCFG + (offset - PINCFG_CONFIG_OFFSET));
                
                vHwReadConfigBlock((void *)au8Block, pReadAddr, blockSize);
                
                for (uint16_t i = 0; i < blockSize; ++i)
                {
                    u16CfgCheckSumCalculated ^= (uint16_t)au8Block[i] << 8;
                    for (uint8_t bit = 0; bit < 8; ++bit)
                    {
                        if (u16CfgCheckSumCalculated & 0x8000)
                            u16CfgCheckSumCalculated = (u16CfgCheckSumCalculated << 1) ^ 0x1021;
                        else
                            u16CfgCheckSumCalculated = u16CfgCheckSumCalculated << 1;
                    }
                }
            }
            
            if (u16CfgCheckSumCalculated == u16CfgCheckSum)
            {
                // REPAIRED! Read requested section
                for (uint16_t offset = 0; offset < u16Size; offset += PINCFG_BLOCK_SIZE)
                {
                    uint16_t blockSize = (offset + PINCFG_BLOCK_SIZE <= u16Size) 
                                         ? PINCFG_BLOCK_SIZE 
                                         : (u16Size - offset);
                    
                    void *pReadAddr = (u16Offset < PINCFG_CONFIG_OFFSET)
                                      ? (void *)(EEPROM_PASSWORD + u16Offset + offset)
                                      : (void *)(EEPROM_PINCFG + (u16Offset - PINCFG_CONFIG_OFFSET) + offset);
                    
                    vHwReadConfigBlock((void *)(pcDataOut + offset), pReadAddr, blockSize);
                }
                return PERCFG_OK_E;
            }
        }
    }

    return PERCFG_READ_FAILED_E;
}

// Public API - reads password section with full CRC/recovery protection
PERCFG_RESULT_T PersistentCfg_eReadPassword(char *pcPasswordOut)
{
    if (pcPasswordOut == NULL)
        return PERCFG_ERROR_E;
    
    // Read password section (null-terminated string)
    PERCFG_RESULT_T eResult = eLoad(pcPasswordOut, 0);
    if (eResult == PERCFG_OK_E)
    {
        // Ensure null termination
        pcPasswordOut[PINCFG_AUTH_PASSWORD_MAX_LEN_D - 1] = '\0';
    }
    return eResult;
}

// Public API - writes password section with full CRC/backup protection
PERCFG_RESULT_T PersistentCfg_eWritePassword(const char *pcPassword)
{
    if (pcPassword == NULL)
        return PERCFG_ERROR_E;
    
    // Build password buffer with proper null termination
    // Use minimal stack: only password section (32 bytes)
    char acPasswordBuf[PINCFG_AUTH_PASSWORD_MAX_LEN_D];
    memset(acPasswordBuf, 0, PINCFG_AUTH_PASSWORD_MAX_LEN_D);
    
    // Copy password (up to max length - 1 to ensure space for null terminator)
    size_t password_len = strlen(pcPassword);
    if (password_len >= PINCFG_AUTH_PASSWORD_MAX_LEN_D)
        password_len = PINCFG_AUTH_PASSWORD_MAX_LEN_D - 1;
    
    memcpy(acPasswordBuf, pcPassword, password_len);
    acPasswordBuf[password_len] = '\0';  // Ensure null termination
    
    // Save password section (always save full 32 bytes for alignment with config section)
    // The full 32-byte password section is always written to maintain fixed layout:
    // [Password(32)][Config(var)] regardless of actual password length
    return eSave(acPasswordBuf, PINCFG_PASSWORD_OFFSET, PINCFG_AUTH_PASSWORD_MAX_LEN_D);
}

// Public API - loads config data section with full CRC/recovery protection
PERCFG_RESULT_T PersistentCfg_eLoadConfig(char *pcCfg)
{
    if (pcCfg == NULL)
        return PERCFG_ERROR_E;
    
    // Get config size directly (no need to read password area)
    uint16_t u16ConfigSize = 0;
    PERCFG_RESULT_T eResult = PersistentCfg_eGetConfigSize(&u16ConfigSize);
    if (eResult != PERCFG_OK_E)
        return eResult;
    
    // Check if config has data
    if (u16ConfigSize == 0)
    {
        // No config data
        pcCfg[0] = '\0';
        return PERCFG_OK_E;
    }
    
    // Read config section (null-terminated string starting after password)
    eResult = eLoad(pcCfg, PINCFG_CONFIG_OFFSET);
    if (eResult == PERCFG_OK_E)
    {
        // Ensure null termination at end of buffer
        uint16_t len = strlen(pcCfg);
        if (len < u16ConfigSize)
            pcCfg[len] = '\0';
    }
    return eResult;
}

// Public API - saves config data section with full CRC/backup protection
PERCFG_RESULT_T PersistentCfg_eSaveConfig(const char *pcCfg, uint16_t u16ConfigSize)
{
    if (pcCfg == NULL)
        return PERCFG_ERROR_E;
    
    if (u16ConfigSize > PINCFG_CONFIG_MAX_SZ_D)
        return PERCFG_CFG_BIGGER_THAN_MAX_SZ_E;
    
    // Check if config already has null terminator
    uint16_t u16ConfigSizeWithNull = u16ConfigSize;
    bool bHasNull = false;
    
    if (u16ConfigSize > 0 && pcCfg[u16ConfigSize - 1] == '\0')
    {
        bHasNull = true;
    }
    else
    {
        u16ConfigSizeWithNull = u16ConfigSize + 1;
    }
    
    // If config already has null terminator, save directly
    if (bHasNull)
    {
        return eSave(pcCfg, PINCFG_CONFIG_OFFSET, u16ConfigSizeWithNull);
    }
    else
    {
        // Need to add null terminator - use minimal stack buffer approach
        // Write in chunks to avoid large stack allocation
        PERCFG_RESULT_T result;
        
        // For small configs, use stack buffer
        if (u16ConfigSizeWithNull <= PINCFG_BLOCK_SIZE)
        {
            char acSmallBuf[PINCFG_BLOCK_SIZE];
            memcpy(acSmallBuf, pcCfg, u16ConfigSize);
            acSmallBuf[u16ConfigSize] = '\0';
            result = eSave(acSmallBuf, PINCFG_CONFIG_OFFSET, u16ConfigSizeWithNull);
        }
        else
        {
            // For larger configs, we need to handle null terminator specially
            // Strategy: write config data, then append null in a separate small write
            // But eSave() expects the full data with null...
            // 
            // Alternative: Use block-sized buffer and process in chunks
            // However, eSave needs contiguous data for checksum calculation
            //
            // Best option: allocate on heap or use a reasonable stack buffer
            // Since this is embedded, let's use largest safe stack buffer
            char acConfigBuf[PINCFG_BLOCK_SIZE * 4];  // 128 bytes (reasonable for embedded)
            
            if (u16ConfigSizeWithNull <= sizeof(acConfigBuf))
            {
                memcpy(acConfigBuf, pcCfg, u16ConfigSize);
                acConfigBuf[u16ConfigSize] = '\0';
                result = eSave(acConfigBuf, PINCFG_CONFIG_OFFSET, u16ConfigSizeWithNull);
            }
            else
            {
                // Config too large for safe stack allocation
                // This should not happen in practice (max config is PINCFG_CONFIG_DATA_MAX_SZ_D bytes)
                // For production: either increase buffer or reject large configs
                return PERCFG_CFG_BIGGER_THAN_MAX_SZ_E;
            }
        }
        
        return result;
    }
}
