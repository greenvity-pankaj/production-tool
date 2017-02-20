/* ========================================================
 *
 * @file:  gv701x_hpgpdriver.h
 * 
 * @brief: This file contains all defines needed for the HPGP Driver
 *
 *  Copyright (C) 2010-2015, Greenvity Communications, Inc.
 *  All Rights Reserved
 *  
 * =========================================================*/
 
#ifdef HPGP_DRIVER_APP

#ifndef GV701X_HPGPDRIVER_H
#define GV701X_HPGPDRIVER_H

/****************************************************************************** 
  *	Defines
  ******************************************************************************/
  
#define OEM_LIST								(1)

/* Network Keys */
typedef struct 
{
	u8 nid[NID_LEN];
	u8 nmk[ENC_KEY_LEN];	
}hpgp_nwk_keys_t;

/* Application Info*/
typedef struct 
{
	struct
	{
		u8 ouid[OUID_LEN];
	}oem[OEM_LIST];
	u8 byte_arr[VENDOR_SPEC_FIELD_LEN];	
}hpgp_nwk_app_info_t;

/* Network Parameters */
typedef struct 
{
	u8 line_mode;
	u8 txpower_mode;
	u8 dc_frequency;
	u8 tei;
	eDevMode role;	
	u16 net_id;
	hpgp_nwk_keys_t key;	
	hpgp_nwk_app_info_t app_info;
}__PACKED__ hpgp_nwk_params_t;

typedef struct 
{
	u8 mac_addr[MAC_ADDR_LEN];
	hpgp_nwk_params_t nwk;			
}__PACKED__ hpgp_params_t;

/*Slave Data Structure*/
typedef struct 
{
	hpgp_params_t params;
	struct {
		u8 entries;
		hostEventScanList list[MAX_NETWORK_LIST];	
	}netlist;
}hpgp_nwk_data_t;

typedef enum 
{
	HPGPDRV_INIT = 0,
	HPGPDRV_START,		
	HPGPDRV_UP,
	HPGPDRV_DOWN	
}hpgp_drv_state_types_t;

typedef enum 
{
	HPGPDRV_IDLE_EVENT = 0,	
	HPGPDRV_START_EVNT,
	HPGPDRV_SCAN_EVNT,
	HPGPDRV_STOP_EVNT
}hpgp_drv_event_t;

typedef enum 
{
	HPGPDRV_IDLE_IND = 0,	
	HPGPDRV_UP_IND,		
	HPGPDRV_DWN_IND,
	HPGPDRV_SCAN_IND	
}hpgp_drv_ind_t;

typedef enum 
{
	HPGPDRV_SCAN_TIMEOUT_EVT = 0,	
	HPGPDRV_START_TIMEOUT_EVT
}hpgp_drv_timer_event_t;

typedef struct 
{
	u8 app_id;
	gv701x_app_queue_t queues;
	struct 
	{
		u8 app_id;
		bool active;
		tTime time;
	}scan;
	struct 
	{
		u8 app_id;
		bool active;
		tTime time;
		tTimerId timer;	
	}start;		
	gv701x_state_t state;	
}__PACKED__ hpgp_drv_data_t;

typedef struct 
{
	u8 event;		
}__PACKED__ hpgp_drv_start_evnt_msg_t;

typedef struct 
{
	u8 event;		
}__PACKED__ hpgp_drv_stop_evnt_msg_t;

typedef struct 
{
	u8 event;		
}__PACKED__ hpgp_drv_scan_evnt_msg_t;

typedef struct 
{
	u8 event;		
}__PACKED__ hpgp_drv_scan_ind_msg_t;

typedef struct 
{
	u8 event;		
	nwIdState_e state;	
	u8 reason;	
}__PACKED__ hpgp_drv_up_ind_msg_t;

typedef struct 
{
	u8 event;		
	nwIdState_e state;	
	u8 reason;		
}__PACKED__ hpgp_drv_dwn_ind_msg_t;

/****************************************************************************** 
  *	Externals
  ******************************************************************************/

extern u8 cco_nid[NID_LEN];
extern u8 nmk[ENC_KEY_LEN];

extern hpgp_nwk_data_t hpgp_nwk_data;
extern hpgp_drv_data_t hpgp_drv_data;

/****************************************************************************** 
  *	Function Prototypes
  ******************************************************************************/

void GV701x_HPGPDriverInit(u8 app_id);
void GV701x_HPGPSetHwspec(u8* mac_addr, u8 line_mode, u8 txpower_mode, u8 er_mode, u8 dc_frequency);
void GV701x_HPGPGetHwspec(void);
void GV701x_HPGPReStartNwk(void);
void GV701x_HPGPGetDevStats(void);
void GV701x_HPGPSetPsAvln(u8 mode);
void GV701x_HPGPSetPsSta(u8 mode, u8 awd, u8 psp);
void GV701x_HPGPGetPeerInfo(void);
void GV701x_HPGPSetNetId(u8* nmk, u8* nid);
void GV701x_HPGPNetExit(void);
void GV701x_HPGPStartNwk(u8 netoption, u8* nid);
void GV701x_HPGPScanNetwork(tTime scantime);
void GV701x_HPGPNwkVendorFieldAccess(u8 action, u8* ouid, u8* vendor_info);
void GV701x_HPGPDriverSM(gv701x_state_t* state);
void GV701x_HPGPDriverTimerHandler(u8* buf);
void GV701x_HPGPDriverRxAppMsg(sEvent* event);
void GV701x_HPGPDriver_CmdProcess(char* CmdBuf);

#endif /*GV701X_HPGPDRIVER_H*/
#endif /*HPGP_DRIVER_APP*/
