/**
 * @file mac_scan.c
 *
 * MLME_SCAN primitives.
 *
 * This file implements functions to handle all MLME-SCAN primitives and the
 * corresponding scan options: ED, Active, Passive and Orphan
 *
 * $Id: mac_scan.c,v 1.6 2014/11/26 13:19:41 ranjan Exp $
 *
 * Copyright (c) 2012, Greenvity Communication All rights reserved. 
 *
 */
#ifdef HYBRII_802154

/* === Includes ============================================================ */

#include <stdio.h>
#include <string.h>
#include "papdef.h" 
#include "timer.h"
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
#include "utils_fw.h"
#include "fm.h"

/* === Macros =============================================================== */

/**
 * Scan duration formula: aBaseSuperframeDuration (2^SD + 1)
 * where 0 <= SD <= 14
 */
#define MAC_CALCULATE_SYMBOL_TIME_SCANDURATION(SD) \
    (aBaseSuperframeDuration * ((1UL << (SD)) + 1))

/*
 * Max beacon Order in beacon-enabled network
 */
#define BEACON_NETWORK_MAX_BO              (14)

/*
 * Beacon Request and Orphan Notification command frame payload length
 */
#define BEAC_REQ_ORPH_NOT_PAYLOAD_LEN       (1)


/* === Globals ============================================================= */

static uint32_t scan_channels;
static uint8_t  scan_curr_channel;
static uint8_t  scan_type;
static uint8_t  scan_curr_page;
static uint8_t  scan_duration;

/* === Prototypes ========================================================== */

static void mac_scan_set_complete(retval_t set_status);

/* === Implementation ====================================================== */

/*
 * Check for Scanning is running
 */
bool mac_scan_is_running (void)
{
    if (MAC_SCAN_IDLE == mac_scan_state) {
        return (FALSE);
    }
    return (TRUE);
}

/*
 * Clean-up for scanning
 *
 * This is a helper function for clean-up functionality during the end of
 * scanning.
 *
 * buffer_p - Pointer to mlme_scan_conf_t structure
 */
static void mac_scan_clean_up (buffer_t *buffer_p)
{
    mac_scan_state = MAC_SCAN_IDLE;
    /* Send the scan confirm message into the internal event queue */
#if (defined UM) && (!defined ZBMAC_DIAG)
	mlme_send_to_host(buffer_p);
#else
	mlme_scan_conf(buffer_p);
#endif		

    /* Set original channel page and channel. */
    scan_curr_page = mac_scan_orig_page;

    set_hal_pib_internal(phyCurrentPage, (void *)&scan_curr_page);

    scan_curr_channel = mac_scan_orig_channel;

    set_hal_pib_internal(phyCurrentChannel, (void *)&scan_curr_channel);

    /* Set radio to sleep if allowed */
    mac_trx_sleep();
}

/**
 * Send a beacon request or orphan notification command frame
 *
 * This function sends a beacon request or orphan notification command frame.
 * An MPDU containing either a beacon request or an orphan notification command
 * frame is constructed and sent.
 *
 * beacon_req - True  -> Send beacon request
 *              False -> Send orphan notification command frame
 *
 */
static bool mac_scan_send_scan_cmd (bool beacon_req)
{
    retval_t tx_status;
    uint8_t  frame_len;
    uint8_t  *frame_ptr;
    uint16_t fcf;
    uint16_t bc_addr = BROADCAST;

    /*
     * mac_scan_cmd_buf_ptr holds the buffer allocated for sending beacon
     * request or orphan notification command. In active scan the scan request
     * buffer is used to send a beacon request. In orphan scan new buffer is
     * allocated to send an orphan notification.
     */
    frame_info_t *transmit_frame_p =
        (frame_info_t *)BMM_BUFFER_POINTER((buffer_t*)mac_scan_cmd_buf_ptr);

    /* Get the payload pointer. */
    frame_ptr = (uint8_t *)transmit_frame_p +
                BUFFER_SIZE -
                BEAC_REQ_ORPH_NOT_PAYLOAD_LEN;

    transmit_frame_p->buffer_header_p = (buffer_t *)mac_scan_cmd_buf_ptr;

    if (beacon_req) {
        fcf = FCF_SET_FRAMETYPE(FCF_FRAMETYPE_MAC_CMD) |
              FCF_SET_DEST_ADDR_MODE(FCF_SHORT_ADDR)   |
              FCF_SET_SOURCE_ADDR_MODE(FCF_NO_ADDR);

        frame_len = BEAC_REQ_ORPH_NOT_PAYLOAD_LEN +
                    SHORT_ADDR_LEN                + /* short Dest Address */
                    PAN_ID_LEN                    + /* Dest PAN-Id */
                    FCS_LEN                       + /* DSN  */
                    SEQ_NUM_LEN;                    /* SEQ */

        *frame_ptr = transmit_frame_p->msg_type = BEACONREQUEST;
    } else {
        /* Orphan Notification command */
        fcf = FCF_SET_FRAMETYPE(FCF_FRAMETYPE_MAC_CMD) |
              FCF_SET_DEST_ADDR_MODE(FCF_SHORT_ADDR) |
              FCF_SET_SOURCE_ADDR_MODE(FCF_LONG_ADDR) |
              FCF_PAN_ID_COMPRESSION;

        /* Update the length. */
        frame_len = BEAC_REQ_ORPH_NOT_PAYLOAD_LEN +
                    SHORT_ADDR_LEN                + /* short Dest Address */
                    PAN_ID_LEN                    + /* Dest PAN-Id */
                    EXT_ADDR_LEN                  + /* Ext Src Address */
                    FCS_LEN                       + /* DSN  */
                    SEQ_NUM_LEN;                    /* SEQ */

        *frame_ptr = transmit_frame_p->msg_type = ORPHANNOTIFICATION;

        /* Orphan notification contains long source address. */
        frame_ptr -= EXT_ADDR_LEN;
        mac_utils_64_bit_to_byte_array(hal_pib_IeeeAddress, frame_ptr);
    }

    /* Destination address */
    frame_ptr -= SHORT_ADDR_LEN;
    mac_utils_16_bit_to_byte_array(bc_addr, frame_ptr);

    /* Destination PANid */
    frame_ptr -= PAN_ID_LEN;
    mac_utils_16_bit_to_byte_array(bc_addr, frame_ptr);


    /* Set DSN. */
    frame_ptr--;
    *frame_ptr = mac_pib_macDSN++;


    /* Set the FCF. */
    frame_ptr -= FCS_LEN;
    mac_utils_16_bit_to_byte_array(fcf, frame_ptr);


    /* First element shall be length of PHY frame. */
    frame_ptr--;
    *frame_ptr = frame_len;

    /* Finished building of frame. */
    transmit_frame_p->mpdu_p = frame_ptr;

    /* Transmit data with unslotted CSMA-CA and no frame retry. */
    tx_status = mac_hal_tx_frame(transmit_frame_p, CSMA_UNSLOTTED, false);

    if (MAC_SUCCESS == tx_status) {
        MAC_BUSY();
        return (TRUE);
    } else {
        return (FALSE);
    }
}

/*
 * Proceed with a scan request
 *
 * This function proceeds with the scanning.
 * The current channel is incremented. It checked if the channel belongs to the
 * list of channels to scan. If so, start scanning. If all channels done,
 * send out the MLME_SCAN.confirm message.
 *
 * scan_type - The type of the scan operation to proceed with.
 * buf_p     - Buffer to send mlme scan confirm to NHLE.
 */
static void mac_scan_proceed (uint8_t scan_type, buffer_t *buf_p)
{
    retval_t set_status;
    mlme_scan_conf_t *msc_p = (mlme_scan_conf_t *)BMM_BUFFER_POINTER(buf_p);

    /* Set the channel page to perform scan */
    set_status = set_hal_pib_internal(phyCurrentPage,
                                      (void *)&scan_curr_page);

    /* Loop over all channels the MAC has been requested to scan */
    for (; scan_curr_channel <= MAX_CHANNEL; scan_curr_channel++) {
        if (((MAC_SCAN_ACTIVE == mac_scan_state) ||
             (MAC_SCAN_PASSIVE == mac_scan_state)) &&
              mac_pib_macAutoRequest) {
            /*
             * According to 802.15.4-2006 PAN descriptor are only present
             * in the scan confirm message in case the PIB attribute
             * macAutoRequest is true.
             */
            if (msc_p->ResultListSize >= MAX_PANDESCRIPTORS) {
                break;
            }
        }
        if (MLME_SCAN_TYPE_ORPHAN == scan_type) {
            /*
             * In an orphan scan, terminate if any coordinator
             * realignment packet has been received.
             */
            if (msc_p->ResultListSize) {
                break;
            }
        }

        if ((msc_p->UnscannedChannels & (1UL << scan_curr_channel)) != 0) {
            if (MLME_SCAN_TYPE_ACTIVE == scan_type) {
                mac_scan_state = MAC_SCAN_ACTIVE;
            }
            if (MLME_SCAN_TYPE_PASSIVE == scan_type) {
                mac_scan_state = MAC_SCAN_PASSIVE;
            }
            if (MLME_SCAN_TYPE_ORPHAN == scan_type) {
                mac_scan_state = MAC_SCAN_ORPHAN;
            }

            /* Set the channel to perform scan */
            set_status = set_hal_pib_internal(phyCurrentChannel,
                                              (void *)&scan_curr_channel);

            if (MAC_SUCCESS != set_status) {
                /*
                 * Free the buffer used for sending orphan notification command
                 */
                bmm_buffer_free((buffer_t *)mac_scan_cmd_buf_ptr);

                mac_scan_cmd_buf_ptr = NULL;

                /* Set radio to sleep if allowed */
                mac_trx_sleep();

                msc_p->status = MAC_NO_BEACON;

                /* Orphan scan does not return any list. */
                msc_p->ResultListSize = 0;

                /* Send scan confirm message */
#if (defined UM) && (!defined ZBMAC_DIAG)
				mlme_send_to_host(buf_p);
#else
				mlme_scan_conf(buf_p);
#endif						

                mac_scan_state = MAC_SCAN_IDLE;
             }
            /* Continue scanning, after setting channel */ 
            mac_scan_set_complete(set_status);
            return;
        }
    }

    /* All channels were scanned. The confirm needs to be prepared */
    switch (scan_type) {
    case MLME_SCAN_TYPE_ED:
        msc_p->status = MAC_SUCCESS;
        mac_scan_clean_up(buf_p);
        break;

    case MLME_SCAN_TYPE_ACTIVE:
        /*
         * Free the buffer which was received from scan request and reused
         * for beacon request frame transmission.
         */
        bmm_buffer_free((buffer_t *)mac_scan_cmd_buf_ptr);

        mac_scan_cmd_buf_ptr = NULL;

        if (!mac_pib_macAutoRequest) {
            msc_p->status = MAC_SUCCESS;
        } else if (msc_p->ResultListSize >= MAX_PANDESCRIPTORS) {
            msc_p->status = MAC_LIMIT_REACHED;
        } else if (msc_p->ResultListSize) {
            msc_p->status = MAC_SUCCESS;
        } else {
            msc_p->status = MAC_NO_BEACON;
        }

        /* Restore macPANId after active scan completed. */
        set_hal_pib_internal(macPANId, (void *)&mac_scan_orig_panid);

        /* Done with scanning */
        mac_scan_clean_up((buffer_t *)mac_conf_buf_ptr);
        break;

    case MLME_SCAN_TYPE_PASSIVE:
        if (!mac_pib_macAutoRequest) {
            msc_p->status = MAC_SUCCESS;
        } else if (msc_p->ResultListSize >= MAX_PANDESCRIPTORS) {
            msc_p->status = MAC_LIMIT_REACHED;
        } else if (msc_p->ResultListSize) {
            msc_p->status = MAC_SUCCESS;
        } else {
            msc_p->status = MAC_NO_BEACON;
        }

        /* Restore macPANId after passive scan completed. */
        set_status =
            set_hal_pib_internal(macPANId, (void *)&mac_scan_orig_panid);

        mac_scan_clean_up(buf_p);
        break;


    case MLME_SCAN_TYPE_ORPHAN:
        /* Free the buffer used for sending orphan notification command */
        bmm_buffer_free((buffer_t *)mac_scan_cmd_buf_ptr);

        mac_scan_cmd_buf_ptr = NULL;

        if (msc_p->ResultListSize > 0) {
            msc_p->status = MAC_SUCCESS;
        } else {
            msc_p->status = MAC_NO_BEACON;
        }

        /* Orphan scan does not return any list. */
        msc_p->ResultListSize = 0;

        mac_scan_clean_up(buf_p);
        break;

    default:
        break;
    }
}

/*
 * MAC scan timer callback
 *
 * callback_parameter - Callback parameter.
 */

void mac_scan_duration_cb (void *callback_parameter)
{
    /* mac_conf_buf_ptr holds the buffer for scan confirm */
    mlme_scan_conf_t *msc_p =
        (mlme_scan_conf_t *)BMM_BUFFER_POINTER((buffer_t *)mac_conf_buf_ptr);

    switch (mac_scan_state) {
    case MAC_SCAN_ACTIVE:
    case MAC_SCAN_PASSIVE:
    case MAC_SCAN_ORPHAN: 
        msc_p->UnscannedChannels &= ~(1UL << scan_curr_channel);
        mac_scan_proceed(scan_type, (buffer_t *)mac_conf_buf_ptr);
        break;

    default:
        break;
    }

    callback_parameter = callback_parameter;
}

/**
 * Continue scanning after setting of PIB attributes
 *
 * This functions continues scanning once the corresponding PIB
 * attribute change has been completed depending on the status.
 *
 * set_status Status of the Request to change the PIB attribute
 */
static void mac_scan_set_complete (retval_t set_status)
{
    switch (mac_scan_state) {
    case MAC_SCAN_ED:
        if (MAC_SUCCESS == set_status) {
            MAC_BUSY();
            mac_hal_ed_start(scan_duration);
        } else {
            /* Channel not supported, continue. */
            scan_curr_channel++;
        }
        break;


    case MAC_SCAN_ACTIVE:
        if (MAC_SUCCESS == set_status) {
            /* Send an beacon request command */
            if (!mac_scan_send_scan_cmd(true)) {

                /*
                 * Beacon request could not be transmitted
                 * since there is no buffer available stop scanning
                 */
                //FIXME - Add code to handle this
            }
        } else {
            /* Channel not supported, continue. */
            scan_curr_channel++;
        }
        break;


    case MAC_SCAN_PASSIVE:
        if (MAC_SUCCESS == set_status) {
            uint8_t status = mac_hal_hw_control(PHY_RX_ON);

            if (PHY_RX_ON == status) {
#ifdef RTX51_TINY_OS
                eStatus timer_status;
#endif
                uint32_t tmr;

                tmr = MAC_CALCULATE_SYMBOL_TIME_SCANDURATION(scan_duration);
#ifdef RTX51_TINY_OS
                timer_status = STM_StartTimer(scan_duration_timer,
                                     HAL_CONVERT_SYMBOLS_TO_US(tmr) / 1000);
                if (STATUS_SUCCESS != timer_status) {
                    /*
                     * Scan duration timer could not be started 
                     */
                    //FIXME - Add code to handle this
                }
#endif
            } else {
                scan_curr_channel++;
                mac_scan_proceed(MLME_SCAN_TYPE_PASSIVE,
                                 (buffer_t *)mac_conf_buf_ptr);
            }
        } else {
            /* Channel not supported, continue. */
            scan_curr_channel++;
        }
        break;


    case MAC_SCAN_ORPHAN:
        if (MAC_SUCCESS == set_status) {
            /* Send an orphan notification command */
            if (!mac_scan_send_scan_cmd(false)) {
                /*
                 * Orphan notification could not be transmitted. 
                 * Stop scanning
                 */
                //FIXME - Add code to handle this
            }
        } else {
            /* Channel not supported, continue. */
            scan_curr_channel++;
        }
        break;

    default:
        break;
    }
}

/*
 * Continues handling of MLME_SCAN.request once the radio is awake
 *
 * scan_buf_p - Pointer to Scan request buffer.
 */
static void mac_scan_awake_scan (buffer_t *scan_buf_p)
{
    mlme_scan_conf_t *msc_p;
    uint16_t broadcast_panid = BROADCAST;
    retval_t set_status;

    msc_p = (mlme_scan_conf_t *)BMM_BUFFER_POINTER(scan_buf_p);

    /* Set the first channel at which the scan is started */
    scan_curr_channel = MIN_CHANNEL;

    switch (scan_type) {
    case MLME_SCAN_TYPE_ED:
        msc_p->scan_result_list[0].ed_value[1] = 0;
        mac_scan_state = MAC_SCAN_ED; 
        mac_scan_proceed(MLME_SCAN_TYPE_ED, (buffer_t *)scan_buf_p);
        break;

    case MLME_SCAN_TYPE_ACTIVE:
    case MLME_SCAN_TYPE_PASSIVE:
        /*
         * Before commencing an active or passive scan, the MAC sublayer
         * shall store the value of macPANId and then set it to 0xFFFF for
         * the duration of the scan. This enables the receive filter to
         * accept all beacons rather than just the beacons from its
         * current PAN (see 7.5.6.2). On completion of the scan, the
         * MAC sublayer shall restore the value of macPANId to the
         * value stored before the scan began.
         */
        mac_scan_orig_panid = hal_pib_PANId;

        set_status =
            set_hal_pib_internal(macPANId, (void *)&broadcast_panid);

        if (MLME_SCAN_TYPE_ACTIVE == scan_type) {
            /*
             * In active scan reuse the scan request buffer for
             * sending beacon request.
             */
            mac_scan_cmd_buf_ptr = (uint8_t *)scan_buf_p;
        }

        /* Allocate a large size buffer for scan confirm. */
        mac_conf_buf_ptr = (uint8_t *)bmm_buffer_alloc(BUFFER_SIZE);

        if (NULL == mac_conf_buf_ptr) {
            /*
             * Buffer is not available for sending scan confirmation,
             * hence the scan request buffer (small buffer) is used to send
             * the scan confirmation.
             */
            FM_Printf(FM_APP, "\nba:scan2F");
            msc_p->status = MAC_INVALID_PARAMETER;

            /* Send scan confirm message */
#if (defined UM) && (!defined ZBMAC_DIAG)
			mlme_send_to_host(scan_buf_p);
#else
			mlme_scan_conf(scan_buf_p);
#endif			

            /* Set radio to sleep if allowed */
            mac_trx_sleep();
            return;
        }

        if (MLME_SCAN_TYPE_PASSIVE == scan_type) {
            /* Free the scan request buffer when in passive scan. */
            bmm_buffer_free(scan_buf_p);
        }

        msc_p = (mlme_scan_conf_t *)
                BMM_BUFFER_POINTER((buffer_t *)mac_conf_buf_ptr);

        msc_p->cmdcode = MLME_SCAN_CONFIRM;
        msc_p->ScanType = scan_type;
        msc_p->ChannelPage = scan_curr_page;
        msc_p->UnscannedChannels = scan_channels;
        msc_p->ResultListSize = 0;
        msc_p->scan_result_list[0].ed_value[0] = 0;
 
        mac_scan_proceed(scan_type, (buffer_t *)mac_conf_buf_ptr);
        break;

    case MLME_SCAN_TYPE_ORPHAN:
        /* Buffer allocated for orphan notification command */
        mac_scan_cmd_buf_ptr = (uint8_t *)bmm_buffer_alloc(BUFFER_SIZE);

        if (NULL == mac_scan_cmd_buf_ptr) {
			FM_Printf(FM_APP, "\nba:scanF");
            msc_p->status = MAC_INVALID_PARAMETER;

            /* Send scan confirm message */
#if (defined UM) && (!defined ZBMAC_DIAG)
			mlme_send_to_host(scan_buf_p);
#else
			mlme_scan_conf(scan_buf_p);
#endif			

            /* Set radio to sleep if allowed */
            mac_trx_sleep();
            return;
        }

        mac_scan_proceed(MLME_SCAN_TYPE_ORPHAN,
                         (buffer_t *)mac_conf_buf_ptr);
        break;


    default:
        msc_p->status = MAC_INVALID_PARAMETER;
        /* Send scan confirm message */
#if (defined UM) && (!defined ZBMAC_DIAG)
		mlme_send_to_host(scan_buf_p);
#else
		mlme_scan_conf(scan_buf_p);
#endif			

        /* Set radio to sleep if allowed */
        mac_trx_sleep();
        break;
    }
}

/**
 * Continue scanning after the completion of frame transmission.
 *
 * This functions continues the corresponding scaning depending on status
 * from the transmission of a beacon request or orphan notification frame.
 *
 * status - Status of transmission
 */
void mac_scan_send_complete (retval_t status)
{
#ifdef RTX51_TINY_OS
    eStatus timer_status;
#endif

    mac_pib_macDSN++;

    if (MAC_SUCCESS == status) {
        uint32_t tmr = 0;

        if (MAC_SCAN_ACTIVE == mac_scan_state) {
            tmr = MAC_CALCULATE_SYMBOL_TIME_SCANDURATION(scan_duration);
        } else {
            tmr = mac_pib_macResponseWaitTime;
        }
#ifdef RTX51_TINY_OS
        timer_status = STM_StartTimer(scan_duration_timer,
                                      HAL_CONVERT_SYMBOLS_TO_US(tmr) / 1000);
        if (STATUS_SUCCESS != timer_status) {
            /*
             * Scan duration timer could not be started, so we call
             * the timer callback function directly. This will basically
             * shorten scanning without having really scanned.
             */           
            mac_scan_duration_cb(NULL);
        }
#endif
    } else {
        /* Did not work, continue. */
        scan_curr_channel++;
        mac_scan_proceed(scan_type, (buffer_t *)mac_conf_buf_ptr);
    }
}


/**
 * The MLME-SCAN.request primitive makes a request for a node to
 * start a scan procedure.
 *
 * See 802.15.4. Section 7.1.11.1.
 *
 * msg_p - The MLME_SCAN.request message
 */
void mlme_scan_request (buffer_t *buf_p)
{
    mlme_scan_conf_t *msc_p;
    mlme_scan_req_t *msr_p =
        (mlme_scan_req_t *)BMM_BUFFER_POINTER(buf_p);
 
    /* Save the original channel. */
    mac_scan_orig_channel = hal_pib_CurrentChannel;

    /* Save the original channel page. */
    mac_scan_orig_page = hal_pib_CurrentPage;

    /* Save the scan request parameters */
    scan_duration = msr_p->ScanDuration;
    scan_type = msr_p->ScanType;
    scan_channels = msr_p->ScanChannels;
    scan_curr_page = msr_p->ChannelPage;

    msc_p = (mlme_scan_conf_t *)msr_p;

    /*
     * Store the scan request buffer reused to create the corresponding
     * scan confirmation
     */
    mac_conf_buf_ptr = (uint8_t *)buf_p;

    msc_p->cmdcode = MLME_SCAN_CONFIRM;
    msc_p->ScanType = scan_type;
    msc_p->UnscannedChannels = scan_channels;
    msc_p->ChannelPage = scan_curr_page;
    msc_p->ResultListSize = 0;
    msc_p->scan_result_list[0].ed_value[0] = 0;

    if ((MAC_POLL_IDLE != mac_poll_state) ||
        (MAC_SCAN_IDLE != mac_scan_state)) {
        /* Ignore scan request while being in a polling state or scanning. */
        msc_p->status = MAC_INVALID_PARAMETER;

        /* Send Scan Confirm with error status */
#if (defined UM) && (!defined ZBMAC_DIAG)
		mlme_send_to_host(buf_p);
#else
		mlme_scan_conf(buf_p);
#endif			

        return;
    }

    /*
     * Check for invalid channels to scan.
     * This can be either an emtpy scan mask, or a scan mask that contains
     * invalid channels for this band.
     */
    /*
     * Checck also for a scan duration that is lower than
     * the max. beacon order.
     */
    if ((0 == scan_channels) ||
        ((scan_channels & INVERSE_CHANNEL_MASK) != 0) ||
        (scan_duration > BEACON_NETWORK_MAX_BO)) {
        msc_p->status = MAC_INVALID_PARAMETER;

        /* Send the scan confirm message */
#if (defined UM) && (!defined ZBMAC_DIAG)
		mlme_send_to_host(buf_p);
#else
		mlme_scan_conf(buf_p);
#endif	

        return;
    }

    /* wake up radio first */
    mac_trx_wakeup();
    mac_hal_hw_control(PHY_RX_ON);
 
    mac_scan_awake_scan(buf_p);
}


/**
 * ED scan callback function.
 *
 * This function is a callback function from the HAL after ED scan
 * is performed on a specified channel.
 *
 * energy_level - Energy level on the channel
 */
void mac_scan_ed_end_cb (uint8_t energy_level)
{

    mlme_scan_conf_t *msc_p;
    uint8_t n_eds;

    MAC_NOT_BUSY();

    /*
     * Scan request buffer is used to generate a scan confirm for the ED scan
     * which is stored in mac_conf_buf_ptr.
     */
    msc_p = (mlme_scan_conf_t *)
            BMM_BUFFER_POINTER((buffer_t *)mac_conf_buf_ptr);


    n_eds = msc_p->ResultListSize;
    msc_p->scan_result_list[0].ed_value[n_eds] = energy_level;
    msc_p->ResultListSize++;
    msc_p->scan_result_list[0].ed_value[n_eds + 1] = 0;
    msc_p->UnscannedChannels &= ~(1UL << scan_curr_channel);

    /* Continue with next channel */
    mac_scan_proceed(MLME_SCAN_TYPE_ED, (buffer_t *)mac_conf_buf_ptr);
}


/**
 * Processing a coordinator realignment command frame during Orphan scan
 *
 * This function processes a coordinator realignment command frame received
 * as a response to the reception of an orphan notification
 * command frame (i.e. while being in the middle of an orphan scan procedure).
 * The PAN ID, coord. short address, logical channel, and the device's new
 * short address will be written to the PIB.
 *
 * buf_ptr - Frame reception buffer
 */
void mac_scan_process_orphan_realign (buffer_t *buf_ptr)
{
    retval_t set_status;
    mlme_scan_conf_t *msc_p;

    /* Device received a coordinator realignment during an orphan scan */

    /* Free the buffer used for sending orphan notification command */
    bmm_buffer_free((buffer_t *)mac_scan_cmd_buf_ptr);
    mac_scan_cmd_buf_ptr = NULL;

    /*
     * Scan confirm with scan type orphan is given to the NHLE using the
     * scan request buffer, which was stored in mac_conf_buf_ptr.
     */
    msc_p = (mlme_scan_conf_t*)BMM_BUFFER_POINTER((buffer_t *)mac_conf_buf_ptr);

    msc_p->cmdcode = MLME_SCAN_CONFIRM;
    msc_p->status = MAC_SUCCESS;
    msc_p->ScanType = MLME_SCAN_TYPE_ORPHAN;
    msc_p->UnscannedChannels = 0;
    msc_p->ResultListSize = 0;


    /* Send the scan confirmation message */ 
#if (defined UM) && (!defined ZBMAC_DIAG)
	mlme_send_to_host((buffer_t *)mac_conf_buf_ptr);
#else
	mlme_scan_conf((buffer_t *)mac_conf_buf_ptr);
#endif		

    mac_scan_state = MAC_SCAN_IDLE;

    /* Set radio to sleep if allowed */
    mac_trx_sleep();

    /*
     * The buffer in which the coordinator realignment is received is
     * freed up
     */
    bmm_buffer_free(buf_ptr);

    /* Set the appropriate PIB entries */
    set_status =
        set_hal_pib_internal(macPANId, (void *)
            &mac_parse_data.mac_payload_data.coord_realign_data.pan_id);

    if (BROADCAST !=
        mac_parse_data.mac_payload_data.coord_realign_data.short_addr) {
        /* Short address only to be set if not broadcast address */
        set_status = 
        set_hal_pib_internal(macShortAddress, (void *)
            &mac_parse_data.mac_payload_data.coord_realign_data.short_addr);
    }

    mac_pib_macCoordShortAddress =
        mac_parse_data.mac_payload_data.coord_realign_data.coord_short_addr;

    /*
     * If frame version subfield indicates a 802.15.4-2006 compatible frame,
     * the channel page is appended as additional information element.
     */
    if (mac_parse_data.fcf & FCF_FRAME_VERSION_2006) {
        set_status =
        set_hal_pib_internal(phyCurrentPage, (void *)
            &mac_parse_data.mac_payload_data.coord_realign_data.channel_page);
    }

    set_status =
    set_hal_pib_internal(phyCurrentChannel, (void *)
        &mac_parse_data.mac_payload_data.coord_realign_data.logical_channel);

    mac_scan_set_complete(set_status);
}

#endif //HYBRII_802154
