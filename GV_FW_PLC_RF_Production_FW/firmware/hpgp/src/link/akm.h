/** =========================================================
 *
 *  @file akm.h
 * 
 *  @brief Authentication and Key Manager 
 *
 *  Copyright (C) 2012, Greenvity Communications, Inc.
 *  All Rights Reserved
 *  
 * ==========================================================*/


#ifndef  _AKM_H
#define  _AKM_H

#include "hpgpdef.h"
#include "hpgpevt.h"
#include "papdef.h"

#define HPGP_TEK_LIFETIME 60000

enum
{
    AKM_KEEP_NEK,
    AKM_NEW_NEK,
};
enum
{
    AKM_ROLE_UNKNOWN, //unknown
    AKM_ROLE_ORIG,    //originator
    AKM_ROLE_TERM,    //terminator
};


/* PEKS (Payload Encryption Key Select */
typedef enum _peks
{
    PEKS_DAK  = 0,
    PEKS_NMK,
    /* 0x2-0xE for TEK */
    PEKS_NOKEY = 15,
}ePeks;


/* Key Type */
typedef enum _keyType
{
    KEY_TYPE_DAK       = 0,
    KEY_TYPE_NMK,
    KEY_TYPE_NEK,
    KEY_TYPE_TEK,
    KEY_TYPE_HASH_KEY,
    KEY_TYPE_NO_KEY,
}eKeyType;


/* AVLN Status */
typedef enum _avlStatus
{
    AVLN_STATUS_UNASSOC_L0_CCO_CAP     = 0,
    AVLN_STATUS_UNASSOC_L1_CCO_CAP,
    AVLN_STATUS_UNASSOC_L2_CCO_CAP,
    AVLN_STATUS_UNASSOC_L3_CCO_CAP,
    AVLN_STATUS_ASSOC_NO_PCO_CAP,
    AVLN_STATUS_ASSOC_PCO_CAP,
    AVLN_STATUS_CCO                    = 8,
}eAvlnStatus;


typedef enum
{
    AUTH_PID_NEW_STA       = 0,   /* NEk auth for new sta joining the avln */
    AUTH_PID_NEW_NEK,             /* NEk update */
    AUTH_PID_NMK_WITH_DAK,        /* NMK auth using DAK */
    AUTH_PID_NMK_WITH_UKE,        /* NMK auth using UKE */
    AUTH_PID_HLE,                 /* third party auth protocol */
}eAuthPid;


enum
{
    AKM_STATE_INIT,
    AKM_STATE_READY,
    AKM_STATE_WAITFOR_HASH_GET_CNF,
    AKM_STATE_WAITFOR_NMK_SET_CNF,
    AKM_STATE_WAITFOR_NEK_SET_CNF,
    AKM_STATE_WAITFOR_GET_NEK_CNF,
    AKM_STATE_WAITFOR_NEK_UPDATE_CNF,
};





/* Auth and Key Manager */
typedef struct akm
{
    u8          state:        3;
    u8          akmRole:      1;
    u8          akmMode:      1;            /* STA or CCo */
    u8          alvnStatus:   1;            /* ALVN status */
    u8          nmkStatus:    1;
//    u8          currNekInd:   1;
  //  u8          netCntEnable: 1;            /* start NEK update countdown */
//    u8          keyCnt;                     /* Key update countdown */

    /* the key type, pid, prn, and pmn are used by the STA 
     * during the authentication 
     */
    u8          keyType;

    u8          pid;
    u16         prn;
    u8          pmn;
    u8          pad;  /* rf + padding */
   
    u8          myNonce[4];
    u8          yourNonce[4];

    u8          peerTei;
    u8         *peerMacAddr;

    u8          eks;               /* PEKS */

  //  u8          dakPeks;           /* PEKS for DAK */
  //  u8          dak[ENC_KEY_LEN];  /* TEK */
  //  u8          dakIv[ENC_IV_LEN]; /* IV for DAK */

//    u8          nmkPeks;           /* PEKS for NMK */
  //  u8          nmk[ENC_KEY_LEN];  /* NMK */
    u8          nmkIv[ENC_IV_LEN]; /* IV for NMK */
#ifdef UKE
    u8          tekPeks;           /* PEKS for TEK */
    u8          tek[ENC_KEY_LEN];  /* TEK */
    u8          tekIv[ENC_IV_LEN]; /* IV for TEK */
#endif

    tTimerId    akmTimer;
#ifdef UKE	
    tTimerId    TekTimer;
#endif	
    u8          txRetryCnt;


}sAkm, *psAkm;



void AKM_ProcEvent(sAkm *akm, sEvent *event);
eStatus AKM_Init(sAkm *akm);
eStatus AKM_Start(sAkm *akm, u8 mode, u8 newNek );
eStatus AKM_Stop(sAkm *akm);

#ifdef UKE
eStatus GenerateKey(u8 *pwd, u8 pwdlen, u8 *key);
u8 AKM_GetNewEks(sAkm *akm);
eStatus AKM_GenerateTek();
u8* AKM_GetTek();
#endif

void FillRandomNumber(u8 *, u16);


#endif


/** =========================================================
 *
 * Edit History
 *
 * $Source: /home/cvsrepo/Hybrii_B_OSFW_Dev/firmware/hpgp/src/link/akm.h,v $
 *
 * $Log: akm.h,v $
 * Revision 1.3  2014/07/05 09:16:27  prashant
 * 100 Devices support- only association tested, memory adjustments
 *
 * Revision 1.2  2014/06/19 17:13:19  ranjan
 * -uppermac fixes for lvnet and reset command for cco and sta mode
 * -backup cco working
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
 * Revision 1.5  2013/07/12 08:56:36  ranjan
 * -UKE Push Button Security Feature.
 * Verified : DirectEntry Security Works.Datapath Works.
 *                 command SetSecMode for UKE works.
 * Added against bug-160
 *
 * Revision 1.4  2012/07/08 18:42:20  yuanhua
 * (1)fixed some issues when ctrl layer changes its state from the UCC to ACC. (2) added a event CNSM_START.
 *
 * Revision 1.3  2012/03/11 17:02:24  yuanhua
 * (1) added NEK auth in AKM (2) added NMA (3) modified hpgp layer data structures (4) added Makefile for linux simulation
 *
 * Revision 1.2  2011/09/18 01:32:08  yuanhua
 * designed the AKM for both STA and CCo.
 *
 * Revision 1.1  2011/07/03 06:01:45  jie
 * Initial check in
 *
 * Revision 1.1  2011/05/28 06:31:19  kripa
 * Combining corresponding STA and CCo modules.
 *
 * Revision 1.1  2011/05/06 19:10:12  kripa
 * Adding link layer files to new source tree.
 *
 * Revision 1.3  2011/04/24 03:37:38  kripa
 * Using relative path for 'event.h' inclusion, to avoid conflict with Windows system header file.
 *
 * Revision 1.2  2011/04/23 19:48:45  kripa
 * Fixing stm.h and event.h inclusion, using relative paths to avoid conflict with windows system header files.
 *
 * Revision 1.1  2011/04/08 21:42:45  yuanhua
 * Framework
 *
 *
 * =========================================================*/

