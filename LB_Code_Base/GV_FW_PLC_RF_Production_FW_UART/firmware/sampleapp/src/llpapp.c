
/* ========================================================
 *
 * @file:  llpapp.c
 * 
 * @brief: This application implements the Led Lighting Protocol 
 *         on the remote nodes that are connected to either PLC 
 *         link or IEEE 802.15.4 link.
 *
 *  Copyright (C) 2010-2015, Greenvity Communications, Inc.
 *  All Rights Reserved
 *  
 * =========================================================*/

#ifdef LLP_APP

/****************************************************************************** 
  *	Includes
  ******************************************************************************/

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "gv701x_includes.h"
#ifdef HPGP_DRIVER_APP
#include "gv701x_hpgpdriver.h"
#endif
#ifdef LRWPAN_DRIVER_APP
#include "gv701x_lrwpandriver.h"
#endif
#ifdef NWKSTARTUP_APP
#include "gv701x_nwkstartup.h"
#endif
#ifdef DEVICEINTF_APP
#include "deviceintfapp.h"
#endif
#ifdef ROUTE_APP
#include "route.h"
#include "route_fw.h"
#endif
#ifdef REGISTER_APP
#include "register.h"
#include "register_fw.h"
#endif
#ifdef RTOPO_APP
#include "route_topo_fw.h"
#include "route_topo.h"
#endif
#include "llpapp.h"
#include "llpapp_fw.h"


/****************************************************************************** 
  *	Global Data
  ******************************************************************************/  
u8 node_app_id;
gv701x_app_queue_t node_queues;	

/* Node data */
node_data_t node_data;  	

/* Node state */
gv701x_state_t node_state;			

/****************************************************************************** 
  *	External Data
  ******************************************************************************/

/******************************************************************************
  *	External Funtion prototypes
  ******************************************************************************/

/******************************************************************************
  *	Funtion prototypes
  ******************************************************************************/
void LlpApp_Tx(u8* frame, u8 payloadLen, u8 retrycnt);
void LlpApp_HandleUpdateReq(llp_msg_req_t* msg);
void LlpApp_HandleGroupUpdateReq(llp_msg_req_t* msg);
void LlpApp_SendUpdateRsp(u8 solicited, u8 action, u16 seq);
void LlpApp_DispStats(void);
void LlpApp_HandleDevStats(llp_msg_req_t *msg);

/******************************************************************************
 * @fn      LlpApp_Init
 *
 * @brief   Initializes the Light Link Layer
 *
 * @param   app_id - application identification number
 *
 * @return  none
 */
void LlpApp_Init (u8 app_id) 
{	
	u8* macaddr;
#ifdef DEVICEINTF_APP
	u8 idx;
#endif

	/*Initializing Node Data*/
	memset(&node_data, 0x00, sizeof(node_data_t));
	node_app_id = app_id;
	SLIST_Init(&node_queues.appRxQueue);

	FM_Printf(FM_USER, "\nInit LLPApp (app id %bu)", app_id);	
	memcpy((u8*)&(node_data.macaddr), (u8*)macaddr, MAC_ADDR_LEN);
#if 0	
	FM_HexDump(FM_APP, "MAC: ", (u8*)&(node_data.macaddr), MAC_ADDR_LEN);			
#endif

	/*Initializing State machine*/
	memset(&node_state, 0x00, sizeof(gv701x_state_t));
	node_state.state = NODE_IDLE_STATE;
	node_state.event = NODE_IDLE_EVNT;
	node_state.statedata = NULL;
	node_state.statedatalen = 0;
	node_data.active = TRUE;
		
#ifdef DEVICEINTF_APP		
	/*Initialize the device type*/
	(u8)(*node_data.dev_type) = DEVINTF_NONE;
	for(idx = 0; (dev_list[idx].type != DEVINTF_NONE); idx++)
	{		
		if(dev_list[idx].type != DEVINTF_NONE)
		{
			node_data.dev_type = &dev_list[idx].type;
			node_data.dev_subtype = &dev_list[idx].subtype;
			break;
		}
	}
#endif	
}

/******************************************************************************
 * @fn      LlpApp_Rx
 *
 * @brief   Receives Over the Air frame
 *
 * @param   buf - data packet
 *			len - length in number of bytes
 *
 * @return  none
 */

void LlpApp_Rx(u8* buf, u8 len)
{
	sEth2Hdr* petherhdr = (sEth2Hdr*)(buf);
#ifdef ROUTE_APP
	route_hdr_t* rhdr = (route_hdr_t* )(&buf[sizeof(sEth2Hdr)]);
#endif
	llp_msg_req_t* msg_hdr;	
	len = len;

	if(petherhdr->ethtype != APP_ETHER_PROTO)
		return;

	if(RHDR_GET_CMDID(rhdr) != APP_FRAME)
		return;
	
	msg_hdr = (llp_msg_req_t*)(&(buf[LLP_HDR_OFFSET]));

	switch(msg_hdr->cmd_id) 
	{		
		case UPDATE_DEVICE_REQ:
			if(node_state.state == NODE_ACTIVE_STATE)		
				LlpApp_HandleUpdateReq((llp_msg_req_t*)msg_hdr);
			else 
				node_data.stats.drop++;
		break;

		case CHANGE_GROUP: 
			if(node_state.state == NODE_ACTIVE_STATE) 
				LlpApp_HandleGroupUpdateReq((llp_msg_req_t *)msg_hdr);			
			else 
				node_data.stats.drop++;
		break;
		
		case DEV_STATS_REQ:
			LlpApp_HandleDevStats((llp_msg_req_t *)msg_hdr);	
		break;
		
		default:			
		break;
	}	
}

/******************************************************************************
 * @fn      LlpApp_Tx
 *
 * @brief   Sends an Over the Air frame
 *
 * @param   buf - data packet
 *			payloadLen - length in number of bytes
 *			retrycnt - number of re-transmissions
 *
 * @return  none
 */

void LlpApp_Tx(u8* buf, u8 payloadLen, u8 retrycnt)
{
	u8 i;
	u8 brdcstmac[MAC_ADDR_LEN] = {0xff,0xff,0xff,0xff,0xff,0xff};
	sEth2Hdr *pEth2Hdr = (sEth2Hdr *)buf;
	llp_msg_req_t* msg_hdr = (llp_msg_req_t*)(buf + sizeof(sEth2Hdr));

	/*Fill llp frame control*/
	msg_hdr->fc.control_bits |= LLP_FRAME_DIR_LIGHT_TO_CONTROLLER \
								<< LLP_FRAME_DIRECTION;	
	
	/* If node is not in the active state, we cannot send data out */
	if(node_state.state != NODE_ACTIVE_STATE) 
	{
		node_data.stats.drop++;
		return;
	}

	/*Send frame and then retry */
	for(i=0; i<retrycnt; i++)  
	{
		if(i)
			msg_hdr->fc.control_bits |= cpu_to_le16(LLP_FRAME_RETRY);	

		if(node_state.state == NODE_ACTIVE_STATE) 
		{
#ifdef ROUTE_APP			
			route_send_to_ll((u8*)buf, payloadLen, APP_FRAME, FALSE);			
#endif
		}
		else
		{
#ifdef ROUTE_APP		
			route_send_to_ll((u8*)buf, payloadLen, APP_FRAME, FALSE);
#endif
		}
	}		
}

/******************************************************************************
 * @fn      LlpApp_HandleUpdateReq
 *
 * @brief   Handles the Update Request frame
 *
 * @param   msg - LLP message (define found in llpapp.h)
 *
 * @return  none
 */

void LlpApp_HandleUpdateReq (llp_msg_req_t* msg)
{
	u8 idx, jdx;
	llp_sta_update_msg_t* update_req = (llp_sta_update_msg_t*)(msg + 1);
	u8 *payload = (u8*)(update_req);
	u8 dev_type = 0;
	u8 num_of_tlv = 0;
	u8 update_len = 0;
	u16 mseq = 0;

	node_data.stats.updtreq++;

	if( (!(cpu_to_le16(msg->fc.control_bits) & LLP_FRAME_BROADCAST)) &&
		(!(cpu_to_le16(msg->fc.control_bits) & LLP_FRAME_GROUPCAST)) ) 
	{		
		mseq = cpu_to_le16(msg->fc.seq_num);
		
		if(memcmp_cpu_to_le(&(update_req->mac_addr), 
						&(node_data.macaddr), MAC_ADDR_LEN) )
		{ 
			return;
		}
	}
	else if((cpu_to_le16(msg->fc.control_bits) & LLP_FRAME_GROUPCAST)) 
	{
		if(node_data.llp_grp != msg->llp_group) 
		{
#if 0		
			FM_Printf(FM_APP, "\nLLP Group not matched");
#endif
			return;
		}
	}

	if((update_req->action != LLP_UPDATE_SET) &&
	  (update_req->action != LLP_UPDATE_GET))
	{
		return;
	}

	if(update_req->action == LLP_UPDATE_SET)
	{
		update_len += sizeof (llp_sta_update_msg_t);
		dev_type = (u8)payload[update_len];
		update_len += sizeof(dev_type);
		num_of_tlv = (u8)payload[update_len];		
		update_len += sizeof(num_of_tlv);			
		payload = payload + update_len;
		
#ifdef DEVICEINTFAPP		
		if(dev_type == DEVINTF_NONE)
			return;			
#endif					
		
	node_data.active = update_req->active;
	}
	else if(update_req->action == LLP_UPDATE_GET)
	{
		update_len += sizeof (llp_sta_update_msg_t);
		dev_type = (u8)payload[update_len];
		update_len += sizeof(dev_type);
		num_of_tlv = (u8)payload[update_len];		
		update_len += sizeof(num_of_tlv);			
		payload = payload + update_len;	
		dev_type = (u8)(*node_data.dev_type);
	}

#if 0
	FM_Printf(FM_APP, "\nUpdReq (a %bu dt %bu act %bu)", update_req->action, dev_type,
			update_req->active);
#endif

#ifdef DEVICEINTF_APP	
	
	for(idx = 0; (deviceintf_data.dev_list[idx].type != DEVINTF_NONE); idx++)
	{
		if(deviceintf_data.dev_list[idx].type == dev_type)
		{
			break;
		}
	}
	
	if(idx != 0)
		return;		
	

	for (jdx = 0; (deviceintf_data.dev_list[idx].io_list[jdx].type != DEVINTF_IO_NONE); jdx++) 
	{				
		if(update_req->action == LLP_UPDATE_SET)
		{
		    if(update_req->active == 0)
            {

            }
			else if((u8)(*payload) == deviceintf_data.dev_list[idx].io_list[jdx].type)
			{			
				payload += 1;
				memcpy_cpu_to_le((u8*)deviceintf_data.dev_list[idx].io_list[jdx].p_val, (u8*)payload, 
								 deviceintf_data.dev_list[idx].io_list[jdx].len);				
				//FM_HexDump(FM_USER,"\nSet Tlv: ",(u8*)deviceintf_data.dev_list[idx].io_list[jdx].p_val,
				//	       deviceintf_data.dev_list[idx].io_list[jdx].len);
								
				deviceintf_data.dev_list[idx].io_list[jdx].trigger = TRUE;
				payload = payload + (u8)deviceintf_data.dev_list[idx].io_list[jdx].len;
			}
		}
		/* Temporarily here cos in a get frame the host does not send tlv's*/
		else if(update_req->action == LLP_UPDATE_GET)
		{
			if((u8)(*payload) == deviceintf_data.dev_list[idx].io_list[jdx].type)		
			{
			//	printf("\nType Match %bu", deviceintf_data.dev_list[idx].io_list[jdx].type);		
				deviceintf_data.dev_list[idx].io_list[jdx].trigger = TRUE;
				payload = payload + 1;
				/*TBD: if TLV do not match then there should exists an exit procedure*/
			}
		}		
	}	
	
	if(update_req->action == LLP_UPDATE_SET)	
	{
		device_inst_msg_t device_inst;
				
		if(update_req->active == 0)
		{
		
			device_inst.event = DEVINTF_IO_OFF;
			device_inst.dev_type = dev_type;
			node_data.active = 0;
								
		
		}
		else
		{
			device_inst.event = DEVINTF_IO_INSTRUCTION;
			device_inst.dev_type = dev_type;
			node_data.active = 1;
				
		}
		
		GV701x_SendAppEvent(node_app_id, deviceintf_data.app_id, APP_MSG_TYPE_APPEVENT,
					APP_MAC_ID, EVENT_CLASS_CTRL, MGMT_FRM_ID,			
					&device_inst, sizeof(device_inst_msg_t), 0);					
	}
#endif /*DEVICEINTF_APP*/

	/*Send Response only if unicast*/
	if((cpu_to_le16(msg->fc.control_bits) & LLP_FRAME_BROADCAST) ||
		(cpu_to_le16(msg->fc.control_bits) & LLP_FRAME_GROUPCAST))
	{
	}
	else 
	{
		LlpApp_SendUpdateRsp(TRUE, update_req->action, mseq);
	}
}

/******************************************************************************
 * @fn      LlpApp_HandleGroupUpdateReq
 *
 * @brief   Handles the Group Update Request frame
 *
 * @param   msg - LLP message (define found in llpapp.h)
 *
 * @return  none
 */

void LlpApp_HandleGroupUpdateReq(llp_msg_req_t* msg)
{
	llp_sta_group_update_req_t *grp_upt = (llp_sta_group_update_req_t *)(msg + 1);

	if( (!(cpu_to_le16(msg->fc.control_bits) & LLP_FRAME_BROADCAST)) &&
		(!(cpu_to_le16(msg->fc.control_bits) & LLP_FRAME_GROUPCAST)) )
	{
		if(memcmp_cpu_to_le(&(grp_upt->mac_addr), 
						&(node_data.macaddr), MAC_ADDR_LEN) )
			return;
	}
	else if((cpu_to_le16(msg->fc.control_bits) & LLP_FRAME_GROUPCAST))
	{
		if(node_data.llp_grp != msg->llp_group)
			return;		
	}

    node_data.llp_grp = grp_upt->llp_grp;
}

/******************************************************************************
 * @fn      LlpApp_HandleGroupUpdateReq
 *
 * @brief   Sends the Update Response frame
 *
 * @param   solicited - if the frame is in response to a request
 *                      (TRUE or FLASE)
 *			action - LLP_UPDATE_SET: when sent asynchronously 
 *                   LLP_UPDATE_GET: when accompained by a request
 *
 * @return  none
 */

void LlpApp_SendUpdateRsp(u8 solicited, u8 action, u16 seq) 
{
	u8 idx, jdx, count = 0;
	u8* payload_numtlv_off = NULL;
	u8 num_of_tlv = 0;
	u8 buf[MAX_PKT_BUFFSIZE];
	llp_msg_req_t* msg_hdr = &buf[LLP_HDR_OFFSET];
	llp_sta_update_msg_t* update = (llp_sta_update_msg_t*)(msg_hdr + 1);
	u8 *payload = (u8 *)(update + 1); 
	
	memset(buf, 0x00, MAX_PKT_BUFFSIZE);	

#if 1
	FM_Printf(FM_APP, "\nUpdRsp (act %bu s %bu seq %u)", action, solicited, seq);
#endif

	/*Fill message header*/ 
	msg_hdr->fc.control_bits = 0;
	msg_hdr->fc.seq_num = cpu_to_le16(seq);

	if(solicited == TRUE)
	{
		u16 ctrl_bits = 0;
		ctrl_bits |= LLP_FRAME_SOLICITED;
		msg_hdr->fc.control_bits = cpu_to_le16(ctrl_bits);
		msg_hdr->cmd_id = UPDATE_DEVICE_RSP;
		node_data.stats.updtrsp++; 
	}
	else
	{
		node_data.stats.asyncrsp++;
		msg_hdr->cmd_id = UPDATE_ASYNC_DATA_REQ;
	}
	
#ifdef REGISTER_APP
	node_data.nwk_addr = register_data.nwk_addr.addr_16bit; 
#endif	
	msg_hdr->hpgp_group = cpu_to_le16(node_data.nwk_addr);
	msg_hdr->llp_group = node_data.llp_grp;

	update->groupid = node_data.llp_grp;
	update->action = action;
	update->reqmsg = FALSE; 
    update->active = node_data.active;
	memcpy_cpu_to_le((u8*)&(update->mac_addr), (u8*)&(node_data.macaddr), 
			MAC_ADDR_LEN);

	payload[count] = (u8)(*node_data.dev_type);
	count++;
	payload_numtlv_off = &payload[count];
	count++;
#ifdef DEVICEINTF_APP	
	for(idx = 0; (deviceintf_data.dev_list[idx].type != DEVINTF_NONE); idx++)
	{
		if(deviceintf_data.dev_list[idx].type == (u8)(*node_data.dev_type))
		{
			break;
		}
	}
	
	for (jdx = 0; (deviceintf_data.dev_list[idx].io_list[jdx].type != DEVINTF_IO_NONE); jdx++) 
	{
		if(deviceintf_data.dev_list[idx].io_list[jdx].trigger == TRUE)
		{						
			num_of_tlv++;
			if(solicited == TRUE) 
			{
				if(update->action == LLP_UPDATE_GET)
				{
					deviceintf_data.dev_list[idx].io_list[jdx].trigger = FALSE; 		
				}
			} 
			else 
			{
				deviceintf_data.dev_list[idx].io_list[jdx].trigger = FALSE;
			}
			
			payload[count] = deviceintf_data.dev_list[idx].io_list[jdx].type; 
			count++;
			memcpy((u8*)(&payload[count]), (u8*)deviceintf_data.dev_list[idx].io_list[jdx].p_val, 
							 deviceintf_data.dev_list[idx].io_list[jdx].len);
						
			count += (u8)deviceintf_data.dev_list[idx].io_list[jdx].len;
		}
	}	
	*payload_numtlv_off = num_of_tlv;
#endif /*DEVICEINTF_APP*/

	LlpApp_Tx(buf,(sizeof(llp_msg_req_t) + sizeof(llp_sta_update_msg_t) + 
					count + LLP_HDR_OFFSET), 1);
}

/******************************************************************************
 * @fn      LlpApp_HandleDevStats
 *
 * @brief   Handles the request to fetch device statistics
 *
 * @param   msg - LLP message (define found in llpapp.h)
 *
 * @return  none
 */

void LlpApp_HandleDevStats(llp_msg_req_t *msg)
{
	u8 len = 0;
	u8 buf[MAX_PKT_BUFFSIZE];	
	llp_msg_req_t* msg_hdr = &buf[LLP_HDR_OFFSET];
	llp_devstats_req_msg_t* devstats_req = (llp_devstats_req_msg_t*)(msg + 1);
	llp_devstats_req_msg_t* devstats_rsp = (llp_devstats_rsp_msg_t*)(msg_hdr + 1);	
	llp_cmd_slave_stats_t* llp_stats =  (u8 *)(devstats_rsp + 1);
	
	memset(buf, 0x00, MAX_PKT_BUFFSIZE);	

	if((devstats_req->action != LLP_UPDATE_SET) &&
	  (devstats_req->action != LLP_UPDATE_GET))
	{
		return;
	}

	if(devstats_req->action == LLP_UPDATE_SET)
	{
		/* ToDo - Action is not decided yet */
	}
	else if(devstats_req->action == LLP_UPDATE_GET)
	{
		/*Fill message header*/ 
		msg_hdr->fc.control_bits = 0;
		msg_hdr->cmd_id = DEV_STATS_RSP;	
		devstats_rsp->action = devstats_req->action;
		
		memcpy_cpu_to_le((u8*)&(devstats_rsp->mac_addr), 
						(u8*)&(node_data.macaddr), MAC_ADDR_LEN);
		llp_stats->llp_RxCnt = RTOCL(node_data.stats.updtrsp);
		llp_stats->llp_TxCnt = RTOCL(node_data.stats.updtrsp);	
		//llp_stats->dev_type = 0;	
		
#ifdef ROUTE_APP	
		//memcpy_cpu_to_le((u8*)&llp_stats->short_add, (u8*)&route_info.zid, sizeof(route_info.zid));
		memcpy_cpu_to_le((u8*)&llp_stats->rank, (u8*)&route_info.rank, sizeof(route_info.rank));
		
		if(route_info.parent != NULL) 
			llp_stats->link = route_info.parent->link;
		llp_stats->rssi = route_info.parent->rssi;	
		llp_stats->lqi	= route_info.parent->lqi;		
		//llp_stats->totalRx = RTOCL(totalRx);
		//llp_stats->totalTx = RTOCL(totalTx);			
#endif

#ifdef LRWPAN_DRIVER_APP			
		llp_stats->rf_channel =  lrwpan_db.channel;
		memcpy_cpu_to_le((u8*)&llp_stats->rf_panid, (u8*)&lrwpan_db.panid, sizeof(lrwpan_db.panid));
#endif	
#ifdef HPGP_DRIVER_APP
		memcpy((u8*)&llp_stats->plc_nid, (u8*)&hpgp_nwk_data.params.nwk.key.nid[0], NID_LEN);
#endif
		len += sizeof(llp_cmd_slave_stats_t);		
	}

	LlpApp_Tx(buf,(sizeof(llp_msg_req_t) + sizeof(llp_devstats_req_msg_t) + 
			  len + LLP_HDR_OFFSET), 1);
}

/******************************************************************************
 * @fn      LlpApp_RxAppMsg
 *
 * @brief   Receives a message from another app/fw
 *
 * @params  msg_buf - message buffer
 *
 * @return  none
 */

void LlpApp_RxAppMsg(sEvent* event)
{
	gv701x_app_msg_hdr_t* msg_hdr = (gv701x_app_msg_hdr_t*)event->buffDesc.dataptr;
	hostHdr_t* hybrii_hdr;
	hostEventHdr_t* evnt_hdr;

	hybrii_hdr = (hostHdr_t*)(msg_hdr + 1);

	if(msg_hdr->dst_app_id == node_app_id)
	{
		memcpy(&node_state.msg_hdr, msg_hdr, sizeof(gv701x_app_msg_hdr_t));
		node_state.eventproto = hybrii_hdr->protocol;
		if((msg_hdr->src_app_id == APP_FW_MSG_APPID) && 
			(hybrii_hdr->type == EVENT_FRM_ID))
		{
			evnt_hdr = (hostEventHdr_t*)(hybrii_hdr + 1);
			node_state.event = evnt_hdr->type; 	
			node_state.statedata = (u8*)(evnt_hdr + 1);
			node_state.statedatalen = (u16)(hybrii_hdr->length - sizeof(hostEventHdr_t));		
		}
		else
		{
			node_state.event = (u8)(*((u8*)(hybrii_hdr + 1)));
			node_state.statedata = (u8*)(hybrii_hdr + 1);
			node_state.statedatalen = (u16)hybrii_hdr->length;
		}		
		node_state.eventtype = hybrii_hdr->type;
		node_state.eventclass = event->eventHdr.eventClass;

		if((msg_hdr->src_app_id == APP_FW_MSG_APPID) && 
			(hybrii_hdr->type == EVENT_FRM_ID) &&
			(node_state.event == HOST_EVENT_APP_TIMER))
		{			
			LlpApp_TimerHandler((u8*)(evnt_hdr + 1)); 
			return;
		}				
		else if((msg_hdr->src_app_id == APP_FW_MSG_APPID) && 
			(hybrii_hdr->type == EVENT_FRM_ID) &&
			(node_state.event == HOST_EVENT_APP_CMD))
		{			
			LlpApp_CmdProcess((char*)(evnt_hdr + 1)); 
			return;
		}					
	}
	else if(msg_hdr->dst_app_id == APP_BRDCST_MSG_APPID)
	{
		u8 *event = (u8*)(hybrii_hdr + 1);
#ifdef NWKSTARTUP_APP			
		if(msg_hdr->src_app_id == nwkstartup_data.app_id)
		{
			if(msg_hdr->type == APP_MSG_TYPE_APPIND)
			{
				switch(*event)
				{
					case NWK_START_IND:
					{
						nwk_start_ind_msg_t* nwk_start_ind = (nwk_start_ind_msg_t*)event;
#ifdef RTOPO_APP
						{
							rtopo_start_evnt_msg_t rtopo_start;
							rtopo_start.event = RTOPO_START_EVNT;
							rtopo_start.link = nwk_start_ind->link;
							rtopo_start.mode = RTOPO_SCAN_PEER_SELECTION;
							GV701x_SendAppEvent(node_app_id, rtopo_app_id, APP_MSG_TYPE_APPEVENT,
									APP_MAC_ID, EVENT_CLASS_CTRL, MGMT_FRM_ID,			
									&rtopo_start, sizeof(rtopo_start_evnt_msg_t), 0);	
						}				
#else
#ifdef NWKSTARTUP_APP					
						{
							nwk_start_evnt_msg_t nwk_start;
							nwk_start.event = NWK_START_EVENT;
							nwk_start.link = nwk_start_ind->link;
							GV701x_SendAppEvent(node_app_id, nwkstartup_data.app_id, APP_MSG_TYPE_APPEVENT,
									APP_MAC_ID, EVENT_CLASS_CTRL, MGMT_FRM_ID,			
									&nwk_start, sizeof(nwk_start_evnt_msg_t), 0);								
						}
#endif
#endif
					}
					break;

					case NWK_LINKUP_IND:
						node_state.state = NODE_ACTIVE_STATE;
					break;
					
					case NWK_LINKDWN_IND:
					{
						if((nwkstartup_data.link.power_line.state != LINK_UP) &&
							(nwkstartup_data.link.wireless.state != LINK_UP))
						{
							node_state.state = NODE_INACTIVE_STATE;
						}
					}
					break;
					
					default:
					break;
				}	
			}
		}
#endif		
		return;
	}	
	LlpApp_StateMachine(&node_state);			
}

/******************************************************************************
 * @fn      LlpApp_TimerHandler
 *
 * @brief   Timer handler for LLP timer events
 *
 * @param   event - event from firmware
 *
 * @return  none
 *
 */	
void LlpApp_TimerHandler(u8* buf)
{		
	hostTimerEvnt_t* timerevt = 
		(hostTimerEvnt_t*)buf;			

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
 * @fn      LlpApp_StateMachine
 *
 * @brief   The LLP State Machine, it executes all internal/external 
 *			events triggered
 *
 * @param   state - state machine object of the module
 *          (passed as a reference incase there are more than one object)
 *
 * @return  none
 *
 */

void LlpApp_StateMachine(gv701x_state_t* state) 
{
	if(state == NULL)
		return;

#if 1
	if(state->event != NODE_IDLE_EVNT)
		FM_Printf(FM_APP, "\nLLP State %bu Event %bu P %bu C %bu E %bu Da %bu Sa %bu T %bu", 
				state->state, state->event,
				state->eventproto, state->eventclass, state->eventtype, 
				state->msg_hdr.dst_app_id, state->msg_hdr.src_app_id, state->msg_hdr.type);
#endif

	switch(state->state) 
	{
		case NODE_IDLE_STATE:
			if(state->eventproto == APP_MAC_ID)
			{				
				switch(state->event) 
				{		
					case NODE_IDLE_EVNT:									
					break;
					
					case NODE_ACTIVE_EVNT:
						state->state = NODE_ACTIVE_STATE;	
					break;
						
					default:					
					break;
				}
			}
		break;
						
		case NODE_ACTIVE_STATE:
			if(state->eventproto == APP_MAC_ID)
			{								
				switch(state->event) 
				{		
					case NODE_TRIGGER_EVNT:
						LlpApp_SendUpdateRsp(FALSE, LLP_UPDATE_SET, 0);
					break;
				
					case NODE_INACTIVE_EVNT:
						state->state = NODE_INACTIVE_STATE;
					break;
									
					default:
					break;
				}
			}
		break;

		case NODE_INACTIVE_STATE:
			if(state->eventproto == APP_MAC_ID)
			{				
				switch(state->event) 
				{		
					case NODE_ACTIVE_EVNT:
						state->state = NODE_ACTIVE_STATE;
						break;
					default:
						break;
				}
			}
			break;
		
		default:
		break;
	}
	
	state->event = NODE_IDLE_EVNT;
	state->eventtype = 0;
	state->eventclass = 0;
	state->eventproto = 0;
	state->statedata = NULL;	
	state->statedatalen = 0;	
	memset((u8*)&state->msg_hdr, 0x00, sizeof(gv701x_app_msg_hdr_t));
}

/******************************************************************************
 * @fn      LlpApp_DispStats
 *
 * @brief   Displays the frame statistics
 *
 * @param   none
 *
 * @return  none
 */

void LlpApp_DispStats(void)
{
#if 1
	printf("\ndevtype   = %bu", (u8)(*node_data.dev_type));
	printf("\ndevsubtype	= %bu", (u8)(*node_data.dev_subtype));
	printf("\nupdtreq   = %lu",node_data.stats.updtreq);
	printf("\nupdtrsp   = %lu",node_data.stats.updtrsp);
	printf("\nasyncrsp  = %lu", node_data.stats.asyncrsp);	
	printf("\ndrop      = %lu", node_data.stats.drop);
#endif	
}

/******************************************************************************
 * @fn      LlpApp_ResetStats
 *
 * @brief   Reset's the frame statistics
 *
 * @param   none
 *
 * @return  none
 */

void LlpApp_ResetStats(void) 
{
	node_data.stats.updtreq = 0 ;
	node_data.stats.updtrsp = 0;	
	node_data.stats.updtrspbt = 0;
	node_data.stats.drop = 0;	
	node_data.stats.asyncrsp = 0;	
}

/******************************************************************************
 * @fn      LlpApp_CmdProcess
 *
 * @brief   It handles application command line requests
 *
 * @param   CmdBuf - command string
 *
 * @return  none
 *
 */

void LlpApp_CmdProcess(char* CmdBuf) 
{
	if(strcmp(CmdBuf, "state") == 0) 
	{
		printf("\nLLp S %bu E %bu", node_state.state, node_state.event);
	}
	else if(strcmp(CmdBuf, "stats") == 0) 
	{
		LlpApp_DispStats();
	}
	else if(strcmp(CmdBuf, "nvclear") == 0) 
	{
		GV701x_FlashErase(node_app_id);
	}	
}
#endif /*LLP_APP*/
