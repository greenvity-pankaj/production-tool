#ifdef ROUTE_APP

#ifndef ROUTE_FW_H
#define ROUTE_FW_H

/*LED configuration*/
#define RED_LED									(GP_PB_IO4_WR)
#define GREEN_LED								(GP_PB_IO5_WR)
#define LED_ON									((u8)~0)
#define LED_OFF									((u8)~LED_ON)

/*Routing tables defines*/
#define MAX_NEIGHBOUR_DEVICES					(15)

/*Neighbor table defines*/
#define ROUTE_LINK_REF_LEN						(15)
#define MAX_NEIGHBOUR_LIFETIME					(5)

/*Rank and Meteric defines*/
#define ALPHA									(10)
#define INFINITE_RANK							(0xFFFF)
#define MIN_HOP_RANL_INCR						(255)
#define RTR_LINK_ESTIMATE_UNIT          		(8)
#define INITIAL_LINK_ESTIMATE 					(16)
#define RTR_LINK_ESTIMATE_ALPHA 				((3 * (RTR_LINK_ESTIMATE_UNIT)) / 8)
#define RTR_MIN_HOPRANKINC         		 		(256)
#define MAX_PATH_COST							(100)

/*Route table defines*/
#define MAX_ROUTE_TABLE_ENTRIES					(50)

/*Discover defines*/
#define ROUTE_DISC_MAX_HIGH_CNT					(4)
#define ROUTE_DISC_MAX_LOW_CNT					(4)
#define ROUTE_DISCOVER_PERIOD_HIGH				(2000)
#define ROUTE_DISCOVER_PERIOD_LOW				(1000)

/*Sroute defines*/
#define MAX_SROUTE_PERIODIC_INTERVAL_HIGH		(30000)
#define MAX_SROUTE_PERIODIC_INTERVAL_LOW		(2000)
#define MAX_SROUTE_FAIL_COUNT					(5)
#define MAX_FIRST_SROUTE_FAIL_COUNT				(3)


/*Poll timer defines*/
#define ROUTE_SYS_POLL_TIMEOUT_LHIGH			(300)
#define ROUTE_SYS_POLL_TIMEOUT_LLOW				(1000)

/*Probe defines*/
#define MAX_PROBE_INTERVAL						(30000)
#define MAX_PROBE_RETRANS_INTERVAL				(3000)
#define MAX_PROBE_FRAMES_PER_DEVICE				(10)

/*OTA Frame defines*/
#define MAX_ROUTE_FRM_SIZE						(100)
#define BROADCAST_NET_ADDR						(0xFFFF)
#define MAX_PROBE_RETRANSMIT					(10)

/*
 * The rank must differ more than 
 * 1/PARENT_SWITCH_THRESHOLD_DIV in order
 * to switch preferred parent.
 */
#define PARENT_SWITCH_THRESHOLD_DIV				(2)

/* Rank of a root node. */
#define ROOT_RANK()            					(RTR_MIN_HOPRANKINC)
/*
 * The ETX in the metric container is 
 * expressed as a fixed-point value 
 * whose integer part can be obtained 
 * by dividing the value by 
 * RPL_DAG_MC_ETX_DIVISOR.
 */
#define RPL_DAG_MC_ETX_DIVISOR					(128)


/* ETX_DIVISOR is the value that a 
 * fix-point representation of the ETX 
 * should be divided by in order to obtain 
 * the integer representation. 
 */
#define NEIGHBOR_INFO_ETX_DIVISOR       		(16)

/* Macros for converting between a 
 * fix-point representation of the ETX 
 * and a integer representation. 
 */
#define NEIGHBOR_INFO_ETX2FIX(etx)    			((etx) * NEIGHBOR_INFO_ETX_DIVISOR)
#define NEIGHBOR_INFO_FIX2ETX(fix)    			((fix) / NEIGHBOR_INFO_ETX_DIVISOR)

#define INITIAL_LINK_METRIC						(NEIGHBOR_INFO_ETX2FIX(5))

#define ROUTE_RSSI_THRESHOLD					(138)
#define ROUTE_BCNRX_THRESHOLD					(80)
#define ROUTE_LQI_THRESHOLD						(140)
#define ROUTE_ED_THRESHOLD						(0xE0)
#define ROUTE_LQI_DIFF							(100)
#define ROUTE_ED_DIFF							(0xA0)
#define ROUTE_PER_THRESHOLD						((ROUTE_DISC_MAX_LOW_CNT + ((ROUTE_DISC_MAX_LOW_CNT%2) ? (1):(0)))/2)

enum 
{
	DEV_BRIDGING,
	DEV_SLAVE,
	DEV_MASTER
};

typedef enum 
{
	ROUTE_INIT = 0,		
	ROUTE_START,		
	ROUTE_DISCOVER,			
	ROUTE_COMPLETE,	
}route_state_types_t;

typedef enum 
{
	ROUTE_DISCOVER_TIMEOUT_EVT = 0,	
	PROBE_TIMEOUT_EVT,	
	ROUTE_SROUTE_PERIODIC_TIMEOUT_EVT,
	ROUTE_PROBE_RE_TIMEOUT_EVT,
	ROUTE_SYS_POLL_TIMEOUT_EVT,
	ROUTE_REC_TIMEOUT_EVT,	
}route_timer_event_t;

/* Neighbor Table */
typedef struct 
{
	u8 valid;
	u8 link;
	u16 addr;
	u16 rank;
	u8 timeout;	
	u16 etx;
	u16 path_metric;
	u8 rssi;
	u32 bcn_rx;
	u8 cnt;
	u8 lqi;	
	u8 ch;
	u8 ed;	
	u32 probe_count;
	u8 probe_mark;
	u16 parent_addr;
	u8 ieee_addr[IEEE_MAC_ADDRESS_LEN];		
}__PACKED__ neighbor_info_t;


/* Register statistics */
typedef struct	
{
	u32 fpreq_tx;
	u32 fpreq_rx;
	u32 fprsp_tx;
	u32 fprsp_rx;		
	u32 preq_tx;
	u32 preq_rx;	
	u32 prsp_tx;	
	u32 prsp_rx;	
	u32 sroute_rx;	
	u32 sroute_tx;	
	u32 fwdup;
}route_stats_t;

typedef struct
{
	/*Probe timer*/ 
	tTimerId prd_timer;		
	/*Probe Retransmit Timer*/	
	tTimerId retrans_timer;
	u8 retrans_index;		
	u8 retrans_count;
}__PACKED__ probe_info_t;

typedef struct 
{
	struct 
	{
		u8 cnt;
		u8 active;
		u8 assignparent;
	}powerline;		
	struct 
	{
		u8 cnt;
		u8 active;
		u8 assignparent;
	}wireless;		
	u8 link;
	u8 assignparent;
}discovery_params_t;

/* Route Info */
typedef struct
{
	u8 unreachable;
	u16 rank;
	u16 zid;	
	u8 ieee_addr[IEEE_MAC_ADDRESS_LEN];
	neighbor_info_t *parent;
	tTimerId discovery_timer;	
	tTimerId sroute_fail_cntdwn;
	tTimerId route_rdy_timer;
	tTimerId sroute_timer;
	tTimerId rec_timer;	
	probe_info_t probe;
	u8 root_ieee_addr[IEEE_MAC_ADDRESS_LEN];
	u8 disc_cnt;
	route_stats_t stats;
	tTimerId sys_poll_timer; 
	discovery_params_t disc_params;
	u8 rssi_threshold;
	u32 bcn_threshold;
	u8 lqi;
	u8 wireless_ch;		
	u8 lqi_threshold;
	u8 ed_threshold;
	u8 lqi_diff;
	u8 ed_diff;		
	struct 
	{
		struct 
		{
			u8 app_id;
			bool active;
		}wireless;
		struct 
		{
			u8 app_id;
			bool active;
		}power_line;		
	}start;
	u8 route_sel_active;
	u8 roaming_enable;
}__PACKED__ route_info_t;

/* Route Table */
typedef struct
{
	u8 valid;
	u16 target;
	u8 ieee_address[MAC_ADDR_LEN];	
	u8 parent_idx;
	u8 link;
	u8 timeout;
}__PACKED__ route_table_t;

/* Frame Request */
typedef struct 
{
	route_hdr_t rhdr;	
	u8 ieee_address[IEEE_MAC_ADDRESS_LEN];	
}__PACKED__ fpreq_t;

typedef struct 
{
	route_hdr_t rhdr;	
	u16 parent;
	u16 path_metric;
	u8 ieee_address[IEEE_MAC_ADDRESS_LEN];
}__PACKED__ fprsp_t;

typedef struct 
{
	route_hdr_t rhdr;
	u8 ieee_address[IEEE_MAC_ADDRESS_LEN];
}__PACKED__ sroute_req_t;

typedef struct
{
	route_hdr_t rhdr;
}__PACKED__ sroute_rsp_t;

typedef struct 
{
	route_hdr_t rhdr;
}__PACKED__ probe_t;

#ifdef BRIDGE
#define RTR_DEVICE_TYPE							DEV_BRIDGING
#else
#define RTR_DEVICE_TYPE							DEV_SLAVE
#endif


typedef struct
{
	u8 device_type;
}__PACKED__ route_device_profile_t;


/****************************************************************************** 
  *	External Data
  ******************************************************************************/
extern route_info_t route_info;
extern u32 totalTx;
extern u32 totalRx;
extern route_info_t route_info;

/****************************************************************************** 
  *	Function Prototypes
  ******************************************************************************/
 
u8 route_assign_best_parent(neighbor_info_t* nbr);
void RouteApp_CmdProcess(char* CmdBuf) ;
void RouteApp_Display_Help(void);
void RouteApp_DispStats(void);
void RouteApp_DispRouteTable(void);
void RouteApp_DispNeighTable(void);
bool route_update_neighbor_ed(u8 ch, u8 ed);
bool route_is_neightable_empty(u8 link);
neighbor_info_t* route_select_best_neighbor(u8 link);

#endif /*ROUTE_FW_H*/ 
#endif /*ROUTE_APP*/
