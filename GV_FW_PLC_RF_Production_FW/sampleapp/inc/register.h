/* ========================================================
 *
 * @file:  register.h
 * 
 * @brief: This file contains all defines and profile needed 
 *         by the registration module
 * 
 *  Copyright (C) 2010-2015, Greenvity Communications, Inc.
 *  All Rights Reserved
 *  
 * =========================================================*/
 
#ifndef REGSTER_H
#define REGSTER_H

/****************************************************************************** 
  *	Defines
  ******************************************************************************/

typedef enum 
{
	REGISTER_IDLE_EVNT = 0,		
	REGISTER_START_EVNT,	
	REGISTER_STOP_EVNT
}register_event_t;

typedef enum 
{
	REGISTER_IDLE_IND = 0,		
	REGISTER_UP_IND
}register_ind_t;

typedef struct 
{
	u8 event;		
}__PACKED__ register_start_evnt_msg_t;

typedef struct 
{
	u8 event;		
}__PACKED__ register_stop_evnt_msg_t;

typedef struct 
{
	u8 event;	
	u16 nwk_addr;
}__PACKED__ register_up_ind_msg_t;


/****************************************************************************** 
  *	Externals
  ******************************************************************************/
extern u8 register_app_id;
extern gv701x_app_queue_t register_queues; 
extern gv701x_state_t register_state;

/****************************************************************************** 
  *	Function Prototypes
  ******************************************************************************/

void RegisterApp_Init(u8 app_id);
void RegisterApp_StateMachine(gv701x_state_t* state); 
void RegisterApp_TimerHandler(u8* buf);
void RegisterApp_RxAppMsg(sEvent* event);

#endif /*REGISTER_H*/
