/**
 * @file
 *
 * Implement MLME-RX-ENABLE functionality
 *
 * $Id: mac_rx_enable.c,v 1.6 2014/11/26 13:19:41 ranjan Exp $
 *
 * Copyright (c) 2012, Greenvity Communication All rights reserved.
 *
 */
#ifdef HYBRII_802154

/* === Includes ============================================================ */

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


/* === Macros =============================================================== */


/* === Globals ============================================================= */

/* === Prototypes ========================================================== */

/* === Implementation ====================================================== */

/*
 * @brief Set the transceiver state to PHY_TRX_OFF
 *
 * This actually turns the radio receiver off - i.e. this is the end
 * of the PHY_RX_ON period.
 *
 * @param callback_parameter Callback parameter
 */
static void mac_rx_enable_off (void)
{
    uint8_t status;
 
    /*
     * In case macRxOnWhenIdle is not set, Set h/w to PHY_TRX_OFF
     * state
     */
    if (FALSE == mac_pib_macRxOnWhenIdle) {
        mac_rx_enabled = false;
        status = mac_hal_hw_control(PHY_TRX_OFF);
        return;
    }
}

/*
 * Initiate rx enable confirm message.
 *
 * This function creates the rx enable confirm structure,
 * and appends it into internal event queue.
 *
 * buf_p Pointer to buffer for rx enable confirmation.
 * status Status of attempt to switch receiver on.
 */
static void mac_rx_enable_conf (buffer_t *buf_p, uint8_t status)
{
    mlme_rx_enable_conf_t *rec = (mlme_rx_enable_conf_t *)
                                 BMM_BUFFER_POINTER(buf_p);

    rec->cmdcode = MLME_RX_ENABLE_CONFIRM;
    rec->status = status;
	
#if (defined UM) && (!defined ZBMAC_DIAG)
	mlme_send_to_host(buf_p);
#else
	mlme_rx_enable_conf(buf_p);
#endif		
}

/*
 * Beacon Network RX Enable
 *
 * This function immediately enables the receiver with the given
 * RxOnDuration time in symbols from now.
 *
 * rx_on_duration_symbols: Duration in symbols that the reciever is turned on
 */
static void mac_rx_enable_on_bc (mlme_rx_enable_req_t *rxe_p,
                                 buffer_t *buf_p)
{
    bool status;

    /*
     * Determine if (RxOnTime + RxOnDuration) is less than the beacon
     * interval.
     * According to 7.1.10.1.3:
     * On a beacon-enabled PAN, the MLME first determines whether
     * (RxOnTime + RxOnDuration) is less than the beacon interval, defined
     * by macBeaconOrder. If it is not less, the MLME issues the
     * MLME-RX-ENABLE.confirm primitive with a status of 
     * MAC_INVALID_PARAMETER.
     */

    if ((rxe_p->RxOnTime + rxe_p->RxOnDuration) >= 
         HAL_GET_BEACON_INTERVAL_TIME(hal_pib_BeaconOrder)) {
        /* Send the confirm immediately. */
        mac_rx_enable_conf(buf_p, MAC_INVALID_PARAMETER);
        return;
    }
    /* Rx is enabled */
    mac_rx_enabled = TRUE;
    status = mac_hal_rx_enable(TRUE, rxe_p); /* For BC network */

    if (status == TRUE) {
        mac_rx_enable_conf(buf_p, MAC_SUCCESS);
    } else {
        mac_rx_enable_conf(buf_p, MAC_INVALID_PARAMETER);
    }
}

/*
 * Non Beacon Network RX Enable
 *
 * This function immediately enables the receiver with the given
 * RxOnDuration time in symbols from now.
 *
 * rx_on_duration_symbols: Duration in symbols that the reciever is turned on
 */
static void mac_rx_enable_on_non_bc (mlme_rx_enable_req_t *rxe_p,
                                     buffer_t *buf_p)
{
    /* Rx is enabled */
    mac_rx_enabled = TRUE;
    mac_hal_rx_enable(FALSE, rxe_p);  /* For non BC network */
    mac_rx_enable_conf(buf_p, MAC_SUCCESS);
}

/*
 * The MLME-RX-ENABLE.request primitive is generated by the next
 * higher layer and issued to MAC to enable the receiver for a
 * fixed duration, at a time relative to the start of the current or
 * next superframe on a beacon-enabled PAN or immediately on a
 * nonbeacon-enabled PAN. The receiver is enabled exactly once per
 * primitive request.
 *
 * buf_ptr Pointer to the MLME-RX-ENABLE.request message
 */
void mlme_rx_enable_request (buffer_t *buf_ptr)
{
    mlme_rx_enable_req_t *rxe_p;

    rxe_p = (mlme_rx_enable_req_t *)BMM_BUFFER_POINTER(buf_ptr);

    /* If RxOnDuration is zero, the receiver shall be disabled */
    if (0 == rxe_p->RxOnDuration) {
        /*
         * Turn off the RX
         */
        mac_rx_enable_off(); 
        /* Send the confirm immediately. */
        mac_rx_enable_conf(buf_ptr, MAC_SUCCESS);
        return;
    }
    /*
     * Reject the request when the MAC is currently in any of the
     * polling states or scanning.
     */
    if ((MAC_POLL_IDLE != mac_poll_state) ||
        (MAC_SCAN_IDLE != mac_scan_state)) {
        /* Send the confirm immediately. */
        mac_rx_enable_conf(buf_ptr, MAC_TX_ACTIVE);
        return;
    }

    if (NON_BEACON_NWK == hal_pib_BeaconOrder) {
        mac_rx_enable_on_non_bc(rxe_p, buf_ptr); 
    } else {
        mac_rx_enable_on_bc(rxe_p, buf_ptr);
    }
}

#endif //HYBRII_802154

