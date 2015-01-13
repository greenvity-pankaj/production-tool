/** ========================================================
 *
 * @file nsm.c
 * 
 *  @brief Network System Manager:
 *         CNSM: CCO Network System Manager
 *         SNSM: STA Network System Manager
 *
 *  Copyright (C) 2010-2011, Greenvity Communications, Inc.
 *  All Rights Reserved
 *  
 * =========================================================*/


#include <string.h>
#include <stdio.h>
#include "papdef.h"
#ifdef ROUTE
#include "hpgp_route.h"
#endif
#include "linkl.h"
#include "nsm.h"
#include "nam.h"
#include "muxl.h"
#include "hpgpapi.h"
#include "hpgpconf.h"
#include "fm.h"
#include "ism.h"
#include "hpgpevt.h"
#include "mmsg.h"
#include "timer.h"
#include "stm.h"
#include "hal.h"

#include "sys_common.h"
#ifdef HPGP_HAL
#include "hal_hpgp.h"
#else
#include "sdrv.h"
#endif

#ifndef CALLBACK
#include "hpgpapi.h"
#endif
#include "hybrii_tasks.h"
#ifndef UART_HOST_INTF
#define DISC_BCN
#endif
#define HPGP_TIME_BBT                  1000   //2 seconds

#define HPGP_TIME_USAI                  1000   //1 seconds
//#define HPGP_TIME_DISC_AGING            120000 // 2 minutes
#define HPGP_TIME_DISC_AGING            15000 // test 
#define HPGP_TIME_DISC_PERIOD_MAX       5000   //10 seconds - MaxDiscoverPeriod
//#define HPGP_TIME_STA_AGING_CNT       5  //5* 10 seconds - MaxDiscoverPeriod

#define HPGP_HO_COUNTDOWN_MAX           5   //5 beacon periods
#define HPGP_HO_SWITCH_NONE             0   //no handover switch
#define HPGP_HO_SWITCH_STA              1   //switch to the STA mdoe/role
#define HPGP_HO_SWITCH_CCO              2   //switch to the CCO mdoe/role

/* default regions (in unit of ALU) */
#define HPGP_REGION_MIN_SHARED_CSMA    0x5DC  /* minimum shared CSMA */
                                              /* 1500 usec */        
#define HPGP_REGION_MIN_LOCAL_CSMA     0x5DC  /* minimum local CSMA  */
                                              /* 1500 usec */             
#define HPGP_REGION_MAX_BEACON         (8*HPGP_BEACON_SLOT_ATU)  
                                              /* maximum beacon region */
                                              /* 8 slots */             
#define HPGP_GLID_LOCAL_CSMA           0xFF
#define HPGP_GLID_SHARED_CSMA          0xFE
#define HPGP_GLID_DISC_BEACON          0xFD
#define HPGP_GLID_GPP                  0xFB

#ifdef SIMU
#define HPGP_TIME_BCN_INTERVAL    4 //4 ms
#endif

//beacon source
enum
{
    BCN_SRC_CCO,  // central beacon from the CCo/proxy CCo in the same network
    BCN_SRC_DISC, // discovery beacon from the STA in the same network 
    BCN_SRC_OTHER_CCO, //CCo or proxy CCo in other networks
    BCN_SRC_OTHER_DISC, //discovery beacon from other networks
    BCN_SRC_UNKNOWN,     //unknown
};
u16 CSMA_REGIONS_50Hz[HYBRII_MAXSMAREGION_CNT] = {0x30, 0xE7F ,0xFFF,0xFFF, 0xFFF, 0xFFF }; 
u16 CSMA_REGIONS_60Hz[HYBRII_MAXSMAREGION_CNT] = {0x30, 0xBF4 ,0xFFF,0xFFF, 0xFFF, 0xFFF };
extern void LINKL_TimerHandler(u16 type, void *cookie);
#ifdef ROUTE
extern void ROUTE_prepareHoldList(sCrm *crm, sScb *scb);
extern void ROUTE_preparteAndSendUnreachable(sScb *scb);
extern eStatus ROUTE_sendRouteInfo(u16 mmType, sEvent *reqEvent);
#endif

extern u8 psDebug;


void SCB_UpdateDiscStaList(sScb *scb, sDiscStaInfoRef *discStaInfoRef)
{
    u8 i;
    u8 new, k;
	  sLinkLayer    *linkl = (sLinkLayer *)HPGPCTRL_GetLayer(HP_LAYER_TYPE_LINK);
		sStaInfo *staInfo = &linkl->staInfo;

    //search through the discovered STA list 
    //always update the STA information whether or not it is new or old.
    new = 0;
    
    k = DISC_STA_LIST_MAX;
    for(i = 0; i < DISC_STA_LIST_MAX; i++)
    {
        if(staInfo->discStaInfo[i].valid == TRUE)
        {
            if(memcmp(staInfo->discStaInfo[i].macAddr, discStaInfoRef->macAddr, 
                      MAC_ADDR_LEN) == 0)
            {
                //update
                k = i;
                new = 0;
                break;
            }
        } 
        else
        {
            k = i;  
            new = 1;
        }
    }
    
    if(k < DISC_STA_LIST_MAX)
    {
        if(new == 1)
        {
            //found a new discovered STA
            scb->discUpdate = 1;
            staInfo->discStaInfo[k].valid = TRUE;
            memcpy(staInfo->discStaInfo[k].macAddr, discStaInfoRef->macAddr, MAC_ADDR_LEN);
            scb->numDiscSta++;
        }         
//        memcpy(scb->discStaInfo[k].nid, discStaInfoRef->nid, NID_LEN-1);
//        scb->discStaInfo[k].nid[NID_LEN-1] = discStaInfoRef->nid[NID_LEN-1]&0x3F;
        staInfo->discStaInfo[k].tei = discStaInfoRef->tei;
        staInfo->discStaInfo[k].staCap.byte =  discStaInfoRef->discInfo->staCap.byte;

        staInfo->discStaInfo[k].sameNet = discStaInfoRef->sameNet;

        staInfo->discStaInfo[k].hit = 1;
    }
    else
    {
        FM_Printf(FM_WARN, "SNSM: Discovered STA List is full.\n");
    }
}

void SCB_UpdateDiscNetList(sScb *scb, sDiscNetInfoRef *discNetInfoRef)
{
    
    u8 i;
    u8 k;
	  sLinkLayer    *linkl = (sLinkLayer *)HPGPCTRL_GetLayer(HP_LAYER_TYPE_LINK);
	  sStaInfo *staInfo = &linkl->staInfo;
    k = DISC_NET_LIST_MAX;
    for(i = 0; i < DISC_NET_LIST_MAX; i++)
    {
        if(staInfo->discNetInfo[i].valid == TRUE)
        {
            if( (memcmp(staInfo->discNetInfo[i].nid, discNetInfoRef->nid, NID_LEN-1) == 0) &&
                ((staInfo->discNetInfo[i].nid[NID_LEN-1]&NID_EXTRA_BIT_MASK) == (discNetInfoRef->nid[NID_LEN-1]&NID_EXTRA_BIT_MASK)) )
            {
                //already in the list. no update
                staInfo->discNetInfo[k].hit = 1;
                return;
            }
        }
        else
        {
            k = i;  
        }
    }
    if(k < DISC_NET_LIST_MAX)
    {
        //new AVLN is found
        scb->numDiscNet++;
        memcpy(staInfo->discNetInfo[k].nid, discNetInfoRef->nid, NID_LEN-1);
        staInfo->discNetInfo[k].nid[NID_LEN-1] = discNetInfoRef->nid[NID_LEN-1]&NID_EXTRA_BIT_MASK;
//        memcpy(snsm->discNetInfo[k].bpsto, discNetInfoRef->bpsto, 3);
        staInfo->discNetInfo[k].netMode = discNetInfoRef->netMode;
        staInfo->discNetInfo[k].hybridMode = discNetInfoRef->hybridMode;
        staInfo->discNetInfo[k].numBcnSlots = discNetInfoRef->numBcnSlots;
        staInfo->discNetInfo[k].hit = 1;
        scb->discUpdate = 1;
        staInfo->discNetInfo[k].valid  = 1;
    }
    else
    {
        FM_Printf(FM_WARN, "SNSM:Disc NetList full\n");
    }

}


void SCB_AgeDiscLists(sScb *scb)
{
    u8 i;
    sLinkLayer    *linkl = (sLinkLayer *)HPGPCTRL_GetLayer(HP_LAYER_TYPE_LINK);
	sScb          *uscb = NULL;
	sCrm          *crm = LINKL_GetCrm(linkl);
	  sStaInfo *staInfo = &linkl->staInfo;

    //scb = snsm->staInfo->staScb;
    for(i = 0; i < DISC_STA_LIST_MAX; i++)
    {
        if(staInfo->discStaInfo[i].valid == TRUE)
        {
            if(staInfo->discStaInfo[i].hit == 1)
            {
                staInfo->discStaInfo[i].hit = 0;
            }
            else
            {

#ifdef P8051
             FM_Printf(FM_MINFO, "SCB:age out disc entry(tei: %bu)\n",
                                  staInfo->discStaInfo[i].tei);
#else
			FM_Printf(FM_MINFO, "SCB:age out disc entry(tei: %d)\n",
				              staInfo->discStaInfo[i].tei);
#endif
#if 0		//[YM] temporary commnet ageout function
 //Send unreachable Ind
				uscb = CRM_GetScb(crm, scb->discStaInfo[i].tei);
#ifdef POWERSAVE 
				if (uscb->psState == PSM_PS_STATE_ON)
				{
					// distant STA is is PS mode
                	scb->discStaInfo[i].hit = 0; // ?? find a limit time before tearing down the connection
				}
				else
				{
#endif
#ifdef ROUTE


		    	ROUTE_preparteAndSendUnreachable(uscb); 			 
#endif
				{
					sEvent *newEvent;
					
					//send the event to the SNAM to renew the TEI
					newEvent = EVENT_Alloc(MAC_ADDR_LEN, 
										   EVENT_HPGP_CTRL_HEADROOM);
					if(newEvent != NULL)
					{
						sHpgpHdr *hpgpHdr;
						newEvent->eventHdr.eventClass = EVENT_CLASS_CTRL;
						newEvent->eventHdr.type = EVENT_TYPE_STA_AGEOUT;
						hpgpHdr = (sHpgpHdr *)newEvent->buffDesc.buff;
						hpgpHdr->scb = (sScb*)uscb;

						LINKL_SendEvent(linkl, newEvent);
					}
					

				}

                //remove the entry from the list
                memset(&staInfo->discStaInfo[i], 0, sizeof(sDiscStaInfo));
                scb->discUpdate = 1;
                scb->numDiscSta--;
#ifdef POWERSAVE 
				}
#endif
#endif  //[YM]
            }
        }
    }

    for(i = 0; i < DISC_NET_LIST_MAX; i++)
    {
        if(staInfo->discNetInfo[i].valid == TRUE)
        {
            if(staInfo->discNetInfo[i].hit == 1)
            {
                staInfo->discNetInfo[i].hit = 0;
            }
            else
            {
                //remove the entry from the list
                memset(&staInfo->discNetInfo[i], 0, sizeof(sDiscNetInfo));
                scb->discUpdate = 1;
                scb->numDiscNet--;
            }
        }
    }
}

#ifdef CCO_FUNC

/* ========================== 
 * CCO  network system manager
 * ========================== */


eStatus CNSM_SendMgmtMsg(sCnsm *cnsm, 
						u16 mmType,
						u8 snid,
						u8 dsttei, u8 *macAddr)
{
    eStatus         status = STATUS_SUCCESS;
    sEvent         *newEvent = NULL;
    sHpgpHdr       *hpgpHdr = NULL;
	sStaInfo       *staInfo = cnsm->staInfo;
	
    //send the CC_DISCOVER_LIST.REQ 
    //to query the discovered sta/network list
    //for the topology table
    newEvent = EVENT_MgmtAlloc(HPGP_DATA_PAYLOAD_MIN, EVENT_HPGP_MSG_HEADROOM);
    if(newEvent == NULL)
    {
        FM_Printf(FM_ERROR, "EAllocErr\n");
        return STATUS_FAILURE;
    }
    newEvent->eventHdr.eventClass = EVENT_CLASS_MSG;
    newEvent->eventHdr.type = mmType;
//                        newEvent->eventHdr.tei = event->eventHdr.tei;
    hpgpHdr = (sHpgpHdr *)newEvent->buffDesc.buff;
    hpgpHdr->tei = dsttei;
	hpgpHdr->snid = snid;
    hpgpHdr->macAddr = macAddr;
    /* TODO: based on the each station encryption status, set the EKS */
    
                       
    if (mmType == EVENT_TYPE_CC_DISCOVER_LIST_REQ)
    {
#ifdef P8051
        FM_Printf(FM_MMSG, "CNSM:>>>CC_DISC_LIST.REQ(tei: %bu)\n",
                            hpgpHdr->tei);
#else
        FM_Printf(FM_MMSG, "CNSM:>>>CC_DISC_LIST.REQ(tei: %d)\n",
                            hpgpHdr->tei);
#endif

		hpgpHdr->eks = staInfo->nekEks;
    }
	else
	if ((mmType == EVENT_TYPE_NN_INL_REQ) ||
		(mmType == EVENT_TYPE_NN_INL_CNF))
	{
		sNnINLReq *inlReq = (sNnINLReq*)newEvent->buffDesc.dataptr;

		hpgpHdr->eks = HPGP_EKS_NONE;
		
//		hpgpHdr->mnbc = 1;
			
		if (mmType == EVENT_TYPE_NN_INL_REQ)
		{
			FM_Printf(FM_MMSG, "CNSM:>>>NN_INL_REQ(tei:%bu)(snid:%bu)\n",
			              hpgpHdr->tei, snid);
		}else
		if (mmType == EVENT_TYPE_NN_INL_CNF)		
		{
			FM_Printf(FM_MMSG, "CNSM:>>>NN_INL_CNF(tei:%bu)(snid:%bu)\n",
			              hpgpHdr->tei, snid);
		}
	 
		inlReq->numNw = 0;
		memcpy(inlReq->srcNid, staInfo->nid, sizeof(staInfo->nid));
		inlReq->srcSnid = staInfo->snid;
		inlReq->srcTei = staInfo->staScb->tei;
		inlReq->srcNumAuthSta = CRM_GetScbNum(cnsm->crm);			

		newEvent->buffDesc.datalen = sizeof(sNnINLReq);	
	}
	else	
	{
		newEvent->buffDesc.datalen = HPGP_DATA_PAYLOAD_MIN;
	}
    //transmit CM_UNASSOCIATED_STA_IND in the MNBC
    status = MUXL_TransmitMgmtMsg(HPGPCTRL_GetLayer(HP_LAYER_TYPE_MUX), 
                                  newEvent);
    //the event is freed by MUXL if the TX is successful
    if(status == STATUS_FAILURE)
    {
        EVENT_Free(newEvent);
    }
    return status;
}


void CNSM_UpdateSched(sCnsm *cnsm, u8 schedInd, u8 pscd, u8 cscd)
{
    u8 j, k;
    sCsmaRegion *region = cnsm->regionTable[schedInd].region;
    u8 regionNum =  cnsm->regionTable[schedInd].regionNum;
    sSai *sai = cnsm->saiTable[schedInd].sai;
    u16 endTime = 0;
    /* build a sai table based on the region */
    k = 0;
    j = 0;
    while((j< regionNum) && (k < HPGP_SESS_MAX))
    {
        /* we only schedule two types of regions: shared csma and local csma */
        if (region[j].regionType == REGION_TYPE_SHARED_CSMA)
        {
            /* it is assumed that the contiguous regions should have
             * different region type */
            sai[k].stpf = 1;
            sai[k].glid = HPGP_GLID_SHARED_CSMA;
            sai[k].startTime = region[j].startTime;
            sai[k].duration = region[j].endTime - region[j].startTime;
            k++;
        }
        else if (region[j].regionType == REGION_TYPE_LOCAL_CSMA)
        {
            /* it is assumed that the contiguous regions should have
             * different region type */
            sai[k].stpf = 1;
            sai[k].glid = HPGP_GLID_LOCAL_CSMA;
            sai[k].startTime = region[j].startTime;
            sai[k].duration = region[j].endTime - region[j].startTime;
            k++;
        }
        endTime = region[j].endTime;
        j++;
    }
    cnsm->saiTable[schedInd].saiNum = k;
    cnsm->saiTable[schedInd].pscd = pscd;
    cnsm->saiTable[schedInd].cscd = cscd;
#if 0
for (j=0; j<k; j++) {
FM_Printf(FM_HINFO, "CNSM: sai %bu \n", j);
FM_Printf(FM_HINFO, "stpf %bu, glid %bu, start: 0x%x, duration: 0x%x\n",
sai[j].stpf, sai[j].glid, sai[j].startTime, sai[j].duration);
}
#endif
}




extern u8 gEthMacAddrBrdcast[];
#if 1

void CNSM_NcoDetected(sCnsm *cnsm, sRxDesc *rxdesc, u8* bcn)
{
	sBcnHdr *bcnHdr;
    u8       nbe = 0;
	sBeHdr *beHdr;
	u8 		macAddr[MAC_ADDR_LEN];
	u8 		*dataptr;	

	bcnHdr = (sBcnHdr *)bcn;
				  
	nbe = bcnHdr->nbe;
	dataptr = bcn + sizeof(sBcnHdr);
	beHdr = (sBeHdr *) dataptr;


	//(2) Process Beacon Management Information (BMI)
	//Note: According to the standard, the BENTRYs within the MBI shall 
	//be arranged in increasing order of their BEHDR values.
	while(nbe)
	{ 
		dataptr += sizeof(sBeHdr); //move to the start of BEENTRY 
		switch (beHdr->beType)
		{	case BEHDR_MAC_ADDR:
			{
				memcpy(macAddr, dataptr, 6);
				break;
			}
			default:
			{
			}
		}
		//move to the next BEHDR
		dataptr = dataptr + beHdr->beLen; 
		beHdr = (sBeHdr *) dataptr;
		nbe--;
	}
	
	CNSM_SendMgmtMsg(cnsm, EVENT_TYPE_NN_INL_REQ, rxdesc->snid,
			 		bcnHdr->stei, gEthMacAddrBrdcast);

}
#endif

//We split the beacon processing into two parts:
//High priority: those requiring immdiate response
//Low  priority: those tolerating the processing delay .

u8 CNSM_ProcBcnLow(sCnsm *cnsm, sRxDesc *rxdesc, u8* bcn )
{
    //eStatus         status = STATUS_FAILURE;
    sBcnHdr        *bcnHdr = NULL;
    sBeHdr         *beHdr = NULL;
    u8              nid7; 
    u8              bcnsrc = BCN_SRC_UNKNOWN; 
    u8              reqDiscList = 0;
    u8              nbe = 0;
    u8             *dataptr = 0;
    u8             *macAddr = NULL;
    sDiscInfoEntry *discInfoEntry = NULL;
    sDiscStaInfoRef discStaInfoRef;
    sDiscNetInfoRef discNetInfoRef;
    //sBeHdr         *beRef[NELEMENT(] = NULL;
    //sEvent         *newEvent = NULL;
    //sHpgpHdr       *hpgpHdr = NULL;
    sScb           *scb = NULL; 
    sStaInfo       *staInfo = cnsm->staInfo;

    //(1) process the beacon header
//    bcnHdr = (sBcnHdr *) event->buffDesc.dataptr;
    bcnHdr = (sBcnHdr *)bcn;

    nid7 = bcnHdr->nid[NID_LEN-1];

    bcnHdr->nid[NID_LEN-1] &= NID_EXTRA_BIT_MASK;

    if((memcmp(staInfo->nid, bcnHdr->nid, NID_LEN) == 0))
    {
        if( (bcnHdr->bt == BEACON_TYPE_CENTRAL)||
            (bcnHdr->bt == BEACON_TYPE_PROXY))
        {
            //it should not occur
            FM_Printf(FM_MMSG|FM_LINFO, "CNSM:<<<CENTRAL/PROXY BCN(L)\n");
            bcnsrc = BCN_SRC_CCO;

						
			CNSM_NcoDetected(cnsm, rxdesc, bcn);
	
			
            return bcnsrc; 
        }
        else
        {
#ifdef P8051
            FM_Printf(FM_HINFO, "CNSM:<<<DISC BCN(L)(tei:%bu)\n", bcnHdr->stei);
#else
            FM_Printf(FM_HINFO, "CNSM:<<<DISC BCN(L)(tei:%d)\n", bcnHdr->stei);
#endif
            bcnsrc = BCN_SRC_DISC;
#ifdef LG_WAR
            return bcnsrc;
#endif

        }
    }
    else
    {
        if( (bcnHdr->bt == BEACON_TYPE_CENTRAL)||
            (bcnHdr->bt == BEACON_TYPE_PROXY))
        {
            bcnsrc = BCN_SRC_OTHER_CCO;
            //update the network list
            discNetInfoRef.nid = bcnHdr->nid;
            discNetInfoRef.hybridMode = nid7>>6;
            discNetInfoRef.netMode = bcnHdr->nm;
            discNetInfoRef.numBcnSlots = bcnHdr->numslots;
            SCB_UpdateDiscNetList(cnsm->staInfo->ccoScb, &discNetInfoRef);
        }
        else
        {
            bcnsrc = BCN_SRC_OTHER_DISC;
        }
    }
//    FM_Printf(FM_MINFO, "CNSM: classify CCo %d.\n", bcnsrc);

                        
    nbe = bcnHdr->nbe;
    dataptr = bcn + sizeof(sBcnHdr);
    beHdr = (sBeHdr *) dataptr;
   
    
    //(2) Process Beacon Management Information (BMI)
    //Note: According to the standard, the BENTRYs within the MBI shall 
    //be arranged in increasing order of their BEHDR values.
    while(nbe)
    { 
        dataptr += sizeof(sBeHdr); //move to the start of BEENTRY 
        switch (beHdr->beType)
        {
            case BEHDR_REGIONS:
            {
                u8 nextSchedInd = (cnsm->currSchedInd + 1) & 0x1;
                u8 j, k;
                u8 regionNum = 0;
                sRegionEntry *regionEntry = NULL;
                sCsmaRegion *nextRegion = NULL, *currRegion = NULL;
                u16 duration, endTime;
                if(bcnsrc ==  BCN_SRC_DISC) 
                {
                    break;
                }

                if (cnsm->updateSched) 
                {
                    /* we are already in an updating process */
                    break;
                }
 
                currRegion = cnsm->regionTable[cnsm->currSchedInd].region;
                /* save the region info in the next region */
                nextRegion = cnsm->regionTable[nextSchedInd].region;
                memset(nextRegion, 0, HPGP_REGION_MAX);
                /* NR */
                regionNum = *dataptr;
                regionEntry  = (sRegionEntry *)(dataptr + 1);
                endTime = 0;
                j = 0;
                k = 0;
                /* regions */			
                /* assume at present the regions entry will cover the entire 
                 * beacon period, including the beacon region, which will be 
                 * specified by a CCO in uncoordinated or coordinated mode.
                 * but not by a CCO in CSMA-only mode.
                 */
                while((j < regionNum) && (k < HPGP_REGION_MAX))
                {
                    nextRegion[k].startTime = endTime;
                    endTime = (regionEntry->endTimeHi << 8) |
                        regionEntry->endTimeLo;
                    duration = endTime - nextRegion[k].startTime;
                    if (regionEntry->regionType == REGION_TYPE_BEACON)
                    {
                         /* The beacon region should be the first region
                         * if it exists. 
                         */
                        nextRegion[k].regionType = REGION_TYPE_BEACON;
                        nextRegion[k].endTime = endTime;
                    }
                    else if (regionEntry->regionType == REGION_TYPE_SHARED_CSMA)
                    {
                        /* the shared csma region is either the first or second
                         * region if the beacon region exists */
                        if (duration >= (HPGP_REGION_MIN_SHARED_CSMA + 
                            HPGP_REGION_MIN_LOCAL_CSMA)) 
                        {
                            /* since the shared csma is large enough, 
                             * split it to two regions: 
                             * shared csma region and local csma region */
                            nextRegion[k].endTime = nextRegion[k].startTime + HPGP_REGION_MIN_SHARED_CSMA;
                            nextRegion[k].regionType = REGION_TYPE_SHARED_CSMA;
                            k++;
                            nextRegion[k].startTime = nextRegion[k-1].startTime + HPGP_REGION_MIN_SHARED_CSMA;
                            nextRegion[k].endTime = nextRegion[k].startTime + (duration - HPGP_REGION_MIN_SHARED_CSMA);
                            nextRegion[k].regionType = REGION_TYPE_LOCAL_CSMA;
                        }
                        else
                        {
                            nextRegion[k].regionType = REGION_TYPE_SHARED_CSMA;
                            //nextRegion[k].endTime = nextRegion[k].startTime + duration;
                        }
                    }
                    else
                    {
                        /* to provide passive coordination to a CCO 
                         * in coordinated mode, specify a Stayout region 
                         * in all regions other than CSMA regions */
                        nextRegion[k].regionType = REGION_TYPE_STAYOUT;
                        nextRegion[k].endTime = nextRegion[k].startTime + duration;
                    }
                    k++;
                    j++;
                    regionEntry++;		 		
                }
                cnsm->regionTable[nextSchedInd].regionNum = k;
                /* check if the recevied region is different 
                 * from the current region */
                for (j=0; j< k; j++)
                {
                    if ((nextRegion[j].regionType != currRegion[j].regionType) || 
                        (nextRegion[j].startTime != currRegion[j].startTime) || 
                        (nextRegion[j].endTime != currRegion[j].endTime) )
                    {
                        cnsm->updateSched = 1;
                        break;
                    }
                }
                if (cnsm->updateSched)
                {
                    /* update the next sai table */
                    CNSM_UpdateSched(cnsm, nextSchedInd, 5, 4);
                }
                break;
            }
            case BEHDR_MAC_ADDR:
            {
                macAddr = dataptr;
                break;
            }
            case BEHDR_DISCOVER:
            {
                break;
            }
            case BEHDR_DISC_INFO:
            {
                discInfoEntry = (sDiscInfoEntry *)dataptr;
                discStaInfoRef.discInfo = (sDiscInfoEntry *)dataptr;
                if (macAddr)
                {
                    discStaInfoRef.macAddr = macAddr;
//                    discStaInfoRef.nid = bcnHdr->nid;
//                   discStaInfoRef.tei = rxdesc->stei;
                    discStaInfoRef.tei = bcnHdr->stei;
                    discStaInfoRef.snid = rxdesc->snid;
                    if( bcnsrc == BCN_SRC_DISC)
                    {
                        discStaInfoRef.sameNet = 1;
                    }
                    else //am I interested in other discovery beacons here?
                    {
                        discStaInfoRef.sameNet = 0;
                    }

                    //update the discovred STA list
                    SCB_UpdateDiscStaList(cnsm->staInfo->ccoScb, &discStaInfoRef);
                    
#ifdef ROUTE
                    scb = CRM_GetScb(cnsm->crm, bcnHdr->stei);
                    if(scb)
                    {
                        if(scb->lrtEntry.nTei != scb->tei)
                        {
                            scb->lrtEntry.nTei = scb->tei;
                            scb->lrtEntry.rnh = 0;
                        }
                    }
#endif
                }
                
                scb = CRM_GetScb(cnsm->crm, bcnHdr->stei);
                if( memcmp(macAddr, scb->macAddr, MAC_ADDR_LEN) )
                {
                    FM_Printf(FM_WARN, "CNSM:TEI & MAC not matched in Disc BCN\n");
                    break;
                }
                if(scb)
                {
                	sLinkLayer *linkl = (sLinkLayer*)HPGPCTRL_GetLayer(HP_LAYER_TYPE_LINK);
					
                    //update the discovery info for the sta
                    scb->staCap.byte = discInfoEntry->staCap.byte; 
                    scb->staStatus = discInfoEntry->staStatus; 

                    //scb->numDiscSta = discInfoEntry->numDiscSta; 
                    //scb->numDiscNet = discInfoEntry->numDiscNet;  
                    //scb = CRM_GetScb(cnsm->crm, bcnHdr->stei);

					
					if(!linkl->ccoNam.backupCCoCfg.scb)
						{
							CNAM_SelectBackupCCo(&linkl->ccoNam, NULL);
						}
					
                    if(discInfoEntry->staCap.fields.update)
                    {
                        reqDiscList = 1;
                        //send the CC_DISCOVER_LIST.REQ 
                        //to query the discovered sta/network list
                        //for the topology table
                        CNSM_SendMgmtMsg(cnsm, EVENT_TYPE_CC_DISCOVER_LIST_REQ,
                                         staInfo->snid, scb->tei, scb->macAddr);
/*
                        newEvent = EVENT_Alloc(EVENT_DEFAULT_SIZE, 
                                               EVENT_HPGP_MSG_HEADROOM);
                        if(newEvent == NULL)
                        {
                            FM_Printf(FM_ERROR, "Cannot allocate an event.\n");
                            break;
                        }
                        newEvent->eventHdr.eventClass = EVENT_CLASS_MSG;
                        newEvent->eventHdr.type = EVENT_TYPE_CC_DISCOVER_LIST_REQ;
//                        newEvent->eventHdr.tei = event->eventHdr.tei;
                        hpgpHdr = (sHpgpHdr *)newEvent->buffDesc.buff;
                        hpgpHdr->tei = bcnHdr->stei; 
                        hpgpHdr->macAddr = scb->macAddr;
                       
              FM_Printf(FM_MMSG, "CNSM: >>> CC_DISCOVER_LIST.REQ (tei: %d).\n",
                                  hpgpHdr->tei);
                        //transmit CM_UNASSOCIATED_STA_IND in the MNBC
                        status = MUXL_TransmitMgmtMsg(HOMEPLUG_GetLayer(HP_LAYER_TYPE_MUX), newEvent);
                        //the event is freed by MUXL if the TX is successful
                        if(status == STATUS_FAILURE)
                        {
                            EVENT_Free(newEvent);
                        }
*/
                    }

                }
                break;
            }
            case BEHDR_ENCRYP_KEY_CHANGE:
            {
                break;
            }
            case BEHDR_CCO_HANDOVER:
            {
                break;
            }
            case BEHDR_BCN_RELOC:
            {
                break;
            }
            case BEHDR_ACL_SYNC_CNTDOWN:
            {
                break;
            }
            case BEHDR_CHANGE_NUM_SLOTS:
            {
                break;
            }
            case BEHDR_CHANGE_HM:
            {
                break;
            }
            case BEHDR_CHANGE_SNID:
            {
                break;
            }
            default:
            {
            }
        }
        //move to the next BEHDR
        dataptr = dataptr + beHdr->beLen; 
        beHdr = (sBeHdr *) dataptr;
        nbe--;
    }


    return bcnsrc; 
}

void CNSM_ProcBcnHigh(sCnsm *cnsm, u8* bcn )
{
    sBcnHdr        *bcnHdr = NULL;
    sBeHdr         *beHdr = NULL;
    u8              nid7; 
    u8              bcnsrc = BCN_SRC_UNKNOWN; 
    u8              nbe = 0;
    u8             *dataptr = 0;

    sStaInfo       *staInfo = cnsm->staInfo;
    //(1) process the beacon header
//    bcnHdr = (sBcnHdr *) event->buffDesc.dataptr;
    bcnHdr = (sBcnHdr *)bcn;

    nid7 = bcnHdr->nid[NID_LEN-1];

    bcnHdr->nid[NID_LEN-1] &= NID_EXTRA_BIT_MASK;

    if((memcmp(staInfo->nid, bcnHdr->nid, NID_LEN) == 0))
    {
        if( (bcnHdr->bt == BEACON_TYPE_CENTRAL)||
            (bcnHdr->bt == BEACON_TYPE_PROXY))
        {
            //it should not occur
#ifdef NSM_CCO_PRINT			
            FM_Printf(FM_MMSG|FM_LINFO, "CNSM:<<<CENTRAL/PROXY BCN(H)\n");
#endif			
            bcnsrc = BCN_SRC_CCO;
            return; 
        }
        else
        {
#ifdef P8051
            FM_Printf(FM_HINFO, "CNSM:<<<DISC BCN(H)(tei:%bu)\n", bcnHdr->stei);
#else
            FM_Printf(FM_HINFO, "CNSM:<<<DISC BCN(H)(tei:%d)\n", bcnHdr->stei);
#endif
            bcnsrc = BCN_SRC_DISC;

#ifdef LOG_FLASH
            logEvent(DISC_BCN_LOG,0,0,&bcnHdr->stei,1);
#endif

#ifdef LG_WAR
            return;
#endif
            
        }
    }
    else
    {
        if( (bcnHdr->bt == BEACON_TYPE_CENTRAL)||
            (bcnHdr->bt == BEACON_TYPE_PROXY))
        {
            bcnsrc = BCN_SRC_OTHER_CCO;
        }
        else
        {
            bcnsrc = BCN_SRC_OTHER_DISC;
        }
    }

    bcnHdr->nid[NID_LEN-1] = nid7;

                        
    nbe = bcnHdr->nbe;
    dataptr = bcn + sizeof(sBcnHdr);
    beHdr = (sBeHdr *) dataptr;
    
    //(2) Process Beacon Management Information (BMI)
    //Note: According to the standard, the BENTRYs within the MBI shall 
    //be arranged in increasing order of their BEHDR values.
    while(nbe)
    { 
        dataptr += sizeof(sBeHdr); //move to the start of BEENTRY 
        switch (beHdr->beType)
        {
            case BEHDR_NON_PERSISTENT_SCHED:
            {
                //copy 
                break;
            }
            case BEHDR_PERSISTENT_SCHED:
            {
                break;
            }
            case BEHDR_BPSTO:
            {
                //set it to MAC

                break;
            }
            default:
            {
            }
        }
        //move to the next BEHDR
        dataptr = dataptr + beHdr->beLen; 
        beHdr = (sBeHdr *) dataptr;
        nbe--;
    }

}

void LINKL_CcoProcBcnHandler(void *cookie, sEvent *event)
{
    sLinkLayer     *linkl = (sLinkLayer *)cookie;
    sCnsm          *cnsm = (sCnsm *)LINKL_GetCnsm(linkl);

    CNSM_ProcBcnHigh(cnsm, event->buffDesc.dataptr);
}


void CNSM_PerformAutoCcoSelection(sCnsm *cnsm)
{
    sScb        *scbIter = NULL;
    sScb        *scb = NULL;
    sEvent      *newEvent = NULL;
    u8          ccoCap = 0;
    u8          staCap = 0;
    sHpgpHdr   *hpgpHdr = NULL;
//    sLinkLayer *linkl = (sLinkLayer *)HPGPCTRL_GetLayer(HP_LAYER_TYPE_LINK);
    sLinkLayer *linkl = cnsm->linkl;
    sStaInfo   *staInfo = LINKL_GetStaInfo(linkl);
    

    ccoCap = staInfo->ccoScb->staCap.fields.ccoCap;
    //FM_Printf(FM_ERROR, "CNSM: CCO Cap: %d.\n", ccoCap);
    
    scbIter = CRM_GetNextScb(cnsm->crm, scbIter);
    while(scbIter)
    {
       staCap = scbIter->staCap.fields.ccoCap;
       //FM_Printf(FM_ERROR, "CNSM: TEI: %bu. CCo Cap: %bu.\n", scbIter->tei, staCap); 
//        if(ccoCap < scbIter->staCap.fields.ccoCap) 
        if(ccoCap < staCap) 
        {
           
             ccoCap = staCap;
//           ccoCap = scbIter->staCap.fields.ccoCap;  
           scb = scbIter;
        }
        scbIter = CRM_GetNextScb(cnsm->crm, scbIter);
    }

    if(scb)
    {
        //send an indication
        newEvent = EVENT_Alloc(EVENT_DEFAULT_SIZE, EVENT_HPGP_CTRL_HEADROOM);
        if(newEvent == NULL)
        {
            FM_Printf(FM_ERROR, "EAllocErr\n");
            return ;
        }
        newEvent->eventHdr.eventClass = EVENT_CLASS_CTRL;
        newEvent->eventHdr.type = EVENT_TYPE_CCO_SELECT_IND;

        hpgpHdr = (sHpgpHdr *)newEvent->buffDesc.buff;
        hpgpHdr->scb = scb;
//FM_Printf(FM_ERROR, "CNSM: send a cco selected ind.\n"); 
        //LINKL_SendEvent(linkl, newEvent);
        SLIST_Put(&linkl->intEventQueue, &newEvent->link);
    }
    return;
}



eStatus CNSM_InitRegion(sCnsm *cnsm, sLinkLayer *linkl)
{
    sCsmaRegion *region = NULL;
    u16 *pRegion;
    /* initial regsions */
    cnsm->currSchedInd = 0;
    region = cnsm->regionTable[0].region;
    memset(region, 0, HPGP_REGION_MAX);
    if(gHpgpHalCB.lineFreq == FREQUENCY_50HZ)
    {
        pRegion = &CSMA_REGIONS_50Hz[0];
    }
    else
    {
        pRegion = &CSMA_REGIONS_60Hz[0];
    }

    region[0].startTime = 0x0;
#if 1
//    if (cnsm->staInfo->lineMd == LINE_MODE_DC)
    {
        region[0].endTime = pRegion[0];
        region[0].regionType = REGION_TYPE_BEACON; //REGION_TYPE_SHARED_CSMA;
        region[0].rxOnly = 0;
        region[0].hybridMd  = 1;
        region[1].startTime = pRegion[0];
        region[1].endTime  = pRegion[1];
        region[1].rxOnly  = 0;
        region[1].regionType = REGION_TYPE_LOCAL_CSMA;
        region[1].hybridMd   = 1;        
        
        region[2].startTime = pRegion[1];
        region[2].endTime  = pRegion[2];
        region[2].rxOnly  = 1;
        region[2].regionType = REGION_TYPE_STAYOUT;
        region[2].hybridMd   = 1;
        region[3].startTime = pRegion[2];
        region[3].endTime  = pRegion[3];
        region[3].rxOnly  = 1;
        region[3].regionType = REGION_TYPE_STAYOUT;
        region[3].hybridMd   = 1;
        region[4].startTime = pRegion[3];
        region[4].endTime  = pRegion[4];
        region[4].rxOnly  = 1;
        region[4].regionType = REGION_TYPE_STAYOUT;
        region[4].hybridMd   = 1;
        region[5].startTime = pRegion[4];
        region[5].endTime  = pRegion[5];
        region[5].rxOnly  = 1;
        region[5].regionType = REGION_TYPE_STAYOUT;
        region[5].hybridMd   = 1;
    }
#if 0
    else
    {
        region[0].endTime = 0xCA;// 0x2FF; /* in ATU  ( usec) */
        region[0].regionType = REGION_TYPE_BEACON; //REGION_TYPE_SHARED_CSMA;
        region[0].rxOnly = 0;
        region[0].hybridMd  = 1;
        region[1].startTime = 0xCA;
        region[1].endTime  = 0xAF0;
        region[1].rxOnly  = 0;
        region[1].regionType = REGION_TYPE_LOCAL_CSMA;
        region[1].hybridMd   = 1;
        region[2].startTime = 0xAF0;
        region[2].endTime  = endOfRegion;//0xD75;
        region[2].rxOnly  = 1;
        region[2].regionType = REGION_TYPE_STAYOUT;
        region[2].hybridMd   = 1;
        region[3].startTime = endOfRegion;//0xD75;
        region[3].endTime  = endOfRegion;//0xD75;
        region[3].rxOnly  = 1;
        region[3].regionType = REGION_TYPE_STAYOUT;
        region[3].hybridMd   = 1;
        region[4].startTime = endOfRegion;//0xD75; /* 0x2FF + 0x6A1 */
        region[4].endTime  = endOfRegion;//0xD75;
        region[4].rxOnly  = 1;
        region[4].regionType = REGION_TYPE_STAYOUT;
        region[4].hybridMd   = 1;
        region[5].startTime = endOfRegion;//0xD75; /* 0x2FF + 0x6A1 */
        region[5].endTime  = endOfRegion;//0xD75;
        region[5].rxOnly  = 1;
        region[5].regionType = REGION_TYPE_STAYOUT;
        region[5].hybridMd   = 1;
    }
#endif
    cnsm->regionTable[0].regionNum = 6;
#else
    region[0].endTime = 0xA00; /* in ATU */
    region[0].regionType = REGION_TYPE_SHARED_CSMA;
    cnsm->regionTable[0].regionNum = 1;
#endif	
    CNSM_UpdateSched(cnsm, cnsm->currSchedInd, 0, 4);
    cnsm->updateSched = 0;
    HHAL_SetCsmaRegions(cnsm->regionTable[0].region, 
        cnsm->regionTable[0].regionNum);
	return STATUS_SUCCESS;
}
//Start CCo mode
eStatus CNSM_Start(sCnsm *cnsm, u8 ccoType)
{
    sLinkLayer *linkl = (sLinkLayer *)HPGPCTRL_GetLayer(HP_LAYER_TYPE_LINK);
    sEvent *event = NULL;
    u8 *pos = NULL;
    static u8 regionInitFlag = 1;
    if(regionInitFlag == 1)
    {
        CNSM_InitRegion(cnsm, linkl);
        regionInitFlag = 0;
    }
    event = EVENT_Alloc(1, EVENT_HPGP_CTRL_HEADROOM);
    if(event == NULL)
    {
        FM_Printf(FM_ERROR, "EAllocErr\n");
        return STATUS_FAILURE;
    }
    
    event->eventHdr.eventClass = EVENT_CLASS_CTRL;
    event->eventHdr.type = EVENT_TYPE_CNSM_START;
    pos = event->buffDesc.dataptr;
    *pos = ccoType; 
 
    event->buffDesc.datalen = 1;
#ifdef NSM_CCO_PRINT	
	FM_Printf(FM_ERROR, "CNSM:Start(%bu)\n", ccoType);
#endif	
    LINKL_SendEvent(linkl, event);
    return STATUS_SUCCESS;
}


void CNSM_Stop(sCnsm *cnsm)
{
    cnsm->hoCntDown = HPGP_HO_COUNTDOWN_MAX;
    cnsm->hoEnabled = 0;
    cnsm->hoReady = 0;
    cnsm->nctei = 0;

	gHpgpHalCB.syncComplete = 0;

    
    cnsm->state = CNSM_STATE_INIT;

    STM_StopTimer(cnsm->discTimer);   
    STM_StopTimer(cnsm->discAgingTimer);   
#ifdef SIMU
    STM_StopTimer(cnsm->bcnTimer);   
#else
    //disable the MAC HW to transmit the beacon
#endif

}

u8 CNSM_QueryAnyAlvn(sCnsm *cnsm)
{
    if(cnsm->staInfo->ccoScb->numDiscNet)
    {
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}



void CNSM_EnableHo(sCnsm *cnsm, u8 enable)
{
    cnsm->hoEnabled = enable;
}


void CNSM_StartHo(sCnsm *cnsm, u8 nctei)
{
    cnsm->hoReady = 1;
    cnsm->nctei = nctei;
}

void CNSM_UpdateDiscBcnSched(sCnsm *cnsm, sScb *scb)
{
    if(cnsm->discScb == scb)
    {
        cnsm->discScb = CRM_GetNextScb(cnsm->crm, cnsm->discScb);
    }

}

#endif /* CCO_FUNC */

#if 0
//beacon entry header type
u8 BeHdrType[] =
{
    BEHDR_NON_PERSISTENT_SCHED,    //0x00   //Non-Persistent Schedule
    BEHDR_PERSISTENT_SCHED,        //0x01   //Current Persistent Schedule
    BEHDR_PERSISTENT_SCHED,        //0x01   //Preview Persistent Schedule
    BEHDR_REGIONS,                 //0x02   //region Schedule
    BEHDR_MAC_ADDR,                //0x03   //MAC Address
    BEHDR_DISCOVER,                //0x04   //Discover
    BEHDR_DISC_INFO,               //0x05   //Discovered Info
    BEHDR_BPSTO,                   //0x06   //Beacon Period Start Time Offset
    BEHDR_ENCRYP_KEY_CHANGE,       //0x07   //Encryption Key Change
    BEHDR_CCO_HANDOVER,            //0x08   //CCo Handover
    BEHDR_BCN_RELOC,               //0x09   //Beacon Relocation
    BEHDR_ACL_SYNC_CNTDOWN,        //0x0A   //AC Line Sync Countdown
    BEHDR_CHANGE_NUM_SLOTS,        //0x0B   //Change NumSlots
    BEHDR_CHANGE_HM,               //0x0C   //Change Hybrid Mode
    BEHDR_CHANGE_SNID,             //0x0D   //Change SNID
    BEHDR_RSN_INFO,                //0x0E   //RSN Info Element
    BEHDR_ISP,                     //0x0F   //ISP BENTRY
    BEHDR_EXT_BAND_STAY_OUT,       //0x10   //Extended Band Stay Out
    BEHDR_AG_ASSIGN,               //0x11   //AG Assignment
    BEHDR_EXT_CARR_SUPPORT,        //0x12   //Extended Carriers Support
    BEHDR_PWR_SAVE,                //0x13   //Power Save BENTRY
    BEHDR_VENDOR_SPEC,             //0xFF   //Vendor Specific
};

//Maximum size allowed for each entry corresponding to BeHdrType[]
u8 BeLenMax[] =
{
    sizeof(sBcnHdr) + 2 + 16,     //non-persistent: 4 SAIs (4 octets per SAI)
    sizeof(sBcnHdr) + 2 + 16,     //current persistent: 4 SAIs (4 octets per SAI)
    sizeof(sBcnHdr) + 2 + 16,     //preview persistent: 4 SAIs (4 octets per SAI)
    sizeof(sBcnHdr) + 1 + 8,      //region: 4 regions
    sizeof(sBcnHdr) + MAC_ADDR_LEN,  //MAC Address
    sizeof(sBcnHdr) + 1,             //Discover
    sizeof(sBcnHdr) + sizeof(sDiscInfoEntry),   //Discovered Info
    sizeof(sBcnHdr) + 3,            //Beacon Period Start Time Offset
    sizeof(sBcnHdr) + sizeof(sEncrypKeyChangeEntry),  //Encryption Key Change
    sizeof(sBcnHdr) + sizeof(sCcoHoEntry),      //CCo Handover
    sizeof(sBcnHdr) + sizeof(sBcnRelocEntry),   //Beacon Relocation
    sizeof(sBcnHdr) + sizeof(sAclSyncCntDownEntry),  //AC Line Sync Countdown
    sizeof(sBcnHdr) + sizeof(sChangeNumSlotsEntry),  //Change NumSlots
    sizeof(sBcnHdr) + sizeof(sChangeHmEntry),        //Change Hybrid Mode
    sizeof(sBcnHdr) + sizeof(sChangeSnidEntry),      //Change SNID
    0,                             //RSN Info Element
    0,                             //ISP BENTRY
    0,                             //Extended Band Stay Out
    0,                             //AG Assignment
    0,                             //Extended Carriers Support
    sizeof(sBcnHdr) + 12,          //Power Save BENTRY: 8 stations
    0,                             //Vendor Specific
};
#else
extern u8 BeHdrType[];
extern u8 BeLenMax[];
#endif


#ifdef CCO_FUNC

eStatus CNSM_BcnUpdateActive(sCnsm *cnsm)
{

    return (cnsm->bcnUpdateProgress);
    
}
eStatus CNSM_BuildBeacon(sCnsm *cnsm, u8 bcnType)
{
    sBcnHdr        *bcnHdr = NULL;
    sBeHdr         *beHdr = NULL;
    u8              bcnLen = 0;
    u8              beLen = 0;
    u8             *dataptr = NULL;
//    u8             *nbe = NULL; //number of beacon entries
    u8              offset = 0; 
    u8              i;
    u8              done = 0;
    sDiscInfoEntry *discInfoEntry = NULL;
    sCcoHoEntry    *ccoHo = NULL;
    sRegionEntry   *regionEntry = NULL;
    u16             endTime = 0;
    u8              schedInd = cnsm->currSchedInd;
//    sLinkLayer     *linkl = (sLinkLayer *)HPGPCTRL_GetLayer(HP_LAYER_TYPE_LINK);
    sLinkLayer     *linkl = cnsm->linkl;
    sStaInfo       *staInfo = LINKL_GetStaInfo(linkl);

#ifdef NSM_CCO_PRINT
    FM_Printf(FM_LINFO, "CNSM:Build a BCN payload\n");
#endif
    memset(cnsm->bcnBuff, 0, BEACON_BUFF_LEN);

#ifdef SIMU
    offset = sizeof(sFrmCtrlBlk) + sizeof(sTxDesc);
#else
    offset = sizeof(sFrmCtrlBlk);
#endif
    /* build a beacon payload */
    bcnHdr = (sBcnHdr *)(cnsm->bcnBuff + offset);

    //build beacon header
    memcpy(bcnHdr->nid, staInfo->nid, NID_LEN);
    bcnHdr->nid[NID_LEN-1] = (bcnHdr->nid[NID_LEN-1]&NID_EXTRA_BIT_MASK)|(staInfo->hm <<6);
    bcnHdr->stei = staInfo->ccoScb->tei;
    bcnHdr->bt = bcnType;
    bcnHdr->ncnr = 0;
    bcnHdr->npsm = 0;
    bcnHdr->numslots = 0;
    bcnHdr->slotusage = 0;
    bcnHdr->ccocap = staInfo->ccoScb->staCap.fields.ccoCap;

    if ( (bcnType == BEACON_TYPE_CENTRAL)&&
         (cnsm->hoEnabled))
    {
        bcnHdr->hoip = 1;
    }
    else
    {
        bcnHdr->hoip = 0;
    }
         

    bcnLen = sizeof(sBcnHdr);    //13 bytes, including nbe

//    nbe = (u8 *)bcnHdr + sizeof(sBcnHdr);
    dataptr = (u8 *)bcnHdr + sizeof(sBcnHdr);
    //prepare the Beacon entries  
//    dataptr = nbe;
//    dataptr++;
//    bcnLen += 1;                 //1 byte 

   
    i = 0;
    done = 0;
    beHdr = (sBeHdr *)dataptr;
    bcnHdr->nbe = 0;
    while((!done) && ((offset + bcnLen + BeLenMax[i]) <= BEACON_PAYLOAD_SIZE))
    {
        beLen = 0;
        switch(BeHdrType[i])
        {
            case BEHDR_NON_PERSISTENT_SCHED:
            {
                //build non-persistent schedule entry
                beHdr->beType = BEHDR_NON_PERSISTENT_SCHED;
                //dataptr += sizeof(sBeHdr); 
                break;
            }
            case BEHDR_PERSISTENT_SCHED:
            {
                /* this case may run twice: */
                /* one for current schedule, the other for the future */
               
                u8 j;
                sSaiWithSt    *saiwst = NULL;
                sSaiWithoutSt *saiwost = NULL;
                u8            *ptr = NULL;
                sSai          *sai = NULL;
                u8            saiNum;
                u8            scd;
                
                /* build a persistent schedule entry */
                beHdr->beType = BEHDR_PERSISTENT_SCHED;
                /* schedule count down */
                if (schedInd == cnsm->currSchedInd)
                {
                    /* the first persistent schedule 
                     * for the current schedule */
                    if (cnsm->updateSched)
                    {
                        /* CSCD counts down */
                        cnsm->saiTable[schedInd].cscd--;
                    }
                } 
                else if (cnsm->updateSched)
                {
                    /* the second persistent schedule 
                     * for the future/preview schedule */
                    if (cnsm->saiTable[schedInd].pscd == 0)
                    {
                        /* switch to the preview scheduler */
                        cnsm->currSchedInd = (cnsm->currSchedInd +1)&0x1;
                        schedInd = cnsm->currSchedInd;
                        cnsm->updateSched = 0;
#ifdef HPGP_HAL
     //           HHAL_SetCsmaRegions(cnsm->regionTable[schedInd].regoin, 
     //               cnsm->regionTable[schedInd].regionNum);
#endif
                    }
                    else
                    {
                        /* PSCD counts down */
                        cnsm->saiTable[schedInd].pscd--;
                    }
                }
                
                cnsm->saiTable[schedInd].cscd = 0; // test
                if ((schedInd == cnsm->currSchedInd) || (cnsm->updateSched))
                {
                    sai = cnsm->saiTable[schedInd].sai;
                    saiNum = cnsm->saiTable[schedInd].saiNum;
                    scd = cnsm->saiTable[schedInd].pscd |   
                          (cnsm->saiTable[schedInd].cscd << 3);
                    dataptr += sizeof(sBeHdr); 
                    *dataptr = scd;
                    /* NS */
                    *(dataptr + 1)= saiNum & 0x3F;
                    ptr = dataptr + 2;
                    beLen = 2;
                    endTime = 0;
                    for (j=0; j< saiNum; j++)
                    {
                        if (sai[j].stpf)
                        {
                            saiwst = (sSaiWithSt *)ptr;
                            saiwst->stpf = 1;
                            saiwst->glid = sai[j].glid;
                            saiwst->stLo = (sai[j].startTime) & 0xFF;
                            saiwst->stHi = (sai[j].startTime >> 8) & 0xF;
                            endTime = sai[j].startTime + sai[j].duration;
                            saiwst->etLo = endTime & 0xF;
                            saiwst->etHi = (endTime >> 4) & 0xFF;
                            ptr += sizeof(sSaiWithSt);
                            beLen += sizeof(sSaiWithSt);
                        }
                        else
                        {
                            saiwost = (sSaiWithoutSt *)ptr;
                            saiwost->stpf = 0;
                            saiwost->glid = sai[j].glid;
                            endTime += sai[j].duration;
                            saiwost->etLo = endTime & 0xFF;
                            saiwost->etHi = (endTime >> 8) & 0xF;
                            ptr += sizeof(sSaiWithoutSt);
                            beLen += sizeof(sSaiWithoutSt);
                        } 
                    }
                    schedInd = (schedInd + 1) & 0x1;
                }                
//FM_HexDump(FM_DATA, "CNSM sched entry:", dataptr, beLen);
                break;
            }
            case BEHDR_REGIONS:
            {			
                u8 j;
                u8 regionNum;
                sCsmaRegion *region;
                /* build region entry */
                beHdr->beType = BEHDR_REGIONS;
                dataptr += sizeof(sBeHdr); 
                /* schedule regions */
                schedInd = cnsm->currSchedInd;
                region = cnsm->regionTable[schedInd].region;
                regionNum = cnsm->regionTable[schedInd].regionNum;

                /* NR */
                *dataptr = regionNum;
                beLen = 1;
				regionEntry = (sRegionEntry *)(dataptr + 1);
				/* regions */
				endTime = region[0].startTime;
				for (j=0; j< regionNum; j++)
				{
				    endTime = region[j].endTime;
				    //regionEntry->endTimeHi = (endTime >> 8)&0xF;
					//regionEntry->endTimeLo = endTime & 0xFF;
					regionEntry->endTimeLo =  (u8) endTime & 0xF;
				    regionEntry->endTimeHi = (u8) (endTime >> 4 ) & 0xFF;
					regionEntry->regionType = region[j].regionType;
					regionEntry++;
					beLen += sizeof(sRegionEntry); 					
				}
//FM_HexDump(FM_DATA, "CNSM region entry:", dataptr, beLen);				
                break;
            }
            case BEHDR_MAC_ADDR:
            {
//               if(bcnType == BEACON_TYPE_DISCOVER)
              if(cnsm->txDiscBcn)
               {
                    beHdr->beType = BEHDR_MAC_ADDR;
                    dataptr += sizeof(sBeHdr); 
                    memcpy(dataptr, staInfo->ccoScb->macAddr, MAC_ADDR_LEN);
                    beLen = MAC_ADDR_LEN;
               } 

                break;
            }
            case BEHDR_DISCOVER:
            {
                if(cnsm->schedDiscBcn)
                {
                    //schedule the discover beacon
                    //get the TEI of associated STA
                    cnsm->discScb = CRM_GetNextScb(cnsm->crm, cnsm->discScb);
                    if( cnsm->discScb != NULL)
                    {
                    
                        if( (cnsm->discScb->tei != staInfo->ccoScb->tei) &&
                            (cnsm->discScb->namState == STA_NAM_STATE_CONN) )
                        {
#ifdef P8051
 FM_Printf(FM_HINFO, "CNSM:Schedule disc bcn(tei:%bu)\n", 
                     cnsm->discScb->tei);
#else
 FM_Printf(FM_HINFO, "CNSM:Schedule disc bcn(tei:%d)\n", 
                     cnsm->discScb->tei);
#endif
                            beHdr->beType = BEHDR_DISCOVER;
                            dataptr += sizeof(sBeHdr); 
                            *dataptr = cnsm->discScb->tei;
                            beLen = 1;
                        }
                        cnsm->bcnUpdate = 1;
                    }
                    else
                    {
                        //all STAs have scheduled so far
                        cnsm->schedDiscBcn = 0;
                        cnsm->bcnUpdate = 1;
                    }
                }
                break;
            }
            case BEHDR_DISC_INFO:
            {
//               if(bcnType == BEACON_TYPE_DISCOVER)
               if(cnsm->txDiscBcn)
               {
                    cnsm->txDiscBcn--;
                    beHdr->beType = BEHDR_DISC_INFO;
                    dataptr += sizeof(sBeHdr); 
                    discInfoEntry = (sDiscInfoEntry *)dataptr;
                    discInfoEntry->staCap.byte = staInfo->ccoScb->staCap.byte;

                    discInfoEntry->numDiscSta = staInfo->ccoScb->numDiscSta; 
                    discInfoEntry->numDiscNet = staInfo->ccoScb->numDiscNet; 

                    discInfoEntry->staStatus.byte = staInfo->ccoScb->staStatus.byte; 
                    beLen = sizeof(sDiscInfoEntry);
               }
                break;
            }
            case BEHDR_BPSTO:
            {
                /* build bpsto entry */
                beHdr->beType = BEHDR_BPSTO;
                dataptr += sizeof(sBeHdr); 
                beLen = 3;
//                         cnsm->bpstoOffset = bcnLen + sizeof(sBeHdr);
                cnsm->bpstoOffset = ((u8 *)beHdr  + sizeof(sBeHdr)) -  ((u8 *)cnsm->bcnBuff );
                break;
            }
            case BEHDR_ENCRYP_KEY_CHANGE:
            {
                //build encrption key change entry
                //beHdr->beType = BEHDR_ENCRYP_KEY_CHANGE;
                //dataptr += sizeof(sBeHdr); 

                break;
            }
            case BEHDR_CCO_HANDOVER:
            {
                //build cco handover entry
                if ( (bcnType == BEACON_TYPE_CENTRAL)&&
                     (cnsm->hoReady))
                {
                    beHdr->beType = BEHDR_CCO_HANDOVER;
                    dataptr += sizeof(sBeHdr); 
                    ccoHo = (sCcoHoEntry *)dataptr;
                    ccoHo->hcd = cnsm->hoCntDown;
                    ccoHo->nctei = cnsm->nctei; //TODO
                    beLen = sizeof(sCcoHoEntry);
     
                }
                break;
            }
            case BEHDR_BCN_RELOC:
            {
                //build beacon relocation entry
                //beHdr->beType = BEHDR_BCN_RELOC;
                //dataptr += sizeof(sBeHdr); 

                break;
            }
            case BEHDR_ACL_SYNC_CNTDOWN:
            {
                //build AC line sync countdown entry
                //beHdr->beType = BEHDR_ACL_SYNC_CNTDOWN;
                //dataptr += sizeof(sBeHdr); 

                break;
            }
            case BEHDR_CHANGE_NUM_SLOTS:
            {
                //build change number of slots entry
                //beHdr->beType = BEHDR_CHANGE_NUM_SLOTS;
                //dataptr += sizeof(sBeHdr); 

                break;
            }
            case BEHDR_CHANGE_HM:
            {
                //build change hybrid mode entry
                //beHdr->beType = BEHDR_CHANGE_HM;
                //dataptr += sizeof(sBeHdr); 

                break;
            }
            case BEHDR_CHANGE_SNID:
            {
                //build change snid entry
                //beHdr->beType = BEHDR_CHANGE_SNID;
                //dataptr += sizeof(sBeHdr); 

                break;
            }
            case BEHDR_PWR_SAVE:
            {
#ifdef POWERSAVE
				sPowerSaveEntry *pPsBentry ;
				sHaLayer *hal;

                beHdr->beType = BEHDR_PWR_SAVE;
                dataptr += sizeof(sBeHdr);
				pPsBentry = (sPowerSaveEntry *) dataptr;

				hal = (sHaLayer *) HOMEPLUG_GetHal();
    			if (hal->hhalCb->psAvln == FALSE)
				{
					// if AVLN PS is disabled, set Stop PS flag to TRUE, no need to set anything else
					// if there are HPAV1.1 stations in AVLN, set HPAV11 to TRUE
					pPsBentry->spsf = TRUE;
	    			pPsBentry->tpss = 0xf;
					if (hal->hhalCb->psHPAV11 == TRUE)
					{
						pPsBentry->av11pf = TRUE;
					}

					beLen = sizeof(sPowerSaveEntry);
				}
				else
				{
				    sLinkLayer    *linkl = (sLinkLayer *)HPGPCTRL_GetLayer(HP_LAYER_TYPE_LINK);
				    sStaInfo      *staInfo = LINKL_GetStaInfo(linkl);
    				sScb          *scb = NULL;
					sStations	  *psStation;

	    			pPsBentry->bpCnt_Lo = staInfo->ccoScb->bpCnt & 0xff;		// use only 12 bits
	    			pPsBentry->bpCnt_Hi = (staInfo->ccoScb->bpCnt & 0xf00) >> 8;
	    			pPsBentry->av11pf = FALSE;
	    			pPsBentry->tpss = staInfo->ccoScb->pss;
	    			pPsBentry->pssi = staInfo->ccoScb->pssi;
					beLen = sizeof(sPowerSaveEntry);
                	psStation = (sStations *) (dataptr + beLen);
					// go through all STAs in AVLN
//printf("CRM_GetscbNum()=%bu\n", CRM_GetScbNum(cnsm->crm));
                    scb = CRM_GetNextScb(cnsm->crm, scb);
    				while(scb)
    				{
						if (scb->psState == PSM_PS_STATE_ON)
						{
							psStation->tei = scb->tei;
							psStation->pss = scb->pss;
							++psStation;
							beLen += sizeof(sStations);
//							printf("add PS bentry from tei %bu, pss %bu\n", scb->tei, scb->pss);
						}
                    	scb = CRM_GetNextScb(cnsm->crm, scb);
					}
				}
#ifdef PS_DEBUG
				if (psDebug)
					printf("CNSM_BuildBeacon:scb->bpCnt=%d\n", staInfo->ccoScb->bpCnt);
#endif
/*
					if (psDebug)
					{
						u8 i;
						u8 *tmpp = dataptr;

						for (i=0;i<beLen;i++)
							printf("0x%bx ", tmpp[i]);
						printf("\n"); 
					}
*/
                break;
#endif
            }
            case BEHDR_VENDOR_SPEC:  
            {
                done = 1;
                break;
            }
            default:
            {
                //do not build the entry in the beacon
            }
        }
        //add the non-zero entry 
        if( beLen != 0)
        { 
//            beHdr->beLen = beLen + sizeof(sBcnHdr); 
            beHdr->beLen = beLen; 
            bcnLen += sizeof(sBeHdr) + beHdr->beLen;
            dataptr += beLen;
            beHdr = (sBeHdr *)dataptr;
            beLen = 0;
//            *nbe = *nbe+1;
            bcnHdr->nbe++;
        }
        i++;
    }
    cnsm->bcnLen = bcnLen;
    return STATUS_SUCCESS;
}
     
eStatus CNSM_TransmitBeacon(sCnsm *cnsm)
{
    sTxDesc    txinfo;
    sBuffDesc  buffDesc;
    u8         offset = 0;
//    sLinkLayer *linkl = (sLinkLayer *)HPGPCTRL_GetLayer(HP_LAYER_TYPE_LINK);
    sLinkLayer *linkl = cnsm->linkl;
    sStaInfo   *staInfo = LINKL_GetStaInfo(linkl);

#ifdef SIMU
    offset = sizeof(sFrmCtrlBlk) + sizeof(sTxDesc);
#else
    offset = sizeof(sFrmCtrlBlk);
#endif

    //transmit the beacon 
    txinfo.dtei = 0xFF;
    txinfo.stei = staInfo->ccoScb->tei;
    txinfo.frameType = BEACON_TYPE_CENTRAL;
    txinfo.snid = staInfo->snid;
  
    //prepare tx control information
    buffDesc.buff = cnsm->bcnBuff;
    buffDesc.bufflen = BEACON_BUFF_LEN;
    buffDesc.dataptr = cnsm->bcnBuff + offset;
    buffDesc.datalen = BEACON_PAYLOAD_SIZE;

//  FM_HexDump(FM_DATA, "CNSM beacon:", buffDesc.dataptr, buffDesc.datalen);

    HAL_TransmitBeacon(HOMEPLUG_GetHal(), &txinfo, &buffDesc, cnsm->bpstoOffset);

        
    return STATUS_SUCCESS;    

}


//beacon transmission in the interrupt context
void LINKL_BcnTxHandler(void* cookie)
{
    sCnsm      *cnsm = NULL;
    sEvent     *event = NULL;
    eStatus     status = STATUS_SUCCESS;
    sScb       *scb = NULL;
    sScb       *nextscb = NULL;
    sScb       *tmpScb;
    sLinkLayer *linkl = (sLinkLayer *)cookie;
    sStaInfo   *staInfo = LINKL_GetStaInfo(linkl);

    cnsm = &linkl->ccoNsm;
    //build central beacon
    status = CNSM_BuildBeacon(cnsm, BEACON_TYPE_CENTRAL);
    if(status == STATUS_SUCCESS)
    {
        CNSM_TransmitBeacon(cnsm);
    }

    if(cnsm->hoCntDown == 0)
    {

        FM_Printf(FM_HINFO, "CNSM: HO switch (become assoc STA)\n");
        //send event CCO_HO_IND to the ctrl       
        event = EVENT_Alloc(EVENT_DEFAULT_SIZE, 0);
        if(event == NULL)
        {
            FM_Printf(FM_ERROR, "EAllocErr\n");
            return ;
        }

        //Handover: switch to the STA mode/role
        //still keep the SCBs in CRM for TEI MAP, 
        //but free the tei timer for all SCBs
        staInfo->staScb = staInfo->ccoScb;
        scb = CRM_GetNextScb(cnsm->crm, scb);
        while(scb)
        {
            STM_StopTimer(scb->teiTimer);
            STM_FreeTimer(scb->teiTimer);
            scb->teiTimer = STM_TIMER_INVALID_ID;
            if (scb->tei == cnsm->nctei)
            {
                staInfo->ccoScb = scb;
            }
            nextscb = CRM_GetNextScb(cnsm->crm, scb);
            scb = nextscb;
        }

        if( staInfo->ccoScb == NULL)
        {
            //never happen
            FM_Printf(FM_ERROR, "CNSM:can't find cco scb\n");
        }
        CNSM_Stop(cnsm);

        event->eventHdr.eventClass = EVENT_CLASS_CTRL;
        event->eventHdr.type = EVENT_TYPE_CCO_HO_IND;
        //deliver the event to the upper layer
#ifdef CALLBACK
        linkl->deliverEvent(linkl->eventcookie, event);
#else
        CTRLL_ReceiveEvent(linkl->eventcookie, event);
#endif
    }
    else
    {
        if(cnsm->hoReady)
        {
            cnsm->hoCntDown--;
            FM_Printf(FM_MINFO, "CNSM:HO cntdown(%d)\n", cnsm->hoCntDown);
        }
#ifdef SIMU
        STM_StartTimer(cnsm->bcnTimer, HPGP_TIME_BCN_INTERVAL);   
#endif
    }
}

#ifdef SIMU
void LINKL_BcnTimerHandler(u16 type, void* cookie)
{
//    sEvent *event = NULL;
    sLinkLayer * linkl = (sLinkLayer *)cookie;
    //sCnsm* cnsm =  LINKL_GetCnsm(linkl);

    LINKL_BcnTxHandler(linkl);
     
}
#endif


void CNSM_ProcEvent(sCnsm *cnsm, sEvent *event)
{
    sEvent            *newEvent = NULL;
    sHpgpHdr          *hpgpHdr = NULL;
    sScb              *staScb = NULL;
    sCcDiscStaInfo    *ccDiscStaInfo = NULL;
    sCcDiscNetInfo    *ccDiscNetInfo = NULL;
    sRxDesc           rxdesc;
    u8                 num;
    u8                *dataptr = NULL;
    u8                *bcn = NULL;
    u8                 i, ccoType;
   
//    sLinkLayer *linkl = (sLinkLayer *)HPGPCTRL_GetLayer(HP_LAYER_TYPE_LINK);
    sLinkLayer *linkl = cnsm->linkl;
    sStaInfo   *staInfo = LINKL_GetStaInfo(linkl);
#ifdef ROUTE
    sCrm              *crm = LINKL_GetCrm(linkl);
    u32 ntb;
#endif
    hpgpHdr = (sHpgpHdr *)event->buffDesc.buff;
    switch(cnsm->state)
    { 
        case CNSM_STATE_INIT:
        {
            if( (event->eventHdr.eventClass == EVENT_CLASS_CTRL) && 
                (event->eventHdr.type == EVENT_TYPE_CNSM_START) )
            {
                dataptr = event->buffDesc.dataptr;
                ccoType = *dataptr;  
                if(ccoType == LINKL_CCO_TYPE_UNASSOC)
                {
                    cnsm->ccoNotification = 1;
                }
                else if(ccoType == LINKL_CCO_TYPE_HO)
				{
					cnsm->ccoNotification = 0;
					cnsm->txDiscBcn = 10;
					cnsm->bcnUpdate = 1;

                    /* Send CC_DISCOVER_LIST.REQ to each STA mainly due to handover */
                    staScb = CRM_GetNextScb(cnsm->crm, staScb);
                    while(staScb)
                    {
                        if(staScb->tei != cnsm->staInfo->ccoScb->tei)
                        {
                            CNSM_SendMgmtMsg(cnsm, EVENT_TYPE_CC_DISCOVER_LIST_REQ,
                                 			staInfo->snid,
                                 			staScb->tei, 
                                 			staScb->macAddr);
                        }
      
                        staScb = CRM_GetNextScb(cnsm->crm, staScb);
                    }
                }
                else
                {
				    /* CNSM starts first time as Associated CCo */
                    cnsm->ccoNotification = 0;
                }
                cnsm->discScb = NULL;

 







#ifdef DISC_BCN
                STM_StartTimer(cnsm->discTimer, HPGP_TIME_DISC_PERIOD_MAX);
#endif
                STM_StartTimer(cnsm->discAgingTimer, HPGP_TIME_DISC_AGING);
#ifdef NSM_CCO_PRINT				
#ifdef P8051   
FM_Printf(FM_ERROR, "CNSM:Start disc timer(tid: %bu)\n", cnsm->discTimer);
FM_Printf(FM_ERROR, "CNSM:Start disc aging timer(tid: %bu)\n", 
         cnsm->discAgingTimer);
#else
FM_Printf(FM_ERROR, "CNSM: Start disc timer(tid: %d)\n", cnsm->discTimer);
FM_Printf(FM_ERROR, "CNSM: Start disc aging timer(tid: %d)\n", 
         cnsm->discAgingTimer);
#endif
#endif
    
#ifdef SIMU
FM_Printf(FM_ERROR, "CNSM:Start beacon timer(tid: %d)\n", cnsm->bcnTimer);
                STM_StartTimer(cnsm->bcnTimer, HPGP_TIME_BCN_INTERVAL);   
#else
    //enable the MAC HW to transmit the beacon
#ifdef HPGP_HAL
               ISM_EnableMacIrq(MAC_INT_IRQ_PLC_BCN_TX);
#endif
#endif
               cnsm->state = CNSM_STATE_READY;

                CNSM_BuildBeacon(cnsm, BEACON_TYPE_CENTRAL);

                CNSM_TransmitBeacon(cnsm);

			   if (ccoType != LINKL_CCO_TYPE_HO)
             	  AKM_Start(&linkl->akm, LINKL_STA_MODE_CCO, AKM_NEW_NEK);

                
            }
            break;
        }
        case CNSM_STATE_READY:
        {
            if(event->eventHdr.eventClass == EVENT_CLASS_MSG)
            {
                switch(event->eventHdr.type)
                {
                	case EVENT_TYPE_CM_UNASSOC_STA_IND:
					{
						FM_Printf(FM_MMSG, "CNSM:<<<UNASSOC STA IND(tei:%bu)\n",
                                  hpgpHdr->tei);

						break;

            		}
                	case EVENT_TYPE_NN_INL_REQ:
                	case EVENT_TYPE_NN_INL_CNF:
					{
						sNnINLReq * inlReq = (sNnINLReq*)event->buffDesc.dataptr;
						u8 numSta = CRM_GetScbNum(cnsm->crm);


						if (event->eventHdr.type == EVENT_TYPE_NN_INL_REQ)
						FM_Printf(FM_MMSG, "CNSM:<<<NN_INL_REQ(tei:%bu)\n",
                                  hpgpHdr->tei);
						else
						FM_Printf(FM_MMSG, "CNSM:<<<NN_INL_CNF(tei:%bu)\n",
                                  hpgpHdr->tei);
											   
						if (numSta == inlReq->srcNumAuthSta)
						{							
							//FM_HexDump(FM_ERROR,"m1", staInfo->macAddr, 6);
							//FM_HexDump(FM_ERROR,"m2", hpgpHdr->macAddr, 6);
							
							if (memcmp(staInfo->macAddr, hpgpHdr->macAddr, 6) < 0)
							{
								
								newEvent = EVENT_Alloc(EVENT_DEFAULT_SIZE, 0);
								if(newEvent != NULL)
								{
									//printf("e %bu\n", EVENT_TYPE_NCO_IND);
									newEvent->eventHdr.type = EVENT_TYPE_NCO_IND;
									CTRLL_ReceiveEvent(NULL, newEvent);
								}
							}
							
						}
						else
						if (numSta <= inlReq->srcNumAuthSta)
						{
								newEvent = EVENT_Alloc(EVENT_DEFAULT_SIZE, 0);
								if(newEvent != NULL)
								{
									newEvent->eventHdr.type = EVENT_TYPE_NCO_IND;
									CTRLL_ReceiveEvent(NULL, newEvent);
								}
						}


						if (event->eventHdr.type == EVENT_TYPE_NN_INL_REQ)
						{
								
							CNSM_SendMgmtMsg(cnsm, EVENT_TYPE_NN_INL_CNF,
											inlReq->srcSnid,
											inlReq->srcTei, hpgpHdr->macAddr);
						}
										  
			
						break;

					}


					case EVENT_TYPE_CC_DISCOVER_LIST_CNF:
                    {
#ifdef P8051
                        FM_Printf(FM_MMSG, "CNSM:<<<CC_DISC_LIST.CNF(tei:%bu)\n",
                                hpgpHdr->tei);
#else
                        FM_Printf(FM_MMSG, "CNSM:<<<CC_DISC_LIST.CNF(tei:%d)\n",
                                hpgpHdr->tei);
#endif
                        staScb = hpgpHdr->scb;
                        if(staScb == NULL)
                        {
                            FM_Printf(FM_ERROR, "CNSM:no scb\n");
                            break;
                        }
#if 0						
                        /* reset the discovered sta/net lists in the scb */
                        for (i = 0; i < DISC_STA_LIST_MAX; i ++)
                        {
                            staScb->discStaInfo[i].valid = 0;
                        }

                        for (i = 0; i < DISC_NET_LIST_MAX; i ++)
                        {
                            staScb->discNetInfo[i].valid = 0;
                        }
#endif						
		                staScb->numDiscSta = 0;				
                        staScb->numDiscNet = 0;

                        //update the discovered sta/net lists in the scb
                        //i.e. update the topology table
                        dataptr = event->buffDesc.dataptr;
                        num = *dataptr;   //number of stations
#if 0                
                        if(num > DISC_STA_LIST_MAX)
                        {
                            FM_Printf(FM_ERROR, "CNSM: less resource for discovered sta list in SCB\n");
                        } 
#endif						
                        dataptr++;
                        i = 0; 
#if 0						
                        while(num)
                        {
                            ccDiscStaInfo = (sCcDiscStaInfo *)dataptr; 
                            if(i < DISC_STA_LIST_MAX)
                            {
                                staScb->discStaInfo[i].valid = 1;
                                staScb->discStaInfo[i].hit = 1;
                                memcpy(staScb->discStaInfo[i].macAddr,  
                                    ccDiscStaInfo->macAddr, MAC_ADDR_LEN);
                                staScb->discStaInfo[i].tei = ccDiscStaInfo->tei;
                                staScb->discStaInfo[i].sameNet = ccDiscStaInfo->sameNet;
                                staScb->discStaInfo[i].snid = ccDiscStaInfo->snid;

                                staScb->discStaInfo[i].staCap.byte = ccDiscStaInfo->staCap.byte;

//                                staScb->discStaInfo[i].sigLevel = ccDiscStaInfo->sigLevel;
//                                staScb->discStaInfo[i].avgBle= ccDiscStaInfo->avgBle;
                            }
                            else
                            {
FM_Printf(FM_WARN, "CNSM: size of Rx discover sta list exceeds the max\n");
                                break;
                            }
                            dataptr += sizeof(sCcDiscStaInfo); 
                            i++;
                            num--;
                        }
                        staScb->numDiscSta = i;

#else
						staScb->numDiscSta = num;
#endif
                        num = *dataptr;   //number of net
#if 0                        
                        if(num > DISC_NET_LIST_MAX)
                        {
                            FM_Printf(FM_ERROR, "CNSM: less resource for discovered net list in SCB\n");
                        } 
#endif
                        dataptr++;
                        i = 0; 
#if 0						
                        while(num)
                        {
                            ccDiscNetInfo = (sCcDiscNetInfo *)dataptr; 
                            if(i < DISC_NET_LIST_MAX)
                            {
                                staScb->discNetInfo[i].valid = 1;
                                staScb->discNetInfo[i].hit = 1;
                                memcpy(staScb->discNetInfo[i].nid,  
                                    ccDiscNetInfo->nid, NID_LEN);
                                staScb->discNetInfo[i].snid = ccDiscNetInfo->snid;
                                staScb->discNetInfo[i].hybridMode = ccDiscNetInfo->hybridMode;
                                staScb->discNetInfo[i].numBcnSlots= ccDiscNetInfo->numBcnSlots;
                                staScb->discNetInfo[i].coordStatus= ccDiscNetInfo->coordStatus;
                                staScb->discNetInfo[i].offset= ccDiscNetInfo->offset;
                            }
                            else
                            {
								FM_Printf(FM_WARN, "CNSM:Rx disc exceeds\n");
                                break;
                            }
                            dataptr += sizeof(sCcDiscNetInfo); 
                            i++;
                            num--;
                        }
                        staScb->numDiscNet = i;
#else
						staScb->numDiscNet = num;
#endif
                        break;
                   }
#ifdef UKE				   
                   case EVENT_TYPE_CM_SC_JOIN_REQ: //UKE
                   {
                        FM_Printf(FM_MMSG, "CNSM:<<<CM_SC_JOIN.REQ\n");
                         
                       if(staInfo->secMode == SEC_MODE_SC_ADD)
                       {
                           FM_Printf(FM_MMSG, "SNSM:<<<CM_SC_JOIN.REQ\n");
                         
                           LINKL_SendMgmtMsg(cnsm->staInfo, EVENT_TYPE_CM_SC_JOIN_CNF,
                                        ((sHpgpHdr*)(event->buffDesc.buff))->macAddr);

                           CNAM_EnableAssocNotification(&linkl->ccoNam,
                                                ((sHpgpHdr*)(event->buffDesc.buff))->macAddr);
                               
                       }
                           
                       
                       break;
                    } 
#endif	
                    
#ifdef ROUTE			
                    case EVENT_TYPE_CM_ROUTE_INFO_REQ:
                    {
                        FM_Printf(FM_MMSG, "ROUTE:<<<ROUTE_INFO.REQ(tei:%bu)\n",
                               hpgpHdr->tei);
                        ROUTE_sendRouteInfo(EVENT_TYPE_CM_ROUTE_INFO_CNF, event);
                        break;
                    }
                    case EVENT_TYPE_CM_ROUTE_INFO_CNF:
                    {
                        ROUTE_procRouteInfo((sRouteInfo *)&event->buffDesc.dataptr[1], event->buffDesc.dataptr[0], hpgpHdr->tei);
                        break;
                    }
                    case EVENT_TYPE_CM_ROUTE_INFO_IND:
                    {
                        ROUTE_procRouteInfo((sRouteInfo *)&event->buffDesc.dataptr[1], event->buffDesc.dataptr[0], hpgpHdr->tei);
                        break;
                    }
                    case EVENT_TYPE_CM_UNREACHABLE_IND:
                    {
                        u32 *unreachableNtb = event->buffDesc.dataptr;
                        ROUTE_procUnreachableInd(&event->buffDesc.dataptr[5], event->buffDesc.dataptr[4], 
                            hpgpHdr->tei, (u32)*event->buffDesc.dataptr);
                        //Start HD_Duration timer using diff between unreachableNtb and current ntb
                        break;
                    }
#endif
                   default:
                   {
                   }
               }
           }
           else //control events
           {
               switch(event->eventHdr.type)
               {
                   case EVENT_TYPE_CNSM_START: 
                   {
#ifdef ROUTE
                        dataptr = event->buffDesc.dataptr;
                        ccoType = *dataptr;  
                        if(ccoType == LINKL_CCO_TYPE_ASSOC)
                        {
                            if(cnsm->staInfo->identifyCaps.routingCap == TRUE)
                            {
                                ROUTE_startUpdateTimer();
                            }
                        }
#endif
                   }
                   case EVENT_TYPE_CC_BCN_IND:
                   {
                       rxdesc.snid = hpgpHdr->snid;  
                       bcn = event->buffDesc.dataptr;

                       if( (CNSM_ProcBcnLow(cnsm, &rxdesc, bcn) == BCN_SRC_OTHER_CCO)
                           &&(cnsm->ccoNotification))
                       {
                           newEvent = EVENT_Alloc(EVENT_DEFAULT_SIZE, 0);
                           if(newEvent == NULL)
                           {
                               FM_Printf(FM_ERROR, "EAllocErr\n");
                               return ;
                           }
                           cnsm->ccoNotification = 0; //reset
                           newEvent->eventHdr.type = EVENT_TYPE_NET_DISC_IND;
                           //deliver the event to the upper layer
#ifdef CALLBACK
                           linkl->deliverEvent(linkl->eventcookie, newEvent);
#else
                           CTRLL_ReceiveEvent(linkl->eventcookie, newEvent);
#endif
                       }
                       break;
                   }
                   case EVENT_TYPE_TIMER_DISC_IND: 
                   {
                       //perform the auto-CCo selection if the cco is not an
                       //user-appointed CCo
                       if(!staInfo->ccoScb->staStatus.fields.apptCcoStatus)
                       {
                          // rajan  CNSM_PerformAutoCcoSelection(cnsm);
                       }

/*
                //check alive STA. It may not be needed, as the TEI lease
                //timer provide the same funcation
                scbIter = CRM_GetNextScb(cnsm->crm, scbIter);
                while(scbIter)
                {
                    if( scbIter->aliveCnt++ > HPGP_TIME_STA_AGING_CNT)
                    {
                        //STA is dead, silently remove the STA
                        
                    }
                    scbIter = CRM_GetNextScb(cnam->crm, scbIter);
                }
*/

                       //NOTE: transmit the discover info in the central beacon 
                       //instead of scheduling the beacon transmission for CCO itself
                
#ifdef NSM_CCO_PRINT
                       FM_Printf(FM_HINFO, "CNSM:Add discovery info in beacon\n");
#endif					   
                       cnsm->txDiscBcn = 1;
                       cnsm->bcnUpdate = 1;
/*
                status = CNSM_BuildBeacon(cnsm, BEACON_TYPE_DISCOVER);
                if(status == STATUS_SUCCESS)
                {
                    CNSM_TransmitBeacon(cnsm);
                }
*/

                       //start to schedule the discover beacon
                       //assume that the CCO could schedule all discover beacons
                       //in MaxDiscoverPeriod
                       cnsm->schedDiscBcn = 1;

                       //restart the discovery timer
                      STM_StartTimer(cnsm->discTimer, HPGP_TIME_DISC_PERIOD_MAX);   
 
                       break;
                   }
                   case EVENT_TYPE_TIMER_DISC_AGING_IND:
                   {
#ifdef DISC_BCN
                       SCB_AgeDiscLists(cnsm->staInfo->ccoScb);
#endif
                       STM_StartTimer(cnsm->discAgingTimer, HPGP_TIME_DISC_AGING);   

                       break;
                   }       
#ifdef ROUTE
                   case EVENT_TYPE_ROUTE_UPDATE_TIMEOUT:
                   {
                       ROUTE_sendRouteInfo(EVENT_TYPE_CM_ROUTE_INFO_IND, NULL);
                       ROUTE_startUpdateTimer();
                       break;
                   }
                   case EVENT_TYPE_ROUTE_HD_DURATION_TIMEOUT:
                   {
                       ROUTE_procHdDurationTimeout();
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
        default:
        {
        }
    }
}



eStatus CNSM_Init(sCnsm *cnsm, sLinkLayer *linkl)
{
    //sLinkLayer *linkLayer = HPGPCTRL_GetLayer(HP_LAYER_TYPE_LINK);
    cnsm->linkl = linkl;
    
    cnsm->state = CNSM_STATE_INIT;
#ifdef CALLBACK
    cnsm->discTimer = STM_AllocTimer(LINKL_TimerHandler, 
                          EVENT_TYPE_TIMER_DISC_IND, linkl);
#else
    cnsm->discTimer = STM_AllocTimer(HP_LAYER_TYPE_LINK,
                          EVENT_TYPE_TIMER_DISC_IND, linkl);
#endif

    if(cnsm->discTimer == STM_TIMER_INVALID_ID)
    {
        return STATUS_FAILURE;
    }
#ifdef NSM_CCO_PRINT	
#ifdef P8051
FM_Printf(FM_ERROR, "CNSM:disc timer id:%bu\n", cnsm->discTimer);
#else
FM_Printf(FM_ERROR, "CNSM:disc timer id:%d\n", cnsm->discTimer);
#endif
#endif

#ifdef CALLBACK
    cnsm->discAgingTimer = STM_AllocTimer(LINKL_TimerHandler,
                               EVENT_TYPE_TIMER_DISC_AGING_IND, linkl);
#else
    cnsm->discAgingTimer = STM_AllocTimer(HP_LAYER_TYPE_LINK,
                               EVENT_TYPE_TIMER_DISC_AGING_IND, linkl);
#endif
    if(cnsm->discAgingTimer == STM_TIMER_INVALID_ID)
    {
        return STATUS_FAILURE;
    }
#ifdef NSM_CCO_PRINT	
#ifdef P8051
FM_Printf(FM_ERROR, "CNSM:disc aging timer id:%bu\n", cnsm->discAgingTimer);
#else
FM_Printf(FM_ERROR, "CNSM:disc aging timer id:%d\n", cnsm->discAgingTimer);
#endif
#endif

#ifdef SIMU
    //simulate the beacon tx interrup for beacon interval
#ifdef CALLBACK
    cnsm->bcnTimer = STM_AllocTimer(LINKL_BcnTimerHandler, 
                         EVENT_TYPE_TIMER_BCN_TX_IND, linkl);
#else
    cnsm->bcnTimer = STM_AllocTimer(HP_LAYER_TYPE_LINK,
                         EVENT_TYPE_TIMER_BCN_TX_IND, linkl);
#endif
#ifdef NSM_CCO_PRINT
FM_Printf(FM_ERROR, "CNSM:BCN timer id: %d\n", cnsm->bcnTimer);
#endif
#endif

    ISM_RegisterIntHandler(MAC_INT_IRQ_PLC_BCN_TX, LINKL_BcnTxHandler, linkl);

    cnsm->state = CNSM_STATE_INIT;

    cnsm->staInfo = LINKL_GetStaInfo(linkl);
//    cnsm->ccoInfo = LINKL_GetCcoInfo(linkLayer);
    cnsm->crm = LINKL_GetCrm(linkl);
    cnsm->discScb = NULL;
    cnsm->hoEnabled = 0;
    cnsm->hoReady = 0;
    cnsm->nctei = 0;
    cnsm->ccoNotification = 0;
    cnsm->schedDiscBcn = 0;
    cnsm->bcnUpdate = 0;
    cnsm->bcnUpdateProgress =0;
    cnsm->txDiscBcn = 0;
    cnsm->stopSnam = 0;
    cnsm->hoCntDown = HPGP_HO_COUNTDOWN_MAX;
    /* initial regsions */
    /* minimum CSMA regsion */

    
    CNSM_InitRegion(cnsm, linkl);
        


    return STATUS_SUCCESS;
}
#endif /* CCO_FUNC */

//
#if 0
void SNSM_Start(sSns
{
    switch(staType)
    {
        case LINKL_STA_TYPE_UNASSOC:
        {
            //NOTE: better to send an start event, 
            //instead of setting the state here
            snsm->enableCcoSelection = 1;
            snsm->enableCcoDetection = 1;
            //select PhyClk

            STM_StartTimer(snsm->usttTimer, HPGP_TIME_USAI);   
            STM_StartTimer(snsm->discAgingTimer, HPGP_TIME_DISC_AGING);   
            snsm->state = SNSM_STATE_NET_DISC;
            break;
        }
        case LINKL_STA_TYPE_NETDISC:
        {
            snsm->enableCcoSelection = 0;
            snsm->enableCcoDetection = 1;
            //select PhyClk

            STM_StartTimer(snsm->usttTimer, HPGP_TIME_USAI);   
            STM_StartTimer(snsm->discAgingTimer, HPGP_TIME_DISC_AGING);   
            snsm->state = SNSM_STATE_NET_DISC;
            break;
        }
        case LINKL_STA_TYPE_ASSOC:
        {
            //NOTE: better to send an start event, 
            //instead of setting the state here
            STM_StopTimer(snsm->usttTimer);   
            snsm->enableCcoSelection = 0;
            snsm->enableCcoDetection = 1;
            snsm->state = SNSM_STATE_CONN;
            break;
        }
        case LINKL_STA_TYPE_CONN:
        {
            //NOTE: better to send an start event, 
            //instead of setting the state here
            STM_StopTimer(snsm->usttTimer);   
            snsm->enableCcoSelection = 0;
            snsm->enableCcoDetection = 0;
            snsm->state = SNSM_STATE_CONN;
        }
        default:
        {
        }
    }
}
#endif


/*
void SNSM_StartConn(sSnsm *snsm)
{
    STM_StopTimer(snsm->usttTimer);   
    snsm->state = SNSM_STATE_CONN;
}


void SNSM_LeaveConn(sSnsm *snsm)
{
    snsm->state = SNSM_STATE_INIT;
}
*/


/** =========================================================
 *
 * Edit History
 *
 * $Source: /home/cvsrepo/Hybrii_B_OSFW_Dev/firmware/hpgp/src/link/nsm_cco.c,v $
 *
 * $Log: nsm_cco.c,v $
 * Revision 1.32  2014/09/05 09:28:18  ranjan
 * 1. uppermac cco-sta switching feature fix
 * 2. general stability fixes for many station associtions
 * 3. changed mgmt memory pool for many STA support
 *
 * Revision 1.31  2014/08/25 07:37:34  kiran
 * 1) RSSI & LQI support
 * 2) Fixed Sync related issues
 * 3) Fixed timer 0 timing drift for SDK
 * 4) MMSG & Error Logging in Flash
 *
 * Revision 1.30  2014/08/05 13:12:55  kiran
 * Fixed CP loss issue with UART Host & Peripheral interface
 *
 * Revision 1.29  2014/07/22 10:03:52  kiran
 * 1) SDK Supports Power Save
 * 2) Uart_Driver.c cleanup
 * 3) SDK app memory pool optimization
 * 4) Prints from STM.c are commented
 * 5) Print messages are trimmed as common no memory left in common
 * 6) Prins related to Power save and some other modules are in compilation flags. To enable module specific prints please refer respective module
 *
 * Revision 1.28  2014/07/16 10:47:40  kiran
 * 1) Updated SDK
 * 2) Fixed Diag test in SDK
 * 3) Ethernet and SPI interfaces removed from SDK as common memory is less
 * 4) GPIO access API's added in SDK
 * 5) GV701x chip reset command supported
 * 6) Start network and Join network supported in SDK (Forced CCo and STA)
 * 7) Some bug fixed in SDK (CP free, p app command issue etc.)
 *
 * Revision 1.27  2014/07/11 10:23:37  kiran
 * power save changes
 *
 * Revision 1.26  2014/07/10 11:42:45  prashant
 * power save commands added
 *
 * Revision 1.25  2014/07/05 09:16:27  prashant
 * 100 Devices support- only association tested, memory adjustments
 *
 * Revision 1.24  2014/06/19 17:13:19  ranjan
 * -uppermac fixes for lvnet and reset command for cco and sta mode
 * -backup cco working
 *
 * Revision 1.23  2014/06/19 07:16:02  prashant
 * Region fix, frequency setting fix
 *
 * Revision 1.22  2014/06/13 14:55:11  ranjan
 * -fixing memory issue due to previous checkin
 *
 * Revision 1.21  2014/06/12 13:15:44  ranjan
 * -separated bcn,mgmt,um event pools
 * -fixed datapath issue due to previous checkin
 * -work in progress. neighbour cco detection
 *
 * Revision 1.20  2014/06/11 15:08:28  tri
 * took out debug printf
 *
 * Revision 1.19  2014/06/11 13:17:47  kiran
 * UART as host interface and peripheral interface supported.
 *
 * Revision 1.18  2014/06/05 10:26:07  prashant
 * Host Interface selection isue fix, Ac sync issue fix
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
 * Revision 1.15  2014/05/21 23:02:09  tri
 * more PS
 *
 * Revision 1.14  2014/05/20 05:57:45  prashant
 * persistent schedule code updated
 *
 * Revision 1.13  2014/05/13 20:05:46  tri
 * more PS
 *
 * Revision 1.12  2014/05/12 08:09:57  prashant
 * Route fix, STA sync issue with AV CCO and handling discover bcn issue fix
 *
 * Revision 1.11  2014/04/24 21:51:07  yiming
 * Working Code for Mitsumi
 *
 * Revision 1.10  2014/04/09 21:10:07  tri
 * more PS
 *
 * Revision 1.9  2014/03/20 23:20:46  tri
 * more PS
 *
 * Revision 1.8  2014/03/12 09:41:22  ranjan
 * 1. added ageout event to cco cnam,backupcco ageout handling
 * 2.  fix linking issue in zb_lx51_asic due to backup cco checkin
 *
 * Revision 1.7  2014/03/10 05:58:10  ranjan
 * 1. added HomePlug BackupCCo feature. verified C&I test.(passed.) (bug 176)
 *
 * Revision 1.6  2014/02/27 10:42:47  prashant
 * Routing code added
 *
 * Revision 1.5  2014/02/26 23:16:02  tri
 * more PS code
 *
 * Revision 1.4  2014/02/19 10:22:41  ranjan
 * - common sync for hal_tst and upper mac project
 * - ism.c is MAC interrupt handler for hhal_tst and upper mac.
 *    chal_ext1isr function   is removed
 * - verified : lower mac sync, upper mac sync data traffic.
 *
 * Revision 1.3  2014/01/28 17:45:15  tri
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
 * Revision 1.7  2013/07/12 08:56:36  ranjan
 * -UKE Push Button Security Feature.
 * Verified : DirectEntry Security Works.Datapath Works.
 *                 command SetSecMode for UKE works.
 * Added against bug-160
 *
 * Revision 1.6  2013/04/17 13:00:59  ranjan
 * Added FW ready event, Removed hybrii header from datapath, Modified hybrii header
 *  formate
 *
 * Revision 1.5  2013/03/26 12:07:26  ranjan
 * -added  host sw reset command
 * - fixed issue in bcn update
 *
 * Revision 1.4  2013/03/22 12:21:49  prashant
 * default FM_MASK and FM_Printf modified for USER INFO
 *
 * Revision 1.3  2013/03/14 11:49:18  ranjan
 * 1.handled cases  for CCo toSTA switch and  viceversa
 * 2.UM uses bcntemplate
 *
 * Revision 1.2  2013/02/15 12:53:57  prashant
 * ASSOC.REQ changes for DEVELO
 *
 * Revision 1.1  2012/09/05 00:13:08  mark
 * separate nsm.c into nsm_cco.c and nsm_sta.c,  reason is nsm.c can't be put into bank
 *
 * Revision 1.40  2012/08/25 15:12:01  yuanhua
 * correct the csma region setting in SNSM when a persistent schedule is received
 *
 * Revision 1.39  2012/08/25 05:49:15  yuanhua
 * fix a potential overwriting of region array in SNSM when receive a beacon.
 *
 * Revision 1.38  2012/08/24 04:40:12  yuanhua
 * set initial regions in CNSM for CCO
 *
 * Revision 1.37  2012/08/23 04:06:52  yuanhua
 * made a fix in SNSM for persistent schedule process
 *
 * Revision 1.36  2012/08/20 04:57:35  yuanhua
 * modify the region entry and add persistent schedule entry for beacon
 *
 * Revision 1.35  2012/08/03 04:03:23  kripa
 * *** empty log message ***
 *
 * Revision 1.34  2012/08/01 04:56:02  kripa
 * Fixed the variable reuse within BuildBeacon regions Bentry, that was causing bcn corruption.
 * Committed on the Free edition of March Hare Software CVSNT Client.
 * Upgrade to CVS Suite for more features and support:
 * http://march-hare.com/cvsnt/
 *
 * Revision 1.33  2012/07/31 14:53:34  kripa
 * Initilalizing snsm->hoSwitch = HPGP_HO_SWITCH_NONE ; it fixes a bug that causes STA to begin a Handover process, soon afer initial sync.
 *
 * Revision 1.32  2012/07/30 04:37:55  yuanhua
 * fixed an issue that an event memory could be overwritten in the HAL when the HAL receives a mgmt message.
 *
 * Revision 1.31  2012/07/15 17:31:07  yuanhua
 * (1)fixed a potential memory overwriting in MUXL (2)update prints for 8051.
 *
 * Revision 1.30  2012/07/12 05:47:34  kripa
 * Commenting out Disc Bcn dump, as this could interfere with sync.
 * Committed on the Free edition of March Hare Software CVSNT Client.
 * Upgrade to CVS Suite for more features and support:
 * http://march-hare.com/cvsnt/
 *
 * Revision 1.29  2012/07/08 18:42:20  yuanhua
 * (1)fixed some issues when ctrl layer changes its state from the UCC to ACC. (2) added a event CNSM_START.
 *
 * Revision 1.28  2012/06/30 19:53:58  kripa
 * Commenting out Schedule Bentrys enoding & parsing temporarily.
 * Committed on the Free edition of March Hare Software CVSNT Client.
 * Upgrade to CVS Suite for more features and support:
 * http://march-hare.com/cvsnt/
 *
 * Revision 1.27  2012/06/30 19:47:39  kripa
 * Commenting out Schedule Bentry encoding+parsing temporarily.
 * Committed on the Free edition of March Hare Software CVSNT Client.
 * Upgrade to CVS Suite for more features and support:
 * http://march-hare.com/cvsnt/
 *
 * Revision 1.26  2012/06/29 03:05:31  kripa
 * Commenting out SetCsmaRegions() call temporarily until it has been tested.
 * Passing Linemode argument to setDevMode call.
 * Committed on the Free edition of March Hare Software CVSNT Client.
 * Upgrade to CVS Suite for more features and support:
 * http://march-hare.com/cvsnt/
 *
 * Revision 1.25  2012/06/27 04:28:18  yuanhua
 * added region entry in the beacon.
 *
 * Revision 1.24  2012/06/20 17:57:05  kripa
 * Multiple changes to fix bcn sync. May need review later.
 * Committed on the Free edition of March Hare Software CVSNT Client.
 * Upgrade to CVS Suite for more features and support:
 * http://march-hare.com/cvsnt/
 *
 * Revision 1.23  2012/06/15 04:35:21  yuanhua
 * add a STA type of passive unassoc STA. With this STA type, the device acts as a STA during the network discovery. It performs the network scan for beacons from the CCO, but does not transmit the UNASSOC_STA.IND and does not involve in the CCO selection process. Thus, it joins the existing network.
 *
 * Revision 1.22  2012/06/14 06:51:50  yuanhua
 * still keep discovery aging timer to run after the CCO is found.
 *
 * Revision 1.21  2012/06/14 06:45:06  yuanhua
 * stop the periodic timers after the STA finds the CCo during the network discovery, as no need to transmit an unassociated_sta.ind if the CCO is detected.
 *
 * Revision 1.20  2012/06/14 06:14:47  yuanhua
 * (1) remove the net scan when the device is set to the CCO mode(2) start the net scan when the CCO is found, but the STA is not sync with CCO yet.
 *
 * Revision 1.19  2012/06/13 16:10:06  son
 * Commenting out ScanNet() call in Network Discovery state - so that unassocSta.Ind messages can be sent.
 * Committed on the Free edition of March Hare Software CVSNT Client.
 * Upgrade to CVS Suite for more features and support:
 * http://march-hare.com/cvsnt/
 *
 * Revision 1.18  2012/06/13 06:24:31  yuanhua
 * add code for tx bcn interrupt handler integration and data structures for region entry schedule. But they are not in execution yet.
 *
 * Revision 1.17  2012/06/11 14:49:59  yuanhua
 * changed the HAL back to the STA mode after finding the CCo during the network discovery so that the CTRLL issues an association request in the HAL STA mode.
 *
 * Revision 1.16  2012/06/11 06:29:01  yuanhua
 * changed HAL_SetBpsto to HAL_SyncNet.
 *
 * Revision 1.15  2012/06/08 05:50:57  yuanhua
 * added snid function.
 *
 * Revision 1.14  2012/06/05 22:37:12  son
 * UART console does not get initialized due to task ID changed
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
 * Revision 1.8  2012/05/01 04:51:09  yuanhua
 * added compiler flags STA_FUNC and CCO_FUNC in link and ctrl layers.
 *
 * Revision 1.7  2012/05/01 00:18:47  son
 * Added _CCO_FUNC_ and _STA_FUNC_ compiler flags
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
 * Revision 1.12  2011/09/06 05:01:46  yuanhua
 * Made a fix such that the STA continues periodic TEI renew after CCo handover.
 *
 * Revision 1.11  2011/08/12 23:13:22  yuanhua
 * (1)Added Control Layer (2) Fixed bugs for user-selected CCo handover (3) Made changes to SNAM/CNAM and SNSM/CNSM for CCo handover switch (from CCo to STA, from STA to CCo, and from STA to STA but with different CCo) and post CCo handover
 *
 * Revision 1.10  2011/08/09 22:45:44  yuanhua
 * changed to event structure, seperating HPGP-related events from the general event defination so that the general event could be used for other purposes than the HPGP.
 *
 * Revision 1.9  2011/08/08 22:05:41  yuanhua
 * user-selected CCo handover fix
 *
 * Revision 1.8  2011/08/05 17:06:29  yuanhua
 * (1) added an internal queue in Link Layer for communication btw modules within Link Layer (2) Fixed bugs in CCo Handover. Now, CCo handover could be triggered by auto CCo selection, CCo handover messages work fine (3) Made some modifications in SHAL.
 *
 * Revision 1.7  2011/08/02 16:06:00  yuanhua
 * (1) Fixed a bug in STM (2) Made STA discovery work according to the standard, including aging timer. (3) release the resource after the STA leave (4) STA will switch to the backup CCo if the CCo failure occurs (5) At this point, the CCo could work with multiple STAs correctly, including CCo association/leave, TEI renew, TEI map updating, discovery beacon scheduling, discovery STA list updating ang aging, CCo failure, etc.
 *
 * Revision 1.6  2011/07/30 02:43:35  yuanhua
 * (1) Split the beacon process into two parts: one requiring an immdiate response, the other tolerating the delay (2) Changed the API btw the MUX and SHAL for packet reception (3) Fixed bugs in various modules. Now, multiple STAs could successfully associate/leave the CCo
 *
 * Revision 1.5  2011/07/22 18:51:05  yuanhua
 * (1) added the hardware timer tick simulation (hts.h/hts.c) (2) Added semaphors for sync in simulation and defined macro for critical section (3) Fixed some bugs in list.h and CRM (4) Modified and fixed bugs in SHAL (5) Changed SNSM/CNSM and SNAM/CNAM for bug fix (6) Added debugging prints, and more.
 *
 * Revision 1.4  2011/07/16 17:11:23  yuanhua
 * (1)Implemented SHOM and CHOM modules, including handover procedure, SCB resource updating for HO (2) Update SNAM and CNAM modules to support uer-appointed CCo handover (3) Made the SCB resources to support the TEI MAP for the STA mode and management of associated STA resources (e.g. TEI) (4) Modified SNSM and CNSM to perform all types of handover switch (CCo handover to the new STA, STA taking over the CCo, STA switching to the new CCo)
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
 * Revision 1.3  2011/04/23 19:48:45  kripa
 * Fixing stm.h and event.h inclusion, using relative paths to avoid conflict with windows system header files.
 *
 * Revision 1.2  2011/04/23 17:35:36  kripa
 * Used relative path for inclusion of stm.h, to avoid conflict with a system header file in VC.
 *
 * Revision 1.1  2011/04/08 21:42:45  yuanhua
 * Framework
 *
 *
 * =========================================================*/

