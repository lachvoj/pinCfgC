#ifdef UNIT_TEST
#include "EEPROMMock.h"
#else
#include <core/MyEepromAddresses.h>
#endif

#include <stdint.h>
#include <string.h>

#include "PersistentConfiguration.h"

#define EEPROM_TOTAL_SIZE 1024U

// Total config size used internally (password + config data)
#define PINCFG_TOTAL_SIZE (PINCFG_AUTH_PASSWORD_LEN_D + PINCFG_CONFIG_MAX_SZ_D)

// Block-level checksum configuration
#define PINCFG_BLOCK_SIZE 32U
#define PINCFG_MAX_NUM_BLOCKS ((PINCFG_TOTAL_SIZE + PINCFG_BLOCK_SIZE - 1) / PINCFG_BLOCK_SIZE)

// EEPROM Memory Layout - DYNAMIC backup strategy
// Total available: 611 bytes (1024 - 413 MySensors reserved)
// Dynamic layout: Block checksums and backup area positioned based on actual config size
// Layout: [MySensors(413)][CRC16×3(6)][Password(32)][Config(N)][BlockCRC×2(K)][Backup(REST)]
// Where: N = actual config size, K = 2 × ceil((32+N)/32) block CRCs, REST = remaining space

// Data section offsets (relative to start of data area, not EEPROM address)
#define PINCFG_PASSWORD_OFFSET 0U
#define PINCFG_CONFIG_OFFSET PINCFG_AUTH_PASSWORD_LEN_D

// EEPROM absolute addresses (fixed portion - start immediately after MySensors)
#define EEPROM_PINCFG_CHKSUM (EEPROM_LOCAL_CONFIG_ADDRESS)           // Start at 413 (no gap!)
#define EEPROM_PINCFG_CHKSUM_B1 (EEPROM_PINCFG_CHKSUM + (2U))        // CRC16 backup 1
#define EEPROM_PINCFG_CHKSUM_B2 (EEPROM_PINCFG_CHKSUM_B1 + (2U))     // CRC16 backup 2
#define EEPROM_PASSWORD (EEPROM_PINCFG_CHKSUM_B2 + (2U))             // Password section start
#define EEPROM_PINCFG (EEPROM_PASSWORD + PINCFG_AUTH_PASSWORD_LEN_D) // Config data start

// Dynamic address calculation helpers
// u16TotalSize = password section (32) + config data size (including null terminator)
static inline uint16_t getNumBlocks(uint16_t u16TotalSize)
{
    return (u16TotalSize + PINCFG_BLOCK_SIZE - 1) / PINCFG_BLOCK_SIZE;
}

static inline uint16_t getBlockChecksumsAddr(uint16_t u16TotalSize)
{
    // Block checksums start immediately after config data
    // Address = EEPROM_PINCFG + (u16TotalSize - password_size)
    uint16_t u16ConfigSize = (u16TotalSize > PINCFG_CONFIG_OFFSET) ? (u16TotalSize - PINCFG_CONFIG_OFFSET) : 0;
    return EEPROM_PINCFG + u16ConfigSize;
}

static inline uint16_t getBlockChecksumsB1Addr(uint16_t u16TotalSize)
{
    return getBlockChecksumsAddr(u16TotalSize) + getNumBlocks(u16TotalSize);
}

static inline uint16_t getBackupStartAddr(uint16_t u16TotalSize)
{
    // Backup starts after both copies of block checksums
    return getBlockChecksumsB1Addr(u16TotalSize) + getNumBlocks(u16TotalSize);
}

static inline uint16_t getMaxBackupSize(uint16_t u16TotalSize)
{
    uint16_t backupStart = getBackupStartAddr(u16TotalSize);
    return (backupStart < EEPROM_TOTAL_SIZE) ? (EEPROM_TOTAL_SIZE - backupStart) : 0;
}

// Forward declarations for internal functions
static PERCFG_RESULT_T eSave(const char *pcData, uint16_t u16Offset, uint16_t u16Size);
static PERCFG_RESULT_T eLoad(char *pcDataOut, uint16_t u16Offset);

// Helper function: Scan EEPROM region for null terminator
// Returns size INCLUDING null terminator, or 0 if not found
static uint16_t scanForNull(uint16_t u16StartAddr, uint16_t u16MaxSize)
{
    uint8_t au8Block[PINCFG_BLOCK_SIZE];

    for (uint16_t offset = 0; offset < u16MaxSize; offset += PINCFG_BLOCK_SIZE)
    {
        uint16_t blockSize = (offset + PINCFG_BLOCK_SIZE <= u16MaxSize) ? PINCFG_BLOCK_SIZE : (u16MaxSize - offset);

        vHwReadConfigBlock((void *)au8Block, (void *)(uintptr_t)(u16StartAddr + offset), blockSize);

        for (uint16_t i = 0; i < blockSize; i++)
        {
            if (au8Block[i] == '\0')
                return offset + i + 1; // Size including null terminator
        }
    }

    return 0; // Not found
}

// Calculate how much of config can be backed up (whatever fits in remaining EEPROM)
// With dynamic layout, backup size = min(totalSize, availableSpace)
static inline uint16_t getBackupSize(uint16_t u16TotalSize)
{
    // Backup as much as possible, limited by:
    // 1. Actual total size (password + config)
    // 2. Available EEPROM space after dynamic block checksums
    uint16_t u16MaxBackup = getMaxBackupSize(u16TotalSize);
    return (u16TotalSize <= u16MaxBackup) ? u16TotalSize : u16MaxBackup;
}

// Scan EEPROM config area to find config size (ends at first \0)
// Returns size of actual config data (NOT including null terminator) via pointer
// Returns 0 for empty config (just null terminator)
PERCFG_RESULT_T PersistentCfg_eGetConfigSize(uint16_t *pu16ConfigSize)
{
    if (pu16ConfigSize == NULL)
        return PERCFG_ERROR_E;

    // Scan only the config area using helper function
    uint16_t u16SizeWithNull = scanForNull(EEPROM_PINCFG, PINCFG_CONFIG_MAX_SZ_D);

    if (u16SizeWithNull > 0)
    {
        // Return size of actual data (NOT including null terminator)
        *pu16ConfigSize = u16SizeWithNull - 1;
    }
    else
    {
        // No null found in config section - no valid config
        *pu16ConfigSize = 0;
    }

    return PERCFG_OK_E;
}

// CRC16-CCITT (polynomial 0x1021) - detects all 1-2 bit errors and most burst errors
// Incremental update: process one byte at a time
static inline void crc16_update(uint16_t *pu16Crc, uint8_t u8Byte)
{
    *pu16Crc ^= (uint16_t)u8Byte << 8;
    for (uint8_t bit = 0; bit < 8; ++bit)
    {
        if (*pu16Crc & 0x8000)
            *pu16Crc = (*pu16Crc << 1) ^ 0x1021;
        else
            *pu16Crc = *pu16Crc << 1;
    }
}

// Block CRC16 calculation using incremental function
uint16_t crc16(const uint8_t *data, uint16_t dataSize)
{
    uint16_t crc = 0xFFFF;
    for (uint16_t i = 0; i < dataSize; ++i)
        crc16_update(&crc, data[i]);
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
        uint16_t blockSize = (offset + PINCFG_BLOCK_SIZE <= u16CfgSize) ? PINCFG_BLOCK_SIZE : (u16CfgSize - offset);
        pu8BlockChecksums[u16BlockIdx++] = crc8((const uint8_t *)(pcCfg + offset), blockSize);
    }
}

// Write config to EEPROM optimized for flash paging
// Writes entire data in ONE operation for optimal flash page utilization
PERCFG_RESULT_T eWriteCfg(void *iEePos, uint16_t u16CfgSize, const char *pcCfg)
{
    // For flash-backed EEPROM: write entire block at once to maximize page efficiency
    // This is optimal for systems with flash page writes (typically 32-256 bytes)
    vHwWriteConfigBlock((void *)pcCfg, iEePos, u16CfgSize);

    // Verify entire write in blocks (read is fast, write is expensive)
    uint16_t u16Offset = 0;
    uint8_t au8Block[PINCFG_BLOCK_SIZE];

    while (u16Offset < u16CfgSize)
    {
        uint16_t u16BlockSize =
            (u16Offset + PINCFG_BLOCK_SIZE <= u16CfgSize) ? PINCFG_BLOCK_SIZE : (u16CfgSize - u16Offset);

        vHwReadConfigBlock((void *)au8Block, (void *)((uint8_t *)iEePos + u16Offset), u16BlockSize);

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
        u16ConfigSize = scanForNull(EEPROM_PINCFG, PINCFG_CONFIG_MAX_SZ_D);

        // If no config exists (fresh EEPROM or never saved config), we need to
        // initialize config area with a null terminator for consistent dynamic layout
        if (u16ConfigSize == 0)
        {
            // Write null terminator to config area start to mark "empty config"
            uint8_t u8Null = 0;
            vHwWriteConfigBlock((void *)&u8Null, (void *)(uintptr_t)EEPROM_PINCFG, 1);
            u16ConfigSize = 1; // Empty config is just the null terminator
        }

        u16TotalSize = PINCFG_CONFIG_OFFSET + u16ConfigSize;
    }
    else
    {
        // Updating config - use new config size
        // Password section is always PINCFG_CONFIG_OFFSET bytes (32) for alignment
        u16ConfigSize = u16Size;
        u16TotalSize = PINCFG_CONFIG_OFFSET + u16ConfigSize;
    }

    // ============================================================
    // SINGLE-PASS CHECKSUM CALCULATION (CRC16 + block CRC8s together)
    // Eliminates duplicate EEPROM reads for 50% read reduction
    // ============================================================
    uint16_t u16CfgCheckSum = 0xFFFF;
    uint8_t au8Block[PINCFG_BLOCK_SIZE];
    uint16_t u16NumBlocks = getNumBlocks(u16TotalSize);
    uint8_t au8BlockChecksums[PINCFG_MAX_NUM_BLOCKS];
    memset(au8BlockChecksums, 0, u16NumBlocks);
    uint16_t u16BlockIdx = 0;

    // Process password section - calculate both CRC16 and block CRC8 in one pass
    for (uint16_t offset = 0; offset < PINCFG_CONFIG_OFFSET; offset += PINCFG_BLOCK_SIZE)
    {
        uint16_t blockSize =
            (offset + PINCFG_BLOCK_SIZE <= PINCFG_CONFIG_OFFSET) ? PINCFG_BLOCK_SIZE : (PINCFG_CONFIG_OFFSET - offset);

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
            vHwReadConfigBlock((void *)au8Block, (void *)(uintptr_t)(EEPROM_PASSWORD + offset), blockSize);
        }

        // Calculate block CRC8
        au8BlockChecksums[u16BlockIdx++] = crc8((const uint8_t *)au8Block, blockSize);

        // Update CRC16 incrementally
        for (uint16_t i = 0; i < blockSize; ++i)
            crc16_update(&u16CfgCheckSum, au8Block[i]);
    }

    // Process config section - calculate both CRC16 and block CRC8 in one pass
    if (u16ConfigSize > 0)
    {
        for (uint16_t offset = 0; offset < u16ConfigSize; offset += PINCFG_BLOCK_SIZE)
        {
            uint16_t blockSize =
                (offset + PINCFG_BLOCK_SIZE <= u16ConfigSize) ? PINCFG_BLOCK_SIZE : (u16ConfigSize - offset);

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
                vHwReadConfigBlock((void *)au8Block, (void *)(uintptr_t)(EEPROM_PINCFG + offset), blockSize);
            }

            // Calculate block CRC8
            au8BlockChecksums[u16BlockIdx++] = crc8((const uint8_t *)au8Block, blockSize);

            // Update CRC16 incrementally
            for (uint16_t i = 0; i < blockSize; ++i)
                crc16_update(&u16CfgCheckSum, au8Block[i]);
        }
    }

    // ============================================================
    // ATOMIC WRITE ORDER: Data → Backup → Block CRCs → CRC16
    // This ensures consistent state on power failure:
    // - If power fails before CRC16: old CRC won't match new data → load fails safely
    // - If power fails after CRC16: everything consistent → load succeeds
    // ============================================================

    // overwrite node id to force obtain new one
    uint16_t u16NodeId = AUTO;
    vHwWriteConfigBlock((void *)&u16NodeId, (void *)(uintptr_t)EEPROM_NODE_ID_ADDRESS, 1u);

    // STEP 1: Write actual data FIRST
    void *pWriteAddr = (u16Offset == PINCFG_PASSWORD_OFFSET)
                           ? (void *)(uintptr_t)(EEPROM_PASSWORD + u16Offset)
                           : (void *)(uintptr_t)(EEPROM_PINCFG + (u16Offset - PINCFG_CONFIG_OFFSET));

    PERCFG_RESULT_T eRet = eWriteCfg(pWriteAddr, u16Size, pcData);
    if (eRet != PERCFG_OK_E)
        return eRet;

    // STEP 2: Write backup
    uint16_t u16BackupStartAddr = getBackupStartAddr(u16TotalSize);
    uint16_t u16BackupSize = getBackupSize(u16TotalSize);

    // Write password section to backup (always at offset 0 in backup)
    if (u16BackupSize >= PINCFG_CONFIG_OFFSET)
    {
        // Full password fits in backup
        if (u16Offset == PINCFG_PASSWORD_OFFSET)
        {
            // We're updating password - use new data
            eWriteCfg((void *)(uintptr_t)u16BackupStartAddr, u16Size, pcData);
            // Zero-fill rest of password section if needed
            if (u16Size < PINCFG_CONFIG_OFFSET)
            {
                uint8_t zeros[PINCFG_BLOCK_SIZE];
                memset(zeros, 0, PINCFG_BLOCK_SIZE);
                for (uint16_t off = u16Size; off < PINCFG_CONFIG_OFFSET; off += PINCFG_BLOCK_SIZE)
                {
                    uint16_t sz = (off + PINCFG_BLOCK_SIZE <= PINCFG_CONFIG_OFFSET) ? PINCFG_BLOCK_SIZE
                                                                                    : (PINCFG_CONFIG_OFFSET - off);
                    eWriteCfg((void *)(uintptr_t)(u16BackupStartAddr + off), sz, (char *)zeros);
                }
            }
        }
        else
        {
            // We're updating config - copy existing password to backup
            for (uint16_t off = 0; off < PINCFG_CONFIG_OFFSET; off += PINCFG_BLOCK_SIZE)
            {
                uint16_t sz = PINCFG_BLOCK_SIZE;
                vHwReadConfigBlock((void *)au8Block, (void *)(uintptr_t)(EEPROM_PASSWORD + off), sz);
                eWriteCfg((void *)(uintptr_t)(u16BackupStartAddr + off), sz, (char *)au8Block);
            }
        }

        // Write config section to backup (at offset PINCFG_CONFIG_OFFSET in backup)
        if (u16ConfigSize > 0 && u16BackupSize > PINCFG_CONFIG_OFFSET)
        {
            uint16_t u16ConfigBackupMax = u16BackupSize - PINCFG_CONFIG_OFFSET;
            uint16_t u16ConfigBackupSize = (u16ConfigSize <= u16ConfigBackupMax) ? u16ConfigSize : u16ConfigBackupMax;

            if (u16Offset == PINCFG_CONFIG_OFFSET)
            {
                // We're updating config - use new data
                eWriteCfg((void *)(uintptr_t)(u16BackupStartAddr + PINCFG_CONFIG_OFFSET), u16ConfigBackupSize, pcData);
            }
            else
            {
                // We're updating password - copy existing config to backup
                for (uint16_t off = 0; off < u16ConfigBackupSize; off += PINCFG_BLOCK_SIZE)
                {
                    uint16_t sz = (off + PINCFG_BLOCK_SIZE <= u16ConfigBackupSize) ? PINCFG_BLOCK_SIZE
                                                                                   : (u16ConfigBackupSize - off);
                    vHwReadConfigBlock((void *)au8Block, (void *)(uintptr_t)(EEPROM_PINCFG + off), sz);
                    eWriteCfg(
                        (void *)(uintptr_t)(u16BackupStartAddr + PINCFG_CONFIG_OFFSET + off), sz, (char *)au8Block);
                }
            }
        }
    }
    else if (u16BackupSize > 0)
    {
        // Partial password backup only
        if (u16Offset == PINCFG_PASSWORD_OFFSET)
        {
            uint16_t u16BackupWriteSize = (u16Size <= u16BackupSize) ? u16Size : u16BackupSize;
            eWriteCfg((void *)(uintptr_t)u16BackupStartAddr, u16BackupWriteSize, pcData);
        }
        else
        {
            // Copy existing password (partial) to backup
            vHwReadConfigBlock((void *)au8Block, (void *)(uintptr_t)EEPROM_PASSWORD, u16BackupSize);
            eWriteCfg((void *)(uintptr_t)u16BackupStartAddr, u16BackupSize, (char *)au8Block);
        }
    }

    // STEP 3: Write block checksums (double redundancy) using dynamic addresses
    uint16_t u16BlockChkAddr = getBlockChecksumsAddr(u16TotalSize);
    uint16_t u16BlockChkB1Addr = getBlockChecksumsB1Addr(u16TotalSize);

    uint8_t au8BlockCrcBoth[PINCFG_MAX_NUM_BLOCKS * 2];
    memcpy(&au8BlockCrcBoth[0], au8BlockChecksums, u16NumBlocks);
    memcpy(&au8BlockCrcBoth[u16NumBlocks], au8BlockChecksums, u16NumBlocks);
    vHwWriteConfigBlock((void *)au8BlockCrcBoth, (void *)(uintptr_t)u16BlockChkAddr, u16NumBlocks * 2);

    // Verify block checksums
    vHwReadConfigBlock((void *)au8Block, (void *)(uintptr_t)u16BlockChkAddr, u16NumBlocks);
    if (memcmp(au8BlockChecksums, au8Block, u16NumBlocks) != 0)
        return PERCFG_WRITE_CHECKSUM_FAILED_E;
    vHwReadConfigBlock((void *)au8Block, (void *)(uintptr_t)u16BlockChkB1Addr, u16NumBlocks);
    if (memcmp(au8BlockChecksums, au8Block, u16NumBlocks) != 0)
        return PERCFG_WRITE_CHECKSUM_FAILED_E;

    // STEP 4: Write CRC16 checksum LAST (triple redundancy)
    // This is the final "commit" - if CRC matches, data is valid
    uint8_t au8CrcBlock[6];
    memcpy(&au8CrcBlock[0], &u16CfgCheckSum, sizeof(uint16_t));
    memcpy(&au8CrcBlock[2], &u16CfgCheckSum, sizeof(uint16_t));
    memcpy(&au8CrcBlock[4], &u16CfgCheckSum, sizeof(uint16_t));
    vHwWriteConfigBlock((void *)au8CrcBlock, (void *)EEPROM_PINCFG_CHKSUM, 6);

    // Verify CRC16 checksums
    vHwReadConfigBlock((void *)au8Block, (void *)(uintptr_t)EEPROM_PINCFG_CHKSUM, 6);
    if (memcmp(au8CrcBlock, au8Block, 6) != 0)
        return PERCFG_WRITE_CHECKSUM_FAILED_E;

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

    uint8_t au8Block[PINCFG_BLOCK_SIZE];
    uint16_t u16Size;

    // Determine size to read based on what section we're reading
    if (u16Offset < PINCFG_CONFIG_OFFSET)
    {
        // Reading password section - fixed size binary hash (no null terminator)
        u16Size = PINCFG_AUTH_PASSWORD_LEN_D;
    }
    else
    {
        // Reading config section - find the \0 terminator using helper function
        uint16_t u16StartAddr = EEPROM_PINCFG + (u16Offset - PINCFG_CONFIG_OFFSET);
        uint16_t u16MaxSearch = PINCFG_TOTAL_SIZE - u16Offset;

        u16Size = scanForNull(u16StartAddr, u16MaxSearch);

        // If no \0 found within valid range, return error
        if (u16Size == 0)
            return PERCFG_INVALID_E;
    }

    // Calculate total size for CRC validation
    // Password section always occupies full 32 bytes (PINCFG_CONFIG_OFFSET) for alignment
    // Total size = full password section + config data size
    uint16_t u16TotalSize;
    if (u16Offset < PINCFG_CONFIG_OFFSET)
    {
        // Reading password - need to determine total size including any config data
        // Password section is always 32 bytes, so scan for config null terminator
        uint16_t u16ConfigSize = scanForNull(EEPROM_PINCFG, PINCFG_CONFIG_MAX_SZ_D);
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

    vHwReadConfigBlock((void *)&u16CfgCheckSum, (void *)(uintptr_t)EEPROM_PINCFG_CHKSUM, sizeof(uint16_t));
    vHwReadConfigBlock((void *)&u16CfgCheckSumB1, (void *)(uintptr_t)EEPROM_PINCFG_CHKSUM_B1, sizeof(uint16_t));
    vHwReadConfigBlock((void *)&u16CfgCheckSumB2, (void *)(uintptr_t)EEPROM_PINCFG_CHKSUM_B2, sizeof(uint16_t));

    // 3-way voting on checksum
    if (u16CfgCheckSum != u16CfgCheckSumB1 && u16CfgCheckSum == u16CfgCheckSumB2)
        vHwWriteConfigBlock((void *)&u16CfgCheckSum, (void *)(uintptr_t)EEPROM_PINCFG_CHKSUM_B1, sizeof(uint16_t));
    else if (u16CfgCheckSum == u16CfgCheckSumB1 && u16CfgCheckSum != u16CfgCheckSumB2)
        vHwWriteConfigBlock((void *)&u16CfgCheckSum, (void *)(uintptr_t)EEPROM_PINCFG_CHKSUM_B2, sizeof(uint16_t));
    else if (u16CfgCheckSumB1 == u16CfgCheckSumB2 && u16CfgCheckSumB1 != u16CfgCheckSum)
    {
        vHwWriteConfigBlock((void *)&u16CfgCheckSumB1, (void *)(uintptr_t)EEPROM_PINCFG_CHKSUM, sizeof(uint16_t));
        u16CfgCheckSum = u16CfgCheckSumB1;
    }
    else if (
        u16CfgCheckSum != u16CfgCheckSumB1 && u16CfgCheckSum != u16CfgCheckSumB2 &&
        u16CfgCheckSumB1 != u16CfgCheckSumB2)
        return PERCFG_READ_CHECKSUM_FAILED_E;

    // Calculate dynamic addresses for block checksums based on total size
    uint16_t u16NumBlocks = getNumBlocks(u16TotalSize);
    uint16_t u16BlockChkAddr = getBlockChecksumsAddr(u16TotalSize);
    uint16_t u16BlockChkB1Addr = getBlockChecksumsB1Addr(u16TotalSize);

    // read block checksums (double redundancy) from dynamic addresses
    uint8_t au8BlockChecksums[PINCFG_MAX_NUM_BLOCKS];
    uint8_t au8BlockChecksumsB1[PINCFG_MAX_NUM_BLOCKS];
    vHwReadConfigBlock((void *)au8BlockChecksums, (void *)(uintptr_t)u16BlockChkAddr, u16NumBlocks);
    vHwReadConfigBlock((void *)au8BlockChecksumsB1, (void *)(uintptr_t)u16BlockChkB1Addr, u16NumBlocks);

    // ============================================================
    // SINGLE-PASS VALIDATION: Calculate CRC16 and block CRC8s together
    // This eliminates duplicate EEPROM reads (50% read reduction)
    // ============================================================

    uint16_t u16CfgCheckSumCalculated = 0xFFFF;
    uint8_t au8BlockChecksumsCalculated[PINCFG_MAX_NUM_BLOCKS];
    memset(au8BlockChecksumsCalculated, 0, u16NumBlocks);
    uint16_t u16BlockIdx = 0;

    for (uint16_t offset = 0; offset < u16TotalSize; offset += PINCFG_BLOCK_SIZE)
    {
        uint16_t blockSize = (offset + PINCFG_BLOCK_SIZE <= u16TotalSize) ? PINCFG_BLOCK_SIZE : (u16TotalSize - offset);

        void *pReadAddr = (offset < PINCFG_CONFIG_OFFSET)
                              ? (void *)(uintptr_t)(EEPROM_PASSWORD + offset)
                              : (void *)(uintptr_t)(EEPROM_PINCFG + (offset - PINCFG_CONFIG_OFFSET));

        vHwReadConfigBlock((void *)au8Block, pReadAddr, blockSize);

        // Calculate block CRC8
        au8BlockChecksumsCalculated[u16BlockIdx++] = crc8((const uint8_t *)au8Block, blockSize);

        // Update CRC16 incrementally
        for (uint16_t i = 0; i < blockSize; ++i)
            crc16_update(&u16CfgCheckSumCalculated, au8Block[i]);
    }

    bool bIsCfgOk = (u16CfgCheckSumCalculated == u16CfgCheckSum);

    // ============================================================
    // VALIDATED BLOCK CHECKSUM REPAIR
    // Compare both stored copies against calculated values from actual data
    // Only repair if we can determine which copy is correct
    // ============================================================
    if (memcmp(au8BlockChecksums, au8BlockChecksumsB1, u16NumBlocks) != 0)
    {
        // Stored copies differ - determine which matches calculated values
        bool bFirstMatchesCalculated = (memcmp(au8BlockChecksums, au8BlockChecksumsCalculated, u16NumBlocks) == 0);
        bool bSecondMatchesCalculated = (memcmp(au8BlockChecksumsB1, au8BlockChecksumsCalculated, u16NumBlocks) == 0);

        if (bFirstMatchesCalculated && !bSecondMatchesCalculated)
        {
            // First copy is correct, repair second
            vHwWriteConfigBlock((void *)au8BlockChecksums, (void *)(uintptr_t)u16BlockChkB1Addr, u16NumBlocks);
        }
        else if (!bFirstMatchesCalculated && bSecondMatchesCalculated)
        {
            // Second copy is correct, repair first
            vHwWriteConfigBlock((void *)au8BlockChecksumsB1, (void *)(uintptr_t)u16BlockChkAddr, u16NumBlocks);
            memcpy(au8BlockChecksums, au8BlockChecksumsB1, u16NumBlocks);
        }
        // If neither matches or both match (shouldn't happen), don't repair - data may be corrupted
    }

    // Fast path: if no corruption detected, copy requested section to output
    if (bIsCfgOk)
    {
        // Read the requested section directly into output buffer
        for (uint16_t offset = 0; offset < u16Size; offset += PINCFG_BLOCK_SIZE)
        {
            uint16_t blockSize = (offset + PINCFG_BLOCK_SIZE <= u16Size) ? PINCFG_BLOCK_SIZE : (u16Size - offset);

            void *pReadAddr = (u16Offset < PINCFG_CONFIG_OFFSET)
                                  ? (void *)(uintptr_t)(EEPROM_PASSWORD + u16Offset + offset)
                                  : (void *)(uintptr_t)(EEPROM_PINCFG + (u16Offset - PINCFG_CONFIG_OFFSET) + offset);

            vHwReadConfigBlock((void *)(pcDataOut + offset), pReadAddr, blockSize);
        }
        return PERCFG_OK_E;
    }

    // ============================================================
    // CORRUPTION RECOVERY PHASE
    // ============================================================

    uint16_t u16BackupStartAddr = getBackupStartAddr(u16TotalSize);
    uint16_t u16BackupSize = getBackupSize(u16TotalSize);
    uint16_t u16BackupBlocks = (u16BackupSize + PINCFG_BLOCK_SIZE - 1) / PINCFG_BLOCK_SIZE;

    // Check if corrupted blocks are within backed-up range
    bool bCorruptedBlockIsBackedUp = false;
    for (uint16_t i = 0; i < u16BackupBlocks && i < u16NumBlocks; ++i)
    {
        if (au8BlockChecksums[i] != au8BlockChecksumsCalculated[i])
        {
            bCorruptedBlockIsBackedUp = true;
            break;
        }
    }

    if (bCorruptedBlockIsBackedUp && u16BackupSize > 0)
    {
        // ============================================================
        // VALIDATE BACKUP AGAINST CRC16 (not block checksums which may be corrupted)
        // CRC16 has triple redundancy with voting, so it's the authoritative checksum
        // ============================================================
        uint16_t u16BackupCrc = 0xFFFF;

        // Calculate CRC16 of backup data
        for (uint16_t offset = 0; offset < u16BackupSize; offset += PINCFG_BLOCK_SIZE)
        {
            uint16_t blockSize =
                (offset + PINCFG_BLOCK_SIZE <= u16BackupSize) ? PINCFG_BLOCK_SIZE : (u16BackupSize - offset);

            vHwReadConfigBlock((void *)au8Block, (void *)(uintptr_t)(u16BackupStartAddr + offset), blockSize);

            for (uint16_t i = 0; i < blockSize; ++i)
                crc16_update(&u16BackupCrc, au8Block[i]);
        }

        // If backup is smaller than total size, we need to include the non-backed-up portion
        // from main storage in the CRC calculation (it should still be valid)
        if (u16BackupSize < u16TotalSize)
        {
            for (uint16_t offset = u16BackupSize; offset < u16TotalSize; offset += PINCFG_BLOCK_SIZE)
            {
                uint16_t blockSize =
                    (offset + PINCFG_BLOCK_SIZE <= u16TotalSize) ? PINCFG_BLOCK_SIZE : (u16TotalSize - offset);

                void *pReadAddr = (offset < PINCFG_CONFIG_OFFSET)
                                      ? (void *)(uintptr_t)(EEPROM_PASSWORD + offset)
                                      : (void *)(uintptr_t)(EEPROM_PINCFG + (offset - PINCFG_CONFIG_OFFSET));

                vHwReadConfigBlock((void *)au8Block, pReadAddr, blockSize);

                for (uint16_t i = 0; i < blockSize; ++i)
                    crc16_update(&u16BackupCrc, au8Block[i]);
            }
        }

        // Check if backup matches the stored CRC16 (triple-redundancy voted value)
        bool bBackupOk = (u16BackupCrc == u16CfgCheckSum);

        if (bBackupOk)
        {
            // Restore from backup - optimize for flash page writes
            // Write password section as ONE block if backed up
            if (u16BackupSize >= PINCFG_CONFIG_OFFSET)
            {
                // Full password section is backed up - write in one go (reuse au8Block)
                vHwReadConfigBlock((void *)au8Block, (void *)(uintptr_t)u16BackupStartAddr, PINCFG_AUTH_PASSWORD_LEN_D);
                vHwWriteConfigBlock((void *)au8Block, (void *)(uintptr_t)EEPROM_PASSWORD, PINCFG_AUTH_PASSWORD_LEN_D);

                // Write config section in PINCFG_BLOCK_SIZE chunks (reuse au8Block)
                uint16_t u16ConfigBackupSize = u16BackupSize - PINCFG_CONFIG_OFFSET;
                for (uint16_t offset = 0; offset < u16ConfigBackupSize; offset += PINCFG_BLOCK_SIZE)
                {
                    uint16_t chunkSize = (offset + PINCFG_BLOCK_SIZE <= u16ConfigBackupSize)
                                             ? PINCFG_BLOCK_SIZE
                                             : (u16ConfigBackupSize - offset);

                    vHwReadConfigBlock(
                        (void *)au8Block,
                        (void *)(uintptr_t)(u16BackupStartAddr + PINCFG_CONFIG_OFFSET + offset),
                        chunkSize);
                    vHwWriteConfigBlock((void *)au8Block, (void *)(uintptr_t)(EEPROM_PINCFG + offset), chunkSize);
                }
            }
            else
            {
                // Partial password backup - write what we have (reuse au8Block)
                vHwReadConfigBlock((void *)au8Block, (void *)(uintptr_t)u16BackupStartAddr, u16BackupSize);
                vHwWriteConfigBlock((void *)au8Block, (void *)(uintptr_t)EEPROM_PASSWORD, u16BackupSize);
            }

            // Revalidate with CRC16
            u16CfgCheckSumCalculated = 0xFFFF;
            for (uint16_t offset = 0; offset < u16TotalSize; offset += PINCFG_BLOCK_SIZE)
            {
                uint16_t blockSize =
                    (offset + PINCFG_BLOCK_SIZE <= u16TotalSize) ? PINCFG_BLOCK_SIZE : (u16TotalSize - offset);

                void *pReadAddr = (offset < PINCFG_CONFIG_OFFSET)
                                      ? (void *)(uintptr_t)(EEPROM_PASSWORD + offset)
                                      : (void *)(uintptr_t)(EEPROM_PINCFG + (offset - PINCFG_CONFIG_OFFSET));

                vHwReadConfigBlock((void *)au8Block, pReadAddr, blockSize);

                // Update CRC16 incrementally
                for (uint16_t i = 0; i < blockSize; ++i)
                    crc16_update(&u16CfgCheckSumCalculated, au8Block[i]);
            }

            if (u16CfgCheckSumCalculated == u16CfgCheckSum)
            {
                // REPAIRED! Read requested section
                for (uint16_t offset = 0; offset < u16Size; offset += PINCFG_BLOCK_SIZE)
                {
                    uint16_t blockSize =
                        (offset + PINCFG_BLOCK_SIZE <= u16Size) ? PINCFG_BLOCK_SIZE : (u16Size - offset);

                    void *pReadAddr =
                        (u16Offset < PINCFG_CONFIG_OFFSET)
                            ? (void *)(uintptr_t)(EEPROM_PASSWORD + u16Offset + offset)
                            : (void *)(uintptr_t)(EEPROM_PINCFG + (u16Offset - PINCFG_CONFIG_OFFSET) + offset);

                    vHwReadConfigBlock((void *)(pcDataOut + offset), pReadAddr, blockSize);
                }
                return PERCFG_OK_E;
            }
        }
    }

    return PERCFG_READ_FAILED_E;
}

// Public API - reads password section (32-byte binary hash) with full CRC/recovery protection
PERCFG_RESULT_T PersistentCfg_eReadPassword(uint8_t *pu8PasswordOut)
{
    if (pu8PasswordOut == NULL)
        return PERCFG_ERROR_E;

    // Read password section (fixed 32 bytes binary hash)
    // Note: eLoad expects char* but we're reading binary data - cast is safe
    return eLoad((char *)pu8PasswordOut, 0);
}

// Public API - writes password section (32-byte binary hash) with full CRC/backup protection
PERCFG_RESULT_T PersistentCfg_eWritePassword(const uint8_t *pu8Password)
{
    if (pu8Password == NULL)
        return PERCFG_ERROR_E;

    // Save password section - always write full PINCFG_AUTH_PASSWORD_LEN_D bytes (32 bytes binary)
    return eSave((const char *)pu8Password, PINCFG_PASSWORD_OFFSET, PINCFG_AUTH_PASSWORD_LEN_D);
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
PERCFG_RESULT_T PersistentCfg_eSaveConfig(const char *pcCfg)
{
    if (pcCfg == NULL)
        return PERCFG_ERROR_E;

    // Calculate size including null terminator
    uint16_t u16ConfigSize = strlen(pcCfg) + 1;

    if (u16ConfigSize > PINCFG_CONFIG_MAX_SZ_D)
        return PERCFG_CFG_BIGGER_THAN_MAX_SZ_E;

    // Save directly - string is already null-terminated
    return eSave(pcCfg, PINCFG_CONFIG_OFFSET, u16ConfigSize);
}
