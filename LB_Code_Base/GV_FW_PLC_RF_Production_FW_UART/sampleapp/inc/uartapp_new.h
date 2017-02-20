/* ========================================================
 *
 * @file: uartapp.h
 * 
 * @brief: This file contains all defines needed for uartapp
 *
 *  Copyright (C) 2010-2014, Greenvity Communications, Inc.
 *  All Rights Reserved
 *  
 * =========================================================*/
 
#ifdef UART_APP

#ifndef UARTAPP_H
#define UARTAPP_H

/*****************************************************************************  
  *	Defines
  *****************************************************************************/
  
#define MAX_PKT_BUFFSIZE					(128)

/* Node Data Structure */
typedef struct 
{
	u8 app_id;
	/* MAC Address*/
	mac_addr_t macaddr;	  
	/*Displays Network Stats perodically*/	
	tTimerId stats_timer;  
}__PACKED__ node_data_t;

/* States */
typedef enum 
{
	NODE_IDLE_STATE = 1,
	NODE_ACTIVE_STATE,	
	NODE_INACTIVE_STATE,	
	NODE_RESTART_STATE,	
}node_state_types_t;

typedef enum 
{
	STATS_TIMER_EVNT,
}node_timer_event_t;


/*****************************************************************************  
  *	Function prototypes
  *****************************************************************************/

void UartApp_Init(u8 app_id);
void UartApp_Rx(u8 port, u8* databuf, u16 len);
void UartApp_ProcessEvent(gv701x_event_t event);
void UartApp_StateMachine(gv701x_state_t* state); 
void UartApp_TimerHandler(gv701x_event_t event);

#endif /*UARTAPP_H*/
#endif /*UART_APP*/
