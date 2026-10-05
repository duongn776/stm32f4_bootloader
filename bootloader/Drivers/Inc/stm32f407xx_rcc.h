/*
 * stm32f407xx_rcc.h
 *
 *  Read the current system / bus clock frequencies from the RCC registers.
 */

#ifndef INC_STM32F407XX_RCC_H_
#define INC_STM32F407XX_RCC_H_

#include "stm32f407xx.h"

/* Internal RC oscillator frequency (fixed) */
#define HSI_VALUE           16000000U

/* External crystal frequency: 8 MHz on the STM32F407 Discovery board */
#ifndef HSE_VALUE
#define HSE_VALUE           8000000U
#endif

uint32_t RCC_GetSysClockValue(void);
uint32_t RCC_GetHCLKValue(void);
uint32_t RCC_GetPCLK1_Value(void);
uint32_t RCC_GetPCLK2_Value(void);

#endif /* INC_STM32F407XX_RCC_H_ */
