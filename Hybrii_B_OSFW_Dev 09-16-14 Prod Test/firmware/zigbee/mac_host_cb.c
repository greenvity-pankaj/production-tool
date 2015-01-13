/**
 * @file mac_host_cb.c
 *
 * Wrapper code for MAC callback functions.
 *
 * $Id: mac_host_cb.c,v 1.2 2014/06/09 13:19:46 kiran Exp $
 *
 * Copyright (c) 2011, Greenvity Communication All rights reserved.
 *
 */

/* === Includes ============================================================ */
#include <stdio.h>
#include <string.h>
#include "return_val.h"
#include "papdef.h"
#include "bmm.h"
#include "qmm.h"
#include "mac_const.h"
#include "mac_msgs.h"
#include "mac_hal.h"
#include "mac_api.h"
#include "mac_data_structures.h"
#include "mac_internal.h"
#include "mac.h"
#include "mac_intf_common.h"
#include "zb_usr_mac_sap.h"
#ifdef _LED_DEMO_
#include "led_board.h"
#endif

/* === Macros ============================================================== */


/* === Globals ============================================================= */


/* === Prototypes ========================================================== */


/* === Implementation ====================================================== */

/**
 *
 * This function is a callback for mcps data indication
 *
 * buf_p - Pointer to message structure
 */
void mcps_data_ind (buffer_t *buf_p)
{
    mcps_data_ind_t *pmsg;
    wpan_addr_spec_t src_addr;
    wpan_addr_spec_t dst_addr;

    /* Get the buffer body from buffer header */
    pmsg = (mcps_data_ind_t *)BMM_BUFFER_POINTER(buf_p);

    /* Source address spec */
    src_addr.AddrMode = pmsg->SrcAddrMode;
    src_addr.PANId = pmsg->SrcPANId;
    src_addr.Addr.long_address = pmsg->SrcAddr;

    /* Destination address spec */
    dst_addr.AddrMode = pmsg->DstAddrMode;
    dst_addr.PANId = pmsg->DstPANId;
    dst_addr.Addr.long_address = pmsg->DstAddr;

    /* Callback function */
#ifdef HOST_INTF_EN
    usr_mcps_data_ind(&src_addr,
						  &dst_addr,
						  pmsg->msduLength,
						  pmsg->msdu_p,
						  pmsg->mpduLinkQuality,
						  pmsg->DSN,
						  pmsg->Timestamp,
						  pmsg->Security.SecurityLevel,
						  pmsg->Security.KeyIdMode,
						  pmsg->Security.KeyIndex);
#endif
#ifdef _LED_DEMO_
    led_msg_decode(pmsg->msdu_p);
#endif
#ifdef __DEBUG__
{
    uint8_t idx;

    printf("\nRX bytes = %bu, LQI = %bx\n",
           pmsg->msduLength, pmsg->mpduLinkQuality);
    for (idx = 0; idx < pmsg->msduLength + 2; idx++) {
        printf("%02bx ", pmsg->msdu_p[idx]);
    }
	printf("\n");
}
#endif
#ifdef _VERIFY_PAYLOAD_
{
    extern uint8_t test_tx_data[];

    uint16_t idx;

    if (FALSE == hal_pib_PromiscuousMode) {
        for (idx = 0; idx < pmsg->msduLength; idx++) {
            if (pmsg->msdu_p[idx] != test_tx_data[idx]) {
                printf("\n[%d]%02bx - Expected %02bx\n",
                       idx, pmsg->msdu_p[idx],
                       test_tx_data[idx]);
            }
        }
    }
}
#endif

    /* Free the buffer */
    bmm_buffer_free(buf_p);
}

/**
 *
 * This function is a callback for mcps data confirm.
 *
 * buf_p - Pointer to message structure
 */
void mcps_data_conf (buffer_t *buf_p)
{
    mcps_data_conf_t *pmsg;

    /* Get the buffer body from buffer header */
    pmsg = (mcps_data_conf_t *)BMM_BUFFER_POINTER(buf_p);

#ifdef HOST_INTF_EN
    usr_mcps_data_conf(pmsg->msduHandle, pmsg->status, pmsg->Timestamp);
#endif

    /* Free the buffer */
    bmm_buffer_free(buf_p);
}

/**
 *
 * This function is a callback for mcps purge confirm.
 *
 * buf_p - Pointer to message structure
 */
void mcps_purge_conf (buffer_t *buf_p)
{
    mcps_purge_conf_t *pmsg;

    /* Get the buffer body from buffer header */
    pmsg = (mcps_purge_conf_t *)BMM_BUFFER_POINTER(buf_p);

#ifdef HOST_INTF_EN
    usr_mcps_purge_conf(pmsg->msduHandle, pmsg->status);
#endif

    /* Free the buffer */
    bmm_buffer_free(buf_p);
}

/**
 *
 * This function is a callback for mlme associate confirm.
 *
 * buf_p - Pointer to message structure
 */
void mlme_associate_conf (buffer_t *buf_p)
{
    mlme_associate_conf_t *pmsg;

    /* Get the buffer body from buffer header */
    pmsg = (mlme_associate_conf_t *)BMM_BUFFER_POINTER(buf_p);

#ifdef HOST_INTF_EN
    usr_mlme_associate_conf(pmsg->AssocShortAddress, pmsg->status);
#endif

    /* Free the buffer */
    bmm_buffer_free(buf_p);
}

/**
 *
 * This function is a callback for mlme associate indication.
 *
 * buf_p - Pointer to message structure
 */
void mlme_associate_ind (buffer_t *buf_p)
{
    mlme_associate_ind_t *pmsg;

    /* Get the buffer body from buffer header */
    pmsg = (mlme_associate_ind_t *)BMM_BUFFER_POINTER(buf_p);

#ifdef HOST_INTF_EN
    usr_mlme_associate_ind(pmsg->DeviceAddress, pmsg->CapabilityInformation);
#endif

    /* Free the buffer */
    bmm_buffer_free(buf_p);
}

/**
 *
 * This function is a callback for mlme beacon notify indication.
 *
 * buf_p - Pointer to message structure
 */
void mlme_beacon_notify_ind (buffer_t *buf_p)
{
    mlme_beacon_notify_ind_t *pmsg;

    /* Get the buffer body from buffer header */
    pmsg = (mlme_beacon_notify_ind_t *)BMM_BUFFER_POINTER(buf_p);

    /* Callback function */
#ifdef HOST_INTF_EN
    usr_mlme_beacon_notify_ind(pmsg->BSN,               // BSN
                               &(pmsg->PANDescriptor),  // PANDescriptor
                               pmsg->PendAddrSpec,      // PendAddrSpec
                               pmsg->AddrList,          // AddrList
                               pmsg->sduLength,         // sduLength
                               pmsg->sdu);              // sdu
#endif

    /* Free the buffer */
    bmm_buffer_free(buf_p);
}

/**
 *
 * This function is a callback for mlme comm status indication.
 *
 * buf_p - Pointer to message structure
 */
void mlme_comm_status_ind (buffer_t *buf_p)
{
    mlme_comm_status_ind_t *pmsg;
    wpan_addr_spec_t src_addr;
    wpan_addr_spec_t dst_addr;

    /* Get the buffer body from buffer header */
    pmsg = (mlme_comm_status_ind_t *)BMM_BUFFER_POINTER(buf_p);

    /* Source address spec */
    src_addr.PANId = pmsg->PANId;
    src_addr.AddrMode = pmsg->SrcAddrMode;
    src_addr.Addr.long_address = pmsg->SrcAddr;

    /* Destintion address spec */
    dst_addr.PANId = pmsg->PANId;
    dst_addr.AddrMode = pmsg->DstAddrMode;
    dst_addr.Addr.long_address = pmsg->DstAddr;

    /* Callback function */
#ifdef HOST_INTF_EN
    usr_mlme_comm_status_ind(&src_addr,
                             &dst_addr,
                             pmsg->status);
#endif

    /* Free the buffer */
    bmm_buffer_free(buf_p);
}

/**
 *
 * This function is a callback for mlme disassociate confirm.
 *
 * buf_p - Pointer to message structure
 */
void mlme_disassociate_conf (buffer_t *buf_p)
{
    mlme_disassociate_conf_t *pmsg;
    wpan_addr_spec_t device_addr;

    /* Get the buffer body from buffer header */
    pmsg = (mlme_disassociate_conf_t *)BMM_BUFFER_POINTER(buf_p);

    /* Device address spec */
    device_addr.AddrMode = pmsg->DeviceAddrMode;
    device_addr.PANId = pmsg->DevicePANId;
    device_addr.Addr.long_address = pmsg->DeviceAddress.long_address;

#ifdef HOST_INTF_EN
    usr_mlme_disassociate_conf(pmsg->status,
                               &device_addr);
#endif

    /* Free the buffer */
    bmm_buffer_free(buf_p);
}

/**
 *
 * This function is a callback for mlme disassociate indication.
 *
 * buf_p - Pointer to message structure
 */
void mlme_disassociate_ind (buffer_t *buf_p)
{
    mlme_disassociate_ind_t *pmsg;

    /* Get the buffer body from buffer header */
    pmsg = (mlme_disassociate_ind_t *)BMM_BUFFER_POINTER(buf_p);

#ifdef HOST_INTF_EN
    usr_mlme_disassociate_ind(pmsg->DeviceAddress,
                              pmsg->DisassociateReason);
#endif

    /* Free the buffer */
    bmm_buffer_free(buf_p);
}

/**
 *
 * This function is a callback for mlme get confirm.
 *
 * buf_p - Pointer to message structure
 */
void mlme_get_conf (buffer_t *buf_p)
{
    mlme_get_conf_t *pmsg;

    /* Get the buffer body from buffer header */
    pmsg = (mlme_get_conf_t *)BMM_BUFFER_POINTER(buf_p);

    /* Callback function */
#ifdef HOST_INTF_EN
    usr_mlme_get_conf(pmsg->status,
                      pmsg->PIBAttribute,
                      pmsg->PIBAttributeIndex,
                      &pmsg->PIBAttributeValue);
#endif

    /* Free the buffer */
    bmm_buffer_free(buf_p);
}

/**
 *
 * This function is a callback for mlme orphan indication.
 *
 * buf_p - Pointer to message structure
 */
void mlme_orphan_ind (buffer_t *buf_p)
{
    mlme_orphan_ind_t *pmsg;

    /* Get the buffer body from buffer header */
    pmsg = (mlme_orphan_ind_t *)BMM_BUFFER_POINTER(buf_p);

    /* Callback function */
#ifdef HOST_INTF_EN
    usr_mlme_orphan_ind(pmsg->OrphanAddress);
#endif

    /* Free the buffer */
    bmm_buffer_free(buf_p);

}

/**
 *
 * This function is a callback for mlme poll confirm.
 *
 * buf_p - Pointer to message structure
 */
void mlme_poll_conf (buffer_t *buf_p)
{
    mlme_poll_conf_t *pmsg;

    /* Get the buffer body from buffer header */
    pmsg = (mlme_poll_conf_t *)BMM_BUFFER_POINTER(buf_p);

#ifdef HOST_INTF_EN
    usr_mlme_poll_conf(pmsg->status);
#endif

    /* Free the buffer */
    bmm_buffer_free(buf_p);
}

/**
 *
 * This function is a callback for mlme reset confirm.
 *
 * buf_p - Pointer to message structure
 */
void mlme_reset_conf (buffer_t *buf_p)
{
    mlme_reset_conf_t *pmsg;

    /* Get the buffer body from buffer header */
    pmsg = (mlme_reset_conf_t *)BMM_BUFFER_POINTER(buf_p);

#ifdef HOST_INTF_EN
    usr_mlme_reset_conf(pmsg->status);
#endif

    /* Free the buffer */
    bmm_buffer_free(buf_p);
}

/**
 *
 * This function is a callback for mlme rx enable confirm.
 *
 * buf_p - Pointer to message structure
 */
void mlme_rx_enable_conf (buffer_t *buf_p)
{
    mlme_rx_enable_conf_t *pmsg;

    /* Get the buffer body from buffer header */
    pmsg = (mlme_rx_enable_conf_t *)BMM_BUFFER_POINTER(buf_p);

#ifdef HOST_INTF_EN
    usr_mlme_rx_enable_conf(pmsg->status);
#endif

    /* Free the buffer */
    bmm_buffer_free(buf_p);
}

/**
 *
 * This function is a callback for mlme scan confirm.
 *
 * buf_p - Pointer to message structure
 */
void mlme_scan_conf (buffer_t *buf_p)
{
    mlme_scan_conf_t *pmsg;

    /* Get the buffer body from buffer header */
    pmsg = (mlme_scan_conf_t *)BMM_BUFFER_POINTER(buf_p);

    /* Callback */
#ifdef HOST_INTF_EN
    usr_mlme_scan_conf(pmsg->status,
                       pmsg->ScanType,
                       pmsg->ChannelPage,
                       pmsg->UnscannedChannels,
                       pmsg->ResultListSize,
                       &pmsg->scan_result_list);
#endif

    /* Free the buffer */
    bmm_buffer_free(buf_p);
}

/**
 *
 * This function is a callback for mlme set confirm.
 *
 * buf_p - Pointer to message structure
 */
void mlme_set_conf (buffer_t *buf_p)
{
    mlme_set_conf_t *pmsg;

    /* Get the buffer body from buffer header */
    pmsg = (mlme_set_conf_t *)BMM_BUFFER_POINTER(buf_p);

#ifdef HOST_INTF_EN
    usr_mlme_set_conf(pmsg->status, pmsg->PIBAttribute,
                      pmsg->PIBAttributeIndex);
#endif

    /* Free the buffer */
    bmm_buffer_free(buf_p);
}

/**
 *
 * This function is a callback for mlme start confirm.
 *
 * buf_p - Pointer to message structure
 */
void mlme_start_conf (buffer_t *buf_p)
{
    mlme_start_conf_t *pmsg;

    /* Get the buffer body from buffer header */
    pmsg = (mlme_start_conf_t *)BMM_BUFFER_POINTER(buf_p);

#ifdef HOST_INTF_EN
    usr_mlme_start_conf(pmsg->status);
#endif

    /* Free the buffer */
    bmm_buffer_free(buf_p);
}

/**
 *
 * This function is a callback for mlme sync loss indication.
 *
 * buf_p - Pointer to message structure
 */
void mlme_sync_loss_ind (buffer_t *buf_p)
{
    mlme_sync_loss_ind_t *pmsg;

    /* Get the buffer body from buffer header */
    pmsg = (mlme_sync_loss_ind_t *)BMM_BUFFER_POINTER(buf_p);

#ifdef HOST_INTF_EN
    usr_mlme_sync_loss_ind(pmsg->LossReason,
                           pmsg->PANId,
                           pmsg->LogicalChannel,
                           pmsg->ChannelPage);
#endif

    /* Uses static buffer for sync loss indication and it is not freed */
}
