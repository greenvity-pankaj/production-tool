/* ========================================================
 *
 * @file:  rgb_fading.h
 * 
 * @brief: This file contains all defines and profile needed 
 *         by the fading sequence alogrithm
 * 
 *  Copyright (C) 2010-2015, Greenvity Communications, Inc.
 *  All Rights Reserved
 *  
 * =========================================================*/

#ifdef RGB_FADING_DEMO

#ifndef RGBFADING_H
#define RGBFADING_H

/****************************************************************************** 
  *	Defines
  ******************************************************************************/

#define NUM_OF_FADE_CHANNELS					(1)
#if (NUM_OF_FADE_CHANNELS == 1)
#define FADE_CHANNEL_NUM						(DIMM_CH4)
#endif

#define POLL_PERIOD								(25)

typedef enum 
{
	UNFADE_RED = 1,
	FADE_RED,	
	UNFADE_GREEN,
	FADE_GREEN,	
	UNFADE_BLUE,
	FADE_BLUE,
	LAST_INST
}fade_instructions_t;

typedef struct 
{
	u8 app_id;
	tTimerId poll_timer;
	u8 inst_cnt;
	u16 inst[250];
}__PACKED__ rgbfading_data_t;

typedef enum 
{
	POLL_TIMER_EVNT,		
}rgbfading_timer_event_t;

/****************************************************************************** 
  *	Function Prototypes
  ******************************************************************************/

void rgbfading_init(u8 app_id);
void rgbfading_timerhandler(u8* buf);
void rgbfading_poll(void);

#endif /*RGBFADING_H*/
#endif /*RGB_FADING_DEMO*/
