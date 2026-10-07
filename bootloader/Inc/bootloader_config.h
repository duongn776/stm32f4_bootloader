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
 * Commands (one text line in Bootloader mode, instead of an SREC line)
 ******************************************************************************/
#define CMD_ROLLBACK        "ROLLBACK"      /* Copy BACKUP → APP, then run the old App */

/*******************************************************************************
 * Flash write
 ******************************************************************************/
#define FLASH_WORD_SIZE     4U              /* Flash is programmed 4 bytes at a time (PSIZE x32) */
#define NO_WORD             0xFFFFFFFFU     /* wordAddr value when the word buffer is empty */

/*******************************************************************************
 * Install flag (Metadata A sector, see memory_map.h)
 *
 *   word 0 = INSTALL_PENDING : written before BACKUP / APP is erased
 *   word 2 = BACKUP_DONE     : backup step finished (APP copied to BACKUP,
 *                              or skipped because APP was not valid)
 *   word 1 = INSTALL_DONE    : written after APP is copied and checked
 *
 *   PENDING without DONE at reset → power was lost during the install
 *   → install again (DOWNLOAD is not changed by the install):
 *       BACKUP_DONE missing → APP not erased yet, backup again
 *       BACKUP_DONE written → BACKUP holds the old App, APP may be broken,
 *                             skip the backup (never copy a broken APP
 *                             over the old App), copy DOWNLOAD → APP again
 ******************************************************************************/
#define FLAG_PENDING_ADDR   (META_A_ADDR + 0U)
#define FLAG_DONE_ADDR      (META_A_ADDR + 4U)
#define FLAG_BACKUP_ADDR    (META_A_ADDR + 8U)
#define INSTALL_PENDING     0x50454E44U     /* "PEND" */
#define INSTALL_DONE        0x444F4E45U     /* "DONE" */
#define BACKUP_DONE         0x424B444EU     /* "BKDN" */

/*******************************************************************************
 * Rollback flag (same sector, same idea as the install flag)
 *
 *   word 3 = ROLLBACK_PENDING : written before APP is erased
 *   word 4 = ROLLBACK_DONE    : written after BACKUP is copied to APP
 *
 *   ROLLBACK_PENDING without ROLLBACK_DONE at reset → copy BACKUP → APP again
 ******************************************************************************/
#define FLAG_RB_PENDING_ADDR    (META_A_ADDR + 12U)
#define FLAG_RB_DONE_ADDR       (META_A_ADDR + 16U)
#define ROLLBACK_PENDING        0x52425044U     /* "RBPD" */
#define ROLLBACK_DONE           0x5242444EU     /* "RBDN" */

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
#define MSG_RESUME          "Install was interrupted (power lost?), copy DOWNLOAD -> APP again...\r\n"
#define MSG_RESUME_DONE     "Install done.\r\n"
#define MSG_ROLLBACK        "Rollback: copy BACKUP -> APP...\r\n"
#define MSG_ROLLBACK_RESUME "Rollback was interrupted (power lost?), copy BACKUP -> APP again...\r\n"
#define MSG_ROLLBACK_DONE   "\r\n=== ROLLBACK DONE ===\r\n"

/* Bootloader mode */
#define MSG_BL_MODE         "\r\n=== BOOTLOADER MODE ===\r\n"
#define MSG_SEND_SREC       "Send .SREC file via UART to update firmware,\r\n" \
                            "or send \"" CMD_ROLLBACK "\" to go back to the previous firmware.\r\n"
#define MSG_DOWNLOAD_DONE   "\r\n=== DOWNLOAD DONE ===\r\n" \
                            "New firmware stored in DOWNLOAD area, App not changed.\r\n"
#define MSG_BACKUP          "Backup: copy APP -> BACKUP...\r\n"
#define MSG_BACKUP_SKIP     "Backup skipped: no valid App to save.\r\n"
#define MSG_INSTALL         "Installing: copy DOWNLOAD -> APP...\r\n"
#define MSG_INSTALL_DONE    "\r\n=== INSTALL DONE ===\r\n" \
                            "Release the boot button and reset the board to run the new UserApp.\r\n"

#define MSG_DOWNLOAD_FAIL   "\r\n=== DOWNLOAD FAILED ===\r\n" \
                            "Errors during download, App not changed. Reset and send the file again.\r\n"

/* Errors */
#define MSG_ERR_ERASE       "ERROR: erase failed!\r\n"
#define MSG_ERR_PARSE       "ERROR: bad SREC line (checksum, hex, length)!\r\n"
#define MSG_ERR_RANGE       "ERROR: address outside App area!\r\n"
#define MSG_ERR_FLASH       "ERROR: flash program failed!\r\n"
#define MSG_ERR_BACKUP      "ERROR: backup failed, APP not changed!\r\n"
#define MSG_ERR_INSTALL     "ERROR: install failed, APP is not valid!\r\n"
#define MSG_ERR_NO_BACKUP   "ERROR: no valid App in BACKUP, rollback not done!\r\n"
#define MSG_ERR_ROLLBACK    "ERROR: rollback failed, APP is not valid!\r\n"
#define MSG_ERR_QUEUE       "ERROR: queue full, SREC line lost!\r\n"
#define MSG_ERR_UART        "ERROR: UART receive error, character lost!\r\n"

/*******************************************************************************
 * Types
 ******************************************************************************/
/* Function pointer type for UserApp entry point (Reset_Handler) */
typedef void (*AppEntry_t)(void);

/* Result of one SREC record */
typedef enum
{
    REC_OK,             /* S0/S5 ignored or data written to flash */
    REC_END,            /* S7/S8/S9: last word written, end of file */
    REC_ERR_RANGE,      /* Data outside the App area, not written */
    REC_ERR_FLASH       /* Flash_ProgramWord() failed */
} RecResult_t;

#endif /* BOOTLOADER_CONFIG_H_ */
