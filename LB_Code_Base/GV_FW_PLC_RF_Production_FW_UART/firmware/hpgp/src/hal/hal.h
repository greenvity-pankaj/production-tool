/** ========================================================
 *
 *  @file hal.h
 * 
 *  @brief Hardware Abstract Layer  
 *
 *  Copyright (C) 2010-2011, Greenvity Communications, Inc.
 *  All Rights Reserved
 *  
 * ==========================================================*/

#ifndef _HAL_H
#define _HAL_H

#include "papdef.h"
#include "list.h"
#include "event.h"
#include "hpgpdef.h"
#ifdef HPGP_HAL
#include "hal_common.h"
#include "hal_hpgp.h"
#endif

#define HAL_BUF_LEN 32

/* three taskes in HAL: Beacon TX,  Beacon RX, and Frame RX */
#define HAL_TASK_BCN_RX         BIT(0)
#define HAL_TASK_FRM_RX         BIT(1)
#define HAL_GetMacAddr(hal)     (hal->macAddr)

#ifdef HPGP_HAL
//typedef struct rxFrmSwDesc sRxFrmSwDesc;
//typedef struct _sHpgpHalCB sHpgpHalCB, *psHpgpHalCB;
#else
#ifdef SIMU
typedef struct _SHAL_CB sSHAL_CB, *spSHAL_CB;
#endif
#endif

typedef struct haLayer
{
    u8  access:       1;
    u8  taskBitMap:   7;   /* bit map for tasks in the hal */

//    u8  snid;

    sSlist   txQueue;  /* tx queue */

	
    u8  macAddr[MAC_ADDR_LEN]; /* MAC address myself */
#ifdef CALLBACK
	
	   /* callback to transmit the beacon from the upper layer */
    void           (*xmitBcn)(void XDATA *cookie);
#endif
    void           *txBcnCookie;

#ifdef CALLBACK
    /* callback to deliver the beacon to the upper layer */
    void           (*procBcn)(void XDATA *cookie, sEvent XDATA *event);
#endif
    void           *bcnCookie;

#ifdef CALLBACK
    /* callback to deliver the mgmt packet to the upper layer */
    void           (*deliverMgmtPacket)(void XDATA *cookie, sEvent XDATA *event);
#endif
    void           *mgmtCookie;
#ifdef CALLBACK
    /* callback to deliver the net mgmt packet to the upper layer */
    void           (*deliverNetMgmtPacket)(void XDATA *cookie, sEvent XDATA *event);
#endif
    void           *netMgmtCookie;

#ifdef HPGP_HAL
#ifdef CALLBACK
    /* callback to deliver the data packet to the upper layer */
    void           (*deliverDataPacket)(void XDATA *cookie, 
                                        sSwFrmDesc XDATA *rxFrmSwDesc);
#endif
    void           *dataCookie;

    sHpgpHalCB     *hhalCb;    
#else
#ifdef SIMU
    /* simulation hal */
    sSHAL_CB       *shal;
    u8             devMode;
#endif
#endif


    /* statistics */
//     u32           frmCnt;    /* frame counter */


} sHaLayer, *psHaLayer;


eStatus  HAL_Init(sHaLayer *hal);
u8       HAL_ProcFrameRx(void *cookie);
u8       HAL_Proc(void *cookie);
void     HAL_RegisterXmitBcnCallback(
            sHaLayer *hal,
            void (*XmitBcnCallback)(void XDATA *cookie),
            void *cookie );

void     HAL_RegisterProcBcnCallback(
            sHaLayer *hal,
            void (*ProcBcnCallback)(void XDATA *cookie, sEvent XDATA *event),
            void *cookie );

void     HAL_RegisterRxMgmtCallback(
            sHaLayer *hal,
            void (*mgmtCallback)(void XDATA *mgmtCookie, sEvent XDATA *event),
            void *mgmtCookie );

void     HAL_RegisterRxNetMgmtCallback(
            sHaLayer *hal,
            void (*netMgmtCallback)(void XDATA *netMgmtCookie, sEvent XDATA *event),
            void *netMgmtCookie );

#ifdef HPGP_HAL
void     HAL_RegisterRxDataCallback(
            sHaLayer  *hal,
            void (*dataCallback)(void XDATA *dataCookie, sSwFrmDesc XDATA *rxFrmSwDesc),
            void *dataCookie );
void     HAL_ScanNet(u8 enable);
#else
void     HAL_SetDevMode(sHaLayer *hal, u8 devMode);  
#endif

eStatus  HAL_TransmitBeacon(sHaLayer *hal, sTxDesc *txdesc,
                            sBuffDesc *buffDesc, u8 bpstoOffset);
eStatus  HAL_Transmit(sHaLayer *hal, sEvent *event);

#ifndef HPGP_HAL_TEST

eStatus  HAL_RecvFrame(sHaLayer *hal, sSwFrmDesc *rxFrmSwDesc);

#endif
void HAL_SetTei(sHaLayer *hal, u8 tei);

#ifndef HPGP_HAL
void HAL_BcnRxIntHandler(void *cookie);
#endif

#ifndef HAL_INT
void HAL_EnablePoll(sHaLayer *hal);
#endif

#endif



/** =========================================================
 *
 * Edit History
 *
 * $Source: /home/cvsrepo/Hybrii_B_OSFW_Dev/firmware/hpgp/src/hal/hal.h,v $
 *
 * $Log: hal.h,v $
 * Revision 1.6  2014/11/11 14:52:58  ranjan
 * 1.New Folder Architecture espically in /components
 * 2.Modular arrangment of functionality in new files
 *    anticipating the need for exposing them as FW App
 *    development modules
 * 3.Other improvisation in code and .h files
 *
 * Revision 1.5  2014/10/28 16:27:43  kiran
 * 1) Software recovery using Watchdog Timer
 * 2) Hardware recovery monitor and policies
 * 3) Timer Polling in Control Task and Frame task for better accuracy
 * 4) Common memory optimized by reducing prints
 * 5) Discovered netlist corruption fixed
 * 6) VCO fix in HHAL_AFEInit()
 * 7) Idata optimized by removing floating point operation
 * 8) Fixed EVENT_TYPE_CC_BCN_IND false indication during association @ CCO
 * 9) Beacon processing protected from interrupts
 * 10) Corrupted Beacons are dropped
 * 11) Some unused arguments removed to improve code size
 *
 * Revision 1.4  2014/07/05 09:16:27  prashant
 * 100 Devices support- only association tested, memory adjustments
 *
 * Revision 1.3  2014/05/28 10:58:59  prashant
 * SDK folder structure changes, Uart changes, removed htm (UI) task
 * Varified - UM, LM - iperf (UDP/TCP), overnight test pass
 *
 * Revision 1.2  2014/01/10 17:15:32  yiming
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
 * Revision 1.4  2013/09/04 14:50:45  yiming
 * New changes for Hybrii_A code merge
 *
 * Revision 1.16  2012/10/26 06:07:03  prashant
 * Remove some warnings from hal_tst project and STA and CCO project
 *
 * Revision 1.15  2012/10/11 06:21:00  ranjan
 * ChangeLog:
 * 1. Added HPGP_MAC_SAP to support linux host data and command path.
 *     define HPGP_MAC_SAP, NMA needs to be added in project.
 *
 * 2. Added 'p ping' command in htm.c . Feature is under AUTO_PING macro.
 *
 * 3. Extended  'p key' command to include PPEK support.
 *
 * verified :
 *   1. Datapath ping works overnite after association,auth
 *   2. HAL TEST project is intact
 *
 * Revision 1.14  2012/09/24 06:01:38  yuanhua
 * (1) Integrate the NMA and HAL in Rx path (2) add a Tx queue in HAL to have less stack size needed in tx path, and Tx in HAL is performed by polling now.
 *
 * Revision 1.13  2012/06/13 06:24:31  yuanhua
 * add code for tx bcn interrupt handler integration and data structures for region entry schedule. But they are not in execution yet.
 *
 * Revision 1.12  2012/06/08 05:50:57  yuanhua
 * added snid function.
 *
 * Revision 1.11  2012/06/05 07:25:59  yuanhua
 * (1) add a scan call to the MAC during the network discovery. (2) add a tiny task in ISM for interrupt polling (3) modify the frame receiving integration. (4) modify the tiny task in STM.
 *
 * Revision 1.10  2012/05/19 20:32:17  yuanhua
 * added non-callback option for the protocol stack.
 *
 * Revision 1.9  2012/05/14 05:22:29  yuanhua
 * support the SCHED without using callback functions.
 *
 * Revision 1.8  2012/05/07 04:17:57  yuanhua
 * (1) updated hpgp Tx integration (2) added Rx poll option
 *
 * Revision 1.7  2012/04/30 04:05:57  yuanhua
 * (1) integrated the HAL mgmt Tx. (2) various updates
 *
 * Revision 1.6  2012/04/25 13:53:40  yuanhua
 * changed the HAL_Transmit prototype.
 *
 * Revision 1.5  2012/04/19 16:46:30  yuanhua
 * fixed some C51 compiler errors for the integration.
 *
 * Revision 1.4  2012/04/17 23:09:50  yuanhua
 * fixed compiler errors for the hpgp hal test due to the integration changes.
 *
 * Revision 1.3  2012/04/13 06:15:11  yuanhua
 * integrate the HPGP protocol stack with the HAL for the beacon TX/RX and mgmt msg RX.
 *
 * Revision 1.2  2011/09/09 07:02:31  yuanhua
 * migrate the firmware code from the greenchip to the hybrii.
 *
 * Revision 1.2  2011/07/22 18:51:04  yuanhua
 * (1) added the hardware timer tick simulation (hts.h/hts.c) (2) Added semaphors for sync in simulation and defined macro for critical section (3) Fixed some bugs in list.h and CRM (4) Modified and fixed bugs in SHAL (5) Changed SNSM/CNSM and SNAM/CNAM for bug fix (6) Added debugging prints, and more.
 *
 * Revision 1.1  2011/05/06 19:09:17  kripa
 * Adding hal layer files to new source tree.
 *
 * Revision 1.2  2011/04/23 19:48:45  kripa
 * Fixing stm.h and event.h inclusion, using relative paths to avoid conflict with windows system header files.
 *
 * Revision 1.1  2011/04/08 21:43:07  yuanhua
 * Framework
 *
 *
 * =========================================================*/

