/**
 * @file    bootloader_config.h
 * @brief   Bootloader settings, UART messages and types used by main.c.
 *
 * The flash layout (addresses, sectors) is in memory_map.h.
 */

#ifndef BOOTLOADER_CONFIG_H_
#define BOOTLOADER_CONFIG_H_

#include <stdint.h>

/*******************************************************************************
 * Hardware
 ******************************************************************************/
#define BTN_PORT            GPIOA
#define BTN_PIN             GPIO_PIN_0      /* USER button (active high, pull-down on board) */

#define UART_IRQ_PRIORITY   5U

/*******************************************************************************
 * Flash write
 ******************************************************************************/
#define FLASH_WORD_SIZE     4U              /* Flash is programmed 4 bytes at a time (PSIZE x32) */
#define NO_WORD             0xFFFFFFFFU     /* wordAddr value when the word buffer is empty */

/*******************************************************************************
 * UART messages
 ******************************************************************************/
/* Startup */
#define MSG_UART_READY      "UART ready!!!\r\n"
#define MSG_ERASE           "Erasing DOWNLOAD area...\r\n"
#define MSG_ERASE_DONE      "Erase done.\r\n"
#define MSG_BOOT            "Entering Bootloader mode...\r\n"
#define MSG_APP             "Jumping to UserApp...\r\n"
#define MSG_NO_APP          "No valid UserApp found!\r\n"

/* Bootloader mode */
#define MSG_BL_MODE         "\r\n=== BOOTLOADER MODE ===\r\n"
#define MSG_SEND_SREC       "Send .SREC file via UART to update firmware.\r\n"
#define MSG_BL_READY        "READY\r\n"     /* Flash tool starts sending after this line */
#define MSG_DOWNLOAD_DONE   "\r\n=== DOWNLOAD DONE ===\r\n" \
                            "New firmware stored in DOWNLOAD area, App not changed.\r\n"

/* Answer to each SREC line (tools/flash_tool.py sends the next line only after ACK) */
#define MSG_ACK             "ACK\r\n"
#define MSG_NACK_PARSE      "NACK 1\r\n"    /* Bad line (checksum, hex, length) → tool resends it */
#define MSG_NACK_RANGE      "NACK 2\r\n"    /* Address outside App area         → tool stops      */
#define MSG_NACK_FLASH      "NACK 3\r\n"    /* Flash program error              → tool stops      */

/* Errors */
#define MSG_ERR_ERASE       "ERROR: erase failed!\r\n"
#define MSG_ERR_QUEUE       "ERROR: queue full, SREC line lost!\r\n"
#define MSG_ERR_UART        "ERROR: UART receive error, character lost!\r\n"

/*******************************************************************************
 * Types
 ******************************************************************************/
/* Function pointer type for UserApp entry point (Reset_Handler) */
typedef void (*AppEntry_t)(void);

/* Result of one SREC record, sent back to the flash tool as ACK / NACK <code> */
typedef enum
{
    REC_OK,             /* S0/S5 ignored or data written to flash  → ACK               */
    REC_END,            /* S7/S8/S9: last word written             → ACK, end of file  */
    REC_ERR_RANGE,      /* Data outside the App area, not written  → NACK 2            */
    REC_ERR_FLASH       /* Flash_ProgramWord() failed              → NACK 3            */
} RecResult_t;

#endif /* BOOTLOADER_CONFIG_H_ */
