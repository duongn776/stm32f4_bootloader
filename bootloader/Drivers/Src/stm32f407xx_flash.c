/*
 * stm32f407xx_flash.c
 *
 *  Created on: Oct 5, 2026
 *      Author: nhduo
 */

#include "stm32f407xx_flash.h"

/* Error flags of FLASH_SR (cleared by writing 1) */
#define FLASH_SR_ERRORS     ((1U << FLASH_SR_OPERR)  | (1U << FLASH_SR_WRPERR) | \
                             (1U << FLASH_SR_PGAERR) | (1U << FLASH_SR_PGPERR) | \
                             (1U << FLASH_SR_PGSERR))


/**
  * @brief  Wait for the end of the operation, then check and clear the error flags.
  * @retval FLASH_OK or FLASH_ERROR.
  */
static Flash_Status Flash_WaitForLastOperation(void)
{
	uint32_t errors;

	while (FLASH->SR & (1U << FLASH_SR_BSY));

	errors = FLASH->SR & FLASH_SR_ERRORS;
	if (errors)
	{
		FLASH->SR = errors;
		return FLASH_ERROR;
	}

	return FLASH_OK;
}

/**
  * @brief  Unlock FLASH_CR with the key sequence.
  * @retval FLASH_OK or FLASH_ERROR.
  */
Flash_Status Flash_Unlock(void)
{
	if (FLASH->CR & (1U << FLASH_CR_LOCK))
	{
		FLASH->KEYR = FLASH_KEY1;
		FLASH->KEYR = FLASH_KEY2;

		if (FLASH->CR & (1U << FLASH_CR_LOCK))
		{
			return FLASH_ERROR;
		}
	}

	return FLASH_OK;
}

/**
  * @brief  Lock FLASH_CR.
  * @retval None
  */
void Flash_Lock(void)
{
	FLASH->CR |= (1U << FLASH_CR_LOCK);
}

/**
  * @brief  Erase one sector.
  * @param  Sector 0 .. FLASH_SECTOR_NUM - 1
  * @retval FLASH_OK or FLASH_ERROR.
  */
Flash_Status Flash_EraseSector(uint8_t Sector)
{
	Flash_Status status;

	status = Flash_WaitForLastOperation();
	if (status != FLASH_OK)
	{
		return status;
	}

	/* Clear PSIZE and SNB first, then set sector erase, sector number and x32 */
	FLASH->CR &= ~((3U << FLASH_CR_PSIZE) | (0xFU << FLASH_CR_SNB));
	FLASH->CR |= (FLASH_PSIZE_X32 << FLASH_CR_PSIZE) | ((uint32_t)Sector << FLASH_CR_SNB) | (1U << FLASH_CR_SER);

	/* Start */
	FLASH->CR |= (1U << FLASH_CR_STRT);

	status = Flash_WaitForLastOperation();

	/* Disable the SER and SNB bits */
	FLASH->CR &= ~((1U << FLASH_CR_SER) | (0xFU << FLASH_CR_SNB));

	return status;
}

/**
  * @brief  Program one 32-bit word. The word must be erased before.
  * @param  Address Word aligned address
  * @param  Data    Value to write
  * @retval FLASH_OK or FLASH_ERROR.
  */
Flash_Status Flash_ProgramWord(uint32_t Address, uint32_t Data)
{
	Flash_Status status;

	status = Flash_WaitForLastOperation();
	if (status != FLASH_OK)
	{
		return status;
	}

	/* Program size x32, then set PG */
	FLASH->CR &= ~(3U << FLASH_CR_PSIZE);
	FLASH->CR |= (FLASH_PSIZE_X32 << FLASH_CR_PSIZE) | (1U << FLASH_CR_PG);

	*(__vo uint32_t *)Address = Data;

	status = Flash_WaitForLastOperation();

	/* Disable the PG bit */
	FLASH->CR &= ~(1U << FLASH_CR_PG);

	return status;
}
