/**
 * @file    test_gpio.c
 * @brief   Button + LED test for the GPIO and NVIC drivers (STM32F407 Discovery).
 *
 * Board wiring (STM32F407G-DISC1):
 *   - USER button : PA0  (active high, external pull-down on the board)
 *   - LED green   : PD12
 *   - LED orange  : PD13
 *   - LED red     : PD14
 *   - LED blue    : PD15
 *
 * Expected behavior:
 *   1. At start-up the 4 LEDs light up one by one   -> GPIO output works.
 *   2. Green LED is ON while the button is held     -> GPIO_ReadPin (polling) works.
 *   3. Each press toggles the orange LED            -> EXTI + NVIC + callback work.
 *   4. Blue LED blinks slowly                       -> main loop is alive.
 */

#include "test_gpio.h"
#include "stm32f407xx_gpio.h"

/* Private defines --------------------------------------------------------- */

#define BTN_PORT        GPIOA
#define BTN_PIN         GPIO_PIN_0

#define LED_PORT        GPIOD
#define LED_GREEN       GPIO_PIN_12
#define LED_ORANGE      GPIO_PIN_13
#define LED_RED         GPIO_PIN_14
#define LED_BLUE        GPIO_PIN_15

#define DEBOUNCE_MS     200U

/* Private variables ------------------------------------------------------- */

/* Set by the EXTI callback, cleared by the main loop.
 * 'volatile' because it is shared between the ISR and the main loop */
static volatile uint8_t g_buttonPressed = 0;

/* Private functions ------------------------------------------------------- */

/**
 * @brief  Rough busy-wait delay.
 * @note   Calibrated for the default 16 MHz HSI clock, not accurate.
 *         Only for testing, use a timer/SysTick in real code.
 * @param  ms Delay in milliseconds (approximately).
 */
static void delay_ms(uint32_t ms)
{
    /* ~4 CPU cycles per loop iteration -> 16 MHz / 4 = 4000 iterations per ms */
    for (volatile uint32_t i = 0; i < ms * 4000U; i++) {
    }
}

/**
 * @brief  Light the 4 LEDs one by one, then turn them all off.
 */
static void led_startup_sequence(void)
{
    const uint8_t leds[] = { LED_GREEN, LED_ORANGE, LED_RED, LED_BLUE };

    for (uint32_t i = 0; i < sizeof(leds); i++) {
        GPIO_WritePin(LED_PORT, leds[i], GPIO_PIN_SET);
        delay_ms(150);
    }

    for (uint32_t i = 0; i < sizeof(leds); i++) {
        GPIO_WritePin(LED_PORT, leds[i], GPIO_PIN_RESET);
    }
}

/* Public functions -------------------------------------------------------- */

/**
 * @brief  Run the button + LED test, never returns.
 */
void Test_GPIO_ButtonLed(void)
{
    /* LEDs as push-pull outputs */
    GPIO_ConfigPin(LED_PORT, LED_GREEN,  GPIO_MODE_OUTPUT, GPIO_NOPULL);
    GPIO_ConfigPin(LED_PORT, LED_ORANGE, GPIO_MODE_OUTPUT, GPIO_NOPULL);
    GPIO_ConfigPin(LED_PORT, LED_RED,    GPIO_MODE_OUTPUT, GPIO_NOPULL);
    GPIO_ConfigPin(LED_PORT, LED_BLUE,   GPIO_MODE_OUTPUT, GPIO_NOPULL);

    /* Button: interrupt on rising edge (press).
     * No internal pull: the board already has an external pull-down */
    GPIO_ConfigPin(BTN_PORT, BTN_PIN, GPIO_MODE_IT_RISING, GPIO_NOPULL);

    /* Set the priority before enabling the IRQ */
    NVIC_SetPriority(IRQ_NO_EXTI0, 15);
    NVIC_IRQConfig(IRQ_NO_EXTI0, ENABLE);

    led_startup_sequence();

    uint32_t loopCount = 0;

    for (;;) {
        /* Test 2: polling, green LED follows the button */
        GPIO_WritePin(LED_PORT, LED_GREEN, GPIO_ReadPin(BTN_PORT, BTN_PIN));

        /* Test 3: interrupt, toggle orange LED once per press.
         * The button bounces for a few ms and fires several interrupts:
         * wait DEBOUNCE_MS, then clear the flag to drop those extra edges */
        if (g_buttonPressed) {
            GPIO_TogglePin(LED_PORT, LED_ORANGE);
            delay_ms(DEBOUNCE_MS);
            g_buttonPressed = 0;
        }

        /* Test 4: heartbeat, toggle blue LED every ~500 ms */
        delay_ms(10);
        if (++loopCount >= 50U) {
            loopCount = 0;
            GPIO_TogglePin(LED_PORT, LED_BLUE);
        }
    }
}

/* Interrupt handlers ------------------------------------------------------ */

/**
 * @brief  EXTI line 0 vector (name must match startup_stm32f407vgtx.s).
 */
void EXTI0_IRQHandler(void)
{
    GPIO_IRQHandler(BTN_PIN);
}

/**
 * @brief  Overrides the weak callback in stm32f407xx_gpio.c.
 * @note   Keep it short: only set a flag, the work is done in the main loop.
 * @param  GPIO_pin Pin number (= EXTI line) that triggered the interrupt.
 */
void GPIO_EXTI_Callback(uint8_t GPIO_pin)
{
    if (GPIO_pin == BTN_PIN) {
        g_buttonPressed = 1;
    }
}
