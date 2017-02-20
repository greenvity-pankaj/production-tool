/**
 * @file qmm.c
 *
 *  This file implements the  functions for initializing the queues,
 *  appending a buffer into the queue, removing a buffer from the queue and
 *  reading a buffer from the queue as per the search criteria.
 *
 * $Id: qmm.c,v 1.2 2014/11/11 14:52:59 ranjan Exp $
 *
 * Copyright (c) 2011, Greenvity Communication All rights reserved.
 *
 */
#ifdef HYBRII_802154 
/* === Includes ============================================================ */
#include <REG51.H>
#include <stdio.h>
#include "papdef.h"
#include "bmm.h"
#include "qmm.h"

/* === Types =============================================================== */

/* === Macros ============================================================== */

/* === Prototypes ========================================================== */

/* === Implementation ====================================================== */

/**
 * This function initializes the queue. Note that this function
 * should be called before invoking any other functionality of QMM.
 *
 */
void qmm_queue_init (queue_t *q_p, uint8_t capacity)
{
    q_p->head = NULL;
    q_p->tail = NULL;
    q_p->size = 0;
    q_p->capacity = capacity;
}


/**
 * This function appends a buffer into the queue.
 *
 * q_p   - Pointer to the Queue where buffer should be appended
 *
 * buf_p - Pointer to the buffer that should be appended into the queue.
 * Note that this pointer should be same as the
 * pointer returned by bmm_buffer_alloc.
 */
bool qmm_queue_append (queue_t *q_p, buffer_t *buf_p)
{
    bool status;
    u8   interrupt_status;

    interrupt_status = EA;
    EA = 0;

    /* Check if queue is full */
    if (q_p->size == q_p->capacity) {
        /* Buffer cannot be appended as queue is full */
        status = FALSE;
    } else {
        /* Check whether queue is empty */
        if (q_p->size == 0) {
            /* Add the buffer at the head */
            q_p->head = buf_p;
        } else {
            /* Add the buffer at the end */
            q_p->tail->next = buf_p;
        }

        /* Update the list */
        q_p->tail = buf_p;

        /* Terminate the list */
        buf_p->next = NULL;

        /* Update size */
        q_p->size++;

        status = TRUE;
    }

    EA = interrupt_status;

    return (status);
}


/*
 *
 * This function reads or removes a buffer from a queue as per
 * the search criteria provided. If search criteria is NULL, then the first
 * buffer is returned, otherwise buffer matching the given criteria is returned
 *
 * q_p  - Pointer to the queue where the buffer is to be read or removed.
 *
 * mode - Mode of operations. If this parameter has value REMOVE_MODE,
 * buffer will be removed from queue and returned. If this parameter is
 * READ_MODE, buffer pointer will be returned without removing from queue.
 *
 * search_p - Search criteria structure pointer.
 *
 * Return buffer header pointer, if the buffer is successfully
 * removed or read, otherwise NULL is returned.
 */
static buffer_t *queue_read_or_remove (queue_t *q_p,
                                       buffer_mode_t mode,
                                       search_t *search_p)
{

    buffer_t *buffer_current_p = NULL;
    buffer_t *buffer_previous_p;
    u8       interrupt_status;
    
    interrupt_status = EA;

    EA = 0;
    /* Check whether queue is empty */
    if (q_p->size != 0) {
        buffer_current_p  = q_p->head;
        buffer_previous_p = q_p->head;

        /* First get buffer matching with criteria */
        if (NULL != search_p) {
            bool match;
            /* Search for all buffers in the queue */
            while (NULL != buffer_current_p) {
#ifdef CALLBACK
                match = search_p->compare_func((void *)buffer_current_p->body,
                                                search_p->handle);
#else
                switch (search_p->compare_func_id) {
                case MAC_BEACON_ADD_PENDING_SHORT_ADDR:
                    match = 
                        mac_beacon_add_pending_short_address_cb(
                            (void *)buffer_current_p->body, search_p->handle);
                    break;
                case MAC_BEACON_ADD_PENDING_EXT_ADDR:
                    match =
                        mac_beacon_add_pending_extended_address_cb(
                            (void *)buffer_current_p->body, search_p->handle);
                    break;
                case MAC_DATA_FIND_SHORT_ADDR:
                    match =
                        mac_data_find_short_addr_buffer(
                            (void *)buffer_current_p->body, search_p->handle);
                    break;
                case MAC_DATA_FIND_EXT_ADDR:
                    match =
                        mac_data_find_long_addr_buffer(
                            (void *)buffer_current_p->body, search_p->handle);
                    break;
                case MAC_DECREMENT_PERSI_TIME:
                    match =
                        decrement_persistence_time(
                            (void *)buffer_current_p->body, search_p->handle);
                    break;
                case MAC_CHECK_PERSI_TIME_ZERO:
                    match =
                        check_persistence_time_zero(
                            (void *)buffer_current_p->body, search_p->handle);
                    break;
                case MAC_FIND_BUFFER:
                    match =
                        find_buffer((void *)buffer_current_p->body,
                                    search_p->handle);
                    break;
                case MAC_FIND_MSDU:
                    match =
                        check_msdu_handle_cb((void *)buffer_current_p->body,
                                             search_p->handle);
                    break;
                default:
                    goto done;
                }
#endif

                if (match == TRUE) {
                    /* Break, if search criteria matches */
                    break;
                }

                buffer_previous_p = buffer_current_p;
                buffer_current_p  = buffer_current_p->next;
            }
        }

        /* Buffer matching with search criteria found */
        if (NULL != buffer_current_p) {
            /* Remove buffer from the queue */
            if (REMOVE_MODE == mode) {
                /* Update head if buffer removed is first node */
                if (buffer_current_p == q_p->head) {
                    q_p->head = buffer_current_p->next;
                } else {
                    /* Update the link by removing the buffer */
                    buffer_previous_p->next = buffer_current_p->next;
                }

                /* Update tail if buffer removed is last node */
                if (buffer_current_p == q_p->tail) {
                    q_p->tail = buffer_previous_p;
                }

                /* Update size */
                q_p->size--;

                if (NULL == q_p->head) {
                    q_p->tail = NULL;
                }
            } else {
                /* Nothing needs done if the mode is READ_MODE */
            }
        }
    }

done:
    EA = interrupt_status;

    /* Return the buffer. note that pointer to header of buffer is returned */
    return (buffer_current_p);
}

/**
 *
 * This function removes a buffer from queue
 *
 * q_p - Pointer to the queue where buffer should be removed
 *
 * search_p - Search criteria. If this parameter is NULL, first buffer in the
 * queue will be removed. Otherwise buffer matching the criteria will be
 * removed.
 *
 * return Pointer to the buffer header, if the buffer is successfully removed,
 * NULL otherwise.
 */
buffer_t *qmm_queue_remove (queue_t *q_p, search_t *search_p)
{
    return (queue_read_or_remove(q_p, REMOVE_MODE, search_p));
}

/**
 *
 * This function reads either the first buffer if search is NULL or buffer
 * matching the given criteria from queue.
 *
 * q_p - Pointer to The queue where the buffer should be read.
 *
 * search_p - If this parameter is NULL first buffer in the queue will be
 * read. Otherwise buffer matching the criteria will be read
 *
 * Return pointer to the buffer header which is to be read, NULL if the buffer
 * is not available
 */
buffer_t *qmm_queue_read (queue_t *q_p, search_t *search_p)
{
    return (queue_read_or_remove(q_p, READ_MODE, search_p));
}

/**
 * flushing a specific queue
 *
 * q - Pointer to the Queue to be flushed
 */
void qmm_queue_flush (queue_t *q_p)
{
    buffer_t *buf_to_free_p;

    while (q_p->size > 0) {
        /* Remove the buffer from the queue and free it */
        buf_to_free_p = qmm_queue_remove(q_p, NULL);

        if (NULL == buf_to_free_p) {
            q_p->size = 0;
            return;
        }
        bmm_buffer_free(buf_to_free_p);
    }
}

#endif //HYBRII_802154

