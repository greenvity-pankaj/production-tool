/*
* $Id: zb_usr_mac_sap.c,v 1.1 2014/06/09 13:26:39 kiran Exp $
*
* $Source: /home/cvsrepo/Hybrii_B_OSFW_Dev/firmware/zigbee/zb_usr_mac_sap.c,v $
*
* Description : Zigbee MAC SAP module.
*
* Copyright (c) 2012 Greenvity Communications, Inc.
* All rights reserved.
*
* Purpose : Zigbee MAC Service Access Point 
*    
*
*/



#include <stdio.h>
#include <string.h>
#include "fm.h"

#include "mac_intf_common.h"
#include "papdef.h"
#include "hpgpapi.h"
#include "mac_msgs.h"
#include "mac_const.h"
#include "qmm.h"
#include "nma_fw.h"
#include "zb_usr_mac_sap.h"
#include "mac_api.h"
#include "zigbee_mac_sap_def.h"


void zb_mac_sap_parser(hostHdr_t *pHostHdr)
{
	uint8_t 					*pos;
	hostCmdHdr_t				*pcmdHdr;

	pos = (uint8_t *)pHostHdr + sizeof(hostHdr_t);
	pcmdHdr =  (hostCmdHdr_t*)pos;

	switch(pcmdHdr->command){
	case ZB_MCPS_DATA_REQUEST:{
	
		host_mcps_data_request_t 	*pWpan_mcps_data_request;
		pWpan_mcps_data_request = (host_mcps_data_request_t *)pos;

#ifdef HOST_ENDIAN
pWpan_mcps_data_request->DstAddrSpec.PANId =
					le16_to_cpu(pWpan_mcps_data_request->DstAddrSpec.PANId);

if(pWpan_mcps_data_request->DstAddrSpec.AddrMode == WPAN_ADDRMODE_SHORT){
	pWpan_mcps_data_request->DstAddrSpec.Addr.short_address = 
		le16_to_cpu(pWpan_mcps_data_request->DstAddrSpec.Addr.short_address);
} else {
	long_address = pWpan_mcps_data_request->DstAddrSpec.Addr.long_address;
	temp = le32_to_cpu(long_address.hi_u32);
	long_address.hi_u32 = le32_to_cpu(long_address.lo_u32);
	long_address.lo_u32 = temp;
	pWpan_mcps_data_request->DstAddrSpec.Addr.long_address = long_address;
}
#endif

#ifdef HOST_INTF_EN
mac_api_mcps_data_req (pWpan_mcps_data_request->SrcAddrMode,
		                    &pWpan_mcps_data_request->DstAddrSpec,
		                    pWpan_mcps_data_request->msduLength,
		                    &pWpan_mcps_data_request->msdu,
		                    pWpan_mcps_data_request->msduHandle,
		                    pWpan_mcps_data_request->TxOptions,
		                    &pWpan_mcps_data_request->sec);
#else // This code will get executed if HOST_INTF_EN flag is not enabled. It is oly for testing of Zigbee MAC Sap if Zigbee mac code is not present. Kiran
			  // Son we need to delete this code after Zigbee PLC integration
usr_mcps_data_conf(pWpan_mcps_data_request->msduHandle, \
					0x00, 0x00000010);

{
	wpan_addr_spec_t src_addr_spec;
	src_addr_spec.AddrMode= pWpan_mcps_data_request->SrcAddrMode;
	src_addr_spec.PANId = pWpan_mcps_data_request->DstAddrSpec.PANId;
	src_addr_spec.Addr.short_address = 0x0002; //this is only for proof of concept not intended for final release. Random values

	usr_mcps_data_ind(&src_addr_spec,
					&pWpan_mcps_data_request->DstAddrSpec,
					pWpan_mcps_data_request->msduLength, \
					&pWpan_mcps_data_request->msdu,
					0xff, 0xff,0x00000010,
					pWpan_mcps_data_request->sec.SecurityLevel,
					pWpan_mcps_data_request->sec.KeyIdMode,
					pWpan_mcps_data_request->sec.KeyIndex);
}
		
#endif
		break;
	}

	case ZB_MCPS_PURGE_REQUEST:{
	
		host_mcps_purge_req_t *pPurgeReq;
		pPurgeReq = (host_mcps_purge_req_t *)pos;
		
		//#ifdef SAP_DEBUG
		FM_Printf(FM_USER,"\nMCPS Purge\n");
		FM_Printf(FM_USER,"MSDU Handle = %bu\r\n",pPurgeReq->msduHandle);
		//#endif
		
		#ifdef HOST_INTF_EN
		mac_api_mcps_purge_req (pPurgeReq->msduHandle);
		#else
		usr_mcps_purge_conf(pPurgeReq->msduHandle, 0x00);
		#endif
		
		break;
	
	}

	case ZB_MLME_START_REQUEST:{
	
		host_mlme_start_req_t *pStartReq;
		pStartReq = (host_mlme_start_req_t *)pos;

#ifdef HOST_ENDIAN		
		pStartReq->PANId = le16_to_cpu(pStartReq->PANId);
		pStartReq->StartTime = le32_to_cpu(pStartReq->StartTime);

#endif		
		
#ifdef SAP_DEBUG
	FM_Printf(FM_USER,"\nMLME Start Req\n");
	FM_Printf(FM_USER,"PANId = %x\n",pStartReq->PANId);
	FM_Printf(FM_USER,"Start T = %lx\n",pStartReq->StartTime);
	FM_Printf(FM_USER,"LC = %bx\n",pStartReq->LogicalChannel);
	FM_Printf(FM_USER,"Channel P = %bx\n",pStartReq->ChannelPage);
	FM_Printf(FM_USER,"Beacon Order = %bx\n",pStartReq->BeaconOrder);
	FM_Printf(FM_USER,"SuperFrameOrder = %bx\n",pStartReq->SuperframeOrder);
	FM_Printf(FM_USER,"PANCoord = %bx\n",pStartReq->PANCoordinator);
	FM_Printf(FM_USER,"Battery = %bx\n",pStartReq->BatteryLifeExtension);
	FM_Printf(FM_USER,"CoordRealign = %bx\n",pStartReq->CoordRealignment);
#endif

#ifdef HOST_INTF_EN
	mac_api_mlme_start_req (pStartReq->PANId,
	                             pStartReq->LogicalChannel,
	                             pStartReq->ChannelPage,
	                             pStartReq->StartTime,
	                             pStartReq->BeaconOrder,
	                             pStartReq->SuperframeOrder,
	                             pStartReq->PANCoordinator,
	                             pStartReq->BatteryLifeExtension,
	                             pStartReq->CoordRealignment,
	                            &pStartReq->CoordRealignmentSecurity,
	                            &pStartReq->BeaconSecurity);
#else
	usr_mlme_start_conf(0x00);//Remove this after Zigbee + PLC integration
#endif
		break;
	
	}
	
	case ZB_MLME_ASSOCIATE_REQUEST:{
	
		host_mlme_associate_req_t *pAssocReq;
		pAssocReq = (host_mlme_associate_req_t *)pos;

#ifdef HOST_ENDIAN
		pAssocReq->CoordAddress.PANId = le16_to_cpu(pAssocReq->CoordAddress.PANId);

		if(pAssocReq->CoordAddress.AddrMode == WPAN_ADDRMODE_SHORT)
		{
			pAssocReq->CoordAddress.Addr.short_address = 
										le16_to_cpu(pAssocReq->CoordAddress.Addr.short_address);
		}
		else
		{
			temp = le32_to_cpu(pAssocReq->CoordAddress.Addr.long_address.hi_u32);						
			pAssocReq->CoordAddress.Addr.long_address.hi_u32 = 
									le32_to_cpu(pAssocReq->CoordAddress.Addr.long_address.lo_u32);
			pAssocReq->CoordAddress.Addr.long_address.lo_u32 = temp;
		}
#endif

#ifdef SAP_DEBUG
	FM_Printf(FM_USER,"\nMLME Associate Request\n");
	FM_Printf(FM_USER,"Logical Channel = %bx\n", pAssocReq->LogicalChannel);
	FM_Printf(FM_USER,"Channel Page = %bx\n", pAssocReq->ChannelPage);
	FM_Printf(FM_USER,"PANID = %x\n", pAssocReq->CoordAddress.PANId);
	FM_Printf(FM_USER,"Address Mode = %bx\n", pAssocReq->CoordAddress.AddrMode);
#endif

#ifdef HOST_INTF_EN
	mac_api_mlme_associate_req (pAssocReq->LogicalChannel,
	                                 pAssocReq->ChannelPage,
	                                 &pAssocReq->CoordAddress,
	                                 pAssocReq->CapabilityInformation,
	                                 &pAssocReq->Security);
#else
	usr_mlme_associate_conf(0x0002, 0x00);//Remove this after Zigbee + PLC integration		
#endif
		
		break;
	}
					
	case ZB_MLME_ASSOCIATE_RES:{
	
		host_mlme_associate_resp_t *pAssocRes;
		pAssocRes = (host_mlme_associate_resp_t *)pos;

#ifdef HOST_ENDIAN

		temp = le32_to_cpu(pAssocRes->DeviceAddress.hi_u32);
		pAssocRes->DeviceAddress.hi_u32 = le32_to_cpu(pAssocRes->DeviceAddress.lo_u32);
		pAssocRes->DeviceAddress.lo_u32 = temp;	
		
		pAssocRes->AssocShortAddress = le16_to_cpu(pAssocRes->AssocShortAddress);
#endif

		#ifdef SAP_DEBUG
		FM_Printf(FM_USER,"\nMLME Associate Response\n");
		FM_Printf(FM_USER,"Device Address = %lx %lx\n",pAssocRes->DeviceAddress.hi_u32,pAssocRes->DeviceAddress.lo_u32);
		FM_Printf(FM_USER,"Assoc Short Address = %x\n",pAssocRes->AssocShortAddress);
		#endif
		
		#ifdef HOST_INTF_EN
		mac_api_mlme_associate_resp(pAssocRes->DeviceAddress,
	                             pAssocRes->AssocShortAddress,
	                             pAssocRes->status,
	                             &pAssocRes->Security);
		#else
		
		#endif
	  
		break;
	
	}
	
	case ZB_MLME_DISASSOCIATE_REQ:{
	
		host_mlme_disassoc_req_t *pDisassocReq;
		pDisassocReq = (host_mlme_disassoc_req_t *)pos;

#ifdef HOST_ENDIAN

		pDisassocReq->DeviceAddress.PANId = le16_to_cpu(pDisassocReq->DeviceAddress.PANId);

		if(pDisassocReq->DeviceAddress.AddrMode == WPAN_ADDRMODE_SHORT)
		{
			pDisassocReq->DeviceAddress.Addr.short_address = 
								le16_to_cpu(pDisassocReq->DeviceAddress.Addr.short_address);
		}
		else
		{
			temp = le32_to_cpu(pDisassocReq->DeviceAddress.Addr.long_address.hi_u32);
			pDisassocReq->DeviceAddress.Addr.long_address.hi_u32 = le32_to_cpu(pDisassocReq->DeviceAddress.Addr.long_address.lo_u32);
			pDisassocReq->DeviceAddress.Addr.long_address.lo_u32 = temp;	
		}
#endif
#ifdef SAP_DEBUG
FM_Printf(FM_USER,"\nMLME Disassociate Request\n");
FM_Printf(FM_USER,"Disassociation Reason = %bx\n",pDisassocReq->DisassociateReason);
FM_Printf(FM_USER,"Address Mode = %xx\n",pDisassocReq->DeviceAddress.AddrMode);
FM_Printf(FM_USER,"PAN ID = %x\n",pDisassocReq->DeviceAddress.PANId);
FM_Printf(FM_USER,"Device Address = %lx %lx\n",pDisassocReq->DeviceAddress.Addr.long_address.hi_u32,\
									pDisassocReq->DeviceAddress.Addr.long_address.lo_u32);
#endif
		
#ifdef HOST_INTF_EN
mac_api_mlme_disassociate_req (&pDisassocReq->DeviceAddress,
	                               pDisassocReq->DisassociateReason,
	                               pDisassocReq->TxIndirect,
	                               &pDisassocReq->Security);
#else
FM_Printf(FM_USER,"Unimplemented in Zigbee Lower MAC\n");
#endif
		
		break;
	}
				
	case ZB_MLME_ORPHAN_RES:{
	
		host_mlme_orphan_resp_t *pOrphanRes;
		pOrphanRes = (host_mlme_orphan_resp_t *)pos;

#ifdef HOST_ENDIAN

		temp = le32_to_cpu(pOrphanRes->OrphanAddress.hi_u32);
		pOrphanRes->OrphanAddress.hi_u32 = le32_to_cpu(pOrphanRes->OrphanAddress.lo_u32);
		pOrphanRes->OrphanAddress.lo_u32 = temp;

		pOrphanRes->ShortAddress = le16_to_cpu(pOrphanRes->ShortAddress);
#endif

#ifdef SAP_DEBUG
FM_Printf(FM_USER,"\nMLME Orphan Response\n");
FM_Printf(FM_USER,"Short Address = %x\n",pOrphanRes->ShortAddress);
FM_Printf(FM_USER,"Device Address = %lx %lx\n",pOrphanRes->OrphanAddress.hi_u32,\
									pOrphanRes->OrphanAddress.lo_u32);
FM_Printf(FM_USER,"Associated Member = %bx\n",pOrphanRes->AssociatedMember);

#endif
		
#ifdef HOST_INTF_EN
mac_api_mlme_orphan_resp (pOrphanRes->OrphanAddress,
                               pOrphanRes->ShortAddress,
                               pOrphanRes->AssociatedMember,
                               &pOrphanRes->Security);
#else
usr_mlme_orphan_ind(pOrphanRes->OrphanAddress);//Remove this after Zigbee + PLC integration
#endif
		break;
	}
					
	case ZB_MLME_RESET_REQUEST:{
	
		host_mlme_reset_req_t *pResetReq;
		pResetReq = (host_mlme_reset_req_t *)pos;		

		#ifdef SAP_DEBUG
		FM_Printf(FM_USER,"\nMLME Reset Request\n");
		FM_Printf(FM_USER,"SetDefaultPIB = %bx\n",pResetReq->SetDefaultPIB);
		#endif
		
		#ifdef HOST_INTF_EN	
		mac_api_mlme_reset_req (pResetReq->SetDefaultPIB);
		#else
		usr_mlme_reset_conf(0x00);//Remove this after Zigbee + PLC integration	
		#endif
		
		break;
	
	}				
	case ZB_MLME_GET_REQUEST:{
	
		host_mlme_get_req_t *pGetReq;
		pGetReq = (host_mlme_get_req_t *)pos;
		
		#ifdef SAP_DEBUG
		FM_Printf(FM_USER,"\nMLME Get Req\r\n");
		FM_Printf(FM_USER,"PIBAttr = %bx\r\n",pGetReq->PIBAttribute);
		FM_Printf(FM_USER,"PIBAttrIndex = %bx\r\n",pGetReq->PIBAttributeIndex);
		#endif
		
		#ifdef HOST_INTF_EN
		mac_api_mlme_get_req (pGetReq->PIBAttribute, 
								pGetReq->PIBAttributeIndex);
		#else //Remove this after Zigbee + PLC integration	
		{
		host_mlme_get_conf_t GetConf;
		GetConf.PIBAttribute = pGetReq->PIBAttribute;
		GetConf.PIBAttributeIndex = pGetReq->PIBAttributeIndex;	
		GetConf.PIBAttributeValue.pib_value_16bit = 0x4321;	//PAN ID
		usr_mlme_get_conf(0x00,
                  GetConf.PIBAttribute,
                  GetConf.PIBAttributeIndex,
                  &GetConf.PIBAttributeValue); // PAN ID
		}
		#endif						
								
		break;
	}
					
	case ZB_MLME_SET_REQUEST:{
	
		host_mlme_set_req_t *pSetReq;
		pSetReq = (host_mlme_set_req_t *)pos;

#ifdef HOST_ENDIAN	
		switch(mac_get_pib_attribute_size(pSetReq->PIBAttribute)){
			case sizeof(uint16_t):
				pSetReq->PIBAttributeValue.pib_value_16bit = le16_to_cpu(pSetReq->PIBAttributeValue.pib_value_16bit);
				break;
			
			case sizeof(uint32_t):
				pSetReq->PIBAttributeValue.pib_value_32bit = le32_to_cpu(pSetReq->PIBAttributeValue.pib_value_32bit);	
				break;	
	
			case sizeof(uint64_t):	
				temp = le32_to_cpu(pSetReq->PIBAttributeValue.pib_value_64bit.hi_u32);		
			    pSetReq->PIBAttributeValue.pib_value_64bit.hi_u32 = 
						                                   le32_to_cpu(pSetReq->PIBAttributeValue.pib_value_64bit.lo_u32);
				pSetReq->PIBAttributeValue.pib_value_64bit.lo_u32 = temp;	
				break;	
			
			default:
				break;
		}
#endif

#ifdef SAP_DEBUG
	FM_Printf(FM_USER,"\nMLME Set Req\r\n");
	FM_Printf(FM_USER,"PIBAttr = %bx\n",pSetReq->PIBAttribute);
	FM_Printf(FM_USER,"PIBAttrIndex = %bx\n",pSetReq->PIBAttributeIndex);
	FM_Printf(FM_USER,"PIBAttrVal = %lx %lx\n",pSetReq->PIBAttributeValue.pib_value_64bit.hi_u32,\
												pSetReq->PIBAttributeValue.pib_value_64bit.lo_u32);
#endif

#ifdef HOST_INTF_EN
	mac_api_mlme_set_req (pSetReq->PIBAttribute,
	   						pSetReq->PIBAttributeIndex,
	   							(void *)&pSetReq->PIBAttributeValue);
#else //Remove this after Zigbee + PLC integration	
	usr_mlme_set_conf(0x00, pSetReq->PIBAttribute,
	          				pSetReq->PIBAttributeIndex);										
#endif
		
		break;
	}
	
	case ZB_MLME_RX_ENABLE_REQUEST:{
	
		host_mlme_rx_enable_req_t *pRxEnbReq;
		pRxEnbReq = (host_mlme_rx_enable_req_t *)pos;
#ifdef HOST_ENDIAN
		pRxEnbReq->RxOnTime = le32_to_cpu(pRxEnbReq->RxOnTime);
		pRxEnbReq->RxOnDuration = le32_to_cpu(pRxEnbReq->RxOnDuration);
#endif

#ifdef SAP_DEBUG
	FM_Printf(FM_USER,"\nMLME RX E Req\r\n");
	FM_Printf(FM_USER,"DeferP = %bx\r\n",pRxEnbReq->DeferPermit);
	FM_Printf(FM_USER,"RxOnT = %lx\r\n",pRxEnbReq->RxOnTime);
	FM_Printf(FM_USER,"RxOnD = %lx\r\n",pRxEnbReq->RxOnDuration);
#endif

#ifdef HOST_INTF_EN
	mac_api_mlme_rx_enable_req (pRxEnbReq->DeferPermit,
	                                 pRxEnbReq->RxOnTime,
	                                 pRxEnbReq->RxOnDuration);
#else
	usr_mlme_rx_enable_conf(0x00);//Remove this after Zigbee + PLC integration	
#endif
		
		break;
	}
					
	case ZB_MLME_SCAN_REQUEST:{
	
		host_mlme_scan_req_t *pScanReq;
		pScanReq = (host_mlme_scan_req_t *)pos;
		
#ifdef HOST_ENDIAN		
		pScanReq->ScanChannels = le32_to_cpu(pScanReq->ScanChannels);
#endif

#ifdef SAP_DEBUG
	FM_Printf(FM_USER,"\nMLME Scan Req\r\n");
	FM_Printf(FM_USER,"Scan Type = %bx\r\n",pScanReq->ScanType);
	FM_Printf(FM_USER,"Scan Channels = %lx\r\n",pScanReq->ScanChannels);
	FM_Printf(FM_USER,"Scan Duration = %bx\r\n",pScanReq->ScanDuration);
	FM_Printf(FM_USER,"Channel Page = %bx\r\n",pScanReq->ChannelPage);
#endif
		
#ifdef HOST_INTF_EN
	mac_api_mlme_scan_req (pScanReq->ScanType,
	                            pScanReq->ScanChannels,
	                            pScanReq->ScanDuration,
	                            pScanReq->ChannelPage,
	                            &pScanReq->Security);	
#else

#endif		
		break;
	}
	
	case ZB_MLME_SYNC_REQUEST:{
	
		host_mlme_sync_req_t *pSyncReq;
		pSyncReq = (host_mlme_sync_req_t *)pos;

#ifdef SAP_DEBUG
	FM_Printf(FM_USER,"\nMLME Sync Req\r\n");
	FM_Printf(FM_USER,"LC = %bx\r\n",pSyncReq->LogicalChannel);
	FM_Printf(FM_USER,"Ch Page = %lx\r\n",pSyncReq->ChannelPage);
	FM_Printf(FM_USER,"Track Beacon = %bx\r\n",pSyncReq->TrackBeacon);
#endif
		
#ifdef HOST_INTF_EN
	mac_api_mlme_sync_req (pSyncReq->LogicalChannel,
	                            pSyncReq->ChannelPage,
	                            pSyncReq->TrackBeacon);
#else					 //Remove this after Zigbee + PLC integration									
	usr_mlme_sync_loss_ind(0xE0,
	               0x4321,
	               pSyncReq->LogicalChannel,
	               pSyncReq->ChannelPage);//MAC Beacon Loss
#endif
		break;
	}
	
	case ZB_MLME_POLL_REQUEST:{
	
		host_mlme_poll_req_t *pPollReq;
		pPollReq = (host_mlme_poll_req_t *)pos;

#ifdef HOST_ENDIAN

		pPollReq->CoordAddress.PANId = le16_to_cpu(pPollReq->CoordAddress.PANId);

		if(pPollReq->CoordAddress.AddrMode == WPAN_ADDRMODE_SHORT){
			pPollReq->CoordAddress.Addr.short_address = le16_to_cpu(pPollReq->CoordAddress.Addr.short_address);
		} else {
			temp = le32_to_cpu(pPollReq->CoordAddress.Addr.long_address.hi_u32);
			pPollReq->CoordAddress.Addr.long_address.hi_u32 = 
			                             le32_to_cpu(pPollReq->CoordAddress.Addr.long_address.lo_u32);
			pPollReq->CoordAddress.Addr.long_address.lo_u32 = temp;
		}
#endif

#ifdef SAP_DEBUG
	FM_Printf(FM_USER,"\nMLME Poll Req\r\n");
	FM_Printf(FM_USER,"Coord PAN ID = %x\r\n",pPollReq->CoordAddress.PANId);
	FM_Printf(FM_USER,"Coord Addr = %lx %lx\r\n",pPollReq->CoordAddress.Addr.long_address.hi_u32,\
												pPollReq->CoordAddress.Addr.long_address.lo_u32);
#endif

#ifdef HOST_INTF_EN
	mac_api_mlme_poll_req (&pPollReq->CoordAddress,
                            &pPollReq->Security);
#else
	usr_mlme_poll_conf(0x00);//Remove this after Zigbee + PLC integration	
#endif
		
		break;
	}	
	
	default:
		break;
					
	}
}


eStatus mac_sap_builder(uint8_t *buffer , uint16_t packetlen)
{
	uint8_t evtClass;
	uint8_t evtType;
	sEvent  *event = NULL;
	sNma    *nma   = HOMEPLUG_GetNma();
	eStatus ret    = STATUS_FAILURE;

	
	evtClass = EVENT_CLASS_CTRL;
	evtType  = *buffer;

	event = NMA_EncodeRsp(evtType, buffer, packetlen);			


    if(event != NULL)
    {
        NMA_TransmitMgmtMsg(nma, event);
        ret = STATUS_SUCCESS;
    }
    
    return ret;
	
}	

void usr_mcps_data_conf(uint8_t msduHandle, uint8_t status, uint32_t Timestamp)
{
	host_mcps_data_confirm_t 	WpanCnf;


	WpanCnf.command 	= 	ZB_MCPS_DATA_CONFIRM;
	WpanCnf.result 		= 	status;
	WpanCnf.msduHandle 	= 	msduHandle;
	WpanCnf.status 		= 	status;
	WpanCnf.Timestamp	= 	Timestamp;

#ifdef SAP_DEBUG
	FM_Printf(FM_USER,"MCPS Confirm\r\n");
#endif

	mac_sap_builder((uint8_t *)&WpanCnf,sizeof(host_mcps_data_confirm_t));
}


void usr_mcps_data_ind(wpan_addr_spec_t *src_addr_info_p, wpan_addr_spec_t *dst_addr_info_p,
							uint8_t msduLength, uint8_t *msdu_p, uint8_t mpduLinkQuality, uint8_t DSN,
							uint32_t Timestamp, uint8_t SecurityLevel, uint8_t KeyIdMode, uint8_t KeyIndex)
{
	host_mcps_data_ind_t 		WpanInd;
	uint16_t					mcps_data_ind_size;


	mcps_data_ind_size = sizeof(host_mcps_data_ind_t) - ZIGBEE_PAYLOAD_SIZE + msduLength;

	WpanInd.command = ZB_MCPS_DATA_INDICATION;

	WpanInd.SrcAddrMode 				= 	src_addr_info_p->AddrMode;
	WpanInd.SrcPANId 					= 	src_addr_info_p->PANId;


	WpanInd.SrcAddr.hi_u32		    	= 	src_addr_info_p->Addr.long_address.hi_u32;
	WpanInd.SrcAddr.lo_u32			    = 	src_addr_info_p->Addr.long_address.lo_u32;


	WpanInd.DstAddrMode					=	dst_addr_info_p->AddrMode;
	WpanInd.DstPANId					=	dst_addr_info_p->PANId;


	WpanInd.DstAddr.hi_u32				=	dst_addr_info_p->Addr.long_address.hi_u32;
	WpanInd.DstAddr.lo_u32				=	dst_addr_info_p->Addr.long_address.lo_u32;

	WpanInd.msduLength					=	msduLength;

	memcpy(WpanInd.msdu,msdu_p,msduLength);
													   
	WpanInd.mpduLinkQuality				=	mpduLinkQuality;
	WpanInd.DSN							=	DSN;
	WpanInd.Timestamp					=	Timestamp;
	WpanInd.Security.SecurityLevel		=	SecurityLevel;
	WpanInd.Security.KeyIdMode			=	KeyIdMode;
	WpanInd.Security.KeyIndex			=	KeyIndex;

#ifdef SAP_DEBUG
	FM_Printf(FM_USER,"MCPS Indication\r\n");
	FM_Printf(FM_USER,"MCPS Data Indication Size = %u\n",mcps_data_ind_size);
#endif

	mac_sap_builder((uint8_t *) &WpanInd,mcps_data_ind_size);

}



void usr_mcps_purge_conf(uint8_t msduHandle, uint8_t status)
{
  
	host_mcps_purge_conf_t McpsPurge;

	McpsPurge.command = ZB_MCPS_PURGE_CONFIRM;
	McpsPurge.msduHandle = msduHandle;

	McpsPurge.status  = status;
	McpsPurge.result  = status;

#ifdef SAP_DEBUG
	FM_Printf(FM_USER,"MCPS Purge\r\n");
#endif

	mac_sap_builder((uint8_t *) &McpsPurge,sizeof(host_mcps_purge_conf_t));

}


void usr_mlme_start_conf(uint8_t status)
{
	host_mlme_start_conf_t StartConf;

	StartConf.command = ZB_MLME_START_CONFIRM;
	StartConf.status  = status;
	StartConf.result  = status;

#ifdef SAP_DEBUG
	FM_Printf(FM_USER,"\nMLME Start Conf\r\n");
	FM_Printf(FM_USER,"Status = %bx",StartConf.status);
#endif

	mac_sap_builder((uint8_t *)&StartConf,sizeof(host_mlme_start_conf_t));
}



void usr_mlme_associate_conf(uint16_t AssocShortAddress, uint8_t status)
{

	host_mlme_associate_cnf_t AssocCnf;

	AssocCnf.command 			 = ZB_MLME_ASSOCIATE_CONFIRM;
	AssocCnf.AssocShortAddress 	 = AssocShortAddress;
	AssocCnf.status 			 = status;
	AssocCnf.result 			 = status;

#ifdef SAP_DEBUG
	FM_Printf(FM_USER,"\nMLME Assoc Conf\n");
	FM_Printf(FM_USER,"AssocShortAddress = %x\n",AssocCnf.AssocShortAddress);
#endif

	mac_sap_builder((uint8_t *)&AssocCnf,sizeof(host_mlme_associate_cnf_t));
}



void usr_mlme_associate_ind(uint64_t DeviceAddress, uint8_t CapabilityInformation)
{
	host_mlme_associate_ind_t AssocInd;

	AssocInd.command 				 = ZB_MLME_ASSOCIATE_INDICATION;

	AssocInd.DeviceAddress.lo_u32    = DeviceAddress.lo_u32;
	AssocInd.DeviceAddress.hi_u32    = DeviceAddress.hi_u32;

	AssocInd.CapabilityInformation   = CapabilityInformation;

#ifdef SAP_DEBUG
	FM_Printf(FM_USER,"MLME Assoc Indication\r\n");
#endif

#ifdef HOST_INTF_EN
	mac_api_mlme_associate_resp(DeviceAddress,
		                         0x0002,
		                         0x00,
		                         NULL);// This is only for testing. In future state machine will take care of this response 
#endif

	mac_sap_builder((uint8_t *)&AssocInd,sizeof(host_mlme_associate_ind_t));

}




void usr_mlme_beacon_notify_ind(uint8_t BSN,               // BSN
                               pandescriptor_t * PANDescriptor,  // PANDescriptor
                               uint8_t PendAddrSpec,      // PendAddrSpec
                               uint8_t *AddrList,          // AddrList
                               uint8_t sduLength,         // sduLength
                               uint8_t *sdu)              // sdu
{
	uint8_t lAddressListLen;
	host_mlme_beacon_notify_ind_t BeaconInd;

	BeaconInd.command 						  					 = ZB_MLME_BEACON_NOTIFY_INDICATION;
	BeaconInd.BSN 							    				 = BSN;

	BeaconInd.PANDescriptor.CoordAddrSpec.PANId 				 = PANDescriptor->CoordAddrSpec.PANId;
	BeaconInd.PANDescriptor.CoordAddrSpec.Addr.long_address.hi_u32 = PANDescriptor->\
																		CoordAddrSpec.Addr.long_address.hi_u32;
	BeaconInd.PANDescriptor.CoordAddrSpec.Addr.long_address.lo_u32 = PANDescriptor->\
																		CoordAddrSpec.Addr.long_address.lo_u32;
	BeaconInd.PANDescriptor.LogicalChannel						 = PANDescriptor->LogicalChannel;
	BeaconInd.PANDescriptor.GTSPermit								 = PANDescriptor->GTSPermit;
	BeaconInd.PANDescriptor.LinkQuality							 = PANDescriptor->LinkQuality;

	BeaconInd.PANDescriptor.SuperframeSpec 						 = PANDescriptor->SuperframeSpec;
	BeaconInd.PANDescriptor.TimeStamp 							 = PANDescriptor->TimeStamp;
	BeaconInd.uPendAddrSpec.PendAddrSpec  						 = PendAddrSpec;

	lAddressListLen = BeaconInd.uPendAddrSpec.s.shortaddr + BeaconInd.uPendAddrSpec.s.extaddr;
	BeaconInd.sduLength = sduLength;

	if(lAddressListLen <= MAX_ADDRESS_LIST){
		memcpy(BeaconInd.AddrList,AddrList,lAddressListLen );
	} else {
		return;// to prevent memory corruption in case Address List length is more than 7
	}
	
	memcpy(BeaconInd.sdu,sdu,sduLength);

#ifdef SAP_DEBUG
	FM_Printf(FM_USER,"MLME Beacon Indication\r\n");
#endif

	mac_sap_builder((uint8_t *) &BeaconInd,sizeof(host_mlme_beacon_notify_ind_t));

}

void usr_mlme_comm_status_ind(wpan_addr_spec_t * src_addr,
                             wpan_addr_spec_t * dst_addr,
                             uint8_t status)
{

	host_mlme_comm_status_ind_t CommStatusInd;

	CommStatusInd.command 						   = ZB_MLME_COMM_STATUS_INDICATION;
	CommStatusInd.SrcAddr.Addr.long_address.hi_u32 = src_addr->Addr.long_address.hi_u32;
	CommStatusInd.SrcAddr.Addr.long_address.lo_u32 = src_addr->Addr.long_address.lo_u32;
	CommStatusInd.SrcAddr.AddrMode 				   = src_addr->AddrMode;
	CommStatusInd.SrcAddr.PANId 		           = src_addr->PANId;
	CommStatusInd.DstAddr.Addr.long_address.hi_u32 = dst_addr->Addr.long_address.hi_u32;
	CommStatusInd.DstAddr.Addr.long_address.lo_u32 = dst_addr->Addr.long_address.lo_u32;
	CommStatusInd.DstAddr.AddrMode 	               = dst_addr->AddrMode;
	CommStatusInd.DstAddr.PANId 				   = dst_addr->PANId;
	CommStatusInd.status 				   		   = status;

#ifdef SAP_DEBUG
	FM_Printf(FM_USER,"MLME Comm Status Ind\n");
#endif

	mac_sap_builder((uint8_t *) &CommStatusInd,sizeof(host_mlme_comm_status_ind_t));

}

void usr_mlme_disassociate_conf(uint8_t status,
                               wpan_addr_spec_t * device_addr)
{

	host_mlme_disassoc_cnf_t DisassocCnf;

	DisassocCnf.command 								 = ZB_MLME_DISASSOCIATE_CONFIRM;
	DisassocCnf.DeviceAddress.Addr.long_address.hi_u32	 = device_addr->Addr.\
																		long_address.hi_u32;
	DisassocCnf.DeviceAddress.Addr.long_address.lo_u32 	 = device_addr->Addr.\
																		long_address.lo_u32;
	DisassocCnf.DeviceAddress.AddrMode 				 	 = device_addr->AddrMode;
	DisassocCnf.DeviceAddress.PANId 					 = device_addr->PANId;
	DisassocCnf.status 								 	 = status;
	DisassocCnf.result 								 	 = status;

#ifdef SAP_DEBUG
	FM_Printf(FM_USER,"MLME Disassoc Conf\n");
#endif

	mac_sap_builder((uint8_t *) &DisassocCnf,sizeof(host_mlme_disassoc_cnf_t));

}

void usr_mlme_get_conf(uint8_t status,
                      uint8_t PIBAttribute,
                      uint8_t PIBAttributeIndex,
                      pib_value_t *PIBAttributeValue)
{
	host_mlme_get_conf_t GetConf;

	GetConf.command 								  = ZB_MLME_GET_CONFIRM;

	GetConf.PIBAttribute 							  = PIBAttribute;
	GetConf.PIBAttributeIndex 						  = PIBAttributeIndex;

	switch(mac_get_pib_attribute_size(GetConf.PIBAttribute)){
		case sizeof(uint8_t):
			GetConf.PIBAttributeValue.pib_value_8bit = PIBAttributeValue->pib_value_8bit;
			break;
		case sizeof(uint16_t):
			GetConf.PIBAttributeValue.pib_value_16bit = PIBAttributeValue->pib_value_16bit;		
			break;
		
		case sizeof(uint32_t):
			GetConf.PIBAttributeValue.pib_value_32bit = PIBAttributeValue->pib_value_32bit;		
			break;
			
		case sizeof(uint64_t):
			GetConf.PIBAttributeValue.pib_value_64bit.lo_u32 = PIBAttributeValue->pib_value_64bit.lo_u32;		
			GetConf.PIBAttributeValue.pib_value_64bit.hi_u32 = PIBAttributeValue->pib_value_64bit.hi_u32;			
			break;
		
		default:
			break;
	}

	GetConf.status 	= status;
	GetConf.result  = status;

#ifdef SAP_DEBUG
	FM_Printf(FM_USER,"MLME Get Conf\r\n");
#endif

	mac_sap_builder((uint8_t *) &GetConf,sizeof(host_mlme_get_conf_t));

}

void usr_mlme_disassociate_ind(uint64_t DeviceAddress,
                              uint8_t DisassociateReason)
{
	host_mlme_disassoc_ind_t DisassocInd;

	DisassocInd.command 			 = ZB_MLME_DISASSOCIATE_INDICATION;

	DisassocInd.DeviceAddress.hi_u32 = DeviceAddress.hi_u32;
	DisassocInd.DeviceAddress.lo_u32 = DeviceAddress.lo_u32;
	DisassocInd.DisassociateReason 	 = DisassociateReason;

#ifdef SAP_DEBUG
	FM_Printf(FM_USER,"MLME Disassociate Ind\r\n");
#endif

	mac_sap_builder((uint8_t *)&DisassocInd,sizeof(host_mlme_disassoc_ind_t));

}

void usr_mlme_orphan_ind(uint64_t OrphanAddress)
{
	host_mlme_orphan_ind_t OrphanInd;

	OrphanInd.command 			 = ZB_MLME_ORPHAN_INDICATION;
	OrphanInd.OrphanAddress.hi_u32 = OrphanAddress.hi_u32;
	OrphanInd.OrphanAddress.lo_u32 = OrphanAddress.lo_u32;

#ifdef SAP_DEBUG
	FM_Printf(FM_USER,"MLME Orphan Ind\r\n");
#endif

	mac_sap_builder((uint8_t *) &OrphanInd,sizeof(host_mlme_orphan_ind_t));

}

void usr_mlme_poll_conf(uint8_t status)
{
	host_mlme_poll_conf_t PollConf;

	PollConf.command = ZB_MLME_POLL_CONFIRM;
	PollConf.status  = status;
	PollConf.result  = status;

#ifdef SAP_DEBUG
	FM_Printf(FM_USER,"MLME Poll Conf\r\n");
#endif

	mac_sap_builder((uint8_t *) &PollConf,sizeof(host_mlme_poll_conf_t));

}

void usr_mlme_reset_conf(uint8_t status)
{
	host_mlme_reset_conf_t ResetConf;

	ResetConf.command = ZB_MLME_RESET_CONFIRM;
	ResetConf.status  = status;
	ResetConf.result  = status;

#ifdef SAP_DEBUG
	FM_Printf(FM_USER,"MLME Reset Conf\r\n");
#endif

	mac_sap_builder((uint8_t *) &ResetConf,sizeof(host_mlme_reset_conf_t));

}

void usr_mlme_rx_enable_conf(uint8_t status)
{
	host_mlme_rx_enable_conf_t RxEnable;

	RxEnable.command = ZB_MLME_RX_ENABLE_CONFIRM; 
	RxEnable.status  = status;
	RxEnable.result  = status;

#ifdef SAP_DEBUG
	FM_Printf(FM_USER,"MLME Rx Enable Conf\r\n");
#endif 

	mac_sap_builder((uint8_t *) &RxEnable,sizeof(host_mlme_rx_enable_conf_t));

}

void usr_mlme_scan_conf(uint8_t status,
                       uint8_t ScanType,
                       uint8_t ChannelPage,
                       uint32_t UnscannedChannels,
                       uint8_t ResultListSize,
                       scan_result_list_t *scan_result_list)
{

	host_mlme_scan_conf_t ScanConf;

	ScanConf.command 			 							= ZB_MLME_SCAN_CONFIRM;

	ScanConf.status            								= status;
	ScanConf.result            								= status;

	ScanConf.ScanType          								= ScanType;
	ScanConf.ChannelPage 		 							= ChannelPage;
	ScanConf.UnscannedChannels 								= UnscannedChannels;
	ScanConf.ResultListSize    								= ResultListSize;

	if((ScanConf.ScanType == MLME_SCAN_TYPE_ED) || (ScanConf.ScanType == MLME_SCAN_TYPE_ORPHAN)){
		ScanConf.scan_result_list.ed_value[0] 					= scan_result_list->ed_value[0];
	} else {
		ScanConf.scan_result_list.PANDescriptor.ChannelPage 	= scan_result_list->PANDescriptor.ChannelPage;
		ScanConf.scan_result_list.PANDescriptor.SuperframeSpec  = scan_result_list->\
																				PANDescriptor.SuperframeSpec;
		ScanConf.scan_result_list.PANDescriptor.GTSPermit 		= scan_result_list->PANDescriptor.GTSPermit;
		ScanConf.scan_result_list.PANDescriptor.LinkQuality 	= scan_result_list->PANDescriptor.LinkQuality;
		ScanConf.scan_result_list.PANDescriptor.TimeStamp 		= scan_result_list->PANDescriptor.TimeStamp;
	}

#ifdef SAP_DEBUG
	FM_Printf(FM_USER,"MLME Scan Confirm\r\n");
#endif

	mac_sap_builder((uint8_t *) &ScanConf,sizeof(host_mlme_scan_conf_t));

}

void usr_mlme_set_conf(uint8_t status, uint8_t PIBAttribute,
                      uint8_t PIBAttributeIndex)
{
	host_mlme_set_conf_t SetConf;

	SetConf.command 			= ZB_MLME_SET_CONFIRM;
	SetConf.status 				= status;
	SetConf.result 				= status;

	SetConf.PIBAttribute        = PIBAttribute;
	SetConf.PIBAttributeIndex   = PIBAttributeIndex;

#ifdef SAP_DEBUG
	FM_Printf(FM_USER,"MLME Set Conf\r\n");
#endif

	mac_sap_builder((uint8_t *) &SetConf,sizeof(host_mlme_set_conf_t));

}

void usr_mlme_sync_loss_ind(uint8_t LossReason,
                           uint16_t PANId,
                           uint8_t LogicalChannel,
                           uint8_t ChannelPage)
{

	host_mlme_synhostc_loss_ind_t SyncLoss;

	SyncLoss.command 		  = ZB_MLME_SYNC_LOSS_INDICATION;
	SyncLoss.LossReason 	  = LossReason;
	SyncLoss.PANId 		  	  = PANId;
	SyncLoss.LogicalChannel   = LogicalChannel;
	SyncLoss.ChannelPage      = ChannelPage;

#ifdef SAP_DEBUG
	FM_Printf(FM_USER,"MLME Sync Loss Ind\r\n");
#endif

	mac_sap_builder((uint8_t *) &SyncLoss,sizeof(host_mlme_synhostc_loss_ind_t));

}




