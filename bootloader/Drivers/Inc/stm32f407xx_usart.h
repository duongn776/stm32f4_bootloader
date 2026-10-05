/*
 * stm32f407xx_usart.h
 *
 *  Created on: Feb 12, 2025
 *      Author: nhduong
 */

#ifndef INC_STM32F407XX_USART_H_
#define INC_STM32F407XX_USART_H_

#include "stm32f407xx.h"

/**
  * @brief USART Init Structure definition
  */
typedef struct
{
    uint8_t     Mode;           /*!< Specifes whether the Receive or Transmit mode is enabled or disabled.
                                     This parameter can be a value of @ref USART_Mode                           */

    uint32_t    BaudRate;       /*!< This member configure the Usart communication baud rate.
                                     The parameter can be a value of @ref @USART_BaudRate                       */

    uint8_t     WordLength;     /*!< Specifies the number of data bits transmitted or received in a frame.
                                     The parameter can be a value of @ref USART_Word_Length                     */

    uint8_t     Oversampling;     /*!< Specifies the Oversampling mode.
                                       The parameter can be a value of @ref USART_Oversampling                 */

    uint8_t     StopBits;       /*!< Specifes the number of stop bits transmitted.
                                     The parameter can be a value of @ref USART_Stop_Bits                       */

    uint8_t     ParityControl;  /*!< Specifies the parity control mode.
                                     This parameter can be a value of @ref USART_Parity_Control                 */

    uint8_t     HWFlowControl;  /*!< Specifies the hardware flow control mode.
                                     This parameter can be a value of @ref USART_HW_FlowControl                 */

}USART_InitTypeDef;

/**
  * @brief USART Handle Structure definition
  */
typedef struct
{
    USART_RegDef_t          *pUSARTx;       /*!< Usart registers base address               */

    USART_InitTypeDef       Init;           /*!< Usart communication parameters             */

    uint8_t                 *pTxBuffer;     /*!< Pointer to Usart Tx transfer Buffer        */

    uint8_t                 *pRxBuffer;     /*!< Pointer to Usart Rx transfer Buffer        */

    uint32_t                TxLen;          /*!< Usart Tx Transfer length                   */

    uint32_t                RxLen;          /*!< Usart Rx Transfer length                   */

    uint8_t                 TxState;        /*!< Usart Tx Transfer state                    */

    uint8_t                 RxState;        /*!< Usart Rx Transfer state                    */

}USART_HandleTypeDef;

/** @defgroup USART_Mode USART Mode
  *
  */
#define USART_MODE_TX               0
#define USART_MODE_RX               1
#define USART_MODE_TX_RX            2

/** @defgroup USART_BaudRate USART BaudRate
  *
  */
#define USART_BAUDRATE_1200                 1200
#define USART_BAUDRATE_2400                 2400
#define USART_BAUDRATE_9600                 9600
#define USART_BAUDRATE_19200                19200
#define USART_BAUDRATE_38400                38400
#define USART_BAUDRATE_57600                57600
#define USART_BAUDRATE_115200               115200
#define USART_BAUDRATE_230400               230400
#define USART_BAUDRATE_460800               460800
#define USART_BAUDRATE_921600               921600
#define USART_BAUDRATE_2M                   2000000
#define USART_BAUDRATE_3M                   3000000

/** @defgroup USART_Parity USART Parity
  *
  */
#define USART_PARITY_NONE       0
#define USART_PARITY_EVEN       1
#define USART_PARITY_ODD        2

/** @defgroup USART_Word_Length USART Word Length
  *
  */
#define USART_WORDLENGTH_8BITS      0
#define USART_WORDLENGTH_9BITS      1

/** @defgroup USART_Oversampling USART Oversampling
  *
  */
#define USART_OVER8_DISABLE			0
#define USART_OVER8_ENABLE     		1


/** @defgroup USART_Stop_Bits USART Number of Stop Bits
  * @note  0.5 and 1.5 stop bits are not available on UART4/UART5
  */
#define USART_STOPBITS_1     0
#define USART_STOPBITS_0_5   1
#define USART_STOPBITS_2     2
#define USART_STOPBITS_1_5   3

/** @defgroup USART_HW_FlowControl USART Hardware Flow control
  * @note  CTS/RTS are not available on UART4/UART5
  */
#define USART_HW_NONE               0
#define USART_HW_CTS                1
#define USART_HW_RTS                2
#define USART_HW_CTS_RTS            3

/** @defgroup USART_Flags USART Flags
  *
  */
#define USART_FLAG_TXE               (1 << USART_SR_TXE)
#define USART_FLAG_TC                (1 << USART_SR_TC)
#define USART_FLAG_RXNE              (1 << USART_SR_RXNE)
#define USART_FLAG_IDLE              (1 << USART_SR_IDLE)
#define USART_FLAG_ORE               (1 << USART_SR_ORE)
#define USART_FLAG_NE                (1 << USART_SR_NE)
#define USART_FLAG_FE                (1 << USART_SR_FE)
#define USART_FLAG_PE                (1 << USART_SR_PE)
#define USART_FLAG_LBD               (1 << USART_SR_LBD)
#define USART_FLAG_CTS               (1 << USART_SR_CTS)

/** @defgroup USART_States USART States
  *
  */
#define USART_STATE_READY           0
#define USART_STATE_BUSY_TX         1
#define USART_STATE_BUSY_RX         2

/** @defgroup USART_Event_Error USART Event and Error
  *
  */
#define     USART_EVENT_TX_CMPLT    0
#define     USART_EVENT_RX_CMPLT    1
#define     USART_EVENT_IDLE        2
#define     USART_EVENT_CTS         3
#define     USART_EVENT_PE          4
#define     USART_ERR_FE            5
#define     USART_ERR_NE            6
#define     USART_ERR_ORE           7

/******************************************************************************************
 *                              APIs supported by this driver
 *       For more information about the APIs check the function definitions
 ******************************************************************************************/

/*
 * Peripheral Clock setup
 */
void USART_PeriClockControl(USART_RegDef_t *pUSARTx, uint8_t state);

/*
 * Init and De-init
 */
void USART_Init(USART_HandleTypeDef *husart);
void USART_DeInit(USART_RegDef_t *pUSARTx);

/*
 * Data Send and Receive
 * Len is the number of frames. With 9-bit word length and no parity each frame
 * takes 2 bytes (uint16_t) in the buffer, otherwise 1 byte.
 */
void USART_Transmit(USART_HandleTypeDef *husart, uint8_t *pTxBuffer, uint32_t Len);
void  USART_Receive(USART_HandleTypeDef *husart,uint8_t *pRxBuffer, uint32_t Len);
uint8_t USART_Transmit_IT(USART_HandleTypeDef *husart,uint8_t *pTxBuffer, uint32_t Len);
uint8_t USART_Receive_IT(USART_HandleTypeDef *husart,uint8_t *pRxBuffer, uint32_t Len);

/*
 * ISR handling
 * Use NVIC_IRQConfig()/NVIC_SetPriority() from stm32f407xx_nvic.h to configure the IRQ
 */
void USART_IRQHandler(USART_HandleTypeDef *husart);

/*
 * Other Peripheral Control APIs
 */

uint8_t USART_GetFlagStatus(USART_RegDef_t *pUSARTx, uint16_t FlagName);
void USART_ClearFlag(USART_RegDef_t *pUSARTx, uint16_t FlagName);
void USART_PeripheralControl(USART_RegDef_t *pUSARTx, uint8_t state);
void USART_SetBaudRate(USART_RegDef_t *pUSARTx, uint32_t BaudRate);


/*
 * Application Callbacks
 */
void USART_ApplicationEventCallback(USART_HandleTypeDef *husart,uint8_t event);


#endif /* INC_STM32F407XX_USART_H_ */
