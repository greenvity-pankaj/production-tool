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
 
#ifndef UARTAPP_H
#define UARTAPP_H

/*****************************************************************************  
  *	Defines
  *****************************************************************************/
  
#define MAX_PKT_BUFFSIZE					(128)
#define ETHER_TYPE 						    (0x88e2)

/* MAC Address */
typedef union {
	u8 ui8[MAC_ADDR_LEN];
	u16 ui16[MAC_ADDR_LEN/2];
}__PACKED__ mac_addr_t;

/* Ethernet Header size (imported from hpgpdef.h)*/
#define ETH_HDR_SIZE 						(sizeof(sEth2Hdr))

/* Network Keys */
typedef struct {
	u8 nid[NID_LEN];
	u8 nmk[ENC_KEY_LEN];	
}nwk_keys_t;

/* Network Parameters */
typedef struct {
	nwk_keys_t key;	
}__PACKED__ nwkParams_t;

/* Node Data Structure */
typedef struct {
	/* MAC Address*/
	mac_addr_t macaddr;	  
	nwkParams_t nwk;
	/*Displays Network Stats perodically*/	
	tTimerId stats_timer;  
}__PACKED__ node_data_t;

/* State Machine */
typedef struct {
	u8 state;
	u8 event;
	u16 statedatalen;
	void* statedata;	
}node_state_t;

/* States */
typedef enum {
	IDLE_STATE = 1,
	ASSOCIATED_STATE = 2,
}node_state_types_t;

/* Events */
typedef enum {
	IDLE_EVNT = 0,		
	FW_READY_EVNT,
	HARDWARE_SPEC_GET_EVNT,		
	HARDWARE_SPEC_SET_EVNT,	
	DEVICE_STATS_EVNT,		
	NETWORK_SET_CNF,
	NETWORK_KEY_CNF,
	NETWORK_IND_EVNT,
	TIMER_EVNT,	
	CMD_EVNT,
}node_event_t;

/* Timer Events */
typedef enum {
	STATS_TIMER_EVNT = 1,
}timer_events_t;


/*****************************************************************************  
  *	Function prototypes
  *****************************************************************************/

void UartApp_Init(gv701x_aps_queue_t* dataQ);
u8 UartApp_Poll(void);

#endif // UARTAPP_H