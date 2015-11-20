#ifdef NWKSTARTUP_APP

/****************************************************************************** 
  *	Includes
  ******************************************************************************/
  
#include <string.h>
#include <stdio.h>
#include "gv701x_includes.h"
#include "gv701x_nwkstartup.h"
#ifdef HPGP_DRIVER_APP
#include "gv701x_hpgpdriver.h"
#endif
#ifdef LRWPAN_DRIVER_APP
#include "gv701x_lrwpandriver.h"
#endif

/****************************************************************************** 
  *	Global Data
  ******************************************************************************/
/*Driver Data*/
nwkstartup_data_t nwkstartup_data;

/*Driver State Machines*/
gv701x_state_t nwkstartup_state;
	
/****************************************************************************** 
  *	External Data
  ******************************************************************************/

/******************************************************************************
  *	Funtion prototypes
  ******************************************************************************/
void GV701x_NwkSendEvent(u8 ind, u8 link);

/******************************************************************************
  *	External Funtion prototypes
  ******************************************************************************/

/******************************************************************************
 * @fn      GV701x_NwkInit
 *
 * @brief   Initializes the driver
 *
 * @param   app_id - application identification number
 *
 * @return  none
 */

void GV701x_NwkInit(u8 app_id)
{	
	u8* macaddr;
	/*Initialize the database*/
	memset(&nwkstartup_data, 0x00, sizeof(nwkstartup_data_t));
	memset(&nwkstartup_state, 0x00, sizeof(gv701x_state_t));	

	macaddr = GV701x_ReadMacAddress();
	/*Initialize the State Machines*/
	nwkstartup_data.app_id = app_id;	
	SLIST_Init(&nwkstartup_data.queues.appRxQueue);
	
	FM_Printf(FM_USER, "\nInit NwkApp (app id %bu)", app_id);
	
	memcpy((u8*)nwkstartup_data.link.long_addr.mac_addr, (u8*)macaddr, MAC_ADDR_LEN);
#if 0	
	FM_HexDump(FM_APP, "MAC: ", (u8*)nwkstartup_data.link.long_addr.mac_addr, MAC_ADDR_LEN);			
#endif

	nwkstartup_state.state = NWK_INIT;
	nwkstartup_state.event = NWK_IDLE_EVENT;	
	nwkstartup_state.statedata = NULL;		
	nwkstartup_state.statedatalen = 0;		

	/*Initial state of the links*/
	nwkstartup_data.link.fw_ready.wireless = FALSE;
	nwkstartup_data.link.fw_ready.power_line = FALSE;	
	nwkstartup_data.link.power_line.state = LINK_DOWN;
#ifdef BRIDGE	
	nwkstartup_data.link.wireless.state = LINK_DOWN;
#endif	

#ifdef HPGP_DRIVER_APP
	nwkstartup_data.link.power_line.addr = hpgp_nwk_data.params.nwk.tei;	
#endif
#ifdef LRWPAN_DRIVER_APP
	nwkstartup_data.link.wireless.addr = lrwpan_db.short_addr;	
#endif
}

/******************************************************************************
 * @fn      GV701x_NwkInit
 *
 * @brief   Starts the network profile
 *
 * @param   link - The link to start with (PLC_NIC - PLC link or 
 *                 RF_NIC - Wireless link of bitmap of both)(defines found in nma.h)
 *
 * @return  none
 */

void GV701x_NwkStart(u8 link)
{	
	/*Start PLC link*/
	if((nwkstartup_data.link.power_line.state != LINK_DISABLE) &&
	    (nwkstartup_data.link.fw_ready.power_line == TRUE) &&
		(link & PLC_NIC))
	{	
#ifdef HPGP_DRIVER_APP		
		hpgp_drv_start_evnt_msg_t hpgp_start;	
		nwkstartup_state.state = NWK_START;
		nwkstartup_data.link.power_line.state = LINK_DOWN;
		hpgp_start.event = HPGPDRV_START_EVNT;
		GV701x_SendAppEvent(nwkstartup_data.app_id, hpgp_drv_data.app_id, APP_MSG_TYPE_APPEVENT, 
			APP_MAC_ID, EVENT_CLASS_CTRL, MGMT_FRM_ID, &hpgp_start, 
			sizeof(hpgp_drv_start_evnt_msg_t), 0);
#endif
	}

	/*Start Wireless link*/	
	if((nwkstartup_data.link.wireless.state != LINK_DISABLE) &&
	   (nwkstartup_data.link.fw_ready.wireless == TRUE) &&
	   (link & RF_NIC))
	{
#ifdef LRWPAN_DRIVER_APP
		lrwpan_start_evnt_msg_t lrwpan_start;
		nwkstartup_state.state = NWK_START;
		nwkstartup_data.link.wireless.state = LINK_DOWN;
		
		/*Start LRWPAN Driver */
		lrwpan_start.event = LRWPAN_START_EVNT;
		lrwpan_db.short_addr = nwkstartup_data.link.wireless.addr;	
		GV701x_SendAppEvent(nwkstartup_data.app_id, lrwpan_db.app_id, APP_MSG_TYPE_APPEVENT, 
			APP_MAC_ID, EVENT_CLASS_CTRL, MGMT_FRM_ID,&lrwpan_start, 
			sizeof(lrwpan_start_evnt_msg_t), 0);
#endif		
	}	
}

/******************************************************************************
 * @fn      GV701x_NwkStop
 *
 * @brief   Starts the network profile
 *
 * @param   link - The link to start with (PLC_NIC - PLC link or 
 *                 RF_NIC - Wireless link of bitmap of both)(defines found in nma.h)
 *
 * @return  none
 */

void GV701x_NwkStop(u8 link)
{	
	/*Stop PLC link*/
	if((nwkstartup_data.link.power_line.state != LINK_DISABLE) &&
	    (nwkstartup_data.link.fw_ready.power_line == TRUE) &&
		(link & PLC_NIC))
	{	
#ifdef HPGP_DRIVER_APP		
		hpgp_drv_stop_evnt_msg_t hpgp_stop;	
		hpgp_stop.event = HPGPDRV_STOP_EVNT;
		GV701x_SendAppEvent(nwkstartup_data.app_id, hpgp_drv_data.app_id, APP_MSG_TYPE_APPEVENT,
			APP_MAC_ID, EVENT_CLASS_CTRL, MGMT_FRM_ID, &hpgp_stop, 
			sizeof(hpgp_drv_stop_evnt_msg_t), 0);
#endif
	}
	
	/*Stop Wireless link*/	
	if((nwkstartup_data.link.wireless.state != LINK_DISABLE) &&
	   (nwkstartup_data.link.fw_ready.wireless == TRUE) &&
	   (link & RF_NIC))
	{
#ifdef LRWPAN_DRIVER_APP
		lrwpan_stop_evnt_msg_t lrwpan_stop;
		lrwpan_stop.event = LRWPAN_STOP_EVNT;
		GV701x_SendAppEvent(nwkstartup_data.app_id, lrwpan_db.app_id, APP_MSG_TYPE_APPEVENT,
			APP_MAC_ID, EVENT_CLASS_CTRL, MGMT_FRM_ID,&lrwpan_stop, 
			sizeof(lrwpan_stop_evnt_msg_t), 0);
#endif		
	}	
}

/******************************************************************************
 * @fn      GV701x_NwkRxAppMsg
 *
 * @brief   Receives a message from another app/fw
 *
 * @params  msg_buf - message buffer
 *
 * @return  none
 */

void GV701x_NwkRxAppMsg(sEvent* event)
{
	gv701x_app_msg_hdr_t* msg_hdr = (gv701x_app_msg_hdr_t*)event->buffDesc.dataptr;
	hostHdr_t* hybrii_hdr;
	hostEventHdr_t* evnt_hdr;

	hybrii_hdr = (hostHdr_t*)(msg_hdr + 1);
		
	if((msg_hdr->dst_app_id == nwkstartup_data.app_id) ||
		(msg_hdr->dst_app_id == APP_BRDCST_MSG_APPID))
	{	
		memcpy(&nwkstartup_state.msg_hdr, msg_hdr, sizeof(gv701x_app_msg_hdr_t));
		nwkstartup_state.eventproto = hybrii_hdr->protocol;
		if((msg_hdr->src_app_id == APP_FW_MSG_APPID) && 
			(hybrii_hdr->type == EVENT_FRM_ID))
		{
			evnt_hdr = (hostEventHdr_t*)(hybrii_hdr + 1);
			nwkstartup_state.event = evnt_hdr->type; 		
			nwkstartup_state.statedata = (u8*)(evnt_hdr + 1);
			nwkstartup_state.statedatalen = (u16)(hybrii_hdr->length - sizeof(hostEventHdr_t));			
		}
		else
		{
			nwkstartup_state.event = (u8)(*((u8*)(hybrii_hdr + 1)));
			nwkstartup_state.statedata = (u8*)(hybrii_hdr + 1);
			nwkstartup_state.statedatalen = (u16)hybrii_hdr->length;
		}		
		nwkstartup_state.eventtype = hybrii_hdr->type;
		nwkstartup_state.eventclass = event->eventHdr.eventClass;	
		if((msg_hdr->src_app_id == APP_FW_MSG_APPID) && 
			(hybrii_hdr->type == EVENT_FRM_ID) &&
			(nwkstartup_state.event == HOST_EVENT_APP_TIMER))
		{			
			return;
		}
		else if((msg_hdr->src_app_id == APP_FW_MSG_APPID) && 
			(hybrii_hdr->type == EVENT_FRM_ID) &&
			(nwkstartup_state.event == HOST_EVENT_APP_CMD))
		{
			GV701x_Nwk_CmdProcess((char*)(evnt_hdr + 1));	
			return;
		}				
	}
	GV701x_NwkStartupSM(&nwkstartup_state);
}

/******************************************************************************
 * @fn      GV701x_NwkStartupSM
 *
 * @brief   The NetworkStartup State Machine, it executes all internal/external 
 *			events triggered
 *
 * @param   state - state machine object of the driver
 *          (passed as a reference incase there are more than one object)
 *
 * @return  none
 *
 */

void GV701x_NwkStartupSM(gv701x_state_t* state) 
{
	if(state == NULL)
		return;

#if 1
	if(state->event != NWK_IDLE_EVENT)
		FM_Printf(FM_APP, "\nNwk S %bu E %bu P %bu C %bu E %bu Da %bu Sa %bu T %bu", 
				state->state, state->event,
				state->eventproto, state->eventclass, state->eventtype, 
				state->msg_hdr.dst_app_id, state->msg_hdr.src_app_id, state->msg_hdr.type);
#endif

	switch(state->state)
	{
		case NWK_INIT:			
			if(state->eventproto == SYS_MAC_ID)
			{
				if((state->eventclass == EVENT_CLASS_CTRL) && 
				   (state->eventtype == EVENT_FRM_ID))				
				{
					switch(state->event)
					{
						/*Firmware ready event*/
						case HOST_EVENT_FW_READY:
						{										
							if(state->statedata)
							{
								u8* link = (u8*)state->statedata;
								u8 allowedlink = 0;
								
								/*Check if PLC or Wireless firmware is present*/
								if(((*link & PLC_NIC) ? ((nwkstartup_data.link.power_line.state != LINK_DISABLE) ? (1):(0)): (0)) && 
									((*link & RF_NIC) ? ((nwkstartup_data.link.wireless.state != LINK_DISABLE) ? (1):(0)): (0)) )
								{							
									nwkstartup_data.link.fw_ready.power_line = TRUE;
									nwkstartup_data.link.fw_ready.wireless = TRUE;	
									allowedlink |= PLC_NIC;
									allowedlink |= RF_NIC;
								}
								else if((*link & PLC_NIC) ? ((nwkstartup_data.link.power_line.state != LINK_DISABLE) ? (1):(0)): (0))
								{						
									nwkstartup_data.link.fw_ready.power_line = TRUE;
									allowedlink |= PLC_NIC;
								}
								else if((*link & RF_NIC) ? ((nwkstartup_data.link.wireless.state != LINK_DISABLE) ? (1):(0)): (0))
								{
									nwkstartup_data.link.fw_ready.wireless = TRUE;
									allowedlink |= RF_NIC;									
								}	
								
								if(allowedlink != 0)
									GV701x_NwkSendEvent(NWK_START_IND, allowedlink);								
							}			
						}
						break;

						default:
						break;
					}
				}			
				else if((state->eventclass == EVENT_CLASS_MGMT) && 
				   (state->eventtype == MGMT_FRM_ID))
				{					
					switch(state->event)
					{							
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
						/*Start event*/
						case NWK_START_EVENT:
						{
							nwk_start_evnt_msg_t* nwk_start = (nwk_start_evnt_msg_t*)state->statedata;
							GV701x_NwkStart(nwk_start->link);
						}
						break;

						default:
						break;
					}
				}
			}
		break;
		
		case NWK_START:
		case NWK_DOWN:
			if(state->eventproto == APP_MAC_ID)
			{		
#ifdef HPGP_DRIVER_APP			
				if(state->msg_hdr.src_app_id == hpgp_drv_data.app_id)
				{
					if(state->msg_hdr.type == APP_MSG_TYPE_APPIND)
					{				
						switch(state->event)
						{
							case HPGPDRV_DWN_IND:	
							{
								sEvent* event = NULL;
								state->state = NWK_DOWN;
								GV701x_NwkSendEvent(NWK_LINKDWN_IND, PLC_NIC);
							}
							break;
						
							case HPGPDRV_UP_IND:
								if(nwkstartup_data.link.power_line.state != LINK_DISABLE)					
									nwkstartup_data.link.power_line.state = LINK_UP;

								state->state = NWK_UP;
								GV701x_NwkSendEvent(NWK_LINKUP_IND, PLC_NIC);
							break;									
						
							default:
							break;
						}
					}
				}
#endif				
#ifdef LRWPAN_DRIVER_APP
				else if(state->msg_hdr.src_app_id == lrwpan_db.app_id)
				{
					if(state->msg_hdr.type == APP_MSG_TYPE_APPIND)
					{				
						switch(state->event)
						{
							case LRWPAN_DWN_IND:
								state->state = NWK_DOWN;
								GV701x_NwkSendEvent(NWK_LINKDWN_IND, RF_NIC);
							break;
							
							case LRWPAN_UP_IND:
								if(nwkstartup_data.link.wireless.state != LINK_DISABLE)
									nwkstartup_data.link.wireless.state = LINK_UP;
																
								state->state = NWK_UP;					
								GV701x_NwkSendEvent(NWK_LINKUP_IND, RF_NIC);						
							break;	
						
							default:
							break;
						}
					}
				}
#endif				
				else
				{
					if(state->msg_hdr.dst_app_id != APP_BRDCST_MSG_APPID) 				
					{
						if(state->msg_hdr.type == APP_MSG_TYPE_APPEVENT)
						{
							switch(state->event)
							{			
								/*Start event*/
								case NWK_START_EVENT:						
								{
									nwk_start_evnt_msg_t* nwk_start = (nwk_start_evnt_msg_t*)state->statedata;
									GV701x_NwkStart(nwk_start->link);
								}
								break;

								case NWK_STOP_EVENT:
								{
									nwk_stop_evnt_msg_t* nwk_stop = (nwk_stop_evnt_msg_t*)state->statedata;								
									GV701x_NwkStop(nwk_stop->link);
								}
								break;

								default:
								break;
							}
						}
					}
				}
			}					
		break;		

		case NWK_UP:
			if(state->eventproto == APP_MAC_ID)
			{		
#ifdef HPGP_DRIVER_APP			
				if(state->msg_hdr.src_app_id == hpgp_drv_data.app_id)
				{
					if(state->msg_hdr.type == APP_MSG_TYPE_APPIND)
					{				
						switch(state->event)
						{						
							case HPGPDRV_DWN_IND:
								if(nwkstartup_data.link.power_line.state != LINK_DISABLE)					
									nwkstartup_data.link.power_line.state = LINK_DOWN;					

								/*Network is down if both the links are down*/
								if((nwkstartup_data.link.wireless.state == LINK_DISABLE) ? (1) :
										(nwkstartup_data.link.wireless.state == LINK_DOWN))
								{				
									state->state = NWK_DOWN;
									GV701x_NwkSendEvent(NWK_LINKDWN_IND, (PLC_NIC | RF_NIC));							
								}	
								else
									GV701x_NwkSendEvent(NWK_LINKDWN_IND, PLC_NIC);
							break;
						
							case HPGPDRV_UP_IND:	
								if(nwkstartup_data.link.power_line.state != LINK_DISABLE)										
									nwkstartup_data.link.power_line.state = LINK_UP;	

								GV701x_NwkSendEvent(NWK_LINKUP_IND, PLC_NIC);
							break;					

							default:
							break;
						}
					}
				}
#endif			
#ifdef LRWPAN_DRIVER_APP
				else if(state->msg_hdr.src_app_id == lrwpan_db.app_id) 
				{		
					if(state->msg_hdr.type == APP_MSG_TYPE_APPIND)
					{				
						switch(state->event)
						{			
							case LRWPAN_DWN_IND:
								if(nwkstartup_data.link.wireless.state != LINK_DISABLE)
									nwkstartup_data.link.wireless.state = LINK_DOWN;

								/*Network is down if both the links are down*/										
								if((nwkstartup_data.link.power_line.state == LINK_DISABLE) ? (1) :
										(nwkstartup_data.link.power_line.state == LINK_DOWN))
								{
									state->state = NWK_DOWN;
									GV701x_NwkSendEvent(NWK_LINKDWN_IND, (PLC_NIC | RF_NIC));
								}
								else
									GV701x_NwkSendEvent(NWK_LINKDWN_IND, RF_NIC);
							break;
											
							case LRWPAN_UP_IND: 		
								if(nwkstartup_data.link.wireless.state != LINK_DISABLE) 				
									nwkstartup_data.link.wireless.state = LINK_UP;

								GV701x_NwkSendEvent(NWK_LINKUP_IND, RF_NIC);
							break;	
											
							default:
							break;				
						}
					}
				}
#endif						
				else
				{
					if(state->msg_hdr.dst_app_id != APP_BRDCST_MSG_APPID) 
					{
						if(state->msg_hdr.type == APP_MSG_TYPE_APPEVENT)
						{					
							switch(state->event)
							{			
								/*Start event*/
								case NWK_START_EVENT:						
								{
									nwk_start_evnt_msg_t* nwk_start = (nwk_start_evnt_msg_t*)state->statedata;
									GV701x_NwkStart(nwk_start->link);
								}
								break;

								case NWK_STOP_EVENT:
								{
									nwk_stop_evnt_msg_t* nwk_stop = (nwk_stop_evnt_msg_t*)state->statedata;
									GV701x_NwkStop(nwk_stop->link);
								}
								break;
							
								default:
								break;
							}
						}
					}
				}
			}				
		break;
		
		default:
		break;
	}

	state->event = NWK_IDLE_EVENT;
	state->eventtype = 0;
	state->eventclass = 0;
	state->eventproto = 0;
	state->statedata = NULL;	
	state->statedatalen = 0;	
	memset((u8*)&state->msg_hdr, 0x00, sizeof(gv701x_app_msg_hdr_t));	
}

/******************************************************************************
 * @fn      GV701x_NwkSendEvent
 *
 * @brief   Broadcasts an indication to all applications based on its state change
 *			events triggered
 *
 * @param   ind - indication type
 *          link - link on which the transition occured
 *
 * @return  none
 *
 */

void GV701x_NwkSendEvent(u8 ind, u8 link) 
{	
	if(ind == NWK_START_IND)
	{
		nwk_start_ind_msg_t nwk_start_msg;
		nwk_start_msg.event = ind;
		nwk_start_msg.link = link;
		GV701x_SendAppEvent(nwkstartup_data.app_id, APP_BRDCST_MSG_APPID, APP_MSG_TYPE_APPIND,
			APP_MAC_ID, EVENT_CLASS_CTRL, MGMT_FRM_ID, &nwk_start_msg, sizeof(nwk_start_ind_msg_t), 0);		
	}
	else if(ind == NWK_LINKUP_IND)
	{
		nwk_up_ind_msg_t nwk_up_msg;
		nwk_up_msg.event = ind;
		nwk_up_msg.link = link;
		GV701x_SendAppEvent(nwkstartup_data.app_id, APP_BRDCST_MSG_APPID, APP_MSG_TYPE_APPIND,
			APP_MAC_ID, EVENT_CLASS_CTRL, MGMT_FRM_ID, &nwk_up_msg, sizeof(nwk_up_ind_msg_t), 0);		
	}
	else if(ind == NWK_LINKDWN_IND)
	{
		sEvent* event =NULL;
		nwk_dwn_ind_msg_t nwk_dwn_msg;
		nwk_dwn_msg.event = ind;
		nwk_dwn_msg.link = link;
		GV701x_SendAppEvent(nwkstartup_data.app_id, APP_BRDCST_MSG_APPID, APP_MSG_TYPE_APPIND,
			APP_MAC_ID, EVENT_CLASS_CTRL, MGMT_FRM_ID, &nwk_dwn_msg, sizeof(nwk_dwn_ind_msg_t), 0);		
	}
	else
		return;
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

void GV701x_Nwk_CmdProcess(char* CmdBuf) 
{
	if(strcmp(CmdBuf, "state") == 0) 
	{
		printf("\nNwk S %bu E %bu pl %bu wl: %bu", nwkstartup_state.state, nwkstartup_state.event, 
				nwkstartup_data.link.power_line.state,
				nwkstartup_data.link.wireless.state);
	}
	else if(strcmp(CmdBuf, "stats") == 0) 
	{
		printf("\nNwk p(fw) %bu w(fw) %bu pl %bu wl %bu pa %x wa %x", 
				nwkstartup_data.link.fw_ready.power_line, nwkstartup_data.link.fw_ready.wireless, 
				nwkstartup_data.link.power_line.state, nwkstartup_data.link.wireless.state, 
				nwkstartup_data.link.power_line.addr, nwkstartup_data.link.wireless.addr);
	}
	else if(strcmp(CmdBuf, "nvclear") == 0) 
	{
		GV701x_FlashErase(nwkstartup_data.app_id);
	}	
}					

#endif /*NWKSTARTUP_APP*/
