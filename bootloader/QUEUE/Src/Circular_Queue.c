/*******************************************************************************
 * @file    Circular_Queue.c
 * @brief   Implementation of a fixed-size circular queue for SREC text lines.
 * @details Provides enqueue/dequeue operations for buffering S-Record lines
 *          received via communication interfaces before parsing.
 *
 *          Example with QUEUE_MAX_SIZE = 4 (rear = next slot to write):
 *
 *            Init         :  [ ][ ][ ][ ]   front = 0, rear = 0, count = 0
 *            Push A, B, C :  [A][B][C][ ]   front = 0, rear = 3, count = 3
 *            Pop  → A     :  [ ][B][C][ ]   front = 1, rear = 3, count = 2
 *            Push D, E    :  [E][B][C][D]   front = 1, rear = 1, count = 4 (full)
 *                             ↑ E wrapped around: rear = (3 + 1) % 4 = 0
 *
 * @date    Oct 19, 2025
 * @author  nhduong
 ******************************************************************************/

#include <string.h>
#include "Circular_Queue.h"
#include "stm32f407xx.h"

/**
 * @brief Initialize the circular queue to an empty state.
 * @param q Pointer to queue instance.
 */
void Queue_Init(SrecQueue_t *q)
{
    q->front = 0U;
    q->rear  = 0U;
    q->count = 0U;
}

/**
 * @brief Check if the queue is empty.
 * @param q Pointer to queue instance.
 * @return true  if empty, false otherwise.
 */
bool Queue_IsEmpty(const SrecQueue_t *q)
{
    return (q->count == 0U);
}

/**
 * @brief Check if the queue is full.
 * @param q Pointer to queue instance.
 * @return true  if full, false otherwise.
 */
bool Queue_IsFull(const SrecQueue_t *q)
{
    return (q->count == QUEUE_MAX_SIZE);
}

/**
 * @brief Push (enqueue) a new line into the queue.
 *
 * Called from the UART interrupt. The main loop cannot run in the middle of
 * an interrupt, so count++ here needs no protection.
 *
 * @param q    Pointer to queue instance.
 * @param line Null-terminated string to enqueue.
 * @return true  if successfully enqueued.
 * @return false if queue is full, input is NULL or the line is too long
 *               (line is NOT stored).
 */
bool Queue_Push(SrecQueue_t *q, const char *line)
{
    size_t len;

    if ((q == NULL) || (line == NULL) || Queue_IsFull(q))
    {
        return false;
    }

    /* Refuse a line that does not fit, instead of cutting it silently */
    len = strlen(line);
    if (len >= QUEUE_MAX_LINE_LEN)
    {
        return false;
    }

    /* 1. Copy the line (with its '\0') into the free slot */
    memcpy(q->buffer[q->rear], line, len + 1U);

    /* 2. Move rear to the next free slot (wrap around at the end) */
    q->rear = (uint8_t)((q->rear + 1U) % QUEUE_MAX_SIZE);

    /* 3. One more line in the queue, only after the data is copied */
    q->count++;

    return true;
}

/**
 * @brief Pop (dequeue) the oldest line from the queue.
 *
 * Called from the main loop. count-- is done in 3 CPU steps (read, subtract,
 * write). If the UART interrupt runs Queue_Push between those steps, its
 * count++ would be overwritten and one line would be lost. So interrupts are
 * disabled while count is updated.
 *
 * @param q         Pointer to queue instance.
 * @param out_line  Destination buffer to receive the dequeued line.
 *                  Must be at least QUEUE_MAX_LINE_LEN bytes.
 * @return true  if a line was successfully dequeued.
 * @return false if the queue is empty or output buffer is NULL.
 */
bool Queue_Pop(SrecQueue_t *q, char *out_line)
{
    if ((q == NULL) || (out_line == NULL) || Queue_IsEmpty(q))
    {
        return false;
    }

    /* 1. Copy the oldest line out (the interrupt never writes this slot
          while it is still counted in the queue) */
    strcpy(out_line, q->buffer[q->front]);

    /* 2. Move front to the next slot (wrap around at the end) */
    q->front = (uint8_t)((q->front + 1U) % QUEUE_MAX_SIZE);

    /* 3. Free the slot. The barrier in DISABLE_IRQ() also keeps the copy above
          before count--, so the interrupt cannot overwrite the slot too early */
    DISABLE_IRQ();
    q->count--;
    ENABLE_IRQ();

    return true;
}

/*=============================================================================
 * End of File
 *===========================================================================*/
