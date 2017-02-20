/**
 * @file mac_api.c
 *
 * This file contains MAC API functions.
 *
 * $Id: mac_api.c,v 1.4 2014/11/26 13:19:41 ranjan Exp $
 *
 * Copyright (c) 2011, Greenvity Communication All rights reserved.
 *
 */

/* === Includes ============================================================ */
#ifdef HYBRII_802154
#ifdef RTX51_TINY_OS
#include <rtx51tny.h>
#include "hybrii_tasks.h"
#endif
#include <stdio.h>
#include <string.h>
#include "papdef.h"
#include "timer.h"
#include "return_val.h"
#include "bmm.h"
#include "qmm.h"
#include "mac_const.h"
#include "mac_msgs.h"
#include "mac_data_structures.h"
#include "mac_hal.h"
#include "mac_internal.h"
#include "mac.h"
#include "fm.h"

/* === Types =============================================================== */


/* === Macros ============================================================== */


/* === Globals ============================================================= */


/* === Prototypes ========================================================== */


/* === Implementation ====================================================== */
static bool mac_api_to_nhle_mac_queue (buffer_t *buffer_p)
{
    if (FALSE == qmm_queue_append(&nhle_mac_q, buffer_p)) {
        bmm_buffer_free(buffer_p);
        return (false);
    } 
#ifdef RTX51_TINY_OS
#ifdef MAC_802154_TASK
	os_set_ready(MAC_802154_TASK_ID);
#else
	os_set_ready(HYBRII_TASK_ID_FRAME);
#endif /* MAC_802154_TASK */
#ifdef UM
	pending_802154_task = TRUE;
#endif
    os_switch_task();
#endif
    return (true);
}

/* MAC level API */

bool mac_api_mcps_data_req (uint8_t SrcAddrMode,
                            wpan_addr_spec_t *DstAddrSpec_p,
                            uint8_t msduLength,
                            uint8_t *msdu_p,
                            uint8_t msduHandle,
                            uint8_t TxOptions,
                            security_info_t *sec_p)
{
    buffer_t *buffer_p;
    mcps_data_req_t *mcps_data_req_p;
    uint8_t *payload_pos_p;

    if (msduLength > aMaxMACPayloadSize) {
        /* Frame is too long and thus rejected immediately */
        return false;
    }

    /* Allocate a buffer for mcps data request */
    buffer_p = bmm_buffer_alloc(BUFFER_SIZE);

    if (NULL == buffer_p) {
        /* Buffer is not available */
        return false;
    }

    /* Get the buffer body from buffer header */
    mcps_data_req_p = (mcps_data_req_t *)BMM_BUFFER_POINTER(buffer_p);

    /* Construct mcps_data_req_t message */
    mcps_data_req_p->cmdcode = MCPS_DATA_REQUEST;

    /* Source addr mode */
    mcps_data_req_p->SrcAddrMode = SrcAddrMode;

    /* Destination addr spec */
    mcps_data_req_p->DstAddrMode = DstAddrSpec_p->AddrMode;

    mcps_data_req_p->DstPANId = DstAddrSpec_p->PANId;

    mcps_data_req_p->DstAddr.hi_u32 = 0;
    mcps_data_req_p->DstAddr.lo_u32 = 0;
	
    if (WPAN_ADDRMODE_SHORT == mcps_data_req_p->DstAddrMode) {
        mcps_data_req_p->DstAddr.lo_u32 = 
                               (uint32_t)DstAddrSpec_p->Addr.short_address; 
    } else {
        mcps_data_req_p->DstAddr = DstAddrSpec_p->Addr.long_address;
    }

    /* Other fields */
    mcps_data_req_p->msduHandle             = msduHandle;
    mcps_data_req_p->TxOptions              = TxOptions;
    mcps_data_req_p->msduLength             = msduLength;
    if (sec_p) {
        mcps_data_req_p->Security.SecurityLevel = sec_p->SecurityLevel;
        mcps_data_req_p->Security.KeyIdMode     = sec_p->KeyIdMode;
        mcps_data_req_p->Security.KeyIndex      = sec_p->KeyIndex;
        memcpy(mcps_data_req_p->Security.KeySource, sec_p->KeySource,
               SEC_KEY_SRC_MAX);
    } else {
        mcps_data_req_p->Security.SecurityLevel = 0;
    }

    /* Find the position where the data payload is to be updated */
    payload_pos_p = ((uint8_t *)mcps_data_req_p) +
                     (BUFFER_SIZE - msduLength);

    /* Copy the payload to the end of buffer */
    memcpy(payload_pos_p, msdu_p, msduLength);

    /* Enqueue the message to MAC Q */
    return (mac_api_to_nhle_mac_queue(buffer_p));
}

bool mac_api_mcps_purge_req (uint8_t msduHandle)
{
    buffer_t *buffer_p;
    mcps_purge_req_t *mcps_purge_req_p;

    buffer_p = bmm_buffer_alloc(BUFFER_SIZE);

    if (NULL == buffer_p) {
        /* Buffer is not available */
        return false;
    }

    /* Get the buffer body from buffer header */
    mcps_purge_req_p = (mcps_purge_req_t *)BMM_BUFFER_POINTER(buffer_p);

    /* Update the purge request structure */
    mcps_purge_req_p->cmdcode = MCPS_PURGE_REQUEST;
    mcps_purge_req_p->msduHandle = msduHandle;

    /* Enqueue the message to MAC Q */
    return (mac_api_to_nhle_mac_queue(buffer_p));
}

bool mac_api_mlme_start_req (uint16_t PANId,
                             uint8_t LogicalChannel,
                             uint8_t ChannelPage,
                             uint32_t StartTime,
                             uint8_t BeaconOrder,
                             uint8_t SuperframeOrder,
                             bool PANCoordinator,
                             bool BatteryLifeExtension,
                             bool CoordRealignment,
                             security_info_t *CoordRealignmentSecurity_p,
                             security_info_t *BeaconSecurity_p)
{
    buffer_t *buffer_p;
    mlme_start_req_t *mlme_start_req_p;

    /* Allocate a buffer for start request */
    buffer_p = bmm_buffer_alloc(BUFFER_SIZE);

    if (NULL == buffer_p) {
        /* Buffer is not available */
        return false;
    }

    /* Get the buffer body from buffer header */
    mlme_start_req_p = (mlme_start_req_t *)BMM_BUFFER_POINTER(buffer_p);

    /* Update the start request structure */
    mlme_start_req_p->cmdcode = MLME_START_REQUEST;

    mlme_start_req_p->PANId = PANId;
    mlme_start_req_p->LogicalChannel = LogicalChannel;
    mlme_start_req_p->ChannelPage = ChannelPage;
    mlme_start_req_p->StartTime = StartTime;
    mlme_start_req_p->BeaconOrder = BeaconOrder;
    mlme_start_req_p->SuperframeOrder = SuperframeOrder;
    mlme_start_req_p->PANCoordinator = PANCoordinator;
    mlme_start_req_p->BatteryLifeExtension = BatteryLifeExtension;
    mlme_start_req_p->CoordRealignment = CoordRealignment;
    mlme_start_req_p->ChannelPage = ChannelPage;
    if (CoordRealignmentSecurity_p) {
        mlme_start_req_p->CoordRealignmentSecurity.SecurityLevel =
                                      CoordRealignmentSecurity_p->SecurityLevel;
        mlme_start_req_p->CoordRealignmentSecurity.KeyIdMode =
                                      CoordRealignmentSecurity_p->KeyIdMode;
        mlme_start_req_p->CoordRealignmentSecurity.KeyIndex  =
                                      CoordRealignmentSecurity_p->KeyIndex;
        memcpy(mlme_start_req_p->CoordRealignmentSecurity.KeySource,
               CoordRealignmentSecurity_p->KeySource, SEC_KEY_SRC_MAX);
    }
    if (BeaconSecurity_p) {
        mlme_start_req_p->BeaconSecurity.SecurityLevel =
                                      BeaconSecurity_p->SecurityLevel;
        mlme_start_req_p->BeaconSecurity.KeyIdMode =
                                      BeaconSecurity_p->KeyIdMode;
        mlme_start_req_p->BeaconSecurity.KeyIndex  =
                                      BeaconSecurity_p->KeyIndex;
        memcpy(mlme_start_req_p->BeaconSecurity.KeySource,
               BeaconSecurity_p->KeySource, SEC_KEY_SRC_MAX);
    } else {
        mlme_start_req_p->BeaconSecurity.SecurityLevel = 0;
    }

    /* Enqueue the message to MAC Q */
    return (mac_api_to_nhle_mac_queue(buffer_p));
}

bool mac_api_mlme_associate_req (uint8_t LogicalChannel,
                                 uint8_t ChannelPage,
                                 wpan_addr_spec_t *CoordAddrSpec_p,
                                 uint8_t CapabilityInformation,
                                 security_info_t *sec_p)
{
    buffer_t *buffer_p;
    mlme_associate_req_t *mlme_associate_req_p;

    /* Allocate a buffer for mlme associate request */
    buffer_p = bmm_buffer_alloc(BUFFER_SIZE);

    /* Check for buffer availability */
    if (NULL == buffer_p) {
        return false;
    }

    /* Get the buffer body from buffer header */
    mlme_associate_req_p = (mlme_associate_req_t*)
                           BMM_BUFFER_POINTER(buffer_p);

    /* Construct mlme_associate_req_t message */
    mlme_associate_req_p->cmdcode = MLME_ASSOCIATE_REQUEST;

    /* Operating channel */
    mlme_associate_req_p->LogicalChannel = LogicalChannel;

    /* Coordinator address spec */
    mlme_associate_req_p->CoordAddrMode = CoordAddrSpec_p->AddrMode;
    mlme_associate_req_p->CoordPANId    = CoordAddrSpec_p->PANId;

    mlme_associate_req_p->CoordAddress.long_address.lo_u32 = 
                                      CoordAddrSpec_p->Addr.long_address.lo_u32;
    mlme_associate_req_p->CoordAddress.long_address.hi_u32 = 
                                      CoordAddrSpec_p->Addr.long_address.hi_u32;

    /* Other fields */
    mlme_associate_req_p->CapabilityInformation = CapabilityInformation;
    mlme_associate_req_p->ChannelPage           = ChannelPage;

    if (sec_p) {
        mlme_associate_req_p->Security.SecurityLevel = sec_p->SecurityLevel;
        mlme_associate_req_p->Security.KeyIdMode     = sec_p->KeyIdMode;
        mlme_associate_req_p->Security.KeyIndex      = sec_p->KeyIndex;
        memcpy(mlme_associate_req_p->Security.KeySource, sec_p->KeySource,
               SEC_KEY_SRC_MAX);
    } else {
        mlme_associate_req_p->Security.SecurityLevel = 0;
    }

    /* Enqueue the message to MAC Q */
    return (mac_api_to_nhle_mac_queue(buffer_p));
}

bool mac_api_mlme_associate_resp (uint64_t DeviceAddress,
                             uint16_t AssocShortAddress,
                             uint8_t status,
                             security_info_t *sec_p)
{
    buffer_t *buffer_p;
    mlme_associate_resp_t *mlme_associate_resp_p;

    /* Allocate a buffer for association response */
    buffer_p = bmm_buffer_alloc(BUFFER_SIZE);

    if (NULL == buffer_p) {
        /* Buffer is not available */
        return false;
    }

    /* Get the buffer body from buffer header */
    mlme_associate_resp_p = (mlme_associate_resp_t *)
                            BMM_BUFFER_POINTER(buffer_p);

    /* Construct mlme_associate_resp_t message */
    mlme_associate_resp_p->cmdcode = MLME_ASSOCIATE_RESPONSE;

    /* Other fields */
    mlme_associate_resp_p->DeviceAddress  = DeviceAddress;
    mlme_associate_resp_p->AssocShortAddress = AssocShortAddress;
    mlme_associate_resp_p->status = status;
    if (sec_p) {
        mlme_associate_resp_p->Security.SecurityLevel = sec_p->SecurityLevel;
        mlme_associate_resp_p->Security.KeyIdMode     = sec_p->KeyIdMode;
        mlme_associate_resp_p->Security.KeyIndex      = sec_p->KeyIndex;
        memcpy(mlme_associate_resp_p->Security.KeySource, sec_p->KeySource,
               SEC_KEY_SRC_MAX);
    } else {
        mlme_associate_resp_p->Security.SecurityLevel = 0;
    }
    /* Enqueue the message to MAC Q */
    return (mac_api_to_nhle_mac_queue(buffer_p));
}

bool mac_api_mlme_disassociate_req (wpan_addr_spec_t *DeviceAddrSpec,
                               uint8_t DisassociateReason,
                               bool TxIndirect,
                               security_info_t *sec_p)
{
    buffer_t *buffer_p;
    mlme_disassociate_req_t *mlme_disassociate_req_p;

    /* Allocate a buffer for disassociation request */
    buffer_p = bmm_buffer_alloc(BUFFER_SIZE);

    if (NULL == buffer_p) {
        /* Buffer is not available */
        return false;
    }

    /* Get the buffer body from buffer header */
    mlme_disassociate_req_p = (mlme_disassociate_req_t *)
                              BMM_BUFFER_POINTER(buffer_p);

    /* Update the disassociate request structure */
    mlme_disassociate_req_p->cmdcode = MLME_DISASSOCIATE_REQUEST;
    mlme_disassociate_req_p->DisassociateReason = DisassociateReason;
    mlme_disassociate_req_p->DeviceAddrMode = DeviceAddrSpec->AddrMode;
    mlme_disassociate_req_p->DevicePANId = DeviceAddrSpec->PANId;
    mlme_disassociate_req_p->DeviceAddress.long_address = 
                                            DeviceAddrSpec->Addr.long_address;
    mlme_disassociate_req_p->TxIndirect = TxIndirect;
    if (sec_p) {
        mlme_disassociate_req_p->Security.SecurityLevel = sec_p->SecurityLevel;
        mlme_disassociate_req_p->Security.KeyIdMode     = sec_p->KeyIdMode;
        mlme_disassociate_req_p->Security.KeyIndex      = sec_p->KeyIndex;
        memcpy(mlme_disassociate_req_p->Security.KeySource, sec_p->KeySource,
               SEC_KEY_SRC_MAX);
    } else {
        mlme_disassociate_req_p->Security.SecurityLevel = 0;
    }

    /* Enqueue the message to MAC Q */
    return (mac_api_to_nhle_mac_queue(buffer_p));
}

bool mac_api_mlme_orphan_resp (uint64_t OrphanAddress,
                               uint16_t ShortAddress,
                               bool AssociatedMember,
                               security_info_t *sec_p)
{
    buffer_t *buffer_p;
    mlme_orphan_resp_t *mlme_orphan_resp_p;

    /* Allocate a small buffer for orphan response */
    buffer_p = bmm_buffer_alloc(BUFFER_SIZE);

    if (NULL == buffer_p) {
        /* Buffer is not available */
        return false;
    }

    /* Get the buffer body from buffer header */
    mlme_orphan_resp_p = (mlme_orphan_resp_t *)BMM_BUFFER_POINTER(buffer_p);

    /* Update the orphan response structure */
    mlme_orphan_resp_p->cmdcode = MLME_ORPHAN_RESPONSE;
    mlme_orphan_resp_p->OrphanAddress = OrphanAddress;
    mlme_orphan_resp_p->ShortAddress  = ShortAddress;
    mlme_orphan_resp_p->AssociatedMember = AssociatedMember;
    if (sec_p) {
        mlme_orphan_resp_p->Security.SecurityLevel = sec_p->SecurityLevel;
        mlme_orphan_resp_p->Security.KeyIdMode     = sec_p->KeyIdMode;
        mlme_orphan_resp_p->Security.KeyIndex      = sec_p->KeyIndex;
        memcpy(mlme_orphan_resp_p->Security.KeySource, sec_p->KeySource,
               SEC_KEY_SRC_MAX);
    } else {
        mlme_orphan_resp_p->Security.SecurityLevel = 0;
    }

    /* Enqueue the message to MAC Q */
    return (mac_api_to_nhle_mac_queue(buffer_p));
}

bool mac_api_mlme_reset_req (bool SetDefaultPib)
{
    buffer_t *buffer_p;
    mlme_reset_req_t *mlme_reset_req_p;

    /* Allocate a buffer for reset request */
    buffer_p = bmm_buffer_alloc(BUFFER_SIZE);

    if (NULL == buffer_p) {
        /* Buffer is not available */
        return false;
    }

    /* Get the buffer body from buffer header */
    mlme_reset_req_p = (mlme_reset_req_t *)BMM_BUFFER_POINTER(buffer_p);

    /* Update the reset request structure */
    mlme_reset_req_p->cmdcode = MLME_RESET_REQUEST;
    mlme_reset_req_p->SetDefaultPIB = SetDefaultPib;

    /* Enqueue the message to MAC Q */
    return (mac_api_to_nhle_mac_queue(buffer_p));
}

bool mac_api_mlme_get_req (uint8_t PIBAttribute, uint8_t PIBAttributeIndex)
{
    buffer_t *buffer_p;
    mlme_get_req_t *mlme_get_req_p;

    buffer_p = bmm_buffer_alloc(BUFFER_SIZE);

    /* Check for buffer availability */
    if (NULL == buffer_p) {
        return false;
    }

    /* Get the buffer body from buffer header */
    mlme_get_req_p = (mlme_get_req_t*)BMM_BUFFER_POINTER(buffer_p);

    /* Update the get request structure */
    mlme_get_req_p->cmdcode = MLME_GET_REQUEST;
    mlme_get_req_p->PIBAttribute = PIBAttribute;
    mlme_get_req_p->PIBAttributeIndex = PIBAttributeIndex;

    /* Enqueue the message to MAC Q */
    return (mac_api_to_nhle_mac_queue(buffer_p));
}

bool mac_api_mlme_set_req (uint8_t PIBAttribute,
                           uint8_t PIBAttributeIndex,
                           void *PIBAttributeValue)
{
    buffer_t *buffer_p;
    mlme_set_req_t *mlme_set_req_p;
    uint8_t pib_attribute_octet_no;

    buffer_p = bmm_buffer_alloc(BUFFER_SIZE);

    /* Check for buffer availability */
    if (NULL == buffer_p) {
        return false;
    }

    /* Get size of PIB attribute to be set */
    pib_attribute_octet_no = mac_get_pib_attribute_size(PIBAttribute);

    /* Get the buffer body from buffer header */
    mlme_set_req_p = (mlme_set_req_t *)BMM_BUFFER_POINTER(buffer_p);

    /* Construct mlme_set_req_t message */
    mlme_set_req_p->cmdcode = MLME_SET_REQUEST;

    /* Attribute and attribute value length */
    mlme_set_req_p->PIBAttribute = PIBAttribute;
    mlme_set_req_p->PIBAttributeIndex = PIBAttributeIndex;

    /* Attribute value */

    memcpy((void *)&(mlme_set_req_p->PIBAttributeValue),
                    (void *)PIBAttributeValue,
                    (size_t)pib_attribute_octet_no);

    /* Enqueue the message to MAC Q */
    return (mac_api_to_nhle_mac_queue(buffer_p));
}

bool mac_api_mlme_rx_enable_req (bool DeferPermit,
                                 uint32_t RxOnTime,
                                 uint32_t RxOnDuration)
{
    buffer_t *buffer_p;
    mlme_rx_enable_req_t *mlme_rx_enable_req_p;

    /* Allocate a buffer for rx enable request */
    buffer_p = bmm_buffer_alloc(BUFFER_SIZE);

    if (NULL == buffer_p) {
        /* Buffer is not available */
        return false;
    }

    /* Get the buffer body from buffer header */
    mlme_rx_enable_req_p = (mlme_rx_enable_req_t *)
                           BMM_BUFFER_POINTER(buffer_p);

    /* Update the rx enable request structure */
    mlme_rx_enable_req_p->cmdcode = MLME_RX_ENABLE_REQUEST;
    mlme_rx_enable_req_p->DeferPermit = DeferPermit;
    mlme_rx_enable_req_p->RxOnTime = RxOnTime;
    mlme_rx_enable_req_p->RxOnDuration = RxOnDuration;

    /* Enqueue the message to MAC Q */
    return (mac_api_to_nhle_mac_queue(buffer_p));
}

bool mac_api_mlme_scan_req (uint8_t ScanType,
                            uint32_t ScanChannels,
                            uint8_t ScanDuration,
                            uint8_t ChannelPage,
                            security_info_t *sec_p)
{
    buffer_t *buffer_p;
    mlme_scan_req_t* mlme_scan_req_p;

    /* Allocate a buffer for scan request */
    buffer_p = bmm_buffer_alloc(BUFFER_SIZE);

    if (NULL == buffer_p) {
        /* Buffer is not available */
		FM_Printf(FM_APP, "\nba:scanF");
        return false;
    }

    /* Get the buffer body from buffer header */
    mlme_scan_req_p = (mlme_scan_req_t*)BMM_BUFFER_POINTER(buffer_p);

    /* Update the scan request structure */
    mlme_scan_req_p->cmdcode = MLME_SCAN_REQUEST;
    mlme_scan_req_p->ScanType = ScanType;
    mlme_scan_req_p->ScanChannels = ScanChannels;
    mlme_scan_req_p->ScanDuration = ScanDuration;
    mlme_scan_req_p->ChannelPage = ChannelPage;
    if (sec_p) {
        mlme_scan_req_p->Security.SecurityLevel = sec_p->SecurityLevel;
        mlme_scan_req_p->Security.KeyIdMode     = sec_p->KeyIdMode;
        mlme_scan_req_p->Security.KeyIndex      = sec_p->KeyIndex;
        memcpy(mlme_scan_req_p->Security.KeySource, sec_p->KeySource,
               SEC_KEY_SRC_MAX);
    } else {
        mlme_scan_req_p->Security.SecurityLevel = 0; 
    }
 
    /* Enqueue the message to MAC Q */
    return (mac_api_to_nhle_mac_queue(buffer_p));
}

bool mac_api_mlme_sync_req (uint8_t LogicalChannel,
                            uint8_t ChannelPage,
                            bool TrackBeacon)
{
    buffer_t *buffer_p;
    mlme_sync_req_t *mlme_sync_req_p;

    /* Allocate a small buffer for sync request */
    buffer_p = bmm_buffer_alloc(BUFFER_SIZE);

    if (NULL == buffer_p) {
        /* Buffer is not available */
        return false;
    }

    /* Get the buffer body from buffer header */
    mlme_sync_req_p = (mlme_sync_req_t *)BMM_BUFFER_POINTER(buffer_p);

    /* Update the sync request structure */
    mlme_sync_req_p->cmdcode = MLME_SYNC_REQUEST;
    mlme_sync_req_p->LogicalChannel = LogicalChannel;
    mlme_sync_req_p->ChannelPage = ChannelPage;
    mlme_sync_req_p->TrackBeacon = TrackBeacon;

    /* Enqueue the message to MAC Q */
    return (mac_api_to_nhle_mac_queue(buffer_p));
}

bool mac_api_mlme_poll_req (wpan_addr_spec_t *CoordAddrSpec,
                            security_info_t *sec_p)
{
    buffer_t *buffer_p;
    mlme_poll_req_t *mlme_poll_req_p;

    /* Allocate a buffer for poll request */
    buffer_p = bmm_buffer_alloc(BUFFER_SIZE);

    if (NULL == buffer_p) {
        /* Buffer is not available */
        return false;
    }

    /* Get the buffer body from buffer header */
    mlme_poll_req_p = (mlme_poll_req_t *)BMM_BUFFER_POINTER(buffer_p);

    /* construct mlme_poll_req_t message */
    mlme_poll_req_p->cmdcode = MLME_POLL_REQUEST;

    /* Other fileds. */
    mlme_poll_req_p->CoordAddrMode = CoordAddrSpec->AddrMode;
    mlme_poll_req_p->CoordPANId = CoordAddrSpec->PANId;

    mlme_poll_req_p->CoordAddress.long_address.hi_u32 = 0;
    mlme_poll_req_p->CoordAddress.long_address.lo_u32 = 0;
    if (WPAN_ADDRMODE_SHORT == CoordAddrSpec->AddrMode) {
        mlme_poll_req_p->CoordAddress.long_address.lo_u32 = CoordAddrSpec->Addr.short_address;
    } else {
        mlme_poll_req_p->CoordAddress.long_address = CoordAddrSpec->Addr.long_address;
    }

    if (sec_p) {
        mlme_poll_req_p->Security.SecurityLevel = sec_p->SecurityLevel;
        mlme_poll_req_p->Security.KeyIdMode     = sec_p->KeyIdMode;
        mlme_poll_req_p->Security.KeyIndex      = sec_p->KeyIndex;
        memcpy(mlme_poll_req_p->Security.KeySource, sec_p->KeySource,
               SEC_KEY_SRC_MAX);
    } else {
        mlme_poll_req_p->Security.SecurityLevel = 0;
    }

    /* Enqueue the message to MAC Q */
    return (mac_api_to_nhle_mac_queue(buffer_p));
}
#endif // HYBRII_802154
