/** =========================================================
 *
 *  @file cnam.h
 * 
 *  @brief Network Access Manager 
 *
 *  Copyright (C) 2010-2011, Greenvity Communications, Inc.
 *  All Rights Reserved
 *  
 * ==========================================================*/


#ifndef  _NAM_H
#define  _NAM_H

#include "hpgpevt.h"
#include "papdef.h"
#include "hpgpdef.h"

struct linkLayer;
/* ------------------------------
 * CCO network access manager
 * --------------------------- */
//up to 64 STAs in TEI MAP
#define HPGP_TEI_MAP_BUFF_MAX  ( 520 + EVENT_HPGP_MSG_HEADROOM + \
                                    sizeof(sEvent) ) 

enum cnamState 
{
    CNAM_STATE_INIT,
    CNAM_STATE_READY,
    CNAM_STATE_SHUTTINGDOWN,
};



typedef struct backupCCoCfg
{
	sScb *scb;
	u8 result;

}sBackupCCoCfg;


typedef struct cnam
{
    struct linkLayer *linkl;

    sStaInfo   *staInfo;
    sCrm       *crm;
//    sCcoInfo   *ccoInfo;

    //state
    u8       state;
	u8 		numSta;
  
    u8       accNotification;
    u16      teiLeaseTime;        //no auth, in minutes
    u16      teiLeaseTimeAuth;    //auth, in minutes
    
//    u8       msgBuff[HPGP_TEI_MAP_BUFF_MAX];
#ifdef UKE
    u8       ukePeer[MAC_ADDR_LEN];
#endif	

    sScb    *ccoApptOrigScb;  //station originating the CCO appoint request

	sBackupCCoCfg backupCCoCfg;

#ifdef KEEP_ALIVE
	tTimerId staAgingTimer;
#endif
} sCnam, *psCnam;



/* ------------------------------
 * STA network access manager
 * --------------------------- */

enum snamState
{
    SNAM_STATE_INIT,
    SNAM_STATE_READY,
    SNAM_STATE_WAITFOR_CC_ASSOC_RSP,
    SNAM_STATE_WAITFOR_AUTH_RSP,
    SNAM_STATE_CONN,
    SNAM_STATE_WAITFOR_CC_LEAVE_RSP,
};

typedef struct snam
{

    struct linkLayer *linkl;
    sStaInfo   *staInfo;
#ifdef UKE    
    u8          ukePeer[MAC_ADDR_LEN];
#endif	
//    sCcoInfo   *ccoInfo;

    //state
    u8 state;

    //flag indicating the first
    //u8 initJoin;
    u8 txRetryCnt;      //Tx retry count
    u8 apptTxRetryCnt;  //user appoint CCo req tx retry count

    u8 teiRenew:     1;
    u8 stopDataPath: 1;
    u8 rvsd:         6;

    //for retransmission of mgmt msgs 
#ifdef UKE	
    u8                ukePeerNotification;
#endif
    u8                tei;  
    sCcCcoApptReq     ccoApptReq; //for resend and response

    sCcLeaveReq       leaveReq; 
	
	sBackupCCoCfg     backupCCoCfg;

    tTimerId accTimer;   //timer for association and leave
    tTimerId apptTimer;  //timer for user appoint CCo
    tTimerId teiTimer;  //tei lease timer    
    tTimerId   identifyCapTimer;
#ifdef KEEP_ALIVE
	tTimerId keepAlive;
#endif
}sSnam, *psSnam;


void    SNAM_ProcEvent(sSnam *snam, sEvent *event);
#ifdef UKE
void SNAM_EnableAssocNtf(sSnam *snam, u8 *macAddr);
#endif

eStatus SNAM_Start(sSnam *snam, u8 staType);
eStatus SNAM_Stop(sSnam *snam);
void SNAM_StartTEIRenew();

void    SNAM_PerformHoSwitch(sSnam *snam);
void    SNAM_StartConn(sSnam *snam);
eStatus SNAM_Init(sSnam *snam, struct linkLayer *linkl);

void    CNAM_ProcEvent(sCnam *cnam, sEvent *event);
void    CNAM_PerformHoSwitch(sCnam *cnam);
void    CNAM_Start(sCnam *cnam, u8 ccoType);
void CNAM_Stop(sCnam *cnam);

eStatus CNAM_Init(sCnam *cnam, struct linkLayer *linkl);

void CNAM_EnableAssocNotification(sCnam *cnam, u8 *macAddr);


extern eStatus CNAM_SelectBackupCCo(sCnam *cnam, sScb *scbIter);
extern void CNAM_BackupCCoAgeOut(sCnam *cnam, sScb *scb);


#endif


/** =========================================================
 *
 * Edit History
 *
 * $Source: /home/cvsrepo/Hybrii_B_OSFW_Dev/firmware/hpgp/src/link/nam.h,v $
 *
 * $Log: nam.h,v $
 * Revision 1.6  2015/01/02 14:55:36  kiran
 * 1) Timer Leak fixed while freeing SCB fixed
 * 2) Software broadcast supported for LG
 * 3) UART Loopback supported for LG
 * 4) Keep Alive feature to ageout defunctional STA
 * 5) Improved flash API's for NO Host Solution
 * 6) Imporved PLC Hang recovery mechanism
 * 7) Reduced nested call tree of common path functions
 * 8) Code optimization and cleanup (unused arguments, unused local variables)
 * 9) Work around for UART hardware interrupt issues (unintended interrupts and no interrupts)
 * 10) Use of memory specific pointers instead of generic pointers
 *
 * Revision 1.5  2014/07/05 09:16:27  prashant
 * 100 Devices support- only association tested, memory adjustments
 *
 * Revision 1.4  2014/06/12 13:15:43  ranjan
 * -separated bcn,mgmt,um event pools
 * -fixed datapath issue due to previous checkin
 * -work in progress. neighbour cco detection
 *
 * Revision 1.3  2014/03/10 05:58:10  ranjan
 * 1. added HomePlug BackupCCo feature. verified C&I test.(passed.) (bug 176)
 *
 * Revision 1.2  2014/02/27 10:42:47  prashant
 * Routing code added
 *
 * Revision 1.1  2013/12/18 17:05:23  yiming
 * no message
 *
 * Revision 1.1  2013/12/17 21:47:56  yiming
 * no message
 *
 * Revision 1.3  2013/09/04 14:51:01  yiming
 * New changes for Hybrii_A code merge
 *
 * Revision 1.6  2013/07/12 08:56:36  ranjan
 * -UKE Push Button Security Feature.
 * Verified : DirectEntry Security Works.Datapath Works.
 *                 command SetSecMode for UKE works.
 * Added against bug-160
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
 * Revision 1.9  2011/08/12 23:13:22  yuanhua
 * (1)Added Control Layer (2) Fixed bugs for user-selected CCo handover (3) Made changes to SNAM/CNAM and SNSM/CNSM for CCo handover switch (from CCo to STA, from STA to CCo, and from STA to STA but with different CCo) and post CCo handover
 *
 * Revision 1.8  2011/08/09 22:45:44  yuanhua
 * changed to event structure, seperating HPGP-related events from the general event defination so that the general event could be used for other purposes than the HPGP.
 *
 * Revision 1.7  2011/08/08 22:05:41  yuanhua
 * user-selected CCo handover fix
 *
 * Revision 1.6  2011/08/05 17:06:29  yuanhua
 * (1) added an internal queue in Link Layer for communication btw modules within Link Layer (2) Fixed bugs in CCo Handover. Now, CCo handover could be triggered by auto CCo selection, CCo handover messages work fine (3) Made some modifications in SHAL.
 *
 * Revision 1.5  2011/07/30 02:43:35  yuanhua
 * (1) Split the beacon process into two parts: one requiring an immdiate response, the other tolerating the delay (2) Changed the API btw the MUX and SHAL for packet reception (3) Fixed bugs in various modules. Now, multiple STAs could successfully associate/leave the CCo
 *
 * Revision 1.4  2011/07/16 17:11:23  yuanhua
 * (1)Implemented SHOM and CHOM modules, including handover procedure, SCB resource updating for HO (2) Update SNAM and CNAM modules to support uer-appointed CCo handover (3) Made the SCB resources to support the TEI MAP for the STA mode and management of associated STA resources (e.g. TEI) (4) Modified SNSM and CNSM to perform all types of handover switch (CCo handover to the new STA, STA taking over the CCo, STA switching to the new CCo)
 *
 * Revision 1.3  2011/07/08 22:23:48  yuanhua
 * (1) Implemented CNSM, including its state machine, beacon transmission and process, discover beacon scheduling, auto CCo selection, discover list, handover countdown, etc. (2) Updated SNSM, including discover list processing, triggering a switch to the new CCo, etc. (3) Updated CNAM and SNAM, adding the connection state in the SNAM, switch to the new CCo, etc. (4) Other updates
 *
 * Revision 1.2  2011/07/02 22:09:02  yuanhua
 * Implemented both SNAM and CNAM modules, including network join and leave procedures, systemm resource (such as TEI) allocation and release, TEI renew/release timers, and TEI reuse timer, etc.
 *
 * Revision 1.1  2011/05/28 06:31:19  kripa
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

