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
 *   2       0x0800 8000  16 KB   App info (reserved: valid flag, size, CRC, version)
 *   3       0x0800 C000  16 KB   Reserved
 *   4       0x0801 0000  64 KB   Application
 *   5       0x0802 0000  128 KB  Application
 *   6       0x0804 0000  128 KB  Application
 *   7       0x0806 0000  128 KB  Application
 *
 * Keep in sync with the linker scripts:
 *   bootloader/STM32F407VGTX_FLASH.ld       : FLASH ORIGIN = BL_START_ADDR,  LENGTH = 32K
 *   User_Application/STM32F407VGTX_FLASH.ld : FLASH ORIGIN = APP_START_ADDR, LENGTH = 448K
 */

#ifndef MEMORY_MAP_H_
#define MEMORY_MAP_H_

/* Bootloader: sectors 0..1 */
#define BL_START_ADDR           0x08000000U
#define BL_SIZE                 (32U * 1024U)
#define BL_FIRST_SECTOR         0U
#define BL_LAST_SECTOR          1U

/* App info: sector 2 */
#define APP_INFO_ADDR           0x08008000U
#define APP_INFO_SIZE           (16U * 1024U)
#define APP_INFO_SECTOR         2U

/* Reserved: sector 3 */
#define RESERVED_ADDR           0x0800C000U
#define RESERVED_SIZE           (16U * 1024U)
#define RESERVED_SECTOR         3U

/* Application: sectors 4..7 */
#define APP_START_ADDR          0x08010000U
#define APP_SIZE                (448U * 1024U)
#define APP_END_ADDR            (APP_START_ADDR + APP_SIZE)     /* 0x08080000, first address after the app */
#define APP_FIRST_SECTOR        4U
#define APP_LAST_SECTOR         7U

#endif /* MEMORY_MAP_H_ */
