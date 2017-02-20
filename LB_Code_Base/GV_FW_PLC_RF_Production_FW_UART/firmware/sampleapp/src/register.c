/* ========================================================
 *
 * @file:  register.c
 * 
 * @brief: This file handles the registration process of the 
 *		   node with the Host
 *      
 *  Copyright (C) 2010-2015, Greenvity Communications, Inc.
 *  All Rights Reserved
 *  
 * =========================================================*/

#ifdef REGISTER_APP
/****************************************************************************** 
  *	Includes
  ******************************************************************************/

#include <REG51.H>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "gv701x_includes.h"
#include "register.h"
#include "register_fw.h"
#ifdef ROUTE_APP
#include "route.h"
#include "route_fw.h"
#endif
#ifdef LLP_APP
#include "llpapp.h"
#include "llpapp_fw.h"
#endif
#ifdef HPGP_DRIVER_APP
#include "gv701x_hpgpdriver.h"
#endif
#ifdef LRWPAN_DRIVER_APP
#include "gv701x_lrwpandriver.h"
#endif
#ifdef NWKSTARTUP_APP
#include "gv701x_nwkstartup.h"
#endif

/****************************************************************************** 
  *	Global Data
  ******************************************************************************/
u8 register_app_id;
gv701x_app_queue_t register_queues;	

register_data_t register_data;  	 
gv701x_state_t register_state;	 

/****************************************************************************** 
  *	External Data
  ******************************************************************************/
  
/******************************************************************************
  * Funtion prototypes
  ******************************************************************************/

void register_send_req(u16* cnt);
void register_send_confirm(void);
void register_handle_timeout(void);
u8 register_tx(u8* buf, u8 payloadLen, u8 frametype);

/******************************************************************************
 * @fn      RegisterApp_Init
 *
 * @brief   Initializes the Registration Layer
 *
 * @param   app_id - application identification number
 *
 * @return  none
 */

void RegisterApp_Init(u8 app_id) 
{	
	u8* macaddr;
	memset(&register_state, 0x00, sizeof(gv701x_state_t));
	memset(&register_data, 0x00, sizeof(register_data_t));

	/*Record the applications id,will 
	 be used while allocating timers*/
	register_app_id = app_id;	
	SLIST_Init(&register_queues.appRxQueue);
	
	FM_Printf(FM_USER, "\nInit RegisterApp (app id %bu)", app_id);
	
	memcpy((u8*)&register_data.macaddr, (u8*)macaddr, MAC_ADDR_LEN);
#if 0	
	FM_HexDump(FM_APP, "MAC: ", (u8*)&register_data.macaddr, MAC_ADDR_LEN);			
#endif

#if 0
	/*Fetches previously accquired short address if any*/
	if(STATUS_SUCCESS == GV701x_FlashRead(register_app_id, (u8*)&register_data.nwk_addr, 
						sizeof(register_data.nwk_addr)))
	{		
		if((register_data.nwk_addr.addr_16bit == 0xFFFF) || 
			(register_data.nwk_addr.addr_16bit == 0))
			register_state.state = REGISTER_IDLE_STATE;		
		else
			register_state.state = REGISTER_REGISTERED_STATE;
	}

	if(STATUS_SUCCESS == GV701x_FlashRead(route_app_id, (u8*)(&(route_info.root_ieee_addr[0])), 
						IEEE_MAC_ADDRESS_LEN - 2))
	{		
	}
#else
	register_state.state = REGISTER_IDLE_STATE;	
#endif

	/*Initialize State machine*/
	register_state.event = REGISTER_IDLE_EVNT;	
	register_state.statedata = NULL;
	register_state.statedatalen = 0;
	register_data.reg_req_retry_cnt = 0;

	/*Allocated Registration timer*/
	register_data.register_timer = STM_AllocTimer(SW_LAYER_TYPE_APP, 
						REGISTRATION_TIMER_EVNT,&register_app_id);
}


/******************************************************************************
 * @fn      register_send_req
 *
 * @brief   Sends Registration request to the Host
 *
 * @param   cnt - pointer to the re-transmit counter
 *               (re-transmissions occur at an exponential index)
 *
 * @return  none
 */

void register_send_req(u16* cnt) 
{
	u8 buf[REGISTER_MAX_PKT_BUFFSIZE];
	route_hdr_t* rhdr = &buf[(sizeof(sEth2Hdr))];
	register_req_t* req = (register_req_t*)(rhdr + 1);

	if(register_state.state != REGISTER_UNREGISTERED_STATE)
		return;

	memset(buf, 0x00, REGISTER_MAX_PKT_BUFFSIZE);	

	/*Fill message header*/	
	req->fc.control_bits = 0;
	req->flags |= REG_REQ_BOTH_PRESENT;  	
	req->tei = register_data.tei;
#ifdef HPGP_DRIVER_APP
	memcpy(req->nid, hpgp_nwk_data.params.nwk.key.nid, NID_LEN);
#if 0
	FM_HexDump(FM_APP, "\nReg Nid:", req->nid, NID_LEN);
#endif
#endif
	memcpy_cpu_to_le(&(req->mac_addr), 
			&(register_data.macaddr), MAC_ADDR_LEN);

	/*Start Registration timer*/ 
	if(STATUS_SUCCESS == STM_StartTimer(register_data.register_timer, 
							register_data.registration_time)) 
	{						
		if(STATUS_SUCCESS == register_tx(buf, 
			(sizeof (route_hdr_t) + sizeof(register_req_t) + (sizeof(sEth2Hdr))), REGISTRATION_REQ))
		{			
			register_data.stats.regreq++;
			if((*cnt) < REGISTER_TIMEOUT_MAX_EXPONENT)
			{
				register_data.registration_time = ( (register_data.registration_time)*
													(REGISTER_TIMEOUT_EXPONENT) );
			} 			
			*cnt = *cnt + 1;			
		}
	}
}

/******************************************************************************
 * @fn      register_send_confirm
 *
 * @brief   Sends Registration confirm to the Host
 *
 * @param   cnt - pointer to the re-transmit counter
 *               (re-transmissions occur at an exponential index)
 *
 * @return  none
 */

void register_send_confirm(void) 
{
	u8 buf[REGISTER_MAX_PKT_BUFFSIZE];	
	register_up_ind_msg_t register_up;	
	route_updt_addr_evnt_msg_t route_update;
	route_hdr_t* rhdr = &buf[(sizeof(sEth2Hdr))];
	register_cnf_t* cnf = (register_cnf_t*)(rhdr + 1);	

	memset(buf, 0x00, REGISTER_MAX_PKT_BUFFSIZE);

 	/*Fill message header*/	
	cnf->fc.control_bits = 0;
	cnf->status = TRUE;	
#ifdef LLP_APP
	cnf->dev_type = (u8)(*node_data.dev_type);	
	cnf->sub_type = (u8)(*node_data.dev_subtype);
#endif
	cnf->nwkaddr = cpu_to_le16(register_data.nwk_addr.addr_16bit);
	
	memcpy_cpu_to_le(&(cnf->mac_addr), 
			&(register_data.macaddr), MAC_ADDR_LEN); 

#if 1
	FM_Printf(FM_APP, "\nRCnf Tx  (s - 0x%02x, d - 0x%02x, l - %bu", 
				le16_to_cpu(rhdr->target), 
				le16_to_cpu(rhdr->parent), 
				(route_info.parent != NULL)? route_info.parent->link:0);
#endif
	register_data.stats.regcnf++;
	if(FALSE == register_tx(buf, (sizeof (route_hdr_t) + sizeof(register_cnf_t) +  (sizeof(sEth2Hdr))), 
					REGISTRATION_CNF))
	{
	}

	register_state.state = REGISTER_REGISTERED_STATE;

#ifdef ROUTE_APP
	route_info.unreachable = TRUE;
#endif
	/*Trigger Route Acquired event to the Routing layer*/
	register_up.event = REGISTER_UP_IND;
	register_up.nwk_addr = register_data.nwk_addr.addr_16bit;
	GV701x_SendAppEvent(register_app_id, APP_BRDCST_MSG_APPID, APP_MSG_TYPE_APPIND,
		APP_MAC_ID, EVENT_CLASS_CTRL, MGMT_FRM_ID,
		      &register_up, sizeof(register_up_ind_msg_t), 0);	
	
	route_update.event = ROUTE_UPDATE_ADDR_EVNT;
	route_update.addr = register_data.nwk_addr.addr_16bit;
	GV701x_SendAppEvent(register_app_id, route_app_id, APP_MSG_TYPE_APPEVENT,
		APP_MAC_ID, EVENT_CLASS_CTRL, MGMT_FRM_ID,
		      &route_update, sizeof(route_updt_addr_evnt_msg_t), 0);	
}

/******************************************************************************
 * @fn      register_rx_rsp
 *
 * @brief   Handles Registration response received from the Host
 *
 * @param   frm - the response frame
 * 			len - length in bytes
 *
 * @return  none
 */

void register_rx_rsp(u8* frm, u8 len) 
{
	route_hdr_t* rhdr = (route_hdr_t*)&frm[(sizeof(sEth2Hdr))];
	register_rsp_t* rsp = (register_rsp_t*)(rhdr + 1);
	len = len;	
	
	if(memcmp_cpu_to_le((u8*)&(rsp->mac_addr), (u8*)&(register_data.macaddr), MAC_ADDR_LEN))
		return;

	STM_StopTimer(register_data.register_timer);	
#ifdef ROUTE_APP
	{
		sEth2Hdr* petherhdr = (sEth2Hdr*)frm;
		memcpy(&(route_info.root_ieee_addr[0]), &(petherhdr->srcaddr), IEEE_MAC_ADDRESS_LEN-2);		
	}
#endif	

	register_data.stats.regrsp++;
	register_data.nwk_addr.addr_16bit = cpu_to_le16(rsp->nwkaddr);
	nwkstartup_data.link.power_line.addr = register_data.nwk_addr.addr_16bit;
	nwkstartup_data.link.wireless.addr = register_data.nwk_addr.addr_16bit;
#ifdef LRWPAN_DRIVER_APP	
	lrwpan_set_shortaddr(register_data.nwk_addr.addr_16bit);
#endif			
#if 1	
	FM_Printf(FM_APP, "\nRRsp Rx  (s - 0x%02x, d - 0x%02x, l - %bu", 
				le16_to_cpu(rhdr->parent), 
				le16_to_cpu(rhdr->target), 
				(route_info.parent != NULL)? route_info.parent->link:0);
#endif

	register_send_confirm();

#if 0
	/*Write the newly acquired short address in flash*/
	if(STATUS_FAILURE == GV701x_FlashWrite(register_app_id, (u8*)&register_data.nwk_addr, 
						  				sizeof(register_data.nwk_addr)))
	{
	}

#ifdef ROUTE_APP
	if(STATUS_FAILURE == GV701x_FlashWrite(route_app_id, (u8*)&(route_info.root_ieee_addr[0]), 
						  				IEEE_MAC_ADDRESS_LEN-2))
	{
	}
	else
	{
	}	
#endif
#endif
}

/******************************************************************************
 * @fn      register_rx_rereq
 *
 * @brief   Handles Re-Registration request received from the Host
 *
 * @param   frm - the response frame
 * 			len - length in bytes
 *
 * @return  none
 */

void register_rx_rereq(u8* frm, u8 len) 
{
	u8 brdcst_macaddr[MAC_ADDR_LEN] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
	route_hdr_t* rhdr = (route_hdr_t*)&frm[(sizeof(sEth2Hdr))];
	reregister_req_t* rereg = (reregister_req_t*)(rhdr + 1);
	len = len;

	if(register_state.state == REGISTER_IDLE_STATE)
		return;

	if((!memcmp_cpu_to_le(&(rereg->mac_addr), 
				&(register_data.macaddr), MAC_ADDR_LEN)) ||
		(!memcmp_cpu_to_le(&(rereg->mac_addr), 
				brdcst_macaddr, MAC_ADDR_LEN))) 
	{

		register_data.stats.reregreq++;		

		/*Trigger Re-registraion event*/
		register_state.state = REGISTER_IDLE_STATE;
		register_state.event = REGISTER_START_EVNT;		
	}
}

/******************************************************************************
 * @fn      RegisterApp_RxAppMsg
 *
 * @brief   Receives a message from another app/fw
 *
 * @params  msg_buf - message buffer
 *
 * @return  none
 */

void RegisterApp_RxAppMsg(sEvent* event)
{
	gv701x_app_msg_hdr_t* msg_hdr = (gv701x_app_msg_hdr_t*)event->buffDesc.dataptr;
	hostHdr_t* hybrii_hdr;
	hostEventHdr_t* evnt_hdr;
	
	hybrii_hdr = (hostHdr_t*)(msg_hdr + 1);

	if(msg_hdr->dst_app_id == register_app_id)
	{
		memcpy(&register_state.msg_hdr, msg_hdr, sizeof(gv701x_app_msg_hdr_t));
		register_state.eventproto = hybrii_hdr->protocol;
		if((msg_hdr->src_app_id == APP_FW_MSG_APPID) && 
			(hybrii_hdr->type == EVENT_FRM_ID))
		{
			evnt_hdr = (hostEventHdr_t*)(hybrii_hdr + 1);
			register_state.event = evnt_hdr->type; 	
			register_state.statedata = (u8*)(evnt_hdr + 1);
			register_state.statedatalen = (u16)(hybrii_hdr->length - sizeof(hostEventHdr_t));		
		}
		else
		{
			register_state.event = (u8)(*((u8*)(hybrii_hdr + 1)));
			register_state.statedata = (u8*)(hybrii_hdr + 1);
			register_state.statedatalen = (u16)hybrii_hdr->length;
		}		
		register_state.eventtype = hybrii_hdr->type;
		register_state.eventclass = event->eventHdr.eventClass;

		if((msg_hdr->src_app_id == APP_FW_MSG_APPID) && 
			(hybrii_hdr->type == EVENT_FRM_ID) &&
			(register_state.event == HOST_EVENT_APP_TIMER))
		{			
			RegisterApp_TimerHandler((u8*)(evnt_hdr + 1)); 
			return;
		}			
		else if((msg_hdr->src_app_id == APP_FW_MSG_APPID) && 
			(hybrii_hdr->type == EVENT_FRM_ID) &&
			(register_state.event == HOST_EVENT_APP_CMD))
		{			
			RegisterApp_CmdProcess((char*)(evnt_hdr + 1)); 
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
				switch((u8)*event)
				{
					case NWK_LINKUP_IND:					
					{
						register_start_evnt_msg_t register_start;
						register_start.event = REGISTER_START_EVNT;
						GV701x_SendAppEvent(register_app_id, register_app_id, APP_MSG_TYPE_APPEVENT, APP_MAC_ID, EVENT_CLASS_CTRL,
											MGMT_FRM_ID, &register_start, sizeof(register_start_evnt_msg_t), 0);						
					}
	 				break;
					
					case NWK_LINKDWN_IND:
					{
						nwk_dwn_ind_msg_t* nwk_dwn = (nwk_dwn_ind_msg_t*)event;					
						if((nwk_dwn->link & PLC_NIC) || (nwk_dwn->link & RF_NIC)) 	
						{					
							if(route_info.parent == NULL)
							{
								register_stop_evnt_msg_t register_stop;
								register_start_evnt_msg_t register_start;								
								
								register_stop.event = REGISTER_STOP_EVNT;						
								GV701x_SendAppEvent(register_app_id, register_app_id, APP_MSG_TYPE_APPEVENT, APP_MAC_ID, EVENT_CLASS_CTRL,
													MGMT_FRM_ID, &register_stop, sizeof(register_stop_evnt_msg_t), 0);						

								register_start.event = REGISTER_START_EVNT;
								GV701x_SendAppEvent(register_app_id, register_app_id, APP_MSG_TYPE_APPEVENT, APP_MAC_ID, EVENT_CLASS_CTRL,
													MGMT_FRM_ID, &register_start, sizeof(register_start_evnt_msg_t), 0);						
								
							}
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
	RegisterApp_StateMachine(&register_state);	
}

/******************************************************************************
 * @fn      RegisterApp_Start
 *
 * @brief   Register the node
 *
 * @params  none
 *
 * @return  none
 */

void RegisterApp_Start(void)
{
#if	0 	
	register_data.nwk_addr.addr_16bit = 0xFFFF;
	if(STATUS_SUCCESS == GV701x_FlashWrite(register_app_id, (u8*)&register_data.nwk_addr, 
						sizeof(register_data.nwk_addr)))
#else
	
#endif
	{
#ifdef HPGP_DRIVER_APP					
		register_data.tei = hpgp_nwk_data.params.nwk.tei;
#endif
#ifdef NWKSTARTUP_APP					
		memcpy(&(register_data.macaddr), (u8*)nwkstartup_data.link.long_addr.mac_addr, 
					MAC_ADDR_LEN);
#endif
		register_state.state = REGISTER_UNREGISTERED_STATE;
		register_data.registration_cnt = 0;
		register_data.registration_time = REGISTER_BASE_TIMEOUT;
		register_send_req(&register_data.registration_cnt);
	}
}

/******************************************************************************
 * @fn      RegisterApp_TimerHandler
 *
 * @brief   Timer handler for Registration timer events
 *
 * @param   event - event from firmware
 *
 * @return  none
 *
 */

void RegisterApp_TimerHandler(u8* buf)
{	
	hostTimerEvnt_t* timerevt = 
			(hostTimerEvnt_t*)buf;	
	
	if(buf == NULL)
		return;

	/*Demultiplexing the specific timer event*/ 					
	switch((u8)timerevt->type)
	{								
		case REGISTRATION_TIMER_EVNT: 
			register_handle_timeout();
		break;						
		
		default:
		break;
	}			
}

/******************************************************************************
 * @fn      RegisterApp_StateMachine
 *
 * @brief   The Registration State Machine, it executes all internal/external 
 *			events triggered
 *
 * @param   state - state machine object of the driver
 *          (passed as a reference incase there are more than one object)
 *
 * @return  none
 *
 */

void RegisterApp_StateMachine(gv701x_state_t* state) 
{
	if(state == NULL)
		return;
#if 1
	if(state->event != REGISTER_IDLE_EVNT)
		FM_Printf(FM_APP, "\nRegister S %bu E %bu P %bu C %bu E %bu Da %bu Sa %bu T %bu", 
				state->state, state->event,
				state->eventproto, state->eventclass, state->eventtype, 
				state->msg_hdr.dst_app_id, state->msg_hdr.src_app_id, state->msg_hdr.type);
#endif

	switch(state->state) 
	{	
		case REGISTER_IDLE_STATE:
			if(state->eventproto == APP_MAC_ID)
			{
				if(state->msg_hdr.type == APP_MSG_TYPE_APPEVENT)
				{								
					switch(state->event) 
					{		
						case REGISTER_IDLE_EVNT:									
						break;

						case REGISTER_START_EVNT:
							RegisterApp_Start();
						break;		

						default:
						break;				
					}
				}
			}
		break;
				
		case REGISTER_UNREGISTERED_STATE:
			if(state->eventproto == APP_MAC_ID)
			{
				if(state->msg_hdr.type == APP_MSG_TYPE_APPEVENT)
				{											
					switch(state->event) 
					{					
						case REGISTER_STOP_EVNT:
							register_data.registration_cnt = 0;
							register_data.registration_time = REGISTER_BASE_TIMEOUT;							
							STM_StopTimer(register_data.register_timer);
							state->state = REGISTER_IDLE_STATE;
						break;


						case REGISTER_START_EVNT:
							RegisterApp_Start();
						break;	

						default:
						break;					
					}
				}
			}
		break;
					
		case REGISTER_REGISTERED_STATE: 
			if(state->eventproto == APP_MAC_ID)
			{
				if(state->msg_hdr.type == APP_MSG_TYPE_APPEVENT)
				{											
					switch(state->event) 
					{
						case REGISTER_STOP_EVNT:
							register_data.registration_cnt = 0;
							register_data.registration_time = REGISTER_BASE_TIMEOUT;							
							STM_StopTimer(register_data.register_timer);
							state->state = REGISTER_IDLE_STATE;
						break;

						case REGISTER_START_EVNT:
							nwkstartup_data.link.power_line.addr = 
												register_data.nwk_addr.addr_16bit;
							nwkstartup_data.link.wireless.addr = 		
											register_data.nwk_addr.addr_16bit;
#ifdef LRWPAN_DRIVER_APP	
							lrwpan_set_shortaddr(register_data.nwk_addr.addr_16bit);
#endif							

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

	
	state->event = REGISTER_IDLE_EVNT;
	state->eventtype = 0;
	state->eventclass = 0;
	state->eventproto = 0;
	state->statedata = NULL;	
	state->statedatalen = 0;	
	memset((u8*)&state->msg_hdr, 0x00, sizeof(gv701x_app_msg_hdr_t));
}

/******************************************************************************
 * @fn      register_handle_timeout
 *
 * @brief   Handles registration timeout
 *
 * @param   none
 *
 * @return  none
 *
 */

void register_handle_timeout(void)
{
	if(register_data.registration_cnt >= MAX_REGISTER_RETRY_COUNT)
	{	
		route_stop_evnt_msg_t route_stop;		
#ifdef NWKSTARTUP_APP		
		nwk_stop_evnt_msg_t nwk_stop;		
#endif		
		register_data.registration_cnt = 0;
		register_data.registration_time = REGISTER_BASE_TIMEOUT;	
		register_state.state = REGISTER_IDLE_STATE;
		register_state.event = REGISTER_IDLE_EVNT;

#ifdef ROUTE_APP						
		route_stop.event = ROUTE_STOP_EVNT;
		route_stop.link = route_info.parent->link;
		GV701x_SendAppEvent(register_app_id, route_app_id, APP_MSG_TYPE_APPEVENT, APP_MAC_ID, EVENT_CLASS_CTRL,
							MGMT_FRM_ID, &route_stop, sizeof(route_stop_evnt_msg_t), 0);							
#endif
#ifdef NWKSTARTUP_APP
		nwk_stop.event = NWK_STOP_EVENT;
		nwk_stop.link = route_info.parent->link;
		
		GV701x_SendAppEvent(register_app_id, nwkstartup_data.app_id, APP_MSG_TYPE_APPEVENT, APP_MAC_ID, EVENT_CLASS_CTRL,
							MGMT_FRM_ID, &nwk_stop, sizeof(nwk_stop_evnt_msg_t), 0);							
#endif		
	}
	else
	{
		/*Trigger Registration request message on registration timeout*/
		register_send_req(&register_data.registration_cnt);
	}
}

/******************************************************************************
 * @fn      register_tx
 *
 * @brief   Used to send an OTA frame to the Host
 *
 * @param   none
 *
 * @return  none
 *
 */

u8 register_tx(u8* buf, u8 payloadLen, u8 frametype)
{
	u8 ret = STATUS_FAILURE;
	u8 brdcstmac[MAC_ADDR_LEN] = {0xff,0xff,0xff,0xff,0xff,0xff};

#ifdef ROUTE_APP		
	ret = route_send_to_ll((u8*)buf, payloadLen, frametype, FALSE);

	if(ret == FALSE)
		ret = STATUS_FAILURE;
	else
		ret = STATUS_SUCCESS;		
#endif
	return ret;
}

/******************************************************************************
 * @fn      RegisterApp_CmdProcess
 *
 * @brief   It handles application command line requests
 *
 * @param   CmdBuf - command string
 *
 * @return  none
 *
 */

void RegisterApp_CmdProcess(char* CmdBuf) 
{
	if(strcmp(CmdBuf, "state") == 0) 
	{
		printf("\nRegister S %bu E %bu", register_state.state, register_state.event);			  		
	}
	else if(strcmp(CmdBuf, "stats") == 0) 
	{
		printf("\nRegister addr %x \nregreq %u regrsp %u regcnf %u reregreq %u",
			   register_data.nwk_addr.addr_16bit, register_data.stats.regreq,
			   register_data.stats.regrsp, register_data.stats.regcnf,
			   register_data.stats.reregreq);		
	}
	else if(strcmp(CmdBuf, "nvclear") == 0) 
	{
		GV701x_FlashErase(register_app_id);
	}	
}

#endif /*REGISTER_APP*/
