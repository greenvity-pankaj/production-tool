#ifdef LLP_APP
#ifndef LLPAPP_FW_H
#define LLPAPP_FW_H

/****************************************************************************** 
  *	Defines
  ******************************************************************************/
#define MAX_PKT_BUFFSIZE							(512)

#ifdef ROUTE_APP
#define LLP_HDR_OFFSET								((sizeof(sEth2Hdr)) + sizeof(route_hdr_t))
#else
#define LLP_HDR_OFFSET								((sizeof(sEth2Hdr)))
#endif

/*LLP Frame Control defines*/
#define LLP_FRAME_BROADCAST							BIT(1)
#define LLP_FRAME_DIRECTION							BIT(2)
#define LLP_FRAME_RETRY								BIT(3)
#define LLP_FRAME_GROUPCAST							BIT(4)
#define LLP_FRAME_SOLICITED							BIT(5)
#define LLP_FRAME_DIRECTION_POS						(2)
#define LLP_FRAME_DIR_CONTROLLER_TO_LIGHT			(1)
#define LLP_FRAME_DIR_LIGHT_TO_CONTROLLER			(0)
#define LLP_UPDATE_SET								(1)
#define LLP_UPDATE_GET								(2)
#define LLP_MAX_GROUP_NAME							(32)


/*LLP Frame types*/
typedef enum 
{
	UPDATE_DEVICE_REQ = 0x00,
	UPDATE_DEVICE_RSP,
	UPDATE_ASYNC_DATA_REQ,
	CHANGE_GROUP,
	DEV_STATS_REQ,	
	DEV_STATS_RSP,	
}llp_sta_cmd_id;

/*Group Structures*/
typedef struct 
{
	u8 inuse;
	u16 groupid;				/* Group Identifier */	
	u8 max_devices;				/* Maximum number of devices in this group */
	u8 devices_inuse;			/* Number of devices using this group */
}__PACKED__ llp_sta_user_group_msg_t;

/*LLP common message frame control structure*/
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
}__PACKED__ llp_msghdr_fc_t;

/*LLP common message structure*/
typedef struct 
{
	llp_msghdr_fc_t fc;			/* Frame Control */
	u16 hpgp_group;				/* HPGP Group */
	u8 cmd_id;					/* Command identifier */
	u8 llp_group;				/* LLp Group */
	u8 rsvd;
}__PACKED__ llp_msg_req_t;	

/*LLP update request/response message structure*/
typedef struct 
{
	mac_addr_t mac_addr;
	u8 groupid;
	u8 action; 					/* SET/GET */
	u8 reqmsg; 					/* TRUE(Request) / FALSE(Response)*/
	u8 status_bits;
    u8 active:1;
    u8 reserved:7;
}__PACKED__ llp_sta_update_msg_t;

/*Fetch device statistics request message*/
typedef struct 
{
	mac_addr_t mac_addr;
	u8 action;			
	u8 reserved;
}__PACKED__ llp_devstats_req_msg_t;

/*Fetch device statistics response message*/
typedef struct 
{
	mac_addr_t mac_addr;
	u8 action;			
	u8 reserved;
}__PACKED__ llp_devstats_rsp_msg_t;

/* LLP group update request */
typedef struct 
{
	mac_addr_t mac_addr;
	u8 llp_grp;				/* Group Id*/	
	u8 group_type;
	u8 group_name[LLP_MAX_GROUP_NAME];
}__PACKED__ llp_sta_group_update_req_t;

/* LLP frame statistics */
typedef struct	
{	
	u32 updtreq;
	u32 updtrsp;	
	u32 updtrspbt;
	u32 asyncrsp;
	u32 drop;
}llp_sta_frame_stats_t;

/*Slave Data Structure*/
typedef struct 
{
	u8 app_id;
	u8 hpgp_grp;
	u8 llp_grp;
	mac_addr_t macaddr;
	mac_addr_t ccomacaddr;
	u16 nwk_addr;
	u8 state;
    u8 active:1;
    u8 reserved:7;
	u8* dev_type;
	u8* dev_subtype;
	llp_sta_frame_stats_t stats;	
}__PACKED__ node_data_t;

/*LLP States*/
typedef enum
{
	NODE_IDLE_STATE = 0,
	NODE_ACTIVE_STATE,	
	NODE_INACTIVE_STATE,	
}node_state_types_t;

typedef enum 
{
	STATS_TIMER_EVNT,		
}node_timer_event_t;

/****************************************************************************** 
  *	Externals
  ******************************************************************************/

extern node_data_t node_data;  	

/****************************************************************************** 
  *	Function Prototypes
  ******************************************************************************/

void LlpApp_Rx(u8* buf, u8 len);
void LlpApp_DispStats(void);
void LlpApp_CmdProcess(char* CmdBuf) ;

#endif /*LLPAPP_FW_H*/
#endif /*LLP_APP*/
