/**
 * @file mac_mcps_data.c
 *
 * Handles MCPS related primitives
 *
 * $Id: mac_mcps_data.c,v 1.5 2014/06/09 13:19:46 kiran Exp $
 *
 * Copyright (c) 2011, Greenvity Communication All rights reserved.
 */

/* === Includes ============================================================ */

#include <string.h>
#include "papdef.h"
#include "return_val.h"
#include "bmm.h"
#include "qmm.h"
#include "mac_const.h"
#include "mac_msgs.h"
#include "mac_data_structures.h"
#include "mac_hal.h"
#include "mac_internal.h"
#include "mac.h"
#include "mac_security.h"
#include "utils.h"
#include "timer.h"
#include "stm.h"

/* === Implementation ====================================================== */

/*
 * Checks whether MSDU handle matches
 *
 * buf_p    - Pointer to indirect data buffer
 * handle_p - MSDU handle to be searched
 *
 * Return TRUE if MSDU handle matches with the indirect data, FALSE otherwise
 */
bool check_msdu_handle_cb (void xdata *buf_p, void xdata *handle_p)
{
    frame_info_t *frame_p = (frame_info_t *)buf_p;
    uint8_t msdu;

    msdu = *((uint8_t *)handle_p);

    /* Compare the MSDU handle */
    if (frame_p->msduHandle == msdu) {
        return (TRUE);
    }
    return (FALSE);
}

/*
 * Purges a buffer that match the MSDU handle
 *
 * This function tries to purge a given msdu by finding its msdu handle.
 * If the handle is found, that buffer is freed up for further use.
 * This routine will typically be called from the mlme_purge_request routine.
 *
 * msdu_handle - The MSDU handle
 *
 * True if the MSDU handle is found in the indirect queue and removed
 * successfully, false otherwise.
 */
static bool mac_buffer_purge (uint8_t msdu_handle)
{
    uint8_t *buf_ptr;
    search_t find_buf;
    uint8_t handle = msdu_handle;

    /*
     * Callback function  for searching the data having MSDU handle
     * given by purge request
     */
#ifdef CALLBACK
    find_buf.compare_func = check_msdu_handle_cb;
#else
    find_buf.compare_func_id = MAC_FIND_MSDU;
#endif

    /* Update the MSDU handle to be searched */
    find_buf.handle = &handle;

    /* Remove from indirect queue if the short address matches */
    buf_ptr = (uint8_t *)qmm_queue_remove(&indirect_data_q, &find_buf);

    if (NULL != buf_ptr) {
        /* Free the buffer allocated, after purging */
        bmm_buffer_free((buffer_t *)buf_ptr);

        return (TRUE);
    }
   
    /* No data available in the indirect queue with the MSDU handle. */
    mac_hal_frame_pending(false);  /* Tell h/w no more pending frame */

    return (FALSE);
}

/*
 * Builds MCPS data frame
 *
 * This function builds the data frame.
 *
 * mdr_p   - Poiinter to data request parameters
 * frame_p - Pointer to transmission frame
 */
static retval_t build_data_frame (mcps_data_req_t *mdr_p,
                                  frame_info_t *tx_frame_p)
{
    uint8_t  frame_len;
    uint8_t  *frame_p;
    uint16_t fcf = 0;
    uint8_t  *mac_payload_p;

    frame_len = mdr_p->msduLength + FCF_LEN + SEQ_NUM_LEN;

    /*
     * Payload pointer points to data, which was already been copied
     * into buffer
     */
    frame_p = (uint8_t *)tx_frame_p + BUFFER_SIZE - mdr_p->msduLength;

    mac_payload_p = frame_p;
    /*
     * Note: The value of the payload_length parameter will be updated
     *       if security needs to be applied.
     */
    if (mdr_p->Security.SecurityLevel > 0) {
        retval_t build_sec = mac_build_aux_sec_header(&frame_p,
                                                      &mdr_p->Security,
                                                      &frame_len);
        if (MAC_SUCCESS != build_sec) {
            return (build_sec);
        }
    }

    /*
     * Set Source Address.
     */
    if (FCF_SHORT_ADDR == mdr_p->SrcAddrMode) {
        frame_p   -= SHORT_ADDR_LEN;
        frame_len += SHORT_ADDR_LEN;
        mac_utils_16_bit_to_byte_array(hal_pib_ShortAddress, frame_p);
    } else if (FCF_LONG_ADDR == mdr_p->SrcAddrMode) {
        frame_p   -= EXT_ADDR_LEN;
        frame_len += EXT_ADDR_LEN;
        mac_utils_64_bit_to_byte_array(hal_pib_IeeeAddress, frame_p);
    }

    /* Shall the Intra-PAN bit set? */
    if ((hal_pib_PANId == mdr_p->DstPANId)    &&
        (FCF_NO_ADDR   != mdr_p->SrcAddrMode) &&
        (FCF_NO_ADDR   != mdr_p->DstAddrMode)) {
        /*
         * Both address are present and both PAN-Ids are identical.
         * Set intra-PAN bit.
         */
        fcf |= FCF_PAN_ID_COMPRESSION;
    } else if (FCF_NO_ADDR != mdr_p->SrcAddrMode) {
        /* Set Source PAN-Id. */
        frame_p   -= PAN_ID_LEN;
        frame_len += PAN_ID_LEN;
        mac_utils_16_bit_to_byte_array(hal_pib_PANId, frame_p);
    }

    /* Set the Destination Addressing fields. */
    if (FCF_NO_ADDR != mdr_p->DstAddrMode) {
        if (FCF_SHORT_ADDR == mdr_p->DstAddrMode) {
            frame_p   -= SHORT_ADDR_LEN;
            frame_len += SHORT_ADDR_LEN;
            mac_utils_16_bit_to_byte_array((uint16_t)mdr_p->DstAddr.lo_u32,
                                           frame_p);
        } else {
            frame_p   -= EXT_ADDR_LEN;
            frame_len += EXT_ADDR_LEN;
            mac_utils_64_bit_to_byte_array(mdr_p->DstAddr, frame_p);
        }

        frame_p   -= PAN_ID_LEN;
        frame_len += PAN_ID_LEN;
        mac_utils_16_bit_to_byte_array(mdr_p->DstPANId, frame_p);
    }

    /* Set DSN. */
    frame_p--;
    *frame_p = mac_pib_macDSN++;

    /* Set the FCF. */
    fcf |= FCF_SET_FRAMETYPE(FCF_FRAMETYPE_DATA);

    if (mdr_p->Security.SecurityLevel > 0) {
        fcf |= FCF_SECURITY_ENABLED | FCF_FRAME_VERSION_2006;
    }

    if (mdr_p->TxOptions & TX_CAP_ACK) {
        fcf |= FCF_ACK_REQUEST;
    }

    /*
     * 802.15.4-2006 section 7.1.1.1.3:
     *
     * If the msduLength parameter is greater than aMaxMACSafePayloadSize,
     * the MAC sublayer will set the Frame Version subfield of the
     * Frame Control field to one.
     */
    if (mdr_p->msduLength > aMaxMACSafePayloadSize) {
        fcf |= FCF_FRAME_VERSION_2006;
    }

    /* Set FCFs address mode */
    fcf |= FCF_SET_SOURCE_ADDR_MODE(mdr_p->SrcAddrMode);
    fcf |= FCF_SET_DEST_ADDR_MODE(mdr_p->DstAddrMode);

    frame_p -= FCF_LEN;
    mac_utils_16_bit_to_byte_array(fcf, frame_p);

    /*
     * In case the frame gets too large, return error.
     */
    if (frame_len > aMaxPHYPacketSize) {
       return MAC_FRAME_TOO_LONG;
    }

    /* First element shall be length of PHY frame. */
    frame_p -= LENGTH_FIELD_LEN;
    *frame_p = frame_len;

    /* Finished building of frame. */
    tx_frame_p->mpdu_p = frame_p;

    if (mdr_p->Security.SecurityLevel > 0) {
        if (mac_pib_macSecurityEnabled == TRUE) {
            wpan_addr_spec_t dst_addr_spec;

            dst_addr_spec.AddrMode          = mdr_p->DstAddrMode; 
            dst_addr_spec.PANId             = mdr_p->DstPANId; 
            dst_addr_spec.Addr.long_address = mdr_p->DstAddr; 

            return (mac_secure(&mdr_p->Security, &dst_addr_spec));
        } else {
            return (MAC_UNSUPPORTED_SECURITY);
        }
    }

    return MAC_SUCCESS;
}

/*
 * This function creates the mcps data confirm structure,
 * and appends it into internal event queue.
 *
 * buf Buffer for mcps data confirmation.
 * status Data transmission status.
 * handle MSDU handle.
 * timestamp Time in symbols at which the data were transmitted
 */
void mac_gen_mcps_data_conf (buffer_t *buff_p, uint8_t status, uint8_t handle,
                             uint32_t timestamp)
{
    frame_info_t     *tx_frame_p;
    mcps_data_conf_t *mdc_p;
    
    tx_frame_p = (frame_info_t     *)BMM_BUFFER_POINTER(buff_p);
    mdc_p      = (mcps_data_conf_t *)BMM_BUFFER_POINTER(buff_p);

    mdc_p->cmdcode    = MCPS_DATA_CONFIRM;
    mdc_p->msduHandle = handle;
    mdc_p->status     = status;
    mdc_p->Timestamp  = timestamp;

    switch (status) {
    case MAC_NO_ACK:
        mac_stats_g.tx_pkts_no_ack++;
        break;
    case MAC_CHANNEL_ACCESS_FAILURE:
        mac_stats_g.tx_pkts_no_cca++;
        mac_stats_g.tx_errors++;
        break;
    case HAL_FRAME_PENDING:
        mac_stats_g.frame_pending++;
        break;
    case MAC_SUCCESS:
        break;
    default:
        mac_stats_g.tx_errors++;
        break;
    }

    mcps_data_conf(buff_p);
}

/**
 * This function builds the data frame for transmission.
 * The NWK App layer has supplied the parameters.
 * The frame_info_t data type is constructed and filled in.
 * Also the FCF is constructed based on the parameters passed.
 *
 * msg - Pointer to the MCPS-DATA.request parameter
 */
uint32_t data_req_g = 0;
void mcps_data_request (buffer_t *buf_p)
{
    mcps_data_req_t mdr;
    frame_info_t    *tx_frame_p;
    retval_t        status = MAC_FAILURE;

    data_req_g ++;
    memcpy(&mdr, BMM_BUFFER_POINTER(buf_p),
           sizeof(mcps_data_req_t));

    if ((mdr.TxOptions & TX_INDIRECT) == 0) {
        /*
         * Data Requests for a coordinator using direct transmission are
         * accepted in all non-transient states (no polling and no scanning
         * is ongoing).
         */
        if ((MAC_POLL_IDLE != mac_poll_state) ||
            (MAC_SCAN_IDLE != mac_scan_state)) {
            mac_gen_mcps_data_conf(buf_p,
                                   (uint8_t)MAC_CHANNEL_ACCESS_FAILURE,
                                   mdr.msduHandle,
                                   0);
            return;
        }
    }


    /* Check whether somebody requests an ACK of broadcast frames */
    if ((mdr.TxOptions & TX_CAP_ACK) &&
        (FCF_SHORT_ADDR == mdr.DstAddrMode) &&
         (BROADCAST == mdr.DstAddr.lo_u32)) {
        mac_gen_mcps_data_conf(buf_p,
                           (uint8_t)MAC_INVALID_PARAMETER,
                           mdr.msduHandle,
                           0);
        return;
    }

    /* Check whether both Src and Dst Address are not present */
    if ((FCF_NO_ADDR == mdr.SrcAddrMode) &&
        (FCF_NO_ADDR == mdr.DstAddrMode)) {
        mac_gen_mcps_data_conf(buf_p,
                               (uint8_t)MAC_INVALID_ADDRESS,
                               mdr.msduHandle,
                               0);
        return;
    }

    /* Check whether Src or Dst Address indicate reserved values */
    if ((FCF_RESERVED_ADDR == mdr.SrcAddrMode) ||
        (FCF_RESERVED_ADDR == mdr.DstAddrMode)) {
        mac_gen_mcps_data_conf(buf_p,
                               (uint8_t)MAC_INVALID_PARAMETER,
                               mdr.msduHandle,
                               0);
        return;
    }

    tx_frame_p = (frame_info_t *)BMM_BUFFER_POINTER(buf_p);

    /* Store the message type */
    tx_frame_p->msg_type   = MCPS_MESSAGE;
    tx_frame_p->msduHandle = mdr.msduHandle;

    tx_frame_p->indirect_in_transit = false;

    status = build_data_frame(&mdr, tx_frame_p);

    if (MAC_SUCCESS != status) {
        /* The frame is too long. */
        mac_gen_mcps_data_conf(buf_p,
                               (uint8_t)status,
                               mdr.msduHandle,
                               0);
        return;
    }

    /*
     * Broadcast transmission in a beacon-enabled network intiated by a
     * PAN Coordinator or Coordinator is put into the broadcast queue..
     */
    if (((MAC_PAN_COORD_STARTED == mac_state) || 
        (MAC_COORDINATOR == mac_state)) &&
        (hal_pib_BeaconOrder < NON_BEACON_NWK) &&
        (FCF_SHORT_ADDR == mdr.DstAddrMode) &&
        (BROADCAST == mdr.DstAddr.lo_u32)) {
        /* Append the MCPS data request into the broadcast queue */
        if (FALSE == qmm_queue_append(&broadcast_q, buf_p)) {
            mac_gen_mcps_data_conf(buf_p,
                                   (uint8_t)MAC_CHANNEL_ACCESS_FAILURE,
                                   mdr.msduHandle,
                                   0);
            return;
        }
    }

    /*
     * Indirect transmission is only allowed if we are
     * a PAN coordinator or coordinator.
     */
    if ((mdr.TxOptions & TX_INDIRECT) &&
        ((MAC_PAN_COORD_STARTED == mac_state) ||
         (MAC_COORDINATOR == mac_state))) {
        /* Append the MCPS data request into the indirect data queue */
        if (FALSE == qmm_queue_append(&indirect_data_q, buf_p)) {
            mac_gen_mcps_data_conf(buf_p,
                                   (uint8_t)MAC_TRANSACTION_OVERFLOW,
                                   mdr.msduHandle,
                                   0);
            return;
        }

        /*
         * If an FFD does have pending data,
         * the MAC persistence timer needs to be started.
         */
        tx_frame_p->persistence_time = mac_pib_macTransactionPersistenceTime;
        mac_start_persistence_timer();
    } else {
        /* Transmission should be done with CSMA-CA and with frame retries. */
        tx_mode_t tx_mode;

        /*
         * We are NOT indirect, so we need to transmit using
         * CSMA_CA in the CAP (for beacon enabled) or immediately (for
         * a non-beacon enabled).
         */
        mac_trx_wakeup();

        tx_frame_p->buffer_header_p = buf_p;

        if (NON_BEACON_NWK == hal_pib_BeaconOrder) {
            /* In Nonbeacon network the frame is sent with unslotted CSMA-CA. */
            tx_mode = CSMA_UNSLOTTED;
        } else {
            /* In Beacon network the frame is sent with slotted CSMA-CA. */
            tx_mode = CSMA_SLOTTED;
        }

        status = mac_hal_tx_frame(tx_frame_p, tx_mode,
                                  (mdr.Security.SecurityLevel > 0) ? TRUE:FALSE);

        if (MAC_SUCCESS == status) {
            MAC_BUSY();
        } else {
            /* Transmission to TAL failed, generate confirmation message. */
            mac_gen_mcps_data_conf(buf_p,
                                   (uint8_t)MAC_CHANNEL_ACCESS_FAILURE,
                                   mdr.msduHandle,
                                   0);

            /* Set radio to sleep if allowed */
            mac_trx_sleep();
        }
    }
} /* mcps_data_request() */


/**
 *
 * This functions processes a MCPS-PURGE.request from the NHLE.
 * The MCPS-PURGE.request primitive allows the next higher layer
 * to purge an MSDU from the transaction queue.
 * On receipt of the MCPS-PURGE.request primitive, the MAC sublayer
 * attempts to find in its transaction queue the MSDU indicated by the
 * msduHandle parameter. If an MSDU matching the given handle is found,
 * the MSDU is discarded from the transaction queue, and the MAC
 * sublayer issues the MCPSPURGE. confirm primitive with a status of
 * MAC_SUCCESS. If an MSDU matching the given handle is not found, the MAC
 * sublayer issues the MCPS-PURGE.confirm primitive with a status of
 * INVALID_HANDLE.
 *
 * @param msg Pointer to the MCPS-PURGE.request parameter
 */
void mcps_purge_request (buffer_t *buf_p)
{
    mcps_purge_req_t *mpr_p =
        (mcps_purge_req_t *)BMM_BUFFER_POINTER(buf_p);

    mcps_purge_conf_t *mpc_p = (mcps_purge_conf_t *)mpr_p;

    uint8_t purge_handle = mpr_p->msduHandle;

    /* Update the purge confirm structure */
    mpc_p->cmdcode = MCPS_PURGE_CONFIRM;
    mpc_p->msduHandle = purge_handle;

    if (mac_buffer_purge(purge_handle)) {
        mpc_p->status = MAC_SUCCESS;
    } else {
        mpc_p->status = MAC_INVALID_HANDLE;
    }

    mcps_purge_conf(buf_p);
}

/**
 *
 * This function processes the data frames received and sends
 * mcps_data_indication to the NHLE.
 *
 * buf_ptr - Pointer to receive buffer of the data frame
 */
void mac_process_data_frame (buffer_t *buf_ptr)
{
    mcps_data_ind_t *mdi_p = (mcps_data_ind_t *)BMM_BUFFER_POINTER(buf_ptr);

    if (mac_parse_data.mac_payload_length == 0) {
        /*
         * A null frame is neither indicated to the higher layer
         * nor checked for for frame pending bit set, since
         * null data frames with frame pending bit set are nonsense.
         */
        /* Since no indication is generated, the frame buffer is released. */
        bmm_buffer_free(buf_ptr);

        /* Set radio to sleep if allowed */
        mac_trx_sleep();
    } else {
        /* Build the MLME_Data_indication parameters. */
        mdi_p->DSN = mac_parse_data.sequence_number;
        mdi_p->Timestamp = mac_parse_data.time_stamp;

        /* Source address info */
        mdi_p->SrcAddrMode = mac_parse_data.src_addr_mode;
        mdi_p->SrcPANId = mac_parse_data.src_panid;

        if (FCF_LONG_ADDR == mdi_p->SrcAddrMode ||
            FCF_SHORT_ADDR == mdi_p->SrcAddrMode) {
            EXT_ADDR_CLEAR(mdi_p->SrcAddr);
            if (FCF_SHORT_ADDR == mdi_p->SrcAddrMode) {
                ADDR_COPY_DST_SRC_16(mdi_p->SrcAddr.lo_u32,
                                     mac_parse_data.src_addr.short_address);
            } else if (FCF_LONG_ADDR == mdi_p->SrcAddrMode) {
                ADDR_COPY_DST_SRC_64(mdi_p->SrcAddr,
                                     mac_parse_data.src_addr.long_address);
            }
        } else {
            /*
             * Even if the Source address mode is zero, and the source address
             * informationis ís not present, the values are cleared to prevent
             * the providing of trash information.
             */
            mdi_p->SrcPANId = 0;
            EXT_ADDR_CLEAR(mdi_p->SrcAddr);
        }


        /* Start of duplicate detection. */
        if ((mdi_p->DSN == mac_last_dsn) &&
            (EXT_ADDR_MATCH(mdi_p->SrcAddr, mac_last_src_addr))) {
#ifndef DUP_CNT
            /*
             * Don't count duplicate packets per Hung/Rachel request
             */
            mac_stats_g.rx_pkts_count--;
            mac_stats_g.rx_bytes_count -= mac_parse_data.mpdu_length;
#endif
            /*
             * This is a duplicated frame.
             * It will not be indicated to the next higher layer,
             * but nevetheless the frame pending bit needs to be
             * checked and acted upon.
             */
            bmm_buffer_free(buf_ptr);
        } else {
            /* Generate data indication to next higher layer. */

            /* Store required information for perform subsequent
             * duplicate detections.
             */
            mac_last_dsn = mdi_p->DSN;
            mac_last_src_addr = mdi_p->SrcAddr;

            /* Destination address info */
            mdi_p->DstAddrMode = mac_parse_data.dest_addr_mode;
            /*
             * Setting the address to zero is required for a short address
             * and in case no address is included. Therefore the address
             * is first always set to zero to reduce code size.
             */
            EXT_ADDR_CLEAR(mdi_p->DstAddr);
            /*
             * Setting the PAN-Id to the Destiantion PAN-Id is required
             * for a both short and long address, but not in case no address
             * is included. Therefore the PAN-ID is first always set to
             * the Destination PAN-IDto reduce code size.
             */
            mdi_p->DstPANId = mac_parse_data.dest_panid;
            if (FCF_LONG_ADDR == mdi_p->DstAddrMode) {
                ADDR_COPY_DST_SRC_64(mdi_p->DstAddr,
                                     mac_parse_data.dest_addr.long_address);
            } else if (FCF_SHORT_ADDR == mdi_p->DstAddrMode) {
                ADDR_COPY_DST_SRC_16(mdi_p->DstAddr.lo_u32,
                                     mac_parse_data.dest_addr.short_address);
            } else {
                /*
                 * Even if the Destination address mode is zero,
                 * and the destination address information is ís not present,
                 * the values are cleared to prevent the providing of
                 * trash information.
                 * The Desintation address was already cleared above.
                 */
                mdi_p->DstPANId = 0;
            }

            mdi_p->mpduLinkQuality = mac_parse_data.ppdu_link_quality;

            mdi_p->Security.SecurityLevel = mac_parse_data.sec_ctrl.sec_level;
            mdi_p->Security.KeyIdMode = mac_parse_data.sec_ctrl.key_id_mode;
            if (mac_parse_data.sec_ctrl.key_id_mode == 1) {
                mdi_p->Security.KeyIndex = mac_parse_data.key_id[0];
            } else if (mac_parse_data.sec_ctrl.key_id_mode == 2) {
                memcpy(mdi_p->Security.KeySource, mac_parse_data.key_id,
                       4);
                mdi_p->Security.KeyIndex = mac_parse_data.key_id[4];
            } else if (mac_parse_data.sec_ctrl.key_id_mode == 3) {
                memcpy(mdi_p->Security.KeySource, mac_parse_data.key_id,
                       8);
                mdi_p->Security.KeyIndex = mac_parse_data.key_id[8];
            }

            mdi_p->msduLength = mac_parse_data.mac_payload_length;

            /* Set pointer to data frame payload. */
            mdi_p->msdu_p =
                mac_parse_data.mac_payload_data.payload_data.payload_p;

            mdi_p->cmdcode = MCPS_DATA_INDICATION;

            mcps_data_ind(buf_ptr);

        }


        /* 
         * Continue with checking the frame pending bit in the received
         * data frame.
         */
        if (mac_parse_data.fcf & FCF_FRAME_PENDING) {
            /* An node that is not PAN coordinator may poll for pending data. */
            if (MAC_PAN_COORD_STARTED != mac_state) {
                address_field_t src_addr;

                /* Build command frame due to implicit poll request */
                /*
                 * No explicit destination address attached, so use current
                 * values of PIB attributes macCoordShortAddress or
                 * macCoordExtendedAddress.
                 */
                /*
                 * This implicit poll (i.e. corresponding data request
                 * frame) is to be sent to the same node that we have received
                 * this data frame. Therefore the source address information
                 * from this data frame needs to be extracted, and used for the
                 * data request frame appropriately.
                 * Use this as destination address expclitily and
                 * feed this to the function mac_build_and_tx_data_req
                 */
                if (FCF_SHORT_ADDR == mac_parse_data.src_addr_mode) {
                    ADDR_COPY_DST_SRC_16(src_addr.short_address,
                                         mac_parse_data.src_addr.short_address);

                    mac_data_build_and_tx_data_req(false, false, FCF_SHORT_ADDR,
                                                (address_field_t *)&(src_addr),
                                                mac_parse_data.src_panid);
                } else if (FCF_LONG_ADDR == mac_parse_data.src_addr_mode) {
                    ADDR_COPY_DST_SRC_64(src_addr.long_address,
                                         mac_parse_data.src_addr.long_address);

                    mac_data_build_and_tx_data_req(false, false, FCF_LONG_ADDR,
                                                 (address_field_t *)&(src_addr),
                                                   mac_parse_data.src_panid);
                } else {
                    mac_data_build_and_tx_data_req(false, false, 0, NULL, 0);
                }
            }
        } else {
            /* Frame pending but was not set, so no further action required. */
            /* Set radio to sleep if allowed */
            mac_trx_sleep();
        }   /* if (mac_parse_data.fcf & FCF_FRAME_PENDING) */
    }   /* (mac_parse_data.payload_length == 0) */
}


/*
 * Start the Persistence timer for indirect data
 *
 * This function starts the persistence timer for handling of indirect
 * data.
 */
void mac_start_persistence_timer (void)
{
    retval_t status = MAC_FAILURE;
    uint32_t persistence_int_us;
    uint8_t bo_for_persistence_tmr;
#ifdef RTX51_TINY_OS
    eStatus timer_status;
#endif

    if (hal_pib_BeaconOrder == NON_BEACON_NWK) {
        /*
         * The timeout interval for the indirect data persistence timer is
         * based on the define below and is the same as for a nonbeacon build.
         */
        bo_for_persistence_tmr = BO_USED_FOR_MAC_PERS_TIME;
    } else {
        /*
         * The timeout interval for the indirect data persistence timer is
         * based on the current beacon order.
         */
        bo_for_persistence_tmr = hal_pib_BeaconOrder;
    }

    persistence_int_us =
        HAL_CONVERT_SYMBOLS_TO_US(
            HAL_GET_BEACON_INTERVAL_TIME(bo_for_persistence_tmr)) / 1000;

    if (indirect_data_q.size > 0) {
        mac_hal_frame_pending(true);
    }

    /* Start the indirect data persistence timer now. */
#ifdef RTX51_TINY_OS
    timer_status = STM_StartTimer(indirect_data_persistence_timer,
                                  persistence_int_us);
#endif
}

/*
 * This function generates the confirmation for those indirect data buffers
 * whose persistence time has reduced to zero.
 *
 * @param buf_ptr Pointer to buffer of indirect data whose persistance time
 * has reduced to zero
 */
static void handle_exp_persistence_timer (buffer_t *buf_ptr)
{
    frame_info_t *tx_frame_p = (frame_info_t *)BMM_BUFFER_POINTER(buf_ptr);

    /*
     * The frame should never be in transmission while this function
     * is called.
     */
    switch (tx_frame_p->msg_type) {
    case ASSOCIATIONRESPONSE:
        mac_mlme_comm_status(MAC_TRANSACTION_EXPIRED, buf_ptr);
        break;

    case DISASSOCIATIONNOTIFICATION:
        /*
         * Prepare disassociation confirm message after transmission of
         * the disassociation notification frame.
         */
        mac_prep_disassoc_conf(buf_ptr, MAC_TRANSACTION_EXPIRED);
        break;

    case MCPS_MESSAGE:
        mac_gen_mcps_data_conf(buf_ptr, (uint8_t)MAC_TRANSACTION_EXPIRED,
                               tx_frame_p->msduHandle, 0);
        break;

    default:
        /* Nothing to be done here. */
        break;
    }
}

/*
 *
 * Handles the decrement of the persistance time of each indirect data frame
 * in the indirect queue.
 * If the persistance time of any indirect data reduces to zero, the
 * frame is removed from the indirect queue and
 * a confirmation for that indirect data is sent with the status
 * transaction expired.
 */
static void handle_persistence_time_decrement (void)
{
    search_t find_buf;
    buffer_t *buffer_persistent_zero = NULL;

    /*
     * This callback function traverses through the indirect queue and
     * decrements the persistence time for each data frame.
     */
#ifdef CALLBACK
    find_buf.compare_func = decrement_persistence_time;
#else
    find_buf.compare_func_id = MAC_DECREMENT_PERSI_TIME;
#endif

    /*
     * At the end of this function call (qmm_queue_read), the indirect data
     * will be updated with the decremented persistence time.
     */
    qmm_queue_read(&indirect_data_q, &find_buf);

    /*
     * Once we have updated the persistence timer, any frame with a persistence
     * time of zero needs to be removed from the indirect queue.
     */

    /*
     * This callback function traverses through the indirect queue and
     * searches for a data frame with persistence time equal to zero.
     */
#ifdef CALLBACK
    find_buf.compare_func = check_persistence_time_zero;
#else
    find_buf.compare_func_id = MAC_CHECK_PERSI_TIME_ZERO;
#endif

    do {
        buffer_persistent_zero = qmm_queue_remove(&indirect_data_q, &find_buf);

        if (NULL != buffer_persistent_zero) {
            handle_exp_persistence_timer(buffer_persistent_zero);
        }
    } while (NULL != buffer_persistent_zero);
    
    mac_hal_frame_pending(false); /* Tell h/w no more frame pending */
}

/*
 * @brief Handles timeout of indirect data persistence timer
 *
 * This function is a callback function of the timer started for checking
 * the mac persistence time of indirect data in the queue.
 *
 * @param callback_parameter Callback parameter
 */
void mac_persistence_timer_cb (void *callback_parameter)
{
    /* Decrement the persistence time for indirect data. */
    handle_persistence_time_decrement();

    if (indirect_data_q.size > 0) {
        /* Restart persistence timer. */
        mac_start_persistence_timer();
    }

    callback_parameter = callback_parameter; /* Keep compiler happy. */
}

/*
 *
 * buf_ptr Pointer to the indirect data in the indirect queue
 * handle Callback parameter
 *
 * return FALSE to traverse through the full indirect queue
 *
 */
bool decrement_persistence_time (void *buf_ptr, void *handle)
{
    frame_info_t *frame_p = (frame_info_t *)buf_ptr;

    /*
     * In case the frame is currently in the process of being transmitted,
     * the persistence time is not decremented, to avoid the expiration of
     * the persistence timer during transmission.
     * Once the transmission is done (and was not successful),
     * the frame will still be in the indirect queue and the persistence
     * timer will be decremented again.
     */
    if (!frame_p->indirect_in_transit) {
        /* Decrement the persistence time for this indirect data frame. */
        frame_p->persistence_time--;
    }

    handle = handle;    /* Keep compiler happy. */

    return (false);
}

/*
 *
 * This callback function checks whether the persistence time
 * of the indirect data is set to zero.
 *
 * buf_p - Pointer to indirect data buffer
 *
 * @return 1 if extended address passed matches with the destination
 * address of the indirect frame , 0 otherwise
 */
bool check_persistence_time_zero (void *buf_ptr, void *handle)
{
    frame_info_t *frame = (frame_info_t *)buf_ptr;

    /* Frame shall not be in transmission. */
    if (!frame->indirect_in_transit) {
        if (frame->persistence_time == 0) {
            return (true);
        }
    }

    handle = handle;    /* Keep compiler happy. */

    return (false);
}

