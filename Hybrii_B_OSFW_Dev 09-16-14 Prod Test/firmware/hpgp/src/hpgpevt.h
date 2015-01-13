/** =======================================================
 * @file hpgpevt.h
 * 
 *  @brief HPGP Event Module
 *
 *  Copyright (C) 2010-2011, Greenvity Communications, Inc.
 *  All Rights Reserved
 *  
 * ========================================================*/


#ifndef _HPGPEVT_H
#define _HPGPEVT_H

#include "papdef.h"
#include "hpgpdef.h"
#include "event.h"



#define EVENT_CLASS_CTRL   0
#define EVENT_CLASS_MSG    1
#define EVENT_CLASS_MGMT   2
#define EVENT_CLASS_DATA   3


//Control events
enum eventType
{
    //
    EVENT_TYPE_RESTART_REQ,
    EVENT_TYPE_RESTART_IND,
    EVENT_TYPE_USTA_IND,
    EVENT_TYPE_ASTA_IND,
    EVENT_TYPE_UCCO_IND,
    EVENT_TYPE_ACCO_IND,   //5 
    EVENT_TYPE_NET_EXIT_REQ,
    EVENT_TYPE_NET_EXIT_CNF, 
    EVENT_TYPE_NET_EXIT_IND,
    EVENT_TYPE_SET_NETWORK_REQ,
    EVENT_TYPE_SET_NETWORK_CNF, //10

    //STA
    EVENT_TYPE_SNSM_START,  
    EVENT_TYPE_SNSM_STOP,
    EVENT_TYPE_SNAM_START,
    EVENT_TYPE_SNAM_STOP,
    EVENT_TYPE_AKM_START,  //15
    EVENT_TYPE_AKM_STOP,
    EVENT_TYPE_CC_BCN_IND,
    EVENT_TYPE_NCO_IND,
    EVENT_TYPE_CCO_DISC_IND,
    EVENT_TYPE_CCO_SLCT_IND,  
    EVENT_TYPE_NET_DISC_IND,  //21 new
    EVENT_TYPE_NET_ACC_REQ,
    EVENT_TYPE_NET_ACC_RSP,
    EVENT_TYPE_NET_LEAVE_REQ,
    EVENT_TYPE_NET_LEAVE_RSP, 
    EVENT_TYPE_NET_LEAVE_IND,//26
    EVENT_TYPE_AUTH_REQ,
    EVENT_TYPE_AUTH_RSP,
    EVENT_TYPE_AUTH_IND,
    EVENT_TYPE_ASSOC_IND,
    EVENT_TYPE_CCO_SELECT_IND, //31
    EVENT_TYPE_CCO_HO_REQ,  
    EVENT_TYPE_CCO_HO_RSP,
    EVENT_TYPE_CCO_HO_IND,
    EVENT_TYPE_CCO_APPOINT_REQ, 
    EVENT_TYPE_CCO_APPOINT_RSP, //36
    EVENT_TYPE_CCO_LOST_IND,
    EVENT_TYPE_BCN_MISS_IND,
    EVENT_TYPE_TIMER_BBT_IND, 
    EVENT_TYPE_TIMER_USTT_IND,
    EVENT_TYPE_TIMER_ACC_IND,   
    EVENT_TYPE_TIMER_APPT_IND, //42
    EVENT_TYPE_TIMER_DISC_AGING_IND,
    EVENT_TYPE_TIMER_BEACON_LOSS_IND,
    EVENT_TYPE_TIMER_TEI_IND,        //TEI lease timer STA and CCO
    EVENT_TYPE_TIMER_KEY_IND,
//    EVENT_TYPE_TIMER_TEI_REUSE_IND,  //TEI reuse timer
    EVENT_TYPE_TIMER_TEK_IND,
    //CCO
    EVENT_TYPE_CNSM_START,   //47
    EVENT_TYPE_CNSM_STOP_REQ,    
    EVENT_TYPE_CNSM_STOP_CNF,    
    EVENT_TYPE_CNAM_STOP_REQ,    
    EVENT_TYPE_CNAM_STOP_CNF,
    EVENT_TYPE_BCN_TX_IND,  
    EVENT_TYPE_NET_ACC_IND,  
    EVENT_TYPE_NO_STA_IND,	 //54
    EVENT_TYPE_TIMER_DISC_IND,
    EVENT_TYPE_TIMER_BCN_TX_IND,
//    EVENT_TYPE_TIMER_TEI_IND,
    EVENT_TYPE_TIMER_JOIN_IND, 
    EVENT_TYPE_TIMER_STA_IND,
    EVENT_TYPE_TIMER_HO_IND,   //59

    // Host
    EVENT_TYPE_SET_SEC_MODE,

	/*Frame Receive Interrupt*/
	EVENT_TYPE_FRAME_RX,
#ifdef POWERSAVE
	/* Power Save events */
    EVENT_TYPE_SPSM_START,
    EVENT_TYPE_PPSM_START,
	EVENT_TYPE_TIMER_ACK_IND,  //64
	EVENT_TYPE_STA_START_PS,
	EVENT_TYPE_STA_STOP_PS,
	EVENT_TYPE_STA_PS_EXIT_REQ,
	EVENT_TYPE_CCO_SND_STOP_PS_REQ,
#endif
#ifdef ROUTE
	EVENT_TYPE_ROUTE_HD_DURATION_TIMEOUT, 
    EVENT_TYPE_ROUTE_UPDATE_TIMEOUT,    //70
#endif
    EVENT_TYPE_AUTH_CPLT,
    EVENT_TYPE_IDENTIFY_CAP_TIMEOUT,
    EVENT_TYPE_STA_AGEOUT,
};

//message event
#define    EVENT_TYPE_CC_CCO_APPOINT_REQ   MMTYPE_CC_CCO_APPOINT_REQ  
#define    EVENT_TYPE_CC_CCO_APPOINT_CNF   MMTYPE_CC_CCO_APPOINT_CNF 
#define    EVENT_TYPE_CC_CCO_APPOINT_IND   MMTYPE_CC_CCO_APPOINT_IND 
#define    EVENT_TYPE_CC_CCO_APPOINT_RSP   MMTYPE_CC_CCO_APPOINT_RSP

#define    EVENT_TYPE_CC_BACKUP_APPOINT_REQ  MMTYPE_CC_BACKUP_APPOINT_REQ
#define    EVENT_TYPE_CC_BACKUP_APPOINT_CNF  MMTYPE_CC_BACKUP_APPOINT_CNF


#define    EVENT_TYPE_CC_HANDOVER_REQ       MMTYPE_CC_HANDOVER_REQ
#define    EVENT_TYPE_CC_HANDOVER_CNF       MMTYPE_CC_HANDOVER_CNF 
#define    EVENT_TYPE_CC_HANDOVER_RSP       MMTYPE_CC_HANDOVER_RSP 
#define    EVENT_TYPE_CC_HANDOVER_INFO_IND  MMTYPE_CC_HANDOVER_INFO_IND
#define    EVENT_TYPE_CC_HANDOVER_INFO_RSP  MMTYPE_CC_HANDOVER_INFO_RSP 

#define    EVENT_TYPE_CC_DISCOVER_LIST_REQ  MMTYPE_CC_DISCOVER_LIST_REQ 
#define    EVENT_TYPE_CC_DISCOVER_LIST_CNF  MMTYPE_CC_DISCOVER_LIST_CNF 
#define    EVENT_TYPE_CC_DISCOVER_LIST_IND  MMTYPE_CC_DISCOVER_LIST_IND 
#define    EVENT_TYPE_CC_DISCOVER_LIST_RSP  MMTYPE_CC_DISCOVER_LIST_RSP 

#define    EVENT_TYPE_CC_WHO_RU_REQ         MMTYPE_CC_WHO_RU_REQ
#define    EVENT_TYPE_CC_WHO_RU_CNF         MMTYPE_CC_WHO_RU_CNF

#define    EVENT_TYPE_CC_ASSOC_REQ          MMTYPE_CC_ASSOC_REQ
#define    EVENT_TYPE_CC_ASSOC_CNF          MMTYPE_CC_ASSOC_CNF

#define    EVENT_TYPE_CC_LEAVE_REQ          MMTYPE_CC_LEAVE_REQ
#define    EVENT_TYPE_CC_LEAVE_CNF          MMTYPE_CC_LEAVE_CNF
#define    EVENT_TYPE_CC_LEAVE_IND          MMTYPE_CC_LEAVE_IND
#define    EVENT_TYPE_CC_LEAVE_RSP          MMTYPE_CC_LEAVE_RSP

#define    EVENT_TYPE_CC_SET_TEI_MAP_REQ    MMTYPE_CC_SET_TEI_MAP_REQ
#define    EVENT_TYPE_CC_SET_TEI_MAP_IND    MMTYPE_CC_SET_TEI_MAP_IND

#define    EVENT_TYPE_CC_HP1_DET_REQ        MMTYPE_CC_HP1_DET_REQ 
#define    EVENT_TYPE_CC_HP1_DET_CNF        MMTYPE_CC_HP1_DET_CNF 

#define    EVENT_TYPE_CC_ISP_DET_REPORT_IND MMTYPE_CC_ISP_DET_REPORT_IND 

#define    EVENT_TYPE_CC_ISP_START_RESYNC_REQ   MMTYPE_CC_ISP_START_RESYNC_REQ
#define    EVENT_TYPE_CC_ISP_FINISH_RESYNC_REQ  MMTYPE_CC_ISP_FINISH_RESYNC_REQ
#define    EVENT_TYPE_CC_ISP_RESYNC_DET_IND MMTYPE_CC_ISP_RESYNC_DET_IND 
#define    EVENT_TYPE_CC_ISP_RESYNC_TX_REQ  MMTYPE_CC_ISP_RESYNC_TX_REQ   

#define    EVENT_TYPE_CC_PWR_SAVE_REQ       MMTYPE_CC_PWR_SAVE_REQ 
#define    EVENT_TYPE_CC_PWR_SAVE_CNF       MMTYPE_CC_PWR_SAVE_CNF 
#define    EVENT_TYPE_CC_PWR_SAVE_EXIT_REQ  MMTYPE_CC_PWR_SAVE_EXIT_REQ 
#define    EVENT_TYPE_CC_PWR_SAVE_EXIT_CNF  MMTYPE_CC_PWR_SAVE_EXIT_CNF 
#define    EVENT_TYPE_CC_PWR_SAVE_LIST_REQ  MMTYPE_CC_PWR_SAVE_LIST_REQ 
#define    EVENT_TYPE_CC_PWR_SAVE_LIST_CNF  MMTYPE_CC_PWR_SAVE_LIST_CNF 
#define    EVENT_TYPE_CC_STOP_PWR_SAVE_REQ  MMTYPE_CC_STOP_PWR_SAVE_REQ 
#define    EVENT_TYPE_CC_STOP_PWR_SAVE_CNF  MMTYPE_CC_STOP_PWR_SAVE_CNF 

#define    EVENT_TYPE_CM_UNASSOC_STA_IND    MMTYPE_CM_UNASSOC_STA_IND

#define    EVENT_TYPE_CM_ENCRY_PAYLOAD_IND  MMTYPE_CM_ENCRY_PAYLOAD_IND
#define    EVENT_TYPE_CM_ENCRY_PAYLOAD_RSP  MMTYPE_CM_ENCRY_PAYLOAD_RSP

#define    EVENT_TYPE_CM_SET_KEY_REQ        MMTYPE_CM_SET_KEY_REQ
#define    EVENT_TYPE_CM_SET_KEY_CNF        MMTYPE_CM_SET_KEY_CNF
#define    EVENT_TYPE_CM_GET_KEY_REQ        MMTYPE_CM_GET_KEY_REQ
#define    EVENT_TYPE_CM_GET_KEY_CNF        MMTYPE_CM_GET_KEY_CNF

#define    EVENT_TYPE_CM_SC_JOIN_REQ        MMTYPE_CM_SC_JOIN_REQ 
#define    EVENT_TYPE_CM_SC_JOIN_CNF        MMTYPE_CM_SC_JOIN_CNF 

#define    EVENT_TYPE_CM_CHAN_EST_IND       MMTYPE_CM_CHAN_EST_IND

#define    EVENT_TYPE_CM_BRG_INFO_REQ       MMTYPE_CM_BRG_INFO_REQ
#define    EVENT_TYPE_CM_BRG_INFO_CNF       MMTYPE_CM_BRG_INFO_CNF
#ifdef ROUTE
#define    EVENT_TYPE_CM_ROUTE_INFO_REQ     MMTYPE_CM_ROUTE_INFO_REQ
#define    EVENT_TYPE_CM_ROUTE_INFO_CNF     MMTYPE_CM_ROUTE_INFO_CNF
#define    EVENT_TYPE_CM_ROUTE_INFO_IND     MMTYPE_CM_ROUTE_INFO_IND
#define    EVENT_TYPE_CM_UNREACHABLE_IND    MMTYPE_CM_UNREACHABLE_IND
#endif

#define    EVENT_TYPE_CM_STA_IDENTIFY_REQ   MMTYPE_CM_STA_IDENTIFY_REQ
#define    EVENT_TYPE_CM_STA_IDENTIFY_CNF   MMTYPE_CM_STA_IDENTIFY_CNF
#define    EVENT_TYPE_CM_STA_IDENTIFY_IND   MMTYPE_CM_STA_IDENTIFY_IND
#define    EVENT_TYPE_CM_STA_IDENTIFY_RSP   MMTYPE_CM_STA_IDENTIFY_RSP

#define    EVENT_TYPE_NN_INL_REQ			MMTYPE_NN_INL_REQ
#define    EVENT_TYPE_NN_INL_CNF			MMTYPE_NN_INL_CNF



/* hpgp event sub header */
typedef struct hpgpHdr
{
    u8       tei;             /* source(RX)/destination(TX) TEI */
    u8       *macAddr;        /* source(Rx)/destination(Tx) MAC address */

    u8       snid:      4;    /* snid */
    u8       eks:       4;    /* EKS (encryption key select  */

    u8       mnbc:      1;    /* multi-network broadcast */
    u8       plid:      2;    /* priority link id */
    u8       rsvd:      4;
	u8       mcst:       1;
    void    *scb;             /* associated STA control block */
} sHpgpHdr, *psHpgpHdr;



typedef struct snsmStartEvent
{
    u8    staType;
} sSnsmStartEvent, *psSnsmStartEvent;

typedef  sSnsmStartEvent    sSnamStartEvent;

typedef struct ccoApptReqEvent
{
    u8    reqType;
    u8    macAddr[MAC_ADDR_LEN];
} sCcoApptReqEvent, *psCcoApptReqEvent;

typedef  struct ccoApptRspEvent
{
    u8    result;
} sCcoApptRspEvent, *psCcoApptRspEvent;


typedef  struct netAccRspEvent
{
    u8    result;
} sNetAccRspEvent, *psNetAccRspEvent;



typedef struct ccoHoReqEvent
{
    u8    reason;
} sCcoHoReqEvent, *psCcoHoReqEvent;

typedef struct ccoHoRspEvent
{
    u8    reason;
    u8    result;
} sCcoHoRspEvent, *psCcoHoRspEvent;

typedef struct netLeaveIndEvent
{
    u8    reason;
} sNetLeaveIndEvent, *psNetLeaveIndEvent;

typedef struct ccoLostIndEvent
{
    u8    reason;
} sCcoLostIndEvent, *psCcoLostIndEvent;


enum
{
    AUTH_TYPE_NEK,
    AUTH_TYPE_NMK,
};

typedef struct authReq
{
    u8    authType;
    u8    dak[ENC_KEY_LEN];
    u8    nmk[ENC_KEY_LEN];
    u8    macAddr[MAC_ADDR_LEN];
    u8    avlnStatus;
} sAuthReq, *psAuthReq;

typedef struct authInd
{
    u8   keyType;
    u8   secMode;
    u8   result;
} sAuthInd, *psAuthInd;

typedef struct authRsp
{
    u8    authType;
    u8    result;
} sAuthRsp, *psAuthRsp;

typedef struct akmStart 
{
    u8    mode;
    u8    newNek;
} sAkmStart, *psAkmStart;


union eventParam 
{
    sSnsmStartEvent         snsmStart;
    sAkmStart               akmStart;
    sNetAccRspEvent         netAccRsp;
    sNetLeaveIndEvent       netLeaveInd;
    sCcoApptReqEvent        ccoApptReq;
    sCcoApptRspEvent        ccoApptRsp;
    sCcoHoReqEvent          ccoHoReq;
    sCcoHoRspEvent          ccoHoRsp;
    sAuthReq                authReq;
    sAuthRsp                authRsp;
    sAuthInd                authInd;
};

typedef union eventParam uEventBody;
typedef union eventParam uEventParam;



#define EVENT_DEFAULT_SIZE       0

#define HPGP_MSG_HEADROOM        MAX(sizeof(sHpgpHdr), HPGP_MF_OPT_HDR_LEN)

#ifdef SIMU 
#define EVENT_HPGP_MSG_HEADROOM  (sizeof(sEthHdr)+sizeof(sMmHdr)+sizeof(sTxRxDesc)+sizeof(sHpgpHdr))       
#else
#define EVENT_HPGP_MSG_HEADROOM  (sizeof(sEthHdr)+sizeof(sMmHdr)+HPGP_MSG_HEADROOM)       
#endif

#define EVENT_HPGP_CTRL_HEADROOM  (sizeof(sHpgpHdr))       


#endif


/** =========================================================
 *
 * Edit History
 *
 * $Source: /home/cvsrepo/Hybrii_B_OSFW_Dev/firmware/hpgp/src/hpgpevt.h,v $
 *
 * $Log: hpgpevt.h,v $
 * Revision 1.12  2014/09/05 09:28:18  ranjan
 * 1. uppermac cco-sta switching feature fix
 * 2. general stability fixes for many station associtions
 * 3. changed mgmt memory pool for many STA support
 *
 * Revision 1.11  2014/08/25 07:37:34  kiran
 * 1) RSSI & LQI support
 * 2) Fixed Sync related issues
 * 3) Fixed timer 0 timing drift for SDK
 * 4) MMSG & Error Logging in Flash
 *
 * Revision 1.10  2014/07/04 03:53:08  tri
 * Updated event #s
 *
 * Revision 1.9  2014/06/12 13:15:43  ranjan
 * -separated bcn,mgmt,um event pools
 * -fixed datapath issue due to previous checkin
 * -work in progress. neighbour cco detection
 *
 * Revision 1.8  2014/05/28 10:58:59  prashant
 * SDK folder structure changes, Uart changes, removed htm (UI) task
 * Varified - UM, LM - iperf (UDP/TCP), overnight test pass
 *
 * Revision 1.7  2014/05/21 23:04:21  tri
 * more PS
 *
 * Revision 1.6  2014/03/12 09:41:22  ranjan
 * 1. added ageout event to cco cnam,backupcco ageout handling
 * 2.  fix linking issue in zb_lx51_asic due to backup cco checkin
 *
 * Revision 1.5  2014/03/10 05:58:10  ranjan
 * 1. added HomePlug BackupCCo feature. verified C&I test.(passed.) (bug 176)
 *
 * Revision 1.4  2014/02/27 10:42:47  prashant
 * Routing code added
 *
 * Revision 1.3  2014/01/28 17:42:14  tri
 * Added Power Save code
 *
 * Revision 1.2  2014/01/10 17:13:09  yiming
 * check in Rajan 1/8/2014 code release
 *
 * Revision 1.4  2014/01/08 10:53:54  ranjan
 * Changes for LM OS support.
 * New Datapath FrameTask
 * LM and UM  datapath, feature verified.
 *
 * known issues : performance numbers needs revisit
 *
 * review : pending.
 *
 * Revision 1.3  2013/09/04 14:49:33  yiming
 * New changes for Hybrii_A code merge
 *
 * Revision 1.10  2013/07/12 08:56:36  ranjan
 * -UKE Push Button Security Feature.
 * Verified : DirectEntry Security Works.Datapath Works.
 *                 command SetSecMode for UKE works.
 * Added against bug-160
 *
 * Revision 1.9  2013/04/17 13:00:59  ranjan
 * Added FW ready event, Removed hybrii header from datapath, Modified hybrii header
 *  formate
 *
 * Revision 1.8  2013/03/14 11:49:18  ranjan
 * 1.handled cases  for CCo toSTA switch and  viceversa
 * 2.UM uses bcntemplate
 *
 * Revision 1.7  2012/09/11 05:00:06  yuanhua
 * fixed an memory leak in NMA
 *
 * Revision 1.6  2012/07/08 18:42:20  yuanhua
 * (1)fixed some issues when ctrl layer changes its state from the UCC to ACC. (2) added a event CNSM_START.
 *
 * Revision 1.5  2012/05/07 04:17:57  yuanhua
 * (1) updated hpgp Tx integration (2) added Rx poll option
 *
 * Revision 1.4  2012/04/30 04:05:57  yuanhua
 * (1) integrated the HAL mgmt Tx. (2) various updates
 *
 * Revision 1.3  2012/04/13 06:15:11  yuanhua
 * integrate the HPGP protocol stack with the HAL for the beacon TX/RX and mgmt msg RX.
 *
 * Revision 1.2  2012/03/11 17:02:24  yuanhua
 * (1) added NEK auth in AKM (2) added NMA (3) modified hpgp layer data structures (4) added Makefile for linux simulation
 *
 * Revision 1.1  2011/09/09 07:02:31  yuanhua
 * migrate the firmware code from the greenchip to the hybrii.
 *
 * Revision 1.2  2011/08/12 23:13:21  yuanhua
 * (1)Added Control Layer (2) Fixed bugs for user-selected CCo handover (3) Made changes to SNAM/CNAM and SNSM/CNSM for CCo handover switch (from CCo to STA, from STA to CCo, and from STA to STA but with different CCo) and post CCo handover
 *
 * Revision 1.1  2011/08/09 22:45:44  yuanhua
 * changed to event structure, seperating HPGP-related events from the general event defination so that the general event could be used for other purposes than the HPGP.
 *
 * Revision 1.9  2011/08/08 22:05:41  yuanhua
 * user-selected CCo handover fix
 *
 * Revision 1.8  2011/08/05 17:06:29  yuanhua
 * (1) added an internal queue in Link Layer for communication btw modules within Link Layer (2) Fixed bugs in CCo Handover. Now, CCo handover could be triggered by auto CCo selection, CCo handover messages work fine (3) Made some modifications in SHAL.
 *
 * Revision 1.7  2011/08/02 16:06:00  yuanhua
 * (1) Fixed a bug in STM (2) Made STA discovery work according to the standard, including aging timer. (3) release the resource after the STA leave (4) STA will switch to the backup CCo if the CCo failure occurs (5) At this point, the CCo could work with multiple STAs correctly, including CCo association/leave, TEI renew, TEI map updating, discovery beacon scheduling, discovery STA list updating ang aging, CCo failure, etc.
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
 * Revision 1.2  2011/05/28 06:23:56  kripa
 * combining list header files
 *
 * Revision 1.1  2011/05/06 18:31:47  kripa
 * Adding common utils and isr files for Greenchip firmware.
 *
 * Revision 1.3  2011/04/23 23:11:39  kripa
 * sEvent Struct; renamed 'u8* data' to 'u8* eventData', since data is reserved word in Keil.
 *                changed 'u8 buff[0]' to u8 buff[1]', since Keil doesnt allow empty array field.
 *
 * Revision 1.2  2011/04/23 17:11:59  kripa
 * Renamed 'u8 class' field in 'struct event' to 'eventClass'; 'class' is a keyword in VC.
 *
 * Revision 1.1  2011/04/08 21:40:41  yuanhua
 * Framework
 *
 *
 * ========================================================*/


