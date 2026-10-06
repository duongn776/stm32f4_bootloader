/*******************************************************************************
 * @file    Circular_Queue.h
 * @brief   Fixed-size circular queue for S-Record text lines.
 * @details This module provides a lightweight ring buffer to temporarily store
 *          SREC lines (null-terminated ASCII) received over a stream interface
 *          (e.g., UART) before parsing/processing in a bootloader.
 *
 * @date    Oct 19, 2025
 * @author  nhduong
 ******************************************************************************/

#ifndef INC_CIRCULAR_QUEUE_H_
#define INC_CIRCULAR_QUEUE_H_

/*=============================================================================
 * Includes
 *===========================================================================*/
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

/*=============================================================================
 * Definitions
 *===========================================================================*/
#define QUEUE_MAX_SIZE        4U
#define QUEUE_MAX_LINE_LEN    70U

/*=============================================================================
 * Type Definitions
 *===========================================================================*/

/*
 * Usage: Queue_Push() is called from the UART interrupt,
 *        Queue_Pop()  is called from the main loop.
 *
 *   front : changed only by Queue_Pop  (main)
 *   rear  : changed only by Queue_Push (interrupt)
 *   count : changed by BOTH → volatile + protected in Queue_Pop
 */
typedef struct
{
    char             buffer[QUEUE_MAX_SIZE][QUEUE_MAX_LINE_LEN]; /*!< Storage for lines           */
    uint8_t          front;     /*!< Index of the oldest line (next to pop)                  */
    uint8_t          rear;      /*!< Index of the next free slot (next to push)              */
    volatile uint8_t count;     /*!< Number of stored lines (shared with interrupt)          */
} SrecQueue_t;

/*=============================================================================
 * API Prototypes
 *===========================================================================*/
void Queue_Init(SrecQueue_t *q);
bool Queue_Push(SrecQueue_t *q, const char *line);
bool Queue_Pop(SrecQueue_t *q, char *out_line);
bool Queue_IsEmpty(const SrecQueue_t *q);
bool Queue_IsFull(const SrecQueue_t *q);

#endif /* INC_CIRCULAR_QUEUE_H_ */

/*=============================================================================
 * End of File
 *===========================================================================*/
