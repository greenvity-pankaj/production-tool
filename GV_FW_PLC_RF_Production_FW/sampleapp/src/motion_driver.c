/* ========================================================
 *
 * @file:  motion_driver.c
 * 
 * @brief: This file implements a simple motion sensing algorithm
 *
 *  Copyright (C) 2010-2015, Greenvity Communications, Inc.
 *  All Rights Reserved
 *  
 * =========================================================*/

#ifdef MOTION_DRIVER
/****************************************************************************** 
  *	Includes
  ******************************************************************************/
#include <stdio.h>
#include <string.h>
#include "gv701x_includes.h"
#include "gv701x_osal.h"
#include "gv701x_gpiodriver.h"
#ifdef DEVICEINTF_APP
#include "deviceintfapp.h"
#endif
#include "motion_driver.h"

/****************************************************************************** 
  *	Global Data
  ******************************************************************************/

/*Motion Database*/
motion_driver_db_t motion_driver_db;

/*Motion TLV's*/
io_list_t motion_io_list[] = 
{{"enable", MOTION_ENABLE, 1, &motion_driver_db.motion.enable, FALSE}, 
 {"detected", MOTION_DETECTED, 1, &motion_driver_db.motion.detected, FALSE}, 
 {"none", DEVINTF_IO_NONE, 0, NULL, FALSE}
};

/****************************************************************************** 
  *	Function Prototypes
  ******************************************************************************/

void motion_driver_timerhandler(u8* buf);

/******************************************************************************
 * @fn      motion_driver_init
 *
 * @brief   Initializes the Motion driver
 *
 * @param   app_id - application identification number
 *
 * @return  none
 */

void motion_driver_init(u8 appid)
{
	memset(&motion_driver_db, 0x00, sizeof(motion_driver_db_t));
    motion_driver_db.app_id = appid;
	SLIST_Init(&motion_driver_db.queues.appRxQueue);
    motion_driver_db.motion.enable = TRUE;
    motion_driver_db.motion.detected = FALSE;
    motion_driver_db.motion.timer = STM_AllocTimer(SW_LAYER_TYPE_APP, 
                                        MOTION_TIMEOUT_EVNT, 
                                        &motion_driver_db.app_id);
		
	/*Configure PIR GPIO as input pin*/
    GV701x_GPIO_Config(READ_ONLY, PIR_INPUT_PIN);
	/*Configure Sensor control GPIO as output pin*/
    GV701x_GPIO_Config(WRITE_ONLY, SENSOR_OUTPUT_PIN);
	/*Turn On Sensor*/
    GV701x_GPIO_Write(SENSOR_OUTPUT_PIN, ON); 
}

/******************************************************************************
 * @fn      motion_driver_poll
 *
 * @brief   Perodic polling function for motion sensing
 *
 * @param   none
 *
 * @return  none
 */

void motion_driver_poll(void)
{
	u8 idx;

	for(idx = 0; (motion_io_list[idx].type != DEVINTF_IO_NONE); idx++) 
	{	
		switch(motion_io_list[idx].type)
		{				
			case MOTION_DETECTED:
				if(motion_driver_db.motion.enable == TRUE)				
				{
					/*Check if Motion is Detected*/
					if(GV701x_GPIO_Read(PIR_INPUT_PIN) == TRUE)							
					{													    
						if(motion_driver_db.motion.detected == FALSE)
						{
							node_trigger_evnt_msg_t node_trigger;	
							
							motion_io_list[idx].p_val = TRUE;
							
							node_trigger.event = NODE_TRIGGER_EVNT;
							GV701x_SendAppEvent(motion_driver_db.app_id, node_app_id, APP_MSG_TYPE_APPEVENT,
												APP_MAC_ID, EVENT_CLASS_CTRL, MGMT_FRM_ID,
												&node_trigger, sizeof(node_trigger_evnt_msg_t), 0);
							STM_StartTimer(motion_driver_db.motion_timer, MOTION_SAMPLING_TIME);
						}
						else
						{
							STM_StopTimer(motion_driver_db.motion_timer);
							STM_StartTimer(motion_driver_db.motion_timer, MOTION_SAMPLING_TIME);
						}
					}
					else
					{
						if(motion_driver_db.motion.detected == FALSE)
						{
							node_trigger_evnt_msg_t node_trigger;
							
							motion_io_list[idx].p_val = FALSE;
							node_trigger.event = NODE_TRIGGER_EVNT;
							GV701x_SendAppEvent(motion_driver_db.app_id, node_app_id, APP_MSG_TYPE_APPEVENT,
												APP_MAC_ID, EVENT_CLASS_CTRL, MGMT_FRM_ID,
												&node_trigger, sizeof(node_trigger_evnt_msg_t), 0);							
						}
					}
				}
			break;

			default:
			break;
		}
	}	
}

/******************************************************************************
 * @fn      motion_driver_rxappmsg
 *
 * @brief   Receives a message from another app/fw
 *
 * @params  event - message buffer
 *
 * @return  none
 */

void motion_driver_rxappmsg(sEvent* event)
{
	gv701x_app_msg_hdr_t* msg_hdr = (gv701x_app_msg_hdr_t*)event->buffDesc.dataptr;
	hostHdr_t* hybrii_hdr;
	hostEventHdr_t* evnt_hdr;

	hybrii_hdr = (hostHdr_t*)(msg_hdr + 1);

	if(msg_hdr->dst_app_id == motion_driver_db.appid)
	{
		if((msg_hdr->src_app_id == APP_FW_MSG_APPID) && 
			(hybrii_hdr->type == EVENT_FRM_ID) &&
			(led_state.event == HOST_EVENT_APP_TIMER))
		{			
			motion_driver_timerhandler((u8*)(evnt_hdr + 1)); 
			return;
		}				
	}
	else if(msg_hdr->dst_app_id == APP_BRDCST_MSG_APPID)
	{
		u8 *event = (u8*)(hybrii_hdr + 1);
		return;
	}	
}

/******************************************************************************
 * @fn		motion_driver_timerhandler
 *
 * @brief	Timer handler for Motion timer events
 *
 * @param	event - event from firmware
 *
 * @return	none
 *
 */ 
void motion_driver_timerhandler(u8* buf)
{		
	hostTimerEvnt_t* timerevt = (hostTimerEvnt_t*)buf;			

	if(buf == NULL)
		return; 
		
	/*Demultiplexing the specific timer event*/ 					
	switch((u8)timerevt->type)
	{								
		case MOTION_SAMPLE_TIMEOUT_EVNT:				
			if(motion_driver_db.motion.detected == TRUE)
			{
            	motion_driver_db.motion.detected = FALSE;
			}
		break;
		
		default:
		break;
	}
}
#endif /*MOTION_DRIVER*/
