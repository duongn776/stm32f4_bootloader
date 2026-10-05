/*******************************************************************************
 * @file    Srec_Parser.c
 * @brief   Implementation of Motorola S-Record parser module.
 * @details This file contains helper functions to parse a single SREC line,
 *          extract address, data, and verify checksum integrity.
 *
 *          Line layout (each byte = 2 hex characters):
 *            S3 15 08010000 00000220...49070108 74
 *            |  |  |        |                   +- checksum (1 byte)
 *            |  |  |        +- data (count - address - 1 bytes)
 *            |  |  +- address (2/3/4 bytes, big-endian)
 *            |  +- count = number of bytes after it (address + data + checksum)
 *            +- record type, not counted
 *
 * @date    Oct 19, 2025
 * @author  nhduong
 ******************************************************************************/

/*******************************************************************************
 * Includes
 ******************************************************************************/
#include <string.h>
#include "Srec_Parser.h"

/*******************************************************************************
 * Private Functions Prototypes
 ******************************************************************************/
static int32_t hexchar_to_value(char c);
static int32_t hexbyte_to_value(const char *hexbyte);


/**
 * @brief Convert a hexadecimal character to its integer value.
 *
 * @param c Hexadecimal character ('0'-'9', 'A'-'F', 'a'-'f').
 * @return int32_t The corresponding integer value, or -1 if invalid.
 */
static int32_t hexchar_to_value(char c)
{
    int32_t ret_val;

    /* Convert hex character to integer value */
    if ((c >= '0') && (c <= '9'))
    {
        ret_val = (int32_t)(c - '0');
    }
    else if ((c >= 'A') && (c <= 'F'))
    {
        ret_val = (int32_t)(10 + (c - 'A'));
    }
    else if ((c >= 'a') && (c <= 'f'))
    {
        ret_val = (int32_t)(10 + (c - 'a'));
    }
    else
    {
        /* Invalid hex character */
        ret_val = -1;
    }

    return ret_val;
}

/**
 * @brief Convert two hexadecimal characters to a byte value.
 *
 * @param hexbyte Pointer to a 2-character hexadecimal string.
 * @return int32_t The corresponding byte value (0–255), or -1 if invalid.
 */
static int32_t hexbyte_to_value(const char *hexbyte)
{
    int32_t high_nibble, low_nibble;

    /* Convert high nibble */
    high_nibble = hexchar_to_value(hexbyte[0]);

    /* Convert low nibble */
    low_nibble = hexchar_to_value(hexbyte[1]);

    /* Check for invalid hex digits */
    if ((high_nibble < 0) || (low_nibble < 0))
    {
        return -1;
    }

    /* Combine nibbles to form full byte */
    return ((high_nibble << 4) | low_nibble);
}


bool SREC_Parse_Line(const char *line, SREC_Record_t *rec)
{
    int32_t tmp = 0;
    size_t len = 0U;
    uint16_t offset = 4U;           /* uint16_t: a line can be up to 4 + 2 * 255 = 514 chars */
    uint8_t byte_count = 0U;
    uint8_t addr_len = 0U;
    int32_t data_len = 0;
    uint32_t sum = 0U;

    /* Validate input */
    if ((line == NULL) || (rec == NULL))
    {
        return false;
    }

    /* Reset the output, so a failed call never leaves values of a previous line */
    rec->record_type = SREC_TYPE_UNKNOWN;
    rec->address = 0U;
    rec->data_length = 0U;
    rec->checksum_ok = false;

    /* Length without the line ending ("\r\n" or "\n") */
    len = strlen(line);
    while ((len > 0U) && ((line[len - 1U] == '\r') || (line[len - 1U] == '\n')))
    {
        len--;
    }

    /* At least "S" + type + count (4 characters) before reading the count */
    if ((len < 4U) || (line[0] != 'S'))
    {
        return false;
    }

    /***************************************************************************
     * Step 1: Identify record type (S0–S9, S4 is reserved)
     * The enum value is the digit itself, so '3' - '0' = 3 = SREC_TYPE_S3
     ***************************************************************************/
    if ((line[1] < '0') || (line[1] > '9') || (line[1] == '4'))
    {
        return false;
    }
    rec->record_type = (SREC_RecordType_t)(line[1] - '0');

    /***************************************************************************
     * Step 2: Parse byte count field, then check the line length
     ***************************************************************************/
    tmp = hexbyte_to_value(&line[2]);
    if (tmp < 0)
    {
        return false;
    }
    byte_count = (uint8_t)tmp;
    sum += (uint32_t)tmp;

    /* "S" + type + count (4 chars) + byte_count bytes of 2 chars each.
     * After this check, every read below stays inside the string */
    if (len != (4U + (2U * (size_t)byte_count)))
    {
        return false;
    }

    /***************************************************************************
     * Step 3: Determine address field length (depends on record type)
     ***************************************************************************/
    switch (rec->record_type)
    {
        case SREC_TYPE_S0:
        case SREC_TYPE_S1:
        case SREC_TYPE_S5:
        case SREC_TYPE_S9:
            addr_len = 2U;
            break;

        case SREC_TYPE_S2:
        case SREC_TYPE_S6:
        case SREC_TYPE_S8:
            addr_len = 3U;
            break;

        case SREC_TYPE_S3:
        case SREC_TYPE_S7:
            addr_len = 4U;
            break;

        default:
            return false;
    }

    /***************************************************************************
     * Step 4: Parse address field (big-endian: first byte is the highest)
     ***************************************************************************/
    for (uint8_t i = 0U; i < addr_len; i++)
    {
        tmp = hexbyte_to_value(&line[offset]);
        if (tmp < 0)
        {
            return false;
        }

        rec->address = (rec->address << 8U) | (uint8_t)tmp;
        sum += (uint32_t)tmp;
        offset += 2U;
    }

    /***************************************************************************
     * Step 5: Parse data field
     ***************************************************************************/
    data_len = (int32_t)byte_count - (int32_t)addr_len - 1;
    if (data_len < 0)
    {
        return false;
    }

    /* S5-S9 have no data field */
    if ((rec->record_type >= SREC_TYPE_S5) && (data_len != 0))
    {
        return false;
    }

    for (uint8_t i = 0U; i < (uint8_t)data_len; i++)
    {
        tmp = hexbyte_to_value(&line[offset]);
        if (tmp < 0)
        {
            return false;
        }

        rec->data[i] = (uint8_t)tmp;
        sum += (uint32_t)tmp;
        offset += 2U;
    }
    rec->data_length = (uint8_t)data_len;

    /***************************************************************************
     * Step 6: Verify checksum
     * checksum = 0xFF - (low byte of the sum of count + address + data),
     * so adding the checksum to that sum always gives 0xFF in the low byte
     ***************************************************************************/
    tmp = hexbyte_to_value(&line[offset]);
    if (tmp < 0)
    {
        return false;
    }
    sum += (uint32_t)tmp;

    rec->checksum_ok = ((sum & 0xFFU) == 0xFFU);

    return rec->checksum_ok;
}
