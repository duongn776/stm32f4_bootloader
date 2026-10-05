/*
 * stm32f407xx_flash.h
 *
 *  Created on: Oct 5, 2026
 *      Author: nhduo
 */

#ifndef INC_STM32F407XX_FLASH_H_
#define INC_STM32F407XX_FLASH_H_

#include "stm32f407xx.h"

/**
  * @brief  FLASH Status structures definition
  */
typedef enum
{
  FLASH_OK       = 0x00U,
  FLASH_ERROR    = 0x01U,
} Flash_Status;


/** @defgroup FLASH_Keys FLASH Keys
  * @{
  */
#define FLASH_KEY1               0x45670123U
#define FLASH_KEY2               0xCDEF89ABU

/** @defgroup FLASH_Sectors FLASH Sectors (STM32F407VG, 1 MB)
  *   Sector 0..3  : 16 KB  from 0x0800 0000
  *   Sector 4     : 64 KB  from 0x0801 0000
  *   Sector 5..11 : 128 KB from 0x0802 0000
  * @{
  */
#define FLASH_SECTOR_NUM         12U

/** @defgroup FLASH_Program_Size FLASH_CR PSIZE value
  *   x32 for a 2.7 V - 3.6 V supply
  * @{
  */
#define FLASH_PSIZE_X32          2U


/*
 * FLASH_CR must be unlocked before erase/program, then locked again
 */
Flash_Status Flash_Unlock(void);
void Flash_Lock(void);
Flash_Status Flash_EraseSector(uint8_t Sector);
Flash_Status Flash_ProgramWord(uint32_t Address, uint32_t Data);



#endif /* INC_STM32F407XX_FLASH_H_ */
