/* ========================================================
 *
 * @file:  gv701x_lrwpandriver.c
 * 
 * @brief: This file governs and maintains the IEEE 802.15.4 link
 *      
 *  Copyright (C) 2010-2015, Greenvity Communications, Inc.
 *  All Rights Reserved
 *  
 * =========================================================*/

#ifdef LRWPAN_DRIVER_APP

/****************************************************************************** 
  *	Includes
  ******************************************************************************/
  
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "gv701x_includes.h"
#include "gv701x_lrwpandriver.h"
#ifdef ROUTE_APP
#include "route.h"
#endif

/****************************************************************************** 
  *	Global Data
  ******************************************************************************/

lrwpan_db_t lrwpan_db;
gv701x_state_t lrwpan_state;

/****************************************************************************** 
  *	Local Data
  ******************************************************************************/  
  
/****************************************************************************** 
  *	External Data
  ******************************************************************************/

/******************************************************************************
  *	External Funtion prototypes
  ******************************************************************************/

/******************************************************************************
  *	Funtion prototypes
  ******************************************************************************/
  
static void lrwpan_device_config(void); 
static void lrwpan_device_start(void);
void GV701x_LrwpanDriverFlush(void);

void lrwpan_SendAssocRsp(uint64_t DeviceAddress,
                             uint16_t AssocShortAddress,
                             uint8_t status,
                             security_info_t *sec_p);

void lrwpan_SendScanReq(uint8_t ScanType,
                            uint32_t ScanChannels,
                            uint8_t ScanDuration,
                            uint8_t ChannelPage,
                            security_info_t *sec_p);

void lrwpan_SendAssocReq(uint8_t LogicalChannel,
                                 uint8_t ChannelPage,
                                 wpan_addr_spec_t *CoordAddrSpec_p,
                                 uint8_t CapabilityInformation,
                                 security_info_t *sec_p);

void lrwpan_SendStartReq(uint16_t PANId,
                              uint8_t LogicalChannel,
                              uint8_t ChannelPage,
                              uint32_t StartTime,
                              uint8_t BeaconOrder,
                              uint8_t SuperframeOrder,
                              bool PANCoordinator,
                              bool BatteryLifeExtension,
                              bool CoordRealignment,
                              security_info_t *CoordRealignmentSecurity_p,
                              security_info_t *BeaconSecurity_p);

void lrwpan_SendResetReq(uint8_t SetDefaultPIB);

void GV701x_LrwpanScanInd(uint8_t* ind);

void GV701x_LrwpanBcnUpdate(mlme_beacon_notify_ind_t* bcn_ind);


/******************************************************************************
 * @fn      GV701x_LrwpanDriverInit
 *
 * @brief   Initializes the LRWPAN driver
 *
 * @param   app_id - application identification number
 *
 * @return  none
 */

void GV701x_LrwpanDriverInit(u8 app_id)
{
	u8* macaddr;
	mlme_set_req_t mlme_set;
	mlme_set.cmdcode = MLME_SET_REQUEST;
	mlme_set.PIBAttributeIndex = 0;	

	memset(&lrwpan_db, 0x00, sizeof (lrwpan_db_t));
	
	/*Record the applications id,will 
	 be used while allocating timers*/
	lrwpan_db.app_id = app_id;	
	SLIST_Init(&lrwpan_db.queues.appRxQueue);
	
	FM_Printf(FM_USER, "\nInit Lrwpan (app id %bu)", app_id);
	
	memcpy((u8*)&lrwpan_db.ieee_addr, (u8*)macaddr, MAC_ADDR_LEN);
#if 0	
	FM_HexDump(FM_APP, "MAC: ", (u8*)&lrwpan_db.ieee_addr, MAC_ADDR_LEN);			
#endif
	
	/*Initialize the state machine*/
	lrwpan_state.state = LRWPAN_IDLE;
	lrwpan_state.event = LRWPAN_IDLE_EVNT;
	lrwpan_state.statedata = NULL;
	lrwpan_state.statedatalen = 0;	

	
	lrwpan_db.dev = COORDINATOR; 
	lrwpan_db.scan.time = 5;		
	lrwpan_db.scan.type = MLME_SCAN_TYPE_ACTIVE;		
	lrwpan_db.panid = LRWPAN_PANID; 
	lrwpan_db.scan.ch_mask = LRWPAN_CHANNEL_MASK;	
	lrwpan_db.channel = LRWPAN_CHANNEL;
	lrwpan_db.scan.active = FALSE;
	lrwpan_db.start.active = FALSE; 
	lrwpan_db.cfg.active = FALSE;
	lrwpan_db.cfg.params = 0xFF;

	/* Allocate Start timer */
	lrwpan_db.start.timer = STM_AllocTimer(SW_LAYER_TYPE_APP, 
												LRWPAN_START_TIMEOUT_EVT, 
												&lrwpan_db.app_id);	
	/* Allocate Profile timer */
	lrwpan_db.profile_timer = STM_AllocTimer(SW_LAYER_TYPE_APP, 
												LRWPAN_PROFILE_TIMEOUT_EVT, 
												&lrwpan_db.app_id);	

	srand(TL0);
	lrwpan_db.short_addr = (u16)rand(); 	
#ifndef OLD_SCAN	
	lrwpan_device_config();
#else
	lrwpan_db.auto_request = TRUE;
	mlme_set.PIBAttribute = macAutoRequest;				
	memcpy((u8*)&mlme_set.PIBAttributeValue, &lrwpan_db.auto_request, sizeof(uint8_t));		
	GV701x_SendAppEvent(lrwpan_db.app_id, APP_FW_MSG_APPID, APP_MSG_TYPE_FW, IEEE802_15_4_MAC_ID,
					EVENT_CLASS_MGMT, MGMT_FRM_ID, &mlme_set, sizeof(mlme_set_req_t), 0);
#endif	
}

/******************************************************************************
 * @fn      GV701x_LrwpanDriverStart
 *
 * @brief   Starts the LRWPAN driver
 *
 * @param   app_id - application identification number
 *
 * @return  none
 */

void GV701x_LrwpanDriverStart(void)
{	
	lrwpan_state.state = LRWPAN_START;	

	/*Start profile timeout timer*/
	STM_StopTimer(lrwpan_db.profile_timer);
	STM_StopTimer(lrwpan_db.start.timer);	
	STM_StartTimer(lrwpan_db.profile_timer, LRWPAN_PROFILE_TIME);
	STM_StartTimer(lrwpan_db.start.timer, LRWPAN_START_TIME);
	
}

/******************************************************************************
 * @fn      lrwpan_set_shortaddr
 *
 * @brief   Sets the IEEE802.15.4 short address
 *
 * @param   addr - short address
 *
 * @return  none
 */

void lrwpan_set_shortaddr(uint16_t addr)
{
	mlme_set_req_t mlme_set;
	mlme_set.cmdcode = MLME_SET_REQUEST;
	mlme_set.PIBAttributeIndex = 0;

	lrwpan_db.short_addr = addr;
	mlme_set.PIBAttribute = macShortAddress;		
	memcpy((u8*)&mlme_set.PIBAttributeValue, &lrwpan_db.short_addr, sizeof(uint16_t));								
	GV701x_SendAppEvent(lrwpan_db.app_id, APP_FW_MSG_APPID, APP_MSG_TYPE_FW, IEEE802_15_4_MAC_ID,
						EVENT_CLASS_MGMT, MGMT_FRM_ID, &mlme_set, sizeof(mlme_set_req_t), 0);
}

/******************************************************************************
 * @fn      lrwpan_device_config
 *
 * @brief   Configures all the PIB's required to instantiate the device
 *
 * @param   none
 *
 * @return  none
 */

static void lrwpan_device_config(void) 
{
	mlme_set_req_t mlme_set;
	mlme_set.cmdcode = MLME_SET_REQUEST;
	mlme_set.PIBAttributeIndex = 0;	

	if(lrwpan_db.dev == COORDINATOR)
	{
		uint16_t panid = 0;
		uint8_t seq = 0x84;
		bool pan_coord = TRUE;		
		uint8_t association_permit;
		uint8_t beacon_payload_len = 10;		
		uint8_t beacon_payload[10];
		
		mlme_set.PIBAttribute = macPANId;				
		
		memcpy((u8*)&mlme_set.PIBAttributeValue, &lrwpan_db.panid, sizeof(uint16_t));		
		GV701x_SendAppEvent(lrwpan_db.app_id, APP_FW_MSG_APPID, APP_MSG_TYPE_FW, IEEE802_15_4_MAC_ID,
						EVENT_CLASS_MGMT, MGMT_FRM_ID, &mlme_set, sizeof(mlme_set_req_t), 0);			
		
		mlme_set.PIBAttribute = macIeeeAddress;		
		memcpy((u8*)&mlme_set.PIBAttributeValue, &lrwpan_db.ieee_addr, sizeof(uint64_t));			
		GV701x_SendAppEvent(lrwpan_db.app_id, APP_FW_MSG_APPID, APP_MSG_TYPE_FW, IEEE802_15_4_MAC_ID,
						EVENT_CLASS_MGMT, MGMT_FRM_ID, &mlme_set, sizeof(mlme_set_req_t), 0);
#ifndef OLD_SCAN
		mlme_set.PIBAttribute = phyCurrentChannel;		
		memcpy((u8*)&mlme_set.PIBAttributeValue, &lrwpan_db.channel, sizeof(uint8_t));					
		GV701x_SendAppEvent(lrwpan_db.app_id, APP_FW_MSG_APPID, APP_MSG_TYPE_FW, IEEE802_15_4_MAC_ID,
						EVENT_CLASS_MGMT, MGMT_FRM_ID, &mlme_set, sizeof(mlme_set_req_t), 0);
#endif		
				
		mlme_set.PIBAttribute = macShortAddress;		
		memcpy((u8*)&mlme_set.PIBAttributeValue, &lrwpan_db.short_addr, sizeof(uint16_t));								
		GV701x_SendAppEvent(lrwpan_db.app_id, APP_FW_MSG_APPID, APP_MSG_TYPE_FW, IEEE802_15_4_MAC_ID,
							EVENT_CLASS_MGMT, MGMT_FRM_ID, &mlme_set, sizeof(mlme_set_req_t), 0);
		association_permit = PERMIT_ASSOCIATION;			

		mlme_set.PIBAttribute = macAssociationPermit;		
		memcpy((u8*)&mlme_set.PIBAttributeValue, &association_permit, sizeof(uint8_t));								
		GV701x_SendAppEvent(lrwpan_db.app_id, APP_FW_MSG_APPID, APP_MSG_TYPE_FW, IEEE802_15_4_MAC_ID,
							EVENT_CLASS_MGMT, MGMT_FRM_ID, &mlme_set, sizeof(mlme_set_req_t), 0);			

		mlme_set.PIBAttribute = macBeaconPayloadLength;		
		memcpy((u8*)&mlme_set.PIBAttributeValue, &beacon_payload_len, sizeof(uint8_t));											
		GV701x_SendAppEvent(lrwpan_db.app_id, APP_FW_MSG_APPID, APP_MSG_TYPE_FW, IEEE802_15_4_MAC_ID,
							EVENT_CLASS_MGMT, MGMT_FRM_ID, &mlme_set, sizeof(mlme_set_req_t), 0);

		mlme_set.PIBAttribute = macBeaconPayload;		
		memcpy(beacon_payload, (u8*)&lrwpan_db.ieee_addr, MAC_ADDR_LEN);		
		memcpy((u8*)&mlme_set.PIBAttributeValue, beacon_payload, beacon_payload_len);											
		GV701x_SendAppEvent(lrwpan_db.app_id, APP_FW_MSG_APPID, APP_MSG_TYPE_FW, IEEE802_15_4_MAC_ID,
							EVENT_CLASS_MGMT, MGMT_FRM_ID, &mlme_set, 
							sizeof(mlme_set_req_t) - sizeof(pib_value_t) + beacon_payload_len, 0);

		mlme_set.PIBAttribute = macBSN;
		memcpy((u8*)&mlme_set.PIBAttributeValue, &seq, sizeof(uint8_t));			
		GV701x_SendAppEvent(lrwpan_db.app_id, APP_FW_MSG_APPID, APP_MSG_TYPE_FW, IEEE802_15_4_MAC_ID,
							EVENT_CLASS_MGMT, MGMT_FRM_ID, &mlme_set, sizeof(mlme_set_req_t), 0);	
	}
}

/******************************************************************************
 * @fn      lrwpan_device_start
 *
 * @brief   Start the device
 *
 * @param   device_type - ROUTER or COORDINATOR
 *
 * @return  none
 */

static void lrwpan_device_start(void)
{
	if(lrwpan_state.state != LRWPAN_START)
		return;

	if(lrwpan_db.dev == ROUTER) 
	{	
		/*Scan the network*/
		lrwpan_SendScanReq(MLME_SCAN_TYPE_ACTIVE, lrwpan_db.scan.ch_mask, 
				lrwpan_db.scan.time, 0, NULL);
	}
	else if(lrwpan_db.dev == COORDINATOR) 
	{	    		
		security_info_t CoordRealignmentSecurity;
		security_info_t BeaconSecurity;
			
		CoordRealignmentSecurity.SecurityLevel = 0;
		CoordRealignmentSecurity.KeyIdMode = 0x00;
		CoordRealignmentSecurity.KeySource[0] = 0x00;
		CoordRealignmentSecurity.KeySource[1] = 0x00;
		CoordRealignmentSecurity.KeySource[2] = 0x00;
		CoordRealignmentSecurity.KeySource[3] = 0x00;
		CoordRealignmentSecurity.KeySource[4] = 0x00;
		CoordRealignmentSecurity.KeySource[5] = 0x00;
		CoordRealignmentSecurity.KeySource[6] = 0x00;
		CoordRealignmentSecurity.KeySource[7] = 0x00;
		CoordRealignmentSecurity.KeyIndex = 0x01;
		
		BeaconSecurity.SecurityLevel = 0;
		BeaconSecurity.KeyIdMode = 0x00;
		BeaconSecurity.KeySource[0] = 0x00;
		BeaconSecurity.KeySource[1] = 0x00;
		BeaconSecurity.KeySource[2] = 0x00;
		BeaconSecurity.KeySource[3] = 0x00;
		BeaconSecurity.KeySource[4] = 0x00;
		BeaconSecurity.KeySource[5] = 0x00;
		BeaconSecurity.KeySource[6] = 0x00;
		BeaconSecurity.KeySource[7] = 0x00;
		BeaconSecurity.KeyIndex = 0x01;

		/*Start the network on the specified PAN*/
		lrwpan_SendStartReq(lrwpan_db.panid, lrwpan_db.channel,
							0, 0, 0x0F, 0x0F, 0, 0, 0,
							&CoordRealignmentSecurity,
                            &BeaconSecurity);			
	}
}

/******************************************************************************
 * @fn      GV701x_LrwpanDriverRxAppMsg
 *
 * @brief   Receives a message from another app/fw
 *
 * @params  msg_buf - message buffer
 *
 * @return  none
 */

void GV701x_LrwpanDriverRxAppMsg(sEvent* event)
{
	gv701x_app_msg_hdr_t* msg_hdr = (gv701x_app_msg_hdr_t*)event->buffDesc.dataptr;
	hostHdr_t* hybrii_hdr;
	hostEventHdr_t* evnt_hdr;

	hybrii_hdr = (hostHdr_t*)(msg_hdr + 1);
	
	if(msg_hdr->dst_app_id == lrwpan_db.app_id)
	{
		memcpy(&lrwpan_state.msg_hdr, msg_hdr, sizeof(gv701x_app_msg_hdr_t));
		lrwpan_state.eventproto = hybrii_hdr->protocol;
		if((msg_hdr->src_app_id == APP_FW_MSG_APPID) && 
			(hybrii_hdr->type == EVENT_FRM_ID))
		{
			evnt_hdr = (hostEventHdr_t*)(hybrii_hdr + 1);
			lrwpan_state.event = evnt_hdr->type; 		
			lrwpan_state.statedata = (u8*)(evnt_hdr + 1);
			lrwpan_state.statedatalen = (u16)(hybrii_hdr->length - sizeof(hostEventHdr_t));			
		}
		else
		{
			lrwpan_state.event = (u8)(*((u8*)(hybrii_hdr + 1)));
			lrwpan_state.statedata = (u8*)(hybrii_hdr + 1);
			lrwpan_state.statedatalen = (u16)hybrii_hdr->length;
		}		
		lrwpan_state.eventtype = hybrii_hdr->type;
		lrwpan_state.eventclass = event->eventHdr.eventClass;
		if((msg_hdr->src_app_id == APP_FW_MSG_APPID) && 
			(hybrii_hdr->type == EVENT_FRM_ID) &&
			(lrwpan_state.event == HOST_EVENT_APP_TIMER))
		{			
			GV701x_LrwpanTimerHandler((u8*)(evnt_hdr + 1)); 
			return;
		}		
		if((msg_hdr->src_app_id == APP_FW_MSG_APPID) && 
			(hybrii_hdr->type == EVENT_FRM_ID) &&
			(lrwpan_state.event == HOST_EVENT_APP_CMD))
		{			
			GV701x_LrwpanCmdProcess((char*)(evnt_hdr + 1)); 
			return;
		}				
	}	
	else if(msg_hdr->dst_app_id == APP_BRDCST_MSG_APPID)
	{
		u8 *event = (u8*)(hybrii_hdr + 1);
		return;
	}
	GV701x_LrwpanDriverSM(&lrwpan_state);
}

/******************************************************************************
 * @fn      lrwpan_mlme_assoc_ind
 *
 * @brief   Association Indication coming from a peer device 
 *
 * @param   address - the address of the device requesting association
 *			capability - the operational capabilities of the device requesting association
 *
 * @return  none
 */

void lrwpan_mlme_assoc_ind(uint8_t* buf)
{
	mlme_associate_ind_t* AssocInd = (mlme_associate_ind_t* )buf;

	if(lrwpan_db.dev == COORDINATOR)
	{
		/*The command shall be ignored if association is not permitted*/
		if(!PERMIT_ASSOCIATION)
		{
			return;
		}
			
		if(lrwpan_state.state == LRWPAN_UP) 
		{
			uint16_t shortAddress;
			shortAddress = 0x1200;	
			/*Send Association Response*/
			lrwpan_SendAssocRsp(AssocInd->DeviceAddress, 
								shortAddress, MAC_SUCCESS, NULL);
		}
	}
}

/******************************************************************************
 * @fn      lrwpan_mcps_data_ind
 *
 * @brief   Data coming from a peer device 
 *
 * @param   src_panid - the PAN Id of the originating device
 *			long_src_addr - the IEEE source address of the peer
 *			long_dst_addr - the IEEE destination address to which the frame was sent
 *			mpduLength - data length in bytes
 *			msdu - data
 *
 * @return  none
 */
 
bool lrwpan_mcps_data_ind(uint8_t* buf)
{
	mcps_data_ind1_t* DataInd = (mcps_data_ind1_t* )buf;
	
#if 0	
	if(lrwpan_state.state != LRWPAN_UP) 
	{
		return FALSE;
	}
#endif

	if(lrwpan_db.panid != DataInd->SrcPANId) 
	{	
		return FALSE;
	}

#ifdef ROUTE_APP
	/*Data Handler of the Routing application*/
	route_handle_rx_from_ll((uint8_t*)(DataInd + 1), DataInd->msduLength, 
							WIRELESS, DataInd->mpduLinkQuality);
#endif	
	return TRUE;
}

/******************************************************************************
 * @fn      lrwpan_SendAssocRsp
 *
 * @brief   Send Association Response primitive 
 *
 * @param   DeviceAddress - the address of the device requesting association
 *			AssocShortAddress - the short device address allocated by the coordinator 
 *								on successful association  
 *			status - status of the opertaion (as defined in return_val.h)
 *			sec_p - security keys
 *			
 * @return  none
 */

void lrwpan_SendAssocRsp(uint64_t DeviceAddress,
                             uint16_t AssocShortAddress,
                             uint8_t status,
                             security_info_t *sec_p)
{
	u8 len;
	u8 buf[MAX_HOST_CMD_LENGTH];	
	mlme_associate_resp_t* mlme_associate_resp = (mlme_associate_resp_t*)buf; 
	
	memset(buf, 0x00, MAX_HOST_CMD_LENGTH);	
	
	mlme_associate_resp->cmdcode = MLME_ASSOCIATE_RESPONSE;
    mlme_associate_resp->DeviceAddress  = DeviceAddress;
    mlme_associate_resp->AssocShortAddress = AssocShortAddress;
    mlme_associate_resp->status = status;
	
    if(sec_p) 
	{
        mlme_associate_resp->Security.SecurityLevel = sec_p->SecurityLevel;
        mlme_associate_resp->Security.KeyIdMode     = sec_p->KeyIdMode;
        mlme_associate_resp->Security.KeyIndex      = sec_p->KeyIndex;
        memcpy(mlme_associate_resp->Security.KeySource, sec_p->KeySource,
               SEC_KEY_SRC_MAX);
		len = sizeof(mlme_associate_resp_t) + SEC_KEY_SRC_MAX;
    } 
	else 
   	{
        mlme_associate_resp->Security.SecurityLevel = 0;
		len = sizeof(mlme_associate_resp_t);
    }	

	GV701x_SendAppEvent(lrwpan_db.app_id, APP_FW_MSG_APPID, APP_MSG_TYPE_FW, IEEE802_15_4_MAC_ID,
						EVENT_CLASS_MGMT, MGMT_FRM_ID, buf, len, 0);	
}

/******************************************************************************
 * @fn      lrwpan_SendScanReq
 *
 * @brief   Send a Scan Request primitive 
 *
 * @param   ScanType - the scan type (as defined in mac_internal.h)
 *			ScanChannels - bitmap of the channels to scan
 *			ScanDuration - scan duration (as define by IEEE 802.15.4 spec)
 *			ChannelPage - not used
 *			sec_p - security keys
 *			
 * @return  none
 */

void lrwpan_SendScanReq(uint8_t ScanType,
                            uint32_t ScanChannels,
                            uint8_t ScanDuration,
                            uint8_t ChannelPage,
                            security_info_t *sec_p)
{
	u8 len;
	u8 buf[MAX_HOST_CMD_LENGTH];
	mlme_scan_req_t* mlme_scan_req = (mlme_scan_req_t*)buf; 

	memset(buf, 0x00, MAX_HOST_CMD_LENGTH);	

    mlme_scan_req->cmdcode = MLME_SCAN_REQUEST;
    mlme_scan_req->ScanType = ScanType;
    mlme_scan_req->ScanChannels = ScanChannels;
    mlme_scan_req->ScanDuration = ScanDuration;
    mlme_scan_req->ChannelPage = ChannelPage;

	if(sec_p) 
	{
		mlme_scan_req->Security.SecurityLevel = sec_p->SecurityLevel;
		mlme_scan_req->Security.KeyIdMode = sec_p->KeyIdMode;
		mlme_scan_req->Security.KeyIndex = sec_p->KeyIndex;
		memcpy(mlme_scan_req->Security.KeySource, sec_p->KeySource,
			   SEC_KEY_SRC_MAX);
		len = sizeof(mlme_scan_req_t) + SEC_KEY_SRC_MAX;		
	} 
	else 
	{
		mlme_scan_req->Security.SecurityLevel = 0; 
		len = sizeof(mlme_scan_req_t);		
	}

	GV701x_SendAppEvent(lrwpan_db.app_id, APP_FW_MSG_APPID, APP_MSG_TYPE_FW, IEEE802_15_4_MAC_ID,
						EVENT_CLASS_MGMT, MGMT_FRM_ID, buf, len, 0);		
}

/******************************************************************************
 * @fn      lrwpan_SendAssocReq
 *
 * @brief   Send a Association Request primitive 
 *
 * @param   LogicalChannel - the channel to be sent on
 *			ChannelPage - not used
 *			CoordAddrSpec_p - Coordinator Address specification (as defined in mac_msgs.h)
 *			CapabilityInformation - Device capability (as define in IEEE 802.15.4 spec)
 *			sec_p - security keys
 *			
 * @return  none
 */

void lrwpan_SendAssocReq(uint8_t LogicalChannel,
                                 uint8_t ChannelPage,
                                 wpan_addr_spec_t *CoordAddrSpec_p,
                                 uint8_t CapabilityInformation,
                                 security_info_t *sec_p)
{
	u8 len;
	u8 buf[MAX_HOST_CMD_LENGTH];
	mlme_associate_req_t *mlme_associate_req = (mlme_associate_req_t*)buf; 

	memset(buf, 0x00, MAX_HOST_CMD_LENGTH);
	
    mlme_associate_req->cmdcode = MLME_ASSOCIATE_REQUEST;
    mlme_associate_req->LogicalChannel = LogicalChannel;
    mlme_associate_req->CoordAddrMode = CoordAddrSpec_p->AddrMode;
    mlme_associate_req->CoordPANId = CoordAddrSpec_p->PANId;
    mlme_associate_req->CoordAddress.long_address.lo_u32 = 
                                      CoordAddrSpec_p->Addr.long_address.lo_u32;
    mlme_associate_req->CoordAddress.long_address.hi_u32 = 
                                      CoordAddrSpec_p->Addr.long_address.hi_u32;

    mlme_associate_req->CapabilityInformation = CapabilityInformation;
    mlme_associate_req->ChannelPage = ChannelPage;

    if(sec_p) 
	{
        mlme_associate_req->Security.SecurityLevel = sec_p->SecurityLevel;
        mlme_associate_req->Security.KeyIdMode = sec_p->KeyIdMode;
        mlme_associate_req->Security.KeyIndex = sec_p->KeyIndex;
        memcpy(mlme_associate_req->Security.KeySource, sec_p->KeySource,
               SEC_KEY_SRC_MAX);
		len = sizeof(mlme_associate_req_t) + SEC_KEY_SRC_MAX;
    } 
	else 
    {
        mlme_associate_req->Security.SecurityLevel = 0;
		len = sizeof(mlme_associate_req_t);		
    }	

	GV701x_SendAppEvent(lrwpan_db.app_id, APP_FW_MSG_APPID, APP_MSG_TYPE_FW, IEEE802_15_4_MAC_ID,
						EVENT_CLASS_MGMT, MGMT_FRM_ID, buf, len, 0);			
}

/******************************************************************************
 * @fn      lrwpan_SendAttributeSetReq
 *
 * @brief   Sends a MLME Set request 
 *
 * @param   attribute - attribute number(found in mac_const.h)
 *			
 * @return  none
 */

void lrwpan_SendAttributeSetReq(uint8_t attribute, uint8_t* value)
{
	uint8_t len;
	uint8_t buf[MAX_HOST_CMD_LENGTH];
	mlme_set_req_t* mlme_set_req = (mlme_set_req_t *)buf; 

	attribute = attribute;
	value = value;
	memset(buf, 0x00, MAX_HOST_CMD_LENGTH);
	
    mlme_set_req->cmdcode = MLME_SET_REQUEST;
    mlme_set_req->PIBAttribute = phyCurrentChannel;
	mlme_set_req->PIBAttributeValue.pib_value_8bit = lrwpan_db.channel;
	//memcpy((uint8_t*)&mlme_set_req[sizeof(lrwpan_cfg_evnt_msg_t) - sizeof(pib_value_t)], (uint8_t*)value, sizeof(pib_value_t));
	len = sizeof(mlme_set_req_t);		

	GV701x_SendAppEvent(lrwpan_db.app_id, APP_FW_MSG_APPID, APP_MSG_TYPE_FW, IEEE802_15_4_MAC_ID,
						EVENT_CLASS_MGMT, MGMT_FRM_ID, buf, len, 0);			
}

/******************************************************************************
 * @fn      lrwpan_SendResetReq
 *
 * @brief   Sends a MLME Reset request 
 *
 * @param   SetDefaultPIB - if TRUE it resets all PIB values to default
 *			
 * @return  none
 */

void lrwpan_SendResetReq(uint8_t SetDefaultPIB)
{
	u8 len;
	u8 buf[MAX_HOST_CMD_LENGTH];
	mlme_reset_req_t* mlme_reset_req = (mlme_reset_req_t*)buf; 

	memset(buf, 0x00, MAX_HOST_CMD_LENGTH);
	
    mlme_reset_req->cmdcode = MLME_RESET_REQUEST;
    mlme_reset_req->SetDefaultPIB = SetDefaultPIB;
	len = sizeof(mlme_reset_req_t );		

	GV701x_SendAppEvent(lrwpan_db.app_id, APP_FW_MSG_APPID, APP_MSG_TYPE_FW, IEEE802_15_4_MAC_ID,
						EVENT_CLASS_MGMT, MGMT_FRM_ID, buf, len, 0);			
}

/******************************************************************************
 * @fn      lrwpan_SendStartReq
 *
 * @brief   Send a Start Request primitive 
 *
 * @param   PANId - the PAN Id the netowrk is to be formed
 *			LogicalChannel - the channel on which the network is to be formed
 *			ChannelPage - not used
 *			StartTime - start time (as defined in IEEE 802.15.4 spec)
 *			BeaconOrder - Beacon Order(as define in IEEE 802.15.4 spec)
 *			SuperframeOrder - Superframe Order(as define in IEEE 802.15.4 spec)
 *			PANCoordinator - PAN specification
 *			BatteryLifeExtension - (as define in IEEE 802.15.4 spec)
 *			CoordRealignment - (as define in IEEE 802.15.4 spec)
 *			CoordRealignmentSecurity_p - security keys (as define in IEEE 802.15.4 spec)
 *			BeaconSecurity_p - security keys (as define in IEEE 802.15.4 spec)
 *			
 * @return  none
 */

void lrwpan_SendStartReq(uint16_t PANId,
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
	u8 len;
	u8 buf[MAX_HOST_CMD_LENGTH];
	u8 seclength = 0;
	mlme_start_req_t *mlme_start_req = (mlme_start_req_t*)buf; 

	memset(buf, 0x00, MAX_HOST_CMD_LENGTH); 

    mlme_start_req->cmdcode = MLME_START_REQUEST;
    mlme_start_req->PANId = PANId;
    mlme_start_req->LogicalChannel = LogicalChannel;
    mlme_start_req->ChannelPage = ChannelPage;
    mlme_start_req->StartTime = StartTime;                                      
    mlme_start_req->BeaconOrder = BeaconOrder;
    mlme_start_req->SuperframeOrder = SuperframeOrder;
    mlme_start_req->PANCoordinator = PANCoordinator;
    mlme_start_req->BatteryLifeExtension = BatteryLifeExtension;
    mlme_start_req->CoordRealignment = CoordRealignment;                                          
	
    if(CoordRealignmentSecurity_p) 
	{
        mlme_start_req->CoordRealignmentSecurity.SecurityLevel = CoordRealignmentSecurity_p->SecurityLevel;
        mlme_start_req->CoordRealignmentSecurity.KeyIdMode = CoordRealignmentSecurity_p->KeyIdMode;
        mlme_start_req->CoordRealignmentSecurity.KeyIndex = CoordRealignmentSecurity_p->KeyIndex;
        memcpy(mlme_start_req->CoordRealignmentSecurity.KeySource, CoordRealignmentSecurity_p->KeySource,
               SEC_KEY_SRC_MAX);
		seclength = SEC_KEY_SRC_MAX;
    } 
	else 
    {
        mlme_start_req->CoordRealignmentSecurity.SecurityLevel = 0;
		seclength = 0;
    }	

    if(BeaconSecurity_p) 
	{
        mlme_start_req->BeaconSecurity.SecurityLevel = BeaconSecurity_p->SecurityLevel;
        mlme_start_req->BeaconSecurity.KeyIdMode = BeaconSecurity_p->KeyIdMode;
        mlme_start_req->BeaconSecurity.KeyIndex = BeaconSecurity_p->KeyIndex;
        memcpy(mlme_start_req->BeaconSecurity.KeySource, BeaconSecurity_p->KeySource,
               SEC_KEY_SRC_MAX);
		seclength = SEC_KEY_SRC_MAX;		
    } 
	else 
    {
        mlme_start_req->BeaconSecurity.SecurityLevel = 0;
		seclength = 0;
    }	

	len = sizeof(mlme_start_req_t) + seclength;

	GV701x_SendAppEvent(lrwpan_db.app_id, APP_FW_MSG_APPID, APP_MSG_TYPE_FW, IEEE802_15_4_MAC_ID,
						EVENT_CLASS_MGMT, MGMT_FRM_ID, buf, len, 0);			
}

/******************************************************************************
 * @fn      lrwpan_SendData
 *
 * @brief   Send a Data Request primitive 
 *
 * @param   addr - the short address of the destination
 *			msdu - data packet
 *			msduLength - data packet length
 *			
 * @return  none
 */

void lrwpan_SendData(uint16_t addr, uint8_t* msdu, u8 msduLength) 
{	
	u8 buf[MAX_HOST_CMD_LENGTH];
	uint16_t addr1;
	mcps_data_req_t *mcps_data_req = (mcps_data_req_t *)buf; 

	memset(buf, 0x00, MAX_HOST_CMD_LENGTH);	
	addr1 = addr;
	mcps_data_req->cmdcode = MCPS_DATA_REQUEST;
	mcps_data_req->SrcAddrMode = WPAN_ADDRMODE_SHORT;
	mcps_data_req->DstAddrMode = WPAN_ADDRMODE_SHORT;
	
	if(addr1 == 0xFFFF)
		mcps_data_req->DstPANId = 0xFFFF;
	else
		mcps_data_req->DstPANId = lrwpan_db.panid;
	
	if(lrwpan_db.dev == COORDINATOR)
	{
		memcpy((uint8_t*)&mcps_data_req->DstAddr, (uint8_t*)&addr1, sizeof(uint16_t));		
	}
	else
	{
		if(addr1 == 0xFFFF)
			memset((uint8_t*)&mcps_data_req->DstAddr, 0xFF, sizeof(uint64_t));
		else
			memcpy((uint8_t*)&mcps_data_req->DstAddr, (uint8_t*)&addr1, sizeof(uint16_t));			
	}
	
	if(addr1 == 0xFFFF)
		mcps_data_req->TxOptions = TX_CAP;
	else
		mcps_data_req->TxOptions = TX_CAP_ACK;	
	
	mcps_data_req->Security.SecurityLevel = 0;
	mcps_data_req->msduHandle = 0x01;
	mcps_data_req->msdu_p = (uint8_t*)(mcps_data_req + 1);
	memcpy((uint8_t*)mcps_data_req->msdu_p, (uint8_t*)msdu, msduLength);
	mcps_data_req->msduLength = msduLength;

	GV701x_SendAppEvent(lrwpan_db.app_id, APP_FW_MSG_APPID, APP_MSG_TYPE_FW, IEEE802_15_4_MAC_ID,
						EVENT_CLASS_MGMT, MGMT_FRM_ID, buf, sizeof(mcps_data_req_t) + msduLength, 0);		
}

/******************************************************************************
 * @fn      GV701x_LrwpanTimerHandler
 *
 * @brief   Timer handler for LRWPAN driver timer events
 *
 * @param   event - event from firmware
 *
 * @return  none
 *
 */

void GV701x_LrwpanTimerHandler(uint8_t* buf)
{	
	hostTimerEvnt_t* timerevt = (hostTimerEvnt_t*)buf; 					

	if(buf == NULL)
		return;

	/*Demultiplexing the specific timer event*/ 					
	switch((u8)timerevt->type)
	{											
		case LRWPAN_PROFILE_TIMEOUT_EVT:	
			GV701x_LrwpanDriverStart();
		break;

		case LRWPAN_START_TIMEOUT_EVT:	
			lrwpan_device_start();
		break;
		
		default:
		break;
	}							
}

/******************************************************************************
 * @fn      GV701x_LrwpanSendEvent
 *
 * @brief   Sends an indication(on state change) to the 
 *          app that requested the service
 *
 * @param   ind - indication type
 *          status - status of the operation requested
 *                   (STATUS_SUCCESS/STATUS_FAILURE only valid for Scan service)
 *          reason - reason for the transition 
 *                  (hostEventNetworkIndReason_e in hpgp_msgs.h)
 *
 * @return  none
 *
 */

void GV701x_LrwpanSendEvent(u8 ind, u8 status, u8 value) 
{ 	
	if(ind == LRWPAN_UP_IND)
	{
		lrwpan_up_ind_t lrwpan_up;
		lrwpan_up.event = ind;
		GV701x_SendAppEvent(lrwpan_db.app_id, lrwpan_db.start.app_id, APP_MSG_TYPE_APPIND, APP_MAC_ID,
							EVENT_CLASS_CTRL, MGMT_FRM_ID, &lrwpan_up, sizeof(lrwpan_up_ind_t), 0);		
		lrwpan_state.state = LRWPAN_UP;
		lrwpan_db.start.active = FALSE;	
		lrwpan_db.start.app_id = 0;	
		STM_StopTimer(lrwpan_db.profile_timer);
		STM_StopTimer(lrwpan_db.start.timer);		
	}
	else if(ind == LRWPAN_DWN_IND)
	{
		lrwpan_dwn_ind_t lrwpan_dwn;
		lrwpan_dwn.event = ind;

		GV701x_SendAppEvent(lrwpan_db.app_id, lrwpan_db.start.app_id, APP_MSG_TYPE_APPIND, APP_MAC_ID,
							EVENT_CLASS_CTRL, MGMT_FRM_ID, &lrwpan_dwn, sizeof(lrwpan_dwn_ind_t), 0);		
		lrwpan_state.state = LRWPAN_DOWN;
		lrwpan_db.start.active = FALSE;	
		lrwpan_db.start.app_id = 0;			
	}
	else if(ind == LRWPAN_SCAN_IND)
	{
		lrwpan_scan_ind_t lrwpan_scanind;
		lrwpan_scanind.event = ind;
		lrwpan_scanind.status = status;
		GV701x_SendAppEvent(lrwpan_db.app_id, lrwpan_db.scan.app_id, APP_MSG_TYPE_APPIND, APP_MAC_ID,
							EVENT_CLASS_CTRL, MGMT_FRM_ID, &lrwpan_scanind, 
							sizeof(lrwpan_scan_ind_t), APP_EVNT_TX_CRITICAL_OPT);		
		
		//if(lrwpan_db.scan.result.UnscannedChannels == 0)
		{ 
			lrwpan_state.state = lrwpan_db.scan.prev_state;		
			lrwpan_db.scan.prev_state = LRWPAN_IDLE;
			lrwpan_db.scan.active = FALSE;	
			lrwpan_db.scan.app_id = 0;		
		}
	}
	else if(ind == LRWPAN_BCN_IND)
	{
		lrwpan_bcn_ind_t lrwpan_bcnind;
		lrwpan_bcnind.event = ind;
		GV701x_SendAppEvent(lrwpan_db.app_id, lrwpan_db.scan.app_id, APP_MSG_TYPE_APPIND, APP_MAC_ID,
							EVENT_CLASS_CTRL, MGMT_FRM_ID, &lrwpan_bcnind, 
							sizeof(lrwpan_bcn_ind_t), 0);		
	}	
	else if(ind == LRWPAN_CFG_IND)
	{
		lrwpan_cfg_ind_t lrwpan_cfg_ind;
		lrwpan_cfg_ind.event = ind;
		lrwpan_cfg_ind.status = status;
		lrwpan_cfg_ind.attribute = value;
		lrwpan_db.cfg.active = FALSE;			
		lrwpan_db.cfg.params = 0xFF;
		GV701x_SendAppEvent(lrwpan_db.app_id, lrwpan_db.cfg.app_id, APP_MSG_TYPE_APPIND, APP_MAC_ID,
							EVENT_CLASS_CTRL, MGMT_FRM_ID, &lrwpan_cfg_ind, 
							sizeof(lrwpan_cfg_ind_t), 0);		
		lrwpan_db.cfg.app_id = 0;						
	}	
	else
		return;
}

/******************************************************************************
 * @fn      GV701x_LrwpanScanInd
 *
 * @brief   Services the result of a scan 
 *
 * @param   ind - scan result(mlme_scan_conf_t found in mac_msgs.h) 
 *
 * @return  none
 *
 */

void GV701x_LrwpanScanInd(uint8_t* ind) 
{ 		
	uint8_t i, j = 0xFF;
	mlme_scan_conf_t* cnf = NULL;

	if(ind == NULL)
		GV701x_LrwpanSendEvent(LRWPAN_SCAN_IND, STATUS_FAILURE, 0);	

	cnf = (mlme_scan_conf_t*)ind;		

#if 0
	FM_Printf(FM_APP, "\nScan CNf s %bx ch %bx Uc %lx rs %bu", cnf->status, 
		cnf->ChannelPage, cnf->UnscannedChannels, cnf->ResultListSize);
#endif

	if((cnf->status != MAC_SUCCESS) &&
		(cnf->status != MAC_NO_BEACON))
	{
		GV701x_LrwpanSendEvent(LRWPAN_SCAN_IND, STATUS_FAILURE, 0);
	}
	
	/*Only Channel Page 0 is supported*/		
	if(cnf->ChannelPage != 0x00)
		GV701x_LrwpanSendEvent(LRWPAN_SCAN_IND, STATUS_FAILURE, 0);	
	
	lrwpan_db.scan.result.status = cnf->status;
	lrwpan_db.scan.result.ScanType = cnf->ScanType;	
	lrwpan_db.scan.result.ChannelPage = cnf->ChannelPage;		
	lrwpan_db.scan.result.UnscannedChannels = cnf->UnscannedChannels;	
	if((cnf->status == MAC_LIMIT_REACHED) && (cnf->ResultListSize > LRWPAN_MAX_SCAN_LIST))
		lrwpan_db.scan.result.ResultListSize = LRWPAN_MAX_SCAN_LIST;		
	else 
		lrwpan_db.scan.result.ResultListSize = cnf->ResultListSize;	

	for(i = 0; i < (cnf->ResultListSize < LRWPAN_MAX_SCAN_LIST ? 
		cnf->ResultListSize : LRWPAN_MAX_SCAN_LIST); i++)
	{	
		if(cnf->ScanType == MLME_SCAN_TYPE_ACTIVE)
		{		
			if( (lrwpan_db.scan.result.list[i].active == TRUE) &&
				(memcmp(&lrwpan_db.scan.result.list[i].val.PANDescriptor.CoordAddrSpec,
						&cnf->scan_result_list[i].PANDescriptor.CoordAddrSpec, sizeof(wpan_addr_spec_t)) == 0)  &&
				(lrwpan_db.scan.result.list[i].val.PANDescriptor.LogicalChannel == 
					cnf->scan_result_list[i].PANDescriptor.LogicalChannel))									
			{		
#if 0			
				memcpy(lrwpan_db.scan.result.list[i].bcn_payload, bcn_ind.sdu, 
						(bcn_ind.sduLength > MAX_BCN_PAYLOAD) ? MAX_BCN_PAYLOAD : bcn_ind.sduLength);			
				break;
#endif
			}
			else
			{
				if(lrwpan_db.scan.result.list[i].active == FALSE)
				{
					j = i;
				}
			}
		}
		else if(cnf->ScanType == MLME_SCAN_TYPE_ED)
		{	
			memcpy((uint8_t*)&lrwpan_db.scan.result.list[0].val.ed_value[i], 
				(uint8_t*)&cnf->scan_result_list[0].ed_value[i], 
				sizeof(cnf->scan_result_list[0].ed_value[0]));
#if 0
			FM_Printf(FM_APP, "\nNo. %bu ED %bx" , i, 
				lrwpan_db.scan.result.list[0].val.ed_value[i]);
#endif
		}			
	}

	if(cnf->ScanType == MLME_SCAN_TYPE_ACTIVE)
	{
		if(i == (cnf->ResultListSize < LRWPAN_MAX_SCAN_LIST ? 
			cnf->ResultListSize : LRWPAN_MAX_SCAN_LIST))
		{
			if(j != 0xFF)
			{
				lrwpan_db.scan.result.list[j].active = TRUE;
				memcpy(&lrwpan_db.scan.result.list[j].val.PANDescriptor,
						&cnf->scan_result_list[i].PANDescriptor, sizeof(pandescriptor_t));
#if 0				
				memcpy(lrwpan_db.scan.result.list[j].bcn_payload, bcn_ind.sdu, 
						(bcn_ind.sduLength > MAX_BCN_PAYLOAD) ? MAX_BCN_PAYLOAD : bcn_ind.sduLength);			
#endif
			}
		}
	}
	
	GV701x_LrwpanSendEvent(LRWPAN_SCAN_IND, STATUS_SUCCESS, 0);	
}

/******************************************************************************
 * @fn      GV701x_LrwpanBcnUpdate
 *
 * @brief   Populates the beacon payload in the scaned peer list
 *
 * @param   bcn_ind - beacon notification 
 *                    (mlme_beacon_notify_ind_t found in mac_msgs.h) 
 *
 * @return  none
 *
 */

void GV701x_LrwpanBcnUpdate(mlme_beacon_notify_ind_t* bcn_ind) 
{ 	
	uint8_t i, j = 0xFF;
	uint8_t bcn_update = FALSE;	

	if(lrwpan_db.scan.active == TRUE)
	{
		if(bcn_ind->PANDescriptor.CoordAddrSpec.PANId != LRWPAN_PANID)
			return;

#if 0		
		FM_Printf(FM_APP, "\nBcn: Ch %bx Pan %x AM %bu A %x Lqi %bu", 
			bcn_ind->PANDescriptor.LogicalChannel,
			bcn_ind->PANDescriptor.CoordAddrSpec.PANId,
			bcn_ind->PANDescriptor.CoordAddrSpec.AddrMode,
			bcn_ind->PANDescriptor.CoordAddrSpec.Addr.short_address,
			bcn_ind->PANDescriptor.LinkQuality);	
#endif		
		for(i = 0; i < LRWPAN_MAX_SCAN_LIST; i++)
		{
			if( (lrwpan_db.scan.result.list[i].active == TRUE) &&
				(memcmp(&lrwpan_db.scan.result.list[i].val.PANDescriptor.CoordAddrSpec,
						&bcn_ind->PANDescriptor.CoordAddrSpec, sizeof(wpan_addr_spec_t)) == 0)  &&
				(lrwpan_db.scan.result.list[i].val.PANDescriptor.LogicalChannel == 
					bcn_ind->PANDescriptor.LogicalChannel))									
			{		
				if(memcmp(&lrwpan_db.scan.result.list[i].bcn_payload,
						bcn_ind->sdu, (bcn_ind->sduLength > MAX_BCN_PAYLOAD) ? MAX_BCN_PAYLOAD : bcn_ind->sduLength) != 0)
				{
					memcpy(lrwpan_db.scan.result.list[i].bcn_payload, bcn_ind->sdu, 
							(bcn_ind->sduLength > MAX_BCN_PAYLOAD) ? MAX_BCN_PAYLOAD : bcn_ind->sduLength);			
					bcn_update = TRUE;
				}
				break;			
			}
			else
			{
				if((lrwpan_db.scan.result.list[i].active == FALSE) && (j == 0xFF))
				{
					j = i;
				}
			}			
		}

		if(i == LRWPAN_MAX_SCAN_LIST)
		{
			if(j != 0xFF)
			{
				lrwpan_db.scan.result.list[j].active = TRUE;
				memcpy(&lrwpan_db.scan.result.list[j].val.PANDescriptor,
						&bcn_ind->PANDescriptor, sizeof(pandescriptor_t));
				memcpy(lrwpan_db.scan.result.list[j].bcn_payload, bcn_ind->sdu, 
						(bcn_ind->sduLength > MAX_BCN_PAYLOAD) ? MAX_BCN_PAYLOAD : bcn_ind->sduLength);
#if 0
				FM_HexDump(FM_APP,"\nBcnPay:", lrwpan_db.scan.result.list[j].bcn_payload,
							MAX_BCN_PAYLOAD);
#endif
				
				if(lrwpan_db.scan.active == FALSE)				
					bcn_update = TRUE;
			}
		}

		if(bcn_update == TRUE)
		{
			GV701x_LrwpanSendEvent(LRWPAN_BCN_IND, STATUS_SUCCESS, 0);			
		}
	}
}

/******************************************************************************
 * @fn      GV701x_LrwpanDriverFlush
 *
 * @brief   Flushes all the driver variables and MAC configuration
 *
 * @param   none
 *
 * @return  none
 *
 */
void GV701x_LrwpanDriverFlush(void)
{
	lrwpan_db.dev = COORDINATOR; 
	lrwpan_db.scan.time = 5;	
#if 0	
	lrwpan_db.scan.ch_mask = LRWPAN_CHANNEL_MASK;
#endif
	lrwpan_db.scan.type = MLME_SCAN_TYPE_ACTIVE;		
	lrwpan_db.panid = LRWPAN_PANID; 
	lrwpan_db.channel = LRWPAN_CHANNEL;
	lrwpan_state.state = LRWPAN_IDLE;
	lrwpan_db.cfg.active = FALSE;
	lrwpan_db.cfg.params = 0xFF;
	lrwpan_SendResetReq(FALSE);
}

/******************************************************************************
 * @fn      GV701x_LrwpanDriverSM
 *
 * @brief   LRWPAN State Machine, it executes all internal/external 
 *			events triggered
 *
 * @param   state - state machine object of the driver
 *          (passed as a reference incase there are more than one object)
 *
 * @return  none
 *
 */

void GV701x_LrwpanDriverSM(gv701x_state_t* state)
{
	if(state == NULL)
		return;

#if 1
	if(state->event != LRWPAN_IDLE_EVNT)
		FM_Printf(FM_APP, "\nLrwpan S %bu E %bx P %bu C %bu E %bu Da %bu Sa %bu T %bu a %bu p %bu", 
				state->state, state->event,
				state->eventproto, state->eventclass, state->eventtype, 
				state->msg_hdr.dst_app_id, state->msg_hdr.src_app_id, state->msg_hdr.type,
				lrwpan_db.cfg.active, lrwpan_db.cfg.params);				
#endif
	switch(state->state) 
	{
		case LRWPAN_IDLE:		
			if(state->eventproto == APP_MAC_ID)
			{	
				if(state->msg_hdr.type == APP_MSG_TYPE_APPEVENT)
				{
					switch(state->event) 
					{				
						/*Start event*/
						case LRWPAN_START_EVNT:
							if((lrwpan_db.start.active == FALSE) &&
							   (lrwpan_db.scan.active == FALSE))
							{
								lrwpan_db.start.active = TRUE;	
								lrwpan_db.start.app_id = state->msg_hdr.src_app_id; 
								lrwpan_device_config();
								GV701x_LrwpanDriverStart();
							}
						break;
						
						/*Scan event*/
						case LRWPAN_SCAN_EVNT:	
							if((lrwpan_db.scan.active == FALSE) &&			
							   (lrwpan_db.start.active == FALSE))
							{
								lrwpan_db.scan.active = TRUE;	
								lrwpan_db.scan.prev_state = state->state;
								state->state = LRWPAN_SCAN;
								lrwpan_db.scan.app_id = state->msg_hdr.src_app_id;
								lrwpan_SendScanReq(lrwpan_db.scan.type, lrwpan_db.scan.ch_mask, 
										lrwpan_db.scan.time, 0, NULL);							
							}
						break;					

						/*Configuration event*/
						case LRWPAN_CFG_EVNT:	
							if((lrwpan_db.scan.active == FALSE) &&			
							   (lrwpan_db.start.active == FALSE) &&
							   (lrwpan_db.cfg.active == FALSE))
							{
								lrwpan_cfg_evnt_msg_t* cfg_msg = (lrwpan_cfg_evnt_msg_t*)state->state;
								lrwpan_db.cfg.active = TRUE;	
								lrwpan_db.cfg.app_id = state->msg_hdr.src_app_id;
								lrwpan_db.cfg.params =  cfg_msg->attribute;
								lrwpan_SendAttributeSetReq(cfg_msg->attribute, 
									(u8*)&cfg_msg[sizeof(lrwpan_cfg_evnt_msg_t) - sizeof(pib_value_t)]);
							}
						break;	
						
						default:
						break;					
					}	
				}
			}	
			else if(state->eventproto == IEEE802_15_4_MAC_ID)
			{
				switch(state->event) 
				{			
					case MLME_RESET_CONFIRM:
						if(state->statedata != NULL)
						{								
							mlme_reset_conf_t* reset_cnf = (mlme_reset_conf_t*)state->statedata;
							
							if(reset_cnf->status != STATUS_SUCCESS)
							{	
								lrwpan_SendResetReq(FALSE);
							}
							else
							{
								GV701x_LrwpanSendEvent(LRWPAN_DWN_IND, reset_cnf->status, 0);
							}
						}
					break;

					case MLME_SET_CONFIRM:
						if(state->statedata != NULL)
						{								
							mlme_set_conf_t* set_cnf = (mlme_set_conf_t*)state->statedata;
							if((lrwpan_db.cfg.active == TRUE) && 
								(lrwpan_db.cfg.params == set_cnf->PIBAttribute))
							{
								GV701x_LrwpanSendEvent(LRWPAN_CFG_IND, (set_cnf->status == MAC_SUCCESS ?
									STATUS_SUCCESS : STATUS_FAILURE), set_cnf->PIBAttribute);
							}
						}
					break;	

					case MCPS_DATA_INDICATION:		
						if(state->statedata != NULL)
						{
							lrwpan_mcps_data_ind(state->statedata);						
						}
					break;						

					default:
					break;
				}
			}
		break;

		case LRWPAN_SCAN:
		case LRWPAN_START:
		case LRWPAN_DOWN:	
			if(state->eventproto == APP_MAC_ID)
			{			
				if(state->msg_hdr.type == APP_MSG_TYPE_APPEVENT)
				{			
					switch(state->event) 
					{
						/*Start event*/
						case LRWPAN_START_EVNT:							
							if((lrwpan_db.start.active == FALSE) &&
							   (lrwpan_db.scan.active == FALSE))
							{
								lrwpan_db.start.active = TRUE;	
								lrwpan_db.start.app_id = state->msg_hdr.src_app_id;
								lrwpan_device_config();
								GV701x_LrwpanDriverStart();
							}
						break;									

						/*Scan event*/
						case LRWPAN_SCAN_EVNT:	
							if((lrwpan_db.scan.active == FALSE) &&			
							   (lrwpan_db.start.active == FALSE))					
							{
								lrwpan_db.scan.active = TRUE;	
								lrwpan_db.scan.prev_state = state->state;
								state->state = LRWPAN_SCAN;
								lrwpan_db.scan.app_id = state->msg_hdr.src_app_id;
								lrwpan_SendScanReq(lrwpan_db.scan.type, lrwpan_db.scan.ch_mask, 
										lrwpan_db.scan.time, 0, NULL);							
							}
						break;	

						/*Configuration event*/
						case LRWPAN_CFG_EVNT:	
							if((lrwpan_db.scan.active == FALSE) &&			
							   (lrwpan_db.start.active == FALSE) &&
							   (lrwpan_db.cfg.active == FALSE))
							{
								lrwpan_cfg_evnt_msg_t* cfg_msg = (lrwpan_cfg_evnt_msg_t*)state->state;
								lrwpan_db.cfg.active = TRUE;	
								lrwpan_db.cfg.app_id = state->msg_hdr.src_app_id;
								lrwpan_db.cfg.params =  cfg_msg->attribute;
								lrwpan_SendAttributeSetReq(cfg_msg->attribute,
									(u8*)&cfg_msg[sizeof(lrwpan_cfg_evnt_msg_t) - sizeof(pib_value_t)]);
							}
						break;	

						case LRWPAN_STOP_EVNT:
							lrwpan_db.start.active = FALSE;	
							lrwpan_db.start.app_id = state->msg_hdr.src_app_id;						
							GV701x_LrwpanDriverFlush();
						break;
						
						default:
						break;						
					}
				}
			}
			else if(state->eventproto == IEEE802_15_4_MAC_ID)
			{
				switch(state->event) 
				{			
					/*Start complete event*/
					case MLME_BEACON_NOTIFY_INDICATION:	
						if(state->statedata != NULL)						
						{
#if 0						
							GV701x_LrwpanBcnUpdate((mlme_beacon_notify_ind_t*)state->statedata);
#endif
						}
					break;	
					
					/*Start complete event*/
					case MLME_SCAN_CONFIRM:							
						GV701x_LrwpanScanInd(state->statedata);						
					break;	
					
					case MLME_START_CONFIRM:
						if(state->statedata != NULL)
						{					
							mlme_start_conf_t* cnf = (mlme_start_conf_t*)state->statedata;
							if(cnf->status == MAC_SUCCESS)								
								GV701x_LrwpanSendEvent(LRWPAN_UP_IND, STATUS_SUCCESS, 0);
							else
								GV701x_LrwpanSendEvent(LRWPAN_DWN_IND, STATUS_SUCCESS, 0);
						}					
					break;

					case MLME_SET_CONFIRM:
						if(state->statedata != NULL)
						{								
							mlme_set_conf_t* set_cnf = (mlme_set_conf_t*)state->statedata;
							if((lrwpan_db.cfg.active == TRUE) && 
								(lrwpan_db.cfg.params == set_cnf->PIBAttribute))
							{
								GV701x_LrwpanSendEvent(LRWPAN_CFG_IND, (set_cnf->status == MAC_SUCCESS ?
									STATUS_SUCCESS : STATUS_FAILURE), set_cnf->PIBAttribute);
							}
						}
					break;	

					case MCPS_DATA_INDICATION:		
						if(state->statedata != NULL)
						{
							lrwpan_mcps_data_ind(state->statedata);						
						}
					break;
					
					default:
					break;
				}
			}				
		break;
									
		case LRWPAN_UP:
			if(state->eventproto == APP_MAC_ID)
			{	
				if(state->msg_hdr.type == APP_MSG_TYPE_APPEVENT)
				{			
					switch(state->event) 
					{
						/*Start event*/
						case LRWPAN_START_EVNT:						
							if((lrwpan_db.start.active == FALSE) &&
							   (lrwpan_db.scan.active == FALSE))
							{
								lrwpan_db.start.active = TRUE;	
								lrwpan_db.start.app_id = state->msg_hdr.src_app_id;
								lrwpan_device_config();
								GV701x_LrwpanDriverStart();
							}
						break;

						/*Scan event*/
						case LRWPAN_SCAN_EVNT:			
							if((lrwpan_db.scan.active == FALSE) &&			
							   (lrwpan_db.start.active == FALSE))					
							{						
								lrwpan_db.scan.active = TRUE;
								lrwpan_db.scan.prev_state = state->state;
								state->state = LRWPAN_SCAN;
								lrwpan_db.scan.app_id = state->msg_hdr.src_app_id;
								lrwpan_SendScanReq(lrwpan_db.scan.type, lrwpan_db.scan.ch_mask, 
										lrwpan_db.scan.time, 0, NULL);							
							}
						break;	

						/*Configuration event*/
						case LRWPAN_CFG_EVNT:	
							if((lrwpan_db.scan.active == FALSE) &&			
							   (lrwpan_db.start.active == FALSE) &&
							   (lrwpan_db.cfg.active == FALSE))
							{
								lrwpan_cfg_evnt_msg_t* cfg_msg = (lrwpan_cfg_evnt_msg_t*)state->state;
								lrwpan_db.cfg.active = TRUE;	
								lrwpan_db.cfg.app_id = state->msg_hdr.src_app_id;
								lrwpan_db.cfg.params =  cfg_msg->attribute;
								lrwpan_SendAttributeSetReq(cfg_msg->attribute, 
									(u8*)&cfg_msg[sizeof(lrwpan_cfg_evnt_msg_t) - sizeof(pib_value_t)]);
							}
						break;	

						case LRWPAN_STOP_EVNT:
							lrwpan_db.start.active = FALSE;	
							lrwpan_db.start.app_id = state->msg_hdr.src_app_id;	
							GV701x_LrwpanDriverFlush();
						break;
						
						default:
						break;
					}
				}
			}
			else if(state->eventproto == IEEE802_15_4_MAC_ID)
			{
				switch(state->event)
				{			
					case MCPS_DATA_CONFIRM:
						if(state->statedata != NULL)						
						{
							mcps_data_conf_t* data_cnf = (mcps_data_conf_t*)(state->statedata);
						}
					break;
					
					case MCPS_DATA_INDICATION:		
						if(state->statedata != NULL)
						{
							lrwpan_mcps_data_ind(state->statedata);						
						}
					break;	

					case MLME_SET_CONFIRM:
						if(state->statedata != NULL)
						{								
							mlme_set_conf_t* set_cnf = (mlme_set_conf_t*)state->statedata;							
							if((lrwpan_db.cfg.active == TRUE) && 
								(lrwpan_db.cfg.params == set_cnf->PIBAttribute))
							{
								GV701x_LrwpanSendEvent(LRWPAN_CFG_IND, (set_cnf->status == MAC_SUCCESS ?
									STATUS_SUCCESS : STATUS_FAILURE), set_cnf->PIBAttribute);
							}
						}
					break;	
					
					default:
					break;
				}
			}
		break;
		
		default:
		break;
	}


	state->event = LRWPAN_IDLE_EVNT;	
	state->eventtype = 0;
	state->eventclass = 0;
	state->eventproto = 0;
	state->statedata = NULL;	
	state->statedatalen = 0;	
	memset((u8*)&state->msg_hdr, 0x00, sizeof(gv701x_app_msg_hdr_t));	
}

/******************************************************************************
 * @fn      GV701x_LrwpanCmdProcess
 *
 * @brief   It handles application command line requests
 *
 * @param   CmdBuf - command string
 *
 * @return  none
 *
 */

void GV701x_LrwpanCmdProcess(char* CmdBuf) 
{
#if 1
	if(strcmp(CmdBuf, "state") == 0) 
	{
		printf("\nLrwpan S %bu E %bu", lrwpan_state.state, lrwpan_state.event);				
	}
	else if(strcmp(CmdBuf, "stats") == 0) 
	{
		printf("\nLrwpan pan %x ch %bx chmask %lx", lrwpan_db.panid, lrwpan_db.channel, 
			    lrwpan_db.scan.ch_mask);		
	}	
	else if(strcmp(CmdBuf, "nvclear") == 0) 
	{
		GV701x_FlashErase(lrwpan_db.app_id);
	}
#else
	CmdBuf = CmdBuf;
#endif	
}	
#endif /*LRWPAN_DRIVER_APP*/
