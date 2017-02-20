/**
 * @file
 *
 * Implement MLME-RESET, MLME-RX-ENABLE
 *
 * $Id: mac_misc.c,v 1.6 2014/11/26 13:19:41 ranjan Exp $
 *
 * Copyright (c) 2012, Greenvity Communication All rights reserved.
 *
 */
#ifdef HYBRII_802154

/* === Includes ============================================================ */

#include <string.h>
#include <stdio.h>
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
#include "timer.h"
#include "list.h"
#include "stm.h"
/* === Macros =============================================================== */


/* === Globals ============================================================= */

/* === Prototypes ========================================================== */

/* === Implementation ====================================================== */

/*
 * Creates and send Communication Status Indication message to the upper layer
 *
 * status  - Status of the last operation
 * buf_ptr - Buffer for Communication Status Indication to the NHLE
 */
void mac_mlme_comm_status (uint8_t status, buffer_t *buf_ptr)
{
    uint64_t destination_address;
    /*
     * The pointer to the destination address (received as one of the function
     * paramters) points to a location in buf_ptr.
     * As the same buffer is used to generate the comm status
     * indication, it is typecasted to the 'mlme_comm_status_ind_t'. This may
     * result in loosing destination address (which is still a part of this
     * buffer), hence the destination address is backed up in a stack variable.
     */
    mlme_comm_status_ind_t *csi_p;
    frame_info_t *frame_ptr = (frame_info_t *)BMM_BUFFER_POINTER(buf_ptr);

    memcpy(&destination_address, &frame_ptr->mpdu_p[PL_POS_DST_ADDR_START],
           sizeof(uint64_t));

    csi_p = (mlme_comm_status_ind_t*)BMM_BUFFER_POINTER(buf_ptr);

    csi_p->cmdcode = MLME_COMM_STATUS_INDICATION;

    csi_p->PANId = hal_pib_PANId;

    csi_p->SrcAddrMode = FCF_LONG_ADDR;

    /* Initialize the source address */
    csi_p->SrcAddr = hal_pib_IeeeAddress;

    csi_p->DstAddrMode = FCF_LONG_ADDR;

    /* Initialize the destination address */
    csi_p->DstAddr = destination_address;

    csi_p->status = status;

#if (defined UM) && (!defined ZBMAC_DIAG)
	mlme_send_to_host(buf_ptr);
#else
	mlme_comm_status_ind(buf_ptr);
#endif		            			
}

/*
 * @brief Sends mlme reset confirm
 *
 * @param m Buffer for reset confirm
 * @param status Status of MAC reset operation
 */
static void mac_misc_send_reset_conf (buffer_t *buf_ptr, uint8_t status)
{
    mlme_reset_conf_t *mrc = (mlme_reset_conf_t *)BMM_BUFFER_POINTER(buf_ptr);	

    mrc->status = status;
    mrc->cmdcode = MLME_RESET_CONFIRM;

    /* Append the mlme reset confirm to the MAC-NHLE queue */
#if (defined UM) && (!defined ZBMAC_DIAG)
	mlme_send_to_host(buf_ptr);
#else
	mlme_reset_conf(buf_ptr);
#endif	
}

/*
 *
 * The MLME-RESET.request primitive allows the next higher layer to request
 * that the MLME performs a reset operation.
 *
 * @param m Pointer to the MLME_RESET.request given by the NHLE
 */
void mlme_reset_request (buffer_t *buf_ptr)
{
    mlme_reset_req_t *mrr = (mlme_reset_req_t *)BMM_BUFFER_POINTER(buf_ptr);
    retval_t         status;

    /* Wakeup the radio */
    mac_trx_wakeup();

    /* Start MAC reset functionality */
    //status = mac_reset(mrr->SetDefaultPIB);	
	status = MAC_SUCCESS;
	
    /* Set radio to sleep if allowed */
    mac_trx_sleep();

    /*
     * As this is a mlme_reset request, all the requests, data (whether direct
     * or indirect), incoming frames are removed from the queues
     */
    mac_misc_send_reset_conf(buf_ptr, status);
	
    mac_q_flush();
}

#endif //HYBRII_802154

