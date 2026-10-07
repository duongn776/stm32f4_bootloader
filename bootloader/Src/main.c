/******************************************************************************
 * File:    main.c
 * Author:  Nguyen Hoang Duong
 * Date:    12/10/2025
 * Target:  STM32F407VG (STM32F407G-DISC1)
 * Tool:    STM32CubeIDE
 *
 * Description:
 *   UART bootloader for STM32F407.
 *
 *   Flow after reset:
 *     1. Init UART2 (PA2 TX, PA3 RX, 115200 8N1) and the USER button PA0
 *     2. Read button:
 *        • Pressed     → Bootloader mode (line "ROLLBACK" instead of a file:
 *                          copy BACKUP → APP, then run the old App):
 *                          erase DOWNLOAD area → receive .SREC over UART
 *                          → parse each line → program data into DOWNLOAD
 *                          → end of file: no error → install (backup APP → BACKUP,
 *                                                    then copy DOWNLOAD → APP)
 *                                         error    → App not changed
 *                          (App area is not touched during the download: a
 *                           failed or interrupted download never breaks the
 *                           running application)
 *        • Not pressed → jump to UserApp at APP_START_ADDR (0x08020000)
 *
 *   Data path in Bootloader mode:
 *     UART RX interrupt ──(1 char)──> line buffer ──(1 line)──> queue
 *     main loop: queue ──> SREC_Parse_Line() ──> Flash_WriteByte()
 *
 *   The file is sent as a whole from a terminal (Hercules): no answer per
 *   line, only errors are printed.
 *
 *   The SREC file is linked for the App area (0x0802xxxx). Each address is
 *   checked against the App area, then written at address + DL_OFFSET
 *   (0x0804xxxx) in the DOWNLOAD area.
 *
 *   Settings, messages and types: bootloader_config.h
 *   Flash layout:                 memory_map.h
 *
 *   Clock: HSI 16 MHz after reset (no PLL), enough for 115200 baud.
 ******************************************************************************/
#include <string.h>
#include "stm32f407xx.h"
#include "stm32f407xx_nvic.h"
#include "memory_map.h"
#include "bootloader_config.h"
#include "Srec_Parser.h"
#include "Circular_Queue.h"

/*******************************************************************************
 * Global Variables
 ******************************************************************************/
static USART_HandleTypeDef husart2;                 /* UART2 handle (also used by the IRQ handler) */
static uint8_t     uartRxChar;                      /* Last received character  */
static char        rxLine[QUEUE_MAX_LINE_LEN];      /* Line being received      */
static uint32_t    rxLineLen = 0U;                  /* Characters in rxLine     */

/* Complete SREC lines: pushed by UART interrupt, popped by main loop */
static SrecQueue_t srecQueue;
static volatile uint8_t queueOverflow = 0U;         /* Set by interrupt if a line was dropped    */
static volatile uint8_t uartError = 0U;             /* Set by interrupt on overrun/framing/noise */

/* 4-byte flash word being collected before programming */
static uint8_t     wordBuf[FLASH_WORD_SIZE];
static uint32_t    wordAddr = NO_WORD;              /* 4-byte aligned start address */
static uint32_t    flashErrors = 0U;                /* Failed Flash_ProgramWord() calls */

/*******************************************************************************
 * Function Prototypes
 ******************************************************************************/
static void        UART_SendString(const char *s);
static void        UART_Init(void);
static void        Button_Init(void);
static void        Flash_FlushWord(void);
static void        Flash_WriteByte(uint32_t addr, uint8_t value);
static RecResult_t Bootloader_HandleSrecRecord(const SREC_Record_t *rec);
static bool        App_IsValid(uint32_t slotAddr);
static bool        Copy_Slot(uint32_t srcAddr, uint32_t dstAddr, uint8_t dstSector);
static bool        Bootloader_Install(void);
static bool        Bootloader_InstallInterrupted(void);
static bool        Bootloader_Rollback(void);
static bool        Bootloader_RollbackInterrupted(void);
static void        Bootloader_RollbackCmd(void);
static void        Bootloader_Mode(void);
static void        JumpToUserApp(void);

/*******************************************************************************
 * Button
 ******************************************************************************/
/**
 * @brief Configure PA0 as input (USER button, external pull-down on the board)
 */
static void Button_Init(void)
{
    GPIO_ConfigPin(BTN_PORT, BTN_PIN, GPIO_MODE_INPUT, GPIO_NOPULL);
}

/*******************************************************************************
 * UART
 ******************************************************************************/
/**
 * @brief Send a null-terminated string over UART2 (blocking)
 */
static void UART_SendString(const char *s)
{
    USART_Transmit(&husart2, (uint8_t *)s, strlen(s));
}

/**
 * @brief Initialize UART2: PA2 TX / PA3 RX (AF7), 115200, 8 data bits, no parity, 1 stop bit
 */
static void UART_Init(void)
{
    GPIO_HandleTypeDef hgpio = {
        .pGPIOx = GPIOA,
        .Init = {
            .Pin       = GPIO_PIN_2,
            .Mode      = GPIO_MODE_AF,
            .Pull      = GPIO_NOPULL,
            .Speed     = GPIO_SPEED_FAST,
            .OPType    = GPIO_OPTYPE_PP,
            .Alternate = AF7,
        },
    };

    GPIO_Init(&hgpio);                      /* TX */

    hgpio.Init.Pin  = GPIO_PIN_3;           /* RX: pull-up = idle level of the line */
    hgpio.Init.Pull = GPIO_PULLUP;
    GPIO_Init(&hgpio);

    husart2.pUSARTx                = USART2;
    husart2.Init.Mode              = USART_MODE_TX_RX;
    husart2.Init.BaudRate          = USART_BAUDRATE_115200;
    husart2.Init.WordLength        = USART_WORDLENGTH_8BITS;
    husart2.Init.Oversampling      = USART_OVER8_DISABLE;
    husart2.Init.StopBits          = USART_STOPBITS_1;
    husart2.Init.ParityControl     = USART_PARITY_NONE;
    husart2.Init.HWFlowControl     = USART_HW_NONE;

    USART_Init(&husart2);
    USART_PeripheralControl(USART2, ENABLE);

    NVIC_SetPriority(IRQ_NO_USART2, UART_IRQ_PRIORITY);
    NVIC_IRQConfig(IRQ_NO_USART2, ENABLE);
}

/**
 * @brief UART2 interrupt callback, called by USART_IRQHandler()
 *
 * Collects characters into rxLine. When '\r' or '\n' is received, the
 * complete line is pushed into srecQueue for the main loop to process.
 */
void USART_ApplicationEventCallback(USART_HandleTypeDef *husart, uint8_t event)
{
    if (event == USART_EVENT_RX_CMPLT)
    {
        if ((uartRxChar == '\r') || (uartRxChar == '\n'))
        {
            /* End of line: push it (ignore empty lines, e.g. the '\n' of "\r\n") */
            if (rxLineLen > 0U)
            {
                rxLine[rxLineLen] = '\0';
                if (!Queue_Push(&srecQueue, rxLine))
                {
                    queueOverflow = 1U;     /* Queue full → line lost */
                }
                rxLineLen = 0U;
            }
        }
        else if (rxLineLen < (QUEUE_MAX_LINE_LEN - 1U))
        {
            /* Normal character: append to current line */
            rxLine[rxLineLen++] = (char)uartRxChar;
        }

        /* Start receiving the next character */
        USART_Receive_IT(husart, &uartRxChar, 1);
    }
    else if ((event == USART_ERR_ORE) || (event == USART_ERR_FE) || (event == USART_ERR_NE))
    {
        uartError = 1U;                     /* A character was lost or corrupted */
    }
}

/**
 * @brief UART2 interrupt vector (name from startup_stm32f407vgtx.s)
 */
void USART2_IRQHandler(void)
{
    USART_IRQHandler(&husart2);
}

/*******************************************************************************
 * Flash write (4-byte word buffer)
 *
 *   Flash_ProgramWord() writes 4 bytes at a 4-byte aligned address. SREC data
 *   can start at any address and have any length, so each byte is placed into
 *   wordBuf at position (addr & 3). When a byte belongs to a different word,
 *   the old word is programmed first. Unused bytes stay 0xFF (erased value).
 *
 *   Assumption: SREC data records come in increasing address order
 *   (a word must not be programmed twice).
 ******************************************************************************/
/**
 * @brief Program the collected word into flash (if any)
 */
static void Flash_FlushWord(void)
{
    uint32_t word;

    if (wordAddr == NO_WORD)
    {
        return;
    }

    /* Bytes in memory order → 32-bit value (STM32 is little-endian) */
    memcpy(&word, wordBuf, FLASH_WORD_SIZE);

    if (Flash_ProgramWord(wordAddr, word) != FLASH_OK)
    {
        flashErrors++;
    }

    wordAddr = NO_WORD;
}

/**
 * @brief Put one byte into the word buffer, programming the previous
 *        word when the address moves to a new word
 *
 * @param addr  Flash address of the byte
 * @param value Byte value
 */
static void Flash_WriteByte(uint32_t addr, uint8_t value)
{
    uint32_t base = addr & ~(FLASH_WORD_SIZE - 1U);     /* Round down to 4 */

    if (base != wordAddr)
    {
        Flash_FlushWord();                              /* Program old word     */
        memset(wordBuf, 0xFF, FLASH_WORD_SIZE);         /* Start with erased    */
        wordAddr = base;
    }

    wordBuf[addr & (FLASH_WORD_SIZE - 1U)] = value;
}

/*******************************************************************************
 * Bootloader
 ******************************************************************************/
/**
 * @brief Handle one parsed SREC record
 *
 *   S0       : header           → ignored
 *   S5       : record count     → ignored
 *   S1/S2/S3 : data             → App address checked, data written into
 *                                 the DOWNLOAD area (address + DL_OFFSET)
 *   S7/S8/S9 : end of file      → program the last word
 *
 * A flash error is found by comparing flashErrors before and after writing.
 * A word is programmed only when the next word starts, so an error in the
 * last bytes of a record is reported with the next record.
 *
 * @param rec Parsed SREC record
 * @return Result of the record (see RecResult_t)
 */
static RecResult_t Bootloader_HandleSrecRecord(const SREC_Record_t *rec)
{
    uint32_t i;
    uint32_t errorsBefore = flashErrors;

    switch (rec->record_type)
    {
        case SREC_TYPE_S1:
        case SREC_TYPE_S2:
        case SREC_TYPE_S3:
            /* The file must be built for the App area (0x0802xxxx) */
            if ((rec->address < APP_START_ADDR) ||
                ((rec->address + rec->data_length) > APP_END_ADDR))
            {
                return REC_ERR_RANGE;
            }

            /* Same data, one slot higher: App 0x0802xxxx → DOWNLOAD 0x0804xxxx */
            for (i = 0U; i < rec->data_length; i++)
            {
                Flash_WriteByte(rec->address + DL_OFFSET + i, rec->data[i]);
            }
            return (flashErrors != errorsBefore) ? REC_ERR_FLASH : REC_OK;

        case SREC_TYPE_S7:
        case SREC_TYPE_S8:
        case SREC_TYPE_S9:
            Flash_FlushWord();
            return (flashErrors != errorsBefore) ? REC_ERR_FLASH : REC_END;

        default:
            /* S0 header, S5/S6 record count: nothing to write */
            return REC_OK;
    }
}

/**
 * @brief Check the vector table of an App image stored in a slot
 *
 *   [slotAddr + 0] : initial stack pointer → must be inside RAM
 *   [slotAddr + 4] : Reset_Handler         → must be inside the App area
 *                                            with bit 0 = 1 (Thumb)
 *
 * Every image is linked for the App area, so Reset_Handler is checked
 * against the App area also for an image stored in DOWNLOAD or BACKUP.
 * Erased flash (0xFFFFFFFF) fails both checks.
 *
 * @param slotAddr Start address of the slot (APP, DOWNLOAD or BACKUP)
 * @return true if the vector table looks valid
 */
static bool App_IsValid(uint32_t slotAddr)
{
    uint32_t appStack = *(volatile uint32_t *)(slotAddr);
    uint32_t appReset = *(volatile uint32_t *)(slotAddr + 4U);

    return (appStack >= RAM_START_ADDR) && (appStack <= RAM_END_ADDR) &&
           (appReset >= APP_START_ADDR) && (appReset < APP_END_ADDR) &&
           ((appReset & 1U) != 0U);
}

/**
 * @brief Copy one slot into another one
 *
 *   1. Erase the destination sector
 *   2. Copy the whole slot word by word (erased words 0xFFFFFFFF are
 *      skipped: the destination is already 0xFF after the erase)
 *   3. Compare destination with source
 *
 * The source is only read. Flash must be unlocked.
 *
 * @param srcAddr   Start address of the source slot
 * @param dstAddr   Start address of the destination slot
 * @param dstSector Sector of the destination slot
 * @return true if the destination is now an exact copy of the source
 */
static bool Copy_Slot(uint32_t srcAddr, uint32_t dstAddr, uint8_t dstSector)
{
    const uint32_t *src = (const uint32_t *)srcAddr;
    uint32_t        i;

    if (Flash_EraseSector(dstSector) != FLASH_OK)
    {
        return false;
    }

    for (i = 0U; i < (SLOT_SIZE / FLASH_WORD_SIZE); i++)
    {
        if (src[i] == 0xFFFFFFFFU)
        {
            continue;
        }

        if (Flash_ProgramWord(dstAddr + (i * FLASH_WORD_SIZE), src[i]) != FLASH_OK)
        {
            return false;
        }
    }

    return (memcmp((const void *)dstAddr, (const void *)srcAddr, SLOT_SIZE) == 0);
}

/**
 * @brief Install the downloaded firmware, keeping the old App in BACKUP
 *
 *   1. Write the flag PENDING (BACKUP / APP are about to be erased),
 *      skipped when resuming an interrupted install (PENDING is already there)
 *   2. Backup: copy APP → BACKUP (skipped if APP is not valid, e.g. the
 *      first install), then write the flag BACKUP_DONE
 *   3. Copy DOWNLOAD → APP
 *   4. Write the flag DONE
 *
 * APP is erased only after BACKUP_DONE is written, so:
 *   - BACKUP_DONE missing → APP still holds the old App
 *   - BACKUP_DONE written → BACKUP holds the old App
 *
 * If the power is lost between 1 and 4, main() finds PENDING without DONE
 * at the next reset and calls this function again.
 *
 * Flash must be unlocked.
 *
 * @return true if APP is now an exact copy of DOWNLOAD
 */
static bool Bootloader_Install(void)
{
    /* New install: erase the flags of the previous one (a flash word can
       only be programmed once between two erases), then write PENDING.
       Resumed install: PENDING is already written and must stay, APP may
       be already broken, so the flags must never be erased here */
    if (!Bootloader_InstallInterrupted())
    {
        if ((Flash_EraseSector(META_A_SECTOR) != FLASH_OK) ||
            (Flash_ProgramWord(FLAG_PENDING_ADDR, INSTALL_PENDING) != FLASH_OK))
        {
            return false;
        }
    }

    /* Backup step, done only once per install: if BACKUP_DONE is already
       written, APP may be half copied and must never be copied over the
       old App stored in BACKUP */
    if (*(volatile uint32_t *)FLAG_BACKUP_ADDR != BACKUP_DONE)
    {
        if (App_IsValid(APP_START_ADDR))
        {
            UART_SendString(MSG_BACKUP);
            if (!Copy_Slot(APP_START_ADDR, BK_START_ADDR, BK_SECTOR))
            {
                UART_SendString(MSG_ERR_BACKUP);
                return false;
            }
        }
        else
        {
            UART_SendString(MSG_BACKUP_SKIP);   /* BACKUP keeps what it had */
        }

        /* Written also when skipped: from now on APP is erased, a resumed
           install must not try to back it up */
        if (Flash_ProgramWord(FLAG_BACKUP_ADDR, BACKUP_DONE) != FLASH_OK)
        {
            return false;
        }
    }

    UART_SendString(MSG_INSTALL);
    if (!Copy_Slot(DL_START_ADDR, APP_START_ADDR, APP_SECTOR))
    {
        return false;
    }

    return (Flash_ProgramWord(FLAG_DONE_ADDR, INSTALL_DONE) == FLASH_OK);
}

/**
 * @brief Check if the last install was interrupted (power lost)
 *
 * @return true if PENDING is written but DONE is not
 */
static bool Bootloader_InstallInterrupted(void)
{
    return (*(volatile uint32_t *)FLAG_PENDING_ADDR == INSTALL_PENDING) &&
           (*(volatile uint32_t *)FLAG_DONE_ADDR != INSTALL_DONE);
}

/**
 * @brief Rollback: copy the old App from BACKUP back into APP
 *
 *   1. Write the flag ROLLBACK_PENDING (APP is about to be erased),
 *      skipped when resuming an interrupted rollback
 *   2. Copy BACKUP → APP
 *   3. Write the flag ROLLBACK_DONE
 *
 * Same protection as the install: if the power is lost between 1 and 3,
 * main() finds ROLLBACK_PENDING without ROLLBACK_DONE at the next reset and
 * calls this function again. BACKUP is only read, so it is never lost.
 *
 * The caller checks that BACKUP holds a valid App. Flash must be unlocked.
 *
 * @return true if APP is now an exact copy of BACKUP
 */
static bool Bootloader_Rollback(void)
{
    /* New rollback: erase the flags of the last install / rollback (APP is
       still valid here), then write ROLLBACK_PENDING.
       Resumed rollback: APP is broken, the flags must never be erased */
    if (!Bootloader_RollbackInterrupted())
    {
        if ((Flash_EraseSector(META_A_SECTOR) != FLASH_OK) ||
            (Flash_ProgramWord(FLAG_RB_PENDING_ADDR, ROLLBACK_PENDING) != FLASH_OK))
        {
            return false;
        }
    }

    if (!Copy_Slot(BK_START_ADDR, APP_START_ADDR, APP_SECTOR))
    {
        return false;
    }

    return (Flash_ProgramWord(FLAG_RB_DONE_ADDR, ROLLBACK_DONE) == FLASH_OK);
}

/**
 * @brief Check if the last rollback was interrupted (power lost)
 *
 * @return true if ROLLBACK_PENDING is written but ROLLBACK_DONE is not
 */
static bool Bootloader_RollbackInterrupted(void)
{
    return (*(volatile uint32_t *)FLAG_RB_PENDING_ADDR == ROLLBACK_PENDING) &&
           (*(volatile uint32_t *)FLAG_RB_DONE_ADDR != ROLLBACK_DONE);
}

/**
 * @brief Command "ROLLBACK" received in Bootloader mode
 *
 *   No valid App in BACKUP → error, APP not touched, stay in Bootloader mode
 *   Otherwise              → copy BACKUP → APP, then run the old App
 *
 * Returns only if there is no backup.
 */
static void Bootloader_RollbackCmd(void)
{
    if (!App_IsValid(BK_START_ADDR))
    {
        UART_SendString(MSG_ERR_NO_BACKUP);
        return;
    }

    UART_SendString(MSG_ROLLBACK);

    /* Flash is unlocked in Bootloader mode, but locked again after an install */
    if ((Flash_Unlock() != FLASH_OK) || !Bootloader_Rollback())
    {
        UART_SendString(MSG_ERR_ROLLBACK);
        while (1)                           /* Do not jump into a broken APP */
        {
        }
    }
    Flash_Lock();
    UART_SendString(MSG_ROLLBACK_DONE);

    UART_SendString(MSG_APP);
    JumpToUserApp();

    /* Only reached when there is no valid UserApp */
    while (1)
    {
    }
}

/**
 * @brief Bootloader mode: receive SREC lines over UART and program them
 *
 * The UART interrupt fills srecQueue; this loop takes one line at a time,
 * parses it and writes its data into flash. Nothing is answered per line,
 * only errors are printed. A line with an error is lost (no resend), so
 * at the end of file the image is installed only if there was no error.
 * This function never returns.
 */
static void Bootloader_Mode(void)
{
    char          srecLine[QUEUE_MAX_LINE_LEN];
    SREC_Record_t record;
    uint32_t      errors = 0U;                      /* Errors during this download */

    UART_SendString(MSG_BL_MODE);
    UART_SendString(MSG_SEND_SREC);

    Queue_Init(&srecQueue);
    USART_Receive_IT(&husart2, &uartRxChar, 1);     /* Start receiving */

    while (1)
    {
        if (queueOverflow)
        {
            queueOverflow = 0U;
            errors++;
            UART_SendString(MSG_ERR_QUEUE);
        }

        if (uartError)
        {
            uartError = 0U;
            errors++;
            UART_SendString(MSG_ERR_UART);
        }

        if (!Queue_Pop(&srecQueue, srecLine))
        {
            continue;
        }

        if (strcmp(srecLine, CMD_ROLLBACK) == 0)
        {
            Bootloader_RollbackCmd();       /* Returns only if there is no backup */
            continue;
        }

        if (!SREC_Parse_Line(srecLine, &record))
        {
            errors++;
            UART_SendString(MSG_ERR_PARSE);
            continue;
        }

        switch (Bootloader_HandleSrecRecord(&record))
        {
            case REC_END:
                if (errors != 0U)
                {
                    UART_SendString(MSG_DOWNLOAD_FAIL);     /* APP not touched */
                }
                else
                {
                    UART_SendString(MSG_DOWNLOAD_DONE);
                    if (Bootloader_Install())
                    {
                        UART_SendString(MSG_INSTALL_DONE);
                    }
                    else
                    {
                        UART_SendString(MSG_ERR_INSTALL);
                    }
                }
                Flash_Lock();
                break;

            case REC_ERR_RANGE:
                errors++;
                UART_SendString(MSG_ERR_RANGE);
                break;

            case REC_ERR_FLASH:
                errors++;
                UART_SendString(MSG_ERR_FLASH);
                break;

            case REC_OK:
            default:
                break;
        }
    }
}

/**
 * @brief Jump to UserApp located at APP_START_ADDR
 *
 *   UserApp vector table:
 *     [APP_START_ADDR + 0] : initial Main Stack Pointer
 *     [APP_START_ADDR + 4] : Reset_Handler address (entry point)
 *
 * Returns only if no valid application is present.
 */
static void JumpToUserApp(void)
{
    uint32_t   appStack = *(volatile uint32_t *)(APP_START_ADDR);
    uint32_t   appReset = *(volatile uint32_t *)(APP_START_ADDR + 4U);
    AppEntry_t appEntry = (AppEntry_t)appReset;

    if (!App_IsValid(APP_START_ADDR))
    {
        UART_SendString(MSG_NO_APP);
        return;
    }

    /* Stop everything the bootloader started, so no interrupt of the
       bootloader fires inside the application */
    DISABLE_IRQ();
    NVIC_IRQConfig(IRQ_NO_USART2, DISABLE);
    USART_DeInit(USART2);

    SCB_VTOR = APP_START_ADDR;      /* Use UserApp's vector table  */
    SET_MSP(appStack);              /* Load UserApp's stack pointer */

    /* Interrupts enabled again as after a reset: the app (HAL) expects it */
    ENABLE_IRQ();
    appEntry();                     /* Run UserApp's Reset_Handler */
}

/*******************************************************************************
 * Main
 ******************************************************************************/
int main(void)
{
    UART_Init();
    Button_Init();

    UART_SendString(MSG_UART_READY);

    /* Power lost during the last install: APP is erased or half copied,
       DOWNLOAD still holds the complete new firmware → copy it again */
    if (Bootloader_InstallInterrupted())
    {
        UART_SendString(MSG_RESUME);
        if ((Flash_Unlock() != FLASH_OK) || !Bootloader_Install())
        {
            UART_SendString(MSG_ERR_INSTALL);
            while (1)                       /* Do not jump into a broken APP */
            {
            }
        }
        Flash_Lock();
        UART_SendString(MSG_RESUME_DONE);
    }

    /* Power lost during the last rollback: same idea, BACKUP is complete */
    if (Bootloader_RollbackInterrupted())
    {
        UART_SendString(MSG_ROLLBACK_RESUME);
        if ((Flash_Unlock() != FLASH_OK) || !Bootloader_Rollback())
        {
            UART_SendString(MSG_ERR_ROLLBACK);
            while (1)                       /* Do not jump into a broken APP */
            {
            }
        }
        Flash_Lock();
        UART_SendString(MSG_ROLLBACK_DONE);
    }

    if (GPIO_ReadPin(BTN_PORT, BTN_PIN) == GPIO_PIN_SET)   /* Button pressed (active high) */
    {
        UART_SendString(MSG_ERASE);
        if ((Flash_Unlock() != FLASH_OK) || (Flash_EraseSector(DL_SECTOR) != FLASH_OK))
        {
            UART_SendString(MSG_ERR_ERASE);
            while (1)
            {
            }
        }
        UART_SendString(MSG_ERASE_DONE);   /* Flash stays unlocked until the end of file */

        UART_SendString(MSG_BOOT);
        Bootloader_Mode();                  /* Never returns */
    }

    UART_SendString(MSG_APP);
    JumpToUserApp();

    /* Only reached when there is no valid UserApp */
    while (1)
    {
    }
}
