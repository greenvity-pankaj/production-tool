/** =========================================================
 *
 *  @file linkl.h
 * 
 *  @brief Link Layer  
 *
 *  Copyright (C) 2010-2012, Greenvity Communications, Inc.
 *  All Rights Reserved
 *  
 * ==========================================================*/

#ifndef _LINKL_H
#define _LINKL_H

#include "list.h"
#include "papdef.h"
#include "sched.h"
#include "hpgpevt.h"

#include "hpgpconf.h" //sta configuration

#include "crm.h"
#include "nsm.h"
#include "nam.h"
#ifdef POWERSAVE
#include "psm.h"
#endif
#include "akm.h"
#include "hom.h"
#include "muxl.h"
#include "hal.h"
//typedef struct muxLayer  sMuxLayer, *psMuxLayer;
//typedef struct haLayer   sHaLayer,  *psHaLayer;

#define LINKL_STA_MODE_STA   	0
#define LINKL_STA_MODE_CCO   	1
#define LINKL_STA_MODE_SNIFFER 	2

enum
{
    LINKL_STA_TYPE_SC_JOIN, 
    LINKL_STA_TYPE_SC_ADD, 
    LINKL_STA_TYPE_NETDISC, 
    LINKL_STA_TYPE_UNASSOC, 
	LINKL_STA_TYPE_UNASSOC_PASSIVE,
    LINKL_STA_TYPE_ASSOC, 
//    LINKL_STA_TYPE_HO, 
};

enum
{
    LINKL_CCO_TYPE_UNASSOC, 
    LINKL_CCO_TYPE_ASSOC, 
    LINKL_CCO_TYPE_HO, 
};

enum linkTrans
{
    LINKL_TRANS_UNKNOWN,
    /* Transaction for STA */
    LINKL_TRANS_SNSM,
    LINKL_TRANS_SNAM,
    LINKL_TRANS_SHOM,
#ifdef POWERSAVE
    LINKL_TRANS_SPSM,
#endif

    /* Transaction for CCO */
    LINKL_TRANS_CNSM,
    LINKL_TRANS_CNAM,
    LINKL_TRANS_CHOM,
#ifdef POWERSAVE
    LINKL_TRANS_CPSM,
#endif

    /* Transaction for both */
    LINKL_TRANS_AKM,
};






typedef struct linkLayer
{
    u8       mode;        //Link Layer mode

    /* STA mode */
    sSnsm    staNsm;      //STA network system manager
    sSnam    staNam;      //STA network access manager
    sShom    staHom;      //STA handover manager
#ifdef POWERSAVE
    sSpsm    staPsm;      //STA Power Save manager
#endif

    /* CCO mode */
    sCnsm    ccoNsm;      //CCO network system manager
    sCnam    ccoNam;      //CCO network access manager
    sChom    ccoHom;      //CCO handover manager
#ifdef POWERSAVE
    sCpsm    ccoPsm;      //CCO Power Save manager
#endif

    sAkm     akm;         //auth and key manager
    sCrm     ccoRm;       //central resouce manager

    sStaInfo  staInfo;      //STA configuration Information

//    sCcoInfo  ccoInfo;      //CCO configuration Information

#ifndef P8051
#if defined(WIN32) || defined(_WIN32)
    HANDLE   linkSem;
#else //POSIX
    sem_t    linkSem;
#endif
#endif

    sSlist   eventQueue;  //external event queue 
    sSlist   intEventQueue;  //internal event queue 

  //  void     (*deliverEvent)(void XDATA *eventcookie, sEvent XDATA *event);
    void     *eventcookie;

//    sExecTask  task;

    sMuxLayer *muxl;
    sHaLayer  *hal;

} sLinkLayer, *psLinkLayer;



void LINKL_RegisterEventCallback(sLinkLayer *linkl, 
    void (*deliverEvent)(void XDATA *eventcookie, sEvent XDATA *event),
    void *eventcookie);


u8 LINKL_Proc(void *linkl);

eStatus LINKL_Init(sLinkLayer *linkl);
u8 LINKL_GetMode(sLinkLayer *linkLayer);


void LINKL_SetCCoCap(sLinkLayer *linkLayer, u8 ccoCap);
//sStaInfo* LINKL_GetStaInfo(sLinkLayer *linkLayer);
//sCrm*  LINKL_GetCrm(sLinkLayer *linkLayer);
//sCnsm* LINKL_GetCnsm(sLinkLayer *linkLayer);
//sSnsm* LINKL_GetSnsm(sLinkLayer *linkLayer);
//sCnam* LINKL_GetCnam(sLinkLayer *linkLayer);
//sSnam* LINKL_GetSnam(sLinkLayer *linkLayer);
//sCcoInfo* LINKL_GetCcoInfo(sLinkLayer *linkLayer);

#define LINKL_GetStaInfo(linkl)    (&linkl->staInfo)

#define LINKL_GetCrm(linkl)        (&linkl->ccoRm)
#define LINKL_GetCnsm(linkl)       (&linkl->ccoNsm)
#define LINKL_GetSnsm(linkl)       (&linkl->staNsm)
#define LINKL_GetCnam(linkl)       (&linkl->ccoNam)
#define LINKL_GetSnam(linkl)       (&linkl->staNam)

eStatus LINKL_SendMgmtMsg(sStaInfo *staInfo, u16 mmType, u8 *macAddr);
eStatus LINKL_SetKey(sLinkLayer *linkLayer, u8 *nmk, u8 *nid);
eStatus LINKL_GetKey(sLinkLayer *linkLayer, u8 *nmk, u8 *nid);


void LINKL_FillHpgpHdr(sHpgpHdr *hpgpHdr, u8 tei, u8 *macAddr, u8 snid, u8 mnbc,
                          u8 eks);


eStatus LINKL_SendEvent(sLinkLayer *linkl, sEvent *event) __REENTRANT__;


/* ==================
 *   STA mode API 
 * =================*/

void    LINKL_SetStaMode(sLinkLayer *linkLayer);
eStatus LINKL_BcnUpdateActive();

void    LINKL_StartSta(sLinkLayer *linkLayer, u8 staType);
void    LINKL_StopSta(sLinkLayer *linkLayer);

u8      LINKL_DetermineStaRole(sLinkLayer *linkLayer);

void    LINKL_EnableAssocNotification(sLinkLayer *linkLayer);

void    LINKL_CcoHandover(sLinkLayer *linkLayer);

eStatus LINKL_SetSecurityMode(sLinkLayer *linkLayer, u8 secMode);
eStatus LINKL_GetSecurityMode(sLinkLayer *linkLayer, u8* secMode);
eStatus LINKL_SetKey(sLinkLayer *linkLayer, u8 *nmk, u8 *nid);
eStatus LINKL_GetKey(sLinkLayer *linkLayer, u8 *nmk, u8 *nid);
eStatus LINKL_SetLineMode(sLinkLayer *linkLayer, eLineMode lineMd);

eStatus LINKL_StartAuth(sLinkLayer *linkLayer, u8 *nmk, u8 *dak, u8* macAddr, u8 sl);
eStatus LINKL_ApptCCo(sLinkLayer *linkLayer, u8 *macAddr, u8 reqType);
eStatus LINKL_SetPpKeys(sLinkLayer *linkLayer, u8 *ppEks, u8 *ppek, u8* macAddr);
void LINKL_CommitStaProfile(sLinkLayer *linkLayer);


/* ==================
 *   CCO mode API 
 * =================*/

void    LINKL_SetCcoMode(sLinkLayer *linkLayer);

void    LINKL_StartCco(sLinkLayer *linkLayer, u8 discEnable);
eStatus LINKL_StopCco(sLinkLayer *linkLayer);
void LINKL_PostStopCCo(sLinkLayer *linkLayer);

u8      LINKL_QueryAnySta(sLinkLayer *linkLayer);

u8      LINKL_QueryAnyAlvn(sLinkLayer *linkLayer);
void LINKL_SetBcnInit(void);
void LINKL_UpdateBeacon();



#endif


/** =========================================================
 *
 * Edit History
 *
 * $Source: /home/cvsrepo/Hybrii_B_OSFW_Dev/firmware/hpgp/src/link/linkl.h,v $
 *
 * $Log: linkl.h,v $
 * Revision 1.7  2014/07/05 09:16:27  prashant
 * 100 Devices support- only association tested, memory adjustments
 *
 * Revision 1.6  2014/06/24 16:26:45  ranjan
 * -zigbee frame_handledata fix.
 * -added reason code for uppermac host events
 * -small cleanups
 *
 * Revision 1.5  2014/06/19 17:13:19  ranjan
 * -uppermac fixes for lvnet and reset command for cco and sta mode
 * -backup cco working
 *
 * Revision 1.4  2014/05/16 08:52:30  kiran
 * - System Profile Flashing API's Added. Upper MAC functionality tested
 *
 * Revision 1.3  2014/02/19 10:22:41  ranjan
 * - common sync for hal_tst and upper mac project
 * - ism.c is MAC interrupt handler for hhal_tst and upper mac.
 *    chal_ext1isr function   is removed
 * - verified : lower mac sync, upper mac sync data traffic.
 *
 * Revision 1.2  2014/01/28 17:46:02  tri
 * Added Power Save code
 *
 * Revision 1.1  2013/12/18 17:05:23  yiming
 * no message
 *
 * Revision 1.1  2013/12/17 21:47:56  yiming
 * no message
 *
 * Revision 1.4  2013/09/04 14:51:01  yiming
 * New changes for Hybrii_A code merge
 *
 * Revision 1.13  2013/07/12 08:56:36  ranjan
 * -UKE Push Button Security Feature.
 * Verified : DirectEntry Security Works.Datapath Works.
 *                 command SetSecMode for UKE works.
 * Added against bug-160
 *
 * Revision 1.12  2013/03/26 12:07:26  ranjan
 * -added  host sw reset command
 * - fixed issue in bcn update
 *
 * Revision 1.11  2013/02/05 10:19:57  ranjan
 * Fix compilation issue and unresolved extern
 *
 * Revision 1.10  2012/10/25 11:38:48  prashant
 * Sniffer code added for MAC_SAP, Added new commands in MAC_SAP for sniffer, bridge,
 *  hardware settings and peer information.
 *
 * Revision 1.9  2012/06/29 03:02:21  kripa
 *
 * Committed on the Free edition of March Hare Software CVSNT Client.
 * Upgrade to CVS Suite for more features and support:
 * http://march-hare.com/cvsnt/
 *
 * Revision 1.8  2012/06/15 04:35:21  yuanhua
 * add a STA type of passive unassoc STA. With this STA type, the device acts as a STA during the network discovery. It performs the network scan for beacons from the CCO, but does not transmit the UNASSOC_STA.IND and does not involve in the CCO selection process. Thus, it joins the existing network.
 *
 * Revision 1.7  2012/05/24 05:08:18  yuanhua
 * define sendEvent functions in CTRL/LINK layer as reentrant.
 *
 * Revision 1.6  2012/04/17 23:09:50  yuanhua
 * fixed compiler errors for the hpgp hal test due to the integration changes.
 *
 * Revision 1.5  2012/04/13 06:15:11  yuanhua
 * integrate the HPGP protocol stack with the HAL for the beacon TX/RX and mgmt msg RX.
 *
 * Revision 1.4  2012/03/11 17:02:25  yuanhua
 * (1) added NEK auth in AKM (2) added NMA (3) modified hpgp layer data structures (4) added Makefile for linux simulation
 *
 * Revision 1.3  2011/09/18 01:32:08  yuanhua
 * designed the AKM for both STA and CCo.
 *
 * Revision 1.2  2011/09/09 07:02:31  yuanhua
 * migrate the firmware code from the greenchip to the hybrii.
 *
 * Revision 1.11  2011/08/12 23:13:22  yuanhua
 * (1)Added Control Layer (2) Fixed bugs for user-selected CCo handover (3) Made changes to SNAM/CNAM and SNSM/CNSM for CCo handover switch (from CCo to STA, from STA to CCo, and from STA to STA but with different CCo) and post CCo handover
 *
 * Revision 1.10  2011/08/09 22:45:44  yuanhua
 * changed to event structure, seperating HPGP-related events from the general event defination so that the general event could be used for other purposes than the HPGP.
 *
 * Revision 1.9  2011/08/05 17:06:29  yuanhua
 * (1) added an internal queue in Link Layer for communication btw modules within Link Layer (2) Fixed bugs in CCo Handover. Now, CCo handover could be triggered by auto CCo selection, CCo handover messages work fine (3) Made some modifications in SHAL.
 *
 * Revision 1.8  2011/07/30 02:43:35  yuanhua
 * (1) Split the beacon process into two parts: one requiring an immdiate response, the other tolerating the delay (2) Changed the API btw the MUX and SHAL for packet reception (3) Fixed bugs in various modules. Now, multiple STAs could successfully associate/leave the CCo
 *
 * Revision 1.7  2011/07/22 18:51:04  yuanhua
 * (1) added the hardware timer tick simulation (hts.h/hts.c) (2) Added semaphors for sync in simulation and defined macro for critical section (3) Fixed some bugs in list.h and CRM (4) Modified and fixed bugs in SHAL (5) Changed SNSM/CNSM and SNAM/CNAM for bug fix (6) Added debugging prints, and more.
 *
 * Revision 1.6  2011/07/16 17:11:23  yuanhua
 * (1)Implemented SHOM and CHOM modules, including handover procedure, SCB resource updating for HO (2) Update SNAM and CNAM modules to support uer-appointed CCo handover (3) Made the SCB resources to support the TEI MAP for the STA mode and management of associated STA resources (e.g. TEI) (4) Modified SNSM and CNSM to perform all types of handover switch (CCo handover to the new STA, STA taking over the CCo, STA switching to the new CCo)
 *
 * Revision 1.5  2011/07/08 22:23:48  yuanhua
 * (1) Implemented CNSM, including its state machine, beacon transmission and process, discover beacon scheduling, auto CCo selection, discover list, handover countdown, etc. (2) Updated SNSM, including discover list processing, triggering a switch to the new CCo, etc. (3) Updated CNAM and SNAM, adding the connection state in the SNAM, switch to the new CCo, etc. (4) Other updates
 *
 * Revision 1.4  2011/07/02 22:09:01  yuanhua
 * Implemented both SNAM and CNAM modules, including network join and leave procedures, systemm resource (such as TEI) allocation and release, TEI renew/release timers, and TEI reuse timer, etc.
 *
 * Revision 1.3  2011/06/24 14:33:18  yuanhua
 * (1) Changed event structure (2) Implemented SNSM, including the state machines in network discovery and connection states, becaon process, discover process, and handover detection (3) Integrated the HPGP and SHAL
 *
 * Revision 1.2  2011/05/28 06:31:19  kripa
 * Combining corresponding STA and CCo modules.
 *
 * Revision 1.1  2011/05/06 19:10:12  kripa
 * Adding link layer files to new source tree.
 *
 * Revision 1.1  2011/04/08 21:42:45  yuanhua
 * Framework
 *
 *
 * =========================================================*/

