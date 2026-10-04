/**
 * @file    stm32f407xx_gpio.c
 * @brief   GPIO driver for STM32F407xx (bare-metal, no HAL).
 * @author  nhduong
 * @date    Feb 15, 2025
 */

#include "stm32f407xx_gpio.h"

/* Clock control ----------------------------------------------------------- */

/**
 * @brief  Enable or disable the peripheral clock of a GPIO port.
 * @param  pGPIOx     GPIO port (GPIOA..GPIOI).
 * @param  clockState ENABLE or DISABLE.
 */
void GPIO_PeriClockControl(GPIO_RegDef_t *pGPIOx, uint8_t clockState)
{
    if (clockState == ENABLE) {
        if (pGPIOx == GPIOA) {
            GPIOA_CLK_ENABLE();
        } else if (pGPIOx == GPIOB) {
            GPIOB_CLK_ENABLE();
        } else if (pGPIOx == GPIOC) {
            GPIOC_CLK_ENABLE();
        } else if (pGPIOx == GPIOD) {
            GPIOD_CLK_ENABLE();
        } else if (pGPIOx == GPIOE) {
            GPIOE_CLK_ENABLE();
        } else if (pGPIOx == GPIOF) {
            GPIOF_CLK_ENABLE();
        } else if (pGPIOx == GPIOG) {
            GPIOG_CLK_ENABLE();
        } else if (pGPIOx == GPIOH) {
            GPIOH_CLK_ENABLE();
        } else if (pGPIOx == GPIOI) {
            GPIOI_CLK_ENABLE();
        }
        /* Errata: wait 2 clock cycles before accessing the port,
         * reading the register back is enough to create that delay */
        (void)RCC->AHB1ENR;
    } else {
        if (pGPIOx == GPIOA) {
            GPIOA_CLK_DISABLE();
        } else if (pGPIOx == GPIOB) {
            GPIOB_CLK_DISABLE();
        } else if (pGPIOx == GPIOC) {
            GPIOC_CLK_DISABLE();
        } else if (pGPIOx == GPIOD) {
            GPIOD_CLK_DISABLE();
        } else if (pGPIOx == GPIOE) {
            GPIOE_CLK_DISABLE();
        } else if (pGPIOx == GPIOF) {
            GPIOF_CLK_DISABLE();
        } else if (pGPIOx == GPIOG) {
            GPIOG_CLK_DISABLE();
        } else if (pGPIOx == GPIOH) {
            GPIOH_CLK_DISABLE();
        } else if (pGPIOx == GPIOI) {
            GPIOI_CLK_DISABLE();
        }
    }
}

/* Init / de-init ---------------------------------------------------------- */

/**
 * @brief  Configure one GPIO pin according to hGPIO->Init.
 * @note   The port clock is enabled automatically.
 * @note   For interrupt modes the NVIC is NOT configured here,
 *         call NVIC_SetPriority() and NVIC_IRQConfig() afterwards.
 * @param  hGPIO Handle with the port and the pin configuration.
 */
void GPIO_Init(GPIO_HandleTypeDef *hGPIO)
{
    GPIO_RegDef_t *pGPIOx = hGPIO->pGPIOx;
    uint8_t pin  = hGPIO->Init.Pin;
    uint8_t mode = hGPIO->Init.Mode;

    if (!IS_GPIO_PIN(pin) || !IS_GPIO_MODE(mode)) {
        return;
    }

    GPIO_PeriClockControl(pGPIOx, ENABLE);

    /* MODER, OSPEEDR, PUPDR use 2 bits per pin -> field starts at bit (2 * pin) */
    uint32_t pos2 = 2U * pin;

    /* 1. Mode: clearing the field selects input mode, which is also
     *    the required mode for EXTI (interrupt) pins */
    pGPIOx->MODER &= ~(0x3U << pos2);

    if (mode <= GPIO_MODE_ANALOG) {
        pGPIOx->MODER |= ((uint32_t)mode << pos2);
    } else {
        /* Select the trigger edge(s) */
        if (mode == GPIO_MODE_IT_FALLING) {
            EXTI->FTSR |=  (1U << pin);
            EXTI->RTSR &= ~(1U << pin);
        } else if (mode == GPIO_MODE_IT_RISING) {
            EXTI->RTSR |=  (1U << pin);
            EXTI->FTSR &= ~(1U << pin);
        } else {
            EXTI->RTSR |=  (1U << pin);
            EXTI->FTSR |=  (1U << pin);
        }
        
        uint8_t  reg      = pin / 4U;
        uint32_t pos      = pin % 4U;
        uint32_t portCode = GPIO_BASEADDR_TO_CODE(pGPIOx); /* GPIOA = 0 ... GPIOI = 8 */

        SYSCFG_CLK_ENABLE();
        (void)RCC->APB2ENR; /* Errata: delay after enabling the clock */
        SYSCFG->EXTICR[reg] &= ~(0xFU << pos);
        SYSCFG->EXTICR[reg] |= (portCode << pos);

        /* Unmask the line so the interrupt reaches the NVIC */
        EXTI->IMR |= (1U << pin);
    }

    /* 2. Speed */
    pGPIOx->OSPEEDR &= ~(0x3U << pos2);
    pGPIOx->OSPEEDR |= ((uint32_t)hGPIO->Init.Speed << pos2);

    /* 3. Pull-up / pull-down */
    pGPIOx->PUPDR &= ~(0x3U << pos2);
    pGPIOx->PUPDR |= ((uint32_t)hGPIO->Init.Pull << pos2);

    /* 4. Output type (1 bit per pin) */
    pGPIOx->OTYPER &= ~(0x1U << pin);
    pGPIOx->OTYPER |= ((uint32_t)hGPIO->Init.OPType << pin);

    /* 5. Alternate function.
     *    AFR[0] (AFRL) = pins 0..7, AFR[1] (AFRH) = pins 8..15, 4 bits per pin */
    if (mode == GPIO_MODE_AF) {
        uint8_t  reg = pin / 8U;
        uint32_t pos = 4U * (pin % 8U);

        pGPIOx->AFR[reg] &= ~(0xFU << pos);
        pGPIOx->AFR[reg] |= ((uint32_t)(hGPIO->Init.Alternate & 0xFU) << pos);
    }
}

/**
 * @brief  Quick setup of one pin: push-pull, low speed, no alternate function.
 *
 * Example:
 * @code
 * GPIO_ConfigPin(GPIOD, GPIO_PIN_12, GPIO_MODE_OUTPUT, GPIO_NOPULL);
 * @endcode
 *
 * @param  pGPIOx   GPIO port (GPIOA..GPIOI).
 * @param  GPIO_pin Pin number, a value of @ref GPIO_Pins.
 * @param  mode     A value of @ref GPIO_Modes.
 * @param  pull     A value of @ref GPIO_Pulls.
 */
void GPIO_ConfigPin(GPIO_RegDef_t *pGPIOx, uint8_t GPIO_pin, uint8_t mode, uint8_t pull)
{
    GPIO_HandleTypeDef hGPIO = {
        .pGPIOx = pGPIOx,
        .Init = {
            .Pin       = GPIO_pin,
            .Mode      = mode,
            .Pull      = pull,
            .Speed     = GPIO_SPEED_LOW,
            .OPType    = GPIO_OPTYPE_PP,
            .Alternate = AF0,
        },
    };

    GPIO_Init(&hGPIO);
}

/**
 * @brief  Reset all registers of a GPIO port to their reset values.
 * @note   The EXTI/SYSCFG settings of the pins are not reset.
 * @param  pGPIOx GPIO port (GPIOA..GPIOI).
 */
void GPIO_DeInit(GPIO_RegDef_t *pGPIOx)
{
    if (pGPIOx == GPIOA) {
        GPIOA_REG_RESET();
    } else if (pGPIOx == GPIOB) {
        GPIOB_REG_RESET();
    } else if (pGPIOx == GPIOC) {
        GPIOC_REG_RESET();
    } else if (pGPIOx == GPIOD) {
        GPIOD_REG_RESET();
    } else if (pGPIOx == GPIOE) {
        GPIOE_REG_RESET();
    } else if (pGPIOx == GPIOF) {
        GPIOF_REG_RESET();
    } else if (pGPIOx == GPIOG) {
        GPIOG_REG_RESET();
    } else if (pGPIOx == GPIOH) {
        GPIOH_REG_RESET();
    } else if (pGPIOx == GPIOI) {
        GPIOI_REG_RESET();
    }
}

/* IO operations ----------------------------------------------------------- */

/**
 * @brief  Read the input level of one pin.
 * @param  pGPIOx   GPIO port (GPIOA..GPIOI).
 * @param  GPIO_pin Pin number, a value of @ref GPIO_Pins.
 * @return GPIO_PIN_SET or GPIO_PIN_RESET (also returned for an invalid pin).
 */
uint8_t GPIO_ReadPin(GPIO_RegDef_t *pGPIOx, uint8_t GPIO_pin)
{
    if (!IS_GPIO_PIN(GPIO_pin)) {
        return GPIO_PIN_RESET;
    }

    return (uint8_t)((pGPIOx->IDR >> GPIO_pin) & 0x1U);
}

/**
 * @brief  Read the input level of the whole port.
 * @param  pGPIOx GPIO port (GPIOA..GPIOI).
 * @return 16-bit value of IDR (bit n = pin n).
 */
uint16_t GPIO_ReadPort(GPIO_RegDef_t *pGPIOx)
{
    return (uint16_t)pGPIOx->IDR;
}

/**
 * @brief  Set or clear one output pin.
 * @note   Uses BSRR: the write is atomic and does not touch other pins.
 * @param  pGPIOx   GPIO port (GPIOA..GPIOI).
 * @param  GPIO_pin Pin number, a value of @ref GPIO_Pins.
 * @param  pinState GPIO_PIN_SET or GPIO_PIN_RESET.
 */
void GPIO_WritePin(GPIO_RegDef_t *pGPIOx, uint8_t GPIO_pin, uint8_t pinState)
{
    if (!IS_GPIO_PIN(GPIO_pin)) {
        return;
    }

    /* BSRR: bits 0..15 set the pin, bits 16..31 clear the pin, writing 0 does nothing */
    if (pinState == GPIO_PIN_SET) {
        pGPIOx->BSRR = (1U << GPIO_pin);
    } else {
        pGPIOx->BSRR = (1U << (GPIO_pin + 16U));
    }
}

/**
 * @brief  Write a 16-bit value to the whole output port.
 * @param  pGPIOx GPIO port (GPIOA..GPIOI).
 * @param  value  Value written to ODR (bit n = pin n).
 */
void GPIO_WritePort(GPIO_RegDef_t *pGPIOx, uint16_t value)
{
    pGPIOx->ODR = value;
}

/**
 * @brief  Toggle one output pin.
 * @note   Uses BSRR: the write is atomic and does not touch other pins.
 * @param  pGPIOx   GPIO port (GPIOA..GPIOI).
 * @param  GPIO_pin Pin number, a value of @ref GPIO_Pins.
 */
void GPIO_TogglePin(GPIO_RegDef_t *pGPIOx, uint8_t GPIO_pin)
{
    if (!IS_GPIO_PIN(GPIO_pin)) {
        return;
    }

    uint32_t mask = (1U << GPIO_pin);
    uint32_t odr  = pGPIOx->ODR;

    /* Pin high -> write its reset bit (upper half),
     * pin low  -> write its set bit   (lower half) */
    pGPIOx->BSRR = ((odr & mask) << 16U) | (~odr & mask);
}

/* Interrupt handling ------------------------------------------------------ */

/**
 * @brief  Common EXTI handler for one pin.
 *
 * Call it from the matching vector, it clears the pending flag
 * and then calls GPIO_EXTI_Callback():
 * @code
 * void EXTI0_IRQHandler(void) { GPIO_IRQHandler(GPIO_PIN_0); }
 * @endcode
 *
 * @param  GPIO_pin Pin number (= EXTI line) that triggered the interrupt.
 */
void GPIO_IRQHandler(uint8_t GPIO_pin)
{
    if (!IS_GPIO_PIN(GPIO_pin)) {
        return;
    }

    uint32_t mask = (1U << GPIO_pin);

    if (EXTI->PR & mask) {
        /* PR is "write 1 to clear": use '=' not '|=',
         * otherwise the other pending lines are cleared too */
        EXTI->PR = mask;
        GPIO_EXTI_Callback(GPIO_pin);
    }
}

/**
 * @brief  EXTI callback, called by GPIO_IRQHandler().
 * @note   Weak empty function, define your own version in the application.
 * @param  GPIO_pin Pin number (= EXTI line) that triggered the interrupt.
 */
__weak void GPIO_EXTI_Callback(uint8_t GPIO_pin)
{
    (void)GPIO_pin;
}
