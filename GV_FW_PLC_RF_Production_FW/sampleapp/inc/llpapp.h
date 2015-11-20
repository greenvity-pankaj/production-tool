/* ========================================================
 *
 * @file:  llpapp.h
 * 
 * @brief: This file contains all defines needed for the LLP
 *
 *  Copyright (C) 2010-2015, Greenvity Communications, Inc.
 *  All Rights Reserved
 *  
 * =========================================================*/
 
#ifdef LLP_APP

#ifndef LLPAPP_H
#define LLPAPP_H

/****************************************************************************** 
  *	Defines
  ******************************************************************************/
  
typedef enum 
{
	NODE_IDLE_EVNT = 0,
	NODE_ACTIVE_EVNT,
	NODE_INACTIVE_EVNT,	
	NODE_TRIGGER_EVNT,	
}node_state_event_t;

typedef struct 
{
	u8 event;		
}__PACKED__ node_trigger_evnt_msg_t;

typedef struct 
{
	//u16 short_add;
	u16 rank;
	u8 link;
	u32	llp_RxCnt;
	u32 llp_TxCnt;
	//u8 dev_type;
	u8 rssi;
	u8 lqi;
	u8 rf_channel;
	u16 rf_panid;
	//u32 totalRx;
	//u32 totalTx;
	//u16 noPlcPeer;
	//u16 noRfPeer;
	u16	noChild;	
	u8 plc_nid[NID_LEN];
	u8 version[20];
}__PACKED__ llp_cmd_slave_stats_t;

/****************************************************************************** 
  *	Externals
  ******************************************************************************/
extern u8 node_app_id;
extern gv701x_app_queue_t node_queues; 
extern gv701x_state_t node_state;

/****************************************************************************** 
  *	Function Prototypes
  ******************************************************************************/

void LlpApp_Init (u8 app_id); 
void LlpApp_StateMachine(gv701x_state_t* state); 
void LlpApp_TimerHandler(u8* buf);
void LlpApp_RxAppMsg(sEvent* event);

#endif /*LLPAPP_H*/
#endif /*LLP_APP*/