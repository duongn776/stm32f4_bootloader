/**
 * @file    stm32f407xx_nvic.h
 * @brief   NVIC helpers shared by all drivers (GPIO, SPI, I2C, UART, ...).
 */

#ifndef INC_STM32F407XX_NVIC_H_
#define INC_STM32F407XX_NVIC_H_

#include "stm32f407xx.h"

/** Highest IRQ number on STM32F407 (82 maskable interrupts: 0..81, RM0090 Table 62). */
#define NVIC_IRQ_MAX    81U

void NVIC_IRQConfig(uint8_t IRQNumber, uint8_t state);
void NVIC_SetPriority(uint8_t IRQNumber, uint8_t priority);

#endif /* INC_STM32F407XX_NVIC_H_ */
