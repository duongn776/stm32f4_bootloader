/*
 * stm32f407xx_spi.c
 *
 *  Created on: Feb 19, 2025
 *      Author: nhduong
 */

#include "stm32f407xx_spi.h"

static void spi_txe_interrupt_handler(SPI_HandleTypeDef *hspi);
static void spi_rxne_interrupt_handler(SPI_HandleTypeDef *hspi);
static void spi_ovr_error_interrupt_handler(SPI_HandleTypeDef *hspi);


/**
   * @brief  Initialize SPI1 peripheral
   * @retval None
   */
 void SPI1_Init(SPI_HandleTypeDef *spi1)
 {
 	spi1->pSPIx = SPI1;
 	spi1->Init.Direction = SPI_DIRECTION_FD;
  	spi1->Init.Mode = SPI_MODE_MASTER;
  	spi1->Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_2;//generates sclk of 8MHz
  	spi1->Init.DataSize = SPI_DATASIZE_8BIT;
  	spi1->Init.CLKPhase = SPI_PHASE_1stEDGE;
  	spi1->Init.CLKPolarity = SPI_POLARITY_LOW;
  	spi1->Init.NSS = SPI_NSS_HARD;
  	SPI_Init(spi1);
  	SPI1_GPIOInits();
  	SPI_PeripheralControl(SPI1,ENABLE);
  }

/**
   * @brief  Initialize SPI2 peripheral
   * @retval None
   */
 void SPI2_Init(SPI_HandleTypeDef *spi2)
 {
	 spi2->pSPIx = SPI2;
	 spi2->Init.Direction = SPI_DIRECTION_FD;
	 spi2->Init.Mode = SPI_MODE_MASTER;
	 spi2->Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_2;//generates sclk of 8MHz
	 spi2->Init.DataSize = SPI_DATASIZE_8BIT;
	 spi2->Init.CLKPhase = SPI_PHASE_1stEDGE;
	 spi2->Init.CLKPolarity = SPI_POLARITY_LOW;
	 spi2->Init.NSS = SPI_NSS_HARD;
  	SPI_Init(spi2);
  	SPI2_GPIOInits();
  	SPI_PeripheralControl(SPI2,ENABLE);
  }

/**
  * @brief  Initialize GPIO pins for SPI1
  * @retval None
  *
  */
void SPI1_GPIOInits(void)
 {
 	GPIO_HandleTypeDef GPIO_Pin;

 	GPIO_Pin.pGPIOx = GPIOA;
 	GPIO_Pin.Init.Mode = GPIO_MODE_AF;
 	GPIO_Pin.Init.Alternate = 5;
 	GPIO_Pin.Init.OPType = GPIO_OPTYPE_PP;
 	GPIO_Pin.Init.Pull = GPIO_NOPULL;
 	GPIO_Pin.Init.Speed = GPIO_SPEED_FAST;

 	//SCLK
 	GPIO_Pin.Init.Pin = GPIO_PIN_5;
 	GPIO_Init(&GPIO_Pin);

 	//MISO
 	GPIO_Pin.Init.Pin = GPIO_PIN_6;
 	GPIO_Init(&GPIO_Pin);

 	//MOSI
 	GPIO_Pin.Init.Pin = GPIO_PIN_7;
 	GPIO_Init(&GPIO_Pin);

 	//NSS
 	GPIO_Pin.Init.Pin = GPIO_PIN_4;
 	GPIO_Init(&GPIO_Pin);
 }

/**
   * @brief  Initialize GPIO pins for SPI2
   * @retval None
   *
   */
void SPI2_GPIOInits(void)
  {
  	GPIO_HandleTypeDef GPIO_Pin;

  	GPIO_Pin.pGPIOx = GPIOB;
  	GPIO_Pin.Init.Mode = GPIO_MODE_AF;
  	GPIO_Pin.Init.Alternate = 5;
  	GPIO_Pin.Init.OPType = GPIO_OPTYPE_PP;
  	GPIO_Pin.Init.Pull = GPIO_NOPULL;
  	GPIO_Pin.Init.Speed = GPIO_SPEED_FAST;

  	//SCLK
  	GPIO_Pin.Init.Pin = GPIO_PIN_13;
  	GPIO_Init(&GPIO_Pin);

  	//MISO
  	GPIO_Pin.Init.Pin = GPIO_PIN_14;
  	GPIO_Init(&GPIO_Pin);

  	//MOSI
  	GPIO_Pin.Init.Pin = GPIO_PIN_15;
  	GPIO_Init(&GPIO_Pin);

  	//NSS
  	GPIO_Pin.Init.Pin = GPIO_PIN_12;
  	GPIO_Init(&GPIO_Pin);
  }

/**
 * @brief  Enables or disables the clock for the specified SPI peripheral.
 * @param  pSPIx Pointer to SPI_RegDef_t structure representing SPI1, SPI2, or SPI3.
 * @param  clockState ENABLE (1) to enable the clock, DISABLE (0) to disable it.
 * @retval None
 */
void SPI_PeriClockControl(SPI_RegDef_t *pSPIx, uint8_t clockState) {

	if (clockState == ENABLE) {
		if (pSPIx == SPI1) {
			SPI1_CLK_ENABLE();
			(void)RCC->APB2ENR; // Errata: delay after enabling the clock
		} else if (pSPIx == SPI2) {
			SPI2_CLK_ENABLE();
			(void)RCC->APB1ENR;
		} else if (pSPIx == SPI3) {
			SPI3_CLK_ENABLE();
			(void)RCC->APB1ENR;
		}
	} else {
		if (pSPIx == SPI1) {
			SPI1_CLK_DISABLE();
		} else if (pSPIx == SPI2) {
			SPI2_CLK_DISABLE();
		} else if (pSPIx == SPI3) {
			SPI3_CLK_DISABLE();
		}
	}
}

/**
  * @brief  Initialize the SPI according to the specified parameters
  *         in the SPI_InitTypeDef and initialize the associated handle.
  *
  * @param  hspi pointer to a SPI_HandleTypeDef structure that contains
  *               the configuration information for SPI module.
  *
  * @retval None
  */
void SPI_Init(SPI_HandleTypeDef *hspi)
{
	uint32_t tempreg = 0;

	// 1. Enable the SPI peripheral clock
	SPI_PeriClockControl(hspi->pSPIx, ENABLE);

	// 2. Configure device mode (Master/Slave)
	tempreg |= hspi->Init.Mode << SPI_CR1_MSTR;

	// 3. Configure bus configuration (Full-Duplex, Half-Duplex, or Simplex RX-only)
	if (hspi->Init.Direction == SPI_DIRECTION_FD)
	{
		tempreg &= ~(1 << SPI_CR1_BIDIMODE);
	}
	else if (hspi->Init.Direction == SPI_DIRECTION_HD)
	{
		tempreg |= (1 << SPI_CR1_BIDIMODE);
	}
	else // SPI_DIRECTION_SIMPLEX_RXONLY
	{
		tempreg &= ~(1 << SPI_CR1_BIDIMODE);
		tempreg |= (1 << SPI_CR1_RXONLY);
	}

	// 4. Set the Baud Rate Prescaler
	tempreg |= hspi->Init.BaudRatePrescaler << SPI_CR1_BR;

	// 5. Configure Data Frame Format (8-bit or 16-bit)
	tempreg |= hspi->Init.DataSize << SPI_CR1_DFF;

	// 6. Set Clock Polarity (CPOL)
	tempreg |= hspi->Init.CLKPolarity << SPI_CR1_CPOL;

	// 7. Set Clock Phase (CPHA)
	tempreg |= hspi->Init.CLKPhase << SPI_CR1_CPHA;

	// 8. Configure NSS management
	if (hspi->Init.NSS == SPI_NSS_SOFT)
	{
		// Software Slave Management
		tempreg |= (1 << SPI_CR1_SSM);
		// SSI must be in tempreg, CR1 is overwritten below.
		// Master needs SSI = 1, otherwise mode fault (MODF) clears MSTR and SPE.
		// Slave needs SSI = 0 to be selected.
		if (hspi->Init.Mode == SPI_MODE_MASTER)
		{
			tempreg |= (1 << SPI_CR1_SSI);
		}
	}
	else // SPI_NSS_HARD
	{
		tempreg &= ~(1 << SPI_CR1_SSM);
		SPI_SSOEConfig(hspi->pSPIx, ENABLE); // Enable SSOE bit
	}

	// 9. Write final configuration to CR1 register
	hspi->pSPIx->CR1 = tempreg;

	// 10. Reset the interrupt transfer state
	hspi->TxState = SPI_STATE_READY;
	hspi->RxState = SPI_STATE_READY;
	hspi->TxLen = 0;
	hspi->RxLen = 0;
}

/**
 * @brief  De-initializes the SPIx peripheral registers to their default reset values.
 * @param  pSPIx Pointer to SPI_RegDef_t structure representing SPI1, SPI2, or SPI3.
 * @retval None
 */
void SPI_DeInit(SPI_RegDef_t *pSPIx) {
	if (pSPIx == SPI1) {
		SPI1_REG_RESET();
	} else if (pSPIx == SPI2) {
		SPI2_REG_RESET();
	} else if (pSPIx == SPI3) {
		SPI3_REG_RESET();
	}
}

/**
 * @brief  Enables or disables the SPI peripheral.
 * @param  pSPIx  Pointer to the SPI peripheral base address (e.g., SPI1, SPI2).
 * @param  state  ENABLE to turn on the SPI peripheral, DISABLE to turn it off.
 * @retval None
 */
void SPI_PeripheralControl(SPI_RegDef_t *pSPIx, uint8_t state) {
	if (state == ENABLE) {
		pSPIx->CR1 |= (1 << SPI_CR1_SPE);
	} else {
		pSPIx->CR1 &= ~(1 << SPI_CR1_SPE);
	}
}

/**
 * @brief  Configures the SSI (Internal Slave Select) bit.
 * @param  pSPIx  Pointer to the SPI peripheral base address.
 * @param  state  ENABLE to set SSI bit (NSS high), DISABLE to clear it (NSS low).
 * @retval None
 */
void SPI_SSIConfig(SPI_RegDef_t *pSPIx, uint8_t state) {
	if (state == ENABLE) {
		pSPIx->CR1 |= (1 << SPI_CR1_SSI);
	} else {
		pSPIx->CR1 &= ~(1 << SPI_CR1_SSI);
	}
}

/**
 * @brief  Configures the SSOE (Slave Select Output Enable) bit.
 * @param  pSPIx  Pointer to the SPI peripheral base address.
 * @param  state  ENABLE to enable NSS output (SSOE = 1), DISABLE to disable it.
 *
 * @retval None
 */
void SPI_SSOEConfig(SPI_RegDef_t *pSPIx, uint8_t state) {
	if (state == ENABLE) {
		pSPIx->CR2 |= (1 << SPI_CR2_SSOE);
	} else {
		pSPIx->CR2 &= ~(1 << SPI_CR2_SSOE);
	}

}

/**
 * @brief  Checks the status of a specific flag in the SPI Status Register (SR).
 * @param  pSPIx Pointer to the SPI peripheral (SPI1, SPI2 and SPI3).
 * @param  FlagName The flag to check (e.g., SPI_FLAG_TXE, SPI_FLAG_RXNE, SPI_FLAG_BSY).
 * @retval FLAG_SET(1) or FLAG_RESET(0).
 */
uint8_t SPI_GetFlagStatus(SPI_RegDef_t *pSPIx, uint32_t FlagName) {
	return ((pSPIx->SR & FlagName) != 0) ? FLAG_SET : FLAG_RESET;
}

/**
 * @brief  Transmit an amount of data in blocking mode.
 * @note   Full-duplex: every byte sent also receives one byte, it is read
 *         and dropped here so no overrun (OVR) happens.
 * @param  pSPIx Pointer to the SPI peripheral (SPI1, SPI2 and SPI3).
 * @param  pTxBuffer pointer to transmit data buffer
 * @param  Len amount of data to be sent (bytes)
 * @retval None
 */
void SPI_Transmit(SPI_RegDef_t *pSPIx, const uint8_t *pTxBuffer, uint32_t Len) {
    SPI_PeripheralControl(pSPIx, ENABLE);
    SPI_ClearOVRFlag(pSPIx); // drop old received data
    while (Len > 0) {
	// wait until TXE is set
	while (!(pSPIx->SR & SPI_FLAG_TXE));
	// check the data size
	if (pSPIx->CR1 & (1 << SPI_CR1_DFF)) {
		// 16 bit in CR1
		// load data into the DR
		pSPIx->DR = *((const uint16_t*) pTxBuffer);
		Len = (Len >= 2) ? (Len - 2) : 0;
		pTxBuffer += 2;
	} else {
		// 8 bit in DFF
		pSPIx->DR = *pTxBuffer;
		Len--;
		pTxBuffer++;
	}
	// wait for the byte received at the same time and drop it
	while (!(pSPIx->SR & SPI_FLAG_RXNE));
	(void)pSPIx->DR;
    }
    while (SPI_GetFlagStatus(pSPIx, SPI_FLAG_BSY));
    SPI_PeripheralControl(pSPIx, DISABLE);
}

/**
 * @brief  Receive an amount of data in blocking mode.
 * @note   The master only generates the clock when it sends, so a dummy
 *         byte (0xFF) is sent for every byte received.
 * @param  pSPIx Pointer to the SPI peripheral (SPI1, SPI2 and SPI3).
 * @param  pRxBuffer pointer to receive data buffer
 * @param  Len amount of data to be receive (bytes)
 * @retval None
 */
void SPI_Receive(SPI_RegDef_t *pSPIx, uint8_t *pRxBuffer, uint32_t Len) {
	SPI_PeripheralControl(pSPIx, ENABLE);
	SPI_ClearOVRFlag(pSPIx); // drop old received data
	while (Len > 0) {
		// send a dummy byte to generate the clock
		while (!(pSPIx->SR & SPI_FLAG_TXE));
		pSPIx->DR = 0xFFFF;

		// wait until RXNE is set
		while (!SPI_GetFlagStatus(pSPIx, SPI_FLAG_RXNE));

		// Check the data size
		if (pSPIx->CR1 & (1 << SPI_CR1_DFF)) {
			// 16 bit in CR1
			// load data from DR to pRxbuffer
			*((uint16_t*) pRxBuffer) = (uint16_t)pSPIx->DR;
			Len = (Len >= 2) ? (Len - 2) : 0;
			pRxBuffer += 2;
		} else {
			// 8 bit
			*pRxBuffer = (uint8_t)pSPIx->DR;
			Len--;
			pRxBuffer++;
		}
	}
	while (SPI_GetFlagStatus(pSPIx, SPI_FLAG_BSY));
	SPI_PeripheralControl(pSPIx, DISABLE);
}

/**
 * @brief  Transmit and receive at the same time in blocking mode (full-duplex).
 * @note   In SPI every byte sent also receives one byte: pTxBuffer[i] is sent
 *         while pRxBuffer[i] is received.
 *         Example, read a register: tx = {reg | 0x80, 0x00} -> value in rx[1].
 * @param  pSPIx Pointer to the SPI peripheral (SPI1, SPI2 and SPI3).
 * @param  pTxBuffer pointer to transmit data buffer, NULL = send dummy 0xFF
 * @param  pRxBuffer pointer to receive data buffer, NULL = drop received data
 *                   (can be the same buffer as pTxBuffer)
 * @param  Len amount of data to be sent and received (bytes)
 * @retval None
 */
void SPI_TransmitReceive(SPI_RegDef_t *pSPIx, const uint8_t *pTxBuffer, uint8_t *pRxBuffer, uint32_t Len) {
	SPI_PeripheralControl(pSPIx, ENABLE);
	SPI_ClearOVRFlag(pSPIx); // drop old received data
	while (Len > 0) {
		uint8_t is16 = (pSPIx->CR1 & (1 << SPI_CR1_DFF)) ? 1 : 0;

		// wait until TXE is set, then send one frame (dummy if no Tx buffer)
		while (!(pSPIx->SR & SPI_FLAG_TXE));
		if (pTxBuffer == NULL) {
			pSPIx->DR = 0xFFFF;
		} else if (is16) {
			pSPIx->DR = *((const uint16_t*) pTxBuffer);
			pTxBuffer += 2;
		} else {
			pSPIx->DR = *pTxBuffer;
			pTxBuffer++;
		}

		// wait until RXNE is set, then read the frame received at the same time
		while (!(pSPIx->SR & SPI_FLAG_RXNE));
		if (pRxBuffer == NULL) {
			(void)pSPIx->DR;
		} else if (is16) {
			*((uint16_t*) pRxBuffer) = (uint16_t)pSPIx->DR;
			pRxBuffer += 2;
		} else {
			*pRxBuffer = (uint8_t)pSPIx->DR;
			pRxBuffer++;
		}

		// 16 bit frame = 2 bytes of the buffers
		if (is16) {
			Len = (Len >= 2) ? (Len - 2) : 0;
		} else {
			Len--;
		}
	}
	while (SPI_GetFlagStatus(pSPIx, SPI_FLAG_BSY));
	SPI_PeripheralControl(pSPIx, DISABLE);
}

/**
 * @brief  Transmit an amount of data in non-blocking mode with Interrupt.
 * @param  hspi pointer to a SPI_HandleTypeDef structure that contains
 *               the configuration information for SPI module.
 * @param  pTxBuffer pointer to data buffer
 * @param  Len amount of data to be sent
 * @retval State of the mode
 */
uint8_t SPI_Transmit_IT(SPI_HandleTypeDef *hspi, uint8_t *pTxBuffer,
		uint32_t Len) {
	uint8_t state = hspi->TxState;

	if (state != SPI_STATE_BUSY_TX) {
		// save the Tx buffer address and len information in some global variables
		hspi->pTxBuffer = pTxBuffer;
		hspi->TxLen = Len;

		// mark the SPI state as busy in transmission so that
		// no other code can take over same SPI peripheral until transmisson is over
		hspi->TxState = SPI_STATE_BUSY_TX;

		// the blocking functions disable SPI at the end, enable it again
		SPI_PeripheralControl(hspi->pSPIx, ENABLE);

		// enable the TXEIE control bit to get interrupt whenever TXE is set in SR
		hspi->pSPIx->CR2 |= (1 << SPI_CR2_TXEIE);
	}
	return state;
}

/**
 * @brief  Receive an amount of data in non-blocking mode with Interrupt.
 * @note   In master mode the clock only runs while transmitting:
 *         call SPI_Receive_IT() first, then SPI_Transmit_IT() with the same length.
 * @param  hspi pointer to a SPI_HandleTypeDef structure that contains
 *               the configuration information for SPI module.
 * @param  pRxBuffer pointer to data buffer
 * @param  Len amount of data to be received
 * @retval State of the mode
 */
uint8_t SPI_Receive_IT(SPI_HandleTypeDef *hspi, uint8_t *pRxBuffer,
		uint32_t Len) {
	uint8_t state = hspi->RxState;

	if (state != SPI_STATE_BUSY_RX) {
		// save the Rx buffer address and len information in some global variables
		hspi->pRxBuffer = pRxBuffer;
		hspi->RxLen = Len;

		// mark the SPI state as busy in transmission so that
		// no other code can take over same SPI peripheral until receive is over
		hspi->RxState = SPI_STATE_BUSY_RX;

		// the blocking functions disable SPI at the end, enable it again
		SPI_PeripheralControl(hspi->pSPIx, ENABLE);
		SPI_ClearOVRFlag(hspi->pSPIx); // drop old received data

		// enable the RXNEIE control bit to get interrupt whenever RX is set in SR
		hspi->pSPIx->CR2 |= (1 << SPI_CR2_RXNEIE);
	}
	return state;
}

/**
 * @brief  Enables or disables the specified IRQ number.
 * @param  IRQNumber Specifies the IRQ number.
 * @param  state ENABLE or DISABLE the IRQ.
 * @retval None
 */
void SPI_IRQInterruptConfig(uint8_t IRQNumber, uint8_t state) {
	NVIC_IRQConfig(IRQNumber, state);
}

/**
 * @brief  Configures the priority of an IRQ.
 * @param  IRQNumber Specifies the IRQ number.
 * @param  IRQPriority Specifies the priority level (0-15, lower is higher priority).
 * @retval None
 */
void SPI_IRQPriorityConfig(uint8_t IRQNumber, uint32_t IRQPriority) {
	NVIC_SetPriority(IRQNumber, (uint8_t)IRQPriority);
}

/**
 * @brief  Handle SPI interrupt request.
 * @param  hspi pointer to a SPI_HandleTypeDef structure that contains
 *               the configuration information for the specified SPI module.
 * @retval None
 */
void SPI_IRQHandler(SPI_HandleTypeDef *hspi) {
	uint8_t tmp1, tmp2;

	// check for RXNE first: read the received byte before the next one arrives
	tmp1 = hspi->pSPIx->SR & (1 << SPI_SR_RXNE);
	tmp2 = hspi->pSPIx->CR2 & (1 << SPI_CR2_RXNEIE);
	if (tmp1 && tmp2) {
		// handle RXNE
		spi_rxne_interrupt_handler(hspi);
	}

	// check for TXE
	tmp1 = hspi->pSPIx->SR & (1 << SPI_SR_TXE);
	tmp2 = hspi->pSPIx->CR2 & (1 << SPI_CR2_TXEIE);
	if (tmp1 && tmp2) {
		// handle TXE
		spi_txe_interrupt_handler(hspi);
	}

	// check for ovr flag
	tmp1 = hspi->pSPIx->SR & (1 << SPI_SR_OVR);
	tmp2 = hspi->pSPIx->CR2 & (1 << SPI_CR2_ERRIE);
	if (tmp1 && tmp2) {
		// handle OVR error
		spi_ovr_error_interrupt_handler(hspi);
	}
}

// some help function implementations

/**
 * @brief  Handle SPI TXE (Transmit Buffer Empty) interrupt.
 * @param  hspi Pointer to SPI_HandleTypeDef structure.
 * @retval None
 */
static void spi_txe_interrupt_handler(SPI_HandleTypeDef *hspi) {
	// check DFF bit in CR1
	if (hspi->pSPIx->CR1 & (1 << SPI_CR1_DFF)) {
		// 16 bit
		hspi->pSPIx->DR = *((const uint16_t*) hspi->pTxBuffer);
		hspi->TxLen = (hspi->TxLen >= 2) ? (hspi->TxLen - 2) : 0;
		hspi->pTxBuffer += 2;
	} else {
		// 8 bit
		hspi->pSPIx->DR = *(hspi->pTxBuffer);
		hspi->TxLen--;
		hspi->pTxBuffer++;
	}
	if (!hspi->TxLen) {
		//Transmission is complete
		SPI_CloseTransmisson(hspi);

		// Call application event callback
		SPI_ApplicationEventCallback(hspi, SPI_EVENT_TX_COMPLETE);
	}
}

static void spi_rxne_interrupt_handler(SPI_HandleTypeDef *hspi) {
	// check DFF bit in CR1
	if (hspi->pSPIx->CR1 & (1 << SPI_CR1_DFF)) {
		// 16 bit
		*((uint16_t*) hspi->pRxBuffer) = (uint16_t)hspi->pSPIx->DR;
		hspi->RxLen = (hspi->RxLen >= 2) ? (hspi->RxLen - 2) : 0;
		hspi->pRxBuffer += 2;
	} else {
		// 8 bit
		*(hspi->pRxBuffer) = (uint8_t)hspi->pSPIx->DR;
		hspi->RxLen--;
		hspi->pRxBuffer++;
	}
	if (!hspi->RxLen) {
		//reception is complete
		SPI_CloseReception(hspi);

		// Call application event callback
		SPI_ApplicationEventCallback(hspi, SPI_EVENT_RX_COMPLETE);
	}
}

static void spi_ovr_error_interrupt_handler(SPI_HandleTypeDef *hspi) {
	// clear the ovr flag, otherwise the interrupt fires again and again
	SPI_ClearOVRFlag(hspi->pSPIx);

	// Call application event callback
	SPI_ApplicationEventCallback(hspi, SPI_EVENT_OVR_ERROR);
}

void SPI_CloseTransmisson(SPI_HandleTypeDef *hspi) {
	hspi->pSPIx->CR2 &= ~(1 << SPI_CR2_TXEIE);
	hspi->pTxBuffer = NULL;
	hspi->TxLen = 0;
	hspi->TxState = SPI_STATE_READY;

}

void SPI_CloseReception(SPI_HandleTypeDef *hspi) {
	hspi->pSPIx->CR2 &= ~(1 << SPI_CR2_RXNEIE);
	hspi->pRxBuffer = NULL;
	hspi->RxLen = 0;
	hspi->RxState = SPI_STATE_READY;

}

void SPI_ClearOVRFlag(SPI_RegDef_t *pSPIx) {
	uint8_t temp;
	temp = pSPIx->DR;
	temp = pSPIx->SR;
	(void) temp;
}

__weak void SPI_ApplicationEventCallback(SPI_HandleTypeDef *hspi, uint8_t appEvent) {
	//This is a weak implementation . the user application may override this function.
	(void) hspi;
	(void) appEvent;
}
