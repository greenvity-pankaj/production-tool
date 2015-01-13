/** =========================================================
 *
 *  @file mux.h
 * 
 *  @brief Multiplex Layer  
 *
 *  Copyright (C) 2010-2011, Greenvity Communications, Inc.
 *  All Rights Reserved
 *  
 * ===========================================================*/

#ifndef _MUX_H
#define _MUX_H

#include "list.h"
#include "papdef.h"
#include "sched.h"
#include "hpgpevt.h"
#include "hal.h"

#if defined(WIN32) || defined(_WIN32)
#include <windows.h>
#endif

typedef struct mux
{
    //TX
  //  u16      txThreshold;
    //u8       txnfmi;        //number of fragments (NF_MI) for TX
//    u8       txfmsn;        //fragmentation message sequence number for TX

    //RX
    sSlist   rxQueue;       //message reassembly queue 
    u8       rxqlen;
    u8       rxnfmi;        //number of fragments (NF_MI) for RX
    u8       rxfmsn;        //fragmentation message sequence number for RX
#ifdef CALLBACK
    void     (*deliverMgmtMsg)(void XDATA *mgmtcookie, sEvent XDATA *event);
#endif
    void     *mgmtcookie;

} sMux, *psMux;

typedef struct muxLayer
{
    sMux    mux;

    //this semaphore is used to control enqueuing/dequeuing an event
    //to/from the eventQueue in the simulation 
#ifndef P8051
#if defined(WIN32) || defined(_WIN32)
    HANDLE   muxSem;
#else //POSIX
    sem_t    muxSem;
#endif
#endif

    sSlist  eventQueue; 

  //  sExecTask  task;
	
    sHaLayer  *hal;

} sMuxLayer, *psMuxLayer;


void MUXL_RegisterMgmtMsgCallback(sMuxLayer *muxl, 
    void (*deliverMgmtMsg)(void XDATA *cookie, sEvent XDATA *event),
    void *cookie);

eStatus MUXL_Init(sMuxLayer *muxl);

eStatus MUXL_TransmitMgmtMsg(sMuxLayer *mux, sEvent *event);

u8 MUXL_Proc(void *muxl);

#endif



/** =========================================================
 *
 * Edit History
 *
 * $Source: /home/cvsrepo/Hybrii_B_OSFW_Dev/firmware/hpgp/src/mux/muxl.h,v $
 *
 * $Log: muxl.h,v $
 * Revision 1.2  2014/07/05 09:16:27  prashant
 * 100 Devices support- only association tested, memory adjustments
 *
 * Revision 1.1  2013/12/18 17:05:33  yiming
 * no message
 *
 * Revision 1.1  2013/12/17 21:48:08  yiming
 * no message
 *
 * Revision 1.4  2013/09/04 14:51:16  yiming
 * New changes for Hybrii_A code merge
 *
 * Revision 1.5  2012/09/15 17:30:38  yuanhua
 * fixed compilation errors and a missing field (hal) in NMA
 *
 * Revision 1.4  2012/04/13 06:15:11  yuanhua
 * integrate the HPGP protocol stack with the HAL for the beacon TX/RX and mgmt msg RX.
 *
 * Revision 1.3  2012/03/11 17:02:25  yuanhua
 * (1) added NEK auth in AKM (2) added NMA (3) modified hpgp layer data structures (4) added Makefile for linux simulation
 *
 * Revision 1.2  2011/09/09 07:02:31  yuanhua
 * migrate the firmware code from the greenchip to the hybrii.
 *
 * Revision 1.5  2011/08/09 22:45:44  yuanhua
 * changed to event structure, seperating HPGP-related events from the general event defination so that the general event could be used for other purposes than the HPGP.
 *
 * Revision 1.4  2011/07/22 18:51:05  yuanhua
 * (1) added the hardware timer tick simulation (hts.h/hts.c) (2) Added semaphors for sync in simulation and defined macro for critical section (3) Fixed some bugs in list.h and CRM (4) Modified and fixed bugs in SHAL (5) Changed SNSM/CNSM and SNAM/CNAM for bug fix (6) Added debugging prints, and more.
 *
 * Revision 1.3  2011/06/24 14:33:18  yuanhua
 * (1) Changed event structure (2) Implemented SNSM, including the state machines in network discovery and connection states, becaon process, discover process, and handover detection (3) Integrated the HPGP and SHAL
 *
 * Revision 1.2  2011/05/28 06:33:42  kripa
 * *** empty log message ***
 *
 * Revision 1.1  2011/05/06 19:12:25  kripa
 * Adding mux layer files to new source tree.
 *
 * Revision 1.2  2011/04/23 19:48:45  kripa
 * Fixing stm.h and event.h inclusion, using relative paths to avoid conflict with windows system header files.
 *
 * Revision 1.1  2011/04/08 21:42:11  yuanhua
 * Framework
 *
 *
 *  =======================================================*/

