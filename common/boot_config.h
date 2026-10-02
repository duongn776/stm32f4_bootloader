/**
 * @file    boot_config.h
 * @brief   Single source of truth for the STM32F407VG flash/RAM layout.
 *
 * Shared by bootloader, application, tools and PC unit tests.
 * Must NOT include any HAL/CMSIS header.
 *
 * Reference: RM0090 Rev 21, Table 5 (flash module organization, STM32F40x/41x)
 * and PROJECT_BRIEF.md section 4. Do not change without updating the brief.
 */
#ifndef BOOT_CONFIG_H
#define BOOT_CONFIG_H

/* ------------------------------------------------------------------------- */
/* Flash geometry (single bank, 12 sectors)                                  */
/* ------------------------------------------------------------------------- */
#define BOOT_FLASH_BASE             0x08000000u
#define BOOT_FLASH_SIZE             0x00100000u     /* 1 MB */
#define BOOT_FLASH_END              (BOOT_FLASH_BASE + BOOT_FLASH_SIZE)  /* exclusive */
#define BOOT_FLASH_NUM_SECTORS      12u

/* ------------------------------------------------------------------------- */
/* Bootloader: sectors 0-1                                                   */
/* ------------------------------------------------------------------------- */
#define BOOT_BL_ADDR                0x08000000u
#define BOOT_BL_SIZE                0x00008000u     /* 32 KB */
#define BOOT_BL_FIRST_SECTOR        0u
#define BOOT_BL_NUM_SECTORS         2u

/* ------------------------------------------------------------------------- */
/* Metadata: sector 2 (A) and sector 3 (B), append-only log                  */
/* ------------------------------------------------------------------------- */
#define BOOT_META_A_ADDR            0x08008000u
#define BOOT_META_A_SECTOR          2u
#define BOOT_META_B_ADDR            0x0800C000u
#define BOOT_META_B_SECTOR          3u
#define BOOT_META_SECTOR_SIZE       0x00004000u     /* 16 KB */
#define BOOT_META_RECORD_SIZE       32u
#define BOOT_META_RECORDS_PER_SECTOR (BOOT_META_SECTOR_SIZE / BOOT_META_RECORD_SIZE) /* 512 */

/* ------------------------------------------------------------------------- */
/* Reserved: sector 4 (64 KB), unused                                        */
/* ------------------------------------------------------------------------- */
#define BOOT_RESERVED_ADDR          0x08010000u
#define BOOT_RESERVED_SECTOR        4u
#define BOOT_RESERVED_SIZE          0x00010000u

/* ------------------------------------------------------------------------- */
/* Slots: 3 x 128 KB sectors each                                            */
/* ------------------------------------------------------------------------- */
#define BOOT_SLOT_SECTOR_SIZE       0x00020000u     /* 128 KB */
#define BOOT_SLOT_NUM_SECTORS       3u
#define BOOT_SLOT_SIZE              (BOOT_SLOT_SECTOR_SIZE * BOOT_SLOT_NUM_SECTORS) /* 0x60000 */

#define BOOT_SLOT_A_ADDR            0x08020000u     /* running slot */
#define BOOT_SLOT_A_FIRST_SECTOR    5u              /* sectors 5..7 */
#define BOOT_SLOT_B_ADDR            0x08080000u     /* download slot */
#define BOOT_SLOT_B_FIRST_SECTOR    8u              /* sectors 8..10 */
#define BOOT_SCRATCH_ADDR           0x080E0000u
#define BOOT_SCRATCH_SECTOR         11u
#define BOOT_SCRATCH_SIZE           BOOT_SLOT_SECTOR_SIZE

/* ------------------------------------------------------------------------- */
/* Image layout inside a slot: 512 B header, then the app vector table       */
/* ------------------------------------------------------------------------- */
#define BOOT_IMG_HDR_SIZE           0x00000200u     /* also satisfies VTOR 512 B alignment */
#define BOOT_IMG_MAX_SIZE           (BOOT_SLOT_SIZE - BOOT_IMG_HDR_SIZE) /* 0x5FE00 = 392704 */

/* App is always linked to run from slot A */
#define BOOT_APP_VECTOR_ADDR        (BOOT_SLOT_A_ADDR + BOOT_IMG_HDR_SIZE) /* 0x08020200 */
#define BOOT_APP_VECTOR_OFFSET      (BOOT_APP_VECTOR_ADDR - BOOT_FLASH_BASE) /* 0x00020200 */
#define BOOT_APP_VECTOR_TABLE_SIZE  0x188u          /* 16 + 82 vectors on F407 */

/* ------------------------------------------------------------------------- */
/* RAM ranges (used to sanity-check the app initial MSP)                     */
/* Upper bounds are inclusive for MSP: Cube linker sets _estack = end.       */
/* ------------------------------------------------------------------------- */
#define BOOT_SRAM_BASE              0x20000000u     /* SRAM1 112 KB + SRAM2 16 KB */
#define BOOT_SRAM_END               0x20020000u
#define BOOT_CCM_BASE               0x10000000u     /* CPU only, no DMA */
#define BOOT_CCM_END                0x10010000u

/* ------------------------------------------------------------------------- */
/* System memory / device signature                                          */
/* ------------------------------------------------------------------------- */
#define BOOT_OTP_ADDR               0x1FFF7800u     /* never written by bootloader */
#define BOOT_UID_ADDR               0x1FFF7A10u     /* 96-bit unique ID */
#define BOOT_FLASH_SIZE_REG_ADDR    0x1FFF7A22u     /* u16, size in KB */
#define BOOT_OPTBYTES_ADDR          0x1FFFC000u     /* RDP = bits 15:8 */
#define BOOT_OPTBYTES_WRP_ADDR      0x1FFFC008u     /* nWRP = bits 11:0 */
#define BOOT_DBGMCU_IDCODE_ADDR     0xE0042000u
#define BOOT_DEV_ID_F407            0x413u

/* ------------------------------------------------------------------------- */
/* Compile-time layout checks                                                */
/* ------------------------------------------------------------------------- */
#if (BOOT_BL_ADDR + BOOT_BL_SIZE) != BOOT_META_A_ADDR
#error "Bootloader must end where metadata A starts"
#endif
#if (BOOT_META_A_ADDR + BOOT_META_SECTOR_SIZE) != BOOT_META_B_ADDR
#error "Metadata A/B must be contiguous"
#endif
#if (BOOT_META_B_ADDR + BOOT_META_SECTOR_SIZE) != BOOT_RESERVED_ADDR
#error "Metadata B must end where the reserved sector starts"
#endif
#if (BOOT_RESERVED_ADDR + BOOT_RESERVED_SIZE) != BOOT_SLOT_A_ADDR
#error "Reserved sector must end where slot A starts"
#endif
#if (BOOT_SLOT_A_ADDR + BOOT_SLOT_SIZE) != BOOT_SLOT_B_ADDR
#error "Slot A must end where slot B starts"
#endif
#if (BOOT_SLOT_B_ADDR + BOOT_SLOT_SIZE) != BOOT_SCRATCH_ADDR
#error "Slot B must end where scratch starts"
#endif
#if (BOOT_SCRATCH_ADDR + BOOT_SCRATCH_SIZE) != BOOT_FLASH_END
#error "Scratch must end at the end of flash"
#endif
#if (BOOT_APP_VECTOR_ADDR & 0x1FFu) != 0u
#error "App vector table must be 512-byte aligned (VTOR)"
#endif

#endif /* BOOT_CONFIG_H */
