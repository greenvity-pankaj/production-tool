/*
* $Id: zb_usr_mac_sap.h,v 1.1 2014/06/09 13:26:39 kiran Exp $
*
* $Source: /home/cvsrepo/Hybrii_B_OSFW_Dev/firmware/zigbee/zb_usr_mac_sap.h,v $
*
* Description : Zigbee MAC SAP Header Files.
*
* Copyright (c) 2012 Greenvity Communications, Inc.
* All rights reserved.
*
* Purpose :
*    
*
*/


extern uint8_t mac_get_pib_attribute_size (uint8_t pib_attribute_id);

void zb_mac_sap_parser(hostHdr_t *pHostHdr);

eStatus mac_sap_builder(uint8_t *buffer , uint16_t packetlen);


void usr_mcps_data_ind(wpan_addr_spec_t *src_addr_info_p, wpan_addr_spec_t *dst_addr_info_p,uint8_t msduLength, uint8_t *msdu_p,
							uint8_t mpduLinkQuality, uint8_t DSN,uint32_t Timestamp, uint8_t SecurityLevel, uint8_t KeyIdMode, uint8_t KeyIndex);

void usr_mcps_data_conf(uint8_t msduHandle, uint8_t status, uint32_t Timestamp);

void usr_mcps_purge_conf(uint8_t msduHandle, uint8_t status);

void usr_mlme_associate_conf(uint16_t AssocShortAddress, uint8_t status);


void usr_mlme_associate_ind(uint64_t DeviceAddress, uint8_t CapabilityInformation);

void usr_mlme_start_conf(uint8_t status);

void usr_mlme_beacon_notify_ind(uint8_t BSN,              
                               pandescriptor_t * PANDescriptor,  
                               uint8_t PendAddrSpec,      
                               uint8_t *AddrList,       
                               uint8_t sduLength,         
                               uint8_t *sdu);              

void usr_mlme_comm_status_ind(wpan_addr_spec_t * src_addr,
                             wpan_addr_spec_t * dst_addr,
                             uint8_t status);

void usr_mlme_disassociate_conf(uint8_t status,
                               wpan_addr_spec_t * device_addr);
	
void usr_mlme_get_conf(uint8_t status,
                      uint8_t PIBAttribute,
                      uint8_t PIBAttributeIndex,
                      pib_value_t * PIBAttributeValue);

void usr_mlme_disassociate_ind(uint64_t DeviceAddress,
                              uint8_t DisassociateReason);

void usr_mlme_orphan_ind(uint64_t OrphanAddress);

void usr_mlme_poll_conf(uint8_t status);

void usr_mlme_reset_conf(uint8_t status);

void usr_mlme_rx_enable_conf(uint8_t status);

void usr_mlme_scan_conf(uint8_t status,
                       uint8_t ScanType,
                       uint8_t ChannelPage,
                       uint32_t UnscannedChannels,
                       uint8_t ResultListSize,
                       scan_result_list_t *scan_result_list);


void usr_mlme_set_conf(uint8_t status, uint8_t PIBAttribute,
                      uint8_t PIBAttributeIndex);

void usr_mlme_sync_loss_ind(uint8_t LossReason,
                           uint16_t PANId,
                           uint8_t LogicalChannel,
                           uint8_t ChannelPage);
