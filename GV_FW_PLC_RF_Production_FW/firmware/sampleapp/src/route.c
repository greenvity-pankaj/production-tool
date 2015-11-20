/* ========================================================
 *
 * @file:  route.c
 * 
 * @brief: This file handles the registration process of the 
 *		   node with the Host
 *      
 *  Copyright (C) 2010-2015, Greenvity Communications, Inc.
 *  All Rights Reserved
 *  
 * =========================================================*/

/****************************************************************************** 
  *	Includes
  ******************************************************************************/

#ifdef ROUTE_APP

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "gv701x_includes.h"
#include "route.h"
#include "route_fw.h"
#ifdef REGISTER_APP
#include "register.h"
#include "register_fw.h"
#endif
#ifdef LLP_APP
#include "llpapp.h"
#include "llpapp_fw.h"
#endif
#ifdef NWKSTARTUP_APP
#include "gv701x_nwkstartup.h"
#endif
#ifdef LRWPAN_DRIVER_APP
#include "gv701x_lrwpandriver.h"
#endif
#ifdef HPGP_DRIVER_APP
#include "gv701x_hpgpdriver.h"
#endif
#ifdef RTOPO_APP
#include "route_topo_fw.h"
#include "route_topo.h"
#endif

/****************************************************************************** 
  *	Global Data
  ******************************************************************************/
/*Route State machine*/
gv701x_state_t route_state;

u8 route_app_id;
gv701x_app_queue_t route_queues;	

/* Routing info of this device */
route_info_t route_info;

/* Neighbouring or Potential Parent table */
neighbor_info_t route_neighbor[MAX_NEIGHBOUR_DEVICES];

/* Routing table */
static route_table_t route_table[MAX_ROUTE_TABLE_ENTRIES]; 

/* Route Device Profile */
route_device_profile_t route_device_profile;

u32 totalTx = 0;
u32 totalRx = 0;

/****************************************************************************** 
  *	External Data
  ******************************************************************************/
extern rtopo_data_t	rtopo_data;

/****************************************************************************** 
  *	External Function Prototypes
  ******************************************************************************/
extern u8 CRM_AddScbVendorField(u8 *macAddr, u16 vendor_field);
extern eStatus CRM_GetMacAddr(u8 tei, u8 *macAddr);
extern u8 CRM_FindScbVendorField(u16 vendor_field, u8 *macAddr);
#define GV701x_getHPGPDeviceMAC CRM_GetMacAddr

/****************************************************************************** 
  *	Function Prototypes
  ******************************************************************************/

static u8 route_handle_received_frm (u8 *frm, u8 len, u8 link, u8 lqi);
static void route_handle_fpreq (u8 *frm, u8 link, u8 lqi) ;
static void route_handle_fprsp (u8 *frm, u8 link, u8 lqi); 
static void route_handle_sroute_req (u8 *frm, u8 len, u8 link); 
static void route_handle_sroute_rsp (u8 *frm, u8 len, u8 link); 
static void route_handle_probe_req (u8 *frm, u8 len, u8 link);
static void route_handle_probe_rsp (u8 *frm, u8 len, u8 link);
static bool route_create_discover_fpreq_frame (u16 target, u8* wir_mac_addr, u16 wir_addr, 
												u8* pwr_mac_addr, u16 pwr_addr, u8 link);
static bool route_create_and_send_fprsp_frame (u8 *dst_ieee_addr, u16 target, u8 link);
static bool route_create_and_send_sroute_frame (u16 target);
static bool route_create_and_send_probe_rsp (u8 *ieee_addr, u16 dest, u8 rank, u8 len, u8 link);
static bool route_create_and_send_probe_req (neighbor_info_t *nbr, u8 index);
static bool route_get_my_header (route_hdr_t *hdr, u16 destn, u8 frametype, u8 direction);
static route_table_t *route_get_next_hop (u16 target);
static route_table_t * route_get_route (u16 target);
static u8 route_get_neighbor_index(u8* ieee_addr, u8 link);
static neighbor_info_t *route_get_neighbor_link(u8 link);
static bool route_remove_route (u16 target);
static bool route_remove_neighbor(u16 neigh_addr, u8 link);
static bool route_remove_neighbor_using_index(u8 idx);
static bool route_remove_link_neighbor (u8 link);
static bool route_flush_nwktables (void);
static bool route_add_route (u16 target, u16 parent, u8 *ieee_addr, u8 link_type);
neighbor_info_t* route_select_best_neighbor(u8 link);
static neighbor_info_t *route_get_neighbor (u16 neigh_addr);
static bool route_send_frm (u8 *dstn_ieee_addr, u8 *src_ieee_addr, u16 dest, u16 src, u8 dir, 
								u8 *buff, u8 len, u8 link, u8 rhdr_present, u8 forward);
static u8 route_send_to_ul (u8* frm, u8 len, u8 link);
static void route_periodic_probing (void); 
static void route_rtable_aging_timer (void); 
//u8 RouteApp_LedControl(u8 statemode_en);
void RouteApp_StartDiscovery(u8 link, u8 assignparent, u8 cont_disc); 
void RouteApp_SendEvent(u8 ind, u8 link, u8 reason);



/******************************************************************************
 * @fn      RouteApp_Init
 *
 * @brief   Initializes the Routing Layer
 *
 * @param   app_id - application identification number
 *
 * @return  none
 */

void RouteApp_Init(u8 app_id)
{
	u8* macaddr;
	memset (&route_info, 0x00, sizeof (route_info_t));
	memset(&route_state, 0x00, sizeof(gv701x_state_t));
	
	/*Record the applications id,will 
	 be used while allocating timers*/
	route_app_id = app_id;
	SLIST_Init(&route_queues.appRxQueue);

	FM_Printf(FM_USER, "\nInit RouteApp (app id)", app_id);
	
	macaddr = GV701x_ReadMacAddress();
	memcpy((u8*)&route_info.ieee_addr, (u8*)macaddr, MAC_ADDR_LEN);
#if 0	
	FM_HexDump(FM_APP, "MAC: ", (u8*)&route_info.ieee_addr, MAC_ADDR_LEN);			
#endif

	/*Initialize the database*/
	memset(&route_table, 0x00, MAX_ROUTE_TABLE_ENTRIES*(sizeof(route_table_t))); 
	memset (route_neighbor, 0x00, MAX_NEIGHBOUR_DEVICES*(sizeof (neighbor_info_t)));		
	route_device_profile.device_type = RTR_DEVICE_TYPE;	

	/*Initialize the state machine*/
	route_state.state = ROUTE_INIT;
	route_state.event = ROUTE_IDLE_EVNT;	
	route_state.statedata = NULL;	
	route_state.statedatalen = 0;	
	route_info.unreachable = TRUE;

	/*Initalize the different configurations*/
	route_info.rssi_threshold = ROUTE_RSSI_THRESHOLD;
	route_info.bcn_threshold = ROUTE_BCNRX_THRESHOLD;
	route_info.lqi_diff = ROUTE_LQI_DIFF;
	route_info.ed_diff = ROUTE_ED_DIFF;	
	route_info.lqi_threshold = ROUTE_LQI_THRESHOLD;	
	route_info.ed_threshold = (0xFF - ROUTE_ED_THRESHOLD);	

#if 0
	FM_Printf(FM_APP, "\nEd Thresh: %bx %bu", route_info.ed_threshold, route_info.ed_threshold);
#endif
	route_info.sroute_fail_cntdwn = MAX_SROUTE_FAIL_COUNT;					
	route_info.route_sel_active = TRUE;

	/* Configures GP_PB_IO4 & 5 as output pins for LED indication */
#ifdef HQ_LINK_TEST
	GV701x_GPIO_Config(WRITE_ONLY, RED_LED | GREEN_LED);
	GV701x_GPIO_Write(GREEN_LED, LED_OFF);
	GV701x_GPIO_Write(RED_LED, LED_OFF);			
#endif
	/* Allocate Discovery timer */
	route_info.discovery_timer = STM_AllocTimer(SW_LAYER_TYPE_APP, 
												ROUTE_DISCOVER_TIMEOUT_EVT, 
												&route_app_id);	


#ifdef ROUTE_PROBE

	/* Alloc Probe timer */
	route_info.probe.prd_timer = STM_AllocTimer(SW_LAYER_TYPE_APP, 
													PROBE_TIMEOUT_EVT, 
													&route_app_id);
	
	/* Alloc Probe Retransmit timer */
	route_info.probe.retrans_timer = STM_AllocTimer(SW_LAYER_TYPE_APP, 
													ROUTE_PROBE_RE_TIMEOUT_EVT, 
													&route_app_id);

#endif

	/* Sroute Request Periodic timer */
	route_info.sroute_timer = STM_AllocTimer(SW_LAYER_TYPE_APP, 
													ROUTE_SROUTE_PERIODIC_TIMEOUT_EVT, 
													&route_app_id);

#ifdef ROUTE_RECOVERY
	route_info.rec_timer = STM_AllocTimer(SW_LAYER_TYPE_APP, 
												ROUTE_REC_TIMEOUT_EVT, 
												&route_app_id);	

	STM_StartTimer(route_info.rec_timer, ROUTE_RECOVERY_TIME_BOOTUP ); //15 minutes

#endif	
}

/******************************************************************************
 * @fn      RouteApp_TimerHandler
 *
 * @brief   Timer handler for Routing timer events
 *
 * @param   event - event from firmware
 *
 * @return  none
 *
 */

#ifdef ROUTE_RECOVERY 
extern void swResetGV701x(u8 reset);
#endif
void RouteApp_TimerHandler(u8* buf)
{	
	hostTimerEvnt_t* timerevt = 
			(hostTimerEvnt_t*)buf;	

	if(buf == NULL)
		return;

	/*Demultiplexing the specific timer event*/ 					
	switch((u8)timerevt->type)
	{								
		case ROUTE_DISCOVER_TIMEOUT_EVT:
		case PROBE_TIMEOUT_EVT:
		case ROUTE_PROBE_RE_TIMEOUT_EVT:				
		case ROUTE_SROUTE_PERIODIC_TIMEOUT_EVT:
#ifdef ROUTE_RECOVERY			
		case ROUTE_REC_TIMEOUT_EVT:
#endif
			/*Discover timeout*/
			if(((u8)timerevt->type == ROUTE_DISCOVER_TIMEOUT_EVT) &&
				(route_info.disc_params.link != 0))
			{		
				u8 disc_link = route_info.disc_params.link;
				neighbor_info_t* powerline_nbr = NULL;
				neighbor_info_t* wireless_nbr = NULL;

#if 1
				FM_Printf(FM_APP, "\nDisc (dl %bx ap %bx pc %bu wc %bu pl %bu ra %bu)", route_info.disc_params.link,
					   route_info.disc_params.assignparent, route_info.disc_params.powerline.cnt,
					   route_info.disc_params.wireless.cnt, rtopo_data.link_pref.link,
					   route_info.route_sel_active);
#endif
				/*Check if PLC is the current discovering link*/
				if((route_info.disc_params.link & POWER_LINE) == POWER_LINE)
				{						
					if((route_info.disc_params.powerline.cnt >  \
						(((route_info.disc_params.assignparent & POWER_LINE) == POWER_LINE) ? \
						(ROUTE_DISC_MAX_HIGH_CNT) : (ROUTE_DISC_MAX_LOW_CNT))))
					{
						if((route_info.disc_params.assignparent & POWER_LINE) == POWER_LINE)
						{
							if((route_info.disc_params.link & WIRELESS) == WIRELESS)
							{
								/*Change pereference to Wireless when PLC 
								  discovery trials is exhausts*/
#ifdef RTOPO_APP						
								rtopo_data.link_pref.link = WIRELESS;
#endif
							}
							else
								rtopo_data.link_pref.link = 0;							
						}
						route_info.disc_params.link &= ~POWER_LINE;							
						route_info.start.power_line.active = FALSE;
					}
					else
					{
						if((route_info.disc_params.assignparent & POWER_LINE) == POWER_LINE)
						{					
							if(route_info.disc_params.powerline.cnt == ROUTE_DISC_MAX_HIGH_CNT)
							{
								/*Try to find a potential parent on PLC*/
								powerline_nbr = route_select_best_neighbor(POWER_LINE);								
							}
						}
						route_info.disc_params.powerline.cnt++;						
					}						
				}

				/*Check if Wireless is the current discovering link*/
				if((route_info.disc_params.link & WIRELESS) == WIRELESS)
				{
					if((route_info.disc_params.wireless.cnt > \
						(((route_info.disc_params.assignparent & WIRELESS) == WIRELESS) ? \
						(ROUTE_DISC_MAX_HIGH_CNT) : (ROUTE_DISC_MAX_LOW_CNT))))
					{
						if((route_info.disc_params.assignparent & WIRELESS) == WIRELESS)
						{					
							if((route_info.disc_params.link & POWER_LINE) == POWER_LINE)
							{
								/*Change pereference to PLC when Wireless
								  discovery trials is exhausts*/							
#ifdef RTOPO_APP						
								rtopo_data.link_pref.link = POWER_LINE;
#endif
							}
							else
								rtopo_data.link_pref.link = 0;
						}
						route_info.disc_params.link &= ~WIRELESS;
						route_info.start.wireless.active = FALSE;
					}
					else
					{
						if((route_info.disc_params.assignparent & WIRELESS) == WIRELESS)					
						{
							if(route_info.disc_params.wireless.cnt == ROUTE_DISC_MAX_HIGH_CNT)
							{
								/*Try to find a potential parent on Wireless*/
								wireless_nbr = route_select_best_neighbor(WIRELESS);		
							}
						}
						route_info.disc_params.wireless.cnt++;
					}
				}
#ifdef RTOPO_APP				
				/*Access the PLC link*/				
				if((rtopo_data.link_pref.link == POWER_LINE) &&
				   ((powerline_nbr != NULL) ? (powerline_nbr->addr != 0): (FALSE)))
				{
					if((route_info.disc_params.assignparent & POWER_LINE) == POWER_LINE)
					{
						if(TRUE == route_assign_best_parent(powerline_nbr))
						{
							/*Parent found on PLC, stop discovery on all links*/
							route_info.disc_params.link &= ~POWER_LINE;
							route_info.disc_params.powerline.cnt = 0;
							//rtopo_data.link_pref.link = POWER_LINE;
						}
						else
						{
							route_remove_link_neighbor(POWER_LINE);
						}
					}
				}

				/*Access the Wireless link*/
			 	if((rtopo_data.link_pref.link == WIRELESS) &&
					((wireless_nbr != NULL) ? (wireless_nbr->addr != 0): (FALSE)))
				{	
					if((route_info.disc_params.assignparent & WIRELESS) == WIRELESS)
					{
						if(TRUE == route_assign_best_parent(wireless_nbr))
						{
							/*Parent found on Wireless, stop discovery on all links*/						
							route_info.disc_params.link &= ~WIRELESS;
							route_info.disc_params.wireless.cnt = 0;	
						}
						else
						{
							route_remove_link_neighbor(WIRELESS);
						}						
					}
				}	
#endif		
#if 0
				/*Continue discovering if no parent found*/
				if(route_info.disc_params.link != 0)
				{				
					if((route_info.disc_params.assignparent & POWER_LINE) == POWER_LINE)
					{				
						if(powerline_nbr != NULL)
							rtopo_data.link_pref.link = POWER_LINE;
					}
					RouteApp_StartDiscovery(route_info.disc_params.link, 
									route_info.disc_params.assignparent, 0);	
				}	
				else
#endif					
				{
#if 0
					FM_Printf(FM_APP, "\nEnd(dl %bx ap %bx lo %bu %bu)", disc_link,
					   	route_info.disc_params.assignparent, 
					   	((route_info.disc_params.assignparent & WIRELESS) != WIRELESS),
					   	((route_info.disc_params.assignparent & POWER_LINE) != POWER_LINE));
#endif

					if((disc_link & WIRELESS) || (disc_link & POWER_LINE))
					{
						u8 done_link = 0;
						u8 undone_link = 0;

#if 0				
						FM_Printf(FM_APP, "\nDdone");
#endif					
						if(disc_link & WIRELESS) 
						{							
							if((route_info.disc_params.link & WIRELESS) != WIRELESS)
								done_link |= WIRELESS; 
							else
								undone_link |= WIRELESS; 
						}
						
						if(disc_link & POWER_LINE)
						{
						   if((route_info.disc_params.link & POWER_LINE) != POWER_LINE)
								done_link |= POWER_LINE;
						   else
								undone_link |= POWER_LINE; 
						}
#if 1				
						FM_Printf(FM_APP, "\n1.0 dl %bu udl %bu pc %bu wc %bu ra %bu parl %bu", done_link, undone_link,
								route_info.disc_params.powerline.cnt,
								route_info.disc_params.wireless.cnt, 
								route_info.route_sel_active, 
								((route_info.parent != NULL) ? (route_info.parent->link) : (0)));
#endif
						if(undone_link != 0)
						{		
							if((undone_link & POWER_LINE) && \
								(((route_info.disc_params.powerline.cnt-1) ==	\
								(((route_info.disc_params.assignparent & POWER_LINE) == POWER_LINE) ? \
								(ROUTE_DISC_MAX_HIGH_CNT) : (ROUTE_DISC_MAX_LOW_CNT)))))
							{								
								if(FALSE == route_is_neightable_empty(POWER_LINE))
								{
									done_link |= POWER_LINE;
									undone_link &= ~POWER_LINE;
									route_info.start.power_line.active = FALSE;
								}
							}

							if((undone_link & WIRELESS) && \
								(((route_info.disc_params.wireless.cnt-1) ==	\
								(((route_info.disc_params.assignparent & WIRELESS) == WIRELESS) ? \
								(ROUTE_DISC_MAX_HIGH_CNT) : (ROUTE_DISC_MAX_LOW_CNT)))))
							{
								if(FALSE == route_is_neightable_empty(WIRELESS))
								{
									done_link |= WIRELESS;
									undone_link &= ~WIRELESS;
									route_info.start.wireless.active = FALSE;
								}
							}

#if 0
							FM_Printf(FM_APP, "\nDL %bu UL %bu", done_link, undone_link);
#endif
						}

						if(done_link != 0)
						{														
							route_info.disc_params.link &= ~done_link;								
							if(done_link & POWER_LINE)
								route_info.disc_params.powerline.cnt = 0;
							if(done_link & WIRELESS)
								route_info.disc_params.wireless.cnt = 0;	

							if((((route_info.disc_params.assignparent & WIRELESS) != WIRELESS) &&
								(done_link & WIRELESS)) ||
							   (((route_info.disc_params.assignparent & POWER_LINE) != POWER_LINE) &&
							   (done_link & POWER_LINE)))
							{																		
								RouteApp_SendEvent(ROUTE_DISC_IND, done_link, 
												   ROUTE_REASON_MANUAL);
							}							
							
							if((((route_info.disc_params.assignparent & WIRELESS) == WIRELESS) &&
								(done_link & WIRELESS)) ||
							   (((route_info.disc_params.assignparent & POWER_LINE) == POWER_LINE) &&
							   (done_link & POWER_LINE)))										
							{								
								route_info.disc_params.assignparent &= ~done_link;
								if(route_info.parent != NULL) 
								{
									if((route_info.parent->link & done_link) == 0)
									{
										if(route_info.route_sel_active == TRUE) 
										{
											RouteApp_SendEvent(ROUTE_DWN_IND, done_link, 
															   ROUTE_REASON_ROUTEFAIL); 					
										}
									}
									else
									{
									}
								}
								else 
								{
									RouteApp_SendEvent(ROUTE_DWN_IND, done_link, 
													   ROUTE_REASON_ROUTEFAIL); 
								}							   
							}						
						}
						if(undone_link != 0)
						{
							RouteApp_StartDiscovery(undone_link, 
										route_info.disc_params.assignparent, 0);
						}						
					}

				}
			}
			else
			{	
#ifdef ROUTE_RECOVERY			
				if ((u8)timerevt->type == ROUTE_REC_TIMEOUT_EVT)
				{
					swResetGV701x(1);
				}
				else
#endif					
#ifdef ROUTE_PROBE
				/*Probe timeout*/
				if((u8)timerevt->type == PROBE_TIMEOUT_EVT)
				{
					route_periodic_probing();						
					/* Start Periodic timer */								
					if(STATUS_SUCCESS != STM_StartTimer(route_info.probe.prd_timer, 
																	MAX_PROBE_INTERVAL))
					{							
					}				
				}
				
				/*Sroute timeout*/
				else 
#endif					
				if((u8)timerevt->type == ROUTE_SROUTE_PERIODIC_TIMEOUT_EVT)
				{
					/*If sroute fails*/
					if(route_info.sroute_fail_cntdwn == 0)
					{
#ifdef ROUTE_ROAM				
						if(route_info.parent->link == WIRELESS)
						{
						
							route_info.sroute_fail_cntdwn = MAX_SROUTE_FAIL_COUNT;

							route_info.roaming_enable = 1;
							printf("ROAMEN\n");							
							RouteApp_StartDiscovery(WIRELESS, WIRELESS, route_info.parent->link);							
						}
						else
						
#endif
						{																							
							/*Remove parent from neighbor table and 
							  trigger stop event*/							
							route_info.sroute_fail_cntdwn = MAX_SROUTE_FAIL_COUNT;
							RouteApp_SendEvent(ROUTE_DWN_IND, route_info.parent->link, 
												ROUTE_REASON_ROUTEFAIL);
						}
					}					
					else 
					{
						if(TRUE == route_create_and_send_sroute_frame(route_info.zid)) 
						{
							/* Start SROUTE periodic timer frame */	
							if (STATUS_FAILURE == STM_StartTimer(route_info.sroute_timer, 
									((route_state.state == ROUTE_COMPLETE) ? 
									  MAX_SROUTE_PERIODIC_INTERVAL_HIGH : MAX_SROUTE_PERIODIC_INTERVAL_LOW))) 
							{			
							}								
						}	

						/*Age the entries*/
						route_rtable_aging_timer();																		
					}						
				}

				/*Probe Re-transmission timeout*/
#ifdef ROUTE_PROBE				
				else if((u8)timerevt->type == ROUTE_PROBE_RE_TIMEOUT_EVT)
				{
					if (route_info.probe.retrans_index != 0xFF) 
					{
						if (route_info.probe.retrans_count++ <= MAX_PROBE_RETRANSMIT) 
						{											
							if(TRUE == route_create_and_send_probe_req (&route_neighbor[route_info.probe.retrans_index], 
								route_info.probe.retrans_index))
							{	
							}
						}
						else 
						{
							/*Probe failed, if the neighbor was the parent trigger
							  a route stop event immediately*/
							if(route_neighbor[route_info.probe.retrans_index].addr == route_info.parent->addr)
							{
								RouteApp_SendEvent(ROUTE_DWN_IND, route_info.parent->link, ROUTE_REASON_ROUTEFAIL);
							}
							route_info.probe.retrans_index = 0xFF;
							route_info.probe.retrans_count = 0; 											
						}
					}
				}
#endif				
			}
		break;

		default:
		break;
	}
}

/******************************************************************************
 * @fn      RouteApp_RxAppMsg
 *
 * @brief   Receives a message from another app/fw
 *
 * @params  msg_buf - message buffer
 *
 * @return  none
 */

void RouteApp_RxAppMsg(sEvent* event)
{
	gv701x_app_msg_hdr_t* msg_hdr = (gv701x_app_msg_hdr_t*)event->buffDesc.dataptr;
	hostHdr_t* hybrii_hdr;
	hostEventHdr_t* evnt_hdr;

	hybrii_hdr = (hostHdr_t*)(msg_hdr + 1);
	
	if(msg_hdr->dst_app_id == route_app_id)
	{
		memcpy(&route_state.msg_hdr, msg_hdr, sizeof(gv701x_app_msg_hdr_t));
		route_state.eventproto = hybrii_hdr->protocol;
		if((msg_hdr->src_app_id == APP_FW_MSG_APPID) && 
			(hybrii_hdr->type == EVENT_FRM_ID))
		{
			evnt_hdr = (hostEventHdr_t*)(hybrii_hdr + 1);
			route_state.event = evnt_hdr->type; 	
			route_state.statedata = (u8*)(evnt_hdr + 1);
			route_state.statedatalen = (u16)(hybrii_hdr->length - sizeof(hostEventHdr_t));		
		}
		else
		{
			route_state.event = (u8)(*((u8*)(hybrii_hdr + 1)));
			route_state.statedata = (u8*)(hybrii_hdr + 1);
			route_state.statedatalen = (u16)hybrii_hdr->length;
		}		
		route_state.eventtype = hybrii_hdr->type;
		route_state.eventclass = event->eventHdr.eventClass;

		if((msg_hdr->src_app_id == APP_FW_MSG_APPID) && 
			(hybrii_hdr->type == EVENT_FRM_ID) &&
			(route_state.event == HOST_EVENT_APP_TIMER))
		{			
			RouteApp_TimerHandler((u8*)(evnt_hdr + 1)); 
			return;
		}		
		else if((msg_hdr->src_app_id == APP_FW_MSG_APPID) && 
			(hybrii_hdr->type == EVENT_FRM_ID) &&
			(route_state.event == HOST_EVENT_APP_CMD))
		{			
			RouteApp_CmdProcess((char*)(evnt_hdr + 1)); 
			return;
		}				
	}
	else if(msg_hdr->dst_app_id == APP_BRDCST_MSG_APPID)
	{
		u8 *event = (u8*)(hybrii_hdr + 1);	
		return;
	}
	RouteApp_StateMachine(&route_state);	
}

/******************************************************************************
 * @fn      route_handle_received_frm
 *
 * @brief   Demultiplexes all the OTA Routing frames
 *
 * @param   event - event from firmware
 *
 * @return  none
 *
 */

static u8 route_handle_received_frm (u8* frm, u8 len, u8 link, u8 lqi)
{
	u8 ret = 0;
	route_hdr_t *rhdr = (route_hdr_t *)&frm[(sizeof(sEth2Hdr))];
	u8 cmdid = RHDR_GET_CMDID(rhdr);
	totalRx++;
	
	switch (cmdid)
	{
		case FINDPARENT_REQ:
			route_info.stats.fpreq_rx++;
			route_handle_fpreq (frm, link, lqi);
		break;
		case FINDPARENT_RSP:
			route_info.stats.fprsp_rx++;			
			route_handle_fprsp (frm, link, lqi);
		break;
		case SOURCE_ROUTE_REQ:
			route_handle_sroute_req (frm, len, link);		
		break;
		case SOURCE_ROUTE_RSP:
			route_handle_sroute_rsp (frm, len, link);		
		break;		
		case PROBE_REQ:
			route_info.stats.preq_rx++;	
			route_handle_probe_req (frm, len, link);		
		break;
		case PROBE_RSP:
			route_info.stats.prsp_rx++;			
			route_handle_probe_rsp (frm, len, link);		
		break;	
#ifdef REGISTER_APP
		case REGISTRATION_RSP:
			register_rx_rsp(frm, len);		
		break;	
		case REREGISTRATION_REQ:	
			register_rx_rereq(frm, len);
		break;		
#endif				
		case APP_FRAME:
			ret = route_send_to_ul (frm, len, link);
		break;
		
		case GET_PARAM_RSP:	
			if(RHDR_GET_DIR(rhdr) != DIRECTION_DOWN) 
			{
				return ret;
			}
			ret = route_send_to_ul (frm, len, link);			
		break;		

		default:		
		break;
	}
	return ret;
}

/******************************************************************************
 * @fn      route_calc_path_metric
 *
 * @brief   Calculates the meteric of the neighbor
 *
 * @param   p - pointer to the neighbor entry (define found in route.h)
 *
 * @return  meteric
 *
 */

static u16 route_calc_path_metric(neighbor_info_t *p)
{
	if(p->link == WIRELESS)
	{
		if((p == NULL) ? (1) : ((route_info.wireless_ch != 0) ? 
					(p->ch != route_info.wireless_ch): (FALSE)))
		{
#if 0		
			FM_Printf(FM_APP, "\n8.0");
#endif
			return 0;
		}
		else 
		{	
#if 0		
			FM_Printf(FM_APP, "\n8.1");
#endif
			if(route_info.wireless_ch != 0)
			{
#if 0		
				FM_Printf(FM_APP, "\n8.2");
#endif
				if(p->lqi <= route_info.lqi_threshold)			
				{
#if 0				
					FM_Printf(FM_APP, "\n8.3");
#endif
					return 0;
				}
				else
				{
#if 0				
					FM_Printf(FM_APP, "\n8.3.0 %bu", p->cnt);
#endif				
					if(p->cnt >= ROUTE_PER_THRESHOLD)	
					{
#if 0				
						FM_Printf(FM_APP, "\n8.4.0");
#endif
						return p->lqi;
					}
					else
					{
#if 0				
						FM_Printf(FM_APP, "\n8.4");
#endif
						return 0;
					}
				}
			}
			else
			{
#if 0			
				FM_Printf(FM_APP, "\n8.5 %bx %bx", p->ed, route_info.ed_threshold);
#endif
				if(p->ed <= route_info.ed_threshold)
				{
#if 0				
					FM_Printf(FM_APP, "\n8.6");
#endif
				  	return 0;
				}
				else
				{
#if 0				
					FM_Printf(FM_APP, "\n8.7");
#endif
					return p->ed;
				}
			}
		}
	}  
	else if(p->link == POWER_LINE)
	{
		if((p == NULL) ? (1) : (p->rssi <= route_info.rssi_threshold))
		{
			return 0;
		}
		else
		{			
			if(p->bcn_rx > route_info.bcn_threshold)
			{
				if((route_info.parent != NULL)? (route_info.zid != p->parent_addr):
						(TRUE)) 
				{
					return p->rssi; 
				}
				else
				{
					return 0; 
				}
			}
			else
			{
				return 0;
			}
		}
	}
	else
	{
		return 0;
	}
}

/******************************************************************************
 * @fn      route_best_parent
 *
 * @brief   Compares the two neighbors and selects the neighbor with the best meteric 
 *
 * @param   p1 - pointer to the neighbor entry (define found in route.h)
 * 			p2 - pointer to the neighbor entry (define found in route.h)
 *
 * @return  neighbor
 *
 */

static neighbor_info_t *route_best_parent(neighbor_info_t *p1, neighbor_info_t *p2)
{
	u16 min_diff;
	u16 p1_metric = 0;
	u16 p2_metric = 0;

	if(((p1 != NULL)?(p1->link == WIRELESS):(1)) && 
		((p2 != NULL)?(p2->link == WIRELESS):(1)))
	{
#if 0	
		min_diff = RPL_DAG_MC_ETX_DIVISOR /
		         PARENT_SWITCH_THRESHOLD_DIV;
#else
		if(route_info.wireless_ch != 0)
		{
			/*LQI diff*/
			min_diff = route_info.lqi_diff;
#if 0			
			FM_Printf(FM_APP, "\n9.0 lqid %u", min_diff);
#endif
		}
		else
		{
			/*ED diff*/
			min_diff = route_info.ed_diff;
#if 0			
			FM_Printf(FM_APP, "\n9.1 edd %x", min_diff);
#endif
		}			
#endif

		if(p1 != NULL)
			p1_metric = route_calc_path_metric(p1);

		if(p2 != NULL)
			p2_metric = route_calc_path_metric(p2);

		if((p2_metric != 0) && (p1_metric != 0))
		{
#if 0		
			FM_Printf(FM_APP, "\np1 %u p2 %u",p1_metric, p2_metric);
#endif
			if((abs(p1_metric - p2_metric)) >= min_diff)
			{
#if 0		
				FM_Printf(FM_APP, "\nDif p1 %u %x p2 %u %x d %u %x",
				        p1_metric, p1_metric, p2_metric, p2_metric,
				        min_diff, min_diff);
#endif
				return p1_metric > p2_metric ? p1 : p2;
			}
			else
			{
				if(route_info.wireless_ch != 0)
				{
#if 0			
					FM_Printf(FM_APP, "\n8.8 p1 %u p2 %u", p1_metric, p2_metric);
#endif
					return p1_metric > p2_metric ? p1 : p2;
				}
				else
				{
					/*LQI diff*/
					min_diff = route_info.lqi_diff;
#if 0					
					FM_Printf(FM_APP, "\n8.9 %bx %bx lqidiff %u", p1->lqi, p2->lqi, min_diff);
#endif
					if((abs(p1->lqi - p2->lqi)) >= min_diff)
					{
#if 0		
						FM_Printf(FM_APP, "\nDif p1 %bu p2 %bu d %x ",
						        p1->lqi, p2->lqi, min_diff);
#endif
						return p1->lqi > p2->lqi ? p1 : p2;
					}	
					else
					{
#if 0					
						FM_Printf(FM_APP, "\n11.13 %bx %bx", p1->ed, p2->ed);	
#endif
						if(p1->ed > p2->ed)
						{
#if 0						
							FM_Printf(FM_APP, "\n11.10");
#endif
							return p1;
						}
						else if(p1->ed < p2->ed)
						{
#if 0						
							FM_Printf(FM_APP, "\n11.11");
#endif
							return p2;
						}
						else
						{
#if 0						
							FM_Printf(FM_APP, "\n11.12");
#endif						
							return p1->lqi > p2->lqi ? p1 : p2;
						}
					}
				}	  	
			}
		}
		else
		{
#if 0		
			FM_Printf(FM_APP, "\n7.1");
#endif
			if(p1_metric != 0)
			{
#if 0			
				FM_Printf(FM_APP, "\n7.2");
#endif
				return p1;
			}
			else if(p2_metric != 0)
			{
#if 0			
				FM_Printf(FM_APP, "\n7.3");
#endif
				return p2;
			}
			else
			{
#if 0			
				FM_Printf(FM_APP, "\n7.4");
#endif
				return NULL;
			}
		}		
	}
	else if(((p1 != NULL)?(p1->link == POWER_LINE):(1)) && 
		    ((p2 != NULL)?(p2->link == POWER_LINE):(1)))
	{
		min_diff = 30;

		if(p1 != NULL)
			p1_metric = route_calc_path_metric(p1);
		if(p2 != NULL)
			p2_metric = route_calc_path_metric(p2);
#if 0			
			FM_Printf(FM_APP, "P-RPL: p1 = %u p2 = %u\n",
		 	        p1_metric, p2_metric);
#endif
		if((p2_metric != 0) && (p1_metric != 0))
		{
			if(p2_metric > p1_metric + min_diff)
				return p2;
			else
				return p1;
		}
		else
		{
			if(p1_metric != 0)
				return p1;
			else if(p2_metric != 0)
				return p2;
			else
				return NULL;
		}
	}
	else
		return NULL;
}

/******************************************************************************
 * @fn      route_calculate_rank
 *
 * @brief   Calculates the rank of the new neighbor
 *
 * @param   p - pointer to the neighbor entry (define found in route.h)
 *          base_rank - 
 *
 * @return  neighbor
 *
 */

static u16 route_calculate_rank(neighbor_info_t *p, u16 base_rank)
{
	u16 new_rank;
	u16 rank_increase;

	if(p == NULL) 
	{
		if(base_rank == 0) 
			return INFINITE_RANK;

		rank_increase = NEIGHBOR_INFO_FIX2ETX(INITIAL_LINK_METRIC) * RTR_MIN_HOPRANKINC;
	} 
	else 
	{
		/* multiply first, then scale down to avoid truncation effects */
		rank_increase = NEIGHBOR_INFO_FIX2ETX(p->etx * RTR_MIN_HOPRANKINC);
		if(base_rank == 0) 
		  base_rank = p->rank;
	}

	if(INFINITE_RANK - base_rank < rank_increase) 
	{
		/* Reached the maximum rank. */
		new_rank = INFINITE_RANK;
	} 
	else 
	{
		/* Calculate the rank based on the new rank information from DIO or
		  stored otherwise. */
		new_rank = base_rank + rank_increase;
	}

	return new_rank;
}

/******************************************************************************
 * @fn      route_upate_metric
 *
 * @brief   Updates the meteric of the neighbor based on PER
 *
 * @param   nbr - pointer to the neighbor entry (define found in route.h)
 *          tx_count - packets sent to the neighbor
 *
 * @return  none
 *
 */

void route_upate_metric(u8 tx_count, neighbor_info_t *nbr)
{
	u32 netx; 

	if (nbr->etx == RTR_LINK_ESTIMATE_UNIT) 
	{
		nbr->etx = tx_count * RTR_LINK_ESTIMATE_UNIT;
    } 
	
	netx = (((u32)tx_count * RTR_LINK_ESTIMATE_UNIT) * 
			RTR_LINK_ESTIMATE_ALPHA + 
			nbr->etx * (RTR_LINK_ESTIMATE_UNIT - RTR_LINK_ESTIMATE_ALPHA)) / 
                       	RTR_LINK_ESTIMATE_UNIT ;
#if 0	
	FM_Printf(FM_APP, "\nMupt 0x%02x, e %u, n %u, c %bx", nbr->addr, nbr->etx, netx, tx_count);
#endif
	nbr->etx = netx;  
}

/******************************************************************************
 * @fn      route_handle_fpreq
 *
 * @brief   Parses and handles the find parent request frame received from the remote peer
 *
 * @param   frm - pointer to the packet
 *          link - the link on which the packet arrived (WIRELESS or POWER_LINE)
 *			lqi - the link quality of the received frame
 *
 * @return  none
 *
 */

static void route_handle_fpreq(u8 *frm, u8 link, u8 lqi) 
{
	fpreq_t* fpreq = (fpreq_t *)&frm[(sizeof(sEth2Hdr))];
	route_hdr_t* rhdr = (route_hdr_t*)fpreq;

#if 1
	FM_Printf(FM_APP, "\nFPREQ Rx(s 0x%02x, d 0x%02x, l %bu lqi %bu)", 
		    le16_to_cpu(fpreq->rhdr.target), 
		    le16_to_cpu(fpreq->rhdr.parent), link, lqi);
#endif

	if(route_device_profile.device_type != DEV_BRIDGING)
		return;
	
	/* If route has not been discovered yet, then do not respond */
	if(route_state.state != ROUTE_COMPLETE)
		return;

#if 0
	if(link == POWER_LINE)
	{
		if((route_info.parent != NULL) ? (route_info.parent->link == POWER_LINE) : (TRUE))
			return;
	}
#endif

	/* Ignore this frame if the direction is not right */
	if(RHDR_GET_DIR(rhdr) != DIRECTION_UP) 
		return;

	/* If target is my parent, we should not send response to him */
	if(cpu_to_le16(fpreq->rhdr.target) == route_info.parent->addr) 
		return;

	
	if((link == WIRELESS)&&(lqi <= ROUTE_LQI_THRESHOLD))
	{
		return;
	}
	
	route_create_and_send_fprsp_frame (&fpreq->ieee_address[2], 
				cpu_to_le16(fpreq->rhdr.target), link);
	
	return;
}

/******************************************************************************
 * @fn      route_handle_fprsp
 *
 * @brief   Parses and handles the find parent response frame received from the remote peer
 *
 * @param   frm - pointer to the packet
 *          link - the link on which the packet arrived (WIRELESS or POWER_LINE)
 *
 * @return  none
 *
 */

static void route_handle_fprsp(u8 *frm, u8 link, u8 lqi) 
{
	u8 tmac[MAC_ADDR_LEN];
	fprsp_t* fprsp = (fprsp_t *)&frm[(sizeof(sEth2Hdr))];
	route_hdr_t* rhdr = (route_hdr_t*)fprsp;
	
	memset(tmac, 0x00, MAC_ADDR_LEN);

	/* Ignore this frame if the direction is not right */
	if(RHDR_GET_DIR(rhdr)!= DIRECTION_DOWN)		
		return;
	
	memcpy_cpu_to_le(tmac, &fprsp->ieee_address[2], IEEE_MAC_ADDRESS_LEN-2);
#if 1
	FM_Printf(FM_APP, "\nFPRSP Rx(s 0x%02x, d 0x%02x, l %bu lqi %bu pl %bu, lqi %bu)", 
				le16_to_cpu(fprsp->rhdr.parent), 
				le16_to_cpu(fprsp->rhdr.target), link, lqi, rtopo_data.link_pref.link, lqi);
#endif

	/* Add device to the neighbor table */
	if(!route_add_neighbor(le16_to_cpu(fprsp->rhdr.parent), tmac,
							le16_to_cpu(fprsp->rhdr.rank), link, 
							le16_to_cpu(fprsp->path_metric), 0, 0, lqi, 0, route_info.wireless_ch, 0)) 
	{
#if 0	
		FM_Printf(FM_APP, "\nNbr fail (nbr %x)", 
			    le16_to_cpu(fprsp->parent)); 
#endif
	}	
}

/******************************************************************************
 * @fn      route_handle_sroute_req
 *
 * @brief   Parses and handles the sroute request frame received from the remote peer
 *
 * @param   frm - pointer to the packet
 *			len - length in number of bytes
 *          link - the link on which the packet arrived (WIRELESS or POWER_LINE)
 *
 * @return  none
 *
 */

static void route_handle_sroute_req (u8 *frm, u8 len, u8 link) 
{
	u8 addr[IEEE_MAC_ADDRESS_LEN];
	sroute_req_t* sroute = (sroute_req_t *)&frm[(sizeof(sEth2Hdr))];
	route_hdr_t* rhdr = (route_hdr_t*)sroute;
	
	len = len;
#if 1
	FM_Printf(FM_APP, "\nSREQ Rx(s 0x%02x, d 0x%02x, l %bu)", 
				le16_to_cpu(sroute->rhdr.target), 
				le16_to_cpu(sroute->rhdr.parent), link);
#endif
	/* Ignore this frame if the direction is not right */
	if(RHDR_GET_DIR(rhdr)!= DIRECTION_UP) 
		return;

	memcpy_cpu_to_le(addr, &sroute->ieee_address[2], IEEE_MAC_ADDRESS_LEN-2);

	if(RHDR_GET_BRIDGE(rhdr) == TRUE)
	{
		/* Add target route in the table */
		if (route_add_route (le16_to_cpu(sroute->rhdr.target), 
			 le16_to_cpu(sroute->rhdr.parent), addr, link) == FALSE) 
		{
#if 0			 
			FM_Printf(FM_APP, "\nNo route 0x%02x", le16_to_cpu(sroute->rhdr.target)); 
#endif
		}
	}
	
	CRM_AddScbVendorField(addr, le16_to_cpu(sroute->rhdr.target));		
}

/******************************************************************************
 * @fn      route_handle_sroute_rsp
 *
 * @brief   Parses and handles the sroute response frame received from the remote peer
 *
 * @param   frm - pointer to the packet
 *			len - length in number of bytes
 *          link - the link on which the packet arrived (WIRELESS or POWER_LINE)
 *
 * @return  none
 *
 */

static void route_handle_sroute_rsp (u8 *frm, u8 len, u8 link) 
{
	sroute_rsp_t *sroute_rsp = (sroute_rsp_t *)&frm[(sizeof(sEth2Hdr))];

	len = len;
	link = link;
#if 1
	FM_Printf(FM_APP, "\nSRSP Rx(s 0x%02x, d 0x%02x, l %bu)", 
				le16_to_cpu(sroute_rsp->rhdr.parent), 
				le16_to_cpu(sroute_rsp->rhdr.target), link);
#endif

	route_info.stats.sroute_rx++;

	/* If SROUTE response comes, clear STA reset flat */	
	route_info.sroute_fail_cntdwn = MAX_SROUTE_FAIL_COUNT; 	

	if(route_state.state != ROUTE_COMPLETE)
	{
		route_updt_route_evnt_msg_t	route_update;
		route_update.event = ROUTE_UPDATE_ROUTE_EVNT;
		GV701x_SendAppEvent(route_app_id, route_app_id, APP_MSG_TYPE_APPEVENT, 
				APP_MAC_ID, EVENT_CLASS_CTRL, MGMT_FRM_ID,			
				&route_update, sizeof(route_updt_route_evnt_msg_t), 0);					
	}
}

/******************************************************************************
 * @fn      route_handle_probe_req
 *
 * @brief   Parses and handles the probe request frame received from the remote peer
 *
 * @param   frm - pointer to the packet
 *			len - length in number of bytes
 *          link - the link on which the packet arrived (WIRELESS or POWER_LINE)
 *
 * @return  none
 *
 */

static void route_handle_probe_req (u8 *frm, u8 len, u8 link)
{
	probe_t* probe = (probe_t *)&frm[(sizeof(sEth2Hdr))];
	route_hdr_t* rhdr = (route_hdr_t*)probe;

#if 0
	FM_Printf(FM_APP, "\nPREQ Rx(s 0x%02x, d 0x%02x, l %bu)", 
				le16_to_cpu(probe->rhdr.target), 
				le16_to_cpu(probe->rhdr.parent), link);
#endif

	if(route_state.state != ROUTE_COMPLETE)
		return;

	/* Ignore this frame if the direction is not right */
	if(RHDR_GET_DIR(rhdr) != DIRECTION_UP) 
		return;
	
	if (le16_to_cpu(probe->rhdr.rank) < route_info.rank) 
		return;
	
	route_create_and_send_probe_rsp (&frm[6], cpu_to_le16(probe->rhdr.target), 
									 cpu_to_le16(probe->rhdr.rank), len, link); 	
}

/******************************************************************************
 * @fn      route_handle_probe_rsp
 *
 * @brief   Parses and handles the probe response frame received from the remote peer
 *
 * @param   frm - pointer to the packet
 *			len - length in number of bytes
 *          link - the link on which the packet arrived (WIRELESS or POWER_LINE)
 *
 * @return  none
 *
 */

static void route_handle_probe_rsp (u8 *frm, u8 len, u8 link)
{
	probe_t* probe = (probe_t *)&frm[(sizeof(sEth2Hdr))];
	route_hdr_t* rhdr = (route_hdr_t*)probe;
	neighbor_info_t *nbr = NULL;

	len= len;
	link = link;

#if 0
	FM_Printf(FM_APP, "\nPRSP Rx(s 0x%04x, d 0x%04x, l %bu)", 
				le16_to_cpu(probe->rhdr.parent), 
				le16_to_cpu(probe->rhdr.target), link);
#endif

	/* Ignore this frame if the direction is not right */
	if(RHDR_GET_DIR(rhdr) != DIRECTION_DOWN)
		return;

	/* Look up Neighbor table */
	nbr = route_get_neighbor (le16_to_cpu(probe->rhdr.parent));
	
	if (nbr == NULL) 
		return;		

	/* If neighbor is not matching with the current neighbor under probe - 
	   ignore this frame */
	if (route_neighbor[route_info.probe.retrans_index].addr != 
				le16_to_cpu(probe->rhdr.parent)) 
		return;	

	/* Calculate metric */
	route_upate_metric((route_info.probe.retrans_count == 0) ? 
					   1 : route_info.probe.retrans_count, nbr);

#ifdef ROUTE_PROBE	
	STM_StopTimer (route_info.probe.retrans_timer);
#endif
	route_info.probe.retrans_index = 0xFF;
	route_info.probe.retrans_count = 0;
}

/******************************************************************************
 * @fn      route_create_and_send_fprsp_frame
 *
 * @brief   Creates and sends the find parent response frame to the remote peer
 *
 * @param   dst_ieee_addr - pointer to the ieee address of the destination node
 *			target - the short address of the destination node
 *          link - the link on which the packet arrived (WIRELESS or POWER_LINE)
 *
 * @return  result - TRUE or FALSE
 *
 */

static bool route_create_and_send_fprsp_frame (u8 *dst_ieee_addr, u16 target, u8 link)
{
	u8 tmac[MAC_ADDR_LEN];
	u8 buff[MAX_ROUTE_FRM_SIZE];
	fprsp_t *fprsp = (fprsp_t *)&buff[(sizeof(sEth2Hdr))];

	if (route_state.state != ROUTE_COMPLETE) 
		return FALSE;
	
	/* Form a FPRSP frame */
	fprsp->rhdr.fc.control_bits = 0;
	fprsp->rhdr.fc.control_bits |= RHDR_SET_DIR(DIRECTION_DOWN);
	fprsp->rhdr.fc.control_bits |= RHDR_SET_CMDID(FINDPARENT_RSP);
	fprsp->rhdr.fc.control_bits |= RHDR_SET_ONEHOP(TRUE);
	fprsp->rhdr.target = cpu_to_le16(target);
	fprsp->rhdr.parent = cpu_to_le16(route_info.zid);	
	fprsp->rhdr.rank = cpu_to_le16(route_info.rank);
	fprsp->parent = cpu_to_le16(route_info.zid);
	fprsp->path_metric = cpu_to_le16(route_calc_path_metric(route_info.parent));
	memcpy_cpu_to_le (fprsp->ieee_address, route_info.ieee_addr, IEEE_MAC_ADDRESS_LEN);

	route_info.stats.fprsp_tx++;

#if 1
	FM_Printf(FM_APP, "\nFPRSP Tx(s 0x%02x, d 0x%02x, l %bu)", 
				le16_to_cpu(fprsp->rhdr.parent), 
				le16_to_cpu(fprsp->rhdr.target), link);
#endif
	if (dst_ieee_addr)
		memcpy_cpu_to_le(tmac, dst_ieee_addr, IEEE_MAC_ADDRESS_LEN-2); 
	else 
		memset (tmac, 0xFF, IEEE_MAC_ADDRESS_LEN-2);

	if (link & POWER_LINE) 
	{
		/* Send Frame */
		route_send_frm (tmac, route_info.ieee_addr, 
					target, 0, 
					DIRECTION_DOWN, buff, 
					(sizeof(sEth2Hdr)) + sizeof (fprsp_t), 
					POWER_LINE, TRUE, FALSE);
	}
	if (link & WIRELESS) 
	{
		/* Send Frame */
		route_send_frm (tmac, route_info.ieee_addr, target, 0, DIRECTION_DOWN, buff, 											
						(sizeof(sEth2Hdr)) + sizeof (fprsp_t), WIRELESS, TRUE, FALSE);						
	}

	return TRUE;
}

/******************************************************************************
 * @fn      route_create_discover_fpreq_frame
 *
 * @brief   Creates and sends the find parent response frame to the remote peer
 *
 * @param   target - short addres of the destination node
 *			wir_mac_addr - the mac address of the destination wireless node 
 *                         (NULL if it is to be broadcasted)
 *			pwr_mac_addr - the mac address of the destination powerline node
 *						   (NULL if it is to be broadcasted)
 *          link - the link on which to send the packet (WIRELESS and/or POWER_LINE)
 *
 * @return  result - TRUE or FALSE
 *
 */

static bool route_create_discover_fpreq_frame (u16 target, u8* wir_mac_addr, u16 wir_addr, 
												u8* pwr_mac_addr, u16 pwr_addr, u8 link)
{
	u8 buff[MAX_ROUTE_FRM_SIZE];
	neighbor_info_t* nbr = NULL;
	fpreq_t *fpreq = (fpreq_t *)&buff[(sizeof(sEth2Hdr))];

	/* Create FPREQ frame */
	fpreq->rhdr.fc.control_bits = 0;
	fpreq->rhdr.fc.control_bits |= RHDR_SET_DIR(DIRECTION_UP);
	fpreq->rhdr.fc.control_bits |= RHDR_SET_CMDID(FINDPARENT_REQ);
	fpreq->rhdr.fc.control_bits |= RHDR_SET_ONEHOP(TRUE);
	fpreq->rhdr.target = cpu_to_le16(target);	
	fpreq->rhdr.parent = cpu_to_le16(BROADCAST_NET_ADDR);
	fpreq->rhdr.rank = cpu_to_le16(INFINITE_RANK);
	memcpy_cpu_to_le (fpreq->ieee_address, route_info.ieee_addr, 8);

#if 1
	FM_Printf(FM_APP, "\nFPREQ Tx(s 0x%02x,", 
				le16_to_cpu(fpreq->rhdr.target));
	if(link == POWER_LINE)
		FM_Printf(FM_APP, " d(pl) 0x%02x) cnt %bu", pwr_addr, route_info.disc_params.powerline.cnt);
	else if(link == WIRELESS)
		FM_Printf(FM_APP, " d(wl) 0x%02x) cnt %bu", wir_addr, route_info.disc_params.wireless.cnt);
	else
		FM_Printf(FM_APP, " d(pl) 0x%02x d(wl) 0x%02x) pcnt %bu wcnt %bu", pwr_addr, wir_addr, 
					route_info.disc_params.wireless.cnt, route_info.disc_params.wireless.cnt);									
		FM_Printf(FM_APP, " l %bu dl %bu pl %bu", link, route_info.disc_params.link, 
					rtopo_data.link_pref.link);
#endif
	route_info.stats.fpreq_tx++;

	if((link & POWER_LINE) == POWER_LINE)
	{
		route_send_frm (pwr_mac_addr, route_info.ieee_addr, pwr_addr, 
						0, DIRECTION_UP, buff, (sizeof(sEth2Hdr)) + sizeof (fpreq_t), 
						POWER_LINE, TRUE, FALSE);
	}

	if((link & WIRELESS) == WIRELESS)
	{
		route_send_frm (wir_mac_addr, route_info.ieee_addr, wir_addr,
						0, DIRECTION_UP, buff, (sizeof(sEth2Hdr)) + sizeof (fpreq_t), 
						WIRELESS, TRUE, FALSE);
	}
	return TRUE;	
}

/******************************************************************************
 * @fn      route_create_and_send_sroute_frame
 *
 * @brief   Creates and sends the sroute request frame to the Host
 *
 * @param   target - short addres of this node
 *
 * @return  result - TRUE or FALSE
 *
 */

static bool route_create_and_send_sroute_frame (u16 target)
{
	u8 buff[MAX_ROUTE_FRM_SIZE];
	u8 tmac[MAC_ADDR_LEN];
	sroute_req_t *sroute = (sroute_req_t *)&buff[(sizeof(sEth2Hdr))];

	if((route_state.state != ROUTE_DISCOVER) &&
		(route_state.state != ROUTE_COMPLETE)) 
		return FALSE;

	/* Create FPREQ frame */
	sroute->rhdr.fc.control_bits = 0;
	sroute->rhdr.fc.control_bits |= RHDR_SET_DIR(DIRECTION_UP);
	sroute->rhdr.fc.control_bits |= RHDR_SET_CMDID(SOURCE_ROUTE_REQ);
	sroute->rhdr.fc.control_bits |= RHDR_SET_TOROOT(TRUE);
	sroute->rhdr.fc.control_bits |= RHDR_SET_ONEHOP(FALSE);
	
	if (route_device_profile.device_type == DEV_BRIDGING) 
		sroute->rhdr.fc.control_bits |= RHDR_SET_BRIDGE(TRUE);
	
	if(route_info.unreachable == TRUE) 
		sroute->rhdr.fc.control_bits |= RHDR_SET_RESET(TRUE);
	else 
		sroute->rhdr.fc.control_bits |= RHDR_SET_RESET(FALSE);
	
	sroute->rhdr.target = cpu_to_le16(target);
	sroute->rhdr.parent = cpu_to_le16(route_info.parent->addr);
	sroute->rhdr.rank = cpu_to_le16(route_info.rank);
	memcpy_cpu_to_le (&sroute->ieee_address, &route_info.ieee_addr, (IEEE_MAC_ADDRESS_LEN));

#if 1
	FM_Printf(FM_APP, "\nSREQ Tx(s 0x%02x, d 0x%02x, l %bu)", le16_to_cpu(sroute->rhdr.target),			 
			le16_to_cpu(sroute->rhdr.parent), route_info.parent->link);			
#endif
	memcpy(tmac, route_info.parent->ieee_addr, (IEEE_MAC_ADDRESS_LEN - 2));					

	/* Send Frame to the parent */
	route_send_frm (tmac, route_info.ieee_addr, 
					route_info.parent->addr, route_info.zid,
					DIRECTION_UP, buff, 
					(sizeof(sEth2Hdr)) + sizeof (sroute_req_t), 
					route_info.parent->link, TRUE, FALSE);

	route_info.stats.sroute_tx++;

	if(route_info.sroute_fail_cntdwn != 0)
		route_info.sroute_fail_cntdwn--; 		

	return TRUE;	
}

/******************************************************************************
 * @fn      route_create_and_send_probe_req
 *
 * @brief   Creates and sends the probe request frame to the remote peer
 *
 * @param   nbr - pointer to the remote neighbor 
 * 			index - index to the neighbor in the neighbor table
 *
 * @return  result - TRUE or FALSE
 *
 */

#ifdef ROUTE_PROBE

static bool route_create_and_send_probe_req (neighbor_info_t *nbr, u8 index)
{
	u8 buff[MAX_ROUTE_FRM_SIZE];
	probe_t *probe = (probe_t *)&buff[(sizeof(sEth2Hdr))];

	memset (buff, 0x00, (sizeof(sEth2Hdr)) + sizeof (probe_t));
	
	/* Create FPREQ frame */
	probe->rhdr.fc.control_bits = 0;
	probe->rhdr.fc.control_bits |= RHDR_SET_DIR(DIRECTION_UP);
	probe->rhdr.fc.control_bits |= RHDR_SET_CMDID(PROBE_REQ);
	probe->rhdr.fc.control_bits |= RHDR_SET_ONEHOP(TRUE);
	
	if (route_device_profile.device_type == DEV_BRIDGING) 
		probe->rhdr.fc.control_bits |= RHDR_SET_BRIDGE(TRUE);

	probe->rhdr.target = cpu_to_le16(route_info.zid);
	probe->rhdr.parent = cpu_to_le16(nbr->addr);
	probe->rhdr.rank = cpu_to_le16(nbr->rank);

#if 1
	FM_Printf(FM_APP, "\nPREQ Tx(s 0x%02x, d 0x%02x, l %bu", le16_to_cpu(probe->rhdr.target), 				
				le16_to_cpu(probe->rhdr.parent), nbr->link);				
#endif

	/* Start Retransmit timer */
	if(STATUS_SUCCESS != STM_StartTimer(route_info.probe.retrans_timer, 
													MAX_PROBE_RETRANS_INTERVAL))
	{
#if 0	
		FM_Printf(FM_APP, "\nProbe retrans timer fail");							
#endif
	}

	
	if(nbr->probe_count >= 0xFFFFFFFE)
		nbr->probe_count = 0;

	nbr->probe_count++;		
	route_info.stats.preq_tx++;
	route_info.probe.retrans_index = index;
	/* Send Frame to the neighbor */
	route_send_frm(nbr->ieee_addr, route_info.ieee_addr, (nbr->addr), route_info.zid,					
					DIRECTION_UP, buff, (sizeof(sEth2Hdr)) + sizeof (probe_t), nbr->link, TRUE, FALSE);
					
	return TRUE;	
}

#endif

/******************************************************************************
 * @fn      route_create_and_send_probe_rsp
 *
 * @brief   Creates and sends the probe response frame to the remote peer
 *
 * @param   ieee_addr - the ieee address of the remote peer
 * 			dest - the short adddress of the peer
 * 			rank - the rank of the peer
 *			len - the length in number of bytes
 *			link - the link to the peer
 *
 * @return  result - TRUE or FALSE
 *
 */

static bool route_create_and_send_probe_rsp (u8 *ieee_addr, u16 dest, u8 rank, u8 len, u8 link)
{
	u8 buff[MAX_ROUTE_FRM_SIZE];
	probe_t *probe = (probe_t *)&buff[(sizeof(sEth2Hdr))];

	len = len;
	/* Create PROBE Response frame */
	probe->rhdr.fc.control_bits = 0;
	probe->rhdr.fc.control_bits |= RHDR_SET_DIR(DIRECTION_DOWN);
	probe->rhdr.fc.control_bits |= RHDR_SET_CMDID(PROBE_RSP);
	probe->rhdr.fc.control_bits |= RHDR_SET_ONEHOP(TRUE);
	probe->rhdr.target = cpu_to_le16(dest);
	probe->rhdr.parent = cpu_to_le16(route_info.zid);
	probe->rhdr.rank = cpu_to_le16(rank);

#if 1
	FM_Printf(FM_APP, "\nPRSP Tx(s 0x%02x, d 0x%02x, l %bu)", le16_to_cpu(probe->rhdr.parent), 				
				le16_to_cpu(probe->rhdr.target), link);				
#endif	

	/* Send Frame to the source of probe_req */
	route_send_frm (ieee_addr, route_info.ieee_addr, dest, route_info.zid, DIRECTION_DOWN, 					
					buff, (sizeof(sEth2Hdr)) + sizeof (probe_t), link, TRUE, FALSE);
					
	return TRUE;	
}


/******************************************************************************
 * @fn      route_send_frm
 *
 * @brief   Sends an OTA frame 
 *
 * @param   dstn_ieee_addr - the ieee address of the destination node
 * 			src_ieee_addr - the ieee address of the originating node
 * 			dest - the short address of the destination
 *			src - the short address of the originating node
 *			dir - the direction to the destination (DIRECTION_UP or DIRECTION_DOWN)
 *			buff - pointer to the data frame 
 *			len - the length in number of bytes
 *			link - the link to the peer (POWER_LINE or WIRELESS)
 *			rhdr_present - TRUE if routing header is already present else FALSE otherwise
 *			forward - TRUE if the frame is to be forwarded to the sender
 *
 * @return  result - TRUE or FALSE
 *
 */

static bool route_send_frm (u8 *dstn_ieee_addr, u8 *src_ieee_addr, u16 dest, u16 src, u8 dir, 
								u8 *buff, u8 len, u8 link, u8 rhdr_present, u8 forward)
{
	sEth2Hdr* petherhdr = (sEth2Hdr*)(buff);

	src_ieee_addr = src_ieee_addr;
	src = src;
	dir = dir;
	rhdr_present = rhdr_present;	
	forward = forward;

	totalTx++;
	
	if(dstn_ieee_addr == NULL)
		memset(&petherhdr->dstaddr, 0xFF, MAC_ADDR_LEN);
	else
		memcpy(&petherhdr->dstaddr, dstn_ieee_addr, MAC_ADDR_LEN); 
	
	memcpy(&petherhdr->srcaddr, &route_info.ieee_addr, MAC_ADDR_LEN);		
	petherhdr->ethtype = APP_ETHER_PROTO;		

	/* Send frame to the given link */
	if((link == POWER_LINE) && 
#ifdef NWKSTARTUP_APP		
		(nwkstartup_data.link.power_line.state == LINK_UP)
#else
	  (TRUE)
#endif		
	  )
	{
		GV701x_SendData(APP_PORT_PLC, buff, len, 0);	
	}
	else if(link == WIRELESS)    
	{
#ifdef LRWPAN_DRIVER_APP				
		if((dstn_ieee_addr != NULL)	|| (dest != BROADCAST_NET_ADDR))			   
			lrwpan_SendData(dest, buff, len);
		else
			lrwpan_SendData(BROADCAST_NET_ADDR, buff, len);
#endif
	} 
	else 
	{
		return FALSE;
	}
	
	return TRUE;		
}

/******************************************************************************
 * @fn      route_assign_best_parent
 *
 * @brief   Assigns the given neighbor to be the Parent and checks
 *			for the transitions between the previous parents
 *
 * @param   nbr - the pointer to the neighbor
 *
 * @return  result - TRUE or FALSE
 *
 */

u8 route_assign_best_parent(neighbor_info_t* nbr)
{
	u8 link;
	u16 addr;
	
	if(route_info.parent != NULL)
	{
		link = route_info.parent->link;
		addr = route_info.parent->addr;
	}
	else
	{
		link = 0;
		addr = 0;
	}

#if 1
	FM_Printf(FM_APP, "\nAPar l %bu ol %bu oa %x a %x", nbr->link, link, addr, nbr->addr);
#endif
			
	if((route_info.parent != NULL) ? ((route_info.parent->addr != nbr->addr) ||
	   (route_info.parent->link != nbr->link) ): (0))
	{	
		if((route_info.route_sel_active == TRUE)
#ifdef ROUTE_ROAM
			||(route_info.roaming_enable)
#endif
			)
		{
			if(route_info.parent->link != nbr->link)
			{
				if(route_info.parent->link == WIRELESS)
				{				
#if 0				
					route_info.start.wireless.active = FALSE;
					route_info.start.wireless.app_id = 0;									
#endif
				}
			}
			if(route_state.state == ROUTE_COMPLETE)
				route_create_and_send_fprsp_frame (NULL, 0xFFFF, WIRELESS | POWER_LINE);

#if 1
			route_remove_neighbor(route_info.parent->addr, 
					  route_info.parent->link);	
#endif
#if 0			
			route_state.state = ROUTE_START;
#endif
			route_info.sroute_fail_cntdwn = MAX_SROUTE_FAIL_COUNT;
		}
	}
	else
	{
#if 0	
#ifdef RTOPO_APP
		rtopo_prof_evnt_msg_t rtopo_prof;
		rtopo_prof.event = RTOPO_PROFILE_EVNT; 
		GV701x_SendAppEvent(route_app_id, rtopo_app_id, APP_MSG_TYPE_APPEVENT, 
			APP_MAC_ID, EVENT_CLASS_CTRL, MGMT_FRM_ID,
				  &rtopo_prof, sizeof(rtopo_prof_evnt_msg_t), 0);	
#endif
#endif
	}

	if((route_info.route_sel_active == TRUE) 
#ifdef ROUTE_ROAM		
		|| (route_info.roaming_enable)
#endif		
		)
	{	
		/* Select new parent device and advertise */
		route_info.parent = nbr;	
		route_info.rank = route_calculate_rank (route_info.parent, 0);

#if 0
		FM_Printf(FM_APP, "\nPar(p 0x%02x, r 0x%02x, l %bu rssi %bu)", route_info.parent->addr, 						
						route_info.parent->rank, route_info.parent->link, route_info.parent->rssi);						
#endif		
		route_state.state = ROUTE_DISCOVER;

		if(nbr->link == WIRELESS)
			route_info.start.wireless.active = FALSE;
		else if(nbr->link == POWER_LINE)
			route_info.start.power_line.active = FALSE;
		
		if(register_state.state == REGISTER_REGISTERED_STATE)
		{
			route_updt_addr_evnt_msg_t route_updt_addr;
			route_updt_addr.event = ROUTE_UPDATE_ADDR_EVNT;
			route_updt_addr.addr = register_data.nwk_addr.addr_16bit;
			GV701x_SendAppEvent(route_app_id, route_app_id, APP_MSG_TYPE_APPEVENT, 
				APP_MAC_ID, EVENT_CLASS_CTRL, MGMT_FRM_ID,
				      &route_updt_addr, sizeof(route_updt_addr), 0);		
		}		

#ifdef ROUTE_ROAM
		route_info.roaming_enable = 0;
#endif
		return TRUE;
	}
	else
		return FALSE;
}

/******************************************************************************
 * @fn      route_add_neighbor
 *
 * @brief   Adds a nearby peer to the neighbor table
 *
 * @param   neigh_addr - the short address of the peer
 *			ieee_addr - the IEEE address of the peer
 *			rank - the rank of the peer
 *			metric - th meteric of the peer
 *			rssi - the rssi value of the peer w.r.t this node
 *			bcn_rx - the beacons received from the peer
 *			lqi - the average LQI value to that peer
 *			parent_addr - the parent short address (if any or-else 0)of the peer 
 *			ch - the wireless channel on which the peer exists  
 * @return  result - TRUE or FALSE
 *
 */

bool route_add_neighbor(u16 neigh_addr, u8* ieee_addr, u16 rank, u8 link, u16 metric,
                         u8 rssi, u32 bcn_rx, u8 lqi, u16 parent_addr, u8 ch, u8 ed)
{
	u8 idx = 0;
	u8 jdx = MAX_NEIGHBOUR_DEVICES + 1;
	u8 cond = FALSE;
	u8 acc_lqi;

#if 1	
	if(ieee_addr) 
		FM_HexDump(FM_APP,"\nNIeee:",ieee_addr,IEEE_MAC_ADDRESS_LEN-2);
#endif

	/*Check whether entry already exists, update it*/
	for(idx = 0; idx < MAX_NEIGHBOUR_DEVICES; idx++) 
	{
#if 0	
		FM_Printf(FM_APP, "\n1. i: %bu v: %bu, n 0x%02x, na 0x%02x rssi %bu", 
			idx, route_neighbor[idx].valid, 
			route_neighbor[idx].addr, neigh_addr, route_neighbor[idx].rssi);
#endif
		if(route_neighbor[idx].valid == TRUE)
		{
			if(ieee_addr != NULL)
			{
#if 0			
				FM_HexDump(FM_ERROR,"\nieee:",route_neighbor[idx].ieee_addr,IEEE_MAC_ADDRESS_LEN-2);
#endif
				if((0 == memcmp(ieee_addr, route_neighbor[idx].ieee_addr, IEEE_MAC_ADDRESS_LEN-2)) &&
					(link == route_neighbor[idx].link))
					cond = TRUE;
				else
					cond = FALSE;
			}
			if(cond == TRUE)
			{
				jdx = idx;
#if 0				
				FM_Printf(FM_APP, "\nNeigh Updt %bu na 0x%02x rssi %bu", jdx, neigh_addr, rssi);			
#endif
				route_neighbor[idx].timeout = MAX_NEIGHBOUR_LIFETIME; 
				if(neigh_addr != 0x0000)
					route_neighbor[idx].cnt++; 
				break;
			}
		}
	}

	/*Add new entry in the table */
	if(jdx >= MAX_NEIGHBOUR_DEVICES) 
	{
		for (idx = 0; idx < MAX_NEIGHBOUR_DEVICES; idx++) 
		{
			if (route_neighbor[idx].valid == FALSE) 
			{			
				jdx = idx;
#if 0				
				FM_Printf(FM_APP, "\nNeigh Add %bu na 0x%02x rssi %bu", jdx, neigh_addr, rssi);
#endif
				route_neighbor[idx].timeout = MAX_NEIGHBOUR_LIFETIME; 
				if(neigh_addr != 0x0000)
					route_neighbor[idx].cnt = 1; 
				break;
			}
		}	
	}

	/*Entry does not exist in the table, check whether it can replace 
	  existing entry based on the path metric */	
	if(jdx >= MAX_NEIGHBOUR_DEVICES) 
	{
		for(idx = 0; idx < MAX_NEIGHBOUR_DEVICES; idx++) 
		{
			if (route_neighbor[idx].valid && ((route_info.parent != NULL)?				
				(route_neighbor[idx].addr != route_info.parent->addr) : TRUE)) 				
			{	
				if (rssi > route_neighbor[idx].rssi) 
				{
					jdx = idx;
#if 0					
					FM_Printf(FM_APP, "\nNeigh Replace na 0x%02x rssi %bu", neigh_addr, rssi);					
#endif
					route_neighbor[idx].timeout = MAX_NEIGHBOUR_LIFETIME; 
					if(neigh_addr != 0x0000)
						route_neighbor[idx].cnt = 1; 
					break;
				}
			}
		}	
	}
	
	if(jdx >= MAX_NEIGHBOUR_DEVICES) 
		return FALSE;
	
	/* This indicates neighbor table is full. We can replace an entry 
	   which has lowest rank except parent device */	

	route_neighbor[jdx].valid = TRUE;
	route_neighbor[jdx].rank = rank;
	route_neighbor[jdx].addr = neigh_addr;
	route_neighbor[jdx].link = link;
	route_neighbor[jdx].probe_mark = FALSE;
	route_neighbor[jdx].path_metric = metric;
	if(rssi != 0)
		route_neighbor[jdx].rssi = rssi;
	if(bcn_rx != 0)
		route_neighbor[jdx].bcn_rx = bcn_rx;
	if(lqi != 0)
	{
		if(route_neighbor[jdx].lqi == 0)
		{
			route_neighbor[jdx].lqi = lqi;	
		}		
		else
		{
			acc_lqi = (((route_neighbor[jdx].lqi  * 80) + (lqi * 20))/100); 
#if 0			
			FM_Printf(FM_APP, "\nalqi %bu l1 %bu l2 %bu", acc_lqi, route_neighbor[jdx].lqi, lqi);
#endif
			route_neighbor[jdx].lqi = acc_lqi;
		}
	}
	if(ch != 0)
		route_neighbor[jdx].ch = ch;	
	
	if(ed != 0)
		route_neighbor[jdx].ed = (0xFF - ed);

	if(parent_addr != 0)
		route_neighbor[jdx].parent_addr = parent_addr;	
	
	route_neighbor[jdx].etx = RTR_LINK_ESTIMATE_UNIT;		
	route_neighbor[jdx].timeout = MAX_NEIGHBOUR_LIFETIME; 
	
	if(ieee_addr != NULL)
		memcpy(route_neighbor[jdx].ieee_addr, ieee_addr, IEEE_MAC_ADDRESS_LEN-2);

#if 1
	FM_Printf(FM_APP, "\nAddN jdx: %bu rank: %u paddr: %x ch %bx lqi %bu ed1 %bx ed %bx l %bx a %x rssi %bx cnt %bu", 	
			jdx, route_neighbor[jdx].rank, route_neighbor[jdx].parent_addr, route_neighbor[jdx].ch, 
			route_neighbor[jdx].lqi, ed, route_neighbor[jdx].ed, route_neighbor[jdx].link, 
			route_neighbor[jdx].addr, route_neighbor[jdx].rssi, route_neighbor[idx].cnt);
#endif
	return TRUE;
}

static u8 route_get_neighbor_index(u8* ieee_addr, u8 link)
{
	u8 idx;

	if(ieee_addr == NULL)
		return MAX_NEIGHBOUR_DEVICES;
#if 0	
	if(ieee_addr) 
		FM_HexDump(FM_APP,"\nRIeee:", ieee_addr, IEEE_MAC_ADDRESS_LEN-2);	
#endif

	for(idx = 0; idx < MAX_NEIGHBOUR_DEVICES; idx++) 
	{
		if(route_neighbor[idx].valid == TRUE)
		{			
			if((0 == memcmp(ieee_addr, route_neighbor[idx].ieee_addr, IEEE_MAC_ADDRESS_LEN-2)) &&
			   (link == route_neighbor[idx].link))
			{
#if 0			
				FM_Printf(FM_APP, "\nG %bu", idx);
#endif
				return idx;
			}			
		}
	}

	return MAX_NEIGHBOUR_DEVICES;
}

bool route_update_neighbor_ed(u8 ch, u8 ed)
{
	u8 idx = 0;
	u8 ret = FALSE;
	
#if 0
	if(ch) 
		FM_Printf(FM_APP, "\nAddNCh: %bx", ch);
#endif

	if(ch == 0)
		ret = FALSE;
	
	/*Check whether entry already exists, update it */
	for (idx = 0; idx < MAX_NEIGHBOUR_DEVICES; idx++) 
	{
#if 0
		FM_Printf(FM_APP, "\n1. i: %bu v: %bu, ch %bx l %bx", idx, route_neighbor[idx].valid, 			
				route_neighbor[idx].ch, route_neighbor[idx].link);
#endif
		if(route_neighbor[idx].valid == TRUE)
		{
			if(route_neighbor[idx].link == WIRELESS)
			{
				if(route_neighbor[idx].ch == ch)
				{
					if(ed != 0)
						route_neighbor[idx].ed = (0xFF - ed);			
#if 0				
					FM_Printf(FM_APP, "\nChM %bu ed %bx ed1 %bx", idx, ed, route_neighbor[idx].ed);
#endif
					ret = TRUE;
				}
			}
		}
	}	
	return ret;
}

/******************************************************************************
 * @fn      route_remove_neighbor
 *
 * @brief   Removes a neighbor from the table
 *
 * @param   neigh_addr - the short address of the peer
 *			link - the link type of the peer
 *
 * @return  result - TRUE or FALSE
 *
 */

static bool route_remove_neighbor(u16 neigh_addr, u8 link)
{
	u8 idx = 0;
	
	link = link;
	for(idx = 0; idx < MAX_NEIGHBOUR_DEVICES; idx++) 
	{
		if ((route_neighbor[idx].valid == TRUE) && 			
			(route_neighbor[idx].addr == neigh_addr) &&
			(route_neighbor[idx].link == link)) 
		{
#if 1		
			FM_Printf(FM_APP, "\nRem(a) idx %bu nbr 0x%02x link %bx isP %bu", idx, neigh_addr,
										route_neighbor[idx].link, 
										(route_info.parent == (&route_neighbor[idx])));
#endif
			route_neighbor[idx].rank = 0;
			route_neighbor[idx].addr = 0;
			route_neighbor[idx].valid = FALSE;
			route_neighbor[idx].link = 0;
			route_neighbor[idx].probe_mark = FALSE;
			route_neighbor[idx].probe_count = FALSE;
			route_neighbor[idx].etx = 0;
			route_neighbor[idx].path_metric = 0;
			route_neighbor[idx].rssi = 0;
			route_neighbor[idx].bcn_rx = 0;
			route_neighbor[idx].lqi = 0;
			route_neighbor[idx].ch = 0;			
			route_neighbor[idx].cnt = 0;	

#if 1		
			if((route_info.parent != NULL) &&
			   (route_info.parent == (&route_neighbor[idx])))
			{
				route_info.parent = NULL;
			}
#endif			
			memset(route_neighbor[idx].ieee_addr, 0x00, IEEE_MAC_ADDRESS_LEN);						
			return TRUE;
		}
	}	
#if 0	
	FM_Printf(FM_APP, "\nCould not remove neighbor 0x%02x", neigh_addr);
#endif
	return FALSE;
}

/******************************************************************************
 * @fn      route_remove_neighbor_using_index
 *
 * @brief   Removes a neighbor from the table looking up the index
 *
 * @param   idx - the index in the table
 *
 * @return  result - TRUE or FALSE
 *
 */

static bool route_remove_neighbor_using_index(u8 idx)
{
	if(route_neighbor[idx].valid == TRUE)
	{
#if 1	
		FM_Printf(FM_APP, "\nRem(i) idx %bu nbr 0x%02x link %bx isP %bu", idx,
									route_neighbor[idx].addr, route_neighbor[idx].link,
									(route_info.parent == (&route_neighbor[idx])));
#endif
		route_neighbor[idx].rank = 0;
		route_neighbor[idx].addr = 0;
		route_neighbor[idx].valid = FALSE;
		route_neighbor[idx].link = 0;
		route_neighbor[idx].probe_mark = FALSE;
		route_neighbor[idx].probe_count = FALSE;
		route_neighbor[idx].etx = 0;
		route_neighbor[idx].path_metric = 0;
		route_neighbor[idx].rssi = 0;
		route_neighbor[idx].bcn_rx = 0;
		route_neighbor[idx].lqi = 0;
		route_neighbor[idx].ch = 0;	
		route_neighbor[idx].cnt = 0;	
#if 1		
		if((route_info.parent != NULL) &&
		   (route_info.parent == (&route_neighbor[idx])))
		{
			route_info.parent = NULL;
		}			
#endif		
		memset(route_neighbor[idx].ieee_addr, 0x00, IEEE_MAC_ADDRESS_LEN);						
		return TRUE;
	}

#if 0
	FM_Printf(FM_APP, "\nCould not remove neighbor 0x%02x", neigh_addr);
#endif
	return FALSE;
}

/******************************************************************************
 * @fn      route_flush_nwktables
 *
 * @brief   Clears all route information
 *
 * @param   none
 *
 * @return  result - TRUE or FALSE
 *
 */

static bool route_flush_nwktables (void)
{
#ifdef ROUTE_PROBE

	STM_StopTimer(route_info.probe.prd_timer);					
	STM_StopTimer(route_info.probe.retrans_timer);
	
#endif	
	STM_StopTimer(route_info.sroute_timer); 
	STM_StopTimer(route_info.discovery_timer);	
	route_info.disc_params.powerline.cnt = 0;
	route_info.disc_params.wireless.cnt = 0;
	route_info.start.wireless.active = FALSE;
	route_info.start.power_line.active = FALSE;	
	route_info.disc_params.link = 0;
	route_info.disc_params.assignparent = 0;
	route_info.rank = INFINITE_RANK;
	route_info.parent = NULL;
	memset((u8*)&route_info.stats, 0x00, sizeof(route_stats_t));	
	memset (&route_neighbor, 0x00, MAX_NEIGHBOUR_DEVICES*(sizeof (neighbor_info_t)));			
	memset(&route_table, 0x00, MAX_ROUTE_TABLE_ENTRIES*(sizeof(route_table_t))); 		
#ifdef HQ_LINK_TEST
	GV701x_GPIO_Write(GREEN_LED, LED_OFF);	
	GV701x_GPIO_Write(RED_LED, LED_OFF);	
#endif
	return TRUE;
}

/******************************************************************************
 * @fn      route_remove_link_neighbor
 *
 * @brief   Removes all neighbors on a particular link
 *
 * @param   link - the link type
 *
 * @return  result - TRUE or FALSE
 *
 */

static bool route_remove_link_neighbor(u8 link)
{
	u8 idx = 0;
	
	for(idx = 0; idx < MAX_NEIGHBOUR_DEVICES; idx++) 
	{
		if((route_neighbor[idx].valid == TRUE) && 
		   (route_neighbor[idx].link == link)) 
		{
#if 1	
			FM_Printf(FM_APP, "\nRem(l)idx %bu nbr 0x%02x link %bx isP %bu", idx,
										route_neighbor[idx].link,
										(route_info.parent == (&route_neighbor[idx])));
#endif
			route_neighbor[idx].rank = 0;
			route_neighbor[idx].addr = 0;
			route_neighbor[idx].valid = FALSE;
			route_neighbor[idx].link = 0;
			route_neighbor[idx].probe_mark = FALSE;
			route_neighbor[idx].probe_count = FALSE;
			route_neighbor[idx].etx = 0;
			route_neighbor[idx].path_metric = 0;
			route_neighbor[idx].rssi = 0;
			route_neighbor[idx].bcn_rx = 0;
			route_neighbor[idx].lqi = 0;
			route_neighbor[idx].ch = 0;	
			route_neighbor[idx].cnt = 0;	
#if 1			
			if((route_info.parent != NULL) &&
			   (route_info.parent == (&route_neighbor[idx])))
			{
				route_info.parent = NULL;
			}			
#endif			
			memset(route_neighbor[idx].ieee_addr, 0x00, IEEE_MAC_ADDRESS_LEN);
			return TRUE;			
		}
	}	
	return FALSE;
}

/******************************************************************************
 * @fn      route_get_neighbor
 *
 * @brief   Fetches a neighbor looking up its short address
 *
 * @param   neigh_addr - the short address of the neighbor
 *
 * @return  neighbor_info_t - a pointer the neighbor
 *
 */

static neighbor_info_t *route_get_neighbor(u16 neigh_addr)
{
	u8 idx = 0;
	
	for (idx = 0; idx < MAX_NEIGHBOUR_DEVICES; idx++) 
	{
		if((route_neighbor[idx].valid) && 
		    (route_neighbor[idx].addr == neigh_addr)) 
			return &route_neighbor[idx];
	}	
	return NULL;
}

/******************************************************************************
 * @fn      route_get_neighbor_link
 *
 * @brief   Fetches a neighbor looking up its link
 *
 * @param   link - the link type of the neighbor
 *
 * @return  neighbor_info_t - a pointer the neighbor
 *
 */

static neighbor_info_t *route_get_neighbor_link(u8 link)
{
	u8 idx = 0;

	for (idx = 0; idx < MAX_NEIGHBOUR_DEVICES; idx++) 
	{
		if((route_neighbor[idx].valid == TRUE) && 
		    (route_neighbor[idx].link == link)) 
		{			
			return &route_neighbor[idx];
		}
	}	
	return NULL;
}

/******************************************************************************
 * @fn      route_select_best_neighbor
 *
 * @brief   Selects the best parent based on the best meteric
 *
 * @param   link - the link on which the best parent is to be found
 *
 * @return  neighbor_info_t - a pointer the best parent
 *
 */

neighbor_info_t *route_select_best_neighbor(u8 link)
{
	u8 idx;
	neighbor_info_t *p = NULL;
	u8 p_idx = 0;
	u8 pold_idx = 0;
	neighbor_info_t *p_old = NULL;	
	
	/* Best parent is selected based on lower rank and best path meteric */	
	for (idx = 0; idx < MAX_NEIGHBOUR_DEVICES; idx++) 
	{		
		if((route_neighbor[idx].valid == TRUE) && (route_neighbor[idx].link == link) &&			
			((link == WIRELESS) ? ((route_info.wireless_ch != 0) ?			
			(route_neighbor[idx].ch == route_info.wireless_ch) : (TRUE)) : TRUE))			
		{
#if 0		
			FM_Printf(FM_APP, "\nCmp i %bu l %bu", idx, route_neighbor[idx].link);			
#endif
			p_old = p;
			pold_idx = p_idx;
			p = route_best_parent(p, &route_neighbor[idx]);
			if(p != NULL)
			{
				p_idx = route_get_neighbor_index(p->ieee_addr, p->link);
				
				if(p_idx == MAX_NEIGHBOUR_DEVICES)
				{
					FM_Printf(FM_APP, "\nBPE");
					return NULL;
				}
			
				if((link == POWER_LINE) || (link == WIRELESS))
				{
					if(p_old != NULL)
					{
						if(p_old == p)
						{			
							if(TRUE == route_remove_neighbor_using_index(pold_idx == p_idx ? idx : pold_idx))
							{
#if 0							
								FM_Printf(FM_APP, "\nNRmv 1.0 %bu", pold_idx == p_idx ? idx : pold_idx);
#endif
							}
						}
						else
						{
							if(TRUE == route_remove_neighbor_using_index(pold_idx == p_idx ? idx : pold_idx))
							{
#if 0							
								FM_Printf(FM_APP, "\nNRmv 1.1 %bu", pold_idx == p_idx ? idx : pold_idx);
#endif
							}
						}
					}
				}
			}
			else
			{
				if((link == POWER_LINE) || (link == WIRELESS))
				{
					if(TRUE == route_remove_neighbor_using_index(pold_idx == p_idx ? idx : pold_idx))
					{
#if 0					
						FM_Printf(FM_APP, "\nNRmv 1.2 %bu", pold_idx == p_idx ? idx : pold_idx);
#endif
					}
				}
			}
		}
	}	

#if 1	
	if(p != NULL)
	{
		if(p->ieee_addr) 
			FM_HexDump(FM_APP,"\nBIeee:", p->ieee_addr, IEEE_MAC_ADDRESS_LEN-2);
		
		FM_Printf(FM_APP, "\nBN pidx: %bu rank: %u paddr: %x ch %bx lqi %bu ed %bx l %bx a %x rssi %bx cnt %bu", p_idx, 
				route_neighbor[p_idx].rank, route_neighbor[p_idx].parent_addr, route_neighbor[p_idx].ch, 
				route_neighbor[p_idx].lqi, route_neighbor[p_idx].ed, route_neighbor[p_idx].link, 
				route_neighbor[p_idx].addr, route_neighbor[p_idx].rssi, route_neighbor[p_idx].cnt);
	}
#endif	
	return p;
}

/******************************************************************************
 * @fn      route_is_neightable_empty
 *
 * @brief   Checks if neigbor table is empty
 *
 * @param   link - seacrh neighbors on this link (WIRELESSS or POWER_LINE)
 *
 * @return  bool - TRUE if neighbor exists otherwise its FALSE 
 *
 */

bool route_is_neightable_empty(u8 link)
{
	u8 idx;
	
	for(idx = 0; idx < MAX_NEIGHBOUR_DEVICES; idx++) 
	{		
		if((route_neighbor[idx].valid == TRUE) &&
		   (route_neighbor[idx].link == link))
			return FALSE;
	}	

	return TRUE;
}

/******************************************************************************
 * @fn      route_periodic_probing
 *
 * @brief   Sends a probe request frame to all neighbors one neighbor 
 * 			in one iteration of the function
 *
 * @param   none
 *
 * @return  none
 *

  */
#ifdef ROUTE_PROBE

static void route_periodic_probing (void) 
{
	u8 idx;
	neighbor_info_t *nbr = NULL;

	if(route_state.state != ROUTE_COMPLETE)	
		return;
	
	for(idx = 0; idx < MAX_NEIGHBOUR_DEVICES; idx++) 
	{		
		if((route_neighbor[idx].valid) && (!route_neighbor[idx].probe_mark) &&			
			(route_info.parent == &route_neighbor[idx])) 
		{	
			route_neighbor[idx].probe_mark = TRUE;			
#if 0			
			FM_Printf(FM_APP, "\nPrb Dev idx %bu addr: %x", idx, route_neighbor[idx].addr);
#endif
			route_create_and_send_probe_req(&route_neighbor[idx], idx);
			return;
		}
	}

	/*One cycle is complete. All are done, unmark all active members*/
	for(idx = 0; idx < MAX_NEIGHBOUR_DEVICES; idx++) 
	{ 	
		if((route_neighbor[idx].valid) && (route_neighbor[idx].probe_mark)) 
			route_neighbor[idx].probe_mark = FALSE;
	}
}

#endif

/******************************************************************************
 * @fn      route_add_route
 *
 * @brief   Adds a route in the routing table 
 *
 * @param   target - the short address of the remote peer
 *			parent - the short address of parent of the remote peer
 *			ieee_addr - the IEEE address of the peer
 *			link_type - the link type of the peer
 *
 * @return  TRUE or FALSE
 *
 */

static bool route_add_route(u16 target, u16 parent, u8 *ieee_addr, u8 link_type)
{
	u8 idx = 0;
	u8 jdx = 0xFF;
	u8 tidx = 0xFF;
	u8 pidx = 0xFF;	

	for(idx = 0; idx < MAX_ROUTE_TABLE_ENTRIES; idx++)		
	{		
		if(route_table[idx].valid) 
		{
			if(route_table[idx].target == parent) 
				pidx = idx;
			
			if(route_table[idx].target == target) 
				tidx = idx;
		} 
		else 
		{
			if(jdx == 0xFF) 
				jdx = idx;
		}
	}
	
	if((tidx != 0xFF) && (pidx != 0xFF)) 
	{		
		route_table[tidx].valid = TRUE;
		route_table[tidx].target = target;				
		route_table[tidx].parent_idx = pidx;
		route_table[tidx].link = link_type;	
		route_table[tidx].timeout = MAX_SROUTE_FAIL_COUNT; 
		return TRUE;
	}

	if((jdx != 0xFF) && (tidx == 0xFF)) 
	{
		route_table[jdx].valid = TRUE;
		route_table[jdx].target = target;				
		route_table[jdx].parent_idx = pidx;
		route_table[jdx].link = link_type;	
		memcpy(route_table[jdx].ieee_address, ieee_addr, MAC_ADDR_LEN);
		route_table[jdx].timeout = MAX_SROUTE_FAIL_COUNT;
		return TRUE;
	}

#if 0	
	FM_Printf(FM_APP,  "\nCouldnt add route");	
#endif
	return FALSE;
}

/******************************************************************************
 * @fn      route_remove_route
 *
 * @brief   Remove a route from the routing table 
 *
 * @param   target - the short address of the remote peer
 * 
 * @return  TRUE or FALSE
 *
 */

static bool route_remove_route (u16 target)
{
	u8 idx = 0;
	for(idx = 0; idx < MAX_ROUTE_TABLE_ENTRIES; idx++)		
	{	
		/* Check if target id is valid */	
		if((route_table[idx].valid) && (route_table[idx].target == target))
		{			
			route_table[idx].valid = FALSE;
			route_table[idx].target = 0;
			route_table[idx].link = 0;
			route_table[idx].parent_idx = MAX_ROUTE_TABLE_ENTRIES;	
			route_table[idx].timeout = 0;			
			return TRUE;			
		}	
	}

#if 0	
	FM_Printf(FM_APP, "\nCouldnt remove route: Invalid Target id");
#endif
	return FALSE;	
}

/******************************************************************************
 * @fn      route_rtable_aging_timer
 *
 * @brief   Perform of all the routes
 *
 * @param   none
 * 
 * @return  none
 *
 */

static void route_rtable_aging_timer (void) 
{
	u8 idx = 0;
	for(idx = 0; idx < MAX_ROUTE_TABLE_ENTRIES; idx++)		
	{	
		/* Check if target id is valid */	
		if (route_table[idx].valid == TRUE) 
		{
			if(route_table[idx].target != route_info.zid) 
			{
#if 0			
				FM_Printf(FM_APP,  "\nRoute rm t - 0x%02x, TO - %bu", route_table[idx].target, route_table[idx].timeout); 
#endif
				if (route_table[idx].timeout) 
					route_table[idx].timeout--;
				else 
					route_remove_route (route_table[idx].target);
			}
		}
	}
}

/******************************************************************************
 * @fn      route_get_route
 *
 * @brief   Fetch the route given the ahort address of the target node
 *
 * @param   target - the short address of the target node
 * 
 * @return  route_table_t - a pointer to the parent to the target node
 *
 */

static route_table_t* route_get_route(u16 target)
{
	u8 idx = 0;
	u8 cond = FALSE;
	
	for(idx = 0; idx < MAX_ROUTE_TABLE_ENTRIES; idx++)		
	{	
#if 0	
		FM_Printf(FM_APP, "\nGet Route idx: %bu v %bu t1 %x t %x", 
		    	idx, route_table[idx].valid, target, route_table[idx].target);	
#endif
		/* Check if target id is valid */	
		if((route_table[idx].valid == TRUE) && (route_table[idx].target == target))
			return &route_table[idx];
	}
#if 0	
	FM_Printf(FM_APP,  "\nCouldnt get route: Invalid Target id");
#endif
	return NULL;	
}

/******************************************************************************
 * @fn      route_get_next_hop
 *
 * @brief   Fetches the short address of the next node towards the target node
 *
 * @param   target - the short address of the target node
 * 
 * @return  route_table_t - a pointer to the next node towards the target node
 *
 */

static route_table_t* route_get_next_hop(u16 target)
{
	u8 idx = 0;
	u8 jdx = 0xFF;
	
	for (idx = 0; idx < MAX_ROUTE_TABLE_ENTRIES; idx++) 
	{	
#if 0	
		FM_Printf(FM_APP, "\nGet Hop idx: %bu v %bu t1 %x t %x", idx, route_table[idx].valid, 
		    	target, route_table[idx].target);			
#endif
		if((route_table[idx].valid == TRUE) && (target == route_table[idx].target)) 
		{
			jdx = idx;
			break;
		}		
	}

	if(jdx != 0xFF) 
	{
		for(idx = 0; idx < MAX_ROUTE_TABLE_ENTRIES; idx++) 
		{	
			if(route_table[route_table[jdx].parent_idx].target == route_info.zid) 
				return &route_table[jdx];
			else 
				jdx = route_table[jdx].parent_idx;
		}	
	}
#if 0	
	FM_Printf(FM_APP,  "\nNo next hop");	
#endif
	return NULL;	
}

/******************************************************************************
 * @fn      route_get_my_header
 *
 * @brief   Fills the routing header on behalf of the application
 *
 * @param   hdr - a pointer to the buffer holding the application frame
 * 			destn - the short address of the destination node
 *			frametype - the framtype of the OTA frame
 *			direction - the direction of the frame
 *
 * @return  TRUE or FALSE
 *
 */

static bool route_get_my_header(route_hdr_t *hdr, u16 destn, u8 frametype, u8 direction)
{
	destn = destn;
	
	if(route_info.parent == NULL) 
		return FALSE;
	
	/* Create FPREQ frame */
	hdr->fc.control_bits = 0;
	hdr->fc.control_bits |= RHDR_SET_DIR(direction);
	hdr->fc.control_bits |= RHDR_SET_CMDID(frametype);
	hdr->fc.control_bits |= RHDR_SET_TOROOT(TRUE);
	hdr->fc.control_bits |= RHDR_SET_ONEHOP(FALSE);

	if(frametype == REGISTRATION_CNF)
	{
#ifdef REGISTER_APP		
		hdr->target = cpu_to_le16(register_data.nwk_addr.addr_16bit);
#endif
	}
	else
		hdr->target = cpu_to_le16(route_info.zid);
	
	hdr->parent = cpu_to_le16(route_info.parent->addr);
	hdr->rank = cpu_to_le16(route_info.rank);
	return TRUE;
}

/******************************************************************************
 * @fn      route_handle_rx_from_ll
 *
 * @brief   Contains all the routing fowarding logic applied to an incoming frame
 *
 * @param   frm - a pointer to the buffer holding the incoming frame
 * 			len - the length in bytes 
 *			link - the link on which the frame is received
 *
 * @return  TRUE or FALSE
 *
 */

bool route_handle_rx_from_ll (u8 *frm, u8 len, u8 link, u8 lqi)
{
	route_table_t *route = NULL;
	route_hdr_t *rhdr = (route_hdr_t *)&frm[sizeof(sEth2Hdr)];
	sEth2Hdr* petherhdr = (sEth2Hdr*)(frm);
	u16 dir = RHDR_GET_DIR(rhdr);
	u8 cmdid = RHDR_GET_CMDID(rhdr); 
	u8 tmac[MAC_ADDR_LEN] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
					
	if(petherhdr->ethtype != APP_ETHER_PROTO)
		return FALSE;

	if (le16_to_cpu(rhdr->parent) == 0xFFFF)
	{
		u8 ret = 0;
#if 0		
		FM_Printf(FM_APP, "\nBcast (l-%bu)",link);
#endif
		ret = route_handle_received_frm (frm, len,  link, lqi);				
		return TRUE;
	}

	/*This device is the target*/	
	if(le16_to_cpu(rhdr->target) == route_info.zid)
	{
		u8 ret = 0;
#if 0		
		FM_Printf(FM_APP, "\nTarget"); 
#endif
		ret = route_handle_received_frm (frm, len, link, lqi);	
	} 
	
	/*This device is the parent*/	
	else if (route_info.parent != NULL) 
	{
#if 0	
		FM_Printf(FM_APP, "\nParent");
		FM_Printf(FM_APP,  "\nC3 p-%x, z-%x", le16_to_cpu(rhdr->parent), route_info.zid);				
#endif
		if(le16_to_cpu(rhdr->parent) == route_info.zid) 
		{
#if 0		
			FM_Printf(FM_APP, "\nC3.0 r %u hr %u", le16_to_cpu(rhdr->rank), route_info.rank); 				
#endif
			/* I am the parent of the device, chek packet direction to avoid any looping */			   			
			if(le16_to_cpu(rhdr->rank) >= route_info.rank) 
			{	
				if(dir != DIRECTION_UP)
				{
#if 0				
					FM_Printf(FM_APP, "\nWrong dir - rk"); 	
#endif
					return FALSE;
				}
			
				/* Sroute is handled only for Bridge devices */								
				if((RHDR_GET_ONEHOP(rhdr) == TRUE) || 
				   (RHDR_GET_CMDID(rhdr) == SOURCE_ROUTE_REQ)) 
				{
#if 0				
					//FM_Printf(FM_APP,  "\nCase 4.0"); 
#endif
					route_handle_received_frm (frm, len, link, lqi);		
				}

				/*Forward frame to parent's parent*/			
				if(RHDR_GET_ONEHOP(rhdr) == FALSE)
				{
					route_info.stats.fwdup++;

					/* Send to parent */
					route_send_frm (route_info.parent->ieee_addr, route_info.ieee_addr, 
								route_info.parent->addr, route_info.zid,
								DIRECTION_UP, frm, len, 
								route_info.parent->link, 
								TRUE, TRUE);
				}
			}
			else if(dir == DIRECTION_DOWN) 
			{
				/* Look up route */
#if 0					
				FM_Printf(FM_APP,  "\nFW-Ch All 1");
#endif

				if((RHDR_GET_CMDID(rhdr) == REGISTRATION_RSP))
				{
#if 0					
					FM_Printf(FM_APP, "\nRegRsp");	
#endif
					rhdr->fc.control_bits |= RHDR_SET_ONEHOP(TRUE);
					
					/* Down direction - Wireless */
					route_send_frm (NULL, route_info.ieee_addr, le16_to_cpu(rhdr->target), 										
									route_info.zid, DIRECTION_DOWN, frm, len, 										
									WIRELESS, TRUE, TRUE);		
					
					/* Down direction - PLC */
					route_send_frm (NULL, route_info.ieee_addr, le16_to_cpu(rhdr->target), 										
									route_info.zid, DIRECTION_DOWN, frm, len, 										
									POWER_LINE, TRUE, TRUE);						
				}
				else
				{
					route = route_get_next_hop (le16_to_cpu(rhdr->target));
					if(route == NULL) 
					{														
						if(CRM_FindScbVendorField(le16_to_cpu(rhdr->target), tmac) == STATUS_SUCCESS)
						{
#if 0							
							FM_Printf(FM_APP, "\nFW-Nh (nh 0x%02x) plc", route->target); 
#endif
							/* Down direction - PLC */
							route_send_frm(tmac, route_info.ieee_addr, le16_to_cpu(rhdr->target), 												
											route_info.zid, DIRECTION_DOWN, frm, len, 												
											POWER_LINE, TRUE, TRUE);
						}
					} 
					else 
					{
						/* Send it to the next hop device */
#if 0							
						FM_Printf(FM_APP,  "\nFW-Nh (nh 0x%02x) zb", route->target); 
#endif
						if(route->link == WIRELESS)
						{
							route_send_frm (route->ieee_address, route_info.ieee_addr, 
										    route->target, route_info.zid, DIRECTION_DOWN, frm, len, 											
										    route->link, TRUE, TRUE);											
						}
						else if(CRM_FindScbVendorField(le16_to_cpu(rhdr->target), tmac) == STATUS_SUCCESS)
						{								
							route_send_frm(tmac, route_info.ieee_addr, route->target, route_info.zid,											
										   DIRECTION_DOWN, frm, len, route->link, TRUE, TRUE);
						}
					}							
				}						
			}	
		}
		else 
		{
			/* I am not parent and I am not target, so need to forward it */
#if 0			
			FM_Printf(FM_APP,  "\nInter"); 
#endif
			if(le16_to_cpu(rhdr->rank) > route_info.rank) 
			{
#if 0			
				FM_Printf(FM_APP, "\nFW-P"); 	
#endif
				if(dir != DIRECTION_UP)
				{
						return FALSE;
				}

				/* Sroute is handled only for Bridge devices */ 							
				if ((RHDR_GET_CMDID(rhdr) == SOURCE_ROUTE_REQ)) 
				{				
					route_handle_received_frm (frm, len, link, lqi); 	
				}

				/* Direction is up */
				/* Send it to parent */				
				route_send_frm(route_info.parent->ieee_addr, route_info.ieee_addr, route_info.parent->addr,
							   route_info.zid, DIRECTION_UP, frm, len, route_info.parent->link, TRUE, TRUE);																					
			} 
			else 
			{
#if 0			
				FM_Printf(FM_APP,  "\nFW-C"); 	
#endif
				if(dir != DIRECTION_DOWN) 
					return FALSE;
				
				/* Direction is DOWN */
				if(RHDR_GET_ONEHOP(rhdr) == FALSE)
				{																	
					/* Get route of the parent device */
					route = route_get_next_hop (le16_to_cpu(rhdr->parent));
					if (route == NULL) 
					{
#if 0					
						FM_Printf(FM_APP,  "\nNo route"); 			
#endif
						return FALSE;			
					} 
					else 
					{
						/* Send it to the next hop device */
#if 0						
						FM_Printf(FM_APP, "\nFW-Nh (nh 0x%02x)", route->target); 
#endif
						route_send_frm (route->ieee_address, route_info.ieee_addr, route->target, 
										route_info.zid, DIRECTION_DOWN, frm, len, route->link, TRUE, TRUE);
					}
				}
				else
				{
#if 0				
					FM_Printf(FM_APP, "\nReject(On hop only)");
#endif
				}
			}
		}		
	}
	/*Target is broadcast*/
	else if (le16_to_cpu(rhdr->target) == 0xFFFF) 
	{
		return FALSE;
	}
	/*This device is a forwarder or router*/	
	else
	{
#if 0	
		FM_Printf(FM_APP, "\nNo target 0x%02x, No parent 0x%02x", 
				le16_to_cpu(rhdr->target), 
				le16_to_cpu(rhdr->parent));
#endif		
		return FALSE;
	}
	return TRUE;
}

/******************************************************************************
 * @fn      route_send_to_ll
 *
 * @brief   Application uses this to send a frame OTA
 *
 * @param   buff - a pointer to the buffer holding the application frame
 * 			len - the length in bytes 
 *			frametype - the frametype of the application frame
 *			ether_hop_en - TRUE if frame is destined to the HOST, 
 *						   FALSE if the destination is the next hop towards the Host
 * @return  TRUE or FALSE
 *
 */

bool route_send_to_ll (u8 *buff, u8 len, u8 frametype, u8 ether_hop_en) 
{
	route_hdr_t* rhdr = NULL;
	sEth2Hdr* petherhdr = (sEth2Hdr*)(buff);
	
	if(((route_state.state != ROUTE_DISCOVER) && (route_state.state != ROUTE_COMPLETE)) 
		 || (route_info.parent == NULL))
		return FALSE;

	rhdr = (route_hdr_t*)&buff[sizeof(sEth2Hdr)];
	
	if(ether_hop_en == FALSE)
		memcpy(petherhdr->dstaddr, route_info.parent->ieee_addr, MAC_ADDR_LEN);	
	else		
		memcpy(petherhdr->dstaddr, route_info.root_ieee_addr, MAC_ADDR_LEN);

	memcpy(petherhdr->srcaddr, route_info.ieee_addr, MAC_ADDR_LEN);

	if(FALSE == route_get_my_header (rhdr, route_info.parent->addr, frametype, DIRECTION_UP)) 
	{
#if 0	
		FM_Printf(FM_APP, "\nRHfail"); 
#endif
		return FALSE;
	}

	if(FALSE == route_send_frm (petherhdr->dstaddr, petherhdr->srcaddr, route_info.parent->addr, 
					route_info.zid, DIRECTION_UP, buff, len, route_info.parent->link, TRUE, FALSE))
		return FALSE;

	return TRUE;
}

/******************************************************************************
 * @fn      route_send_to_ul
 *
 * @brief   Place holder for application to place their receive handlers
 *
 * @param   frm - a pointer to the received frame
 * 			len - the length in bytes 
 *			link - the link on which the frame is received
 *
 * @return  TRUE or FALSE
 *
 */

u8 route_send_to_ul (u8* frm, u8 len, u8 link)
{
	if((link == POWER_LINE) || (link == WIRELESS))
	{
#ifdef LLP_APP
		LlpApp_Rx(frm, len);
#endif
#ifdef RTOPO_APP
		rtopo_rx(frm, len);
#endif
		return TRUE;
	}	
	return FALSE;
}

/******************************************************************************
 * @fn      RouteApp_Poll
 *
 * @brief   A routine called perodically for a feature demading routine execution
 *
 * @param   none
 *
 * @return  none
 *
 */

void RouteApp_Poll(void)
{
}

void RouteApp_SendEvent(u8 ind, u8 link, u8 reason)
{	
	u8 i, num_app = 0;
	u8 app_id[2];
	
	if(ind == ROUTE_DWN_IND)
	{
		route_dwn_ind_msg_t route_dwn;	
#ifndef RTOPO_APP
#ifdef NWKSTARTUP_APP		
		nwk_start_evnt_msg_t nwk_start;
#endif
#endif

#if 0		
		FM_Printf(FM_APP, "\nRD l %bu r %bu", link, reason);		
#endif
		if(link == (WIRELESS | POWER_LINE))
		{
			route_flush_nwktables();		
			route_state.state = ROUTE_INIT;
			route_dwn.event = ROUTE_DWN_IND;
			route_dwn.link = link;
			route_dwn.reason =  reason;
			if(route_info.start.power_line.app_id != 
				route_info.start.wireless.app_id)
			{
				num_app = 2;
				app_id[0] = route_info.start.power_line.app_id;
				app_id[1] = route_info.start.wireless.app_id;
			}
			else
			{
				num_app = 1;
				app_id[0] = route_info.start.power_line.app_id;
				app_id[0] = route_info.start.wireless.app_id;
			}

			route_info.start.wireless.app_id = 0;
			route_info.start.wireless.active = FALSE;

			route_info.start.power_line.app_id = 0;
			route_info.start.power_line.active = FALSE; 			
				
#ifndef RTOPO_APP
#ifdef NWKSTARTUP_APP
			nwk_start.event = NWK_START_EVENT;
			nwk_start.link = route_dwn->link;						
			GV701x_SendAppEvent(rtopo_app_id, nwkstartup_data.app_id, APP_MSG_TYPE_APPEVENT, 
					APP_MAC_ID, EVENT_CLASS_CTRL, MGMT_FRM_ID,			
					&nwk_start, sizeof(nwk_start_evnt_msg_t), 0);
#endif
#endif
		}		
		else
		{
			if(route_info.parent != NULL)
			{
				if(route_info.parent->link & link)
				{
#if 1				
					FM_Printf(FM_APP, "\nPfail");
#endif
					route_remove_neighbor(route_info.parent->addr, 
										  route_info.parent->link);				
#ifdef ROUTE_PROBE

					STM_StopTimer(route_info.probe.prd_timer);					
					STM_StopTimer(route_info.probe.retrans_timer);
					
#endif	
					STM_StopTimer(route_info.sroute_timer); 
					route_info.rank = INFINITE_RANK;
					route_info.parent = NULL;
					memset((u8*)&route_info.stats, 0x00, sizeof(route_stats_t));	
				}							
			}

			if(link & WIRELESS)
			{
				route_info.disc_params.wireless.cnt = 0;
				route_info.disc_params.link &= ~WIRELESS;	
				route_info.disc_params.assignparent &= ~WIRELESS;
				app_id[0] = route_info.start.wireless.app_id;
				num_app = 1;
				route_info.start.wireless.app_id = 0;
				route_info.start.wireless.active = FALSE;
				route_remove_link_neighbor(WIRELESS);				
			}
			else if(link & POWER_LINE)
			{
				route_info.disc_params.powerline.cnt = 0;
				route_info.disc_params.link &= ~POWER_LINE; 
				route_info.disc_params.assignparent &= ~POWER_LINE;				
				app_id[0] = route_info.start.power_line.app_id;
				num_app = 1;
				route_info.start.power_line.app_id = 0;
				route_info.start.power_line.active = FALSE;		
				route_remove_link_neighbor(POWER_LINE);
			}	
				
			if(route_info.disc_params.link == 0)		
			{
#if 1			
				FM_Printf(FM_APP, "\nDiscStop");
#endif
				STM_StopTimer(route_info.discovery_timer);				
			}
			route_dwn.event = ROUTE_DWN_IND;
			route_dwn.link = link;
			route_dwn.reason = reason;
		}

		for(i = 0; i < num_app; i++)
		{
			GV701x_SendAppEvent(route_app_id, app_id[i], APP_MSG_TYPE_APPIND, 
					APP_MAC_ID, EVENT_CLASS_CTRL, MGMT_FRM_ID,			
					&route_dwn, sizeof(route_dwn_ind_msg_t), 0);	
		}
	}
	else if(ind == ROUTE_UP_IND)
	{
		route_up_ind_msg_t route_up;
		route_up.event = ROUTE_UP_IND;
		route_up.link = link;		

#if 0		
		FM_Printf(FM_APP, "Route Up (%bu)", link);		
#endif		
		if(route_info.start.power_line.app_id != 
			route_info.start.wireless.app_id)
		{
			num_app = 2;
			app_id[0] = route_info.start.power_line.app_id;
			app_id[1] = route_info.start.wireless.app_id;
		}
		else
		{
			num_app = 1;
			app_id[0] = route_info.start.power_line.app_id;
			app_id[0] = route_info.start.wireless.app_id;
		}		

		for(i = 0; i < num_app; i++)
		{		
			GV701x_SendAppEvent(route_app_id, app_id[i], APP_MSG_TYPE_APPIND, 
					APP_MAC_ID, EVENT_CLASS_CTRL, MGMT_FRM_ID,			
					&route_up, sizeof(route_up_ind_msg_t), 0);								
		}
	}
	else if(ind == ROUTE_DISC_IND)
	{
		route_disc_ind_msg_t route_disc;
		route_disc.event = ROUTE_DISC_IND;
		route_disc.link = link;
		route_disc.reason = reason;

#if 0
		FM_Printf(FM_APP, "Route Disc (%bu)", link);		
#endif		
		if(route_info.start.power_line.app_id != 
			route_info.start.wireless.app_id)
		{
			num_app = 2;
			app_id[0] = route_info.start.power_line.app_id;
			app_id[1] = route_info.start.wireless.app_id;
		}
		else
		{
			num_app = 1;
			app_id[0] = route_info.start.power_line.app_id;
			app_id[0] = route_info.start.wireless.app_id;
		}		

		for(i = 0; i < num_app; i++)
		{		
			GV701x_SendAppEvent(route_app_id, app_id[i], APP_MSG_TYPE_APPIND, 
					APP_MAC_ID, EVENT_CLASS_CTRL, MGMT_FRM_ID,			
					&route_disc, sizeof(route_disc_ind_msg_t), 0);								
		}		
	}
}

/******************************************************************************
 * @fn      RouteApp_StateMachine
 *
 * @brief   The Routing State Machine, it executes all internal/external 
 *			events triggered
 *
 * @param   state - state machine object of the driver
 *          (passed as a reference incase there are more than one object)
 *
 * @return  none
 *
 */

void RouteApp_StateMachine(gv701x_state_t* state) 
{
	if(state == NULL)
		return;

#if 1
	if(state->event != ROUTE_IDLE_EVNT)
		FM_Printf(FM_APP, "\nRoute S %bu E %bu P %bu C %bu E %bu Da %bu Sa %bu T %bu", 
				state->state, state->event,
				state->eventproto, state->eventclass, state->eventtype, 
				state->msg_hdr.dst_app_id, state->msg_hdr.src_app_id, state->msg_hdr.type);
#endif
	switch(state->state)
	{
		case ROUTE_INIT:
			if(state->eventproto == APP_MAC_ID)
			{	
				if(state->msg_hdr.type == APP_MSG_TYPE_APPEVENT)
				{			
					switch(state->event)
					{
						case ROUTE_START_EVNT:					
						{
							route_start_evnt_msg_t* route_start;
							route_start = (route_start_evnt_msg_t*)(state->statedata);						
							
							if(((route_info.start.wireless.active == FALSE) &&
								(route_start->link & WIRELESS)) ||
								((route_info.start.power_line.active == FALSE) &&
								(route_start->link & POWER_LINE)))						
							{					
								if(route_start->link & POWER_LINE)
								{
									route_info.start.power_line.active = TRUE;
									route_info.start.power_line.app_id = state->msg_hdr.src_app_id;
								}
								if(route_start->link & WIRELESS)
								{
									route_info.start.wireless.active = TRUE;
									route_info.start.wireless.app_id = state->msg_hdr.src_app_id;								
								}	
								RouteApp_StartDiscovery(route_start->link, route_start->assignparent, route_start->link);
							}
						}
						break;
										
						default:
						break;				
					}
				}
			}
		break;
		
		case ROUTE_START:
			if(state->eventproto == APP_MAC_ID)
			{		
				if(state->msg_hdr.type == APP_MSG_TYPE_APPEVENT)
				{			
					switch(state->event)
					{														
						case ROUTE_START_EVNT:					
						{
							route_start_evnt_msg_t* route_start;
							route_start = (route_start_evnt_msg_t*)(state->statedata);						
							
							if(((route_info.start.wireless.active == FALSE) &&
								(route_start->link == WIRELESS)) ||
								((route_info.start.power_line.active == FALSE) &&
								(route_start->link == POWER_LINE))) 					
							{							
								if(route_start->link == POWER_LINE)
								{
									route_info.start.power_line.active = TRUE;
									route_info.start.power_line.app_id = state->msg_hdr.src_app_id;
								}
								if(route_start->link == WIRELESS)
								{
									route_info.start.wireless.active = TRUE;
									route_info.start.wireless.app_id = state->msg_hdr.src_app_id;								
								}	
								RouteApp_StartDiscovery(route_start->link, route_start->assignparent, route_start->link);
							}
						}
						break;
					
						case ROUTE_STOP_EVNT:
						{
							route_dwn_ind_msg_t* route_dwn = (route_dwn_ind_msg_t*)(state->statedata);					
							RouteApp_SendEvent(ROUTE_DWN_IND, route_dwn->link, ROUTE_REASON_MANUAL);
						}
						break;
						
						default:
						break;
					}
				}
			}
		break;
		
		case ROUTE_DISCOVER:
			if(state->eventproto == APP_MAC_ID)
			{			
				if(state->msg_hdr.type == APP_MSG_TYPE_APPEVENT)
				{			
					switch(state->event)
					{					
						case ROUTE_UPDATE_ADDR_EVNT:			
						{
							route_updt_addr_evnt_msg_t* route_update;
							route_update = (route_updt_addr_evnt_msg_t*)state->statedata;			
#ifdef REGISTER_APP						
							route_info.zid = route_update->addr;
#endif											
							STM_StopTimer(route_info.sroute_timer);
							if(TRUE == route_create_and_send_sroute_frame(route_info.zid)) 
							{
								STM_StartTimer(route_info.sroute_timer, 
											   MAX_SROUTE_PERIODIC_INTERVAL_LOW);
							}
						}
						break;

						case ROUTE_UPDATE_ROUTE_EVNT:
						{
							state->state = ROUTE_COMPLETE;
							route_info.unreachable = FALSE;
#ifdef ROUTE_RECOVERY							
							STM_StopTimer(route_info.rec_timer);
#endif
							RouteApp_SendEvent(ROUTE_UP_IND, route_info.parent->link, ROUTE_REASON_NONE);
#ifdef HQ_LINK_TEST
							if(route_info.parent->link == WIRELESS)						
							{
								GV701x_GPIO_Write(RED_LED, LED_ON);	
								GV701x_GPIO_Write(GREEN_LED, LED_OFF);	
							}
							else if(route_info.parent->link == POWER_LINE)
							{
								GV701x_GPIO_Write(GREEN_LED, LED_ON);	
								GV701x_GPIO_Write(RED_LED, LED_OFF);	
							}		
#endif
#ifdef RTOPO_APP	
							if(route_info.route_sel_active == TRUE)
							{						
								rtopo_prof_evnt_msg_t rtopo_prof;
								rtopo_prof.event = RTOPO_PROFILE_EVNT; 
								GV701x_SendAppEvent(route_app_id, rtopo_app_id, APP_MSG_TYPE_APPEVENT, 
									APP_MAC_ID, EVENT_CLASS_CTRL, MGMT_FRM_ID,
										  &rtopo_prof, sizeof(rtopo_prof_evnt_msg_t), 0);	
							}
#endif
#if 0
							FM_Printf(FM_APP, "\nRC 0x%04x)", route_info.zid);	
#endif
#ifdef NWKSTARTUP_APP					
							nwkstartup_data.link.wireless.addr = route_info.zid; 
							nwkstartup_data.link.power_line.addr = route_info.zid; 
#endif
							if(route_add_route(route_info.zid, 0, route_info.ieee_addr, route_info.parent->link) == TRUE)
								route_create_and_send_fprsp_frame (NULL, 0xFFFF, WIRELESS | POWER_LINE);											

#ifdef ROUTE_PROBE							
							if(route_device_profile.device_type == DEV_BRIDGING) 
							{ 
								STM_StopTimer(route_info.probe.prd_timer);						
								STM_StartTimer(route_info.probe.prd_timer, MAX_PROBE_INTERVAL);
							}
#endif							
							
#ifdef HPGP_DRIVER_APP					
							memcpy(hpgp_nwk_data.params.nwk.app_info.byte_arr, 
									route_info.ieee_addr, MAC_ADDR_LEN);
							memcpy((u8*)&hpgp_nwk_data.params.nwk.app_info.byte_arr[MAC_ADDR_LEN], 
								   (u8*)&route_info.parent->addr, sizeof(route_info.parent->addr));
#endif			
						}
						break;

						case ROUTE_START_EVNT:					
						{
							route_start_evnt_msg_t* route_start;
							route_start = (route_start_evnt_msg_t*)(state->statedata);						
							
							if(((route_info.start.wireless.active == FALSE) &&
								(route_start->link == WIRELESS)) ||
								((route_info.start.power_line.active == FALSE) &&
								(route_start->link == POWER_LINE))) 					
							{							
								if(route_start->link == POWER_LINE)
								{
									route_info.start.power_line.active = TRUE;
									route_info.start.power_line.app_id = state->msg_hdr.src_app_id;
								}
								if(route_start->link == WIRELESS)
								{
									route_info.start.wireless.active = TRUE;
									route_info.start.wireless.app_id = state->msg_hdr.src_app_id;								
								}								
								RouteApp_StartDiscovery(route_start->link, route_start->assignparent, route_start->link);
							}
						}
						break;

						case ROUTE_STOP_EVNT:					
						{
							route_dwn_ind_msg_t* route_dwn = (route_dwn_ind_msg_t*)(state->statedata);					
							RouteApp_SendEvent(ROUTE_DWN_IND, route_dwn->link, ROUTE_REASON_MANUAL);
						}
						break;
														
						default:
						break;				
					}
				}
			}
		break;		

		case ROUTE_COMPLETE:
			if(state->eventproto == APP_MAC_ID)
			{		
				if(state->msg_hdr.type == APP_MSG_TYPE_APPEVENT)	
				{
					switch(state->event)
					{
						case ROUTE_START_EVNT:
						{
							route_start_evnt_msg_t* route_start;
							route_start = (route_start_evnt_msg_t*)(state->statedata);						
							
							if(((route_info.start.wireless.active == FALSE) &&
								(route_start->link == WIRELESS)) ||
								((route_info.start.power_line.active == FALSE) &&
								(route_start->link == POWER_LINE))) 					
							{							
								if(route_start->link == POWER_LINE)
								{
									route_info.start.power_line.active = TRUE;
									route_info.start.power_line.app_id = state->msg_hdr.src_app_id;
								}
								if(route_start->link == WIRELESS)
								{
									route_info.start.wireless.active = TRUE;
									route_info.start.wireless.app_id = state->msg_hdr.src_app_id;								
								}		
								
								RouteApp_StartDiscovery(route_start->link,
												route_start->assignparent,
												route_start->link); 			
							}
						}
						break;

						case ROUTE_STOP_EVNT:
						{
							route_dwn_ind_msg_t* route_dwn = (route_dwn_ind_msg_t*)(state->statedata);					
							RouteApp_SendEvent(ROUTE_DWN_IND, route_dwn->link, ROUTE_REASON_MANUAL);
						}

						break;
										
						default:
						break;				
					}
				}
			}
		break;
		
		default:
		break;		
	}


	state->event = ROUTE_IDLE_EVNT;	
	state->eventtype = 0;
	state->eventclass = 0;
	state->eventproto = 0;
	state->statedata = NULL;	
	state->statedatalen = 0;	
	memset((u8*)&state->msg_hdr, 0x00, sizeof(gv701x_app_msg_hdr_t));
}

/******************************************************************************
 * @fn      RouteApp_StartDiscovery
 *
 * @brief   This function starts the discovery process on the given link/s
 *
 * @param   link - a bitmap indicating which link discovery is to be performed
 *			cont_disc - a bitmap indicating which link to continue to discover,
 *						set to TRUE to continue to discover, FALSE otherwise.
 *
 * @return  status - STATUS_SUCCESS/STATUS_FAILURE
 *
 */

void RouteApp_StartDiscovery(u8 link, u8 assignparent, u8 cont_disc) 
{
	u16 time; 
	neighbor_info_t* wir_nbr = NULL;
	neighbor_info_t* pwr_nbr = NULL;
#ifdef HPGP_DRIVER_APP				
	route_info.zid = nwkstartup_data.link.power_line.addr;
#endif				
#ifdef LRWPAN_DRIVER_APP				
	route_info.zid = nwkstartup_data.link.wireless.addr;
#endif				

#ifdef NWKSTARTUP_APP								
	memcpy(route_info.ieee_addr, (u8*)nwkstartup_data.link.long_addr.ieee_addr, 
			IEEE_MAC_ADDRESS_LEN);				
#endif

	if(route_state.state == ROUTE_INIT)
		route_state.state = ROUTE_START;
	
	if((link & POWER_LINE) && (!(route_info.disc_params.link & POWER_LINE)))
	{
		route_info.disc_params.powerline.cnt = 1;		
		route_info.disc_params.powerline.active = TRUE;
	}
	
	if((link & WIRELESS) && (!(route_info.disc_params.link & WIRELESS)))
	{
		route_info.disc_params.wireless.cnt = 1;		
		route_info.disc_params.wireless.active = TRUE;
	}
	
	route_info.disc_params.link |= link;	
	route_info.disc_params.assignparent |= assignparent;
	STM_StopTimer(route_info.discovery_timer);

	srand(route_info.zid);
	time = (u16)rand(); 	
	time = time % (0xFFFF / ROUTE_DISCOVER_PERIOD_HIGH);

#if 0	
	FM_Printf(FM_APP, "\nS dl %bu t %u", route_info.disc_params.link, 
			((time < ROUTE_DISCOVER_PERIOD_LOW) ? (ROUTE_DISCOVER_PERIOD_LOW) : (time)));
#endif

	/* Start timer to start route discovery */		
	if (STATUS_FAILURE == STM_StartTimer (route_info.discovery_timer, 
		((time < ROUTE_DISCOVER_PERIOD_LOW) ? (ROUTE_DISCOVER_PERIOD_LOW) : (time)))) 
	{
		//FM_Printf(FM_APP, "\nCould not start route discovery timer"); 
	}		
	
	if((link & WIRELESS) && ((route_info.disc_params.assignparent & WIRELESS) == WIRELESS))
	{
		wir_nbr = route_get_neighbor_link(WIRELESS);
		if((cont_disc & WIRELESS) && (wir_nbr != NULL))
			wir_nbr->cnt = 0;
	}

	if((link & POWER_LINE) && ((route_info.disc_params.assignparent & POWER_LINE) == POWER_LINE))
	{
		pwr_nbr = route_get_neighbor_link(POWER_LINE);
		if((cont_disc & POWER_LINE) && (pwr_nbr != NULL))
			pwr_nbr->cnt = 0;
	}
	
	/* Create Discovery Frame */
	route_create_discover_fpreq_frame(route_info.zid, 														
							((wir_nbr != NULL) ? (wir_nbr->ieee_addr) : (NULL)),
							((wir_nbr != NULL) ? (wir_nbr->addr) : (BROADCAST_NET_ADDR)),
							((pwr_nbr != NULL) ? (pwr_nbr->ieee_addr) : (NULL)),
							((pwr_nbr != NULL) ? (pwr_nbr->addr) : (BROADCAST_NET_ADDR)), link);				
}

/******************************************************************************
 * @fn      RouteApp_DispStats
 *
 * @brief   Displays all frame counters, states and other statistics
 *
 * @param   none
 *
 * @return  none
 *
 */

void RouteApp_DispStats(void)
{
#if 1
	if(route_info.parent != NULL)
	{
		printf("\nRoute: a 0x%02x ", route_info.zid);
		FM_HexDump(FM_USER,"ieee ", route_info.parent->ieee_addr, IEEE_MAC_ADDRESS_LEN);	
		printf("\nparl %s ", ((route_info.parent->link == WIRELESS) ? 
				"Wir" : "Pwr"));				
		printf("\nr 0x%02x", route_info.rank);	
		if(route_info.parent != NULL)
			printf("\nrl %s", ((route_info.parent->link == WIRELESS) ? 
					"BCCO" : "BSTA"));							
	}
#if 0
	printf("\nm(plc)- rssi %bu bcnt %lu (rf)- lqi %bu ract %bu", 
			route_info.rssi_threshold, route_info.bcn_threshold, 
			route_info.lqi_threshold, route_info.route_sel_active);
#endif
#endif	
#if 0
	printf("\nsr_rx %lu sr_tx %lu", route_info.stats.sroute_rx,
			route_info.stats.sroute_tx);
	printf("\nfpreq_tx %lu fpreq_rx %lu", route_info.stats.fpreq_tx,
			route_info.stats.fpreq_rx);
	printf("\nfprsp_tx %lu fprsp_rx %lu", route_info.stats.fprsp_tx,
			route_info.stats.fprsp_rx);	
	printf("\npreq_tx %lu preq_rx %lu", route_info.stats.preq_tx,
			route_info.stats.preq_rx);	
	printf("\nprsp_tx %lu prsp_rx %lu", route_info.stats.prsp_tx,
			route_info.stats.prsp_rx);	
	printf("\nfwdup %lu", route_info.stats.fwdup);		
#endif


}

/******************************************************************************
 * @fn      RouteApp_DispNeighTable
 *
 * @brief   Displays the Neighbor table
 *
 * @param   none
 *
 * @return  none
 *
 */

void RouteApp_DispNeighTable(void)
{
#if 0
	u8 i;
	for (i = 0; i < MAX_NEIGHBOUR_DEVICES; i++) 
	{
		if (route_neighbor[i].valid == TRUE) 
		{
			printf("\n%bu. l %bu a 0x%02x", i, route_neighbor[i].link, route_neighbor[i].addr);
			FM_HexDump(FM_USER,"ieee ", route_neighbor[i].ieee_addr, IEEE_MAC_ADDRESS_LEN);				
			if(route_neighbor[i].link == POWER_LINE) 
				printf("\nm rssi %bu bcnrx %lu cnt %bu paddr %x", route_neighbor[i].rssi, route_neighbor[i].bcn_rx,
				        route_neighbor[i].cnt, route_neighbor[i].parent_addr);
			else if (route_neighbor[i].link == WIRELESS) 				
				printf("\nm ch %bx ed %bx lqi %bu", route_neighbor[i].ch, 
					  (0xFF - route_neighbor[i].ed), route_neighbor[i].lqi);			
		}
	}
#endif	
}

/******************************************************************************
 * @fn      RouteApp_DispRouteTable
 *
 * @brief   Displays the Routing table
 *
 * @param   none
 *
 * @return  none
 *
 */

void RouteApp_DispRouteTable(void)
{
#if 0
	u8 i= 0;

	for(i = 0; i < MAX_ROUTE_TABLE_ENTRIES; i++) 
	{
		if (route_table[i].valid == TRUE) 
		{
			printf("\n%bu. l %bu ", i, route_table[i].link);
			FM_HexDump(FM_USER,"ieee ", route_table[i].ieee_address, MAC_ADDR_LEN);				
			printf("\nt %x pid %bu", i, route_table[i].target, route_table[i].parent_idx);					
		}
	}
#endif	
}

/******************************************************************************
 * @fn      RouteApp_CmdProcess
 *
 * @brief   It handles application command line requests
 *
 * @param   CmdBuf - command string
 *
 * @return  none
 *
 */

void RouteApp_CmdProcess(char* CmdBuf) 
{
	if(strcmp(CmdBuf, "state") == 0)
	{		
		printf("\nRoute S %bu E %bu", route_state.state, route_state.event);		
	}	
	else if(strcmp(CmdBuf, "stats") == 0)
	{
		char* subcmd = NULL;
		subcmd = strtok(CmdBuf, " ");
		subcmd = strtok(NULL, "\0");

#if 0		
		if(strcmp(subcmd, "rt") == 0)
		{
			RouteApp_DispRouteTable(); 	
		}
		else if(strcmp(subcmd, "nt") == 0) 
		{
			RouteApp_DispNeighTable(); 		
		} 		
		else if((strcmp(subcmd, "all") == 0) ||
			    (subcmd == NULL))
#endif			    
		{	
			RouteApp_DispStats();			
			RouteApp_DispNeighTable();					
			RouteApp_DispRouteTable();				
		}		
	} 
#if 0
	else if(strcmp(CmdBuf, "test") == 0)
	{
		char testname[15];		
		u32 testval;
		
		memset(testname, 0x00, sizeof(testname));

		if(sscanf(CmdBuf + sizeof("test"), "%s %lu", testname, &testval) >= 1)
		{		
			if(strcmp(testname, "rssithr") == 0)
			{
				route_info.rssi_threshold = (u8)testval;
			}
			else if(strcmp(testname, "bcnthr") == 0)
			{
				route_info.bcn_threshold = (u32)testval;
			}			
			else if(strcmp(testname, "lqithr") == 0)
			{
				route_info.lqi_threshold = (u8)testval;
			}						
			else if(strcmp(testname, "lqidiff") == 0)
			{
				route_info.lqi_diff = (u8)testval;
				GV701x_FlashWrite(route_app_id, (u8*)&route_info.lqi_diff, 2);					
			}									
			else if(strcmp(testname, "eddiff") == 0)
			{
				route_info.ed_diff = (u8)testval;
				GV701x_FlashWrite(route_app_id, (u8*)&route_info.lqi_diff, 2);				
			}
		}
	}	
#endif	
	else if(strcmp(CmdBuf, "nvclear") == 0) 
	{
		GV701x_FlashErase(route_app_id);
	}	
}

#endif /*ROUTE_APP*/
