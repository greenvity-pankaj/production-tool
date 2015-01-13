/**
 * @file mac_beacon.c
 *
 * Implements the building of beacon frames and initiates transmission via
 * CSMA-CA after reception of a beacon request frame in a nonbeacon-enabled PAN.
 * Also processing the received beacon frame from the coordinator
 *
 * Copyright (c) 2011, Greenvity Communication All rights reserved.
 *
 */

/* === Includes ============================================================= */

#include <string.h>
#include "hal_common.h"
#include "return_val.h"
#include "bmm.h"
#include "qmm.h"
#include "mac_const.h"
#include "mac_msgs.h"
#include "mac_data_structures.h"
#include "mac_hal.h"
#include "mac_api.h"
#include "mac_internal.h"
#include "mac.h"
#include "utils.h"
#include "mac_security.h"
#include "timer.h"
#include "stm.h"

/* === Macros =============================================================== */

/*
 * Time in (advance) symbols before beacon interval when beacon is prepared
 */
#define ADVNC_BCN_PREP_TIME                 (50)

/*
 * (Minimal) Beacon payload length
 * 2 octets Superframe Spec
 * At least 1 octet GTS fields
 * At least 1 octet Pending Address fields
 */
#define BEACON_PAYLOAD_LEN                  (4)

/*
 * Maximum number of pending extended and/short addresses to be added to
 * Beacon frame indicating pending data.
 */
#define BEACON_MAX_PEND_ADDR_CNT            (7)

/*
 * Extract the beacon order from the Superframe Spec.
 */
#define GET_BEACON_ORDER(spec)          ((spec) & 0x000F)

/*
 * Extract the superframe order from the Superframe Spec.
 */
#define GET_SUPERFRAME_ORDER(spec)      (((spec) & 0x00F0) >> 4)

/*
 * Extract the final CAP slot from Superframe Spec.
 */
#define GET_FINAL_CAP(spec)             (((spec) & 0x0F00) >> 8)

/* === Globals ============================================================== */

/*
 * Static buffer used for beacon transmission
 */
static uint8_t beacon_buffer[BUFFER_SIZE];

/* Pointer used for adding pending addresses to the beacon frame. */
static uint8_t *beacon_ptr;

/* Variable to hold number the pending addresses. */
static uint8_t pending_address_count;

static bool first_beacon_pkt = FALSE;

uint32_t beacon_time_to_send = 0;

/* === Prototypes =========================================================== */

/* === Implementation ======================================================= */
/*
 *
 * This function appends pending extended addresses in the indirect queue
 * to the beacon.
 *
 * buf_ptr - Pointer to the indirect data in the indirect queue
 * handle  - Callback parameter
 *
 * return false to traverse through the full indirect queue
 *
 */
bool mac_beacon_add_pending_extended_address_cb (void xdata *buf_ptr,
                                                 void xdata *handle)
{
    frame_info_t *frame_p = (frame_info_t *)buf_ptr;

    /* Only 7 extended addresses are allowed in one Beacon frame. */
    if (pending_address_count < BEACON_MAX_PEND_ADDR_CNT) {
        /*
         * Only if the destination addressing mode is extended address mode
         * then the indirect data is used to populate the beacon buffer 
         * with extended destination address.
         */
        if (FCF_LONG_ADDR == 
            ((frame_p->mpdu_p[PL_POS_FCF_2] >> FCF_2_DEST_ADDR_OFFSET) & 
             FCF_ADDR_MASK)) {
            beacon_ptr -= sizeof(uint64_t);
            memcpy(beacon_ptr, &frame_p->mpdu_p[PL_POS_DST_ADDR_START],
                   sizeof(uint64_t));
            pending_address_count++;
        }
    }
    handle = handle;

    return (false);  /* Continue next record in the indirect queue */ 
}

/*
 * This function appends the pending short addresses to the beacon based
 * on frames currently in the indirect queue.
 *
 * buf_ptr - Pointer to the indirect data in the indirect queue
 * handle  - Callback parameter
 *
 * return false to traverse through the full indirect queue
 *
 */
bool mac_beacon_add_pending_short_address_cb (void xdata *buf_ptr,
                                              void xdata *handle)
{
    frame_info_t *frame_p = (frame_info_t *)buf_ptr;

    /* Only 7 short addresses are allowed in one Beacon frame. */
    if (pending_address_count < BEACON_MAX_PEND_ADDR_CNT) {
        /*
         * Only if the destination addressing mode is short address mode 
         * then the indirect data is used to populate the beacon buffer
         * with short destination address.
         */
        if (FCF_SHORT_ADDR == 
            ((frame_p->mpdu_p[PL_POS_FCF_2] >> FCF_2_DEST_ADDR_OFFSET) &
              FCF_ADDR_MASK)) {
            beacon_ptr -= sizeof(uint16_t);
            memcpy(beacon_ptr, &frame_p->mpdu_p[PL_POS_DST_ADDR_START],
                   sizeof(uint16_t));
            pending_address_count++;
        }
    }
    handle = handle;
    return (false);  /* Continue next record in the indirect queue */ 
}

/*
 * @brief Populates the beacon frame with pending addresses
 *
 * This function populates the beacon frame with pending addresses by
 * traversing through the indirect queue.
 *
 * buf_ptr - Pointer to the location in the beacon frame buffer where the
 * pending addresses are to be updated
 *
 * return Number of bytes added in the beacon frame as a result of pending
 * address
 */
static uint8_t mac_buffer_add_pending (uint8_t *buf_ptr)
{
    search_t find_buf;
    uint8_t  number_of_ext_address = 0;

    /*
     * Set pointer to beacon to proper place. Currently it is still
     * pointing at the first octet of the beacon payload (if there
     * is any)
     * The beacon_ptr pointer will be updated (i.e. decreased) in
     * the functions mac_beacon_add_pending_short_address_cb() &
     * add_pending_extended_address_cb() according to the included
     * octets containing the pending addresses.
     *
     * Note: Since the pending addresses is filled from the back,
     * the extended are filled in first.
     */
    beacon_ptr = buf_ptr;

    /* Initialize extended address count. */
    pending_address_count = 0;

    /*
     * This callback function traverses through the indirect queue and
     * updates the beacon buffer with the pending extended addresses.
     */
#ifdef CALLBACK
    find_buf.compare_func = mac_beacon_add_pending_extended_address_cb;
#else
    find_buf.compare_func_id = MAC_BEACON_ADD_PENDING_EXT_ADDR;
#endif

    /*
     * At the end of this function call (qmm_queue_read), the beacon buffer
     * will be updated with the short address (if any) of the indirect
     * data (if any) present in the indirect queue.
     */
    qmm_queue_read(&indirect_data_q, &find_buf);

    /*
     * The count of extended addresses added in the beacon frame is backed up
     * (as the same variable will be used to count the number of added
     * short addresses).
     */
    number_of_ext_address = pending_address_count;

    /* Initialize extended address count. */
    pending_address_count = 0;

    /*
     * This callback function traverses through the indirect queue and
     * updates the beacon buffer with the pending short addresses.
     */
#ifdef CALLBACK
    find_buf.compare_func = mac_beacon_add_pending_short_address_cb;
#else
    find_buf.compare_func_id = MAC_BEACON_ADD_PENDING_SHORT_ADDR;
#endif

    /*
     * At the end of this function call (qmm_queue_read), the beacon buffer
     * will be updated with the extended address (if any) of the indirect
     * data (if any) present in the indirect queue.
     */
    qmm_queue_read(&indirect_data_q, &find_buf);

    /*
     * Update buf_ptr to current position of beginning of
     * pending address specifications filled above.
     * Fill in Pending Address Specification (see IEEE 802.15.4-2006 Table 46).
     * In order to this buf_ptr needs to be decremented.
     */
    buf_ptr = beacon_ptr - 1;
    *buf_ptr = (pending_address_count) | (number_of_ext_address  << 4);


    /*
     * Total number of bytes used for pending address in beacon frame.
     * Note: The length of the one octet for the Pending Address Specification
     * is already included in the default beacon frame length
     * (see BEACON_PAYLOAD_LEN).
     */
    pending_address_count = (pending_address_count * sizeof(uint16_t)) +
                            (number_of_ext_address * sizeof(uint64_t));

    return (pending_address_count);
}

/**
 *
 * This function is called to build a beacon frame. For beaconless network
 * this function also transmits the generated beacon frame.
 *
 * beacon_enabled Flag indicating the mode of beacon transmission
 **/
void mac_build_and_tx_beacon (bool beacon_enabled, security_info_t *sec_info_p)
{
    frame_info_t *tx_frame_p;
    uint16_t     superframe_spec = 0;
    uint16_t     fcf = 0;
    uint8_t      frame_len;
    uint8_t      *frame_ptr;
    bool         encrypt = FALSE;
    retval_t     status;
    /*
     * The frame is given to the HAL in the 'frame_info_t' format,
     * hence an instance of the frame_info_t is created.
     */
    tx_frame_p = (frame_info_t *)beacon_buffer;

    /* Buffer header not required in BEACON since static buffer is used */
    tx_frame_p->buffer_header_p = NULL;

    tx_frame_p->msg_type = BEACON_MESSAGE;

    /* Update the payload length. */
    frame_len = BEACON_PAYLOAD_LEN +
                PAN_ID_LEN         +
                FCF_LEN            +
                SEQ_NUM_LEN;

    /* Get the payload pointer. */
    frame_ptr = (uint8_t *)tx_frame_p + BUFFER_SIZE;

    /* Build the beacon payload if it exists. */
    if (mac_pib_macBeaconPayloadLength > 0) {
        frame_ptr -= mac_pib_macBeaconPayloadLength;
        frame_len += mac_pib_macBeaconPayloadLength;

        memcpy(frame_ptr, mac_beacon_payload, mac_pib_macBeaconPayloadLength);
    }


    /*
     * Check if the indirect queue has entries, otherwise there is nothing
     * to add as far as pending addresses is concerned.
     */
    if (indirect_data_q.size > 0) {
        uint8_t pending_addr_octets = mac_buffer_add_pending(frame_ptr);
        frame_len += pending_addr_octets;
        frame_ptr -= pending_addr_octets + 1;
    } else {
        /* No pending data available. */
        /* len was added as part of BEACON_PAYLOAD_LEN */
        frame_ptr--;
        *frame_ptr = 0;
    }

    /* frame_ptr now points to the Pending Address Specification (Octet 1). */
    /* Build the (empty) GTS fields. */
    /* FIXME - GTS support to be added */
    frame_ptr--;
    *frame_ptr = 0;


    /* The superframe specification field is updated. */
    superframe_spec = hal_pib_BeaconOrder;
    superframe_spec |= (hal_pib_SuperFrameOrder << 4);
    superframe_spec |= (mac_final_cap_slot << 8);

    if (hal_pib_BattLifeExt) {
        superframe_spec |= (1U << BATT_LIFE_EXT_BIT_POS);
    }

    if (MAC_PAN_COORD_STARTED == mac_state) {
        superframe_spec |= (1U << PAN_COORD_BIT_POS);
    }

    if (mac_pib_macAssociationPermit) {
        superframe_spec |= (1U << ASSOC_PERMIT_BIT_POS);
    }

    /* Set the Superframe Specification Field. */
    /* len was added as part of BEACON_PAYLOAD_LEN */
    frame_ptr -= SUPER_FRAME_SPEC_LEN;
    mac_utils_16_bit_to_byte_array(superframe_spec, frame_ptr);
    
    if (sec_info_p->SecurityLevel > 0) {
        if (mac_pib_macSecurityEnabled == TRUE) {

            status = mac_build_aux_sec_header(&frame_ptr, sec_info_p,
                                              &frame_len);
            if (MAC_SUCCESS != status) {
                return;
            }
            fcf = FCF_SECURITY_ENABLED | FCF_FRAME_VERSION_2006;
            encrypt = TRUE;
        } else {
            return;
        }
    }

    /*
     * Source address.
     */
    if (MAC_NO_SHORT_ADDR_VALUE == hal_pib_ShortAddress) {
        frame_ptr -= EXT_ADDR_LEN;
        frame_len += EXT_ADDR_LEN;
        mac_utils_64_bit_to_byte_array(hal_pib_IeeeAddress, frame_ptr);

        fcf |= FCF_SET_SOURCE_ADDR_MODE((uint16_t)FCF_LONG_ADDR);
    } else {
        frame_ptr -= SHORT_ADDR_LEN;
        frame_len += SHORT_ADDR_LEN;
        mac_utils_16_bit_to_byte_array(hal_pib_ShortAddress, frame_ptr);

        fcf |= FCF_SET_SOURCE_ADDR_MODE((uint16_t)FCF_SHORT_ADDR);
    }

    /* Source PAN-Id */
    frame_ptr -= PAN_ID_LEN;
    mac_utils_16_bit_to_byte_array(hal_pib_PANId, frame_ptr);

    /* Set BSN. */
    frame_ptr -= SEQ_NUM_LEN;
    *frame_ptr = mac_pib_macBSN++;

    fcf = fcf | FCF_SET_FRAMETYPE(FCF_FRAMETYPE_BEACON);

    /*
     * In case
     * 1) the node is a PAN Coordinator or Coordinator, and
     * 2) this is beacon-enabled network, and
     * 3) there is a broadcast frame to be transmitted,
     * the frame pending bit in the frame control field of the beacon frame
     * to be transmitted needs to be set in order to indicate this to all
     * listening children nodes.
     */
    if (((MAC_PAN_COORD_STARTED == mac_state) ||
         (MAC_COORDINATOR == mac_state)) &&
        (hal_pib_BeaconOrder < NON_BEACON_NWK) &&
        (broadcast_q.size > 0)) {
        fcf |= FCF_FRAME_PENDING;
    }

    /* Set the FCF. */
    frame_ptr -= FCF_LEN;
    mac_utils_16_bit_to_byte_array(fcf, frame_ptr);


    /* First element shall be length of PHY frame. */
    frame_ptr -= LENGTH_FIELD_LEN;
    *frame_ptr = frame_len;

    /* Finished building of frame. */
    tx_frame_p->mpdu_p = frame_ptr;
    if (TRUE == encrypt) {
        wpan_addr_spec_t dst_addr_spec;

        dst_addr_spec.AddrMode = FCF_NO_ADDR;
        status = mac_secure(sec_info_p, &dst_addr_spec);
        if (MAC_SUCCESS != status) {
            return;
        }
    }
    if (beacon_enabled == FALSE) {
        /*
         * In a beaconless network the beacon is transmitted with
         * unslotted CSMA-CA.
         */
        mac_hal_tx_frame(tx_frame_p, CSMA_UNSLOTTED, encrypt);

        MAC_BUSY();

    }
    /*
     * In case beaconing mode is enabled, the beacon will be transmitted
     * using function hal_tx_beacon once the beacon timer has expired.
     * Therefore there is nothing to be done here.
     */
}

/*
 * @brief Callback function of the beacon preparation time timer
 *
 * This is the calback function of the timer started with timeout
 * value equal to ADVNC_BCN_PREP_TIME symbols less than beacon interval.
 *
 * In case of the beacon enabled network the beacon frame is created and
 * stored at the TAL. After preparing the beacon, the timer is restarted for
 * the next beacon interval less the beacon preparation time.
 *
 * @param callback_parameter Callback parameter
 */
static void mac_beacon_prepare_beacon_cb (void *callback_parameter)
{
    security_info_t  sec;

    /* Wake up radio first */
    mac_trx_wakeup();

    /* FIXME - */
    sec.SecurityLevel   = (uint8_t)0;
    sec.KeyIdMode       = (uint8_t)0;
    sec.KeyIndex        = (uint8_t)0;

    /* For a beacon enabled network, the beacon is stored at the HAL. */
    mac_build_and_tx_beacon(true, &sec);

    callback_parameter = callback_parameter;  /* Keep compiler happy. */
}



/*
 * @brief Handles transmission of pending broadcast data
 *
 * This function handles the transmission of pending broadcast data
 * frames in a beacon-enabled network, which need to be transmitted
 * immediately after the beacon transmission.
 * As defined by 802.15.4-2006 exactly one broadcast data frame is
 * transmitted at one point of time. Further pending broadcast frames
 * are transmitted after the next beacon frame.
 */
void mac_tx_pending_bc_data (void)
{
    buffer_t     *buf_ptr;
    frame_info_t *tx_frame_p;
    retval_t     tx_status;

    buf_ptr = qmm_queue_remove(&broadcast_q, NULL);

    if (NULL == buf_ptr) {
        /* Nothing ot be done. */
        return;
    }

    /* Broadcast data present and to be sent. */
    tx_frame_p = (frame_info_t *)BMM_BUFFER_POINTER(buf_ptr);

    tx_frame_p->buffer_header_p = buf_ptr;

    tx_status = mac_hal_tx_frame(tx_frame_p, NO_CSMA, FALSE);

    if (MAC_SUCCESS == tx_status) {
        MAC_BUSY();
    } else {
        mac_gen_mcps_data_conf((buffer_t *)tx_frame_p->buffer_header_p,
                               (uint8_t)MAC_CHANNEL_ACCESS_FAILURE,
                               tx_frame_p->msduHandle,
                               0);
    }
}

/**
 *
 *  This function is called when a beacon request frame has been received by
 *  a coordinator. In a nonbeacon-enabled PAN the generation of a beacon frame
 *  using CSMA-CA is initiated. In a beacon-enabled PAN no extra beacon frame
 *  will be transmitted apart from the standard beacon frames.
 *
 *  msg Pointer to the buffer in which the beaocn request was received
 */
void mac_process_beacon_request (buffer_t *msg_p)
{
    /*
     * The buffer in which the beacon request was received is freed up.
     * This is only done in a BEACON build, since a static buffer is used
     * to transmit the beacon frame.
     */
    bmm_buffer_free(msg_p);

    /*
     * If the network is a beacon enabled network then the beacons will not be
     * transmitted.
     */
    if (hal_pib_BeaconOrder == NON_BEACON_NWK) {
	security_info_t  sec;

        /* FIXME - */
        sec.SecurityLevel   = (uint8_t)0;
        sec.KeyIdMode       = (uint8_t)0;
        sec.KeyIndex        = (uint8_t)0;

        /* The beacon is transmitted using CSMA-CA. */
        mac_build_and_tx_beacon(false, &sec);
    }
}

/*
 * Transmits beacon frame after beacon interval
 */
void mac_beacon_send_cb (void)
{
    retval_t status = MAC_FAILURE;
    /*
     * This check here is done in order to stop beacon timing in case
     * the network has transitioned from a beacon-enabled network to
     * nonbeacon-enabled network.
     */
    if (hal_pib_BeaconOrder < NON_BEACON_NWK) {
        /*
         * In case the node is currently scanning, no beacon will be
         * transmitted.
         */
        if (MAC_SCAN_IDLE == mac_scan_state) {
            /*
             * The frame is given to the TAL in the 'frame_info_t' format,
             * hence an instance of the frame_info_t is created.
             */
            frame_info_t *tx_frame_p = (frame_info_t *)beacon_buffer;

            if (first_beacon_pkt == TRUE) {
                mac_hal_tx_frame(tx_frame_p, CSMA_UNSLOTTED, FALSE);
            } else {
	            security_info_t  sec;

                /* FIXME - Add beacon secure support */
                sec.SecurityLevel   = (uint8_t)0;
                sec.KeyIdMode       = (uint8_t)0;
                sec.KeyIndex        = (uint8_t)0;

                mac_build_and_tx_beacon(true, &sec);
                mac_hal_tx_beacon(tx_frame_p, FALSE);
            }
            /*
             * We do not want to set the MAC_BUSY as beacon to be sent
             * in CSMA_SLOTTED can take some times to transmit. In the mean
             * time, other packets still can be sent
             */
        }

        /*
         * Transmit pending broadcast frame will be transmitted.
         * Of course this is only done if the node is not scanning.
         */
        if (MAC_SCAN_IDLE == mac_scan_state) {
            /*
             * Check for pending broadcast data frames in the broadcast queue
             * and transmit exactly one broadcast data frame in case there
             * are pending broadcast frames.
             */
            if (broadcast_q.size > 0) {
                mac_tx_pending_bc_data();
            }
        }
    }
}

/**
 *
 * This function is called during MLME_START.request operation to start the
 * timers for beacon transmission in case the network is of type beacon enabled,
 * or to start the timer to count the transcation presistence time of indirect
 * data in case the network is nonbeacon enabled.
 *
 * The timers started for a beacon enabled and nonbeacon enabled networks are
 * different with different timeout values.
 *
 * In case of a beacon enabled network two timers are started. The first timer
 * prepares the beacon and second timer sends the beacon. In case of the
 * nonbeacon enabled network, only a single timer is started with timeout as
 * the beacon interval.
 *
 * For a beacon enabled network the first beacon of the network is prepared and
 * sent in this function and the subsequent beacons are sent in the callback
 * function of the timer.
 */
void mac_start_beacon_timer (void)
{
    uint32_t beacon_tx_time;
    uint32_t beacon_int_symbols;
    security_info_t  sec;

    /* FIXME - */
    sec.SecurityLevel   = (uint8_t)0;
    sec.KeyIdMode       = (uint8_t)0;
    sec.KeyIndex        = (uint8_t)0;

    /*
     * This is a beacon enabled network.
     * The First beacon in a beacon enabled network is transmitted
     * directly without CSMA-CA. Call mac_build_and_tx_beacon with 
     * beacon_enable = true so that the beacon frame is built but 
     * is not sent
     */
    mac_build_and_tx_beacon(true, &sec);

    /*
     * For the first beacon the current time is used as beacon transmission
     * time. For consecutive beacon transmissions, this beacon transmission
     * time is added to the beacon interval and then used.
     * beacon_tx_time is in symbols
     */
    mac_hal_get_current_time(&beacon_tx_time);

    /* 
     * Since the PIB attribute macBeaconTxTime is supposed to hold the time
     * of the last transmitted beacon frame, but we are going to transmit
     * our first beacon frame just now (in the subsequentially called function
     * mac_beacon_send_cb(), we need to actually substract the time of one
     * entire beacon period from this time.
     * Otherwise we would leave out the second beacon in function
     * mac_beacon_send_cb().
     */
    beacon_int_symbols = HAL_GET_BEACON_INTERVAL_TIME(hal_pib_BeaconOrder);

    beacon_tx_time = mac_hal_sub_time_symbols(beacon_tx_time,
                                              beacon_int_symbols);

    mac_hal_pib_set(macBeaconTxTime, (void *)&beacon_tx_time);

    /*
     * Indicate to the mac_beacon_send_cb() that this is the 1st beacon to
     * be transmitted.
     */
    first_beacon_pkt = TRUE;
    mac_beacon_send_cb();
    first_beacon_pkt = FALSE;
} /* mac_start_beacon_timer() */

/**
 *
 * This function processes a received beacon frame.
 * When the system is scanning it records PAN descriptor information
 * contained in the beacon. These PAN descriptors will be reported to the
 * next higher layer via MLME_SCAN.confirm.
 * Also this routine constructs the MLME_BEACON_NOTIFY.indication.
 * Additionally when a device is synced with the coordinator, it tracks beacon
 * frames, checks whether the coordinator does have pending data and will
 * initiate the transmission of a data request frame.
 * The routine uses global "parse_data" structure.
 * The PAN descriptors are stored in the mlme_scan_conf_t structure.
 *
 * beacon_p - Pointer to the buffer in which the beacon was received
 *
 */
void mac_process_beacon_frame (buffer_t *beacon_p)
{
    bool matchflag;
    pandescriptor_t *pan_desc_list_p = NULL;
    pandescriptor_t pan_desc;
    mlme_scan_conf_t *msc_p = NULL;
    uint8_t numaddrshort;
    uint8_t numaddrlong;
    uint8_t index;

    /*
     * Extract the superframe parameters of the beacon frame only if
     * scanning is NOT ongoing.
     */
    if (MAC_SCAN_IDLE == mac_scan_state) {
        /* Beacon frames are not of interest for a PAN coordinator. */
        if (MAC_PAN_COORD_STARTED != mac_state) {
            uint8_t superframe_order;
            uint8_t beacon_order;

            /*
             * For a device, the parameters obtained from the beacons are
             * used to update the HAL's PIBs
             */
            beacon_order =
                GET_BEACON_ORDER(mac_parse_data.mac_payload_data.\
                                 beacon_data.superframe_spec);

            set_hal_pib_internal(macBeaconOrder, (void *)&beacon_order);

            superframe_order =
                GET_SUPERFRAME_ORDER(mac_parse_data.mac_payload_data.\
                                     beacon_data.superframe_spec);
            set_hal_pib_internal(macSuperframeOrder,
                                 (void *)&superframe_order);

            mac_final_cap_slot =
                GET_FINAL_CAP(mac_parse_data.mac_payload_data.\
                              beacon_data.superframe_spec);

        }  /* (MAC_PAN_COORD_STARTED != mac_state) */
    }  /* (MAC_SCAN_IDLE == mac_scan_state) */

    /*
     * The following section needs to be done when we are
     * either scanning (and look for a new PAN descriptor to be returned
     * as part of the scan confirm message),
     * or we need to create a beacon notification (in which case we are
     * interested in any beacon, but omit the generation of scan confirm).
     *
     * If we are scanning a scan confirm needs to be created.
     *
     * According to 802.15.4-2006 this is only done in case the PIB
     * attribute macAutoRequest is true. Otherwise the PAN descriptor will
     * NOT be put into the PAN descriptor list of the Scan confirm message.
     */
     if (((MAC_SCAN_ACTIVE == mac_scan_state)   || 
          (MAC_SCAN_PASSIVE == mac_scan_state)) && mac_pib_macAutoRequest) {
        /*
         * mac_conf_buf_ptr points to the buffer allocated for scan
         * confirmation.
         */
         msc_p =  (mlme_scan_conf_t *)BMM_BUFFER_POINTER(
                        ((buffer_t *)mac_conf_buf_ptr));

        /*
         * The PAN descriptor list is updated with the PANDescriptor of the
         * received beacon
         */
        pan_desc_list_p = (pandescriptor_t *)&msc_p->scan_result_list;
    }

    /*
     * The beacon data received from the parse variable is arranged
     * into a PAN descriptor structure style
     */
    pan_desc.CoordAddrSpec.AddrMode = mac_parse_data.src_addr_mode;
    pan_desc.CoordAddrSpec.PANId    = mac_parse_data.src_panid;

    if (FCF_SHORT_ADDR == pan_desc.CoordAddrSpec.AddrMode) {
        /* Initially clear the complete address. */
        EXT_ADDR_CLEAR(pan_desc.CoordAddrSpec.Addr.long_address);
        ADDR_COPY_DST_SRC_16(pan_desc.CoordAddrSpec.Addr.short_address,
                             mac_parse_data.src_addr.short_address);
    } else {
        ADDR_COPY_DST_SRC_64(pan_desc.CoordAddrSpec.Addr.long_address,
                             mac_parse_data.src_addr.long_address);
    }

    pan_desc.LogicalChannel = hal_pib_CurrentChannel;
    pan_desc.ChannelPage    = hal_pib_CurrentPage;
    pan_desc.SuperframeSpec = mac_parse_data.mac_payload_data.\
                              beacon_data.superframe_spec;
    pan_desc.GTSPermit      = mac_parse_data.mac_payload_data.\
                              beacon_data.gts_spec >> 7;
    pan_desc.LinkQuality    = mac_parse_data.ppdu_link_quality;
    pan_desc.TimeStamp      = mac_parse_data.time_stamp;

    /*
     * If we are scanning we need to check whether this is a new
     * PAN descriptor. Check for pan_desc_list_p instead since it
     * is set only when in scanning state
     *
     * According to 802.15.4-2006 this is only done in case the PIB
     * attribute macAutoRequest is true. Otherwise the PAN descriptor will
     * NOT be put into the PAN descriptor list of the Scan confirm message.
     */
    if (pan_desc_list_p != NULL) {
        /*
         * This flag is used to indicate a match of the current (received) PAN
         * descriptor with one of those present already in the list.
         */
        matchflag = false;

        /*
         * The beacon frame PAN descriptor is compared with the PAN descriptors
         * present in the list and determine if the current PAN
         * descriptor is to be taken as a valid one. A PAN is considered to be
         * the same as an existing one, if all, the PAN Id, the coordinator
         * address mode, the coordinator address, and the Logical Channel
         * are same.
         */
        for (index = 0; index < msc_p->ResultListSize; index++) {
            if ((pan_desc.CoordAddrSpec.PANId ==
                 pan_desc_list_p->CoordAddrSpec.PANId) &&
                (pan_desc.CoordAddrSpec.AddrMode ==
                 pan_desc_list_p->CoordAddrSpec.AddrMode) &&
                (pan_desc.LogicalChannel ==
                 pan_desc_list_p->LogicalChannel) &&
                (pan_desc.ChannelPage == pan_desc_list_p->ChannelPage)) {
                if (pan_desc.CoordAddrSpec.AddrMode == WPAN_ADDRMODE_SHORT) {
                    if (pan_desc.CoordAddrSpec.Addr.short_address ==
                        pan_desc_list_p->CoordAddrSpec.Addr.short_address) {
                        /* Beacon with same parameters already received */
                        matchflag = true;
                        break;
                    }
                } else {
                    if (EXT_ADDR_MATCH(pan_desc.CoordAddrSpec.Addr.long_address,
                        pan_desc_list_p->CoordAddrSpec.Addr.long_address)) {
                        /* Beacon with same parameters already received */
                        matchflag = true;
                        break;
                    }
                }
            }
            pan_desc_list_p++;  /* Next pan desc in the result list */
        }

        /*
         * If the PAN descriptor is not in the current list, and there is space
         * left, it is put into the list
         */
        if ((matchflag == FALSE) &&
            (msc_p->ResultListSize < MAX_PANDESCRIPTORS)) {
            memcpy(pan_desc_list_p, &pan_desc, sizeof(pan_desc));
            msc_p->ResultListSize++;
        }
    }


    /* The short and extended pending addresses are extracted from the beacon */
    numaddrshort =
        NUM_SHORT_PEND_ADDR(mac_parse_data.mac_payload_data.\
                            beacon_data.pending_addr_spec);

    numaddrlong =
        NUM_LONG_PEND_ADDR(mac_parse_data.mac_payload_data.\
                           beacon_data.pending_addr_spec);

    /*
     * In all cases (PAN or device) if the payload is not equal to zero
     * or macAutoRequest is false, MLME_BEACON_NOTIFY.indication is
     * generated
     */
    if ((mac_parse_data.mac_payload_data.beacon_data.beacon_payload_len > 0) ||
        (mac_pib_macAutoRequest == FALSE)) {
        mlme_beacon_notify_ind_t *mbni = (mlme_beacon_notify_ind_t *)
                                     BMM_BUFFER_POINTER(((buffer_t *)beacon_p));

        /* The beacon notify indication structure is built */
        mbni->cmdcode       = MLME_BEACON_NOTIFY_INDICATION;
        mbni->BSN           = mac_parse_data.sequence_number;
        mbni->PANDescriptor = pan_desc;
        mbni->PendAddrSpec  = mac_parse_data.mac_payload_data.\
                              beacon_data.pending_addr_spec;

        if ((numaddrshort > 0) || (numaddrlong > 0)) {
            mbni->AddrList = mac_parse_data.mac_payload_data.\
                             beacon_data.pending_addr_list_p;
        }

        mbni->sduLength = mac_parse_data.mac_payload_data.\
                          beacon_data.beacon_payload_len;
        mbni->sdu = mac_parse_data.mac_payload_data.\
                    beacon_data.beacon_payload_p;

        mlme_beacon_notify_ind(beacon_p);
    } else {
        /* Payload is not present, hence the buffer is freed here */
        bmm_buffer_free(beacon_p);
    }


    /* Handling of ancounced broadcast traffic by the parent. */
    if (MAC_SCAN_IDLE == mac_scan_state) {
        /*
         * In case this is a beaconing network, and this node is not scanning,
         * and the FCF indicates pending data thus indicating broadcast data at
         * parent, the node needs to be awake until the received broadcast
         * data has been received.
         */
        if (mac_parse_data.fcf & FCF_FRAME_PENDING) {
            mac_bc_data_indicated = true;

            /*
             * Start timer since the broadcast frame is expected within
             * macMaxFrameTotalWaitTime symbols.
             */
            if (MAC_POLL_IDLE == mac_poll_state) {
                /*
                 * If the poll state is not idle, there is already an
                 * indirect transaction ongoing.
                 * Since the T_Poll_Wait_Time is going to be re-used,
                 * this timer can only be started, if we are not in
                 * a polling state other than idle.
                 */
                uint32_t response_timer = mac_pib_macMaxFrameTotalWaitTime;
                response_timer = HAL_CONVERT_SYMBOLS_TO_US(response_timer) / 1000;

                /*
                 * Schedule a Polling timer to call 
                 * mac_t_wait_for_bc_time_cb at response_timer
                 */
                if (STATUS_SUCCESS != 
                    STM_StartTimer(poll_wait_timer, response_timer)) {
                    mac_poll_wait_time_cb(NULL);
                }
            } else {
                /*
                 * Any indirect poll operation is ongoing, so the timer will
                 * not be started, i.e. nothing to be done here.
                 * Once this ongoing indirect transaction has finished, this
                 * node will go back to sleep anyway.
                 */
            }
        } else {
            mac_bc_data_indicated = false;
        }
    }   /* (MAC_SCAN_IDLE == mac_scan_state) */


    /* Handling of presented indirect traffic by the parent for this node. */
    if (MAC_SCAN_IDLE == mac_scan_state) {
        /*
         * If this node is NOT scanning, and is doing a mlme_sync_request,
         * then the pending address list of the beacon is examined to see
         * if the node's parent has data for this node.
         */
        if (mac_pib_macAutoRequest) {
            if (MAC_SYNC_NEVER != mac_sync_state) {
                uint8_t index;
                uint16_t cur_short_addr;
                uint64_t cur_long_addr;

                /*
                 * Short address of the device is compared with the
                 * pending short address in the beacon frame
                 */

                /*
                 * PAN-ID and CoordAddress does not have to be checked here,
                 * since the device is already synced with the coordinator,
                 * and only beacon frames passed from data_ind.c (where
                 * the first level filtering is already done) are received.
                 * The pending addresses in the beacon frame are compared
                 * with the device address. If a match is found, it indicates
                 * that a data belonging to this deivce is present with the
                 * coordinator and hence a data request is sent to the 
                 * coordinator.
                 */

                for (index = 0; index < numaddrshort; index++) {
                    cur_short_addr = mac_utils_byte_array_to_16_bit(
                                      (mac_parse_data.mac_payload_data.\
                                       beacon_data.pending_addr_list_p +
                                       index * sizeof(uint16_t)));
                    if (cur_short_addr == hal_pib_ShortAddress) {
                        /*
                         * Device short address matches with one of the address
                         * in the beacon address list. Implicit poll (using the
                         * device short address) is done to get the pending
                         * data
                         */
                        mac_data_build_and_tx_data_req(false, false, 0, NULL,
                                                       0);
                        return;
                    }
                }

                /*
                 * Extended address of the device is compared with
                 * the pending extended address in the beacon frame
                 */

                for (index = 0; index < numaddrlong; index++) {
                    cur_long_addr = mac_utils_byte_array_to_64_bit(
                                    (mac_parse_data.mac_payload_data.\
                                     beacon_data.pending_addr_list_p +
                                     numaddrshort * sizeof(uint16_t) +
                                     index * sizeof(uint64_t)));

                    if (EXT_ADDR_MATCH(cur_long_addr,hal_pib_IeeeAddress)) {
                        /*
                         * Device extended address matches with one of the
                         * address in the beacon address list. Implicit poll
                         * (using the device extended address) is done to get
                         * the pending data
                         */
                        mac_data_build_and_tx_data_req(false, true, 0, NULL, 0);
                        return;
                    }
                }
            }
        }   /* (mac_pib_macAutoRequest) */
    }   /* (MAC_SCAN_IDLE == mac_scan_state) */
}
