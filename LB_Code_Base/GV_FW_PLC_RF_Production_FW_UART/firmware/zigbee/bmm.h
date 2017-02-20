/**
 * @file 
 *
 * This file contains the Buffer Management Module definitions.
 *
 * $Id: bmm.h,v 1.1 2014/01/10 17:27:11 yiming Exp $
 *
 * Copyright (c) 2011, Greenvity Communication All rights reserved.
 *
 */

/* Prevent double inclusion */
#ifndef _BMM_H_
#define _BMM_H_

/* === Includes ============================================================ */

/* === Macros ============================================================== */

#define TOTAL_NUMBER_OF_BUFFERS     16
#define BUFFER_SIZE                192
/**
 * This macro provides the pointer to the corresponding body of the supplied buffer header.
 */
#define BMM_BUFFER_POINTER(buf) ((buf)->body)

/* === Types =============================================================== */

typedef enum buffer_mode_e
{
    REMOVE_MODE,
    READ_MODE
} buffer_mode_t;

/**
 * Buffer structure holding information of each buffer.
 *
 */
typedef struct buffer_s
{
    uint8_t *body;             /* Pointer to the buffer body */
    struct buffer_s *next;     /* Pointer to next free buffer */
} buffer_t;

/* === Externals =========================================================== */


/* === Prototypes ========================================================== */

void bmm_buffer_init(void);
buffer_t *bmm_buffer_alloc(uint8_t size);
void bmm_buffer_free(buffer_t *pbuffer);

#endif /* _BMM_H_ */
