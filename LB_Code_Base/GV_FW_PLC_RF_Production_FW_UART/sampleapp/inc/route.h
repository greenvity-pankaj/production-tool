/* ========================================================
 *
 * @file:  route.h
 * 
 * @brief: This file hold the routing prototypes
 *		   node with the Host
 *      
 *  Copyright (C) 2010-2015, Greenvity Communications, Inc.
 *  All Rights Reserved
 *  
 * =========================================================*/
#ifdef ROUTE_APP

#ifndef ROUTE_H
#define ROUTE_H

/****************************************************************************** 
  *	Defines
  ******************************************************************************/
  
enum 
{
	DIRECTION_UP = 0x00,
	DIRECTION_DOWN = 0x01,
};

enum 
{
	FINDPARENT_REQ = 0x00,
	FINDPARENT_RSP = 0x01,
	REGISTRATION_REQ = 0x02,
	REGISTRATION_RSP = 0x03,
	REGISTRATION_CNF = 0x04,	
	REREGISTRATION_REQ = 0x05,	
	SOURCE_ROUTE_REQ = 0x06,
	SOURCE_ROUTE_RSP = 0x07,	
	PROBE_REQ = 0x08,
	PROBE_RSP = 0x09,
	APP_FRAME = 0x0A,
	GET_PARAM_REQ = 0x0B,
	GET_PARAM_RSP = 0x0C,
	GET_PARAM_IND = 0x0D,
};	

#define POWER_LINE 										BIT(0)
#define WIRELESS 										BIT(1)

typedef struct 
{
	u16 control_bits;
}__PACKED__ route_fc_t;

typedef struct 
{
	route_fc_t fc;
	u16 parent;
	u16 target;
	u16 rank;
}__PACKED__ route_hdr_t;

/*OTA Frame defines*/
#define RHDR_FC_DIR_MASK								(0x0001)
#define RHDR_FC_CMDID_MASK								(0x001E)
#define RHDR_FC_TOROOT_MASK								(0x0020)
#define RHDR_FC_ONEHOP_MASK								(0x0040)
#define RHDR_FC_BRIDGE_MASK								(0x0080)
#define RHDR_FC_RESET_MASK								(0x0200)
#define RHDR_FC_DIR_SHIFT								(0)
#define RHDR_FC_CMDID_SHIFT								(1)
#define RHDR_FC_TOROOT_SHIFT							(5)
#define RHDR_FC_ONEHOP_SHIFT							(6)
#define RHDR_FC_BRIDGE_SHIFT							(7)
#define RHDR_FC_RESET_SHIFT								(9)

#define RHDR_GET_DIR(x)		((le16_to_cpu(x->fc.control_bits) & RHDR_FC_DIR_MASK) >> RHDR_FC_DIR_SHIFT)
#define RHDR_GET_CMDID(x)	((le16_to_cpu(x->fc.control_bits) & RHDR_FC_CMDID_MASK) >> RHDR_FC_CMDID_SHIFT)
#define RHDR_GET_ONEHOP(x)	((le16_to_cpu(x->fc.control_bits) & RHDR_FC_ONEHOP_MASK) >> RHDR_FC_ONEHOP_SHIFT)
#define RHDR_GET_BRIDGE(x)	((le16_to_cpu(x->fc.control_bits) & RHDR_FC_BRIDGE_MASK) >> RHDR_FC_BRIDGE_SHIFT)
#define RHDR_GET_RESET(x)	((le16_to_cpu(x->fc.control_bits) & RHDR_FC_RESET_MASK) >> RHDR_FC_RESET_SHIFT)

#define RHDR_SET_DIR(x) 	(cpu_to_le16(x << RHDR_FC_DIR_SHIFT))
#define RHDR_SET_CMDID(x) 	(cpu_to_le16(x << RHDR_FC_CMDID_SHIFT))
#define RHDR_SET_TOROOT(x) 	(cpu_to_le16(x << RHDR_FC_TOROOT_SHIFT))
#define RHDR_SET_ONEHOP(x) 	(cpu_to_le16(x << RHDR_FC_ONEHOP_SHIFT))
#define RHDR_SET_BRIDGE(x) 	(cpu_to_le16(x << RHDR_FC_BRIDGE_SHIFT))
#define RHDR_SET_RESET(x) 	(cpu_to_le16(x << RHDR_FC_RESET_SHIFT))


#ifdef ROUTE_RECOVERY
#define ROUTE_RECOVERY_TIME_BOOTUP 1800000 
#define ROUTE_RECOVERY_TIME_LINKFAIL 300000
#endif

typedef enum 
{
	ROUTE_IDLE_EVNT = 0,		
	ROUTE_START_EVNT,		
	ROUTE_STOP_EVNT,	
	ROUTE_UPDATE_ADDR_EVNT,	
	ROUTE_UPDATE_ROUTE_EVNT
}route_event_t;

typedef enum 
{
	ROUTE_IDLE_IND = 0,		
	ROUTE_UP_IND,		
	ROUTE_DWN_IND,
	ROUTE_DISC_IND
}route_ind_t;

typedef enum 
{
	ROUTE_REASON_NONE = 0,		
	ROUTE_REASON_MANUAL,			
	ROUTE_REASON_DISCFAIL,
	ROUTE_REASON_ROUTEFAIL,		
}route_reason_t;

typedef struct 
{
	u8 event;		
	u8 link;
	u8 assignparent;
}__PACKED__ route_start_evnt_msg_t;

typedef struct 
{
	u8 event;		
	u16 addr;
}__PACKED__ route_updt_addr_evnt_msg_t;

typedef struct 
{
	u8 event;		
}__PACKED__ route_updt_route_evnt_msg_t;

typedef struct 
{
	u8 event;		
	u8 link;
}__PACKED__ route_stop_evnt_msg_t;

typedef struct 
{
	u8 event;		
	u8 link;
}__PACKED__ route_up_ind_msg_t;

typedef struct 
{
	u8 event;		
	u8 link;
	u8 reason;
}__PACKED__ route_disc_ind_msg_t;

typedef struct 
{
	u8 event;		
	u8 link;
	u8 reason;
}__PACKED__ route_dwn_ind_msg_t;


/****************************************************************************** 
  *	External Data
  ******************************************************************************/
extern u8 route_app_id;
extern gv701x_app_queue_t route_queues;
extern gv701x_state_t route_state;

/****************************************************************************** 
  *	Function Prototypes
  ******************************************************************************/

void RouteApp_Init(u8 app_id); 
void RouteApp_Poll(void);
void RouteApp_StateMachine(gv701x_state_t* state); 
void RouteApp_TimerHandler(u8* buf);
bool route_handle_rx_from_ll (u8 *frm, u8 len, u8 link, u8 lqi);
bool route_send_to_ll (u8 *buff, u8 len, u8 frametype, u8 ether_hop_en);
bool route_add_neighbor (u16 neigh_addr, u8* ieee_addr, u16 rank, u8 link, 
			u16 metric, u8 rssi, u32 bcn_rx, u8 lqi, u16 parent_addr, u8 ch, u8 ed);			
void RouteApp_RxAppMsg(sEvent* event);

#endif /*ROUTE_H*/
#endif /*ROUTE_APP*/

