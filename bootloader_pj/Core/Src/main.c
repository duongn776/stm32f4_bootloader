/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <stdarg.h>
#include <string.h>
#include <stdio.h>
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */
#define FLASH_SECTOR2_BASE_ADDESS	0x8008000U
#define BL_RX_LEN					200U
/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
CRC_HandleTypeDef hcrc;

UART_HandleTypeDef huart2;

/* USER CODE BEGIN PV */
uint8_t bl_rx_buffer[BL_RX_LEN];
uint8_t supported_commands[] = {
                                 BL_GET_VER ,
                                 BL_GET_HELP,
                                 BL_GET_CID,
                                 BL_GET_RDP_STATUS,
                                 BL_GO_TO_ADDR,
                                 BL_FLASH_ERASE,
                                 BL_MEM_WRITE,
                                 BL_READ_SECTOR_P_STATUS} ;

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_CRC_Init(void);
static void MX_USART2_UART_Init(void);
/* USER CODE BEGIN PFP */
static void printmsg(char *format,...);
void  bootloader_uart_read_data(void);
void  bootloader_jump_to_user_app(void);
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_CRC_Init();
  MX_USART2_UART_Init();
  /* USER CODE BEGIN 2 */
  /* Lets check whether button is pressed or not, if not pressed jump to user application */
	if (HAL_GPIO_ReadPin(B1_GPIO_Port, B1_Pin)) {

		/* Bootloader mode */
		///printmsg("BL_DEBUG_MSG:Bootloader mode\n");
		bootloader_uart_read_data();
	} else {

		/* User application mode */
		//printmsg("BL_DEBUG_MSG:User application mode\n");
		bootloader_jump_to_user_app();

	}
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Configure the main internal regulator output voltage
  */
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
  RCC_OscInitStruct.PLL.PLLM = 8;
  RCC_OscInitStruct.PLL.PLLN = 50;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = 7;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV8;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV4;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_1) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief CRC Initialization Function
  * @param None
  * @retval None
  */
static void MX_CRC_Init(void)
{

  /* USER CODE BEGIN CRC_Init 0 */

  /* USER CODE END CRC_Init 0 */

  /* USER CODE BEGIN CRC_Init 1 */

  /* USER CODE END CRC_Init 1 */
  hcrc.Instance = CRC;
  if (HAL_CRC_Init(&hcrc) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN CRC_Init 2 */

  /* USER CODE END CRC_Init 2 */

}

/**
  * @brief USART2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART2_UART_Init(void)
{

  /* USER CODE BEGIN USART2_Init 0 */

  /* USER CODE END USART2_Init 0 */

  /* USER CODE BEGIN USART2_Init 1 */

  /* USER CODE END USART2_Init 1 */
  huart2.Instance = USART2;
  huart2.Init.BaudRate = 115200;
  huart2.Init.WordLength = UART_WORDLENGTH_8B;
  huart2.Init.StopBits = UART_STOPBITS_1;
  huart2.Init.Parity = UART_PARITY_NONE;
  huart2.Init.Mode = UART_MODE_TX_RX;
  huart2.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart2.Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_UART_Init(&huart2) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART2_Init 2 */

  /* USER CODE END USART2_Init 2 */

}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
  /* USER CODE BEGIN MX_GPIO_Init_1 */

  /* USER CODE END MX_GPIO_Init_1 */

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOA_CLK_ENABLE();

  /*Configure GPIO pin : B1_Pin */
  GPIO_InitStruct.Pin = B1_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_EVT_RISING;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(B1_GPIO_Port, &GPIO_InitStruct);

  /* USER CODE BEGIN MX_GPIO_Init_2 */

  /* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */
static void printmsg(char *format,...) {
  char str[80];

  /*Extract the the argument list using VA apis */
  va_list args;
  va_start(args, format);
  vsprintf(str, format,args);
  HAL_UART_Transmit(&huart2,(uint8_t *)str, strlen(str),HAL_MAX_DELAY);
  va_end(args);
}

void  bootloader_uart_read_data(void) {
  uint8_t rcv_len=0;

  while(1) {
    memset(bl_rx_buffer,0,200);
    /* we will read and decode the commands coming from host */
    /* first read only one byte from the host , which is the "length" field of the command packet */
    HAL_UART_Receive(&huart2, bl_rx_buffer, 1, HAL_MAX_DELAY);
    rcv_len= bl_rx_buffer[0];
    HAL_UART_Receive(&huart2, &bl_rx_buffer[1], rcv_len, HAL_MAX_DELAY);

    switch ( bl_rx_buffer[1] ) {
      case BL_GET_VER:
          bootloader_handle_getver_cmd(bl_rx_buffer);
          break;
      case BL_GET_HELP:
          bootloader_handle_gethelp_cmd(bl_rx_buffer);
          break;
      case BL_GET_CID:
          bootloader_handle_getcid_cmd(bl_rx_buffer);
          break;
      case BL_GET_RDP_STATUS:
          bootloader_handle_getrdp_cmd(bl_rx_buffer);
          break;
      case BL_GO_TO_ADDR:
          bootloader_handle_go_cmd(bl_rx_buffer);
          break;
      case BL_FLASH_ERASE:
          bootloader_handle_flash_erase_cmd(bl_rx_buffer);
          break;
      case BL_MEM_WRITE:
          bootloader_handle_mem_write_cmd(bl_rx_buffer);
          break;
      case BL_EN_RW_PROTECT:
          bootloader_handle_en_rw_protect(bl_rx_buffer);
          break;
      case BL_MEM_READ:
          bootloader_handle_mem_read(bl_rx_buffer);
          break;
      case BL_READ_SECTOR_P_STATUS:
          bootloader_handle_read_sector_protection_status(bl_rx_buffer);
          break;
      case BL_OTP_READ:
          bootloader_handle_read_otp(bl_rx_buffer);
          break;
      case BL_DIS_R_W_PROTECT:
          bootloader_handle_dis_rw_protect(bl_rx_buffer);
          break;
       default:
         // printmsg("BL_DEBUG_MSG:Invalid command code received from host \n");
          break;

    }
  }
}
void  bootloader_jump_to_user_app(void)
{
	/* Function pointer to hold the address of the reset handler of the user app */
	void (*app_reset_handler)(void);

	printmsg("BL_DEBUG_MSG:bootloader_jump_to_user_app\n");

	/* Configure the MSP by reading the value from base address of the SECTOR 2 */
	uint32_t msp_value = *(volatile uint32_t *)FLASH_SECTOR2_BASE_ADDESS;
	printmsg("BL_DEBUG_MSG:MSP value : %#x\n",msp_value);

	/* Set MSP */
	__set_MSP(msp_value);

	/* Fetch the reset handler address from the user application  from the location FLASH_SECTOR2_BASE_ADDESS */
	uint32_t reset_handler_address = *(volatile uint32_t *) (FLASH_SECTOR2_BASE_ADDESS + 4);

	app_reset_handler = (void *)reset_handler_address;

	printmsg("BL_DEBUG_MSG: app reset handler addr : %#x\n",app_reset_handler);

	/* Jump to reset handler of the user application */
	app_reset_handler();
}

/* Helper function to handle BL_GET_VER command */
void bootloader_handle_getver_cmd(uint8_t *bl_rx_buffer)
{
	uint8_t bl_version = 0;

	/* Total length of the command packet */
	uint32_t command_packet_len = bl_rx_buffer[0] + 1;

	/* Extract the CRC32 send by HOST */
	uint32_t host_crc = *((uint32_t *)(bl_rx_buffer + command_packet_len - 4));

	if (! bootloader_verify_crc(&bl_rx_buffer[0], command_packet_len - 4, host_crc))
	{
		bootloader_send_ack(bl_rx_buffer[0], 1);
		bl_version = get_bootloader_version();
		bootloader_uart_write_data(&bl_version, 1);
	}
	else {
		// checksum is wrong send nack
		bootloader_send_nack();
	}

}
void bootloader_handle_gethelp_cmd(uint8_t *pBuffer)
{
	// Total length of the command packet
	uint32_t common_packet_len = pBuffer[0] + 1;

	// extract the CRC32 sent by the Host
	uint32_t host_crc = *((uint32_t *)(pBuffer + common_packet_len - 4));

	if (! bootloader_verify_crc(&pBuffer[0], common_packet_len - 4, host_crc))
	{
		bootloader_send_ack(pBuffer[0], sizeof(supported_commands));
		bootloader_uart_write_data(supported_commands, sizeof(supported_commands));
	}
	else {
		bootloader_send_nack();
	}
}

/*Helper function to handle BL_GET_CID command */
void bootloader_handle_getcid_cmd(uint8_t *pBuffer)
{
	uint16_t bl_chip_id = 0;

	// Total length of the command packet
	uint32_t command_packet_len = pBuffer[0] + 1;

	//extract the CRC32 sent by the Host
	uint32_t host_crc = *((uint32_t *)(pBuffer + command_packet_len - 4));

	if (! bootloader_verify_crc(pBuffer, command_packet_len - 4, host_crc))
	{
		bootloader_send_ack(pBuffer[0], 2);
		bl_chip_id = get_mcu_chip_id();
		bootloader_uart_write_data((uint8_t*)&bl_chip_id, 2);
	}else {
		bootloader_send_nack();
	}


}
void bootloader_handle_getrdp_cmd(uint8_t *pBuffer)
{
	uint8_t rdp_level = 0;
	// Total length of the command packet
	uint32_t command_packet_len = pBuffer[0] + 1;

	//extract the CRC32 sent by the Host
	uint32_t host_crc = *((uint32_t *) (pBuffer + command_packet_len - 4));

	if (! bootloader_verify_crc(pBuffer, command_packet_len - 4, host_crc))
	{
		bootloader_send_ack(pBuffer[0], 1);
		rdp_level = get_flash_rdp_level();
		bootloader_uart_write_data(&rdp_level, 1);
	}else {
		bootloader_send_nack();
	}
}

/*Helper function to handle BL_GO_TO_ADDR command */
void bootloader_handle_go_cmd(uint8_t *pBuffer)
{
	uint32_t go_address = 0;
	uint8_t addr_valid = ADDR_VALID;
	uint8_t addr_invalid = ADDR_INVALID;

	// Total length of the command packet
	uint32_t command_packet_len = pBuffer[0] + 1;

	//extract the CRC32 sent by the Host
	uint32_t host_crc = *((uint32_t*) (pBuffer + command_packet_len - 4));

	if (!bootloader_verify_crc(bl_rx_buffer, command_packet_len - 4, host_crc)) {
		bootloader_send_ack(pBuffer[0], 1);
		//extract the go address
		go_address = *((uint32_t *)&pBuffer[2]);
		if (verify_address(go_address) == ADDR_VALID)
		{
			//tell host that address is fine
			bootloader_uart_write_data(&addr_valid, 1);

			go_address += 1; //make T bit = 1

			void (*lets_jump)(void) = (void *)go_address;
			lets_jump();
		}else {
			//tell host that address is invalid
			bootloader_uart_write_data(&addr_invalid,1);
		}
	} else {
		bootloader_send_nack();
	}
}

void bootloader_handle_flash_erase_cmd(uint8_t *pBuffer)
{
	uint8_t earse_status = 0;

	// Total length of the command packet
	uint32_t command_packet_len = pBuffer[0] + 1;

	// Extract the CRC32 sent by the Host
	uint32_t host_crc = *((uint32_t *) (pBuffer + command_packet_len - 4));

	if (! bootloader_verify_crc(&pBuffer[0], command_packet_len-4, host_crc))
	{
		bootloader_send_ack(pBuffer[0],1);
		HAL_GPIO_WritePin(GPIOD, GPIO_PIN_12, 1);
		earse_status = execute_flash_erase(pBuffer[2] , pBuffer[3]);
		HAL_GPIO_WritePin(GPIOD, GPIO_PIN_12, 0);
		bootloader_uart_write_data(&earse_status, 1);
	}
	else {
		bootloader_send_nack();
	}
}
void bootloader_handle_mem_write_cmd(uint8_t *pBuffer)
{
	uint8_t write_status = 0;
	uint8_t payload_len = pBuffer[6];
	uint32_t mem_address = *((uint32_t *)(&pBuffer[2]));


	uint32_t command_packet_len = pBuffer[0] + 1;
	 uint32_t host_crc = *((uint32_t * ) (pBuffer + command_packet_len - 4) );
	 if (! bootloader_verify_crc(&bl_rx_buffer[0], command_packet_len - 4, host_crc))
	 {
		 bootloader_send_ack(pBuffer[0], 1);

		 if (verify_address(mem_address) == ADDR_VALID)
		 {
			 write_status = execute_mem_write(&pBuffer[7], mem_address, payload_len);
			 // inform host that address is valid
			 bootloader_uart_write_data(&write_status, 1);
		 }
		 else {
			 write_status = ADDR_INVALID;
			 // inform host that address is invalid
			 bootloader_uart_write_data(&write_status,1);
		 }
	 }
	 else {
		 bootloader_send_nack();
	 }
}
void bootloader_handle_en_rw_protect(uint8_t *pBuffer)
{

}
void bootloader_handle_mem_read (uint8_t *pBuffer)
{

}
void bootloader_handle_read_sector_protection_status(uint8_t *pBuffer)
{

}
void bootloader_handle_read_otp(uint8_t *pBuffer)
{

}
void bootloader_handle_dis_rw_protect(uint8_t *pBuffer)
{

}

//This verifies the CRC of the given buffer in pData .
uint8_t bootloader_verify_crc (uint8_t *pData, uint32_t len,uint32_t crc_host)
{
	uint32_t check_value = 0xFF;

	for (uint32_t i = 0; i < len; i++)
	{
		uint32_t data = pData[i];
		check_value = HAL_CRC_Accumulate(&hcrc, &data, 1);
	}

	/* Reset CRC Calculation Unit */
	__HAL_CRC_DR_RESET(&hcrc);

	if (check_value == crc_host)
	{
		return VERIFY_CRC_SUCCESS;
	}

	return VERIFY_CRC_FAIL;
}

/* This function sends ACK if CRC matches along with "len to follow" */
void bootloader_send_ack(uint8_t command_code, uint8_t follow_len)
{
	/* we send 2 byte.. first byte is ack and the second byte is len value */
	uint8_t ack_buf[2];
	ack_buf[0] = BL_ACK;
	ack_buf[1] = follow_len;
	HAL_UART_Transmit(&huart2, ack_buf, 2, HAL_MAX_DELAY);
}

/*This function sends NACK */
void bootloader_send_nack(void) {

  uint8_t nack = BL_NACK;
  HAL_UART_Transmit(&huart2, &nack, 1, HAL_MAX_DELAY);
}


//Just returns the macro value .
uint8_t get_bootloader_version(void)
{
  return (uint8_t)BL_VERSION;
}

/* This function writes data */
void bootloader_uart_write_data(uint8_t *pBuffer,uint32_t len)
{
  HAL_UART_Transmit(&huart2, pBuffer , len, HAL_MAX_DELAY);
}

//Read the chip identifier or device Identifier
uint16_t get_mcu_chip_id(void)
{
	uint16_t chip_id;
	chip_id = (uint16_t)(DBGMCU->IDCODE) & 0x0FFF;

	return chip_id;
}

uint8_t get_flash_rdp_level(void)
{
	uint8_t rdp_status = 0;
#if 0
  FLASH_OBProgramInitTypeDef  ob_handle;
  HAL_FLASHEx_OBGetConfig(&ob_handle);
  rdp_status = (uint8_t)ob_handle.RDPLevel;
#else
	volatile uint32_t *option_bytes = (volatile uint32_t*)0x1FFFC000;
	uint32_t option_value = *option_bytes;
	rdp_status = (uint8_t) ((option_value >> 8) & 0xFF);
#endif
	return rdp_status;

}

//verify the address sent by the host .
uint8_t verify_address(uint32_t go_address) {
  if ( go_address >= SRAM1_BASE && go_address <= SRAM1_END) {
    return ADDR_VALID;
  } else if ( go_address >= SRAM2_BASE && go_address <= SRAM2_END) {

    return ADDR_VALID;
  } else if ( go_address >= FLASH_BASE && go_address <= FLASH_END) {

    return ADDR_VALID;
  } else if ( go_address >= BKPSRAM_BASE && go_address <= BKPSRAM_END) {

    return ADDR_VALID;
  } else
    return ADDR_INVALID;
}

uint8_t execute_flash_erase(uint8_t sector_number , uint8_t number_of_sector)
{
	FLASH_EraseInitTypeDef flashErase_handle = {0};
	uint32_t sectorError = 0;
	HAL_StatusTypeDef status;

	if (sector_number == 0xFF)
	{
		flashErase_handle.Banks = FLASH_BANK_1;
		flashErase_handle.TypeErase = FLASH_TYPEERASE_MASSERASE;
	}else if (sector_number <= 11)
	{
		if (number_of_sector == 0)
		{
			return INVALID_SECTOR;
		}

		flashErase_handle.Banks = FLASH_BANK_1;
		/* Limit a range that would extend beyond the last sector. */
		uint8_t remaining_sector = (uint8_t) (12U - sector_number);
		if (number_of_sector > remaining_sector) {
			number_of_sector = remaining_sector;
		}
		 flashErase_handle.TypeErase = FLASH_TYPEERASE_SECTORS;
		 flashErase_handle.Sector = sector_number; // this is the initial sector
		 flashErase_handle.NbSectors = number_of_sector;
	}else {
		return INVALID_SECTOR;
	}

	/* Get access to touch the flash registers. */
	HAL_FLASH_Unlock();
	flashErase_handle.VoltageRange = FLASH_VOLTAGE_RANGE_3;
	status = HAL_FLASHEx_Erase(&flashErase_handle, &sectorError);
	HAL_FLASH_Lock();

	return (uint8_t)status;
}

/*This function writes the contents of pBuffer to  "mem_address" byte by byte */
//Note1 : Currently this function supports writing to Flash only .
//Note2 : This functions does not check whether "mem_address" is a valid address of the flash range.
uint8_t execute_mem_write(uint8_t *pBuffer, uint32_t mem_address, uint32_t len)
{
	 uint8_t status = HAL_OK;
	 // We have to unlock flash module to get control of registers
	 HAL_FLASH_Unlock();

	 for (uint32_t i = 0; i < len; i++)
	 {
		 status = HAL_FLASH_Program(FLASH_TYPEPROGRAM_BYTE, mem_address + i, pBuffer[i]);
	 }

	 HAL_FLASH_Lock();

	 return status;
}

uint8_t configure_flash_sector_rw_protection(uint8_t sector_details, uint8_t protection_mode, uint8_t disable);

uint16_t read_OB_rw_protection_status(void);



/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}
#ifdef USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
