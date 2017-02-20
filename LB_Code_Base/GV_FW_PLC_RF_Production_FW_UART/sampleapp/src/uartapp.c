/* ========================================================
 *
 * @file: uartapp.c
 * 
 * @brief: This simple application forwards any data coming from UART
 *            to PLC and vise versa
 *
 *  Copyright (C) 2010-2014, Greenvity Communications, Inc.
 *  All Rights Reserved
 *  
 * =========================================================*/


/****************************************************************************** 
  *	Includes
  ******************************************************************************/

#include "string.h"
#include "stdio.h"
#include "stdlib.h"
#include "gv701x_includes.h"
#include "gv701x_driver.h"
#include "uartapp.h"

/****************************************************************************** 
  *	Global Data
  ******************************************************************************/

/*Pre defined system variables may be taken at 
    runtime from User or Non Volatile Memory*/
    
/*MAC Address*/
u8 macaddr[MAC_ADDR_LEN] = {0x00,0x60,0x45,0x28,0x17,0x38};// Should be unique for each device
/*Network keys*/
u8 nid[NID_LEN] = {0x47, 0x96, 0x18, 0xdd, 0x60, 0x4C, 0x32};											
u8 nmk[ENC_KEY_LEN] = {0xa4, 0x5e, 0x36, 0x87, 0x5a, 0x6f, 0x8c, 0xbe,
					   0x4e, 0x68, 0x24, 0x41, 0x3c, 0xa1, 0x9d, 0x0e};
#define FLASH_TEST
#ifdef FLASH_TEST	
//u8 flashDataWriteBuff[] = "01234567890987654321abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";		
volatile u8 flashDataWriteBuff[100];						 
volatile u8 flashDataReadBuff[100];
u16 counter;
u8 temp;
#endif						 
/* Node data */
static node_data_t node_data;  	

/* Node state */
static node_state_t	node_state;			

/* Firmware Interface Data Queues */
gv701x_aps_queue_t* fw_dataQ = NULL;			 

/******************************************************************************
  *	Funtion prototypes
  ******************************************************************************/

void UartApp_Init(gv701x_aps_queue_t* dataQ); 
void UartApp_Tx(u8 port, u8* databuf, u16 len);
void UartApp_Rx(u8 port, u8* databuf, u16 len); 
eStatus UartApp_SendData(u8 port, void* databuf, u16 len, u8 options);
u8 UartApp_Poll(void);
void UartApp_ProcessEvent(sEvent *event);
void UartApp_StateMachine(node_state_t* state); 

static u8 dstmacaddr[MAC_ADDR_LEN] = {0xff,0xff,0xff,0xff,0xff,0xff};// All 0xff means broadcast address

/******************************************************************************
Device Role
DEVICE_CCO - Starts Network
DEVICE_STA - Joins Network
DEVICE_AUTO - Network dynamically determines device role or mode
*/

#define DEVICE_AUTO

/******************************************************************************
 * @fn      UartApp_Init
 *
 * @brief   Initializes Node Data
 *
 * @param   dataQ  - Firmware passes a refernce to the Tx Rx Data/event/response 
 * 				   queues to be used to post and receive Data to/from the PLC or UART
 *
 * @return  none
 */
 
void UartApp_Init(gv701x_aps_queue_t* dataQ) 
{	
	uart_linectrl_t lineParam;
	
	FM_Printf(FM_USER, "\nInitializing Uart App..");	
	/*Store reference of Firmware Data queues*/
	fw_dataQ = dataQ;

	/*Initializing Node Data*/
	memset(&node_data, 0x00, sizeof(node_data_t));
	memcpy(&(node_data.macaddr), macaddr, MAC_ADDR_LEN);	
	memcpy(node_data.nwk.key.nid, nid, NID_LEN);
	memcpy(node_data.nwk.key.nmk, nmk, ENC_KEY_LEN);

	/*Initializing State machine*/
	memset(&node_state, 0x00, sizeof(node_state_t));
	node_state.state = IDLE_STATE;
	node_state.event = IDLE_EVNT;	
	node_state.statedata = NULL;
	node_state.statedatalen = 0;
	
  /*Sets UART Line Control Parameters*/
	lineParam.linectrl                   = 0;
	lineParam.linectrl_field.WdLen       = UART_WORD_SIZE_8;
	lineParam.linectrl_field.StopBit     = UART_STOPBIT_1;
	lineParam.linectrl_field.ParityEn    = UART_PARITY_DISABLE;
	lineParam.linectrl_field.EvenParity  = UART_PARITY_ODD;
	lineParam.linectrl_field.ForceParity = UART_FORCEPAR_DISABLE;
	lineParam.linectrl_field.ForceTx0    = UART_SETBREAK_DISABLE;
	
  	GV701x_SetUartLineParam(lineParam); // Sets Stop Bit = 1, No parity.
	/*Configure the UART to 1200 
	   and 20 byte frame length*/
	if(STATUS_SUCCESS != GV701x_UartConfig(1200,0))
	{
		printf("Couldnt configure Uart\n");
	}

	GV701x_SetUartLoopBack(TRUE);
	
	GV701x_SetUartRxTimeout(20); // Sets UART Rx Timeout period. 
	GV701x_UartTxMode(UART_TX_AUTO_MODE);
	GV701x_GPIO_Config(WRITE_ONLY, GP_PB_IO4_CFG | GP_PB_IO5_CFG);// Configures GP_PB_IO4 & 5 as output pins
	GV701x_GPIO_Config(READ_ONLY, GP_PB_IO6_CFG);// Configures GP_PB_IO6 as input pin
	
	if(GV701x_GPIO_Read(GP_PB_IO6_RD))// GP_PB_IO_6 Read example
	{
		printf("\nGPIO 6 H\n");
	}
	else
	{
		printf("\nGPIO 6 L\n");
	}
	/*Reserve a timer*/
	node_data.stats_timer = STM_AllocTimer(SW_LAYER_TYPE_APP, 
						STATS_TIMER_EVNT,&node_data);	
	
#if 0
	/*Start Stats Timer*/
	if(STATUS_SUCCESS != 
		STM_StartTimer(node_data.stats_timer,5000))
		printf( "\nCouldnt Start Stats Timer");						
#endif
#ifdef FLASH_TEST	
	for(counter=0;counter<sizeof(flashDataWriteBuff);counter++)
	{
		
		flashDataWriteBuff[counter] = temp;
		temp++;
	}
	if(GV701x_FlashWrite(1, flashDataWriteBuff, sizeof(flashDataWriteBuff)) == STATUS_SUCCESS)
	{
		u16 i;
		u8 error = 0;
		printf("Flash Content Verification\n");
		for(i=0;i<sizeof(flashDataReadBuff);i++)
		{
			flashDataReadBuff[i] =  GV701x_FlashReadByte(0, i);
			//printf("%bx\n",flashDataReadBuff[i]);	
			if(flashDataWriteBuff[i] != flashDataReadBuff[i])
			{
				printf("failed @ %u\n",i);
				error = 1;
			}
		
		}
		if(error != 1)
		{
			printf("Ok\n");
		}
	
#if 0	// Reads and returns specified number of bytes from start	
		if(GV701x_FlashRead(flashDataReadBuff,sizeof(flashDataWriteBuff)-1)== STATUS_SUCCESS)
		{
			unsigned char i;
			printf("Flash Content\n");
			for(i=0;i<(sizeof(flashDataWriteBuff)-1);i++)
			{
				printf("%c ",flashDataReadBuff[i]);
			}
			printf("\n");
		}
#endif
	}
#endif		
}
	
/******************************************************************************
 * @fn      UartApp_Tx
 *
 * @brief   Prepares and Sends Data frame to be sent on PLC or UART
 *
 * @param   port  - Port to transmit data (PLC or UART) 
 *                databuf  - Data buffer to be transmitter 
 *
 *		    len  - Length of Data to be transmitted
 *
 * @return  none
 *
 * @note In case of PLC, ethernet header is filled and sent hence
 *           14 bytes needs to be reserved in databuf at the begining
 */

void UartApp_Tx(u8 port, u8* databuf, u16 len)
{
	if((port != APP_PORT_PLC) &&
	  (port != APP_PORT_PERIPHERAL))
		return;

	/*Fill Ethernet header in case Data is to transmitted on PLC*/
	if(port == APP_PORT_PLC)
	{
		sEth2Hdr *pEthHdr = (sEth2Hdr *)databuf;
		/*Set Destination MAC address as broadcast*/
	
		node_data.stats.TxPlcCnt++;
		memset(&(pEthHdr->dstaddr), 0xFF, MAC_ADDR_LEN);// For broadcast addressing 
		//memcpy(&(pEthHdr->dstaddr), dstmacaddr, MAC_ADDR_LEN);// For unicast addressing
		
		/*Nodes MAC address is set as the Source address*/				
		memcpy(&(pEthHdr->srcaddr), &node_data.macaddr, MAC_ADDR_LEN); 
		/*We use a reserved ethernet type*/
		pEthHdr->ethtype = ETHER_TYPE;
		len += sizeof(sEth2Hdr);
	}

 	//printf("\nTx");	
	/*Post Data into the firmware data queue*/
	UartApp_SendData(port, databuf, len, BIT(0));
}

/******************************************************************************
 * @fn      UartApp_Rx
 *
 * @brief   Handles Data frame received from PLC and forwards
 *             it to UART and vise versa
 *
 * @param   port  - Port from which data is recieved (PLC or UART) 
 *                databuf  - Data recieved
 *
 *		    len  - Length of Data received
 *
 * @note In case of PLC, ethernet header present at the begining of databuf
 *           is removed and then sent to UART
 */

void UartApp_Rx(u8 port, u8* databuf, u16 len)
{
	u8 buf[MAX_PKT_BUFFSIZE];
	u8 bufPlc[MAX_PKT_BUFFSIZE];

	//printf("\nRx");	

	/*Data recieved from PLC*/	

	node_data.stats.RxCnt++;
	
	if(port == APP_PORT_PLC)
	{
		/*Check if ethernet payload is no greater than Max Tx buffer*/
		if((len - sizeof(sEth2Hdr)) > MAX_PKT_BUFFSIZE)
			return;

		/*Copy payload*/
		
		memcpy(dstmacaddr, (u8*)(databuf + MAC_ADDR_LEN), MAC_ADDR_LEN);// extracts mac address from incoming frame.
		/*Transfer Data coming from PLC to UART*/
		memcpy(bufPlc, (u8*)(databuf + sizeof(sEth2Hdr)), (len - sizeof(sEth2Hdr)));

		/*Transfer Data coming from PLC to UART*/
		UartApp_Tx(APP_PORT_PERIPHERAL, (u8*)bufPlc, (len - sizeof(sEth2Hdr)));
		
		//memcpy(&bufPlc[ETH_HDR_SIZE], (u8*)databuf + ETH_HDR_SIZE, (len - ETH_HDR_SIZE));
		//UartApp_Tx(APP_PORT_PLC, (u8*)bufPlc, (len - ETH_HDR_SIZE));
		GV701x_GPIO_Write(GP_PB_IO4_WR | GP_PB_IO5_WR,1);// Sets GPIO 4 and 5
	}
	
	/*Data recieved from UART*/		
	else if(port == APP_PORT_PERIPHERAL)
	{	
		/*Check if Data to be transmitted is no greater than Max Tx buffer*/		
		if(len > MAX_PKT_BUFFSIZE - sizeof(sEth2Hdr))
			return;

		/*Copy payload after ethernet header*/
		memcpy(&buf[sizeof(sEth2Hdr)], (u8*)databuf, len);

		/*Transfer Data coming from UART to PLC*/		
		UartApp_Tx(APP_PORT_PLC, (u8*)buf, len);	
		GV701x_GPIO_Write(GP_PB_IO4_WR | GP_PB_IO5_WR,0);// Resets GPIO 4 and 5
	}
}	

/******************************************************************************
 * @fn     UartApp_SendData
 *
 * @brief   Adds Data to be sent, to the Firmware Queue
 *
 * @param   port  - Port to which data is to be sent (PLC or UART) 
 *          databuf  - Data to be sent
 *		    len  - Length of Data 
 *			options - Transfer Otptions (BIT(0) -> Software Broadcast)
 *
 * @return  none
 */

eStatus UartApp_SendData(u8 port, void* databuf, u16 len, u8 options)
{
    sEvent* event = NULL;

 	if((databuf == NULL) || (len == 0))
 	{
		return STATUS_FAILURE;
 	}
EA = 0;		
	event = GV701x_EVENT_Alloc(len, 0);
EA = 1;
    if(event == NULL) 
    {
			//printf("Event alloc failed:Send data\n");
    	return STATUS_FAILURE;
    }

    event->eventHdr.eventClass = EVENT_CLASS_DATA;
    event->eventHdr.type = DATA_FRM_ID;
    event->eventHdr.trans = port;	
	event->eventHdr.status = 0;	

	if(options & BIT(0))
		event->eventHdr.status = BIT(0);		
	
EA = 0;				
	memcpy((u8*)(event->buffDesc.dataptr), databuf, len); 
EA = 1;	
	event->buffDesc.datalen = len;	
	node_data.stats.TxCnt++;

    /* Enqueue Data into the firmware transmit queue */	
	SLIST_Put(&(fw_dataQ->txQueue), &event->link);			
    return STATUS_SUCCESS;       
}

/******************************************************************************
 * @fn      UartApp_Poll
 *
 * @brief   Reads Incoming Data frames from the Firmware Queue
 *
 * @param  none
 *
 * @return  none
 *
 * @note This functions needs to be polled from the Task context
 */

u8 UartApp_Poll(void)
{
    sEvent *event = NULL;
    sSlink *slink = NULL;
    u8      ret = 0;

	/*Read Data receive Queue*/
	while(!SLIST_IsEmpty(&(fw_dataQ->rxQueue)))
	{
__CRIT_SECTION_BEGIN__
		slink = SLIST_Pop(&(fw_dataQ->rxQueue));
__CRIT_SECTION_END__
		event = SLIST_GetEntry(slink, sEvent, link);

		if(event == NULL)
		{
			break;
		}
		
		if((event->buffDesc.dataptr == NULL) || 
		  (event->buffDesc.datalen == 0) )
		{
			break; 
		}
		
		if(event->eventHdr.eventClass != EVENT_CLASS_DATA)
		{
			EVENT_Free(event);	
			break;
		}

		/*Call Data Handler*/
		UartApp_Rx(event->eventHdr.trans, 
				   event->buffDesc.dataptr, 
				   event->buffDesc.datalen);  
		EVENT_Free(event);		   
	}

	/*Read Event receive Queue*/
	while(!SLIST_IsEmpty(&(fw_dataQ->rspEvntRxQueue)))
	{
__CRIT_SECTION_BEGIN__
		slink = SLIST_Pop(&(fw_dataQ->rspEvntRxQueue));
__CRIT_SECTION_END__
		event = SLIST_GetEntry(slink, sEvent, link);

		/*Call Event/Response Handler*/
		UartApp_ProcessEvent(event);  
		EVENT_Free(event);		   
	}	
    return ret;
}


/******************************************************************************
 * @fn     UartApp_ProcessEvent
 *
 * @brief   Parses all firmware,timer events and notifications 
 *
 * @param  event - events comming from the firmware
 *
 * @return  none
 *
 */

void UartApp_ProcessEvent(sEvent *event)
{	
	u8 evnttype = 0;
	u8 evntclass;
	u16 evntpayloadlen;
	void* evntpayload = NULL;
	hostHdr_t  *pHostHdr;

	if(event == NULL)
	{
		return;
	}
	
	if(((event->eventHdr.eventClass != EVENT_CLASS_CTRL) && 
		(event->eventHdr.eventClass != EVENT_CLASS_MGMT) ))
	{
		return;
	}
	
	pHostHdr = (hostHdr_t* )event->buffDesc.dataptr;
	
	/*Control Response/Event from firmware*/
	if(event->eventHdr.eventClass == EVENT_CLASS_CTRL) 		  
	{
		switch(pHostHdr->type)
		{
			case CONTROL_FRM_ID:
			{
				u8* ctrlevent = (u8*)(pHostHdr + 1);
				switch(*ctrlevent)
				{
					/*PLC network initiated confirmation*/
					case(APCM_STA_RESTART_CNF):
					{						
						evnttype = NETWORK_SET_CNF;
						evntclass = EVENT_CLASS_CTRL;
						evntpayload = (pHostHdr + 1);
						evntpayloadlen = sizeof(hostCmdNwk);
						printf( "Restart Network confirm\n");						
					}
					break;			
					
					/*PLC network initiated confirmation*/
					case(APCM_SET_KEY_CNF):
					{						
						evnttype = NETWORK_KEY_CNF;
						evntclass = EVENT_CLASS_CTRL;
						evntpayload = (pHostHdr + 1);
						evntpayloadlen = sizeof(hostCmdNetId);
						printf( "Network key set confirm\n");						
					}
					break;									
					
					default:
					break;										
				}
			}
			break;	
	
			/*Event Notifications from firmware*/
			case EVENT_FRM_ID:
			{
				hostEventHdr_t* pHostEvent = (hostEventHdr_t*)(pHostHdr + 1);
				
				if(pHostEvent->eventClass == EVENT_CLASS_CTRL)
				{
					switch(pHostEvent->type)
					{
						case HOST_EVENT_FW_READY:
						{
							/*Firmware ready event*/		
							evnttype = FW_READY_EVNT;
							evntclass = EVENT_CLASS_CTRL;
							printf( "Firmware ready event\n");
						}			
						break;	
						
						case(HOST_EVENT_APP_CMD):
						{						
							/*Application Command received on Command line*/									
							evnttype = CMD_EVNT;
							evntclass = EVENT_CLASS_CTRL;
							evntpayload = (pHostEvent + 1);
							evntpayloadlen = pHostHdr->length - sizeof(hostEventHdr_t);							
						}
						break;	

						case(HOST_EVENT_NETWORK_IND):
						{						
							/*PLC network notifications*/							
							evnttype = NETWORK_IND_EVNT;
							evntclass = EVENT_CLASS_CTRL;
							evntpayload = (pHostEvent + 1);
							evntpayloadlen = pHostHdr->length - sizeof(hostEventHdr_t);
							printf( "Network Indication\n");							
						}
						break;
						
						case(HOST_EVENT_APP_TIMER):
						{						
							/*Application timer event indication*/							
							evnttype = TIMER_EVNT;
							evntclass = EVENT_CLASS_CTRL;
							evntpayload = (u8*)(pHostEvent + 1);
							evntpayloadlen = sizeof(hostTimerEvnt_t);
						}
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
	}
	else if(event->eventHdr.eventClass == EVENT_CLASS_MGMT)
	{
		/*Mgmt Response/Event from firmware*/
		switch(pHostHdr->type)
		{
			case MGMT_FRM_ID:
			{
				u8* mgmtevent = (u8*)(pHostHdr + 1);
				switch(*mgmtevent)
				{
					/*Set hardware spec confirm recieved*/				
					case(HOST_CMD_HARDWARE_SPEC_CNF):
					{
                        hostCmdHwspec* hwspec = mgmtevent;
                        if(hwspec->action == ACTION_SET)
    						evnttype = HARDWARE_SPEC_SET_EVNT;
                        else if(hwspec->action == ACTION_GET)
                            evnttype = HARDWARE_SPEC_GET_EVNT,	
						evntclass = EVENT_CLASS_CTRL;
						evntpayload = (pHostHdr + 1);
						evntpayloadlen = sizeof(hostCmdHwspec); 													
						printf( "Hw Spec confirm\n");
					}
					break;

					/*Set hardware spec confirm recieved*/				
					case(HOST_CMD_DEVICE_STATS_CNF):
					{
						evnttype = DEVICE_STATS_EVNT;
						evntclass = EVENT_CLASS_CTRL;
						evntpayload = (pHostHdr + 1);
						evntpayloadlen = sizeof(hostCmdDevstats);
					}
					break;
					/*Set Power Save AVLN confirm recieved*/
					case HOST_CMD_PSAVLN_CNF:
						evntpayload = (pHostHdr + 1);
						evntpayloadlen = sizeof(hostCmdPsAvln); 													
						printf( "PS AVLN confirm\n");
					break;
					/*Set Power Save STA confirm recieved*/
					case HOST_CMD_PSSTA_CNF:
						evntpayload = (pHostHdr + 1);
						evntpayloadlen = sizeof(hostCmdPsSta); 													
						printf( "PS STA confirm\n");
					break;
					case HOST_CMD_PEERINFO_CNF:
					{
						u8 lclCount;
						hostCmdPeerinfo *peerinfo;
						peerinfodata *peer;
						peerinfo = (hostCmdPeerinfo *)(pHostHdr + 1);
						printf("Get PeerInfo Confirm\n");
						if(peerinfo->result == STATUS_SUCCESS) 
						{
							printf("Get Peerinfo Success.\n");
							printf("No of Entries: %bu\n",peerinfo->noofentries);
							peer = (peerinfodata *)(peerinfo + 1);
							for( lclCount=0; lclCount<(peerinfo->noofentries); lclCount++) 
							{
								printf("\nMAC Addr: %02bX:%02bX:%02bX:%02bX:%02bX:%02bX\n",\
															peer->macaddr[0], peer->macaddr[1],\
															peer->macaddr[2], peer->macaddr[3],\
															peer->macaddr[4], peer->macaddr[5]);
								printf("Tei:  %bu\n", peer->tei);
								printf("RSSI: %02bX\n",peer->rssi);
								printf("LQI:  %02bX\n",peer->lqi);
								peer = (peer + 1);
							}
						} 
						else 
						{
							printf("Get Peerinfo Failed.\n");
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
	}
	
	if(!evnttype)
	{
		return;
	}

	node_state.event = evnttype;
	node_state.statedata = evntpayload;
	node_state.statedatalen = evntpayloadlen;
	UartApp_StateMachine(&node_state);	
	return;
}

/******************************************************************************
 * @fn      UartApp_StateMachine
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

void UartApp_StateMachine(node_state_t* state) {

	if(state == NULL)
	{
		return;
	}

#if 0
	printf("\nState (s: %bu e: %bu)", 		
			  (u8)state->state,  (u8)state->event);	
#endif
	switch(state->state) 
	{
		case IDLE_STATE:
				
			switch(state->event) 
			{		
				case IDLE_EVNT:									
				break;

				case FW_READY_EVNT:									
				{
					/*Set Hardware params like MAC address*/
                    GV701x_GetHwspec((u8*)(&node_data.macaddr));
				}
				break;		
                case HARDWARE_SPEC_GET_EVNT:
                    if(state->statedata)
					{	
						hostCmdHwspec* hwspec = 
							(hostCmdHwspec*)state->statedata; 
                        if(hwspec->result == STATUS_SUCCESS)
                        {
                            memcpy((char*)(&node_data.macaddr), (char*)&hwspec->mac_addr,6);
                        }
                        else
                        {
                            // donothing
                        }
						
					}
					Gv701x_SetHwspec((u8*)(&node_data.macaddr), 
										LINE_MODE_DC, DEFAULT_TX_POWER_MODE, 1);
                break;

				case TIMER_EVNT:
				{
					if(state->statedata)
					{	
						hostTimerEvnt_t* timerevt = 
							(hostTimerEvnt_t*)state->statedata; 					
						
						/*Demultiplexing the specific timer event*/						
						switch((u8)timerevt->type)
						{
							case STATS_TIMER_EVNT:
								GV701x_GetDevStats();								
								/*Start Test Timer*/
								if(STATUS_SUCCESS != 
									STM_StartTimer(node_data.stats_timer,2000))
								{
									printf( "Couldnt Start Stats Timer\n");						
								}								
							break;	
							default:
							break;
						}
					}
				}
				break;
				
				case HARDWARE_SPEC_SET_EVNT:									
				{
					/*Set PLC network keys*/	
					Gv701x_SetNetId(node_data.nwk.key.nmk, 
									node_data.nwk.key.nid);
				}
				break;
				
				case NETWORK_KEY_CNF:									
				{
					/*Initiating the PLC network*/
					#ifdef DEVICE_AUTO
					Gv701x_ReStartNwk(); // Auto device role selection
					#else 
					#ifdef DEVICE_CCO
					GV701x_HPGPStartNwk(NETWORK_START, node_data.nwk.key.nid);// To start device as CCO
					#else
					#ifdef DEVICE_STA
					GV701x_HPGPStartNwk(NETWORK_JOIN, node_data.nwk.key.nid);// To start device as STA
					#else
					#error "Device Type not defined."
					#endif
					#endif
					#endif
				}
				break;				

				case NETWORK_SET_CNF:
				break;				
				
				case NETWORK_IND_EVNT:									
				{
					/*Demultiplexing the type of nwk event*/
					if(state->statedata) 
					{
						hostEvent_NetworkId* nwkId = (hostEvent_NetworkId*)state->statedata;						
				    	switch((u8)(nwkId->state))	 	
				    	{				    	
							case NWID_STATE_INIT:
								printf("Initialized\n");
							break;
							case NWID_STATE_NET_DISC:
								printf("Discovery\n");
							break;
							case NWID_STATE_UNASSOC_STA:
								printf("Unassociated Station\n");
							break;
							case NWID_STATE_ASSOC_STA:
								printf("Associated Station\n");
								state->state = ASSOCIATED_STATE;
								//GV701x_SetPsSta(1, 10, 5);
							break;
							case NWID_STATE_UNASSOC_CCO:
								printf("Unassociated Co-ordinator\n");
							break;
							case NWID_STATE_ASSOC_CCO:
								printf("Associated Co-ordinator\n");
								state->state = ASSOCIATED_STATE;
								//GV701x_SetPsAvln(1);
							break;
    						
							default:
							break;
				    	}						
					}									
				}
				break;								
				
				case CMD_EVNT:
					/*Customized application command available through 
					    the command line interface*/
					if(state->statedata)
					{
						printf("Cmd: %s", state->statedata);
						printf("\n");
						if(strcmp((char* )state->statedata, "stats") == 0)
						{
							printf("\nTx: %lu TxPlc: %lu Rx: %lu\n", 
									node_data.stats.TxCnt, node_data.stats.TxPlcCnt,node_data.stats.RxCnt);							
						}						
					}
				break;
			
				default:					
				break;
			}
		break;
		
		case ASSOCIATED_STATE:
				
			switch(state->event) 
			{	
				case IDLE_EVNT:									
				break;

				/*Some events can occur in more than one state
				    for example a command event can be recieved
				    in Idle state as well as Associated state*/
				case CMD_EVNT:
					if(state->statedata)
					{
						printf("Cmd: %s", 
								  state->statedata);
						printf("\n");
						printf("Cmd: %s", state->statedata);
						printf("\n");
						if(strcmp((char* )state->statedata, "stats") == 0)
						{
							printf("\nTx: %lu TxPlc: %lu Rx: %lu", 
									node_data.stats.TxCnt, node_data.stats.TxPlcCnt,node_data.stats.RxCnt);
						}						
						else if(strcmp((char* )state->statedata, "peer") == 0)
						{
							GV701x_GetPeerInfo();// Get Peer List
						}
					}
				break;
				case NETWORK_IND_EVNT:									
				{
					/*Demultiplexing the type of nwk event*/
					if(state->statedata) 
					{
						hostEvent_NetworkId* nwkId = (hostEvent_NetworkId*)state->statedata;						
				    	switch((u8)(nwkId->state))	 	
				    	{				    	
							case NWID_STATE_NET_DISC:
								printf("Discovery\n");
							break;
							case NWID_STATE_UNASSOC_STA:
								printf("Unassociated Station\n");
								state->state = IDLE_STATE;
							break;
							case NWID_STATE_UNASSOC_CCO:
								printf("Unassociated Co-ordinator\n");
								state->state = IDLE_STATE;								
							break;
    						
							default:
							break;
				    	}						
					}									
				}
				break;
				case TIMER_EVNT:
				{
					if(state->statedata)
					{	
						hostTimerEvnt_t* timerevt = 
							(hostTimerEvnt_t*)state->statedata; 					
						
						/*Demultiplexing the specific timer event*/						
						switch((u8)timerevt->type)
						{
							case STATS_TIMER_EVNT:
								GV701x_GetDevStats();
								/*Start Test Timer*/
								if(STATUS_SUCCESS != 
									STM_StartTimer(node_data.stats_timer,2000))
								{
									printf( "Couldnt Start Stats Timer\n");						
								}								
							break;	
							default:
							break;
						}
					}
				}
				break;

				case DEVICE_STATS_EVNT:									
				{
					if(state->statedata)
					{
						hostCmdDevstats* DevStats = 
							(hostCmdDevstats *)state->statedata;
						/*Display Device Stats*/	
						printf("\n******Device Stats***********\n");					
						printf("Tx Count %lu\n",DevStats->txdatapktcnt);
						printf("Rx Count %lu\n",DevStats->rxdatapktcnt);
						printf("Tx Drop Count %lu\n",DevStats->txpktdropcnt);
						printf("Rx Drop Count %lu\n",DevStats->rxpktdropcnt);
						printf("Tx Ethernet Count %lu\n",DevStats->txhostpktcnt);
						printf("Tx Tx Ethernet Count %lu\n",DevStats->rxhostpktcnt);							
					}
				}
				break;					
				
				default:					
				break;
			}
		break;						
		
		default:
		break;
	}
	
	state->event = IDLE_EVNT;
	state->statedata = NULL;	
	state->statedatalen = 0;
}


