/* ========================================================
 *
 * @file:  gv701x_nwkstartup.h
 * 
 * @brief: This file contains all defines and profile needed 
 *            by the network startup module
 * 
 *  Copyright (C) 2010-2015, Greenvity Communications, Inc.
 *  All Rights Reserved
 *  
 * =========================================================*/
#ifdef NWKSTARTUP_APP

#ifndef GV701X_NWKSTARTUP_H
#define GV701X_NWKSTARTUP_H

/****************************************************************************** 
  *	Defines
  ******************************************************************************/
	
#define NWKSTARTUP_PROFILE_TIME					(5000)
#define NWKSTARTUP_LINK_PREF_CNT 				(4)

typedef enum {
	NWK_INIT = 0,
	NWK_START,		
	NWK_UP,
	NWK_DOWN,	
}nwkstartup_state_types_t;

typedef enum {
	NWK_IDLE_EVENT = 0,
	NWK_START_EVENT,
	NWK_STOP_EVENT	
}nwkstartup_event_t;

typedef enum {
	NWK_IDLE_IND = 0,
	NWK_START_IND,
	NWK_LINKUP_IND,
	NWK_LINKDWN_IND	
}nwkstartup_ind_t;

typedef enum {
	LINK_DISABLE = 0,	
	LINK_UP,
	LINK_DOWN
}nwkstartup_link_state_t;

typedef enum {
	NWK_REASON_NONE = 0,
	NWK_REASON_MANUAL,
	NWK_REASON_LINKFAIL
}nwkstartup_reason_t;

typedef struct {
	struct {
	   u8 state;	
	   u16 addr;
	}power_line;
	struct {
	   u8 state;	
	   u16 addr;	   
	}wireless;
	struct {
		u8 wireless;
		u8 power_line;		
	}fw_ready;
	union {
		u8 mac_addr[MAC_ADDR_LEN];
		u8 ieee_addr[IEEE_MAC_ADDRESS_LEN];
	}long_addr;
	struct {
		u8 link;
		u8 cnt;
	}link_preferrence;
}__PACKED__ nwkstartup_link_info_t;

typedef struct {
	u8 app_id;
	gv701x_app_queue_t queues;
	nwkstartup_link_info_t link;	
}__PACKED__ nwkstartup_data_t;

typedef struct {
	u8 event;		
	u8 link;
}__PACKED__ nwk_start_evnt_msg_t;

typedef struct {
	u8 event;		
	u8 link;
}__PACKED__ nwk_stop_evnt_msg_t;

typedef struct {
	u8 event;		
	u8 link;
}__PACKED__ nwk_start_ind_msg_t;

typedef struct {
	u8 event;		
	u8 link;
}__PACKED__ nwk_up_ind_msg_t;

typedef struct {
	u8 event;		
	u8 link;
}__PACKED__ nwk_dwn_ind_msg_t;


/****************************************************************************** 
  *	Externals
  ******************************************************************************/

extern nwkstartup_data_t nwkstartup_data;
extern gv701x_state_t nwkstartup_state;

/****************************************************************************** 
  *	Function Prototypes
  ******************************************************************************/

void GV701x_NwkInit(u8 app_id);
void GV701x_NwkStart(u8 link);
void GV701x_NwkStop(u8 link);
void GV701x_NwkStartupSM(gv701x_state_t* state); 
void GV701x_Nwk_CmdProcess(char* CmdBuf); 
void GV701x_NwkRxAppMsg(sEvent* event);

#endif /*GV701X_NWKSTARTUP_H*/
#endif /*NWKSTARTUP_APP*/