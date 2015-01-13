/** ==========================================================
 *
 * @file stm.c
 * 
 *  @brief Software Timer Manager
 *
 *  Copyright (C) 2010-2011, Greenvity Communications, Inc.
 *  All Rights Reserved
 *  
 * ============================================================ */

#ifdef RTX51_TINY_OS
#include <rtx51tny.h>
#include "hybrii_tasks.h"
#endif
#include <string.h>
#include "papdef.h"
#include "list.h"
#include "timer.h"
#include "stm.h"
#ifdef ROUTE
#include "hpgp_route.h"
#endif
#include "hal_common.h"

#include "papdef.h"
#include "fm.h"
#ifdef P8051
#include <reg51.h>
#endif

#ifndef CALLBACK
 #ifdef HYBRII_HPGP
 #include "hpgpapi.h"
 #endif
#endif

#include "frametask.h"
#ifdef UM
#include "mac_intf_common.h"
#endif

/* TODO: convert duration from ms to the time tick
 * based on the time tick frequency.
 * At present, it is assumed that one time tick is 1 ms 
 */
#ifdef SIMU
#define STM_MS_TO_TICK(x)      (x)
#else
#ifdef B_ASICPLC
#define STM_MS_TO_TICK(x)      (x/10)
#else
#define STM_MS_TO_TICK(x)      (x/25)
#endif
#endif
//#ifdef B_ASICPLC
//#define STM_TIME_TICK_INIT    0//0xFFFFF000
//#else
//#define STM_TIME_TICK_INIT    0xFFFFF000  //0x00
//#endif
//#define STM_TIME_TICK_MAX     0xFFFFFFFF

volatile sStm Stm;
void STM_StartHwTimer()
{
#ifdef P8051
#ifndef RTX51_TINY_OS
   /* Start 1 ms timer running Timer0 in Mode */
   TMOD = (TMOD & 0xF0) | 0x01;
   
   /* Load Timer register */
   TH0 = HYBRII_MSTIMER25MHZ_HI;
   TL0 = HYBRII_MSTIMER25MHZ_LO;
   
   /* Start Timer */
   TR0 = 1;
   /* Enable Timer0 Int */
#ifdef UART_HOST_INTF
   ET0 = 0; 
#else  
   ET0 = 1;    
#endif                
#endif
#endif
}


u32 STM_GetTick()
{
    return Stm.timeTick;
}

eStatus STM_Init(void)
{
    u8 i;
    eStatus status = STATUS_SUCCESS;
    memset(&Stm, 0, sizeof(sStm));
    Stm.timeTick = STM_TIME_TICK_INIT;

    for (i = 0; i < STM_TIMER_MAX; i++)
    {
        Stm.timerCb[i].tid = i;
    }

    DLIST_Init(&(Stm.timerQueue));
#ifdef SIMU
#if defined(WIN32) || defined(_WIN32)
    Stm.stmSem = CreateSemaphore( 
        NULL,           // default security attributes
        SEM_COUNT,      // initial count
        SEM_COUNT,      // maximum count
        NULL);          // unnamed semaphore
    if(Stm.stmSem == NULL)
#else
    if(sem_init(&Stm.stmSem, 0, SEM_COUNT))
#endif
    {
        status = STATUS_FAILURE;
    }
#endif //end of SIMU
#ifdef RTX51_TINY_OS
  //rajan  os_create_task(HYBRII_TASK_ID_STM);  
#else

#endif
    /* start the hardware timer */
    STM_StartHwTimer();
    return status;
}

/* Timer service request API */
#ifdef CALLBACK
tTimerId STM_AllocTimer(void (*timerHdl)(u16 type, void *cookie), 
                        u16 type, void *cookie)
#else
tTimerId STM_AllocTimer(u8 mid, u16 type, void *cookie)
#endif
{
    u8 i;
    for (i = 0; i < STM_TIMER_MAX; i++)
    {
        if(!Stm.timerCb[i].valid)
        {
#ifdef CALLBACK
            Stm.timerCb[i].timerHandler = timerHdl;
#else
            Stm.timerCb[i].mid = mid;
#endif
            Stm.timerCb[i].type = type;
            Stm.timerCb[i].cookie = cookie;
            Stm.timerCb[i].valid = 1;
#if 0					
#ifdef P8051
            FM_Printf(FM_STM, "STM: alloc timer (tid = %bu)\n", i);
#else
            FM_Printf(FM_STM, "STM: alloc timer (tid = %d)\n", i);
#endif
#endif
            return (i);
        }
    }
    return STM_TIMER_ID_NULL;
}



// Timer service Deregistration
eStatus STM_FreeTimer(tTimerId tid)
{
    // Sanity test for the input parameter
    if ( (tid >= STM_TIMER_MAX) || (Stm.timerCb[tid].valid == 0) )
    {
        return STATUS_SUCCESS;
    }
    
    if ( Stm.timerCb[tid].active == 0 )
    {
        //remove the timer from timerQueue
        //NOTE: it has been removed from the timerQueue 
        //when it is stopped
//        DLIST_Remove(&(Stm.timerCb[tid].link));
        Stm.timerCb[tid].valid = 0;
        Stm.timerCb[tid].active = 0;
        return STATUS_SUCCESS;
    }
    else
    {
        return STATUS_FAILURE;
    }
   
}

/*
 * place the timer into the active timer queue in the increasing order 
 * 
 */

eStatus STM_StartTimer(tTimerId tid, tTime duration)
{
    eStatus   status = STATUS_SUCCESS;
    sTimerCb *ptimer;
    sTimerCb *ntimer;
    sTimerCb *timer;
    tTime     currTimeTick;
    u8        currTimeSector;
    //sanity test on parameters
    if( (tid >= STM_TIMER_MAX )||(Stm.timerCb[tid].valid == 0))
    {
#if 0			
#ifdef P8051
       FM_Printf(FM_WARN, "Could't find the timer (id %bu)\n", tid);
#else
       FM_Printf(FM_WARN, "Could't find the timer (id %d)\n", tid);
#endif
#endif
        return STATUS_FAILURE;
    }

    if(Stm.timerCb[tid].active)
    {
        //timer is already active
        return STATUS_SUCCESS;
    } 

    timer = &(Stm.timerCb[tid]);
#if 0		
#ifdef P8051
    FM_Printf(FM_STM, "STM: start the timer (tid = %bu)\n", tid);
#else
    FM_Printf(FM_STM, "STM: start the timer (tid = %d)\n", tid);
#endif
#endif
    //Disable the Timer 0 interrupt
#ifdef P8051
__CRIT_SECTION_BEGIN__
#else
#ifdef SIMU
    SEM_WAIT(&Stm.stmSem);
#endif
#endif
    currTimeTick = Stm.timeTick;
    currTimeSector = (u8)Stm.timeSector;
    //convert duration from ms to the time tick 
    //based on the time tick frequency
    timer->time = currTimeTick + STM_MS_TO_TICK(duration);  
#if 0		
#ifdef P8051
    FM_Printf(FM_STM, "STM: curr time sector %bu, curr time tick 0x%.8x\n", 
                      currTimeSector, currTimeTick);
#else
    FM_Printf(FM_STM, "STM: curr time sector %d, curr time tick 0x%.8x\n", 
                      currTimeSector, currTimeTick);
#endif
#endif
    if (timer->time < currTimeTick)
    {
        //overflow, thus this timer will expires in the next the time sector 
        //instead of the current time sector
        timer->timeSector = !currTimeSector;
    }
    else
    {
        //the timer will expire in the current time sector.
        timer->timeSector = currTimeSector;
    }
#if 0		
#ifdef P8051
    FM_Printf(FM_STM, "STM: timer id %bu - duration %lu, time sector %bu, time tick 0x%.8x\n", 
                      tid, duration, timer->timeSector, timer->time);
#else
    FM_Printf(FM_STM, "STM: timer %d - duration %d, time sector %d, time tick 0x%.8x\n", 
                      tid, duration, timer->timeSector, timer->time);
#endif
#endif		
    //Place the timer STM.timerCb[tid] into STM.timerQueue 
    //in the increasing time order
    if(DLIST_IsEmpty(&(Stm.timerQueue)))
    {
        Stm.timerCb[tid].active = 1;
        DLIST_Push(&(Stm.timerQueue), &timer->link);
#if 0			
#ifdef P8051
        FM_Printf(FM_STM, "STM: start timer (tid = %bu) successfully (1)\n", tid);
#else
        FM_Printf(FM_STM, "STM: start timer (tid = %d) successfully (1)\n", tid);
#endif
#endif			
        status = STATUS_SUCCESS;
        goto endCrit; 
    }


    //DLIST_For_Each_Entry(&(Stm.timerQueue), ptimer, link)
    DLIST_For_Each_Entry(&(Stm.timerQueue), ptimer, sTimerCb, link)
    {
        if(ptimer->timeSector == currTimeSector) 
        {
           if( (timer->timeSector == ptimer->timeSector) &&
               (timer->time <= ptimer->time))
            {
                //add timer before ptimer
                Stm.timerCb[tid].active = 1;
                DLIST_Put(&ptimer->link, &timer->link);       
                status = STATUS_SUCCESS;
#if 0							
#ifdef P8051
FM_Printf(FM_STM, "STM: start timer (tid = %bu) successfully(2)\n", tid);
#else
FM_Printf(FM_STM, "STM: start timer (tid = %d) successfully(2)\n", tid);
#endif
#endif							
                goto endCrit; 
            }
           
        } else if(ptimer->timeSector != currTimeSector)
        {
              //reach the first timer in the next timer sector
              //break the for loop
              break;
        }
    }

    //at this point, we have the following cases:
    //(1) Reach the tail of time queue and the timer is in the current time 
    //    sector, thus greater than any timer in the queue
    //(2) Reach the tail of time queue and the timer is in the next time 
    //    sector
    //(3) Reach the first timer in the next time sector, and 
    //    the timer is the current time sector 
    //(4) Reach the first timer in the next time sector and 
    //    the timer is in the next time sector.

    //case (1) and (2): place the timer before the ptimer. 
    if(DLIST_IsHead(&(Stm.timerQueue), &ptimer->link))
    {
        Stm.timerCb[tid].active = 1;
        DLIST_Put(&Stm.timerQueue, &timer->link);       
        status = STATUS_SUCCESS;
#if 0			
#ifdef P8051
FM_Printf(FM_STM, "STM: start timer (tid = %bu) successfully(3)\n", tid);
#else
FM_Printf(FM_STM, "STM: start timer (tid = %d) successfully(3)\n", tid);
#endif
#endif			
        goto endCrit; 
    }
    
    //case (3): place the timer before the ptimer
    if(ptimer->timeSector != timer->timeSector)
    { 
        Stm.timerCb[tid].active = 1;
        DLIST_Put(&ptimer->link, &timer->link);       
        status = STATUS_SUCCESS;
#if 0			
#ifdef P8051
FM_Printf(FM_STM, "STM: start timer (tid = %bu) successfully(4)\n", tid);
#else
FM_Printf(FM_STM, "STM: start timer (tid = %d) successfully(4)\n", tid);
#endif
#endif
        goto endCrit; 
    }

    //case (4): starting with ptimer, search until the end of time queue
    //DLIST_For_Each_Entry_Start(&(Stm.timerQueue), ptimer, ntimer, link)
    DLIST_For_Each_Entry_From(&(Stm.timerQueue), ptimer, ntimer, 
                                sTimerCb, link)
    {
#if 0			
#ifdef P8051
FM_Printf(FM_STM, "STM: from ntimer (tid: %bu time sector %bu, time tick 0x%.8x\n", 
                      ntimer->tid, (u8)ntimer->timeSector, ntimer->time);
#else
FM_Printf(FM_STM, "STM: from ntimer (tid: %d time sector %d, time tick 0x%.8x\n", 
                      ntimer->tid, ntimer->timeSector, ntimer->time);
#endif
#endif			
        if( (timer->timeSector == ntimer->timeSector) &&
            (timer->time <= ntimer->time))
        {
            Stm.timerCb[tid].active = 1;
            DLIST_Put(&ntimer->link, &timer->link);       
            status = STATUS_SUCCESS;
#if 0					
#ifdef P8051
FM_Printf(FM_STM, "STM: start timer (tid = %bu) successfully(5)\n", tid);
#else
FM_Printf(FM_STM, "STM: start timer (tid = %d) successfully(5)\n", tid);
#endif
#endif					
            goto endCrit; 
        }

    }

//FM_Printf(FM_STM, "STM: ntimer (tid: %bu time sector %bu, time tick 0x%.8x.\n", 
//                      ntimer->tid, (u8)ntimer->timeSector, ntimer->time);

    //reach the end of the next timer sector
    //thus put the end of queue
//    if( timer->timeSector == ntimer->timeSector) 
    {
        Stm.timerCb[tid].active = 1;
        DLIST_Put(&(Stm.timerQueue), &timer->link);       
        status = STATUS_SUCCESS;
#if 0			
#ifdef P8051
FM_Printf(FM_STM, "STM: start timer (tid = %bu) successfully(6)\n", tid);
#else
FM_Printf(FM_STM, "STM: start timer (tid = %d) successfully(6)\n", tid);
#endif
#endif			
        goto endCrit; 
    }
//    else
//    {
//       FM_Printf(FM_ERROR, "STM: Could not start the timer (tid = %bu).\n", tid);
//       status = STATUS_FAILURE;
//       goto endCrit; 
//   }
    
endCrit: 

#ifdef TEST
STM_DisplayTimerQueue();
#endif
#if 0
#endif

//Enable the Timer 0 interrupt
#ifdef P8051
__CRIT_SECTION_END__
#else
#ifdef SIMU
    SEM_POST(&Stm.stmSem);
#endif
#endif
    
    return status;
}



//stop a timer
eStatus STM_StopTimer(tTimerId tid)
{
    //Sanity check
    if( (tid >= STM_TIMER_MAX )||
        (Stm.timerCb[tid].valid == 0 ) ||
        (Stm.timerCb[tid].active == 0) )
    {
        return STATUS_SUCCESS;
    }

//STM_DisplayTimerQueue();

#ifdef P8051
__CRIT_SECTION_BEGIN__
#else
#ifdef SIMU
    SEM_WAIT(&Stm.stmSem);
#endif
#endif
    //Remove the timer Stm->TimerCb[tid] from TimerQueue.
    DLIST_Remove(&(Stm.timerCb[tid].link));
    Stm.timerCb[tid].active = 0;
#ifdef P8051
__CRIT_SECTION_END__
#else
#ifdef SIMU
    SEM_POST(&Stm.stmSem);
#endif
#endif


#if 0	
#ifdef P8051
    FM_Printf(FM_STM, "STM: stop the timer (tid = %bu)\n", tid);
#else
    FM_Printf(FM_STM, "STM: stop the timer (tid = %d)\n", tid);
#endif
#endif
//STM_DisplayTimerQueue();

    return STATUS_SUCCESS;
}


#ifndef CALLBACK
void STM_ProcTimer(u8 mid, u16 type, void *cookie)
{
    switch(mid)
    {
#ifdef HYBRII_HPGP

#ifdef UM
        case HP_LAYER_TYPE_CTRL:
        {
            CTRLL_TimerHandler(type, cookie);
            break;
        }
        case HP_LAYER_TYPE_LINK:
        {
            LINKL_TimerHandler(type, cookie);
            break;
        } 
#endif		
#ifdef NO_HOST
		case HP_LAYER_TYPE_APP:
		{
			hostTimerEvnt_t timerevent;
			timerevent.type = type;
			Host_SendIndication(HOST_EVENT_APP_TIMER,
								(u8*)&timerevent, sizeof(hostTimerEvnt_t));
			break;
		}	  
#endif
// Auto ping from keil periodically send ping req
// TO send ping req periodically timeout of 1 sec needed
// After timeout wakeup call for UI/htm task
#ifdef AUTO_PING 
        case HP_LAYER_TYPE_HTM:
        {
            os_set_ready(HYBRII_TASK_ID_UI);
            break;
        }
#endif
        
#endif
#ifdef HYBRII_ZIGBEE
        case ZB_LAYER_TYPE_MAC:
        {
            zb_mac_timer_handler(type, cookie);
            break;
        }
#endif
        default:
        {
            break;
        }
    }

}

#endif

void STM_Proc (void)
{
    sTimerCb *ptimer;
    u8        currTimeSector; 

   
    currTimeSector = (u8)Stm.timeSector;


    //FM_Printf(FM_ERROR, "STM: time tick %d.\n",
    //          Stm.timeTick);

    /* 
     * Depending on the software timer resoultion(N), 
     * either invoke the timers every time tick (N=1, 
     * thus, the same as hardware timer)
     * or every N time ticks (N>1, thus lower than the hardware timer). 
     * At present, the software timer resolution is the same as 
     * the hardware timer
     */

    // Search through the software timer active queue
    //Remove those software timers that have the same flag as the timeFlag 
    //and have expired
    
    //if(Stm.timeTick%STM_TIMER_RESOLUTION == 0）
    //{

    do 
    {
        if (DLIST_IsEmpty(&Stm.timerQueue))
        {
            break;
        }

        ptimer = DLIST_PeekHeadEntry(&(Stm.timerQueue), sTimerCb, link);
            
        if(ptimer->timeSector == currTimeSector) 
        {
            if (ptimer->time <= Stm.timeTick)
            {
#ifdef P8051
__CRIT_SECTION_BEGIN__
#else
#ifdef SIMU
                SEM_WAIT(&Stm.stmSem);
#endif
#endif
                DLIST_Remove(&ptimer->link);       
                ptimer->active = 0;
#ifdef P8051
__CRIT_SECTION_END__
#else
#ifdef SIMU
                SEM_POST(&Stm.stmSem);
#endif
#endif
#ifdef P8051
#if 0
                FM_Printf(FM_STM|FM_MINFO,
                          "STM: call a timer handler (tid: %bu, timeSector: %bu, "
                          "time tick: 0x%.8x, currTimeSector %bu, curr timeTick: 0x%.8x)\n", 
                          ptimer->tid, (u8)ptimer->timeSector,  ptimer->time, 
                          currTimeSector, Stm.timeTick);
#endif
#else
#if 0
                FM_Printf(FM_STM|FM_MINFO, 
                          "STM: call a timer handler (tid: %d, timeSector: %d, "
                          "time tick: 0x%.8x, currTimeSector %d, curr timeTick: 0x%.8x)\n", 
                          ptimer->tid, ptimer->timeSector,  ptimer->time, 
                          currTimeSector, Stm.timeTick);
#endif
#endif
#ifdef CALLBACK
                ptimer->timerHandler(ptimer->type, ptimer->cookie); 
#else
                STM_ProcTimer(ptimer->mid, ptimer->type, ptimer->cookie);
#endif
            }
            else
            {
                break;
            }
                
        } 
        else if (ptimer->timeSector == Stm.timeSector) 
        {
            if (ptimer->time <= Stm.timeTick)
            {
#ifdef P8051
__CRIT_SECTION_BEGIN__
#else
#ifdef SIMU
                SEM_WAIT(&Stm.stmSem);
#endif
#endif
                DLIST_Remove(&ptimer->link);       
                ptimer->active = 0;
#ifdef P8051
__CRIT_SECTION_END__
#else
#ifdef SIMU
                SEM_POST(&Stm.stmSem);
#endif
#endif
#if 0							
#ifdef P8051
                FM_Printf(FM_STM|FM_MINFO,
                          "STM (2): call Timer handler (tid: %bu, timeSector: %bu, "
                          "time tick: 0x%.8x, currTimeSector %bu, curr timeTick: 0x%.8x)\n",
                          ptimer->tid, (u8)ptimer->timeSector, ptimer->time, \
                          currTimeSector, Stm.timeTick);
#else
                FM_Printf(FM_STM|FM_MINFO,
                          "STM (2): call Timer handler (tid: %d, timeSector: %d, "
                          "time tick: 0x%8x, currTimeSector %d, curr timeTick: 0x%.8x)\n",
                          ptimer->tid, ptimer->timeSector, ptimer->time, \
                          currTimeSector, Stm.timeTick);
#endif
#endif

#ifdef CALLBACK
                ptimer->timerHandler(ptimer->type, ptimer->cookie); 
#else
                STM_ProcTimer(ptimer->mid, ptimer->type, ptimer->cookie);
#endif
            }
            else
            {
                break;
            }
        } 
        else
        {
            break;
        }
    } while (1);

}

#ifdef RTX51_TINY_OS
void STM_Task (void) //_task_ HYBRII_TASK_ID_STM
{
   while (1) {
        STM_Proc();
        os_switch_task();
    }
}
#endif


#ifdef TEST

void STM_DisplayTimerQueue()
{
    sTimerCb *ptimer;
    FM_Printf(FM_STM, "Timer Queue:\n");
    DLIST_For_Each_Entry(&(Stm.timerQueue), ptimer, sTimerCb, link)
    {
#if 0			
#ifdef P8051
       FM_Printf(FM_STM, "Timer (tid: %bu), timeSector: %bu, time tick: 0x%.8x\n",
                 ptimer->tid, (u8)ptimer->timeSector, ptimer->time);
#else
       FM_Printf(FM_STM, "Timer (tid: %d), timeSector: %d, time tick: 0x%.8x\n",
                 ptimer->tid, ptimer->timeSector, ptimer->time);
#endif
#endif			
    }
           
}
#endif

/** =========================================================
 *
 * Edit History
 *
 * $Source: /home/cvsrepo/Hybrii_B_OSFW_Dev/firmware/common/stm.c,v $
 *
 * $Log: stm.c,v $
 * Revision 1.17  2014/07/22 10:03:52  kiran
 * 1) SDK Supports Power Save
 * 2) Uart_Driver.c cleanup
 * 3) SDK app memory pool optimization
 * 4) Prints from STM.c are commented
 * 5) Print messages are trimmed as common no memory left in common
 * 6) Prins related to Power save and some other modules are in compilation flags. To enable module specific prints please refer respective module
 *
 * Revision 1.16  2014/06/11 13:17:46  kiran
 * UART as host interface and peripheral interface supported.
 *
 * Revision 1.15  2014/06/10 22:46:11  yiming
 * Merge Zigbee-PLC bridging code and System Configuration R/W to flash
 *
 * Revision 1.14  2014/05/28 10:58:58  prashant
 * SDK folder structure changes, Uart changes, removed htm (UI) task
 * Varified - UM, LM - iperf (UDP/TCP), overnight test pass
 *
 * Revision 1.13  2014/05/12 08:35:00  prashant
 * timer fix
 *
 * Revision 1.12  2014/05/12 08:09:57  prashant
 * Route fix, STA sync issue with AV CCO and handling discover bcn issue fix
 *
 * Revision 1.11  2014/05/01 19:18:24  yiming
 * bring FPGA setting back
 *
 * Revision 1.10  2014/04/15 19:52:20  yiming
 * Merge new ASIC setting, Add throughput improvement code, add M_PER code
 *
 * Revision 1.9  2014/04/11 12:23:54  prashant
 * Under PLC_TEST macro Diagnostic Mode code added
 *
 * Revision 1.8  2014/04/09 08:18:10  ranjan
 * 1. Added host events for homeplug uppermac indication (Host_SendIndication)
 * 2. timer workaround  + other fixes
 *
 * Revision 1.7  2014/03/25 17:01:07  son
 * Hybrii B ASIC bring up
 *
 * Revision 1.6  2014/03/12 23:41:28  yiming
 * Merge Hybrii B ASIC code
 *
 * Revision 1.5  2014/03/10 05:58:10  ranjan
 * 1. added HomePlug BackupCCo feature. verified C&I test.(passed.) (bug 176)
 *
 * Revision 1.4  2014/02/27 10:42:47  prashant
 * Routing code added
 *
 * Revision 1.3  2014/01/22 19:34:23  son
 * Reenable calling to Zigbee timer handler
 *
 * Revision 1.2  2014/01/10 17:02:18  yiming
 * check in Rajan 1/8/2014 code release
 *
 * Revision 1.3  2014/01/08 10:53:53  ranjan
 * Changes for LM OS support.
 * New Datapath FrameTask
 * LM and UM  datapath, feature verified.
 *
 * known issues : performance numbers needs revisit
 *
 * review : pending.
 *
 * Revision 1.2  2013/01/24 00:13:46  yiming
 * Use 01-23-2013 Hybrii-A code as first Hybrii-B code base
 *
 * Revision 1.17  2013/01/04 16:11:22  prashant
 * SPI to PLC bridgeing added, Queue added for SPI and Ethernet
 *
 * Revision 1.16  2012/11/27 20:49:25  son
 * Put back interrupt 1 on timer_handler as this cause issue with PLC upper mac project
 *
 * Revision 1.15  2012/11/26 18:02:10  son
 * Avoid timer 1 vector overlapping
 *
 * Revision 1.14  2012/11/22 09:44:02  prashant
 * Code change for auto ping test, sending tei map ind out, random mac addrr generation.
 *
 * Revision 1.13  2012/07/19 21:46:07  son
 * Prepared files for zigbee integration
 *
 * Revision 1.10  2012/06/29 02:41:08  kripa
 * Calling CHAL timer handler from timer ISR.
 * Committed on the Free edition of March Hare Software CVSNT Client.
 * Upgrade to CVS Suite for more features and support:
 * http://march-hare.com/cvsnt/
 *
 * Revision 1.9  2012/06/05 22:37:11  son
 * UART console does not get initialized due to task ID changed
 *
 * Revision 1.8  2012/06/05 07:25:58  yuanhua
 * (1) add a scan call to the MAC during the network discovery. (2) add a tiny task in ISM for interrupt polling (3) modify the frame receiving integration. (4) modify the tiny task in STM.
 *
 * Revision 1.7  2012/06/04 23:27:59  son
 * Added STM_Task
 *
 * Revision 1.6  2012/05/19 05:05:15  yuanhua
 * optimized the timer handlers in CTRL and LINK layers.
 *
 * Revision 1.5  2012/05/17 05:05:58  yuanhua
 * (1) added the option for timer w/o callback (2) added task id and name.
 *
 * Revision 1.4  2012/04/13 06:15:10  yuanhua
 * integrate the HPGP protocol stack with the HAL for the beacon TX/RX and mgmt msg RX.
 *
 * Revision 1.3  2012/03/11 17:02:24  yuanhua
 * (1) added NEK auth in AKM (2) added NMA (3) modified hpgp layer data structures (4) added Makefile for linux simulation
 *
 * Revision 1.2  2011/09/09 07:02:31  yuanhua
 * migrate the firmware code from the greenchip to the hybrii.
 *
 * Revision 1.6  2011/08/12 23:13:21  yuanhua
 * (1)Added Control Layer (2) Fixed bugs for user-selected CCo handover (3) Made changes to SNAM/CNAM and SNSM/CNSM for CCo handover switch (from CCo to STA, from STA to CCo, and from STA to STA but with different CCo) and post CCo handover
 *
 * Revision 1.5  2011/08/02 16:06:00  yuanhua
 * (1) Fixed a bug in STM (2) Made STA discovery work according to the standard, including aging timer. (3) release the resource after the STA leave (4) STA will switch to the backup CCo if the CCo failure occurs (5) At this point, the CCo could work with multiple STAs correctly, including CCo association/leave, TEI renew, TEI map updating, discovery beacon scheduling, discovery STA list updating ang aging, CCo failure, etc.
 *
 * Revision 1.4  2011/07/30 02:43:35  yuanhua
 * (1) Split the beacon process into two parts: one requiring an immdiate response, the other tolerating the delay (2) Changed the API btw the MUX and SHAL for packet reception (3) Fixed bugs in various modules. Now, multiple STAs could successfully associate/leave the CCo
 *
 * Revision 1.3  2011/07/22 18:51:04  yuanhua
 * (1) added the hardware timer tick simulation (hts.h/hts.c) (2) Added semaphors for sync in simulation and defined macro for critical section (3) Fixed some bugs in list.h and CRM (4) Modified and fixed bugs in SHAL (5) Changed SNSM/CNSM and SNAM/CNAM for bug fix (6) Added debugging prints, and more.
 *
 * Revision 1.2  2011/06/24 14:33:18  yuanhua
 * (1) Changed event structure (2) Implemented SNSM, including the state machines in network discovery and connection states, becaon process, discover process, and handover detection (3) Integrated the HPGP and SHAL
 *
 *
 * ==========================================================*/

