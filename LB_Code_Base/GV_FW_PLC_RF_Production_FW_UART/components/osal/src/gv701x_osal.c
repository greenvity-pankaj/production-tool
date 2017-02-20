/* ========================================================
 *
 * @file: gv701x_osal.c
 * 
 * @brief: This file contains the initialization routine and task context 
 *         in which the application needs to place its appropriate functions 
 *
 *  Copyright (C) 2010-2015, Greenvity Communications, Inc.
 *  All Rights Reserved
 *  
 * =========================================================*/

/****************************************************************************** 
  *	Includes
  ******************************************************************************/
	  
#include <string.h>
#include "stdio.h"
#include "gv701x_includes.h"
#ifdef LRWPAN_DRIVER_APP
#include "gv701x_lrwpandriver.h"
#endif
#ifdef SMARTMETER_APP
#include "smapp.h"
#endif
#ifdef UART_APP
#include "uartapp_new.h"
#endif
#ifdef UART_OLD_APP
#include "uartapp.h"
#endif
#ifdef LLP_APP
#include "llpapp.h"
#endif
#ifdef REGISTER_APP
#include "register.h"
#endif
#ifdef RTOPO_APP
#include "route_topo.h"
#endif
#ifdef ROUTE_APP
#include "route.h"
#endif
#ifdef HPGP_DRIVER_APP
#include "gv701x_hpgpdriver.h"
#endif
#ifdef NWKSTARTUP_APP
#include "gv701x_nwkstartup.h"
#endif
#ifdef DEVICEINTF_APP
#include "deviceintfapp.h"
#endif
#ifdef SENSOR_DRIVER
#include "sensor_driver.h"
#endif
#ifdef SMARTPLUG_DRIVER
#include "smartplug_driver.h"
#endif
#ifdef SMARTGRID_APP
#include "smartgridapp.h" 
#endif
#if ((defined SMARTLIGHT_APP) && ((defined LED_RGB_LIGHT) || (defined LED_WNC_LIGHT) \
	|| (defined LED_WHITE_LIGHT) || (defined LED_SMART_LIGHT)))
#include "smartlightapp.h"
#endif
#ifdef RGB_FADING_DEMO
#include "rgbfading.h"
#endif
#ifdef MOTION_DRIVER
#include "motion_driver.h"
#endif

/****************************************************************************** 
  *	Global Data
  ******************************************************************************/

gv701x_aps_queue_t* NwkIntfQueue;
gv701x_app_queue_t AppIntfQueue;

code gv701x_app_queue_lookup_t AppQueueLookup[APP_MAX_APPLICATIONS] = {
	{0, NULL, NULL},
#ifdef HPGP_DRIVER_APP		
	{1, GV701x_HPGPDriverRxAppMsg, &hpgp_drv_data.queues},
#else
    {1, NULL, NULL},
#endif
#ifdef BRIDGE
#ifdef LRWPAN_DRIVER_APP		
	{2, GV701x_LrwpanDriverRxAppMsg, &lrwpan_db.queues},
#else
    {2, NULL, NULL},
#endif
#else
	{2, NULL, NULL},
#endif
#ifdef NWKSTARTUP_APP		
	{3, GV701x_NwkRxAppMsg, &nwkstartup_data.queues},
#else
    {3, NULL, NULL},
#endif
#ifdef RTOPO_APP		
	{4, rtopo_RxAppMsg, &rtopo_queues},
#else
    {4, NULL, NULL},
#endif
#ifdef ROUTE_APP		
	{5, RouteApp_RxAppMsg, &route_queues},
#else
    {5, NULL, NULL},
#endif
#ifdef REGISTER_APP		
	{6, RegisterApp_RxAppMsg, &register_queues},
#else
    {6, NULL, NULL},
#endif
#ifdef LLP_APP		
	{7, LlpApp_RxAppMsg, &node_queues},
#else
    {7, NULL, NULL},
#endif
#ifdef DEVICEINTF_APP		
	{8, DeviceIntfApp_RxAppMsg, &deviceintf_data.queues},
#else
    {8, NULL, NULL},
#endif
#ifdef SMARTPLUG_DRIVER		
	{9, smartplug_driver_rxappmsg, &sp_queues},
#else
    {9, NULL, NULL},
#endif
	{10, GV701x_I2C_RxAppMsg, &i2c_data.queues},
#if ((defined SMARTLIGHT_APP) && ((defined LED_RGB_LIGHT) || (defined LED_WNC_LIGHT) \
	|| (defined LED_WHITE_LIGHT) || (defined LED_SMART_LIGHT)))
	{11, led_driver_rxappmsg, &led_drv_db.queues},
#else
    {11, NULL, NULL},
#endif
#ifdef MOTION_DRIVER	
	{12, motion_driver_rxappmsg, &motion_driver_db.queue},
#else
	{12, NULL, NULL},
#endif
#ifdef RGB_FADING_DEMO
	{13, NULL, NULL},
#else
    {13, NULL, NULL},
#endif
#ifdef SMARTGRID_APP
	{14, NULL, NULL},
#else
    {14, NULL, NULL},
#endif
#ifdef SENSOR_DRIVER	
	{15, SensorApp_RxAppMsg, &sensor_queue},
#else
	{15, NULL, NULL},
#endif
	
};

/****************************************************************************** 
  *	External Data
  ******************************************************************************/
  
/******************************************************************************
  *	External Funtion prototypes
  ******************************************************************************/
  
/******************************************************************************
  * Funtion prototypes
  ******************************************************************************/

/******************************************************************************
 * @fn      GV701x_TaskInit
 *
 * @brief   Initialization function where the application 
 * 		    needs to place its initialization code	
 *
 * @param   nwkIntfQueue - Firmware passes a reference to the Tx Rx Data/event/response 
 * 		    queues that the application would use to send/receive messages to the MAC
 * 
 *			app_id - Identifier numnber for Application (used while starting timers, etc)
 *
 * @return  none
 */

void GV701x_TaskInit(gv701x_aps_queue_t* nwkIntfQueue)
{
	/*memset(&AppQueueLookup, 0x00, 
		sizeof(gv701x_app_queue_lookup_t)*APP_MAX_APPLICATIONS);*/
	
	NwkIntfQueue = nwkIntfQueue;
	SLIST_Init(&AppIntfQueue.appRxQueue);

#ifdef HPGP_DRIVER_APP			
	GV701x_HPGPDriverInit(AppQueueLookup[1].app_id);	
#endif
#ifdef LRWPAN_DRIVER_APP
	GV701x_LrwpanDriverInit(AppQueueLookup[2].app_id);
#endif
#ifdef NWKSTARTUP_APP					
	GV701x_NwkInit(AppQueueLookup[3].app_id);	
#endif
#ifdef SMARTMETER_APP
	SMApp_Init(AppQueueLookup[3].app_id);
#endif
#ifdef UART_APP
	UartApp_Init(AppQueueLookup[3].app_id);
#endif
#ifdef RTOPO_APP
	rtopo_init(AppQueueLookup[4].app_id);
#endif
#ifdef ROUTE_APP
	RouteApp_Init(AppQueueLookup[5].app_id);
#endif
#ifdef REGISTER_APP
	RegisterApp_Init(AppQueueLookup[6].app_id);
#endif
#ifdef LLP_APP
	LlpApp_Init(AppQueueLookup[7].app_id);
#endif
#ifdef DEVICEINTF_APP
	DeviceIntfApp_Init(AppQueueLookup[8].app_id);
#endif
#ifdef SMARTPLUG_DRIVER
	smartplug_driver_init(AppQueueLookup[9].app_id);
#endif
	GV701x_I2C_Init(AppQueueLookup[10].app_id);
#if ((defined SMARTLIGHT_APP) && ((defined LED_RGB_LIGHT) || (defined LED_WNC_LIGHT) \
	|| (defined LED_WHITE_LIGHT) || (defined LED_SMART_LIGHT)))
	led_driver_init(AppQueueLookup[11].app_id);
#endif
#ifdef MOTION_DRIVER
	motion_driver_init(AppQueueLookup[12].app_id);
#endif
#ifdef RGB_FADING_DEMO
	rgbfading_init(AppQueueLookup[13].app_id);
#endif
#ifdef SMARTGRID_APP
	smartgridApp_init(AppQueueLookup[14].app_id);
#endif
#ifdef SENSOR_DRIVER
	sensor_driver_init(AppQueueLookup[15].app_id);
#endif
}

/******************************************************************************
 * @fn      GV701x_Task
 *
 * @brief   Task body in which application places any perodic or polling routines
 *
 * @param   none
 *
 * @return  none
 *
 * @note here APP_TASK_ID is the task id. It SHOULD NOT be changed
 */
#ifdef RTX51_TINY_OS
void GV701x_Task(void) _task_ APP_TASK_ID
#else
void GV701x_Task(void)
#endif
{
	while(1) 
	{
		os_switch_task();
		
		GV701x_ReadQueue();

		GV701x_ReadAppQueue();
		
		/*Place the all application polling routines here*/
#ifdef RTOPO_APP
		rtopo_poll();
#endif
#ifdef ROUTE_APP			
		RouteApp_Poll();
#endif		
#ifdef DEVICEINTF_APP
		DeviceIntfApp_SM(&deviceintf_state);
#endif
#ifdef SMARTPLUG_DRIVER
		//smartplug_driver_sm(&sp_state);	
		smartplug_poll();		
#endif
#ifdef SENSOR_DRIVER
		GV701x_I2C_Poll();
		sensor_driver_i2c_poll();
#endif
#ifdef MOTION_DRIVER
		motion_driver_poll();
#endif
	}
}

/******************************************************************************
 * @fn      GV701x_ReadQueue
 *
 * @brief   Reads the firmware data and event queues
 *
 * @param   none
 *
 * @return  none
 *
 */

void GV701x_ReadQueue(void)
{
    sEvent *event = NULL;
    sSlink *slink = NULL;
    u8 ret = 0;   	

	/*Read Data Queue*/
	while(!SLIST_IsEmpty(&(NwkIntfQueue->rxQueue)))
	{
__CRIT_SECTION_BEGIN__
		slink = SLIST_Pop(&(NwkIntfQueue->rxQueue));
__CRIT_SECTION_END__
		event = SLIST_GetEntry(slink, sEvent, link);

		if(event == NULL)
		{
			break;
		}

		if((event->buffDesc.dataptr == NULL) || 
		  (event->buffDesc.datalen == 0) )
		{
			EVENT_Free(event);	
			break; 
		}
		
		if(event->eventHdr.eventClass != EVENT_CLASS_DATA)
		{
			EVENT_Free(event);	
			break;
		}

		if(event->eventHdr.trans == APP_PORT_PLC)
		{
			/*Place application sata handlers here*/		
#ifdef ROUTE_APP					
			route_handle_rx_from_ll (event->buffDesc.dataptr, 
				   event->buffDesc.datalen, POWER_LINE, 0);
#endif		
#ifdef SMARTMETER_APP
			SMApp_Rx(event->eventHdr.trans, event->buffDesc.dataptr, 
					 event->buffDesc.datalen);
#endif
		}
		
		if(event->eventHdr.trans == APP_PORT_ZIGBEE)
		{
#ifdef SMARTMETER_APP
			SMApp_Rx(event->eventHdr.trans, event->buffDesc.dataptr, 
					 event->buffDesc.datalen);
#endif
		}
		
		if(event->eventHdr.trans == APP_PORT_PERIPHERAL)
		{
#ifdef DEVICEINTF_APP		
#ifdef SMARTPLUG_DRIVER
			smartplug_driver_rx(event->buffDesc.dataptr,
							event->buffDesc.datalen);
#endif
#ifdef SMARTGRID_APP
			smartgridApp_intf_rx(event->buffDesc.dataptr,
							event->buffDesc.datalen);
#endif
#endif
#ifdef UART_APP
			UartApp_Rx(event->eventHdr.trans, event->buffDesc.dataptr,
						event->buffDesc.datalen);
#endif		
		}

		EVENT_Free(event);	
		break;
	}
}

/******************************************************************************
 * @fn      GV701x_SendData
 *
 * @brief   Adds to the Firmware Queue the Data to be sent
 *
 * @param   port - Port to which data is to be sent (PLC or IEEE 802.15.4) 
 *          databuf - Data to be sent
 *		    len - Length of Data 
 *			options - Transmit options (defined in gv701x_osal.h)
 *
 * @return  eStatus - status (STATUS_SUCCESS - if success, STATUS_FAILURE - on failure)
 */
 
eStatus GV701x_SendData(u8 port, void* databuf, u16 len, u8 options)
{
    sEvent* event = NULL;

 	if((databuf == NULL) || (len == 0))
 	{
		return STATUS_FAILURE;
 	}
			
	event = GV701x_EVENT_Alloc(len, 0);
	
    if(event == NULL) 
    {
    	return STATUS_FAILURE;
    }

    event->eventHdr.eventClass = EVENT_CLASS_DATA;
    event->eventHdr.type = 0;
    event->eventHdr.trans = port;	
	event->eventHdr.status = 0;
		
	if(options & APP_DATA_TX_SWBCST_OPT)			
		event->eventHdr.status |= APP_DATA_TX_SWBCST_OPT;	
	
	memcpy((u8*)(event->buffDesc.dataptr), databuf, len);    	
	event->buffDesc.datalen = len;	

    /*Enqueue data into the firmware queue*/
	SLIST_Put(&(NwkIntfQueue->txQueue), &event->link);			
    return STATUS_SUCCESS;       
}

/******************************************************************************
 * @fn      GV701x_SendAppEvent
 *
 * @brief   Adds to the Application/fw Queue the event to be sent
 *
 * @param   src_app_id - source application 
 *          dst_app_id - destination application
 *		    msg_type - event/message type (in gv701x_osal.h)
 *			protocol - protocol type of the message/event (in nma.h)
 *			evnt_class - the event type of the message/event (in event.h)
 *			type - the frame type of the message/event (in nma.h)
 *			databuf - the reference to the data to be sent
 *			len - the length of the message/event
 *			options - the queuing options (eg. priority etc. in gv701x_osal.h)
 *
 * @return  eStatus - status (STATUS_SUCCESS - if success, STATUS_FAILURE - on failure)
 */
 
eStatus GV701x_SendAppEvent(u8 src_app_id, u8 dst_app_id, u8 msg_type, u8 protocol, u8 evnt_class,
							u8 type, void* databuf, u8 len, u8 options)
{
    sEvent* event = NULL;
	sSlist* txqueue = NULL;
	gv701x_app_msg_hdr_t* msg_hdr = NULL;
	hostHdr_t* pHostHdr; 	

 	if((databuf == NULL) || (len == 0) || 
	   ((src_app_id > APP_MAX_APPLICATIONS) ? 
	   ((src_app_id == APP_FW_MSG_APPID) ? (FALSE) : (TRUE)) : (FALSE)))	
 	{
		return STATUS_FAILURE;
 	}

	if((protocol != HPGP_MAC_ID) && (protocol != IEEE802_15_4_MAC_ID) &&
		(protocol != APP_MAC_ID) && (protocol != SYS_MAC_ID))
		return STATUS_FAILURE;		

	if((evnt_class != EVENT_CLASS_CTRL) && (evnt_class != EVENT_CLASS_MGMT))
		return STATUS_FAILURE;

	if(protocol == APP_MAC_ID) 		
	{
		if(dst_app_id <= APP_MAX_APPLICATIONS)
		{
			if(AppQueueLookup[dst_app_id].queues == NULL)
				return STATUS_FAILURE;
			
			txqueue = &AppQueueLookup[dst_app_id].queues->appRxQueue;		
		}
		else if(dst_app_id == APP_BRDCST_MSG_APPID)
			txqueue = &AppIntfQueue.appRxQueue;
	}
	else if((protocol == HPGP_MAC_ID) || (protocol == SYS_MAC_ID) ||
			(protocol == IEEE802_15_4_MAC_ID))
		txqueue = NwkIntfQueue->reqTxQueue;		

	event = GV701x_EVENT_Alloc(len + sizeof(gv701x_app_msg_hdr_t) + sizeof(hostHdr_t), 0);
	if(event == NULL) 
		return STATUS_FAILURE;
	
	event->eventHdr.eventClass = evnt_class;
	event->eventHdr.type = 0;
	event->eventHdr.trans = APP_PORT_APPLICATION;			
	
	msg_hdr = (gv701x_app_msg_hdr_t*)event->buffDesc.dataptr;	
	memcpy((u8*)(&event->buffDesc.dataptr[sizeof(gv701x_app_msg_hdr_t) + sizeof(hostHdr_t)]),
			databuf, len);		
	event->buffDesc.datalen = len + sizeof(gv701x_app_msg_hdr_t) + sizeof(hostHdr_t);	

	/*Fill the app msg header*/
	msg_hdr->dst_app_id = dst_app_id;
	msg_hdr->src_app_id = src_app_id;
	msg_hdr->type = msg_type;
	msg_hdr->len = len + sizeof(hostHdr_t);
	
	/*Fill the hybrii header*/
	pHostHdr = (hostHdr_t*)(msg_hdr + 1);
	pHostHdr->protocol = protocol;
	pHostHdr->type = type;		
	pHostHdr->length = len;
	pHostHdr->rsvd = 0;		

	if((dst_app_id <= APP_BRDCST_MSG_APPID) || (dst_app_id == APP_FW_MSG_APPID))
	{
#if 0	
		FM_Printf(FM_APP, "\nQ to App src %bu dst %bu len %u q %p", src_app_id, dst_app_id, 
				pHostHdr->length, txqueue);			
#endif
	}	

	if(options & APP_EVNT_TX_CRITICAL_OPT)
		SLIST_Push(txqueue, &event->link);			
	else
		SLIST_Put(txqueue, &event->link);			
		
	return STATUS_SUCCESS; 	
}

/******************************************************************************
 * @fn      GV701x_ReadAppQueue
 *
 * @brief   Reads the application event queues
 *
 * @param   none
 *
 * @return  none
 *
 */

void GV701x_ReadAppQueue(void)
{
    sEvent *event = NULL;
    sSlink *slink = NULL;
    u8 ret = 0;   	
	u8 i;
	gv701x_app_msg_hdr_t* msg_hdr = NULL;

	for(i = 1; i < APP_MAX_APPLICATIONS; i++)
	{
		if((AppQueueLookup[i].app_rx_handler == NULL) ||
			(AppQueueLookup[i].queues == NULL) ||
			(AppQueueLookup[i].app_id == 0) || 
			(AppQueueLookup[i].app_id > APP_MAX_APPLICATIONS))
			continue;
		
		/*Read App Data Queue*/
		while(!SLIST_IsEmpty(&AppQueueLookup[i].queues->appRxQueue))
		{
__CRIT_SECTION_BEGIN__
			slink = SLIST_Pop(&AppQueueLookup[i].queues->appRxQueue);
__CRIT_SECTION_END__
			event = SLIST_GetEntry(slink, sEvent, link);

			if(event == NULL)
			{
				break;
			}

			if((event->buffDesc.dataptr == NULL) || (event->buffDesc.datalen == 0)  ||			  
			  (event->eventHdr.trans != APP_PORT_APPLICATION))
			{
				EVENT_Free(event);	
				break; 
			}
			
			msg_hdr = (gv701x_app_msg_hdr_t*)event->buffDesc.dataptr;		
#if 0			
			FM_Printf(FM_APP, "\nApp Q pop (app id %bu src id %bu dst id %bu )", i,
				msg_hdr->src_app_id, msg_hdr->dst_app_id);
#endif

			if((msg_hdr->dst_app_id <= APP_MAX_APPLICATIONS) &&
				(msg_hdr->src_app_id <= APP_MAX_APPLICATIONS) && 
				(i == msg_hdr->dst_app_id))
			{
#if 0			
				FM_Printf(FM_APP, "\ni %bu Handlr %p", i, AppQueueLookup[i].app_rx_handler);				
#endif
				AppQueueLookup[i].app_rx_handler(event);		
			}	
			EVENT_Free(event);	
			break;
		}   
	}

	while(!SLIST_IsEmpty(&AppIntfQueue.appRxQueue))
	{
__CRIT_SECTION_BEGIN__
		slink = SLIST_Pop(&AppIntfQueue.appRxQueue);
__CRIT_SECTION_END__
		event = SLIST_GetEntry(slink, sEvent, link);
		
		if(event == NULL)
			break; 

		if((event->buffDesc.dataptr == NULL) || (event->buffDesc.datalen == 0)  ||		
		  (event->eventHdr.trans != APP_PORT_APPLICATION))
		{
			EVENT_Free(event);	
			break; 
		}
		
		msg_hdr = (gv701x_app_msg_hdr_t*)event->buffDesc.dataptr;
		
		if(msg_hdr->dst_app_id == APP_BRDCST_MSG_APPID)
		{
			for(i = 1; i < APP_MAX_APPLICATIONS; i++)
			{
				if((AppQueueLookup[i].app_rx_handler == NULL) ||
					(AppQueueLookup[i].queues == NULL) ||
					(AppQueueLookup[i].app_id == 0) || 
					(AppQueueLookup[i].app_id > APP_MAX_APPLICATIONS))
					continue;
				
				if(AppQueueLookup[i].app_id != msg_hdr->src_app_id)
				{
#if 0				
					FM_Printf(FM_APP, "\nBrdcst App Q pop (app id %bu src id %bu dst id %bu %p)", i,
						    msg_hdr->src_app_id, msg_hdr->dst_app_id, 
						    AppQueueLookup[i].app_rx_handler);							
#endif
					AppQueueLookup[i].app_rx_handler(event);							
				}
			}
		}	
		EVENT_Free(event);	
		break;
	}  

	/*Read Event Queue*/
	while(!SLIST_IsEmpty(&(NwkIntfQueue->rspEvntRxQueue)))
	{
__CRIT_SECTION_BEGIN__
		slink = SLIST_Pop(&(NwkIntfQueue->rspEvntRxQueue));
__CRIT_SECTION_END__
		event = SLIST_GetEntry(slink, sEvent, link);

		if(event == NULL)
			break;

		if((event->buffDesc.dataptr == NULL) || (event->buffDesc.datalen == 0))		  
		{
			EVENT_Free(event);	
			break; 
		}

		msg_hdr = (gv701x_app_msg_hdr_t*)event->buffDesc.dataptr;
#if 0		
		FM_Printf(FM_APP, "\nFW Q pop (src id %bu dst id %bu )", msg_hdr->src_app_id, msg_hdr->dst_app_id);						
#endif
		if(msg_hdr->src_app_id == APP_FW_MSG_APPID)
		{
			for(i = 1; i < APP_MAX_APPLICATIONS; i++)
			{
				if((AppQueueLookup[i].app_rx_handler == NULL) ||
					(AppQueueLookup[i].queues == NULL) ||
					(AppQueueLookup[i].app_id == 0) || 
					(AppQueueLookup[i].app_id > APP_MAX_APPLICATIONS))
					continue;

				if(msg_hdr->dst_app_id == APP_BRDCST_MSG_APPID)
					AppQueueLookup[i].app_rx_handler(event);
				else if((msg_hdr->dst_app_id <= APP_MAX_APPLICATIONS) && (i == msg_hdr->dst_app_id))
				{			
#if 0
					FM_Printf(FM_APP, "\ni %bu app id %bu Handlr %p", i, msg_hdr->dst_app_id,
						AppQueueLookup[i].app_rx_handler);				
#endif
					AppQueueLookup[i].app_rx_handler(event);
					break;
				}
			}
		}
		EVENT_Free(event);		   
		break;
	}	
}
