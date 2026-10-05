/*
 * stm32f407xx_rcc.c
 *
 *  Read the current system / bus clock frequencies from the RCC registers
 *  (RM0090 7.3.2 RCC_PLLCFGR, 7.3.3 RCC_CFGR).
 */

#include "stm32f407xx_rcc.h"

/* AHB prescaler, index = HPRE[3:0] - 8 (values < 8 mean "not divided") */
static const uint16_t AHB_PreScaler[8] = { 2, 4, 8, 16, 64, 128, 256, 512 };

/* APB prescaler, index = PPREx[2:0] - 4 (values < 4 mean "not divided") */
static const uint8_t APB_PreScaler[4] = { 2, 4, 8, 16 };

/**
 * @brief  Get the system clock (SYSCLK) frequency.
 * @retval SYSCLK in Hz
 */
uint32_t RCC_GetSysClockValue(void)
{
	uint32_t sysclk;
	uint8_t clksrc = (RCC->CFGR >> 2) & 0x3;   // SWS: clock source actually used

	if (clksrc == 0) {
		sysclk = HSI_VALUE;
	} else if (clksrc == 1) {
		sysclk = HSE_VALUE;
	} else {
		// PLL: SYSCLK = (input / PLLM) * PLLN / PLLP
		uint32_t pllm = RCC->PLLCFGR & 0x3F;
		uint32_t plln = (RCC->PLLCFGR >> 6) & 0x1FF;
		uint32_t pllp = (((RCC->PLLCFGR >> 16) & 0x3) + 1) * 2;   // 00 -> 2, 01 -> 4, 10 -> 6, 11 -> 8
		uint32_t pllin = (RCC->PLLCFGR & (1 << 22)) ? HSE_VALUE : HSI_VALUE;   // PLLSRC

		sysclk = ((pllin / pllm) * plln) / pllp;
	}

	return sysclk;
}

/**
 * @brief  Get the AHB clock (HCLK) frequency.
 * @retval HCLK in Hz
 */
uint32_t RCC_GetHCLKValue(void)
{
	uint32_t hpre = (RCC->CFGR >> 4) & 0xF;
	uint32_t div = (hpre < 8) ? 1 : AHB_PreScaler[hpre - 8];

	return RCC_GetSysClockValue() / div;
}

/**
 * @brief  Get the APB1 clock (PCLK1) frequency: I2C1..3, SPI2/3, USART2/3, UART4/5.
 * @retval PCLK1 in Hz
 */
uint32_t RCC_GetPCLK1_Value(void)
{
	uint32_t ppre1 = (RCC->CFGR >> 10) & 0x7;
	uint32_t div = (ppre1 < 4) ? 1 : APB_PreScaler[ppre1 - 4];

	return RCC_GetHCLKValue() / div;
}

/**
 * @brief  Get the APB2 clock (PCLK2) frequency: SPI1, USART1/6.
 * @retval PCLK2 in Hz
 */
uint32_t RCC_GetPCLK2_Value(void)
{
	uint32_t ppre2 = (RCC->CFGR >> 13) & 0x7;
	uint32_t div = (ppre2 < 4) ? 1 : APB_PreScaler[ppre2 - 4];

	return RCC_GetHCLKValue() / div;
}
