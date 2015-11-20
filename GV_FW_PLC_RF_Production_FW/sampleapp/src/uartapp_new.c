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

#ifdef UART_APP
/****************************************************************************** 
  *	Includes
  ******************************************************************************/

#include "string.h"
#include "stdio.h"
#include "stdlib.h"
#include "gv701x_includes.h"
#include "uartapp_new.h"
/****************************************************************************** 
  *	Global Data
  ******************************************************************************/
  
#ifdef FLASH_TEST	
//u8 flashDataWriteBuff[] = "01234567890987654321abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";		
volatile u8 flashDataWriteBuff[500];						 
volatile u8 flashDataReadBuff[500];
u16 counter;
u8 temp;
#endif						 
/* Node data */
node_data_t node_data;  	

/* Node state */
gv701x_state_t node_state;	

/****************************************************************************** 
  *	External Data
  ******************************************************************************/  
#ifdef NWKSTARTUP_APP  
extern nwkstartup_data_t nwkstartup_data;
#endif
/******************************************************************************
  *	External Funtion prototypes
  ******************************************************************************/

/******************************************************************************
  *	Funtion prototypes
  ******************************************************************************/

void UartApp_Init(u8 app_id); 
void UartApp_Tx(u8 port, u8* databuf, u16 len);
void UartApp_Rx(u8 port, u8* databuf, u16 len); 

/******************************************************************************
 * @fn      UartApp_Init
 *
 * @brief   Initializes Node Data
 *
 * @param   app_id - application identification number
 *
 * @return  none
 */
 
void UartApp_Init(u8 app_id) 
{	
	uart_linectrl_t lineParam;
	FM_Printf(FM_USER, "\nInit UartApp (appid %bu)", app_id);

	/*Initializing Node Data*/
	memset(&node_data, 0x00, sizeof(node_data_t));

	/*Record the applications id,will 
	 be used while allocating timers*/
	node_data.app_id = app_id;
		
	/*Initializing State machine*/
	memset(&node_state, 0x00, sizeof(gv701x_state_t));
	node_state.state = NODE_IDLE_STATE;
	node_state.event = NODE_IDLE_EVNT;	
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
		printf("Uart config fail\n");
	}
	GV701x_SetUartLoopBack(TRUE);

	/* Sets UART Rx Timeout period*/ 
	GV701x_SetUartRxTimeout(20); 
	GV701x_UartTxMode(UART_TX_AUTO_MODE);
	GV701x_GPIO_Config(WRITE_ONLY, GP_PB_IO4_CFG | GP_PB_IO5_CFG);
	GV701x_GPIO_Config(READ_ONLY, GP_PB_IO6_CFG);

	/* GP_PB_IO_6 Read example */
	GV701x_GPIO_Read(GP_PB_IO6_RD);
	
	/*Reserve a timer*/
	node_data.stats_timer = STM_AllocTimer(SW_LAYER_TYPE_APP, 
						STATS_TIMER_EVNT,&node_data.app_id);			
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
		memset(&(pEthHdr->dstaddr), 0xFF, MAC_ADDR_LEN);

		/*Nodes MAC address is set as the Source address*/				
		memcpy(&(pEthHdr->srcaddr), &node_data.macaddr, MAC_ADDR_LEN); 
		/*We use a reserved ethernet type*/
		pEthHdr->ethtype = APP_ETHER_PROTO;
		len += sizeof(sEth2Hdr);
	}

	/*Post Data into the firmware data queue*/
	GV701x_SendData(port, databuf, len, APP_DATA_TX_SWBCST_OPT);
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

	/*Data recieved from PLC*/	
	if(port == APP_PORT_PLC)
	{
		/*Check if ethernet payload is no greater than Max Tx buffer*/
		if((len - sizeof(sEth2Hdr)) > MAX_PKT_BUFFSIZE)
			return;

		/*Copy payload*/
		memcpy(buf, (u8*)(databuf + sizeof(sEth2Hdr)), (len - sizeof(sEth2Hdr)));

		/*Transfer Data coming from PLC to UART*/
		UartApp_Tx(APP_PORT_PERIPHERAL, (u8*)bufPlc, (len - sizeof(sEth2Hdr)));
		
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
 * @fn     UartApp_ProcessEvent
 *
 * @brief   Parses all firmware,timer events and notifications 
 *
 * @param  event - events comming from the firmware
 *
 * @return  none
 *
 */

void UartApp_ProcessEvent(gv701x_event_t event)
{	
	u8 evnttype = 0;
	u16 evntpayloadlen;
	void* evntpayload = NULL;

	if((event.eventclass == EVENT_CLASS_CTRL) && 
		(event.eventtype == EVENT_FRM_ID))
	{
		if(event.eventproto == APP_MAC_ID)
		{
			switch(event.event)
			{
				case(HOST_EVENT_APP_CMD):
				{						
					/*Application Command received on Command line*/									
					evnttype = NODE_CMD_EVNT;
					evntpayload = event.eventdata;
					evntpayloadlen = event.eventdatalen;
				}
				break;	
										
				default:
				break;
			}
		}
	}

	if(evnttype != 0)
	{
		node_state.event = evnttype;	
		node_state.statedata = evntpayload;
		node_state.statedatalen = evntpayloadlen;
	}
}

void UartApp_TimerHandler(gv701x_event_t event)
{	
	if((event.event != HOST_EVENT_APP_TIMER) || 
	   (event.eventclass != EVENT_CLASS_CTRL) || 
	   (event.eventtype != EVENT_FRM_ID) || 
	   (event.eventproto != APP_MAC_ID))
		return;		
	
	if(event.eventdata)
	{
		hostTimerEvnt_t* timerevt = 
			(hostTimerEvnt_t*)(event.eventdata); 			
		
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
#ifdef HPGP_DRIVER_APP
					GV701x_HPGPGetDevStats();
#endif
				}								
			break;	
			
			default:
			break;
		}
	}							
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

void UartApp_StateMachine(gv701x_state_t* state) 
{
	if(state == NULL)
		return;
	
	switch(state->state) 
	{

		case NODE_IDLE_STATE:
				
			switch(state->event) 
			{		
				case NODE_IDLE_EVNT:									
				break;
				
				case NODE_ACTIVE_EVNT:
					state->state = NODE_ACTIVE_STATE;	
#ifdef NWKSTARTUP_APP  
					memcpy((u8*)&(node_data.macaddr), (u8*)nwkstartup_data.link.long_addr.mac_addr, 
							MAC_ADDR_LEN);
#endif
					printf("\nNode Active"); 								
				break;
						
				default:					
				break;
			}
		break;
						
		case NODE_ACTIVE_STATE:
							
			switch(state->event) 
			{			
				
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

#endif //UART_APP