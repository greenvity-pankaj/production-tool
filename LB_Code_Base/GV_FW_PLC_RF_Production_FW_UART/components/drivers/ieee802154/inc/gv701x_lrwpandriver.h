/* ========================================================
 *
 * @file: gv701x_lrwpandriver.h
 * 
 * @brief: This file contains functon prototypes and configurations
 *         needed by the lrwpan driver
 *
 *  Copyright (C) 2010-2015, Greenvity Communications, Inc.
 *  All Rights Reserved
 *  
 * =========================================================*/
#ifdef LRWPAN_DRIVER_APP

#ifndef GV701x_LRWPANDRIVER_H
#define GV701x_LRWPANDRIVER_H

/****************************************************************************** 
  *	Includes
  ******************************************************************************/

#include "mac_msgs.h"
#include "mac_const.h"

/****************************************************************************** 
  *	Defines
  ******************************************************************************/
#ifdef OLD_SCAN
#define LRWPAN_CHANNEL								(0x11)
#else
#define LRWPAN_CHANNEL								(0x11)
#endif
#define LRWPAN_PANID								(0x1111)
#define LRWPAN_MAX_CHANNELS							(MAX_CHANNEL - MIN_CHANNEL)
#ifdef OLD_SCAN
#define LRWPAN_CHANNEL_MASK							(0x00020000UL)
#else
#define LRWPAN_CHANNEL_MASK							(0x00020000UL) //(0x06128800UL)
#endif
#define LRWPAN_MAX_SCAN_LIST						(5)
#define MAX_START_ATTEMPTS							(3)
#define MAX_ASSOC_ATTEMPTS							(2)
#define PERMIT_ASSOCIATION							(TRUE)
#define LRWPAN_PROFILE_TIME							(30000)
#define LRWPAN_START_TIME							(3000)
#define MAX_BCN_PAYLOAD								(10)

typedef enum 
{
	LRWPAN_IDLE = 0x00,
	LRWPAN_START,		
	LRWPAN_SCAN,
	LRWPAN_UP,
	LRWPAN_DOWN
}lrwpan_state_t;

typedef enum 
{	
	LRWPAN_IDLE_EVNT = 0,
	LRWPAN_START_EVNT, 
	LRWPAN_SCAN_EVNT,
	LRWPAN_CFG_EVNT,
	LRWPAN_STOP_EVNT
}lrwpan_drv_event_t;

typedef enum 
{	
	LRWPAN_IDLE_IND = 0,
	LRWPAN_UP_IND, 
	LRWPAN_DWN_IND,
	LRWPAN_SCAN_IND,
	LRWPAN_BCN_IND,
	LRWPAN_CFG_IND
}lrwpan_ind_event_t;

typedef enum 
{
	COORDINATOR = 0x00,
	ENDDEVICE,
	ROUTER,
}lrwpan_device_type_t;

typedef enum 
{
	LRWPAN_PROFILE_TIMEOUT_EVT = 0,		
	LRWPAN_START_TIMEOUT_EVT
}lrwpan_timer_event_t;

typedef struct
{
	uint8_t active;
	union 
	{
	    uint8_t ed_value[1];
	    pandescriptor_t PANDescriptor;		
	}val;
	uint8_t bcn_payload[MAX_BCN_PAYLOAD];
} __PACKED__ result_list_t;

typedef struct 
{
	uint8_t status;
	uint8_t ScanType;
	uint8_t ChannelPage;
	uint32_t UnscannedChannels;
	uint8_t ResultListSize;
	result_list_t list[LRWPAN_MAX_SCAN_LIST];	
}scan_result_t;

typedef struct 
{
	uint8_t app_id;
	gv701x_app_queue_t queues;
	struct 
	{
		u8 app_id;
		bool active;
		u8 type;
		u8 time;
		tTimerId timer;	
		u8 prev_state;
		uint32_t ch_mask;
		scan_result_t result;
	}scan;
	struct 
	{
		u8 app_id;
		bool active;
		tTime time;
		tTimerId timer;	
	}start;	
	struct 
	{
		u8 app_id;
		bool active;
		u8 params;
	}cfg;		
	uint8_t dev;
	uint8_t channel;
	uint16_t panid;
	uint8_t auto_request;
	uint8_t capability;
	uint16_t short_addr;
	uint64_t ieee_addr;	
	wpan_addr_spec_t coordinator;	
	tTimerId profile_timer;
	tTimerId start_timer;	
	uint8_t status;
}__PACKED__ lrwpan_db_t;

typedef struct 
{
	uint8_t event;			
}__PACKED__ lrwpan_start_evnt_msg_t;

typedef struct 
{
	uint8_t event;			
}__PACKED__ lrwpan_stop_evnt_msg_t;

typedef struct 
{
	uint8_t event;		
}__PACKED__ lrwpan_scan_evnt_msg_t;

typedef struct 
{
	uint8_t event;		
	uint8_t attribute;
	pib_value_t value;
}__PACKED__ lrwpan_cfg_evnt_msg_t;

typedef struct 
{
	uint8_t event;		
}__PACKED__ lrwpan_up_ind_t;

typedef struct 
{
	uint8_t event;		
}__PACKED__ lrwpan_dwn_ind_t;

typedef struct 
{
	uint8_t event;	
	uint8_t status;
}__PACKED__ lrwpan_scan_ind_t;

typedef struct 
{
	uint8_t event;	
}__PACKED__ lrwpan_bcn_ind_t;

typedef struct 
{
	uint8_t event;	
	uint8_t attribute;
	uint8_t status;
}__PACKED__ lrwpan_cfg_ind_t;

/****************************************************************************** 
  *	Externals
  ******************************************************************************/

extern lrwpan_db_t lrwpan_db;
extern gv701x_state_t lrwpan_state;

/****************************************************************************** 
  *	Function Prototypes
  ******************************************************************************/

void GV701x_LrwpanDriverInit (uint8_t app_id);
void GV701x_LrwpanDriverStart(void);
void GV701x_LrwpanTimerHandler(uint8_t* buf);
void GV701x_LrwpanDriverSM(gv701x_state_t* state);
void GV701x_LrwpanCmdProcess(char* CmdBuf); 
void GV701x_LrwpanDriverRxAppMsg(sEvent* event);
void lrwpan_set_shortaddr(uint16_t addr);
void lrwpan_SendData(uint16_t addr, uint8_t* msdu, u8 msduLength); 

#endif /*GV701x_LRWPANDRIVER_H*/
#endif /*LRWPAN_DRIVER_APP*/
