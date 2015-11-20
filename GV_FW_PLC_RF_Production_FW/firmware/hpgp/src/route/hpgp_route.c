/** =========================================================
 *
 *  @file hpgp_route.c
 * 
 *  @brief Routing Layer
 *
 *  Copyright (C) 2013, Greenvity Communications, Inc.
 *  All Rights Reserved
 *  
 * ===========================================================*/
#ifdef ROUTE

#include <stdlib.h>
#include <string.h>

#include "fm.h"
#include "papdef.h"
#include "hpgp_route.h"
#include "list.h"
#include "event.h"
#include "crm.h"
#include "linkl.h"
#include "hpgpapi.h"
#include "hpgpevt.h"
#include "timer.h"
#include "stm.h"
#include "frametask.h"
#include "event_fw.h"

static sRoute gRoute;
#ifdef ROUTE_TEST
extern u8 dropcco;
#endif

extern void ROUTE_prepareHoldList(sCrm *crm, sScb *scb);

eStatus ROUTE_procRouteInfo(sRouteInfo *rInfo, u8 numEntris, u8 tei)
{
    sScb *pscb = NULL;
    sLinkLayer  *linkl = (sLinkLayer *)HPGPCTRL_GetLayer(HP_LAYER_TYPE_LINK);
    sCrm        *crm = LINKL_GetCrm(linkl);
#ifdef ROUTE_TEST
    if((dropcco == 1) && (linkl->staInfo.ccoScb->tei == tei))
    {
        return STATUS_FAILURE;
    }
#endif
    FM_Printf(FM_MMSG, "ROUTE: <<< EVENT_TYPE_CM_ROUTE_INFO.IND/CNF (tei: %bu).\n", tei);
    if(CRM_GetScb(crm, tei) == NULL)
    {        
        FM_Printf(FM_ROUTE, "ROUTE: <<< TEI not found in SCB table: %bu\n", tei);
        return STATUS_FAILURE;
    }
    
    while(numEntris)
    {
        pscb = CRM_GetScb(crm, rInfo->udtei);
        if(pscb != NULL && pscb->lrtEntry.routeOnHold != TRUE )
        {   
            if((pscb->lrtEntry.rnh > (rInfo->rnh + 1)) ||
                ((pscb->lrtEntry.rnh == (rInfo->rnh + 1)) && (pscb->lrtEntry.rdr < rInfo->rdr)))
            {
                
                routeEvent rEvent;
                pscb->lrtEntry.nTei = tei;
                pscb->lrtEntry.rdr = rInfo->rdr;
                pscb->lrtEntry.rnh = rInfo->rnh + 1;

                rEvent.tei = pscb->tei;
                rEvent.ntei = pscb->lrtEntry.nTei;
                rEvent.numHop = pscb->lrtEntry.rnh;
                if(pscb->lrtEntry.routeIsInvalid == TRUE)
                {
                    pscb->lrtEntry.routeIsInvalid = FALSE;
                    Host_SendIndication(HOST_EVENT_ROUTE_VALID, HPGP_MAC_ID, (u8*)&rEvent, sizeof(routeEvent));
                }
                else
                {
                    
                    Host_SendIndication(HOST_EVENT_ROUTE_CHANGE, HPGP_MAC_ID, (u8*)&rEvent, sizeof(routeEvent));					
                }
            }
        }
        numEntris--;
        rInfo++;
    }
	return STATUS_SUCCESS;
}

eStatus ROUTE_sendRouteInfo(u16 mmType, sEvent *reqEvent)
{
    sScb        *pscb = NULL, *scb = NULL;
    eStatus     status = STATUS_FAILURE;
    sRouteInfo  *routeInfo;    
    sEvent      *event = NULL;      
    sLinkLayer  *linkl = (sLinkLayer *)HPGPCTRL_GetLayer(HP_LAYER_TYPE_LINK);
    sStaInfo    *staInfo = LINKL_GetStaInfo(linkl);
    sCrm        *crm = LINKL_GetCrm(linkl);
    u8          numScb = CRM_GetScbNum(crm);
    u16         eventSize = (sizeof(sRouteInfo) * numScb) + 1;
    u8          *numEntries;
    sHpgpHdr    *reqHpgpHdr;
	sHpgpHdr    *hpgpHdr;

    
    event = EVENT_MgmtAlloc(eventSize, EVENT_HPGP_MSG_HEADROOM );
    if(event == NULL)
    {
        FM_Printf(FM_ROUTE, "Cannot allocate an event.\n");
        return STATUS_FAILURE;
    }

    
    // prepare event    
    event->eventHdr.eventClass = EVENT_CLASS_MSG;

    
    hpgpHdr = (sHpgpHdr *)event->buffDesc.buff;
    hpgpHdr->snid = staInfo->snid;
    hpgpHdr->eks = staInfo->nekEks;
    if(mmType == EVENT_TYPE_CM_ROUTE_INFO_IND)
    {
        
        event->eventHdr.type = EVENT_TYPE_CM_ROUTE_INFO_IND;
        hpgpHdr->tei = 0xFF;
        hpgpHdr->macAddr = bcAddr;
        FM_Printf(FM_MMSG, "ROUTE: >>> EVENT_TYPE_CM_ROUTE_INFO.IND (tei: %bu).\n",
                               hpgpHdr->tei);

    }
    else
    {
        reqHpgpHdr = (sHpgpHdr *)reqEvent->buffDesc.buff;
        event->eventHdr.type = EVENT_TYPE_CM_ROUTE_INFO_CNF;
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
        FM_Printf(FM_MMSG, "ROUTE: >>> EVENT_TYPE_CM_ROUTE_INFO.CNF (tei: %bu).\n",
                                       hpgpHdr->tei);
    }
    
    numEntries = event->buffDesc.dataptr;
    *numEntries = 0;
    pscb = CRM_GetNextScb(crm, pscb);    
    routeInfo = (sRouteInfo*)&event->buffDesc.dataptr[1];
    while(pscb)
    {
        if(pscb->lrtEntry.routeOnHold != TRUE && pscb->lrtEntry.routeIsInvalid != TRUE && pscb != staInfo->staScb)
        {
            routeInfo->rdr = pscb->lrtEntry.rdr;
            routeInfo->rnh = pscb->lrtEntry.rnh;
            routeInfo->udtei = pscb->tei;
            (*numEntries)++;
            routeInfo++;
        }
        pscb = CRM_GetNextScb(crm, pscb);
    }
    if(*numEntries == 0)
    {
        EVENT_Free(event);
        return status;
    }
    event->buffDesc.datalen = (sizeof(sRouteInfo) * (*numEntries)) + 1;
    
    status = MUXL_TransmitMgmtMsg(event);
    //the event is freed by MUXL if the TX is successful
    if(status == STATUS_FAILURE)
    {
        EVENT_Free(event);
    }

    return status;
}

sScb* ROUTE_lookUpLRT(u8 *mac)
{

    return CRM_FindScbMacAddr(mac);

}

eStatus ROUTE_sendRouteInfoReq(sScb *scb)
{
    
    eStatus     status = STATUS_FAILURE;
    sEvent      *event = NULL;      
    sHpgpHdr    *hpgpHdr;    
    sLinkLayer     *linkl = (sLinkLayer *)HPGPCTRL_GetLayer(HP_LAYER_TYPE_LINK);
    sStaInfo    *staInfo = LINKL_GetStaInfo(linkl);
    
    event = EVENT_MgmtAlloc(1, EVENT_HPGP_MSG_HEADROOM );
    if(event == NULL)
    {
        FM_Printf(FM_ROUTE, "Cannot allocate an event.\n");
        return STATUS_FAILURE;
    }

    
    // prepare event    
    event->eventHdr.eventClass = EVENT_CLASS_MSG;
    event->eventHdr.type = EVENT_TYPE_CM_ROUTE_INFO_REQ;
    
    hpgpHdr = (sHpgpHdr *)event->buffDesc.buff;
    hpgpHdr->snid = staInfo->snid;
    hpgpHdr->eks = staInfo->nekEks;
    hpgpHdr->tei = scb->tei;
   
    hpgpHdr->macAddr = scb->macAddr;
    
    FM_Printf(FM_MMSG, "ROUTE: >>> EVENT_TYPE_CM_ROUTE_INFO.REQ (tei: %bu).\n",
                                       hpgpHdr->tei);
    
    event->buffDesc.datalen = 0;
    
    status = MUXL_TransmitMgmtMsg(event);
    //the event is freed by MUXL if the TX is successful
    if(status == STATUS_FAILURE)
    {
        EVENT_Free(event);
    }

    return status;
}

eStatus ROUTE_sendUnreachableInd(u32 ntb)
{
    u8          i;
    sEvent      *event = NULL;
    sUnreachableInd *unrchInd;
    u8          *urchTei;
    eStatus     status = STATUS_FAILURE;
    u16         eventSize = sizeof(sUnreachableInd) + gRoute.numOfTeisOnHold;
    sHpgpHdr    *hpgpHdr;    
    sLinkLayer     *linkl = (sLinkLayer *)HPGPCTRL_GetLayer(HP_LAYER_TYPE_LINK);
    sStaInfo    *staInfo = LINKL_GetStaInfo(linkl);
    tTime duration;
    u32 currNtb;
    u32 ntbDelta;
    
    // Alloc event 
    event = EVENT_MgmtAlloc(eventSize, EVENT_HPGP_MSG_HEADROOM );
    if(event == NULL)
    {
        FM_Printf(FM_ROUTE, "Cannot allocate an event.\n");
        return STATUS_FAILURE;
    }

    
    // prepare event    
    event->eventHdr.eventClass = EVENT_CLASS_MSG;
    event->eventHdr.type = EVENT_TYPE_CM_UNREACHABLE_IND;

    
    hpgpHdr = (sHpgpHdr *)event->buffDesc.buff;
    hpgpHdr->tei = 0xFF; // use unicast
    hpgpHdr->macAddr = bcAddr; // use unicast
    hpgpHdr->snid = staInfo->snid;
    hpgpHdr->eks = staInfo->nekEks;
     
    FM_Printf(FM_MMSG, "ROUTE: >>> CC_UNREACHABLE.IND (tei: %bu).\n",
                                   hpgpHdr->tei);

    unrchInd = (sUnreachableInd *)event->buffDesc.dataptr;
    unrchInd->unrchTs = ntb;
    unrchInd->numEntries = gRoute.numOfTeisOnHold;
    urchTei = event->buffDesc.dataptr + sizeof(sUnreachableInd);
    for(i = 0; i < gRoute.numOfTeisOnHold; i++)
    {
        urchTei[i] = gRoute.holdlist[i];
    }
    event->buffDesc.datalen = eventSize;
                        
    // send event

    gRoute.teiIsOnHold = TRUE;

    status = MUXL_TransmitMgmtMsg(event);
    //the event is freed by MUXL if the TX is successful
    if(status == STATUS_FAILURE)
    {
        EVENT_Free(event);
    }
    // Start HD_Duration timer
    currNtb = rtocl(ReadU32Reg(PLC_ZCNTB_REG));
    ntbDelta = currNtb - ntb;
    duration = HD_DURATION_TIME - ((ntbDelta * 40)/1000000);
    STM_StartTimer(gRoute.hd_duration, duration);
    Host_SendIndication(HOST_EVENT_ROUTE_HOLD, HPGP_MAC_ID, &gRoute.holdlist, gRoute.numOfTeisOnHold);
    return status;

}

eStatus ROUTE_procUnreachableInd(u8 *tei, u8 numTei, u8 srcTei, u32 ntb)
{
    sScb *uscb = NULL;
    u8 i;
    sLinkLayer    *linkl = (sLinkLayer *)HPGPCTRL_GetLayer(HP_LAYER_TYPE_LINK);
    sCrm          *crm = LINKL_GetCrm(linkl);
    FM_Printf(FM_MMSG, "ROUTE: <<< CC_UNREACHABLE.IND (tei: %bu).\n",
                                   srcTei);
    //Send unreachable Ind
    for(i = 0; i < numTei; i++)
    {
        uscb = CRM_GetScb(crm, tei[i]);
        if(uscb != NULL)
        {
            if(uscb->lrtEntry.routeOnHold != TRUE && uscb->lrtEntry.nTei == srcTei)
            { 
                ROUTE_prepareHoldList(crm, uscb);
                // send unreachable ind
                ROUTE_sendUnreachableInd(ntb);                 
            }
        }
    }
	return STATUS_SUCCESS;
}

void ROUTE_routeInit()
{
    sLinkLayer *linkl = (sLinkLayer *)HPGPCTRL_GetLayer(HP_LAYER_TYPE_LINK);
    // Alloc timers
    gRoute.hd_duration = STM_AllocTimer(HP_LAYER_TYPE_LINK, EVENT_TYPE_ROUTE_HD_DURATION_TIMEOUT, linkl);
    gRoute.routeUpdateTimer = STM_AllocTimer(HP_LAYER_TYPE_LINK, EVENT_TYPE_ROUTE_UPDATE_TIMEOUT, linkl);
}
void ROUTE_displayLRT()
{
    sScb *pscb = NULL;
	sLinkLayer  *linkl = (sLinkLayer *)HPGPCTRL_GetLayer(HP_LAYER_TYPE_LINK);
    sCrm        *crm = LINKL_GetCrm(linkl);
    pscb = CRM_GetNextScb(crm, pscb);    
    FM_Printf(FM_USER, "TEI\tRDR\tRNH\tNTEI\tHOLD\tINVALID\n");
    while(pscb)
    {
            
        FM_Printf(FM_USER, "%bu\t%bu\t%bu\t%bu\t%bu\t%bu\n",
            pscb->tei, pscb->lrtEntry.rdr, pscb->lrtEntry.rnh, pscb->lrtEntry.nTei,
            pscb->lrtEntry.routeOnHold, pscb->lrtEntry.routeIsInvalid);
        pscb = CRM_GetNextScb(crm, pscb);
    }

}

void ROUTE_prepareHoldList(sCrm *crm, sScb *scb)
{
    sScb *lscb = NULL;     
    
    gRoute.numOfTeisOnHold = 0;
    scb->lrtEntry.nTei = 0;
    scb->lrtEntry.rdr = 0;
    scb->lrtEntry.rnh = 0xFF;
    scb->lrtEntry.routeOnHold = TRUE;                    
    scb->lrtEntry.routeIsInvalid = FALSE;
    gRoute.holdlist[gRoute.numOfTeisOnHold] = scb->tei;
    gRoute.numOfTeisOnHold++;
    gRoute.teiIsOnHold = TRUE;
    // find ntei of all entries if ntei == scb's tei then put entry on hold
    lscb = CRM_GetNextScb(crm, lscb);
    while(lscb)
    {
       if(scb->tei == lscb->lrtEntry.nTei)
       {
           gRoute.holdlist[gRoute.numOfTeisOnHold] = lscb->tei;
           gRoute.numOfTeisOnHold++;
           lscb->lrtEntry.routeOnHold = TRUE;
       }
       lscb = CRM_GetNextScb(crm, lscb);
    }
}

void ROUTE_preparteAndSendUnreachable(sScb *scb)
{
    u32 ntb;    
    sLinkLayer *linkl = (sLinkLayer *)HPGPCTRL_GetLayer(HP_LAYER_TYPE_LINK);
    sCrm              *crm = LINKL_GetCrm(linkl);
    if(scb && scb->lrtEntry.routeOnHold != TRUE && scb->lrtEntry.routeIsInvalid != TRUE)
    {
        ROUTE_prepareHoldList(crm, scb);
        if(gRoute.teiIsOnHold == TRUE)
        {
            // send unreachable ind
            ntb = rtocl(ReadU32Reg(PLC_ZCNTB_REG));
            ROUTE_sendUnreachableInd(ntb);

        }
    }
}

void ROUTE_update(u8 tei)
{
    sLinkLayer    *linkl = (sLinkLayer *)HPGPCTRL_GetLayer(HP_LAYER_TYPE_LINK);
    sCrm          *crm = LINKL_GetCrm(linkl);
    sScb *scb = CRM_GetScb(crm, tei);    
    routeEvent rEvent;
    
    if(scb)
    {
        if(scb->lrtEntry.nTei != scb->tei)
        {
            scb->lrtEntry.nTei = scb->tei;
            scb->lrtEntry.rnh = 0;

            rEvent.tei = scb->tei;
            rEvent.ntei = scb->lrtEntry.nTei;
            rEvent.numHop = scb->lrtEntry.rnh;
            
            Host_SendIndication(HOST_EVENT_ROUTE_CHANGE, HPGP_MAC_ID, (u8*)&rEvent, sizeof(routeEvent));
        }
    }
}

void ROUTE_procHdDurationTimeout()
{
    sLinkLayer    *linkl = (sLinkLayer *)HPGPCTRL_GetLayer(HP_LAYER_TYPE_LINK);
    sCrm          *crm = LINKL_GetCrm(linkl);
    sScb *pscb = NULL;
    pscb = CRM_GetNextScb(crm, pscb);    
    while(pscb)
    {                            
        if(pscb->lrtEntry.routeOnHold == TRUE)
        {
            pscb->lrtEntry.routeIsInvalid = TRUE;  
            pscb->lrtEntry.nTei = 0;
            pscb->lrtEntry.rdr = 0;
            pscb->lrtEntry.rnh = 0xFF;
            pscb->lrtEntry.routeOnHold = FALSE;
            Host_SendIndication(HOST_EVENT_ROUTE_INVALID, HPGP_MAC_ID, &pscb->tei, 1);
        }
        pscb = CRM_GetNextScb(crm, pscb);
    }
    ROUTE_setTeiIsOnHold(FALSE);
}

void ROUTE_initLrtEntry(sScb *scb)
{
    scb->lrtEntry.nTei = 0;
    scb->lrtEntry.rdr = 0;
    scb->lrtEntry.rnh = 0xFF;
    scb->lrtEntry.routeOnHold = FALSE;                    
    scb->lrtEntry.routeIsInvalid = FALSE;


}
void ROUTE_startUpdateTimer()
{
    STM_StartTimer(gRoute.routeUpdateTimer, ROUTE_INFO_UPDATE_TIME);
}
void ROUTE_stopUpdateTimer()
{

    STM_StopTimer(gRoute.routeUpdateTimer);
}
void ROUTE_setTeiIsOnHold(u8 option)
{
    gRoute.teiIsOnHold = option;
}
#endif // ROUTE

