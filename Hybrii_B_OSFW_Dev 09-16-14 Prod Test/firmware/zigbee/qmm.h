/**
 * @file
 *
 * This file contains the Queue Management Module definitions.
 *
 * $Id: qmm.h,v 1.1 2014/01/10 17:27:11 yiming Exp $
 *
 * Copyright (c) 2011, Greenvity Communication All rights reserved.
 *
 */

#ifndef _QMM_H_
#define _QMM_H_

/* === Includes ============================================================ */

#include "bmm.h"

/* === Macros ============================================================== */


/* === Types =============================================================== */

/*
 * Function ID for non callback method
 */
#define MAC_BEACON_ADD_PENDING_SHORT_ADDR      1 
#define MAC_BEACON_ADD_PENDING_EXT_ADDR        2 
#define MAC_DATA_FIND_SHORT_ADDR               3 
#define MAC_DATA_FIND_EXT_ADDR                 4
#define MAC_DECREMENT_PERSI_TIME               5
#define MAC_CHECK_PERSI_TIME_ZERO              6
#define MAC_FIND_BUFFER                        7
#define MAC_FIND_MSDU                          8

/**
 * @brief Structure to search for a buffer to be removed from a queue
 */
typedef struct search_s
{
#ifdef CALLBACK
    /** Pointer to search criteria function */
    bool (*compare_func)(void xdata *buf, void xdata *handle);
#else
    u8   compare_func_id;
#endif
    /** Handle to callbck parameter */
    void *handle;
} search_t;

typedef struct queue_s
{
    /** Pointer to head of queue */
    buffer_t *head;
    /** Pointer to tail of queue */
    buffer_t *tail;
    /**
     * Maximum number of buffers that can be accomodated in the current queue
     * Note: This is only required if the queue capacity shall be different
     * from 255.
     */
    uint8_t capacity;
    /**
     * Number of buffers present in the current queue
     */
    uint8_t size;
} queue_t;

/* === Externals =========================================================== */
extern bool mac_beacon_add_pending_short_address_cb(void xdata *buf_ptr,
                                                    void xdata *handle);
extern bool mac_beacon_add_pending_extended_address_cb(void xdata *buf_ptr,
                                                       void xdata *handle);
extern bool mac_data_find_short_addr_buffer(void *buf_p, void *short_addr_p);
extern bool mac_data_find_long_addr_buffer(void *buf_p, void *long_addr_p);
extern bool decrement_persistence_time(void *buf_ptr, void *handle);
extern bool check_persistence_time_zero(void *buf_ptr, void *handle);
extern bool find_buffer(void xdata *buf_p, void xdata *search_buf_p);
extern bool check_msdu_handle_cb(void xdata *buf_p, void xdata *handle_p);

/* === Prototypes ========================================================== */

void qmm_queue_init(queue_t *q, uint8_t capacity);
bool qmm_queue_append(queue_t *q, buffer_t *buf);
buffer_t *qmm_queue_remove(queue_t *q, search_t *search);
buffer_t *qmm_queue_read(queue_t *q, search_t *search);
void qmm_queue_flush(queue_t *q);

#endif /* _QMM_H_ */

