/** ==========================================================
 *
 * @file psm.c
 * 
 *  @brief Network Access Manager 
 *
 *  Copyright (C) 2010-2011, Greenvity Communications, Inc.
 *  All Rights Reserved
 *  
 * =========================================================*/

#if defined(POWERSAVE) || defined(LLP_POWERSAVE)
#ifdef RTX51_TINY_OS
#include <rtx51tny.h>
#endif  //RTX51_TINY_OS
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "papdef.h"
#ifdef ROUTE
#include "hpgp_route.h"
#endif
#include "hpgpdef.h"
#include "linkl.h"
#include "nma.h"
#include "psm.h"
#include "stm.h"
#include "hpgpevt.h"
#include "nma.h"
#include "nma_fw.h"
#include "hpgpapi.h"
#include "fm.h"
#include "ism.h"
#include "muxl.h"
#include "hal.h"
#include "linkl.h"
#include "hal_eth.h"
#include "hybrii_tasks.h"
#include "event_fw.h"
#include "gv701x_osal.h"

u32 psNoTxFrmCnt=0;
u32 psTxFrmCnt=0;
u32 psPlcTxWriteErrCnt=0;
u32 psPlcIdleErrCnt=0;
u32 psPlcTxOKCnt=0;
u32 psPclTxWriteFromBcn=0;
u32 psPclTxWriteFromFrame=0;
u32 psFrmBcnNoTxFrmCnt=0;
u32 psFrmBcnTxFrmCnt=0;
u32 psFrmBcnPlcTxWriteErrCnt=0;
u32 psFrmBcnPlcIdleErrCnt=0;
u32 psFrmBcnPlcTxOKCnt=0;
u32 psNoTxWrongBpFrmCnt=0;
u32 psFrmBcnNoTxWrongBpFrmCnt=0;
u32 psNoTxZeroAwdFrmCnt=0;
u32 psFrmBcnNoTxZeroAwdFrmCnt=0;
u32 earlywakeBPintCnt=0;
u32	bcnStartInt=0;
u32 bcnStartIntandInPS = 0;

u32	psSleepCnt = 0;
u32	psAwakeCnt = 0;
u32	psNotInSleepMode = 0;

/*
 * PSM_resetScbPs: reset the SCB's PS fields to initial values
 */
void PSM_resetScbPs(sScb *scb) 
{
	scb->psState = PSM_PS_STATE_OFF;
	scb->pss = 0xf;
	memset(&scb->commAwd, 0, sizeof(sPsSchedule));
	scb->bpCnt = 0;
	scb->pssi = 0;
}

/*
 * PSM_cvrtPss_Awd: converts format PSS to Awake Window Schedule format
 * 	input:
 *		- u8 tmpPss: PSS to convert
 *  output:
 *		- pCommAwd: ptr where to store converted AWD form
 */
void PSM_cvrtPss_Awd(u8 tmpPss, sPsSchedule *pCommAwd)
{
	u8 awd;
	u8 numAwdMs[8]={1, 2, 3, 4, 5, 6, 8, 10};

	if ((tmpPss & 0xf) == 0xf)
	{
		memset(pCommAwd, 0, sizeof(sPsSchedule));
	}
	else
	{
	// decipher the AWD
		awd = (tmpPss & 0xf0) >> 4;
		if (awd < 8)
			pCommAwd->awdTime = numAwdMs[awd]; // use ms
		else if (awd < 0xf)
		{
			pCommAwd->awdTime = 1 << (awd - 8);
			pCommAwd->awdTime |= 0x80;	// mark last bit as use "# of beacons"
		}
		else
		{
			// should not happen: error
		}
		// decipher the PSP
		pCommAwd->numBp = 1 << (tmpPss & 0xf);
	}
}

// PSM_SetStaPs: enable/disable the Power Save mode flag in reg  PLC Line Control
void PSM_SetStaPsHW(u8 flag)
{
	uPlcLineControlReg plcLineCtrl;

	plcLineCtrl.reg = ReadU32Reg(PLC_LINECTRL_REG);
	plcLineCtrl.s.powerSaveMode = flag;	// Note: caller must take care not to pass invalid flag:0 or 1 only
#ifdef DOTHIS
    plcLineCtrl.s.hybernate = flag;
#endif
    plcLineCtrl.s.earlyWakeEn = flag;
//	printf("PSM_SetStaPsHW: flag=%bu, ctorl(plcLineCtrl.reg)=0x%lx\n", flag, ctorl(plcLineCtrl.reg));
	WriteU32Reg(PLC_LINECTRL_REG, plcLineCtrl.reg);
}

void PSM_enter_deep_sleep_PS()
{
	u8 tmpVal;
    uEthMacTxCtl1Reg        ethMacTxCtl1;
    uEthMacRxCtlReg         ethMacRxCtl;
	sHaLayer                *hal = (sHaLayer*)HOMEPLUG_GetHal();
	sHpgpHalCB              *hhalCb;

	hhalCb = hal->hhalCb;
	hhalCb->psInSleepMode = TRUE;

	// disable AFE first
   	mac_utils_spi_write(0x16, 1);
    mac_utils_spi_write(0x36, 0);

	// disable PHY Tx clock
//    WriteU8Reg(0x420, 0x2A);

	// disable PHY Tx clock & AES 
    WriteU8Reg(0x420, 0xAA);

	// disable PHY Rx clock
  	WriteU8Reg(0x421, 0xB2);
}
																																   
void PSM_exit_deep_sleep_PS()
{
	u8 tmpVal;
    uEthMacTxCtl1Reg        ethMacTxCtl1;
    uEthMacRxCtlReg         ethMacRxCtl;
	sHaLayer                *hal = (sHaLayer*)HOMEPLUG_GetHal();
	sHpgpHalCB              *hhalCb;

	hhalCb = hal->hhalCb;
//	if (hhalCb->psInSleepMode == FALSE)
//		return;

	hhalCb->psInSleepMode = FALSE;

	// enable PHY Rx clock
	WriteU8Reg(0x421, 0);

	// enable PHY Tx clock & AES
   	WriteU8Reg(0x420, 0);

	// enable AFE
    //mac_utils_spi_write(36, 0x90);
	mac_utils_spi_write(0x16, 0x0);
}

#endif // PS || LG_PS

#ifdef POWERSAVE
u32	bcnStartIntExitSleep=0;
u32 EarlyBpNotMod=0;
u32 notEQpsp=0;
extern u8 psDebug;
extern u8 txOff;
extern u8 rxOff;
extern u8 phytxOff;
extern u8 phyrxOff;
extern u8 macClkChange;
extern u8 pllOff;
extern u32 EarlyWakeBcnCnt;
extern u32 EarlyWBPnotEven;
extern u32 EarlyWBPnotMOD;

//extern sHpgpHalCB gHpgpHalCB;

typedef struct PsCnfParam
{
    u8    dstTei;         //destination STA
    u8   *dstMacAddr;     //destination STA
	u8    result;
} sPsCnfParam;

bool PSM_validPss(u8 pss)
{
	u8 awd;

	if ((pss & 0xf) == 0xf)
		// PSP cannot be 0xf: not in PS
			return(FALSE);

	awd = (pss & 0xf0) >> 4;
	// value has to be 0..0xe
	if (awd < 8)
		// AWD in ms
		return(TRUE);
				
	if ((awd > 7) && (awd < 0xf))
	{
		// AWD in beacon periods
		awd -= 8;
		if (awd > (pss & 0xf))
		{
			// AWD bcn period cannot be larger than PSP
			return(FALSE);
		}
		return(TRUE);
	}
	return(FALSE);
}

// CCO Power Save Manager

eStatus CPSM_SendMgmtMsg(sCpsm *cpsm, u16 mmType, void *msgParam)
{
    eStatus           status = STATUS_SUCCESS;
    sEvent            *newEvent = NULL;
    sHpgpHdr          *hpgpHdr = NULL;
    sCcPowersaveCnf   *psCnf = NULL;
    sScb              *scbIter = NULL;
    u8                *dataptr = NULL;
    s8                 freeLen = 0;
    u8                 numSta = 0;
    u16                eventSize = 0;
//    sLinkLayer  *linkl = (sLinkLayer *)HPGPCTRL_GetLayer(HP_LAYER_TYPE_LINK);
    sLinkLayer        *linkl = cpsm->linkl;
    sStaInfo          *staInfo = LINKL_GetStaInfo(linkl);
	sPsCnfParam   *psCnfParm;

    psCnfParm = (sPsCnfParam *)msgParam;  

    switch(mmType)
    {
        case EVENT_TYPE_CC_PWR_SAVE_CNF:
        {
            newEvent = EVENT_MgmtAlloc(HPGP_DATA_PAYLOAD_MIN, 
                                   EVENT_HPGP_MSG_HEADROOM);
            if(newEvent == NULL)
            {
                return STATUS_FAILURE;
            }
            newEvent->eventHdr.eventClass = EVENT_CLASS_MSG;
            newEvent->eventHdr.type = EVENT_TYPE_CC_PWR_SAVE_CNF;

            hpgpHdr = (sHpgpHdr *)newEvent->buffDesc.buff;
            hpgpHdr->tei = psCnfParm->dstTei;
            hpgpHdr->macAddr = psCnfParm->dstMacAddr;
            hpgpHdr->snid = staInfo->snid;
            psCnf = (sCcPowersaveCnf *)(newEvent->buffDesc.dataptr);
			psCnf->result =  psCnfParm->result;
            newEvent->buffDesc.datalen +=MAX(HPGP_DATA_PAYLOAD_MIN, sizeof(sCcPowersaveCnf));
#ifdef PS_PRINT			
            FM_Printf(FM_MMSG, "CPSM: >>> CC_PWR_SAVE.CNF (tei: %bu, result=%bu)\n",
                                hpgpHdr->tei, psCnf->result);
#endif
            break;
        }
        case EVENT_TYPE_CC_PWR_SAVE_EXIT_CNF:
        {
            newEvent = EVENT_MgmtAlloc(HPGP_DATA_PAYLOAD_MIN, 
                                   EVENT_HPGP_MSG_HEADROOM);
            if(newEvent == NULL)
            {
                return STATUS_FAILURE;
            }
            newEvent->eventHdr.eventClass = EVENT_CLASS_MSG;
            newEvent->eventHdr.type = EVENT_TYPE_CC_PWR_SAVE_EXIT_CNF;

            hpgpHdr = (sHpgpHdr *)newEvent->buffDesc.buff;
            hpgpHdr->tei = psCnfParm->dstTei; //TODO
            hpgpHdr->macAddr = psCnfParm->dstMacAddr;
            hpgpHdr->snid = staInfo->snid;
            newEvent->buffDesc.datalen += HPGP_DATA_PAYLOAD_MIN;
#ifdef PS_PRINT				
            FM_Printf(FM_MMSG, "CPSM: >>> CC_PWR_SAVE_EXIT.CNF (tei: %bu)\n",
                                hpgpHdr->tei);
#endif
            break;
        }
        case EVENT_TYPE_CC_STOP_PWR_SAVE_REQ:
        {
            newEvent = EVENT_MgmtAlloc(HPGP_DATA_PAYLOAD_MIN, 
                                   EVENT_HPGP_MSG_HEADROOM);
            if(newEvent == NULL)
            {
                return STATUS_FAILURE;
            }
            newEvent->eventHdr.eventClass = EVENT_CLASS_MSG;
            newEvent->eventHdr.type = EVENT_TYPE_CC_STOP_PWR_SAVE_REQ;

            hpgpHdr = (sHpgpHdr *)newEvent->buffDesc.buff;
            hpgpHdr->tei = psCnfParm->dstTei;
            hpgpHdr->macAddr = psCnfParm->dstMacAddr;
            hpgpHdr->snid = staInfo->snid;
            newEvent->buffDesc.datalen += HPGP_DATA_PAYLOAD_MIN;
#ifdef PS_PRINT				
            FM_Printf(FM_MMSG, "CPSM: >>> CC_STOP_PWR_SAVE.REQ (tei: %bu)\n",
                                hpgpHdr->tei);
#endif
            break;
        }
        default:
        {
        }
    }

    EVENT_Assert(newEvent);

    //transmit the mgmt msg
    status =  MUXL_TransmitMgmtMsg(newEvent);
    //the event will be freed by MUXL if the TX is successful
    if(status == STATUS_FAILURE)
    {
        EVENT_Free(newEvent);
    }
    
    return status;
}

void CPSM_ProcEvent(sCpsm *cpsm, sEvent *event)
{
    sEvent            *newEvent = NULL;
    sHpgpHdr          *hpgpHdr = NULL;
    sHpgpHdr          *newHpgpHdr = NULL;
    sLinkLayer        *linkl = NULL;
    sScb              *scb = NULL;
    sStaInfo          *staInfo = NULL;
	sPsCnfParam 	  psCnfParam;
	sCrm          	  *crm;

    linkl = cpsm->linkl;
    staInfo = LINKL_GetStaInfo(linkl);
	crm = LINKL_GetCrm(linkl);
   

    if(cpsm->state != SPSM_STATE_READY)
    {
//        FM_Printf(FM_WARN, "CPSM is not ready yet\n");
        return;
    }

//    FM_Printf(FM_MMSG, "CPSM_ProcEvent: event->eventHdr.type=0x%x\n", event->eventHdr.type);
    hpgpHdr = (sHpgpHdr *)event->buffDesc.buff;
    if( event->eventHdr.eventClass == EVENT_CLASS_MSG) 
    {
        switch(event->eventHdr.type)
        {
            case EVENT_TYPE_CC_PWR_SAVE_REQ:
            {
				sCcPowersaveReq		*psReq;

                psReq = (sCcPowersaveReq *)event->buffDesc.dataptr; 
                hpgpHdr = (sHpgpHdr *)event->buffDesc.buff;
//                scb = (sScb *)(hpgpHdr->scb);
                scb = CRM_FindScbMacAddr(hpgpHdr->macAddr);
#ifdef PS_PRINT
                FM_Printf(FM_MMSG, "CPSM:<<<CC_PWR_SAVE_REQ (tei: %bu, psp=0x%bx)\n",
                          hpgpHdr->tei, psReq->pss);
#endif
                psCnfParam.result = PS_CNF_RESULT_REJECT;

                if(scb && (scb->namState == STA_NAM_STATE_CONN) && gHpgpHalCB.psAvln)
                {
					// this STA must be associated and AVLN PS must be ON
					if (PSM_validPss(psReq->pss) == TRUE)
					{
                    	psCnfParam.result = PS_CNF_RESULT_ACCEPT;
					}
				}

                psCnfParam.dstTei = scb->tei;
                psCnfParam.dstMacAddr = scb->macAddr;

              	if ( CPSM_SendMgmtMsg(cpsm, MMTYPE_CC_PWR_SAVE_CNF, 
                                     &psCnfParam) == STATUS_SUCCESS)
               	{
                    if (psCnfParam.result == PS_CNF_RESULT_ACCEPT)
					{
						// enable PS mode on this station
						scb->psState = PSM_PS_STATE_ON;
						scb->pss = 	psReq->pss;
						staInfo->ccoScb->pssi++;	// increment CCO's pssi eveytime a station enters or exits PS
						if (staInfo->ccoScb->psState == PSM_PS_STATE_ON)
							// everytime there's a new STA entering PS mode,
							// we must recompute CCO's PSP to make sure it's 
							// equal to the shortest PSP in AVLN
							PSM_set_sta_PS(TRUE, staInfo->ccoScb->pss);	
						PSM_recalc_AW(DEV_MODE_CCO);	// recalculate common Awake Window
//						printf("scb in PS_REQ: %p\n", scb);
					}
				}
				else
				{
//					FM_Printf(FM_MMSG, "CPSM_ProcEvent: CPSM_SendMgmtMsg() returned FAILED\n");
				}
				break;
			}
            case EVENT_TYPE_CC_PWR_SAVE_EXIT_REQ:
            {
                scb = (sScb *)(hpgpHdr->scb);
#ifdef PS_PRINT					
                FM_Printf(FM_MMSG, "CPSM: <<< CC_PWR_SAVE_EXIT_REQ (tei: %bu)\n",
                          hpgpHdr->tei);
#endif
                if(scb && (scb->namState == STA_NAM_STATE_CONN))
                {
					// Has to be from an associated STA
					if (scb->psState == PSM_PS_STATE_ON)
					{
						// this allows CCO to send PS_EXT.CNF to all PS_EXIT.REQ retries
                    	psCnfParam.dstTei = scb->tei;
                    	psCnfParam.dstMacAddr = scb->macAddr;
                		if ( CPSM_SendMgmtMsg(cpsm, MMTYPE_CC_PWR_SAVE_EXIT_CNF, 
                                      	&psCnfParam) != STATUS_SUCCESS)
                		{
//						FM_Printf(FM_WARN, "CPSM_ProcEvent: CPSM_SendMgmtMsg(CC_PWR_SAVE_EXIT_CNF) returned error\n");
						}
						scb->psState = PSM_PS_STATE_OFF;
			        	scb->pss = 0xf;
						staInfo->ccoScb->pssi++;	// increment CCO's pssi eveytime a station enters or exits PS
						PSM_recalc_AW(DEV_MODE_CCO);	// recalculate common Awake Window
					} 
//					else FM_Printf(FM_WARN, "scb->psState is bad: %bu, scb=%p\n", scb->psState, scb); 
				}
				else
				{
/*					if (scb == NULL)
						FM_Printf(FM_MMSG, "SCB is NULL. scb=%p\n", scb);
					else FM_Printf(FM_WARN, "scb->namState=%bu\n", scb->namState);
*/ 
				}
				break;
			}
        	default:
        	{
            //perform no operation
        	}
	    }
    }
	else
	{
		// Control event
        switch(event->eventHdr.type)
        {
            case EVENT_TYPE_CCO_SND_STOP_PS_REQ:
			{
				u8 dtei;

				// start the Stop Power Save request process
				dtei = event->buffDesc.dataptr[0];
			    scb = CRM_GetScb(crm, dtei);
                if(scb && (scb->namState == STA_NAM_STATE_CONN))
          		{
					// STA must be in associated STA mode  
                   	psCnfParam.dstTei = dtei;
                   	psCnfParam.dstMacAddr = scb->macAddr;
                    CPSM_SendMgmtMsg(cpsm, MMTYPE_CC_STOP_PWR_SAVE_REQ, &psCnfParam);
                    STM_StartTimer(cpsm->ackTimer, HPGP_TIME_ACK);
                    cpsm->txRetryCnt = 0;	// reset retry count 
				}
				else
				{
					//FM_Printf(FM_MMSG, " CPSM: bad state for Stop_PS: linkl->staNam.state: %bu\n", linkl->staNam.state);
				}
				break;
			}
            case EVENT_TYPE_TIMER_ACK_IND:
            {
				// timed out waiting for a CC_PWR_SAVE_CNF
                if( cpsm->txRetryCnt <= HPGP_TX_RETRY_MAX)
                { 
                    //resend the message
                    CPSM_SendMgmtMsg(cpsm, MMTYPE_CC_STOP_PWR_SAVE_REQ, NULL);
                    STM_StartTimer(cpsm->ackTimer, HPGP_TIME_ACK);
                    cpsm->txRetryCnt++; 
                }
                else
                {
                    //retry exhausted
                    cpsm->txRetryCnt = 0;
                    cpsm->state = SPSM_STATE_READY;
                }
                break;
            }
			default:
			{
			}
		}
	}
}

/*
eStatus CPSM_Stop(sCpsm *spsm)
{
    spsm->state = SPSM_STATE_INIT;

    return STATUS_SUCCESS;

}
*/


eStatus CPSM_Start(sCpsm *cpsm)
{
	cpsm->state = SPSM_STATE_READY;

    return STATUS_SUCCESS;
}


eStatus CPSM_Init(sCpsm *cpsm, sLinkLayer *linkl)
{
	memset(cpsm, 0, sizeof(sCpsm));
    cpsm->linkl = linkl;
    cpsm->staInfo = LINKL_GetStaInfo(linkl);

    cpsm->state = SPSM_STATE_INIT;

    cpsm->ackTimer = STM_AllocTimer(HP_LAYER_TYPE_LINK, 
                         EVENT_TYPE_TIMER_ACK_IND, linkl);
    if(cpsm->ackTimer == STM_TIMER_INVALID_ID)
    {
        return STATUS_FAILURE;
    }
//	FM_Printf(FM_ERROR, "CPSM: ack timer id: %bu.\n", cpsm->ackTimer);

    return STATUS_SUCCESS;
}

// STA Power Save Manager

eStatus SPSM_SendMgmtMsg(sSpsm *spsm, u16 mmType)
{
    eStatus        status = STATUS_SUCCESS;
    sEvent        *newEvent = NULL;
    sHpgpHdr      *newHpgpHdr = NULL;
    u16            eventSize = 0;
//    sLinkLayer    *linkl = (sLinkLayer *)HPGPCTRL_GetLayer(HP_LAYER_TYPE_LINK);
    sLinkLayer    *linkl = spsm->linkl;
    sStaInfo      *staInfo = LINKL_GetStaInfo(linkl);
	sCcPowersaveReq *psReq;

    if((mmType == EVENT_TYPE_CC_PWR_SAVE_REQ) ||
	   (mmType == EVENT_TYPE_CC_PWR_SAVE_EXIT_REQ) ||
	   (mmType == EVENT_TYPE_CC_STOP_PWR_SAVE_CNF))		// limitation: STA can only send msgs to CCO
	{
    	eventSize = MAX(HPGP_DATA_PAYLOAD_MIN, sizeof(sMgmtMsg));
    	newEvent = EVENT_MgmtAlloc(eventSize, EVENT_HPGP_MSG_HEADROOM);
    	if(newEvent == NULL)
    	{
//        	FM_Printf(FM_ERROR, "Cannot allocate an event.\n");
        	return STATUS_FAILURE;
    	}
    	newEvent->eventHdr.eventClass = EVENT_CLASS_MSG;
        newEvent->eventHdr.type = mmType;

        newHpgpHdr = (sHpgpHdr *)newEvent->buffDesc.buff;
        newHpgpHdr->tei = staInfo->ccoScb->tei;
        newHpgpHdr->macAddr = staInfo->ccoScb->macAddr;
        newHpgpHdr->snid = staInfo->snid;
        if (staInfo->staStatus.fields.authStatus)
        {
            newHpgpHdr->eks = staInfo->nekEks;
        }
        else
        {
            newHpgpHdr->eks = HPGP_EKS_NONE;
        }

    	switch(mmType)
        {
        	case EVENT_TYPE_CC_PWR_SAVE_REQ:
        	{
	            psReq = (sCcPowersaveReq *)(newEvent->buffDesc.dataptr); 
    	        psReq->pss = spsm->tpss;
        	    newEvent->buffDesc.datalen +=MAX(HPGP_DATA_PAYLOAD_MIN, sizeof(sCcPowersaveReq));
#ifdef PS_PRINT				
            	FM_Printf(FM_MMSG, "SPSM:>> CC_PWR_SAVE.REQ (tei: %bu, pss=0x%bx)\n",
                               newHpgpHdr->tei, psReq->pss);
#endif
            	break;
        	}
        	case EVENT_TYPE_CC_PWR_SAVE_EXIT_REQ:
        	{
#ifdef PS_PRINT					
            	FM_Printf(FM_MMSG, "SPSM:>> CC_PWR_SAVE_EXIT.REQ (tei: %bu)\n",
                               newHpgpHdr->tei);
#endif
            	break;
			}
        	case EVENT_TYPE_CC_STOP_PWR_SAVE_CNF:
        	{
#ifdef PS_PRINT					
            	FM_Printf(FM_MMSG, "SPSM:>> CC_STOP_PWR_SAVE.CNF (tei: %bu)\n",
                               newHpgpHdr->tei);
#endif
            	break;
			}
	        default:
    	    {
        	}
        }

    	EVENT_Assert(newEvent);
	    //transmit the mgmt msg
    	status = MUXL_TransmitMgmtMsg(newEvent);
	    //the event is freed by MUXL if the TX is successful
    	if(status == STATUS_FAILURE)
	    {
    	    EVENT_Free(newEvent);
	    }

    	return status;
	}
	else return STATUS_FAILURE;
}

void SPSM_ProcEvent(sSpsm *spsm, sEvent *event)
{
    u8             staType;
    u8             txCcoApptRsp;
    sEvent        *newEvent = NULL;
    sHpgpHdr      *hpgpHdr = NULL;
    uEventBody    eventBody;
    u16           eventType;
    sScb          *tmpScb;
    sLinkLayer    *linkl = spsm->linkl;
    sStaInfo      *staInfo = LINKL_GetStaInfo(linkl);
    sLinkLayer    *linkLayer;
    sHpgpHalCB    *hhalCb = HOMEPLUG_GetHal()->hhalCb;

    hpgpHdr = (sHpgpHdr *)event->buffDesc.buff;
    linkLayer = (sLinkLayer *) HPGPCTRL_GetLayer(HP_LAYER_TYPE_LINK);

//    FM_Printf(FM_MMSG, "SPSM_ProcEvent: event->eventHdr.type=%d, spsm->state=%bu\n", event->eventHdr.type, spsm->state);
    switch(spsm->state)
    {
        case SPSM_STATE_READY:
        {
            if( event->eventHdr.eventClass == EVENT_CLASS_CTRL)
            {
                switch(event->eventHdr.type)
                {
                    case EVENT_TYPE_STA_START_PS:
					{
						// start the Power Save request process
/*
                		hpgpHdr = (sHpgpHdr *)event->buffDesc.buff;
*/
						if (gHpgpHalCB.psAvln &&
						    (linkl->staNam.state == SNAM_STATE_CONN))
                		{
							// the PS for AVLN must be enabled and STA must be in associated STA mode  
//                			spsm->pss.psp = event->buffDesc.dataptr[0] & 0x0f;
//                			spsm->pss.awd = (event->buffDesc.dataptr[0] & 0xf0) >> 4;
							spsm->tpss = staInfo->staScb->pss = event->buffDesc.dataptr[0];

//            				FM_Printf(FM_MMSG, "SPSM_ProcEvent: tpss=0x%bx\n", spsm->tpss);
                            SPSM_SendMgmtMsg(spsm, MMTYPE_CC_PWR_SAVE_REQ);
                            STM_StartTimer(spsm->ackTimer, HPGP_TIME_ACK);
							spsm->state = SPSM_STATE_WAITFOR_CC_PS_CNF;
                            spsm->txRetryCnt = 0;	// reset retry count 
						}
						else
						{
							//FM_Printf(FM_MMSG, " bad state: hhalCb=%p, hhalCb->psAvln=%bu,  linkl=%p, linkl->staNam.state: %bu\n", hhalCb, hhalCb->psAvln,linkl, linkl->staNam.state);
						}
						break;
					}
					default:
					{
					}
				}
			}
			break;
		}
        case SPSM_STATE_WAITFOR_CC_PS_CNF:
        {
            if( event->eventHdr.eventClass == EVENT_CLASS_MSG)
            {
                if (event->eventHdr.type == EVENT_TYPE_CC_PWR_SAVE_CNF)
                {
					sCcPowersaveCnf		*psCnf;
					sPsSchedule tmpCommAwd;

                	psCnf = (sCcPowersaveCnf *)event->buffDesc.dataptr; 
#ifdef PS_PRINT						
                	FM_Printf(FM_MMSG, "CPSM: <<< CC_PWR_SAVE_CNF (tei: %bu, result=0x%bx)\n",
                          	hpgpHdr->tei, psCnf->result);
#endif
                    STM_StopTimer(spsm->ackTimer);
					if (linkl->staNam.state == SNAM_STATE_CONN)
               		{
						// STA must be in associated STA mode 
						if (psCnf->result == PS_CNF_RESULT_ACCEPT)
						{
    						sHpgpHalCB *hhalCb = HOMEPLUG_GetHal()->hhalCb;

							tmpScb = staInfo->staScb;
                            spsm->state = SPSM_STATE_PS_ON;
#ifdef PS_PRINT								
                			FM_Printf(FM_MMSG, "CPSM: <<< CC_PWR_SAVE_CNF: NAM_CONNECTED. staScb=%p, ccoScb=%p, staInfo->staScb->pss=0x%bx\n", 
									tmpScb, staInfo->ccoScb, tmpScb->pss);
#endif
							PSM_cvrtPss_Awd(tmpScb->pss, &tmpCommAwd);  // for now, store PSS in tmp place so datapath_transmitDataPlc()
																		// can still tx
							if (!(tmpScb->bpCnt % tmpCommAwd.numBp))
							{
								// AWD must start in a bp whose bpCnt is  a multiple of its PSP 
#ifdef PS_DEBUG
								if (psDebug)
									FM_Printf(FM_MMSG, "SPSM Rx PS_CNF: Config PS HW for AWD. gHpgpHalCB.halStats.psBpIntCnt=%lu, scb->bpCnt=%d, scb->commAwd.numBp=%d\n", 
											gHpgpHalCB.halStats.psBpIntCnt, tmpScb->bpCnt,  tmpScb->commAwd.numBp);
#endif
								tmpScb->psState = PSM_PS_STATE_ON;
								gHpgpHalCB.halStats.psBpIntCnt = tmpScb->bpCnt;	// sync with CCO's bpCnt
								PSM_ConfigStaPsHW(tmpScb->pss);	
							   	PSM_SetStaPsHW(TRUE);
								PSM_cvrtPss_Awd(tmpScb->pss, &tmpScb->commAwd);	// convert PSS to usable format
    							FM_Printf(FM_MMSG, "STA Power Saving ON\n");
							}
							else
							{
								tmpScb->psState = PSM_PS_STATE_WAITING_ON; // wait for the right bp
#ifdef PS_DEBUG
								if (psDebug)
									FM_Printf(FM_MMSG, "SPSM Rx PS_CNF: set to PSM_PS_STATE_WAITING_ON. gHpgpHalCB.halStats.psBpIntCnt=%lu, scb->bpCnt=%d, scb->commAwd.numBp=%d\n", 
											gHpgpHalCB.halStats.psBpIntCnt, tmpScb->bpCnt,  tmpScb->commAwd.numBp);
#endif
							}
						}
						else
						{
							// rejected, go back to READY state
//							printf("CNF with REJECT, back to READY state\n");
                            spsm->state = SPSM_STATE_READY;
						}
					}
#ifdef PS_PRINT						
					else FM_Printf(FM_MMSG, "CPSM: <<CC_PWR_SAVE_CNF: WRONG STATE.\n");
#endif					
 
				}
				else
				{
//		       		FM_Printf(FM_MMSG, "SPSM_ProcEvent: (1) Invalid event->eventHdr.type 0x%x. State = %bu\n", event->eventHdr.type, spsm->state);
				}
			}
			else
			{
				// Control msgs
                switch(event->eventHdr.type)
                {
                    case EVENT_TYPE_TIMER_ACK_IND:
                    {
						// timed out waiting for a CC_PWR_SAVE_CNF
                        if( spsm->txRetryCnt <= HPGP_TX_RETRY_MAX)
                        { 
                            //resend the message
                            SPSM_SendMgmtMsg(spsm, MMTYPE_CC_PWR_SAVE_REQ);
                            STM_StartTimer(spsm->ackTimer, HPGP_TIME_ACK);
                            spsm->txRetryCnt++; 
                        }
                        else
                        {
                            //retry exhausted
                            spsm->txRetryCnt = 0;
                            spsm->state = SPSM_STATE_READY;
                        }
                        break;
                    }
                    default:
                    {
//       					FM_Printf(FM_MMSG, "SPSM_ProcEvent: (2) Invalid event->eventHdr.type 0x%x. State = %bu\n", event->eventHdr.type, spsm->state);
                    }
                }
            }
			break;
		}
        case SPSM_STATE_PS_ON:
        {
			// Power Save mode is enabled on this station
            if( event->eventHdr.eventClass == EVENT_CLASS_CTRL)
            {
                switch(event->eventHdr.type)
                {
                    case EVENT_TYPE_STA_PS_EXIT_REQ:
					{
						// start the Power Save Exit request process
						if ((linkl->staNam.state == SNAM_STATE_CONN) &&
						     (spsm->state == SPSM_STATE_PS_ON))
                		{
							// STA must be in associated mode and its PS must be ON
                            SPSM_SendMgmtMsg(spsm, MMTYPE_CC_PWR_SAVE_EXIT_REQ);
                            STM_StartTimer(spsm->ackTimer, HPGP_TIME_ACK);
							spsm->state = SPSM_STATE_WAITFOR_CC_PS_EXIT_CNF;
                            spsm->txRetryCnt = 0;	// reset retry count 
						}
						else
						{
							//FM_Printf(FM_MMSG, "ps_exit: wrong state\n");
						}
						break;
					}
					default:
					{
//       					FM_Printf(FM_MMSG, "SPSM_ProcEvent: (3) Invalid event->eventHdr.type 0x%x. State = %bu\n", event->eventHdr.type, spsm->state);
					}		
				}
			}
			else
			{
				// received a Management Message
                switch(event->eventHdr.type)
                {
                    case EVENT_TYPE_CC_STOP_PWR_SAVE_REQ:
					{
						// start the Stop_Power Save request process
#ifdef PS_PRINT							
		                FM_Printf(FM_MMSG, "SPSM: <<CC_STOP_PWR_SAVE_REQ (tei: %bu)\n",
        		                  hpgpHdr->tei);
#endif
						if ((linkl->staNam.state == SNAM_STATE_CONN) &&
						     (spsm->state == SPSM_STATE_PS_ON))
                		{
							// STA must be in associated mode and its PS must be ON
                            SPSM_SendMgmtMsg(spsm, MMTYPE_CC_STOP_PWR_SAVE_CNF);	// send back a Confirm
							PSM_set_sta_PS(FALSE, 0xF);		// send a PS_EXIT.REQ
						}
						else
						{
							//FM_Printf(FM_MMSG, "stop_ps: wrong state\n");
						}
						break;
					}
					default:
					{
#ifdef PS_PRINT							
       					FM_Printf(FM_MMSG, "SPSM_ProcEvent: (7) Invalid event->eventHdr.type 0x%x. State = %bu\n", event->eventHdr.type, spsm->state);
#endif
					}		
				}
			}
			break;
		}
        case SPSM_STATE_WAITFOR_CC_PS_EXIT_CNF:
        {
            if( event->eventHdr.eventClass == EVENT_CLASS_MSG)
            {
                if (event->eventHdr.type == EVENT_TYPE_CC_PWR_SAVE_EXIT_CNF)
                {
#ifdef PS_PRINT	                
                	FM_Printf(FM_MMSG, "CPSM: <<< CC_PWR_SAVE_EXIT_CNF (tei: %bu)\n",
                          	hpgpHdr->tei);
#endif
                    STM_StopTimer(spsm->ackTimer);
					if (linkl->staNam.state == SNAM_STATE_CONN)
               		{
						// STA has to be in associated STA mode
						PSM_resetScbPs(linkl->staInfo.staScb);	// set SCB's PS data to init state 
                        spsm->state = SPSM_STATE_READY;
#ifdef PS_PRINT							
						FM_Printf(FM_MMSG, "STA PS OFF\n");
#endif
					}
				}
				else
				{
//   					FM_Printf(FM_MMSG, "SPSM_ProcEvent: (4) Invalid event->eventHdr.type 0x%x. State = %bu\n", event->eventHdr.type, spsm->state);
				}
			}
			else
			{
				// Control msgs
                switch(event->eventHdr.type)
                {
                    case EVENT_TYPE_TIMER_ACK_IND:
                    {
						// timed out waiting for a CC_PWR_SAVE_CNF
                        if( spsm->txRetryCnt <= HPGP_TX_RETRY_MAX)
                        { 
                            //resend the message
                            SPSM_SendMgmtMsg(spsm, MMTYPE_CC_PWR_SAVE_EXIT_REQ);
                            STM_StartTimer(spsm->ackTimer, HPGP_TIME_ACK);
                            spsm->txRetryCnt++; 
                        }
                        else
                        {
                            //retry exhausted
                            spsm->txRetryCnt = 0;
                            spsm->state = SPSM_STATE_READY;
							PSM_resetScbPs(linkl->staInfo.staScb);	// set SCB's PS data to init state 
                        }
                        break;
                    }
                    default:
                    {
//       					FM_Printf(FM_MMSG, "SPSM_ProcEvent: (5) Invalid event->eventHdr.type 0x%x. State = %bu\n", event->eventHdr.type, spsm->state);
                    }
                }
            }
			break;
		}
		default:
		{
//       		FM_Printf(FM_MMSG, "SPSM_ProcEvent: (6) Invalid event->eventHdr.type 0x%x. State = %bu\n", event->eventHdr.type, spsm->state);
		}
	}
}

eStatus SPSM_Stop(sSpsm *spsm)
{
    sLinkLayer    *linkl = spsm->linkl;

    if (spsm->state == SPSM_STATE_PS_ON)
	{
		PSM_set_sta_PS(FALSE, 0xF);		
//		PSM_resetScbPs(scb);	// set SCB's PS data to init state 
	}
    spsm->state = SPSM_STATE_INIT;

    return STATUS_SUCCESS;
}


eStatus SPSM_Start(sSpsm *spsm)
{
    sLinkLayer    *linkl = spsm->linkl;

    spsm->state = SPSM_STATE_READY;
    return STATUS_SUCCESS;
}


eStatus SPSM_Init(sSpsm *spsm, sLinkLayer *linkl)
{
	memset(spsm, 0, sizeof(sSpsm));
    spsm->linkl = linkl;
    spsm->staInfo = LINKL_GetStaInfo(linkl);

    spsm->state = SPSM_STATE_INIT;

    spsm->ackTimer = STM_AllocTimer(HP_LAYER_TYPE_LINK, 
                         EVENT_TYPE_TIMER_ACK_IND, linkl);
    if(spsm->ackTimer == STM_TIMER_INVALID_ID)
    {
        return STATUS_FAILURE;
    }
//	FM_Printf(FM_ERROR, "SPSM: ack timer id: %bu.\n", spsm->ackTimer);

    return STATUS_SUCCESS;
}

void PSM_getLargerPSS(u8 *pThisPss, u8 thatPss)
{
//	printf("PSM_getLargerPSS: compare 0x%bx and 0x%bx\n", *pThisPss, thatPss);
	if ((*pThisPss & 0xf) == 0xf)
	{
		*pThisPss = thatPss;
		return;
	}
	if ((thatPss & 0xf) ==  0xf)
		return;

	// First find the largest PSP
	if ((thatPss & 0xf) > (*pThisPss & 0xf))	  // PSP
	{
		*pThisPss = (*pThisPss & 0xf0) + (thatPss & 0xf);
	}
	// then find the smallest AWD
	if ((thatPss & 0xf0) < (*pThisPss & 0xf0))	  // AWD
	{
		*pThisPss = (*pThisPss & 0xf) + (thatPss & 0xf0);
	}
}

void PSM_get_shortest_PSP(sScb *myScb)
{
    sScb          *scb = NULL;
    sLinkLayer    *linkLayer = (sLinkLayer *)HPGPCTRL_GetLayer(HP_LAYER_TYPE_LINK);
    sStaInfo      *staInfo = LINKL_GetStaInfo(linkLayer);
    sCrm          *crm = LINKL_GetCrm(linkLayer);
	u8			   tmpPsp = myScb->pss & 0xF;

   	scb = CRM_GetNextScb(crm, scb);
    while(scb)
	{
		if (scb->tei != myScb->tei)
		{
			if (scb->psState == PSM_PS_STATE_ON)
			{
				if ((scb->pss & 0xf) < tmpPsp)
				{
					tmpPsp = scb->pss & 0xf;
				}
			}
			else
			{
				// there's at least 1 station with no PS, return 0: shortest PSP
				myScb->pss &= 0xF0;
				return;
			}
		}
        scb = CRM_GetNextScb(crm, scb);
	}
	myScb->pss &= 0xF0;
	myScb->pss |= tmpPsp;
}

/*
 * PSM_recalc_AW(): finds the common Awake Window in the AVLN. The algorithm is first to 
 * find the largest PSP and then find the smallest AWD. The new tuple {largest PSP, smallest AWD}
 * is the common AW
 * 	input:
 *		- u8 devType: either CCO or STA
 *  output:
 *		- common AWD saved in scb->commAwd
 *		- u8 common PSS
 */
void PSM_recalc_AW(u8 devType)
{ 
    sEvent *newEvent = NULL;
    sScb          *scb = NULL;
    sScb          *myScb;
    sLinkLayer    *linkLayer = (sLinkLayer *)HPGPCTRL_GetLayer(HP_LAYER_TYPE_LINK);
    sStaInfo      *staInfo = LINKL_GetStaInfo(linkLayer);
    sCrm          *crm = LINKL_GetCrm(linkLayer);
	u8			  tmpPss;
	bool     	  psExist = FALSE;

//printf("PSM_recalc_AW: devType=%bu\n", devType);
	if (devType == DEV_MODE_STA)
	{
		// if STA, only compare its pss with CCO's
		if (((myScb = staInfo->staScb) != NULL) && (staInfo->ccoScb != NULL))
		{
			tmpPss = myScb->pss;
			PSM_getLargerPSS(&tmpPss, staInfo->ccoScb->pss);
		} else return;
	}
	else
	{
		if ((myScb = staInfo->ccoScb) != NULL)
		{
			tmpPss = myScb->pss;
	    	scb = CRM_GetNextScb(crm, scb);
		    while(scb)
   			{
				if (scb->psState == PSM_PS_STATE_ON)
				{
//					printf("current pss=0x%bx, tmpPss=0x%bx\n", scb->pss, tmpPss);
					if ((tmpPss & 0xf) == 0xf)
					{
						// this stations is not in PS
						tmpPss = scb->pss;
					}
					else PSM_getLargerPSS(&tmpPss, scb->pss);
					psExist = TRUE;
				}
		        scb = CRM_GetNextScb(crm, scb);
			}

			if (psExist == FALSE)
			{
				// no PS stations in AVLN 
				memset(&myScb->commAwd, 0, sizeof(sPsSchedule));
				return;
			}
		} else return;
	}

	PSM_cvrtPss_Awd(tmpPss, &myScb->commAwd);
//	printf("PSM_recalcSWD: scb=%p, scb->commAwd.awdTime=0x%bx,scb->commAwd.numBp=0x%x\n", myScb, myScb->commAwd.awdTime,myScb->commAwd.numBp);
}

// PSM_ConfigStaPs: configures the PS AWD and PSP in reg. Power Save
void PSM_ConfigStaPsHW(u8 pss)
{
    uPlcPowerPsaveReg plcPSMode;

//printf("PSM_ConfigStaPsHW: pss=0x%bx\n", pss);	
	plcPSMode.reg = 0;
	plcPSMode.s.PSP = pss & 0x0f;
	plcPSMode.s.AWD = pss >> 4;

	WriteU32Reg(PLC_POWERSAVE_REG, plcPSMode.reg);
	
}

// PSM_set_sta_PS() enables/disables the PS on this station
// parameters:
//     setFlag: TRUE: enable PS for this station, FALSE: disable PS
//     tpss: this station's PS Schedule  
void PSM_set_sta_PS(u8 setFlag, u8 pss)
{ 
    sEvent *newEvent = NULL;
    sScb          *scb = NULL;
    sLinkLayer    *linkLayer = (sLinkLayer *)HPGPCTRL_GetLayer(HP_LAYER_TYPE_LINK);
    sStaInfo      *staInfo = LINKL_GetStaInfo(linkLayer);

//printf("PSM_set_sta_PS: setFlag=%bu, pss=0x%bx\n", setFlag, pss);
    if (linkLayer->mode == LINKL_STA_MODE_CCO)
	{
		scb = staInfo->ccoScb;
		scb->psState = setFlag;
		scb->pss = pss;
		if (setFlag && ((scb->pss & 0xf) > 0))
		{
			// CCO's PSP must be the the shortest in AVLN 
			PSM_get_shortest_PSP(scb);
			if (!(scb->bpCnt % scb->commAwd.numBp))
			{
				// AWD must start in a bp whose bpCnt is  a multiple of its PSP 
				scb->psState = PSM_PS_STATE_ON;
				PSM_ConfigStaPsHW(scb->pss);	
			   	PSM_SetStaPsHW(TRUE);
			}
			else
			{
				scb->psState = PSM_PS_STATE_WAITING_ON; // wait for the right bp
			}
		}
		scb->pssi++;
		PSM_recalc_AW(DEV_MODE_CCO);	// recalculate common Awake Window
	}
	else
	{
		// this station is STA
	   	newEvent = EVENT_Alloc(0, 0);
   		if(newEvent)
	   	{
    	   	newEvent->eventHdr.eventClass = EVENT_CLASS_CTRL;
			if (setFlag == TRUE)
			{
				// enable Power Save 
       			newEvent->eventHdr.type = EVENT_TYPE_STA_START_PS;
        		newEvent->buffDesc.dataptr[0] = pss;
	        	newEvent->buffDesc.datalen += 1;
			}
			else
			{
    		   	newEvent->eventHdr.type = EVENT_TYPE_STA_PS_EXIT_REQ;
			}
	       	LINKL_SendEvent(linkLayer, newEvent);
   		}
//		else FM_Printf(FM_MMSG, "PSM_set_stat_PS: Falied to allocate an event block\n");
	}

	if (setFlag == FALSE)
	{
		// config pss to HW only when setting OFF PS
		// When setting PS ON, config HW only after
		// receive a PS_REQ_CNF from CCO
		PSM_ConfigStaPsHW(pss);	
	   	PSM_SetStaPsHW(setFlag);
	}

}

// PSM_ForcePsOff(sScb *scb): disable the PS in a station by 1) initialize its scb, and 2) reconfigure the HW PS regs
void PSM_ForcePsOff(sScb *scb)
{
    sLinkLayer    *linkLayer = (sLinkLayer *)HPGPCTRL_GetLayer(HP_LAYER_TYPE_LINK);

	PSM_resetScbPs(scb);	// set SCB's PS data to init state 
	PSM_ConfigStaPsHW(0xF);	// set PSS to 0xF	
   	PSM_SetStaPsHW(FALSE);	// set PS mode option in PLC Line Cycle Control reg. to Disabled
    SPSM_Start(&linkLayer->staPsm);	// set SPSM back to Ready state
}

// PSM_psAvln() enables/disables the PS of AVLN
// parameters:
//     setFlag: TRUE: enable PS of AVLN, FALSE: disable PS
bool PSM_psAvln(u8 setFlag)
{
	u8  input[10];
	u8	tmpVal = 0, sLen;
    sScb          *scb;
    sLinkLayer    *linkLayer = (sLinkLayer *)HPGPCTRL_GetLayer(HP_LAYER_TYPE_LINK);
    sStaInfo      *staInfo = LINKL_GetStaInfo(linkLayer);
    sCrm          *crm = LINKL_GetCrm(linkLayer);

    if (linkLayer->hal->hhalCb->devMode != DEV_MODE_CCO)
	{
		return FALSE;
	}

	if (!setFlag && linkLayer->hal->hhalCb->psAvln)
	{
		// if want to disable PS AVLN, and it's currently enabled, then
		// go through all STAs and disable their PS
		scb = NULL;
    	scb = CRM_GetNextScb(crm, scb);
	    while(scb)
    	{
			if (scb->psState == PSM_PS_STATE_ON)
			{
				if (scb == staInfo->ccoScb)
				{
					// this is the CCO, force PS off
					PSM_ForcePsOff(scb);
				}
				else
				{
				 	scb->psState = 	PSM_PS_STATE_OFF;
					scb->pss = 0xF;
				}
		    }
        	scb = CRM_GetNextScb(crm, scb);
    	}
	}

    linkLayer->hal->hhalCb->psAvln = setFlag;

	return TRUE;
}

void PSM_showStat()
{
    sLinkLayer    *linkLayer = (sLinkLayer *)HPGPCTRL_GetLayer(HP_LAYER_TYPE_LINK);
    sStaInfo      *staInfo = LINKL_GetStaInfo(linkLayer);
	sScb *scb;

    if (linkLayer->mode == LINKL_STA_MODE_CCO)
	{
		// this station is CCO
		scb = staInfo->ccoScb;
	}
	else
	{
		// this station is STA
		scb = staInfo->staScb;
	}
#ifdef PS_PRINT	
	os_switch_task();
	FM_Printf(FM_MMSG, "Frame Tx Stat:\n");
	FM_Printf(FM_MMSG, "psPclTxWriteFromFrame=%lu\n", psPclTxWriteFromFrame);
	FM_Printf(FM_MMSG, "psNoTxFrmCnt=%lu\n", psNoTxFrmCnt);
	os_switch_task();
	FM_Printf(FM_MMSG, "psNoTxWrongBpFrmCnt=%lu\n", psNoTxWrongBpFrmCnt);
	FM_Printf(FM_MMSG, "psNoTxZeroAwdFrmCnt=%lu\n", psNoTxZeroAwdFrmCnt);
	FM_Printf(FM_MMSG, "psTxFrmCnt=%lu\n", psTxFrmCnt);
	os_switch_task();
	FM_Printf(FM_MMSG, "psPlcTxWriteErrCnt=%lu\n", psPlcTxWriteErrCnt);
	FM_Printf(FM_MMSG, "psPlcIdleErrCnt=%lu\n", psPlcIdleErrCnt);
	os_switch_task();
	FM_Printf(FM_MMSG, "psPlcTxOKCnt=%lu\n\n", psPlcTxOKCnt);
	FM_Printf(FM_MMSG, "psPclTxWriteFromBcn=%lu\n", psPclTxWriteFromBcn);
	FM_Printf(FM_MMSG, "psFrmBcnNoTxFrmCnt=%lu\n", psFrmBcnNoTxFrmCnt);
	os_switch_task();
	FM_Printf(FM_MMSG, "psFrmBcnNoTxWrongBpFrmCnt=%lu\n", psFrmBcnNoTxWrongBpFrmCnt);
	FM_Printf(FM_MMSG, "psFrmBcnNoTxZeroAwdFrmCnt=%lu\n", psFrmBcnNoTxZeroAwdFrmCnt);
	FM_Printf(FM_MMSG, "psFrmBcnTxFrmCnt=%lu\n", psFrmBcnTxFrmCnt);
	os_switch_task();
	FM_Printf(FM_MMSG, "psFrmBcnPlcTxWriteErrCnt=%lu\n", psFrmBcnPlcTxWriteErrCnt);
	FM_Printf(FM_MMSG, "psFrmBcnPlcIdleErrCnt=%lu\n", psFrmBcnPlcIdleErrCnt);
	os_switch_task();
	FM_Printf(FM_MMSG, "psFrmBcnPlcTxOKCnt=%lu\n\n", psFrmBcnPlcTxOKCnt);
	FM_Printf(FM_MMSG, "\nDebug Counters:\n");
	FM_Printf(FM_MMSG, "EarlyWakeBcnCnt=%lu\n", EarlyWakeBcnCnt);
	os_switch_task();
	FM_Printf(FM_MMSG, "EarlyWBPnotEven=%lu\n", EarlyWBPnotEven);
	FM_Printf(FM_MMSG, "EarlyWBPnotMOD=%lu\n", EarlyWBPnotMOD);
	FM_Printf(FM_MMSG, "EarlyBpNotMod=%lu\n", EarlyBpNotMod);
	os_switch_task();
	FM_Printf(FM_MMSG, "notEQpsp=%lu\n", notEQpsp);
	FM_Printf(FM_MMSG, "gHpgpHalCB.halStats.psBpIntCnt=%lu\n", gHpgpHalCB.halStats.psBpIntCnt);
	FM_Printf(FM_MMSG, "gHpgpHalCB.psInSleepMode=%bu\n", gHpgpHalCB.psInSleepMode);
	os_switch_task();
	FM_Printf(FM_MMSG, "earlywakeBPintCnt=%lu\n", earlywakeBPintCnt);
	if (scb)
		FM_Printf(FM_MMSG, "scb->bpCnt=%d\n", scb->bpCnt);
	FM_Printf(FM_MMSG, "bcnStartInt=%lu\n", bcnStartInt);
	FM_Printf(FM_MMSG, "bcnStartIntExitSleep=%lu\n", bcnStartIntExitSleep);
	os_switch_task();
#endif
}

void PSM_clearStat()
{
	psNoTxFrmCnt=0;
	psTxFrmCnt=0;
	psPlcTxWriteErrCnt=0;
	psPlcIdleErrCnt=0;
	psPlcTxOKCnt=0;
    psPclTxWriteFromBcn=0;
    psPclTxWriteFromFrame=0;
    psFrmBcnNoTxFrmCnt=0;
    psFrmBcnTxFrmCnt=0;
    psFrmBcnPlcTxWriteErrCnt=0;
    psFrmBcnPlcIdleErrCnt=0;
	psFrmBcnPlcTxOKCnt=0;
	psNoTxWrongBpFrmCnt=0;
	psNoTxZeroAwdFrmCnt=0;
	psFrmBcnNoTxWrongBpFrmCnt=0;
	psFrmBcnNoTxZeroAwdFrmCnt=0;
	EarlyWBPnotEven = 0;
	EarlyWBPnotMOD = 0;
	EarlyBpNotMod = 0;
	earlywakeBPintCnt=0;
	notEQpsp=0;
	bcnStartInt=0;
	bcnStartIntExitSleep=0;
}

void PSM_psDisplayPsList(u8 devMode)	
{

    u8				i=1;
	u8				j;
    sScb          *scb;
    sLinkLayer    *linkl = (sLinkLayer *)HPGPCTRL_GetLayer(HP_LAYER_TYPE_LINK);
    sStaInfo          *staInfo = LINKL_GetStaInfo(linkl);
    sHpgpHalCB *hhalCb = HOMEPLUG_GetHal()->hhalCb;
    sCrm          *crm = LINKL_GetCrm(linkl);
	u8			  tmpStr[20];

	if (devMode == DEV_MODE_CCO)
		scb = staInfo->ccoScb;
	else scb = staInfo->staScb;

	if (scb->commAwd.awdTime & 0x80)
	{
		strcpy(tmpStr,"BPs");
	}
	else
	{
		strcpy(tmpStr,"msecs"); 
	}
		
    FM_Printf(FM_MMSG, "AVLN PS Mode %s\n", hhalCb->psAvln ? "ON":"OFF");
   	FM_Printf(FM_MMSG, "This STA's common AWD=%bu %s, common PSP=%d BPs (PSS=0x%bx)\n",  
				scb->commAwd.awdTime & 0xF, tmpStr, scb->commAwd.numBp, scb->pss);

	scb = NULL;
    scb = CRM_GetNextScb(crm, scb);
    while(scb)
    {
		if (scb->psState == PSM_PS_STATE_ON)
		{
			if (i == 1)
			{			
				FM_Printf(FM_MMSG, "List of STA in PS:\n");
			}
			
	        FM_Printf(FM_MMSG, "\t%bu) Mac Address : ", i++);

    	    for (j = 0; j < 6; j++)
        	{
            	FM_Printf(FM_MMSG, "%02bx  ", scb->macAddr[j]);
	        }
        
        	FM_Printf(FM_MMSG, "\n");         
        	FM_Printf(FM_MMSG, "\t   tei: %bu  \tpss: 0x%bx\n", scb->tei, scb->pss);
	    }
        scb = CRM_GetNextScb(crm, scb);
    }
	
	if (!i)
		FM_Printf(FM_MMSG, "No STA in PS Mode\n");
	
}

// PSM_psAvln() enables/disables the PS of AVLN
// parameters:
//     setFlag: TRUE: enable PS of AVLN, FALSE: disable PS
bool PSM_stop_sta_PS(u8 dtei)
{
    sEvent *newEvent = NULL;
    sScb          *scb = NULL;
    sLinkLayer    *linkLayer = (sLinkLayer *)HPGPCTRL_GetLayer(HP_LAYER_TYPE_LINK);
	sCrm          *crm = LINKL_GetCrm(linkLayer);

//	printf ("PSM_stopPs: dtei %bu\n", dtei);
    scb = CRM_GetScb(crm, dtei);
	if (scb == NULL)
	{
//	printf ("PSM_stopPs: dtei %bu does not exist\n", dtei);
		return FALSE;
	}

   	newEvent = EVENT_Alloc(0, 0);
	if(newEvent)
   	{
   	   	newEvent->eventHdr.eventClass = EVENT_CLASS_CTRL;
		newEvent->eventHdr.type = EVENT_TYPE_CCO_SND_STOP_PS_REQ;
   		newEvent->buffDesc.dataptr[0] = dtei;
       	newEvent->buffDesc.datalen += 1;
       	LINKL_SendEvent(linkLayer, newEvent);
	}
	else
	{
//		printf("PSM_stopPs: no Event allocated\n");
		return FALSE;
	}

	return TRUE;
}
#endif /* 	POWERSAVE */

#ifdef LLP_POWERSAVE

// PSM_ConfigStaPs: configures the PS AWD and PSP in reg. Power Save
void PSM_ConfigStaPsHW(u8 pss, u8 devMode)
{
    uPlcPowerPsaveReg plcPSMode;

//printf("PSM_ConfigStaPsHW: pss=0x%bx\n", pss);	
	plcPSMode.reg = 0;
	plcPSMode.s.PSP = pss & 0x0f;
	plcPSMode.s.AWD = pss >> 4;
	if (devMode == LINKL_STA_MODE_STA)
	{
		// thi is to fix the bug where STA always starts PS 1 bp later than CCO
		plcPSMode.s.BPCnt_lo = 1;
	}

	WriteU32Reg(PLC_POWERSAVE_REG, plcPSMode.reg);
	
}

void PSM_enable_PS(sScb *scb, u8 pss, u16 bpCnt, u8 devMode)
{
	scb->psState = PSM_PS_STATE_ON;
	scb->pss = pss;
	scb->bpCnt = bpCnt;
	PSM_ConfigStaPsHW(pss, devMode);	
   	PSM_SetStaPsHW(TRUE);
	earlywakeBPintCnt = 1;
}

// PSM_ForcePsOff(sScb *scb): disable the PS in a station by 1) initialize its scb, and 2) reconfigure the HW PS regs
void PSM_ForcePsOff(sScb *scb)
{
    sLinkLayer    *linkLayer = (sLinkLayer *)HPGPCTRL_GetLayer(HP_LAYER_TYPE_LINK);

	PSM_resetScbPs(scb);	// set SCB's PS data to init state 
	PSM_ConfigStaPsHW(0xF);	// set PSS to 0xF	
   	PSM_SetStaPsHW(FALSE);	// set PS mode option in PLC Line Cycle Control reg. to Disabled
	PSM_exit_deep_sleep_PS();
}

void PSM_save_PS_to_HalCB(u8 psState, u8 pss)
{
    sHaLayer *hal = (sHaLayer*)HOMEPLUG_GetHal();
	sHpgpHalCB *hhalCb;

	hhalCb = hal->hhalCb;
	hhalCb->savedPsMode.psState = psState;
	hhalCb->savedPsMode.pss = pss;
}

void PSM_copy_PS_from_HalCB(sScb *scb)
{
    sHaLayer *hal = (sHaLayer*)HOMEPLUG_GetHal();
	sHpgpHalCB *hhalCb;

	hhalCb = hal->hhalCb;
	scb->psState = hhalCb->savedPsMode.psState;
	scb->pss = 	hhalCb->savedPsMode.pss;
}

// PSM_set_sta_PS() enables/disables the PS on this station
// parameters:
//     setFlag: TRUE: enable PS for this station, FALSE: disable PS
//     tpss: this station's PS Schedule  
void PSM_set_sta_PS(u8 setFlag, u8 pss)
{ 
    sEvent *newEvent = NULL;
    sScb          *scb = NULL;
    sLinkLayer    *linkLayer = (sLinkLayer *)HPGPCTRL_GetLayer(HP_LAYER_TYPE_LINK);
    sStaInfo      *staInfo = LINKL_GetStaInfo(linkLayer);

//printf("PSM_set_sta_PS: setFlag=%bu, pss=0x%bx\n", setFlag, pss);
    if (linkLayer->mode == LINKL_STA_MODE_CCO)
	{
		scb = staInfo->ccoScb;
		PSM_resetScbPs(scb);	// set SCB's PS data to init state 
		scb->psState = setFlag;
		scb->pss = pss;
		if (setFlag && ((scb->pss & 0xf) > 0))
		{
			scb->psState = PSM_PS_STATE_ON;
			PSM_cvrtPss_Awd(scb->pss, &scb->commAwd);
			PSM_ConfigStaPsHW(scb->pss);	
		   	PSM_SetStaPsHW(TRUE);
		}
//		scb->pssi++;
//		PSM_recalc_AW(DEV_MODE_CCO);	// recalculate common Awake Window

		// save to global memory in case of rescan
		PSM_save_PS_to_HalCB(PSM_PS_STATE_ON, pss);
	}
	else
	{
		// for STA, enable PS mode means to set the psSta to ON
		// STA always follows CCO's PS mode iff its psSta flag = ON
		scb = staInfo->staScb;
		PSM_resetScbPs(scb);	// set SCB's PS data to init state 
		linkLayer->hal->hhalCb->psSta = setFlag;
	}

	if (setFlag == FALSE)
	{
		// disable PS mode
		PSM_ForcePsOff(scb);
		PSM_save_PS_to_HalCB(PSM_PS_STATE_OFF, PSM_PSS_NOT_CONFIG);	// save PS data to HALCB
	}

	FM_Printf(FM_MMSG, "STA Power Save Mode is now %s\n", setFlag == TRUE ? "ON":"OFF");

}

void PSM_showStat()
{
    sLinkLayer    *linkLayer = (sLinkLayer *)HPGPCTRL_GetLayer(HP_LAYER_TYPE_LINK);
    sStaInfo      *staInfo = LINKL_GetStaInfo(linkLayer);
	sScb *scb;

    if (linkLayer->mode == LINKL_STA_MODE_CCO)
	{
		// this station is CCO
		scb = staInfo->ccoScb;
	}
	else
	{
		// this station is STA
		scb = staInfo->staScb;
	}
#ifdef PS_PRINT	
	os_switch_task();
	FM_Printf(FM_MMSG, "Frame Tx Stat:\n");
	FM_Printf(FM_MMSG, "psPclTxWriteFromFrame=%lu\n", psPclTxWriteFromFrame);
	FM_Printf(FM_MMSG, "psNoTxFrmCnt=%lu\n", psNoTxFrmCnt);
	os_switch_task();
	FM_Printf(FM_MMSG, "psNoTxWrongBpFrmCnt=%lu\n", psNoTxWrongBpFrmCnt);
	FM_Printf(FM_MMSG, "psNoTxZeroAwdFrmCnt=%lu\n", psNoTxZeroAwdFrmCnt);
	FM_Printf(FM_MMSG, "psTxFrmCnt=%lu\n", psTxFrmCnt);
	os_switch_task();
	FM_Printf(FM_MMSG, "psPlcTxWriteErrCnt=%lu\n", psPlcTxWriteErrCnt);
	FM_Printf(FM_MMSG, "psPlcIdleErrCnt=%lu\n", psPlcIdleErrCnt);
	os_switch_task();
	FM_Printf(FM_MMSG, "psPlcTxOKCnt=%lu\n\n", psPlcTxOKCnt);
	FM_Printf(FM_MMSG, "psPclTxWriteFromBcn=%lu\n", psPclTxWriteFromBcn);
	FM_Printf(FM_MMSG, "psFrmBcnNoTxFrmCnt=%lu\n", psFrmBcnNoTxFrmCnt);
	os_switch_task();
	FM_Printf(FM_MMSG, "psFrmBcnNoTxWrongBpFrmCnt=%lu\n", psFrmBcnNoTxWrongBpFrmCnt);
	FM_Printf(FM_MMSG, "psFrmBcnNoTxZeroAwdFrmCnt=%lu\n", psFrmBcnNoTxZeroAwdFrmCnt);
	FM_Printf(FM_MMSG, "psFrmBcnTxFrmCnt=%lu\n", psFrmBcnTxFrmCnt);
	os_switch_task();
	FM_Printf(FM_MMSG, "psFrmBcnPlcTxWriteErrCnt=%lu\n", psFrmBcnPlcTxWriteErrCnt);
	FM_Printf(FM_MMSG, "psFrmBcnPlcIdleErrCnt=%lu\n", psFrmBcnPlcIdleErrCnt);
	os_switch_task();
	FM_Printf(FM_MMSG, "psFrmBcnPlcTxOKCnt=%lu\n\n", psFrmBcnPlcTxOKCnt);
	FM_Printf(FM_MMSG, "\nDebug stats:\n");
	FM_Printf(FM_MMSG, "gHpgpHalCB.psInSleepMode=%bu\n", gHpgpHalCB.psInSleepMode);
	FM_Printf(FM_MMSG, "psSleepCnt=%lu\n", psSleepCnt);
	FM_Printf(FM_MMSG, "psAwakeCnt=%lu\n", psAwakeCnt);
	FM_Printf(FM_MMSG, "psNotInSleepMode=%lu\n", psNotInSleepMode);
	FM_Printf(FM_MMSG, "bcnStartInt=%lu\n\n", bcnStartInt);
	FM_Printf(FM_MMSG, "bcnStartIntandInPS=%lu\n\n", bcnStartIntandInPS);
/*
	FM_Printf(FM_MMSG, "\n\nDebug Counters:\n");
	FM_Printf(FM_MMSG, "EarlyWakeBcnCnt=%lu\n", EarlyWakeBcnCnt);
	os_switch_task();
	FM_Printf(FM_MMSG, "EarlyWBPnotEven=%lu\n", EarlyWBPnotEven);
	FM_Printf(FM_MMSG, "EarlyWBPnotMOD=%lu\n", EarlyWBPnotMOD);
	FM_Printf(FM_MMSG, "EarlyBpNotMod=%lu\n", EarlyBpNotMod);
	os_switch_task();
	FM_Printf(FM_MMSG, "notEQpsp=%lu\n", notEQpsp);
	FM_Printf(FM_MMSG, "gHpgpHalCB.halStats.psBpIntCnt=%lu\n", gHpgpHalCB.halStats.psBpIntCnt);
	FM_Printf(FM_MMSG, "gHpgpHalCB.psInSleepMode=%bu\n", gHpgpHalCB.psInSleepMode);
	os_switch_task();
	FM_Printf(FM_MMSG, "earlywakeBPintCnt=%lu\n", earlywakeBPintCnt);
	if (scb)
		FM_Printf(FM_MMSG, "scb->bpCnt=%d\n", scb->bpCnt);
	FM_Printf(FM_MMSG, "bcnStartInt=%lu\n", bcnStartInt);
	FM_Printf(FM_MMSG, "bcnStartIntExitSleep=%lu\n", bcnStartIntExitSleep);
*/
	os_switch_task();
#endif
}

void PSM_clearStat()
{
	psNoTxFrmCnt=0;
	psTxFrmCnt=0;
	psPlcTxWriteErrCnt=0;
	psPlcIdleErrCnt=0;
	psPlcTxOKCnt=0;
    psPclTxWriteFromBcn=0;
    psPclTxWriteFromFrame=0;
    psFrmBcnNoTxFrmCnt=0;
    psFrmBcnTxFrmCnt=0;
    psFrmBcnPlcTxWriteErrCnt=0;
    psFrmBcnPlcIdleErrCnt=0;
	psFrmBcnPlcTxOKCnt=0;
	psNoTxWrongBpFrmCnt=0;
	psNoTxZeroAwdFrmCnt=0;
	psFrmBcnNoTxWrongBpFrmCnt=0;
	psFrmBcnNoTxZeroAwdFrmCnt=0;
	earlywakeBPintCnt=0;
	bcnStartInt=0;
	bcnStartIntandInPS=0;
	psSleepCnt = 0;
	psAwakeCnt = 0;
	psNotInSleepMode=0;
}

void PSM_psDisplayPsList(u8 devMode)	
{

    u8				i=1;
	u8				j;
    sScb          *scb;
    sLinkLayer    *linkl = (sLinkLayer *)HPGPCTRL_GetLayer(HP_LAYER_TYPE_LINK);
    sStaInfo          *staInfo = LINKL_GetStaInfo(linkl);
    sHpgpHalCB *hhalCb = HOMEPLUG_GetHal()->hhalCb;
    sCrm          *crm = LINKL_GetCrm(linkl);
	u8			  tmpStr[20];

	if (devMode == DEV_MODE_CCO)
		scb = staInfo->ccoScb;
	else scb = staInfo->staScb;

	if (scb->commAwd.awdTime & 0x80)
	{
		strcpy(tmpStr,"BPs");
	}
	else
	{
		strcpy(tmpStr,"msecs"); 
	}
		
    FM_Printf(FM_MMSG, "PS Mode is %s\n", scb->psState == PSM_PS_STATE_ON ? "ON":"OFF");
   	FM_Printf(FM_MMSG, "This STA's common AWD=%bu %s, common PSP=%d BPs (PSS=0x%bx)\n",  
				scb->commAwd.awdTime & 0xF, tmpStr, scb->commAwd.numBp, scb->pss);
}

#endif // LG_PS
