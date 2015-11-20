/* ========================================================
 *
 * @file: smapp.c
 * 
 * @brief: This simple application configures the HPGP
 *         network and bridges PLC and IEEE 802.15.4 links 
 *
 *  Copyright (C) 2010-2014, Greenvity Communications, Inc.
 *  All Rights Reserved
 *  
 * =========================================================*/

#ifdef SMARTMETER_APP
/****************************************************************************** 
  *	Includes
  ******************************************************************************/

#include "string.h"
#include "stdio.h"
#include "stdlib.h"
#include "gv701x_includes.h"
#include "smapp.h"

/****************************************************************************** 
  *	Global Data
  ******************************************************************************/
  
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

void SMApp_Tx(u8 port, u8* databuf, u16 len);
void SMApp_Rx(u8 port, u8* databuf, u16 len); 

/******************************************************************************
 * @fn      SMApp_Init
 *
 * @brief   Initializes Node Data
 *
 * @param   app_id - application identification number
 *
 * @return  none
 */
 
void SMApp_Init(u8 app_id) 
{	
	/*Initializing Node Data*/
	memset(&node_data, 0x00, sizeof(node_data_t));
	
	node_data.app_id = app_id;
	FM_Printf(FM_USER, "\nInit SmartMeterApp (tc %bu)", node_data.app_id);
	
	/*Initializing State machine*/
	memset(&node_state, 0x00, sizeof(gv701x_state_t));
	node_state.state = NODE_IDLE_STATE;
	node_state.event = NODE_IDLE_EVNT;	
	node_state.statedata = NULL;
	node_state.statedatalen = 0;

	/*Reserve a timer*/
	node_data.stats_timer = STM_AllocTimer(SW_LAYER_TYPE_APP, 
						STATS_TIMER_EVNT,&node_data.app_id);			
}

/******************************************************************************
 * @fn      SMApp_Tx
 *
 * @brief   Prepares and Sends Data frame to be sent on PLC or IEEE 802.15.4
 *
 * @param   port  - Port to transmit data (PLC or IEEE 802.15.4) 
 *                databuf  - Data buffer to be transmitter 
 *
 *		    len  - Length of Data to be transmitted
 *
 * @return  none
 *
 * @note In case of PLC, ethernet header is filled and sent hence
 *           14 bytes needs to be reserved in databuf at the begining
 */

void SMApp_Tx(u8 port, u8* databuf, u16 len)
{
	if((port != APP_PORT_PLC) &&
	  (port != APP_PORT_PERIPHERAL) && 
	  (port != APP_PORT_ZIGBEE))
		return;

	/*Fill Ethernet header in case Data is to transmitted on PLC*/
	if(port == APP_PORT_PLC)
	{
		sEth2Hdr *pEthHdr = (sEth2Hdr *)databuf;		
		/*Set Destination MAC address as broadcast*/		
		memcpy(&(pEthHdr->dstaddr), (u8*)&node_data.peer_macaddr, MAC_ADDR_LEN); 
		/*Nodes MAC address is set as the Source address*/				
		memcpy(&(pEthHdr->srcaddr), (u8*)&node_data.macaddr, MAC_ADDR_LEN); 
		/*We use a reserved ethernet type*/
		pEthHdr->ethtype = APP_ETHER_PROTO;
		len += sizeof(sEth2Hdr);
	}

	if(port == APP_PORT_PLC)
	{
		//FM_HexDump(FM_APP,"\nPLC tx", databuf, len);
		/*Post Data into the firmware data queue*/
		GV701x_SendData(port, databuf, len, 0);
	}
	else if(port == APP_PORT_ZIGBEE)
	{
		GV701x_SendData(port, databuf, len, 0);
	}
}

/******************************************************************************
 * @fn      SMApp_Rx
 *
 * @brief   Handles Data frame received from PLC and forwards
 *          it to IEEE 802.15.4 and vise versa
 *
 * @param   port  - Port from which data is recieved (PLC or IEEE 802.15.4) 
 *          databuf  - Data recieved
 *		    len  - Length of Data received
 *
 * @note In case of PLC, ethernet header present at the begining of databuf
 *           is removed and then sent to UART
 */

void SMApp_Rx(u8 port, u8* databuf, u16 len)
{
	u8 buf[MAX_PKT_BUFFSIZE];	

	if(node_state.state != NODE_ACTIVE_STATE)
		return;
	
	/*Data recieved from PLC*/	
	if(port == APP_PORT_PLC)
	{		
#if (defined BRIDGE_CCO) || (defined BRIDGE_STA)
		/*Check if ethernet payload is no greater than Max Tx buffer*/
		if((len - sizeof(sEth2Hdr)) > MAX_PKT_BUFFSIZE)
			return;

		/*Copy payload*/
		memcpy(buf, (u8*)(databuf + sizeof(sEth2Hdr)), (len - sizeof(sEth2Hdr)));

		/*Transfer Data coming from PLC to IEEE 802.15.4*/
		SMApp_Tx(APP_PORT_ZIGBEE, (u8*)buf, (len - sizeof(sEth2Hdr)));
#endif
#if 0
#ifdef BRIDGE_STA
		if(len > MAX_PKT_BUFFSIZE)
			return;
		SMApp_Tx(APP_PORT_PLC, (u8*)databuf, (len - sizeof(sEth2Hdr)));	
#endif
#endif
	}

	/*Data recieved from IEEE 802.15.4*/		
	else if(port == APP_PORT_ZIGBEE)
	{	
#if (defined BRIDGE_CCO) || (defined BRIDGE_STA)
		/*Check if Data to be transmitted is no greater than Max Tx buffer*/		
		if(len > MAX_PKT_BUFFSIZE - sizeof(sEth2Hdr))
			return;

		/*Copy payload after ethernet header*/
		memcpy(&buf[sizeof(sEth2Hdr)], (u8*)databuf, len);

		/*Transfer Data coming from IEEE 802.15.4 to PLC*/		
		SMApp_Tx(APP_PORT_PLC, (u8*)buf, len);	
#endif			
	}
}	

void SMApp_TimerHandler(u8* buf)
{	
	hostTimerEvnt_t* timerevt = 
		(hostTimerEvnt_t*)buf;			

	if(event.eventdata)
		return;
		
	if(timerevt->app_id != node_data.app_id)
		return;
		
	/*Demultiplexing the specific timer event*/ 					
	switch((u8)timerevt->type)
	{								
		case STATS_TIMER_EVNT:
			/*Start Test Timer*/
			if(STATUS_SUCCESS != 
				STM_StartTimer(node_data.stats_timer,2000))
			{
				GV701x_HPGPGetDevStats();
			}								
		break;	
		
		default:
		break;
	}							
}

/******************************************************************************
 * @fn      SMApp_StateMachine
 *
 * @brief   Services all application events in all possible states 
 *
 * @param  state - State machine structure containing current
 *				state and current event to be serviced
 *
 * @return  none
 *
 * @note This is not a polling function. It is called asynchronously 
 *           as and when and event occurs.
 */

void SMApp_StateMachine(gv701x_state_t* state) 
{
	if(state == NULL)
		return;
	
	switch(state->state) 
	{

		case NODE_IDLE_STATE:
		case NODE_ACTIVE_STATE:				
			switch(state->event) 
			{		
				case NODE_IDLE_EVNT:									
				break;
				
				case NODE_ACTIVE_EVNT:
					state->state = NODE_ACTIVE_STATE;	
					memcpy((u8*)&node_data.macaddr, (u8*)nwkstartup_data.link.long_addr.mac_addr, 
							MAC_ADDR_LEN);
#ifdef HPGP_DRIVER_APP					
					if(hpgp_nwk_data.params.nwk.peer_info.no_of_peers != 0)
					{
						u8 idx;
						for(idx = 0; idx < hpgp_nwk_data.params.nwk.peer_info.no_of_peers; idx++)
						{
							if(memcmp((u8*)hpgp_nwk_data.params.nwk.peer_info.peer_data[idx].macaddr,
								      (u8*)&node_data.macaddr, MAC_ADDR_LEN) != 0)
							{
								memcpy((u8*)&node_data.peer_macaddr, 
										(u8*)hpgp_nwk_data.params.nwk.peer_info.peer_data[idx].macaddr, 
										MAC_ADDR_LEN);															
								break;
							}
						}
					}
#endif						
				break;
						
				default:					
				break;
			}
		break;
						
		case NODE_RESTART_STATE:				
		break;

		default:
		break;
	}
	
	state->event = NODE_IDLE_EVNT;
	state->statedata = NULL;	
	state->statedatalen = 0;
}
#endif //SMARTMETER_APP
