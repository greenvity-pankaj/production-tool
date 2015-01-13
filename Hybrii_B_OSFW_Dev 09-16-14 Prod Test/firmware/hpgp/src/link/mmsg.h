/** ========================================================
 *
 *  @file mmsg.h
 * 
 *  @brief Management Messages
 *
 *  Copyright (C) 2010-2011, Greenvity Communications, Inc.
 *  All Rights Reserved
 *  
 * ========================================================= */

#ifndef  _MMSG_H
#define  _MMSG_H

#include "papdef.h"
#include "hpgpdef.h"

#ifndef P8051
#ifndef __GNUC__
#pragma pack(1)
#endif
#endif

/* ----------------
 * Beacon
 * ---------------- */
//beacon header 1 - fixed portion (12 bytes)
typedef struct bcnHdr
{
    u8    nid[NID_LEN];   //network id (including two[-bits )
    u8    stei;           //source TEI
    u8    bt: 3;          //beacon type
    u8    ncnr: 1;        //non-coordinating networks reported
    u8    npsm: 1;        //network power saving mode
    u8    numslots: 3;    //number of beacon slots
    u8    slotusage;      //beacon slot usage
    u8    slotid: 3;      //beacon slot id
    u8    aclss: 3;       //AC link cycle sync status
    u8    hoip: 1;        //handover in progress
    u8    rtsbf: 1;       //RTS broadcast flag
    u8    nm: 2;          //network mode
    u8    ccocap: 2;      //CCo capability 
    u8    rsvd: 4;        //reserved
    u8    nbe;            //number of beacon entries
} __PACKED__ sBcnHdr, *psBcnHdr;

//beacon entry header
typedef struct beHdr
{
    u8    beType;         //beacon entry header
    u8    beLen;          //beacon entry length
} __PACKED__ sBeHdr, *psBeHdr;

/* session allocation information w/o start time */
typedef struct saiWithoutSt
{
    u8    stpf:    1;     /* start time present flag */
    u8    glid:    7;     /* Global Link ID */
    u8    etLo;           /* End Time */
    u8    etHi:    4;
    u8    rsvd:    4;
} __PACKED__ sSaiWithoutSt;

/* session allocation information w/ start time */
typedef struct saiWithSt
{
    u8    stpf:    1;     /* start time present flag */
    u8    glid:    7;     /* Global Link ID */
    u8    stLo;           /* End Time */
    u8    stHi:    4;
    u8    etLo:    4;    /* End Time */
    u8    etHi;    
} __PACKED__ sSaiWithSt;


//non-persistent schedule entry
typedef struct npersSchedEntry
{
    u8    ns:    6;    //number of sessions
    u8    rsvd:  2;    //reserved
} __PACKED__ sNpersSchedEntry;

//persistent schedule entry`
typedef struct persSchedEntry
{
    u8    pscd:    3;    //preview schedule countdown
    u8    cscd:    3;    //current schedule countdown
    u8    rsvd1:   2;    //reserved
    u8    ns:      6;    //number of sessions
    u8    rsvd2:   2;    //reserved
} __PACKED__ sPersSchedEntry;


/* regsion entry */
typedef struct regionEntry
{  
    u8        regionType:   4;
    u8        endTimeLo:    4;
    u8        endTimeHi;
} __PACKED__ sRegionEntry;

typedef union staCap {
    struct {
        u8        update:           1;    //
        u8        ccoCap:           2;    //CCo capability level
        u8        proxyNetCap:      1;    //proxy networking capability
        u8        backupCcoCap:     1;    //backup Cco capability
        u8        ccoStatus:        1;    //CCo status
        u8        pcoStatus:        1;    //PCo status
        u8        backupCcoStatus:  1;    //backup CCo Status
    } fields;
    u8    byte;
} uStaCap;


typedef union staStatus {
    struct {
        u8        authStatus:      1;    //authentication status
        u8        apptCcoStatus:   1;    //user appointed Cco status
        u8        greenPhyStatus:  2;    //Green PHY status
        u8        hpavVersion:     3;    //HPAV version
        u8        rsvd:            1;    //reserved
    } fields;
    u8    byte;
} uStaStatus;

//Discovered info entry
typedef struct discInfoEntry
{
    uStaCap      staCap;
    u8           numDiscSta;    //number of entries in the discovered STA list
    u8           numDiscNet;    //number of entries in the discovered Network list
    uStaStatus   staStatus;
}  __PACKED__ sDiscInfoEntry;

//CCo handover entry
typedef struct ccoHoEntry
{
    u8        hcd:      6;    //Handover Countdown
    u8        rsvd:     2;    //reserved
    u8        nctei;          //New Cco TEI

} __PACKED__ sCcoHoEntry;


//encryption key change entry
typedef struct encrypKeyChangeEntry
{
    u8        kccd:   6;
    u8        kbc:    1;
    u8        rsvd1:  1;
    u8        newEks: 4;
    u8        rsvd2:  4;
}  __PACKED__ sEncrypKeyChangeEntry;

//beacon relocation entry
typedef struct bcnrRelocEntry
{
    u8        rcd:   6;
    u8        rlt:   1;
    u8        lgf:   1;
    u8        rlo1;
    u8        rlo2;
    u8        rlo3:     1; 
    u8        rlSlotId: 3;
    u8        rsvd2:    4;
}  __PACKED__ sBcnRelocEntry;


//ac line sync countdown entry
typedef struct aclSyncCntDownEntry
{
    u8        countdown:  6;
    u8        rsvd1:      2;
    u8        Reason:     2;
    u8        rsvd2:      6;
}  __PACKED__ sAclSyncCntDownEntry;

//change numslots entry
typedef struct changeNumSlotsEntry
{
    u8        nsccd:  6;
    u8        rsvd1:  2;
    u8        newNumSlot: 3;
    u8        rsvd2:      5;
} __PACKED__ sChangeNumSlotsEntry;


//change hm entry
typedef struct changeHmEntry
{
    u8        hmccd:  6;
    u8        newHm:  2;
} __PACKED__ sChangeHmEntry;


//change snid entry
typedef struct changeSnidEntry
{
    u8        sccd:     4;
    u8        newSnid:  4;
} __PACKED__ sChangeSnidEntry;


//power save entry
typedef struct _sStations_info {

    u8 tei;
    u8 pss;
}__PACKED__  sStations;

typedef struct powerSaveEntry
{
    u8        bpCnt_Lo;
    u8        bpCnt_Hi: 4;
    u8        spsf:     1;
    u8        av11pf:   1;
    u8        rsvd:     2;
    u8        tpss;
    u8        pssi;
} __PACKED__ sPowerSaveEntry;

//bpsto entry
typedef struct bpstoEntry
{
    u8        bpsto[3];
} __PACKED__ sbpstoEntry;


//beacon entry structure used for encoding/decoding
typedef struct beEntry
{
    u8   beHdr;
    u8   beLen;
    union 
    {
       sPersSchedEntry    persSchedEntry;
       sNpersSchedEntry   npersSchedEntry;
       sDiscInfoEntry     discInfoEntry;
       sCcoHoEntry        ccoHoEntry;
    } beEntry;
} __PACKED__ sBeEntry, *psBeEntry;

#ifndef P8051
#ifndef __GNUC__
#pragma pack()
#endif
#endif


/* minimum length of HPGP mgmt msg */
#define HPGP_MM_MIN_LEN     60

/* ------------------------
 * CM_UNASSOCIATED_STA.IND
 * ------------------------ */
typedef struct CmUaStaInd
{
    u8    nid[NID_LEN];   //NID
    u8    ccoCap;         //CCO capability
} __PACKED__ sCmUaStaInd, *psCmUaStaInd;


/* ------------------------
 * CC_CCO_APPOINT.REQ
 * ------------------------ */
typedef struct CcCcoApptReq
{
    u8    reqType;
    u8    macAddr[MAC_ADDR_LEN];  
} __PACKED__ sCcCcoApptReq, *psCcCcoApptReq;


/* ------------------------
 * CC_CCO_APPOINT.CNF
 * ------------------------ */
typedef struct CcCcoApptCnf
{
    u8    result;
} __PACKED__ sCcCcoApptCnf, *psCcCcoApptCnf;


/*======================
* Backup Cco
* ====================== */

// CC_BACKUP_APPOINT    0
// CC_BACKUP_RELEASE    1
// CC_BACKUP_RESERVED  0x02 - 0xFF



typedef struct CcBackupReq
{
    u8    action;    // apoint, release, reserved
}__PACKED__ sCcBackupReq;


typedef struct CcBackupCnf
{
    u8    result;    // apoint, release, reserved
}__PACKED__ sCcBackupCnf;

/* ------------------------
 * CcIdentifyInd
 * ------------------------ */
typedef struct CcIdentifyInd
{
	u8    greenPhyCap;
	u8    pwrSaveCap;
	u8    allocCap;
	u8    repeatCap;
	u8    homeplugAV;
	u8 efl;
//	u8  ef;	
	
} __PACKED__ sCcIdentifyInd, *psCcIdentifyInd;

/*------------------------
*   NN_INL.REQ  NN_INL.CNF
*
*-------------------------*/

typedef struct NN_INLInfo
{
	u8 snid;
	u8 nid[NID_LEN];
	u8 numSlots;
	u8 slotId;
	u8 offset[2];
	u8 coordStatus;
}__PACKED__ sNnInlInfo;



typedef struct NN_INLReq
{
	u8 srcTei;
	u8 srcSnid;
	u8 srcNid[NID_LEN];
	u8 srcNumAuthSta;
	u8 srcNumSlot;
	u8 srcSlotId;
	u8 srcCoordStatus;
	u8 numNw;
}__PACKED__ sNnINLReq,sNnINLCnf, *psNnINLReq;



/* ------------------------
 * CC_ASSOC.REQ
 * ------------------------ */
typedef struct CcAssocReq
{
    u8    reqType;
    u8    nid[NID_LEN];   //NID
    u8    ccoCap;         //CCO capability
    u8    proxyNetCap;    //Proxy Networking capability
} __PACKED__ sCcAssocReq, *psCcAssocReq;


/* ------------------------
 * CC_ASSOC.CNF
 * ------------------------ */
typedef struct CcAssocCnf
{
    u8    result;
    u8    nid[NID_LEN];   //NID
    u8    snid;           //SNID
    u8    staTei;         //STA TEI assigned 
    u16   leaseTime;      //TEI lease time
} __PACKED__ sCcAssocCnf, *psCcAssocCnf;


/* ------------------------
 * CC_LEAVE.REQ
 * ------------------------ */
typedef struct CcLeaveReq
{
    u8    reason;
} __PACKED__ sCcLeaveReq, *psCcLeaveReq;


/* ------------------------
 * CC_LEAVE.IND
 * ------------------------ */
typedef struct CcLeaveInd
{
    u8    reason;
    u8    nid[NID_LEN];   //NID
} __PACKED__ sCcLeaveInd, *psCcLeaveInd;


/* ------------------------
 * CC_SET_TEI_MAP.IND
 * ------------------------ */
typedef struct teiMap
{
    u8    tei;
    u8    macAddr[MAC_ADDR_LEN];  
    u8    status;
} __PACKED__ sTeiMap, *psTeiMap;


typedef struct CcTeiMapInd
{
    u8         mode;
    u8         numSta;
    sTeiMap    teiMap;
} __PACKED__ sCcTeiMapInd, *psCcTeiMapInd;

/* ------------------------
 * CC_HANDOVER.REQ
 * ------------------------ */
typedef struct CcHoReq
{
    u8    hoType;
    u8    reason;
} __PACKED__ sCcHoReq, *psCcHoReq;



/* ------------------------
 * CC_HANDOVER.CNF
 * ------------------------ */
typedef struct CcHoCnf
{
    u8    result;
} __PACKED__ sCcHoCnf, *psCcHoCnf;


/* ------------------------
 * CC_DISCOVER_LIS.CNF
 * ------------------------ */
typedef struct ccDiscStaInfo
{
    u8        macAddr[MAC_ADDR_LEN];
    u8        tei;
    u8        sameNet;
    u8        snid;
    uStaCap   staCap;
    u8        sigLevel;               //signal level
    u8        avgBle;                 //average BLE
} __PACKED__ sCcDiscStaInfo, *psCcDiscStaInfo;


typedef struct ccDiscNetInfo
{
    u8        nid[NID_LEN];
    u8        snid;
    u8        hybridMode;
    u8        numBcnSlots;      //number of beacon slots
    u8        coordStatus;      //coordination status of CCO
    u16       offset;           //offset btw beacon region of discovered net
                                //and STA's own net
} __PACKED__ sCcDiscNetInfo, *psCcDiscNetInfo;



/* ------------------------
 * CC_HANDOVER_INFO.IND
 * ------------------------ */
typedef struct ccHoStaInfo
{
    u8    tei;
    u8    macAddr[MAC_ADDR_LEN];
    u8    status;
    u8    ptei;
} __PACKED__ sCcHoStaInfo, *pCcHoStaInfo;

#ifdef POWERSAVE
 /*
typedef struct pss
{
    u8    psp:4;		// Awake Window Duration
    u8    awd:4;		// Power Save Period
} __PACKED__ sPss, *pPss;
*/
/* ------------------------
 * CC_POWERSAVE.REQ
 * ------------------------ */
typedef struct CcPowersaveReq
{
//    sPss    pss;	// PS Schedule requested
    u8    pss;	// PS Schedule requested: 0-3: PSP, 4-7: AWD
} __PACKED__ sCcPowersaveReq, *psCcPowersaveReq;

typedef struct CcPowersaveCnf
{
    u8    result;	//0: accept, 1: reject
} __PACKED__ sCcPowersaveCnf, *psCcPowersaveCnf;
#endif

union mgmtMsg
{
    sCmUaStaInd    cmUaStaInd;
    sCcCcoApptReq  ccCcoApptReq;
    sCcCcoApptCnf  ccCcoApptCnf;
    sCcAssocReq    ccAssocReq;
    sCcAssocCnf    ccAssocCnf;
    sCcLeaveReq    ccLeaveReq;
    sCcLeaveInd    ccLeaveInd;
    sCcTeiMapInd   ccTeiMapInd;
	sCcBackupReq   ccBackupCcoReq;
	sCcBackupCnf   ccBackupCcoCnf;
    sCcHoReq       ccHoReq;
    sCcHoCnf       ccHoCnf;
#ifdef POWERSAVE
    sCcPowersaveReq ccPowersaveReq;
#endif
};

typedef union mgmtMsg sMgmtMsg;

union mgmtMsgRef
{
    sCmUaStaInd    *cmUaStaInd;
    sCcAssocReq    *ccAssocReq;
    sCcAssocCnf    *ccAssocCnf;
    sCcLeaveInd    *ccLeaveInd;
    sCcTeiMapInd   *ccTeiMapInd;
    sCcHoReq       *ccHoReq;
};


/* ------------------------
 * CM_ENCRYPTED_PAYLOAD.IND
 * ------------------------ */
typedef struct cmEncryPayloadInd
{
    u8          peks;
    u8          avlnStatus;
//    u8          reserved[5];
    u8          pid;
    u16         prn;
    u8          pmn;
    u8          iv[ENC_IV_LEN];
    u16         len;
}__PACKED__ sCmEncryPayloadInd, *psCmEncryPayloadInd;

/* ------------------------
 * CM_ENCRYPTED_PAYLOAD.RSP
 * ------------------------ */
typedef struct cmEncryPayloadRsp
{
    u8          result;   
    u8          pid;
    u16         prn;  
}__PACKED__ sCmEncryPayloadRsp, *psCmEncryPayloadRsp;


/* ------------------------
 * CM_SET_KEY.REQ
 * ------------------------ */
typedef struct cmSetKeyReq
{
    u8          keyType;
    u8          myNonce[4];
    u8          yourNonce[4];
    u8          pid;
    u16         prn;
    u8          pmn;
    u8          ccoCap;
    u8          nid[NID_LEN];

    u8          newEks;
    u8          pNewKey[ENC_KEY_LEN];

}__PACKED__ sCmSetKeyReq, *psCmSetKeyReq;


/* ------------------------
 * CM_SET_KEY.CNF
 * ------------------------ */
typedef struct cmSetKeyCnf
{
    u8          result;
    u8          myNonce[4];
    u8          yourNonce[4];
    u8          pid;

    u16         prn;
    u8          pmn;

    u8          ccoCapability;

}__PACKED__ sCmSetKeyCnf, *psCmSetKeyCnf;



/* ------------------------
 * CM_GET_KEY.REQ
 * ------------------------ */
typedef struct cmGetKeyReq
{
    u8          reqType;
    u8          reqKeyType;
    u8          nid[NID_LEN];
    u8          myNonce[4];
    u8          pid;
    u16         prn;
    u8          pmn;
    /* variable hash key */
}__PACKED__ sCmGetKeyReq, *psCmGetKeyReq;


/* ------------------------
 * CM_GET_KEY.CNF
 * ------------------------ */
typedef struct cmGetKeyCnf
{
    u8          result;
    u8          reqKeyType;
    u8          myNonce[4];
    u8          yourNonce[4];
    u8          nid[NID_LEN];
    u8          eks;
    u8          pid;
    u16         prn;
    u8          pmn;
    /* encryption key or variable hash key */
}__PACKED__ sCmGetKeyCnf, *psCmGetKeyCnf;




typedef union encMgmtMsg 
{
    sCmSetKeyReq    setKeyReq;
    sCmSetKeyCnf    setKeyCnf;
    sCmGetKeyReq    getKeyReq;
    sCmGetKeyCnf    getKeyCnf;
} uEncMgmtMsg;

#ifdef UKE
/* ------------------------
 * CM_JOIN.REQ
 * ------------------------ */
typedef struct cmJoinReq
{
    u8          ccoCapability;

}__PACKED__ sCmJoinReq, *psCmJoinReq;

/* ------------------------
 * CM_JOIN.CNF
 * ------------------------ */
typedef struct cmJoinCnf
{
    u8          nid[NID_LEN];
    uStaCap     staCap;

}__PACKED__ sCmJoinCnf, *psCmJoinCnf;

#endif


#endif


/** =========================================================
 *
 * Edit History
 *
 * $Source: /home/cvsrepo/Hybrii_B_OSFW_Dev/firmware/hpgp/src/link/mmsg.h,v $
 *
 * $Log: mmsg.h,v $
 * Revision 1.4  2014/06/12 13:15:43  ranjan
 * -separated bcn,mgmt,um event pools
 * -fixed datapath issue due to previous checkin
 * -work in progress. neighbour cco detection
 *
 * Revision 1.3  2014/03/10 05:58:10  ranjan
 * 1. added HomePlug BackupCCo feature. verified C&I test.(passed.) (bug 176)
 *
 * Revision 1.2  2014/01/28 17:46:21  tri
 * Added Power Save code
 *
 * Revision 1.1  2013/12/18 17:05:23  yiming
 * no message
 *
 * Revision 1.1  2013/12/17 21:47:56  yiming
 * no message
 *
 * Revision 1.6  2013/11/14 09:23:17  ranjan
 * -Project setting XDATA Range : 0x2100 to 0xDFFF
 * - Added mgmt.c and  hal_tst_assoc_test.prj for QCA assoc testing. Feature
 *   enabled by c51 define : ASSOC_TEST
 *
 * Revision 1.5  2013/10/16 07:43:38  prashant
 * Hybrii B Upper Mac compiling issues and QCA fix, added default eks code
 *
 * Revision 1.4  2013/09/04 14:51:01  yiming
 * New changes for Hybrii_A code merge
 *
 * Revision 1.11  2013/07/12 08:56:36  ranjan
 * -UKE Push Button Security Feature.
 * Verified : DirectEntry Security Works.Datapath Works.
 *                 command SetSecMode for UKE works.
 * Added against bug-160
 *
 * Revision 1.10  2012/08/25 05:49:15  yuanhua
 * fix a potential overwriting of region array in SNSM when receive a beacon.
 *
 * Revision 1.9  2012/08/20 04:57:35  yuanhua
 * modify the region entry and add persistent schedule entry for beacon
 *
 * Revision 1.8  2012/08/03 01:03:17  kripa
 * Adding SAI with Start Time struct
 *
 * Revision 1.7  2012/07/15 17:31:07  yuanhua
 * (1)fixed a potential memory overwriting in MUXL (2)update prints for 8051.
 *
 * Revision 1.6  2012/06/13 06:24:31  yuanhua
 * add code for tx bcn interrupt handler integration and data structures for region entry schedule. But they are not in execution yet.
 *
 * Revision 1.5  2012/04/13 06:15:11  yuanhua
 * integrate the HPGP protocol stack with the HAL for the beacon TX/RX and mgmt msg RX.
 *
 * Revision 1.4  2012/03/11 17:02:25  yuanhua
 * (1) added NEK auth in AKM (2) added NMA (3) modified hpgp layer data structures (4) added Makefile for linux simulation
 *
 * Revision 1.3  2012/01/21 17:04:44  kripa
 * Added bpsto bentry.
 *
 * Revision 1.2  2011/09/09 07:02:31  yuanhua
 * migrate the firmware code from the greenchip to the hybrii.
 *
 * Revision 1.7  2011/08/02 16:06:00  yuanhua
 * (1) Fixed a bug in STM (2) Made STA discovery work according to the standard, including aging timer. (3) release the resource after the STA leave (4) STA will switch to the backup CCo if the CCo failure occurs (5) At this point, the CCo could work with multiple STAs correctly, including CCo association/leave, TEI renew, TEI map updating, discovery beacon scheduling, discovery STA list updating ang aging, CCo failure, etc.
 *
 * Revision 1.6  2011/07/30 02:43:35  yuanhua
 * (1) Split the beacon process into two parts: one requiring an immdiate response, the other tolerating the delay (2) Changed the API btw the MUX and SHAL for packet reception (3) Fixed bugs in various modules. Now, multiple STAs could successfully associate/leave the CCo
 *
 * Revision 1.5  2011/07/16 17:11:23  yuanhua
 * (1)Implemented SHOM and CHOM modules, including handover procedure, SCB resource updating for HO (2) Update SNAM and CNAM modules to support uer-appointed CCo handover (3) Made the SCB resources to support the TEI MAP for the STA mode and management of associated STA resources (e.g. TEI) (4) Modified SNSM and CNSM to perform all types of handover switch (CCo handover to the new STA, STA taking over the CCo, STA switching to the new CCo)
 *
 * Revision 1.4  2011/07/08 22:23:48  yuanhua
 * (1) Implemented CNSM, including its state machine, beacon transmission and process, discover beacon scheduling, auto CCo selection, discover list, handover countdown, etc. (2) Updated SNSM, including discover list processing, triggering a switch to the new CCo, etc. (3) Updated CNAM and SNAM, adding the connection state in the SNAM, switch to the new CCo, etc. (4) Other updates
 *
 * Revision 1.3  2011/07/02 22:09:01  yuanhua
 * Implemented both SNAM and CNAM modules, including network join and leave procedures, systemm resource (such as TEI) allocation and release, TEI renew/release timers, and TEI reuse timer, etc.
 *
 * Revision 1.2  2011/06/24 14:33:18  yuanhua
 * (1) Changed event structure (2) Implemented SNSM, including the state machines in network discovery and connection states, becaon process, discover process, and handover detection (3) Integrated the HPGP and SHAL
 *
 * Revision 1.1  2011/05/06 19:10:12  kripa
 * Adding link layer files to new source tree.
 *
 * Revision 1.3  2011/04/23 23:04:06  kripa
 * sNpersSchedEntry struct, sPersSchedEntry struct; changed 'u8 sai[0]' to 'u8 sai[1]' since VC,Keil does not allow empty array field.
 *
 * Revision 1.2  2011/04/23 17:43:46  kripa
 * 1.Added #pragma pack if compiler is not GCC. (for VC and Keil compilers).
 * 2.struct bcn, 'u8 beEntry[0]' to 'u8 beEntry[1]', since VC gives error for empty array field in structs.
 *
 * Revision 1.1  2011/04/08 21:42:45  yuanhua
 * Framework
 *
 *
 * =========================================================*/

