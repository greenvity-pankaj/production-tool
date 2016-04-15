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
	EVENT_TYPE_ACCO_IND,    //5

	//STA
	EVENT_TYPE_SNSM_START,
	EVENT_TYPE_SNSM_STOP,
	EVENT_TYPE_SNAM_START,
	EVENT_TYPE_SNAM_STOP,
	EVENT_TYPE_CC_BCN_IND,  //10
	EVENT_TYPE_CCO_DISC_IND,
	EVENT_TYPE_CCO_SLCT_IND,  
	EVENT_TYPE_NET_DISC_IND, 
	EVENT_TYPE_NET_ACC_REQ,
	EVENT_TYPE_NET_ACC_RSP, //15
	EVENT_TYPE_NET_LEAVE_REQ,
	EVENT_TYPE_NET_LEAVE_RSP, 
	EVENT_TYPE_NET_LEAVE_IND,
	EVENT_TYPE_AUTH_REQ,
	EVENT_TYPE_AUTH_RSP,    //20
	EVENT_TYPE_CCO_SELECT_IND,
	EVENT_TYPE_CCO_HO_REQ,  
	EVENT_TYPE_CCO_HO_RSP,
	EVENT_TYPE_CCO_HO_IND,
	EVENT_TYPE_CCO_APPOINT_REQ, //25
	EVENT_TYPE_CCO_APPOINT_RSP,
	EVENT_TYPE_CCO_LOST_IND,
	EVENT_TYPE_TIMER_BBT_IND,
	EVENT_TYPE_TIMER_USTT_IND,
	EVENT_TYPE_TIMER_ACC_IND,   //30
	EVENT_TYPE_TIMER_APPT_IND,
	EVENT_TYPE_TIMER_DISC_AGING_IND,
	EVENT_TYPE_TIMER_TEI_IND,        //TEI lease timer STA and CCO
//	EVENT_TYPE_TIMER_TEI_REUSE_IND,  //TEI reuse timer

	//CCO
	EVENT_TYPE_BCN_TX_IND,
	EVENT_TYPE_NET_ACC_IND,
	EVENT_TYPE_NO_STA_IND,
	EVENT_TYPE_TIMER_DISC_IND,
	EVENT_TYPE_TIMER_BCN_TX_IND,
//	EVENT_TYPE_TIMER_TEI_IND,
	EVENT_TYPE_TIMER_JOIN_IND,
	EVENT_TYPE_TIMER_STA_IND,
	EVENT_TYPE_TIMER_HO_IND,

};

//message event
#define    EVENT_TYPE_CC_CCO_APPOINT_REQ   MMTYPE_CC_CCO_APPOINT_REQ  
#define    EVENT_TYPE_CC_CCO_APPOINT_CNF   MMTYPE_CC_CCO_APPOINT_CNF 
#define    EVENT_TYPE_CC_CCO_APPOINT_IND   MMTYPE_CC_CCO_APPOINT_IND 
#define    EVENT_TYPE_CC_CCO_APPOINT_RSP   MMTYPE_CC_CCO_APPOINT_RSP

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


typedef struct hpgpHdr
{
	//hpgp event sub header
	u8       snid;
	u8       *macAddr;    //source(Rx)/destination(Tx) MAC address
	u8       tei;         //source(RX)/destination(TX) TEI

	u8       auth:  1;     //TX: transmit in encryption   
	u8       eks:   4;     //EKS (encryption key select)
	u8       rsvd:  3;
	void    *scb;      //associated STA control block (used for the tei timer)
}sHpgpHdr, *psHpgpHdr;



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

union eventBody 
{
	sSnsmStartEvent         snsmStart;
	sNetAccRspEvent         netAccRsp;
	sNetLeaveIndEvent       netLeaveInd;
	sCcoApptReqEvent        ccoApptReq;
	sCcoApptRspEvent        ccoApptRsp;
	sCcoHoReqEvent          ccoHoReq;
	sCcoHoRspEvent          ccoHoRsp;
};

typedef union eventBody uEventBody;


#define EVENT_DEFAULT_SIZE       0

#ifdef SIMU
#define EVENT_HPGP_MSG_HEADROOM  (sizeof(sEthHdr)+sizeof(sMmHdr)+sizeof(sTxRxDesc)+sizeof(sHpgpHdr))       
#else
#define EVENT_HPGP_MSG_HEADROOM  (sizeof(sEthHdr)+sizeof(sMmHdr)+sizeof(sHpgpHdr))       
#endif

#define EVENT_HPGP_CTRL_HEADROOM  (sizeof(sHpgpHdr))       


#endif


/** =========================================================
 *
 * Edit History
 *
 * $Source: /home/cvsrepo/hybrii/firmware/hpgp/src/hpgpevt.h,v $
 *
 * $Log: hpgpevt.h,v $
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


