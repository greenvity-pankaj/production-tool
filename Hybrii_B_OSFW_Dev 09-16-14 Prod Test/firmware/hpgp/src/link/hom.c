/** =========================================================
 *
 *  @file chom.c
 * 
 *  @brief CCO Handover Manager 
 *
 *  Copyright (C) 2010-2011, Greenvity Communications, Inc.
 *  All Rights Reserved
 *  
 * ===========================================================*/

#include "papdef.h"
#ifdef ROUTE
#include "hpgp_route.h"
#endif
#include "hom.h"
#include "mmsg.h"
#include "crm.h"
#include "nsm.h"
#include "hpgpapi.h"
#include "linkl.h"
#include "fm.h"
#include "timer.h"
#include "stm.h"
#include "muxl.h"
#include <string.h>
#include "hybrii_tasks.h"

#define HPGP_TIME_HO          2     //2 seconds
#define HPGP_HO_TYPE_SOFT     0x0
#define HPGP_HO_TYPE_HARD     0x1


extern void LINKL_TimerHandler(u16 type, void *cookie);

#ifdef CCO_FUNC
/* ------------------------
 * CCO handover manager
 * --------------------- */

eStatus CHOM_SendMgmtMsg(sChom *chom, u16 mmType, sScb *staScb)
{
    eStatus           status = STATUS_FAILURE;
    sEvent            *newEvent = NULL;
    sHpgpHdr          *newHpgpHdr = NULL;
    sCcHoReq          *ccHoReq = NULL;
    sCcHoStaInfo      *ccHoStaInfo = NULL;
    u8                numSta = 0; 
    sScb              *scbIter = NULL;
    sStaInfo          *staInfo = NULL;
    u8                *dataptr = NULL;
    sCrm              *crm = NULL;
    u8                i; 
    u16               eventSize = 0;

    crm  = LINKL_GetCrm(chom->linkl);
    staInfo =  LINKL_GetStaInfo(chom->linkl);

    switch(mmType)
    {
        case EVENT_TYPE_CC_HANDOVER_REQ:
        {
            eventSize = MAX(HPGP_DATA_PAYLOAD_MIN, sizeof(sCcHoReq));
            newEvent = EVENT_MgmtAlloc(eventSize, EVENT_HPGP_MSG_HEADROOM);
            if(newEvent == NULL)
            {
                FM_Printf(FM_ERROR, "EAllocErr\n");
                return STATUS_FAILURE;
            }

            newEvent->eventHdr.eventClass = EVENT_CLASS_MSG;
            newEvent->eventHdr.type = EVENT_TYPE_CC_HANDOVER_REQ;

            newHpgpHdr = (sHpgpHdr *)newEvent->buffDesc.buff;
            newHpgpHdr->tei = staScb->tei;
            newHpgpHdr->macAddr = staScb->macAddr;
            newHpgpHdr->snid = staInfo->snid;

            ccHoReq = (sCcHoReq *)(newEvent->buffDesc.dataptr);
            ccHoReq->hoType = HPGP_HO_TYPE_HARD;
            ccHoReq->reason = staScb->hoReason;
            newEvent->buffDesc.datalen +=sizeof(sCcHoReq);
            FM_Printf(FM_MMSG, "CHOM:>>>CC_HANDOVER.REQ.\n");
            break;
        }
        case EVENT_TYPE_CC_HANDOVER_INFO_IND:
        {
            numSta = CRM_GetScbNum(crm);
            eventSize = MAX(HPGP_DATA_PAYLOAD_MIN, 3+ (u16)(numSta*sizeof(sCcHoStaInfo))); 
            newEvent = EVENT_MgmtAlloc(eventSize, EVENT_HPGP_MSG_HEADROOM);
            if(newEvent == NULL)
            {
                FM_Printf(FM_ERROR, "EAllocErr\n");
                return STATUS_FAILURE;
            }

            newEvent->eventHdr.eventClass = EVENT_CLASS_MSG;
            newEvent->eventHdr.type = EVENT_TYPE_CC_HANDOVER_INFO_IND;

            newHpgpHdr = (sHpgpHdr *)newEvent->buffDesc.buff;
            newHpgpHdr->tei = staScb->tei;
            newHpgpHdr->macAddr = staScb->macAddr;
            newHpgpHdr->snid = staInfo->snid;

            dataptr = newEvent->buffDesc.dataptr;
            *dataptr = 0;   //handover in progress
            dataptr++;
            *dataptr = 0;   //no backup CCo
            dataptr++;
            *dataptr = numSta;  
            dataptr++;
            newEvent->buffDesc.datalen = 3;

            while(numSta)
            {
                ccHoStaInfo = (sCcHoStaInfo *)dataptr;   
                scbIter = CRM_GetNextScb(crm, scbIter); 
                if(scbIter)
                {
                    ccHoStaInfo->tei = scbIter->tei;
                    memcpy(ccHoStaInfo->macAddr, scbIter->macAddr, MAC_ADDR_LEN);
                    ccHoStaInfo->status = scbIter->staStatus.fields.authStatus;
                    //TODO: search the discovered sta list of the scb 
                    //to find the cco is in the list; Otherwise find the PCCO
                    for (i = 0; i < scbIter->numDiscSta; i++)
                    {
          
                    } 
                    ccHoStaInfo->ptei = 0; 
                    dataptr += sizeof(sCcHoStaInfo);
                    newEvent->buffDesc.datalen += sizeof(sCcHoStaInfo);
                    numSta--;
                }
                else
                {
                    FM_Printf(FM_ERROR, "CHOM:error in CRM\n");
                    break;  //should not happen
                }
            }
            newEvent->buffDesc.datalen = MAX(HPGP_DATA_PAYLOAD_MIN, newEvent->buffDesc.datalen);
            FM_Printf(FM_MMSG, "CHOM:>>>CC_HANDOVER_INFO.IND\n");
            break;
        }
        default:
        {
        }
    }

    EVENT_Assert(newEvent);
    //transmit CC_HANDOVER.REQ
    status = MUXL_TransmitMgmtMsg(HPGPCTRL_GetLayer(HP_LAYER_TYPE_MUX), newEvent);
    //the event is freed by MUXL if the TX is successful
    if(status == STATUS_FAILURE)
    {
        EVENT_Free(newEvent);
    }
    
    return status;
}

eStatus CHOM_SendHoRspEvent(sChom *chom, sScb* staScb, u8 result)
{
    sEvent            *newEvent = NULL;
    sHpgpHdr          *newHpgpHdr = NULL;
    sCcoHoRspEvent    *ccoHoRspEvent = NULL;
    sLinkLayer        *linkl = chom->linkl;

    if(result == FALSE)
    {
        STM_FreeTimer(staScb->staTimer);
        staScb->staTimer = STM_TIMER_INVALID_ID;
        staScb->txRetryCnt = 0; //reset
    }

    newEvent = EVENT_MgmtAlloc(sizeof(sCcoHoRspEvent), EVENT_HPGP_CTRL_HEADROOM);
    if(newEvent == NULL)
    {
        FM_Printf(FM_ERROR, "EAllocErr\n");
        return STATUS_FAILURE;
    }

    newEvent->eventHdr.eventClass = EVENT_CLASS_CTRL;
    newEvent->eventHdr.type = EVENT_TYPE_CCO_HO_RSP;

    newHpgpHdr = (sHpgpHdr *)newEvent->buffDesc.buff;
    newHpgpHdr->scb = staScb;
    ccoHoRspEvent = (sCcoHoRspEvent *)(newEvent->buffDesc.dataptr);
    ccoHoRspEvent->reason = staScb->hoReason;
    ccoHoRspEvent->result = result;
    newEvent->buffDesc.datalen += sizeof(sCcoHoRspEvent);

    //LINKL_SendEvent(linkl, newEvent);
    SLIST_Put(&linkl->intEventQueue, &newEvent->link);

    return STATUS_SUCCESS;

}

void CHOM_ProcEvent(sChom *chom, sEvent *event)
{
//    sEvent            *newEvent = NULL;
//    sScb     *scbIter = NULL;
//    eStatus   status = STATUS_FAILURE;
    sCcHoCnf          *ccHoCnf = NULL;
    sCnsm             *cnsm = NULL;
    sCcoHoReqEvent    *ccoHoReqEvent = NULL;
    sHpgpHdr          *hpgpHdr = NULL;
    sScb              *staScb = NULL; 
//    u8                *dataptr = NULL;
    hpgpHdr = (sHpgpHdr *)event->buffDesc.buff;
    staScb = hpgpHdr->scb;
    
    cnsm =  LINKL_GetCnsm(chom->linkl);

    switch(staScb->homState)
    {
        case STA_HOM_STATE_IDLE:
        {
            if(event->eventHdr.eventClass == EVENT_CLASS_CTRL)
            {
                switch(event->eventHdr.type)
                {
                    case EVENT_TYPE_CCO_HO_REQ: 
                    {
                        //process events
#ifdef POWERSAVE
						if (chom->linkl->hal->hhalCb->psAvln)
						{
    						PSM_psAvln(FALSE);	// CCO to send Stop PS Flag = 1 to force all STAs to disable their PS
							chom->linkl->hal->hhalCb->disPsAvln = TRUE;	// mark it so we can enable AVLN PS when we receive  CC_HANDOVER_CNF
						}
#endif
                        ccoHoReqEvent = (sCcoHoReqEvent *)(event->buffDesc.dataptr);
                        staScb->hoReason = ccoHoReqEvent->reason;
#ifdef CALLBACK
                        staScb->staTimer = STM_AllocTimer(LINKL_TimerHandler, 
                                               EVENT_TYPE_TIMER_HO_IND, staScb);
#else
                        staScb->staTimer = STM_AllocTimer(HP_LAYER_TYPE_LINK,
                                               EVENT_TYPE_TIMER_HO_IND, staScb);
#endif
                        if(staScb->staTimer !=  STM_TIMER_INVALID_ID )
                        {
                            CHOM_SendMgmtMsg(chom, EVENT_TYPE_CC_HANDOVER_REQ, staScb);
                            //if whether or not transmission succeeds,  
                            //always do the following
                            staScb->staTimerType = STA_TIMER_TYPE_HO;
                            STM_StartTimer(staScb->staTimer, HPGP_TIME_HO);
                            staScb->txRetryCnt++;
                            staScb->homState = STA_HOM_STATE_WAITFOR_CC_HO_CNF;
                        }
                        else
                        {
                            //send negative CCO_HO_RSP event
                            staScb->homState = STA_HOM_STATE_IDLE;
                            CHOM_SendHoRspEvent(chom, staScb, FALSE);
                        }
                        break;
                    }
                    default:
                    {
                    }
                }
            }
            else   //mgmt msg
            {
                //no one
            }
            break;
        }
        case STA_HOM_STATE_WAITFOR_CC_HO_CNF:
        {
            //process events
#ifdef POWERSAVE
			if (chom->linkl->hal->hhalCb->disPsAvln)
			{
				PSM_psAvln(TRUE);	// turn AVLN PS mode back on
				chom->linkl->hal->hhalCb->disPsAvln = FALSE;
			}
#endif
            if(event->eventHdr.eventClass == EVENT_CLASS_MSG)
            {
                switch(event->eventHdr.type)
                {
                    case EVENT_TYPE_CC_HANDOVER_CNF:
                    {
                        FM_Printf(FM_MMSG, "CHOM:<<<CC_HANDOVER.CNF\n");
                        STM_StopTimer(staScb->staTimer);
                        ccHoCnf = (sCcHoCnf *)(event->buffDesc.dataptr);
                        if(ccHoCnf->result == 0x00)
                        {
                            //accept
                            CHOM_SendMgmtMsg(chom, 
                                             EVENT_TYPE_CC_HANDOVER_INFO_IND, 
                                             staScb);
                            //if whether or not transmission succeeds,  
                            //always do the following
                            staScb->staTimerType = STA_TIMER_TYPE_HO;
                            STM_StartTimer(staScb->staTimer, HPGP_TIME_HO);
                            staScb->txRetryCnt++;

                            //set HOIP in the beacon
                            CNSM_EnableHo(cnsm, TRUE);
                            //TODO: call CNAM not to process CC_ASSOC.REQ

                            staScb->homState = 
                                STA_HOM_STATE_WAITFOR_CC_HO_INFO_RSP;

                            CHOM_SendHoRspEvent(chom, staScb, TRUE);
                        }
                        else
                        {
                            //TODO: the CCO may select another CCO if available
                            //At present, send negative CCO_HO_RSP event
                            staScb->homState = STA_HOM_STATE_IDLE;
                            CHOM_SendHoRspEvent(chom, staScb, FALSE);
                        }
                        break;
                    }
                    default:
                    {
                    }
                }
            }
            else   //control msg
            {
                switch(event->eventHdr.type)
                {
                    case EVENT_TYPE_TIMER_HO_IND:  //ho timer expired
                    {
                        if( staScb->txRetryCnt <= HPGP_TX_RETRY_MAX)
                        {
                            //resend the message
                            CHOM_SendMgmtMsg(chom, EVENT_TYPE_CC_HANDOVER_REQ, staScb);
                            STM_StartTimer(staScb->staTimer, HPGP_TIME_HO);
                            staScb->txRetryCnt++;
                            //stay in the same state
                        }
                        else
                        {
                            //retry exhausted
                            staScb->homState = STA_HOM_STATE_IDLE;
                            CNSM_EnableHo(cnsm, FALSE);
                            CHOM_SendHoRspEvent(chom, staScb, FALSE);
/*
                            STM_FreeTimer(staScb->staTimer);
                            staScb->txRetryCnt = 0; //reset

                            newEvent = EVENT_Alloc(1, 0);
                            if(newEvent == NULL)
                            {
                                FM_Printf(FM_ERROR, "Cannot allocate an event.\n");
                                break;
                            }
                            //send the negative CCO_ event
*/
#ifdef POWERSAVE
							if (chom->linkl->hal->hhalCb->disPsAvln)
							{
								PSM_psAvln(TRUE);	// turn AVLN PS mode back on
								chom->linkl->hal->hhalCb->disPsAvln = FALSE;
							}
#endif
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
        case STA_HOM_STATE_WAITFOR_CC_HO_INFO_RSP:
        {
            //process events
            if(event->eventHdr.eventClass == EVENT_CLASS_MSG)
            {
                switch(event->eventHdr.type)
                {
                    case EVENT_TYPE_CC_HANDOVER_INFO_RSP:
                    {
                        FM_Printf(FM_MMSG, "CHOM:<<<CC_HANDOVER_INFO.RSP\n");
                        STM_StopTimer(staScb->staTimer);
                        STM_FreeTimer(staScb->staTimer);

                        //Start the HO counter down                        
                        CNSM_StartHo(cnsm, staScb->tei);

                        staScb->homState = STA_HOM_STATE_IDLE;
//                        CHOM_SendHoRspEvent(chom, staScb, TRUE);
                        break;
                    }
                    default:
                    {
                    }
                }
            }
            else   //control msg
            {
                switch(event->eventHdr.type)
                {
                    case EVENT_TYPE_TIMER_HO_IND:  //ho timer expired
                    {
                        if( staScb->txRetryCnt <= HPGP_TX_RETRY_MAX)
                        {
                            //resend the message
                            CHOM_SendMgmtMsg(chom, 
                                             EVENT_TYPE_CC_HANDOVER_INFO_IND, 
                                             staScb);
                            STM_StartTimer(staScb->staTimer, HPGP_TIME_HO);
                            staScb->txRetryCnt++;
                            //stay in the same state
                        }
                        else
                        {
                            //retry exhausted
                            staScb->homState = STA_HOM_STATE_IDLE;
                            STM_FreeTimer(staScb->staTimer);
                            staScb->txRetryCnt = 0; //reset
                            CNSM_EnableHo(cnsm, FALSE);
//                            CHOM_SendHoRspEvent(chom, staScb, FALSE);
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
        default:
        {
        }
    }
}



eStatus CHOM_Init(sChom *chom, sLinkLayer *linkl)
{
    chom->linkl = linkl;
    chom->state = CHOM_STATE_READY;
    return STATUS_SUCCESS;
}

#endif /* CCO_FUNC */


#ifdef STA_FUNC

/* -------------------
 * STA handover manager
 * --------------------- */
void LINKL_HoTimerHandler(void* cookie)
{
    sEvent *event = NULL;
    sLinkLayer * linkl = (sLinkLayer *)cookie;
    //Generate a time event
    event = EVENT_Alloc(EVENT_DEFAULT_SIZE, EVENT_HPGP_CTRL_HEADROOM);
    if(event == NULL)
    {
        FM_Printf(FM_ERROR, "EAllocErr\n");
        return;
    }
    event->eventHdr.type = EVENT_TYPE_TIMER_HO_IND;
    //post the event to the event queue
    LINKL_SendEvent(linkl, event);
}


eStatus SHOM_SendMgmtMsg(sShom *shom, u16 mmType)
{
    eStatus           status = STATUS_FAILURE;
    sEvent            *newEvent = NULL;
    sHpgpHdr          *newHpgpHdr = NULL;
    sCcHoCnf          *ccHoCnf = NULL;
    sStaInfo          *staInfo = NULL;
    u16               eventSize = 0;

    staInfo =  LINKL_GetStaInfo(shom->linkl);
    switch(mmType)
    {
        case EVENT_TYPE_CC_HANDOVER_CNF:
        {
            eventSize = MAX(HPGP_DATA_PAYLOAD_MIN, sizeof(sCcHoCnf));
            newEvent = EVENT_MgmtAlloc(eventSize, EVENT_HPGP_MSG_HEADROOM);
            if(newEvent == NULL)
            {
                FM_Printf(FM_ERROR, "EAllocErr\n");
                return STATUS_FAILURE;
            }
            newEvent->eventHdr.eventClass = EVENT_CLASS_MSG;
            newEvent->eventHdr.type = EVENT_TYPE_CC_HANDOVER_CNF;

            newHpgpHdr = (sHpgpHdr *)newEvent->buffDesc.buff;
            newHpgpHdr->tei = staInfo->ccoScb->tei;
            newHpgpHdr->macAddr = staInfo->ccoScb->macAddr;
            newHpgpHdr->snid = staInfo->snid;

            ccHoCnf = (sCcHoCnf *)(newEvent->buffDesc.dataptr);
            ccHoCnf->result = shom->hoResult;
            newEvent->buffDesc.datalen = eventSize;
            FM_Printf(FM_MMSG, "SHOM:>>>CC_HANDOVER.CNF(tei: %d)\n",
                                newHpgpHdr->tei);
            break;
        }
        case EVENT_TYPE_CC_HANDOVER_INFO_RSP:
        {
            newEvent = EVENT_MgmtAlloc(HPGP_DATA_PAYLOAD_MIN, EVENT_HPGP_MSG_HEADROOM);
            if(newEvent == NULL)
            {
                FM_Printf(FM_ERROR, "EAllocErr\n");
                return STATUS_FAILURE;
            }
            newEvent->eventHdr.eventClass = EVENT_CLASS_MSG;
            newEvent->eventHdr.type = EVENT_TYPE_CC_HANDOVER_INFO_RSP;

            newHpgpHdr = (sHpgpHdr *)newEvent->buffDesc.buff;
            newHpgpHdr->tei = staInfo->ccoScb->tei;
            newHpgpHdr->macAddr = staInfo->ccoScb->macAddr;
            newHpgpHdr->snid = staInfo->snid;
            newEvent->buffDesc.datalen = HPGP_DATA_PAYLOAD_MIN;

            FM_Printf(FM_MMSG, "SHOM:>>>CC_HANDOVER_INFO.RSP(tei: %d)\n",
                                newHpgpHdr->tei);
            break;
        }
        default:
        {
        }
    }

    EVENT_Assert(newEvent);

    //transmit CC_HANDOVER.REQ
    status = MUXL_TransmitMgmtMsg(HPGPCTRL_GetLayer(HP_LAYER_TYPE_MUX), newEvent);
    //the event is freed by MUXL if the TX is successful
    if(status == STATUS_FAILURE)
    {
        EVENT_Free(newEvent);
    }
    
    return status;
}


void SHOM_ProcEvent(sShom *shom, sEvent *event)
{
    sCcHoStaInfo      *ccHoStaInfo = NULL;
    u8                *dataptr = NULL;
    u8                 numSta = 0; 
    u8                 rsc = 0xFF; 
    u8                 backupCco = 0x0; 
    sCrm              *crm = NULL;
    sCcHoReq          *ccHoReq = NULL;
    sScb              *scb = NULL;
    sScb              *nextscb = NULL;
    sLinkLayer        *linkl = NULL;
    sStaInfo          *staInfo =  NULL;

    linkl = shom->linkl;
    staInfo = LINKL_GetStaInfo(linkl);
    crm  = LINKL_GetCrm(linkl);

    if(shom->state != SHOM_STATE_IDLE)
    {
        return;
    }

    if( event->eventHdr.eventClass == EVENT_CLASS_MSG)
    {
        switch(event->eventHdr.type)
        {
            case EVENT_TYPE_CC_HANDOVER_REQ:
            {
                FM_Printf(FM_MMSG, "SHOM:<<<CC_HANDOVER.REQ\n");
                //process events
                ccHoReq = (sCcHoReq *)(event->buffDesc.dataptr);
                if( ccHoReq->hoType == HPGP_HO_TYPE_SOFT)
                {
                    shom->hoResult = 0x01;   //reject soft handover
                }
                else //hard handover
                {
                    //TODO: check my capability
                    shom->hoResult = 0x00;   //accept
                    if(ccHoReq->reason == HPGP_HO_REASON_CCO_APPT)
                    {
                        //I shall become the user-appointed CCO
                        staInfo->staStatus.fields.apptCcoStatus = 1; 
                    }
                }
/*
                shom->hoTimer = STM_AllocTimer(LINKL_HoTimerHandler, linkLayer);
                if(shom->hoTimer == STM_TIMER_ID_NULL)
                {
                    staInfo->staStatus.fields.apptCcoStatus = 0; 
                    shom->hoResult = 0x02;   //reject soft/hard handover
                }
*/

                SHOM_SendMgmtMsg(shom, EVENT_TYPE_CC_HANDOVER_CNF);

                //if whether or not transmission succeeds,  
                //always do the following
                if (shom->hoResult == 0x00)
                {   //accept
                    shom->txRetryCnt = 0;
                //stay in the idle state as the CCO may not
                //send handover info indication
                //STM_StartTimer(shom->hoTimer, HPGP_TIME_HO);
                //shom->state = SHOM_STATE_WAITFOR_HO_INFO_IND;
                }
                break;
            }
            case EVENT_TYPE_CC_HANDOVER_INFO_IND:
            {
                FM_Printf(FM_MMSG, "SHOM:<<<CC_HANDOVER_INFO.IND\n");
                //In case of CC Handover Info Ind retransmission
                //we do not process it, but send a response
                if(!shom->hoReady) 
                {
                    shom->hoReady = 1;
                    dataptr = event->buffDesc.dataptr;
                    rsc = *dataptr;
                    dataptr++;
                    backupCco = *dataptr;
                    dataptr++;
                    numSta = *dataptr;
                    dataptr++;
                    if(numSta > CRM_SCB_MAX)
                    {
                        break;
                    }
                    
                    //free all SCBs in the TEI MAP, except for the sta and CCo
                    scb = NULL;
                    scb = CRM_GetNextScb(crm, scb);
                    while(scb)
                    {
                        nextscb = CRM_GetNextScb(crm, scb);
                        if( (scb->tei != staInfo->staScb->tei)&&
                            (scb->tei != staInfo->ccoScb->tei) )
                        {
                            CRM_FreeScb(crm, scb);
                        }
                        scb = nextscb;
                    }

                    while(numSta)
                    {
                        ccHoStaInfo = (sCcHoStaInfo *)dataptr;   
                        scb = CRM_AddScb(crm, ccHoStaInfo->tei); 
                        if( scb && (scb->tei != staInfo->staScb->tei))
                        {
                            //allocate a tei lease timer for the STA
#ifdef CALLBACK
                            scb->teiTimer = STM_AllocTimer(LINKL_TimerHandler,
                                                EVENT_TYPE_TIMER_TEI_IND, scb);
#else
                            scb->teiTimer = STM_AllocTimer(HP_LAYER_TYPE_LINK, 
                                                EVENT_TYPE_TIMER_TEI_IND, scb);
#endif
                            if(scb->teiTimer != STM_TIMER_INVALID_ID)
                            {
                                memcpy(scb->macAddr, ccHoStaInfo->macAddr, 
                                           MAC_ADDR_LEN);
                                scb->staStatus.fields.authStatus = 
                                               ccHoStaInfo->status; 
                                scb->ptei = ccHoStaInfo->ptei; 

                                scb->namState = STA_NAM_STATE_CONN;
                                scb->homState = STA_HOM_STATE_IDLE;
                            }
                            else
                            {
                                FM_Printf(FM_ERROR, "SHOM:can't alloc a TEI timer for HO\n");
                                break;
                            }
                        }
                        else if(scb == NULL)
                        {
                            FM_Printf(FM_ERROR, "SHOM:can't alloc a SCB for HO\n");
                            break;  //should not happen
                        }
                        dataptr += sizeof(sCcHoStaInfo);   
                        numSta--;
                    }
                } //end of if shom->hoReady
                //send the positive response
                SHOM_SendMgmtMsg(shom, EVENT_TYPE_CC_HANDOVER_INFO_RSP);
            }
            default:
            {
            }
        }
    }
#if 0
            else //control msg
            {
                if(event->eventHdr.type == EVENT_TYPE_TIMER_HO_IND)  //ho timer expired
                {
                    if( shom->txRetryCnt <= HPGP_TX_RETRY_MAX)
                    {
                        //resend the message
                        SHOM_SendMgmtMsg(shom, EVENT_TYPE_CC_HANDOVER_CNF);
                        STM_StartTimer(shom->hoTimer, HPGP_TIME_HO);
                        shom->txRetryCnt++;
                        //stay in the same state
                    }
                    else
                    {
                        //retry exhausted
                        staInfo->staStatus.fields.apptCcoStatus = 0; 
                        STM_FreeTimer(shom->hoTimer);
                        shom->txRetryCnt = 0; //reset
                        //back to the idle state
                        shom->state = SHOM_STATE_IDLE;
                            
                    }
                }
            }
#endif

}


void SHOM_Start(sShom *shom)
{
    shom->state = SHOM_STATE_IDLE;
    shom->hoReady = 0; 
}


void SHOM_Stop(sShom *shom)
{
    shom->state = SNAM_STATE_INIT;
    shom->hoReady = 0; 
}


eStatus SHOM_Init(sShom *shom, sLinkLayer *linkl)
{
    shom->linkl = linkl;
    shom->state = SNAM_STATE_INIT;
    shom->hoReady = 0; 


    return STATUS_SUCCESS;
}


#endif /* STA_FUNC */

/** =========================================================
 *
 * Edit History
 *
 * $Source: /home/cvsrepo/Hybrii_B_OSFW_Dev/firmware/hpgp/src/link/hom.c,v $
 *
 * $Log: hom.c,v $
 * Revision 1.10  2014/07/22 10:03:52  kiran
 * 1) SDK Supports Power Save
 * 2) Uart_Driver.c cleanup
 * 3) SDK app memory pool optimization
 * 4) Prints from STM.c are commented
 * 5) Print messages are trimmed as common no memory left in common
 * 6) Prins related to Power save and some other modules are in compilation flags. To enable module specific prints please refer respective module
 *
 * Revision 1.9  2014/07/05 09:16:27  prashant
 * 100 Devices support- only association tested, memory adjustments
 *
 * Revision 1.8  2014/06/12 13:15:43  ranjan
 * -separated bcn,mgmt,um event pools
 * -fixed datapath issue due to previous checkin
 * -work in progress. neighbour cco detection
 *
 * Revision 1.7  2014/06/11 13:17:47  kiran
 * UART as host interface and peripheral interface supported.
 *
 * Revision 1.6  2014/05/28 10:58:59  prashant
 * SDK folder structure changes, Uart changes, removed htm (UI) task
 * Varified - UM, LM - iperf (UDP/TCP), overnight test pass
 *
 * Revision 1.5  2014/05/13 20:05:46  tri
 * more PS
 *
 * Revision 1.4  2014/05/12 08:09:57  prashant
 * Route fix, STA sync issue with AV CCO and handling discover bcn issue fix
 *
 * Revision 1.3  2014/02/27 10:42:47  prashant
 * Routing code added
 *
 * Revision 1.2  2014/02/26 23:16:02  tri
 * more PS code
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
 * Revision 1.9  2013/03/22 12:21:49  prashant
 * default FM_MASK and FM_Printf modified for USER INFO
 *
 * Revision 1.8  2012/05/19 05:05:15  yuanhua
 * optimized the timer handlers in CTRL and LINK layers.
 *
 * Revision 1.7  2012/05/17 05:05:58  yuanhua
 * (1) added the option for timer w/o callback (2) added task id and name.
 *
 * Revision 1.6  2012/05/01 04:51:09  yuanhua
 * added compiler flags STA_FUNC and CCO_FUNC in link and ctrl layers.
 *
 * Revision 1.5  2012/04/30 04:05:57  yuanhua
 * (1) integrated the HAL mgmt Tx. (2) various updates
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
 * Revision 1.2  2011/06/24 14:33:18  yuanhua
 * (1) Changed event structure (2) Implemented SNSM, including the state machines in network discovery and connection states, becaon process, discover process, and handover detection (3) Integrated the HPGP and SHAL
 *
 * Revision 1.1  2011/05/28 06:31:19  kripa
 * Combining corresponding STA and CCo modules.
 *
 * Revision 1.1  2011/05/06 19:10:12  kripa
 * Adding link layer files to new source tree.
 *
 * Revision 1.2  2011/04/23 19:48:45  kripa
 * Fixing stm.h and event.h inclusion, using relative paths to avoid conflict with windows system header files.
 *
 * Revision 1.1  2011/04/08 21:42:45  yuanhua
 * Framework
 *
 *
 * =========================================================*/
