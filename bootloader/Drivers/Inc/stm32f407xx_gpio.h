/**
 * @file    stm32f407xx_gpio.h
 * @brief   GPIO driver for STM32F407xx (bare-metal, no HAL).
 * @author  nhduong
 * @date    Feb 12, 2025
 */

#ifndef INC_STM32F407XX_GPIO_H_
#define INC_STM32F407XX_GPIO_H_

#include "stm32f407xx.h"
#include "stm32f407xx_nvic.h"

/**
 * @brief GPIO pin configuration.
 */
typedef struct
{
    uint8_t Pin;        /**< Pin number, a value of @ref GPIO_Pins.               */
    uint8_t Mode;       /**< Operating mode, a value of @ref GPIO_Modes.          */
    uint8_t Pull;       /**< Pull-up/pull-down, a value of @ref GPIO_Pulls.       */
    uint8_t Speed;      /**< Output speed, a value of @ref GPIO_Speeds.           */
    uint8_t OPType;     /**< Output type, a value of @ref GPIO_OutputTypes.       */
    uint8_t Alternate;  /**< Alternate function, a value of @ref GPIO_AltFuncs.   */
} GPIO_InitTypeDef;

/**
 * @brief GPIO handle: port + pin configuration.
 */
typedef struct
{
    GPIO_RegDef_t    *pGPIOx;   /**< Base address of the GPIO port (GPIOA..GPIOI). */
    GPIO_InitTypeDef Init;      /**< Pin configuration settings.                   */
} GPIO_HandleTypeDef;

/**
 * @brief GPIO pin logic level.
 */
typedef enum
{
    GPIO_PIN_RESET = 0,     /**< Low level.  */
    GPIO_PIN_SET            /**< High level. */
} GPIO_PinState;

/**
 * @defgroup GPIO_Pins GPIO pin numbers
 * @note These are pin NUMBERS (0..15), not bit masks like in ST HAL.
 * @{
 */
#define GPIO_PIN_0      0U
#define GPIO_PIN_1      1U
#define GPIO_PIN_2      2U
#define GPIO_PIN_3      3U
#define GPIO_PIN_4      4U
#define GPIO_PIN_5      5U
#define GPIO_PIN_6      6U
#define GPIO_PIN_7      7U
#define GPIO_PIN_8      8U
#define GPIO_PIN_9      9U
#define GPIO_PIN_10     10U
#define GPIO_PIN_11     11U
#define GPIO_PIN_12     12U
#define GPIO_PIN_13     13U
#define GPIO_PIN_14     14U
#define GPIO_PIN_15     15U
/** @} */

/**
 * @defgroup GPIO_Modes GPIO modes
 * @note Values 0..3 match the MODER register encoding,
 *       values >= 4 are software-only modes that use EXTI.
 * @{
 */
#define GPIO_MODE_INPUT                 0U  /**< Input.                               */
#define GPIO_MODE_OUTPUT                1U  /**< General purpose output.              */
#define GPIO_MODE_AF                    2U  /**< Alternate function.                  */
#define GPIO_MODE_ANALOG                3U  /**< Analog.                              */
#define GPIO_MODE_IT_FALLING            4U  /**< Input + EXTI on falling edge.        */
#define GPIO_MODE_IT_RISING             5U  /**< Input + EXTI on rising edge.         */
#define GPIO_MODE_IT_RISING_FALLING     6U  /**< Input + EXTI on both edges.          */
/** @} */

/**
 * @defgroup GPIO_OutputTypes GPIO output types
 * @{
 */
#define GPIO_OPTYPE_PP      0U  /**< Push-pull.  */
#define GPIO_OPTYPE_OD      1U  /**< Open-drain. */
/** @} */

/**
 * @defgroup GPIO_Speeds GPIO output speeds
 * @{
 */
#define GPIO_SPEED_LOW      0U
#define GPIO_SPEED_MEDIUM   1U
#define GPIO_SPEED_FAST     2U
#define GPIO_SPEED_HIGH     3U
/** @} */

/**
 * @defgroup GPIO_Pulls GPIO pull-up/pull-down
 * @{
 */
#define GPIO_NOPULL         0U  /**< No pull-up, no pull-down. */
#define GPIO_PULLUP         1U  /**< Pull-up.                  */
#define GPIO_PULLDOWN       2U  /**< Pull-down.                */
/** @} */

/**
 * @defgroup GPIO_AltFuncs GPIO alternate functions
 * @note See the datasheet "Alternate function mapping" table for which
 *       peripheral each AF number selects on a given pin.
 * @{
 */
#define AF0     0U
#define AF1     1U
#define AF2     2U
#define AF3     3U
#define AF4     4U
#define AF5     5U
#define AF6     6U
#define AF7     7U
#define AF8     8U
#define AF9     9U
#define AF10    10U
#define AF11    11U
#define AF12    12U
#define AF13    13U
#define AF14    14U
#define AF15    15U
/** @} */

/**
 * @defgroup GPIO_Check_Macros GPIO parameter check macros
 * @{
 */
#define IS_GPIO_PIN(PIN)            ((PIN) <= GPIO_PIN_15)
#define IS_GPIO_PIN_ACTION(ACTION)  (((ACTION) == GPIO_PIN_RESET) || ((ACTION) == GPIO_PIN_SET))
#define IS_GPIO_MODE(MODE)          ((MODE) <= GPIO_MODE_IT_RISING_FALLING)
#define IS_GPIO_SPEED(SPEED)        ((SPEED) <= GPIO_SPEED_HIGH)
#define IS_GPIO_PULL(PULL)          ((PULL) <= GPIO_PULLDOWN)
#define IS_GPIO_AF(AF)              ((AF) <= AF15)
/** @} */

/* Clock control ----------------------------------------------------------- */
void GPIO_PeriClockControl(GPIO_RegDef_t *pGPIOx, uint8_t clockState);

/* Init / de-init ---------------------------------------------------------- */
void GPIO_Init(GPIO_HandleTypeDef *hGPIO);
void GPIO_ConfigPin(GPIO_RegDef_t *pGPIOx, uint8_t GPIO_pin, uint8_t mode, uint8_t pull);
void GPIO_DeInit(GPIO_RegDef_t *pGPIOx);

/* IO operations ----------------------------------------------------------- */
uint8_t  GPIO_ReadPin(GPIO_RegDef_t *pGPIOx, uint8_t GPIO_pin);
uint16_t GPIO_ReadPort(GPIO_RegDef_t *pGPIOx);
void     GPIO_WritePin(GPIO_RegDef_t *pGPIOx, uint8_t GPIO_pin, uint8_t pinState);
void     GPIO_WritePort(GPIO_RegDef_t *pGPIOx, uint16_t value);
void     GPIO_TogglePin(GPIO_RegDef_t *pGPIOx, uint8_t GPIO_pin);

/* Interrupt handling ------------------------------------------------------ */
void GPIO_IRQHandler(uint8_t GPIO_pin);
void GPIO_EXTI_Callback(uint8_t GPIO_pin);

#endif /* INC_STM32F407XX_GPIO_H_ */
