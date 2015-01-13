/**
 * @file mac_start.c
 *
 * This file implements the MLME-START.request
 *
 * $Id: mac_start.c,v 1.1 2014/01/10 17:27:11 yiming Exp $
 *
 * Copyright (c) 2011, Greenvity Communication All rights reserved.
 *
 */

/* === Includes ============================================================ */

#include <string.h>
#include "papdef.h"
#include "return_val.h"
#include "bmm.h"
#include "qmm.h"
#include "mac_msgs.h"
#include "mac_hal.h"
#include "mac_const.h"
#include "mac_api.h"
#include "mac_data_structures.h"
#include "mac_internal.h"
#include "mac.h"
#include "utils.h"

/* === Macros =============================================================== */
/*
 * Coordinator realignment payload length
 */
/*
 * In 802.15.4-2006 the channel page may be added, if the new channel page is
 * different than the original value. In order to simplify the code, it
 * is always added.
 */
#define COORD_REALIGN_PAYLOAD_LEN       (9)

/* === Globals ============================================================== */

static mlme_start_req_t msr_params;    /* Intermediate start parameters */

/* === Prototypes =========================================================== */


/* === Implementation ======================================================= */

/*
 * Generate confirmation for MLME_START.request
 *
 * buf_p            - Pointer to MLME_START.request
 * start_req_status - Status of the MLME_START.request
 */
static void gen_mlme_start_conf (buffer_t *buf_p,
                                 uint8_t start_req_status)
{

    /* Use the same start request buffer for start confirm */
    mlme_start_conf_t *msc_p = (mlme_start_conf_t *)BMM_BUFFER_POINTER(buf_p);

    msc_p->cmdcode = MLME_START_CONFIRM;
    msc_p->status = start_req_status;

    mlme_start_conf(buf_p);
}



/*
 * Checks the parameters of a MLME_START.request
 *
 * This function validates the parameters of the MLME_START.request.
 *
 * msg_p - Pointer to the MLME_START.request message which holds the
 * start parameters
 *
 * return true if parameters are valid, false otherwise
 */
static bool check_start_parameter (mlme_start_req_t *msg_p)
{
    bool param_validation_status = false;

    /*
     * Value of BO has to be less than or equal to 15 (Non beacon
     * Network). The value of SO has to be either greater than or equal to
     * BO.
     */
    if ((msg_p->BeaconOrder <= NON_BEACON_NWK) &&
        ((msg_p->SuperframeOrder <= msg_p->BeaconOrder) ||
         (NON_BEACON_NWK == msg_p->SuperframeOrder))) {
        /*
         * Coordinator realignment command can only be given by a
         * PAN coordinator or cordinator (which is associated to a
         * PAN coordinator).
         */
        if ((msg_p->CoordRealignment) &&
            ((MAC_ASSOCIATED == mac_state) || (MAC_IDLE == mac_state))) {
            /*
             * We are neigher the requested to be the PAN Coordinator,
             * nor are we associated, so Realignment is not allowed at
             * this stage.
             */
            param_validation_status = false;
        } else {
            param_validation_status = true;
        }
    }

    return (param_validation_status);
}

/**
 *
 * This function is called either in response to the reception of an orphan
 * notification command from a device (cmd_type = ORPHANREALIGNMENT),
 * or gratuitously whenever the PAN parameters are about to be changed
 * (cmd_type = COORDINATORREALIGNMENT). In the former case, the
 * paramater contains a pointer to the respective MLME_ORPHAN.response message,
 * while in the latter case this parameter is unused, and can be passed as NULL.
 *
 * cmd_type    - Determines directed or broadcast mode
 * buf_p       - Pointer to the buffer, using which coord_realignment_command
 *               to be sent
 * new_panid   - Contains the new PAN-ID in case there is a network realignment
 * new_channel - Contains the new channel in case there is a network
 *               realignment
 * new_page    - Contains the new channel page in case there is a network
 *               realignment
 *
 * return true if coord_realignment_command is sent successfully,
 *        false otherwise
 */
bool mac_tx_coord_realignment_command (frame_msgtype_t cmd_type,
                                       buffer_t *buf_p,
                                       uint16_t new_panid,
                                       uint8_t new_channel,
                                       uint8_t new_page)
{
    retval_t           tx_status;
    uint8_t            frame_len;
    uint8_t            *frame_ptr;
    uint8_t            *temp_frame_ptr;
    uint16_t           fcf;
    uint16_t           bc_addr = BROADCAST;
    mlme_orphan_resp_t orphan_resp;
    frame_info_t       *coord_realignment_frame_p;

    /*
     * Orphan request is reused to send coordinator realignment
     * command frame and finally to send comm-status-indication
     */
    memcpy(&orphan_resp,
           (mlme_orphan_resp_t *)BMM_BUFFER_POINTER(buf_p),
           sizeof(mlme_orphan_resp_t));

    coord_realignment_frame_p = (frame_info_t *)BMM_BUFFER_POINTER(buf_p);

    coord_realignment_frame_p->msg_type = cmd_type;
    coord_realignment_frame_p->buffer_header_p = buf_p;

    /* Get the payload pointer. */
    frame_ptr = temp_frame_ptr =
                (uint8_t *)coord_realignment_frame_p +
                BUFFER_SIZE -
                COORD_REALIGN_PAYLOAD_LEN;

    /* Update the payload field. */
    *frame_ptr++ = COORDINATORREALIGNMENT;

    /*
     * The payload of the frame has the parameters of the new PAN
     * configuration
     */
    mac_utils_16_bit_to_byte_array(new_panid, frame_ptr);
    frame_ptr += PAN_ID_LEN;
    mac_utils_16_bit_to_byte_array(hal_pib_ShortAddress, frame_ptr);
    frame_ptr += SHORT_ADDR_LEN;

    *frame_ptr++ = new_channel;

    /*
     * Insert the device's short address, or 0xFFFF if this is a
     * gratuitous realigment.
     */
    if (ORPHANREALIGNMENT == cmd_type) {
        mac_utils_16_bit_to_byte_array(orphan_resp.ShortAddress, frame_ptr);
    } else {
        mac_utils_16_bit_to_byte_array(BROADCAST, frame_ptr);
    }
    frame_ptr += SHORT_ADDR_LEN;

    /* Add channel page no matter if it changes or not. */
    *frame_ptr++ = new_page;

    /* Get the payload pointer again to add the MHR. */
    frame_ptr = temp_frame_ptr;

    /* Update the length. */
    frame_len = COORD_REALIGN_PAYLOAD_LEN +
                FCF_LEN                   +
                PAN_ID_LEN                + // Destination PAN-ID
                SHORT_ADDR_LEN            +
                PAN_ID_LEN                + // Source PAN-ID
                EXT_ADDR_LEN              + // Long Source Address
                SEQ_NUM_LEN               +
                FCF_LEN;


    /* Source address */
    frame_ptr -= EXT_ADDR_LEN;
    mac_utils_64_bit_to_byte_array(hal_pib_IeeeAddress, frame_ptr);

    /* Source PAN-Id */
    frame_ptr -= PAN_ID_LEN;
    mac_utils_16_bit_to_byte_array(hal_pib_PANId, frame_ptr);


    /* Destination Address and FCF */
    if (ORPHANREALIGNMENT == cmd_type) {
        /*
         * Coordinator realignment in response to an orphan
         * notification command received from a device. This is always
         * sent to a 64-bit device address, and the device is
         * requested to acknowledge the reception.
         */
        fcf = FCF_SET_SOURCE_ADDR_MODE(FCF_LONG_ADDR)  |
              FCF_SET_DEST_ADDR_MODE(FCF_LONG_ADDR)    |
              FCF_SET_FRAMETYPE(FCF_FRAMETYPE_MAC_CMD) |
              FCF_ACK_REQUEST                          |
              FCF_FRAME_VERSION_2006;

        frame_ptr -= EXT_ADDR_LEN;
        frame_len += 6; // Add further 6 octets for long Destination Address
        mac_utils_64_bit_to_byte_array(orphan_resp.OrphanAddress,
                                           frame_ptr);
    } else {
        /*
         * Coordinator realignment gratuitously sent when the PAN
         * configuration changes. This is sent to the (16-bit)
         * broadcast address.
         */
        fcf = FCF_SET_SOURCE_ADDR_MODE(FCF_LONG_ADDR)  |
              FCF_SET_DEST_ADDR_MODE(FCF_SHORT_ADDR)   |
              FCF_SET_FRAMETYPE(FCF_FRAMETYPE_MAC_CMD) |
              FCF_FRAME_VERSION_2006;
        /*
         * Since the channel page is always added at the end of the
         * coordinator realignment command frame, the frame version subfield
         * needs to indicate a 802.15.4-2006 compatible frame.
         */

        frame_ptr -= SHORT_ADDR_LEN;  /* BC Addr is short */
        mac_utils_16_bit_to_byte_array(bc_addr, frame_ptr);
    }

    /* Destination PAN-Id */
    frame_ptr -= PAN_ID_LEN;
    mac_utils_16_bit_to_byte_array(bc_addr, frame_ptr);


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
    coord_realignment_frame_p->mpdu_p = frame_ptr;

    if (NON_BEACON_NWK == hal_pib_BeaconOrder) {
        /* In Nonbeacon network the frame is sent with unslotted CSMA-CA. */
        tx_status = mac_hal_tx_frame(coord_realignment_frame_p,
                                     CSMA_UNSLOTTED, FALSE);
    } else {
        /* Beacon-enabled network */
        if (ORPHANREALIGNMENT == cmd_type) {
            /* 
             * In Beacon network the Orphan Realignment frame is sent 
             * with slotted CSMA-CA.
             */
            tx_status = mac_hal_tx_frame(coord_realignment_frame_p,
                                         CSMA_SLOTTED, FALSE);
        } else {
            /*
             * Coordinator Realignment frame is sent to broadcast address,
             * so it needs to be appended at the end of the broadcast queue.
             */
            if (FALSE == qmm_queue_append(&broadcast_q, buf_p)) {
                return (false);
            }
            return (true);
        }
    }

    if (MAC_SUCCESS == tx_status) {
        MAC_BUSY();
        return (true);
    } else {
        return (false);
    }
}

/**
 * The MLME-START.request primitive makes a request for the device to
 * start using a new superframe configuration
 *
 * buf_p - Pointer to MLME_START.request message issued by the NHLE
 */
void mlme_start_request (buffer_t *buf_p)
{
    mlme_start_req_t *msg_p = (mlme_start_req_t *)BMM_BUFFER_POINTER(buf_p);

    /*
     * The MLME_START.request parameters are copied into a global variable
     * structure, which is used by check_start_parameter() function.
     */
    memcpy(&msr_params, msg_p, sizeof(msr_params));

    if (BROADCAST == hal_pib_ShortAddress) {
        /*
         * This device does not have a valid short address to
         * start a network.
         */
        gen_mlme_start_conf(buf_p, MAC_NO_SHORT_ADDRESS);
        return;
    }

    if (check_start_parameter(msg_p) == false) {
        /*
         * The MLME_START.request parameters are invalid, hence confirmation
         * is given to NHLE.
         */
        gen_mlme_start_conf(buf_p, MAC_INVALID_PARAMETER);
    } else {
        /*
         * All the start parameters are valid, hence MLME_START.request can
         * proceed.
         */
        mac_hal_pib_set(I_AM_PAN_COORDINATOR, (void *)&(msg_p->PANCoordinator));

        if (msr_params.CoordRealignment) {
            /* First inform our devices of the configuration change */
            if (!mac_tx_coord_realignment_command(COORDINATORREALIGNMENT,
                                                  buf_p,
                                                  msr_params.PANId,
                                                  msr_params.LogicalChannel,
                                                  msr_params.ChannelPage)) {
                /*
                 * The coordinator realignment command was unsuccessful,
                 * hence the confiramtion is given to NHLE.
                 */
                gen_mlme_start_conf(buf_p, MAC_INVALID_PARAMETER);
            }
        } else {
            /* This is a normal MLME_START.request. */
            retval_t channel_set_status, channel_page_set_status;

            mac_hal_pib_set(macBeaconOrder, (void *)&(msg_p->BeaconOrder));

            /*
             * If macBeaconOrder is equal to 15, set also 
             * macSuperframeOrder to 15
             */
            if (msg_p->BeaconOrder == NON_BEACON_NWK) {
                msg_p->SuperframeOrder = NON_BEACON_NWK;
            }

            mac_hal_pib_set(macSuperframeOrder,
                               (void *)&(msg_p->SuperframeOrder));

            /*
             * Symbol times are calculated according to the new BO and SO
             * values.
             */
            if (hal_pib_BeaconOrder < NON_BEACON_NWK) {
                mac_hal_pib_set(macBattLifeExt,
                                (void *)&(msr_params.BatteryLifeExtension));
            }

            /* Wake up radio first */
            mac_trx_wakeup();

            mac_hal_pib_set(macPANId, (void *)&(msr_params.PANId));

            channel_page_set_status =
                mac_hal_pib_set(phyCurrentPage,
                                (void *)&(msr_params.ChannelPage));

            channel_set_status =
                mac_hal_pib_set(phyCurrentChannel,
                                (void *)&(msr_params.LogicalChannel));

            if ((MAC_SUCCESS == channel_page_set_status) &&
                (MAC_SUCCESS == channel_set_status) &&
                (PHY_RX_ON == mac_hal_hw_control(PHY_RX_ON))) {
                if (msr_params.PANCoordinator) {
                    mac_state = MAC_PAN_COORD_STARTED;
                } else {
                    mac_state = MAC_COORDINATOR;
                }

                gen_mlme_start_conf(buf_p, MAC_SUCCESS);

                /*
                 * In case we have a beaconing network, the beacon timer needs
                 * to be started now.
                 */
                if (hal_pib_BeaconOrder != NON_BEACON_NWK) {
                    mac_start_beacon_timer();
                }
            } else {
                /* Start of network failed. */
                gen_mlme_start_conf(buf_p, MAC_INVALID_PARAMETER);
            }

            /* Set radio to sleep if allowed */
            mac_trx_sleep();
        }
    }
}



/**
 * Continues handling of MLME_START.request (Coordinator realignment)
 * command
 *
 * This function is called once the coordinator realignment command is
 * sent out to continue the handling of the MLME_START.request command.
 *
 * tx_status - Status of the coordinator realignment command transmission
 * buf_ptr   - Buffer for start confirmation
 */
void mac_coord_realignment_command_tx_success (uint8_t tx_status,
                                               buffer_t *buf_p)
{
    uint8_t conf_status = MAC_INVALID_PARAMETER;
    uint8_t cur_beacon_order;
    retval_t channel_set_status, channel_page_set_status;

    if (MAC_SUCCESS == tx_status) {
        /* The parameters of the existing PAN are updated. */
        channel_page_set_status =
            mac_hal_pib_set(phyCurrentPage, (void *)&msr_params.ChannelPage);

        channel_set_status =
            mac_hal_pib_set(phyCurrentChannel,
                            (void *)&msr_params.LogicalChannel);

        if ((MAC_SUCCESS == channel_set_status) &&
             (MAC_SUCCESS == channel_page_set_status)) {
            conf_status = MAC_SUCCESS;

            mac_hal_pib_set(macPANId,
                               (void *)&(msr_params.PANId));

            /*
             * Store current beacon order in order to be able to detect
             * switching from nonbeacon to beacon network.
             */
            cur_beacon_order = hal_pib_BeaconOrder;

            mac_hal_pib_set(macBeaconOrder, (void *)&msr_params.BeaconOrder);
            mac_hal_pib_set(macSuperframeOrder,
                            (void *)&msr_params.SuperframeOrder);

            /*
             * New symbol times for beacon time (in sysbols) and inactive 
             * time are calculated according to the new superframe
             * configuration.
             */
            if (msr_params.BeaconOrder < NON_BEACON_NWK) {
                mac_hal_pib_set(macBattLifeExt,
                                (void *)&msr_params.BatteryLifeExtension);
            }

            if ((NON_BEACON_NWK < cur_beacon_order) &&
                (msr_params.BeaconOrder == NON_BEACON_NWK)) {
                /*
                 * This is a transition from a beacon enabled network to
                 * a nonbeacon enabled network.
                 * In this case the broadcast data queue will never be served.
                 *
                 * Therefore the broadcast queue needs to be emptied.
                 * The standard does not define what to do now.
                 * The current implementation will try to send all 
                 * pending broadcast data frames immediately, thus giving
                 * the receiving nodes a chance receive them.
                 */
                while (broadcast_q.size > 0) {
                    mac_tx_pending_bc_data();
                }
            }

            if ((NON_BEACON_NWK == cur_beacon_order) &&
                (msr_params.BeaconOrder < NON_BEACON_NWK)) {
                /*
                 * This is a transition from a nonbeacon enabled network to
                 * a beacon enabled network, hence the beacon timer will be
                 * started.
                 */
                mac_start_beacon_timer();
            }
        }
    }

    gen_mlme_start_conf(buf_p, conf_status);

    /* Set radio to sleep if allowed */
    mac_trx_sleep();
}
