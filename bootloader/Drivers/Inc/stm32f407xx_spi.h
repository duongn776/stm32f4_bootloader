/*
 * stm32f407xx_spi.h
 *
 *  Created on: Feb 12, 2025
 *      Author: nhduong
 */

#ifndef INC_STM32F407XX_SPI_H_
#define INC_STM32F407XX_SPI_H_

#include "stm32f407xx.h"


/**
  * @brief  SPI Configuration Structure definition
  */
typedef struct
{
	uint8_t Mode; 					/*!< Specifies the SPI operating mode.
					   	   	   	 	 	 This parameter can be a value of @ref SPI_Mode						*/

	uint8_t Direction;				/*!< Specifies the SPI bidirectional mode state.
							 	 	 	This parameter can be a value of @ref SPI_Direction					*/

	uint8_t DataSize;				/*!< Specifies the SPI data size.
							 	 	 	 This parameter can be value of @ref SPI_Data_Size					*/

	uint8_t CLKPolarity;			/*!< Specifies the serial clock steady state.
								 	 	 This parameter can be a value of @ref SPI_Clock_Polarity			*/

	uint8_t CLKPhase;				/*!< Specifies the clock active edge for the bit capture.
								 	 	 This parameter can be a value of @ref SPI_Clock_Phase				*/

	uint8_t NSS;					/*!< Specifies whether the NSS signal is managed by
								 	 	 hardware (NSS pin) or software using the SSI bit.
								 	 	 This parameter can be a value of @ref SPI_Slave_Select_management	*/

	uint8_t BaudRatePrescaler;		/*!< Specifies the Baud Rate prescaler value which will be
								 	 	 used to configure the transmit and receive SCK clock.
								 	 	 This parameter can be a value of @ref SPI_BaudRate_Prescaler		*/

}SPI_InitTypeDef;

/**
  * @brief  SPI handle Structure definition
  */
typedef struct
{
	SPI_RegDef_t 		*pSPIx;			/*!< SPI registers base address			*/

	SPI_InitTypeDef		Init;			/*!< SPI communication parameters		*/

	const uint8_t				*pTxBuffer;		/*!< Pointer to SPI Tx transfer Buffer	*/

	uint8_t				*pRxBuffer;		/*!< Pointer to SPI Rx transfer Buffer	*/

	uint32_t			TxLen;			/*!< SPI Tx Length						*/

	uint32_t 			RxLen;			/*!< SPI Rx Length						*/

	uint8_t				TxState;		/*!< SPI Tx State						*/

	uint8_t 			RxState;		/*!< SPI Rx State						*/
}SPI_HandleTypeDef;

/** @defgroup SPI_Mode SPI Mode
  *
  */
#define SPI_MODE_SLAVE                  	0
#define SPI_MODE_MASTER                 	1

/** @defgroup SPI_Direction SPI Direction Mode
  *
  */
#define SPI_DIRECTION_FD            		0
#define SPI_DIRECTION_HD     				1
#define SPI_DIRECTION_SIMPLEX_RXONLY         	2

/** @defgroup SPI_BaudRate_Prescaler SPI BaudRate Prescaler
  *
  */
#define SPI_BAUDRATEPRESCALER_2         	0
#define SPI_BAUDRATEPRESCALER_4         	1
#define SPI_BAUDRATEPRESCALER_8         	2
#define SPI_BAUDRATEPRESCALER_16        	3
#define SPI_BAUDRATEPRESCALER_32        	4
#define SPI_BAUDRATEPRESCALER_64        	5
#define SPI_BAUDRATEPRESCALER_128       	6
#define SPI_BAUDRATEPRESCALER_256       	7

/** @defgroup SPI_Slave_Select_management SPI Slave Select Management
  *
  */
#define SPI_NSS_SOFT                    	1
#define SPI_NSS_HARD             			0

/** @defgroup SPI_Data_Size SPI Data Size
  *
  */
#define SPI_DATASIZE_8BIT               	0
#define SPI_DATASIZE_16BIT              	1

/** @defgroup SPI_Clock_Polarity SPI Clock Polarity
  *
  */
#define SPI_POLARITY_LOW                	0
#define SPI_POLARITY_HIGH               	1

/** @defgroup SPI_Clock_Phase SPI Clock Phase
  *
  */
#define SPI_PHASE_1stEDGE                 	0
#define SPI_PHASE_2ndEDGE                 	1

/** @defgroup SPI_State SPI State Definition
  *
  */
#define SPI_STATE_READY       				0
#define SPI_STATE_BUSY_TX 					1
#define SPI_STATE_BUSY_RX 					2


/** @defgroup SPI_Event SPI Event Definition
  *
  */
#define SPI_EVENT_TX_COMPLETE   			1
#define SPI_EVENT_RX_COMPLETE   			2
#define SPI_EVENT_OVR_ERROR     			3
#define SPI_EVENT_CRC_ERROR     			4


/** @defgroup SPI_Flags_definition SPI Flags Definition
  *
  */
#define SPI_FLAG_RXNE                   (1 << SPI_SR_RXNE)   /* SPI status flag: Rx buffer not empty flag       */
#define SPI_FLAG_TXE                    (1 << SPI_SR_TXE)    /* SPI status flag: Tx buffer empty flag           */
#define SPI_FLAG_BSY                    (1 << SPI_SR_BSY)    /* SPI status flag: Busy flag                      */
#define SPI_FLAG_CRCERR                 (1 << SPI_SR_CRCERR) /* SPI Error flag: CRC error flag                  */
#define SPI_FLAG_MODF                   (1 << SPI_SR_MODF)   /* SPI Error flag: Mode fault flag                 */
#define SPI_FLAG_OVR                    (1 << SPI_SR_OVR)    /* SPI Error flag: Overrun flag                    */
#define SPI_FLAG_FRE                    (1 << SPI_SR_FRE)    /* SPI Error flag: TI mode frame format error flag */

/** SPI busy timeout
 *
 */
#define SPI_MSBFIRST   			0
#define SPI_LSBFIRST			1


/*
 * Peripheral Clock setup
 */
void SPI_PeriClockControl(SPI_RegDef_t *pSPIx, uint8_t clockState);

/*
 * Init and De-init
 */
void SPI_Init(SPI_HandleTypeDef *hspi);
void SPI_DeInit(SPI_RegDef_t *pSPIx);
void SPI1_Init(SPI_HandleTypeDef *spi1);
void SPI2_Init(SPI_HandleTypeDef *spi2);
void SPI1_GPIOInits(void);
void SPI2_GPIOInits(void);
/*
 * Data Send and Receive
 */
void SPI_Transmit(SPI_RegDef_t *pSPIx, const uint8_t *pTxBuffer, uint32_t Len);
void SPI_Receive(SPI_RegDef_t *pSPIx, uint8_t *pRxBuffer, uint32_t Len);
void SPI_TransmitReceive(SPI_RegDef_t *pSPIx, const uint8_t *pTxBuffer, uint8_t *pRxBuffer, uint32_t Len);
uint8_t SPI_Transmit_IT(SPI_HandleTypeDef *hspi,uint8_t *pTxBuffer, uint32_t Len);
uint8_t SPI_Receive_IT(SPI_HandleTypeDef *hspi, uint8_t *pRxBuffer, uint32_t Len);

/*
 * IRQ Configuration and ISR handling
 */
void SPI_IRQInterruptConfig(uint8_t IRQNumber, uint8_t state);
void SPI_IRQPriorityConfig(uint8_t IRQNumber, uint32_t IRQPriority);
void SPI_IRQHandler(SPI_HandleTypeDef *hspi);

/*
 * Other Peripheral Control APIs
 */
void SPI_PeripheralControl(SPI_RegDef_t *pSPIx, uint8_t state);
void SPI_SSIConfig(SPI_RegDef_t *pSPIx, uint8_t state);
void SPI_SSOEConfig(SPI_RegDef_t *pSPIx, uint8_t state);
uint8_t SPI_GetFlagStatus(SPI_RegDef_t *pSPIx , uint32_t FlagName);
void SPI_ClearOVRFlag(SPI_RegDef_t *pSPIx);
void SPI_CloseTransmisson(SPI_HandleTypeDef *hspi);
void SPI_CloseReception(SPI_HandleTypeDef *hspi);

/*
 * Application callback
 */
void SPI_ApplicationEventCallback(SPI_HandleTypeDef *hspi, uint8_t appEvent);

#endif /* INC_STM32F407XX_SPI_H_ */
