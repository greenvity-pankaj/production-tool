/**
 * @file mac_hal_intf.c
 *
 * Interface between MAC and HAL
 *
 * $Id: mac_hal_intf.c,v 1.4 2014/06/09 13:19:46 kiran Exp $
 *
 * Copyright (c) 2011, Greenvity Communication All rights reserved.
 *
 */

/* === Includes ============================================================ */
#include <stdio.h>
#include <string.h>
#include "papdef.h"
#include "return_val.h"
#include "bmm.h"
#include "qmm.h"
#include "mac_const.h"
#include "mac_msgs.h"
#include "mac_hal.h"
#include "mac_api.h"
#include "mac_data_structures.h"
#include "mac_internal.h"
#include "mac.h"
#include "mac_security.h"
#include "timer.h"
#include "stm.h"

/* === Macros =============================================================== */


/* === Globals ============================================================= */


/* === Prototypes ========================================================== */

/* === Implementation ====================================================== */

/**
 * Checks whether the indirect data frame address matches
 * with the address passed.
 *
 * buf_p        - Pointer to indirect data buffer
 * search_buf_p - Pointer to the buffer to be searched
 *
 * return:
 * TRUE if found the search_buf_p
 * FALSE - otherwise
 */
bool find_buffer (void xdata *buf_p, void xdata *search_buf_p)
{
    uint8_t *buf_body_p = BMM_BUFFER_POINTER((buffer_t *)search_buf_p);

    if (buf_p == buf_body_p) {
        return (TRUE);
    }
    return (FALSE);
}

/**
 * Helper function to remove transmitted indirect data from the queue
 *
 * frame_p - Pointer to transmitted frame
 */
static void remove_frame_from_indirect_q (frame_info_t *frame_p)
{
    search_t find_buf;

#ifdef CALLBACK
    find_buf.compare_func = find_buffer;
#else
    find_buf.compare_func_id = MAC_FIND_BUFFER;
#endif

    /* Update the address to be searched */
    find_buf.handle = (void *)frame_p->buffer_header_p;

    qmm_queue_remove(&indirect_data_q, &find_buf);
    if (indirect_data_q.size == 0) {
        mac_hal_frame_pending(false);
    }
}

/*
 * mac_hal_intf_tx_frame_done
 *
 * According to the frame type that has previously been sent, the
 * corresponding actions are taken and the MAC returns to its standard state.
 *
 * tx_status - Status of transmission
 * frame_p   - Pointer to the transmitted frame
 */
void mac_hal_intf_tx_frame_done (retval_t tx_status,
                                 frame_info_t *frame_p)
{
    buffer_t *mcps_buf_p;

    

    if (HAL_BC_TX_DONE == tx_status) {
        /* 
         * This is the case where the Beacon is sent
         * in CSMA mode from the beacon template.
         * No need to free the buffer as beacon is transmitted using
         * the static buffer.
         */
        mac_stats_g.tx_bc_pkts_count++;
        mac_stats_g.tx_bc_bytes_count += (frame_p->mpdu_p[0] + FCS_LEN);
        return;
    }

    if (tx_status != MAC_CHANNEL_ACCESS_FAILURE) {
        mac_stats_g.tx_pkts_count++;
        mac_stats_g.tx_bytes_count += (frame_p->mpdu_p[0] + FCS_LEN);
    }

    if (((MAC_SCAN_ACTIVE == mac_scan_state) && 
          (frame_p->msg_type == BEACONREQUEST)) ||
        ((MAC_SCAN_ORPHAN == mac_scan_state) && 
         (frame_p->msg_type == ORPHANNOTIFICATION))){
        bmm_buffer_free(frame_p->buffer_header_p);
        mac_scan_send_complete(tx_status);
        return;
    }

    switch (frame_p->msg_type) {
    case MCPS_MESSAGE:
        mcps_buf_p = frame_p->buffer_header_p;
        if (frame_p->indirect_in_transit) {
            frame_p->indirect_in_transit = false;
            if (MAC_SUCCESS == tx_status ||
                MAC_FRAME_TOO_LONG == tx_status) {
                remove_frame_from_indirect_q(frame_p);
                /* Create the MCPS DATA confirmation message */
                mac_gen_mcps_data_conf((buffer_t *)mcps_buf_p,
                                       (uint8_t)tx_status,
                                       frame_p->msduHandle,
                                       frame_p->time_stamp);
            }
        } else {
            /* Create the MCPS DATA confirmation message */
            mac_gen_mcps_data_conf((buffer_t *)mcps_buf_p,
                                   (uint8_t)tx_status,
                                   frame_p->msduHandle,
                                   frame_p->time_stamp);
        }
        break;


    case NULL_FRAME:
        /* Free the buffer allocated for the Null data frame */
        bmm_buffer_free(frame_p->buffer_header_p);
        break;

    case DISASSOCIATIONNOTIFICATION:
        break;

    case ASSOCIATIONREQUEST:
        if (MAC_SUCCESS == tx_status) {
            uint32_t response_timer;

            /*
             ^ Received ACK for Association.REQ, set timer to send Data.REQ
             */
            mac_poll_state = MAC_AWAIT_ASSOC_RESPONSE;
            response_timer = mac_pib_macResponseWaitTime;
            response_timer = HAL_CONVERT_SYMBOLS_TO_US(response_timer) / 1000;
            if (STATUS_SUCCESS != 
                STM_StartTimer(mac_rsp_wait_timer, response_timer)) {
                printf("\nFailed to start mac_rsp_wait_timer");
            }
            
        } else {
            mac_gen_mlme_associate_conf(frame_p->buffer_header_p,
		                                tx_status, BROADCAST);
        }
        break;

    case DATAREQUEST:           /* Caused by MLME-POLL.request */
    case DATAREQUEST_IMPL_POLL: /* poll without MLME-POLL.request */
        /* Free the data_request buffer */
        bmm_buffer_free(frame_p->buffer_header_p);
        break;

    case ASSOCIATIONRESPONSE:
        /*
         * Association Response is ALWAYS indirect, so not further check for
         * indirect_in_transit required, but clear the flag.
         */
        break;

    case ORPHANREALIGNMENT:
        break;

    case PANIDCONFLICTNOTIFICAION:
        /* 
         * Free the buffer allocated for the Pan-Id conflict
         * notification frame
         */
        bmm_buffer_free(frame_p->buffer_header_p);
        break;

    case COORDINATORREALIGNMENT:
        /*
         * The coordinator realignment command has been sent out
         * successfully. Hence the MAC should be updated with the new
         * parameters given in the MLME.START_request with
         * coordinator realignment command
         */
        mac_coord_realignment_command_tx_success(tx_status,
                                                 frame_p->buffer_header_p);
        break;

    case BEACON_MESSAGE:
        /* 
         * No need to free the buffer as beacon is transmitted using
         * the static buffer
         */
        break;

    default:
        return;
    }
}
