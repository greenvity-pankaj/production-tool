/** ========================================================
 *
 *  @file nsm.h
 * 
 *  @brief Network System Manager:
 *         CCO Network System Manager (CNSM)
 *         STA Network System Manager (SNSM)
 *
 *  Copyright (C) 2010-2011, Greenvity Communications, Inc.
 *  All Rights Reserved
 *  
 * =========================================================*/


#ifndef  _CNSM_H
#define  _CNSM_H

#include "papdef.h"
#include "mmsg.h"
#include "hpgpdef.h"
#include "hpgpconf.h"
#include "hpgpevt.h"
#include "crm.h"
#ifdef HPGP_HAL
#include "hal_hpgp.h"
#endif


struct linkLayer;

typedef struct saiTable
{
    /* schedule count down (bit 0-2: pscd, bit 3-5: cscd, bit 6-7 reserved) */
    u8          pscd:  3;
    u8          cscd:  3;
    u8          rsvd:  2;
    u8          saiNum;
    sSai        sai[HPGP_SESS_MAX];   
} sSaiTable;



typedef struct regionTable
{
    u8          regionNum;
    sCsmaRegion region[HPGP_REGION_MAX];
}sRegionTable;

/* ===========================
 * CCO network system manager
 * =========================== */

enum cnsmState 
{
    CNSM_STATE_INIT,
    CNSM_STATE_READY
};


/* Beacon buffer */
#ifdef SIMU
#define BEACON_BUFF_LEN        (BEACON_LEN + sizeof(sTxDesc))
#else
#define BEACON_BUFF_LEN        BEACON_LEN
#endif

typedef struct cnsm
{
    /* reference */
    struct linkLayer *linkl;
    sStaInfo   *staInfo;
    sCrm       *crm;
//    sCcoInfo   *ccoInfo;
    
    /* schedule the discovery beacon for the next SCB */
    sScb       *discScb;

    u8          bcnUpdate;
    u8          bcnUpdateProgress;
    u8          hoEnabled:         1;
    u8          hoReady:           1;
    u8          ccoNotification:   1;
    u8          schedDiscBcn:      1;   
    u8          rsvdx:         1;   
    u8          stopSnam:          1;
    u8          hm:                2;   //hybrid mode
	u8			txDiscBcn;

    u8          nctei;
    u8          hoCntDown;
    u8          state;

    u8          updateSched:       1;
	u8          rsvd:              7;
    /* index to the current sai table and region table */
    u8          currSchedInd;   
    /* session allocation info */
    sSaiTable   saiTable[2];   
    /* region table */
    sRegionTable regionTable[2];
/*
    //discovered sta list
    sDiscStaInfo discStaInfo[DISC_STA_LIST_MAX];
    u8        numDiscSta;

    //discovered network list
    sDiscNetInfo discNetInfo[DISC_NET_LIST_MAX];
    u8        numDiscNet;
*/
    //sBuffDesc   msgbuff[2];
    //todo: need two buffers for beacon, in order to change the beacon
    //Central beacon buffer
    u8 bpstoOffset;
    u8 bcnLen;
    u8 bcnBuff[BEACON_BUFF_LEN];

#ifdef SIMU
//    u8 discBcnBuff[BEACON_PAYLOAD_SIZE + sizeof(sTxDesc)];
#else
//    u8 discBcnBuff[BEACON_PAYLOAD_SIZE];
#endif


    //discovery timer
    tTimerId discTimer;
    //discovery list aging timer
    tTimerId   discAgingTimer;


#ifdef SIMU
    //simulate the beacon tx interrup for beacon interval
    tTimerId bcnTimer;
#endif

} sCnsm, *psCnsm;


eStatus CNSM_Init(sCnsm *cnsm, struct linkLayer *linkl);
void    CNSM_ProcEvent(sCnsm *cnsm, sEvent *event);
eStatus CNSM_InitFixedBcn(sCnsm *cnsm);
eStatus CNSM_Start(sCnsm *cnsm, u8 ccoType);
eStatus CNSM_BcnUpdateActive(sCnsm *cnsm);


u8      CNSM_QueryAnyAlvn(sCnsm *cnsm);
void CNSM_Stop(sCnsm *cnsm);
void CNSM_PostStop(sCnsm *cnsm);


void    CNSM_EnableHo(sCnsm *cnsm, u8 enable);
void    CNSM_StartHo(sCnsm *cnsm, u8 nctei);
void    CNSM_UpdateDiscBcnSched(sCnsm *cnsm, sScb *scb);
void    LINKL_CcoProcBcnHandler(void *cookie, sEvent *event);

/* ===========================
 * STA network system manager
 * =========================== */

#define AVLN_LIST_MAX       3
#define UA_STA_LIST_MAX     10
#define NO_BCN_MAX          8

#define MAX_NO_BEACON_BACKUPCCO   3	 // Before backUP cco should take over
#define MAX_NO_BEACON_NW_DISCOVERY    6   // Before STA starts N/w Discovery


enum snsmState
{
    SNSM_STATE_INIT,
    SNSM_STATE_NET_DISC,
    SNSM_STATE_CONN

};
typedef struct avlnInfo
{
//    sSlink   link;
    u8       valid : 1;
    u8       hit   : 1;
    u8       rsvd  : 6;
    u8       nid[NID_LEN];   //network id
    u8       snid;
                             //HFID
} sAvlnInfo, *psAvlnInfo;

typedef struct avlnInfoRef
{
    u8*       nid;   //network id
    u8       snid;
} sAvlnInfoRef, *psAvlnInfoRef;

//Unassociated STA info
typedef struct uaStaInfo
{
    u8       valid : 1;
    u8       hit   : 1;
    u8       rsvd  : 6;
    u8       macAddr[MAC_ADDR_LEN];   //
    u8       nid[NID_LEN];   //network id
    u8       ccoCap;
                             //HFID
} sUaStaInfo, *psUaStaInfo;

typedef struct bcnRef
{
//    sBcnHdr    *bcnHdr;
    sBeHdr     *nonPersSchedEntry;
    sBeHdr     *persSchedEntry;
    sBeHdr     *regionEntry;
    sBeHdr     *macAddrEntry;
    sBeHdr     *discEntry;
    sBeHdr     *discInfoEntry;
    sBeHdr     *bpstoEntry;
    sBeHdr     *encrypKeyChangeEntry;
    sBeHdr     *ccoHandoverEntry;
    sBeHdr     *bcnRelocEntry;
    sBeHdr     *aclSyncCntdownEntry;
    sBeHdr     *ChangeNumSlotsEntry;
    sBeHdr     *ChangeHmEntry;
    sBeHdr     *ChangeSnidEntry;
} sBcnRef, *psBcnRef;


typedef struct snsm
{
    /* reference */
    struct linkLayer *linkl;
    sStaInfo   *staInfo;
    sCrm       *crm;
//    sCcoInfo   *ccoInfo;

    u8         state;
 
    u8         staRole;
    u8         bpsto[3];

    u8         nctei;  //new CCo TEI for handover
    u8         hoEnabled:       1;
    u8         stopSnam:        1;
    u8         hoSwitch:        2;
    u8         txDiscBcn:       1;
    u8         discUpdate:      1;
    u8         netSync:         1;
    u8         netScan:         1;



    //CCo determination
    u8         enableCcoDetection: 1;
    u8         ccoDetected:        1;
    //CCo auto selection
    u8         enableCcoSelection: 1;
  	u8         enableBcnLossDetection: 1;
    u8         enableBackupCcoDetection: 1;
	
    u8         rsvd2:              3;

    u8         noBcn;  //lost bcn count

    //AVLN list
    sAvlnInfo  avlnInfo[AVLN_LIST_MAX];
    u8         numAvln;

    //pointer to the current sta in the AVLN list for sync
    //unassociated sta list
    sUaStaInfo uaStaInfo[UA_STA_LIST_MAX];
    u8         numUaSta;

    /* session allocation info */
    u8          regionNum;
    sCsmaRegion region[HPGP_REGION_MAX];

/*
    //discovered sta list
    sDiscStaInfo discStaInfo[DISC_STA_LIST_MAX];
    u8         numDiscSta;

    //discovered network list
    sDiscNetInfo discNetInfo[DISC_NET_LIST_MAX];
    u8         numDiscNet;
*/


    tTimerId   bbtTimer;

    tTimerId   usttTimer;
    //discovery list aging timer
    tTimerId   discAgingTimer;
	tTimerId   bcnLossTimer;

    u8 bpstoOffset;
    u8 discBcnLen;
    u8 discBcnBuff[BEACON_BUFF_LEN];

}sSnsm, *psSnsm;


void    SNSM_ProcEvent(sSnsm *snsm, sEvent *event);
eStatus SNSM_Init(sSnsm *snsm, struct linkLayer *linkl);
//void SNSM_Start(sSnsm *snsm, u8 discOpt);
eStatus SNSM_Start(sSnsm *snsm, u8 staType);
eStatus SNSM_Stop(sSnsm *snsm);

//void SNSM_StartConn(sSnsm *snsm);
//void SNSM_LeaveConn(sSnsm *snsm);
u8      SNSM_SelectCco(sSnsm *snsm,  sEvent *event);
u8      SNSM_DetermineStaRole(sSnsm *snsm);
void    SNSM_EnableHO(sSnsm *snsm);
void    SNSM_EnableCcoDetection(sSnsm *snsm);
void    LINKL_StaProcBcnHandler(void *cookie, sEvent *event);
void showStaType(u8 stamode, u8 staType);

#endif


/** =========================================================
 *
 * Edit History
 *
 * $Source: /home/cvsrepo/Hybrii_B_OSFW_Dev/firmware/hpgp/src/link/nsm.h,v $
 *
 * $Log: nsm.h,v $
 * Revision 1.9  2014/09/05 09:28:18  ranjan
 * 1. uppermac cco-sta switching feature fix
 * 2. general stability fixes for many station associtions
 * 3. changed mgmt memory pool for many STA support
 *
 * Revision 1.8  2014/08/25 07:37:34  kiran
 * 1) RSSI & LQI support
 * 2) Fixed Sync related issues
 * 3) Fixed timer 0 timing drift for SDK
 * 4) MMSG & Error Logging in Flash
 *
 * Revision 1.7  2014/07/05 09:16:27  prashant
 * 100 Devices support- only association tested, memory adjustments
 *
 * Revision 1.6  2014/06/26 17:59:42  ranjan
 * -fixes to make uppermac more robust for n/w change
 *
 * Revision 1.5  2014/06/19 17:13:19  ranjan
 * -uppermac fixes for lvnet and reset command for cco and sta mode
 * -backup cco working
 *
 * Revision 1.4  2014/06/12 13:15:44  ranjan
 * -separated bcn,mgmt,um event pools
 * -fixed datapath issue due to previous checkin
 * -work in progress. neighbour cco detection
 *
 * Revision 1.3  2014/03/10 05:58:10  ranjan
 * 1. added HomePlug BackupCCo feature. verified C&I test.(passed.) (bug 176)
 *
 * Revision 1.2  2014/02/19 10:22:41  ranjan
 * - common sync for hal_tst and upper mac project
 * - ism.c is MAC interrupt handler for hhal_tst and upper mac.
 *    chal_ext1isr function   is removed
 * - verified : lower mac sync, upper mac sync data traffic.
 *
 * Revision 1.1  2013/12/18 17:05:23  yiming
 * no message
 *
 * Revision 1.1  2013/12/17 21:47:55  yiming
 * no message
 *
 * Revision 1.4  2013/09/04 14:51:01  yiming
 * New changes for Hybrii_A code merge
 *
 * Revision 1.15  2013/03/26 12:07:26  ranjan
 * -added  host sw reset command
 * - fixed issue in bcn update
 *
 * Revision 1.14  2013/03/22 12:21:49  prashant
 * default FM_MASK and FM_Printf modified for USER INFO
 *
 * Revision 1.13  2013/03/14 11:49:18  ranjan
 * 1.handled cases  for CCo toSTA switch and  viceversa
 * 2.UM uses bcntemplate
 *
 * Revision 1.12  2012/08/20 04:57:35  yuanhua
 * modify the region entry and add persistent schedule entry for beacon
 *
 * Revision 1.11  2012/08/03 01:03:54  kripa
 * Adding poersistent schedule struct to cnsm.
 *
 * Revision 1.10  2012/07/08 18:42:20  yuanhua
 * (1)fixed some issues when ctrl layer changes its state from the UCC to ACC. (2) added a event CNSM_START.
 *
 * Revision 1.9  2012/06/27 04:28:18  yuanhua
 * added region entry in the beacon.
 *
 * Revision 1.8  2012/06/20 17:57:18  kripa
 *
 * Committed on the Free edition of March Hare Software CVSNT Client.
 * Upgrade to CVS Suite for more features and support:
 * http://march-hare.com/cvsnt/
 *
 * Revision 1.7  2012/06/14 06:14:47  yuanhua
 * (1) remove the net scan when the device is set to the CCO mode(2) start the net scan when the CCO is found, but the STA is not sync with CCO yet.
 *
 * Revision 1.6  2012/06/13 06:24:31  yuanhua
 * add code for tx bcn interrupt handler integration and data structures for region entry schedule. But they are not in execution yet.
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
 * Revision 1.12  2011/08/12 23:13:22  yuanhua
 * (1)Added Control Layer (2) Fixed bugs for user-selected CCo handover (3) Made changes to SNAM/CNAM and SNSM/CNSM for CCo handover switch (from CCo to STA, from STA to CCo, and from STA to STA but with different CCo) and post CCo handover
 *
 * Revision 1.11  2011/08/09 22:45:44  yuanhua
 * changed to event structure, seperating HPGP-related events from the general event defination so that the general event could be used for other purposes than the HPGP.
 *
 * Revision 1.10  2011/08/08 22:05:41  yuanhua
 * user-selected CCo handover fix
 *
 * Revision 1.9  2011/08/05 17:06:29  yuanhua
 * (1) added an internal queue in Link Layer for communication btw modules within Link Layer (2) Fixed bugs in CCo Handover. Now, CCo handover could be triggered by auto CCo selection, CCo handover messages work fine (3) Made some modifications in SHAL.
 *
 * Revision 1.8  2011/08/02 16:06:01  yuanhua
 * (1) Fixed a bug in STM (2) Made STA discovery work according to the standard, including aging timer. (3) release the resource after the STA leave (4) STA will switch to the backup CCo if the CCo failure occurs (5) At this point, the CCo could work with multiple STAs correctly, including CCo association/leave, TEI renew, TEI map updating, discovery beacon scheduling, discovery STA list updating ang aging, CCo failure, etc.
 *
 * Revision 1.7  2011/07/30 02:43:35  yuanhua
 * (1) Split the beacon process into two parts: one requiring an immdiate response, the other tolerating the delay (2) Changed the API btw the MUX and SHAL for packet reception (3) Fixed bugs in various modules. Now, multiple STAs could successfully associate/leave the CCo
 *
 * Revision 1.6  2011/07/22 18:51:05  yuanhua
 * (1) added the hardware timer tick simulation (hts.h/hts.c) (2) Added semaphors for sync in simulation and defined macro for critical section (3) Fixed some bugs in list.h and CRM (4) Modified and fixed bugs in SHAL (5) Changed SNSM/CNSM and SNAM/CNAM for bug fix (6) Added debugging prints, and more.
 *
 * Revision 1.5  2011/07/16 17:11:23  yuanhua
 * (1)Implemented SHOM and CHOM modules, including handover procedure, SCB resource updating for HO (2) Update SNAM and CNAM modules to support uer-appointed CCo handover (3) Made the SCB resources to support the TEI MAP for the STA mode and management of associated STA resources (e.g. TEI) (4) Modified SNSM and CNSM to perform all types of handover switch (CCo handover to the new STA, STA taking over the CCo, STA switching to the new CCo)
 *
 * Revision 1.4  2011/07/08 22:57:19  yuanhua
 * Fixed a compiler warning for nam.c
 *
 * Revision 1.3  2011/07/08 22:23:48  yuanhua
 * (1) Implemented CNSM, including its state machine, beacon transmission and process, discover beacon scheduling, auto CCo selection, discover list, handover countdown, etc. (2) Updated SNSM, including discover list processing, triggering a switch to the new CCo, etc. (3) Updated CNAM and SNAM, adding the connection state in the SNAM, switch to the new CCo, etc. (4) Other updates
 *
 * Revision 1.2  2011/06/24 14:33:18  yuanhua
 * (1) Changed event structure (2) Implemented SNSM, including the state machines in network discovery and connection states, becaon process, discover process, and handover detection (3) Integrated the HPGP and SHAL
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

