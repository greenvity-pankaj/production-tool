/* ========================================================
 *
 * @file:  route_topo.h
 * 
 * @brief: This file contains all defines needed for the 
 *         Route topology profiler
 *
 *  Copyright (C) 2010-2015, Greenvity Communications, Inc.
 *  All Rights Reserved
 *  
 * =========================================================*/

#ifdef RTOPO_APP

#ifndef ROUTE_TOPO_FW_H
#define ROUTE_TOPO_FW_H

/****************************************************************************** 
  *	Defines
  ******************************************************************************/

#define RTOPO_MAX_PKT_BUFFSIZE				(512)
#define RTOPO_BASE_TIMEOUT					(2000) 	
#define RTOPO_TIMEOUT_MAX_EXPONENT			(3) 	
#define RTOPO_TIMEOUT_EXPONENT				(2)
#define RTOPO_MAX_TX_RETRY_COUNT			(10)

#define RTOPO_PREF_TIME 					(8000)
#define RTOPO_SCAN_TIME 					(4000)

#define RTOPO_TOPOCHANGE_TIME				(300000)

typedef enum 
{
	RTOPO_INIT = 0,
	RTOPO_START,
	RTOPO_COMPLETE
}rtopo_state_types_t;

typedef enum 
{
	RTOPO_TX_TIMER_EVNT = 0,	
	RTOPO_PROFILE_TIMER_EVNT,
	RTOPO_PROFILE_1_TIMER_EVNT,
	RTOPO_FREEZE_TIMER_EVNT,
	RTOPO_SCAN_TIMER_EVNT,
	RTOPO_PREF_TIMER_EVNT
}rtopo_timer_event_t;

typedef enum 
{
	RTOPO_PLC_NID = 1,
}rtopo_getparam_id_t;

typedef struct 
{
	u8 id;
	u32 value;
}__PACKED__ rtopo_getparam_tv_t;

typedef struct 
{
	u8 ieee_address[IEEE_MAC_ADDRESS_LEN];
	rtopo_getparam_tv_t v[1];
}__PACKED__ rtopo_getparam_t;

typedef struct 
{
	u8 app_id;
	u8 tx_cnt;	
	tTimerId tx_timer;
	tTime tx_time;
	tTimerId profile_timer;
	tTimerId profile_1_timer;
	/*Topology freeze timer*/
	tTimerId freeze_timer; 
	struct 
	{
		bool enabled;
		tTimerId timer;
	}pref;			
	struct 
	{
		struct 
		{			
			bool enabled;
			tTimerId timer;
		}power_line;
		struct 
		{			
			bool enabled;
			u8 channel;
			u16 panid;		
			u8 start;
			u8 scanMode;
			u8 scantype;
			tTimerId timer;	
		}wireless;		
	}scan;		
	struct 
	{
		u8 link;
		u8 cnt;
	}link_pref;
}__PACKED__ rtopo_data_t;

/****************************************************************************** 
  *	Externals
  ******************************************************************************/

extern u8 ouid[OUID_LEN];
extern rtopo_data_t rtopo_data;

/****************************************************************************** 
  *	Function Prototypes
  ******************************************************************************/

void rtopo_init(u8 app_id);
void rtopo_start(u8 link, u8 mode);
void rtopo_poll(void);
void rtopo_rx(u8* buf, u8 len);
u8 rtopo_tx(u8* buf, u8 payloadLen, u8 frametype);
void rtopo_send_getparam(u8 id, u8 *val, u8* cnt);
u8 rtopo_recv_getparam (rtopo_getparam_t* param); 
void rtopo_handle_scanind(u8 link, u8 status);
void rtopo_timerhandler(u8* buf);
void rtopo_sm(gv701x_state_t* state);
void rtopo_start_scan(u8 link, u8 scanMode);
void rtopo_RxAppMsg(sEvent* event);

#endif /*ROUTE_TOPO_FW_H*/
#endif /*RTOPO_APP*/
