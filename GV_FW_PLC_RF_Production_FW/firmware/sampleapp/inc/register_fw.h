/* ========================================================
 *
 * @file:  register_fw.h
 * 
 * @brief: This file handles the registration process of the 
 *		   node with the Host
 *      
 *  Copyright (C) 2010-2015, Greenvity Communications, Inc.
 *  All Rights Reserved
 *  
 * =========================================================*/

#ifndef REGSTER_FW_H
#define REGSTER_FW_H

/****************************************************************************** 
  *	Defines
  ******************************************************************************/

/*Registration control defines*/
#define REG_REQ_BOTH_PRESENT					BIT(2)
#define REGISTER_MAX_PKT_BUFFSIZE				(512)

#define REGISTER_BASE_TIMEOUT					(2000) 	/* milli seconds */
#define REGISTER_TIMEOUT_MAX_EXPONENT			(1) 	/* interger value only*/
#define REGISTER_TIMEOUT_EXPONENT				(1) 	/* interger value only*/

#define MAX_REGISTER_RETRY_COUNT				(10)

/*Register States*/
typedef enum 
{
	REGISTER_IDLE_STATE = 0,
	REGISTER_UNREGISTERED_STATE,
	REGISTER_REGISTERED_STATE,
}register_state_types_t;

typedef enum 
{
	REGISTRATION_TIMER_EVNT,	
}register_timer_event_t;

/* Register statistics */
typedef struct	
{
	u16 regreq;	
	u16 regrsp;
	u16 regcnf;
	u16 reregreq;	
}register_stats_t;

typedef struct
{
	u16 seq_num;
	u16 control_bits;			/* Broadcast, Multicast */
								/* Bit 0 -Single llp_group multicast 
								 * Bit 1 - Broadcast 
								 * Bit 2 - Direction (1 = Controller to light 
								   device, 0 = Light device to controller)
								 * Bit 3 - Retry Packet
								 * Bit 4 -Groupcast	(all llp_groups or hpgp_groups)							 
								 * Bit 5 to 14 -Reserved								 
								 */
}__PACKED__ register_hdr_t;

/*Register Data Structure*/
typedef struct 
{
	u16 handler_id;
	u16 registration_cnt;		/*Number of registration attempts*/
	tTime registration_time;	/*Current registration retry interval (milliseconds)*/	
	tTimerId register_timer;	/*Registration timer*/
	u8 tei;
	mac_addr_t macaddr;	
	union 
	{
		u8 addr_8bit[1];		
		u16 addr_16bit;				
	}nwk_addr;
	u8 reg_req_retry_cnt;
	register_stats_t stats;		
}__PACKED__ register_data_t;

/*Registration request message structure*/
typedef struct 
{
	register_hdr_t fc;
	u8 flags;					/* Bit 0 - Only id used */
								/* Bit 1 - Only MAC Address used */
								/* Bit 2 - Bth MAC and ID used */
	mac_addr_t mac_addr;
	u8 nid[NID_LEN];
	u8 tei;
}__PACKED__ register_req_t;

/*Registration response message structure*/
typedef struct 
{
	register_hdr_t fc;
	u16 handler_id;	 			/* Handler sent in the request frame */
	mac_addr_t mac_addr; 		/* MAC address assigned by host or received from the 
								   slave */
	u16 range_size;				/* Perodic Update Range size */										
	u16 max_slave_lifetime;		/* Maximum time slave entry exists */
	u8 status;					/* SUCCESS/FAIL */
	u16 nwkaddr;
}__PACKED__ register_rsp_t;

/*Registration confirm message structure*/
typedef struct 
{
	register_hdr_t fc;
	mac_addr_t mac_addr;
	u8 status;					/* SUCCESS/FAIL */
	u8 dev_type;	
	u8 sub_type;	
	u16 nwkaddr;	
}__PACKED__ register_cnf_t;

/*Re-registration request message structure*/
typedef struct 
{
	register_hdr_t fc;
	u8 status;	
	u16 backoff;	
	mac_addr_t mac_addr;
}__PACKED__ reregister_req_t;

typedef struct 
{
	u8 cmd;
	u8 tei;
	u8 mac_addr[MAC_ADDR_LEN];
}__PACKED__ register_scbset_event_t;

/****************************************************************************** 
  *	External Data
  ******************************************************************************/

extern register_data_t register_data;

/****************************************************************************** 
  *	Function Prototypes
  ******************************************************************************/

void register_display_stats(void);
void register_rx_rsp(u8* frm, u8 len); 
void register_rx_rereq(u8* frm, u8 len); 
void RegisterApp_CmdProcess(char* CmdBuf);

#endif /*REGSTER_FW_H*/
