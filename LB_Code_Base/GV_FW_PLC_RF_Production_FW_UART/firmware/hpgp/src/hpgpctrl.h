/** =========================================================
 *
 *  @file hpgpctrl.h
 * 
 *  @brief HPGP Control Plane
 *
 *  Copyright (C) 2010-2011, Greenvity Communications, Inc.
 *  All Rights Reserved
 *  
 * ==========================================================*/


#ifndef  _HPGP_CTRL_H
#define  _HPGP_CTRL_H

#include "hpgpdef.h"
#ifdef UM
#include "muxl.h"
#include "linkl.h"
#include "ctrll.h"

typedef struct hpgpCtrl
{
    sCtrlLayer ctrlLayer;
    sLinkLayer linkLayer;
    sMuxLayer  muxLayer; 
#if 0
#ifdef SIMU
    sSHAL_CB   simuHal;
#else
    sHaLayer   haLayer; 
#endif
#endif

} sHpgpCtrl, *psHpgpCtrl;


void HPGPCTRL_Init(sHpgpCtrl *hpgpCtrl);
void HPGPCTRL_Proc(sHpgpCtrl *hpgpCtrl);

#endif
#endif


/** =========================================================
 *
 * Edit History
 *
 * $Source: /home/cvsrepo/Hybrii_B_OSFW_Dev/firmware/hpgp/src/hpgpctrl.h,v $
 *
 * $Log: hpgpctrl.h,v $
 * Revision 1.3  2014/11/11 14:52:58  ranjan
 * 1.New Folder Architecture espically in /components
 * 2.Modular arrangment of functionality in new files
 *    anticipating the need for exposing them as FW App
 *    development modules
 * 3.Other improvisation in code and .h files
 *
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
 * Revision 1.2  2013/04/04 12:21:54  prashant
 * Detecting PLC link failure for HMC. added project for HMC and Renesas
 *
 * Revision 1.1  2012/03/11 17:02:24  yuanhua
 * (1) added NEK auth in AKM (2) added NMA (3) modified hpgp layer data structures (4) added Makefile for linux simulation
 *
 * Revision 1.2  2011/09/09 07:02:31  yuanhua
 * migrate the firmware code from the greenchip to the hybrii.
 *
 * Revision 1.2  2011/07/22 18:51:04  yuanhua
 * (1) added the hardware timer tick simulation (hts.h/hts.c) (2) Added semaphors for sync in simulation and defined macro for critical section (3) Fixed some bugs in list.h and CRM (4) Modified and fixed bugs in SHAL (5) Changed SNSM/CNSM and SNAM/CNAM for bug fix (6) Added debugging prints, and more.
 *
 * Revision 1.1  2011/06/23 23:52:42  yuanhua
 * move green.h green.c hpgpapi.h hpgpdef.h hpgpconf.h to src directory
 *
 * Revision 1.1  2011/05/06 19:14:50  kripa
 * Adding nmp files to the new source tree.
 *
 * Revision 1.1  2011/04/08 21:40:41  yuanhua
 * Framework
 *
 *
 * =========================================================*/

