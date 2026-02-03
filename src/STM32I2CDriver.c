/**
 * @file STM32I2CDriver.c
 * @brief STM32 I2C driver implementation using LL with interrupt support
 */

#include "STM32I2CDriver.h"

// Only compile for STM32 platforms (not unit tests)
#if !defined(UNIT_TEST) &&                                                                                             \
    (defined(ARDUINO_ARCH_STM32) || defined(ARDUINO_ARCH_STM32F1) || defined(ARDUINO_ARCH_STM32F4) ||                  \
     defined(ARDUINO_ARCH_STM32L4) || defined(STM32F1) || defined(STM32F103xB))

#include "stm32f1xx_ll_bus.h"
#include "stm32f1xx_ll_gpio.h"
#include "stm32f1xx_ll_i2c.h"
#include "stm32f1xx_ll_rcc.h"

// I2C1 pins: PB6 = SCL, PB7 = SDA
#ifndef STM32_I2C_INSTANCE
#define STM32_I2C_INSTANCE I2C1
#endif

#ifndef STM32_I2C_SPEED_HZ
#define STM32_I2C_SPEED_HZ 100000 // 100kHz standard mode
#endif

#define STM32_I2C_BUFFER_SIZE 32

// ISR state machine states
typedef enum I2C_STATE_E
{
    I2C_STATE_IDLE_E = 0,
    I2C_STATE_TX_ADDR_E,     // Sending address + write bit
    I2C_STATE_TX_DATA_E,     // Sending data bytes
    I2C_STATE_TX_COMPLETE_E, // TX done
    I2C_STATE_RX_ADDR_E,     // Sending address + read bit
    I2C_STATE_RX_DATA_E,     // Receiving data bytes
    I2C_STATE_RX_COMPLETE_E, // RX done
    I2C_STATE_ERROR_E        // Error occurred
} I2C_STATE_T;

// I2C driver state structure
typedef struct STM32_I2C_STATE_S
{
    // ISR-accessible state (volatile members)
    volatile I2C_STATE_T eState;
    volatile STM32_I2C_RESULT_T eResult;
    volatile uint8_t u8TxIndex;
    volatile uint8_t u8RxIndex;
    volatile uint8_t u8RxExpected;
    volatile bool bSendStop;
    volatile uint8_t au8RxBuffer[STM32_I2C_BUFFER_SIZE];
    volatile uint8_t u8RxLength;

    // Non-volatile state
    uint8_t u8Address;
    const uint8_t *pu8TxData;
    uint8_t u8TxLength;
    bool bInitialized;
} STM32_I2C_STATE_T;

// Single static instance
static STM32_I2C_STATE_T g_sI2C = {0};

// I2C1 Event IRQ Handler
void I2C1_EV_IRQHandler(void)
{
    // Start condition sent (SB flag)
    if (LL_I2C_IsActiveFlag_SB(STM32_I2C_INSTANCE))
    {
        if (g_sI2C.eState == I2C_STATE_TX_ADDR_E)
        {
            // Send address + write bit (0)
            LL_I2C_TransmitData8(STM32_I2C_INSTANCE, (g_sI2C.u8Address << 1) | 0);
        }
        else if (g_sI2C.eState == I2C_STATE_RX_ADDR_E)
        {
            // Send address + read bit (1)
            LL_I2C_TransmitData8(STM32_I2C_INSTANCE, (g_sI2C.u8Address << 1) | 1);
        }
        return;
    }

    // Address sent (ADDR flag)
    if (LL_I2C_IsActiveFlag_ADDR(STM32_I2C_INSTANCE))
    {
        LL_I2C_ClearFlag_ADDR(STM32_I2C_INSTANCE);

        if (g_sI2C.eState == I2C_STATE_TX_ADDR_E)
        {
            g_sI2C.eState = I2C_STATE_TX_DATA_E;
            // Enable TXE interrupt for data transmission
            LL_I2C_EnableIT_BUF(STM32_I2C_INSTANCE);
        }
        else if (g_sI2C.eState == I2C_STATE_RX_ADDR_E)
        {
            g_sI2C.eState = I2C_STATE_RX_DATA_E;

            // For single byte reception, set NACK before clearing ADDR
            if (g_sI2C.u8RxExpected == 1)
            {
                LL_I2C_AcknowledgeNextData(STM32_I2C_INSTANCE, LL_I2C_NACK);
                LL_I2C_GenerateStopCondition(STM32_I2C_INSTANCE);
            }
            else
            {
                LL_I2C_AcknowledgeNextData(STM32_I2C_INSTANCE, LL_I2C_ACK);
            }
            // Enable RXNE interrupt
            LL_I2C_EnableIT_BUF(STM32_I2C_INSTANCE);
        }
        return;
    }

    // TX buffer empty (TXE flag) - transmit mode
    if (LL_I2C_IsActiveFlag_TXE(STM32_I2C_INSTANCE) && (g_sI2C.eState == I2C_STATE_TX_DATA_E))
    {
        if (g_sI2C.u8TxIndex < g_sI2C.u8TxLength)
        {
            LL_I2C_TransmitData8(STM32_I2C_INSTANCE, g_sI2C.pu8TxData[g_sI2C.u8TxIndex++]);
        }
        else if (LL_I2C_IsActiveFlag_BTF(STM32_I2C_INSTANCE))
        {
            // Last byte transferred, BTF set
            LL_I2C_DisableIT_BUF(STM32_I2C_INSTANCE);

            if (g_sI2C.bSendStop)
            {
                LL_I2C_GenerateStopCondition(STM32_I2C_INSTANCE);
            }
            g_sI2C.eState = I2C_STATE_TX_COMPLETE_E;
            g_sI2C.eResult = STM32_I2C_OK_E;

            // Disable all I2C interrupts
            LL_I2C_DisableIT_EVT(STM32_I2C_INSTANCE);
            LL_I2C_DisableIT_ERR(STM32_I2C_INSTANCE);
        }
        return;
    }

    // RX buffer not empty (RXNE flag) - receive mode
    if (LL_I2C_IsActiveFlag_RXNE(STM32_I2C_INSTANCE) && (g_sI2C.eState == I2C_STATE_RX_DATA_E))
    {
        g_sI2C.au8RxBuffer[g_sI2C.u8RxIndex++] = LL_I2C_ReceiveData8(STM32_I2C_INSTANCE);

        // Prepare for last byte (set NACK before reading second-to-last)
        if (g_sI2C.u8RxIndex == g_sI2C.u8RxExpected - 1)
        {
            LL_I2C_AcknowledgeNextData(STM32_I2C_INSTANCE, LL_I2C_NACK);
            LL_I2C_GenerateStopCondition(STM32_I2C_INSTANCE);
        }

        // All bytes received
        if (g_sI2C.u8RxIndex >= g_sI2C.u8RxExpected)
        {
            g_sI2C.u8RxLength = g_sI2C.u8RxIndex;
            g_sI2C.eState = I2C_STATE_RX_COMPLETE_E;
            g_sI2C.eResult = STM32_I2C_OK_E;

            // Disable all I2C interrupts
            LL_I2C_DisableIT_EVT(STM32_I2C_INSTANCE);
            LL_I2C_DisableIT_BUF(STM32_I2C_INSTANCE);
            LL_I2C_DisableIT_ERR(STM32_I2C_INSTANCE);
        }
        return;
    }
}

// I2C1 Error IRQ Handler
void I2C1_ER_IRQHandler(void)
{
    // Bus error
    if (LL_I2C_IsActiveFlag_BERR(STM32_I2C_INSTANCE))
    {
        LL_I2C_ClearFlag_BERR(STM32_I2C_INSTANCE);
        g_sI2C.eResult = STM32_I2C_BUS_ERROR_E;
    }

    // Arbitration lost
    if (LL_I2C_IsActiveFlag_ARLO(STM32_I2C_INSTANCE))
    {
        LL_I2C_ClearFlag_ARLO(STM32_I2C_INSTANCE);
        g_sI2C.eResult = STM32_I2C_ARB_LOST_E;
    }

    // Acknowledge failure (NACK)
    if (LL_I2C_IsActiveFlag_AF(STM32_I2C_INSTANCE))
    {
        LL_I2C_ClearFlag_AF(STM32_I2C_INSTANCE);
        LL_I2C_GenerateStopCondition(STM32_I2C_INSTANCE);
        g_sI2C.eResult = STM32_I2C_NACK_E;
    }

    // Set error state
    g_sI2C.eState = I2C_STATE_ERROR_E;

    // Disable all I2C interrupts
    LL_I2C_DisableIT_EVT(STM32_I2C_INSTANCE);
    LL_I2C_DisableIT_BUF(STM32_I2C_INSTANCE);
    LL_I2C_DisableIT_ERR(STM32_I2C_INSTANCE);
}

void STM32_I2C_vInit(void)
{
    if (g_sI2C.bInitialized)
    {
        // Already initialized, just reset state
        STM32_I2C_vReset();
        return;
    }

    // Enable clocks for I2C1 and GPIOB
    LL_APB1_GRP1_EnableClock(LL_APB1_GRP1_PERIPH_I2C1);
    LL_APB2_GRP1_EnableClock(LL_APB2_GRP1_PERIPH_GPIOB);
    LL_APB2_GRP1_EnableClock(LL_APB2_GRP1_PERIPH_AFIO);

    // Configure PB6 (SCL) and PB7 (SDA) as alternate function open-drain
    LL_GPIO_SetPinMode(GPIOB, LL_GPIO_PIN_6, LL_GPIO_MODE_ALTERNATE);
    LL_GPIO_SetPinMode(GPIOB, LL_GPIO_PIN_7, LL_GPIO_MODE_ALTERNATE);
    LL_GPIO_SetPinOutputType(GPIOB, LL_GPIO_PIN_6, LL_GPIO_OUTPUT_OPENDRAIN);
    LL_GPIO_SetPinOutputType(GPIOB, LL_GPIO_PIN_7, LL_GPIO_OUTPUT_OPENDRAIN);
    LL_GPIO_SetPinSpeed(GPIOB, LL_GPIO_PIN_6, LL_GPIO_SPEED_FREQ_HIGH);
    LL_GPIO_SetPinSpeed(GPIOB, LL_GPIO_PIN_7, LL_GPIO_SPEED_FREQ_HIGH);

    // Disable I2C before configuration
    LL_I2C_Disable(STM32_I2C_INSTANCE);

    // Reset I2C peripheral
    LL_APB1_GRP1_ForceReset(LL_APB1_GRP1_PERIPH_I2C1);
    LL_APB1_GRP1_ReleaseReset(LL_APB1_GRP1_PERIPH_I2C1);

    // Configure I2C timing
    // Get APB1 clock frequency (typically 36MHz on STM32F103)
    uint32_t u32Pclk1 = SystemCoreClock / 2; // APB1 = HCLK/2 typically
    uint32_t u32FreqMHz = u32Pclk1 / 1000000;

    // Set peripheral clock frequency in CR2 (bits 5:0)
    STM32_I2C_INSTANCE->CR2 = (STM32_I2C_INSTANCE->CR2 & ~I2C_CR2_FREQ) | (u32FreqMHz & 0x3F);

    // Configure clock control register for 100kHz standard mode
    // CCR = Pclk1 / (2 * I2C_speed)
    uint32_t u32Ccr = u32Pclk1 / (2 * STM32_I2C_SPEED_HZ);
    if (u32Ccr < 4)
        u32Ccr = 4; // Minimum value
    STM32_I2C_INSTANCE->CCR = u32Ccr;

    // Configure rise time: (1000ns * Pclk1_MHz) + 1 for standard mode
    STM32_I2C_INSTANCE->TRISE = u32FreqMHz + 1;

    // Enable I2C
    LL_I2C_Enable(STM32_I2C_INSTANCE);

    // Setup NVIC for I2C interrupts (priority 6, below CAN at 5)
    NVIC_SetPriority(I2C1_EV_IRQn, 6);
    NVIC_SetPriority(I2C1_ER_IRQn, 6);
    NVIC_EnableIRQ(I2C1_EV_IRQn);
    NVIC_EnableIRQ(I2C1_ER_IRQn);

    // Reset state
    STM32_I2C_vReset();

    g_sI2C.bInitialized = true;
}

bool STM32_I2C_bIsIdle(void)
{
    return (
        g_sI2C.eState == I2C_STATE_IDLE_E || g_sI2C.eState == I2C_STATE_TX_COMPLETE_E ||
        g_sI2C.eState == I2C_STATE_RX_COMPLETE_E || g_sI2C.eState == I2C_STATE_ERROR_E);
}

STM32_I2C_RESULT_T STM32_I2C_eGetResult(void)
{
    return g_sI2C.eResult;
}

STM32_I2C_RESULT_T STM32_I2C_eWriteAsync(uint8_t u8Address, const uint8_t *pu8Data, uint8_t u8Length, bool bSendStop)
{
    if (pu8Data == NULL || u8Length == 0)
    {
        return STM32_I2C_INVALID_PARAM_E;
    }

    if (!STM32_I2C_bIsIdle())
    {
        return STM32_I2C_BUSY_E;
    }

    // Setup state machine
    g_sI2C.u8Address = u8Address;
    g_sI2C.pu8TxData = pu8Data;
    g_sI2C.u8TxLength = u8Length;
    g_sI2C.bSendStop = bSendStop;
    g_sI2C.u8TxIndex = 0;
    g_sI2C.eResult = STM32_I2C_OK_E;
    g_sI2C.eState = I2C_STATE_TX_ADDR_E;

    // Enable I2C event and error interrupts
    LL_I2C_EnableIT_EVT(STM32_I2C_INSTANCE);
    LL_I2C_EnableIT_ERR(STM32_I2C_INSTANCE);

    // Generate START condition - ISR takes over from here
    LL_I2C_GenerateStartCondition(STM32_I2C_INSTANCE);

    return STM32_I2C_OK_E;
}

STM32_I2C_RESULT_T STM32_I2C_eReadAsync(uint8_t u8Address, uint8_t u8Length)
{
    if (u8Length == 0 || u8Length > STM32_I2C_BUFFER_SIZE)
    {
        return STM32_I2C_INVALID_PARAM_E;
    }

    if (!STM32_I2C_bIsIdle())
    {
        return STM32_I2C_BUSY_E;
    }

    // Setup state machine
    g_sI2C.u8Address = u8Address;
    g_sI2C.u8RxExpected = u8Length;
    g_sI2C.u8RxIndex = 0;
    g_sI2C.u8RxLength = 0;
    g_sI2C.eResult = STM32_I2C_OK_E;
    g_sI2C.eState = I2C_STATE_RX_ADDR_E;

    // Enable I2C event and error interrupts
    LL_I2C_EnableIT_EVT(STM32_I2C_INSTANCE);
    LL_I2C_EnableIT_ERR(STM32_I2C_INSTANCE);

    // Generate START condition - ISR takes over from here
    LL_I2C_GenerateStartCondition(STM32_I2C_INSTANCE);

    return STM32_I2C_OK_E;
}

uint8_t STM32_I2C_u8GetRxCount(void)
{
    return g_sI2C.u8RxLength;
}

uint8_t STM32_I2C_u8ReadData(uint8_t *pu8Buffer, uint8_t u8MaxLength)
{
    if (pu8Buffer == NULL || u8MaxLength == 0)
    {
        return 0;
    }

    uint8_t u8Count = (g_sI2C.u8RxLength < u8MaxLength) ? g_sI2C.u8RxLength : u8MaxLength;
    for (uint8_t i = 0; i < u8Count; i++)
    {
        pu8Buffer[i] = g_sI2C.au8RxBuffer[i];
    }
    return u8Count;
}

void STM32_I2C_vReset(void)
{
    g_sI2C.eState = I2C_STATE_IDLE_E;
    g_sI2C.eResult = STM32_I2C_OK_E;
    g_sI2C.u8TxIndex = 0;
    g_sI2C.u8RxIndex = 0;
    g_sI2C.u8RxLength = 0;
    g_sI2C.pu8TxData = NULL;
    g_sI2C.u8TxLength = 0;
}

#endif // STM32 platform check
