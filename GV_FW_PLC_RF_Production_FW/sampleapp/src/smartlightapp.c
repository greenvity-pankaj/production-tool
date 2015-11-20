/* ========================================================
 *
 * @file:  smartlightapp.c
 * 
 * @brief: This file contains the led driver for the GV701x LED boards
 *         Its is a PWM driven or DC modulated Intensity dimming/Color change board.
 *         There is provision for 4 PWM channels or 4 Modulated-DC channel 
 *		   (use LED_PWM or LED_DC respectively) and 2 dedicated O/P pins for 
 *         control of Digital O/P device or relays(as in this example)
 *
 *		   This function incorporates more than one logical device type 
 *		   (LIGHT_RGB - RGB light, LIGHT_TEMP - Light temperature interpretation
 *          LIGHT_DIMM - single channel dimmer device, LED_SMART_LIGHT device)
 *
 *  Copyright (C) 2010-2015, Greenvity Communications, Inc.
 *  All Rights Reserved
 *  
 * =========================================================*/

#if ((defined SMARTLIGHT_APP) && ((defined LED_RGB_LIGHT) || (defined LED_WNC_LIGHT) \
	|| (defined LED_WHITE_LIGHT) || (defined LED_SMART_LIGHT)))

/****************************************************************************** 
  *	Includes
  ******************************************************************************/

#include <stdio.h>
#include <string.h>
#include "gv701x_includes.h"
#ifdef DEVICEINTF_APP
#include "deviceintfapp.h"
#endif
#include "smartlightapp.h"
#ifdef MOTION_DRIVER
#include "motion_driver.h"
#endif
#ifdef LLP_APP
#include "llpapp.h"
#endif

/****************************************************************************** 
  *	Global Data
  ******************************************************************************/

/*Led Database*/
led_drv_db_t led_drv_db;

/*Led Database*/
led_reg_map_t XDATA led_reg_map[LED_MAX_IO] = {
 	{0, CH0_CFG1_REG, CH0_CFG2_REG, CH0_CFG3_REG, CH0_OP1_REG, CH0_OP2_REG},
 	{1, CH1_CFG1_REG, CH1_CFG2_REG, CH1_CFG3_REG, CH1_OP1_REG, CH1_OP2_REG},		
 	{2, CH2_CFG1_REG, CH2_CFG2_REG, CH2_CFG3_REG, CH2_OP1_REG, CH2_OP2_REG}, 	
 	{3, CH3_CFG1_REG, CH3_CFG2_REG, CH3_CFG3_REG, CH3_OP1_REG, CH3_OP2_REG}, 	
 	{4, CH4_CFG1_REG, CH4_CFG2_REG, CH4_CFG3_REG, CH4_OP1_REG, CH4_OP2_REG}, 	
 	{5, CH5_CFG1_REG, CH5_CFG2_REG, CH5_CFG3_REG, CH5_OP1_REG, CH5_OP2_REG}, 	 	
};	

#ifdef LED_RGB_LIGHT
/*RGB I/O's*/
io_list_t led_rgb_io_list[] = 
{{"red", RED_CH1, 1, &led_drv_db.led_val.ch[0], FALSE}, 
 {"green", GREEN_CH2, 1, &led_drv_db.led_val.ch[1], FALSE}, 
 {"blue", BLUE_CH3, 1, &led_drv_db.led_val.ch[2], FALSE},
 {"none", DEVINTF_IO_NONE, 0, NULL, FALSE}
};
#endif

#ifdef LED_WNC_LIGHT
/*Light Temperature(Warm, Natural, Cool)  I/O's*/
io_list_t led_temp_io_list[] = 
{{"warm",  WARM_CH1, 1, &led_drv_db.led_val.ch[0], FALSE},	
 {"natural", NATURAL_CH2, 1, &led_drv_db.led_val.ch[1], FALSE}, 
 {"cool", COOL_CH3, 1, &led_drv_db.led_val.ch[2], FALSE},
 {"none", DEVINTF_IO_NONE, 0, NULL, FALSE}
};
#endif 

#ifdef LED_WHITE_LIGHT
/*Led Dimmer  I/O's*/
io_list_t led_dim_io_list[] = 
{{"dimm", DIMM_CH4, 1, &led_drv_db.led_val.ch[3], TRUE},
 {"none", DEVINTF_IO_NONE, 0, NULL, FALSE}
};
#endif 

#ifdef LED_SMART_LIGHT
io_list_t led_smartlight_io_list[] = 
{{"onoff", ENABLE_CH, 1, &led_drv_db.led_val.enable, FALSE}, 
 {"relaych1", RELAY_1, 1, &led_drv_db.led_val.ch[4], FALSE},
 {"relaych2", RELAY_2, 1, &led_drv_db.led_val.ch[5], FALSE}, 
 {"red", RED_CH1, 1, &led_drv_db.led_val.ch[0], FALSE}, 
 {"green", GREEN_CH2, 1, &led_drv_db.led_val.ch[1], FALSE}, 
 {"blue", BLUE_CH3, 1, &led_drv_db.led_val.ch[2], FALSE},
 {"white", WHITE_CH4, 1, &led_drv_db.led_val.ch[3], FALSE}, 
#ifdef MOTION_DRIVER 
 {"sensor", MOTION_CH, 1, &motion_driver_db.motion.detected, FALSE},
#endif 
 {"none", DEVINTF_IO_NONE, 0, NULL, FALSE}
};
#endif

/****************************************************************************** 
  *	Function Prototypes
  ******************************************************************************/
void led_driver_cfg(void) ;
void led_driver_write(u8 led_tlv, u8 led_val);
void led_driver_cmdprocess(char* CmdBuf);

/******************************************************************************
 * @fn      led_driver_init
 *
 * @brief   Initializes the Led driver
 *
 * @param   app_id - application identification number
 *
 * @return  none
 */

void led_driver_init(u8 app_id)
{	
	u8 idx, jdx;
	device_inst_msg_t device_inst;
	
	memset(&led_drv_db, 0x00, sizeof(led_ioval_t));		
	led_drv_db.appid = app_id;
    SLIST_Init(&led_drv_db.queues.appRxQueue);

	FM_Printf(FM_USER, "\nInit SmartLightApp (app id %bu)", led_drv_db.appid);

	//FM_SetDebugLevel(FM_MASK_DEFAULT | FM_APP);
	led_driver_cfg();

#ifdef DEVICEINTF_APP
	device_inst.event = DEVINTF_IO_INSTRUCTION; 				
#ifdef LED_RGB_LIGHT	
	device_inst.dev_type = LIGHT_RGB;						
#endif
#ifdef LED_WNC_LIGHT	
	device_inst.dev_type = LIGHT_TEMP;						
#endif
#ifdef LED_WHITE_LIGHT	
	device_inst.dev_type = LIGHT_DIMM;						
#endif
#ifdef LED_SMART_LIGHT	
	device_inst.dev_type = LIGHT_SMART; 					
#endif
#endif	

	/*Turning the lights to defaults(ON currently)*/
	for(idx = 0; (dev_list[idx].type != DEVINTF_NONE); idx++)
	{
		if(dev_list[idx].type != DEVINTF_NONE)
		{
			dev_list[idx].subtype = led_drv_db.nv.dev_subtype;
		
			for(jdx = 0; (dev_list[idx].io_list[jdx].type != DEVINTF_IO_NONE); jdx++)
			{
				dev_list[idx].io_list[jdx].trigger = TRUE;	
				memset((u8*)dev_list[idx].io_list[jdx].p_val, 0xFF, 
						dev_list[idx].io_list[jdx].len);
				
#ifdef LED_SMART_LIGHT		
				if((dev_list[idx].io_list[jdx].type == ENABLE_CH) || 
					(dev_list[idx].io_list[jdx].type == RELAY_1) || 
					(dev_list[idx].io_list[jdx].type == RELAY_2) ||
					(dev_list[idx].io_list[jdx].type == MOTION_CH))
				{			
					if(dev_list[idx].io_list[jdx].type == MOTION_CH)
					{
						*((u8*)dev_list[idx].io_list[jdx].p_val) = FALSE;
						dev_list[idx].io_list[jdx].trigger = FALSE;
					}				
					if((dev_list[idx].io_list[jdx].type == RELAY_1) || 
					   (dev_list[idx].io_list[jdx].type == RELAY_2))
					{				
						dev_list[idx].io_list[jdx].trigger = TRUE;
						*((u8*)dev_list[idx].io_list[jdx].p_val) = 0x01;
					}				
				}
#endif			
			}		
			break;
		}
	}
	
#ifdef DEVICEINTF_APP		
	GV701x_SendAppEvent(led_drv_db.appid, deviceintf_data.app_id, APP_MSG_TYPE_APPEVENT,
				APP_MAC_ID, EVENT_CLASS_CTRL, MGMT_FRM_ID,			
				&device_inst, sizeof(device_inst_msg_t), 0);	
#endif	
}

/******************************************************************************
 * @fn      led_driver_cfg
 *
 * @brief   Configures the Led driver based on values stored in flash
 *
 * @param   none
 *
 * @return  none
 */

void led_driver_cfg(void)
{
    u8 idx;

	GV701x_FlashRead(led_drv_db.appid, (u8*)&led_drv_db.nv, sizeof(led_drv_db.nv));	
	
	WriteU8Reg(0x4FA, 0x01);
	WriteU8Reg(0x45C, 0x0);
	WriteU8Reg(0x45D, 0x0);
	WriteU8Reg(0x45E, 0x0);
	WriteU8Reg(0x45F, 0x0);
	WriteU8Reg(0x460, 0x0);
	WriteU8Reg(0x461, 0x0);
	WriteU8Reg(0x462, 0x0);
	WriteU8Reg(0x463, 0x0);
	WriteU8Reg(0x464, 0x0);
	WriteU8Reg(0x465, 0x0);
	WriteU8Reg(0x466, 0x0);
	WriteU8Reg(0x467, 0x0);
	WriteU8Reg(0x468, 0x0);
	WriteU8Reg(0x469, 0x0);
	WriteU8Reg(0x46A, 0x0);
	WriteU8Reg(0x46B, 0x0);
	WriteU8Reg(0x45A, 0xEC);
	WriteU8Reg(0x45B, 0x02);
	WriteU8Reg(0x458, 0x11);
	WriteU8Reg(0x459, 0x11);
	WriteU8Reg(0x4FA, 0x0);
	WriteU8Reg(0x4FA, 0x01);


	FM_Printf(FM_USER, "\nSubtype: %bu", led_drv_db.nv.dev_subtype);

	for(idx = 0; idx < LED_MAX_IO; idx++)
	{
		switch(led_drv_db.nv.io_cfg.io[idx])
		{
			case LED_PWM:
				FM_Printf(FM_USER, " Ch%bu: %s", idx, "PWM");
				WriteU8Reg(led_reg_map[idx].cfg1_reg, 0x01);
				WriteU8Reg(led_reg_map[idx].cfg2_reg, 0x00);
			break;
			
			case LED_DC:
				FM_Printf(FM_USER, " Ch%bu: %s", idx, "DC");
				WriteU8Reg(led_reg_map[idx].cfg1_reg, 0x08);
			break;

			case LED_GPIO:
				FM_Printf(FM_USER, " Ch%bu: %s", idx, "GPIO");
				if((idx == LED_CH4) || (idx == LED_CH5))
				{
#ifndef HQ_LINK_TEST
					WriteU8Reg(led_reg_map[idx].cfg1_reg, 
							   (ReadU8Reg(led_reg_map[idx].cfg1_reg) & 
								led_reg_map[idx].op2_reg));
#endif
				}
				else
				{
					WriteU8Reg(led_reg_map[idx].cfg1_reg, 0xED);
					WriteU8Reg(led_reg_map[idx].cfg2_reg, 0x02);				
				}
			break;			
			
			default:				
			break;
		}
	}
	FM_Printf(FM_USER, "\n");		  
	WriteU8Reg(0x4FA, 0x0);	
}

/******************************************************************************
 * @fn      led_driver_rxappmsg
 *
 * @brief   Receives a message from another app/fw
 *
 * @params  event - message buffer
 *
 * @return  none
 */

void led_driver_rxappmsg(sEvent* event)
{
	gv701x_app_msg_hdr_t* msg_hdr = (gv701x_app_msg_hdr_t*)event->buffDesc.dataptr;
	hostHdr_t* hybrii_hdr;
	hostEventHdr_t* evnt_hdr;

	hybrii_hdr = (hostHdr_t*)(msg_hdr + 1);

	if(msg_hdr->dst_app_id == led_drv_db.appid)
	{
		if((msg_hdr->src_app_id == APP_FW_MSG_APPID) && 
			(hybrii_hdr->type == EVENT_FRM_ID))			
		{			
			evnt_hdr = (hostEventHdr_t*)(hybrii_hdr + 1);
			
			if(evnt_hdr->type == HOST_EVENT_APP_TIMER)
			{
				led_driver_timerhandler((u8*)(evnt_hdr + 1)); 
				return;
			}
			else if(evnt_hdr->type == HOST_EVENT_APP_CMD)
			{
				led_driver_cmdprocess((char*)(evnt_hdr + 1)); 
				return;
			}			
		}				
	}
	else if(msg_hdr->dst_app_id == APP_BRDCST_MSG_APPID)
	{
		u8 *event = (u8*)(hybrii_hdr + 1);
		return;
	}	
}

/******************************************************************************
 * @fn		led_driver_timerhandler
 *
 * @brief	Timer handler for Led timer events
 *
 * @param	event - event from firmware
 *
 * @return	none
 *
 */ 
void led_driver_timerhandler(u8* buf)
{		
	hostTimerEvnt_t* timerevt = (hostTimerEvnt_t*)buf;			

	if(buf == NULL)
		return; 
		
	/*Demultiplexing the specific timer event*/ 					
	switch((u8)timerevt->type)
	{								
		
		default:
		break;
	}
}
	

/******************************************************************************
 * @fn      led_driver_control
 *
 * @brief   Parses the Tlv's(exchanged over LLP) to interepret the channel
 *			over which action is being requested
 *
 * @param   led_tlv - the type of TLV
 *			
 * @return  none
 */

void led_driver_control(u8 led_tlv)
{
#ifdef LED_RGB_LIGHT
	/*RGB device*/
	switch(led_tlv)
	{	
		case RED_CH1:			
			FM_Printf(FM_APP, "\n| R %bu | ",led_drv_db.led_val.ch[LED_CH0]);
			led_driver_write(LED_CH0, led_drv_db.led_val.ch[LED_CH0]);
		break;			

		case GREEN_CH2:							
			FM_Printf(FM_APP, "\n| G %bu | ",led_drv_db.led_val.ch[LED_CH1]);
			led_driver_write(LED_CH1, led_drv_db.led_val.ch[LED_CH1]);
		break;			

		case BLUE_CH3: 		
			FM_Printf(FM_APP, "\n| B %bu | ",led_drv_db.led_val.ch[LED_CH2]);
			led_driver_write(LED_CH2, led_drv_db.led_val.ch[LED_CH2]);
		break;			

		default:
		break;
	}
#endif /*LED_RGB_LIGHT*/	

#ifdef LED_WNC_LIGHT
	/*Light temperature device*/
	switch(led_tlv)
	{
		case WARM_CH1:			
			FM_Printf(FM_APP, "\n| W %bu | ",led_drv_db.led_val.ch[LED_CH0]);
			led_driver_write(LED_CH0, led_drv_db.led_val.ch[LED_CH0]);
		break;			
		
		case NATURAL_CH2: 		
			FM_Printf(FM_APP, "\n| N %bu | ",led_drv_db.led_val.ch[LED_CH1]);
			led_driver_write(LED_CH1, led_drv_db.led_val.ch[LED_CH1]);
		break;			
		
		case COOL_CH3:		
			FM_Printf(FM_APP, "\n| C %bu | ",led_drv_db.led_val.ch[LED_CH2]);
			led_driver_write(LED_CH2, led_drv_db.led_val.ch[LED_CH2]);
		break;			

		default:
		break;
	}
#endif /*LED_WNC_LIGHT*/		

#ifdef LED_WHITE_LIGHT
	/*Single channel dimming device*/
	switch(led_tlv)
	{
		case DIMM_CH4:			
			FM_Printf(FM_APP, "\n| D %bu | ", led_drv_db.led_val.ch[LED_CH0]);
			led_driver_write(LED_CH0, led_drv_db.led_val.ch[LED_CH0]);
		break;			

		default:
		break;
	}
#endif /*LED_WHITE_LIGHT*/	

#ifdef LED_SMART_LIGHT
	switch(led_tlv)
	{		
		case ENABLE_CH:		
			FM_Printf(FM_APP, "\n| ENABLE_CH %bu | ", led_drv_db.led_val.enable);
		break;				
		
		case RED_CH1:			
			FM_Printf(FM_APP, "\n| R %bu | ", led_drv_db.led_val.ch[LED_CH0]);
			led_driver_write(LED_CH0, led_drv_db.led_val.ch[LED_CH0]);
		break;			

		case GREEN_CH2:			
			FM_Printf(FM_APP, "\n| G %bu | ",led_drv_db.led_val.ch[LED_CH1]);
			led_driver_write(LED_CH1, led_drv_db.led_val.ch[LED_CH1]);
		break;			

		case BLUE_CH3: 		
			FM_Printf(FM_APP, "\n| B %bu | ",led_drv_db.led_val.ch[LED_CH2]);
			led_driver_write(LED_CH2, led_drv_db.led_val.ch[LED_CH2]);
		break;			

		case WHITE_CH4: 		
			FM_Printf(FM_APP, "\n| W %bu | ",led_drv_db.led_val.ch[LED_CH3]);
			led_driver_write(LED_CH3, led_drv_db.led_val.ch[LED_CH3]);
		break;			

#ifndef HQ_LINK_TEST		
		case RELAY_1:		
			FM_Printf(FM_APP, "\n| Relay1 %bu | ", led_drv_db.led_val.ch[LED_CH4]);
			led_driver_write(LED_CH4, led_drv_db.led_val.ch[LED_CH4]);
		break;			

		case RELAY_2:		
			FM_Printf(FM_APP, "\n| Relay2 %bu | ", led_drv_db.led_val.ch[LED_CH5]);
			led_driver_write(LED_CH5, led_drv_db.led_val.ch[LED_CH5]);
		break;	
#endif
		default:
		break;
	}
#endif /*LED_SMART_LIGHT*/	
}

/******************************************************************************
 * @fn      led_driver_write
 *
 * @brief   Maps the dimming values(echanged over LLP) to the h/w values
 *
 * @param   led_ch - the channel on which values are to be written
 * 			led_val - the value to be written 
 *			dev_type - the device (device type connected to LLP) 
 *
 * @return  none
 */

void led_driver_write(u8 led_ch, u8 led_val)
{		
	u8 x;
	u8 y; 
	u16 hw_value = 0;					

	WriteU8Reg(0x4FA, 0x01);				
    if(led_drv_db.nv.io_cfg.io[led_ch] == LED_GPIO)
    {  
	    hw_value = led_val;
	    x = hw_value & 0x00FF;
	    y = (hw_value >> 8) & 0x00FF;     
		if (x > 0)
		{
			if((led_ch == LED_CH4) || (led_ch == LED_CH5))
			{			    
				WriteU8Reg(led_reg_map[led_ch].cfg3_reg,	
						  (ReadU8Reg(led_reg_map[led_ch].cfg3_reg) | 
						   led_reg_map[led_ch].op1_reg));					
			}
			else
			{
				WriteU8Reg(led_reg_map[led_ch].cfg3_reg, 
						  (ReadU8Reg(led_reg_map[led_ch].cfg3_reg) & 
						  			led_reg_map[led_ch].op1_reg));
			}
		}
		else
		{
			if((led_ch == LED_CH4) || (led_ch == LED_CH5))
			{
				WriteU8Reg(led_reg_map[led_ch].cfg3_reg, 	
					      (ReadU8Reg(led_reg_map[led_ch].cfg3_reg) &
					       led_reg_map[led_ch].op2_reg));		
			}
			else
			{
				WriteU8Reg(led_reg_map[led_ch].cfg3_reg, 	
					      (ReadU8Reg(led_reg_map[led_ch].cfg3_reg) | 
					       led_reg_map[led_ch].op2_reg));		
			}
		}
    }
    else if(led_drv_db.nv.io_cfg.io[led_ch] == LED_PWM)
    {
    	if((led_val == 0) || (led_val == 1))
    	{
    		hw_value = 0x02FF;
    	}
		else
		{
	        hw_value = (255 - led_val)*3;
		}
		
		if(hw_value == 0)
			hw_value = 1;
			
		x = hw_value & 0x00FF;
		y = (hw_value >> 8) & 0x00FF;				

		WriteU8Reg(led_reg_map[led_ch].cfg1_reg, x);
		WriteU8Reg(led_reg_map[led_ch].cfg2_reg, y);		
    }
    else if(led_drv_db.nv.io_cfg.io[led_ch] == LED_DC)
    {
    	if((led_val == 0) || (led_val == 1))
    	{
    		hw_value = 0x02FF;
    	}
		else
		{
#if 0    
			/* Mapping light level 1 ~ 255 to 40 ~ 8 */    
			hw_value =	64 - (led_val*2 /9);
#else
			/* Mapping light level 1 ~ 255 to 96 ~11 
			  (0x60 to 0xB)register value */
			hw_value =	96 - (led_val/3);			  
#endif
			
		}   
		
		if(hw_value == 0)
			hw_value = 1;
		
	    x = hw_value & 0x00FF;
	    y = (hw_value >> 8) & 0x00FF; 		
		WriteU8Reg(led_reg_map[led_ch].cfg1_reg, x);
		WriteU8Reg(led_reg_map[led_ch].cfg2_reg, y);
	}

	WriteU8Reg(0x4FA, 0x0);
#if 0	
	FM_Printf(FM_APP, "\nProgram CH = %bu, value = %u, x = %bu, y = %bu\n", led_ch, hw_value, x, y);		
#endif
}

/******************************************************************************
 * @fn      led_driver_cmdprocess
 *
 * @brief   It handles application command line requests
 *
 * @param   CmdBuf - command string
 *
 * @return  none
 *
 */

void led_driver_cmdprocess(char* CmdBuf) 
{
	u8 cmd[30];
	
	if((sscanf(CmdBuf, "%s", &cmd) < 1) || strcmp(cmd, "?") == 0)
		return;
		
	if(strcmp(cmd, "cfg") == 0) 
	{	
		if(sscanf(CmdBuf + sizeof("cfg"),"%bu %bu %bu %bu %bu %bu %bu",
		   &led_drv_db.nv.dev_subtype, &led_drv_db.nv.io_cfg.io[0],
		   &led_drv_db.nv.io_cfg.io[1], &led_drv_db.nv.io_cfg.io[2],
		   &led_drv_db.nv.io_cfg.io[3], &led_drv_db.nv.io_cfg.io[4],
		   &led_drv_db.nv.io_cfg.io[5]) >= 1)
		{			   
			GV701x_FlashWrite(led_drv_db.appid, (u8*)&led_drv_db.nv, sizeof(led_drv_db.nv));			
		}
	}
	else if(strcmp(cmd, "test") == 0) 
	{
		if(sscanf(CmdBuf + sizeof("test"), "%bu %bu %bu %bu %bu %bu", 
			&led_drv_db.led_val.ch[0], &led_drv_db.led_val.ch[1],
			&led_drv_db.led_val.ch[2], &led_drv_db.led_val.ch[3],
			&led_drv_db.led_val.ch[4], &led_drv_db.led_val.ch[5]) >= 1)
		{				
#ifdef LED_SMART_LIGHT		
			led_driver_control(RED_CH1);
			led_driver_control(GREEN_CH2); 			
			led_driver_control(BLUE_CH3);								
			led_driver_control(WHITE_CH4); 			
			led_driver_control(RELAY_1); 			
			led_driver_control(RELAY_2); 						
#endif			
		}	
	}
	else if(strcmp(cmd, "nvclear") == 0) 
	{
		GV701x_FlashErase(led_drv_db.appid);
	}	
}

#endif /*((SMARTLIGHT_APP) && (LED_RGB_LIGHT || LED_WNC_LIGHT \
	    *LED_WHITE_LIGHT || LED_SMART_LIGHT))*/
