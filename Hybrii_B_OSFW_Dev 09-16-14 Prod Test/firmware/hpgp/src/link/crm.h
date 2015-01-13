/** =========================================================
 *
 *  @file crm.h
 * 
 *  @brief Central Resource Manager
 *
 *  Copyright (C) 2010-2011, Greenvity Communications, Inc.
 *  All Rights Reserved
 *  
 * ===========================================================*/

#ifndef  _CRM_H
#define  _CRM_H

#include "papdef.h"
#include "hpgpdef.h"
#include "list.h"
#include "mmsg.h"
#include "hal_hpgp.h"
#include "timer.h"

#define    CRM_SCB_HASH_TABLE_SIZE   10 // 4
#define    CRM_SCB_MAX               70 // 12  //128
#define    CRM_SCB_BUCKET_MAX        (CRM_SCB_MAX/CRM_SCB_HASH_TABLE_SIZE)

#define    DISC_STA_LIST_MAX   100 // 4
#define    DISC_NET_LIST_MAX   3
#define    TEI_MAP_TABLE_MAX   4

//SCB.hoTrigger
#define    HO_TRIGGER_TYPE_AUTO   0      //auto select
#define    HO_TRIGGER_TYPE_USER   1      //user select

//SCB.staTimerType
#define    STA_TIMER_TYPE_HO   0      //handover timer




enum {
    STA_STATE_INIT,
    STA_STATE_IDLE,
    STA_STATE_HO_START,
    STA_STATE_WAITFOR_CC_HO_RSP,
    STA_STATE_WAITFOR_CC_HO_INFO_RSP,
    STA_STATE_RELEASE,
};

enum {
    STA_NAM_STATE_INIT,
//    STA_NAM_STATE_IDLE,
    STA_NAM_STATE_CONN,
    STA_NAM_STATE_WAITFOR_CCO_HO_RSP,
    STA_NAM_STATE_RELEASE,
};

enum {
    STA_AKM_STATE_INIT,
    STA_AKM_STATE_IDLE,
};

enum {
    STA_HOM_STATE_INIT,
    STA_HOM_STATE_IDLE,
    STA_HOM_STATE_WAITFOR_CC_HO_CNF,
    STA_HOM_STATE_WAITFOR_CC_HO_INFO_RSP
};

//obtained from CC_DISCOVER_LIST.CNF
typedef struct discStaInfo
{
    u8          valid    : 1;
    u8          hit      : 1;
    u8          sameNet  : 1;
    u8          rsvd     : 5;           
    u8          macAddr[MAC_ADDR_LEN];
    u8          tei;
    u8          snid;                   //frame control from the MAC 
//    u8          nid[NID_LEN];           //network id
    uStaCap     staCap;
//    u8          sigLevel;               //signal level
//    u8          avgBle;                 //average BLE
    //the following information is in the discover beacon
    uStaStatus  staStatus;     //
    u8          numDiscSta;    //number of entries in the discovered STA list
    u8          numDiscNet;    //number of entries in the discovered Network list
} sDiscStaInfo, *psDiscStaInfo;


//obtained from CC_DISCOVER_LIST.CNF
typedef struct discNetInfo
{
    u8       valid : 1;
    u8       hit   : 1;
    u8       rsvd1 : 6;           
    u8       nid[NID_LEN];           //network id from the beacon header
    u8       snid;                   //frame control from the MAC 
    u8       hybridMode:  2;         //from the beacon header   
    u8       netMode:     2;         //from the beacon header  
    u8       numBcnSlots: 3;         //from the beacon header  
    u8       rsvd2:       1;           
//    u8       bpsto[3];               //beacon period start time offset
                                     //from the discover beacon
    u8       coordStatus;            //coordination status
    u16      offset;                 //offset btw beacon region of disovered net
                                     //and STA's own net
} sDiscNetInfo, *psDiscNetInfo;


typedef struct discStaInfoRef
{
    u8             *macAddr;
    u8             *nid;
    u8              snid;
    u8              tei;
    u8              sameNet;
    sDiscInfoEntry *discInfo;
} sDiscStaInfoRef, *psDiscStaInfoRef;

typedef struct discNetInfoRef
{
    u8       *nid;           //network id from the beacon header
    u8       snid;                   //frame control from the MAC 
    u8       netMode:     2;         //from the beacon header  
    u8       hybridMode:  2;         //from the beacon header   
    u8       numBcnSlots: 3;         //from the beacon header  
    u8       rsvd:        1;           
//    u8       *bpsto;               //beacon period start time offset
} sDiscNetInfoRef, *psDiscNetInfoRef;

#ifdef POWERSAVE
typedef struct psSchedule
{
    u8       awdTime;           	 // AWD in ms
    u16      numBp;                  // PSS in # of BP 
} sPsSchedule, *psPsSchedule;
#endif
typedef struct staIdentifyCaps
{
    u8 greenPHYCap;
    u8 powerSaveCap;
    u8 routingCap;
    u8 HPAVVer;
    u8 efl;
} sStaIdentifyCaps;

//STA resource for CCO
typedef struct staCb
{
    sSlink    link;

//    u8        state;
    u8        namState:  4;
    u8        akmState:  4;
    u8        homState:  4;
    u8        rsvd1:     4;

    u8        ptei;
    u8        macAddr[MAC_ADDR_LEN];
    u8        tei;

//    u8        hoTrigger:    1; 
    u8        hoReason:     2;  //will be appointed cco
    u8        staTimerType: 3;
    u8        discUpdate:   1;
    u8        identityCapUpdated: 1;
    u8        rsvd2:        1;
    u8        identityCapUpdatedRetry;
    sStaIdentifyCaps idCaps;
    //STA discovery info 
    uStaCap     staCap;
    uStaStatus  staStatus;

   /* the pid, prn, and pmn are used by the CCo during the authentication */
    u8          pid;
    u16         prn;
    u8          pmn;


#if 0
    u8        nekEks;            //EKS for NEK
    u8        nek[ENC_KEY_LEN];  //NEK
    u8        nekIv[ENC_IV_LEN]; //IV for NEK
#endif
 
    //discovered sta list
//    sDiscStaInfo discStaInfo[DISC_STA_LIST_MAX];
    u8        numDiscSta;    //number of entries in the discovered STA list

    //discovered network list
  //  sDiscNetInfo discNetInfo[DISC_NET_LIST_MAX];
    u8        numDiscNet;    //number of entries in the discovered Network list

//    sDlink    discStaList;   //discovered STA list
//    sDlink    discNetList;   //discovered Network list

    u8        txRetryCnt;

    tTimerId  staTimer;
    tTimerId  teiTimer;    //TEI lease timer
//    tTimerId  teiReuseTimer; //TEI reuse timer (5 minutes after release)
	uPlcRssiLqiReg  rssiLqi;
#ifdef POWERSAVE
	u8 psState;		// 0: OFF, 1: WAITING, 2: ON
	u8 pss;			// Power Save Schedule
	sPsSchedule commAwd;		// Common AWD in AVLN
	u16 bpCnt;		// CCO-only: Bcn Period Count
	u8 pssi;		// CCO-only: PS State Identifier
#endif
#ifdef ROUTE
    sLrtParam       lrtEntry;
#endif
	u16 uMinSSN;
//	u16 uMaxSSN;
    u8  uWrapAround;
	u16 bMinSSN;
//	u16 bMaxSSn;

} sScb, *psScb;


//STA information
typedef struct staInfo
{
//    u8          macAddr[MAC_ADDR_LEN];
    u8         *macAddr;  //point to the MAC address in HAL/SHAL
    u8          tei;
    u8          nid[NID_LEN];   //network id
    u8          snid;
    u8          hfid[HFID_LEN];

    u8          hm;        /* hybrid mode   */
    u8          secLevel;
    u8          secMode;   /* security mode */
	u8			ukeEnable;
	//u8 			greenPhyCap : 1;
	//u8          repeaterRouting : 1;
	//u8          powerSaveCap:     1;
	//u8          rsvd:             2;
	//u8          HPAVSupported :   3;
	
//    u8          appointedCco:  1;
//    u8          rsvd:          7;
//    sDiscInfoEntry  discInfo;
    u8 lastUserAppCCOState;

    uStaCap     staCap;
    uStaStatus  staStatus;
//    u8        numDiscSta;    //number of entries in the discovered STA list
//    u8        numDiscNet;    //number of entries in the discovered Network list

//	u8          devicePassword[MAX_DPW_LEN];
  //  u8          dakPeks;           /* PEKS for DAK */
//    u8          dak[ENC_KEY_LEN];  /* TEK */
  ///  u8          dakIv[ENC_IV_LEN]; /* IV for DAK */

    u8          nmkPeks;           /* PEKS for NMK */
    u8          nmk[ENC_KEY_LEN];  /* NMK */
    u8          nmkIv[ENC_IV_LEN]; /* IV for NMK */

#ifdef UKE
    u8          tekPeks;           /* PEKS for TEK */
    u8          tek[ENC_KEY_LEN];  /* TEK */
    u8          tekIv[ENC_IV_LEN]; /* IV for TEK */
#endif

    u8          nekEks;            /* EKS for NEK */
    u8          nek[ENC_KEY_LEN];  /* NEK */
    /* NOTE: IV for NEK is defined by the 12-byte SOF, 
     * 1 byte PBC, and 3-byte PBH in PHY frame header 
     */

    u8          ppekEks;            /* EKS for NEK */
    u8          ppek[ENC_KEY_LEN];  /* NEK */

		
    sScb       *staScb;
    sScb       *ccoScb;
    sStaIdentifyCaps identifyCaps;
    u8 bridgeSupported;
	sDiscStaInfo discStaInfo[DISC_STA_LIST_MAX];
  	sDiscNetInfo discNetInfo[DISC_NET_LIST_MAX];
    //TEI MAP
//    sTeiMap   teiMap[TEI_MAP_TABLE_MAX]; 
} sStaInfo, *psStaInfo;




//typedef  sStaInfo     sCcoInfo;
//typedef struct ccoInfo
//{
//    u8          nid[NID_LEN];   //network id
//    u8          snid;
//} sCcoInfo;


//CCo Resource Manager
typedef struct crm
{
     //STA

     //preallocated STA resources
     //NOTE: in CCo mode, the SCB resource is used 
     //for management of associated STAs,
     //while in STA mdoe, the SCB resource is used for TEI mapping.
     sScb   scb[CRM_SCB_MAX];

     //hash table for allocated STA resources
     sSlist scbBucket[CRM_SCB_HASH_TABLE_SIZE];
     u16    scbBucketSize[CRM_SCB_HASH_TABLE_SIZE];
     //free STA resources
     sSlist freeQueue;

} sCrm, *psCrm;


void   CRM_Init(sCrm *crm);
sScb  *CRM_AllocScb(sCrm *crm);
void   CRM_FreeScb(sCrm *crm, sScb *scb);
sScb  *CRM_GetScb(sCrm *crm, u8 tei);
sScb  *CRM_AddScb(sCrm *crm, u8 tei);
u8     CRM_GetScbNum(sCrm *crm);
sScb  *CRM_GetNextScb(sCrm *crm, sScb* scb);

sScb  *CRM_GetNextDiscScb(sCrm *crm);

void CRM_RemoveBucket(sCrm *crm, u8 bkt);

sScb *CRM_FindScbMacAddr(u8 *macAddr);
#endif


/** =========================================================
 *
 * Edit History
 *
 * $Source: /home/cvsrepo/Hybrii_B_OSFW_Dev/firmware/hpgp/src/link/crm.h,v $
 *
 * $Log: crm.h,v $
 * Revision 1.13  2014/09/05 09:28:18  ranjan
 * 1. uppermac cco-sta switching feature fix
 * 2. general stability fixes for many station associtions
 * 3. changed mgmt memory pool for many STA support
 *
 * Revision 1.12  2014/07/16 10:47:40  kiran
 * 1) Updated SDK
 * 2) Fixed Diag test in SDK
 * 3) Ethernet and SPI interfaces removed from SDK as common memory is less
 * 4) GPIO access API's added in SDK
 * 5) GV701x chip reset command supported
 * 6) Start network and Join network supported in SDK (Forced CCo and STA)
 * 7) Some bug fixed in SDK (CP free, p app command issue etc.)
 *
 * Revision 1.11  2014/07/05 09:16:27  prashant
 * 100 Devices support- only association tested, memory adjustments
 *
 * Revision 1.10  2014/06/05 08:38:41  ranjan
 * -flash function enabled for uppermac
 * - commit command after any change would flash systemprofiles
 * - verfied upper mac
 *
 * Revision 1.9  2014/05/28 10:58:59  prashant
 * SDK folder structure changes, Uart changes, removed htm (UI) task
 * Varified - UM, LM - iperf (UDP/TCP), overnight test pass
 *
 * Revision 1.8  2014/05/16 08:52:30  kiran
 * - System Profile Flashing API's Added. Upper MAC functionality tested
 *
 * Revision 1.7  2014/04/21 03:30:52  ranjan
 * SSN filter added
 *
 * Revision 1.6  2014/04/09 21:09:26  tri
 * more PS
 *
 * Revision 1.5  2014/02/27 10:42:47  prashant
 * Routing code added
 *
 * Revision 1.4  2014/02/26 23:16:02  tri
 * more PS code
 *
 * Revision 1.3  2014/02/19 10:22:41  ranjan
 * - common sync for hal_tst and upper mac project
 * - ism.c is MAC interrupt handler for hhal_tst and upper mac.
 *    chal_ext1isr function   is removed
 * - verified : lower mac sync, upper mac sync data traffic.
 *
 * Revision 1.2  2014/01/28 17:47:50  tri
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
 * Revision 1.9  2012/10/25 11:38:48  prashant
 * Sniffer code added for MAC_SAP, Added new commands in MAC_SAP for sniffer, bridge,
 *  hardware settings and peer information.
 *
 * Revision 1.8  2012/10/11 06:21:00  ranjan
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
 * Revision 1.7  2012/06/29 03:01:00  kripa
 * Adding lineMd.
 * Committed on the Free edition of March Hare Software CVSNT Client.
 * Upgrade to CVS Suite for more features and support:
 * http://march-hare.com/cvsnt/
 *
 * Revision 1.6  2012/03/11 17:02:24  yuanhua
 * (1) added NEK auth in AKM (2) added NMA (3) modified hpgp layer data structures (4) added Makefile for linux simulation
 *
 * Revision 1.5  2011/09/18 01:32:08  yuanhua
 * designed the AKM for both STA and CCo.
 *
 * Revision 1.4  2011/09/14 06:47:16  yuanhua
 * Added key info in the SCB and stainfo data structures
 *
 * Revision 1.3  2011/09/14 05:52:36  yuanhua
 * Made Keil C251 compilation.
 *
 * Revision 1.2  2011/09/09 07:02:31  yuanhua
 * migrate the firmware code from the greenchip to the hybrii.
 *
 * Revision 1.9  2011/08/02 16:06:00  yuanhua
 * (1) Fixed a bug in STM (2) Made STA discovery work according to the standard, including aging timer. (3) release the resource after the STA leave (4) STA will switch to the backup CCo if the CCo failure occurs (5) At this point, the CCo could work with multiple STAs correctly, including CCo association/leave, TEI renew, TEI map updating, discovery beacon scheduling, discovery STA list updating ang aging, CCo failure, etc.
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

