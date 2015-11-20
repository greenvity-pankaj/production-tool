/** ==========================================================
 *
 * @file cnam.c
 * 
 *  @brief Network Access Manager 
 *
 *  Copyright (C) 2010-2011, Greenvity Communications, Inc.
 *  All Rights Reserved
 *  
 * =========================================================*/

#include <string.h>
#include <assert.h>
#include "papdef.h"
#ifdef ROUTE
#include "hpgp_route.h"
#endif
#include "list.h"
#include "hpgpdef.h"
#include "hal.h"
#include "nma.h"
#include "linkl.h"
#include "nam.h"
#include "nsm.h"
#include "crm.h"
#include "timer.h"
#include "stm.h"
#include "hpgpevt.h"
#include "nma.h"
#include "nma_fw.h"
#include "hpgpapi.h"
#include "fm.h"
#include "muxl.h"
#include "hal.h"
#include "frametask.h"
#include "linkl.h"
#ifdef SIMU
#include "sdrv.h"
#endif
#ifdef UM
#include "gv701x_osal.h"
#endif
#include "hybrii_tasks.h"
#include "sys_common.h"
#include "hpgp_msgs.h"
#include "event_fw.h"


//#define HPGP_TIME_TEI_REUSE        300000  //5 minutes
//#define HPGP_TIME_TEI_REUSE        6000  //5 minutes
#define HPGP_TIME_TEI_REUSE       50  //5 minutes

#define HPGP_TIME_USER_APPT_CCO    60000   //1 minute
#define HPGP_TIME_ASSOC            1000     //100 ms
#define HPGP_IDENTIFY_CAP_TIME               500

#ifdef KEEP_ALIVE
#define HPGP_TIME_KEEP_ALIVE			2000 //30000 // 30sec
#define KEEP_ALIVE_HIT_COUNT			3
#endif

//15 minutes: default lease time for STA associated but not authenticated
#define HPGP_TIME_TEI_LEASE_NOAUTH_MIN        1//0x0F 
//48 hours: default lease time for STA associated and authenticated
#define HPGP_TIME_TEI_LEASE_AUTH_MIN          0xB40 


#define HPGP_ASSOC_RESULT_SUCCESS         0   //resource found/allocated
#define HPGP_ASSOC_RESULT_TEMP_NO_RES     1   //resource temporarily exhausted
#define HPGP_ASSOC_RESULT_NO_RES          2   //resource permanently exhausted
#define HPGP_ASSOC_RESULT_UNKNOWN         3   //due to other reason


#define HPGP_TEI_MAP_MODE_NEW             0   // new STA
#define HPGP_TEI_MAP_MODE_ADD             1   // add STA
#define HPGP_TEI_MAP_MODE_DELETE          2   // add STA

#define HPGP_TEI_MAP_STATUS_ASSOC         0x0
#define HPGP_TEI_MAP_STATUS_AUTH          0x1
#define HPGP_TEI_MAP_STATUS_DISASSOC      0x2
#define HPGP_TEI_MAP_STATUS_INVALID       0xFF


#define HPGP_CCO_APPT_REQ_APPT_HO      0x00   //Appoint CCo and handover
#define HPGP_CCO_APPT_REQ_UNAPPT       0x01   //unappoint
#define HPGP_CCO_APPT_REQ_UNAPPT_HO    0x01   //unappoint CCo and handover

//CCO APPOINT CNF RESULT
#define HPGP_CCO_APPT_CNF_ACCEPT       0x00   //accept
#define HPGP_CCO_APPT_CNF_REJECT       0x01   //reject
#define HPGP_CCO_APPT_CNF_UNKNOWN_STA  0x02   //unknown user-appointed STA
#define HPGP_CCO_APPT_CNF_APPT_CCO     0x03   //CCO is user appointed already
#define HPGP_CCO_APPT_CNF_CCO_UNAPPT   0x04   //CCo is unappointed  
#define HPGP_CCO_APPT_CNF_CCO_NOT_APPT 0x05   //CCo is not user-appointed 
#define HPGP_CCO_APPT_CNF_OTHERS       0x06   //unknown user-appointed STA
#define HPGP_CCO_APPT_CNF_UNAPPT_HO    0x07   //CCo is unappointed and handover
#define HPGP_CCO_APPT_CNF_UNKNOWN_STA2 0x08   //unknown user-appointed STA and
                                              //continue as a user-appointed CCo

#define HPGP_CCO_BACKUP_REQ_APPOINT    0
#define HPGP_CCO_BACKUP_REQ_RELEASE    1
#ifdef LOG_FLASH
extern u16 scbFreeReason;
#endif
/* --------------------------
 * CCO network access manager
 * -------------------------- */



typedef struct backupCCoParam
{
    u8    dstTei;         //destination STA
    u8   *dstMacAddr;     //adestination STA
    u8    action;
} sBackupCCoParam;


typedef struct assocCnfParam
{
    u8    result;
    u8    staTei;
    u16   teiLeaseTime;
    u8    dstTei;         //destination STA
    u8   *dstMacAddr;     //adestination STA
} sAssocCnfParam;

typedef struct leaveIndParam
{
    u8    reason;
    u8    dstTei;         //destination STA
    u8   *dstMacAddr;     //destination STA
} sLeaveIndParam;


typedef struct leaveCnfParam
{
    u8    dstTei;         //destination STA
    u8   *dstMacAddr;     //destination STA
} sLeaveCnfParam;


typedef struct teiMapIndParam
{
    u8    mode;   
    u8    staTei;         //add or delete STA
    u8   *staMacAddr;     //add or delete STA
    u8    staStatus;      //add or delete STA
    u8    dstTei;         //destination STA
    u8   *dstMacAddr;     //destination STA
} sTeiMapIndParam;

typedef struct ccoApptCnfParam
{
    u8    result;
    u8    dstTei;         //destination STA
    u8   *dstMacAddr;     //destination STA
} sCcoApptCnfParam;

typedef union mgmtMsgParamRef
{
    sAssocCnfParam  *assocCnf;
    sLeaveIndParam  *leaveInd;  
    sLeaveCnfParam  *leaveCnf;  
    sTeiMapIndParam *teiMapInd;
    sCcoApptCnfParam *ccoApptCnf;
	sBackupCCoParam  *ccoBackupReq;
} uMgmtMsgParamRef;

extern void LINKL_TimerHandler(u16 type, void *cookie);
#ifdef ROUTE
extern void ROUTE_preparteAndSendUnreachable(sScb *scb);
extern eStatus ROUTE_sendRouteInfoReq(sScb *scb);
extern void ROUTE_initLrtEntry(sScb *scb);
#endif
extern eStatus IDENTIFY_sendFrm(u16 mmType, sEvent *reqEvent, sScb *scb);
extern eStatus IDENTIFY_procFrm(u16 mmType, sEvent *reqEvent);
extern eStatus NMA_SendCcoApptCnf(sNma *nma, u8 result);

#ifdef CCO_FUNC

//#define FM_Printf(x,y,z)   

eStatus CNAM_SendMgmtMsg(sCnam *cnam, u16 mmType, void *msgParam)
{
    eStatus           status = STATUS_FAILURE;
    sEvent        xdata    *newEvent = NULL;
    sHpgpHdr          *hpgpHdr = NULL;
    sCcAssocCnf       *assocCnf = NULL;
    sCcLeaveInd       *leaveInd = NULL;
    sTeiMap           *teiMap = NULL;
    sCcTeiMapInd      *teiMapInd = NULL;
    sCcCcoApptCnf     *ccoApptCnf = NULL;
	sCcBackupReq      *ccoBackUpReq = NULL;
    uMgmtMsgParamRef   mgmtMsgParam;
    sScb              *scbIter = NULL;
    u8                *dataptr = NULL;
    u16                freeLen = 0;
    u8                 numSta = 0;
    u16                eventSize = 0;
//    sLinkLayer  *linkl = (sLinkLayer *)HPGPCTRL_GetLayer(HP_LAYER_TYPE_LINK);
    sLinkLayer        *linkl = cnam->linkl;
    sStaInfo          *staInfo = LINKL_GetStaInfo(linkl);
    
	//FM_HexDump(FM_USER,"cMtype",(u8*)&mmType, 2);

    switch(mmType)
    {
        case EVENT_TYPE_CC_ASSOC_CNF:
        {
#ifdef LOG_FLASH
            u8 buff[2];
#endif
            eventSize = MAX(HPGP_DATA_PAYLOAD_MIN, sizeof(sCcAssocCnf)); 
            newEvent = EVENT_MgmtAlloc(eventSize, EVENT_HPGP_MSG_HEADROOM);
            if(newEvent == NULL)
            {
                //FM_Printf(FM_ERROR, "Cannot allocate an event.\n");
                return STATUS_FAILURE;
            }
            mgmtMsgParam.assocCnf = (sAssocCnfParam *)msgParam;
            newEvent->eventHdr.eventClass = EVENT_CLASS_MSG;
            newEvent->eventHdr.type = EVENT_TYPE_CC_ASSOC_CNF;

            hpgpHdr = (sHpgpHdr *)newEvent->buffDesc.buff;
            hpgpHdr->tei =  mgmtMsgParam.assocCnf->dstTei; //TODO
            hpgpHdr->macAddr =  mgmtMsgParam.assocCnf->dstMacAddr;
            hpgpHdr->snid = staInfo->snid;
			
			hpgpHdr->eks = HPGP_EKS_NONE;

            assocCnf = (sCcAssocCnf *)(newEvent->buffDesc.dataptr); 

            assocCnf->result = mgmtMsgParam.assocCnf->result; 
            assocCnf->staTei = mgmtMsgParam.assocCnf->staTei; 
            memcpy(assocCnf->nid, staInfo->nid, NID_LEN);
            assocCnf->snid = staInfo->snid;
            assocCnf->leaseTime = mgmtMsgParam.assocCnf->teiLeaseTime;  

            newEvent->buffDesc.datalen += eventSize;
#ifdef LOG_FLASH
            buff[0] = assocCnf->result;            
            buff[1] = assocCnf->staTei;
            
            logEvent(MGMT_MSG, 0, EVENT_TYPE_CC_ASSOC_CNF, buff, 2);
#endif
            FM_Printf(FM_MMSG, "CNAM:>>CC_ASSOC.CNF(tei:%bu)\n",
                                hpgpHdr->tei);
            break;
        }
        case EVENT_TYPE_CC_LEAVE_CNF:
        {
            newEvent = EVENT_MgmtAlloc(HPGP_DATA_PAYLOAD_MIN, 
                                   EVENT_HPGP_MSG_HEADROOM);
            if(newEvent == NULL)
            {
//                FM_Printf(FM_ERROR, "Cannot allocate an event.\n");
                return STATUS_FAILURE;
            }
            mgmtMsgParam.leaveCnf = (sLeaveCnfParam *)msgParam;  
            newEvent->eventHdr.eventClass = EVENT_CLASS_MSG;
            newEvent->eventHdr.type = EVENT_TYPE_CC_LEAVE_CNF;

            hpgpHdr = (sHpgpHdr *)newEvent->buffDesc.buff;
            hpgpHdr->tei = mgmtMsgParam.leaveCnf->dstTei; //TODO
            hpgpHdr->macAddr = mgmtMsgParam.leaveCnf->dstMacAddr;
            hpgpHdr->snid = staInfo->snid;

			hpgpHdr->eks = staInfo->nekEks;
						
		
            newEvent->buffDesc.datalen += HPGP_DATA_PAYLOAD_MIN;
#ifdef LOG_FLASH
            logEvent(MGMT_MSG, 0, EVENT_TYPE_CC_LEAVE_CNF, &hpgpHdr->tei, 1);
#endif
#ifdef P8051
            FM_Printf(FM_MMSG, "CNAM:>>CC_LEAVE.CNF(tei:%bu)\n",
                                hpgpHdr->tei);
#else
            FM_Printf(FM_MMSG, "CNAM:>>CC_LEAVE.CNF(tei:%d)\n",
                                hpgpHdr->tei);
#endif
            break;
        }
        case EVENT_TYPE_CC_LEAVE_IND:
        {
            eventSize = MAX(HPGP_DATA_PAYLOAD_MIN, sizeof(sCcLeaveInd)); 
            newEvent = EVENT_MgmtAlloc(eventSize, EVENT_HPGP_MSG_HEADROOM);
            if(newEvent == NULL)
            {
//                FM_Printf(FM_ERROR, "Cannot allocate an event.\n");
                return STATUS_FAILURE;
            }

            mgmtMsgParam.leaveInd = (sLeaveIndParam *)msgParam;  
            newEvent->eventHdr.eventClass = EVENT_CLASS_MSG;
            newEvent->eventHdr.type = EVENT_TYPE_CC_LEAVE_IND;

            hpgpHdr = (sHpgpHdr *)newEvent->buffDesc.buff;
            hpgpHdr->tei = mgmtMsgParam.leaveInd->dstTei; //TODO
            hpgpHdr->macAddr = mgmtMsgParam.leaveInd->dstMacAddr;
            hpgpHdr->snid = staInfo->snid;

			hpgpHdr->eks = staInfo->nekEks;
						
            leaveInd = (sCcLeaveInd *)(newEvent->buffDesc.dataptr); 
            memcpy(leaveInd->nid, staInfo->nid, NID_LEN);
            leaveInd->reason = mgmtMsgParam.leaveInd->reason;

            newEvent->buffDesc.datalen += eventSize;
#ifdef LOG_FLASH
            logEvent(MGMT_MSG, 0, EVENT_TYPE_CC_LEAVE_IND, &hpgpHdr->tei, 1);
#endif
#ifdef P8051
            FM_Printf(FM_MMSG, "CNAM:>>CC_LEAVE.IND(tei:%bu)\n",
                                hpgpHdr->tei);
#else
            FM_Printf(FM_MMSG, "CNAM:>>CC_LEAVE.IND(tei:%d)\n",
                                hpgpHdr->tei);
#endif
            break;
        }
        case EVENT_TYPE_CC_SET_TEI_MAP_IND:
        {
            mgmtMsgParam.teiMapInd = (sTeiMapIndParam *)msgParam;  

            if( mgmtMsgParam.teiMapInd->mode == HPGP_TEI_MAP_MODE_NEW)
            {
                //send the SET_TEI_MAP.IND to the new STA
                numSta = CRM_GetScbNum(cnam->crm);
            }
            else
            {
                numSta = 1;
            }

            eventSize = MAX(HPGP_DATA_PAYLOAD_MIN, (2+(u16)(sizeof(sTeiMap)*numSta))); 
            newEvent = EVENT_MgmtAlloc(eventSize, EVENT_HPGP_MSG_HEADROOM);
            if(newEvent == NULL)
            {
	//                FM_Printf(FM_ERROR, "Cannot allocate an event.\n");
                return STATUS_FAILURE;
            }

            newEvent->eventHdr.eventClass = EVENT_CLASS_MSG;
            newEvent->eventHdr.type = EVENT_TYPE_CC_SET_TEI_MAP_IND;

            hpgpHdr = (sHpgpHdr *)newEvent->buffDesc.buff;
            hpgpHdr->tei =  mgmtMsgParam.teiMapInd->dstTei;
            hpgpHdr->macAddr =  mgmtMsgParam.teiMapInd->dstMacAddr;
            hpgpHdr->snid = staInfo->snid;
			hpgpHdr->eks = HPGP_EKS_NONE;
			hpgpHdr->mcst = 1;
						

            //build the message
            dataptr = newEvent->buffDesc.dataptr;
            teiMapInd = (sCcTeiMapInd *) dataptr;
            teiMapInd->mode = mgmtMsgParam.teiMapInd->mode;
            teiMapInd->numSta = numSta; 
            newEvent->buffDesc.datalen += 2;
            dataptr += 2;

            freeLen = eventSize - 2;//newEvent->buffDesc.bufflen - 2 - 
                      //(newEvent->buffDesc.dataptr - newEvent->buffDesc.buff);

            if( mgmtMsgParam.teiMapInd->mode == HPGP_TEI_MAP_MODE_NEW)
            {
                //send the SET_TEI_MAP.IND to the new STA
                while( (freeLen >= sizeof(sTeiMap)) && numSta)
                {
                    teiMap = (sTeiMap *) dataptr;
                    scbIter = CRM_GetNextScb(cnam->crm, scbIter);
                    if(scbIter == NULL)
                    {
//                        FM_Printf(FM_ERROR, "CNAM: imcompatible in CRM.\n");
                        EVENT_Free(newEvent);
                        return STATUS_FAILURE;
                    }

                    teiMap->tei = scbIter->tei; 
                    memcpy(teiMap->macAddr, scbIter->macAddr, MAC_ADDR_LEN); 
                    teiMap->status = scbIter->staStatus.fields.authStatus; 
                    dataptr += sizeof(sTeiMap);
                    newEvent->buffDesc.datalen += sizeof(sTeiMap);
                    freeLen -= sizeof(sTeiMap);

                    numSta--;
                }
                if((freeLen > 0)&&numSta)
                {
//                    FM_Printf(FM_ERROR, "CNAM: imcompatible with numSta.\n");
                    EVENT_Free(newEvent);
                    return STATUS_FAILURE;
                }
            }
            else
            {

                teiMapInd->teiMap.tei = mgmtMsgParam.teiMapInd->staTei; 
                memcpy(teiMapInd->teiMap.macAddr, mgmtMsgParam.teiMapInd->staMacAddr, MAC_ADDR_LEN); 
                teiMapInd->teiMap.status = mgmtMsgParam.teiMapInd->staStatus; 
                newEvent->buffDesc.datalen += sizeof(sTeiMap); 
            }

            newEvent->buffDesc.datalen = MAX(HPGP_DATA_PAYLOAD_MIN, newEvent->buffDesc.datalen);
    #ifdef P8051
            FM_Printf(FM_MMSG, "CNAM:>>SET_TEI_MAP.IND(tei:%bu)\n",
    #else
            FM_Printf(FM_MMSG, "CNAM:>>SET_TEI_MAP.IND(tei:%d)\n",
    #endif
                                hpgpHdr->tei);
            break;
        }
#ifdef APPOINT
        case EVENT_TYPE_CC_CCO_APPOINT_CNF:
        {
            eventSize = MAX(HPGP_DATA_PAYLOAD_MIN, sizeof(sCcCcoApptCnf)); 
            newEvent = EVENT_MgmtAlloc(eventSize, EVENT_HPGP_MSG_HEADROOM);
            if(newEvent == NULL)
            {
                FM_Printf(FM_ERROR, "AErr\n");
                return STATUS_FAILURE;
            }
            mgmtMsgParam.ccoApptCnf = (sCcoApptCnfParam *)msgParam;
            newEvent->eventHdr.type = EVENT_TYPE_CC_CCO_APPOINT_CNF;
            newEvent->eventHdr.eventClass = EVENT_CLASS_MSG;

            hpgpHdr = (sHpgpHdr *)newEvent->buffDesc.buff;
            hpgpHdr->tei =  mgmtMsgParam.ccoApptCnf->dstTei; //TODO
            hpgpHdr->macAddr =  mgmtMsgParam.ccoApptCnf->dstMacAddr;
            hpgpHdr->snid = staInfo->snid;
			hpgpHdr->eks = staInfo->nekEks;

            ccoApptCnf = (sCcCcoApptCnf *)(newEvent->buffDesc.dataptr); 
            ccoApptCnf->result = mgmtMsgParam.ccoApptCnf->result;
            newEvent->buffDesc.datalen += eventSize;
            FM_Printf(FM_MMSG, "CNAM:>>CCO_APPOINT.CNF(tei:%bu)\n",
                                hpgpHdr->tei);
            break;
        }
#endif
        case EVENT_TYPE_CC_BACKUP_APPOINT_REQ:
        {
            eventSize = MAX(HPGP_DATA_PAYLOAD_MIN, sizeof(ccoBackUpReq)); 
            newEvent = EVENT_MgmtAlloc(eventSize, EVENT_HPGP_MSG_HEADROOM);
            if(newEvent == NULL)
            {
                FM_Printf(FM_ERROR, "AErr\n");
                return STATUS_FAILURE;
            }
            mgmtMsgParam.ccoBackupReq = (sBackupCCoParam*)msgParam;
            newEvent->eventHdr.type = EVENT_TYPE_CC_BACKUP_APPOINT_REQ;
            newEvent->eventHdr.eventClass = EVENT_CLASS_MSG;

            hpgpHdr = (sHpgpHdr *)newEvent->buffDesc.buff;
            hpgpHdr->tei =  mgmtMsgParam.ccoBackupReq->dstTei; //TODO
            hpgpHdr->macAddr =  mgmtMsgParam.ccoBackupReq->dstMacAddr;
            hpgpHdr->snid = staInfo->snid;

			hpgpHdr->eks = staInfo->nekEks;

            ccoBackUpReq = (sCcBackupReq*)(newEvent->buffDesc.dataptr); 
            ccoBackUpReq->action = mgmtMsgParam.ccoBackupReq->action;
            newEvent->buffDesc.datalen += eventSize;
            FM_Printf(FM_MMSG, "CNAM:>>BACKUP_APPOINT_REQ:(tei:%bu)\n",
                                hpgpHdr->tei);
            break;
        }
        default:
        {
        }
    }

	
    //EVENT_Assert(newEvent);
    assert((newEvent->buffDesc.dataptr >= newEvent->buffDesc.buff)&&
           ((newEvent->buffDesc.dataptr - newEvent->buffDesc.buff + 
             newEvent->buffDesc.datalen) <= newEvent->buffDesc.bufflen));  
	

    //transmit the mgmt msg
    status =  MUXL_TransmitMgmtMsg(newEvent);
    //the event will be freed by MUXL if the TX is successful
    if(status == STATUS_FAILURE)
    {
        EVENT_Free(newEvent);
    }

    return status;
}

extern u8 bcAddr[];

#if 1

eStatus CNAM_SendTeiMapInd(sCnam *cnam, sScb *scb, u8 operation)
{
    sScb              *scbIter = NULL;
    sTeiMapIndParam    teiMapIndParam;
    u8                 txInd = 1;
	sScb *minRssiScb = NULL;
	u8 minRssi = 200;

	scbIter = CRM_GetNextScb(cnam->crm, scbIter);
    while(scbIter)
	{

		if (scbIter != cnam->staInfo->staScb)
		{

			if (scbIter->rssiLqi.s.rssi < minRssi)
			{
				minRssiScb = scbIter;
				
			//	FM_Printf(FM_ERROR,"mrssi %bu \n", minRssi);
	
				minRssi = scbIter->rssiLqi.s.rssi;
			}

		}
		scbIter = CRM_GetNextScb(cnam->crm, scbIter);
	
	}

//	FM_Printf(FM_ERROR,"mrssi %bu \n", minRssi);
    scbIter = minRssiScb;
	
    if(scbIter)
    {
        if(1)//scbIter->tei != cnam->staInfo->ccoScb->tei)
        {
            txInd = 1;
//        teiMapIndParam.scb = scbIter;    //destination sta
            teiMapIndParam.dstTei = scbIter->tei;         //destination STA
            teiMapIndParam.dstMacAddr = bcAddr; //destination STA
            teiMapIndParam.staTei = scb->tei;
            teiMapIndParam.staMacAddr = scb->macAddr;
            teiMapIndParam.staStatus = scb->staStatus.fields.authStatus;
            if (operation ==  HPGP_TEI_MAP_MODE_ADD)
            { 
								/*Compiler warning suppression*/
#if 1
                if(1)//scbIter->tei == scb->tei)
                {
                    teiMapIndParam.mode = HPGP_TEI_MAP_MODE_NEW; 
                }
#else								
                else
									
                {
                    teiMapIndParam.mode = HPGP_TEI_MAP_MODE_ADD; 
                }
#endif								
            }
            else
            {
                if(scbIter->tei != scb->tei)
                {
                    teiMapIndParam.mode = HPGP_TEI_MAP_MODE_DELETE; 
                    teiMapIndParam.staStatus = HPGP_TEI_MAP_STATUS_DISASSOC;
                }
                else
                {
                    txInd = 0;
                }
            }

            if(txInd)
            {
                CNAM_SendMgmtMsg(cnam, EVENT_TYPE_CC_SET_TEI_MAP_IND, 
                                 &teiMapIndParam);
            }
        }

    //    scbIter = CRM_GetNextScb(cnam->crm, scbIter);
    } //end of while
    return STATUS_SUCCESS;

}




#else

eStatus CNAM_SendTeiMapInd(sCnam *cnam, sScb *scb, u8 operation)
{
    sScb              *scbIter = NULL;
    sTeiMapIndParam    teiMapIndParam;
    u8                 txInd = 1;
	
    scbIter = CRM_GetNextScb(cnam->crm, scbIter);
    while(scbIter)
    {
        if(scbIter->tei != cnam->staInfo->ccoScb->tei)
        {
            txInd = 1;
//        teiMapIndParam.scb = scbIter;    //destination sta
            teiMapIndParam.dstTei = scbIter->tei;         //destination STA
            teiMapIndParam.dstMacAddr = scbIter->macAddr; //destination STA
            teiMapIndParam.staTei = scb->tei;
            teiMapIndParam.staMacAddr = scb->macAddr;
            teiMapIndParam.staStatus = scb->staStatus.fields.authStatus;
            if (operation ==  HPGP_TEI_MAP_MODE_ADD)
            { 
                if(scbIter->tei == scb->tei)
                {
                    teiMapIndParam.mode = HPGP_TEI_MAP_MODE_NEW; 
                }
                else
                {
                    teiMapIndParam.mode = HPGP_TEI_MAP_MODE_ADD; 
                }
            }
            else
             {
                if(scbIter->tei != scb->tei)
                {
                    teiMapIndParam.mode = HPGP_TEI_MAP_MODE_DELETE; 
                    teiMapIndParam.staStatus = HPGP_TEI_MAP_STATUS_DISASSOC;
                }
                else
                {
                    txInd = 0;
                }
            }

            if(txInd)
            {
                CNAM_SendMgmtMsg(cnam, EVENT_TYPE_CC_SET_TEI_MAP_IND, 
                                 &teiMapIndParam);
            }
        }

        scbIter = CRM_GetNextScb(cnam->crm, scbIter);
    } //end of while
    return STATUS_SUCCESS;

}


#endif

void CNAM_ProcEvent(sCnam *cnam, sEvent *event)
{
    sEvent            *newEvent = NULL;
    sHpgpHdr          *hpgpHdr = NULL;
    sHpgpHdr          *newHpgpHdr = NULL;
    sCcAssocReq       *assocReq = NULL;
    sCcCcoApptReq     *ccoApptReq = NULL;
    sCcoHoReqEvent    *ccoHoReqEvent = NULL;
    sCcoHoRspEvent    *ccoHoRspEvent = NULL;
	sCcBackupCnf      *ccoBackUpCnf = NULL;
    sScb              *scb = NULL;
    sScb              *scbIter = NULL;
    sAssocCnfParam     assocCnfParam;
    sLeaveIndParam     leaveIndParam;
    sLeaveCnfParam     leaveCnfParam;
    sCcoApptCnfParam   ccoApptCnfParam;
    sLinkLayer        *linkl = NULL;
    sStaInfo          *staInfo = NULL;
    u8                 txCcoApptCnf;
    sCnsm             *cnsm = NULL;
    u16                numSta = 0;
    static u8         scbFoundFlag = 0;

		/*Compiler warning suppression*/	
		ccoApptCnfParam = ccoApptCnfParam;
		txCcoApptCnf = txCcoApptCnf;
//    linkl = (sLinkLayer *)HPGPCTRL_GetLayer(HP_LAYER_TYPE_LINK);
    linkl = cnam->linkl;
    staInfo = LINKL_GetStaInfo(linkl);
   

    if(cnam->state == CNAM_STATE_INIT)
    {
#ifdef NAM_PRINT	
//        FM_Printf(FM_WARN, "CNAM not ready\n");
#endif		
        return;
    }

    hpgpHdr = (sHpgpHdr *)event->buffDesc.buff;
    if( event->eventHdr.eventClass == EVENT_CLASS_MSG) 
    {
        switch(event->eventHdr.type)
        {
            case EVENT_TYPE_CC_ASSOC_REQ:
            {
				assocReq = (sCcAssocReq *)event->buffDesc.dataptr; 

				assocReq->nid[NID_LEN - 1] &= NID_EXTRA_BIT_MASK;
				if (memcmp(assocReq->nid, staInfo->nid, NID_LEN))
				{
					break;
				}


				
                FM_Printf(FM_MMSG, "CNAM:<<CC_ASSOC.REQ(tei:%bu)\n",
                          hpgpHdr->tei);
                //process this event at any time regardless of STA state 
                assocCnfParam.staTei = 0;
                assocCnfParam.dstTei = 0;
                assocCnfParam.dstMacAddr = hpgpHdr->macAddr;
                
#ifdef LOG_FLASH
                logEvent(MGMT_MSG, 0, EVENT_TYPE_CC_ASSOC_REQ, hpgpHdr->macAddr, MAC_ADDR_LEN);
#endif
                if(assocReq->reqType == 0)
                {
                    scb = CRM_FindScbMacAddr(hpgpHdr->macAddr);
                    if(scb == NULL) 
                    {
                        //this is a new request
                        scb = CRM_AllocScb(cnam->crm);
                        scbFoundFlag = 0;
#ifdef POWERSAVE
                        if(scb)
                        {                            
							PSM_resetScbPs(scb);	// set SCB's PS data to init state 
						}
#endif
#ifdef ROUTE
                        if(scb)
                        {                            
                            ROUTE_initLrtEntry(scb);
                            scb->lrtEntry.nTei = scb->tei;
                            scb->lrtEntry.rnh = 0;
                        }
#endif
                    }
                    else
                    {
                        scbFoundFlag = 1;
                    }
                    if(scb)
                    {
//                        scb->uMaxSSN = 0;
                        scb->uMinSSN = 0;
                        scb->uWrapAround = 0;
                        if(scbFoundFlag == 0)
                        {
                            //allocate a tei lease timer for the STA
#ifdef CALLBACK
                            scb->teiTimer = STM_AllocTimer(LINKL_TimerHandler, 
                                                EVENT_TYPE_TIMER_TEI_IND, scb);
#else
                            scb->teiTimer = STM_AllocTimer(HP_LAYER_TYPE_LINK, 
                                                EVENT_TYPE_TIMER_TEI_IND, scb);
#endif
                        }
                        if(scb->teiTimer != STM_TIMER_INVALID_ID)
                        {
                            scb->staCap.fields.ccoCap = assocReq->ccoCap;
                            scb->staCap.fields.proxyNetCap = assocReq->proxyNetCap;
                            memcpy(scb->macAddr, hpgpHdr->macAddr, 
                                   MAC_ADDR_LEN);
                            assocCnfParam.result = HPGP_ASSOC_RESULT_SUCCESS;
                            assocCnfParam.teiLeaseTime = cnam->teiLeaseTime;
                            assocCnfParam.staTei = scb->tei;
                            assocCnfParam.dstTei = 0xFF;
                            if(scbFoundFlag == 0)
                            {
#ifdef SIMU                            
                                //call SHAL to add tei to the port MAP by macaddr?
                                //add requester tei to the Port Map in SHAL
                               SHAL_AddTeiToPortMap(HOMEPLUG_GetHal()->shal,
                                scb->macAddr, scb->tei);
#endif
   
                         }
												 
#if 0
#ifdef P8051
    FM_Printf(FM_HINFO,"CNAM: STA MAC Address: %bx:%bx:%bx:%bx:%bx:%bx\n",
#else
   FM_Printf(FM_HINFO,"CNAM: STA MAC Address: %.2x:%.2x:%.2x:%.2x:%.2x:%.2x\n",
#endif
                        scb->macAddr[0], scb->macAddr[1],
                        scb->macAddr[2], scb->macAddr[3],
                        scb->macAddr[4], scb->macAddr[5]);
#endif
												 
                        linkl->akm.peerTei = scb->tei;
                        linkl->akm.peerMacAddr = scb->macAddr;

                        }
                        else
                        {
                            assocCnfParam.result = HPGP_ASSOC_RESULT_TEMP_NO_RES;
                            STM_FreeTimer(scb->teiTimer);
                            scb->teiTimer = STM_TIMER_INVALID_ID;
#ifdef LOG_FLASH
                            scbFreeReason = ASSOC_NO_RESOURCE;
#endif
                            CRM_FreeScb(cnam->crm, scb);
                            scb = NULL;
                        }
                    }
                    else
                    {
                        // scb = NULL;
                        assocCnfParam.result = HPGP_ASSOC_RESULT_TEMP_NO_RES;
                    }

                    if ( CNAM_SendMgmtMsg(cnam, EVENT_TYPE_CC_ASSOC_CNF, 
                                          &assocCnfParam) == STATUS_SUCCESS)
                    {
                        //start the lease timer
                        if(scb)
                        {
                            //TODO: we need to check the lease timer
                            //so that it does not exceed our timer limit



							if(!scbFoundFlag)
								cnam->numSta++;

#ifndef AKM
                            scb->staStatus.fields.authStatus = 1;
#endif
							STM_StopTimer(scb->teiTimer);
                            STM_StartTimer(scb->teiTimer, 
                                           cnam->teiLeaseTime*60000);
#ifdef NAM_PRINT							
#ifdef P8051
							FM_Printf(FM_HINFO, "CNAM:lease(tei:%bu,time:0x%lx)\n",
#else
							FM_Printf(FM_HINFO, "CNAM:lease(tei:%d,time:0x%08x)\n",
#endif
                     scb->tei, assocCnfParam.teiLeaseTime*60000);
#endif
//FM_Printf(FM_HINFO, "CNAM: accNotif %bu\n",cnam->accNotification);   
                            if(cnam->accNotification)
                            {
#ifdef UKE							
                                {
                                    if ((scbFoundFlag == 0) &&
                                         (!memcmp(&cnam->ukePeer, &scb->macAddr, MAC_ADDR_LEN)))
                                    {

                                        //send the event to the SNAM to renew the TEI
                                        newEvent = EVENT_Alloc(MAC_ADDR_LEN, 
                                                           EVENT_HPGP_CTRL_HEADROOM);
                                        if(newEvent == NULL)
                                        {
                                            FM_Printf(FM_ERROR, 
                                                    "EAllocErr\n");
                                            break;
                                        }
                                        
                                        newEvent->eventHdr.eventClass = EVENT_CLASS_CTRL;
                                        newEvent->eventHdr.type = EVENT_TYPE_ASSOC_IND;
                                    
                                        
                                        hpgpHdr = (sHpgpHdr *)newEvent->buffDesc.buff;
                                        hpgpHdr->scb = (sScb*)scb;

                                        memcpy(newEvent->buffDesc.dataptr,
                                               cnam->ukePeer, MAC_ADDR_LEN);
                            
                                        newEvent->buffDesc.datalen = MAC_ADDR_LEN;

                                        SLIST_Put(&linkl->intEventQueue, &newEvent->link);

                                       // LINKL_SendEvent(linkl, newEvent);

                                   }
                                }
#endif								
                                newEvent = EVENT_Alloc(EVENT_DEFAULT_SIZE, 0);
                                if(newEvent)
                                {
                                    newEvent->eventHdr.eventClass = 
                                        EVENT_CLASS_CTRL;
                                    newEvent->eventHdr.type = EVENT_TYPE_NET_ACC_IND;
                                    //eeliver the event to the upper layer
#ifdef CALLBACK
                                    linkl->deliverEvent(linkl->eventcookie, newEvent);
#else
                                    CTRLL_ReceiveEvent(linkl->eventcookie, newEvent);
#endif
                                    cnam->accNotification = 0;
                                }
                                else
                                {
                                    FM_Printf(FM_ERROR, "EAllocErr\n");
                                }
                            }
							scb->namState = STA_NAM_STATE_CONN;
                            scb->homState = STA_HOM_STATE_IDLE;

							{

								hostEvent_peerAssocInd assocInd;

								memcpy(&assocInd.macAddress, (u8*)&scb->macAddr, MAC_ADDR_LEN);
								assocInd.tei = scb->tei;

								Host_SendIndication(HOST_EVENT_TYPE_PEER_ASSOC_IND, HPGP_MAC_ID,
													(u8*)&assocInd, sizeof(assocInd));

							}
#ifdef MCCO
							//linkl->ccoNsm.bcnUpdate = 1;
#endif
							

							if(scbFoundFlag == 1)
							{
								if (linkl->ccoNam.backupCCoCfg.scb == scb)
									linkl->ccoNam.backupCCoCfg.scb = NULL;	
							}

#if 0								
                            //if(scbFoundFlag == 0) // send tei map ind on every assoc req(if it is not renew), if device reset
                            {
                                //update the TEI map
                               	CNAM_SendTeiMapInd(cnam, 
                                                scb, 
                                                HPGP_TEI_MAP_MODE_ADD);
				
                            }
#endif
                           
                        }
                    }
                    else if(scbFoundFlag == 0)                            
                    {
                        //release resource
                        if(scb)
                        {
                            STM_FreeTimer(scb->teiTimer);
                            STM_FreeTimer(scb->staTimer);
                            scb->teiTimer = STM_TIMER_INVALID_ID;
                            scb->staTimer = STM_TIMER_INVALID_ID;                            
#ifdef LOG_FLASH
                            scbFreeReason = ASSOC_CNF_FAILED;
#endif							
							cnam->numSta--;

                            CRM_FreeScb(cnam->crm, scb);
                            scb = NULL;
                        }
                        
                    }
                }
                else  //renew
                {
                    //search through the CRM to get the scb
                    //scb = CRM_GetScb(cnam->crm, event->eventHdr.stei);
                    scb = (sScb *)(hpgpHdr->scb);
                    if(scb)
                    {
                        //save MAC address for TEI renew too 
                        //in case of CCo handover
                        scb->staCap.fields.ccoCap = assocReq->ccoCap;
                        scb->staCap.fields.proxyNetCap = assocReq->proxyNetCap;
                        memcpy(scb->macAddr, hpgpHdr->macAddr, 
                               MAC_ADDR_LEN);
                        assocCnfParam.staTei = scb->tei;
                        assocCnfParam.result = HPGP_ASSOC_RESULT_SUCCESS;
                  
                        // check if the sta is auth
                        if(scb->staStatus.fields.authStatus)
                        {
                            assocCnfParam.teiLeaseTime = cnam->teiLeaseTimeAuth;
                        }
                        else //unauth
                        {
                            assocCnfParam.teiLeaseTime = cnam->teiLeaseTime;
                        }
                        assocCnfParam.dstTei = scb->tei;
                        assocCnfParam.dstMacAddr = scb->macAddr;
                        //restart the TEI release timer
                        STM_StopTimer(scb->teiTimer);
                        STM_StartTimer(scb->teiTimer, 
                                       assocCnfParam.teiLeaseTime*60000);
#ifdef NAM_PRINT						
#ifdef P8051
FM_Printf(FM_HINFO, "CNAM:lease(0x%lx)\n",
#else
FM_Printf(FM_HINFO, "CNAM:lease(0x%.8x)\n",
#endif
                     assocCnfParam.teiLeaseTime*60000);
#endif                    
                    }
                    else
                    {
                        assocCnfParam.dstTei = hpgpHdr->tei;
                        assocCnfParam.dstMacAddr = hpgpHdr->macAddr;
                        assocCnfParam.result = HPGP_ASSOC_RESULT_UNKNOWN;
                    }

                    CNAM_SendMgmtMsg(cnam, EVENT_TYPE_CC_ASSOC_CNF, 
                                           &assocCnfParam);
					CNAM_SendTeiMapInd(cnam, 
						     			scb, 
										HPGP_TEI_MAP_MODE_ADD);			

					
					
                }
                break;
            }

			case EVENT_TYPE_CC_BACKUP_APPOINT_CNF:
			{
				scb = (sScb *)(hpgpHdr->scb);
				if(scb)
				{
					
					
//					FM_Printf(FM_MMSG, "CNAM:<<BACKUP_APPOINT_CNF(tei:%bu)\n",
	//						  scb->tei);

					if (scb->namState == STA_NAM_STATE_CONN)

					{
						ccoBackUpCnf = (sCcBackupCnf*)event->buffDesc.dataptr;

						if (!ccoBackUpCnf->result) 
						{  //Success
							scb->staCap.fields.backupCcoStatus = 1;
							 
							cnam->backupCCoCfg.result = 0;

							break;
		
						}

					}
				
					cnam->backupCCoCfg.scb = NULL;

					cnam->backupCCoCfg.result = 1;	
						
					CNAM_SelectBackupCCo(cnam, scb);
					
				}

				break;
			}
#ifdef APPOINT
            case EVENT_TYPE_CC_CCO_APPOINT_REQ: // user appointed CCO
            {
                scb = (sScb *)(hpgpHdr->scb);
                if(scb && (scb->namState == STA_NAM_STATE_CONN))
                {
//                FM_Printf(FM_MMSG, "CNAM:<<CCO_APPOINT.REQ(tei:%bu)\n",
  //                                  scb->tei);
                    //process the event only in STA_STATE_IDLE
                    ccoApptReq = (sCcCcoApptReq *)event->buffDesc.dataptr; 

                    txCcoApptCnf  = 0;
                    if(ccoApptReq->reqType == HPGP_CCO_APPT_REQ_APPT_HO)
                    {
                        //(1) am I an user appointed CCO already?
//                      if(ccoInfo->appointedCco)
                        if(staInfo->ccoScb->staStatus.fields.apptCcoStatus)
                        {
                            //already a user appointed CCo
                            ccoApptCnfParam.result = 
                                HPGP_CCO_APPT_CNF_APPT_CCO;      
                            //send a negaive CC_CCO_APPOINT_CNF response
                            txCcoApptCnf = 1;
                        }
                        else
                        {

                            //(2) check if the appointed STA is associated 
                            scbIter = NULL;
                            scbIter = CRM_GetNextScb(cnam->crm, scbIter);
                            while(scbIter)
                            {
                                if(memcmp(ccoApptReq->macAddr, 
                                       scbIter->macAddr, MAC_ADDR_LEN) == 0)
                                {
                                    break;
                                }
                                scbIter = CRM_GetNextScb(cnam->crm, scbIter);
                            }
                            if(scbIter == NULL)
                            {
                                //unknown user-appointed STA
                                ccoApptCnfParam.result = 
                                    HPGP_CCO_APPT_CNF_UNKNOWN_STA;      
                                //send a negaive CC_CCO_APPOINT_CNF response
                                txCcoApptCnf = 1;
                            }
                            else
                            {
                                //CCo is appointed and perform handover
//                                ccoInfo->ccoScb->staStatus.fields.apptCcoStatus = 1;
//                                ccoInfo->appointedCco = 1;
                                //success, send a CC_HANDOVER.REQ
                               txCcoApptCnf  = 0;
                            }
                        }
                    }
                    else if(ccoApptReq->reqType == HPGP_CCO_APPT_REQ_UNAPPT)
                    {
                        //if(ccoInfo->appointedCco)
                        if(staInfo->ccoScb->staStatus.fields.apptCcoStatus)
                        { 
                            //ccoInfo->appointedCco = 0;
                            staInfo->ccoScb->staStatus.fields.apptCcoStatus = 0;
                            //CCo is unappointed
                            ccoApptCnfParam.result = 
                                    HPGP_CCO_APPT_CNF_CCO_UNAPPT;      
                        }
                        else
                        {
                            //CCo is not a user appointed Cco
                            ccoApptCnfParam.result = 
                                    HPGP_CCO_APPT_CNF_CCO_NOT_APPT;      
                        }
                        txCcoApptCnf = 1;
                    }
                    else if(ccoApptReq->reqType == 
                                HPGP_CCO_APPT_REQ_UNAPPT_HO)
                    {
                        //check if the appointed STA is associated 
                        scbIter = NULL;
                        scbIter = CRM_GetNextScb(cnam->crm, scbIter);
                        while(scbIter)
                        {
                            if(memcmp(ccoApptReq->macAddr, 
                                   scbIter->macAddr, MAC_ADDR_LEN) == 0)
                            {
                                break;
                            }
                            scbIter = CRM_GetNextScb(cnam->crm, scbIter);
                        }
                        if(scbIter == NULL)
                        {
                            //unknown user-appointed STA
                            ccoApptCnfParam.result = 
                                HPGP_CCO_APPT_CNF_UNKNOWN_STA2;      
                            //send a negaive CC_CCO_APPOINT_CNF response
                            txCcoApptCnf = 1;
                        }
                        else
                        {
                            //ccoInfo->appointedCco = 0;
                            staInfo->ccoScb->staStatus.fields.apptCcoStatus = 0;
                            //success, send a CC_HANDOVER.REQ
                            txCcoApptCnf = 0;
                        }
                    }
                    else
                    {
#ifdef NAM_PRINT                    
                        FM_Printf(FM_ERROR, "CNAM:unknown req\n");
#endif
                    }

                    if(txCcoApptCnf)
                    {
                        ccoApptCnfParam.dstTei = hpgpHdr->tei;
                        ccoApptCnfParam.dstMacAddr = hpgpHdr->macAddr;
                        CNAM_SendMgmtMsg(cnam, EVENT_TYPE_CC_CCO_APPOINT_CNF, 
                                         &ccoApptCnfParam);
                    }
                    else
                    {
                        //initiate the CCO handover
                        newEvent = EVENT_Alloc(sizeof(sCcoHoReqEvent), 
                                               EVENT_HPGP_CTRL_HEADROOM);
                        if(newEvent == NULL)
                        {
                            FM_Printf(FM_ERROR, "AErr\n");
                            break ;
                        }


                        cnam->ccoApptOrigScb = scb;
                    
//                    scbIter->hoTrigger = HO_TRIGGER_TYPE_USER;
                        newEvent->eventHdr.eventClass = EVENT_CLASS_CTRL;
                        newEvent->eventHdr.type = EVENT_TYPE_CCO_HO_REQ;

                        newHpgpHdr = (sHpgpHdr *)newEvent->buffDesc.buff;

                        newHpgpHdr->scb = scbIter;
                        ccoHoReqEvent = (sCcoHoReqEvent *)(newEvent->buffDesc.dataptr); 
                        ccoHoReqEvent->reason = HPGP_HO_REASON_CCO_APPT;
                        newEvent->buffDesc.datalen +=sizeof(sCcoHoReqEvent);
                        EVENT_Assert(newEvent);
                        //LINKL_SendEvent(linkl, newEvent);
                        SLIST_Put(&linkl->intEventQueue, &newEvent->link);
                        scbIter->namState = STA_NAM_STATE_WAITFOR_CCO_HO_RSP;
                    }
                }
                break;
            }
#endif
            case EVENT_TYPE_CC_LEAVE_REQ:
            {
                //process the event only in STA_STATE_IDLE
                //search through the CRM to get the scb
                //scb = CRM_GetScb(cnam->crm, event->eventHdr.stei);
                hpgpHdr = (sHpgpHdr *)event->buffDesc.buff;
                scb = (sScb *)(hpgpHdr->scb);
				
				FM_Printf(FM_MMSG, "CNAM:<<LEAVE.REQ(tei:%bu)\n",
                                    scb->tei);
#ifdef LOG_FLASH
                logEvent(MGMT_MSG, 0, EVENT_TYPE_CC_LEAVE_REQ, &scb->tei, 1);
#endif
                if(scb && (scb->namState == STA_NAM_STATE_CONN))
                {
                    leaveCnfParam.dstTei = scb->tei;
                    leaveCnfParam.dstMacAddr = scb->macAddr;
                    //stop the TEI release timer
                    STM_StopTimer(scb->teiTimer);
      
                    //start the TEI reuser timer
                    STM_StartTimer(scb->teiTimer, 
                                   HPGP_TIME_TEI_REUSE);
                    scb->namState = STA_NAM_STATE_RELEASE;
					
                    CNAM_SendMgmtMsg(cnam, EVENT_TYPE_CC_LEAVE_CNF, 
                                           &leaveCnfParam);
                    //update the TEI map
                    CNAM_SendTeiMapInd(cnam, scb, HPGP_TEI_MAP_MODE_DELETE);

					{	
						hostEvent_peerLeave_t pLeave;
					
						memcpy(pLeave.macAddress, scb->macAddr, MAC_ADDR_LEN);
						pLeave.tei = scb->tei;
						pLeave.reason = HOST_EVENT_PEER_LEAVE_REASON_PEER_LEAVING;
					
					
						Host_SendIndication(HOST_EVENT_PEER_STA_LEAVE, HPGP_MAC_ID, (u8*)&pLeave,
											sizeof(hostEvent_peerLeave_t));
					
					}

#ifdef ROUTE
                    ROUTE_preparteAndSendUnreachable(scb);                    
#endif

                    //Remove MAC addr from scb list
                    //hal hpgp tx uses mac address so we cannot nullify here
//                    memset(&scb->macAddr, 0, MAC_ADDR_LEN);

#ifdef SIMU
                    //NOTE: if the CC_LEAVE_REQ is retransmitted
                    //it would not processed
                    SHAL_DeletePortMap(HOMEPLUG_GetHal()->shal,
                                       scb->tei);
#else
                    //call the HAL to remove TEI 
#endif
                }
                else
                {
#ifdef NAM_PRINT				
                    FM_Printf(FM_ERROR, "CNAM:no scb for leaving STA\n");
#endif					
                }
#ifdef POWERSAVE
				PSM_resetScbPs(scb);	// set SCB's PS data to init state 
#endif
                break;
            }
#ifdef STA_ID			
            case EVENT_TYPE_CM_STA_IDENTIFY_REQ:                    
            {
                IDENTIFY_procFrm(EVENT_TYPE_CM_STA_IDENTIFY_REQ, event);
                break;
            }
            case EVENT_TYPE_CM_STA_IDENTIFY_CNF:                    
            {
                IDENTIFY_procFrm(EVENT_TYPE_CM_STA_IDENTIFY_CNF, event);
                break;
            }
            case EVENT_TYPE_CM_STA_IDENTIFY_IND:                    
            {
                IDENTIFY_procFrm(EVENT_TYPE_CM_STA_IDENTIFY_IND, event);
                break;
            }
#endif			
#ifdef KEEP_ALIVE
			case EVENT_TYPE_VENDOR_KEEP_ALIVE_IND:
			{
				if(((sScb *)(hpgpHdr->scb)) != NULL)
				{
					if(((sScb *)(hpgpHdr->scb))->tei == hpgpHdr->tei)
					{
						((sScb *)(hpgpHdr->scb))->hit = 1;
						((sScb *)(hpgpHdr->scb))->hitCount = 0;
					}
				}
				//FM_Printf(FM_USER,"CNAM<<<Keep Alive:%bu\n",hpgpHdr->tei);
				break;
			}
#endif
            default:
            {
            }
        }
    }
    else // control message
    {
        switch(event->eventHdr.type)
        {

			case EVENT_TYPE_CNAM_STOP_REQ:
			{		
				sLeaveIndParam leaveIndParam;
#ifdef NAM_PRINT
				FM_Printf(FM_USER,"EVENT_TYPE_CNAM_STOP_REQ\n");
#endif				
				scbIter = CRM_GetNextScb(cnam->crm, NULL);
			
				if (LINKL_QueryAnySta(linkl))
				{
				    // Nothing to do
				    
					cnam->state = CNAM_STATE_SHUTTINGDOWN;
				}
				else
				{
					sEvent *newEvent;
				    newEvent = EVENT_Alloc(EVENT_DEFAULT_SIZE, 0);
				    if(newEvent)
				    {				
				        newEvent->eventHdr.eventClass = EVENT_CLASS_CTRL;

						cnam->state = CNAM_STATE_INIT;
						newEvent->eventHdr.type = EVENT_TYPE_NET_EXIT_IND;

				        CTRLL_ReceiveEvent(linkl->eventcookie, newEvent);		
								
				    }
#if 0 //this happens in linkl_poststopcco

					if(staInfo->ccoScb != NULL)
					{
						CRM_FreeScb(&linkl->ccoRm, staInfo->ccoScb);
					}

#endif
						break;
				}
                     				
				while(scbIter)
				{
					if(scbIter->tei != staInfo->tei)
					{
						
	                    leaveIndParam.reason = HPGP_LEAVE_IND_REASON_CCO_DOWN;
	                    leaveIndParam.dstTei = scbIter->tei;
	                    leaveIndParam.dstMacAddr = scbIter->macAddr;
	                    STM_StopTimer(scbIter->teiTimer);

	                    //start the TEI reuser timer
	                    STM_StartTimer(scbIter->teiTimer, 
	                                   50);
	                    scbIter->namState = STA_NAM_STATE_RELEASE;

	                    //send the CC_LEAVE.IND

//					FM_Printf(FM_USER,"cdown\n");
					
	                    CNAM_SendMgmtMsg(cnam, EVENT_TYPE_CC_LEAVE_IND, 
	                                           &leaveIndParam);
				
					}
				
					scbIter = CRM_GetNextScb(cnam->crm, scbIter);
				} //end of while
				

				break;
				
			}

				
		    case EVENT_TYPE_TIMER_TEI_IND:
			case EVENT_TYPE_STA_AGEOUT:
            {
                //process the event at any time 
                //tei lease timer
                hpgpHdr = (sHpgpHdr *)event->buffDesc.buff;
                scb = (sScb *)(hpgpHdr->scb);
#ifdef POWERSAVE 
				if (scb->psState == PSM_PS_STATE_ON)
				{
//					printf("CNAM_ProcEvent: event type %d with PS on (tei: %bu). Don't send LEAVE.IND\n",
//				              event->eventHdr.type, scb->tei);
					break;
				}
#endif
                //release the resource
                if(scb->namState == STA_NAM_STATE_CONN)
                {
                    leaveIndParam.reason = HPGP_LEAVE_IND_REASON_TEI_EXP;
                    leaveIndParam.dstTei = scb->tei;
                    leaveIndParam.dstMacAddr = scb->macAddr;
                    STM_StopTimer(scb->teiTimer);

                    //start the TEI reuser timer
                    STM_StartTimer(scb->teiTimer, 
                                   HPGP_TIME_TEI_REUSE);
                    scb->namState = STA_NAM_STATE_RELEASE;

					
					{	
						hostEvent_peerLeave_t pLeave;
					
						memcpy(pLeave.macAddress, scb->macAddr, MAC_ADDR_LEN);
						pLeave.tei = scb->tei;
						pLeave.reason = HOST_EVENT_PEER_LEAVE_REASON_TEI_TIMEOUT;
					
					
						Host_SendIndication(HOST_EVENT_PEER_STA_LEAVE, HPGP_MAC_ID, (u8*)&pLeave,
											sizeof(hostEvent_peerLeave_t));
					
					}

                    //send the CC_LEAVE.IND
//                    								FM_Printf(FM_USER,"age\n");
                    CNAM_SendMgmtMsg(cnam, EVENT_TYPE_CC_LEAVE_IND, 
                                           &leaveIndParam);
                    //update the TEI map
                    CNAM_SendTeiMapInd(cnam, scb, HPGP_TEI_MAP_MODE_DELETE);

					{	
						hostEvent_peerLeave_t pLeave;
					
						memcpy(pLeave.macAddress, scb->macAddr, MAC_ADDR_LEN);
						pLeave.tei = scb->tei;
						pLeave.reason = HOST_EVENT_PEER_LEAVE_REASON_PEER_LEAVING;
					
					
						Host_SendIndication(HOST_EVENT_PEER_STA_LEAVE, HPGP_MAC_ID, (u8*)&pLeave,
											sizeof(hostEvent_peerLeave_t));
					
					}
					
                    
#ifdef ROUTE
                     ROUTE_preparteAndSendUnreachable(scb);                    
#endif

					CNAM_BackupCCoAgeOut(cnam, scb);
                    //Remove MAC addr from scb list
                    memset(&scb->macAddr, 0, MAC_ADDR_LEN);

#ifdef SIMU
                    //NOTE: if the CC_LEAVE_REQ is retransmitted
                    //it may not be processed
                    SHAL_DeletePortMap(HOMEPLUG_GetHal()->shal,
                                       scb->tei);
#else
                    //call the HAL to remove TEI
#endif
                }
                else if( scb->namState == STA_NAM_STATE_RELEASE)
                { 
#ifdef NAM_PRINT				
#ifdef P8051
                    FM_Printf(FM_MINFO, 
                              "CNAM:free the scb for leaving STA(tei: %bu)\n",
                              scb->tei);
#else
                    FM_Printf(FM_MINFO, 
                              "CNAM:free the scb for leaving STA(tei: %d)\n",
                              scb->tei);
#endif
#endif
					CNAM_BackupCCoAgeOut(cnam, scb);

                    // STA_NAM_STATE_RELEASE:
                    STM_FreeTimer(scb->teiTimer);
                    STM_FreeTimer(scb->staTimer);
                    scb->teiTimer = STM_TIMER_INVALID_ID;
                    scb->staTimer = STM_TIMER_INVALID_ID;
                   
                    cnsm = LINKL_GetCnsm(linkl);
                    CNSM_UpdateDiscBcnSched(cnsm, scb);
                    
#ifdef LOG_FLASH
                    scbFreeReason = STA_AGEOUT;
#endif
 					cnam->numSta--;
                    CRM_FreeScb(cnam->crm, scb);

#ifdef MCCO					
					//linkl->ccoNsm.bcnUpdate = 1;
#endif							
                    // If last STA leave network Become unassoc

                    if (LINKL_QueryAnySta(linkl))
                    {
                        // Nothing to do
                    }
                    else
                    {
                        newEvent = EVENT_Alloc(EVENT_DEFAULT_SIZE, 0);
                        if(newEvent)
                        {
                            newEvent->eventHdr.eventClass = EVENT_CLASS_CTRL;

							if (cnam->state == CNAM_STATE_SHUTTINGDOWN)
							{
								cnam->state = CNAM_STATE_INIT;
								newEvent->eventHdr.type = EVENT_TYPE_NET_EXIT_IND;

							}
							else
							{
								newEvent->eventHdr.type = EVENT_TYPE_NO_STA_IND;
							}
											

							CTRLL_ReceiveEvent(linkl->eventcookie, newEvent);
							
                        }
                    }
                     
                }
             
                break;
            }
            case EVENT_TYPE_CCO_SELECT_IND: 
            {
                hpgpHdr = (sHpgpHdr *)event->buffDesc.buff;
                scb = (sScb *)(hpgpHdr->scb);
                if(scb && (scb->namState == STA_NAM_STATE_CONN))
                {
          
		            //initiate the CCO handover
                    newEvent = EVENT_Alloc(sizeof(sCcoHoReqEvent), 
                                           EVENT_HPGP_CTRL_HEADROOM);
                    if(newEvent == NULL)
                    {
                        FM_Printf(FM_ERROR, "EAllocErr\n");
                        break ;
                    }
//                    scb->hoTrigger = HO_TRIGGER_TYPE_AUTO;
                    newEvent->eventHdr.eventClass = EVENT_CLASS_CTRL;
                    newEvent->eventHdr.type = EVENT_TYPE_CCO_HO_REQ;
                    newHpgpHdr = (sHpgpHdr *)newEvent->buffDesc.buff;

                    newHpgpHdr->scb = scb;
                    ccoHoReqEvent = (sCcoHoReqEvent *)(newEvent->buffDesc.dataptr); 
                    ccoHoReqEvent->reason = HPGP_HO_REASON_CCO_SLCT;
                    newEvent->buffDesc.datalen +=sizeof(sCcoHoReqEvent);
                    EVENT_Assert(newEvent);
                    //LINKL_SendEvent(linkl, newEvent);
                    SLIST_Put(&linkl->intEventQueue, &newEvent->link);
                    scb->namState = STA_NAM_STATE_WAITFOR_CCO_HO_RSP;
                }
                break;
            }
#ifdef HOM
            case EVENT_TYPE_CCO_HO_RSP: 
            {
                //process the event only in STA_STATE_IDLE
                hpgpHdr = (sHpgpHdr *)event->buffDesc.buff;
                scb = (sScb *)(hpgpHdr->scb);
                if(scb && (scb->namState == STA_NAM_STATE_WAITFOR_CCO_HO_RSP))
                {
                    if(cnam->ccoApptOrigScb)
                    {
                        ccoApptCnfParam.dstTei = cnam->ccoApptOrigScb->tei;
                        ccoApptCnfParam.dstMacAddr = 
                            cnam->ccoApptOrigScb->macAddr;
                        ccoHoRspEvent = (sCcoHoRspEvent *)(event->buffDesc.dataptr); 
                        if( ccoHoRspEvent->reason == HPGP_HO_REASON_CCO_APPT)
                        {
                            if(ccoHoRspEvent->result == TRUE)
                            {
                                ccoApptCnfParam.result = 
                                    HPGP_CCO_APPT_CNF_ACCEPT;
                            }
                            else
                            {
                                ccoApptCnfParam.result = 
                                    HPGP_CCO_APPT_CNF_REJECT;
                            }
                        }
                        else if( ccoHoRspEvent->reason == HPGP_HO_REASON_CCO_SLCT)
                        {
                            if(ccoHoRspEvent->result == TRUE)
                            {
                                //CCo is un-appointed and perform handover
                                ccoApptCnfParam.result = 
                                    HPGP_CCO_APPT_CNF_UNAPPT_HO;
                            }
                            else
                            {
                                //not defined in the standard
                                ccoApptCnfParam.result = 
                                    HPGP_CCO_APPT_CNF_REJECT;
                            }
                        }

                        CNAM_SendMgmtMsg(cnam, EVENT_TYPE_CC_CCO_APPOINT_CNF, 
                                         &ccoApptCnfParam);
                        cnam->ccoApptOrigScb = NULL;
                    }
                    //else it is triggered by the auto CCo selection
                }
                break;
            }
#endif
#ifdef KEEP_ALIVE			
			case EVENT_TYPE_TIMER_KEEP_LIVE_IND:
			{
	    		sScb          *scb = NULL;
	           
				if(cnam->state == CNAM_STATE_READY)
				{
					scb = CRM_GetNextScb(cnam->crm, scb);
    				while(scb)
    				{
	    				if(scb->tei != staInfo->tei)
						{
	    					if(scb->hit == 1)
	    					{
								scb->hit = 0;
							}
							else
							{
								if (scb->hitCount < KEEP_ALIVE_HIT_COUNT)
								{
									scb->hitCount++;
								}
								else
								{	
									sLeaveIndParam     leaveIndParam;
									leaveIndParam.reason = HPGP_LEAVE_IND_REASON_RSVD;// Keep Alive is our feature, not defined in spec
					        		leaveIndParam.dstTei = scb->tei;
					        		leaveIndParam.dstMacAddr = scb->macAddr;
					        		STM_StopTimer(scb->teiTimer);

					                    //start the TEI reuser timer
					        		STM_StartTimer(scb->teiTimer, 
					                                   HPGP_TIME_TEI_REUSE);
					        		scb->namState = STA_NAM_STATE_RELEASE;

//									FM_Printf(FM_USER,"kplv\n");
									CNAM_SendMgmtMsg(cnam, EVENT_TYPE_CC_LEAVE_IND, 
					                                           &leaveIndParam);
					                    //update the TEI map
					        		CNAM_SendTeiMapInd(cnam, scb, HPGP_TEI_MAP_MODE_DELETE);

									{	
										hostEvent_peerLeave_t pLeave;

										memcpy(pLeave.macAddress, scb->macAddr, MAC_ADDR_LEN);
										pLeave.tei = scb->tei;
										pLeave.reason = HOST_EVENT_PEER_LEAVE_REASON_PEER_LEAVING;


										Host_SendIndication(HOST_EVENT_PEER_STA_LEAVE, HPGP_MAC_ID, (u8*)&pLeave,
															sizeof(hostEvent_peerLeave_t));

									}

									//FM_Printf(FM_USER,"Entry Removed:%bu\n",scb->tei);//remove entry
								}
								
							}
	    				}
						scb = CRM_GetNextScb(cnam->crm, scb);
					}
				   	STM_StartTimer(cnam->staAgingTimer,HPGP_TIME_KEEP_ALIVE);
				}
				break;
			}
#endif			
            default:
            {
                break;
            }
        }
    }
}

void CNAM_BackupCCoAgeOut(sCnam *cnam, sScb *scb)
{

	if (cnam->backupCCoCfg.scb	== scb)
	{
		scb->staCap.fields.backupCcoStatus = 0;
		scb->staCap.fields.backupCcoCap = 0;
		cnam->backupCCoCfg.scb = NULL;
		
		CNAM_SelectBackupCCo(cnam, scb);
	}
				

}

eStatus CNAM_SelectBackupCCo(sCnam *cnam, sScb *scbIter)
{
	sBackupCCoParam    ccoBackupReq;

	
	scbIter = CRM_GetNextScb(cnam->crm, scbIter);


	while(scbIter)
	{

		if((scbIter->tei != cnam->staInfo->ccoScb->tei) &&
    	   (scbIter->staCap.fields.backupCcoCap))
		{

			ccoBackupReq.dstTei = scbIter->tei;
			ccoBackupReq.dstMacAddr = scbIter->macAddr;
			ccoBackupReq.action = HPGP_CCO_BACKUP_REQ_APPOINT;

			cnam->backupCCoCfg.scb = scbIter;
			
			CNAM_SendMgmtMsg(cnam, EVENT_TYPE_CC_BACKUP_APPOINT_REQ,
							 &ccoBackupReq);

			return STATUS_SUCCESS;
			

		}

		scbIter = CRM_GetNextScb(cnam->crm, scbIter);
	} //end of while

	return STATUS_FAILURE;

	
}

eStatus CNAM_ReleaseBackupCCo()
{
	sBackupCCoParam    ccoBackupReq;
	sLinkLayer *linkl = (sLinkLayer *)HPGPCTRL_GetLayer(HP_LAYER_TYPE_LINK);
	sCnam *cnam = &linkl->ccoNam;
	sScb *scbIter = NULL;
	
	scbIter = CRM_GetNextScb(cnam->crm, scbIter);

	while(scbIter)
	{
		if((scbIter->tei != cnam->staInfo->ccoScb->tei) &&
    	   (scbIter->staCap.fields.backupCcoStatus))
		{

				ccoBackupReq.dstTei = scbIter->tei;
				ccoBackupReq.dstMacAddr = scbIter->macAddr;
				ccoBackupReq.action = HPGP_CCO_BACKUP_REQ_RELEASE;

				cnam->backupCCoCfg.scb = NULL;
				
				CNAM_SendMgmtMsg(cnam, EVENT_TYPE_CC_BACKUP_APPOINT_REQ,
								 &ccoBackupReq);

				return STATUS_SUCCESS;
				
			
		}

		scbIter = CRM_GetNextScb(cnam->crm, scbIter);
	} //end of while

	return STATUS_FAILURE;

	
}



void CNAM_EnableAssocNotification(sCnam *cnam, u8 *macAddr)
{

    sLinkLayer        *linkl = NULL;
    sStaInfo          *staInfo = NULL;


		/*Compiler warning suppression*/
		macAddr = macAddr;
    cnam->accNotification = 1;


#ifdef UKE    
    linkl = cnam->linkl;
    staInfo = LINKL_GetStaInfo(linkl);


    memcpy((u8*)&cnam->ukePeer, macAddr, MAC_ADDR_LEN);
#endif

    
}

void CNAM_Stop(sCnam *cnam)
{
	sEvent *newEvent;
	
	newEvent = EVENT_Alloc(MAC_ADDR_LEN, 
						   EVENT_HPGP_CTRL_HEADROOM);
	if(newEvent == NULL)
	{
		FM_Printf(FM_ERROR, 
				"CNAM:EallocErr\n");
		return;
	}
	
	newEvent->eventHdr.eventClass = EVENT_CLASS_CTRL;
	newEvent->eventHdr.type = EVENT_TYPE_CNAM_STOP_REQ;
	
	newEvent->buffDesc.datalen = 0;
	SLIST_Put(&cnam->linkl->eventQueue, &newEvent->link);

}


void  CNAM_Start(sCnam *cnam, u8 ccoType)
{
    sScb *scbIter = NULL;


    //if(ccoType == LINKL_CCO_TYPE_HO)
    //{
    //}
    if (ccoType == LINKL_CCO_TYPE_UNASSOC)
    {
        cnam->accNotification = 1;
    }
    else
    {
        cnam->accNotification = 0;
        //start the tei timer for each STA due to handover
        //Note: if tei timer starts already (e.g. in when CCo is associated), 
        //it would not take an affect
        scbIter = CRM_GetNextScb(cnam->crm, scbIter);
        while(scbIter)
        {
            if(scbIter->tei != cnam->staInfo->ccoScb->tei)
            {
                STM_StartTimer(scbIter->teiTimer, cnam->teiLeaseTime*60000);
            }
        
            scbIter = CRM_GetNextScb(cnam->crm, scbIter);
        }
    }
#ifdef KEEP_ALIVE
	//FM_Printf(FM_USER,"CNAM STA aging timer started\n");
	STM_StartTimer(cnam->staAgingTimer,HPGP_TIME_KEEP_ALIVE);
#endif
    cnam->state = CNAM_STATE_READY;
}


eStatus CNAM_Init(sCnam *cnam, sLinkLayer *linkl)
{
    cnam->linkl = linkl;

    cnam->staInfo = LINKL_GetStaInfo(linkl);
    cnam->crm = LINKL_GetCrm(linkl);

    cnam->state = CNAM_STATE_INIT;
    cnam->accNotification = 0;
    cnam->teiLeaseTime = HPGP_TIME_TEI_LEASE_NOAUTH_MIN;   //15 minutes
    cnam->teiLeaseTimeAuth = HPGP_TIME_TEI_LEASE_AUTH_MIN; //48 hours

	cnam->backupCCoCfg.scb = NULL;
//    cnam->ccoInfo = LINKL_GetCcoInfo(linkLayer);
#ifdef KEEP_ALIVE
	cnam->staAgingTimer = STM_AllocTimer(HP_LAYER_TYPE_LINK,
							EVENT_TYPE_TIMER_KEEP_LIVE_IND,linkl);
	if(cnam->staAgingTimer == STM_TIMER_INVALID_ID)
	{
		return STATUS_FAILURE;
	}
#endif

    return STATUS_SUCCESS;
}

#endif /* CCO_FUNC */

/* --------------------------
 * STA network access manager
 * -------------------------- */

#ifdef STA_FUNC


#ifdef UKE
void SNAM_EnableAssocNtf(sSnam *snam, u8 *macAddr)
{
    sLinkLayer    *linkl = snam->linkl;
    sStaInfo      *staInfo = LINKL_GetStaInfo(linkl);

    snam->ukePeerNotification = 1;
    
    memcpy(&snam->ukePeer, macAddr, MAC_ADDR_LEN);

}
#endif
eStatus SNAM_SendMgmtMsg(sSnam *snam, u16 mmType)
{
    eStatus        status = STATUS_FAILURE;
    sEvent      xdata  *newEvent = NULL;
    sHpgpHdr      *newHpgpHdr = NULL;
    sCcAssocReq   *assocReq = NULL;
    sCcLeaveReq   *leaveReq = NULL;
    sCcCcoApptReq *ccoApptReq = NULL;
	sCcBackupCnf  *ccoBackupCnf = NULL;
    u16            eventSize = 0;
//    sLinkLayer    *linkl = (sLinkLayer *)HPGPCTRL_GetLayer(HP_LAYER_TYPE_LINK);
    sLinkLayer    *linkl = snam->linkl;
    sStaInfo      *staInfo = LINKL_GetStaInfo(linkl);

    //build the CC_ASSOC.REQ
    eventSize = MAX(HPGP_DATA_PAYLOAD_MIN, sizeof(sMgmtMsg));
    newEvent = EVENT_MgmtAlloc(eventSize, EVENT_HPGP_MSG_HEADROOM);
    if(newEvent == NULL)
    {
        FM_Printf(FM_ERROR, "EAllocErr\n");
        return STATUS_FAILURE;
    }

    newEvent->eventHdr.eventClass = EVENT_CLASS_MSG;
    switch(mmType)
    {
    	case EVENT_TYPE_CC_BACKUP_APPOINT_CNF:
			
			newEvent->eventHdr.type = EVENT_TYPE_CC_BACKUP_APPOINT_CNF;
      
            newHpgpHdr = (sHpgpHdr *)newEvent->buffDesc.buff;
            newHpgpHdr->tei = staInfo->ccoScb->tei;
            newHpgpHdr->macAddr = staInfo->ccoScb->macAddr;
            newHpgpHdr->snid = staInfo->snid;

			newHpgpHdr->eks = staInfo->nekEks;
         
            ccoBackupCnf = (sCcBackupCnf *)(newEvent->buffDesc.dataptr); 
            ccoBackupCnf->result = snam->backupCCoCfg.result; //usre request
            newEvent->buffDesc.datalen += HPGP_DATA_PAYLOAD_MIN;
#ifdef P8051
            FM_Printf(FM_MMSG, "SNAM:>>BACKUP_APPOINT_CNF(tei:%bu)\n",
                               newHpgpHdr->tei);
#else
            FM_Printf(FM_MMSG, "SNAM:>>BACKUP_APPOINT_CNF(tei:%d)\n",
                               newHpgpHdr->tei);
#endif
        	
			break;
			
        case EVENT_TYPE_CC_ASSOC_REQ:
        {
            newEvent->eventHdr.type = EVENT_TYPE_CC_ASSOC_REQ;
         

            newHpgpHdr = (sHpgpHdr *)newEvent->buffDesc.buff;
#ifdef QCA
            newHpgpHdr->tei = 0xFF;
#else
            newHpgpHdr->tei = staInfo->ccoScb->tei;
#endif
            newHpgpHdr->macAddr = staInfo->ccoScb->macAddr;
            newHpgpHdr->snid = staInfo->snid;

            if (snam->teiRenew)
            {
                newHpgpHdr->eks = staInfo->nekEks;
            }
            else
            {
                newHpgpHdr->eks = HPGP_EKS_NONE;
            }
 
            assocReq = (sCcAssocReq *)(newEvent->buffDesc.dataptr); 
            assocReq->reqType = snam->teiRenew; //new request    
            memcpy(assocReq->nid, staInfo->nid, NID_LEN);

			//FM_HexDump(FM_USER,"nidtx", assocReq->nid, NID_LEN);
            assocReq->ccoCap = staInfo->staCap.fields.ccoCap;
            assocReq->proxyNetCap = 0;
            newEvent->buffDesc.datalen +=MAX(HPGP_DATA_PAYLOAD_MIN, sizeof(sCcAssocReq));            
#ifdef LOG_FLASH
            logEvent(MGMT_MSG, 0, EVENT_TYPE_CC_ASSOC_REQ, NULL, 0);
#endif
            FM_Printf(FM_MMSG, "SNAM:>>CC_ASSOC.REQ(tei:%bu)\n",
                               newHpgpHdr->tei);
            break;
        }
        case EVENT_TYPE_CC_LEAVE_REQ:
        {
            newEvent->eventHdr.type = EVENT_TYPE_CC_LEAVE_REQ;

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

            leaveReq = (sCcLeaveReq *)(newEvent->buffDesc.dataptr); 
            leaveReq->reason = snam->leaveReq.reason; //usre request
            newEvent->buffDesc.datalen += HPGP_DATA_PAYLOAD_MIN;
#ifdef LOG_FLASH
            logEvent(MGMT_MSG, 0, EVENT_TYPE_CC_LEAVE_REQ, &newHpgpHdr->tei, 1);
#endif
#ifdef P8051
            FM_Printf(FM_MMSG, "SNAM:>>CC_LEAVE.REQ(tei:%bu)\n",
                               newHpgpHdr->tei);
#else
            FM_Printf(FM_MMSG, "SNAM:>>CC_LEAVE.REQ(tei:%d)\n",
                               newHpgpHdr->tei);
#endif
            break;
        }
#ifdef APPOINT
        case EVENT_TYPE_CC_CCO_APPOINT_REQ:
        {
            newEvent->eventHdr.type = EVENT_TYPE_CC_CCO_APPOINT_REQ;

            newHpgpHdr = (sHpgpHdr *)newEvent->buffDesc.buff;
            newHpgpHdr->tei = staInfo->ccoScb->tei;
            newHpgpHdr->macAddr = staInfo->ccoScb->macAddr;
            newHpgpHdr->snid = staInfo->snid;
			newHpgpHdr->eks = staInfo->nekEks;


            ccoApptReq = (sCcCcoApptReq *)(newEvent->buffDesc.dataptr); 
            memcpy(ccoApptReq, &snam->ccoApptReq, sizeof(sCcCcoApptReq));
            newEvent->buffDesc.datalen +=MAX(HPGP_DATA_PAYLOAD_MIN, sizeof(sCcCcoApptReq));
#ifdef P8051
            FM_Printf(FM_MMSG, "SNAM:>>CCO_APPOINT.REQ(tei:%bu)\n",
                               newHpgpHdr->tei);
#else
            FM_Printf(FM_MMSG, "SNAM:>>CCO_APPOINT.REQ(tei:%d)\n",
                               newHpgpHdr->tei);
#endif
            break;
        }
#endif
#ifdef KEEP_ALIVE				
		case EVENT_TYPE_VENDOR_KEEP_ALIVE_IND:
		{
         
            //FM_Printf(FM_USER, "SNSM:>>CC_VD_KEEP_ALIVE.IND\n");
            newEvent->eventHdr.eventClass = EVENT_CLASS_MSG;
            newEvent->eventHdr.type = EVENT_TYPE_VENDOR_KEEP_ALIVE_IND;

            newHpgpHdr 			= (sHpgpHdr *)newEvent->buffDesc.buff; 
            newHpgpHdr->tei 	= snam->staInfo->ccoScb->tei;
            newHpgpHdr->macAddr = snam->staInfo->ccoScb->macAddr; 
            newHpgpHdr->snid	= snam->staInfo->snid;			
			newHpgpHdr->eks 	= staInfo->nekEks;
			newEvent->buffDesc.datalen += HPGP_DATA_PAYLOAD_MIN;
			break;
		}
#endif		
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


eStatus SNAM_DeliverEvent(sSnam *snam, u16 eventType, uEventBody *eventBody)
{
    sEvent       *newEvent = NULL;
//    sLinkLayer   *linkl = (sLinkLayer *)HPGPCTRL_GetLayer(HP_LAYER_TYPE_LINK);
    sLinkLayer   *linkl = snam->linkl;

    newEvent = EVENT_Alloc(sizeof(uEventBody), 0);
    if(newEvent == NULL)
    {
        FM_Printf(FM_ERROR, "SNAM:EAllocErr\n");
        return STATUS_FAILURE;
    }
    newEvent->eventHdr.eventClass = EVENT_CLASS_CTRL;
    newEvent->eventHdr.type = eventType;
 
    switch(eventType)
    {
        case EVENT_TYPE_NET_LEAVE_IND:
        {
            *(newEvent->buffDesc.dataptr) = eventBody->netLeaveInd.reason;
			newEvent->buffDesc.datalen = 1;
	
            break;
        }
        case EVENT_TYPE_NET_ACC_RSP:
        {
            *(newEvent->buffDesc.dataptr) = eventBody->netAccRsp.result;
			newEvent->buffDesc.datalen = 1;						
				
            break;
        }
        default:
        {
           //event with null event body:
           //EVENT_TYPE_NET_LEAVE_RSP
        }
    }

    //deliver the event to the upper layer
#ifdef CALLBACK
    linkl->deliverEvent(linkl->eventcookie, newEvent);
#else
    CTRLL_ReceiveEvent(linkl->eventcookie, newEvent);
#endif
    
    return STATUS_SUCCESS;
}


//Process CC_SET_TEI_MAP.IND: update tei map table
void SNAM_ProcTeiMapInd(sSnam *snam,  sCcTeiMapInd *teiMapInd)
{
    sTeiMap       *teiMap = NULL;
    u8             numSta;
//    sLinkLayer    *linkl = (sLinkLayer *)HPGPCTRL_GetLayer(HP_LAYER_TYPE_LINK);
    sLinkLayer    *linkl = snam->linkl;
    sCrm          *crm = LINKL_GetCrm(linkl);
	sStaInfo   *staInfo  = LINKL_GetStaInfo(linkl);
    sScb          *scb = NULL;

    switch (teiMapInd->mode)
    {
        case HPGP_TEI_MAP_MODE_NEW: 
        {
            //update entire tei map table
            numSta = teiMapInd->numSta;  
            teiMap = &teiMapInd->teiMap;
            while(numSta)
            {
                scb = CRM_AddScb(crm, teiMap->tei);
                if(scb)
                {
                    memcpy(scb->macAddr, teiMap->macAddr, MAC_ADDR_LEN);
                    scb->staStatus.fields.authStatus = teiMap->status;

					scb->namState = STA_NAM_STATE_CONN;
                    teiMap++;
                    numSta--;
#ifdef ROUTE
                    if(scb != staInfo->staScb)
                    {                        
                        ROUTE_initLrtEntry(scb);                                                
                    }
                    
#endif
                    if(scb != staInfo->staScb)
                    {
#ifdef STA_ID

                        scb->identityCapUpdated = FALSE;
                        scb->identityCapUpdatedRetry = 3;
                        
                        STM_StartTimer(snam->identifyCapTimer,HPGP_IDENTIFY_CAP_TIME);
#else
                        scb->identityCapUpdated = TRUE;
                        scb->identityCapUpdatedRetry = 0;
#endif
                    }
//                    scb->uMaxSSN = 0;
                    scb->uMinSSN = 0;
                    scb->uWrapAround = 0;


					if(scb->teiTimer == STM_TIMER_ID_NULL)
					{
						scb->teiTimer = STM_AllocTimer(HP_LAYER_TYPE_LINK, 
							EVENT_TYPE_TIMER_TEI_IND, scb);
					}


                }
                else
                {
                    FM_Printf(FM_ERROR, "SNAM:can't add scb\n");
                    break;
                }
            }
            break;
        }
        case HPGP_TEI_MAP_MODE_ADD:
        {
            teiMap = &teiMapInd->teiMap;
            scb = CRM_AddScb(crm, teiMap->tei);
            if(scb)
            {
								sHpgpHdr *hpgpHdr;

								/*Compiler warning suppression*/
								hpgpHdr = hpgpHdr;
							
                memcpy(scb->macAddr, teiMap->macAddr, MAC_ADDR_LEN);
                scb->staStatus.fields.authStatus = teiMap->status;
                
#ifdef ROUTE
                ROUTE_initLrtEntry(scb);                
                
#endif
//                scb->uMaxSSN = 0;
                scb->uMinSSN = 0;
                scb->uWrapAround = 0;
                

				scb->namState = STA_NAM_STATE_CONN;
#ifdef STA_ID
                scb->identityCapUpdated = FALSE;
                scb->identityCapUpdatedRetry = 3;
									
                STM_StartTimer(snam->identifyCapTimer,HPGP_IDENTIFY_CAP_TIME);
#else
                scb->identityCapUpdated = TRUE;
                scb->identityCapUpdatedRetry = 0;
#endif

#ifdef UKE 
                if ((snam->ukePeerNotification) &&
                     (!memcmp(&snam->ukePeer, &teiMap->macAddr, MAC_ADDR_LEN )))
                {

                    {
                            sEvent *newEvent;
                            
                            //send the event to the SNAM to renew the TEI
                            newEvent = EVENT_Alloc(MAC_ADDR_LEN, 
                                                   EVENT_HPGP_CTRL_HEADROOM);
                            if(newEvent == NULL)
                            {
                                FM_Printf(FM_ERROR, "SNAM:EAllocErr\n");
                                break;
                            }
                            newEvent->eventHdr.eventClass = EVENT_CLASS_CTRL;
                            newEvent->eventHdr.type = EVENT_TYPE_ASSOC_IND;
                            hpgpHdr = (sHpgpHdr *)newEvent->buffDesc.buff;
                            hpgpHdr->scb = (sScb*)scb;

                            //send the event to the SNAM
                            //LINKL_SendEvent(linkl, newEvent);
                            memcpy(newEvent->buffDesc.dataptr,snam->ukePeer, MAC_ADDR_LEN);
                            newEvent->buffDesc.datalen = MAC_ADDR_LEN;

                            snam->ukePeerNotification = 0;
                            
                            //SLIST_Put(&linkl->intEventQueue, &newEvent->link);
                            LINKL_SendEvent(linkl, newEvent);
                            
                
                        }
                }
#endif				

				if(scb->teiTimer == STM_TIMER_ID_NULL)
				{
					scb->teiTimer = STM_AllocTimer(HP_LAYER_TYPE_LINK, 
											EVENT_TYPE_TIMER_TEI_IND, scb);
				}

            }
            
            break;
        }
        case HPGP_TEI_MAP_MODE_DELETE: 
        {

            //remove it from the TEI MAP
            teiMap = &teiMapInd->teiMap;
            scb = CRM_GetScb(crm, teiMap->tei);
            if(scb)
            {
//                scb->uMaxSSN = 0;
                scb->uMinSSN = 0;
                scb->uWrapAround = 0;
            
#ifdef ROUTE
            ROUTE_preparteAndSendUnreachable(scb);            
#endif  // ROUTE
#ifdef LOG_FLASH
            scbFreeReason = TEI_MAP_DELETE;
#endif			
            CRM_FreeScb(crm, scb);

			{	
				hostEvent_peerLeave_t pLeave;

				memcpy(pLeave.macAddress, scb->macAddr, MAC_ADDR_LEN);
				pLeave.tei = scb->tei;
				pLeave.reason = 0;

					

				Host_SendIndication(HOST_EVENT_PEER_STA_LEAVE, HPGP_MAC_ID, (u8*)&pLeave,
				   					sizeof(hostEvent_peerLeave_t));

			}
#ifdef NAM_PRINT
#ifdef P8051
FM_Printf(FM_ERROR, "SNAM:rm SCB(tei: %bu) TEI map\n", teiMap->tei);
#else
FM_Printf(FM_ERROR, "SNAM:rm SCB(tei: %d) TEI map\n", teiMap->tei);
#endif
#endif
            //TODO: shall we remove it from the discovered sta list right now?
            //Or let it age out later 
            }
            break;
        }
        default:
        {
        }
    }

}

//extern void SNSM_clearBlackList();

//extern void SNSM_blackListCCo(u8 snid);

void SNAM_StartTEIRenew()
{
	sLinkLayer	  *linkl = (sLinkLayer *)HPGPCTRL_GetLayer(HP_LAYER_TYPE_LINK);
	
	sSnam         *snam = &linkl->staNam;
	
	sStaInfo	  *staInfo = LINKL_GetStaInfo(linkl);


	u32 renewTime = (staInfo->macAddr[5] * 40);
	STM_StopTimer(snam->teiTimer);
	STM_StartTimer(snam->teiTimer, renewTime);


}

void SNAM_ProcEvent(sSnam *snam, sEvent *event)
{
    u8             staType;
    u8             txCcoApptRsp;
    sCcAssocCnf   *assocCnf = NULL;
    sCcLeaveInd   *leaveInd = NULL;
    sCcTeiMapInd  *teiMapInd = NULL;
    sCcCcoApptReq *ccoApptReq = NULL;
    sCcCcoApptCnf *ccoApptCnf = NULL;
	sCcBackupReq  *ccoBackupReq;
    sEvent        *newEvent = NULL;
    sHpgpHdr      *hpgpHdr = NULL;
    uEventBody    eventBody;
    u16           eventType;
    sScb          *scb = NULL;
//    sLinkLayer    *linkl = (sLinkLayer *)HPGPCTRL_GetLayer(HP_LAYER_TYPE_LINK);
    sLinkLayer    *linkl = snam->linkl;
    sStaInfo      *staInfo = LINKL_GetStaInfo(linkl);
    sCrm          *crm = LINKL_GetCrm(linkl);
    sNma          *nma = HOMEPLUG_GetNma();
//    sSnsm         *snsm = LINKL_GetSnsm(linkl);
    hpgpHdr = (sHpgpHdr *)event->buffDesc.buff;

		/*Compiler warning suppression*/
		txCcoApptRsp = txCcoApptRsp;
		
    switch(snam->state)
    {
        case SNAM_STATE_INIT:
        {
            if( (event->eventHdr.eventClass == EVENT_CLASS_CTRL)&& 
                (event->eventHdr.type == EVENT_TYPE_SNAM_START) )
            {
                staType = *(event->buffDesc.dataptr);
 #ifdef NAM_PRINT                 
FM_Printf(FM_ERROR, "SNAM:Start(staType: %bu)\n", staType);
#endif
                if(staType == LINKL_STA_TYPE_ASSOC)
                {
#ifdef NAM_PRINT				
FM_Printf(FM_ERROR, "SNAM:Start as ASSOC STA\n");
#endif
                    //handover, role from the CCo to the STA 
                    snam->state = SNAM_STATE_CONN;
#ifdef KEEP_ALIVE			
					STM_StopTimer(snam->keepAlive);
					STM_StartTimer(snam->keepAlive,
	                               HPGP_TIME_KEEP_ALIVE);
#endif	
                }
                else 
                {
                    //for net disc and unassociated STA,
                    //transit to the ready state 
                    //as the upper layer may issue net access request 
                    //when it finds the CCo
                    snam->state = SNAM_STATE_READY;
                }
                //if(staType == LINKL_STA_TYPE_UNASSOC)
            }
            break;
        }
        case SNAM_STATE_READY:
        {
            if( event->eventHdr.eventClass == EVENT_CLASS_CTRL)
            {
                switch(event->eventHdr.type)
                {
                    case EVENT_TYPE_NET_ACC_REQ: 
                    {
//                if(staInfo->staScb == NULL)
//                {
                        snam->teiRenew = 0;
//                }
//                else
//                {
                    //in case of CCo handover changing the CCo to STA 
//                    snam->teiRenew = 1;
//                }
                        SNAM_SendMgmtMsg(snam, EVENT_TYPE_CC_ASSOC_REQ);

                        /* regardless of tx success, do the retransmission */
                        STM_StartTimer(snam->accTimer, HPGP_TIME_ASSOC);
                        snam->txRetryCnt++; 
                        snam->state = SNAM_STATE_WAITFOR_CC_ASSOC_RSP;
                        
                  

                        break;
                    }
                    case EVENT_TYPE_SNAM_STOP: 
                    {
                        LINKL_StopSta(linkl);
                        snam->state = SNAM_STATE_INIT;
                        break;
                    }
                    default:
                    {
                    }
                }
            }
            break;
        }
        case SNAM_STATE_CONN:
        {
            if( event->eventHdr.eventClass == EVENT_CLASS_MSG) 
            {
                switch(event->eventHdr.type)
                {

					case EVENT_TYPE_CC_BACKUP_APPOINT_REQ:
					{
#ifndef LG_WAR
//						FM_Printf(FM_MMSG, "SNAM:<<BACKUP_APPOINT_REQ:(tei:%bu)\n", 
  //                                                  hpgpHdr->tei);

						ccoBackupReq = (sCcBackupReq*)event->buffDesc.dataptr;

						if (ccoBackupReq->action == HPGP_CCO_BACKUP_REQ_APPOINT)
						{

							if (staInfo->staCap.fields.backupCcoCap)
							{
								snam->backupCCoCfg.result = 0;
								staInfo->staCap.fields.backupCcoStatus = 1;


								Host_SendIndication(HOST_EVENT_SELECTED_BACKUP_CCO, HPGP_MAC_ID, NULL, 0);

							}
							else
							{								
								snam->backupCCoCfg.result = 1;								
							}

						}
						else
						if (ccoBackupReq->action == HPGP_CCO_BACKUP_REQ_RELEASE)
						{

							if (staInfo->staCap.fields.backupCcoStatus)
							{
								snam->backupCCoCfg.result = 0;
								staInfo->staCap.fields.backupCcoStatus = 0;

							}
							else
							{								
								snam->backupCCoCfg.result = 1;								
							}

						}

						SNAM_SendMgmtMsg(snam, EVENT_TYPE_CC_BACKUP_APPOINT_CNF);
#endif

						break;
                                


					}
					
                    case EVENT_TYPE_CC_LEAVE_IND:
                    {                      
										
								
                        FM_Printf(FM_MMSG, "SNAM:<<CC_LEAVE_IND(tei:%bu)\n", 
                                                    hpgpHdr->tei);
#ifdef LOG_FLASH
                        logEvent(MGMT_MSG, 0, EVENT_TYPE_CC_LEAVE_IND, &hpgpHdr->tei, 1);
#endif
                        
                        STM_StopTimer(snam->teiTimer);
                        //SNSM_LeaveConn(&linkl->staNsm);
                        SNSM_Stop(&linkl->staNsm);
#ifdef AKM

						AKM_Stop(&linkl->akm);
#endif
                        //CRM_FreeScb(crm, staInfo->staScb);
                        //remove the TEI MAP
                        CRM_Init(crm);
                        staInfo->staScb = NULL;
                        staInfo->ccoScb = NULL;

                        
                        //set the MAC routing table to stop the data path
                      
                        //stop the SHOM

                        leaveInd = (sCcLeaveInd *)(event->buffDesc.dataptr); 
                        eventBody.netLeaveInd.reason = leaveInd->reason;
                        
	   					CHAL_DelayTicks(300000); // give time to cco to STOP beaconing
	   
                        SNAM_DeliverEvent(snam, EVENT_TYPE_NET_LEAVE_IND,
                                          &eventBody);
                        
                        //stay the state    
                        snam->state = SNAM_STATE_READY;
                        break;
                    }
                    case EVENT_TYPE_CC_SET_TEI_MAP_IND:
                    {
#ifdef P8051
                FM_Printf(FM_MMSG, "SNAM:<<SET_TEI_MAP.IND(tei:%bu)\n",
                                            hpgpHdr->tei);
#else
                FM_Printf(FM_MMSG, "SNAM:<<SET_TEI_MAP.IND(tei:%d)\n",
                                            hpgpHdr->tei);
#endif
                        teiMapInd = (sCcTeiMapInd *)event->buffDesc.dataptr; 
                        SNAM_ProcTeiMapInd(snam, teiMapInd);
                        break;
                    }
#ifdef APPOINT
                    case EVENT_TYPE_CC_CCO_APPOINT_CNF:
                    {
#ifdef P8051
           //      FM_Printf(FM_MMSG, "SNAM:<<CCO_APPOINT.CNF(tei:%bu)\n",
             //                               hpgpHdr->tei);
#else
               //  FM_Printf(FM_MMSG, "SNAM:<<CCO_APPOINT.CNF(tei:%d)\n",
                 //                           hpgpHdr->tei);
#endif

                        snam->apptTxRetryCnt = 0; 
                        STM_StopTimer(snam->apptTimer); 
                        STM_FreeTimer(snam->apptTimer); 
                        
                        ccoApptCnf = (sCcCcoApptCnf *)(event->buffDesc.dataptr);
#ifdef NMA
                        /* respond to the NMA */
                        NMA_SendCcoApptCnf(nma, ccoApptCnf->result);
#endif
/*
                        eventBody.ccoApptRsp.result = ccoApptCnf->result;
                        SNAM_DeliverEvent(snam, EVENT_TYPE_CCO_APPOINT_RSP,
                                          &eventBody);
*/ 

                        break;
                    }
#endif
#ifdef STA_ID					
                    case EVENT_TYPE_CM_STA_IDENTIFY_REQ:                    
                    {
                        IDENTIFY_procFrm(EVENT_TYPE_CM_STA_IDENTIFY_REQ, event);
                        break;
                    }
                    case EVENT_TYPE_CM_STA_IDENTIFY_CNF:                    
                    {
                        IDENTIFY_procFrm(EVENT_TYPE_CM_STA_IDENTIFY_CNF, event);
                        break;
                    }
                    case EVENT_TYPE_CM_STA_IDENTIFY_RSP:                    
                    {
                        IDENTIFY_procFrm(EVENT_TYPE_CM_STA_IDENTIFY_RSP, event);
                        break;
                    }
#endif					
                    default:
                    {
                       // FM_Printf(FM_HINFO, "SNAM:%bu\n",event->eventHdr.type);
                    }
                }
            }
            else if( event->eventHdr.eventClass == EVENT_CLASS_CTRL) 
            {
                switch(event->eventHdr.type)
                {
#ifdef APPOINT
                    case EVENT_TYPE_CCO_APPOINT_REQ: // user appointed CCO
                    {
                        snam->apptTxRetryCnt = 0; 
#ifdef CALLBACK
                        snam->apptTimer = STM_AllocTimer(LINKL_TimerHandler,
                                              EVENT_TYPE_TIMER_APPT_IND, linkl);
#else
                        snam->apptTimer = STM_AllocTimer(HP_LAYER_TYPE_LINK,
                                              EVENT_TYPE_TIMER_APPT_IND, linkl);
#endif

                        txCcoApptRsp = 0;
                        if(snam->apptTimer != STM_TIMER_INVALID_ID)
                        {
                            
                            ccoApptReq = (sCcCcoApptReq *)(event->buffDesc.dataptr); 
                            memcpy(&snam->ccoApptReq, ccoApptReq, 
                                   sizeof(sCcCcoApptReq));

                            if(SNAM_SendMgmtMsg(snam, 
                                                EVENT_TYPE_CC_CCO_APPOINT_REQ)
                                == STATUS_SUCCESS)
                            {
                                STM_StartTimer(snam->apptTimer, 
                                               HPGP_TIME_USER_APPT_CCO);
                                snam->apptTxRetryCnt++; 
                                //snam->state = SNAM_STATE_WAITFOR_CC_CCO_APPOINT_CNF;
                                //stay in the same state as it takes a time for rsp?
                            }
                            else
                            {
                                txCcoApptRsp = 1;
                                STM_StopTimer(snam->apptTimer); 
                                STM_FreeTimer(snam->apptTimer); 
                            }
                        }
                        else
                        {
                            txCcoApptRsp = 1;
                        }
                        if(txCcoApptRsp)
                        {
                            //TODO: the result could be various due to
                            //different request type. 
                            //Here we simply use one and fix it later
                            eventBody.ccoApptRsp.result = 
                                HPGP_CCO_APPT_CNF_REJECT;
                            SNAM_DeliverEvent(snam, EVENT_TYPE_CCO_APPOINT_RSP,
                                              &eventBody);
                        }

                        break;
                    }
#endif
/*
                    case EVENT_TYPE_CCO_HO_IND: //switch to the new CCO: renew 
                    {
                        //wait for the central beacon coming first
                        SNSM_EnableCcoDetection(snsm);
                        break; 
                    }
*/
                    case EVENT_TYPE_TIMER_TEI_IND: //tei lease expires: renew 
                    case EVENT_TYPE_CCO_DISC_IND:  //handover to new CCO: renew
                    {
                        snam->teiRenew = 1;
                        if(SNAM_SendMgmtMsg(snam, EVENT_TYPE_CC_ASSOC_REQ)
                                == STATUS_SUCCESS)
                        {
                            STM_StartTimer(snam->accTimer, HPGP_TIME_ASSOC);
                            snam->txRetryCnt++; 
                            snam->state = SNAM_STATE_WAITFOR_CC_ASSOC_RSP;
                        }
                        break;
                    }
                    case EVENT_TYPE_NET_LEAVE_REQ:
                    {
#ifdef AKM						
						 AKM_Stop(&linkl->akm);
#endif
						 linkl->staNsm.enableBcnLossDetection = 0;
#ifdef POWERSAVE
	                	scb = (sScb *)(hpgpHdr->scb);
						if (scb->psState != PSM_PS_STATE_OFF)
						{
							// send PS_EXIT.REQ
							PSM_set_sta_PS(FALSE, 0xF);
							PSM_resetScbPs(scb);	// set SCB's PS data to init state 
						}
#endif
                        snam->leaveReq.reason = HPGP_LEAVE_REQ_REASON_USER_REQ;
#ifndef MULTIDEV
                        if(SNAM_SendMgmtMsg(snam, EVENT_TYPE_CC_LEAVE_REQ) 
                                == STATUS_SUCCESS)
#endif								
                        {
#ifndef MULTIDEV
                            STM_StartTimer(snam->accTimer, HPGP_TIME_ASSOC);
                            snam->txRetryCnt++; 
                            snam->state = SNAM_STATE_WAITFOR_CC_LEAVE_RSP;
#else
                       snam->state = SNAM_STATE_READY;
                            {
                                STM_StopTimer(snam->teiTimer);
#ifdef HOM
								SHOM_Stop(&linkl->staHom);
#endif
                                SNAM_DeliverEvent(snam, EVENT_TYPE_NET_LEAVE_RSP, NULL);
                                snam->stopDataPath = 0;
                            }
#endif
                        }
                        break;
                    }
#ifdef APPOINT
                    case EVENT_TYPE_TIMER_APPT_IND:
                    {
                        if( snam->apptTxRetryCnt <= HPGP_TX_RETRY_MAX)
                        { 
                            //resend the message
                            SNAM_SendMgmtMsg( snam, 
                                              EVENT_TYPE_CC_CCO_APPOINT_REQ);

                            STM_StartTimer(snam->apptTimer, 
                                           HPGP_TIME_USER_APPT_CCO);
                            snam->apptTxRetryCnt++; 
                            //stay in the same CONN state
                        }
                        else
                        {
                            //retry exhausted
                            snam->apptTxRetryCnt = 0; //reset
                            STM_FreeTimer(snam->apptTimer); 
                            //stay in the same CONN state
                        }
                        break;
                    }
#endif
                    case EVENT_TYPE_SNAM_STOP: 
                    {
                        //stop the SNAM Tei lease timer
                        LINKL_StopSta(linkl);
                        STM_StopTimer(snam->teiTimer);
                        snam->state = SNAM_STATE_INIT;
                        break; 
                    }
                    case EVENT_TYPE_AUTH_CPLT:
                    {   
#ifdef STA_ID
                          IDENTIFY_sendFrm(EVENT_TYPE_CM_STA_IDENTIFY_IND, NULL, staInfo->ccoScb);
#endif
#ifdef ROUTE
                          if(staInfo->identifyCaps.routingCap == TRUE)
                          {
                              ROUTE_startUpdateTimer();
                          }
#endif
                          break;
                    }
#ifdef STA_ID

                    case EVENT_TYPE_IDENTIFY_CAP_TIMEOUT:
					{
                        scb = CRM_GetNextScb(&linkl->ccoRm, scb);
                        while(scb)
                        {
                            if(scb->identityCapUpdated == FALSE)
                            {
                                if( scb->identityCapUpdatedRetry)
                                {
                                    IDENTIFY_sendFrm(EVENT_TYPE_CM_STA_IDENTIFY_REQ, NULL, scb);
                                    scb->identityCapUpdatedRetry--;
                                }
                                else
                                {
                                    scb->identityCapUpdated = TRUE;
                                }
                                break;
                            }
                            scb = CRM_GetNextScb(&linkl->ccoRm, scb);
                        }
                        if(scb != NULL)
                        {
                            STM_StartTimer(snam->identifyCapTimer,HPGP_IDENTIFY_CAP_TIME);
                        }
                        break;
					}
#endif					
#ifdef KEEP_ALIVE										
					case EVENT_TYPE_TIMER_KEEP_LIVE_IND:
					{
						SNAM_SendMgmtMsg(snam,EVENT_TYPE_VENDOR_KEEP_ALIVE_IND);
						STM_StartTimer(snam->keepAlive,HPGP_TIME_KEEP_ALIVE);
						break;
					}
#endif
                    default:
                    {
                    }
                }
            }
            break;
        }
        case SNAM_STATE_WAITFOR_CC_ASSOC_RSP:
        {
            if( event->eventHdr.eventClass == EVENT_CLASS_MSG) 
            {
                switch(event->eventHdr.type)
                {
                    case EVENT_TYPE_CC_ASSOC_CNF:
                    {
#ifdef LOG_FLASH
                        u8 buff[2];
#endif
             		    FM_Printf(FM_MMSG, "SNAM:<<CC_ASSOC.CNF(tei:%bu)\n", 
                                            hpgpHdr->tei);
                        //process the CC_ASSOC.CNF
                        assocCnf = (sCcAssocCnf *)event->buffDesc.dataptr; 
#ifdef LOG_FLASH
                        buff[0] = assocCnf->result;
                        buff[1] = assocCnf->staTei;
                        logEvent(MGMT_MSG, 0, EVENT_TYPE_CC_ASSOC_CNF, buff, 2);
#endif
                        if(assocCnf->result == 0)
                        {
                            //success
                            //This is the first time that we know 
                            //CCo MAC address unless we send a query to the CCo
                            //So, save it
#ifdef NAM_PRINT							
#ifdef P8051
    FM_Printf(FM_HINFO,"cco MAC:%bx:%bx:%bx:%bx:%bx:%bx\n",
#else
    FM_Printf(FM_HINFO,"cco MAC:%.2x:%.2x:%.2x:%.2x:%.2x:%.2x\n",
#endif

              staInfo->ccoScb->macAddr[0], staInfo->ccoScb->macAddr[1],
              staInfo->ccoScb->macAddr[2], staInfo->ccoScb->macAddr[3],
              staInfo->ccoScb->macAddr[4], staInfo->ccoScb->macAddr[5]);

#endif

                            memcpy( staInfo->ccoScb->macAddr, 
                                    hpgpHdr->macAddr, MAC_ADDR_LEN);

                            snam->txRetryCnt = 0; //reset
                            STM_StopTimer(snam->accTimer);
                            staInfo->tei = assocCnf->staTei;
                            staInfo->snid = assocCnf->snid;
							scb = CRM_AddScb(crm, assocCnf->staTei);
                            if(scb)
                            {
                                //it is done for renew too in case of CCo HO
                                memcpy(scb->macAddr, staInfo->macAddr,
                                       MAC_ADDR_LEN);
                                scb->staStatus.byte = staInfo->staStatus.byte;
                                scb->staCap.byte = staInfo->staCap.byte;
#ifdef POWERSAVE
                            	if(!snam->teiRenew) 
								{
									// first time doing the Assoc.Req, init all variables
									PSM_resetScbPs(scb);	// set SCB's PS data to init state 
								}
#endif

                                staInfo->staScb = scb;  
                                scb->identityCapUpdated = TRUE;
                                scb->identityCapUpdatedRetry = 3;

                                HAL_SetTei(HOMEPLUG_GetHal(), scb->tei);
                            }
                            else
                            {
                                break;  //should not happen
                            }

                            snam->state = SNAM_STATE_CONN;
							//SNSM_clearBlackList();

                            //start/restart the lease timer. 
                            //5 minutes earlier than 
                            //the assigned lease time expiration 
                            STM_StopTimer(snam->teiTimer); 

						    if(!snam->teiRenew) 
                            {
                            	
								hostEvent_assocInd assocInd;
														  

								memcpy(&assocInd.ccoAddress,
									   &staInfo->ccoScb->macAddr, MAC_ADDR_LEN);
								memcpy(&assocInd.nid, &staInfo->nid, NID_LEN);
								assocInd.tei = assocCnf->staTei;

                                staInfo->ccoScb->uMinSSN = 0;
                                staInfo->ccoScb->uWrapAround = 0;
#ifndef AKM								
								
								staInfo->staStatus.fields.authStatus = 1; 
#endif								

								Host_SendIndication(HOST_EVENT_TYPE_ASSOC_IND, HPGP_MAC_ID,
													(u8*)&assocInd, sizeof(assocInd));
									
                                //this is the first CC_ASSOC.REQ
                                //success
                                //TODO: should place the event in the
                                //internal queue, instead of calling 
                                
                                SNSM_Start(&linkl->staNsm, 
                                           LINKL_STA_TYPE_ASSOC);
#ifdef KEEP_ALIVE			
								STM_StopTimer(snam->keepAlive);
								STM_StartTimer(snam->keepAlive,
											HPGP_TIME_KEEP_ALIVE);
#endif	

                                //set MAC rounting table to start the traffic

                                /* start the SHOM */
#ifdef HOM
									SHOM_Start(&linkl->staHom);
#endif
              
                                /* start the AKM */
// UKE                                AKM_Start(&linkl->akm, LINKL_STA_MODE_STA, 0);    
   
 
                                //deliver a successful response to the upper 
                                //layer. may trigger auth process
                                eventBody.netAccRsp.result = 0;
                                SNAM_DeliverEvent(snam, EVENT_TYPE_NET_ACC_RSP,
                                          &eventBody);
#ifdef ROUTE
                                if(scb)
                                {
                                    ROUTE_initLrtEntry(scb);
                                    scb->lrtEntry.nTei = scb->tei;                                    
                                    scb->lrtEntry.rnh = 0;                                    
                                }                               
                                
#endif
#ifndef AKM
	 			            	SNAM_StartTEIRenew();
#endif

                            }
							else
							{
								STM_StartTimer(snam->teiTimer, 
											(assocCnf->leaseTime)*60000 - 5000 );
#ifdef NAM_PRINT										  
#ifdef P8051
								FM_Printf(FM_HINFO, "SNAM:start the tei lease timer(0x%lx)\n",
#else
								FM_Printf(FM_HINFO, "SNAM:start the tei lease timer(0x%.8x)\n",
#endif

								(assocCnf->leaseTime)*60000 - 5000);
#endif
								Host_SendIndication(HOST_EVENT_TEI_RENEWED, HPGP_MAC_ID, (u8*)&staInfo->tei, 1);
							}
                            //else, it is renew, we do not do anything
                            snam->teiRenew = 0;  //reset
                        }
                        else
                        {
                        	
							//SNSM_blackListCCo(assocCnf->snid);								
                        }
                        //for all other cases, stay in the same state and
                        //retry when the access timer expires.
                     
                        break;
                    }
                    default:
                    {
                    }
                }
            }
            else  //control event
            {
                switch(event->eventHdr.type)
                {
                    case EVENT_TYPE_TIMER_ACC_IND:
                    {
                        if( snam->txRetryCnt <= HPGP_TX_RETRY_MAX)
                        { 
                            //resend the message
                            SNAM_SendMgmtMsg(snam, EVENT_TYPE_CC_ASSOC_REQ);
                            STM_StartTimer(snam->accTimer, HPGP_TIME_ASSOC);
                            snam->txRetryCnt++; 
                            //stay in the same state
                            //snam->state = SNAM_STATE_WAITFOR_CC_ASSOC_RSP;
                        }
                        else
                        {
                            //retry exhausted
                            snam->txRetryCnt = 0; //reset
                            //newEvent = EVENT_Alloc(sizeof(sNetAccRspEvent), 0);
                           // if(newEvent == NULL)
                           // {
                           //     FM_Printf(FM_ERROR, "EAllocErr\n");
                           //     break;
                           // }
                            if(!snam->teiRenew) 
                            {
                                //build Event_Net_ACC_RSP (failure)
                                eventType = EVENT_TYPE_NET_ACC_RSP;
                                eventBody.netAccRsp.result = 1;
								
								//SNSM_blackListCCo(staInfo->snid);
                            }
                            else
                            {
                                snam->teiRenew = 0;
								
								//SNSM_blackListCCo(staInfo->snid);

                                SNSM_Stop(&linkl->staNsm);

                                LINKL_StopSta(linkl);
                                STM_StopTimer(snam->teiTimer);
                                snam->state = SNAM_STATE_INIT;

                                //remove the TEI MAP
                                CRM_Init(crm);
                                staInfo->staScb = NULL;
                                staInfo->ccoScb = NULL;
                        
                                //build Event_Net_LEAVE_IND
                                eventType = EVENT_TYPE_NET_LEAVE_IND;
                                eventBody.netLeaveInd.reason = 1;
                            }
                            //deliver the event to the upper layer
                            SNAM_DeliverEvent(snam, eventType,
                                              &eventBody);

                            //change the state    
                            snam->state = SNAM_STATE_READY;
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
        case SNAM_STATE_WAITFOR_CC_LEAVE_RSP:
        {
            if( event->eventHdr.eventClass == EVENT_CLASS_MSG) 
            {
                switch(event->eventHdr.type)
                {
                    case EVENT_TYPE_CC_LEAVE_CNF:
                    {
#ifdef P8051
                FM_Printf(FM_MMSG, "SNAM:<<CC_LEAVE.CNF(tei:%bu)\n", 
                                            hpgpHdr->tei);
#else
                FM_Printf(FM_MMSG, "SNAM:<<CC_LEAVE.CNF(tei:%d)\n", 
                                            hpgpHdr->tei);
#endif
#ifdef LOG_FLASH
                        logEvent(MGMT_MSG, 0, EVENT_TYPE_CC_LEAVE_CNF, &hpgpHdr->tei, 1);
#endif
                        //message payload for CC_LEAVE.CNF is null

                        STM_StopTimer(snam->accTimer);
                        //see below for other operatins taken 
                        //when the STA leaves the network
                        snam->stopDataPath = 1;

                        //STM_StopTimer(snam->teiTimer);
                        //CRM_FreeScb(crm, staInfo->staScb);
                        //staInfo->staScb = NULL;
                        //SNSM_Stop(&linkl->staNsm);
                     

                        //change the state    
                        snam->state = SNAM_STATE_READY;

                    //    SNAM_DeliverEvent(snam, EVENT_TYPE_NET_LEAVE_RSP,
                      //                    NULL);
                                              
                        break;
                    }
                    default:
                    {
                    }
                }
            }
            else  //control event
            {
                switch(event->eventHdr.type)
                {
                    case EVENT_TYPE_TIMER_ACC_IND:
                    {
                        if( snam->txRetryCnt <= HPGP_TX_RETRY_MAX)
                        { 
                            //resend the message
                            SNAM_SendMgmtMsg(snam, EVENT_TYPE_CC_LEAVE_REQ);
                            STM_StartTimer(snam->accTimer, HPGP_TIME_ASSOC);
                            snam->txRetryCnt++; 
                            //stay in the same state
                            //snam->state = SNAM_STATE_WAITFOR_CC_LEAVE_RSP;
                        }
                        else
                        {
                            //leave request retry exhausted, leave anyway
                            snam->stopDataPath = 1;

                            //STM_StopTimer(snam->teiTimer);
                        
                            //change the state    
                            snam->state = SNAM_STATE_READY;
                        }
                        break;
                    }
                    default:
                    {
                    }
                }
            }

            if(snam->stopDataPath)
            {

                STM_StopTimer(snam->teiTimer);

//                SNSM_Stop(&linkl->staNsm);
#ifdef HOM

                //stop the SHOM
                SHOM_Stop(&linkl->staHom);
#endif
        //set the MAC routing table to stop the data path

//				FM_Printf(FM_ERROR,"nam lrsp\n");

                SNAM_DeliverEvent(snam, EVENT_TYPE_NET_LEAVE_RSP, NULL);
                        
                snam->stopDataPath = 0;
            }
            //process events
            break;
        }
/*
        case SNAM_STATE_WAITFOR_AUTH_RSP:
        {
            //process events
            if( event->eventHdr.eventClass == EVENT_CLASS_CTRL) 
            {
                switch(event->eventHdr.type)
                {
                    case EVENT_TYPE_AUTH_RSP:
                    {
                         result = *(newEvent->buffDesc.dataptr); 
                    }
                    default:
                    {
                    }
                  
                }
            }
            break;
        }
*/
        default:
        {
            //perform no operation
        }
    }
}



/*
void  SNAM_PerformHoSwitch(sSnam *snam)
{
    //stop the SNAM Tei lease timer
    STM_StopTimer(snam->teiTimer);
}
*/

eStatus SNAM_Stop(sSnam *snam)
{
    sEvent *newEvent = NULL;
//    sLinkLayer *linkl = (sLinkLayer *)HPGPCTRL_GetLayer(HP_LAYER_TYPE_LINK);
    sLinkLayer    *linkl = snam->linkl;
#if 0
    newEvent = EVENT_Alloc(0, EVENT_HPGP_CTRL_HEADROOM);
    if(newEvent == NULL)
    {
        FM_Printf(FM_ERROR, "SNSM: Cannot allocate an event.\n");
        return STATUS_FAILURE;
    }

    newEvent->eventHdr.eventClass = EVENT_CLASS_CTRL;
    newEvent->eventHdr.type = EVENT_TYPE_SNAM_STOP;
    //LINKL_SendEvent(linkl, newEvent);
    SLIST_Put(&linkl->EventQueue, &newEvent->link);
#else

    STM_StopTimer(snam->teiTimer);
	STM_StopTimer(snam->accTimer);


#ifdef ROUTE
    ROUTE_stopUpdateTimer();
#endif
#ifdef KEEP_ALIVE			
	STM_StopTimer(snam->keepAlive);
#endif

    snam->state = SNAM_STATE_INIT;
	snam->txRetryCnt = 0;
	snam->teiRenew = 0;

#endif
    return STATUS_SUCCESS;

}

eStatus SNAM_Start(sSnam *snam, u8 staType)
{
    sEvent *newEvent = NULL;
    sLinkLayer    *linkl = snam->linkl;
//    sLinkLayer *linkl = (sLinkLayer *)HPGPCTRL_GetLayer(HP_LAYER_TYPE_LINK);

    newEvent = EVENT_Alloc(sizeof(sSnamStartEvent), EVENT_HPGP_CTRL_HEADROOM);
    if(newEvent == NULL)
    {
        FM_Printf(FM_ERROR, "SNSM: EAllocErr\n");
        return STATUS_FAILURE;
    }

    newEvent->eventHdr.eventClass = EVENT_CLASS_CTRL;
    newEvent->eventHdr.type = EVENT_TYPE_SNAM_START;
    *(newEvent->buffDesc.dataptr) = staType;
    newEvent->buffDesc.datalen += sizeof(sSnamStartEvent); 
    LINKL_SendEvent(linkl, newEvent);
    return STATUS_SUCCESS;
}




eStatus SNAM_Init(sSnam *snam, sLinkLayer *linkl)
{
    snam->linkl = linkl;
    snam->staInfo = LINKL_GetStaInfo(linkl);

    snam->state = SNAM_STATE_INIT;

#ifdef CALLBACK
    snam->accTimer = STM_AllocTimer(LINKL_TimerHandler, 
                         EVENT_TYPE_TIMER_ACC_IND, linkl);
#else
    snam->accTimer = STM_AllocTimer(HP_LAYER_TYPE_LINK, 
                         EVENT_TYPE_TIMER_ACC_IND, linkl);
#endif
    if(snam->accTimer == STM_TIMER_INVALID_ID)
    {
        return STATUS_FAILURE;
    }
#ifdef NAM_PRINT	
#ifdef P8051
FM_Printf(FM_ERROR, "SNSM:acc timer id:%bu\n", snam->accTimer);
#else
FM_Printf(FM_ERROR, "SNSM:acc timer id:%d\n", snam->accTimer);
#endif
#endif
#ifdef CALLBACK
    snam->teiTimer = STM_AllocTimer(LINKL_TimerHandler, 
                         EVENT_TYPE_TIMER_TEI_IND, NULL);
#else
    snam->teiTimer = STM_AllocTimer(HP_LAYER_TYPE_LINK, 
                         EVENT_TYPE_TIMER_TEI_IND, NULL);
#endif

    if(snam->teiTimer == STM_TIMER_INVALID_ID)
    {
        return STATUS_FAILURE;
    }
#ifdef NAM_PRINT	
#ifdef P8051
FM_Printf(FM_ERROR, "SNSM:tei lease timer id:%bu\n", snam->teiTimer);
#else
FM_Printf(FM_ERROR, "SNSM:tei lease timer id:%d\n", snam->teiTimer);
#endif
#endif

    snam->apptTimer = STM_TIMER_INVALID_ID;

    snam->txRetryCnt = 0;
    snam->teiRenew = 0;
    snam->stopDataPath = 0;

//    snam->ccoInfo = LINKL_GetCcoInfo(linkLayer);


//    snam->state = SNAM_STATE_READY;

#ifdef STA_ID    
    snam->identifyCapTimer = STM_AllocTimer(HP_LAYER_TYPE_LINK,
                               EVENT_TYPE_IDENTIFY_CAP_TIMEOUT, linkl);
    if(snam->identifyCapTimer == STM_TIMER_INVALID_ID)
    {
        return STATUS_FAILURE;
    }
#endif	
#ifdef KEEP_ALIVE
	snam->keepAlive = STM_AllocTimer(HP_LAYER_TYPE_LINK,EVENT_TYPE_TIMER_KEEP_LIVE_IND,linkl);
	if(snam->keepAlive == STM_TIMER_INVALID_ID)
    {
        return STATUS_FAILURE;
    }
#endif

    return STATUS_SUCCESS;
}


#endif /* STA_FUNC */

#ifdef STA_ID

eStatus IDENTIFY_sendFrm(u16 mmType, sEvent *reqEvent, sScb *scb)
{
    sStaIdentifyCaps *idCaps;
    eStatus     status = STATUS_FAILURE;
    sEvent    xdata  *event = NULL;      
    sLinkLayer  *linkl = (sLinkLayer *)HPGPCTRL_GetLayer(HP_LAYER_TYPE_LINK);
    sStaInfo    *staInfo = LINKL_GetStaInfo(linkl);
    u16         eventSize = (sizeof(sStaIdentifyCaps));	
	sCrm		*crm = LINKL_GetCrm(linkl);
    sHpgpHdr    *reqHpgpHdr;
	sHpgpHdr    *hpgpHdr;
    u8 addIdentify = 0;
    sScb *lscb = NULL;
    
    event = EVENT_MgmtAlloc(eventSize, EVENT_HPGP_MSG_HEADROOM );
    if(event == NULL)
    {
        FM_Printf(FM_ERROR, "EAllocErr\n");
        return STATUS_FAILURE;
    }

    
    // prepare event    
    event->eventHdr.eventClass = EVENT_CLASS_MSG;

    
    hpgpHdr = (sHpgpHdr *)event->buffDesc.buff;
    hpgpHdr->snid = staInfo->snid;
    hpgpHdr->eks = staInfo->nekEks;
    
    switch(mmType)
    {
        case EVENT_TYPE_CM_STA_IDENTIFY_IND:
            event->eventHdr.type = EVENT_TYPE_CM_STA_IDENTIFY_IND;
            hpgpHdr->tei = scb->tei;
            hpgpHdr->macAddr = scb->macAddr;
            addIdentify = 1;
            FM_Printf(FM_MMSG, "IDENTIFY:>>STA_IDENTIFY.IND(tei:%bu)\n",
                               hpgpHdr->tei);
            break;
        case EVENT_TYPE_CM_STA_IDENTIFY_RSP:
            reqHpgpHdr = (sHpgpHdr *)reqEvent->buffDesc.buff;
            event->eventHdr.type = EVENT_TYPE_CM_STA_IDENTIFY_RSP;
            hpgpHdr->tei = reqHpgpHdr->tei;
            // find mac addr
            scb = CRM_GetScb(crm, reqHpgpHdr->tei);
            if(scb)
            {
                hpgpHdr->macAddr = scb->macAddr;
            }
            else
            {
                EVENT_Free(event);
                return status;
            }
//            FM_Printf(FM_MMSG, "IDENTIFY:>>IDENTIFY.RSP(tei:%bu)\n//",
  //                                     hpgpHdr->tei);
            break;
        case EVENT_TYPE_CM_STA_IDENTIFY_REQ:
            event->eventHdr.type = EVENT_TYPE_CM_STA_IDENTIFY_REQ;
            if(scb == staInfo->ccoScb) 
            {
                hpgpHdr->tei = scb->tei;
                hpgpHdr->macAddr = scb->macAddr;
            }
#ifdef ROUTE
            else if(scb->tei != scb->lrtEntry.nTei)
            {

                lscb = CRM_GetScb(crm, scb->lrtEntry.nTei);
                if(lscb)
                {                
                    hpgpHdr->tei = lscb->tei;
                    hpgpHdr->macAddr = scb->macAddr;
                }
                else
                {
                    EVENT_Free(event);
                    return status;
                }
            }
#else
            else 
            {
                hpgpHdr->tei = scb->tei;
                hpgpHdr->macAddr = scb->macAddr;
            }
#endif
            
           // FM_Printf(FM_MMSG, "IDENTIFY:>>STA_IDENTIFY.REQ(tei:%bu)\n",
             //                  hpgpHdr->tei);
            break;
        case EVENT_TYPE_CM_STA_IDENTIFY_CNF:  
            reqHpgpHdr = (sHpgpHdr *)reqEvent->buffDesc.buff;
            event->eventHdr.type = EVENT_TYPE_CM_STA_IDENTIFY_CNF;
            scb = CRM_GetScb(crm, reqHpgpHdr->tei);
            // find mac addr
            if(scb)
            {                
                hpgpHdr->tei = scb->tei;
                hpgpHdr->macAddr = reqHpgpHdr->macAddr;
            }
            else
            {
                EVENT_Free(event);
                return status;
            }
            addIdentify = 1;
            FM_Printf(FM_MMSG, "IDENTIFY:>>STA_IDENTIFY.CNF(tei:%bu)\n",
                                       hpgpHdr->tei);
            break;
        default:
            FM_Printf(FM_MMSG, "IDENTIFY:>>Invalid Msg\n");


    }
    if(addIdentify)
    {
        idCaps = (sStaIdentifyCaps*)event->buffDesc.dataptr;

        idCaps->efl = staInfo->identifyCaps.efl;
        idCaps->HPAVVer = staInfo->identifyCaps.HPAVVer;
        idCaps->routingCap = staInfo->identifyCaps.routingCap;
        idCaps->powerSaveCap = staInfo->identifyCaps.powerSaveCap;
        idCaps->greenPHYCap = staInfo->identifyCaps.greenPHYCap;

        event->buffDesc.datalen = (sizeof(sStaIdentifyCaps));
    }
    else
    {
        
        event->buffDesc.datalen = 0;
    }
    status = MUXL_TransmitMgmtMsg(event);
    //the event is freed by MUXL if the TX is successful
    if(status == STATUS_FAILURE)
    {
        EVENT_Free(event);
    }

    return status;


}

eStatus IDENTIFY_procFrm(u16 mmType, sEvent *reqEvent)
{
    sStaIdentifyCaps *idCaps;
    sHpgpHdr    *hpgpHdr;
    sScb *scb = NULL;
    sLinkLayer  *linkl = (sLinkLayer *)HPGPCTRL_GetLayer(HP_LAYER_TYPE_LINK);
    sStaInfo    *staInfo = LINKL_GetStaInfo(linkl);
	sCrm		*crm = LINKL_GetCrm(linkl);
    eStatus     status = STATUS_FAILURE;

    hpgpHdr = (sHpgpHdr *)reqEvent->buffDesc.buff;
    switch(mmType)
    {
            break;
        case EVENT_TYPE_CM_STA_IDENTIFY_RSP:
            FM_Printf(FM_MMSG, "IDENTIFY:<<STA_IDENTIFY.RSP(tei:%bu)\n",
                                       hpgpHdr->tei);
            break;
        case EVENT_TYPE_CM_STA_IDENTIFY_REQ:
            
            FM_Printf(FM_MMSG, "IDENTIFY:<<STA_IDENTIFY.REQ(tei:%bu)\n",
                               hpgpHdr->tei);
            IDENTIFY_sendFrm(EVENT_TYPE_CM_STA_IDENTIFY_CNF, reqEvent, NULL);           
            break;
            
        case EVENT_TYPE_CM_STA_IDENTIFY_IND:            
        case EVENT_TYPE_CM_STA_IDENTIFY_CNF:  
            hpgpHdr = (sHpgpHdr *)reqEvent->buffDesc.buff;
            idCaps = (sStaIdentifyCaps*)reqEvent->buffDesc.dataptr;
            // find mac addr
            scb = CRM_FindScbMacAddr(hpgpHdr->macAddr);
            if(scb)
            {
                scb->idCaps.efl = idCaps->efl;
                scb->idCaps.HPAVVer = idCaps->HPAVVer;
                scb->idCaps.routingCap = idCaps->routingCap;
                scb->idCaps.powerSaveCap = idCaps->powerSaveCap;
                scb->idCaps.greenPHYCap = idCaps->greenPHYCap;
                scb->identityCapUpdated = TRUE;
            }
            else
            {
                return status;
            }
            if(mmType == EVENT_TYPE_CM_STA_IDENTIFY_CNF)
            {
                
                FM_Printf(FM_MMSG, "IDENTIFY:<<STA_IDENTIFY.CNF(tei:%bu)\n",
                                   hpgpHdr->tei);
#ifdef ROUTE
                if(scb->idCaps.routingCap == TRUE)
                {
                    ROUTE_sendRouteInfoReq(scb);
                }
#endif
            }
            else
            {
                
                FM_Printf(FM_MMSG, "IDENTIFY:<<STA_IDENTIFY.IND(tei:%bu)\n",
                           hpgpHdr->tei);
                IDENTIFY_sendFrm(EVENT_TYPE_CM_STA_IDENTIFY_RSP, reqEvent, scb);
            }
            break;
        default:
            FM_Printf(FM_MMSG, "IDENTIFY:>>Invalid Msg\n");
    }
    return STATUS_SUCCESS;

}

#endif

/** =========================================================
 *
 * Edit History
 *
 * $Source: /home/cvsrepo/Hybrii_B_OSFW_Dev/firmware/hpgp/src/link/nam.c,v $
 *
 * $Log: nam.c,v $
 * Revision 1.31  2015/01/02 14:55:36  kiran
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
 * Revision 1.30  2014/12/09 07:09:08  ranjan
 * - multicco feature under MCCO flag
 *
 * Revision 1.29  2014/11/11 14:52:58  ranjan
 * 1.New Folder Architecture espically in /components
 * 2.Modular arrangment of functionality in new files
 *    anticipating the need for exposing them as FW App
 *    development modules
 * 3.Other improvisation in code and .h files
 *
 * Revision 1.28  2014/10/28 16:27:43  kiran
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
 * Revision 1.27  2014/10/15 10:42:51  ranjan
 * small fixes in um
 *
 * Revision 1.26  2014/09/05 09:28:18  ranjan
 * 1. uppermac cco-sta switching feature fix
 * 2. general stability fixes for many station associtions
 * 3. changed mgmt memory pool for many STA support
 *
 * Revision 1.25  2014/08/25 07:37:34  kiran
 * 1) RSSI & LQI support
 * 2) Fixed Sync related issues
 * 3) Fixed timer 0 timing drift for SDK
 * 4) MMSG & Error Logging in Flash
 *
 * Revision 1.24  2014/08/12 08:45:43  kiran
 * 1) Event fixes
 * 2) API to change UART line control parameters
 *
 * Revision 1.23  2014/07/30 12:26:26  kiran
 * 1) Software Recovery for CCo
 * 2) User appointed CCo support in SDK
 * 3) Association process performance fixes
 * 4) SSN related fixes
 *
 * Revision 1.22  2014/07/22 10:03:52  kiran
 * 1) SDK Supports Power Save
 * 2) Uart_Driver.c cleanup
 * 3) SDK app memory pool optimization
 * 4) Prints from STM.c are commented
 * 5) Print messages are trimmed as common no memory left in common
 * 6) Prins related to Power save and some other modules are in compilation flags. To enable module specific prints please refer respective module
 *
 * Revision 1.21  2014/07/05 09:16:27  prashant
 * 100 Devices support- only association tested, memory adjustments
 *
 * Revision 1.20  2014/06/26 17:59:42  ranjan
 * -fixes to make uppermac more robust for n/w change
 *
 * Revision 1.19  2014/06/24 16:26:45  ranjan
 * -zigbee frame_handledata fix.
 * -added reason code for uppermac host events
 * -small cleanups
 *
 * Revision 1.18  2014/06/23 06:56:44  prashant
 * Ssn reset fix upon device reset, Duplicate SNID fix
 *
 * Revision 1.17  2014/06/19 17:13:19  ranjan
 * -uppermac fixes for lvnet and reset command for cco and sta mode
 * -backup cco working
 *
 * Revision 1.16  2014/06/12 13:15:43  ranjan
 * -separated bcn,mgmt,um event pools
 * -fixed datapath issue due to previous checkin
 * -work in progress. neighbour cco detection
 *
 * Revision 1.15  2014/06/11 13:17:47  kiran
 * UART as host interface and peripheral interface supported.
 *
 * Revision 1.14  2014/05/28 10:58:59  prashant
 * SDK folder structure changes, Uart changes, removed htm (UI) task
 * Varified - UM, LM - iperf (UDP/TCP), overnight test pass
 *
 * Revision 1.13  2014/05/12 08:09:57  prashant
 * Route fix, STA sync issue with AV CCO and handling discover bcn issue fix
 *
 * Revision 1.12  2014/04/30 22:29:37  tri
 * more PS
 *
 * Revision 1.11  2014/04/09 21:08:52  tri
 * more PS
 *
 * Revision 1.10  2014/04/09 08:18:10  ranjan
 * 1. Added host events for homeplug uppermac indication (Host_SendIndication)
 * 2. timer workaround  + other fixes
 *
 * Revision 1.9  2014/03/25 23:09:15  tri
 * PS
 *
 * Revision 1.8  2014/03/20 23:20:29  tri
 * more PS
 *
 * Revision 1.7  2014/03/12 09:41:22  ranjan
 * 1. added ageout event to cco cnam,backupcco ageout handling
 * 2.  fix linking issue in zb_lx51_asic due to backup cco checkin
 *
 * Revision 1.6  2014/03/10 05:58:10  ranjan
 * 1. added HomePlug BackupCCo feature. verified C&I test.(passed.) (bug 176)
 *
 * Revision 1.5  2014/02/27 10:42:47  prashant
 * Routing code added
 *
 * Revision 1.4  2014/02/26 23:12:54  tri
 * more PS code
 *
 * Revision 1.3  2014/01/28 17:47:02  tri
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
 * Revision 1.31  2013/07/12 08:56:36  ranjan
 * -UKE Push Button Security Feature.
 * Verified : DirectEntry Security Works.Datapath Works.
 *                 command SetSecMode for UKE works.
 * Added against bug-160
 *
 * Revision 1.30  2013/03/22 12:21:49  prashant
 * default FM_MASK and FM_Printf modified for USER INFO
 *
 * Revision 1.29  2013/03/21 07:43:26  ranjan
 * Starting NDC on "p reset" command
 *
 * Revision 1.28  2013/03/18 13:25:59  prashant
 * Changed release TEI timeout 48 hr and data path fix
 *
 * Revision 1.27  2013/03/14 11:49:18  ranjan
 * 1.handled cases  for CCo toSTA switch and  viceversa
 * 2.UM uses bcntemplate
 *
 * Revision 1.26  2013/02/15 12:53:57  prashant
 * ASSOC.REQ changes for DEVELO
 *
 * Revision 1.25  2012/12/18 12:17:46  prashant
 * Stability checkin
 *
 * Revision 1.24  2012/11/22 09:44:02  prashant
 * Code change for auto ping test, sending tei map ind out, random mac addrr generation.
 *
 * Revision 1.23  2012/11/19 07:55:56  prashant
 * Compilation fix for last checkin
 *
 * Revision 1.22  2012/11/19 07:46:23  ranjan
 * Changes for Network discovery modes
 *
 * Revision 1.21  2012/09/24 06:01:38  yuanhua
 * (1) Integrate the NMA and HAL in Rx path (2) add a Tx queue in HAL to have less stack size needed in tx path, and Tx in HAL is performed by polling now.
 *
 * Revision 1.20  2012/07/15 17:31:07  yuanhua
 * (1)fixed a potential memory overwriting in MUXL (2)update prints for 8051.
 *
 * Revision 1.19  2012/07/08 18:42:20  yuanhua
 * (1)fixed some issues when ctrl layer changes its state from the UCC to ACC. (2) added a event CNSM_START.
 *
 * Revision 1.18  2012/07/03 05:18:37  yuanhua
 * fixed an issue in HAL_XmitMacFrame(), which returns the status according to the status from HHAL_PlcTxQWrite() now.
 *
 * Revision 1.17  2012/06/30 23:36:26  yuanhua
 * return the success status for LINKL_SendEvent() when RTX51 OS is used.
 *
 * Revision 1.16  2012/06/20 21:44:42  kripa
 * Assoc.Cnf dtei set to bcst tei.
 * Committed on the Free edition of March Hare Software CVSNT Client.
 * Upgrade to CVS Suite for more features and support:
 * http://march-hare.com/cvsnt/
 *
 * Revision 1.15  2012/06/20 17:56:31  kripa
 *
 * Committed on the Free edition of March Hare Software CVSNT Client.
 * Upgrade to CVS Suite for more features and support:
 * http://march-hare.com/cvsnt/
 *
 * Revision 1.14  2012/06/08 23:23:48  son
 * Fixed tei display
 *
 * Revision 1.13  2012/06/05 07:25:59  yuanhua
 * (1) add a scan call to the MAC during the network discovery. (2) add a tiny task in ISM for interrupt polling (3) modify the frame receiving integration. (4) modify the tiny task in STM.
 *
 * Revision 1.12  2012/06/04 23:34:02  son
 * Added RTX51 OS support
 *
 * Revision 1.11  2012/05/19 20:32:17  yuanhua
 * added non-callback option for the protocol stack.
 *
 * Revision 1.10  2012/05/19 05:05:15  yuanhua
 * optimized the timer handlers in CTRL and LINK layers.
 *
 * Revision 1.9  2012/05/17 05:05:58  yuanhua
 * (1) added the option for timer w/o callback (2) added task id and name.
 *
 * Revision 1.8  2012/05/12 04:11:46  yuanhua
 * (1) added list.h (2) changed the hal tx for the hw MAC implementation.
 *
 * Revision 1.7  2012/05/01 04:51:09  yuanhua
 * added compiler flags STA_FUNC and CCO_FUNC in link and ctrl layers.
 *
 * Revision 1.6  2012/04/30 04:05:57  yuanhua
 * (1) integrated the HAL mgmt Tx. (2) various updates
 *
 * Revision 1.5  2012/04/20 01:39:33  yuanhua
 * integrated uart module and added compiler flag NMA.
 *
 * Revision 1.4  2012/04/13 06:15:11  yuanhua
 * integrate the HPGP protocol stack with the HAL for the beacon TX/RX and mgmt msg RX.
 *
 * Revision 1.3  2012/03/11 17:02:25  yuanhua
 * (1) added NEK auth in AKM (2) added NMA (3) modified hpgp layer data structures (4) added Makefile for linux simulation
 *
 * Revision 1.2  2011/09/09 07:02:31  yuanhua
 * migrate the firmware code from the greenchip to the hybrii.
 *
 * Revision 1.13  2011/09/06 05:01:46  yuanhua
 * Made a fix such that the STA continues periodic TEI renew after CCo handover.
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
 * Revision 1.8  2011/08/02 16:06:00  yuanhua
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
 * Revision 1.4  2011/07/08 22:23:48  yuanhua
 * (1) Implemented CNSM, including its state machine, beacon transmission and process, discover beacon scheduling, auto CCo selection, discover list, handover countdown, etc. (2) Updated SNSM, including discover list processing, triggering a switch to the new CCo, etc. (3) Updated CNAM and SNAM, adding the connection state in the SNAM, switch to the new CCo, etc. (4) Other updates
 *
 * Revision 1.3  2011/07/02 22:09:01  yuanhua
 * Implemented both SNAM and CNAM modules, including network join and leave procedures, systemm resource (such as TEI) allocation and release, TEI renew/release timers, and TEI reuse timer, etc.
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
 * Revision 1.3  2011/04/23 19:48:45  kripa
 * Fixing stm.h and event.h inclusion, using relative paths to avoid conflict with windows system header files.
 *
 * Revision 1.2  2011/04/23 17:34:07  kripa
 * Used relative path for inclusion of stm.h, to avoid conflict with a system header file in VC.
 *
 * Revision 1.1  2011/04/08 21:42:45  yuanhua
 * Framework
 *
 *
 * =========================================================*/

