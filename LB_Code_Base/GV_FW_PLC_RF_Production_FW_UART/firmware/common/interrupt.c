/*
* $Id: interrupt.c,v 1.2 2014/11/11 14:52:56 ranjan Exp $
*
* $Source: /home/cvsrepo/Hybrii_B_OSFW_Dev/firmware/common/interrupt.c,v $
* 
* Description  : Common Interrupt routine module.
* 
* Copyright (c) 2010-2013 Greenvity Communications, Inc.
* All rights reserved.
*
* Purpose      :
*     The files serves the purpose of placing all system 
*     interrupt routines in a consolidate place so that 
*     they can be placed in the common bank.
*     Additionally implements Ext Int 0 ISR and Timer1 ISR.
*
*/
#include "papdef.h"
#include "timer.h"
#include "list.h"
#include "stm.h"
#include "hal_common.h"

extern volatile sStm Stm;
extern void CHAL_IncTimerIntCnt();
extern u32 gtimer2, gtimer1;
extern sHalCB     gHalCB;    // Common  HAL Control Block  

#ifndef UART_HOST_INTF
#ifndef TIMER_POLL
void timer_handler (void) using 2 //__INTERRUPT1__   //[YM] For Hybrii_B ASIC SPI TEST
{
#if 1
	Stm.timeTick++;
#else
    //Increment the software time tick
    if(Stm.timeTick == STM_TIME_TICK_MAX)
    {
        Stm.timeSector = !Stm.timeSector;
        Stm.timeTick = 0;
        
    }
    else
    {
        Stm.timeTick++;
    }
    //CHAL_IncTimerIntCnt();
#ifndef RTX51_TINY_OS
    /* reload timer register */
    TH0 = HYBRII_MSTIMER25MHZ_HI;
    TL0 = HYBRII_MSTIMER25MHZ_LO;
#endif
		

#endif
#ifdef HPGP_HAL_TEST
		 //CHAL_IncTimerIntCnt();
		gHalCB.timerIntCnt++;
		gtimer2++;
		gtimer1++;
#endif
}
#endif
#endif
