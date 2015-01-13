/** ========================================================
 *
 *  @file timer.h
 * 
 *  @brief This file prodives defines for timer usage
 *
 *  Copyright (C) 2010-2014, Greenvity Communications, Inc.
 *  All Rights Reserved
 *  
 * =========================================================*/

#ifndef TIMER_H
#define TIMER_H

/*****************************************************************************  
  *	Defines
  *****************************************************************************/
typedef u32 tTime;
typedef u8 tTimerId;


/*****************************************************************************  
  *	Function Prototypes
  *****************************************************************************/
tTimerId STM_AllocTimer(u8 mid, u16 type, void *cookie);
eStatus STM_StartTimer(tTimerId tid, tTime interval);
eStatus STM_StopTimer(tTimerId tid);
eStatus STM_FreeTimer(tTimerId tid);

#endif //TIMER_H

