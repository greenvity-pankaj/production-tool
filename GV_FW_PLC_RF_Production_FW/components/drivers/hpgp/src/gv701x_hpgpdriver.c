
/* ========================================================
 *
 * @file: gv701x_hpgpdriver.c
 * 
 * @brief: This file supports all routines required by the application
 *         to start and commence the PLC network
 *      
 *  Copyright (C) 2010-2015, Greenvity Communications, Inc.
 *  All Rights Reserved
 *  
 * =========================================================*/

#ifdef HPGP_DRIVER_APP
/****************************************************************************** 
  *	Includes
  ******************************************************************************/
  
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "gv701x_includes.h"
#include "gv701x_osal.h"
#include "gv701x_hpgpdriver.h"

/****************************************************************************** 
  *	Global Data
  ******************************************************************************/
  
/*Network Database*/
hpgp_nwk_data_t hpgp_nwk_data;

/*Driver Data*/
hpgp_drv_data_t hpgp_drv_data;

/*Network keys*/
u8 cco_nid[NID_LEN] = {0xc9, 0x21, 0x20, 0xe8, 0x43, 0x00, 0x00};

u8 nmk[ENC_KEY_LEN] = {0x2e, 0xdc, 0xcf, 0x9c, 0xb0, 0x95, 0x5d, 0xf8, 
					   0xd4, 0x23, 0xb3, 0xfa, 0xc6, 0x98, 0x95, 0xd5};

/****************************************************************************** 
  *	External Data
  ******************************************************************************/
  
/******************************************************************************
  *	External Funtion prototypes
  ******************************************************************************/
  
/******************************************************************************
  * Funtion prototypes
  ******************************************************************************/
  
void GV701x_HPGPSendEvent(u8 link_state, u8 state, u8 reason);

/******************************************************************************
 * @fn      GV701x_HPGPDriverInit
 *
 * @brief   Initializes the HPGP driver
 *
 * @param   app_id - application identification number
 *
 * @return  none
 */

void GV701x_HPGPDriverInit(u8 app_id)
{	
	u8* macaddr;	
	memset(&hpgp_drv_data, 0x00, sizeof(hpgp_drv_data_t));
	memset(&hpgp_nwk_data, 0x00, sizeof(hpgp_nwk_data_t));	
	
	hpgp_drv_data.app_id = app_id;
	SLIST_Init(&hpgp_drv_data.queues.appRxQueue);
	
	FM_Printf(FM_USER, "\nInit HPGPDrv (app id %bu)", app_id);
	
	macaddr = GV701x_ReadMacAddress();
	memcpy((u8*)hpgp_nwk_data.params.mac_addr, (u8*)macaddr, MAC_ADDR_LEN);
#if 0	
	FM_HexDump(FM_APP, "MAC: ", (u8*)hpgp_nwk_data.params.mac_addr, MAC_ADDR_LEN);			
#endif

	/*Initialize the Network Parameters*/	
	hpgp_nwk_data.params.nwk.line_mode = LINE_MODE_AC;	
	hpgp_nwk_data.params.nwk.txpower_mode = HIGH_TX_POWER_MODE;
	hpgp_nwk_data.params.nwk.dc_frequency = FREQ_50HZ;
	memcpy(hpgp_nwk_data.params.nwk.key.nid, cco_nid, NID_LEN);
	memcpy(hpgp_nwk_data.params.nwk.key.nmk, nmk, ENC_KEY_LEN);
	hpgp_nwk_data.params.nwk.net_id	= 0;
	hpgp_nwk_data.params.nwk.tei = 0;
	hpgp_nwk_data.params.nwk.role = DEV_MODE_STA;

	/*TBD: read ouid from mac address*/
	/*0x84,0x86,0xf3*/
	memcpy((u8*)hpgp_nwk_data.params.nwk.app_info.oem[0].ouid, 
		   (u8*)hpgp_nwk_data.params.mac_addr, OUID_LEN); 
	
	/*Initialize the Driver's State Machine*/
	hpgp_drv_data.state.state = HPGPDRV_INIT;
	hpgp_drv_data.state.event = HPGPDRV_IDLE_EVENT;	
	hpgp_drv_data.state.statedata = NULL;		
	hpgp_drv_data.state.statedatalen = 0;	
	
	hpgp_drv_data.scan.active = FALSE;
	hpgp_drv_data.scan.time = 0;
	hpgp_drv_data.start.active = FALSE;
	hpgp_drv_data.start.time = 0;

	srand(TL0);
	hpgp_nwk_data.params.nwk.tei = (u8)rand();	
	
	STM_AllocTimer(hpgp_drv_data.start.timer, HPGPDRV_START_TIMEOUT_EVT, 
					&hpgp_drv_data.app_id);
		
	GV701x_HPGPSetHwspec(hpgp_nwk_data.params.mac_addr,
				hpgp_nwk_data.params.nwk.line_mode,
				hpgp_nwk_data.params.nwk.txpower_mode, TRUE,
				hpgp_nwk_data.params.nwk.dc_frequency); 
}

/********************************************** ********************************
 * @fn      Gv701x_SetHwspec
 *
 * @brief   Sets the attributes of PLC link
 *
 * @param   mac_addr - MAC address to be set
 * 		    line_mode - Sets the PLC line Mode (AC or DC)
 *		    txpower_mode - Sets the PLC tx power Mode 
 *		    er_mode - Sets the extended range mode (TRUE-enabled, FALSE-disabled)
 * 			dc_frequency - The operating frequency in DC mode(invalid for AC mode) 
 *
 * @return  none
 */
void GV701x_HPGPSetHwspec(u8* mac_addr, u8 line_mode, u8 txpower_mode, u8 er_mode, u8 dc_frequency)
{
	u8 buf[MAX_HOST_CMD_LENGTH];
	hostCmdHwspec* hwspec;		
	
	memset(buf, 0x00, MAX_HOST_CMD_LENGTH);	
	
	/*Fill the Hwspec structure*/	
	hwspec = (hostCmdHwspec*)buf;
	hwspec->command = HOST_CMD_HARDWARE_SPEC_REQ;
	hwspec->action = ACTION_SET;
	hwspec->linemode = line_mode;	
	hwspec->dc_frequency = dc_frequency;		
	hwspec->hw_cfg.field.er = er_mode;
	hwspec->txpowermode = txpower_mode;		
	memcpy((u8*)&(hwspec->mac_addr), 
			(u8*)mac_addr, MAC_ADDR_LEN);

	/*Send command to firmware*/	
	GV701x_SendAppEvent(hpgp_drv_data.app_id, APP_FW_MSG_APPID, APP_MSG_TYPE_FW, HPGP_MAC_ID,
						EVENT_CLASS_MGMT, MGMT_FRM_ID, buf, sizeof(hostCmdHwspec), 0);
}

/******************************************************************************
 * @fn      GV701x_HPGPGetHwspec
 *
 * @brief   Gets attributes of PLC link
 *
 * @param   none
 *
 * @return  none
 */

void GV701x_HPGPGetHwspec(void)
{
	u8 buf[MAX_HOST_CMD_LENGTH];
	hostCmdHwspec* hwspec;		
	
	memset(buf, 0x00, MAX_HOST_CMD_LENGTH);	
	
	/*Fill the Hwspec structure*/	
	hwspec = (hostCmdHwspec*)buf;
	hwspec->command = HOST_CMD_HARDWARE_SPEC_REQ;
	hwspec->action = ACTION_GET;
	
	/*Send command to firmware*/	
	GV701x_SendAppEvent(hpgp_drv_data.app_id, APP_FW_MSG_APPID, APP_MSG_TYPE_FW, HPGP_MAC_ID,
						EVENT_CLASS_MGMT, MGMT_FRM_ID, buf, sizeof(hostCmdHwspec), 0);
}


/******************************************************************************
 * @fn      Gv701x_ReStartNwk
 *
 * @brief   Restarts/Initiates the PLC network
 *
 * @param   none
 *
 * @return  none
 */

void GV701x_HPGPReStartNwk(void)
{
	u8 buf[MAX_HOST_CMD_LENGTH];
	hostCmdRstSta* nwk;		

	memset(buf, 0x00, MAX_HOST_CMD_LENGTH); 
	
	/*Fill the Restart Network structure*/	
	nwk = (hostCmdRstSta*)buf;	
	nwk->command = APCM_STA_RESTART_REQ;

	/*Send command to firmware*/	
	GV701x_SendAppEvent(hpgp_drv_data.app_id, APP_FW_MSG_APPID, APP_MSG_TYPE_FW, HPGP_MAC_ID,
						EVENT_CLASS_CTRL, CONTROL_FRM_ID, buf, sizeof(hostCmdRstSta), 0);
}

/******************************************************************************
 * @fn      GV701x_HPGPStartNwk
 *
 * @brief   Starts the PLC network
 *
 * @param   netoption - Start options as defined in hpgpdef.h
 *		    nid - Network Identification Key
 *
 * @return  none
 */

void GV701x_HPGPStartNwk(u8 netoption, u8* nid)
{
	u8 buf[MAX_HOST_CMD_LENGTH];
	hostCmdNwk* nwk; 

	memset(buf, 0x00, MAX_HOST_CMD_LENGTH);	

	/*Fill the Start Network structure*/	
	nwk = (hostCmdNwk*)buf;	
	nwk->command = APCM_SET_NETWORKS_REQ;
	nwk->netoption = netoption;
	if(nid != NULL)
		memcpy((u8*)nwk->nid, (u8*)nid, NID_LEN);	

	/*Send command to firmware*/	
	GV701x_SendAppEvent(hpgp_drv_data.app_id, APP_FW_MSG_APPID, APP_MSG_TYPE_FW, HPGP_MAC_ID,
						EVENT_CLASS_CTRL, CONTROL_FRM_ID, buf, sizeof(hostCmdNwk), 0);
}


/******************************************************************************
 * @fn      Gv701x_SetNetId
 *
 * @brief   Sets the Network keys in the firmware
 *
 *	
 * @param   nmk - Network Managment Key
 *		    nid - Network Identification Key
 *
 * @return  none
 */
 
void GV701x_HPGPSetNetId(u8* nmk, u8* nid)
{
	u8 buf[MAX_HOST_CMD_LENGTH];
	hostCmdNetId* netid;		

	memset(buf, 0x00, MAX_HOST_CMD_LENGTH);	

	/*Fill the Network key structure*/	
	netid = (hostCmdNetId*)buf;	
	netid->command = APCM_SET_KEY_REQ;
	memcpy((u8*)netid->nid, (u8*)nid, NID_LEN);
	memcpy((u8*)netid->nmk, (u8*)nmk, ENC_KEY_LEN);
	/*Setting the security level to defualts*/
	netid->seclvl = SECLV_SC;
    
	/*Send command to firmware*/	
	GV701x_SendAppEvent(hpgp_drv_data.app_id, APP_FW_MSG_APPID, APP_MSG_TYPE_FW, HPGP_MAC_ID,
						EVENT_CLASS_CTRL, CONTROL_FRM_ID, buf, sizeof(hostCmdNetId), 0);
}

/******************************************************************************
 * @fn      GV701x_HPGPGetDevStats
 *
 * @brief   Sends a request to fetch Device Statistics 
 *		    (eg. frame transmit/receive count etc)
 *
 * @param   none
 *
 * @return  none
 */

void GV701x_HPGPGetDevStats(void)
{
	u8 buf[MAX_HOST_CMD_LENGTH];
	hostCmdDevstats *devstats;		

	memset(buf, 0x00, MAX_HOST_CMD_LENGTH);	

	/*Fill the Device Stats structure*/	
	devstats = (hostCmdDevstats*)buf;	
	devstats->command = HOST_CMD_DEVICE_STATS_REQ;
	devstats->action = ACTION_GET;	
	
	/*Send command to firmware*/	
	GV701x_SendAppEvent(hpgp_drv_data.app_id, APP_FW_MSG_APPID, APP_MSG_TYPE_FW, HPGP_MAC_ID,
						EVENT_CLASS_MGMT, MGMT_FRM_ID, buf, sizeof(hostCmdDevstats), 0);
}

/******************************************************************************
 * @fn      GV701x_HPGPSetPsAvln
 *
 * @brief   Sends a request to set AVLN powersave mode
 *
 * @param   mode - the powersave mode intended
 *
 * @return  none
 */

void GV701x_HPGPSetPsAvln(u8 mode)
{
	u8 buf[MAX_HOST_CMD_LENGTH];
	hostCmdPsAvln* psAvln;		

	memset(buf, 0x00, MAX_HOST_CMD_LENGTH); 

	/*Fill the Power Save AVLN structure*/	
	psAvln = (hostCmdPsAvln*)buf;	
	psAvln->command = HOST_CMD_PSAVLN_REQ;
	psAvln->action = ACTION_SET;
	psAvln->mode = mode;

	/*Send command to firmware*/		
	GV701x_SendAppEvent(hpgp_drv_data.app_id, APP_FW_MSG_APPID, APP_MSG_TYPE_FW, HPGP_MAC_ID,
						EVENT_CLASS_MGMT, MGMT_FRM_ID, buf, sizeof(hostCmdPsAvln), 0);
}

/******************************************************************************
 * @fn      GV701x_HPGPSetPsSta
 *
 * @brief   Sends a request to set AVLN powersave mode 
 *          configurations on the Station
 *
 * @param   mode - the powersave mode on the network
 * 		    awd - awake window
 * 		    psp - powersave period 
 *
 * @return  none
 */

void GV701x_HPGPSetPsSta(u8 mode, u8 awd, u8 psp)
{
	u8 buf[MAX_HOST_CMD_LENGTH];
	hostCmdPsSta* psSta;		

	/*Fill the Hybrii Header*/
	memset(buf, 0x00, MAX_HOST_CMD_LENGTH); 

	/*Fill the Power Save STA structure*/	
	psSta = (hostCmdPsSta*)buf;	
	psSta->command = HOST_CMD_PSSTA_REQ;
	psSta->action = ACTION_SET;
	psSta->mode = mode;
	psSta->awd =  awd;
	psSta->psp = psp;

	/*Send command to firmware*/	
	GV701x_SendAppEvent(hpgp_drv_data.app_id, APP_FW_MSG_APPID, APP_MSG_TYPE_FW, HPGP_MAC_ID,
						EVENT_CLASS_MGMT, MGMT_FRM_ID, buf, sizeof(hostCmdPsSta), 0);
}

/******************************************************************************
 * @fn      GV701x_HPGPGetPeerInfo
 *
 * @brief   Sends a request to get all peer information on PLC
 *
 * @param   none
 *
 * @return  none
 */

void GV701x_HPGPGetPeerInfo(void)
{
	u8 buf[MAX_HOST_CMD_LENGTH];
	hostCmdPeerinfo* peerinfo;		

	/*Fill the Hybrii Header*/
	memset(buf, 0x00, MAX_HOST_CMD_LENGTH); 

	/*Fill the Peer Info request structure*/	
	peerinfo = (hostCmdPeerinfo*)buf;	
	peerinfo->command = HOST_CMD_PEERINFO_REQ;
	peerinfo->action = ACTION_GET;

	/*Send command to firmware*/	
	GV701x_SendAppEvent(hpgp_drv_data.app_id, APP_FW_MSG_APPID, APP_MSG_TYPE_FW, HPGP_MAC_ID,
						EVENT_CLASS_MGMT, MGMT_FRM_ID, buf, sizeof(hostCmdPeerinfo), 0);
}

/******************************************************************************
 * @fn      GV701x_HPGPNetExit
 *
 * @brief   Request a network exit
 *
 * @param   none
 *
 * @return  none
 */

void GV701x_HPGPNetExit(void)
{
	u8 buf[MAX_HOST_CMD_LENGTH];
	hostCmdNetExit* netexit;		

	memset(buf, 0x00, MAX_HOST_CMD_LENGTH); 

	/*Fill the Net Exit request structure*/	
	netexit = (hostCmdNetExit*)buf;	
	netexit->command = APCM_NET_EXIT_REQ;
	netexit->action = ACTION_SET;

	/*Send command to firmware*/	
	GV701x_SendAppEvent(hpgp_drv_data.app_id, APP_FW_MSG_APPID, APP_MSG_TYPE_FW, HPGP_MAC_ID,
						EVENT_CLASS_CTRL, CONTROL_FRM_ID, buf, sizeof(hostCmdNetExit), 0);
}

/******************************************************************************
 * @fn      GV701x_HPGPScanNetwork
 *
 * @brief   Sends a request to Scan the network
 *
 * @param   scantime - time to scan 
 *
 * @return  none
 */

void GV701x_HPGPScanNetwork(tTime scantime)
{
	u8 buf[MAX_HOST_CMD_LENGTH];
	hostCmdScanNet* netlist;		

	memset(buf, 0x00, MAX_HOST_CMD_LENGTH); 

	/*Fill the Scan request structure*/	
	netlist = (hostCmdScanNet*)buf;	
	netlist->command = HOST_CMD_SCANNETWORK_REQ;
	netlist->action = ACTION_GET;
	netlist->scanTime = scantime;

	/*Send command to firmware*/	
	GV701x_SendAppEvent(hpgp_drv_data.app_id, APP_FW_MSG_APPID, APP_MSG_TYPE_FW, HPGP_MAC_ID,
						EVENT_CLASS_MGMT, MGMT_FRM_ID, buf, sizeof(hostCmdScanNet), 0);
}

/******************************************************************************
 * @fn      GV701x_HPGPNwkVendorFieldAccess
 *
 * @brief   Sends a request to get/set the network's vendor specific information
 *
 * @param   action (use ACTION_SET to set and ACTION_GET to fetch)
 *
 * @return  none
 */

void GV701x_HPGPNwkVendorFieldAccess(u8 action, u8* ouid, u8* vendor_info)
{
	u8 buf[MAX_HOST_CMD_LENGTH];
	hostCmdVendorSpec* vendorInfo;		

	memset(buf, 0x00, MAX_HOST_CMD_LENGTH); 

	/*Fill the Vendor Info request structure*/	
	vendorInfo = (hostCmdVendorSpec*)buf;	
	vendorInfo->command = HOST_CMD_VENDORSPEC_REQ;
	vendorInfo->action = action;
	vendorInfo->enable = FALSE;
	
	if((action == ACTION_SET) && (vendor_info != NULL) && (ouid != NULL))
	{
		vendorInfo->enable = TRUE;
		memcpy(&vendorInfo->vendor_ota.ouid, ouid, OUID_LEN);
		memcpy(&vendorInfo->vendor_ota.buf, vendor_info, VENDOR_SPEC_FIELD_LEN);
	}
	/*Send command to firmware*/	
	GV701x_SendAppEvent(hpgp_drv_data.app_id, APP_FW_MSG_APPID, APP_MSG_TYPE_FW, HPGP_MAC_ID,
						EVENT_CLASS_MGMT, MGMT_FRM_ID, buf, sizeof(hostCmdVendorSpec), 0);

}

/******************************************************************************
 * @fn      GV701x_HPGPDriverRxAppMsg
 *
 * @brief   Receives a message from another app/fw
 *
 * @params  msg_buf - message buffer
 *
 * @return  none
 */

void GV701x_HPGPDriverRxAppMsg(sEvent* event)
{
	gv701x_app_msg_hdr_t* msg_hdr = (gv701x_app_msg_hdr_t*)event->buffDesc.dataptr;
	hostHdr_t* hybrii_hdr;
	hostEventHdr_t* evnt_hdr;
	
	hybrii_hdr = (hostHdr_t*)(msg_hdr + 1);

	if(msg_hdr->dst_app_id == hpgp_drv_data.app_id)
	{
		memcpy(&hpgp_drv_data.state.msg_hdr, msg_hdr, sizeof(gv701x_app_msg_hdr_t));
		hpgp_drv_data.state.eventproto = hybrii_hdr->protocol;
		if((msg_hdr->src_app_id == APP_FW_MSG_APPID) && 
			(hybrii_hdr->type == EVENT_FRM_ID))
		{
			evnt_hdr = (hostEventHdr_t*)(hybrii_hdr + 1);
			hpgp_drv_data.state.event = evnt_hdr->type;			
			hpgp_drv_data.state.statedata = (u8*)(evnt_hdr + 1);
			hpgp_drv_data.state.statedatalen = (u16)(hybrii_hdr->length - sizeof(hostEventHdr_t));			
		}
		else
		{
			hpgp_drv_data.state.event = (u8)(*((u8*)(hybrii_hdr + 1)));
			hpgp_drv_data.state.statedata = (u8*)(hybrii_hdr + 1);
			hpgp_drv_data.state.statedatalen = (u16)hybrii_hdr->length;
		}		
		hpgp_drv_data.state.eventtype = hybrii_hdr->type;
		hpgp_drv_data.state.eventclass = event->eventHdr.eventClass;
		if((msg_hdr->src_app_id == APP_FW_MSG_APPID) && 
			(hybrii_hdr->type == EVENT_FRM_ID) &&
			(hpgp_drv_data.state.event == HOST_EVENT_APP_TIMER))
		{			
			GV701x_HPGPDriverTimerHandler((u8*)(evnt_hdr + 1)); 
			return;
		}
		else if((msg_hdr->src_app_id == APP_FW_MSG_APPID) && 
			(hybrii_hdr->type == EVENT_FRM_ID) &&
			(hpgp_drv_data.state.event == HOST_EVENT_APP_CMD))
		{	
			GV701x_HPGPDriver_CmdProcess((char*)(evnt_hdr + 1)); 
			return;
		}		
	}	
	else if(msg_hdr->dst_app_id == APP_BRDCST_MSG_APPID)
	{
		u8 *event = (u8*)(hybrii_hdr + 1);
		return;
	}

	GV701x_HPGPDriverSM(&hpgp_drv_data.state);	
}

/******************************************************************************
 * @fn      GV701x_HPGPDriverUpdateNwkTable
 *
 * @brief   Updates the network table about co-existing networks 
 *
 * @param   data_buf - Data coming from the firmware 
 *          of type hostEvent_ScanInd_t(in hpgp_msgs.h)
 *
 * @return  none
 */

void GV701x_HPGPDriverUpdateNwkTable(u8* data_buf)
{
	hostEvent_ScanInd_t* scanInd = (hostEvent_ScanInd_t*)data_buf;

//	if(scanInd->noOfEntries != 0)
	{	
		hpgp_nwk_data.netlist.entries = scanInd->noOfEntries;
		memcpy((u8*)&hpgp_nwk_data.netlist.list, (u8*)&scanInd->networkList, 
					MAX_NETWORK_LIST*sizeof(hostEventScanList));						
	}
	GV701x_HPGPSendEvent(HPGPDRV_SCAN_IND, 0, 0);
}

/******************************************************************************
 * @fn      GV701x_HPGPDriverTimerHandler
 *
 * @brief   Timer handler for HPGP driver timer events
 *
 * @param   event - event from firmware
 *
 * @return  none
 *
 */

void GV701x_HPGPDriverTimerHandler(u8* buf)
{	
	hostTimerEvnt_t* timerevt = (hostTimerEvnt_t*)buf;	

	if(buf == NULL)
		return;
	
	/*Demultiplexing the specific timer event*/ 					
	switch((u8)timerevt->type)
	{									
		default:
		break;
	}						
}

/******************************************************************************
 * @fn      GV701x_HPGPSendEvent
 *
 * @brief   Sends an indication(on state change) to the 
 *          app that requested the service
 *
 * @param   ind - indication type
 *          state - state of the PLC link (_nwIdState_e in hpgp_msgs.h)
 *          reason - reason for the transition 
 *                  (hostEventNetworkIndReason_e in hpgp_msgs.h)
 *
 * @return  none
 *
 */

void GV701x_HPGPSendEvent(u8 ind, u8 state, u8 reason) 
{	
	if(ind == HPGPDRV_UP_IND)
	{
		hpgp_drv_up_ind_msg_t hpgp_up;
		hpgp_up.event = ind;
		hpgp_up.state = state;
		hpgp_up.reason = reason;
		GV701x_SendAppEvent(hpgp_drv_data.app_id, hpgp_drv_data.start.app_id, APP_MSG_TYPE_APPIND, 
								APP_MAC_ID, EVENT_CLASS_CTRL, MGMT_FRM_ID, 
								&hpgp_up, sizeof(hpgp_drv_up_ind_msg_t), 0);		
		hpgp_drv_data.start.active = FALSE;		
	}
	else if(ind == HPGPDRV_DWN_IND)
	{
		hpgp_drv_dwn_ind_msg_t hpgp_dwn;
		hpgp_dwn.event = ind;
		hpgp_dwn.state = state;
		hpgp_dwn.reason = reason;
		GV701x_SendAppEvent(hpgp_drv_data.app_id, hpgp_drv_data.start.app_id, APP_MSG_TYPE_APPIND, 
								APP_MAC_ID, EVENT_CLASS_CTRL, MGMT_FRM_ID, 
								&hpgp_dwn, sizeof(hpgp_drv_dwn_ind_msg_t), 0);		
		hpgp_drv_data.start.active = FALSE;		
	}
	else if(ind == HPGPDRV_SCAN_IND)
	{
		hpgp_drv_scan_ind_msg_t scan_ind;
		scan_ind.event = ind;
		GV701x_SendAppEvent(hpgp_drv_data.app_id, hpgp_drv_data.scan.app_id, APP_MSG_TYPE_APPIND, 
								APP_MAC_ID, EVENT_CLASS_CTRL, MGMT_FRM_ID, 
								&scan_ind, sizeof(hpgp_drv_scan_ind_msg_t), 0);		
		hpgp_drv_data.scan.active = FALSE;
		hpgp_drv_data.scan.app_id = 0;					
	}
	else
		return;
}

/******************************************************************************
 * @fn      GV701x_HPGPDriverSM
 *
 * @brief   HPGP State Machine, it executes all internal/external 
 *			events triggered
 *
 * @param   state - state machine object of the driver
 *          (passed as a reference incase there are more than one object)
 *
 * @return  none
 *
 */

void GV701x_HPGPDriverSM(gv701x_state_t* state) 
{	
	if(state == NULL)
		return;

#if 1	
	if(state->event != HPGPDRV_IDLE_EVENT)
		FM_Printf(FM_APP, "\nHpgp S %bu E %bu P %bu C %bu E %bu Da %bu Sa %bu T %bu", 
				state->state, state->event,
				state->eventproto, state->eventclass, state->eventtype, 
				state->msg_hdr.dst_app_id, state->msg_hdr.src_app_id, state->msg_hdr.type);
#endif
	switch(state->state)
	{
		/*Initial State*/
		case HPGPDRV_INIT:
			if(state->eventproto == HPGP_MAC_ID)
			{
				if((state->eventclass == EVENT_CLASS_MGMT) && 
					(state->eventtype == MGMT_FRM_ID))
				{
					switch(state->event)
					{
						case HOST_CMD_HARDWARE_SPEC_CNF:
						break;

						default:
						break;
					}
				}
				else if((state->eventclass == EVENT_CLASS_CTRL) &&
					    (state->eventtype == EVENT_FRM_ID))
				{		
					switch(state->event)
					{
						case HOST_EVENT_SCAN_COMPLETE_IND:	
							if(state->statedata != NULL)
							{
								GV701x_HPGPDriverUpdateNwkTable(state->statedata);
							}
						break;
						
						default:
						break;
					}
				}				
			}
			else if(state->eventproto == APP_MAC_ID)
			{
				if(state->msg_hdr.type == APP_MSG_TYPE_APPEVENT)
				{
					switch(state->event)
					{				
						/*Scan event*/
						case HPGPDRV_SCAN_EVNT:						
							if(hpgp_drv_data.scan.active == FALSE)
							{
								hpgp_drv_data.scan.active = TRUE;	
								hpgp_drv_data.scan.app_id = state->msg_hdr.src_app_id;	
								GV701x_HPGPScanNetwork(hpgp_drv_data.scan.time);
							}
						break;	

						/*Start event*/
						case HPGPDRV_START_EVNT:
							if(hpgp_drv_data.start.active == FALSE)
							{
								state->state = HPGPDRV_START;
								hpgp_drv_data.start.active = TRUE;		
								hpgp_drv_data.start.app_id = state->msg_hdr.src_app_id;
								/*Exit previous state before starting*/
								GV701x_HPGPNetExit();
							}
						break;

						default:
						break;
					}
				}
			}			
		break;	

		/*Started/Down State*/
		case HPGPDRV_START:
		case HPGPDRV_DOWN:								
			if(state->eventproto == APP_MAC_ID)
			{
				if(state->msg_hdr.type == APP_MSG_TYPE_APPEVENT)
				{
					switch(state->event)
					{				
						/*Scan event*/
						case HPGPDRV_SCAN_EVNT:						
							if(hpgp_drv_data.scan.active == FALSE)
							{
								hpgp_drv_data.scan.active = TRUE;	
								hpgp_drv_data.scan.app_id = state->msg_hdr.src_app_id;	
								GV701x_HPGPScanNetwork(hpgp_drv_data.scan.time);
							}
						break;	

						/*Start event*/
						case HPGPDRV_START_EVNT:
							if(hpgp_drv_data.start.active == FALSE)
							{
								state->state = HPGPDRV_START;
								hpgp_drv_data.start.active = TRUE;		
								hpgp_drv_data.start.app_id = state->msg_hdr.src_app_id;
								/*Exit previous state before starting*/
								GV701x_HPGPNetExit();
							}
						break;

						/*Stop event*/
						case HPGPDRV_STOP_EVNT:
							hpgp_drv_data.start.active = FALSE;
							hpgp_nwk_data.params.nwk.role = DEV_MODE_STA;
							memcpy((u8*)hpgp_nwk_data.params.nwk.key.nid, 
									(u8*)cco_nid, NID_LEN);							
							GV701x_HPGPNetExit();
						break;

						default:
						break;
					}
				}
			}
			else if(state->eventproto == HPGP_MAC_ID)
			{
				if((state->eventclass == EVENT_CLASS_MGMT) && 
					(state->eventtype == MGMT_FRM_ID))
				{
					switch(state->event)
					{				
						case HOST_CMD_SCANNETWORK_CNF:										
						break;		

						case HOST_CMD_VENDORSPEC_CNF:
						{
							u8 startmode;
							/*Finally Start attempt to Join/Start the device*/
							if(hpgp_nwk_data.params.nwk.role == DEV_MODE_STA)						
								startmode = NETWORK_JOIN;
							else if(hpgp_nwk_data.params.nwk.role == DEV_MODE_CCO)
								startmode = NETWORK_START;					
							else
								break;
							
							GV701x_HPGPStartNwk(startmode, hpgp_nwk_data.params.nwk.key.nid);
						}
						break;						
					
						default:
						break;
					}
				}
				else if((state->eventclass == EVENT_CLASS_CTRL) &&
					   (state->eventtype == CONTROL_FRM_ID))
				{
					switch(state->event)
					{	
						case APCM_NET_EXIT_CNF:		
							/*Continuation of Active Start sequence*/
						break;
	
						
						case APCM_SET_KEY_CNF:		
							/*Set the Vendor filed on the network incase when
							  the device starts the network, else just check 
							  what it is*/
							if(hpgp_nwk_data.params.nwk.role == DEV_MODE_CCO)
							{
								GV701x_HPGPNwkVendorFieldAccess(ACTION_SET, hpgp_nwk_data.params.nwk.app_info.oem[0].ouid,
											(u8*)hpgp_nwk_data.params.nwk.app_info.byte_arr);
							}
							else if(hpgp_nwk_data.params.nwk.role == DEV_MODE_STA)
							{
								GV701x_HPGPNwkVendorFieldAccess(ACTION_GET, NULL, NULL);											
							}
						break;
	
						case APCM_SET_NETWORKS_CNF:
						break;

						default:
						break;	
					}
				}
				else if(state->eventtype == EVENT_FRM_ID)
				{		
					switch(state->event)
					{
						case HOST_EVENT_SCAN_COMPLETE_IND:	
							if(state->statedata != NULL)
							{
								GV701x_HPGPDriverUpdateNwkTable(state->statedata);
							}
						break;
										
						/*Association Indication*/
						case HOST_EVENT_TYPE_ASSOC_IND:
							if(state->statedata) 
							{
								hostEvent_assocInd* assocInd = (hostEvent_assocInd*)state->statedata;
								hpgp_nwk_data.params.nwk.tei = assocInd->tei;
							}					
						break;

						case HOST_EVENT_NETWORK_IND:	
							/*Demultiplexing the type of nwk event*/
							if(state->statedata) 
							{
								hostEvent_NetworkId* nwkId = (hostEvent_NetworkId*)state->statedata;
#if 0								
								FM_Printf(FM_APP, "\nIND: s %bu r %bu", nwkId->state, nwkId->reason);
#endif								
								switch((u8)(nwkId->state))		
								{						
									case NWID_STATE_NET_DISC:
									break;
		
									/*Unassociated event*/
									case NWID_STATE_INIT:
									case NWID_STATE_UNASSOC_STA:
									case NWID_STATE_UNASSOC_CCO:										
										switch(nwkId->reason)
										{
											case HOST_EVENT_NW_IND_REASON_NETEXIT_CMD:
												/*Continue with start*/
												if(hpgp_drv_data.start.active == TRUE)
												{
													GV701x_HPGPSetNetId(hpgp_nwk_data.params.nwk.key.nmk, 
																hpgp_nwk_data.params.nwk.key.nid);											
												}
												else
												{
													state->state = HPGPDRV_DOWN;	
													GV701x_HPGPSendEvent(HPGPDRV_DWN_IND, NWID_STATE_ASSOC_STA, 0);
												}												
											break;

											case HOST_EVENT_NW_IND_REASON_CCO_LEAVE_IND:
											case HOST_EVENT_NW_IND_REASON_NO_AVLN:
											case HOST_EVENT_NW_IND_REASON_AUTH_FAIL:
											case HOST_EVENT_NW_IND_REASON_ASSOC_FAIL:
											case HOST_EVENT_NW_IND_REASON_BCNLOSS:
											case HOST_EVENT_NW_IND_REASON_USERCMD:
												/*Check if the event is valid in the context of 
												  operating role*/									
												state->state = HPGPDRV_DOWN;
												GV701x_HPGPSendEvent(HPGPDRV_DWN_IND, nwkId->state, nwkId->reason);
											break;
											default:
											break;
										}
									break;
		
									/*Associated Event*/
									//case NWID_STATE_ASSOC_STA:
									case NWID_STATE_ASSOC_CCO:
										/*Check if the event is valid in the context of 
										  operating role*/								
										if((hpgp_nwk_data.params.nwk.role == DEV_MODE_CCO) &&
											((u8)(nwkId->state) == NWID_STATE_ASSOC_CCO))
										{		
#if 0																	 
											FM_Printf(FM_APP, "\nHPGP Assoc");
#endif											
											state->state = HPGPDRV_UP;
											GV701x_HPGPSendEvent(HPGPDRV_UP_IND, nwkId->state, nwkId->reason);										
										}
									break;
									
									default:
									break;
								}						
							}															
						break;	

						/*Authentication Event*/
						case HOST_EVENT_AUTH_COMPLETE:
							if(hpgp_drv_data.start.active == TRUE)							
							{
								state->state = HPGPDRV_UP;	 
#if 0								
								FM_Printf(FM_APP, "\nHPGP DrvUp");
#endif								
								GV701x_HPGPSendEvent(HPGPDRV_UP_IND, NWID_STATE_ASSOC_STA, 0);						
							}
						break;

						default:
						break;
					}
				}
			}
		break;		

		/*Up State*/
		case HPGPDRV_UP:
			if(state->eventproto == APP_MAC_ID)
			{	
				if(state->msg_hdr.type == APP_MSG_TYPE_APPEVENT)
				{
					switch(state->event)
					{		
						/*Start event*/
						case HPGPDRV_START_EVNT:
							if(hpgp_drv_data.start.active == FALSE)
							{
								hpgp_drv_start_evnt_msg_t* hpgp_start = 
										(hpgp_drv_start_evnt_msg_t*)state->statedata;
							
								state->state = HPGPDRV_START;						
								hpgp_drv_data.start.active = TRUE;						
								hpgp_drv_data.start.app_id = state->msg_hdr.src_app_id;						
#if 0								
								FM_HexDump(FM_APP,"\nStart Nid:",
									(u8*)hpgp_nwk_data.params.nwk.key.nid, NID_LEN);
#endif									
								/*Exit previous state before starting*/
								GV701x_HPGPNetExit();
							}
						break;
						
						/*Scan event*/
						case HPGPDRV_SCAN_EVNT:
							if(hpgp_drv_data.scan.active == FALSE)
							{	
								hpgp_drv_data.scan.active = TRUE;
								hpgp_drv_data.scan.app_id = state->msg_hdr.src_app_id;
								GV701x_HPGPScanNetwork(hpgp_drv_data.scan.time);
							}
						break;	

						/*Stop event*/
						case HPGPDRV_STOP_EVNT:
							hpgp_drv_data.start.active = FALSE;
							hpgp_nwk_data.params.nwk.role = DEV_MODE_STA;
							memcpy((u8*)hpgp_nwk_data.params.nwk.key.nid, 
									(u8*)cco_nid, NID_LEN);							
							GV701x_HPGPNetExit();
						break;	

						default:
						break;
					}
				}
			}
			else if(state->eventproto == HPGP_MAC_ID)
			{
				if((state->eventclass == EVENT_CLASS_MGMT) && 
					(state->eventtype == MGMT_FRM_ID))
				{			
					switch(state->event)
					{	
						case HOST_CMD_SCANNETWORK_CNF:										
						break;	

						case HOST_CMD_PSAVLN_CNF:
						break;
						
						case HOST_CMD_PSSTA_CNF:
						break;
						
						case HOST_CMD_PEERINFO_CNF:
							if(state->statedata)					
							{					
								u8 lclCount;
								hostCmdPeerinfo *peerinfo;
								peerinfodata *peer;
								peerinfo = (hostCmdPeerinfo *)(state->statedata);
	
								if(peerinfo->result == STATUS_SUCCESS) 
								{
									peer = (peerinfodata *)(peerinfo + 1);
									for( lclCount=0; lclCount<(peerinfo->noofentries); lclCount++) 
									{
										peer = (peer + 1);										
									}
								} 
							}
						break;	

						default:
						break;
					}
				}
				else if(state->eventtype == EVENT_FRM_ID)
				{		
					switch(state->event)
					{				
						case HOST_EVENT_SCAN_COMPLETE_IND:
							if(state->statedata != NULL)
							{
								GV701x_HPGPDriverUpdateNwkTable(state->statedata);
							}
						break;

						/*Beacon Loss*/
						case HOST_EVENT_BCN_LOSS:	
#if 0	
							FM_Printf(FM_APP, "\nHPGPbcnloss");
							//state->state = HPGPDRV_DOWN;			
#endif
						break;

						case HOST_EVENT_NETWORK_IND:									
							/*Demultiplexing the type of nwk event*/
							if(state->statedata) 
							{
								hostEvent_NetworkId* nwkId = (hostEvent_NetworkId*)state->statedata;
						    	switch((u8)(nwkId->state))	 	
						    	{			
							    	case NWID_STATE_NET_DISC:
									break;

									case NWID_STATE_INIT:
									case NWID_STATE_UNASSOC_CCO:							
									case NWID_STATE_UNASSOC_STA:
										/*Check if the event is valid in the context of 
										  operating role*/								
										switch(nwkId->reason)
										{										  
											case HOST_EVENT_NW_IND_REASON_NETEXIT_CMD:	
												if(hpgp_drv_data.start.active == TRUE)
												{
													GV701x_HPGPSetNetId(hpgp_nwk_data.params.nwk.key.nmk, 
																hpgp_nwk_data.params.nwk.key.nid);											
												}
												else
												{
													state->state = HPGPDRV_DOWN;	
													GV701x_HPGPSendEvent(HPGPDRV_DWN_IND, NWID_STATE_ASSOC_STA, 0);
												}
											break;
											
											case HOST_EVENT_NW_IND_REASON_CCO_LEAVE_IND:
											case HOST_EVENT_NW_IND_REASON_NO_AVLN:
											case HOST_EVENT_NW_IND_REASON_AUTH_FAIL:
											case HOST_EVENT_NW_IND_REASON_ASSOC_FAIL:
											case HOST_EVENT_NW_IND_REASON_BCNLOSS: 
#if 0										
												FM_Printf(FM_APP, "\nHPGP Down");
#endif												
												state->state = HPGPDRV_DOWN;	
												GV701x_HPGPSendEvent(HPGPDRV_DWN_IND, nwkId->state, nwkId->reason);
											break;
											
											default:
											break;
										}
									break;
																
									default:
									break;
						    	}						
							}									
						break;	

						case HOST_EVENT_PEER_STA_LEAVE:			
#if 0					
							FM_Printf(FM_APP, "\nPeer Leave Ind");
#endif							
							//state->state = HPGPDRV_DOWN;						
						break;				

						default:
						break;
					}
				}
				else if((state->eventclass == EVENT_CLASS_CTRL) &&
					   (state->eventtype == CONTROL_FRM_ID))
				{
					switch(state->event)
					{	
						case APCM_NET_EXIT_CNF:
							//state->state = HPGPDRV_DOWN;														
						break;

						default:
						break;
					}
				}
			}
		break;
		
		default:
		break;		
	}

	state->event = HPGPDRV_IDLE_EVENT;	
	state->eventtype = 0;
	state->eventclass = 0;
	state->eventproto = 0;
	state->statedata = NULL;	
	state->statedatalen = 0;	
	memset((u8*)&state->msg_hdr, 0x00, sizeof(gv701x_app_msg_hdr_t));	
}

/******************************************************************************
 * @fn      GV701x_Nwk_CmdProcess
 *
 * @brief   It handles application command line requests
 *
 * @param   CmdBuf - command string
 *
 * @return  none
 *
 */

void GV701x_HPGPDriver_CmdProcess(char* CmdBuf) 
{
#if 1
	if(strcmp(CmdBuf, "state") == 0) 
		printf("\nHpgp S %bu E %bu", hpgp_drv_data.state.state, hpgp_drv_data.state.event);
	else if(strcmp(CmdBuf, "stats") == 0) 
	{		
		printf("\nHpgp: role %s", (hpgp_nwk_data.params.nwk.role == DEV_MODE_CCO) ? "CCO" : "STA");
		FM_HexDump(FM_USER, "\nnid: ", hpgp_nwk_data.params.nwk.key.nid, NID_LEN);
		FM_HexDump(FM_USER, "\rnmk: ", hpgp_nwk_data.params.nwk.key.nmk, ENC_KEY_LEN);
		FM_HexDump(FM_USER, "\rouid: ", hpgp_nwk_data.params.nwk.app_info.oem[0].ouid, OUID_LEN);
	} 
	else if(strcmp(CmdBuf, "nvclear") == 0) 
	{
		GV701x_FlashErase(hpgp_drv_data.app_id);
	}	
#else
	CmdBuf = CmdBuf;
#endif	
}	


#endif /*HPGP_DRIVER_APP*/
