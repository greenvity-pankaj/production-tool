/** ========================================================
 *
 * @file hpgpapi.h
 * 
 *  @brief HomePlug GREEN PHY API 
 *
 *  Copyright (C) 2010-2011, Greenvity Communications, Inc.
 *  All Rights Reserved
 *  
 * =========================================================*/

#ifndef  _HPGPAPI_H
#define  _HPGPAPI_H

#ifdef P8051
#include "hal.h"
#include "hpgpctrl.h"
#include "nma.h"
#include "event.h"
#else
typedef struct haLayer   sHaLayer,  *psHaLayer;
typedef struct hpgpCtrl  sHpgpCtrl, *psHpgpCtrl;
typedef struct nma       sNma,      *psNma;
typedef struct event     sEvent,    *psEvent;
#endif
#ifdef UM
void*      HPGPCTRL_GetLayer(u8 layer);

sHpgpCtrl* HOMEPLUG_GetCtrlPlane();

sHaLayer*  HOMEPLUG_GetHal();

sNma*      HOMEPLUG_GetNma();
#endif
#ifndef CALLBACK
#ifdef HPGP_HAL
#ifdef HAL_INT
extern u8 HAL_RecvMacFrame(void *cookie);
#else
extern u8 HAL_RxPoll(void *cookie);
#endif
#endif /* HPGP_HAL */
extern u8 MUXL_Proc(void *cookie);
extern u8 LINKL_Proc(void *cookie);
extern u8 CTRLL_Proc(void *cookie);
#ifdef NMA
extern u8 NMA_Proc(void *cookie);
void NMA_RecvMgmtPacket(void* cookie,  sEvent *event);
#endif
void CTRLL_ReceiveEvent(void* cookie, sEvent* event);
void LINKL_RecvMgmtMsg(void *cookie,  sEvent *event);
void LINKL_StaProcBcnHandler(void *cookie, sEvent *event);
void LINKL_CcoProcBcnHandler(void *cookie, sEvent *event);
void MUXL_RecvMgmtPacket(void *cookie, sEvent *event) __REENTRANT__;
void HAL_BcnRxIntHandler(void *cookie);
void LINKL_BcnTxHandler(void* cookie);
void CTRLL_TimerHandler(u16 type, void *cookie);
void LINKL_TimerHandler(u16 type, void *cookie);
void LINKL_DiscAgingTimerHandler(void* cookie);
void LINKL_TeiTimerHandler(void* cookie);
void LINKL_AkmTimerHandler(void* cookie);
#ifdef STA_FUNC
void LINKL_BbtTimerHandler(void* cookie);
void LINKL_UsttTimerHandler(void* cookie);
void LINKL_AccTimerHandler(void* cookie);
void LINKL_ApptTimerHandler(void* cookie);
#endif
#ifdef CCO_FUNC
void LINKL_DiscTimerHandler(void* cookie);
void LINKL_StaTimerHandler(void* cookie);

#ifdef SIMU
void LINKL_BcnTimerHandler(u16 type, void* cookie);
#endif
#endif /* CCO_FUNC */
#endif /* CALLBACK */



#endif


/** =========================================================
 *
 * Edit History
 *
 * $Source: /home/cvsrepo/Hybrii_B_OSFW_Dev/firmware/hpgp/src/hpgpapi.h,v $
 *
 * $Log: hpgpapi.h,v $
 * Revision 1.2  2014/01/10 17:13:09  yiming
 * check in Rajan 1/8/2014 code release
 *
 * Revision 1.5  2014/01/08 10:53:54  ranjan
 * Changes for LM OS support.
 * New Datapath FrameTask
 * LM and UM  datapath, feature verified.
 *
 * known issues : performance numbers needs revisit
 *
 * review : pending.
 *
 * Revision 1.4  2013/09/04 14:49:33  yiming
 * New changes for Hybrii_A code merge
 *
 * Revision 1.11  2013/04/04 12:21:54  prashant
 * Detecting PLC link failure for HMC. added project for HMC and Renesas
 *
 * Revision 1.10  2012/09/24 06:01:38  yuanhua
 * (1) Integrate the NMA and HAL in Rx path (2) add a Tx queue in HAL to have less stack size needed in tx path, and Tx in HAL is performed by polling now.
 *
 * Revision 1.9  2012/07/12 22:05:55  son
 * Moved ISM Polling to ISM Task.
 * UI is now part of init task
 *
 * Revision 1.8  2012/05/19 22:22:16  yuanhua
 * added bcn Tx/Rx non-callback option for the ISM.
 *
 * Revision 1.7  2012/05/19 20:32:17  yuanhua
 * added non-callback option for the protocol stack.
 *
 * Revision 1.6  2012/05/19 05:05:15  yuanhua
 * optimized the timer handlers in CTRL and LINK layers.
 *
 * Revision 1.5  2012/05/17 05:05:58  yuanhua
 * (1) added the option for timer w/o callback (2) added task id and name.
 *
 * Revision 1.4  2012/05/14 05:22:29  yuanhua
 * support the SCHED without using callback functions.
 *
 * Revision 1.3  2012/04/13 06:15:11  yuanhua
 * integrate the HPGP protocol stack with the HAL for the beacon TX/RX and mgmt msg RX.
 *
 * Revision 1.2  2012/03/11 17:02:24  yuanhua
 * (1) added NEK auth in AKM (2) added NMA (3) modified hpgp layer data structures (4) added Makefile for linux simulation
 *
 * Revision 1.1  2011/07/03 06:00:35  jie
 * Initial check in
 *
 * Revision 1.1  2011/06/23 23:52:42  yuanhua
 * move green.h green.c hpgpapi.h hpgpdef.h hpgpconf.h to src directory
 *
 * Revision 1.1  2011/05/06 19:07:48  kripa
 * Adding ctrl layer files to new source tree.
 *
 * Revision 1.1  2011/04/08 21:40:41  yuanhua
 * Framework
 *
 *
 * ========================================================*/


