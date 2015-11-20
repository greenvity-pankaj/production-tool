/* ========================================================
 *
 * @file: rgbfading.c
 * 
 * @brief: This file supports a single fading pattern on a 
 *         single channel
 *      
 *  Copyright (C) 2010-2014, Greenvity Communications, Inc.
 *  All Rights Reserved
 *  
 * =========================================================*/

#ifdef RGB_FADING_DEMO
#include "stdio.h"
#include "string.h"
#include "gv701x_includes.h"
#include "smartlightapp.h"
#include "deviceintfapp.h"
#include "rgbfading.h"

rgbfading_data_t rgbfading_data;
#ifdef DEVICEINTF_APP
extern deviceintf_data_t  deviceintf_data;
extern gv701x_state_t deviceintf_state;
#endif

/*
	
*/
void rgbfading_init(u8 app_id)
{
	FM_Printf(FM_USER, "\nRgb Fading Demo Init..(tc %bu)", app_id);
	memset(&rgbfading_data, 0x00, sizeof(rgbfading_data_t));
	rgbfading_data.app_id = app_id;

#ifdef FADING_THREE_CH
    rgbfading_data.inst[0] = UNFADE_RED;	
    rgbfading_data.inst[1] = UNFADE_GREEN;		
    rgbfading_data.inst[2] = FADE_RED;			
    rgbfading_data.inst[3] = UNFADE_BLUE;		
    rgbfading_data.inst[4] = FADE_GREEN;			
    rgbfading_data.inst[5] = UNFADE_RED;				
    rgbfading_data.inst[6] = UNFADE_GREEN;	
	rgbfading_data.inst_cnt = 7;
#else
	rgbfading_data.inst[0] = UNFADE_RED;	
	rgbfading_data.inst[1] = FADE_RED;			
	rgbfading_data.inst_cnt = 2;
#endif
	rgbfading_data.poll_timer = STM_AllocTimer(SW_LAYER_TYPE_APP, 		
						POLL_TIMER_EVNT,&rgbfading_data.app_id);		
	STM_StartTimer(rgbfading_data.poll_timer, POLL_PERIOD);
}

void rgbfading_timerhandler(u8* buf)
{	
	hostTimerEvnt_t* timerevt = 
		(hostTimerEvnt_t*)buf;			

	if(event.eventdata)
		return;
	
	if(timerevt->app_id != rgbfading_data.app_id)
		return;

	/*Demultiplexing the specific timer event*/ 					
	switch((u8)timerevt->type)
	{								
		case POLL_TIMER_EVNT:
			rgbfading_poll();
			STM_StartTimer(rgbfading_data.poll_timer, POLL_PERIOD);
		break;	
		
		default:
		break;
	}							
}

void rgbfading_poll(void)
{
#ifdef DEVICEINTF_APP	
  char offset;
	u8 idx, jdx;
	static u8 red_active = FALSE, green_active = FALSE, blue_active = FALSE;
	u16 current_inst;
	static u8 red_fade_value = 0, green_fade_value = 0, blue_fade_value = 0;
	static u8 fade_inst = 0;	
	//u8 max_inst = 8*(sizeof(rgbfading_data.inst));
		
	for(idx = 0; (deviceintf_data.dev_list[idx].type != DEVINTF_NONE); idx++)
	{
		if(deviceintf_data.dev_list[idx].type == LIGHT_RGB)
		{
			break;
		}
	}
	
	for (jdx = 0; (deviceintf_data.dev_list[idx].io_list[jdx].type != DEVINTF_IO_NONE); jdx++) 
	{				
		if(rgbfading_data.inst_cnt == 0)
		{
			red_active = FALSE;	
			green_active = FALSE;	
			blue_active = FALSE;
			fade_inst = 0;
#ifdef FADING_THREE_CH
			rgbfading_data.inst[0] = UNFADE_RED;	
			rgbfading_data.inst[1] = UNFADE_GREEN;		
			rgbfading_data.inst[2] = FADE_RED;			
			rgbfading_data.inst[3] = UNFADE_BLUE;		
			rgbfading_data.inst[4] = FADE_GREEN;			
			rgbfading_data.inst[5] = UNFADE_RED;				
			rgbfading_data.inst[6] = UNFADE_GREEN;	
			rgbfading_data.inst_cnt = 7;
#else
			rgbfading_data.inst[0] = UNFADE_RED;	
			rgbfading_data.inst[1] = FADE_RED;			
			rgbfading_data.inst_cnt = 2;
#endif
		}
		
		if((rgbfading_data.inst_cnt != 0) && (rgbfading_data.inst[fade_inst] !=0))
		{			
			switch(rgbfading_data.inst[fade_inst])
			{
				case UNFADE_RED:
					//FM_Printf(FM_APP, "\nred+ %bu", red_fade_value);
					red_active = TRUE;		
					offset = 1;
					if(red_fade_value == 254)
					{
						rgbfading_data.inst[fade_inst] = 0;
						rgbfading_data.inst_cnt--;
						red_active = FALSE;	
						fade_inst++;
						//FM_Printf(FM_APP, "\nfade_inst %bu %u ", fade_inst, (rgbfading_data.inst >> fade_inst));						
					}
				break;

				case FADE_RED:
					//FM_Printf(FM_APP, "\nred- %bu", red_fade_value);
					red_active = TRUE;		
					offset = -1;
					if(red_fade_value == 0)
					{
						rgbfading_data.inst[fade_inst] = 0;
						rgbfading_data.inst_cnt--;
						red_active = FALSE;
						fade_inst++;
						//FM_Printf(FM_APP, "\nfade_inst %bu %u", fade_inst, (rgbfading_data.inst >> fade_inst));												
					}
				break;							

				case UNFADE_GREEN:
					//FM_Printf(FM_APP, "\ngreen+ %bu", green_fade_value);
					green_active = TRUE;		
					offset = 1;
					if(green_fade_value == 254)
					{
						rgbfading_data.inst[fade_inst] = 0;
						rgbfading_data.inst_cnt--;
						green_active = FALSE;
						fade_inst++;
						//FM_Printf(FM_APP, "\nfade_inst %bu %u", fade_inst, (rgbfading_data.inst >> fade_inst));						
					}
				break;

				case FADE_GREEN:
					//FM_Printf(FM_APP, "\ngreen- %bu", green_fade_value);
					green_active = TRUE;		
					offset = -1;
					if(green_fade_value == 0)
					{
						rgbfading_data.inst[fade_inst] = 0;
						rgbfading_data.inst_cnt--;
						green_active = FALSE;
						fade_inst++;
						//FM_Printf(FM_APP, "\nfade_inst %bu %u", fade_inst, (rgbfading_data.inst >> fade_inst));						
					}
				break;							

				case UNFADE_BLUE:
					//FM_Printf(FM_APP, "\nblue+ %bu", blue_fade_value);
					blue_active = TRUE;		
					offset = 1;
					if(blue_fade_value == 254)
					{
						rgbfading_data.inst[fade_inst] = 0;
						rgbfading_data.inst_cnt--;
						blue_active = FALSE;
						fade_inst++;
						//FM_Printf(FM_APP, "\nfade_inst %bu %u", fade_inst, (rgbfading_data.inst >> fade_inst));						
					}
				break;

				case FADE_BLUE:
					//FM_Printf(FM_APP, "\nblue- %bu", blue_fade_value);
					blue_active = TRUE;		
					offset = -1;
					if(blue_fade_value == 0)
					{
						rgbfading_data.inst[fade_inst] = 0;
						rgbfading_data.inst_cnt--;
						blue_active = FALSE;
						fade_inst++;
						//FM_Printf(FM_APP, "\nfade_inst %bu %u", fade_inst, (rgbfading_data.inst >> fade_inst));						
					}
				break;		
			
				default:
					red_active = FALSE;	
					green_active = FALSE;	
					blue_active = FALSE;	
				break;
			}			
		}

			
		if((RED_CH1 == deviceintf_data.dev_list[idx].io_list[jdx].type) && (red_active == TRUE))
		{			
			memcpy_cpu_to_le((u8*)deviceintf_data.dev_list[idx].io_list[jdx].p_val, 
							(u8*)&red_fade_value, 
							 deviceintf_data.dev_list[idx].io_list[jdx].len);				
			red_fade_value += offset;
			//FM_HexDump(FM_APP,"\nRed Value: ",(u8*)deviceintf_data.dev_list[idx].io_list[jdx].p_val,
			//		   deviceintf_data.dev_list[idx].io_list[jdx].len);
			deviceintf_data.dev_list[idx].io_list[jdx].trigger = TRUE;	
			deviceintf_state.event = DEVINTF_IO_INSTRUCTION;
			break;			
		}

		if((GREEN_CH2 == deviceintf_data.dev_list[idx].io_list[jdx].type) && (green_active == TRUE))
		{			
			memcpy_cpu_to_le((u8*)deviceintf_data.dev_list[idx].io_list[jdx].p_val, 
							(u8*)&green_fade_value, 
							 deviceintf_data.dev_list[idx].io_list[jdx].len);				
			green_fade_value += offset;
			//FM_HexDump(FM_APP,"\nGreen Value: ",(u8*)deviceintf_data.dev_list[idx].io_list[jdx].p_val,
			//		   deviceintf_data.dev_list[idx].io_list[jdx].len);
			deviceintf_data.dev_list[idx].io_list[jdx].trigger = TRUE;		
			deviceintf_state.event = DEVINTF_IO_INSTRUCTION;
			break;
		}

		if((BLUE_CH3 == deviceintf_data.dev_list[idx].io_list[jdx].type) && (blue_active == TRUE))
		{			
			memcpy_cpu_to_le((u8*)deviceintf_data.dev_list[idx].io_list[jdx].p_val, 
							(u8*)&blue_fade_value, 
							 deviceintf_data.dev_list[idx].io_list[jdx].len);				
			blue_fade_value += offset;
			//FM_HexDump(FM_APP,"\nBlue Value: ",(u8*)deviceintf_data.dev_list[idx].io_list[jdx].p_val,
			//		   deviceintf_data.dev_list[idx].io_list[jdx].len);
			deviceintf_data.dev_list[idx].io_list[jdx].trigger = TRUE;	
			deviceintf_state.event = DEVINTF_IO_INSTRUCTION;
			break;			
		}									
	}			
#endif //DEVICEINTF_APP		
}
#endif //RGB_FADING_DEMO
