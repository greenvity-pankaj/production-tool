/* ========================================================
 *
 * @file:  deviceintfapp.c
 * 
 * @brief: This file implements a simple peripheral interfacing
 *		   layer for drivers to be pulgged in
 *
 *  Copyright (C) 2010-2015, Greenvity Communications, Inc.
 *  All Rights Reserved
 *  
 * =========================================================*/
#ifdef DEVICEINTF_APP

/****************************************************************************** 
  *	Includes
  ******************************************************************************/
  
#include <stddef.h>
#include "string.h"
#include "stdio.h"
#include "stdlib.h"
#include "gv701x_includes.h"
#include "deviceintfapp.h"
#ifdef LLP_APP
#include "llpapp.h"
#endif
#if ((defined SMARTLIGHT_APP) && ((defined LED_RGB_LIGHT) || (defined LED_WNC_LIGHT) \
	|| (defined LED_WHITE_LIGHT) || (defined LED_SMART_LIGHT)))
#include "smartlightapp.h"
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
#ifdef RGB_FADING_DEMO
#include "rgbfading.h"
#endif
#ifdef MOTION_DRIVER
#include "motion_driver.h"
#endif

/****************************************************************************** 
  *	Global Data
  ******************************************************************************/

gv701x_state_t deviceintf_state;			
deviceintf_data_t deviceintf_data;
device_list_t dev_list[] = 	
{	
#ifdef LED_RGB_LIGHT
	{"RGBLight", DEVINTF_1, DEVINTF_NONE, &led_rgb_io_list},	
#endif	
#ifdef LED_WNC_LIGHT
	{"WNCLight", DEVINTF_2,	DEVINTF_NONE, &led_temp_io_list},
#endif	
#ifdef LED_WHITE_LIGHT
	{"WhiteLight", DEVINTF_3, DEVINTF_NONE, &led_dim_io_list},
#endif	
#ifdef SENSOR_DRIVER	
	{"Sensor", DEVINTF_4, DEVINTF_NONE, &sensor_io_list},    
#endif	
#ifdef SMARTPLUG_DRIVER
	{"SmartPlug", DEVINTF_5, DEVINTF_NONE, &smartplug_io_list},	   
#endif	
#ifdef LED_SMART_LIGHT
	{"SmartLight", DEVINTF_6, DEVINTF_NONE, &led_smartlight_io_list},	   
#endif
#ifdef SMARTGRID_APP
	{"SmartGrid", DEVINTF_20, DEVINTF_NONE, &smartgrid_io_list},	   
#endif	
	{"None", DEVINTF_NONE, DEVINTF_NONE, NULL} 	
};

/******************************************************************************
  *	Funtion prototypes
  ******************************************************************************/

/******************************************************************************
 * @fn      DeviceIntfApp_Init
 *
 * @brief   Initializes Device Interfacing Layer
 *
 * @param   app_id - application identification number
 *
 * @return  none
 */
 
void DeviceIntfApp_Init(u8 app_id) 
{	
#if 0	
	u8 i;
#endif
	
	deviceintf_data.app_id = app_id;	
	deviceintf_data.dev_list = dev_list;

	SLIST_Init(&deviceintf_data.queues.appRxQueue);
	FM_Printf(FM_USER, "\nInit DevIntf (app id %bu)", app_id);

	/*Initializing the State machine*/
	memset(&deviceintf_state, 0x00, sizeof(gv701x_state_t));
	deviceintf_state.state = DEVINTF_IO_NONE;
	deviceintf_state.event = DEVINTF_IO_NONE;	
	deviceintf_state.statedata = NULL;
	deviceintf_state.statedatalen = 0;

#if 0
	for(i = 0; (dev_list[i].type != DEVINTF_NONE); i++)
	{
		FM_Printf(FM_APP, "\n(%bu.)Device Type %s(%bu)", i, dev_list[i].name, dev_list[i].type);
	}
#endif	
}

/******************************************************************************
 * @fn      DeviceIntfApp_RxAppMsg
 *
 * @brief   Receives a message from another app/fw
 *
 * @params  msg_buf - message buffer
 *
 * @return  none
 */

void DeviceIntfApp_RxAppMsg(sEvent* event)
{
	gv701x_app_msg_hdr_t* msg_hdr = (gv701x_app_msg_hdr_t*)event->buffDesc.dataptr;
	hostHdr_t* hybrii_hdr;
	hostEventHdr_t* evnt_hdr;

	hybrii_hdr = (hostHdr_t*)(msg_hdr + 1);

	if(msg_hdr->dst_app_id == deviceintf_data.app_id)
	{
		memcpy(&deviceintf_state.msg_hdr, msg_hdr, sizeof(gv701x_app_msg_hdr_t));
		deviceintf_state.eventproto = hybrii_hdr->protocol;
		if((msg_hdr->src_app_id == APP_FW_MSG_APPID) && 
			(hybrii_hdr->type == EVENT_FRM_ID))
		{
			evnt_hdr = (hostEventHdr_t*)(hybrii_hdr + 1);
			deviceintf_state.event = evnt_hdr->type; 	
			deviceintf_state.statedata = (u8*)(evnt_hdr + 1);
			deviceintf_state.statedatalen = (u16)(hybrii_hdr->length - sizeof(hostEventHdr_t));		
		}
		else
		{
			deviceintf_state.event = (u8)(*((u8*)(hybrii_hdr + 1)));
			deviceintf_state.statedata = (u8*)(hybrii_hdr + 1);
			deviceintf_state.statedatalen = (u16)hybrii_hdr->length;
		}		
		deviceintf_state.eventtype = hybrii_hdr->type;
		deviceintf_state.eventclass = event->eventHdr.eventClass;

		if((msg_hdr->src_app_id == APP_FW_MSG_APPID) && 
			(hybrii_hdr->type == EVENT_FRM_ID) &&
			(deviceintf_state.event == HOST_EVENT_APP_TIMER))
		{			
			return;
		}		
		else if((msg_hdr->src_app_id == APP_FW_MSG_APPID) && 
			(hybrii_hdr->type == EVENT_FRM_ID) &&
			(deviceintf_state.event == HOST_EVENT_APP_CMD))
		{			
			DeviceIntfApp_CmdProcess((char*)(evnt_hdr + 1)); 
			return;
		}			
	}	
	else if(msg_hdr->dst_app_id == APP_BRDCST_MSG_APPID)
	{
		u8 *event = (u8*)(hybrii_hdr + 1);
		return;
	}		
	DeviceIntfApp_SM(&deviceintf_state);		
}

/******************************************************************************
 * @fn      DeviceIntfApp_SM
 *
 * @brief   Services all application events in all possible states 
 *
 * @param   state - State machine structure containing current
 *				    state and current event to be serviced
 *
 * @return  none
 *
 * @note    This is not a polling function. It is called asynchronously 
 *          as and when and event occurs.
 */

void DeviceIntfApp_SM(gv701x_state_t* state) 
{
	static u8 idx = 0;
	static u8 jdx = 0;		

	if(state == NULL)
		return;

#if 0
	if(state->event != DEVINTF_IO_NONE)
		FM_Printf(FM_APP,"\nDevice State %bu Event %bu P %bu C %bu E %bu Da %bu Sa %bu T %bu", 
				state->state, state->event,
				state->eventproto, state->eventclass, state->eventtype, 
				state->msg_hdr.dst_app_id, state->msg_hdr.src_app_id, state->msg_hdr.type);
#endif	
	switch(state->state)
	{
		case DEVINTF_IO_NONE:
			if(state->eventproto == APP_MAC_ID)
			{				
				switch(state->event)
				{
					case DEVINTF_IO_NONE:
					break;
					
					case DEVINTF_IO_INSTRUCTION:											
						state->state = DEVINTF_IO_INSTRUCTION;
					break;
					case DEVINTF_IO_OFF:
						{
							u8 dev_type;
							
					device_inst_msg_t *device_inst = (device_inst_msg_t*)state->statedata;
									
                	dev_type = device_inst->dev_type;
								
	               for(idx = 0; (deviceintf_data.dev_list[idx].type != DEVINTF_NONE); idx++)
	            	{
	                    //if(deviceintf_data.dev_list[idx].type != DEVINTF_NONE)
	                    if(deviceintf_data.dev_list[idx].type == dev_type)
	            		{
	                        for(jdx = 0; (deviceintf_data.dev_list[idx].io_list[jdx].type != DEVINTF_IO_NONE); jdx++) 
	        				{		
	        		//			if(deviceintf_data.dev_list[idx].io_list[jdx].trigger == TRUE)
	        					{
	        						deviceintf_data.dev_list[idx].io_list[jdx].trigger = FALSE;
	                                *((u8*)deviceintf_data.dev_list[idx].io_list[jdx].p_val) = 0;
	        						//state->state = deviceintf_data.dev_list[idx].io_list[jdx].type;
#if ((defined SMARTLIGHT_APP) && ((defined LED_RGB_LIGHT) || (defined LED_WNC_LIGHT) \
		|| (defined LED_WHITE_LIGHT) || (defined LED_SMART_LIGHT)))		
	        			            led_driver_control(deviceintf_data.dev_list[idx].io_list[jdx].type);			


#endif								
	        					//	break;
	        					}
	        				}
	        								
	            			break;
	            		}
	                    
	            	}
					jdx = 0;
					idx = 0;
					state->state = DEVINTF_IO_NONE;
					state->event = DEVINTF_IO_NONE; 							
	                break;

						}
					default:	
					break;
				}
			}
		break;

		case DEVINTF_IO_INSTRUCTION:
		{	
			if(deviceintf_data.dev_list[idx].type != DEVINTF_NONE)
			{
				for(jdx = 0; (deviceintf_data.dev_list[idx].io_list[jdx].type != DEVINTF_IO_NONE); jdx++) 
				{		
					if(deviceintf_data.dev_list[idx].io_list[jdx].trigger == TRUE)
					{
						deviceintf_data.dev_list[idx].io_list[jdx].trigger = FALSE;
						state->state = deviceintf_data.dev_list[idx].io_list[jdx].type;
						break;
					}
				}
				if(deviceintf_data.dev_list[idx].io_list[jdx].type == DEVINTF_IO_NONE)
				{
					jdx = 0;
					idx = 0;
					state->state = DEVINTF_IO_NONE;
					state->event = DEVINTF_IO_NONE; 							
				}					
			}
			else
			{
				idx = 0;
				jdx = 0;
				state->state = DEVINTF_IO_NONE;
				state->event = DEVINTF_IO_NONE; 		
			}			
		}
		break;

		case DEVINTF_IO_1:
		case DEVINTF_IO_2:
		case DEVINTF_IO_3:
		case DEVINTF_IO_4:
		case DEVINTF_IO_5:
		case DEVINTF_IO_6:
		case DEVINTF_IO_7:
		case DEVINTF_IO_8:
		case DEVINTF_IO_9:
		case DEVINTF_IO_10:
		case DEVINTF_IO_11:
		case DEVINTF_IO_12:
		case DEVINTF_IO_13:
		case DEVINTF_IO_14:
		case DEVINTF_IO_15:
		case DEVINTF_IO_16:
		case DEVINTF_IO_17:
		case DEVINTF_IO_18:
		case DEVINTF_IO_19:
		case DEVINTF_IO_20:
		case DEVINTF_IO_21:
		case DEVINTF_IO_22:
		case DEVINTF_IO_23:
		case DEVINTF_IO_24:
		case DEVINTF_IO_25:
		case DEVINTF_IO_26:
		case DEVINTF_IO_27:
		case DEVINTF_IO_28:
		case DEVINTF_IO_29:
		case DEVINTF_IO_30:
		case DEVINTF_IO_31:
		case DEVINTF_IO_32:
		case DEVINTF_IO_33:				
		case DEVINTF_IO_34:				
		case DEVINTF_IO_35:
			
			/* Place Led control Api here */
#if ((defined SMARTLIGHT_APP) && ((defined LED_RGB_LIGHT) || (defined LED_WNC_LIGHT) \
		|| (defined LED_WHITE_LIGHT) || (defined LED_SMART_LIGHT)))
			led_driver_control(state->state);			
#endif		
			/* Place Sensor control Api here */
#ifdef SENSOR_DRIVER			
			sensor_driver_control(state->state);
#endif
			/* Place Smartplug control Api here */
#ifdef SMARTPLUG_DRIVER			
			smartplug_driver_control(state->state);
#endif			
#ifdef SMARTGRID_APP			
			smartgridApp_io_control(state->state);
#endif	
			state->state = DEVINTF_IO_INSTRUCTION;

		break;
		
		default:
		break;	
	}

	state->event = DEVINTF_IO_NONE;
	state->eventtype = 0;
	state->eventclass = 0;
	state->eventproto = 0;
	state->statedata = NULL;	
	state->statedatalen = 0;	
	memset((u8*)&state->msg_hdr, 0x00, sizeof(gv701x_app_msg_hdr_t));	
}

/******************************************************************************
 * @fn      DeviceIntfApp_CmdProcess
 *
 * @brief   It handles application command line requests
 *
 * @param   CmdBuf - command string
 *
 * @return  none
 *
 */

void DeviceIntfApp_CmdProcess(char* CmdBuf) 
{
	if(strcmp(CmdBuf, "state") == 0) 
	{
		printf("\nDevintf S %bu E %bu ", deviceintf_state.state, deviceintf_state.event);
	}
	else if(strcmp(CmdBuf, "stats") == 0) 
	{
		u8 idx, jdx, devtype;		
		char* subcmd = NULL;
						
		subcmd = strtok(CmdBuf, " ");
		subcmd = strtok(NULL, "\0");

		if((subcmd != NULL) ? (sscanf(subcmd, "%bu", &devtype) >= 1) : (FALSE))
		{
			if(devtype == DEVINTF_NONE)
				return;
		}
		
		for(idx = 0; (deviceintf_data.dev_list[idx].type != DEVINTF_NONE); idx++)
		{
			if((subcmd != NULL) ? (deviceintf_data.dev_list[idx].type != devtype) : (FALSE))				
				continue;
			
			printf("\ndevtype[%bu] %bu ", idx, deviceintf_data.dev_list[idx].type);
			for (jdx = 0; (deviceintf_data.dev_list[idx].io_list[jdx].type != DEVINTF_IO_NONE); jdx++) 
			{
				printf("\n IO[%bu] %s type %bu len %bu trigger %bu", jdx,
						deviceintf_data.dev_list[idx].io_list[jdx].name,
						deviceintf_data.dev_list[idx].io_list[jdx].type,
						deviceintf_data.dev_list[idx].io_list[jdx].len,
						deviceintf_data.dev_list[idx].io_list[jdx].trigger);				
				FM_HexDump(FM_USER, "\nval: ", (u8*)deviceintf_data.dev_list[idx].io_list[jdx].p_val, 
						deviceintf_data.dev_list[idx].io_list[jdx].len);										 				
			}
		}		
	}
	else if(strcmp(CmdBuf, "nvclear") == 0) 
	{
		GV701x_FlashErase(deviceintf_data.app_id);
	}		
}	

#endif /*DEVICEINTF_APP*/

