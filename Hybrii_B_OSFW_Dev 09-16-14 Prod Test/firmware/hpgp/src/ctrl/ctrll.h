/** ========================================================
 *
 *  @file ctrll.h
 * 
 *  @brief Control Layer  
 *
 *  Copyright (C) 2010-2011, Greenvity Communications, Inc.
 *  All Rights Reserved
 *  
 * ==========================================================*/

#ifndef _CTRLL_H
#define _CTRLL_H

#include "list.h"
#include "papdef.h"
#include "sched.h"
#include "hpgpevt.h"

#include "linkl.h"

#define MAX_SYSTEM_NAME  32
#define VER_SIZE         20
typedef union _deviceCap_t {
    struct 
	{       
        u8        ccoCap:           2;    //CCo capability level
        u8        proxyNetCap:      1;    //proxy networking capability
        u8        backupCcoCap:     1;    //backup Cco capability
        u8        Rsvd:             4;
        
        u8        greenPhyCap;
		u8        powerSaveCap;
		u8        repeaterRouting;
		u8        HPAVVersion;
        u8        bridgeSupported;
    } fields;
    u16    val;
} deviceCap_t;


typedef struct _sysProfile_t
{
	u8 systemName[MAX_SYSTEM_NAME];
	u8 macAddress[MAC_ADDR_LEN];
	u8 swVersion[VER_SIZE];
	
	u8 ethPortEn: 1;
	u8 spiPort:   1;
	u8 lineMode: 1;  //AC or DC
	u8 lineFreq: 1;  // 50 or 60
	u8 devMode:      2;       // Auto, STA, CCo	
	u8 lastdevMode:  2;

    u8 lastUserAppCCOState;
    u8 secLevel;
	u8 ukeEnable;
    
	u8 powerSaveMode; //Disable, Short_Sleep, Max_PS  2 Bits
	u8 advPowerSaveMode; //Wake Time, Sleep Time
	deviceCap_t cap;
	u8 devicePassword[MAX_DPW_LEN];
	u8 UKEEn;
	u8 nmk[ENC_KEY_LEN];
	u8 nid[NID_LEN];
	u8 zigbeePortEn;
	u8 zigbeeAddr[8];
	u8 plcCalibrationParam[20];
    u8 plcBaseBandParam[20];
    u8 zigbeeBaseBandParam[20];

}__PACKED__ sysProfile_t ;

enum ctrlAgent
{
    CTRLL_TRANS_MCTRL,
    CTRLL_AGENT_NDC,
    CTRLL_AGENT_USC,
    CTRLL_AGENT_ASC,
    CTRLL_AGENT_UCC,
    CTRLL_AGENT_ACC,
};



/* MCTRL state definition */
enum mctrlState 
{
    MCTRL_STATE_INIT,
    MCTRL_STATE_NET_DISC,
    MCTRL_STATE_UNASSOC_STA,
    MCTRL_STATE_ASSOC_STA,
    MCTRL_STATE_UNASSOC_CCO,
    MCTRL_STATE_ASSOC_CCO,
    MCTRL_STATE_MAX
};



/* Main Controller */
typedef struct mctrl
{
    u8 state;
	u8 nextState;
} sMctrl, *psMctrl;

eStatus MCTRL_Init(sMctrl *mctrl);


/* Network Discovery Controller */
enum ndcState 
{
    NDC_STATE_INIT,
    NDC_STATE_READY,
    NDC_STATE_WAITFOR_NET_ACC_RSP
};

typedef struct ndc
{
    u8 state;

    u8  restart:     1;
    u8  rsvd:        7;
    u16 minCcoScanTime;
    u16 maxCcoScanTime;
    u16 minStaScanTime;
    u16 maxStaScanTime;

    //BBT timer
    tTimerId bbtTimer;
} sNdc, *psNdc;



eStatus NDC_Init(sNdc *ndc);


/* Unassociated STA Controller */
enum uscState
{
    USC_STATE_INIT,
    USC_STATE_READY,
    USC_STATE_WAITFOR_NET_ACC_RSP
};

typedef struct usc
{
    // state
    u8 state;
    u16 maxDiscoverPeriod;
    //DISC timer
    tTimerId discTimer;
} sUsc, *psUsc;


eStatus USC_Init(sUsc *usc);


/* Unassociated CCO Controller */
enum uccState
{
    UCC_STATE_INIT,
    UCC_STATE_READY,
};


typedef struct ucc
{
    u8 state;
} sUcc, *psUcc;

eStatus UCC_Init(sUcc *ucc);



/* Associated STA Controller */
enum ascState
{
    ASC_STATE_INIT,
    ASC_STATE_READY,
};



typedef struct asc
{
    u8 state;
} sAsc, *psAsc;


eStatus ASC_Init(sAsc *asc);
void ASC_CcoHo(sAsc *asc);


/* Associated CCO Controller */
enum accState
{
    ACC_STATE_INIT,
    ACC_STATE_READY,
};

typedef struct acc
{
    u8        state;
    u16       maxDiscoverPeriod;
    tTimerId  joinTimer;
} sAcc, *psAcc;

eStatus ACC_Init(sAcc *acc);


/* Control Layer */
typedef struct ctrlLayer
{
    sMctrl   mainCtrl;
    sNdc     netDiscCtrl;
    sUsc     uStaCtrl;
    sAsc     aStaCtrl;
    sUcc     uCcoCtrl;
    sAcc     aCcoCtrl;

#ifndef P8051
#if defined(WIN32) || defined(_WIN32)
    HANDLE   ctrlSem;
#else //POSIX
    sem_t    ctrlSem;
#endif
#endif


    sSlist   eventQueue;     /* external event queue */
    sSlist   intEventQueue;  /* internal event queue */


//    void    (*deliverEvent)(void XDATA *eventcookie, sEvent XDATA *event);
    void     *eventcookie;

//    sExecTask  task;
	u8 pendingEvent;

} sCtrlLayer, *psCtrlLayer;



void CTRLL_RegisterEventCallback(sCtrlLayer *ctrlLayer, 
    void (*deliverEvent)(void *eventcookie, sEvent *event),
    void *eventcookie);


eStatus CTRLL_SendEvent(sCtrlLayer *ctrll, sEvent *event) __REENTRANT__;

u8    CTRLL_Proc(void *cookie);
eStatus CTRLL_GetKey(sCtrlLayer *ctrlLayer, u8 *nmk, u8 *nid);
eStatus CTRLL_SetKey(sCtrlLayer *ctrlLayer, u8 *nmk, u8 *nid);

eStatus CTRLL_StartNetDisc(sCtrlLayer *ctrlLayer);
eStatus CTRLL_StartNetwork(sCtrlLayer *ctrlLayer, u8 type, u8 *nid);
eStatus CTRLL_NetExit(sCtrlLayer *ctrlLayer);
eStatus CTRLL_SendAssocReq(sCtrlLayer *ctrlLayer);
eStatus CTRLL_Init(sCtrlLayer *ctrlLayer);
bool MCTRL_IsAssociated(void);
eStatus CTRLL_SendEventToLinkLayer( sCtrlLayer *, 
            enum eventType , 
            void *);
eStatus CTRLL_setSecMode(sCtrlLayer *ctrlLayer, u8 secMode);
void CTRLL_SetDefaultNid(void);


#endif



/** =========================================================
 *
 * Edit History
 *
 * $Source: /home/cvsrepo/Hybrii_B_OSFW_Dev/firmware/hpgp/src/ctrl/ctrll.h,v $
 *
 * $Log: ctrll.h,v $
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
 * Revision 1.4  2014/06/05 08:38:41  ranjan
 * -flash function enabled for uppermac
 * - commit command after any change would flash systemprofiles
 * - verfied upper mac
 *
 * Revision 1.3  2014/05/16 08:52:30  kiran
 * - System Profile Flashing API's Added. Upper MAC functionality tested
 *
 * Revision 1.2  2014/02/19 10:22:40  ranjan
 * - common sync for hal_tst and upper mac project
 * - ism.c is MAC interrupt handler for hhal_tst and upper mac.
 *    chal_ext1isr function   is removed
 * - verified : lower mac sync, upper mac sync data traffic.
 *
 * Revision 1.1  2013/12/18 17:04:59  yiming
 * no message
 *
 * Revision 1.1  2013/12/17 21:47:26  yiming
 * no message
 *
 * Revision 1.4  2013/09/04 14:50:33  yiming
 * New changes for Hybrii_A code merge
 *
 * Revision 1.9  2013/07/12 08:56:36  ranjan
 * -UKE Push Button Security Feature.
 * Verified : DirectEntry Security Works.Datapath Works.
 *                 command SetSecMode for UKE works.
 * Added against bug-160
 *
 * Revision 1.8  2013/04/04 12:21:54  prashant
 * Detecting PLC link failure for HMC. added project for HMC and Renesas
 *
 * Revision 1.7  2012/06/20 17:48:31  kripa
 *
 * Committed on the Free edition of March Hare Software CVSNT Client.
 * Upgrade to CVS Suite for more features and support:
 * http://march-hare.com/cvsnt/
 *
 * Revision 1.6  2012/05/24 05:08:18  yuanhua
 * define sendEvent functions in CTRL/LINK layer as reentrant.
 *
 * Revision 1.5  2012/04/17 23:09:50  yuanhua
 * fixed compiler errors for the hpgp hal test due to the integration changes.
 *
 * Revision 1.4  2012/04/13 06:15:11  yuanhua
 * integrate the HPGP protocol stack with the HAL for the beacon TX/RX and mgmt msg RX.
 *
 * Revision 1.3  2012/03/11 17:02:24  yuanhua
 * (1) added NEK auth in AKM (2) added NMA (3) modified hpgp layer data structures (4) added Makefile for linux simulation
 *
 * Revision 1.2  2011/09/09 07:02:31  yuanhua
 * migrate the firmware code from the greenchip to the hybrii.
 *
 * Revision 1.4  2011/08/12 23:13:21  yuanhua
 * (1)Added Control Layer (2) Fixed bugs for user-selected CCo handover (3) Made changes to SNAM/CNAM and SNSM/CNSM for CCo handover switch (from CCo to STA, from STA to CCo, and from STA to STA but with different CCo) and post CCo handover
 *
 * Revision 1.3  2011/08/09 22:45:44  yuanhua
 * changed to event structure, seperating HPGP-related events from the general event defination so that the general event could be used for other purposes than the HPGP.
 *
 * Revision 1.2  2011/05/28 06:33:00  kripa
 * *** empty log message ***
 *
 * Revision 1.1  2011/05/06 19:07:48  kripa
 * Adding ctrl layer files to new source tree.
 *
 * Revision 1.1  2011/04/08 21:43:29  yuanhua
 * Framework
 *
 *
 * =========================================================*/

