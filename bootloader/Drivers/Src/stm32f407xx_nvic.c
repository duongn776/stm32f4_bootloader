/**
 * @file    stm32f407xx_nvic.c
 * @brief   NVIC helpers shared by all drivers (GPIO, SPI, I2C, UART, ...).
 */

#include "stm32f407xx_nvic.h"

/**
 * @brief  Enable or disable an IRQ in the NVIC.
 * @param  IRQNumber IRQ number, see IRQ_NO_xxx in stm32f407xx.h.
 * @param  state     ENABLE or DISABLE.
 */
void NVIC_IRQConfig(uint8_t IRQNumber, uint8_t state)
{
    if (IRQNumber > NVIC_IRQ_MAX) {
        return;
    }

    /* Each ISERx/ICERx register covers 32 IRQs: IRQ n -> register n / 32, bit n % 32.
     * Writing 1 sets/clears the enable bit and writing 0 has no effect,
     * so a plain '=' is enough (no read-modify-write) */
    uint32_t reg = IRQNumber / 32U;
    uint32_t bit = 1U << (IRQNumber % 32U);

    if (state == ENABLE) {
        NVIC_ISER0[reg] = bit;
    } else {
        NVIC_ICER0[reg] = bit;
    }
}

/**
 * @brief  Set the priority of an IRQ.
 * @param  IRQNumber IRQ number, see IRQ_NO_xxx in stm32f407xx.h.
 * @param  priority  0..15, 0 is the highest priority.
 */
void NVIC_SetPriority(uint8_t IRQNumber, uint8_t priority)
{
    if (IRQNumber > NVIC_IRQ_MAX) {
        return;
    }

    /* Each IRQ has its own byte in the IPR area, so it can be written directly.
     * Only the upper NO_PR_BITS_IMPLEMENTED (4) bits of that byte are used,
     * which is why the priority is shifted left by 4 */
    __vo uint8_t *pIPR = (__vo uint8_t *)NVIC_PR_BASEADDR;

    pIPR[IRQNumber] = (uint8_t)(priority << (8U - NO_PR_BITS_IMPLEMENTED));
}
