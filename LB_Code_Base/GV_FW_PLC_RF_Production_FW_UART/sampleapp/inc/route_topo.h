/* ========================================================
 *
 * @file:  route_topo.h
 * 
 * @brief: This file hold the mesh topology profiling
 *		   defines
 *      
 *  Copyright (C) 2010-2015, Greenvity Communications, Inc.
 *  All Rights Reserved
 *  
 * =========================================================*/
 #ifdef RTOPO_APP

#ifndef ROUTE_TOPO_H
#define ROUTE_TOPO_H

/****************************************************************************** 
  *	Defines
  ******************************************************************************/

typedef enum 
{
	RTOPO_IDLE_EVENT = 0,
	RTOPO_START_EVNT,
	RTOPO_PROFILE_EVNT,
	RTOPO_STOP_EVNT
}rtopo_event_t;

typedef struct 
{
	u8 event;
	u8 link;
	u8 mode;
}__PACKED__ rtopo_start_evnt_msg_t;

typedef struct 
{
	u8 event;
	u8 link;
}__PACKED__ rtopo_stop_evnt_msg_t;

typedef struct 
{
	u8 event;		
}__PACKED__ rtopo_prof_evnt_msg_t;

typedef struct 
{
	u8 event;		
}__PACKED__ rtopo_up_ind_msg_t;


#define RTOPO_SCAN_PEER_SELECTION			(0)
#define RTOPO_SCAN_CHANNEL_SELECTION		(1)

/****************************************************************************** 
  *	External Data
  ******************************************************************************/
extern u8 rtopo_app_id;
extern gv701x_app_queue_t rtopo_queues;
extern gv701x_state_t rtopo_state;

/****************************************************************************** 
  *	Function Prototypes
  ******************************************************************************/
  
void rtopo_init(u8 app_id);
void rtopo_poll(void);
void rtopo_RxAppMsg(sEvent* event);
void rtopo_cmdprocess(char* CmdBuf); 

#endif /*ROUTE_TOPO_H*/
#endif /*RTOPO_APP*/
