/**
 * @file 
 *
 * This file implements the functions buffers management 
 *
 * $Id: bmm.c,v 1.2 2014/11/11 14:52:58 ranjan Exp $
 *
 * Copyright (c) 2011, Greenvity Communication All rights reserved.
 *
 */
#ifdef HYBRII_802154
/* === Includes ============================================================ */
#include <stdio.h>
#include "papdef.h"
#include "bmm.h"
#include "qmm.h"
#include "fm.h"

/* === Types =============================================================== */


/* === Macros ============================================================== */

/* === Globals ============================================================= */

/**
 * Common Buffer pool holding the buffer user area
 */
static xdata uint8_t buf_pool[(TOTAL_NUMBER_OF_BUFFERS * BUFFER_SIZE)];
/*
 * Array of buffer headers
 */
static xdata buffer_t buf_header[TOTAL_NUMBER_OF_BUFFERS];

/*
 * Queue of buffers
 */
static queue_t free_buffer_q;

/* === Prototypes ========================================================== */


/* === Implementation ====================================================== */

/**
 * bmm_buffer_init.
 *
 * This function initializes the buffer module.
 * This function should be called before using any other functionality
 * of buffer module.
 */
void bmm_buffer_init (void)
{
    uint8_t index;

    /* Initialize free buffer queue */
    qmm_queue_init(&free_buffer_q, TOTAL_NUMBER_OF_BUFFERS);

    for (index = 0; index < TOTAL_NUMBER_OF_BUFFERS; index++) {
        /*
         * Initialize the buffer body pointer with address of the
         * buffer body
         */
        buf_header[index].body = buf_pool + (index * BUFFER_SIZE);

        /* Append the buffer to free large buffer queue */
        qmm_queue_append(&free_buffer_q, &buf_header[index]);
    }
}


/**
 * This function allocates a buffer and returns a pointer to the buffer.
 * The same pointer should be used while freeing the buffer.User should
 * call BMM_BUFFER_POINTER(buf) to get the pointer to buffer user area.
 *
 * size: size of buffer to be allocated.
 *
 * return pointer to the buffer allocated, NULL if buffer not available.
 */

u8 buffCount=0;
 
buffer_t *bmm_buffer_alloc (uint8_t size)
{
    buffer_t *buffer_p = NULL;

    /*
     * Allocate buffer only if size requested is less than or equal to  maximum
     * size that can be allocated.
     */
    if (size <= BUFFER_SIZE) {
        buffer_p = qmm_queue_remove(&free_buffer_q, NULL);
    }
	
	if(buffer_p)
	{
		buffCount++;
		FM_Printf(FM_APP, "\nba1.2 %bu", buffCount);			
	}
	else
	{
		FM_Printf(FM_APP, "\nba1.3 %bu", buffCount);
	}
    return buffer_p;
}


/**
 * Frees up a buffer.
 *
 * This function frees up a buffer. The pointer passed to this function
 * should be the pointer returned during buffer allocation. The result is
 * unpredictable if an incorrect pointer is passed.
 *
 * pbuffer Pointer to buffer that has to be freed.
 */
void bmm_buffer_free (buffer_t *buffer_p)
{
    if (NULL == buffer_p) {
        /* If the buffer pointer is NULL abort free operation */
        return;
    }

    /* Append the buffer into free large buffer queue */
	buffCount--;
	FM_Printf(FM_APP, "\nba1.4:%bu", buffCount);
    qmm_queue_append(&free_buffer_q, buffer_p);
}
#endif //HYBRII_802154