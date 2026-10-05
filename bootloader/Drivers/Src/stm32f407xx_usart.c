/*
 * stm32f407xx_usart.c
 *
 *  Created on: Mar 1, 2025
 *      Author: nhduong
 */

#include "stm32f407xx_usart.h"


static void USART_EndTxTransfer(USART_HandleTypeDef *husart);
static void USART_Transmit_TXE(USART_HandleTypeDef *husart);
static void USART_Receive_RXNE(USART_HandleTypeDef *husart);



/**
  * @brief  Enables or disables the clock for the specified USART peripheral.
  * @param  pUSARTx Pointer to USART_RegDef_t structure representing USART1/2/3/6 or UART4/5.
  * @param  state ENABLE (1) to enable the clock, DISABLE (0) to disable it.
  * @retval None
  */
void USART_PeriClockControl(USART_RegDef_t *pUSARTx, uint8_t state)
{
    if(state == ENABLE)
    {
        if(pUSARTx == USART1)
        {
            USART1_CLK_ENABLE();
        }else if (pUSARTx == USART2)
        {
            USART2_CLK_ENABLE();
        }else if (pUSARTx == USART3)
        {
            USART3_CLK_ENABLE();
        }else if (pUSARTx == UART4)
        {
            UART4_CLK_ENABLE();
        }else if (pUSARTx == UART5)
        {
            UART5_CLK_ENABLE();
        }else if (pUSARTx == USART6)
        {
            USART6_CLK_ENABLE();
        }
    }
    else
    {
        if(pUSARTx == USART1)
        {
            USART1_CLK_DISABLE();
        }else if (pUSARTx == USART2)
        {
            USART2_CLK_DISABLE();
        }else if (pUSARTx == USART3)
        {
            USART3_CLK_DISABLE();
        }else if (pUSARTx == UART4)
        {
            UART4_CLK_DISABLE();
        }else if (pUSARTx == UART5)
        {
            UART5_CLK_DISABLE();
        }else if (pUSARTx == USART6)
        {
            USART6_CLK_DISABLE();
        }
    }
}

/**
  * @brief  Configures the baud rate for the specified USART peripheral.
  * @note   OVER8 must already be programmed in CR1 (RM0090 30.3.4).
  * @param  pUSARTx Pointer to the USART peripheral (USART1/2/3/6 or UART4/5).
  * @param  BaudRate The desired baud rate to set for the USART communication.
  * @retval None
  */

void USART_SetBaudRate(USART_RegDef_t *pUSARTx, uint32_t BaudRate)
{
    uint32_t PCLKx = 0;          // Variable to store the peripheral clock
    uint32_t usartdiv = 0;       // Variable to store USARTDIV value (x100)
    uint32_t M_part = 0, F_part = 0; // Variables to store Mantissa and Fraction parts
    uint32_t F_max = 0;          // Largest value the fraction field can hold
    uint32_t over8 = pUSARTx->CR1 & (1 << USART_CR1_OVER8);

    // Step 1: Get the peripheral clock (PCLKx)
    if (pUSARTx == USART1 || pUSARTx == USART6)
    {
        PCLKx = RCC_GetPCLK2_Value(); // For USART1 and USART6, use APB2 clock
    }
    else
    {
        PCLKx = RCC_GetPCLK1_Value(); // For other USARTs, use APB1 clock
    }

    // Step 2: USARTDIV = PCLKx / (8 * (2 - OVER8) * BaudRate), scaled by 100
    // 25 * PCLKx stays below 2^32 for PCLKx <= 84 MHz
    if (over8)
    {
        // OVER8 = 1 (over-sampling by 8)
        usartdiv = ((25 * PCLKx) / (2 * BaudRate));
        F_max = 0x07;
    }
    else
    {
        // OVER8 = 0 (over-sampling by 16)
        usartdiv = ((25 * PCLKx) / (4 * BaudRate));
        F_max = 0x0F;
    }

    // Step 3: Split USARTDIV into Mantissa (integer) and Fraction (x100) parts
    M_part = usartdiv / 100;
    F_part = usartdiv - (M_part * 100);

    // Step 4: Convert the fraction to 1/16 (or 1/8) units, rounding to nearest
    F_part = ((F_part * (F_max + 1)) + 50) / 100;

    // Step 5: Rounding may give 16 (or 8): carry it into the Mantissa
    // instead of masking it to 0, otherwise the baud rate is off by one Mantissa step
    if (F_part > F_max)
    {
        M_part++;
        F_part = 0;
    }

    // Step 6: Program USART_BRR. With OVER8 = 1, DIV_Fraction[3] stays cleared
    pUSARTx->BRR = (M_part << 4) | F_part;
}


/**
  * @brief  Initializes the USART peripheral according to the specified parameters
  *         in the USART_HandleTypeDef and initializes the associated handle.
  * @note   The USART is left disabled (UE = 0), call USART_PeripheralControl() to enable it.
  * @param  husart Pointer to an USART_HandleTypeDef structure that contains
  *         the configuration information for the specified USART peripheral.
  * @retval None
  */
void USART_Init(USART_HandleTypeDef *husart)
{
	uint32_t tempreg = 0;

	// Enable peripheral clock
	USART_PeriClockControl(husart->pUSARTx, ENABLE);

	// Reset the handle state
	husart->TxState = USART_STATE_READY;
	husart->RxState = USART_STATE_READY;
	husart->TxLen = 0;
	husart->RxLen = 0;

/******************************** Configuration of CR1******************************************/
	// CR1 is written while UE = 0: M and OVER8 must not be changed while the USART is enabled
	// Configure USART mode
	if (husart->Init.Mode == USART_MODE_RX)
	{
		// Enable Receive field
		tempreg |= (1 << USART_CR1_RE);
	}else if (husart->Init.Mode == USART_MODE_TX)
	{
		// Enable Transmit field
		tempreg |= (1 << USART_CR1_TE);
	}else {
		// Enable Receive and Transmit field
		tempreg |= (1 << USART_CR1_RE);
		tempreg |= (1 << USART_CR1_TE);
	}

	// Configure Word Length
	tempreg |= husart->Init.WordLength << USART_CR1_M;

	// Configure parity control bit
	if (husart->Init.ParityControl == USART_PARITY_EVEN)
	{
		// Enable the parity control (PS = 0: even)
		tempreg |= ( 1 << USART_CR1_PCE);
	}else if (husart->Init.ParityControl == USART_PARITY_ODD)
	{
		// Enable the parity control
		tempreg |= ( 1 << USART_CR1_PCE);

		// Enable ODD parity
		tempreg |= ( 1 << USART_CR1_PS);
	}

	// Configure Oversampling mode
	tempreg |= husart->Init.Oversampling << USART_CR1_OVER8;

	//Program the CR1 register
	husart->pUSARTx->CR1 = tempreg;

/******************************** Configuration of CR2******************************************/
	tempreg = 0;

	// Configure the number of stop bits inserted during USART frame transmission
	tempreg |= husart->Init.StopBits << USART_CR2_STOP;

	// Program the CR2 register
	husart->pUSARTx->CR2 = tempreg;

/******************************** Configuration of CR3******************************************/
	tempreg=0;

	//Configuration of USART hardware flow control
	if (husart->Init.HWFlowControl == USART_HW_CTS)
	{
		tempreg |= ( 1 << USART_CR3_CTSE);
	}else if (husart->Init.HWFlowControl == USART_HW_RTS)
	{
		tempreg |= ( 1 << USART_CR3_RTSE);
	}else if (husart->Init.HWFlowControl == USART_HW_CTS_RTS)
	{
		tempreg |= ( 1 << USART_CR3_CTSE);
		tempreg |= ( 1 << USART_CR3_RTSE);
	}
	husart->pUSARTx->CR3 = tempreg;

/******************************** Configuration of BRR(Baudrate register)******************************************/

	// Configure the baud rate (needs OVER8 already set in CR1)
	USART_SetBaudRate(husart->pUSARTx,husart->Init.BaudRate);
}


/**
  * @brief  De-initializes the USART peripheral registers to their default reset values.
  * @param  pUSARTx Pointer to the USART peripheral (USART1/2/3/6 or UART4/5).
  * @retval None
  */
void USART_DeInit(USART_RegDef_t *pUSARTx)
{
    if (pUSARTx == USART1) {
        USART1_REG_RESET();  // Reset USART1
    } else if (pUSARTx == USART2) {
        USART2_REG_RESET();  // Reset USART2
    } else if (pUSARTx == USART3) {
        USART3_REG_RESET();  // Reset USART3
    } else if (pUSARTx == UART4) {
        UART4_REG_RESET();   // Reset UART4
    } else if (pUSARTx == UART5) {
        UART5_REG_RESET();   // Reset UART5
    } else if (pUSARTx == USART6) {
        USART6_REG_RESET();  // Reset USART6
    }
}

/**
  * @brief  Enables or disables the USART peripheral (UE bit).
  * @param  pUSARTx Pointer to the USART peripheral (USART1/2/3/6 or UART4/5).
  * @param  state ENABLE (1) to enable the Peripheral, DISABLE (0) to disable it.
  * @retval None
  */
void USART_PeripheralControl(USART_RegDef_t *pUSARTx, uint8_t state)
{
    if (state == ENABLE)
    {
        pUSARTx->CR1 |= (1 << USART_CR1_UE);  // Set the UE bit to enable the USART
    }
    else
    {
        pUSARTx->CR1 &= ~(1 << USART_CR1_UE); // Clear the UE bit to disable the USART
    }
}

/**
  * @brief  Checks the status of a specific flag in the USART Status Register (SR).
  * @param  pUSARTx Pointer to the USART peripheral (USART1/2/3/6 or UART4/5).
  * @param  FlagName The flag to check, see @ref USART_Flags
  * @retval FLAG_SET(1) or FLAG_RESET(0).
  */
uint8_t USART_GetFlagStatus(USART_RegDef_t *pUSARTx, uint16_t FlagName)
{
	return (pUSARTx->SR & FlagName) ? FLAG_SET : FLAG_RESET;
}

/**
  * @brief  Transmits an amount of data in blocking mode.
  * @param  husart Pointer to a USART_HandleTypeDef structure that contains
  *                the configuration information for the specified USART.
  * @param  pTxBuffer Pointer to the data buffer containing data to be transmitted.
  * @param  Len The number of frames to send.
  * @retval None
  */
void USART_Transmit(USART_HandleTypeDef *husart, uint8_t *pTxBuffer, uint32_t Len)
{
    uint16_t *pData;

    // Loop over until "Len" number of frames are transferred
    for (uint32_t i = 0; i < Len; i++)
    {
        // Wait until TXE flag is set in the SR (Transmitter Empty)
        while (!(USART_GetFlagStatus(husart->pUSARTx, USART_FLAG_TXE)));


        // Check Word Length (9 bits or 8 bits)
        if (husart->Init.WordLength == USART_WORDLENGTH_9BITS)
        {
            // 9 BITS: Load DR with 2 bytes. The bits other than first 9 bits
            pData = (uint16_t*)pTxBuffer;
            husart->pUSARTx->DR = (*pData & (uint16_t)0x01FF);

            // Check for USART Parity control
            if (husart->Init.ParityControl == USART_PARITY_NONE)
            {
                // No parity is used in this transfer, so 9 bits of user data will be sent
                // Increment the buffer by 2 bytes
                pTxBuffer += 2;
            }
            else
            {
                // Parity bit is used in this transfer, so 8 bits of user data will be sent
                // The 9th bit will be replaced by the parity bit by the hardware
                pTxBuffer++;
            }
        }
        else
        {
            // 8 BITS data transfer
            husart->pUSARTx->DR = (*pTxBuffer & (uint8_t)0xFF);

            // Increment the buffer address
            pTxBuffer++;
        }
    }

    // Wait until TC flag is set in the SR (Transmission Complete)
    while (!USART_GetFlagStatus(husart->pUSARTx, USART_FLAG_TC));
}

/**
  * @brief  Receives an amount of data in blocking mode.
  * @note   Reading SR (polling RXNE) then DR also clears PE/FE/NE/ORE.
  * @param  husart Pointer to a USART_HandleTypeDef structure that contains
  *                the configuration information for the specified USART.
  * @param  pRxBuffer Pointer to the data buffer to store the received data.
  * @param  Len The number of frames to receive.
  * @retval None
  */
void  USART_Receive(USART_HandleTypeDef *husart, uint8_t *pRxBuffer, uint32_t Len)
{
    // Loop over until "Len" number of frames are transferred
    for (uint32_t i = 0; i < Len; i++)
    {
        // Wait until RXNE flag is set in the SR
        while (!USART_GetFlagStatus(husart->pUSARTx, USART_FLAG_RXNE));

        // Check Word Length (9 bits or 8 bits)
        if (husart->Init.WordLength == USART_WORDLENGTH_9BITS)
        {
            // We are going to receive 9-bit data in a frame

            // Check for USART Parity control
            if (husart->Init.ParityControl == USART_PARITY_NONE)
            {
                // No parity is used, so all 9 bits will be user data
                *((uint16_t*)pRxBuffer) = (husart->pUSARTx->DR & (uint16_t)0x01FF);
                pRxBuffer += 2; // Increment the pointer by 2 bytes
            }
            else
            {
                // Parity is used, so 8 bits will be user data and 1 bit is for parity
                *pRxBuffer = (husart->pUSARTx->DR & (uint8_t)0xFF);
                pRxBuffer++; // Increment the pointer by 1 byte
            }
        }
        else
        {
            // We are going to receive 8-bit data in a frame

            // Check for USART Parity control
            if (husart->Init.ParityControl == USART_PARITY_NONE)
            {
                // No parity is used, so all 8 bits will be user data
                *pRxBuffer = (husart->pUSARTx->DR & (uint8_t)0xFF);
            }
            else
            {
                // Parity is used, so 7 bits will be user data and 1 bit is parity
                *pRxBuffer = (uint8_t)(husart->pUSARTx->DR & (uint8_t)0x7F);
            }
            pRxBuffer++; // Increment the pointer by 1 byte
        }
    }
}

/**
  * @brief  Send an amount of data in non-blocking mode
  * @param  husart Pointer to a USART_HandleTypeDef structure that contains
  *                the configuration information for the specified USART.
  * @param  pTxBuffer Pointer to data buffer
  * @param  Len Number of frames to be sent
  * @retval State of the mode
  */
uint8_t USART_Transmit_IT(USART_HandleTypeDef *husart, uint8_t *pTxBuffer, uint32_t Len)
{
  uint8_t state = husart->TxState;

  // Check if USART is not busy with transmission
  if (state != USART_STATE_BUSY_TX)
  {
    // Store the pointer to the data buffer and set the length
    husart->pTxBuffer = pTxBuffer;
    husart->TxLen = Len;

    // Set the USART state to BUSY_TX (indicating the transmission is ongoing)
    husart->TxState = USART_STATE_BUSY_TX;

    // Enable only the TXE interrupt here. TC is 1 after reset / previous transfer,
    // so TCIE is enabled by USART_Transmit_TXE() once the last frame is written to DR
    husart->pUSARTx->CR1 |= (1 << USART_CR1_TXEIE);
  }

  return state;  // Return the previous state of the USART transmission
}


/**
  * @brief  Receive an amount of data in non-blocking mode.
  * @param  husart Pointer to a USART_HandleTypeDef structure that contains
  *                the configuration information for the specified USART.
  * @param  pRxBuffer Pointer to data buffer
  * @param  Len Number of frames to be received
  * @retval State of the mode
  */
uint8_t USART_Receive_IT(USART_HandleTypeDef *husart, uint8_t *pRxBuffer, uint32_t Len)
{
  uint8_t state = husart->RxState;

  // Check if USART is not currently busy receiving data
  if (state != USART_STATE_BUSY_RX)
  {
    // Store the pointer to the data buffer and set the length
    husart->pRxBuffer = pRxBuffer;
    husart->RxLen = Len;

    // Set the USART state to BUSY_RX (indicating reception is ongoing)
    husart->RxState = USART_STATE_BUSY_RX;

    // Enable the parity error interrupt when parity is used
    if (husart->Init.ParityControl != USART_PARITY_NONE)
    {
      husart->pUSARTx->CR1 |= (1 << USART_CR1_PEIE);
    }

    // Enable the RXNE interrupt (also raised on ORE) to handle data reception
    husart->pUSARTx->CR1 |= (1 << USART_CR1_RXNEIE);
  }

  return state;  // Return the previous state of the USART reception
}

/**
  * @brief  Clear a specific flag in the USART Status Register (SR).
  * @note   RM0090: CTS, LBD, TC, RXNE are rc_w0 and are cleared by writing 0.
  *         PE, FE, NE, ORE, IDLE are read-only and are cleared by a read of SR
  *         followed by a read of DR (the received data in DR is lost).
  * @param  pUSARTx Pointer to the USART peripheral (USART1/2/3/6 or UART4/5).
  * @param  FlagName The flag(s) to clear, see @ref USART_Flags
  * @retval None
  */
void USART_ClearFlag(USART_RegDef_t *pUSARTx, uint16_t FlagName)
{
	uint16_t rc_w0 = FlagName & (USART_FLAG_CTS | USART_FLAG_LBD | USART_FLAG_TC | USART_FLAG_RXNE);

	if (rc_w0)
	{
		// Write 0 only to the flags to clear, 1 to the others (no effect).
		// No read-modify-write: a flag set by hardware between the read and the write would be lost
		pUSARTx->SR = ~(uint32_t)rc_w0;
	}

	if (FlagName & ~rc_w0)
	{
		(void)pUSARTx->SR;
		(void)pUSARTx->DR;
	}
}

/**
  * @brief  End ongoing Tx transfer on USART peripheral (Transmit completion).
  * @param  husart USART handle.
  * @retval None
  */
static void USART_EndTxTransfer(USART_HandleTypeDef *husart)
{
	// check length
	if (! husart->TxLen)
	{
		// Disable the TC interrupt, TC stays set until the next transfer
		husart->pUSARTx->CR1 &= ~( 1 << USART_CR1_TCIE);

		//Reset the application state
		husart->TxState = USART_STATE_READY;

		//Reset Buffer address to NULL
		husart->pTxBuffer = NULL;

		//Call the application call back with event USART_EVENT_TX_CMPLT
		USART_ApplicationEventCallback(husart,USART_EVENT_TX_CMPLT);
	}
}

/**
  * @brief  Handles the TXE (Transmit Data Register Empty) interrupt for the USART peripheral.
  * @param  husart Pointer to the USART_HandleTypeDef structure
  * 			   that contains the configuration information for the specified USART.
  * @retval None
  */
static void USART_Transmit_TXE(USART_HandleTypeDef *husart)
{
	uint16_t *pdata;
  if(husart->TxState == USART_STATE_BUSY_TX)
  {
      //Keep sending data until Txlen reaches to zero
      if(husart->TxLen > 0)
      {
        //Check the USART_WordLength item for 9BIT or 8BIT in a frame
        if(husart->Init.WordLength == USART_WORDLENGTH_9BITS)
        {
          //if 9BIT load the DR with 2bytes masking  the bits other than first 9 bits
          pdata = (uint16_t*) husart->pTxBuffer;
          husart->pUSARTx->DR = (*pdata & (uint16_t)0x01FF);

          //check for USART_ParityControl
          if(husart->Init.ParityControl == USART_PARITY_NONE)
          {
            //No parity is used in this transfer , so 9bits of user data will be sent
            husart->pTxBuffer += 2;
          }
          else
          {
            //Parity bit is used in this transfer . so 8bits of user data will be sent
            //The 9th bit will be replaced by parity bit by the hardware
            husart->pTxBuffer++;
          }
        }
        else
        {
          //This is 8bit data transfer
          husart->pUSARTx->DR = (*husart->pTxBuffer  & (uint8_t)0xFF);
          husart->pTxBuffer++;
        }

        // One frame sent
        husart->TxLen -= 1;
      }
      if (husart->TxLen == 0 )
      {
        // Last frame is in DR: stop TXE interrupt and wait for TC (frame fully shifted out)
        husart->pUSARTx->CR1 &= ~( 1 << USART_CR1_TXEIE);
        husart->pUSARTx->CR1 |= ( 1 << USART_CR1_TCIE);
      }
  }
}

/**
  * @brief  Handles the RXNE (Receive Data Register Not Empty) interrupt for the USART peripheral.
  * @param  husart Pointer to the USART_HandleTypeDef structure
  * 			   that contains the configuration information for the specified USART.
  * @retval None
  */
static void USART_Receive_RXNE(USART_HandleTypeDef *husart)
{
  if(husart->RxState == USART_STATE_BUSY_RX)
  {
      if(husart->RxLen > 0)
      {
        //Check the USART_WordLength to decide whether we are going to receive 9bit of data in a frame or 8 bit
        if(husart->Init.WordLength == USART_WORDLENGTH_9BITS)
        {
          //We are going to receive 9bit data in a frame

          //Now, check are we using USART_ParityControl control or not
          if(husart->Init.ParityControl == USART_PARITY_NONE)
          {
            //No parity is used , so all 9bits will be of user data

            //read only first 9 bits so mask the DR with 0x01FF
            *((uint16_t*) husart->pRxBuffer) = (husart->pUSARTx->DR  & (uint16_t)0x01FF);

            //Now increment the pRxBuffer two times
            husart->pRxBuffer += 2;
          }
          else
          {
            //Parity is used, so 8bits will be of user data and 1 bit is parity
             *husart->pRxBuffer = (husart->pUSARTx->DR  & (uint8_t)0xFF);
             husart->pRxBuffer++;
          }
        }
        else
        {
          //We are going to receive 8bit data in a frame

          //Now, check are we using USART_ParityControl control or not
          if(husart->Init.ParityControl == USART_PARITY_NONE)
          {
            //No parity is used , so all 8bits will be of user data

            //read 8 bits from DR
             *husart->pRxBuffer = (uint8_t) (husart->pUSARTx->DR  & (uint8_t)0xFF);
          }

          else
          {
            //Parity is used, so , 7 bits will be of user data and 1 bit is parity

            //read only 7 bits , hence mask the DR with 0X7F
             *husart->pRxBuffer = (uint8_t) (husart->pUSARTx->DR  & (uint8_t)0x7F);

          }

          //Now , increment the pRxBuffer
          husart->pRxBuffer++;
        }

        // One frame received
        husart->RxLen -= 1;

      }//if of >0

      if(! husart->RxLen)
      {
        //disable the rxne and parity error interrupts
        husart->pUSARTx->CR1 &= ~(( 1 << USART_CR1_RXNEIE ) | ( 1 << USART_CR1_PEIE ));
        husart->RxState = USART_STATE_READY;
        husart->pRxBuffer = NULL;
        USART_ApplicationEventCallback(husart,USART_EVENT_RX_CMPLT);
      }
    }
}


/**
  * @brief  Handle USART event and error interrupt request.
  * @note   SR is read once at the start. The error flags (PE, FE, NE, ORE) and IDLE
  *         are then cleared by the following read of DR (RM0090 30.6.1).
  * @param  husart pointer to a USART_HandleTypeDef structure that contains
  *               the configuration information for the specified USART module.
  * @retval None
  */
void USART_IRQHandler(USART_HandleTypeDef *husart)
{
  uint32_t sr  = husart->pUSARTx->SR;
  uint32_t cr1 = husart->pUSARTx->CR1;
  uint32_t cr3 = husart->pUSARTx->CR3;
  uint8_t  rx_read = 0;   // set when DR has been read in this ISR

/*************************Check for error flags ********************************************/
// PE  -> interrupt when PEIE = 1
// ORE -> interrupt when RXNEIE = 1 (or EIE = 1 in DMA multibuffer mode)
// FE, NE -> interrupt only when EIE = 1 (DMA multibuffer mode), otherwise they come with RXNE

  if((sr & ( 1 << USART_SR_PE)) && (cr1 & ( 1 << USART_CR1_PEIE)))
  {
    USART_ApplicationEventCallback(husart,USART_EVENT_PE);
  }

  if((sr & ( 1 << USART_SR_ORE)) && ((cr1 & ( 1 << USART_CR1_RXNEIE)) || (cr3 & ( 1 << USART_CR3_EIE))))
  {
    USART_ApplicationEventCallback(husart,USART_ERR_ORE);
  }

  if(cr3 & ( 1 << USART_CR3_EIE))
  {
    if(sr & ( 1 << USART_SR_FE))
    {
      USART_ApplicationEventCallback(husart,USART_ERR_FE);
    }

    if(sr & ( 1 << USART_SR_NE))
    {
      USART_ApplicationEventCallback(husart,USART_ERR_NE);
    }
  }

/*************************Check for RXNE flag ********************************************/

  if((sr & ( 1 << USART_SR_RXNE)) && (cr1 & ( 1 << USART_CR1_RXNEIE)))
  {
    //this interrupt is because of rxne
    if(husart->RxState == USART_STATE_BUSY_RX)
    {
      // Reading DR here also clears PE/FE/NE/ORE (SR was read above)
      USART_Receive_RXNE(husart);
      rx_read = 1;
    }
  }

  // Error flags still pending and DR not read: finish the clear sequence (SR read above, then DR)
  // otherwise ORE keeps the interrupt pending forever
  if(!rx_read && (sr & (( 1 << USART_SR_PE) | ( 1 << USART_SR_FE) | ( 1 << USART_SR_NE) | ( 1 << USART_SR_ORE))))
  {
    (void)husart->pUSARTx->DR;
    rx_read = 1;
  }

/*************************Check for TC flag ********************************************/
// Checked before TXE: TCIE is only enabled after the last frame has been written to DR

  if((sr & ( 1 << USART_SR_TC)) && (cr1 & ( 1 << USART_CR1_TCIE)))
  {
    //this interrupt is because of TC
    //close transmission and call application callback if TxLen is zero
    if ( husart->TxState == USART_STATE_BUSY_TX)
    {
      USART_EndTxTransfer(husart);
    }
  }

/*************************Check for TXE flag ********************************************/

  if((sr & ( 1 << USART_SR_TXE)) && (cr1 & ( 1 << USART_CR1_TXEIE)))
  {
    //this interrupt is because of TXE
    USART_Transmit_TXE(husart);
  }

/*************************Check for CTS flag ********************************************/
//Note : CTS feature is not applicable for UART4 and UART5

  if((sr & ( 1 << USART_SR_CTS)) && (cr3 & ( 1 << USART_CR3_CTSIE)))
  {
    // CTS is rc_w0: clear it by writing 0 (no read-modify-write)
    husart->pUSARTx->SR = ~( 1U << USART_SR_CTS);

    //this interrupt is because of cts
    USART_ApplicationEventCallback(husart,USART_EVENT_CTS);
  }

/*************************Check for IDLE detection flag ********************************************/

  if((sr & ( 1 << USART_SR_IDLE)) && (cr1 & ( 1 << USART_CR1_IDLEIE)))
  {
    // IDLE is read-only: cleared by a read of SR (done above) followed by a read of DR
    if(!rx_read)
    {
      (void)husart->pUSARTx->DR;
    }

    //this interrupt is because of idle
    USART_ApplicationEventCallback(husart,USART_EVENT_IDLE);
  }
}

__weak void USART_ApplicationEventCallback(USART_HandleTypeDef *husart,uint8_t event)
{

}
