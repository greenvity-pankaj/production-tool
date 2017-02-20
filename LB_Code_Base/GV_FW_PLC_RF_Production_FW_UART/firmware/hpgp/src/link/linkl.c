/** =========================================================
 *
 *  @file linkl.c
 * 
 *  @brief Link Layer
 *
 *  Copyright (C) 2010-2011, Greenvity Communications, Inc.
 *  All Rights Reserved
 *  
 *  =======================================================*/
#ifdef RTX51_TINY_OS
#include <rtx51tny.h>
#endif
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "papdef.h"
#ifdef ROUTE
#include "hpgp_route.h"
#endif
#include "list.h"
#include "event.h"
#include "nma.h"
#include "nma_fw.h"
#include "green.h"
#include "fm.h"
#include "ism.h"
#include "linkl.h"
#include "hpgpdef.h"
#include "hpgpapi.h"
#include "muxl.h"
#include "hal.h"
#include "akm.h"
#include "timer.h"
#include "stm.h"
#include "hybrii_tasks.h"
#include "hal_common.h"
#include "green.h"
#include "crm.h"
#include "sys_common.h"
#include "event_fw.h"
#ifdef NO_HOST
#include "gv701x_flash.h"
#endif
#include "gv701x_flash_fw.h"
#include "hpgp_msgs.h"
#ifdef NO_HOST
#include "gv701x_osal.h"
#endif

#define MAXSNID                        16
#ifdef FREQ_DETECT
extern u32 PLC_DC_LINE_CYCLE_FREQENCY;
#endif 
#ifdef SIMU
void LINKL_BcnTimerHandler(u16 type, void* cookie);
#endif
extern u8 opMode;
#ifdef LOG_FLASH
extern u16 scbFreeReason;
#endif
extern sHomePlugCb HomePlug;

sysProfile_t gSysProfile;

#ifdef ROUTE
extern void ROUTE_initLrtEntry(sScb *scb);
#endif
extern void SCB_GetDiscNetList(hostEventScanList* list,u8 * listCnt);
extern void Host_SendIndication(u8 eventId, u8 protocol, u8 *payload, u8 length);
extern void SCB_ClearAgeDiscLists();

#ifdef STA_FUNC

/* -----------
 *  STA Mode
 *  ---------- */
void ConfigParams()
{
	sLinkLayer *linkLayer =(sLinkLayer*)&HomePlug.hpgpCtrl.linkLayer;
	
		
	sStaInfo *staInfo = &linkLayer->staInfo;

	
	
	memset(staInfo, 0, sizeof(sStaInfo));

	memcpy(gHpgpHalCB.nid, gSysProfile.nid, NID_LEN);
	memcpy(staInfo->nid, gSysProfile.nid, NID_LEN);
	memcpy(staInfo->nmk, gSysProfile.nmk, ENC_KEY_LEN);
//	memcpy(staInfo->hfid, gSysProfile.systemName, MAX_SYSTEM_NAME);

	staInfo->staCap.byte = 0;
	staInfo->staCap.fields.ccoCap = gSysProfile.cap.fields.ccoCap;
	staInfo->staCap.fields.backupCcoCap = gSysProfile.cap.fields.backupCcoCap;
	staInfo->staCap.fields.proxyNetCap = gSysProfile.cap.fields.proxyNetCap;

	staInfo->secLevel = gSysProfile.secLevel;

	memcpy(HAL_GetMacAddr(HOMEPLUG_GetHal()),
			gSysProfile.macAddress, MAC_ADDR_LEN);

	staInfo->identifyCaps.efl = 0;
	staInfo->identifyCaps.greenPHYCap = gSysProfile.cap.fields.greenPhyCap;
	staInfo->identifyCaps.HPAVVer = gSysProfile.cap.fields.HPAVVersion;
	staInfo->identifyCaps.powerSaveCap = gSysProfile.cap.fields.powerSaveCap;
	staInfo->identifyCaps.routingCap = gSysProfile.cap.fields.repeaterRouting;
	staInfo->bridgeSupported = gSysProfile.cap.fields.bridgeSupported;

    staInfo->lastUserAppCCOState = gSysProfile.lastUserAppCCOState;

//	memcpy(staInfo->devicePassword, gSysProfile.devicePassword, MAX_DPW_LEN);

	staInfo->ukeEnable = gSysProfile.ukeEnable;

	gHpgpHalCB.devMode = gSysProfile.devMode;

	gHpgpHalCB.lastdevMode = gSysProfile.lastdevMode;

	gHpgpHalCB.lineFreq = gSysProfile.lineFreq;
	gHpgpHalCB.lineMode = gSysProfile.lineMode;
	
#ifdef FREQ_DETECT
	if (gHpgpHalCB.lineFreq == FREQUENCY_50HZ)
		PLC_DC_LINE_CYCLE_FREQENCY = DC_50HZ;
	else
		PLC_DC_LINE_CYCLE_FREQENCY = DC_60HZ;
#endif
	
 
 
}



u8 LINKL_DetermineStaTrans(sEvent *event)
{
    u8 trans = LINKL_TRANS_UNKNOWN;
    if(event->eventHdr.eventClass == EVENT_CLASS_MSG)
    {
        switch(event->eventHdr.type)
        {
            case EVENT_TYPE_CM_UNASSOC_STA_IND:
            case EVENT_TYPE_CC_DISCOVER_LIST_REQ:
#ifdef UKE			
            case EVENT_TYPE_CM_SC_JOIN_REQ:                
            case EVENT_TYPE_CM_SC_JOIN_CNF:
#endif		
            
#ifdef ROUTE
            case EVENT_TYPE_CM_ROUTE_INFO_REQ:
            case EVENT_TYPE_CM_ROUTE_INFO_CNF:
            case EVENT_TYPE_CM_ROUTE_INFO_IND:
            case EVENT_TYPE_CM_UNREACHABLE_IND:
#endif
            {

                trans = LINKL_TRANS_SNSM;
                break;
            }
#ifdef STA_ID


            case EVENT_TYPE_CM_STA_IDENTIFY_REQ:
            case EVENT_TYPE_CM_STA_IDENTIFY_CNF:
            case EVENT_TYPE_CM_STA_IDENTIFY_RSP:
#endif				
            case EVENT_TYPE_CC_ASSOC_CNF:
            case EVENT_TYPE_CC_LEAVE_CNF:
            case EVENT_TYPE_CC_LEAVE_IND:
            case EVENT_TYPE_CC_SET_TEI_MAP_IND:
#ifdef APPOINT
            case EVENT_TYPE_CC_CCO_APPOINT_CNF:	
#endif
			case EVENT_TYPE_CC_BACKUP_APPOINT_REQ:
            {

                trans = LINKL_TRANS_SNAM;
                break;
            }
            case EVENT_TYPE_CM_ENCRY_PAYLOAD_IND:                
            case EVENT_TYPE_CM_ENCRY_PAYLOAD_RSP:
            case EVENT_TYPE_CM_GET_KEY_CNF:
#ifdef UKE			
            case EVENT_TYPE_CM_GET_KEY_REQ:
#endif			
            {
                trans = LINKL_TRANS_AKM;
                break;
            }
#ifdef HOM
            case EVENT_TYPE_CC_HANDOVER_REQ:
            case EVENT_TYPE_CC_HANDOVER_INFO_IND:
            {

                trans = LINKL_TRANS_SHOM;
                break;
            }
#endif
#ifdef POWERSAVE
			case EVENT_TYPE_CC_PWR_SAVE_CNF:
			case EVENT_TYPE_CC_PWR_SAVE_EXIT_CNF:
			case EVENT_TYPE_CC_STOP_PWR_SAVE_REQ:
			{

                trans = LINKL_TRANS_SPSM;
                break;
            }
#endif			
            default:
                trans = LINKL_TRANS_UNKNOWN;
        }
    }
    else  //control
    {
        switch(event->eventHdr.type)
        {
            case EVENT_TYPE_SNSM_START:
            case EVENT_TYPE_SNSM_STOP:
            case EVENT_TYPE_TIMER_BBT_IND:
            case EVENT_TYPE_TIMER_USTT_IND:
            case EVENT_TYPE_TIMER_DISC_AGING_IND:			
            case EVENT_TYPE_TIMER_BEACON_LOSS_IND:
            case EVENT_TYPE_CC_BCN_IND:
            case EVENT_TYPE_SET_SEC_MODE:  
#ifdef SW_RECOVERY							
			case EVENT_TYPE_TIMER_DISC_STALL_IND:
			case EVENT_TYPE_TIMER_BCN_STALL_IND:
#endif            
			case EVENT_TYPE_TIMER_JOIN_TIMEOUT:


#ifdef ROUTE           
            case EVENT_TYPE_ROUTE_UPDATE_TIMEOUT:
            case EVENT_TYPE_ROUTE_HD_DURATION_TIMEOUT:
#endif
			case EVENT_TYPE_BCN_MISS_IND :
            {

                trans = LINKL_TRANS_SNSM;
                break;
            }
            case EVENT_TYPE_SNAM_START:
            case EVENT_TYPE_SNAM_STOP:
            case EVENT_TYPE_NET_ACC_REQ:
            case EVENT_TYPE_NET_LEAVE_REQ:
            case EVENT_TYPE_AUTH_RSP:
            case EVENT_TYPE_TIMER_TEI_IND:
            case EVENT_TYPE_TIMER_ACC_IND:
            case EVENT_TYPE_TIMER_APPT_IND:
            case EVENT_TYPE_CCO_HO_IND:
            case EVENT_TYPE_CCO_DISC_IND:
#ifdef APPOINT
            case EVENT_TYPE_CCO_APPOINT_REQ:
#endif
#ifdef STA_ID				
            case EVENT_TYPE_IDENTIFY_CAP_TIMEOUT:
#endif				
				
            case EVENT_TYPE_AUTH_CPLT:
#ifdef KEEP_ALIVE				
            case EVENT_TYPE_TIMER_KEEP_LIVE_IND:
#endif      

            {

                trans = LINKL_TRANS_SNAM;
                break;
            }
            case EVENT_TYPE_AKM_START:
            case EVENT_TYPE_AUTH_REQ:
            case EVENT_TYPE_TIMER_KEY_IND:
#ifdef UKE			
            case EVENT_TYPE_TIMER_TEK_IND:
            case EVENT_TYPE_ASSOC_IND:
#endif			
            {
                trans = LINKL_TRANS_AKM;
                break;
            }
#ifdef HOM
            case EVENT_TYPE_CCO_HO_REQ:
            {

                trans = LINKL_TRANS_SHOM;
                break;
            }
#endif
#ifdef POWERSAVE
			case EVENT_TYPE_STA_START_PS:
			case EVENT_TYPE_STA_STOP_PS:
			case EVENT_TYPE_STA_PS_EXIT_REQ:
			case EVENT_TYPE_TIMER_ACK_IND:
            {

                trans = LINKL_TRANS_SPSM;
                break;
            }
#endif			
            default:
                trans = LINKL_TRANS_UNKNOWN;
				break;
        }
    }
    return trans;
}

void LINKL_PreProcStaEvent(sLinkLayer *linkLayer, sEvent *event)
{
    //get transaction
    sHpgpHdr *hpgpHdr = (sHpgpHdr *)event->buffDesc.buff;
    
    if(hpgpHdr->scb == NULL)
    {
        //(1) get STA control block
        if (hpgpHdr->tei != 0xFF)
        {
            hpgpHdr->scb = CRM_GetScb(&linkLayer->ccoRm, (u8)hpgpHdr->tei);
        }
    }
    
    event->eventHdr.trans = LINKL_DetermineStaTrans(event);
} 


void LINKL_ProcStaEvent(sLinkLayer *linkLayer, sEvent *event)
{
    switch(event->eventHdr.trans)
    {
        case LINKL_TRANS_SNSM:
        {
            SNSM_ProcEvent(&linkLayer->staNsm, event);
            break;
        }
        case LINKL_TRANS_SNAM:
        {
            SNAM_ProcEvent(&linkLayer->staNam, event);
            break;
        }
#ifdef AKM		
        case LINKL_TRANS_AKM:
        {
            AKM_ProcEvent(&linkLayer->akm, event);
            break;
        }
#endif		
#ifdef HOM
        case LINKL_TRANS_SHOM:
        {
            SHOM_ProcEvent(&linkLayer->staHom, event);
            break;
        }
#endif
#ifdef POWERSAVE
        case LINKL_TRANS_SPSM:
        {
            SPSM_ProcEvent(&linkLayer->staPsm, event);
            break;
        }
#endif
        default:
        {
			break;
        }
    }
}


#endif /* STA_FUNC */


#ifdef CCO_FUNC
/* -----------
 *  CCO Mode
 *  ---------- */

eStatus LINKL_BcnUpdateActive()
{
    sLinkLayer *linkl = HPGPCTRL_GetLayer(HP_LAYER_TYPE_LINK);

    return(CNSM_BcnUpdateActive(&linkl->ccoNsm));
}


void LINKL_UpdateBeacon()//sLinkLayer *linkLayer)
{
    sLinkLayer *linkl = HPGPCTRL_GetLayer(HP_LAYER_TYPE_LINK);


    if (linkl->ccoNsm.bcnUpdate)
    {
        linkl->ccoNsm.bcnUpdateProgress = 1;

        LINKL_BcnTxHandler(linkl);

        linkl->ccoNsm.bcnUpdateProgress = 0;

    }    

}

u8 LINKL_DetermineCcoTrans(sEvent *event)
{
    u8 trans = LINKL_TRANS_UNKNOWN;
    if(event->eventHdr.eventClass == EVENT_CLASS_MSG)
    {
        switch(event->eventHdr.type)
        {
        	case EVENT_TYPE_NN_INL_REQ:
			case EVENT_TYPE_NN_INL_CNF:							
            case EVENT_TYPE_CC_DISCOVER_LIST_CNF:
            case EVENT_TYPE_CC_DISCOVER_LIST_IND:
#ifdef UKE			
            case EVENT_TYPE_CM_SC_JOIN_REQ:
#endif		
            
#ifdef ROUTE
            case EVENT_TYPE_CM_ROUTE_INFO_REQ:
            case EVENT_TYPE_CM_ROUTE_INFO_CNF:
            case EVENT_TYPE_CM_ROUTE_INFO_IND:
            case EVENT_TYPE_CM_UNREACHABLE_IND:
#endif
            case EVENT_TYPE_CM_UNASSOC_STA_IND:
            {

                trans = LINKL_TRANS_CNSM;
                break;
            }
#ifdef STA_ID			
            case EVENT_TYPE_CM_STA_IDENTIFY_REQ:
            case EVENT_TYPE_CM_STA_IDENTIFY_CNF:
            case EVENT_TYPE_CM_STA_IDENTIFY_IND:
#endif				
				
            case EVENT_TYPE_CC_ASSOC_REQ:
#ifdef APPOINT
            case EVENT_TYPE_CC_CCO_APPOINT_REQ:
#endif
            case EVENT_TYPE_CC_LEAVE_REQ:
			case EVENT_TYPE_CC_BACKUP_APPOINT_CNF:
#ifdef KEEP_ALIVE				
			case EVENT_TYPE_VENDOR_KEEP_ALIVE_IND:
#endif

            {

                trans = LINKL_TRANS_CNAM;
                break;
            }
            case EVENT_TYPE_CM_ENCRY_PAYLOAD_IND:
            case EVENT_TYPE_CM_ENCRY_PAYLOAD_RSP:
            case EVENT_TYPE_CM_GET_KEY_REQ:
#ifdef UKE			
            case EVENT_TYPE_CM_GET_KEY_CNF:
#endif			
            {

                trans = LINKL_TRANS_AKM;
                break;
            }
#ifdef HOM
            case EVENT_TYPE_CC_HANDOVER_CNF:
            case EVENT_TYPE_CC_HANDOVER_INFO_RSP:
            {

                trans = LINKL_TRANS_CHOM;
                break;
            }
#endif            
#ifdef POWERSAVE
			case EVENT_TYPE_CC_PWR_SAVE_REQ:
			case EVENT_TYPE_CC_PWR_SAVE_EXIT_REQ:
			case EVENT_TYPE_CC_STOP_PWR_SAVE_CNF:
            {

                trans = LINKL_TRANS_CPSM;
                break;
            }
#endif			
            default:
                trans = LINKL_TRANS_UNKNOWN;
        }
    }
    else  //control messages
    {
        switch(event->eventHdr.type)
        {
            case EVENT_TYPE_CNSM_START:
            case EVENT_TYPE_TIMER_DISC_IND:
            case EVENT_TYPE_TIMER_DISC_AGING_IND:
            case EVENT_TYPE_TIMER_BCN_TX_IND:
            case EVENT_TYPE_CC_BCN_IND:            
            case EVENT_TYPE_CNSM_STOP_REQ:
                
#ifdef ROUTE
            case EVENT_TYPE_ROUTE_UPDATE_TIMEOUT:
            case EVENT_TYPE_ROUTE_HD_DURATION_TIMEOUT:
#endif
#ifdef MCCO
			case EVENT_TYPE_TIMER_CCO_BBT_IND:				
			case EVENT_TYPE_TIMER_BEACON_LOSS_IND:
#endif				
#if 1 //def AC_TO_DC
            case EVENT_TYPE_TIMER_AC_DC_IND:
#endif
				
            {

                trans = LINKL_TRANS_CNSM;
                break;
            }
            case EVENT_TYPE_TIMER_TEI_IND:
            case EVENT_TYPE_CCO_SELECT_IND:
#ifdef HOM
            case EVENT_TYPE_CCO_HO_RSP:
#endif
            case EVENT_TYPE_AUTH_IND:
			case EVENT_TYPE_STA_AGEOUT:
			case EVENT_TYPE_CNAM_STOP_REQ:
#ifdef KEEP_ALIVE							
			case EVENT_TYPE_TIMER_KEEP_LIVE_IND:
#endif				
//            case EVENT_TYPE_TIMER_TEI_REUSE_IND:
            {

                trans = LINKL_TRANS_CNAM;
                break;
            }
            case EVENT_TYPE_AKM_START:
            case EVENT_TYPE_TIMER_KEY_IND:
            case EVENT_TYPE_AUTH_REQ:            
#ifdef UKE			
            case EVENT_TYPE_TIMER_TEK_IND:
            case EVENT_TYPE_ASSOC_IND:
#endif			
            {
                trans = LINKL_TRANS_AKM;
                break;
            }
#ifdef HOM
            case EVENT_TYPE_CCO_HO_REQ:
            case EVENT_TYPE_CCO_HO_IND:
            case EVENT_TYPE_TIMER_HO_IND:
            {

                trans = LINKL_TRANS_CHOM;
                break;
            }
#endif
#ifdef POWERSAVE
			case EVENT_TYPE_CCO_SND_STOP_PS_REQ:
            {
                trans = LINKL_TRANS_CPSM;
                break;
            }
#endif			
            default:
                trans = LINKL_TRANS_UNKNOWN;
        }
    }

    return trans;
}


void LINKL_PreProcCcoEvent(sLinkLayer *linkLayer, sEvent *event)
{
    sHpgpHdr *hpgpHdr = (sHpgpHdr *)event->buffDesc.buff;
    if(hpgpHdr->scb == NULL)
    {
        //(1) get STA control block
        if (hpgpHdr->tei != 0xFF)
        {
            hpgpHdr->scb = CRM_GetScb(&linkLayer->ccoRm, (u8)hpgpHdr->tei);
        }
    }
    //(2) determine transaction
    event->eventHdr.trans = LINKL_DetermineCcoTrans(event);
} 



    
void LINKL_ProcCcoEvent(sLinkLayer *linkLayer, sEvent *event)
{
    switch(event->eventHdr.trans)
    {
        case LINKL_TRANS_CNSM:
        {
            CNSM_ProcEvent(&linkLayer->ccoNsm, event);
            break;
        }
        case LINKL_TRANS_CNAM:
        {
            CNAM_ProcEvent(&linkLayer->ccoNam, event);
            break;
        }
#ifdef AKM
        case LINKL_TRANS_AKM:
        {
            AKM_ProcEvent(&linkLayer->akm, event);
            break;
        }
#endif		
#ifdef HOM
        case LINKL_TRANS_CHOM:
        {
            CHOM_ProcEvent(&linkLayer->ccoHom, event);
            break;
        }
#endif
#ifdef POWERSAVE
        case LINKL_TRANS_CPSM:
        {
            CPSM_ProcEvent(&linkLayer->ccoPsm, event);
            break;
        }
#endif
        default:
        {
        }
    }

}

#endif /* CCO_FUNC */




u8 LINKL_Proc(void *cookie)
{
    sEvent *event = NULL;
    sSlink *slink = NULL;
    u8      ret = 0;
    sLinkLayer *linkLayer = (sLinkLayer *) cookie;
    while(!SLIST_IsEmpty(&linkLayer->eventQueue) 
#ifndef RTX51_TINY_OS
		 &&!(ret = SCHED_IsPreempted(&linkLayer->task))
#endif
		)
    {
#ifdef P8051
__CRIT_SECTION_BEGIN__
#else
        SEM_WAIT(&linkLayer->linkSem);
#endif
        slink = SLIST_Pop(&linkLayer->eventQueue);
#ifdef P8051
__CRIT_SECTION_END__
#else
        SEM_POST(&linkLayer->linkSem);
#endif

		if (slink == NULL)
			return ret;
		
        event = SLIST_GetEntry(slink, sEvent, link);


		///FM_HexDump(FM_CTRL|FM_MINFO, "LINKL: \n", (u8*)&event->eventHdr.type,2);
		
        if(linkLayer->mode == LINKL_STA_MODE_CCO)
        {
#ifdef CCO_FUNC
            //CCO mode
            LINKL_PreProcCcoEvent(linkLayer, event);
            LINKL_ProcCcoEvent(linkLayer, event);
#endif 
        }
        else 
        {
#ifdef STA_FUNC
            //default STA mode
            LINKL_PreProcStaEvent(linkLayer, event);
            LINKL_ProcStaEvent(linkLayer, event);
#endif 
        }
		event->eventHdr.status = EVENT_STATUS_COMPLETE;
     
        if(event->eventHdr.status == EVENT_STATUS_COMPLETE)
        {
            EVENT_Free(event);
        }

        while(!SLIST_IsEmpty(&linkLayer->intEventQueue))
        {
            //no need for sync protection
            slink = SLIST_Pop(&linkLayer->intEventQueue);
            event = SLIST_GetEntry(slink, sEvent, link);

            if(linkLayer->mode == LINKL_STA_MODE_CCO)
            {
#ifdef CCO_FUNC
                //CCO mode
                LINKL_PreProcCcoEvent(linkLayer, event);
                LINKL_ProcCcoEvent(linkLayer, event);
#endif
            }
            else 
            {
#ifdef STA_FUNC
                //default STA mode
                LINKL_PreProcStaEvent(linkLayer, event);
                LINKL_ProcStaEvent(linkLayer, event);
#endif
            }
     
            if(event->eventHdr.status == EVENT_STATUS_COMPLETE)
            {
                EVENT_Free(event);
            }
        }
    }
    return ret;
}

#ifdef UKE
eStatus LINKL_SendMgmtMsg(sStaInfo *staInfo, u16 mmType, u8 *macAddr)
{
    eStatus          status = STATUS_FAILURE;
    sEvent      xdata    *newEvent;
    sHpgpHdr        *hpgpHdr = NULL;
    sCmJoinReq      *cmJoinReq = NULL;
    sCmJoinCnf      *cmJoinCnf = NULL;
    u8               i = 0;
    u16              eventSize = 0;
    
    switch(mmType)
    {
        
        case EVENT_TYPE_CM_SC_JOIN_REQ:
        {
#ifndef RELEASE
            FM_Printf(FM_MMSG, ">>>CM_SC_JOIN_REQ\n");
#endif
            eventSize = MAX(sizeof(sCmJoinReq), HPGP_DATA_PAYLOAD_MIN); 
            newEvent = EVENT_MgmtAlloc(eventSize, EVENT_HPGP_MSG_HEADROOM);
            if(newEvent == NULL)
            {
                FM_Printf(FM_ERROR, "EAF\n");
                return STATUS_FAILURE;
            }
            newEvent->eventHdr.eventClass = EVENT_CLASS_MSG;
            newEvent->eventHdr.type = EVENT_TYPE_CM_SC_JOIN_REQ;
       
       
            
            LINKL_FillHpgpHdr((sHpgpHdr *)newEvent->buffDesc.buff,
                            0xFF,
                            bcAddr,
                            staInfo->snid,
                            1,
                            HPGP_EKS_NONE);
            

            cmJoinReq = (sCmJoinReq *)newEvent->buffDesc.dataptr; 
            cmJoinReq->ccoCapability = staInfo->staCap.fields.ccoCap;
            newEvent->buffDesc.datalen += eventSize;
            
            break;
        }
        case EVENT_TYPE_CM_SC_JOIN_CNF:
        {	             
            FM_Printf(FM_MMSG, "SNSM:>>>CM_SC_JOIN.CNF\n");
            eventSize = MAX(sizeof(sCmJoinCnf), HPGP_DATA_PAYLOAD_MIN); 
            newEvent = EVENT_MgmtAlloc(eventSize, EVENT_HPGP_MSG_HEADROOM);
            if(newEvent == NULL)
            {
                FM_Printf(FM_ERROR, "EAF\n");
                return STATUS_FAILURE;
            }
            newEvent->eventHdr.eventClass = EVENT_CLASS_MSG;
            newEvent->eventHdr.type = EVENT_TYPE_CM_SC_JOIN_CNF;

            
            LINKL_FillHpgpHdr((sHpgpHdr *)newEvent->buffDesc.buff,
                            0xFF,
                            macAddr,
                            staInfo->snid,
                            1,
                            HPGP_EKS_NONE);
                        
            cmJoinCnf = (sCmJoinCnf *)newEvent->buffDesc.dataptr; 
            memcpy(cmJoinCnf->nid, staInfo->nid, NID_LEN);
          
            cmJoinCnf->staCap = staInfo->staCap;
            newEvent->buffDesc.datalen += eventSize;
            
            break;
        }
        default:
        {
            return status;
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

#endif

void LINKL_RecvMgmtMsg(void* cookie,  sEvent *event)
{
    sLinkLayer *linkl = (sLinkLayer *)cookie;

#ifdef P8051
__CRIT_SECTION_BEGIN__
#else
    SEM_WAIT(&linkl->linkSem);
#endif

    //place the event to the queue
    SLIST_Put(&linkl->eventQueue, &event->link);

#ifdef P8051
__CRIT_SECTION_END__
#else
    SEM_POST(&linkl->linkSem);
#endif
    /* schedule the task */
#ifndef RTX51_TINY_OS
    SCHED_Sched(&linkl->task);
#else
  //  os_set_ready(HPGP_TASK_ID_LINK);

	os_set_ready(HPGP_TASK_ID_CTRL);

#endif
}



void LINKL_RegisterEventCallback(sLinkLayer *linkl, 
    void (*callback)(void XDATA *cookie, sEvent XDATA *event),
    void *cookie)
{
		/*Compiler warning suppression*/
		callback = callback;
#ifdef CALLBACK
    linkl->deliverEvent = callback;
#endif
    linkl->eventcookie = cookie;
}

u8 LINKL_GetMode(sLinkLayer *linkLayer)
{
    return linkLayer->mode;
}

#if 0
sStaInfo* LINKL_GetStaInfo(sLinkLayer *linkLayer)
{
    return &linkLayer->staInfo;
}

sCrm* LINKL_GetCrm(sLinkLayer *linkLayer)
{
    return &linkLayer->ccoRm;
}

sCnsm* LINKL_GetCnsm(sLinkLayer *linkLayer)
{
    return &linkLayer->ccoNsm;
}

sSnsm* LINKL_GetSnsm(sLinkLayer *linkLayer)
{
    return &linkLayer->staNsm;
}

sCnam* LINKL_GetCnam(sLinkLayer *linkLayer)
{
    return &linkLayer->ccoNam;
}

sSnam* LINKL_GetSnam(sLinkLayer *linkLayer)
{
    return &linkLayer->staNam;
}

#endif


void LINKL_SetCCoCap(sLinkLayer *linkLayer, u8 ccoCap)
{
    linkLayer->staInfo.staCap.fields.ccoCap = ccoCap;
}





#if 1
//STA general configuration
void LINKL_InitStaInfo(sLinkLayer *linkLayer)
{
   sStaInfo *staInfo = &linkLayer->staInfo;
   
   staInfo->hm = HYBRID_MODE_SHARED_CSMA;//Shared CSMA hybrid mode   

   staInfo->nekEks = HPGP_EKS_NONE;
//   staInfo->ppekEks = HPGP_EKS_NONE;

   staInfo->secMode = SEC_MODE_SC;

   /* point to the memory in the HAL */
   staInfo->macAddr = HAL_GetMacAddr(HOMEPLUG_GetHal());

   staInfo->staScb = NULL;
   staInfo->ccoScb = NULL;
#if 0
	staInfo->staCap.byte = 0;
	   staInfo->staCap.fields.ccoCap = CCO_CAP_LEVEL0;
	   staInfo->staCap.fields.backupCcoCap = 1;
	   
	   staInfo->staStatus.byte = 0;
	   staInfo->staStatus.fields.greenPhyStatus = 1;	 //HPGP 1.0 capable

	staInfo->identifyCaps.efl = 0;
	staInfo->identifyCaps.greenPHYCap = 1;
	staInfo->identifyCaps.HPAVVer = 0;
	staInfo->identifyCaps.powerSaveCap = 0;
	staInfo->identifyCaps.routingCap = TRUE;

	#endif
}
#else
void LINKL_InitStaInfo(sLinkLayer *linkLayer)
{
    sStaInfo *staInfo = &linkLayer->staInfo;

    memset(staInfo, 0, sizeof(sStaInfo));

    //set a default NID, which should be set by the user later
    memcpy(staInfo->nid, gSysProfile.defaultNID, NID_LEN);
    memcpy(staInfo->nmk, gSysProfile.defaultNMK, ENC_KEY_LEN);
    memcpy(staInfo->hfid, gSysProfile.systemName, MAX_SYSTEM_NAME);
    staInfo->hm = HYBRID_MODE_SHARED_CSMA;//Shared CSMA hybrid mode
 
    staInfo->staCap.byte = 0;
    staInfo->staCap.fields.ccoCap = gSysProfile.cap.fields.ccoCap;
    staInfo->staCap.fields.backupCcoCap = gSysProfile.cap.fields.backupCcoCap;
    staInfo->staCap.fields.proxyNetCap = gSysProfile.cap.fields.proxyNetCap;
    
    staInfo->staStatus.byte = 0;
 //   staInfo->staStatus.fields.greenPhyStatus = 1;     //HPGP 1.0 capable
 
    staInfo->nekEks = HPGP_EKS_NONE;
    staInfo->ppekEks = HPGP_EKS_NONE;
 
    staInfo->secMode = SEC_MODE_SC;
    
    staInfo->secLevel = gSysProfile.secLevel;
    /* point to the memory in the HAL */
    staInfo->macAddr = HAL_GetMacAddr(HOMEPLUG_GetHal());
 
    staInfo->staScb = NULL;
    staInfo->ccoScb = NULL;
 

    staInfo->identifyCaps.efl = 0;
    staInfo->identifyCaps.greenPHYCap = gSysProfile.cap.fields.greenPhyCap;
    staInfo->identifyCaps.HPAVVer = gSysProfile.cap.fields.HPAVVersion;
    staInfo->identifyCaps.powerSaveCap = gSysProfile.cap.fields.powerSaveCap;
    staInfo->identifyCaps.routingCap = gSysProfile.cap.fields.repeaterRouting;
    staInfo->bridgeSupported = gSysProfile.cap.fields.bridgeSupported;
}
#endif
void LINKL_CommitStaProfile(sLinkLayer *linkLayer)
{
    sStaInfo *staInfo = &linkLayer->staInfo;

	
    memcpy(gSysProfile.nid, staInfo->nid, NID_LEN);
    memcpy(gSysProfile.nmk, staInfo->nmk, ENC_KEY_LEN);
    memcpy(gSysProfile.systemName, staInfo->hfid, MAX_SYSTEM_NAME);
 
    gSysProfile.cap.fields.ccoCap = staInfo->staCap.fields.ccoCap;
    gSysProfile.cap.fields.backupCcoCap = staInfo->staCap.fields.backupCcoCap;
    gSysProfile.cap.fields.proxyNetCap = staInfo->staCap.fields.proxyNetCap;
        
    gSysProfile.secLevel = staInfo->secLevel;
    memcpy(gSysProfile.macAddress, linkLayer->hal->macAddr, MAC_ADDR_LEN);
   
    gSysProfile.cap.fields.greenPhyCap = staInfo->identifyCaps.greenPHYCap;
    gSysProfile.cap.fields.HPAVVersion = staInfo->identifyCaps.HPAVVer;
    gSysProfile.cap.fields.powerSaveCap = staInfo->identifyCaps.powerSaveCap;
    gSysProfile.cap.fields.repeaterRouting = staInfo->identifyCaps.routingCap;
    gSysProfile.cap.fields.bridgeSupported = staInfo->bridgeSupported;
    gSysProfile.lastUserAppCCOState = staInfo->lastUserAppCCOState;
    
	gSysProfile.devMode =  gHpgpHalCB.devMode;
	gSysProfile.lastdevMode = gHpgpHalCB.devMode;

    gSysProfile.lineFreq = gHpgpHalCB.lineFreq;
	gSysProfile.lineMode =	gHpgpHalCB.lineMode;

	gSysProfile.ukeEnable = staInfo->ukeEnable;
	

    /******************************************************/
#ifdef B_ASICPLC
	flashWrite_config((u8 *)&gSysProfile, 
					  FLASH_SYS_CONFIG_OFFSET, sizeof(gSysProfile));
#endif
    /*********************************************************/
}

eStatus LINKL_SetLineMode(sLinkLayer *linkLayer, eLineMode lineMd)
{
   //sHaLayer *hal = HOMEPLUG_GetHal();// Kiran Optimization
   sStaInfo *staInfo = &linkLayer->staInfo;
   gHpgpHalCB.lineMode = lineMd;

   HHAL_SetDevMode(DEV_MODE_STA, lineMd);
   return STATUS_SUCCESS;
}
eStatus LINKL_SetKey(sLinkLayer *linkLayer, u8 *nmk, u8 *nid)
{
    sStaInfo *staInfo = &linkLayer->staInfo;
    u8 secLevel;

    memcpy(staInfo->nmk, nmk, ENC_KEY_LEN);
    memcpy(staInfo->nid, nid, NID_LEN);
    memcpy(gHpgpHalCB.nid, nid, NID_LEN);
    staInfo->nid[NID_LEN-1] &= NID_EXTRA_BIT_MASK;

    secLevel =  ( staInfo->nid[NID_LEN-1] & 0x30 );

    if (secLevel == SECLV_SC)
    {
        LINKL_SetSecurityMode(linkLayer, SEC_MODE_SC );
    }
    else
    {
        LINKL_SetSecurityMode(linkLayer, SEC_MODE_HS);
    }
#ifndef RELEASE
    FM_Printf(FM_LINK, "LINK:NID:%2x %2x %2x %2x %2x %2x %2x\n", 
                  nid[0], nid[1], nid[2], nid[3], nid[4], nid[5], nid[6]);
    FM_HexDump(FM_LINK, "LINK:NMK", nmk, ENC_KEY_LEN);
#endif
    return STATUS_SUCCESS;
}



eStatus LINKL_GetKey(sLinkLayer *linkLayer, u8 *nmk, u8 *nid)
{
   sStaInfo *staInfo = &linkLayer->staInfo;

   memcpy(nmk, staInfo->nmk, ENC_KEY_LEN);
   memcpy(nid, staInfo->nid, NID_LEN);
   return STATUS_SUCCESS;
}


eStatus LINKL_SetSecurityMode(sLinkLayer *linkLayer, u8 secMode)
{
    sStaInfo *staInfo = &linkLayer->staInfo;
    u8 secLevel = staInfo->nid[NID_LEN-1] & 0x30;
    
    //   sCtrlLayer *ctrll = HPGPCTRL_GetLayer(HP_LAYER_TYPE_CTRL);

    // if current sec mode  is HS then do not change sec mode
    

#if 0
    if(staInfo->secMode == SEC_MODE_HS)
    {
        return STATUS_FAILURE;
    }


    if(staInfo->secMode == secMode)
    {
        return STATUS_SUCCESS;
    }

    if (secMode == SEC_MODE_HS)
    {
        staInfo->nid[NID_LEN-1] &= SECLV_MASK;   
        staInfo->nid[NID_LEN-1] |= (SECLV_HS << SECLV_OFFSET); // 2013 : Security level fix by default SC

    }
    else if (secMode == SEC_MODE_SC || secMode == SEC_MODE_SC_JOIN || secMode == SEC_MODE_SC_ADD)
    {
        staInfo->nid[NID_LEN-1] &= SECLV_MASK;
        staInfo->nid[NID_LEN-1] |= (SECLV_SC << SECLV_OFFSET);
    } 
    else
    {
        return STATUS_FAILURE;
    }   

#else
    if (secMode == SEC_MODE_SC || secMode == SEC_MODE_SC_JOIN || secMode == SEC_MODE_SC_ADD)
    {
        if (secLevel != SECLV_SC)
        {
#ifndef RELEASE
            FM_Printf(FM_ERROR, "SC Mismatch\n");
#endif
            return STATUS_FAILURE;
        }
    }
    else
    if (secMode == SEC_MODE_HS)
    {
        if (!(secLevel & SECLV_HS))
        {
#ifndef RELEASE
            //FM_Printf(FM_ERROR, "HS Mismatch\n");
#endif
            return STATUS_FAILURE;
        }
    }



#endif

    staInfo->secMode = secMode;
    return STATUS_SUCCESS;
}


eStatus LINKL_GetSecurityMode(sLinkLayer *linkLayer, u8 *secMode)
{
   sStaInfo *staInfo = &linkLayer->staInfo;
   *secMode = staInfo->secMode;
   return STATUS_SUCCESS;
}

#ifdef APPOINT
eStatus LINKL_StartAuth(sLinkLayer *linkLayer, u8 *nmk, u8 *dak, u8* macAddr, u8 sl)
{
   /* set them in the AKM */

   return STATUS_SUCCESS;
}


eStatus LINKL_ApptCCo(sLinkLayer *linkLayer, u8 *macAddr, u8 reqType)
{
    sCcoApptReqEvent  *ccoApptReq = NULL;
    sEvent           xdata *event = NULL;

    event = EVENT_Alloc(sizeof(sCcoApptReqEvent), 0);
    if(event == NULL)
    {
        FM_Printf(FM_ERROR, "EAF\n");
        return STATUS_FAILURE;
    }

    event->eventHdr.eventClass = EVENT_CLASS_CTRL;
    event->eventHdr.type = EVENT_TYPE_CCO_APPOINT_REQ;

    ccoApptReq = (sCcoApptReqEvent *)event->buffDesc.dataptr;
    ccoApptReq->reqType = reqType;
    memcpy(ccoApptReq->macAddr, macAddr, MAC_ADDR_LEN);
    event->buffDesc.datalen = sizeof(sCcoApptReqEvent);
    /* place the event into the external queue */
    return LINKL_SendEvent(linkLayer, event);
}


eStatus LINKL_SetPpKeys(sLinkLayer *linkLayer, u8 *ppEks, u8 *ppek, u8* macAddr)
{

    return STATUS_SUCCESS;
}

#endif

//CeO general confinuration
/*
void LINKL_InitCcoInfo(sLinkLayer *linkLayer)
{
   sCcoInfo *ccoInfo = &linkLayer->ccoInfo;
   ccoInfo->ccoScb = NULL;
}
*/

#ifdef STA_FUNC
/* ---------------
 *  STA Mode
 *  -------------- */

void LINKL_SendBcnLossInd(u8 type)
{

	sLinkLayer	 *linkl  = HPGPCTRL_GetLayer(HP_LAYER_TYPE_LINK);
	sEvent xdata *event;

#ifdef POWERSAVE
    sStaInfo      *staInfo = LINKL_GetStaInfo(linkl);
	sScb *scb;

    if (linkl->mode == LINKL_STA_MODE_CCO)
	{
		// this station is CCO
		scb = staInfo->ccoScb;
	}
	else
	{
		// this station is STA
		scb = staInfo->staScb;
	}
	if (scb->psState == PSM_PS_STATE_ON)
	{
		// ?? must find a time limit for this case
//		printf("LINKL_SendBcnLossInd: PS mode is ON, pss = 0x%bx\n", scb->pss);
		return;
	}
#endif
	//send event CCO_SEL_IND to the ctrl		
	event = EVENT_Alloc(1, EVENT_HPGP_CTRL_HEADROOM);
	
	if(event == NULL)
	{
		FM_Printf(FM_ERROR, "EAF\n");
        return;
	}
	
	
	event->eventHdr.eventClass = EVENT_CLASS_CTRL;
	event->eventHdr.type = EVENT_TYPE_BCN_MISS_IND;
	
	
	*event->buffDesc.dataptr = type;

#ifdef P8051
	__CRIT_SECTION_BEGIN__
#else
		SEM_WAIT(&linkl->linkSem);
#endif
	
		//place the event to the queue
		SLIST_Put(&linkl->intEventQueue, &event->link);
	
#ifdef P8051
	__CRIT_SECTION_END__
#else
		SEM_POST(&linkl->linkSem);
#endif
	  //  os_set_ready(HPGP_TASK_ID_LINK);
	
		os_set_ready(HPGP_TASK_ID_CTRL);
	
}
	
void LINKL_SetStaMode(sLinkLayer *linkLayer)
{
    linkLayer->mode = LINKL_STA_MODE_STA;
    /* call the HAL to set the STA mode in the LMAC */
#ifdef HPGP_HAL
    ISM_DisableMacIrq(MAC_INT_IRQ_PLC_BCN_TX);
    ISM_EnableMacIrq(MAC_INT_IRQ_PLC_BCN_RX);
	// FIXME : This call is iterfering with ongoing bcn sync. as this routine gets called repeatedly.
	//         Hence need to comment this out to get Bcn sync to work.
    //HHAL_SetDevMode(DEV_MODE_STA, LINE_MODE_DC);
#else
    HAL_SetDevMode(linkLayer->hal, DEV_MODE_STA);
#endif

#ifdef MCCO
	HHAL_DisablePassSnid();
#endif
    
    HAL_RegisterProcBcnCallback(HOMEPLUG_GetHal(),
        LINKL_StaProcBcnHandler,
        linkLayer);

}

void LINKL_SetKeyDone(sLinkLayer *linkl)
{
	sAuthInd 	authInd;
    sStaInfo    *staInfo = LINKL_GetStaInfo(linkl);	
	sEvent	    xdata  *event = NULL;
	u8			*pos; 

	authInd.keyType = KEY_TYPE_NEK;
    authInd.secMode  = staInfo->secMode;
    authInd.result = STATUS_SUCCESS;

	event = EVENT_Alloc(sizeof(uEventBody), 0);
	if(event == NULL)
	{
		FM_Printf(FM_ERROR, "Linkl:EAF\n");
		return;
	}
	event->eventHdr.eventClass = EVENT_CLASS_CTRL;
	event->eventHdr.type = EVENT_TYPE_AUTH_IND;

	pos = event->buffDesc.dataptr;
	memcpy(pos, &authInd, sizeof(sAuthInd));
	event->buffDesc.datalen = sizeof(sAuthInd);

		
	/* deliver the event to the upper layer */
#ifdef CALLBACK
	linkl->deliverEvent(linkl->eventcookie, event);
#else
	CTRLL_ReceiveEvent(linkl->eventcookie, event);
#endif

}

void LINKL_SetHpgpNetworkKey(sLinkLayer *linkLayer)
{
	u8 i;
	u8			pwd[32];
	sStaInfo *staInfo = &linkLayer->staInfo;
	u8 salt[16] = {0x08, 0x85, 0x6D, 0xAF, 0x7C, 0xF5, 0x81, 0x85,
   				0x08, 0x85, 0x6D, 0xAF, 0x7C, 0xF5, 0x81, 0x85};

	/*Compiler warning suppression*/
	i = i;
	pwd[0] = pwd[0];
					
	staInfo->nekEks = 0;
	staInfo->staStatus.fields.authStatus = 1; 

	memcpy(staInfo->nek, salt, ENC_KEY_LEN);
	
#ifdef HPGP_HAL
        /* set the NEK to the LMAC for the CCo */
    HHAL_AddNEK(staInfo->nekEks, staInfo->nek);
#endif

}

void LINKL_StartSta(sLinkLayer *linkLayer, u8 staType)
{
    u8        snsmStaType = staType;
    sStaInfo *staInfo = &linkLayer->staInfo;
    if(linkLayer->mode != LINKL_STA_MODE_STA)
        return;

    if (staType == LINKL_STA_TYPE_NETDISC)
    {
        if (staInfo->secMode == SEC_MODE_SC_JOIN)
            snsmStaType = LINKL_STA_TYPE_SC_JOIN;
        else if (staInfo->secMode == SEC_MODE_SC_ADD)
            snsmStaType = LINKL_STA_TYPE_SC_ADD;
    }
#ifdef HPGP_HAL
	else
	{
	    //HHAL_SetSnid(staInfo->snid);
	}
#endif

#ifdef AKM
    AKM_Start(&linkLayer->akm, LINKL_STA_MODE_STA, 0);
#else
	
#endif
    showStaType(LINKL_STA_MODE_STA, staType);
    SNSM_Start(&linkLayer->staNsm, snsmStaType);
    SNAM_Start(&linkLayer->staNam, staType);
#ifdef POWERSAVE
    SPSM_Start(&linkLayer->staPsm);
#endif



}


void LINKL_StopSta(sLinkLayer *linkLayer)
{
    sStaInfo *staInfo = &linkLayer->staInfo;
    sCrm          *crm = LINKL_GetCrm(linkLayer);
    if(staInfo->ccoScb != NULL)
    {
#ifdef LOG_FLASH
        scbFreeReason = MCTRL_TRIG;
#endif
        CRM_FreeScb(&linkLayer->ccoRm, staInfo->ccoScb);
    }
    staInfo->ccoScb = NULL;
    
    if(linkLayer->mode == LINKL_STA_MODE_STA)
    {
        SNSM_Stop(&linkLayer->staNsm);
        SNAM_Stop(&linkLayer->staNam);
#ifdef POWERSAVE
        SPSM_Stop(&linkLayer->staPsm);
#endif
    }
	
	staInfo->tei = 0;


	HAL_SetTei(HOMEPLUG_GetHal(), 0);


	//CRM_FreeScb(crm, staInfo->staScb);
	//Remove the TEI MAP
	CRM_Init(crm);
	staInfo->staScb = NULL;
	staInfo->ccoScb = NULL;

	gHpgpHalCB.gFreqCB.freqDetected = 0;

	gHpgpHalCB.halStats.BcnSyncCnt = 0;
		

}

#ifdef HOM
u8 LINKL_DetermineStaRole(sLinkLayer *linkLayer)
{
    if(linkLayer->mode != LINKL_STA_MODE_STA)
        return STA_ROLE_UNKNOWN;

    return SNSM_DetermineStaRole(&linkLayer->staNsm);
}


void LINKL_CcoHandover(sLinkLayer *linkl)
{
    sEvent *event = NULL;
    //Generate a event
    event = EVENT_Alloc(EVENT_DEFAULT_SIZE, 0);
    if(event == NULL)
    {
        FM_Printf(FM_ERROR, "EAF\n");
        return;
    }
    event->eventHdr.type = EVENT_TYPE_CCO_HO_REQ;
#ifdef P8051
__CRIT_SECTION_BEGIN__
#else
    SEM_WAIT(&linkl->linkSem);
#endif

    //post the event to the event queue
    SLIST_Put(&linkl->eventQueue, &event->link);

#ifdef P8051
__CRIT_SECTION_END__
#else
    SEM_POST(&linkl->linkSem);
#endif
}
#endif
#endif /* STA_FUNC */


#ifdef CCO_FUNC
/* ---------------
 *  CCO Mode
 *  -------------- */
//#define HPGP_INIT_CCO_TEI  0x1


void LINKL_ClearBcnInit()
{
    HHAL_ClearBcnInit();
}

void LINKL_SetBcnInit()
{
    HHAL_SetBcnInit();
}

void LINKL_SetCcoMode(sLinkLayer *linkLayer)
{
	u8 i,j;
    sStaInfo *staInfo = &linkLayer->staInfo;

	/*Compiler warning suppression*/
	i = i;
	j = j;
	
    linkLayer->mode = LINKL_STA_MODE_CCO;
    
    if(staInfo->ccoScb == NULL)
    {
        staInfo->ccoScb = CRM_AllocScb(&linkLayer->ccoRm);
        if(staInfo->ccoScb == NULL)
        {
            FM_Printf(FM_ERROR, "Can't alloc SCB for CCo\n");
            return;
        }
        staInfo->staScb = staInfo->ccoScb;
#ifdef ROUTE
        ROUTE_initLrtEntry(staInfo->ccoScb);
        staInfo->ccoScb->lrtEntry.nTei = staInfo->ccoScb->tei;
        staInfo->ccoScb->lrtEntry.rnh = 0;
#endif

        // reset discoverd sta count
        staInfo->numDiscSta = 0;
        staInfo->ccoScb->staCap.byte = staInfo->staCap.byte;
#ifdef LINK_PRINT		
#ifdef P8051
FM_Printf(FM_ERROR, "tei:%bu, staCap %x\n", 
         staInfo->ccoScb->tei, staInfo->ccoScb->staCap.byte);
#else
FM_Printf(FM_ERROR, "tei:%d, staCap %x\n", 
         staInfo->ccoScb->tei, staInfo->ccoScb->staCap.byte);
#endif
#endif
      	
        memcpy(staInfo->ccoScb->macAddr, staInfo->macAddr, MAC_ADDR_LEN);

		staInfo->tei = staInfo->ccoScb->tei;
		
        HAL_SetTei(linkLayer->hal, staInfo->ccoScb->tei);
        /* generate a snid */
#if 0		
        srand(STM_GetTick());
		staInfo->snid = rand()&0xF;
#else

#ifndef MCCO
      	 for(j=1; j<MAXSNID; j++)
        {
			for(i=0; i< AVLN_LIST_MAX; i++)
            {
			    if((staInfo->discNetInfo[i].valid) &&
					(staInfo->discNetInfo[i].snid == j))
                {
                    break;
                }
            }
			if(i == AVLN_LIST_MAX)
            {                
                staInfo->snid = j;                
                break;
			}
            }
        }
#endif

#endif

    }
	staInfo->ccoScb->staStatus.byte = staInfo->staStatus.byte;

	staInfo->ccoScb->staCap.fields.ccoStatus = 1;
	staInfo->ccoScb->staCap.fields.backupCcoCap = 1;
	staInfo->ccoScb->staCap.fields.backupCcoStatus = 0;
	staInfo->ccoScb->staCap.fields.pcoStatus = 0;
#ifdef POWERSAVE
		PSM_resetScbPs(staInfo->ccoScb);	// set SCB's PS data to init state 
#endif

#if 0

#ifdef HPGP_HAL
        /* set the snid */
        HHAL_SetSnid(staInfo->snid);
#ifdef MCCO		
		/* set CCo Mode to the LMAC */
    //HHAL_SetDevMode(linkLayer->hal, DEV_MODE_CCO, LINE_MODE_DC);

	if (gHpgpHalCB.devMode == DEV_MODE_PCCO)
	{
		HHAL_SetDevMode(DEV_MODE_PCCO, gHpgpHalCB.lineMode);
	}
	else			
	{
		HHAL_SetDevMode(DEV_MODE_CCO, gHpgpHalCB.lineMode);
	}

#else
        
	HHAL_SetDevMode(DEV_MODE_CCO, gHpgpHalCB.lineMode);

#endif //Endif of MCO

#else
        HAL_SetDevMode(linkLayer->hal, DEV_MODE_CCO);
#endif

#endif

    HAL_RegisterXmitBcnCallback(HOMEPLUG_GetHal(),
        LINKL_BcnTxHandler,
        linkLayer);
    HAL_RegisterProcBcnCallback(HOMEPLUG_GetHal(),
        LINKL_CcoProcBcnHandler,
        linkLayer);
}


void LINKL_StartCco(sLinkLayer *linkLayer, u8 ccoType)
{
    sStaInfo *staInfo = &linkLayer->staInfo;

    if(linkLayer->mode == LINKL_STA_MODE_CCO)
    {        
        showStaType(LINKL_STA_MODE_CCO, ccoType);
        CNSM_Start(&linkLayer->ccoNsm, ccoType);
        CNAM_Start(&linkLayer->ccoNam, ccoType);
#ifdef POWERSAVE
        CPSM_Start(&linkLayer->ccoPsm);
#endif
#ifdef LLP_POWERSAVE
		if (ccoType == LINKL_CCO_TYPE_UNASSOC)
		{
			PSM_resetScbPs(staInfo->ccoScb);	// set SCB's PS data to init state 
			PSM_copy_PS_from_HalCB(staInfo->ccoScb);	// copy saved PS data from HALCB
			PSM_cvrtPss_Awd(staInfo->ccoScb->pss, &staInfo->ccoScb->commAwd);
		}
#endif 
#ifdef AKM
		if (ccoType == LINKL_CCO_TYPE_HO)
        {
            AKM_Start(&linkLayer->akm, LINKL_STA_MODE_CCO, AKM_KEEP_NEK);
        }
        else //if(ccoType == LINKL_CCO_TYPE_HO)
        {
         //   AKM_Start(&linkLayer->akm, LINKL_STA_MODE_CCO, AKM_KEEP_NEK);
        }
#endif		

    }
}

void LINKL_PostStopCCo(sLinkLayer *linkLayer)
{

	sStaInfo *staInfo = &linkLayer->staInfo;
	HAL_SetTei(HOMEPLUG_GetHal(), 0);

	staInfo->tei = 0;

	LINKL_ClearBcnInit();
	
    CNSM_PostStop(LINKL_GetCnsm(linkLayer));

#ifdef MCCO
	HHAL_DisablePassSnid();
#endif

	HHAL_SetDevMode(DEV_MODE_STA,gHpgpHalCB.lineMode);
		
	if(staInfo->ccoScb != NULL)
	{
#ifdef LOG_FLASH
        scbFreeReason = MCTRL_TRIG;
#endif
		CRM_FreeScb(&linkLayer->ccoRm, staInfo->ccoScb);

		staInfo->ccoScb = NULL;
	}


	LINKL_SetStaMode(linkLayer);


}


eStatus LINKL_StopCco(sLinkLayer *linkLayer)
{

    sStaInfo *staInfo = &linkLayer->staInfo;

    CNSM_Stop(LINKL_GetCnsm(linkLayer));

    CNAM_Stop(LINKL_GetCnam(linkLayer));
	
#ifdef POWERSAVE
//        CPSM_Stop(LINKL_GetCnsm(linkLayer));
#endif
#ifdef ROUTE
    ROUTE_stopUpdateTimer();
#endif
    
    return STATUS_SUCCESS;
}

/*
void LINKL_EnableAssocNotification(sLinkLayer *linkLayer)
{
    if(linkLayer->mode != LINKL_STA_MODE_CCO)
        return;
    CNAM_EnableAssocNotification(&linkLayer->ccoNam);
}
*/


u8 LINKL_QueryAnySta(sLinkLayer *linkLayer)
{
    u8 cnt = 0;
    if(linkLayer->mode != LINKL_STA_MODE_CCO)
        return FALSE;
    cnt = CRM_GetScbNum(&linkLayer->ccoRm);
    if(cnt <= 0) // Invalid condition crm should have one scb for CCO.
    {
        return FALSE;
    }
    if((CRM_GetScbNum(&linkLayer->ccoRm) - 1))        
    {
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}


#endif /* CCO_FUNC */

u8 LINKL_QueryAnyAlvn(sLinkLayer *linkLayer)
{
    if(linkLayer->mode != LINKL_STA_MODE_CCO)
        return 0;

#ifdef CCO_FUNC
    return CNSM_QueryAnyAlvn(&linkLayer->ccoNsm);
#endif

}

eStatus LINKL_SendEvent(sLinkLayer *linkl, sEvent *event) __REENTRANT__
{
    if(event == NULL)
    {
        return STATUS_FAILURE;
    }

#ifdef P8051
__CRIT_SECTION_BEGIN__
#else
    SEM_WAIT(&linkl->linkSem);
#endif

    SLIST_Put(&linkl->eventQueue, &event->link);

#ifdef P8051
__CRIT_SECTION_END__
#else
    SEM_POST(&linkl->linkSem);
#endif
    /* schedule the task */
#ifndef RTX51_TINY_OS
    return SCHED_Sched(&linkl->task);
#else
   // os_set_ready(HPGP_TASK_ID_LINK);

	os_set_ready(HPGP_TASK_ID_CTRL);

	return STATUS_SUCCESS;
#endif
    
}

eStatus LINKL_Init(sLinkLayer *linkLayer)
{
    eStatus    status = STATUS_SUCCESS;
    sStaInfo *staInfo = &linkLayer->staInfo;

    
    LINKL_InitStaInfo(linkLayer);
//    LINKL_InitCcoInfo(linkLayer);

    linkLayer->muxl = (sMuxLayer *)HPGPCTRL_GetLayer(HP_LAYER_TYPE_MUX);
    linkLayer->hal = HOMEPLUG_GetHal();
    linkLayer->mode = LINKL_STA_MODE_STA;
#ifdef STA_FUNC
    //STA mode
    SNSM_Init(&linkLayer->staNsm, linkLayer);
    SNAM_Init(&linkLayer->staNam, linkLayer);
#ifdef HOM
    SHOM_Init(&linkLayer->staHom, linkLayer);
#endif
#endif
#ifdef POWERSAVE
    SPSM_Init(&linkLayer->staPsm, linkLayer);
#ifdef LINK_PRINT	
    FM_Printf(FM_MINFO, "Link Layer:after SPSM_Init\n");
#endif
#endif

#ifdef CCO_FUNC
    //CCO mode
    CNSM_Init(&linkLayer->ccoNsm, linkLayer);
    CNAM_Init(&linkLayer->ccoNam, linkLayer);
#ifdef HOM
    CHOM_Init(&linkLayer->ccoHom, linkLayer);
#endif
#endif

#ifdef AKM
    AKM_Init(&linkLayer->akm);
#endif
    CRM_Init(&linkLayer->ccoRm);
#ifdef POWERSAVE
    CPSM_Init(&linkLayer->ccoPsm, linkLayer);
#ifdef LINK_PRINT	
    FM_Printf(FM_MINFO, "Link Layer:after CPSM_Init\n");
#endif
#endif

#ifndef P8051
#if defined(WIN32) || defined(_WIN32)
    linkLayer->linkSem = CreateSemaphore(
        NULL,           // default security attributes
        SEM_COUNT,      // initial count
        SEM_COUNT,      // maximum count
        NULL);          // unnamed semaphore
    if(linkLayer->linkSem == NULL)
#else
    if(sem_init(&linkLayer->linkSem, 0, SEM_COUNT))
#endif
    {
        status = STATUS_FAILURE;
    }
#endif

    SLIST_Init(&linkLayer->eventQueue);
    SLIST_Init(&linkLayer->intEventQueue);
#ifdef RTX51_TINY_OS
  //  os_create_task(HPGP_TASK_ID_LINK);
#else
    SCHED_InitTask(&linkLayer->task, HPGP_TASK_ID_LINK, "LINK", 
                   HPGP_TASK_PRI_LINK, LINKL_Proc, linkLayer);
#endif
    MUXL_RegisterMgmtMsgCallback(linkLayer->muxl, 
                                 LINKL_RecvMgmtMsg, 
                                 (void*) linkLayer);
#ifdef HPGP_HAL
    if(opMode == LOWER_MAC)
    {
        HHAL_SetDevMode(DEV_MODE_CCO, LINE_MODE_DC);
    }
    else
    {
        HHAL_SetDevMode(DEV_MODE_STA, gHpgpHalCB.lineMode);
    }
#else
    HAL_SetDevMode(linkLayer->hal, DEV_MODE_STA);
#endif
#ifdef STA_FUNC
    HAL_RegisterProcBcnCallback(HOMEPLUG_GetHal(),
        LINKL_StaProcBcnHandler,
        linkLayer);
#endif

	linkLayer->scanTimer = STM_AllocTimer(HP_LAYER_TYPE_LINK, 
				   			EVENT_TYPE_TIMER_SCAN_TIMEOUT_IND,
				   			linkLayer);

    if(linkLayer->scanTimer == STM_TIMER_INVALID_ID)
    {
	    FM_Printf(FM_MINFO, "Link:Scan timer fail\n");    
    }

    //FM_Printf(FM_MINFO, "Link:Init\n");
    return status;
}


void LINKL_TimerHandler(u16 type, void *cookie)
{
    u8          headroom = 0;
    sEvent     *event = NULL;
    sHpgpHdr   *hpgpHdr = NULL;
    sLinkLayer *linkl = (sLinkLayer *)cookie;

#ifdef SIMU
    if (type == EVENT_TYPE_TIMER_BCN_TX_IND)
    {
        LINKL_BcnTimerHandler(type, cookie);
        return;
    }
#endif

	if(type == EVENT_TYPE_TIMER_SCAN_TIMEOUT_IND)
	{	
		u8 i;
		u8 idx;
		u8 nwCnt;
				
		hostEvent_ScanInd_t scanList;	
		sLinkLayer* linkl = (sLinkLayer *)HPGPCTRL_GetLayer(HP_LAYER_TYPE_LINK);
		sStaInfo *staInfo = &linkl->staInfo;
	
		/*Compiler warning suppression*/
		i = i;
		
		//SCB_GetDiscNetList(&scanList.networkList, &scanList.noOfEntries);		
		idx=0;
		nwCnt=0;
		
		for(idx=0;idx<DISC_NET_LIST_MAX;idx++)
		{
			if(staInfo->discNetInfo[idx].valid)
				{
					scanList.networkList[nwCnt].valid = 
								staInfo->discNetInfo[idx].valid;
					
					scanList.networkList[nwCnt].bcnRxCnt = 
								staInfo->discNetInfo[idx].bcnRxCnt;
					scanList.networkList[nwCnt].rssi = 
								staInfo->discNetInfo[idx].rssi;
					scanList.networkList[nwCnt].lqi = 
								staInfo->discNetInfo[idx].lqi;
					memcpy((u8*)&scanList.networkList[nwCnt].nid,
							(u8*)&staInfo->discNetInfo[idx].nid,
							NID_LEN);
					scanList.networkList[nwCnt].snid = 
								staInfo->discNetInfo[idx].snid;

					
					scanList.networkList[nwCnt].numOfSta = 
								staInfo->discNetInfo[idx].numOfSta;

					memcpy((u8*)&scanList.networkList[nwCnt].vendor_ota,
							(u8*)&staInfo->discNetInfo[idx].vendor_ota,
							sizeof(staInfo->discNetInfo[idx].vendor_ota));

					nwCnt++;

				}

		}
		///memcpy((u8*)scanList.networkList, (u8*)staInfo->discNetInfo, 
		//	DISC_NET_LIST_MAX*sizeof(sDiscNetInfo));
		
		scanList.noOfEntries = nwCnt;
		
		//FM_HexDump(FM_ERROR,"\nNetlist:",&scanList.networkList,DISC_NET_LIST_MAX*sizeof(sDiscNetInfo));		
		//printf("noOfEntries %bu\n", scanList.noOfEntries);
		Host_SendIndication(HOST_EVENT_SCAN_COMPLETE_IND, HPGP_MAC_ID, (u8*)&scanList,
								sizeof(hostEvent_ScanInd_t));		
	}
	
    if ( (type == EVENT_TYPE_TIMER_TEI_IND ) ||
	     (type == EVENT_TYPE_TIMER_KEY_IND)  || 
		 (type == EVENT_TYPE_TIMER_TEK_IND)
#ifdef CCO_FUNC
         || (type == EVENT_TYPE_TIMER_HO_IND) 
#endif
       )
    {
        headroom = EVENT_HPGP_CTRL_HEADROOM;
    } 

    event = EVENT_Alloc(0, headroom);
    if(event == NULL)
    {
        FM_Printf(FM_ERROR, "EAF\n");
        return;
    }

    event->eventHdr.eventClass = EVENT_CLASS_CTRL;
    event->eventHdr.type = type;
   
    if ( (type == EVENT_TYPE_TIMER_TEI_IND ) 
#ifdef CCO_FUNC
         || (type == EVENT_TYPE_TIMER_HO_IND) 
#endif
       )
    {
        hpgpHdr = (sHpgpHdr *)event->buffDesc.buff;
        hpgpHdr->scb = (sScb *)cookie;
        linkl = (sLinkLayer *)HPGPCTRL_GetLayer(HP_LAYER_TYPE_LINK);
    }

    /* post the event to the external event queue */
    LINKL_SendEvent(linkl, event);
}

extern void SCB_AgeDiscLists(sScb *scb);

void LINKL_StartScan(tTime scanTime)
{
	u8 i;
    sLinkLayer  *linkl = (sLinkLayer *)HPGPCTRL_GetLayer(HP_LAYER_TYPE_LINK);
	sStaInfo *staInfo = &linkl->staInfo;
	sScb* scb = linkl->staNsm.staInfo->staScb;
	
	/*Compiler warning suppression*/
	i = i;

#if 0

    for(i = 0; i < DISC_NET_LIST_MAX; i++)
    {
        if(staInfo->discStaInfo[i].valid == TRUE)
        {
            memset(&staInfo->discNetInfo[i], 0, sizeof(sDiscStaInfo));
        }
    }
#endif			
	SCB_ClearAgeDiscLists();
	STM_StopTimer(linkl->scanTimer);
	STM_StartTimer(linkl->scanTimer, scanTime);		
}

#if 1

#ifdef RTX51_TINY_OS
void LINK_Task (void)// _task_ HPGP_TASK_ID_LINK  
{
    sLinkLayer  *linkl = (sLinkLayer *)HPGPCTRL_GetLayer(HP_LAYER_TYPE_LINK);
#ifdef UM
	sNma* nma = (sNma*)HOMEPLUG_GetNma();
#endif
    while (1) {
#ifdef UART_HOST_INTF
		os_switch_task();
#else	
        //os_wait1(K_SIG);
		os_switch_task();

#endif		
        LINKL_Proc(linkl);
#ifdef UM
		NMA_Proc(nma);
#endif
    }
}
#endif

#endif

/** =========================================================
 *
 * Edit History
 *
 * $Source: /home/cvsrepo/Hybrii_B_OSFW_Dev/firmware/hpgp/src/link/linkl.c,v $
 *
 * $Log: linkl.c,v $
 * Revision 1.36  2015/01/02 14:55:36  kiran
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
 * Revision 1.35  2014/12/09 07:09:08  ranjan
 * - multicco feature under MCCO flag
 *
 * Revision 1.34  2014/11/11 14:52:58  ranjan
 * 1.New Folder Architecture espically in /components
 * 2.Modular arrangment of functionality in new files
 *    anticipating the need for exposing them as FW App
 *    development modules
 * 3.Other improvisation in code and .h files
 *
 * Revision 1.33  2014/10/28 16:27:43  kiran
 * 1) Software recovery using Watchdog Timer
 * 2) Hardware recovery monitor and policies
 * 3) Timer Polling in Control Task and Frame task for better accuracy
 * 4) Common memory optimized by reducing prints
 * 5) Discovered netlist corruption fixed
 * 6) VCO fix in HHAL_AFEInit()
 * 7) Idata optimized by removing floating point operation
 * 8) Fixed EVENT_TYPE_CC_BCN_IND false indication during association @ CCO
 * 9) Beacon processing protected from interrupts
 * 10) Corrupted Beacons are dropped
 * 11) Some unused arguments removed to improve code size
 *
 * Revision 1.32  2014/09/30 21:47:18  tri
 * Added LLP PS
 *
 * Revision 1.31  2014/09/05 09:28:18  ranjan
 * 1. uppermac cco-sta switching feature fix
 * 2. general stability fixes for many station associtions
 * 3. changed mgmt memory pool for many STA support
 *
 * Revision 1.30  2014/08/25 07:37:34  kiran
 * 1) RSSI & LQI support
 * 2) Fixed Sync related issues
 * 3) Fixed timer 0 timing drift for SDK
 * 4) MMSG & Error Logging in Flash
 *
 * Revision 1.29  2014/08/12 08:45:43  kiran
 * 1) Event fixes
 * 2) API to change UART line control parameters
 *
 * Revision 1.28  2014/07/22 10:03:52  kiran
 * 1) SDK Supports Power Save
 * 2) Uart_Driver.c cleanup
 * 3) SDK app memory pool optimization
 * 4) Prints from STM.c are commented
 * 5) Print messages are trimmed as common no memory left in common
 * 6) Prins related to Power save and some other modules are in compilation flags. To enable module specific prints please refer respective module
 *
 * Revision 1.27  2014/07/16 10:47:40  kiran
 * 1) Updated SDK
 * 2) Fixed Diag test in SDK
 * 3) Ethernet and SPI interfaces removed from SDK as common memory is less
 * 4) GPIO access API's added in SDK
 * 5) GV701x chip reset command supported
 * 6) Start network and Join network supported in SDK (Forced CCo and STA)
 * 7) Some bug fixed in SDK (CP free, p app command issue etc.)
 *
 * Revision 1.26  2014/07/05 09:16:27  prashant
 * 100 Devices support- only association tested, memory adjustments
 *
 * Revision 1.25  2014/07/01 09:49:57  kiran
 * memory (xdata) improvement
 *
 * Revision 1.24  2014/06/26 17:59:42  ranjan
 * -fixes to make uppermac more robust for n/w change
 *
 * Revision 1.23  2014/06/24 16:26:45  ranjan
 * -zigbee frame_handledata fix.
 * -added reason code for uppermac host events
 * -small cleanups
 *
 * Revision 1.22  2014/06/23 06:56:44  prashant
 * Ssn reset fix upon device reset, Duplicate SNID fix
 *
 * Revision 1.21  2014/06/19 17:13:19  ranjan
 * -uppermac fixes for lvnet and reset command for cco and sta mode
 * -backup cco working
 *
 * Revision 1.20  2014/06/19 07:16:02  prashant
 * Region fix, frequency setting fix
 *
 * Revision 1.19  2014/06/12 13:15:43  ranjan
 * -separated bcn,mgmt,um event pools
 * -fixed datapath issue due to previous checkin
 * -work in progress. neighbour cco detection
 *
 * Revision 1.18  2014/06/11 13:17:47  kiran
 * UART as host interface and peripheral interface supported.
 *
 * Revision 1.17  2014/06/05 08:38:41  ranjan
 * -flash function enabled for uppermac
 * - commit command after any change would flash systemprofiles
 * - verfied upper mac
 *
 * Revision 1.16  2014/05/28 10:58:59  prashant
 * SDK folder structure changes, Uart changes, removed htm (UI) task
 * Varified - UM, LM - iperf (UDP/TCP), overnight test pass
 *
 * Revision 1.15  2014/05/21 23:03:08  tri
 * more PS
 *
 * Revision 1.14  2014/05/16 08:52:30  kiran
 * - System Profile Flashing API's Added. Upper MAC functionality tested
 *
 * Revision 1.13  2014/05/12 08:09:57  prashant
 * Route fix, STA sync issue with AV CCO and handling discover bcn issue fix
 *
 * Revision 1.12  2014/04/30 22:28:43  tri
 * more PS
 *
 * Revision 1.11  2014/04/09 21:09:04  tri
 * more PS
 *
 * Revision 1.10  2014/03/27 23:52:23  tri
 * more PS
 *
 * Revision 1.9  2014/03/20 23:20:04  tri
 * more PS
 *
 * Revision 1.8  2014/03/12 09:41:22  ranjan
 * 1. added ageout event to cco cnam,backupcco ageout handling
 * 2.  fix linking issue in zb_lx51_asic due to backup cco checkin
 *
 * Revision 1.7  2014/03/10 05:58:10  ranjan
 * 1. added HomePlug BackupCCo feature. verified C&I test.(passed.) (bug 176)
 *
 * Revision 1.6  2014/03/08 18:15:26  tri
 * added more PS code
 *
 * Revision 1.5  2014/02/27 10:42:47  prashant
 * Routing code added
 *
 * Revision 1.4  2014/02/19 10:22:41  ranjan
 * - common sync for hal_tst and upper mac project
 * - ism.c is MAC interrupt handler for hhal_tst and upper mac.
 *    chal_ext1isr function   is removed
 * - verified : lower mac sync, upper mac sync data traffic.
 *
 * Revision 1.3  2014/01/28 17:46:40  tri
 * Added Power Save code
 *
 * Revision 1.2  2014/01/10 17:17:53  yiming
 * check in Rajan 1/8/2014 code release
 *
 * Revision 1.5  2014/01/08 10:53:54  ranjan
 * Changes for LM OS support.
 * New Datapath FrameTask
 * LM and UM  datapath, feature verified.
 *
 * known issues : performance numbers needs revisit
 *
 * review : pending.
 *
 * Revision 1.4  2013/09/04 14:51:01  yiming
 * New changes for Hybrii_A code merge
 *
 * Revision 1.34  2013/07/12 08:56:36  ranjan
 * -UKE Push Button Security Feature.
 * Verified : DirectEntry Security Works.Datapath Works.
 *                 command SetSecMode for UKE works.
 * Added against bug-160
 *
 * Revision 1.33  2013/05/21 18:47:41  kripa
 * *** empty log message ***
 *
 * Revision 1.32  2013/05/16 08:38:41  prashant
 * "p starttest" command merged in upper mac
 * Dignostic mode added in upper mac
 *
 * Revision 1.31  2013/03/26 12:07:26  ranjan
 * -added  host sw reset command
 * - fixed issue in bcn update
 *
 * Revision 1.30  2013/03/22 12:21:49  prashant
 * default FM_MASK and FM_Printf modified for USER INFO
 *
 * Revision 1.29  2013/03/14 11:49:18  ranjan
 * 1.handled cases  for CCo toSTA switch and  viceversa
 * 2.UM uses bcntemplate
 *
 * Revision 1.28  2013/02/05 10:19:57  ranjan
 * Fix compilation issue and unresolved extern
 *
 * Revision 1.27  2012/11/19 07:46:23  ranjan
 * Changes for Network discovery modes
 *
 * Revision 1.26  2012/10/11 06:21:00  ranjan
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
 * Revision 1.25  2012/07/08 18:42:20  yuanhua
 * (1)fixed some issues when ctrl layer changes its state from the UCC to ACC. (2) added a event CNSM_START.
 *
 * Revision 1.24  2012/06/30 23:36:26  yuanhua
 * return the success status for LINKL_SendEvent() when RTX51 OS is used.
 *
 * Revision 1.23  2012/06/29 03:01:58  kripa
 * Adding setLineMode routine.
 * Committed on the Free edition of March Hare Software CVSNT Client.
 * Upgrade to CVS Suite for more features and support:
 * http://march-hare.com/cvsnt/
 *
 * Revision 1.22  2012/06/20 17:55:58  kripa
 *  CVS: Enter Log.  Lines beginning with `CVS:' are removed automatically
 * Adding SetBcntInit() routine.
 * Commenting out SetDevMode() call from LINKL_SetStaMode(), to get Bcn sync to work.
 * (Repeated calling of this routine was interfering with bcn sync)
 * Committed on the Free edition of March Hare Software CVSNT Client.
 * Upgrade to CVS Suite for more features and support:
 * http://march-hare.com/cvsnt/
 *
 * Revision 1.21  2012/06/15 04:35:21  yuanhua
 * add a STA type of passive unassoc STA. With this STA type, the device acts as a STA during the network discovery. It performs the network scan for beacons from the CCO, but does not transmit the UNASSOC_STA.IND and does not involve in the CCO selection process. Thus, it joins the existing network.
 *
 * Revision 1.20  2012/06/13 06:24:31  yuanhua
 * add code for tx bcn interrupt handler integration and data structures for region entry schedule. But they are not in execution yet.
 *
 * Revision 1.19  2012/06/08 05:50:57  yuanhua
 * added snid function.
 *
 * Revision 1.18  2012/06/05 22:37:12  son
 * UART console does not get initialized due to task ID changed
 *
 * Revision 1.17  2012/06/05 07:25:59  yuanhua
 * (1) add a scan call to the MAC during the network discovery. (2) add a tiny task in ISM for interrupt polling (3) modify the frame receiving integration. (4) modify the tiny task in STM.
 *
 * Revision 1.16  2012/06/04 23:34:02  son
 * Added RTX51 OS support
 *
 * Revision 1.15  2012/05/24 05:08:18  yuanhua
 * define sendEvent functions in CTRL/LINK layer as reentrant.
 *
 * Revision 1.14  2012/05/21 04:20:59  yuanhua
 * enable/disable MAC interrupts when STA/CCO starts.
 *
 * Revision 1.13  2012/05/19 20:32:17  yuanhua
 * added non-callback option for the protocol stack.
 *
 * Revision 1.12  2012/05/19 05:05:15  yuanhua
 * optimized the timer handlers in CTRL and LINK layers.
 *
 * Revision 1.11  2012/05/17 05:05:58  yuanhua
 * (1) added the option for timer w/o callback (2) added task id and name.
 *
 * Revision 1.10  2012/05/14 05:22:29  yuanhua
 * support the SCHED without using callback functions.
 *
 * Revision 1.9  2012/05/01 04:51:09  yuanhua
 * added compiler flags STA_FUNC and CCO_FUNC in link and ctrl layers.
 *
 * Revision 1.8  2012/04/30 04:05:57  yuanhua
 * (1) integrated the HAL mgmt Tx. (2) various updates
 *
 * Revision 1.7  2012/04/17 23:09:50  yuanhua
 * fixed compiler errors for the hpgp hal test due to the integration changes.
 *
 * Revision 1.6  2012/04/15 20:35:09  yuanhua
 * integrated beacon RX changes in HAL and added HTM for on board test.
 *
 * Revision 1.5  2012/04/13 06:15:11  yuanhua
 * integrate the HPGP protocol stack with the HAL for the beacon TX/RX and mgmt msg RX.
 *
 * Revision 1.4  2012/03/11 17:02:24  yuanhua
 * (1) added NEK auth in AKM (2) added NMA (3) modified hpgp layer data structures (4) added Makefile for linux simulation
 *
 * Revision 1.3  2011/09/18 01:32:08  yuanhua
 * designed the AKM for both STA and CCo.
 *
 * Revision 1.2  2011/09/09 07:02:31  yuanhua
 * migrate the firmware code from the greenchip to the hybrii.
 *
 * Revision 1.13  2011/08/12 23:13:21  yuanhua
 * (1)Added Control Layer (2) Fixed bugs for user-selected CCo handover (3) Made changes to SNAM/CNAM and SNSM/CNSM for CCo handover switch (from CCo to STA, from STA to CCo, and from STA to STA but with different CCo) and post CCo handover
 *
 * Revision 1.12  2011/08/09 22:45:44  yuanhua
 * changed to event structure, seperating HPGP-related events from the general event defination so that the general event could be used for other purposes than the HPGP.
 *
 * Revision 1.11  2011/08/08 22:05:41  yuanhua
 * user-selected CCo handover fix
 *
 * Revision 1.10  2011/08/05 17:06:29  yuanhua
 * (1) added an internal queue in Link Layer for communication btw modules within Link Layer (2) Fixed bugs in CCo Handover. Now, CCo handover could be triggered by auto CCo selection, CCo handover messages work fine (3) Made some modifications in SHAL.
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
 * Revision 1.3  2011/04/24 17:53:28  kripa
 * *** empty log message ***
 *
 * Revision 1.2  2011/04/23 17:38:57  kripa
 * u8 LINKL_DetermineCcoTrans(sLinkLayer *linkLayer, sEvent *event); changed event->class to event->eventClass
 *
 * Revision 1.1  2011/04/08 21:42:45  yuanhua
 * Framework
 *
 *
 * =========================================================*/
