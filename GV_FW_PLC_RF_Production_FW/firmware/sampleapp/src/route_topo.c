/* ========================================================
 *
 * @file: route_topo.c
 * 
 * @brief: This file supports all routines required 
 *         for mesh uplink and downlink decision making
 *         and profiling
 *      
 *  Copyright (C) 2010-2015, Greenvity Communications, Inc.
 *  All Rights Reserved
 *  
 * =========================================================*/

#ifdef RTOPO_APP

/****************************************************************************** 
  *	Includes
  ******************************************************************************/
  
#include "stdio.h"
#include "string.h"
#include "gv701x_includes.h"
#ifdef ROUTE_APP
#include "route.h"
#include "route_fw.h"
#endif
#ifdef HPGP_DRIVER_APP
#include "gv701x_hpgpdriver.h"
#endif
#ifdef LRWPAN_DRIVER_APP
#include "gv701x_lrwpandriver.h"
#endif
#ifdef NWKSTARTUP_APP
#include "gv701x_nwkstartup.h"
#endif
#include "route_topo_fw.h"
#include "route_topo.h"

/****************************************************************************** 
  *	Global Data
  ******************************************************************************/
u8 rtopo_app_id;
gv701x_app_queue_t rtopo_queues;	
rtopo_data_t rtopo_data;
gv701x_state_t rtopo_state;

#ifndef OLD_SCAN
static u8 rtopo_rf_curscan_ch = MIN_CHANNEL;
#endif 

/****************************************************************************** 
  *	External Data
  ******************************************************************************/
  
/******************************************************************************
  *	External Funtion prototypes
  ******************************************************************************/
  
/******************************************************************************
  * Funtion prototypes
  ******************************************************************************/

void rtopo_start_devrole(u8 link);
void rtopo_handle_timeout(void);
#ifndef OLD_SCAN
void rtopo_start_wirscan_engine(u8 mode);
#endif

/******************************************************************************
 * @fn      rtopo_init
 *
 * @brief   Initializes the driver
 *
 * @param   app_id - application identification number
 *
 * @return  none
 */

void rtopo_init(u8 app_id)
{	
	/*Initialize the database*/	
	memset(&rtopo_data, 0x00, sizeof(rtopo_data_t));			
	memset(&rtopo_state, 0x00, sizeof(gv701x_state_t));		

	rtopo_app_id = app_id;	
	SLIST_Init(&rtopo_queues.appRxQueue);
	
	FM_Printf(FM_USER, "\nInit RtopoApp (app id %bu)", app_id);		

	rtopo_state.state = RTOPO_INIT;
	rtopo_state.event = RTOPO_IDLE_EVENT;	
	rtopo_state.statedata = NULL;		
	rtopo_state.statedatalen = 0;			

	/*Allocated Re-transmit timer*/
	rtopo_data.tx_timer = STM_AllocTimer(SW_LAYER_TYPE_APP, 
						RTOPO_TX_TIMER_EVNT,&rtopo_app_id);	
	rtopo_data.tx_cnt = 0;	
	rtopo_data.tx_time = RTOPO_BASE_TIMEOUT;

	/*Allocated Profile timer*/
	rtopo_data.profile_timer = STM_AllocTimer(SW_LAYER_TYPE_APP, 
						RTOPO_PROFILE_TIMER_EVNT,&rtopo_app_id);	

	/*Allocated Profile 1 timer*/
	rtopo_data.profile_1_timer = STM_AllocTimer(SW_LAYER_TYPE_APP, 
						RTOPO_PROFILE_1_TIMER_EVNT,&rtopo_app_id);	

	/*Allocated Profile 2 timer*/
	rtopo_data.freeze_timer = STM_AllocTimer(SW_LAYER_TYPE_APP, 
						RTOPO_FREEZE_TIMER_EVNT, &rtopo_app_id);	

	rtopo_data.scan.power_line.enabled = FALSE;
	rtopo_data.scan.wireless.enabled = FALSE;

#ifdef LRWPAN_DRIVER_APP	
	rtopo_data.scan.wireless.scantype = MLME_SCAN_TYPE_ACTIVE;
#endif	
	rtopo_data.scan.wireless.scanMode = RTOPO_SCAN_PEER_SELECTION;


#if 0	
	/*Allocate Network Scan timer*/
	rtopo_data.scan.timer = STM_AllocTimer(SW_LAYER_TYPE_APP, 
												RTOPO_SCAN_TIMER_EVNT, 
												&rtopo_app_id);	
#endif

	rtopo_data.pref.enabled = FALSE;
	/*Allocate Network Scan timer*/
	rtopo_data.pref.timer = STM_AllocTimer(SW_LAYER_TYPE_APP, 
												RTOPO_PREF_TIMER_EVNT, 
												&rtopo_app_id);	
	
	rtopo_data.link_pref.link = POWER_LINE;		
}

/******************************************************************************
 * @fn      rtopo_start
 *
 * @brief   Starts the network profile
 *
 * @param   none
 *
 * @return  none
 */

void rtopo_start(u8 link, u8 mode)
{
	if(link & PLC_NIC)
	{
		STM_StartTimer(rtopo_data.pref.timer, RTOPO_PREF_TIME);
		rtopo_data.pref.enabled = TRUE;
	}

	rtopo_state.state = RTOPO_START;		
	rtopo_start_scan(link, mode);		
}

/******************************************************************************
 * @fn      rtopo_start_scan
 *
 * @brief   Starts the network profile
 *
 * @param   none
 *
 * @return  none
 */
	
void rtopo_start_scan(u8 link, u8 scanMode)
{	
	if(link & PLC_NIC)
	{	
#ifdef HPGP_DRIVER_APP
		if(hpgp_drv_data.scan.active == FALSE)
		{
			hpgp_drv_scan_evnt_msg_t hpgp_scan;			
			hpgp_scan.event = HPGPDRV_SCAN_EVNT;
			hpgp_drv_data.scan.time = RTOPO_SCAN_TIME;
#if 0			
			FM_Printf(FM_APP, "\nPS");
#endif
			GV701x_SendAppEvent(rtopo_app_id, hpgp_drv_data.app_id, APP_MSG_TYPE_APPEVENT,
				APP_MAC_ID, EVENT_CLASS_CTRL, MGMT_FRM_ID, &hpgp_scan, 
				sizeof(hpgp_drv_scan_evnt_msg_t), 0);			
			//rtopo_data.scan.enabled = TRUE;
			rtopo_data.scan.power_line.enabled = TRUE;
		}
#endif
	}
	
	if(link & RF_NIC)
	{	
#if 0
		FM_Printf(FM_APP, "\nSC e %bu t %bu m %bu", rtopo_data.scan.wireless.enabled,
				rtopo_data.scan.wireless.scantype, scanMode);
#endif
		rtopo_data.scan.wireless.scanMode = scanMode;
		
#ifdef HQ_RF_TEST		
		rtopo_data.scan.wireless.scanMode = RTOPO_SCAN_CHANNEL_SELECTION;
#endif
		if(rtopo_data.scan.wireless.enabled == FALSE)
		{
			if(rtopo_data.scan.wireless.scantype == MLME_SCAN_TYPE_ACTIVE)
			{			
#ifndef OLD_SCAN			
				rtopo_rf_curscan_ch = MIN_CHANNEL;
				rtopo_start_wirscan_engine(scanMode);
				return;
#endif				
			}
			else

			{
#ifdef LRWPAN_DRIVER_APP
				if(lrwpan_db.scan.active == FALSE)
				{
					lrwpan_scan_evnt_msg_t lrwpan_scan;			
					lrwpan_scan.event = LRWPAN_SCAN_EVNT;			

					lrwpan_db.scan.type = rtopo_data.scan.wireless.scantype;
					if(rtopo_data.scan.wireless.scantype == MLME_SCAN_TYPE_ACTIVE)
						lrwpan_db.scan.time = 5;									
					else if(rtopo_data.scan.wireless.scantype == MLME_SCAN_TYPE_ED)
						lrwpan_db.scan.time = 3;									
#if 0
					FM_Printf(FM_APP, "\nWS %bu mode %bu", rtopo_data.scan.wireless.scantype, scanMode);
#endif
					GV701x_SendAppEvent(rtopo_app_id, lrwpan_db.app_id, APP_MSG_TYPE_APPEVENT,
						APP_MAC_ID, EVENT_CLASS_CTRL, MGMT_FRM_ID, &lrwpan_scan, 
						sizeof(lrwpan_scan_evnt_msg_t), 0);						
					rtopo_data.scan.wireless.enabled = TRUE;
					rtopo_data.scan.wireless.scanMode = scanMode;			
				}
#endif
			}
		}
	}
}

#ifndef OLD_SCAN
/******************************************************************************
 * @fn      rtopo_start_wirscan_engine
 *
 * @brief   Starts the wireless scan engine
 *
 * @param   mode - the scan mode (in routetopo.h)
 *
 * @return  none
 */
	
void rtopo_start_wirscan_engine(u8 mode)
{		
#ifdef LRWPAN_DRIVER_APP
#if 0	
	FM_Printf(FM_APP, "\nSEng(m %lx i %bx)", lrwpan_db.scan.ch_mask, rtopo_rf_curscan_ch);
#endif
	for(; rtopo_rf_curscan_ch <= MAX_CHANNEL; rtopo_rf_curscan_ch++)
	{
		if(lrwpan_db.scan.ch_mask & BIT(rtopo_rf_curscan_ch))
		{
			lrwpan_cfg_evnt_msg_t lrwpan_cfg;
			route_info.wireless_ch = rtopo_rf_curscan_ch;
#if 1
			//FM_Printf(FM_APP, "\nch: %bx", rtopo_rf_curscan_ch);
#endif
			lrwpan_cfg.event = LRWPAN_CFG_EVNT;
			lrwpan_cfg.attribute = phyCurrentChannel;			
			lrwpan_db.channel = rtopo_rf_curscan_ch;
			memcpy((u8*)&lrwpan_cfg.value, &lrwpan_db.channel, sizeof(uint8_t));
			GV701x_SendAppEvent(rtopo_app_id, lrwpan_db.app_id, APP_MSG_TYPE_APPEVENT,
				APP_MAC_ID, EVENT_CLASS_CTRL, MGMT_FRM_ID, &lrwpan_cfg, 
				sizeof(lrwpan_cfg_evnt_msg_t), 0); 				
			break;
		}
	}

	if(rtopo_rf_curscan_ch == (MAX_CHANNEL + 1))
	{	
		rtopo_start_evnt_msg_t rtopo_start;
		rtopo_data.scan.wireless.enabled = FALSE;
		rtopo_rf_curscan_ch = MIN_CHANNEL;
#if 0
		FM_Printf(FM_APP, "\nSComp chmask %lx", lrwpan_db.scan.ch_mask);
#endif
				
		rtopo_data.scan.wireless.scantype = MLME_SCAN_TYPE_ED;
		rtopo_start.event = RTOPO_START_EVNT;
		rtopo_start.link = RF_NIC; 
		rtopo_start.mode = mode; 
		GV701x_SendAppEvent(rtopo_app_id, rtopo_app_id, APP_MSG_TYPE_APPEVENT,
				APP_MAC_ID, EVENT_CLASS_CTRL, MGMT_FRM_ID,			
				&rtopo_start, sizeof(rtopo_start_evnt_msg_t), 0);			
	}
#endif	
}
#endif

/******************************************************************************
 * @fn      rtopo_rx
 *
 * @brief   Handler to receiver an Over the Air packet
 *
 * @param   buf - Data packet holder 
 *          len - length of data received
 *
 * @return  none
 */

void rtopo_rx(u8* buf, u8 len)
{
	sEth2Hdr* petherhdr = (sEth2Hdr*)(buf);
#ifdef ROUTE_APP
	route_hdr_t* rhdr = (route_hdr_t* )(&buf[sizeof(sEth2Hdr)]);
#endif
	u8 cmdid = RHDR_GET_CMDID(rhdr);
	len = len;

	/*Filter any other ether type packets*/
	if(petherhdr->ethtype != APP_ETHER_PROTO)
		return;

	switch(cmdid) 
	{		
		/*Handle Get Parameter from Host packet*/
		case GET_PARAM_RSP:	
			if(STATUS_SUCCESS == rtopo_recv_getparam((rtopo_getparam_t*)(rhdr + 1)))
			{
				rtopo_data.tx_cnt = 0;
				STM_StopTimer(rtopo_data.tx_timer);
			}
		break;

		case GET_PARAM_IND: 
			STM_StopTimer(rtopo_data.tx_timer);
		break;
		
		default:
		break;
	}	
}

/******************************************************************************
 * @fn      rtopo_tx
 *
 * @brief   Send Data packet Over the Air
 *
 * @param   buf - Data packet holder 
 *          payloadLen - number of bytes to be sent
 *			frametype - type of frame to be sent (defines found in route.h)
 *
 * @return  none
 */

u8 rtopo_tx(u8* buf, u8 payloadLen, u8 frametype)
{
	u8 ret = STATUS_FAILURE;

#ifdef ROUTE_APP		
	ret = route_send_to_ll((u8*)buf, payloadLen, frametype, FALSE);

	if(ret == FALSE)
		ret = STATUS_FAILURE;
	else
		ret = STATUS_SUCCESS;		
#endif
	return ret;
}

/******************************************************************************
 * @fn      rtopo_RxAppMsg
 *
 * @brief   Receives a message from another app/fw
 *
 * @params  msg_buf - message buffer
 *
 * @return  none
 */

void rtopo_RxAppMsg(sEvent* event)
{
	gv701x_app_msg_hdr_t* msg_hdr = (gv701x_app_msg_hdr_t*)event->buffDesc.dataptr;
	hostHdr_t* hybrii_hdr;
	hostEventHdr_t* evnt_hdr;

	hybrii_hdr = (hostHdr_t*)(msg_hdr + 1);

	if(msg_hdr->dst_app_id == rtopo_app_id)
	{	
		memcpy(&rtopo_state.msg_hdr, msg_hdr, sizeof(gv701x_app_msg_hdr_t));
		rtopo_state.eventproto = hybrii_hdr->protocol;
		if((msg_hdr->src_app_id == APP_FW_MSG_APPID) && 
			(hybrii_hdr->type == EVENT_FRM_ID))
		{
			evnt_hdr = (hostEventHdr_t*)(hybrii_hdr + 1);
			rtopo_state.event = evnt_hdr->type;		
			rtopo_state.statedata = (u8*)(evnt_hdr + 1);
			rtopo_state.statedatalen = (u16)(hybrii_hdr->length - sizeof(hostEventHdr_t)); 		
		}
		else
		{
			rtopo_state.event = (u8)(*((u8*)(hybrii_hdr + 1)));
			rtopo_state.statedata = (u8*)(hybrii_hdr + 1);
			rtopo_state.statedatalen = (u16)hybrii_hdr->length;
		}		
		rtopo_state.eventtype = hybrii_hdr->type;
		rtopo_state.eventclass = event->eventHdr.eventClass;	
		if((msg_hdr->src_app_id == APP_FW_MSG_APPID) && 
			(hybrii_hdr->type == EVENT_FRM_ID) &&
			(rtopo_state.event == HOST_EVENT_APP_TIMER))
		{			
			rtopo_timerhandler((u8*)(evnt_hdr + 1)); 
			return;
		}		
		if((msg_hdr->src_app_id == APP_FW_MSG_APPID) && 
			(hybrii_hdr->type == EVENT_FRM_ID) &&
			(rtopo_state.event == HOST_EVENT_APP_CMD))
		{			
			rtopo_cmdprocess((char*)(evnt_hdr + 1));
			return;
		}		
	}
	else if(msg_hdr->dst_app_id == APP_BRDCST_MSG_APPID)
	{
		u8 *event = (u8*)(hybrii_hdr + 1);
#ifdef NWKSTARTUP_APP			
		if(msg_hdr->src_app_id == nwkstartup_data.app_id)
		{
			if(msg_hdr->type == APP_MSG_TYPE_APPIND)
			{			
				switch((u8)*event)
				{
					case NWK_LINKUP_IND:
					{
						nwk_up_ind_msg_t* nwk_up = (nwk_up_ind_msg_t*)event;	
						route_start_evnt_msg_t route_start; 
#ifdef HPGP_DRIVER_APP					
						if(nwk_up->link & PLC_NIC)		
						{						
							if((hpgp_nwk_data.params.nwk.role == DEV_MODE_STA)
								&& (rtopo_data.scan.power_line.enabled == TRUE))
								rtopo_data.scan.power_line.enabled = FALSE;
							
							if(hpgp_nwk_data.params.nwk.role == DEV_MODE_CCO)
								rtopo_data.link_pref.link = WIRELESS;	
							else
							if(hpgp_nwk_data.params.nwk.role == DEV_MODE_STA)
							{
							
								rtopo_data.link_pref.link = POWER_LINE;
								rtopo_state.state = RTOPO_START;
								
								route_start.event = ROUTE_START_EVNT;
								route_start.link = POWER_LINE;							
								route_start.assignparent = POWER_LINE;
								GV701x_SendAppEvent(rtopo_app_id, route_app_id, APP_MSG_TYPE_APPEVENT,
													APP_MAC_ID, EVENT_CLASS_CTRL, MGMT_FRM_ID, &route_start, 
													sizeof(route_start_evnt_msg_t), 0);							
							}
#if 1							
							FM_Printf(FM_APP, "\nP up(p %bu rs %bu rl %bu)", rtopo_data.link_pref.link, rtopo_state.state,
									  hpgp_nwk_data.params.nwk.role);
#endif
						}	
#endif					
						if(nwk_up->link & RF_NIC)
						{
							if((rtopo_data.pref.enabled == FALSE) &&
								(hpgp_drv_data.state.state != HPGPDRV_UP))
							{
								rtopo_data.link_pref.link = WIRELESS; 		
							}
							rtopo_state.state = RTOPO_START;
							//rtopo_data.scan.wireless.enabled = FALSE; 

							
							if(rtopo_data.scan.wireless.scanMode == RTOPO_SCAN_PEER_SELECTION)
							{
								route_start.event = ROUTE_START_EVNT;
								route_start.link = WIRELESS;		
								route_start.assignparent = WIRELESS;
								GV701x_SendAppEvent(rtopo_app_id, route_app_id, APP_MSG_TYPE_APPEVENT, 
													APP_MAC_ID, EVENT_CLASS_CTRL, MGMT_FRM_ID, &route_start,
													sizeof(route_start_evnt_msg_t), 0);							
							}
#if 1														
							FM_Printf(FM_APP, "\nW up(p %bu pe %bu rs %bu sm %bu)", rtopo_data.link_pref.link,
												rtopo_data.pref.enabled, rtopo_state.state,
												rtopo_data.scan.wireless.scanMode);
#endif
						}						
					}
					break;
					
					case NWK_LINKDWN_IND:		
					{
						nwk_dwn_ind_msg_t* nwk_dwn = (nwk_dwn_ind_msg_t*)event;
						route_stop_evnt_msg_t route_stop;
						
						if((nwk_dwn->link & PLC_NIC) && (nwk_dwn->link & RF_NIC))
						{
							rtopo_state.state = RTOPO_INIT; 
							rtopo_data.link_pref.link = POWER_LINE;		

#ifdef ROUTE_APP	
							route_info.route_sel_active = TRUE;							
#endif
							STM_StopTimer(rtopo_data.freeze_timer);
							STM_StopTimer(rtopo_data.tx_timer);
							
#ifdef LRWPAN_DRIVER_APP						
							lrwpan_db.scan.time = 5;						
							lrwpan_db.scan.type = MLME_SCAN_TYPE_ACTIVE;
#endif						
#if 1
							rtopo_start_scan(nwk_dwn->link, RTOPO_SCAN_PEER_SELECTION);
#endif
							rtopo_data.scan.power_line.enabled = FALSE;						
							hpgp_nwk_data.params.nwk.role = DEV_MODE_STA;
							memcpy((u8*)hpgp_nwk_data.params.nwk.key.nid, 
									(u8*)cco_nid, NID_LEN);
							
							route_info.disc_params.powerline.cnt = 0;
							route_info.disc_params.wireless.cnt = 0;
							route_info.disc_params.link &= ~WIRELESS;
							route_info.disc_params.link &= ~POWER_LINE;		

							route_stop.event = ROUTE_STOP_EVNT;
							route_stop.link = POWER_LINE | WIRELESS;							
							GV701x_SendAppEvent(rtopo_app_id, route_app_id, APP_MSG_TYPE_APPEVENT, 
												APP_MAC_ID, EVENT_CLASS_CTRL, MGMT_FRM_ID, &route_stop, 
												sizeof(route_stop_evnt_msg_t), 0);							

#if 1							
							FM_Printf(FM_APP, "\nNWK dwn(rf plc) (pref %bu pse %bu wse %bu)", rtopo_data.link_pref.link,
										rtopo_data.scan.power_line.enabled, rtopo_data.scan.wireless.enabled);							
#endif
						}					
						else if(nwk_dwn->link & RF_NIC)
						{
							rtopo_data.link_pref.link = POWER_LINE; 								
							route_info.disc_params.wireless.cnt = 0;
							route_info.disc_params.link &= ~WIRELESS;
#ifdef ROUTE_APP								
							if(route_info.parent != NULL)
							{
								if(route_info.parent->link == WIRELESS)
								{
									rtopo_state.state = RTOPO_START;
									route_info.route_sel_active = TRUE;
									STM_StopTimer(rtopo_data.freeze_timer);
									STM_StopTimer(rtopo_data.tx_timer);
								}
							}
							else
							{
								rtopo_state.state = RTOPO_START;
								route_info.route_sel_active = TRUE;
								STM_StopTimer(rtopo_data.freeze_timer);
								STM_StopTimer(rtopo_data.tx_timer);
							}

#endif
							rtopo_start_scan(RF_NIC, RTOPO_SCAN_PEER_SELECTION);
#if 1															
							FM_Printf(FM_APP, "\nNwk dwn(rf) (p %bu dl %bu rs %bu)", rtopo_data.link_pref.link, 
									route_info.disc_params.link, rtopo_state.state);						
#endif
							route_stop.event = ROUTE_STOP_EVNT;
							route_stop.link =  WIRELESS;							
							GV701x_SendAppEvent(rtopo_app_id, route_app_id, APP_MSG_TYPE_APPEVENT, 
												APP_MAC_ID, EVENT_CLASS_CTRL, MGMT_FRM_ID, &route_stop, 
												sizeof(route_stop_evnt_msg_t), 0);							

#if 1
							if((nwkstartup_data.link.power_line.state == LINK_UP)
#if 1								
								&& ((route_info.parent != NULL) ? 
								   (route_info.parent->link != POWER_LINE) : (TRUE))
#endif								   
							)
							{
								route_start_evnt_msg_t route_start;
								route_start.event = ROUTE_START_EVNT;
								route_start.link = POWER_LINE;
								route_start.assignparent = POWER_LINE;
								GV701x_SendAppEvent(rtopo_app_id, route_app_id, APP_MSG_TYPE_APPEVENT, 
													APP_MAC_ID, EVENT_CLASS_CTRL, MGMT_FRM_ID, &route_start, 
													sizeof(route_start_evnt_msg_t), 0);							
							}
#endif							
						}
						else if(nwk_dwn->link & PLC_NIC)
						{						
							route_info.disc_params.powerline.cnt = 0;
							route_info.disc_params.link &= ~POWER_LINE;	
							if(rtopo_data.pref.enabled == FALSE)
								rtopo_data.link_pref.link = WIRELESS;
							rtopo_data.scan.power_line.enabled = FALSE;
#ifdef ROUTE_APP															
							if(route_info.parent != NULL)
							{
								if(route_info.parent->link == POWER_LINE)
								{
									rtopo_state.state = RTOPO_START;
									route_info.route_sel_active = TRUE;
									STM_StopTimer(rtopo_data.freeze_timer);
								}
							}
							else
							{
								rtopo_state.state = RTOPO_START;
								route_info.route_sel_active = TRUE;
								STM_StopTimer(rtopo_data.freeze_timer);
							}
#endif	
							if(hpgp_nwk_data.params.nwk.role == DEV_MODE_STA)
							{
								rtopo_state.state = RTOPO_START;
								hpgp_nwk_data.params.nwk.role = DEV_MODE_STA;
								memcpy((u8*)hpgp_nwk_data.params.nwk.key.nid, 
										(u8*)cco_nid, NID_LEN);						
							}

							route_stop.event = ROUTE_STOP_EVNT;
							route_stop.link = POWER_LINE;							
							GV701x_SendAppEvent(rtopo_app_id, route_app_id, APP_MSG_TYPE_APPEVENT,
												APP_MAC_ID, EVENT_CLASS_CTRL, MGMT_FRM_ID, &route_stop, 
												sizeof(route_stop_evnt_msg_t), 0);							

#if 1
							FM_Printf(FM_APP, "\nNWK dwn(plc)(p %bu pe %bu se %bu dl %bu rs %bu hm %bu)", 
									rtopo_data.link_pref.link, rtopo_data.pref.enabled,
									rtopo_data.scan.power_line.enabled, route_info.disc_params.link,
									rtopo_state.state, hpgp_nwk_data.params.nwk.role);										
#endif

							if(nwkstartup_data.link.wireless.state == LINK_UP)
							{
								if(hpgp_nwk_data.params.nwk.role == DEV_MODE_STA)
								{
#if 1								
									if((route_info.parent != NULL) ? 
									   (route_info.parent->link != WIRELESS) : (TRUE))
#endif									   
									{
#if 1							
										route_start_evnt_msg_t route_start;
										route_start.event = ROUTE_START_EVNT;
										route_start.link = WIRELESS;
										route_start.assignparent = WIRELESS;
										GV701x_SendAppEvent(rtopo_app_id, route_app_id, APP_MSG_TYPE_APPEVENT, 
															APP_MAC_ID, EVENT_CLASS_CTRL, MGMT_FRM_ID, &route_start, 
															sizeof(route_start_evnt_msg_t), 0);										
#endif						
									}
								}
							}
								
						}						
					}
					break;
					
					default:
					break;
				}	
			}
		}		
#endif	
		return;
	}
	rtopo_sm(&rtopo_state);
}

/******************************************************************************
 * @fn      rtopo_send_getparam
 *
 * @brief   Send Get Parameter request packet
 *
 * @param   id - parameter id (define found in gv701x_nwkstartup.h) 
 *          val - pointer to the parameter
 *			cnt - pointer to the re-transmit counter
 *               (re-transmissions occur at an exponential index)
 *
 * @return  none
 */

void rtopo_send_getparam(u8 id, u8 *val, u8* cnt)
{
	u8 buf[RTOPO_MAX_PKT_BUFFSIZE];
	route_hdr_t* rhdr = &buf[(sizeof(sEth2Hdr))];
	rtopo_getparam_t* gparm = (rtopo_getparam_t*)(rhdr + 1);
	
	val = val;
	memset(buf, 0x00, RTOPO_MAX_PKT_BUFFSIZE);	

	/*Fill the packet fields*/
#ifdef ROUTE_APP
	memcpy_cpu_to_le (&gparm->ieee_address, &route_info.ieee_addr, (IEEE_MAC_ADDRESS_LEN - 2));
#endif
	gparm->v[0].id = id; 
	gparm->v[0].value = 0;

	/*Start the re-transmit timer*/
	if(STATUS_SUCCESS == STM_StartTimer(rtopo_data.tx_timer, rtopo_data.tx_time))							
	{				
#if 1
		FM_Printf(FM_APP, "\nGPTx (s 0x%02x, d 0x%02x, l %bu cnt %bu", 
				le16_to_cpu(rhdr->target), le16_to_cpu(rhdr->parent),				 
				(route_info.parent != NULL)? route_info.parent->link : 0, *cnt);
#endif	
		/*Send the packet*/
		if(STATUS_SUCCESS == rtopo_tx(buf, ((sizeof(sEth2Hdr)) + sizeof(route_hdr_t) + 
									  sizeof(rtopo_getparam_t)), GET_PARAM_REQ))
		{		
			if((*cnt) < RTOPO_TIMEOUT_MAX_EXPONENT)
			{
				rtopo_data.tx_time = ((rtopo_data.tx_time)*(RTOPO_TIMEOUT_EXPONENT));													
			} 			
			*cnt = *cnt + 1;			
		}
	}
}

/******************************************************************************
 * @fn      rtopo_recv_getparam
 *
 * @brief   Parse the parameter received from the Host
 *
 * @param   param - parameter received (define found in gv701x_nwkstartup.h) 
 *
 * @return  none
 */

u8 rtopo_recv_getparam(rtopo_getparam_t* param) 
{
	u32 temp_nid;
	u16 nid1;

#if 1
	FM_Printf(FM_APP, "\nGRx (s 0x%02x, d 0x%02x, l %bu, rs %bu", 
			((route_info.parent != NULL)? route_info.parent->addr:0), (route_info.zid), 			
			(route_info.parent != NULL)? route_info.parent->link:0, rtopo_state.state);		
#endif
	
	if(rtopo_state.state == RTOPO_COMPLETE)	
		return STATUS_FAILURE;

	/*Unique NID parameter across entire network*/	
	if(param->v[0].id == RTOPO_PLC_NID) 
	{			
		temp_nid = NHTOHL(param->v[0].value);	
		nid1 = temp_nid;

#ifdef BRIDGE
#ifdef HPGP_DRIVER_APP
		memcpy(&hpgp_nwk_data.params.nwk.key.nid[0], cco_nid, NID_LEN -2);
		memcpy(&hpgp_nwk_data.params.nwk.key.nid[5], &nid1, sizeof(u16));	

		/*Write the NID into the flash*/
		if(STATUS_FAILURE == GV701x_FlashWrite(hpgp_drv_data.app_id, 
								(u8*)hpgp_nwk_data.params.nwk.key.nid, NID_LEN))
		{
		}
#endif						

		//rtopo_state.event = RTOPO_STOP_EVNT;
		rtopo_start_devrole(PLC_NIC);
#endif		
	} 
	else 
	{
#if 0	
		FM_Printf(FM_APP, "\nParam ID mismatch");
#endif
		return STATUS_FAILURE;
	}	

	return STATUS_SUCCESS;
}

/******************************************************************************
 * @fn      rtopo_timerhandler
 *
 * @brief   Timer handler for NwkStartupProfile timer events
 *
 * @param   event - event from firmware
 *
 * @return  none
 *
 */

void rtopo_timerhandler(u8* buf)
{						
	hostTimerEvnt_t* timerevt = (hostTimerEvnt_t*)buf;	
	
	if(buf == NULL)
		return;

	/*Demultiplexing the specific timer event*/ 					
	switch((u8)timerevt->type)
	{							
		/*Re-transmit timer event*/
		case RTOPO_TX_TIMER_EVNT: 
			rtopo_handle_timeout();
		break;

		/*Device Profile timer event*/
		case RTOPO_PROFILE_TIMER_EVNT:
		{
#ifdef HPGP_DRIVER_APP				
			nwk_start_evnt_msg_t nwk_start;
			/*Start as CCO*/
			hpgp_nwk_data.params.nwk.role = DEV_MODE_CCO;			
			nwk_start.event = NWK_START_EVENT;
			nwk_start.link = PLC_NIC;
			GV701x_SendAppEvent(rtopo_app_id, nwkstartup_data.app_id, APP_MSG_TYPE_APPEVENT,
								APP_MAC_ID, EVENT_CLASS_CTRL, EVENT_FRM_ID,
								&nwk_start, sizeof(nwk_start_evnt_msg_t), 0);
#endif	
		}
		break;				

		/*Device Profile timer event*/
		case RTOPO_PROFILE_1_TIMER_EVNT:
		{				
			nwk_start_evnt_msg_t nwk_start;
#if (defined NWKSTARTUP_APP) && (defined HPGP_DRIVER_APP) && (defined ROUTE_APP)			
			nwkstartup_data.link.power_line.state = LINK_DOWN;
			nwkstartup_data.link.power_line.addr = route_info.zid;

			hpgp_nwk_data.params.nwk.role = DEV_MODE_CCO;			
			nwk_start.event = NWK_START_EVENT;
			nwk_start.link = PLC_NIC;
			GV701x_SendAppEvent(rtopo_app_id, nwkstartup_data.app_id, APP_MSG_TYPE_APPEVENT,
								APP_MAC_ID, EVENT_CLASS_CTRL, EVENT_FRM_ID,
								&nwk_start, sizeof(nwk_start_evnt_msg_t), 0);
#endif				
		}
		break;

		/*Device Profile timer event*/
		case RTOPO_FREEZE_TIMER_EVNT:
		{	
#ifdef ROUTE_APP			
			if(route_info.parent != NULL) 
			{
				route_info.route_sel_active = FALSE;
			}
#endif
		}
		break;

		case RTOPO_PREF_TIMER_EVNT:
		{		
			rtopo_data.pref.enabled = FALSE;		
			if((hpgp_drv_data.state.state != HPGPDRV_START) &&
				(hpgp_drv_data.state.state != HPGPDRV_UP))
			{
#ifdef LRWPAN_DRIVER_APP				
				if(lrwpan_state.state == LRWPAN_UP)
					rtopo_data.link_pref.link = WIRELESS;
#endif				
			}
#if 0			
			FM_Printf(FM_APP, "\nTO Pref hs %bu ls %bu pl %bu", hpgp_drv_data.state.state,
					lrwpan_state.state, rtopo_data.link_pref.link);						
#endif
		}
		break;

		case RTOPO_SCAN_TIMER_EVNT:
			rtopo_data.scan.power_line.enabled = FALSE;
		break;
		
		default:
		break;
	}			
}

/******************************************************************************
 * @fn      rtopo_handle_timeout
 *
 * @brief   Handles transmit timeout
 *
 * @param   none
 *
 * @return  none
 *
 */

void rtopo_handle_timeout(void)
{
	if(rtopo_data.tx_cnt >= RTOPO_MAX_TX_RETRY_COUNT)
	{	
		route_stop_evnt_msg_t route_stop;
#ifdef NWKSTARTUP_APP		
		nwk_stop_evnt_msg_t nwk_stop;		
#endif		
#if 0
		FM_Printf(FM_APP, "\nRTTO");		
#endif
		rtopo_data.tx_cnt = 0;
		rtopo_data.tx_time = RTOPO_BASE_TIMEOUT;		
		rtopo_state.state = RTOPO_INIT;
		rtopo_state.event = RTOPO_IDLE_EVENT;				
#ifdef ROUTE_APP						
		route_stop.event = ROUTE_STOP_EVNT;
		route_stop.link = route_info.parent->link;
		GV701x_SendAppEvent(rtopo_app_id, route_app_id, APP_MSG_TYPE_APPEVENT, APP_MAC_ID, EVENT_CLASS_CTRL,
							MGMT_FRM_ID, &route_stop, sizeof(route_stop_evnt_msg_t), 0);							
#endif

#ifdef NWKSTARTUP_APP
		nwk_stop.event = NWK_STOP_EVENT;
#ifdef ROUTE_APP
		nwk_stop.link = route_info.parent->link;
#endif		
		GV701x_SendAppEvent(rtopo_app_id, nwkstartup_data.app_id, APP_MSG_TYPE_APPEVENT, APP_MAC_ID, EVENT_CLASS_CTRL,
							MGMT_FRM_ID, &nwk_stop, sizeof(nwk_stop_evnt_msg_t), 0);							
#endif		
	}
	else
	{		
		rtopo_send_getparam(RTOPO_PLC_NID, NULL, &rtopo_data.tx_cnt);
	}
}

/******************************************************************************
 * @fn		rtopo_handle_scanind
 *
 * @brief	Handles the scan indication coming from HPGP driver
 *
 * @param	none
 *
 * @return	none
 *
 */
void rtopo_handle_scanind(u8 link, u8 status) 
{
	if(link & PLC_NIC)
	{
#ifdef HPGP_DRIVER_APP
		u8 i;
		u8 valid = FALSE;
		rtopo_data.scan.power_line.enabled = FALSE;

		for(i = 0; (i < hpgp_nwk_data.netlist.entries); i++)
		{		
			if((hpgp_nwk_data.netlist.list[i].valid == TRUE) &&
				(memcmp(hpgp_nwk_data.netlist.list[i].nid, cco_nid, NID_LEN-2) == 0) &&
				(memcmp(&hpgp_nwk_data.netlist.list[i].vendor_ota.ouid, 
						hpgp_nwk_data.params.nwk.app_info.oem[0].ouid, OUID_LEN) == 0))
			{
				u16* parent_addr;				
				{
					{				
						parent_addr = (u16*)(&hpgp_nwk_data.netlist.list[i].vendor_ota.buf[MAC_ADDR_LEN]);					
#if 1
						FM_Printf(FM_APP, "\ni %bu rssi %bu lqi %bu bcnt %lu ract %bu", i, hpgp_nwk_data.netlist.list[i].rssi,								
								hpgp_nwk_data.netlist.list[i].lqi, hpgp_nwk_data.netlist.list[i].bcnRxCnt, route_info.route_sel_active);								
						FM_HexDump(FM_APP,"\nNid:",(u8*)hpgp_nwk_data.netlist.list[i].nid, NID_LEN);
						FM_HexDump(FM_APP,"\nVField:",(u8*)&hpgp_nwk_data.netlist.list[i].vendor_ota, 
									sizeof(svendorota));	
						FM_Printf(FM_APP, "\nPaddr %x", *parent_addr);
#endif									
						if(hpgp_nwk_data.params.nwk.role == DEV_MODE_CCO)
						{
							valid = TRUE;
#ifdef ROUTE_APP			
							route_add_neighbor(0x0000, hpgp_nwk_data.netlist.list[i].vendor_ota.buf, 
												0, POWER_LINE, 0, hpgp_nwk_data.netlist.list[i].rssi,
												hpgp_nwk_data.netlist.list[i].bcnRxCnt, 
												hpgp_nwk_data.netlist.list[i].lqi, *parent_addr, 0, 0);
#endif				
						}
						else if(hpgp_nwk_data.params.nwk.role == DEV_MODE_STA)
						{
							if((hpgp_drv_data.state.state != HPGPDRV_UP) &&
								(hpgp_drv_data.state.state != HPGPDRV_START))
							{
								valid = TRUE;
#ifdef ROUTE_APP					
								route_add_neighbor (0x0000, hpgp_nwk_data.netlist.list[i].vendor_ota.buf, 
													0, POWER_LINE, 0, hpgp_nwk_data.netlist.list[i].rssi,
													hpgp_nwk_data.netlist.list[i].bcnRxCnt, 
													hpgp_nwk_data.netlist.list[i].lqi, *parent_addr, 0, 0);													
#endif					
							}
							else
								rtopo_data.scan.power_line.enabled = TRUE;
						}
						break;
					}
				}
			}								
		}

#ifdef ROUTE_APP
		if(valid == TRUE)
		{
			neighbor_info_t* best_nbr;
			best_nbr = route_select_best_neighbor(POWER_LINE);	
			
			if(NULL != best_nbr)
			{				
				if(best_nbr->ieee_addr != NULL)
				{
					hostEventScanList* nbr = NULL;	

					for(i = 0; (i < MAX_NETWORK_LIST); i++)
					{
						if((hpgp_nwk_data.netlist.list[i].valid == TRUE) &&
							(memcmp(&hpgp_nwk_data.netlist.list[i].vendor_ota.buf[0], 
							  best_nbr->ieee_addr, IEEE_MAC_ADDRESS_LEN-2) == 0))
						{
#if 0							
							FM_Printf(FM_APP, "\nLU=%bu rssi %bu lqi %bu bcnRxCnt %lu rsel %bu", i, hpgp_nwk_data.netlist.list[i].rssi,									
									hpgp_nwk_data.netlist.list[i].lqi, hpgp_nwk_data.netlist.list[i].bcnRxCnt,
									route_info.route_sel_active);									
							FM_HexDump(FM_APP,"\nNid:",(u8*)hpgp_nwk_data.netlist.list[i].nid, NID_LEN);
							FM_HexDump(FM_APP,"\nVField:",(u8*)&hpgp_nwk_data.netlist.list[i].vendor_ota.buf, 
												sizeof(svendorota));
#endif								
							if(route_info.route_sel_active == TRUE)
							{
								nwk_start_evnt_msg_t nwk_start;
								nwk_start.event = NWK_START_EVENT;
								nwk_start.link = PLC_NIC;
								hpgp_nwk_data.params.nwk.role = DEV_MODE_STA;
								memcpy((u8*)hpgp_nwk_data.params.nwk.key.nid, 
										(u8*)hpgp_nwk_data.netlist.list[i].nid, NID_LEN);
								memcpy((u8*)hpgp_nwk_data.params.nwk.key.nmk, (u8*)nmk, ENC_KEY_LEN);

								GV701x_SendAppEvent(rtopo_app_id, nwkstartup_data.app_id, 
													APP_MSG_TYPE_APPEVENT, APP_MAC_ID, EVENT_CLASS_CTRL, 													
													EVENT_FRM_ID,&nwk_start, sizeof(nwk_start_evnt_msg_t), 0);													
								rtopo_data.scan.power_line.enabled = TRUE;
								break;
							}
						}				
					}	
				}
			}	
		}
#endif	
#endif	
	}

	if(link & RF_NIC)
	{	
#ifdef LRWPAN_DRIVER_APP	
		u8 i;
		u8 channel = 0;
		u8 j = MIN_CHANNEL;
#ifdef ROUTE_APP		
		u8 ed = ROUTE_ED_THRESHOLD;
#endif

		route_info.wireless_ch = 0;
#if 0
		FM_Printf(FM_APP, "\nLs %bu Uc %bu St %bu Sm %bu s %bx\n", 
			lrwpan_db.scan.result.ResultListSize, lrwpan_db.scan.result.UnscannedChannels,
			lrwpan_db.scan.result.ScanType, rtopo_data.scan.wireless.scanMode, status);			
#endif

		if(status == STATUS_FAILURE)
		{
#if 1		
			FM_Printf(FM_APP, "\nScnF");
#endif
			rtopo_data.scan.wireless.enabled = FALSE;
#if 0			
			FM_Printf(FM_APP, "\nCont scan0..");
#endif
			rtopo_start_scan(RF_NIC, rtopo_data.scan.wireless.scanMode);
			return;
		}
			
		for(i = 0; i < lrwpan_db.scan.result.ResultListSize; i++)
		{			
			if(lrwpan_db.scan.result.ScanType == MLME_SCAN_TYPE_ACTIVE)
			{		
#ifdef OLD_SCAN			
				if(lrwpan_db.scan.result.list[i].val.PANDescriptor.CoordAddrSpec.PANId != LRWPAN_PANID)
				{
#if 0				
					FM_Printf(FM_APP, "\nPan mismatch");
#endif
					memset(&lrwpan_db.scan.result.list[i], 0x00, sizeof(result_list_t));
					continue;
				}
																
				lrwpan_db.scan.result.list[i].active = FALSE;
#if 0										
				FM_Printf(FM_APP, "\nNo: %bu Ch %bx ChP %bx AM %bx A %x PAN %x LQI %bu T %lu", i,
					lrwpan_db.scan.result.list[i].val.PANDescriptor.LogicalChannel,
					lrwpan_db.scan.result.list[i].val.PANDescriptor.ChannelPage,			
					lrwpan_db.scan.result.list[i].val.PANDescriptor.CoordAddrSpec.AddrMode, 		
					lrwpan_db.scan.result.list[i].val.PANDescriptor.CoordAddrSpec.Addr.short_address,						
					lrwpan_db.scan.result.list[i].val.PANDescriptor.CoordAddrSpec.PANId,			
					lrwpan_db.scan.result.list[i].val.PANDescriptor.LinkQuality, 
					lrwpan_db.scan.result.list[i].val.PANDescriptor.TimeStamp);		
					//FM_HexDump(FM_APP,"\nBcnPayload:", lrwpan_db.scan.result.list[i].bcn_payload,
						//		MAX_BCN_PAYLOAD);
#endif			
				route_add_neighbor(0x0000, lrwpan_db.scan.result.list[i].bcn_payload, 0, WIRELESS, 0, 
						0, 0, lrwpan_db.scan.result.list[i].val.PANDescriptor.LinkQuality, 0, 
						lrwpan_db.scan.result.list[i].val.PANDescriptor.LogicalChannel, 0);				
#endif
			}								
			else if(lrwpan_db.scan.result.ScanType == MLME_SCAN_TYPE_ED)
			{									
				for(; j <= MAX_CHANNEL; j++)
				{				
					if((lrwpan_db.scan.ch_mask & BIT(j)))
					{
						j++;
						break;
					}						
				}

#if 0
				FM_Printf(FM_APP, "\nNo. %bu ED %bx ed %bx (j-1) %bx" , i,
					lrwpan_db.scan.result.list[0].val.ed_value[i], ed, (j-1));		
#endif								
				if(route_is_neightable_empty(WIRELESS) == TRUE)
				{
					if(lrwpan_db.scan.result.list[0].val.ed_value[i] <= ed)
					{
						ed = lrwpan_db.scan.result.list[0].val.ed_value[i];	
						channel = j - 1;							
					}
				}
				else
				{
					route_update_neighbor_ed((j - 1), 
						((lrwpan_db.scan.result.list[0].val.ed_value[i] == 0) ? 
						(1) : (lrwpan_db.scan.result.list[0].val.ed_value[i])));
				}
			}					
		}

#if 0	
		/*TBD: Fix LMAC RF scan issue*/
		if(lrwpan_db.scan.result.UnscannedChannels == 0)
#endif		
		{						
			u8 stop_scan = FALSE;
			
			if(lrwpan_db.scan.result.ResultListSize != 0)
			{
				if(rtopo_data.scan.wireless.scantype == MLME_SCAN_TYPE_ACTIVE)
				{				
#ifdef OLD_SCAN				
					//rtopo_data.scan.wireless.enabled = FALSE;
#endif					
				}
				else if(rtopo_data.scan.wireless.scantype == MLME_SCAN_TYPE_ED)
				{
					neighbor_info_t* best_nbr = NULL;					
					
					if(route_is_neightable_empty(WIRELESS) == FALSE)
					{			
						if(rtopo_data.scan.wireless.scanMode == RTOPO_SCAN_PEER_SELECTION)	
						{
							for(j = MIN_CHANNEL; j <= MAX_CHANNEL; j++)
							{				
								if((lrwpan_db.scan.ch_mask & BIT(j)))
								{
									route_info.wireless_ch = j;
#if 0								
									FM_Printf(FM_APP, "\nPch %bx", route_info.wireless_ch);
#endif
									if(route_select_best_neighbor(WIRELESS) != NULL)
										stop_scan = TRUE;
								}						
							}		
						}
						else if(rtopo_data.scan.wireless.scanMode == RTOPO_SCAN_CHANNEL_SELECTION)
						{
							stop_scan = TRUE;
#if 0							
							FM_Printf(FM_APP, "\nChannel Sel");
#endif
						}

						if((rtopo_data.scan.wireless.scanMode == RTOPO_SCAN_PEER_SELECTION) ||
							(rtopo_data.scan.wireless.scanMode == RTOPO_SCAN_CHANNEL_SELECTION))
						{
							if(stop_scan == TRUE)
							{
#if 0						
								FM_Printf(FM_APP, "\nYes");
#endif							
								route_info.wireless_ch = 0;
								best_nbr = route_select_best_neighbor(WIRELESS);	
								if(best_nbr != NULL)
								{								
									rtopo_data.scan.wireless.panid = LRWPAN_PANID;
									rtopo_data.scan.wireless.channel = best_nbr->ch;									
									rtopo_data.scan.wireless.enabled = FALSE;
									rtopo_start_devrole(RF_NIC);
								}
								else
									stop_scan = FALSE;
							}
						}
					}
					else 
					{	
						if(rtopo_data.scan.wireless.scanMode == RTOPO_SCAN_CHANNEL_SELECTION)
						{
							if((rtopo_data.pref.enabled == FALSE) && (channel != 0))
							{
								stop_scan = TRUE;
								rtopo_data.scan.wireless.enabled = FALSE;
								rtopo_data.scan.wireless.panid = LRWPAN_PANID;
								rtopo_data.scan.wireless.channel = channel;		
								rtopo_data.scan.wireless.scantype = MLME_SCAN_TYPE_ACTIVE;	
								rtopo_start_devrole(RF_NIC);
							}
							else			
							{
								//rtopo_data.scan.wireless.enabled = FALSE;
								//rtopo_data.scan.wireless.scantype = MLME_SCAN_TYPE_ED;									
								stop_scan = FALSE;
							}
						}
					}					
				}				
			}

			if(rtopo_data.scan.wireless.scantype == MLME_SCAN_TYPE_ACTIVE)
				rtopo_data.scan.wireless.scantype = MLME_SCAN_TYPE_ED;
			else if(rtopo_data.scan.wireless.scantype == MLME_SCAN_TYPE_ED)
				rtopo_data.scan.wireless.scantype = MLME_SCAN_TYPE_ACTIVE;
			
			if(stop_scan == FALSE)
			{
				rtopo_data.scan.wireless.enabled = FALSE;
#if 0			
				FM_Printf(FM_APP, "\nCont scan1..");
#endif
				rtopo_start_scan(RF_NIC, rtopo_data.scan.wireless.scanMode);
			}
			else 
			{
#if 1
				FM_Printf(FM_APP, "\nStopS");
#endif				
#if 0				
#ifndef OLD_SCAN			
				route_stop_evnt_msg_t route_stop;
				route_stop.event = ROUTE_STOP_EVNT;
				route_stop.link = WIRELESS;							
				GV701x_SendAppEvent(rtopo_app_id, route_app_id, APP_MSG_TYPE_APPEVENT, 
									APP_MAC_ID, EVENT_CLASS_CTRL, MGMT_FRM_ID, &route_stop, 
									sizeof(route_stop_evnt_msg_t), 0);	
#endif
#endif				
			}
		}
#endif		
	}
}

/******************************************************************************
 * @fn		rtopo_handle_scanind
 *
 * @brief	Handles the scan indication coming from HPGP driver
 *
 * @param	none
 *
 * @return	none
 *
 */
void rtopo_start_devrole(u8 link) 
{
#if (defined NWKSTARTUP_APP) && (defined ROUTE_APP)
	
#ifdef HPGP_DRIVER_APP				   
	/*Start CCO on the PLC link*/
	if((link & PLC_NIC) && (nwkstartup_data.link.power_line.state != LINK_DISABLE))						
	{
		if(hpgp_nwk_data.params.nwk.role != DEV_MODE_CCO)
		{
			u16 time;
			nwk_start_evnt_msg_t nwk_start;
					
			if(route_info.zid >= 10)
				time = route_info.zid/10;
			else
				time = route_info.zid;
			time = time*100;
#if 0			
			
			/*Set a Random time to start CCO*/
			STM_StartTimer(rtopo_data.profile_timer, time);

#else
#ifdef HPGP_DRIVER_APP					
			/*Start as CCO*/
			hpgp_nwk_data.params.nwk.role = DEV_MODE_CCO;			
			nwk_start.event = NWK_START_EVENT;
			nwk_start.link = PLC_NIC;
			GV701x_SendAppEvent(rtopo_app_id, nwkstartup_data.app_id, APP_MSG_TYPE_APPEVENT,
								APP_MAC_ID, EVENT_CLASS_CTRL, EVENT_FRM_ID,
								&nwk_start, sizeof(nwk_start_evnt_msg_t), 0);
#endif
#endif
			rtopo_state.state = RTOPO_COMPLETE;
		}
	}
#endif

#ifdef LRWPAN_DRIVER_APP
	if((link & RF_NIC) && (nwkstartup_data.link.wireless.state != LINK_DISABLE))
	{				
		nwk_start_evnt_msg_t nwk_start;
		nwk_start.event = NWK_START_EVENT;
		nwk_start.link = RF_NIC;
		lrwpan_db.short_addr = nwkstartup_data.link.wireless.addr;	
		lrwpan_db.panid = rtopo_data.scan.wireless.panid; 
		lrwpan_db.channel = rtopo_data.scan.wireless.channel;
		route_info.wireless_ch = lrwpan_db.channel;
		
		GV701x_SendAppEvent(rtopo_app_id, nwkstartup_data.app_id, APP_MSG_TYPE_APPEVENT,
							APP_MAC_ID, EVENT_CLASS_CTRL, EVENT_FRM_ID,
							&nwk_start, sizeof(nwk_start_evnt_msg_t), 0);				
#if 0		
		FM_Printf(FM_APP, "\nLStart A %x PAN %x CH %bx", lrwpan_db.short_addr, 
					rtopo_data.scan.wireless.panid, 
						rtopo_data.scan.wireless.channel); 										
#endif				
		rtopo_data.scan.wireless.panid = 0; 
		rtopo_data.scan.wireless.channel = 0;			
		rtopo_data.scan.wireless.enabled = FALSE;	
		rtopo_data.scan.wireless.start = FALSE;		
	}
#endif
#endif
}

/******************************************************************************
 * @fn      rtopo_poll
 *
 * @brief   The NetowrkStartup Device profiling State Machine, 
 *			it executes all internal/external 
 *			events triggered
 *
 * @param   state - state machine object of the driver
 *          (passed as a reference incase there are more than one object)
 *
 * @return  none
 *
 */

void rtopo_poll(void) 
{
	if(rtopo_state.state != RTOPO_INIT)
	{
#ifdef HPGP_DRIVER_APP	
		if(rtopo_data.scan.power_line.enabled == FALSE)
		{		
			if(hpgp_nwk_data.params.nwk.role == DEV_MODE_CCO)				
			{					
				rtopo_start_scan(PLC_NIC,RTOPO_SCAN_PEER_SELECTION);
			}
			else if(hpgp_nwk_data.params.nwk.role == DEV_MODE_STA)
			{
				if((hpgp_drv_data.state.state != HPGPDRV_UP) /*&&
					(hpgp_drv_data.state.state != HPGPDRV_START)*/)
				{										
					rtopo_start_scan(PLC_NIC, RTOPO_SCAN_PEER_SELECTION);					
				}
			}
		}
#endif

#if 0
#ifdef LRWPAN_DRIVER_APP
		if(rtopo_data.scan.wireless.enabled == FALSE)
		{		
			if((lrwpan_state.state != LRWPAN_START) &&
				(lrwpan_state.state != LRWPAN_SCAN) &&
				(lrwpan_state.state != LRWPAN_UP))
			{			
				rtopo_start_scan(RF_NIC);					
			}
		}		
#endif		
#endif

 	}
}

/******************************************************************************
 * @fn      rtopo_sm
 *
 * @brief   The NetowrkStartup Device profiling State Machine, 
 *			it executes all internal/external 
 *			events triggered
 *
 * @param   state - state machine object of the driver
 *          (passed as a reference incase there are more than one object)
 *
 * @return  none
 *
 */

void rtopo_sm(gv701x_state_t* state) 
{
	if(state == NULL)
		return;

#if 1	
	if(state->event != RTOPO_IDLE_EVENT)
		FM_Printf(FM_APP, "\nRtopo S %bu E %bu P %bu C %bu E %bu Da %bu Sa %bu T %bu", 
				state->state, state->event,
				state->eventproto, state->eventclass, state->eventtype, 
				state->msg_hdr.dst_app_id, state->msg_hdr.src_app_id, state->msg_hdr.type);
#endif

	switch(state->state)
	{		
		case RTOPO_INIT:
		case RTOPO_START:			
		case RTOPO_COMPLETE:
			if(state->eventproto == APP_MAC_ID)
			{
#ifdef HPGP_DRIVER_APP										
				if(state->msg_hdr.src_app_id == hpgp_drv_data.app_id)
				{
					if(state->msg_hdr.type == APP_MSG_TYPE_APPIND)
					{					
						switch(state->event)
						{							
							case HPGPDRV_SCAN_IND:
								rtopo_handle_scanind(PLC_NIC, STATUS_SUCCESS);
							break;			
							default:
							break;
						}
					}
				}
#endif				
#ifdef LRWPAN_DRIVER_APP
				if(state->msg_hdr.src_app_id == lrwpan_db.app_id)
				{
					if(state->msg_hdr.type == APP_MSG_TYPE_APPIND)
					{									
						switch(state->event)
						{							
							case LRWPAN_SCAN_IND:							
								if(state->statedata != NULL)
								{
									lrwpan_scan_ind_t* lrwpan_scan = (lrwpan_scan_ind_t*)state->statedata;
									rtopo_handle_scanind(RF_NIC, lrwpan_scan->status);
								}							
							break;	

							case LRWPAN_CFG_IND:							
								if(state->statedata != NULL)
								{
									lrwpan_cfg_ind_t* lrwpan_cfg = (lrwpan_cfg_ind_t*)state->statedata;
#if 0									
									FM_Printf(FM_APP, "\nLCIND %bu", lrwpan_cfg->status);
#endif
									if(lrwpan_cfg->status == STATUS_SUCCESS)
									{
										if(lrwpan_cfg->attribute == phyCurrentChannel)
										{										
											route_start_evnt_msg_t route_start; 
#if 0
											FM_Printf(FM_APP, "\nSCont(s %bx ch %bx)", lrwpan_cfg->status, lrwpan_db.channel);		
#endif																						
											route_start.event = ROUTE_START_EVNT;
											route_start.link = WIRELESS;
											route_start.assignparent = 0;
											GV701x_SendAppEvent(rtopo_app_id, route_app_id, APP_MSG_TYPE_APPEVENT,
																APP_MAC_ID, EVENT_CLASS_CTRL, MGMT_FRM_ID, &route_start, 
																sizeof(route_start_evnt_msg_t), 0);		
										}
									}
								}							
							break;	
							
							default:
							break;
						}
					}
				}
#endif
#ifdef ROUTE_APP
				if(state->msg_hdr.src_app_id == route_app_id) 
				{	
					if(state->msg_hdr.type == APP_MSG_TYPE_APPIND)
					{									
						switch(state->event)
						{			
							case ROUTE_DWN_IND:
							{		
								nwk_stop_evnt_msg_t nwk_stop;	
								route_dwn_ind_msg_t* route_dwn = (route_dwn_ind_msg_t*)state->statedata; 
#if 1								
								FM_Printf(FM_APP, "\nRdwn l %bu r %bu pl %bu wl %bu re %bu we %bu pe %bu", route_dwn->link, route_dwn->reason,
									nwkstartup_data.link.power_line.state, 
									nwkstartup_data.link.wireless.state, hpgp_nwk_data.params.nwk.role,
									rtopo_data.scan.wireless.enabled,
									rtopo_data.scan.power_line.enabled);	
#endif

#ifdef ROUTE_RECOVERY
								
	//							swResetGV701x(1);
								STM_StartTimer(route_info.rec_timer, 
											   ROUTE_RECOVERY_TIME_LINKFAIL); //10 minutes
#endif


								if(route_dwn->reason != ROUTE_REASON_MANUAL)
								{
									//rtopo_state.state = RTOPO_INIT;
									if((route_dwn->link & WIRELESS)||
										(route_dwn->link & POWER_LINE))
									{
										//rtopo_start_scan(RF_NIC, RTOPO_SCAN_PEER_SELECTION);
									}
									
									if((route_dwn->link & POWER_LINE) && 
										(rtopo_data.scan.power_line.enabled == FALSE))
									{
										rtopo_data.scan.power_line.enabled = TRUE;												
									}
								
									if( ((route_dwn->link & POWER_LINE) &&
										(((hpgp_nwk_data.params.nwk.role == DEV_MODE_STA) ?
										(nwkstartup_data.link.power_line.state == LINK_UP) : (TRUE)))) ||										
									    ((route_dwn->link & WIRELESS) &&
										(nwkstartup_data.link.wireless.state == LINK_UP)) )
									{
										nwk_stop.event = NWK_STOP_EVENT;
										nwk_stop.link = 0;
										
										if(hpgp_nwk_data.params.nwk.role == DEV_MODE_CCO)
										{
											nwk_stop.link |= PLC_NIC;
											hpgp_nwk_data.params.nwk.role = DEV_MODE_STA;											
											memcpy((u8*)hpgp_nwk_data.params.nwk.key.nid, 
													(u8*)cco_nid, NID_LEN); 					
										}
										nwk_stop.link |= ((route_dwn->link == POWER_LINE) ? 
														   PLC_NIC : RF_NIC);
										nwk_stop.link |= ((route_dwn->link == WIRELESS) ? 
														   RF_NIC : PLC_NIC);
										
										GV701x_SendAppEvent(rtopo_app_id, nwkstartup_data.app_id, 
														APP_MSG_TYPE_APPEVENT, APP_MAC_ID, EVENT_CLASS_CTRL,
														MGMT_FRM_ID, &nwk_stop, sizeof(nwk_stop_evnt_msg_t), 0); 				
											
									}
								}
							}
							break;

							case ROUTE_DISC_IND:
							{
								route_disc_ind_msg_t* route_disc = (route_disc_ind_msg_t*)state->statedata; 
#if 0								
								FM_Printf(FM_APP, "\nRdisc l %bu r %bu",	route_disc->link, route_disc->reason);					
#endif																			
								if(((route_disc->link & WIRELESS) == WIRELESS) &&
									(route_disc->reason == ROUTE_REASON_MANUAL) &&
									(rtopo_data.scan.wireless.scanMode == RTOPO_SCAN_PEER_SELECTION))
								{									
#ifndef OLD_SCAN			
									rtopo_rf_curscan_ch++;
									rtopo_start_wirscan_engine(rtopo_data.scan.wireless.scanMode);
#endif
								}															
							}
							break;
							
							default:
							break;
						}
					}
				}
#endif
								
				if(state->msg_hdr.dst_app_id != APP_BRDCST_MSG_APPID) 
				{				
					if(state->msg_hdr.type == APP_MSG_TYPE_APPEVENT)
					{									
						switch(state->event)
						{				
							case 
								RTOPO_START_EVNT:
							{
								rtopo_start_evnt_msg_t* start_msg = (rtopo_start_evnt_msg_t*)state->statedata;
								rtopo_start(start_msg->link, start_msg->mode);
							}
							break;			
							
							case RTOPO_PROFILE_EVNT:
#if (defined NWKSTARTUP_APP) && (defined ROUTE_APP)
#if 1
								FM_Printf(FM_APP, "\nProf(ws: %bu ps: %bu rs: %bu, rv: %bu pl: %bu)", 
										nwkstartup_data.link.wireless.state, 
										nwkstartup_data.link.power_line.state, route_state.state, 
										(route_info.parent != NULL)? route_info.parent->valid:0, 
										(route_info.parent != NULL)? route_info.parent->link:0);
#endif
#endif							
#if 1	
								if(route_info.route_sel_active == TRUE)
								{
									/*If timer is already running it wont update the timeout value*/
									STM_StartTimer(rtopo_data.freeze_timer, RTOPO_TOPOCHANGE_TIME);									
								}
#endif								
								/*If Wireless link is up*/
								if(rtopo_state.state == RTOPO_COMPLETE)
									return;
#ifdef NWKSTARTUP_APP
								if((nwkstartup_data.link.wireless.state == LINK_UP) &&
									(nwkstartup_data.link.power_line.state == LINK_DOWN) && 
									(hpgp_drv_data.state.state != HPGPDRV_START))
#endif						
								{
#if (defined ROUTE_APP) && (defined HPGP_DRIVER_APP) && (defined HPGP_DRIVER_APP)
									/*If routing is complete and the profiling hasnt begun*/
									if((route_state.state == ROUTE_COMPLETE) && 
										((route_info.parent == NULL) ? (0) : 
										((route_info.parent->valid == TRUE) &&
										(route_info.parent->link == WIRELESS))))
									{
										u8 tmp_nid2[NID_LEN] = {0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF};
										u8 tmp_nid3[NID_LEN] = {0x00,0x00,0x00,0x00,0x00,0x00,0x00};				
										u8 tmp_nid1[NID_LEN] = {0x00,0x00,0x00,0x00,0x00,0x00,0x00};		
										
										/*Fetch from flash if there exists any preconfigured NID
										  to start as CCO*/
#if 0										  
										if(STATUS_SUCCESS == GV701x_FlashRead(hpgp_drv_data.app_id, 
															(u8*)tmp_nid1, 
															NID_LEN))
#else
										if(1)
#endif
										{		
											if((0 == memcmp((u8*)tmp_nid1, tmp_nid2, NID_LEN)) ||
												(0 == memcmp((u8*)tmp_nid1, tmp_nid3, NID_LEN)))
											{				
												/*Request for NID from the Host*/
												rtopo_data.tx_cnt = 0;
												rtopo_send_getparam(RTOPO_PLC_NID, NULL, &rtopo_data.tx_cnt);
											}								
											else
											{
												if((0 == memcmp((u8*)tmp_nid1, cco_nid, NID_LEN-2)))
												{
													/*NID exists in flash, complete the profiling*/
													memcpy(hpgp_nwk_data.params.nwk.key.nid, tmp_nid1, NID_LEN);	
													rtopo_start_devrole(PLC_NIC);
												}										
												else
												{
													/*Request for NID from the Host*/
													rtopo_data.tx_cnt = 0;	
													rtopo_send_getparam(RTOPO_PLC_NID, NULL, &rtopo_data.tx_cnt);	
												}
											}
										}
									}	
#endif						
								}


#if (defined NWKSTARTUP_APP) && (defined ROUTE_APP) && (defined HPGP_DRIVER_APP)
								/*If PLC link is up*/
								if(nwkstartup_data.link.power_line.state == LINK_UP)
								{
#if 0								
									FM_Printf(FM_APP, "\n10.0");
#endif
									if((route_state.state == ROUTE_COMPLETE) &&
										(route_info.parent != NULL))
									{
#if 0									
										FM_Printf(FM_APP, "\n10.1");									
#endif
										if((route_info.parent->valid == TRUE) &&
											(route_info.parent->link == POWER_LINE))
										{	
											STM_StopTimer(rtopo_data.tx_timer);
#if 1										
											FM_Printf(FM_APP, "\n10.2");
#endif
											rtopo_data.scan.wireless.start = TRUE;
											//rtopo_start_scan(RF_NIC, RTOPO_SCAN_CHANNEL_SELECTION);		
											rtopo_data.scan.wireless.scanMode = RTOPO_SCAN_CHANNEL_SELECTION;
											//rtopo_start_devrole(RF_NIC);
											rtopo_state.state = RTOPO_COMPLETE;
										}
									}
								}
#endif						
							break;

							case RTOPO_STOP_EVNT:
#if 0							
								state->state = RTOPO_INIT;
								state->event = RTOPO_IDLE_EVENT;
								//rtopo_data.scan.power_line.enabled = FALSE;
#endif
							break;
							
							default:
							break;				
						}
					}
				}				
			}
		break;
			
		default:
		break;			
	}


	state->event = RTOPO_IDLE_EVENT;	
	state->eventtype = 0;
	state->eventclass = 0;
	state->eventproto = 0;
	state->statedata = NULL;	
	state->statedatalen = 0;	
	memset((u8*)&state->msg_hdr, 0x00, sizeof(gv701x_app_msg_hdr_t));
}

/******************************************************************************
 * @fn      rtopo_cmdprocess
 *
 * @brief   It handles application command line requests
 *
 * @param   CmdBuf - command string
 *
 * @return  none
 *
 */

void rtopo_cmdprocess(char* CmdBuf) 
{
	if(strcmp(CmdBuf, "state") == 0) 
	{
		printf("\nRtopo S %bu E %bu", rtopo_state.state, rtopo_state.event);			  		
	}
	else if(strcmp(CmdBuf, "stats") == 0) 
	{
		printf("\nRtopo p %bu ps %bu ws %bu (ch %bx panid %x s %bu m %bu t %bu)",
			  rtopo_data.pref.enabled , rtopo_data.scan.power_line.enabled,
			  rtopo_data.scan.wireless.enabled, rtopo_data.scan.wireless.channel,
			  rtopo_data.scan.wireless.panid, rtopo_data.scan.wireless.start,
			  rtopo_data.scan.wireless.scanMode, rtopo_data.scan.wireless.scantype);
	}
	else if(strcmp(CmdBuf, "nvclear") == 0) 
	{
		GV701x_FlashErase(rtopo_app_id);
	}
}

#endif /*RTOPO_APP*/
