#ifndef _PSM_H
#define _PSM_H

#include "hpgpevt.h"
#include "papdef.h"
#include "hpgpdef.h"
//#include "mmsg.h"

//struct linkLayer;

#define PS_CNF_RESULT_ACCEPT	0
#define PS_CNF_RESULT_REJECT	1
#define PSM_PSS_NOT_CONFIG		0xF
#define HPGP_TIME_ACK            1000     //100 ms
#define MAX_NUM_PS_STA 20


enum 
{
	// values of scb->psState
	PSM_PS_STATE_OFF,
	PSM_PS_STATE_WAITING_ON,
	PSM_PS_STATE_ON
};

// STA Power Save Mgr definitions
enum
{ 
        SPSM_STATE_INIT, 
        SPSM_STATE_READY, 
        SPSM_STATE_WAITFOR_CC_PS_CNF, 
        SPSM_STATE_PS_ON, 
        SPSM_STATE_WAITFOR_CC_PS_EXIT_CNF, 
}; 

typedef struct spsm 
{ 
    struct linkLayer *linkl;
    sStaInfo   *staInfo;

    //state 
    u8 state; 

    u16 bpCnt;
//    u8  pssi;    // PS State Id 
//    sPss  tpss;  // Transmitter's PS Schedule 
    u8 tpss;       // Transmitter's PS Schedule 
	u8 txRetryCnt;
    tTimerId  ackTimer; 

} sSpsm, *psSpsm;    

// CCO Power Save Mgr definitions
enum{ 
    CPSM_STATE_INIT, 
    CPSM_STATE_READY 
}; 

typedef struct staPss 
{
    u8   tei;    // STA's TEI
//    sPss pss;    // STA's PSS  
} sStaPss, *psStaPss;

typedef struct cpsm 
{ 
    struct linkLayer *linkl;
    sStaInfo   *staInfo;

    u8 state; //state
    u8 pssi;  // PS State Id
	u8 txRetryCnt;
    tTimerId  ackTimer; 
} sCpsm, *psCpsm;    

#if defined(POWERSAVE) || defined(LLP_POWERSAVE)
void PSM_resetScbPs(sScb *scb); 
void PSM_SetStaPsHW(u8 flag);
void PSM_ForcePsOff(sScb *scb);
void PSM_cvrtPss_Awd(u8 tmpPss, sPsSchedule *pCommAwd);
eStatus SPSM_Start(sSpsm *spsm);
eStatus SPSM_Stop(sSpsm *spsm);
eStatus SPSM_Init(sSpsm *spsm, struct linkLayer *linkl);
void PSM_enter_deep_sleep_PS();
void PSM_exit_deep_sleep_PS();
#endif

#ifdef POWERSAVE
void CPSM_ProcEvent(sCpsm *cpsm, sEvent *event);
eStatus CPSM_Start(sCpsm *cpsm);
eStatus CPSM_Init(sCpsm *cpsm, struct linkLayer *linkl);
void SPSM_ProcEvent(sSpsm *spsm, sEvent *event);
void PSM_set_sta_PS(bool setFlag, u8 pss);
bool PSM_psAvln(u8 setFlag);
void PSM_ConfigStaPsHW(u8 pss);
void PSM_recalc_AW(u8 devType);
void PSM_psDisplayPsList(u8 devType);	
void PSM_TxTest();
void PSM_showStat();
void PSM_clearStat();
void PSM_getLargerPSS(u8 *pThisPss, u8 thatPss);
bool PSM_stop_sta_PS(u8 dtei);
void EHAL_Print_ethHWStat();
void EHAL_Clear_ethHWStat();
#else
#ifdef LLP_POWERSAVE
void PSM_ConfigStaPsHW(u8 pss, u8 devMode);
void PSM_enable_PS(sScb *scb, u8 pss, u16 bpCnt, u8 devMode);
void PSM_save_PS_to_HalCB(u8 psState, u8 pss);
void PSM_copy_PS_from_HalCB(sScb *scb);
void PSM_psDisplayPsList(u8 devType);	
void PSM_set_sta_PS(bool setFlag, u8 pss);
void PSM_showStat();
void PSM_clearStat();
#endif
#endif //else

#endif

