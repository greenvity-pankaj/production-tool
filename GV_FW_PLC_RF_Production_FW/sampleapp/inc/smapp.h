/* ========================================================
 *
 * @file: smapp.h
 * 
 * @brief: This file contains all defines needed for smapp
 *
 *  Copyright (C) 2010-2014, Greenvity Communications, Inc.
 *  All Rights Reserved
 *  
 * =========================================================*/

#ifdef SMARTMETER_APP

#ifndef SMAPP_H
#define SMAPP_H

/*****************************************************************************  
  *	Defines
  *****************************************************************************/
  
#define MAX_PKT_BUFFSIZE					(128)

/* Node Data Structure */
typedef struct 
{
	u8 app_id;
	mac_addr_t macaddr;
	mac_addr_t peer_macaddr;
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
  *	External Data
  *****************************************************************************/

extern gv701x_state_t node_state;	

/*****************************************************************************  
  *	Function prototypes
  *****************************************************************************/

void SMApp_Init(u8 app_id);
void SMApp_Rx(u8 port, u8* databuf, u16 len);
void SMApp_Tx(u8 port, u8* databuf, u16 len);
void SMApp_TimerHandler(u8* buf);
void SMApp_StateMachine(gv701x_state_t* state); 

#endif /*SMAPP_H*/
#endif /*SMARTMETER_APP*/

