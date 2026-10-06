/**
 * @file    memory_map.h
 * @brief   Flash layout shared by the bootloader and the application.
 *
 * The chip reports 512 KB of Flash (FLASH size register at 0x1FFF7A22),
 * only sectors 0..7 are used:
 *
 *   Sector  Address      Size    Region
 *   0       0x0800 0000  16 KB   Bootloader
 *   1       0x0800 4000  16 KB   Bootloader
 *   2       0x0800 8000  16 KB   Metadata A (FEE, ping-pong)
 *   3       0x0800 C000  16 KB   Metadata B (FEE, ping-pong)
 *   4       0x0801 0000  64 KB   Reserved
 *   5       0x0802 0000  128 KB  APP      : the application runs here
 *   6       0x0804 0000  128 KB  DOWNLOAD : new firmware is downloaded here
 *   7       0x0806 0000  128 KB  BACKUP   : previous firmware, used for rollback
 *
 * Keep in sync with the linker scripts:
 *   bootloader/STM32F407VGTX_FLASH.ld       : FLASH ORIGIN = BL_START_ADDR,  LENGTH = 32K
 *   User_Application/STM32F407VGTX_FLASH.ld : FLASH ORIGIN = APP_START_ADDR, LENGTH = 128K
 *   User_Application system_stm32f4xx.c     : VECT_TAB_OFFSET = APP_START_ADDR - 0x08000000
 */

#ifndef MEMORY_MAP_H_
#define MEMORY_MAP_H_

/* Bootloader: sectors 0..1 */
#define BL_START_ADDR           0x08000000U
#define BL_SIZE                 (32U * 1024U)

/* Metadata (FEE): sectors 2..3, one sector each */
#define META_A_ADDR             0x08008000U
#define META_A_SECTOR           2U
#define META_B_ADDR             0x0800C000U
#define META_B_SECTOR           3U
#define META_SECTOR_SIZE        (16U * 1024U)

/* Reserved: sector 4 */
#define RESERVED_ADDR           0x08010000U
#define RESERVED_SECTOR         4U
#define RESERVED_SIZE           (64U * 1024U)

/* Slots: one 128 KB sector each */
#define SLOT_SIZE               (128U * 1024U)

#define APP_START_ADDR          0x08020000U     /* Application runs here, linked at this address */
#define APP_SECTOR              5U

#define DL_START_ADDR           0x08040000U     /* Download slot */
#define DL_SECTOR               6U

#define BK_START_ADDR           0x08060000U     /* Backup slot */
#define BK_SECTOR               7U

#define APP_END_ADDR            (APP_START_ADDR + SLOT_SIZE)    /* 0x08040000, first address after the app */

/* RAM: valid range for the application initial stack pointer */
#define RAM_START_ADDR          0x20000000U
#define RAM_END_ADDR            0x20020000U     /* 128 KB */

#endif /* MEMORY_MAP_H_ */
