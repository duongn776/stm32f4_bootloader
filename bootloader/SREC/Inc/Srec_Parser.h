/*******************************************************************************
 * @file    Srec_Parser.h
 * @brief   Header file for Motorola S-Record parser module.
 * @details This module provides data structures and function declarations
 *          for parsing SREC formatted records used in Bootloader applications.
 *          It does not use any hardware, so it can also be built and tested on a PC.
 *
 * @date    Oct 19, 2025
 * @author  nhduong
 ******************************************************************************/

#ifndef INC_SREC_PARSER_H_
#define INC_SREC_PARSER_H_

/*******************************************************************************
 * Includes
 ******************************************************************************/
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

/*******************************************************************************
 * Definitions
 ******************************************************************************/
#define SREC_MAX_DATA_LEN    252U  /*!< Max data bytes in one record: 255 - 2 (S1 address) - 1 (checksum) */

/*******************************************************************************
 * Type Definitions
 ******************************************************************************/

/**
 * @enum SREC_RecordType_t
 * @brief SREC record type identifiers as defined by Motorola S-Record format.
 *        The value is the digit after 'S' (S4 is reserved and not used).
 */
typedef enum
{
    SREC_TYPE_S0 = 0,      /*!< Header record (typically contains information) */
    SREC_TYPE_S1 = 1,      /*!< Data record with 2-byte (16-bit) address */
    SREC_TYPE_S2 = 2,      /*!< Data record with 3-byte (24-bit) address */
    SREC_TYPE_S3 = 3,      /*!< Data record with 4-byte (32-bit) address */
    SREC_TYPE_S5 = 5,      /*!< Count record, 16-bit count of S1/S2/S3 records */
    SREC_TYPE_S6 = 6,      /*!< Count record, 24-bit count of S1/S2/S3 records */
    SREC_TYPE_S7 = 7,      /*!< Termination record for S3 (4-byte start address) */
    SREC_TYPE_S8 = 8,      /*!< Termination record for S2 (3-byte start address) */
    SREC_TYPE_S9 = 9,      /*!< Termination record for S1 (2-byte start address) */
    SREC_TYPE_UNKNOWN = 0xFF /*!< Unknown or invalid record type */
} SREC_RecordType_t;

/**
 * @struct SREC_Record
 * @brief Structure to hold parsed information from a single SREC line.
 */
typedef struct
{
    SREC_RecordType_t record_type;              /*!< Type of the SREC record */
    uint32_t address;                           /*!< Address (S1-S3), count (S5-S6) or start address (S7-S9) */
    uint8_t  data[SREC_MAX_DATA_LEN];           /*!< Data bytes from record */
    uint8_t  data_length;                       /*!< Number of valid data bytes */
    bool     checksum_ok;                       /*!< Checksum verification result */
} SREC_Record_t;

/*******************************************************************************
 * API Prototypes
 ******************************************************************************/

/**
 * @brief  Parse one SREC line (a trailing "\r\n" or "\n" is allowed).
 * @retval true if the line is valid and the checksum is correct,
 *         false on a format error or a wrong checksum.
 */
bool SREC_Parse_Line(const char *line, SREC_Record_t *rec);

#endif /* INC_SREC_PARSER_H_ */

/*******************************************************************************
 * End of File
 ******************************************************************************/
