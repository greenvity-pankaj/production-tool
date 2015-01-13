/**
 * @file mac_data.c
 *
 * Implements incoming frame handling in the MAC
 *
 * $Id: mac_data.c,v 1.4 2014/06/09 13:19:46 kiran Exp $
 *
 * Copyright (c) 2011, Greenvity Communication All rights reserved.
 *
 */

/* === Includes ============================================================ */
#ifdef RTX51_TINY_OS
#include <rtx51tny.h>
#include "hybrii_tasks.h"
#endif
#include <stdio.h>
#include <string.h>
#include "papdef.h"
#include "return_val.h"
#include "bmm.h"
#include "qmm.h"
#include "utils.h"
#include "mac_const.h"
#include "mac_msgs.h"
#include "mac_hal.h"
#include "mac_api.h"
#include "mac_data_structures.h"
#include "mac_internal.h"
#include "mac_security.h"
#include "mac.h"
#include "timer.h"
#include "stm.h"

/*
 * Mask of the GTS descriptor counter
 */
#define GTS_DESCRIPTOR_COUNTER_MASK     (0x07)

/*
 * Extract the PAN Coordinator bit from the Superframe Spec.
 */
#define GET_PAN_COORDINATOR(spec)      (((spec) & 0x4000) >> 14)

/*
 * PAN-Id conflict notification payload length
 */
#define PAN_ID_CONFLICT_PAYLOAD_LEN     (1)

/*
 * Data request payload length
 */
#define DATA_REQ_PAYLOAD_LEN            (1)

/* === Implementation ====================================================== */

/*
 * Constructs a null data frame
 *
 * return Pointer to the created null data frame, NULL otherwise.
 */
static buffer_t * mac_data_build_null_frame (void)
{
    bool         use_long_addr_dest;
    frame_info_t *tx_frame_p;
    uint8_t      frame_len;
    uint8_t      *frame_ptr;
    uint16_t     fcf = 0;
    buffer_t     *buf_ptr = bmm_buffer_alloc(BUFFER_SIZE);

    if (NULL == buf_ptr) {
        return NULL;
    }

    tx_frame_p = (frame_info_t *)BMM_BUFFER_POINTER(buf_ptr);

    /* No data payload, this is a null packet.*/
    tx_frame_p->msg_type = NULL_FRAME;
    tx_frame_p->buffer_header_p = buf_ptr;

    /* No indirect transmission. */
    tx_frame_p->indirect_in_transit = false;

    /* Update the payload length. */
    frame_len = SHORT_ADDR_LEN +  // short Destination Address
                PAN_ID_LEN     +  // Destination PAN-Id
                SHORT_ADDR_LEN +  // short Source Address
                SEQ_NUM_LEN    +  // Sequence number
                FCF_LEN;          // FCF

    /* Get the payload pointer. */
    frame_ptr = (uint8_t *)tx_frame_p + BUFFER_SIZE;

    /*
     * Set Source Address.
     */
    if ((BROADCAST == hal_pib_ShortAddress) ||
        (MAC_NO_SHORT_ADDR_VALUE == hal_pib_ShortAddress)) {
        /* Use long address as source address. */
        frame_ptr -= EXT_ADDR_LEN;
        frame_len += 6;  /* Already add 2 bytes for short addr above */
        mac_utils_64_bit_to_byte_array(hal_pib_IeeeAddress, frame_ptr);
        fcf |= FCF_SET_SOURCE_ADDR_MODE(FCF_LONG_ADDR);
    } else {
        /* Use short address as source address. */
        frame_ptr -= SHORT_ADDR_LEN;
        mac_utils_16_bit_to_byte_array(hal_pib_ShortAddress, frame_ptr);
        fcf |= FCF_SET_SOURCE_ADDR_MODE(FCF_SHORT_ADDR);
    }

    /* Shall the Intra-PAN bit set? */
    if (hal_pib_PANId == mac_parse_data.src_panid) {
        /*
         * Both PAN-Ids are identical.
         * Set intra-PAN bit.
         */
        fcf |= FCF_PAN_ID_COMPRESSION;
    } else {
        /* Set Source PAN-Id. */
        frame_ptr -= PAN_ID_LEN;
        frame_len += PAN_ID_LEN;
        mac_utils_16_bit_to_byte_array(hal_pib_PANId, frame_ptr);
    }

    /* Set Destination Address. */
    use_long_addr_dest = (FCF_LONG_ADDR == mac_parse_data.src_addr_mode);

    /* Destination address is set from source address of received frame. */
    if (use_long_addr_dest) {
        frame_ptr -= EXT_ADDR_LEN;
        frame_len += 6; /* Already add 2 bytes for short addr above */
        mac_utils_64_bit_to_byte_array(mac_parse_data.src_addr.long_address,
                                       frame_ptr);

        fcf |= FCF_SET_DEST_ADDR_MODE(FCF_LONG_ADDR) |
               FCF_SET_FRAMETYPE(FCF_FRAMETYPE_DATA);
    } else {
        frame_ptr -= SHORT_ADDR_LEN;
        mac_utils_16_bit_to_byte_array(mac_parse_data.src_addr.short_address,
                                       frame_ptr);
        fcf |= FCF_SET_DEST_ADDR_MODE(FCF_SHORT_ADDR) |
              FCF_SET_FRAMETYPE(FCF_FRAMETYPE_DATA);
    }

    /* Destination PANId is set from source PANId of received frame. */
    frame_ptr -= PAN_ID_LEN;
    mac_utils_16_bit_to_byte_array(mac_parse_data.src_panid, frame_ptr);

    /* Set DSN. */
    frame_ptr -= SEQ_NUM_LEN;
    *frame_ptr = mac_pib_macDSN++;


    /* Set the FCF. */
    frame_ptr -= FCF_LEN;
    mac_utils_16_bit_to_byte_array(fcf, frame_ptr);


    /* First element shall be length of PHY frame. */
    frame_ptr -= LENGTH_FIELD_LEN;
    *frame_ptr = frame_len;

    /* Finished building of frame. */
    tx_frame_p->mpdu_p = frame_ptr;

    return buf_ptr;
}

/*
 * This function creates and transmits a Null Data frame in case the
 * coordinator does not have pending data to be transmitted.
 */
void mac_data_tx_null_frame (void)
{
    frame_info_t *tx_frame_p;
    retval_t     tx_status;
    buffer_t     *buf_ptr;

    /*
     * No matching pending item in the queue,
     * so a Null Data frame is created.
     */
    buf_ptr = mac_data_build_null_frame();

    if (NULL != buf_ptr) {
        tx_frame_p = (frame_info_t *)BMM_BUFFER_POINTER(buf_ptr);

        /*
         * Transmission should be done with CSMA-CA or
         * quickly after the ACK of the data request command.
         * Here it's done quickly after the ACK w/o CSMA.
         */
        tx_status = mac_hal_tx_frame(tx_frame_p, NO_CSMA, FALSE);

        if (MAC_SUCCESS == tx_status) {
            MAC_BUSY();
        } else {
            /*
             * Transmission to HAL failed, free up the buffer used to create
             * Null Data frame.
             */
            bmm_buffer_free(buf_ptr);
        }
    }
}

/*
 *
 * This callback function checks whether the passed short address
 * matches with the frame in the queue.
 *
 * buf_p      - Pointer to indirect data buffer
 * short_addr - Short address to be searched
 *
 * return true if short address passed matches with the destination
 * address of the indirect frame , false otherwise
 */
bool mac_data_find_short_addr_buffer (void *buf_p, void *short_addr_p)
{
    uint8_t dst_addr_mode;
    frame_info_t *data_p = (frame_info_t *)buf_p;

    if (data_p->indirect_in_transit == FALSE) {
        /*
         * Read octet 2 of Frame Control Field containing the type
         * of destination address.
         */
        dst_addr_mode = (data_p->mpdu_p[PL_POS_FCF_2] >>
                         FCF_2_DEST_ADDR_OFFSET) & FCF_ADDR_MASK;

        /*
         * Compare indirect data frame's dest_address(short)
         * with short address passed.
         */
        if ((dst_addr_mode == FCF_SHORT_ADDR) &&
            (*(uint16_t *)short_addr_p == mac_utils_byte_array_to_16_bit(
             &data_p->mpdu_p[PL_POS_DST_ADDR_START]))) {
            return (true);
        }
    }

    return (false);
}

/*
 *
 * This callback function checks whether the passed short address
 * matches with the frame in the queue.
 *
 * buf_p       - Pointer to indirect data buffer
 * long_addr_p - Extended address to be searched
 *
 * return true if extended address passed matches with the destination
 * address of the indirect frame, false otherwise
 */
bool mac_data_find_long_addr_buffer (void *buf_p, void *long_addr_p)
{
    uint8_t dst_addr_mode;
    frame_info_t *data_p = (frame_info_t *)buf_p;

    if (data_p->indirect_in_transit == FALSE) {
        /*
         * Read octet 2 of Frame Control Field containing the type
         * of destination address.
         */
        dst_addr_mode = (data_p->mpdu_p[PL_POS_FCF_2] >>
                         FCF_2_DEST_ADDR_OFFSET) & FCF_ADDR_MASK;

        /*
         * Compare indirect data frame's dest_address(extended)
         * with the exended address passed.
         */
        if (dst_addr_mode == FCF_LONG_ADDR) {
            uint64_t addr_in, addr_indirect;

			addr_in = *(uint64_t *)long_addr_p;
			addr_indirect = 
			    mac_utils_byte_array_to_64_bit(
                    &data_p->mpdu_p[PL_POS_DST_ADDR_START]);
            if (EXT_ADDR_MATCH(addr_in, addr_indirect)) {
                return (true);
            }
        }
    }

    return (false);
}

/**
 *
 * This function processes a received data request command frame
 * at the coordinator, searches for pending indirect data frames
 * for the originator and initiates the frame transmission of the
 * data frame with CSMA-CA.
 *
 * buf_p - Frame reception buffer pointer
 */
void mac_data_process_data_request (buffer_t *buf_p)
{
    /* Buffer pointer to next indirect data frame to be transmitted. */
    buffer_t     *buf_next_data_p;
    search_t     find_buf;
    frame_info_t *tx_frame_p;
    retval_t     tx_status;

    /* Free the buffer of the received frame. */
    bmm_buffer_free(buf_p);

    /* Ignore data request if we are not PAN coordinator or coordinator. */
    if ((MAC_IDLE == mac_state) ||
        (MAC_ASSOCIATED == mac_state)) {
        return;
    }

    /* Check the addressing mode */
    if (mac_parse_data.src_addr_mode == FCF_SHORT_ADDR) {
        /*
         * Look for pending data in the indirect queue for
         * this short address.
         */

        /*
         * Assign the function pointer for searching the
         * data having address of the requested device.
         */
#ifdef CALLBACK
        find_buf.compare_func = mac_data_find_short_addr_buffer;
#else
        find_buf.compare_func_id = MAC_DATA_FIND_SHORT_ADDR;
#endif

        /* Update the short address to be searched. */
        find_buf.handle = &mac_parse_data.src_addr.short_address;
    } else if (mac_parse_data.src_addr_mode == FCF_LONG_ADDR) {
        /*
         * Look for pending data in the indirect queue for
         * this long address.
         */

        /*
         * Assign the function pointer for searching the
         * data having address of the requested device.
         */
#ifdef CALLBACK
        find_buf.compare_func = mac_data_find_long_addr_buffer;
#else
        find_buf.compare_func_id = MAC_DATA_FIND_EXT_ADDR;
#endif

        /* Update the long address to be searched. */
        find_buf.handle = &mac_parse_data.src_addr.long_address;
    } else {
        return;
    }

    /*
     * Read from the indirect queue. The removal of items from this queue
     * will be done after successful transmission of the frame.
     */
    buf_next_data_p = qmm_queue_read(&indirect_data_q, &find_buf);
    /* Note: The find_buf structure is reused below, so do not change this. */

    if (NULL == buf_next_data_p) {
        mac_data_tx_null_frame();
        return;
    } else {
        /* Indirect data found and to be sent. */
        tx_frame_p = (frame_info_t *)BMM_BUFFER_POINTER(buf_next_data_p);

        /*
         * We need to check whether the source PAN-Id of the previously
         * received data request frame is identical to the destination PAN-Id
         * of the pending frame. If not the frame shall not be transmitted,
         * but a Null Data frame instead.
         */
        if (mac_parse_data.src_panid !=
            mac_utils_byte_array_to_16_bit(
                &tx_frame_p->mpdu_p[PL_POS_DST_PAN_ID_START])) {
            mac_data_tx_null_frame();
            return;
        } else {
            /*
             * The frame to be transmitted next is marked.
             * This is necessary since the queue needs to be traversed again
             * to find other pending indirect data frames for this particular
             * recipient.
             */
            tx_frame_p->indirect_in_transit = true;
            tx_frame_p->buffer_header_p = buf_next_data_p;

            /*
             * Go through the indirect data queue to find out the frame
             * pending for the device which has requested for the data.
             */
            /*
             * Since the buffer header has already been stored in
             * transmit_frame->buffer_header, it can be reused here for
             * other purpose.
             */
            /*
             * It is assumed that the find_buf struct does still have
             * the original values from above.
             */
            buf_next_data_p = qmm_queue_read(&indirect_data_q, &find_buf);
            /*
             * Check whether there is another indirect data available
             * for the same recipient.
             */
            if (NULL != buf_next_data_p) {
                tx_frame_p->mpdu_p[PL_POS_FCF_1] |= FCF_FRAME_PENDING;
            }
        }

        /*
         * Transmission should be done with CSMA-CA or
         * quickly after the ACK of the data request command.
         * Here it's done quickly after the ACK w/o CSMA.
         */
        tx_status = mac_hal_tx_frame(tx_frame_p, NO_CSMA, FALSE);

        if (MAC_SUCCESS == tx_status) {
            MAC_BUSY();
        } else {
            /*
             * TAL rejects frame transmission, since it's too close to the next
             * beacon transmission. The frame is kept in the indirect queue.
             */
            tx_frame_p->indirect_in_transit = false;
        }
    }
}

static void mac_data_promis_mode_rx_frame (buffer_t *b_ptr,
                                           frame_info_t *f_ptr)
{
    mcps_data_ind_t *mdi = (mcps_data_ind_t *)BMM_BUFFER_POINTER(b_ptr);

    /*
     * In promiscous mode the MCPS_DATA.indication is used as container
     * for the received frame.
     */


    /*
     * Since both f_ptr and mdi point to the same data storage place,
     * we need to save all required data first.
     * The time stamp has already been saved into .
     * So lets save the payload now.
     */

    /* Set payload pointer to MPDU of received frame. */
    mdi->msduLength = f_ptr->mpdu_p[0];
    mdi->msdu_p = &f_ptr->mpdu_p[1];
    

    /* Build the MLME_Data_indication parameters */
    mdi->DSN = 0;
    mdi->Timestamp = f_ptr->time_stamp;

    /* Source address mode is 0. */
    mdi->SrcAddrMode = FCF_NO_ADDR;
    mdi->SrcPANId = 0;
    mdi->SrcAddr.lo_u32 = 0;
    mdi->SrcAddr.hi_u32 = 0;
									
    /* Destination address mode is 0.*/
    mdi->DstAddrMode = FCF_NO_ADDR;
    mdi->DstPANId = 0;
    mdi->DstAddr.lo_u32 = 0;
    mdi->DstAddr.hi_u32 = 0;

    mdi->mpduLinkQuality = mac_parse_data.ppdu_link_quality;
    mdi->cmdcode = MCPS_DATA_INDICATION;

    mcps_data_ind(b_ptr);
}

/*
 * Helper function to extract the complete address information
 *        of the received frame
 *
 * frame_ptr - Pointer to first octet of Addressing fields of received frame
 *        (See IEEE 802.15.4-2006 Figure 41)
 *
 * return:
 * Length of Addressing fields
 */
static uint8_t mac_data_extract_mhr_addr_info (uint8_t *frame_ptr)
{
    uint16_t fcf = mac_parse_data.fcf;
    uint8_t src_addr_mode = (fcf >> FCF_SOURCE_ADDR_OFFSET) & FCF_ADDR_MASK;
    uint8_t dst_addr_mode = (fcf >> FCF_DEST_ADDR_OFFSET) & FCF_ADDR_MASK;
    bool intra_pan = fcf & FCF_PAN_ID_COMPRESSION;
    uint8_t addr_field_len = 0;

    if (dst_addr_mode != 0) {
        mac_parse_data.dest_panid = 
                            mac_utils_byte_array_to_16_bit(frame_ptr);
        frame_ptr += PAN_ID_LEN;
        addr_field_len += PAN_ID_LEN;

        if (FCF_SHORT_ADDR == dst_addr_mode) {
            /*
             * First initialize the complete long address with zero, since
             * later only 16 bit are actually written.
             */
            mac_parse_data.dest_addr.long_address.lo_u32 = 0;
            mac_parse_data.dest_addr.long_address.hi_u32 = 0;
            mac_parse_data.dest_addr.short_address = 
                              mac_utils_byte_array_to_16_bit(frame_ptr);
            frame_ptr += SHORT_ADDR_LEN;
            addr_field_len += SHORT_ADDR_LEN;
        } else if (FCF_LONG_ADDR == dst_addr_mode) {
            mac_parse_data.dest_addr.long_address = 
                              mac_utils_byte_array_to_64_bit(frame_ptr);
            frame_ptr += EXT_ADDR_LEN;
            addr_field_len += EXT_ADDR_LEN;
        }
    }

    if (src_addr_mode != 0) {
        if (!intra_pan) {
            /*
             * Source PAN ID is present in the frame only if the intra-PAN bit
             * is zero and src_addr_mode is non zero.
             */
            mac_parse_data.src_panid =
                              mac_utils_byte_array_to_16_bit(frame_ptr);
            frame_ptr += PAN_ID_LEN;
            addr_field_len += PAN_ID_LEN;
        } else {
            /*
             * The received frame does not contain a source PAN ID, hence
             * source PAN ID is updated with the destination PAN ID.
             */
            mac_parse_data.src_panid = mac_parse_data.dest_panid;
        }

        /* The source address is updated. */
        if (FCF_SHORT_ADDR == src_addr_mode) {
            /*
             * First initialize the complete long address with zero, since
             * later only 16 bit are actually written.
             */
            mac_parse_data.src_addr.long_address.lo_u32 = 0;
            mac_parse_data.src_addr.long_address.hi_u32 = 0;
            mac_parse_data.src_addr.short_address = 
                                  mac_utils_byte_array_to_16_bit(frame_ptr);
            frame_ptr += SHORT_ADDR_LEN;
            addr_field_len += SHORT_ADDR_LEN;
        } else if (FCF_LONG_ADDR == src_addr_mode) {
	    mac_parse_data.src_addr.long_address = 
                                  mac_utils_byte_array_to_64_bit(frame_ptr);
            frame_ptr += EXT_ADDR_LEN;
            addr_field_len += EXT_ADDR_LEN;
        }
    }

    /*
     * The length of the Addressing Field is known, so the length of the
     * MAC payload can be calcluated.
     * The actual MAC payload length is calculated from
     * the length of the mpdu minus 2 octets FCS, minus 1 octet sequence
     * number, minus the length of the addressing fields, minus 2 octet FCS.
     */
    mac_parse_data.mac_payload_length = mac_parse_data.mpdu_length -
                                        FCF_LEN -
                                        SEQ_NUM_LEN -
                                        addr_field_len -
                                        FCS_LEN;

    mac_parse_data.src_addr_mode = src_addr_mode;
    mac_parse_data.dest_addr_mode = dst_addr_mode;

    return (addr_field_len);
}

static void mac_data_parse_cmd (uint8_t  *temp_frame_ptr,
                                uint8_t  payload_index,
                                uint16_t fcf)
{
    payload_index = 1;

    switch (mac_parse_data.mac_command) {
    case ASSOCIATIONREQUEST:
        mac_parse_data.mac_payload_data.assoc_req_data.capability_info =
                 temp_frame_ptr[payload_index++];
        break;

    case ASSOCIATIONRESPONSE:
        memcpy(&mac_parse_data.mac_payload_data.assoc_response_data.short_addr,
               &temp_frame_ptr[payload_index], sizeof(uint16_t));
        payload_index += sizeof(uint16_t);
        mac_parse_data.mac_payload_data.assoc_response_data.assoc_status =
               temp_frame_ptr[payload_index];
        break;

    case DISASSOCIATIONNOTIFICATION:
        mac_parse_data.mac_payload_data.disassoc_req_data.disassoc_reason =
               temp_frame_ptr[payload_index++];
        break;

    case COORDINATORREALIGNMENT:
        memcpy(&mac_parse_data.mac_payload_data.coord_realign_data.pan_id,
               &temp_frame_ptr[payload_index], sizeof(uint16_t));
        payload_index += sizeof(uint16_t);
        memcpy(&mac_parse_data.mac_payload_data.coord_realign_data.\
               coord_short_addr, &temp_frame_ptr[payload_index],
               sizeof(uint16_t));
        payload_index += sizeof(uint16_t);

        mac_parse_data.mac_payload_data.coord_realign_data.logical_channel =
                    temp_frame_ptr[payload_index++];

        memcpy(&mac_parse_data.mac_payload_data.coord_realign_data.short_addr,
               &temp_frame_ptr[payload_index], sizeof(uint16_t));
        payload_index += sizeof(uint16_t);

        /*
         * If frame version subfield indicates a 802.15.4-2006 compatible 
         * frame, the channel page is appended as additional information
         *  element.
         */
        if (fcf & FCF_FRAME_VERSION_2006) {
            mac_parse_data.mac_payload_data.coord_realign_data.channel_page =
                  temp_frame_ptr[payload_index++];
        }
        break;

    case ORPHANNOTIFICATION:
    case DATAREQUEST:
    case BEACONREQUEST:
    case PANIDCONFLICTNOTIFICAION:
        break;

    default:
        return;
    }
}

static void mac_data_parse_data (uint8_t *temp_frame_ptr,
                                 uint8_t payload_index)
{
    if (mac_parse_data.mac_payload_length) {
        /*
         * In case the device got a frame with a corrupted payload
         * length
         */
        if (mac_parse_data.mac_payload_length >= aMaxMACPayloadSize) {
            mac_parse_data.mac_payload_length = aMaxMACPayloadSize;
        }

        /*
         * Copy the pointer to the data frame payload for
         * further processing later.
         */
        mac_parse_data.mac_payload_data.payload_data.payload_p = 
                                               &temp_frame_ptr[payload_index];
    } else {
        mac_parse_data.mac_payload_length = 0;
    }
}

static void mac_data_parse_beacon (uint8_t *temp_frame_ptr,
                                   uint8_t payload_index)
{
    uint8_t temp_byte;
    uint8_t number_bytes_short_addr;
    uint8_t number_bytes_long_addr;

    /* Get the Superframe specification */
    mac_parse_data.mac_payload_data.beacon_data.superframe_spec =
        mac_utils_byte_array_to_16_bit(&temp_frame_ptr[payload_index]);
    payload_index += sizeof(uint16_t);

    /* Get the GTS specification */
    mac_parse_data.mac_payload_data.beacon_data.gts_spec = 
                                             temp_frame_ptr[payload_index++];

    /*
     * If the GTS specification descriptor count is > 0, then
     * increase the index by the correct GTS field octet number
     * GTS directions and GTS address list will not be parsed
     */
    temp_byte = (mac_parse_data.mac_payload_data.beacon_data.gts_spec &
                 GTS_DESCRIPTOR_COUNTER_MASK);
    if (temp_byte > 0) {
        /* 1 octet GTS diresctions + GTS address list */
        payload_index += 1 + temp_byte;
    }

    /* Get the Pending address specification */
    mac_parse_data.mac_payload_data.beacon_data.pending_addr_spec =
                temp_frame_ptr[payload_index++];
        /*
         * If the Pending address specification indicates that the number of
         * short or long addresses is > 0, then get the short and/or
         * long addresses
         */
    number_bytes_short_addr = NUM_SHORT_PEND_ADDR(
                mac_parse_data.mac_payload_data.beacon_data.pending_addr_spec);
    number_bytes_long_addr = NUM_LONG_PEND_ADDR(
                mac_parse_data.mac_payload_data.beacon_data.pending_addr_spec);

    if ((number_bytes_short_addr) || (number_bytes_long_addr)) {
        mac_parse_data.mac_payload_data.beacon_data.pending_addr_list_p =
                            &temp_frame_ptr[payload_index];
    }

    if (number_bytes_short_addr) {
        payload_index += (number_bytes_short_addr * sizeof(uint16_t));
    }

    if (number_bytes_long_addr) {
        payload_index += (number_bytes_long_addr * sizeof(uint64_t));
    }

        /* Is there a beacon payload ? */
    if (mac_parse_data.mac_payload_length > payload_index) {
        mac_parse_data.mac_payload_data.beacon_data.beacon_payload_len =
                             mac_parse_data.mac_payload_length - payload_index;

        /* Store pointer to received beacon payload. */
        mac_parse_data.mac_payload_data.beacon_data.beacon_payload_p =
                                &temp_frame_ptr[payload_index];

    } else {
        mac_parse_data.mac_payload_data.beacon_data.beacon_payload_len = 0;
    }
}

/*
 * Parse an MPDU
 *
 * This function parses an MPDU which got from data_indication,
 * and leaves the parse result in mac_parse_data.
 *
 * rx_frame_ptr - Pointer to frame received from TAL
 *
 * return:
 * TRUE - frame OK
 * FALSE- frame is invalid.
 */
static bool mac_data_parse_mpdu (frame_info_t *rx_frame_ptr)
{
    uint8_t     payload_index;
    uint16_t    fcf;
    uint8_t     *temp_frame_ptr = &(rx_frame_ptr->mpdu_p[1]);
    /* temp_frame_ptr points now to first octet of FCF. */

    /* Extract the FCF. */
    fcf = mac_utils_byte_array_to_16_bit(temp_frame_ptr);
    mac_parse_data.fcf = fcf;
    temp_frame_ptr += 2;

    /* Extract the Sequence Number. */
    mac_parse_data.sequence_number = *temp_frame_ptr++;

    /* Extract the complete address information from the MHR. */
    temp_frame_ptr += mac_data_extract_mhr_addr_info(temp_frame_ptr);
    /*
     * Note: temp_frame_ptr points now to the Auxiliry Security Header or
     * the first octet of the MAC payload if available.
     */

    mac_parse_data.frame_type = FCF_GET_FRAMETYPE(fcf);

    if (FCF_FRAMETYPE_MAC_CMD == mac_parse_data.frame_type) {
        mac_parse_data.mac_command = *temp_frame_ptr;
    }

    payload_index = 0;

    /* The timestamping is only required for beaconing networks. */
    mac_parse_data.time_stamp = rx_frame_ptr->time_stamp;

    if (fcf & FCF_SECURITY_ENABLED) {
        retval_t status;
        status = mac_unsecure(&mac_parse_data, rx_frame_ptr->mpdu_p, 
                              temp_frame_ptr, &payload_index);

        if (status != MAC_SUCCESS) {
            mlme_comm_status_ind_t *csi_p;

            /* Generate MLME-COMM-STATUS.indication. */
            /*
             * In order to not interfere with the regular flow of parsing the
             * frame and buffer handling, a fresh buffer is seized for the
             * MLME-COMM-STATUS.indication, which will be release at the API
             * level.
             */
            buffer_t *buffer_header = bmm_buffer_alloc(BUFFER_SIZE);

            if (NULL == buffer_header) {
                /* Buffer is not available */
                return (false);
            }

            /* Get the buffer body from buffer header */
            csi_p = 
                    (mlme_comm_status_ind_t*)BMM_BUFFER_POINTER(buffer_header);

            csi_p->cmdcode = MLME_COMM_STATUS_INDICATION;

            csi_p->PANId = hal_pib_PANId;

            csi_p->SrcAddrMode = mac_parse_data.src_addr_mode;
            csi_p->SrcAddr = mac_parse_data.src_addr.long_address;

            csi_p->DstAddrMode = mac_parse_data.dest_addr_mode;
            csi_p->DstAddr = mac_parse_data.dest_addr.long_address;

            csi_p->status = status;

            mlme_comm_status_ind(buffer_header);

            /*
             * Return false - this will lead to the release of the original 
             * buffer.
             */
            return (false);
        }
    } else {
        // 7.5.8.2.3 a)
        mac_parse_data.sec_ctrl.sec_level = 0;
    }

    /* temp_frame_ptr still points to the first octet of the MAC payload. */
    switch (mac_parse_data.frame_type) {
    case FCF_FRAMETYPE_BEACON:
        mac_data_parse_beacon(temp_frame_ptr, payload_index);
        break;

    case FCF_FRAMETYPE_DATA:
        mac_data_parse_data(temp_frame_ptr, payload_index);
        break;

    case FCF_FRAMETYPE_MAC_CMD:
        mac_data_parse_cmd(temp_frame_ptr, payload_index, fcf);
        break;

    default:
        return (false);
    }
    return (true);
}

static bool mac_data_handle_mac_cmd (uint8_t command, buffer_t *buf_p)
{
    bool processed_data_ind = true;

    switch (command) {
    case ASSOCIATIONREQUEST:
        mac_process_associate_request(buf_p);
        break;

    case DISASSOCIATIONNOTIFICATION:
        mac_process_disassociate_notification(buf_p);
        /*
         * Device needs to scan for networks again,
         * go into idle mode and reset variables
         */
        mac_idle_trans();
        break;

    case DATAREQUEST:
        if (indirect_data_q.size > 0) {
            mac_data_process_data_request(buf_p);
        } else {
            mac_data_tx_null_frame();
            processed_data_ind = false;
        }
        break;

    case PANIDCONFLICTNOTIFICAION:
        if (MAC_PAN_COORD_STARTED == mac_state) {
            mac_sync_loss(MAC_PAN_ID_CONFLICT);
        } else {
            processed_data_ind = false;
        }
        break;

    case ORPHANNOTIFICATION:
        mac_process_orphan_notification(buf_p);
        break;

    case BEACONREQUEST:
        if (MAC_COORDINATOR == mac_state) {
            /*
             * Only a Coordinator can both poll and AND answer beacon 
             * request frames. PAN Coordinators do not poll. End devices
             * do not answer beacon requests.
             */
            mac_process_beacon_request(buf_p);
        } else {
            processed_data_ind = false;
        }
        break;

    case COORDINATORREALIGNMENT:
        /*
         * Received coordinator realignment frame for
         * entire PAN.
         */
        if (MAC_PAN_COORD_STARTED != mac_state) {
            mac_sync_process_coord_realign(buf_p);
        } else {
            processed_data_ind = false;
        }
        break;

    default:
        processed_data_ind = false;
        break;
    }

    return (processed_data_ind);
}

/**
 *
 * This function is called in case a device (that is associated to a
 * PAN Coordinator) detects a PAN-Id conflict situation.
 *
 * Return - 
 * true if the PAN-Id conflict notification is sent successfully,
 * false otherwise
 */
static bool mac_data_tx_pan_id_conflict_notif (void)
{
    retval_t tx_status;
    uint8_t frame_len;
    uint8_t *frame_ptr;
    uint16_t fcf;
    frame_info_t *pan_id_conf_frame_p;
    tx_mode_t tx_mode;
    buffer_t *buf_p = bmm_buffer_alloc(BUFFER_SIZE);

    if (NULL == buf_p) {
        return (false);
    }

    pan_id_conf_frame_p = (frame_info_t *)BMM_BUFFER_POINTER(buf_p);

    pan_id_conf_frame_p->msg_type = PANIDCONFLICTNOTIFICAION;

    /*
     * The buffer header is stored as a part of frame_info_t structure 
     * before the * frame is given to the HAL. After the transmission of
     * the frame, reuse the buffer using this pointer.
     */
    pan_id_conf_frame_p->buffer_header_p = buf_p;

    /* Update the payload length. */
    frame_len = PAN_ID_CONFLICT_PAYLOAD_LEN +
                EXT_ADDR_LEN                + // long Source Address
                EXT_ADDR_LEN                + // long Destination Address
                PAN_ID_LEN                  + // Destination PAN-Id
                SEQ_NUM_LEN                 + // Sequence number
                FCF_LEN;                      // FCF

    /* Get the payload pointer. */
    frame_ptr = (uint8_t *)pan_id_conf_frame_p +
                BUFFER_SIZE -
                PAN_ID_CONFLICT_PAYLOAD_LEN;

    /*
     * Build the command frame id.
     * This is actually being written into "transmit_frame->layload[0]".
     */
    *frame_ptr = PANIDCONFLICTNOTIFICAION;

    /* Source Address */
    frame_ptr -= EXT_ADDR_LEN;
    mac_utils_64_bit_to_byte_array(hal_pib_IeeeAddress, frame_ptr);

    /* Destination Address */
    frame_ptr -= EXT_ADDR_LEN;
    mac_utils_64_bit_to_byte_array(mac_pib_macCoordExtendedAddress, frame_ptr);

    /* Destination PAN-Id */
    frame_ptr -= PAN_ID_LEN;
    mac_utils_16_bit_to_byte_array(hal_pib_PANId, frame_ptr);


    /* Set DSN. */
    frame_ptr -= SEQ_NUM_LEN;
    *frame_ptr = mac_pib_macDSN++;


    /* Build the FCF. */
    fcf = FCF_SET_FRAMETYPE(FCF_FRAMETYPE_MAC_CMD) |
          FCF_ACK_REQUEST                          |
          FCF_PAN_ID_COMPRESSION                   |
          FCF_SET_SOURCE_ADDR_MODE(FCF_LONG_ADDR)  |
          FCF_SET_DEST_ADDR_MODE(FCF_LONG_ADDR);

    /* Set the FCF. */
    frame_ptr -= FCF_LEN;
    mac_utils_16_bit_to_byte_array(fcf, frame_ptr);


    /* First element shall be length of PHY frame. */
    frame_ptr -= LENGTH_FIELD_LEN;
    *frame_ptr = frame_len;

    /* Finished building of frame. */
    pan_id_conf_frame_p->mpdu_p = frame_ptr;

    /*
     * In Beacon network the frame is sent with slotted CSMA-CA only if:
     * 1) the node is associated, or
     * 2) the node is idle, but synced before association,
     * 3) the node is a Coordinator (we assume, that coordinators are always
     *    in sync with their parents).
     *
     * In all other cases, the frame has to be sent using unslotted CSMA-CA.
     */

    if (NON_BEACON_NWK != hal_pib_BeaconOrder) {
        if (((MAC_IDLE == mac_state) && 
             (MAC_SYNC_BEFORE_ASSOC == mac_sync_state)) ||
             (MAC_ASSOCIATED == mac_state) || (MAC_COORDINATOR == mac_state)) {
            tx_mode = CSMA_SLOTTED;
        } else {
            tx_mode = CSMA_UNSLOTTED;
        }
    } else {
        /* In Nonbeacon network the frame is sent with unslotted CSMA-CA. */
        tx_mode = CSMA_UNSLOTTED;
    }

    tx_status = mac_hal_tx_frame(pan_id_conf_frame_p, tx_mode, FALSE);

    if (MAC_SUCCESS == tx_status) {
        MAC_BUSY();
        return (true);
    } else {
        /* HAL is busy, hence the data request could not be transmitted */
        bmm_buffer_free(buf_p);

        return (false);
    }
}

/**
 *
 * This function handles the detection of PAN-Id Conflict detection
 * in case the node is NOT a PAN Coordinator.
 *
 * in_scan Indicates whether node is currently scanning
 */
static void mac_data_in_pan_id_conflict_detect (bool in_scan)
{
    /*
     * Check whether the received frame has the PAN Coordinator bit set
     * in the Superframe Specification field of the beacon frame.
     */
    if (GET_PAN_COORDINATOR(
        mac_parse_data.mac_payload_data.beacon_data.superframe_spec)) {
        /*
         * The received beacon frame is from a PAN Coordinator
         * (not necessarily ours).
         * Now check if the PAN-Id is ours.
         */
        if (((in_scan == FALSE) &&
             (mac_parse_data.src_panid == hal_pib_PANId)) ||
            (mac_parse_data.src_panid == mac_scan_orig_panid)) {
            /* This beacon frame has our own PAN-Id.
             * If the address of the source is different from our own
             * parent, a PAN-Id conflict has been detected.
             */
            if ((mac_parse_data.src_addr.short_address !=
                 mac_pib_macCoordShortAddress) &&
                (EXT_ADDR_NOMATCH(mac_parse_data.src_addr.long_address,
                                  mac_pib_macCoordExtendedAddress))) {
                mac_data_tx_pan_id_conflict_notif();
            }
        }
    }
}

static bool mac_data_ind_handle_my_coor_beacon (buffer_t *buf_p,
                                                frame_info_t *frame_p)
{
    uint32_t beacon_tx_time_symb;
    bool     processed_data_ind = true;

    /* Check for PAN-Id conflict being NOT a PAN Corodinator. */
    if (mac_pib_macAssociatedPANCoord && (MAC_IDLE != mac_state)) {
        mac_data_in_pan_id_conflict_detect(false);
    }

    /* Check if the beacon is received from my coordinator */
    if ((mac_parse_data.src_panid == hal_pib_PANId) &&
        (((mac_parse_data.src_addr_mode == FCF_SHORT_ADDR) &&
          (mac_parse_data.src_addr.short_address ==
           mac_pib_macCoordShortAddress)) ||
           ((mac_parse_data.src_addr_mode == FCF_LONG_ADDR) &&
            (EXT_ADDR_MATCH(mac_parse_data.src_addr.long_address,
                            mac_pib_macCoordExtendedAddress))))) {

        beacon_tx_time_symb = frame_p->time_stamp;

        set_hal_pib_internal(macBeaconTxTime, (void *)&beacon_tx_time_symb);
        if ((MAC_SYNC_TRACKING_BEACON == mac_sync_state) ||
            (MAC_SYNC_BEFORE_ASSOC == mac_sync_state)) {
            //uint32_t nxt_bcn_tm;
            uint32_t beacon_int_symb;

            /* Process a received beacon. */
            mac_process_beacon_frame(buf_p);

            /* Initialize beacon tracking timer. */
            if (hal_pib_BeaconOrder < NON_BEACON_NWK) {
                beacon_int_symb = 
                        HAL_GET_BEACON_INTERVAL_TIME(hal_pib_BeaconOrder);
            } else {
                beacon_int_symb =
                        HAL_GET_BEACON_INTERVAL_TIME(BO_USED_FOR_MAC_PERS_TIME);
            }

            /* Stop the beacon tracking timer */
            STM_StopTimer(beacon_tracking_timer);

            /* Calculate the time for next beacon transmission */
            beacon_tx_time_symb =
                mac_hal_add_time_symbols(beacon_tx_time_symb, 
                                         beacon_int_symb) / 1000;

            /* 
             * Start beacon tracking timer to call mac_sync_tracking_beacons_cb
             */
            STM_StartTimer(beacon_tracking_timer, 
                           HAL_CONVERT_SYMBOLS_TO_US(beacon_tx_time_symb));

            /*
             * FIXME - Initialize superframe timer if required only
             * for devices because Superframe timer is already running for
             * coordinator.
             */

            /* Initialize missed beacon timer. */
            mac_sync_start_missed_beacon_timer();

            /*
             * A device that is neither scanning nor polling 
             * shall go to sleep now.
             */
            if ((MAC_COORDINATOR != mac_state) &&
                (MAC_SCAN_IDLE == mac_scan_state) &&
                (MAC_POLL_IDLE == mac_poll_state)) {
                /*
                 * If the last received beacon frame from our parent
                 * has indicated pending broadbast data, we need to
                 * stay awake, until the broadcast data has been received.
                 */
                if (!mac_bc_data_indicated) {
                    /* Set radio to sleep if allowed */
                    mac_trx_sleep();
                }
            }
        } else if (MAC_SYNC_ONCE == mac_sync_state) {
            mac_process_beacon_frame(buf_p);

            /* Do this after processing the beacon. */
            mac_sync_state = MAC_SYNC_NEVER;

            /*
             * A device that is neither scanning nor polling 
             * shall go to sleep now.
             */
            if ((MAC_COORDINATOR != mac_state) &&
                (MAC_SCAN_IDLE == mac_scan_state) &&
                (MAC_POLL_IDLE == mac_poll_state)) {
                /*
                 * If the last received beacon frame from our parent
                 * has indicated pending broadbast data, we need to
                 * stay awake, until the broadcast data has been received.
                 */
                if (!mac_bc_data_indicated) {
                    /* Set radio to sleep if allowed */
                    mac_trx_sleep();
                }
            }
        } else {
            processed_data_ind = false;
        }
    } else {
        /* No action taken, buffer will be freed. */
        processed_data_ind = false;
    }

    return (processed_data_ind);
}

/**
 * Continues processing a data indication from the HAL when the MAC is not 
 * polling or scanning
 *
 * buf_p   - Pointer to the buffer header.
 * frame_p - Pointer to the frame_info_t structure.
 *
 * bool - True if frame has been processed
 *        False otherwise.
 */
static bool mac_data_non_transient (buffer_t *buf_p,
                                    frame_info_t *frame_p)
{
    bool processed_data_ind = true;
    uint8_t track = 0;

    switch (mac_state) {
    case MAC_PAN_COORD_STARTED:
        if (FCF_FRAMETYPE_MAC_CMD == mac_parse_data.frame_type) {
            processed_data_ind =
                mac_data_handle_mac_cmd(mac_parse_data.mac_command, buf_p);
            track = 1;
        } else if (FCF_FRAMETYPE_DATA == mac_parse_data.frame_type) {
            mac_process_data_frame(buf_p);
            track = 2;
        } else if (FCF_FRAMETYPE_BEACON == mac_parse_data.frame_type) {
            /* PAN-Id conflict detection as PAN-Coordinator. */
            /* Node is not scanning. */
            mac_data_in_pan_id_conflict_detect(false);
            processed_data_ind = false;
            track = 3;
        } else {
            processed_data_ind = false;
            track = 4;
        }
        break;

    case MAC_IDLE:
    case MAC_ASSOCIATED:
    case MAC_COORDINATOR:
        if (FCF_FRAMETYPE_MAC_CMD == mac_parse_data.frame_type) {
            processed_data_ind =
                mac_data_handle_mac_cmd(mac_parse_data.mac_command, buf_p);
            track = 5;
        } else if (FCF_FRAMETYPE_DATA == mac_parse_data.frame_type) {
            mac_process_data_frame(buf_p);
            track = 6;
        } else if (FCF_FRAMETYPE_BEACON == mac_parse_data.frame_type) {
            processed_data_ind =
                        mac_data_ind_handle_my_coor_beacon(buf_p, frame_p);
            track = 7;
        } else {
            processed_data_ind = false;
            track = 8;
        }
        break;
    default:
        processed_data_ind = false;
        track = 9;
        break;
    }

    if (false == processed_data_ind) {
#ifdef _DEBUG_
        printf("\ntrack => %bu\n", track);
#endif
    }

    return (processed_data_ind);
}

/**
 * PAN-Id conflict detection NOT as PAN-Coordinator.
 *
 * This function handles the detection of PAN-Id Conflict detection
 * in case the node is NOT a PAN Coordinator.
 *
 * in_scan - whether node is currently scanning
 */
static void mac_data_pan_id_conflict_non_pc (bool in_scan)
{
    /*
     * Check whether the received frame has the PAN Coordinator bit set
     * in the Superframe Specification field of the beacon frame.
     */
    if (GET_PAN_COORDINATOR(mac_parse_data.mac_payload_data.beacon_data.superframe_spec)) {
        /*
         * The received beacon frame is from a PAN Coordinator
         * (not necessarily ours).
         * Now check if the PAN-Id is ours.
         */
        if (((FALSE == in_scan) && (mac_parse_data.src_panid == hal_pib_PANId)) ||
            (mac_parse_data.src_panid == mac_scan_orig_panid)) {
            /* This beacon frame has our own PAN-Id.
             * If the address of the source is different from our own
             * parent, a PAN-Id conflict has been detected.
             */
            if ((mac_parse_data.src_addr.short_address != 
                 mac_pib_macCoordShortAddress) &&
                (EXT_ADDR_MATCH(mac_parse_data.src_addr.long_address, 
                                mac_pib_macCoordExtendedAddress))) {
                mac_data_tx_pan_id_conflict_notif();
            }
        }
    }    
}

/*
 * PAN-Id conflict detection as PAN-Coordinator.
 *
 * This function handles the detection of PAN-Id Conflict detection
 * in case the node is a PAN Coordinator.
 *
 * in_scan - Whether node is currently scanning
 */
static void mac_data_pan_id_conflict_pc (bool in_scan)
{
    /*
     * Check whether the received frame has the PAN Coordinator bit set
     * in the Superframe Specification field of the beacon frame, and
     * whether the received beacon frame has the same PAN-Id as the current
     * network.
     */
    if (GET_PAN_COORDINATOR(mac_parse_data.mac_payload_data.beacon_data.superframe_spec)) {
        if (((!in_scan) && (mac_parse_data.src_panid == hal_pib_PANId)) ||
            (mac_parse_data.src_panid == mac_scan_orig_panid)) {
            mac_sync_loss(MAC_PAN_ID_CONFLICT);
        }
    }
}

/*
 * Processing indication from the HAL while scanning
 *
 * b_ptr Pointer to the buffer header.
 *
 * Return True if frame has been processed, or false otherwise.
 */
static bool process_data_ind_scanning (buffer_t *b_ptr)
{
    bool processed_in_scanning = false;
    /*
     * We are in a scanning process now (mac_scan_state is not MAC_SCAN_IDLE),
     * so continue with the specific scanning states.
     */
    switch (mac_scan_state) {
    /* Energy Detect scan */
    case MAC_SCAN_ED:
        /*
         * Ignore all frames received while performing ED measurement,
         * or while performing CCA.
         */
        break;

    /* Active scan or passive scan */
    case MAC_SCAN_ACTIVE:
    case MAC_SCAN_PASSIVE:
        if (FCF_FRAMETYPE_BEACON == mac_parse_data.frame_type) {
            /* PAN-Id conflict detection as PAN-Coordinator. */
            if (MAC_PAN_COORD_STARTED == mac_state) {
                /* Node is currently scanning. */
                mac_data_pan_id_conflict_pc(true);
            }

            if (mac_pib_macAssociatedPANCoord &&
                ((MAC_ASSOCIATED == mac_state) || 
                 (MAC_COORDINATOR == mac_state))) {
                mac_data_pan_id_conflict_non_pc(true);
            }

            mac_process_beacon_frame(b_ptr);
            processed_in_scanning = true;
        }
        break;

    /* Orphan scan */
    case MAC_SCAN_ORPHAN:
        if (FCF_FRAMETYPE_MAC_CMD == mac_parse_data.frame_type &&
            COORDINATORREALIGNMENT == mac_parse_data.mac_command) {
            /*
             * Received coordinator realignment frame in the middle of
             * an orphan scan.
             */
            STM_StopTimer(scan_duration_timer);

            mac_scan_process_orphan_realign(b_ptr);
            processed_in_scanning = true;
        }
        break;

    default:
        break;
    }

    return (processed_in_scanning);
}

static bool mac_data_handle_mac_poll_sm (buffer_t *buf_p, frame_info_t *frame_p)
{
    bool processed_data_ind = true;
    uint8_t track = 0;

    switch (mac_poll_state) {
    case MAC_POLL_IDLE:
        /*
         * We are in no transient state.
         * Now are either in a non-transient MAC state or scanning.
         */
        if (MAC_SCAN_IDLE == mac_scan_state) {
            /*
             * Continue with handling the "real" non-transient MAC states now.
             */
            track = 1;
            processed_data_ind = mac_data_non_transient(buf_p, frame_p);
        } else {
            /* Scanning is ongoing. */
            processed_data_ind = process_data_ind_scanning(buf_p);
            track = 2;
            processed_data_ind = false;
        }
        break;

    /*
     * This is the 'wait for data' state after either
     * explicit poll or implicit poll.
     */
    case MAC_POLL_EXPLICIT:
    case MAC_POLL_IMPLICIT:
        /*
         * Function mac_poll_process_data_response() resets the
         * MAC poll state.
         */
        mac_poll_process_data_response();

        if (FCF_FRAMETYPE_MAC_CMD == mac_parse_data.frame_type) {
            track = 3;
            processed_data_ind =
                mac_data_handle_mac_cmd(mac_parse_data.mac_command, buf_p);
        } else if (FCF_FRAMETYPE_DATA == mac_parse_data.frame_type) {
            track = 4;
            mac_process_data_frame(buf_p);
        } else {
            track = 5;
            processed_data_ind = false;
        }
        break;

    case MAC_AWAIT_ASSOC_RESPONSE:
        /*
         * We are either expecting an association reponse frame
         * or a null data frame.
         */
        if ((FCF_FRAMETYPE_MAC_CMD == mac_parse_data.frame_type) &&
            (ASSOCIATIONRESPONSE == mac_parse_data.mac_command)) {
            /* This is the expected association response frame. */
            STM_StopTimer(mac_asso_rsp_wait_timer);
            mac_process_associate_response(buf_p);
            track = 6;
        } else if (FCF_FRAMETYPE_DATA == mac_parse_data.frame_type) {
            mac_process_data_frame(buf_p);
            track = 7;
        }
        break;

    default:
        track = 8;
        processed_data_ind = false;
        break;
    }

    if (false == processed_data_ind) {
#ifdef _DEBUG_
        printf("\ntrack = %bu\n", track);
#endif
    }

    return (processed_data_ind);
}

/**
 * Depending on received frame the appropriate function is called
 *
 * buf_p - Pointer to the buffer.
 */
void mac_data_process_packet (buffer_t *buf_ptr)
{
    frame_info_t *frame_ptr = (frame_info_t *)BMM_BUFFER_POINTER(buf_ptr);
    bool processed_data_ind = false;

    mac_parse_data.mpdu_length = frame_ptr->mpdu_p[0];
    
    mac_stats_g.rx_pkts_count++;
    mac_stats_g.rx_bytes_count += mac_parse_data.mpdu_length;
    
    /* First extract LQI since this is already needed in Promiscuous Mode. */
    mac_parse_data.ppdu_link_quality = 
                       frame_ptr->mpdu_p[mac_parse_data.mpdu_length + LQI_LEN];

    if (hal_pib_PromiscuousMode) {
        /*
         * In promiscuous mode all received frames are forwarded to the
         * higher layer or application using MCPS_DATA.indication
         * primitives.
         */
        mac_data_promis_mode_rx_frame(buf_ptr, frame_ptr);
        return;
    }

    if (mac_data_parse_mpdu(frame_ptr) == FALSE) {
        /* Frame parsing failed */
        bmm_buffer_free(buf_ptr);
        return;
    }

    /* Check if the MAC is busy processing the previous requests */
    if (mac_busy) {
        /*
         * If MAC has to process an incoming frame that requires a response
         * (i.e. beacon request and data request) then process this operation
         * once the MAC has become free. Put the request received back into the
         * MAC internal event queue.
         */
        if (FCF_FRAMETYPE_MAC_CMD == mac_parse_data.frame_type) {
            if (DATAREQUEST == mac_parse_data.mac_command ||
                BEACONREQUEST == mac_parse_data.mac_command) {
                if (FALSE == qmm_queue_append(&hal_mac_q, buf_ptr)) {
                    bmm_buffer_free(buf_ptr);
                } else {
#ifdef RTX51_TINY_OS
                    os_set_ready(ZIGBEE_TASK_ID_MAC);
#endif
                }
                return;
            }
        }
    }

    processed_data_ind = mac_data_handle_mac_poll_sm(buf_ptr, frame_ptr);

    /* If message is not processed */
    if (!processed_data_ind) {
        bmm_buffer_free(buf_ptr);
    }
}

void mac_data_rx_frame_cb (frame_info_t *frame_p)
{
    frame_p->msg_type = (frame_msgtype_t)HAL_DATA_INDICATION;

    if (NULL == frame_p->buffer_header_p) {
        return;
    }

    mac_data_process_packet(frame_p->buffer_header_p);
}

/**
 *
 * This function builds and tranmits a data request command frame.
 *
 *
 * expl_poll           - Data request due to explicit MLME poll request
 * force_own_long_addr - Forces the usage of the Extended Address as
 *                       Source Address. This a allows for implicitly
 *                       poll for pending data at the coordinator if
 *                       the Extended Address was used in the Beacon frame.
 * expl_dest_addr_mode - Mode of subsequent destination address to be used
 *                       explicitly (0/2/3).
 *                       0: No explicit destination address attached,
 *                          use either macCoordShortAddress or
 *                          macCoordExtendedAddress
 *                       2: Use explicitly attached address in parameter
 *                          expl_dest_addr as destination address as
 *                          short address
 *                       3: Use explicitly attached address in parameter
 *                          expl_dest_addr as destination address as
 *                          extended address
 * expl_dest_addr      - Explicitly attached destination address for data
 *                       request frame. This is to be treated as either not
 *                       present, short or extended address, depending on
 *                       parameter expl_dest_addr_mode.
 * expl_dest_pan_id    - Explicitly attached destination PAN-Id (Coordinator
 *                       PAN-Id) for data request frame.
 *                       This is to be treated only as present, depending on
 *                       parameter expl_dest_addr_mode.
 *
 * return - True if data request command frame was created and sent to
 *          the HAL successfully, false otherwise.
 */
bool mac_data_build_and_tx_data_req (bool expl_poll,
                                     bool force_own_long_addr,
                                     uint8_t expl_dest_addr_mode,
                                     address_field_t *expl_dest_addr,
                                     uint16_t expl_dest_pan_id)
{
    retval_t tx_status;
    bool intrabit = false;
    uint8_t frame_len;
    uint8_t *frame_ptr;
    uint16_t fcf;
    tx_mode_t tx_mode;
    frame_info_t *tx_frame_p;
    buffer_t *buf_ptr = bmm_buffer_alloc(BUFFER_SIZE);

    if (NULL == buf_ptr) {
        return false;
    }

    tx_frame_p = (frame_info_t *)BMM_BUFFER_POINTER(buf_ptr);

    /*
     * If this data request cmd frame was initiated by a device due to implicit
     * poll, set msgtype to DATAREQUEST_IMPL_POLL.
     * If this data request cmd frame was initiated by a MLME poll request,
     * set msgtype to DATAREQUEST.
     */
    if (expl_poll == TRUE) {
        tx_frame_p->msg_type = DATAREQUEST;
    } else {
        tx_frame_p->msg_type = DATAREQUEST_IMPL_POLL;
    }

    /*
     * The buffer header is stored as a part of frame_info_t structure 
     * before the frame is given to the HAL. After the transmission, reuse
     * the buffer using this pointer.
     */
    tx_frame_p->buffer_header_p = buf_ptr;

    /* Update the payload length. */
    frame_len = DATA_REQ_PAYLOAD_LEN +
                SHORT_ADDR_LEN       + // short Source Address
                SHORT_ADDR_LEN       + // short Destination Address
                PAN_ID_LEN           + // Destination PAN-Id
                SEQ_NUM_LEN          + // Sequence number
                FCF_LEN;               // FCF

    /* Get the payload pointer. */
    frame_ptr = (uint8_t *)tx_frame_p + BUFFER_SIZE - DATA_REQ_PAYLOAD_LEN;

    /*
     * Build the command frame id.
     * This is actually being written into "transmit_frame->layload[0]".
     */
    *frame_ptr = DATAREQUEST;

    /* Source Address */
    /*
     * Long address needs to be used if a short address is not present
     * or if we are forced to use the long address.
     *
     * This is used for example in cases where the coordinator indicates
     * pending data for us using our extended address.
     *
     * This is also used for transmitting a data request frame
     * during association, since here we always need to use our
     * extended address.
     */
    if ((BROADCAST == hal_pib_ShortAddress) ||
        (MAC_NO_SHORT_ADDR_VALUE == hal_pib_ShortAddress) ||
        force_own_long_addr) {
        frame_ptr -= EXT_ADDR_LEN;
        frame_len += 6; // Add further 6 octets for long Source Address

        /* Build the Source address. */
        mac_utils_64_bit_to_byte_array(hal_pib_IeeeAddress, frame_ptr);
        fcf = FCF_SET_FRAMETYPE(FCF_FRAMETYPE_MAC_CMD) |
              FCF_SET_SOURCE_ADDR_MODE(FCF_LONG_ADDR)  |
              FCF_ACK_REQUEST;
    } else {
        frame_ptr -= SHORT_ADDR_LEN;

        /* Build the Source address. */
        mac_utils_16_bit_to_byte_array(hal_pib_ShortAddress, frame_ptr);

        fcf = FCF_SET_FRAMETYPE(FCF_FRAMETYPE_MAC_CMD) |
              FCF_SET_SOURCE_ADDR_MODE(FCF_SHORT_ADDR) |
              FCF_ACK_REQUEST;
    }


    /* Source PAN-Id */
    /*
     * In IEEE 802.15.4 the PAN ID Compression bit may always be set.
     * See page 154:
     * If the data request command is being sent in response to the receipt
     * of a beacon frame indicating that data are pending for that device,
     * the Destination Addressing Mode subfield of the Frame Control field
     * may be set to zero ..."
     * In order to keep the implementation simple the address info is also in
     * this case 2 or 3, i.e. the destination address info is present.
     * This in return means that the PAN ID Compression bit is always set for
     * data request frames, except the expl_dest_pan_id parameter is
     * different from our own PAN-Id PIB attribute.
     */
    if ((expl_dest_addr_mode != FCF_NO_ADDR) &&
        (expl_dest_pan_id    != hal_pib_PANId)) {
        frame_ptr -= PAN_ID_LEN;
        frame_len += PAN_ID_LEN;

        mac_utils_16_bit_to_byte_array(hal_pib_PANId, frame_ptr);
    } else {
        /*
         * The source PAN Id is not present since the PAN ID
         * Compression bit is set.
         */
        /* Set intra-PAN bit. */
        intrabit = true;
        fcf |= FCF_PAN_ID_COMPRESSION;
    }


    /* Destination Address */
    if (FCF_SHORT_ADDR == expl_dest_addr_mode) {
        /* An explicit short destination address is requested. */
        fcf |= FCF_SET_DEST_ADDR_MODE(FCF_SHORT_ADDR);

        frame_ptr -= SHORT_ADDR_LEN;
        mac_utils_16_bit_to_byte_array(expl_dest_addr->short_address,
                                       frame_ptr);
    } else if (FCF_LONG_ADDR == expl_dest_addr_mode) {
        /* An explicit long destination address is requested. */
        fcf |= FCF_SET_DEST_ADDR_MODE(FCF_LONG_ADDR);

        frame_ptr -= EXT_ADDR_LEN;
        frame_len += 6; // Add further 6 octets for long Destination Address
        mac_utils_64_bit_to_byte_array(expl_dest_addr->long_address, frame_ptr);
    } else {
        /* No explicit destination address is requested. */
        if (MAC_NO_SHORT_ADDR_VALUE != mac_pib_macCoordShortAddress) {
            /*
             * If current value of short address for coordinator PIB is
             * NOT 0xFFFE, the current value of the short address for
             * coordinator shall be used as desination address.
             */
            fcf |= FCF_SET_DEST_ADDR_MODE(FCF_SHORT_ADDR);

            frame_ptr -= SHORT_ADDR_LEN;
            mac_utils_16_bit_to_byte_array(mac_pib_macCoordShortAddress,
                                           frame_ptr);
        } else {
            /*
             * If current value of short address for coordinator PIB is 0xFFFE,
             * the current value of the extended address for coordinator
             * shall be used as desination address.
             */
            fcf |= FCF_SET_DEST_ADDR_MODE(FCF_LONG_ADDR);

            frame_ptr -= EXT_ADDR_LEN;
            frame_len += 6; // Add further 6 octets for long Destination Address
            mac_utils_64_bit_to_byte_array(mac_pib_macCoordExtendedAddress,
                                           frame_ptr);
        }
    }


    /* Destination PAN-Id */
    frame_ptr -= PAN_ID_LEN;

    if (intrabit) {
        /* Add our PAN-Id. */
        mac_utils_16_bit_to_byte_array(hal_pib_PANId, frame_ptr);
    } else {
        /*
         * There is an expclicit destination address present AND
         * the destination PAN-Id is different from our own PAN-ID,
         * so include the source PAN-id into the frame.
         */
        mac_utils_16_bit_to_byte_array(expl_dest_pan_id, frame_ptr);
    }


    /* Set DSN. */
    frame_ptr -= SEQ_NUM_LEN;
    *frame_ptr = mac_pib_macDSN++;


    /* Set the FCF. */
    frame_ptr -= FCF_LEN;
    mac_utils_16_bit_to_byte_array(fcf, frame_ptr);


    /* First element shall be length of PHY frame. */
    frame_ptr -= LENGTH_FIELD_LEN;
    *frame_ptr = frame_len;

    /* Finished building of frame. */
    tx_frame_p->mpdu_p = frame_ptr;


    /* Transmission should be done with CSMA-CA and frame retries. */
    /*
     * In Beacon network the frame is sent with slotted CSMA-CA only if:
     * 1) the node is associated, or
     * 2) the node is idle, but synced before association,
     * 3) the node is a Coordinator (we assume, that coordinators are always
     *    in sync with their parents).
     *
     * In all other cases, the frame has to be sent using unslotted CSMA-CA.
     */;

    if (NON_BEACON_NWK != hal_pib_BeaconOrder) {
        if (((MAC_IDLE == mac_state) &&
            (MAC_SYNC_BEFORE_ASSOC == mac_sync_state)) ||
            (MAC_ASSOCIATED == mac_state) ||
            (MAC_COORDINATOR == mac_state)) {
            tx_mode = CSMA_SLOTTED;
        } else {
            tx_mode = CSMA_UNSLOTTED;
        }
    } else {
        /* In Nonbeacon network the frame is sent with unslotted CSMA-CA. */
        tx_mode = CSMA_UNSLOTTED;
    }

    tx_status = mac_hal_tx_frame(tx_frame_p, tx_mode, FALSE);

    if (MAC_SUCCESS == tx_status) {
        MAC_BUSY();
        return (true);
    } else {
        /* HAL is busy, hence the data request could not be transmitted */
        bmm_buffer_free(buf_ptr);

        return (false);
    }
}

