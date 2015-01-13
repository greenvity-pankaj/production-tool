/** ========================================================
 *
 *  @file stm.h
 * 
 *  @brief Software Timer Manager
 *
 *  Copyright (C) 2010-2011, Greenvity Communications, Inc.
 *  All Rights Reserved
 *  
 * =========================================================*/

#ifndef  _STM_H
#define  _STM_H

#include "papdef.h"
#include "list.h"


//according to the HPGP standard, 32 bit timer is sufficient 
//to cover the TEI lease time, assume that 1 time tick is at least 1 ms. 
//If the hardware timer is higher resoultion (e.g. 1 time tick is 1 ns)
//or if the platform supports only 16-bits variable,
//we may implement more than two time sectors, instead of two at present
//(or multiple timer queue, instead of one at present).

//typedef    u32      tTime;


#define STM_TIMER_MAX           (120) // (64)   
#define STM_TIMER_INVALID_ID    (0xFF)   
#define STM_TIMER_ID_NULL       (0xFF)   

#ifdef B_ASICPLC
#define STM_TIME_TICK_INIT   0// 0xFFFFF000
#else
#define STM_TIME_TICK_INIT    0x00
#endif
#define STM_TIME_TICK_MAX     0xFFFFFFFF


//timer control block
typedef struct timerCb
{
    sDlink      link;
    u8          valid:      1;
    u8          active:     1;
    u8          timeSector: 1;  //time sector in which the timer will expire 
    u8          rsvd:       5;

    u8          tid;           /* timer id */

#ifdef CALLBACK
    void      (*timerHandler)(u16 type, void * cookies);
#else
    u8          mid;           /* module id */
#endif
    u16         type;          /* timer type */
    void       *cookie;

    tTime       time;          //timer expiration (in time tick)
                               //in the time sector indicated by timeSector

} sTimerCb, *psTimerCb;

typedef struct stm
{
    //timer resource
    sTimerCb timerCb[STM_TIMER_MAX];

    //time tick starting with the first time interrupt
    tTime      timeTick;

    //two time sectors: 0 and 1.
    //change of timeSector indicates that time tick enters into 
    //the next time sector because of time tick overflow 
    //in the current time sector
    u8       timeSector: 1; 
    u8       rsvd:       7;

    //this semaphore is used to control enqueuing/dequeuing the timer
    //to/from the timerQueue, because the hardware timer tick is simulated 
    //by the HTS running in a thread
#ifdef SIMU
#if defined(WIN32) || defined(_WIN32)
    HANDLE   stmSem;
#else //POSIX
    sem_t    stmSem; 
#endif
#endif
    //active timer queue
    sDlink   timerQueue;    

} sStm, *psStm;

eStatus STM_Init(void);

#ifdef CALLBACK
tTimerId STM_AllocTimer(void (*timerHdl)(u16 type, void * cookie), u16 type, void* cookie);
#else
tTimerId STM_AllocTimer(u8 mid, u16 type, void *cookie);
#endif
//eStatus STM_StartTimer(tTimerId tid, u16 duration);
//NOTE: assume that the time unit is ms at present
eStatus STM_StartTimer(tTimerId tid, tTime interval);
eStatus STM_StopTimer(tTimerId tid);
eStatus STM_FreeTimer(tTimerId tid);



extern void zb_mac_timer_handler(u16 type, void *cookie);
#ifdef TEST
void STM_DisplayTimerQueue();

#endif

extern tTimerId scan_duration_timer;
extern tTimerId beacon_tracking_timer;
extern tTimerId indirect_data_persistence_timer;
extern tTimerId poll_wait_timer;
extern tTimerId beacon_missed_timer;
extern tTimerId mac_rsp_wait_timer;
extern tTimerId mac_asso_rsp_wait_timer;
#endif

u32 STM_GetTick();

/** =========================================================
 *
 * Edit History
 *
 * $Source: /home/cvsrepo/Hybrii_B_OSFW_Dev/firmware/common/stm.h,v $
 *
 * $Log: stm.h,v $
 * Revision 1.7  2014/07/05 09:16:27  prashant
 * 100 Devices support- only association tested, memory adjustments
 *
 * Revision 1.6  2014/05/28 10:58:58  prashant
 * SDK folder structure changes, Uart changes, removed htm (UI) task
 * Varified - UM, LM - iperf (UDP/TCP), overnight test pass
 *
 * Revision 1.5  2014/04/11 12:23:54  prashant
 * Under PLC_TEST macro Diagnostic Mode code added
 *
 * Revision 1.4  2014/03/12 23:41:28  yiming
 * Merge Hybrii B ASIC code
 *
 * Revision 1.3  2014/03/10 05:58:10  ranjan
 * 1. added HomePlug BackupCCo feature. verified C&I test.(passed.) (bug 176)
 *
 * Revision 1.2  2014/01/10 17:02:18  yiming
 * check in Rajan 1/8/2014 code release
 *
 * Revision 1.4  2014/01/08 10:53:53  ranjan
 * Changes for LM OS support.
 * New Datapath FrameTask
 * LM and UM  datapath, feature verified.
 *
 * known issues : performance numbers needs revisit
 *
 * review : pending.
 *
 * Revision 1.3  2013/10/23 16:01:16  son
 * Added timer to wait for Association.RESP
 *
 * Revision 1.2  2013/01/24 00:13:46  yiming
 * Use 01-23-2013 Hybrii-A code as first Hybrii-B code base
 *
 * Revision 1.8  2013/01/04 16:11:22  prashant
 * SPI to PLC bridgeing added, Queue added for SPI and Ethernet
 *
 * Revision 1.7  2012/11/20 22:47:56  son
 * Added misc. mac functions
 *
 * Revision 1.6  2012/07/19 21:46:07  son
 * Prepared files for zigbee integration
 *
 * Revision 1.5  2012/05/19 05:05:15  yuanhua
 * optimized the timer handlers in CTRL and LINK layers.
 *
 * Revision 1.4  2012/05/17 05:05:58  yuanhua
 * (1) added the option for timer w/o callback (2) added task id and name.
 *
 * Revision 1.3  2012/03/11 17:02:24  yuanhua
 * (1) added NEK auth in AKM (2) added NMA (3) modified hpgp layer data structures (4) added Makefile for linux simulation
 *
 * Revision 1.2  2011/09/09 07:02:31  yuanhua
 * migrate the firmware code from the greenchip to the hybrii.
 *
 * Revision 1.5  2011/07/30 02:43:35  yuanhua
 * (1) Split the beacon process into two parts: one requiring an immdiate response, the other tolerating the delay (2) Changed the API btw the MUX and SHAL for packet reception (3) Fixed bugs in various modules. Now, multiple STAs could successfully associate/leave the CCo
 *
 * Revision 1.4  2011/07/22 18:51:04  yuanhua
 * (1) added the hardware timer tick simulation (hts.h/hts.c) (2) Added semaphors for sync in simulation and defined macro for critical section (3) Fixed some bugs in list.h and CRM (4) Modified and fixed bugs in SHAL (5) Changed SNSM/CNSM and SNAM/CNAM for bug fix (6) Added debugging prints, and more.
 *
 * Revision 1.3  2011/07/02 22:09:01  yuanhua
 * Implemented both SNAM and CNAM modules, including network join and leave procedures, systemm resource (such as TEI) allocation and release, TEI renew/release timers, and TEI reuse timer, etc.
 *
 * Revision 1.2  2011/05/28 06:23:56  kripa
 * combining list header files
 *
 * Revision 1.1  2011/05/06 18:31:47  kripa
 * Adding common utils and isr files for Greenchip firmware.
 *
 * Revision 1.1  2011/04/08 21:40:42  yuanhua
 * Framework
 *
 *
 * =========================================================*/

