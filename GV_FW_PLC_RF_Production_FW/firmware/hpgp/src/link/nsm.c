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
#include "linkl.h"
#include "nsm.h"
#include "nam.h"
#include "muxl.h"
#include "hpgpapi.h"
#include "hpgpconf.h"
#include "papdef.h"
#include "fm.h"
#include "ism.h"
#include "hpgpevt.h"
#include "mmsg.h"
#include "../../../common/stm.h"
#include "hal.h"
#ifdef HPGP_HAL
#include "hal_hpgp.h"
#else
#include "sdrv.h"
#endif

#ifndef CALLBACK
#include "hpgpapi.h"
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

extern void LINKL_TimerHandler(u16 type, void *cookie);
void CNSM_Stop(sCnsm *cnsm);
void SCB_UpdateDiscNetList(sScb *scb, sDiscNetInfoRef *discNetInfoRef);
void SCB_UpdateDiscStaList(sScb *scb, sDiscStaInfoRef *discStaInfoRef);
void SCB_AgeDiscLists(sScb *scb);

static u8  bcAddr[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};



void SCB_UpdateDiscStaList(sScb *scb, sDiscStaInfoRef *discStaInfoRef)
{
    u8 i;
    u8 new, k;

    //search through the discovered STA list 
    //always update the STA information whether or not it is new or old.
    new = 0;
    
    k = DISC_STA_LIST_MAX;
    for(i = 0; i < DISC_STA_LIST_MAX; i++)
    {
        if(scb->discStaInfo[i].valid == TRUE)
        {
            if(memcmp(scb->discStaInfo[i].macAddr, discStaInfoRef->macAddr, 
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
            scb->discStaInfo[k].valid = TRUE;
            memcpy(scb->discStaInfo[k].macAddr, discStaInfoRef->macAddr, MAC_ADDR_LEN);
            scb->numDiscSta++;
        }         
//        memcpy(scb->discStaInfo[k].nid, discStaInfoRef->nid, NID_LEN-1);
//        scb->discStaInfo[k].nid[NID_LEN-1] = discStaInfoRef->nid[NID_LEN-1]&0x3F;
        scb->discStaInfo[k].tei = discStaInfoRef->tei;
        scb->discStaInfo[k].staCap.byte =  discStaInfoRef->discInfo->staCap.byte;

        scb->discStaInfo[k].sameNet = discStaInfoRef->sameNet;

        scb->discStaInfo[k].hit = 1;
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
    k = DISC_NET_LIST_MAX;
    for(i = 0; i < DISC_NET_LIST_MAX; i++)
    {
        if(scb->discNetInfo[i].valid == TRUE)
        {
            if( (memcmp(scb->discNetInfo[i].nid, discNetInfoRef->nid, NID_LEN-1) == 0) &&
                ((scb->discNetInfo[i].nid[NID_LEN-1]&0x3F) == (discNetInfoRef->nid[NID_LEN-1]&0x3F)) )
            {
                //already in the list. no update
                scb->discNetInfo[k].hit = 1;
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
        memcpy(scb->discNetInfo[k].nid, discNetInfoRef->nid, NID_LEN-1);
        scb->discNetInfo[k].nid[NID_LEN-1] = discNetInfoRef->nid[NID_LEN-1]&0x3F;
//        memcpy(snsm->discNetInfo[k].bpsto, discNetInfoRef->bpsto, 3);
        scb->discNetInfo[k].netMode = discNetInfoRef->netMode;
        scb->discNetInfo[k].hybridMode = discNetInfoRef->hybridMode;
        scb->discNetInfo[k].numBcnSlots = discNetInfoRef->numBcnSlots;
		scb->discNetInfo[k].valid = 1;
        scb->discNetInfo[k].hit = 1;
        scb->discUpdate = 1;
    }
    else
    {
        FM_Printf(FM_WARN, "SNSM: Discovered Net List is full.\n");
    }

}


void SCB_AgeDiscLists(sScb *scb)
{
    u8 i;
    //scb = snsm->staInfo->staScb;
    for(i = 0; i < DISC_STA_LIST_MAX; i++)
    {
        if(scb->discStaInfo[i].valid == TRUE)
        {
            if(scb->discStaInfo[i].hit == 1)
            {
                scb->discStaInfo[i].hit = 0;
            }
            else
            {
#ifdef P8051
             FM_Printf(FM_MINFO, "SCB: age out a discovery entry (tei: %bu).\n",
                                  scb->discStaInfo[i].tei);
#else
             FM_Printf(FM_MINFO, "SCB: age out a discovery entry (tei: %d).\n",
                                  scb->discStaInfo[i].tei);
#endif
#ifndef MPER  //[YM] temporary comment out for Mitsumi test
                //remove the entry from the list
                memset(&scb->discStaInfo[i], 0, sizeof(sDiscStaInfo));
                scb->discUpdate = 1;
                scb->numDiscSta--;
#endif				
            }
        }
    }

    for(i = 0; i < DISC_NET_LIST_MAX; i++)
    {
        if(scb->discNetInfo[i].valid == TRUE)
        {
            if(scb->discNetInfo[i].hit == 1)
            {
                scb->discNetInfo[i].hit = 0;
            }
            else
            {
#if 1  //[YM] temporary comment out for Mitsumi test
                //remove the entry from the list
                memset(&scb->discNetInfo[i], 0, sizeof(sDiscNetInfo));
                scb->discUpdate = 1;
                scb->numDiscNet--;
#endif				
            }
        }
    }
}

#ifdef CCO_FUNC

/* ========================== 
 * CCO  network system manager
 * ========================== */


eStatus CNSM_SendMgmtMsg(sCnsm *cnsm, u16 mmType, sScb *dstScb)
{
    eStatus         status = STATUS_SUCCESS;
    sEvent         *newEvent = NULL;
    sHpgpHdr       *hpgpHdr = NULL;
    //send the CC_DISCOVER_LIST.REQ 
    //to query the discovered sta/network list
    //for the topology table
    newEvent = EVENT_Alloc(HPGP_DATA_PAYLOAD_MIN, EVENT_HPGP_MSG_HEADROOM);
    if(newEvent == NULL)
    {
        FM_Printf(FM_ERROR, "Cannot allocate an event.\n");
        return STATUS_FAILURE;
    }
    newEvent->eventHdr.eventClass = EVENT_CLASS_MSG;
    newEvent->eventHdr.type = mmType;
//                        newEvent->eventHdr.tei = event->eventHdr.tei;
    hpgpHdr = (sHpgpHdr *)newEvent->buffDesc.buff;
    hpgpHdr->tei = dstScb->tei; 
    hpgpHdr->macAddr = dstScb->macAddr;
    /* TODO: based on the each station encryption status, set the EKS */
    hpgpHdr->eks = HPGP_EKS_NONE;
                       
    if (mmType == EVENT_TYPE_CC_DISCOVER_LIST_REQ)
    {
#ifdef P8051
        FM_Printf(FM_MMSG, "CNSM: >>> CC_DISCOVER_LIST.REQ (tei: %bu).\n",
                            hpgpHdr->tei);
#else
        FM_Printf(FM_MMSG, "CNSM: >>> CC_DISCOVER_LIST.REQ (tei: %d).\n",
                            hpgpHdr->tei);
#endif
    }
    newEvent->buffDesc.datalen = HPGP_DATA_PAYLOAD_MIN;
    //transmit CM_UNASSOCIATED_STA_IND in the MNBC
    status = MUXL_TransmitMgmtMsg(newEvent);
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
            sai[k].duration = region[j].duration;
            k++;
        }
        else if (region[j].regionType == REGION_TYPE_LOCAL_CSMA)
        {
            /* it is assumed that the contiguous regions should have
             * different region type */
            sai[k].stpf = 1;
            sai[k].glid = HPGP_GLID_LOCAL_CSMA;
            sai[k].startTime = region[j].startTime;
            sai[k].duration = region[j].duration;
            k++;
        }
        endTime = region[j].startTime + region[j].duration;
        j++;
    }
    cnsm->saiTable[schedInd].saiNum = k;
    cnsm->saiTable[schedInd].pscd = pscd;
    cnsm->saiTable[schedInd].cscd = cscd;

for (j=0; j<k; j++) {
FM_Printf(FM_HINFO, "CNSM: sai %bu \n", j);
FM_Printf(FM_HINFO, "stpf %bu, glid %bu, start: 0x%x, duration: 0x%x\n",
sai[j].stpf, sai[j].glid, sai[j].startTime, sai[j].duration);
}

}




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

    bcnHdr->nid[NID_LEN-1] &= 0x3F;

    if((memcmp(staInfo->nid, bcnHdr->nid, NID_LEN) == 0))
    {
        if( (bcnHdr->bt == BEACON_TYPE_CENTRAL)||
            (bcnHdr->bt == BEACON_TYPE_PROXY))
        {
            //it should not occur
            FM_Printf(FM_MMSG|FM_LINFO, "CNSM: <<< CENTRAL/PROXY BEACON (L).\n");
            bcnsrc = BCN_SRC_CCO;
            return bcnsrc; 
        }
        else
        {
#ifdef P8051
            FM_Printf(FM_MMSG|FM_MINFO, "CNSM: <<< DISCOVERY BEACON (L) (tei: %bu).\n", bcnHdr->stei);
#else
            FM_Printf(FM_MMSG|FM_MINFO, "CNSM: <<< DISCOVERY BEACON (L) (tei: %d).\n", bcnHdr->stei);
#endif
            bcnsrc = BCN_SRC_DISC;
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
                        nextRegion[k].duration = duration;
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
                            nextRegion[k].duration = HPGP_REGION_MIN_SHARED_CSMA;
                            nextRegion[k].regionType = REGION_TYPE_SHARED_CSMA;
                            k++;
                            nextRegion[k].startTime = nextRegion[k-1].startTime + HPGP_REGION_MIN_SHARED_CSMA;
                            nextRegion[k].duration = duration - HPGP_REGION_MIN_SHARED_CSMA;
                            nextRegion[k].regionType = REGION_TYPE_LOCAL_CSMA;
                        }
                        else
                        {
                            nextRegion[k].regionType = REGION_TYPE_SHARED_CSMA;
                            nextRegion[k].duration = duration;
                        }
                    }
                    else
                    {
                        /* to provide passive coordination to a CCO 
                         * in coordinated mode, specify a Stayout region 
                         * in all regions other than CSMA regions */
                        nextRegion[k].regionType = REGION_TYPE_STAYOUT;
                        nextRegion[k].duration = duration;
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
                        (nextRegion[j].duration != currRegion[j].duration) )
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
                }
                
                scb = CRM_GetScb(cnsm->crm, bcnHdr->stei);
                if( memcmp(macAddr, scb->macAddr, MAC_ADDR_LEN) )
                {
                    FM_Printf(FM_WARN, "CNSM: TEI and MAC address are not matched in Discover Beacon.\n");
                    break;
                }
                if(scb)
                {
                    //update the discovery info for the sta
                    scb->staCap.byte = discInfoEntry->staCap.byte; 
                    scb->staStatus = discInfoEntry->staStatus; 

                    //scb->numDiscSta = discInfoEntry->numDiscSta; 
                    //scb->numDiscNet = discInfoEntry->numDiscNet;  
                    //scb = CRM_GetScb(cnsm->crm, bcnHdr->stei);

                    if(discInfoEntry->staCap.fields.update)
                    {
                        reqDiscList = 1;
                        //send the CC_DISCOVER_LIST.REQ 
                        //to query the discovered sta/network list
                        //for the topology table
                        CNSM_SendMgmtMsg(cnsm, EVENT_TYPE_CC_DISCOVER_LIST_REQ,
                                         scb);
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
                        status = MUXL_TransmitMgmtMsg(newEvent);
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

    bcnHdr->nid[NID_LEN-1] &= 0x3F;

    if((memcmp(staInfo->nid, bcnHdr->nid, NID_LEN) == 0))
    {
        if( (bcnHdr->bt == BEACON_TYPE_CENTRAL)||
            (bcnHdr->bt == BEACON_TYPE_PROXY))
        {
            //it should not occur
            FM_Printf(FM_MMSG|FM_LINFO, "CNSM: <<< CENTRAL/PROXY BEACON (H).\n");
            bcnsrc = BCN_SRC_CCO;
            return; 
        }
        else
        {
#ifdef P8051
            FM_Printf(FM_MMSG|FM_MINFO, "CNSM: <<< DISCOVERY BEACON (H) (tei: %bu).\n", bcnHdr->stei);
#else
            FM_Printf(FM_MMSG|FM_MINFO, "CNSM: <<< DISCOVERY BEACON (H) (tei: %d).\n", bcnHdr->stei);
#endif
            bcnsrc = BCN_SRC_DISC;
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
            FM_Printf(FM_ERROR, "Cannot allocate an event.\n");
            return ;
        }
        newEvent->eventHdr.eventClass = EVENT_CLASS_CTRL;
        newEvent->eventHdr.type = EVENT_TYPE_CCO_SELECT_IND;

        hpgpHdr = (sHpgpHdr *)newEvent->buffDesc.buff;
        hpgpHdr->scb = scb;
FM_Printf(FM_ERROR, "CNSM: send a cco selected ind.\n"); 
        //LINKL_SendEvent(linkl, newEvent);
        SLIST_Put(&linkl->intEventQueue, &newEvent->link);
    }
    return;
}




//Start CCo mode
eStatus CNSM_Start(sCnsm *cnsm, u8 ccoType)
{
    sLinkLayer *linkl = (sLinkLayer *)HPGPCTRL_GetLayer(HP_LAYER_TYPE_LINK);
    sEvent *event = NULL;
    u8 *pos = NULL;
    event = EVENT_Alloc(1, EVENT_HPGP_CTRL_HEADROOM);
    if(event == NULL)
    {
        FM_Printf(FM_ERROR, "Cannot allocate an event.\n");
        return STATUS_FAILURE;
    }
    event->eventHdr.eventClass = EVENT_CLASS_CTRL;
    event->eventHdr.type = EVENT_TYPE_CNSM_START;
    pos = event->buffDesc.dataptr;
    *pos = ccoType; 
 
    event->buffDesc.datalen = 1;
FM_Printf(FM_ERROR, "CNSM: Start (%bu).\n", ccoType);
    LINKL_SendEvent(linkl, event);
    return STATUS_SUCCESS;
}


void CNSM_Stop(sCnsm *cnsm)
{
    cnsm->hoCntDown = HPGP_HO_COUNTDOWN_MAX;
    cnsm->hoEnabled = 0;
    cnsm->hoReady = 0;
    cnsm->nctei = 0;

    
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


    FM_Printf(FM_LINFO, "CNSM: Build a Beacon payload.\n");

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
    bcnHdr->nid[NID_LEN-1] = (bcnHdr->nid[NID_LEN-1]&0x3F)|(staInfo->hm <<6);
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
                //HHAL_SetCsmaRegions(cnsm->regionTable[schedInd].regoin, 
                //    cnsm->regionTable[schedInd].regionNum);
#endif
                    }
                    else
                    {
                        /* PSCD counts down */
                        cnsm->saiTable[schedInd].pscd--;
                    }
                }

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
				    endTime += region[j].duration;
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
 FM_Printf(FM_HINFO, "CNSM: Schedule a discover beacon (tei: %bu).\n", 
                     cnsm->discScb->tei);
#else
 FM_Printf(FM_HINFO, "CNSM: Schedule a discover beacon (tei: %d).\n", 
                     cnsm->discScb->tei);
#endif
                            beHdr->beType = BEHDR_DISCOVER;
                            dataptr += sizeof(sBeHdr); 
                            *dataptr = cnsm->discScb->tei;
                            beLen = 1;
                        }
                    }
                    else
                    {
                        //all STAs have scheduled so far
                        cnsm->schedDiscBcn = 0;
                    }
                }
                break;
            }
            case BEHDR_DISC_INFO:
            {
//               if(bcnType == BEACON_TYPE_DISCOVER)
               if(cnsm->txDiscBcn)
               {
                    cnsm->txDiscBcn = 0;
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
                //build power save entry
                //beHdr->beType = BEHDR_PWR_SAVE;
                //dataptr += sizeof(sBeHdr); 
                
                break;
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

    FM_Printf(FM_LINFO, "CNSM: Transmit a Beacon.\n");

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

        FM_Printf(FM_HINFO, "CNSM: HO switch (become assoc STA).\n");
        //send event CCO_HO_IND to the ctrl       
        event = EVENT_Alloc(EVENT_DEFAULT_SIZE, 0);
        if(event == NULL)
        {
            FM_Printf(FM_ERROR, "Cannot allocate an event.\n");
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
            FM_Printf(FM_ERROR, "CNSM: cannot find the cco scb.\n");
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
            FM_Printf(FM_MINFO, "CNSM: HO countdown (%d).\n", cnsm->hoCntDown);
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

                    /* Send CC_DISCOVER_LIST.REQ to each STA mainly due to handover */
                    staScb = CRM_GetNextScb(cnsm->crm, staScb);
                    while(staScb)
                    {
                        if(staScb->tei != cnsm->staInfo->ccoScb->tei)
                        {
                            CNSM_SendMgmtMsg(cnsm, EVENT_TYPE_CC_DISCOVER_LIST_REQ,
                                 staScb);
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

 


                STM_StartTimer(cnsm->discTimer, HPGP_TIME_DISC_PERIOD_MAX);   
                STM_StartTimer(cnsm->discAgingTimer, HPGP_TIME_DISC_AGING);
#ifdef P8051   
FM_Printf(FM_ERROR, "CNSM: Start disc timer (tid: %bu).\n", cnsm->discTimer);
FM_Printf(FM_ERROR, "CNSM: Start disc aging timer (tid: %bu).\n", 
         cnsm->discAgingTimer);
#else
FM_Printf(FM_ERROR, "CNSM: Start disc timer (tid: %d).\n", cnsm->discTimer);
FM_Printf(FM_ERROR, "CNSM: Start disc aging timer (tid: %d).\n", 
         cnsm->discAgingTimer);
#endif

    
#ifdef SIMU
FM_Printf(FM_ERROR, "CNSM: Start beacon timer (tid: %d).\n", cnsm->bcnTimer);
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
#ifdef AKM

                AKM_Start(&linkl->akm, LINKL_STA_MODE_CCO, AKM_NEW_NEK);
#endif                

            }
            break;
        }
        case CNSM_STATE_READY:
        {
            if(event->eventHdr.eventClass == EVENT_CLASS_MSG)
            {
                switch(event->eventHdr.type)
                {
                    case EVENT_TYPE_CC_DISCOVER_LIST_CNF:
                    {
#ifdef P8051
                        FM_Printf(FM_MMSG, "CNSM: <<< CC_DISCOVER_LIST.CNF.(tei: %bu).\n",
                                hpgpHdr->tei);
#else
                        FM_Printf(FM_MMSG, "CNSM: <<< CC_DISCOVER_LIST.CNF.(tei: %d).\n",
                                hpgpHdr->tei);
#endif
                        staScb = hpgpHdr->scb;
                        if(staScb == NULL)
                        {
                            FM_Printf(FM_ERROR, "CNSM: no scb in the event.\n");
                            break;
                        }
                        /* reset the discovered sta/net lists in the scb */
                        for (i = 0; i < DISC_STA_LIST_MAX; i ++)
                        {
                            staScb->discStaInfo[i].valid = 0;
                        }
                        staScb->numDiscSta = 0;
                        for (i = 0; i < DISC_NET_LIST_MAX; i ++)
                        {
                            staScb->discNetInfo[i].valid = 0;
                        }
                        staScb->numDiscNet = 0;

                        //update the discovered sta/net lists in the scb
                        //i.e. update the topology table
                        dataptr = event->buffDesc.dataptr;
                        num = *dataptr;   //number of stations
                 
                        if(num > DISC_STA_LIST_MAX)
                        {
                            FM_Printf(FM_ERROR, "CNSM: less resource for discovered sta list in SCB.\n");
                        } 
                        dataptr++;
                        i = 0; 
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
FM_Printf(FM_WARN, "CNSM: size of Rx discover sta list exceeds the maximu.\n");
                                break;
                            }
                            dataptr += sizeof(sCcDiscStaInfo); 
                            i++;
                            num--;
                        }
                        staScb->numDiscSta = i;

                        num = *dataptr;   //number of net
                        if(num > DISC_NET_LIST_MAX)
                        {
                            FM_Printf(FM_ERROR, "CNSM: less resource for discovered net list in SCB.\n");
                        } 
                        dataptr++;
                        i = 0; 
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
FM_Printf(FM_WARN, "CNSM: size of Rx discover net list exceeds the maximu.\n");
                                break;
                            }
                            dataptr += sizeof(sCcDiscNetInfo); 
                            i++;
                            num--;
                        }
                        staScb->numDiscNet = i;
                        break;
                   }
                   default:
                   {
                   }
               }
           }
           else //control events
           {
               switch(event->eventHdr.type)
               {
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
                               FM_Printf(FM_ERROR, "Cannot allocate an event.\n");
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
                           CNSM_PerformAutoCcoSelection(cnsm);
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
                

                       FM_Printf(FM_LINFO, "CNSM: Add discovery info in beacon.\n");
                       cnsm->txDiscBcn = 1;
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
                       SCB_AgeDiscLists(cnsm->staInfo->ccoScb);
                       STM_StartTimer(cnsm->discAgingTimer, HPGP_TIME_DISC_AGING);   

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



eStatus CNSM_Init(sCnsm *cnsm, sLinkLayer *linkl)
{
    sCsmaRegion *region = NULL;
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
#ifdef P8051
FM_Printf(FM_ERROR, "CNSM: disc timer id: %bu.\n", cnsm->discTimer);
#else
FM_Printf(FM_ERROR, "CNSM: disc timer id: %d.\n", cnsm->discTimer);
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
#ifdef P8051
FM_Printf(FM_ERROR, "CNSM: disc aging timer id: %bu.\n", cnsm->discAgingTimer);
#else
FM_Printf(FM_ERROR, "CNSM: disc aging timer id: %d.\n", cnsm->discAgingTimer);
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
FM_Printf(FM_ERROR, "CNSM: BCN timer id: %d.\n", cnsm->bcnTimer);
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
    cnsm->txDiscBcn = 0;
    cnsm->stopSnam = 0;
    cnsm->hoCntDown = HPGP_HO_COUNTDOWN_MAX;
    /* initial regsions */
    cnsm->currSchedInd = 0;
    region = cnsm->regionTable[0].region;
    memset(region, 0, HPGP_REGION_MAX);
    /* minimum CSMA regsion */

    region[0].startTime = 0;
#if 1
    region[0].duration = 0x2FF; /* in ATU  ( usec) */
    region[0].regionType = REGION_TYPE_SHARED_CSMA;
    region[0].bcnRegion = 0;
    region[0].hybridMd  = 1;
    if (gHpgpHalCB.lineMode == LINE_MODE_DC)
    {
        region[1].startTime = 0x2FF;
        region[1].duration  = 0x6A1;
        region[1].bcnRegion  = 0;
        region[1].regionType = REGION_TYPE_LOCAL_CSMA;
		region[1].hybridMd   = 0;
        region[2].startTime = 0x9A0; /* 0x2FF + 0x6A1 */
        region[2].duration  = 0x5A3;
        region[2].bcnRegion  = 0;
        region[2].regionType = REGION_TYPE_STAYOUT;
		region[2].hybridMd   = 0;

    }
    else
    {
        /* AC mode */
        region[1].startTime = 0x2FF;
        region[1].duration  = 0x4A1;
        region[1].bcnRegion  = 0;
        region[1].regionType = REGION_TYPE_LOCAL_CSMA;
        region[2].startTime = 0x7A0;
        region[2].duration  = 0x5A3;
        region[2].bcnRegion  = 0;
        region[2].regionType = REGION_TYPE_STAYOUT;
    }
    cnsm->regionTable[0].regionNum = 3;
#else
	region[0].duration = 0xA00; /* in ATU */
	region[0].regionType = REGION_TYPE_SHARED_CSMA;
    cnsm->regionTable[0].regionNum = 1;
#endif	
    /* update non persistent schedule */
    CNSM_UpdateSched(cnsm, cnsm->currSchedInd, 0, 4);
    cnsm->updateSched = 0;
#ifdef HPGP_HAL
//    HHAL_SetCsmaRegions(cnsm->regionTable[0].region, 
//        cnsm->regionTable[0].regionNum);
#endif

    return STATUS_SUCCESS;
}
#endif /* CCO_FUNC */

#ifdef STA_FUNC
/* ========================== 
 * STA  network system manager
 * ========================== */
void SNSM_UpdateUaStaList(sSnsm *snsm, sCmUaStaInd *uaStaInfo, u8 *macAddr)
{
    u8 i;
    u8 k = UA_STA_LIST_MAX;
    for(i = 0; i < UA_STA_LIST_MAX; i++)
    {
        if(snsm->uaStaInfo[i].valid == TRUE)
        {
            if( (memcmp(snsm->uaStaInfo[i].nid, uaStaInfo->nid, NID_LEN) == 0)&&
                (memcmp(snsm->uaStaInfo[i].macAddr, macAddr, MAC_ADDR_LEN) == 0))
            {
                snsm->uaStaInfo[i].hit = 1;
                return;
            }
        }
        else
        {
            k = i;
        }
    }

    if(k < UA_STA_LIST_MAX)
    {
        snsm->uaStaInfo[k].valid = 1;
        snsm->uaStaInfo[k].hit = 1;
        memcpy(snsm->uaStaInfo[k].nid, uaStaInfo->nid, NID_LEN);
        memcpy(snsm->uaStaInfo[k].macAddr, macAddr, MAC_ADDR_LEN);
        snsm->uaStaInfo[k].ccoCap = uaStaInfo->ccoCap;
    }
}
            

void SNSM_UpdateAvlnList(sSnsm *snsm, sAvlnInfoRef *avlnInfo)
{
    u8 i;
    u8 k;
    //search through the AVLN list 
    k = AVLN_LIST_MAX;
    for(i = 0; i < AVLN_LIST_MAX; i++)
    {
        if(snsm->avlnInfo[i].valid == TRUE)
        {
            if((memcmp(snsm->avlnInfo[i].nid, avlnInfo->nid, NID_LEN-1) == 0) &&
               ((snsm->avlnInfo[i].nid[NID_LEN-1]&0x3F) == (avlnInfo->nid[NID_LEN-1]&0x3F)))
            {
                //found the existing AVLN
                snsm->avlnInfo[i].hit = 1;
                return;
            }
        }
        else
        {
            k = i;  
        }
    }
    
    if(k < AVLN_LIST_MAX)
    {
        //found a new AVLN
        snsm->avlnInfo[k].valid = TRUE;
        memcpy(snsm->avlnInfo[k].nid, avlnInfo->nid, NID_LEN-1);
        snsm->avlnInfo[k].nid[NID_LEN-1] = avlnInfo->nid[NID_LEN-1]&0x3F;
        snsm->avlnInfo[k].hit = 1;
        snsm->numAvln++;

    }
    else
    {
        FM_Printf(FM_WARN, "SNSM: AVLN List is full.\n");
    }
}



//aging the discovered STA and network lists
void SNSM_PerformAging(sSnsm *snsm)
{
    u8 i;
//    sLinkLayer *linkl = NULL;
//    sEvent *event = NULL;
//    sScb   *scb = NULL;

    if(snsm->state ==  SNSM_STATE_NET_DISC)
    {
        for(i = 0; i < UA_STA_LIST_MAX; i++)
        {
            if(snsm->uaStaInfo[i].valid == TRUE)
            {
                if(snsm->uaStaInfo[i].hit == 1)
                {
                    snsm->uaStaInfo[i].hit = 0;
                }
                else
                {
                   //remove the entry from the list
                   memset(&snsm->uaStaInfo[i], 0, sizeof(sUaStaInfo));
                   // reduce uasta count
                   snsm->numUaSta--; 
                }
            }
        }

        for(i = 0; i < AVLN_LIST_MAX; i++)
        {
            if(snsm->avlnInfo[i].valid == TRUE)
            {
                if(snsm->avlnInfo[i].hit == 1)
                {
                    snsm->avlnInfo[i].hit = 0;
                }
                else
                {
                   //remove the entry from the list
                   memset(&snsm->avlnInfo[i], 0, sizeof(sAvlnInfo));
                   // reduce avln count
                   snsm->numAvln--;
                }
            }
        }
        
        
    }

/*
    if(snsm->state ==  SNSM_STATE_CONN)
    {
        scb = snsm->staInfo->staScb;
        SCB_AgeDiscLists(scb);
        for(i = 0; i < DISC_STA_LIST_MAX; i++)
        {
            if(scb->discStaInfo[i].valid == TRUE)
            {
                if(scb->discStaInfo[i].hit == 1)
                {
                    scb->discStaInfo[i].hit = 0;
                }
                else
                {
FM_Printf(FM_ERROR, "SNSM: age out a discovery entry (tei: %d).\n",
                    scb->discStaInfo[i].tei);
                   //remove the entry from the list
                   memset(&scb->discStaInfo[i], 0, sizeof(sDiscStaInfo));
                   scb->discUpdate = 1;
                   scb->numDiscSta--;
                }
            }
        }

        for(i = 0; i < DISC_NET_LIST_MAX; i++)
        {
            if(scb->discNetInfo[i].valid == TRUE)
            {
                if(scb->discNetInfo[i].hit == 1)
                {
                    scb->discNetInfo[i].hit = 0;
                }
                else
                {
                   //remove the entry from the list
                   memset(&scb->discNetInfo[i], 0, sizeof(sDiscNetInfo));
                   scb->discUpdate = 1;
                   scb->numDiscNet--;
                }
            }
        }

        //NOTE: noBcn is reset after the central beacon is received
        snsm->noBcn++; 

        if(snsm->noBcn > NO_BCN_MAX)
        {
            //send event CCO_SEL_IND to the ctrl        
            event = EVENT_Alloc(EVENT_DEFAULT_SIZE, 0);
            if(event == NULL)
            {
                FM_Printf(FM_ERROR, "Cannot allocate an event.\n");
            }
            else
            {
                //free all SCBs in the TEI map
                CRM_Init(snsm->crm);

                //TODO: search for the backup CCO through the TEI MAP list

                event->eventHdr.type = EVENT_TYPE_CCO_LOST_IND;
                //deliver the event to the upper layer
                linkl = (sLinkLayer *)HOMEPLUG_GetLayer(HP_LAYER_TYPE_LINK);
                linkl->deliverEvent(linkl->eventcookie, event);
            }
                                 
        } 
    }
*/
}


u8 SNSM_SelectCco(sSnsm *snsm,  sEvent *event)
{
    u8          *myMacAddr = NULL;
    u8          *staMacAddr = NULL;
    u8           beCco = FALSE;
    sCmUaStaInd *uaStaInfo = (sCmUaStaInd *) event->buffDesc.dataptr;

    sHpgpHdr    *hpgpHdr = (sHpgpHdr *)event->buffDesc.buff;

    //chceck if the NID is matched
    if((memcmp(snsm->staInfo->nid, uaStaInfo->nid, NID_LEN) == 0))
    {
        //mached NID
        //Select CCO
        if(snsm->staInfo->staCap.fields.ccoCap > uaStaInfo->ccoCap )
        {
            beCco = TRUE;
        }
        else if (snsm->staInfo->staCap.fields.ccoCap == uaStaInfo->ccoCap )
        {
            //compare MAC address
            myMacAddr = snsm->staInfo->macAddr;
            staMacAddr = hpgpHdr->macAddr;                                
   FM_Printf(FM_MINFO,"my MAC Address: %.2x:%.2x:%.2x:%.2x:%.2x:%.2x\n",
                        myMacAddr[0], myMacAddr[1],
                        myMacAddr[2], myMacAddr[3],
                        myMacAddr[4], myMacAddr[5]);
   FM_Printf(FM_MINFO,"peer MAC Address: %.2x:%.2x:%.2x:%.2x:%.2x:%.2x\n",
                        staMacAddr[0], staMacAddr[1],
                        staMacAddr[2], staMacAddr[3],
                        staMacAddr[4], staMacAddr[5]);

            if((memcmp(myMacAddr, staMacAddr, MAC_ADDR_LEN) > 0))
            {
                beCco = TRUE;
            }
        }
    } 
    return beCco;
}

//this function is called by the upper layer (e.g. when BBT timer expires) 
//to determine the role of STA during network discovery
u8 SNSM_DetermineStaRole(sSnsm *snsm)
{
    u8 i;
    u8  staRole = STA_ROLE_USTA;
    for(i = 0; i < UA_STA_LIST_MAX; i++)
    {
        if(snsm->uaStaInfo[i].valid == TRUE)
        {
            if( memcmp(snsm->uaStaInfo[i].nid, snsm->staInfo->nid, NID_LEN) == 0)
            {
                //matched NID is found
                if(snsm->staInfo->staCap.fields.ccoCap > snsm->uaStaInfo[i].ccoCap)
                {
                    staRole = STA_ROLE_ACCO;
                }
                else if(snsm->staInfo->staCap.fields.ccoCap == snsm->uaStaInfo[i].ccoCap)
                {
                    if((memcmp(snsm->staInfo->macAddr, 
                               snsm->uaStaInfo[i].macAddr, MAC_ADDR_LEN) > 0))
                    {
                        staRole = STA_ROLE_ACCO;
                    }
                    else
                    {
                        staRole = STA_ROLE_USTA;
                    }
                } 
                else
                {
                    staRole = STA_ROLE_USTA;
                }
                break;
            }
        }
    }         

    if (i == UA_STA_LIST_MAX)
    {
        if(snsm->numAvln)
        {
            staRole = STA_ROLE_USTA;
        } 
        else
        {
            staRole = STA_ROLE_UCCO;
        }
    }

    return staRole;
}


eStatus SNSM_BuildDiscBcn(sSnsm *snsm, sBeHdr *beHdrRef[])
{
    u8              i, done;
    u8              offset = 0;
    u8              beLen = 0;
    u8              bcnLen = 0;
    sBcnHdr        *bcnHdr = NULL;
    sBeHdr         *centralBeHdr = NULL;
    sBeHdr         *beHdr = NULL;
    sDiscInfoEntry *discInfoEntry = NULL;
    sStaInfo       *staInfo = NULL;
    u8             *dataptr = NULL;

#ifdef SIMU
    offset = sizeof(sFrmCtrlBlk) + sizeof(sTxDesc);
#else
    offset = sizeof(sFrmCtrlBlk);
#endif


    staInfo = snsm->staInfo;

    memset(snsm->discBcnBuff, 0, BEACON_BUFF_LEN );
    bcnHdr = (sBcnHdr *)(snsm->discBcnBuff + offset);
    //(1)prepare the Beacon first 12 fixed portion
    memcpy(bcnHdr->nid, staInfo->nid, NID_LEN);
    bcnHdr->nid[NID_LEN-1] = (bcnHdr->nid[NID_LEN-1]&0x3F)|(staInfo->hm<<6);
    bcnHdr->stei = staInfo->staScb->tei;

    bcnHdr->bt = BEACON_TYPE_DISCOVER;
    bcnLen = sizeof(sBcnHdr);       //13 bytes, including nbe

    //(2)prepare the Beacon entries  
    dataptr = (u8 *)bcnHdr + sizeof(sBcnHdr);

    i = 0;
    done = 0;
    beHdr = (sBeHdr *)dataptr;
    bcnHdr->nbe = 0;
    //TODO: may verify if there is enough room to transmit
    //the schedule entries.
    while((!done) && ((offset + bcnLen + BeLenMax[i]) <= BEACON_PAYLOAD_SIZE))
    {
        beLen = 0;
        switch(BeHdrType[i])
        {
            case BEHDR_NON_PERSISTENT_SCHED:
            case BEHDR_PERSISTENT_SCHED:
            case BEHDR_REGIONS:
            case BEHDR_CCO_HANDOVER:
            case BEHDR_BCN_RELOC:
            case BEHDR_CHANGE_NUM_SLOTS:
            case BEHDR_CHANGE_SNID:
            {
               //copy those beacon entries
               //from the central beacon to discover beacon
               centralBeHdr = beHdrRef[BeHdrType[i]];
               if(centralBeHdr)
               {
                   memcpy(dataptr, centralBeHdr, 
                          centralBeHdr->beLen+sizeof(sBeHdr));  
                   dataptr +=sizeof(sBeHdr);
                   beLen = centralBeHdr->beLen;
               }
               break;
            }
            case BEHDR_MAC_ADDR: 
            {
                //always include mac addre entry
                beHdr->beType = BEHDR_MAC_ADDR; 
                dataptr +=sizeof(sBeHdr);
                memcpy(dataptr, staInfo->macAddr, MAC_ADDR_LEN);

                beLen = MAC_ADDR_LEN;

                break;
            } 
            case BEHDR_DISC_INFO:
            {
                //always include discovered info entry
                beHdr->beType = BEHDR_DISC_INFO;
                dataptr +=sizeof(sBeHdr);
                discInfoEntry = (sDiscInfoEntry *)dataptr;
                discInfoEntry->staCap.byte = staInfo->staCap.byte;
                discInfoEntry->staCap.fields.update = 
                    staInfo->staScb->discUpdate;
FM_Printf(FM_ERROR, "SNSM: discover beacon update: %d, %d.\n", discInfoEntry->staCap.fields.update, staInfo->staScb->discUpdate);

                discInfoEntry->numDiscSta = staInfo->staScb->numDiscSta; 
                discInfoEntry->numDiscNet = staInfo->staScb->numDiscNet; 
                discInfoEntry->staStatus.byte = staInfo->staStatus.byte;

                beLen = sizeof(sDiscInfoEntry);
                break;
            }
            case BEHDR_BPSTO:
            {
                //always include beacon period start time offset
                beHdr->beType = BEHDR_BPSTO;
                dataptr += sizeof(sBeHdr);
                beLen = 3;
                snsm->bpstoOffset = bcnLen + sizeof(sBeHdr);
                break;
            }
            case BEHDR_VENDOR_SPEC:  
            {
                done = 1;
                break;
            }
            default:
            {
            }
        }
        if( beLen != 0)
        { 
            beHdr->beLen = beLen; 
            bcnLen += sizeof(sBeHdr) + beHdr->beLen;
            dataptr += beLen;
            beHdr = (sBeHdr *)dataptr;
            beLen = 0;
            bcnHdr->nbe++;
        }
        i++;
    }


    if((offset + bcnLen) > BEACON_PAYLOAD_SIZE)
    {
        FM_Printf(FM_ERROR, "SNSM: discover beacon is too big.\n");
        return STATUS_FAILURE;
    }

    return STATUS_SUCCESS;

}

void SNSM_TransmitDiscBcn(sSnsm *snsm) 
{
    sTxDesc         txinfo;
    sBuffDesc       buffDesc;
    u8              offset = 0; 

    FM_Printf(FM_MMSG, "SNSM: >>> DISCOVERY BEACON.\n");

#ifdef SIMU
    offset = sizeof(sFrmCtrlBlk) + sizeof(sTxDesc);
#else
    offset = sizeof(sFrmCtrlBlk);
#endif

    //transmit the beacon 
    txinfo.dtei = 0xFF;
    txinfo.stei = snsm->staInfo->staScb->tei;
    txinfo.frameType = BEACON_TYPE_DISCOVER;
    txinfo.snid = snsm->staInfo->snid;
  
    //prepare tx control information
    buffDesc.buff = snsm->discBcnBuff;
    buffDesc.bufflen = BEACON_BUFF_LEN;
    buffDesc.dataptr = snsm->discBcnBuff + offset;
    buffDesc.datalen = BEACON_PAYLOAD_SIZE;

    //FM_HexDump(FM_DATA|FM_MINFO, "SNSM: discovery beacon:", 
    //                             buffDesc.dataptr,
    //                             buffDesc.datalen);

    HAL_TransmitBeacon(HOMEPLUG_GetHal(), &txinfo, &buffDesc, snsm->bpstoOffset);
}


u8 SNSM_DetectCco(sSnsm *snsm, sBcnHdr *bcnHdr) 
{
    sLinkLayer     *linkl = NULL;
    sStaInfo       *staInfo = NULL;
    sCrm           *crm = NULL;
    u8              bcnsrc = BCN_SRC_UNKNOWN;

//    linkl = (sLinkLayer *)HPGPCTRL_GetLayer(HP_LAYER_TYPE_LINK);
    linkl = snsm->linkl;
    staInfo = LINKL_GetStaInfo(linkl);
    crm = LINKL_GetCrm(linkl);



    bcnHdr->nid[NID_LEN-1] &= 0x3F;
/*
   FM_Printf(FM_MINFO,"my NID: %.2x:%.2x:%.2x:%.2x:%.2x:%.2x:%.2x\n",
                        snsm->staInfo->nid[0], snsm->staInfo->nid[1],
                        snsm->staInfo->nid[2], snsm->staInfo->nid[3],
                        snsm->staInfo->nid[4], snsm->staInfo->nid[5],
                        snsm->staInfo->nid[6]);
   FM_Printf(FM_MINFO,"beacon NID: %.2x:%.2x:%.2x:%.2x:%.2x:%.2x:%.2x\n",
                        bcnHdr->nid[0], bcnHdr->nid[1],
                        bcnHdr->nid[2], bcnHdr->nid[3],
                        bcnHdr->nid[4], bcnHdr->nid[5],
                        bcnHdr->nid[6]);
*/

    if((memcmp(staInfo->nid, bcnHdr->nid, NID_LEN) == 0))
    {
        if( (bcnHdr->bt == BEACON_TYPE_CENTRAL)||
            (bcnHdr->bt == BEACON_TYPE_PROXY))
        {
            bcnsrc = BCN_SRC_CCO;
            if(staInfo->ccoScb == NULL)
            {
                FM_Printf(FM_MINFO, "SNSM: Add a scb for CCo.\n");
                staInfo->ccoScb = CRM_AddScb(crm, bcnHdr->stei);
                //NOTE: Beacon event does not contain CCo's MAC adddr
                //The STA does not know CCo's MAC address until it associates
                //with the CCo or it sends a query or receives discovery beacon
                if(!staInfo->ccoScb) 
                {
                    FM_Printf(FM_WARN, "SNSM: cannot get a scb for CCo.\n");
                }
                memcpy(staInfo->ccoScb->macAddr, bcAddr, MAC_ADDR_LEN);
            }
        }
        else
        {
            bcnsrc = BCN_SRC_DISC;
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
//    FM_Printf(FM_MINFO, "SNSM: Detect CCo %d.\n", bcnsrc);
    return bcnsrc;
}


//This function is called only when the STA is associated with the CCo
//void SNSM_ProcBcn(sSnsm *snsm, sEvent *event)
//We split the beacon processing into two parts:
//High priority: those requiring immdiate response
//Low  priority: those tolerating the delay.
void SNSM_ProcBcnLow(sSnsm *snsm, sRxDesc *rxdesc, u8* bcn )
{
    sBcnHdr        *bcnHdr = NULL;
    sBeHdr         *beHdr = NULL;
    u8              bcnsrc;
    u8              nid7;
    u8              nbe = 0;
    u8             *dataptr = 0;
    u8             *macAddr = NULL;
    sDiscStaInfoRef discStaInfoRef;
    sDiscNetInfoRef discNetInfoRef;
//    sBcnRef         bcnRef;
    sCcoHoEntry    *ccoHo = NULL;
    sStaInfo       *staInfo = NULL;

    staInfo = snsm->staInfo;

    //(1) process the beacon header
//    bcnHdr = (sBcnHdr *) event->buffDesc.dataptr;
    bcnHdr = (sBcnHdr *) bcn;
    //get the Hybrid Mode before calling SNSM_DetectCco(),
    //which will remove the HM in the beacon header
    nid7 = bcnHdr->nid[NID_LEN-1];
  
    //first, check if it is my CCo
    bcnsrc = SNSM_DetectCco(snsm, bcnHdr); 
    switch(bcnsrc)
    {
        case BCN_SRC_CCO:
        {
            //now, it is my network CCo
            snsm->noBcn = 0;
            if(snsm->enableCcoDetection)
            {
                FM_Printf(FM_HINFO, "SNSM: Detect the CCo.\n");
                snsm->enableCcoDetection = 0;
                snsm->ccoDetected = 1;
            }
            break;
        }
        case BCN_SRC_DISC:
        {
#ifdef P8051
            FM_Printf(FM_MMSG|FM_MINFO, "SNSM: <<< DISCOVERY BEACON (L) (tei: %bu).\n", bcnHdr->stei);
#else
            FM_Printf(FM_MMSG|FM_MINFO, "SNSM: <<< DISCOVERY BEACON (L) (tei: %d).\n", bcnHdr->stei);
#endif
            break;
        }
        case BCN_SRC_OTHER_CCO:
        {
            //It is a CCo from other networks
            //update the network list
            discNetInfoRef.nid = bcnHdr->nid;
            discNetInfoRef.hybridMode = nid7>>6;
            discNetInfoRef.netMode = bcnHdr->nm;
            discNetInfoRef.numBcnSlots = bcnHdr->numslots;
//            discNetInfoRef.bpsto = snsm->bpsto; //TODO: how to get the bpsto 
            SCB_UpdateDiscNetList(snsm->staInfo->staScb, &discNetInfoRef);
            return;
        }
        case BCN_SRC_OTHER_DISC:
        {
            //discovery sta list may include the STA not in the same network
            //but do not support it at present, though it is easy.
            //
            return;
        }
        default:
        {
        }
    }

    //now, the beacon is either central/proxy/discovery beacon 
    //from the same network

    if(bcnHdr->hoip)
    {
        //Call the SNAM to suspend the association request and traffic
        snsm->stopSnam = 1;
    }
    else if(snsm->stopSnam) //may not be necessary
    {
        //Call the SNAM to resume the association request and traffic
        snsm->stopSnam = 0;
    }


//    memset(&bcnRef, 0, sizeof(sBcnRef));
    nbe = bcnHdr->nbe;
    dataptr = bcn + sizeof(sBcnHdr);
    beHdr = (sBeHdr *) dataptr;

//FM_Printf(FM_HINFO, "SNSM: Beacon entry number (%d).\n", nbe);


    //(2) Process Beacon Management Information (BMI)
    //Note: According to the standard, the BENTRYs within the MBI shall 
    //be arranged in increasing order of their BEHDR values.
    while(nbe)
    {
        dataptr += sizeof(sBeHdr); //move to the start of BEENTRY 
        switch (beHdr->beType)
        {
            case BEHDR_MAC_ADDR:
            {
                macAddr = dataptr;
                if( (bcnsrc == BCN_SRC_CCO)&&
                    (memcmp(staInfo->ccoScb->macAddr, macAddr, MAC_ADDR_LEN)) )
                {
                    //the central beacon from the backup CCo
                    //perform the CCo switch 
                    //(let the previous CCo scb aging out)
                    staInfo->ccoScb = CRM_AddScb(snsm->crm, bcnHdr->stei);

                    //send CCO_DISC.IND to the SNAM to renew TEI with the
                    //backup CCo
                    snsm->ccoDetected = 1;

                    FM_Printf(FM_HINFO, "SNSM: switch to the backup CCo.\n");
                }
               
                break;
            }
            case BEHDR_DISC_INFO:
            {
                discStaInfoRef.discInfo = (sDiscInfoEntry *)dataptr;
                if (macAddr)
                {
                    discStaInfoRef.macAddr = macAddr;
//                    discStaInfoRef.nid = bcnHdr->nid;
//                    discStaInfoRef.tei = rxdesc->stei;
                    discStaInfoRef.tei = bcnHdr->stei;
                    discStaInfoRef.snid = rxdesc->snid;
                    //NOTE: hm filed in the beacon is set to zero
                    //in SNSM_DetectCco()
                    if( memcmp(snsm->staInfo->nid, bcnHdr->nid, NID_LEN) == 0 ) 
                    {
                        discStaInfoRef.sameNet = 1;
                    }
                    else
                    {
                        discStaInfoRef.sameNet = 0;
                    }
                    //update the discovred STA list
                    SCB_UpdateDiscStaList(staInfo->staScb, &discStaInfoRef);
                }
                else
                {
                    FM_Printf(FM_WARN, "SNSM: Unknown discovered STA in Beacon.\n");
                }
                break;
            }
            case BEHDR_ENCRYP_KEY_CHANGE:
            {
                break;
            }
            case BEHDR_CCO_HANDOVER:
            {
                ccoHo = (sCcoHoEntry *)dataptr;
                FM_Printf(FM_HINFO, "SNSM: HO countdown (%d).\n", ccoHo->hcd);
                if(ccoHo->hcd == 0)
                {
                    //FM_Printf(FM_HINFO, "SNSM: HO switch.\n");
                    //HO countdown expires. perform handover
                    if(ccoHo->nctei == snsm->staInfo->tei)
                    {
                        //switch to the CCO mode/role
                        snsm->hoSwitch = HPGP_HO_SWITCH_CCO;
                    }
                    else
                    {
                        //still in the STA mode but associate to new CCO 
                        snsm->nctei = ccoHo->nctei;
                        snsm->hoSwitch = HPGP_HO_SWITCH_STA;
                    }
                }
                break;
            }
            case BEHDR_ACL_SYNC_CNTDOWN:
            {
                break;
            }
            case BEHDR_CHANGE_HM:
            {
                break;
            }
            default:
            {
            }
        }
        //move to the next BEHDR
        dataptr = dataptr +  beHdr->beLen; 
        beHdr = (sBeHdr *) dataptr;
        nbe--;
    }         

}

void SNSM_ProcBcnHigh(sSnsm *snsm, u8* bcn, u8 snid)
{
    sBcnHdr        *bcnHdr = NULL;
    sBeHdr         *beHdr = NULL;
    sBeHdr         *beHdrRef[NELEMENTS(BeHdrType)];
    u8              nid7; 
    u8              bcnsrc = BCN_SRC_UNKNOWN; 
    u8              nbe = 0;
    u8             *dataptr = 0;
    u8              i = 0;
    u8              j = 0;
    sStaInfo       *staInfo = NULL;
    sCsmaRegion    *region = NULL;
    u8              regionNum = 0;
    u16             endTime = 0;
    sRegionEntry   *regionEntry = NULL;

    staInfo = snsm->staInfo;

// FM_Printf(FM_HINFO, "SNSM: BeHdrType size = %d\n", NELEMENTS(BeHdrType));
    for(i = 0; i< NELEMENTS(BeHdrType); i++)
    {
        beHdrRef[i] = NULL;
    }
    //(1) process the beacon header
//    bcnHdr = (sBcnHdr *) event->buffDesc.dataptr;
    bcnHdr = (sBcnHdr *)bcn;

    nid7 = bcnHdr->nid[NID_LEN-1];

    bcnHdr->nid[NID_LEN-1] &= 0x3F;

 //FM_Printf(FM_HINFO, "SNSM_ProcBcnHigh\n");

    if((memcmp(staInfo->nid, bcnHdr->nid, NID_LEN) == 0))
    {
        if( (bcnHdr->bt == BEACON_TYPE_CENTRAL)||
            (bcnHdr->bt == BEACON_TYPE_PROXY))
        {
            //it should not occur
            FM_Printf(FM_MMSG|FM_LINFO, "SNSM: <<< CENTRAL/PROXY BEACON (H).\n");
            bcnsrc = BCN_SRC_CCO;
        }
        else
        {
#ifdef P8051
            FM_Printf(FM_MMSG|FM_MINFO, "SNSM: <<< DISCOVERY BEACON (H) (tei: %bu)\n", bcnHdr->stei);
#else
            FM_Printf(FM_MMSG|FM_MINFO, "SNSM: <<< DISCOVERY BEACON (H) (tei: %d)\n", bcnHdr->stei);
#endif
            bcnsrc = BCN_SRC_DISC;
        }
    }
    else
    {
        if( (bcnHdr->bt == BEACON_TYPE_CENTRAL)||
            (bcnHdr->bt == BEACON_TYPE_PROXY))
        {
            bcnsrc = BCN_SRC_OTHER_CCO;
            // Process other bcn for avln detection
            if(snsm->state != SNSM_STATE_NET_DISC)
            {
                return; 
            }
        }
        else
        {
            bcnsrc = BCN_SRC_OTHER_DISC;
            //not interested in other discover beacons at present
            return;
        }
    }

    bcnHdr->nid[NID_LEN-1] = nid7;

    /* see if the beacon region exists */
    region = snsm->region;
    j = 0;
    if (bcnHdr->numslots)
    {
        /* need to include beacon region */
        region[j].startTime = 0;
        region[j].duration = bcnHdr->numslots * HPGP_BEACON_SLOT_ATU;
        region[j].regionType = REGION_TYPE_BEACON;
        region[j].bcnRegion = 1;
        j++;
    }
                        
    nbe = bcnHdr->nbe;
    dataptr = bcn + sizeof(sBcnHdr);
    beHdr = (sBeHdr *) dataptr;
    
    //(2) Process Beacon Management Information (BMI)
    //Note: According to the standard, the BENTRYs within the MBI shall 
    //be arranged in increasing order of their BEHDR values.

    //FM_HexDump(FM_DATA|FM_MINFO, "Rx Bcn:",
    //           bcn, PLC_BCNRX_LEN);
    while(nbe)
    { 
        dataptr += sizeof(sBeHdr); //move to the start of BEENTRY 
        switch (beHdr->beType)
        {
            case BEHDR_NON_PERSISTENT_SCHED:
            {
                beHdrRef[BEHDR_NON_PERSISTENT_SCHED] = beHdr;
//                bcnRef.nonPersSchedEntry = beHdr;
 
                break;
            }
            case BEHDR_PERSISTENT_SCHED:
            {
                sSaiWithSt    *saiwst = NULL;
                sSaiWithoutSt *saiwost = NULL;
                u8            *ptr = NULL;
                u8 ns = 0; 
                u8 scd = 0;
                u8 stpf = 0;
                
                beHdrRef[BEHDR_PERSISTENT_SCHED] = beHdr;
//                bcnRef.persSchedEntry = beHdr;

                /* schedule count down */
                scd = *dataptr;
                /* At present, interested in the current schedule only, 
                 * and ignore the preview schedule */ 
                if (scd & 0x7) 
                {
                    /* it is a preview schedule */
                    break; 
                }
                /* now it is a current schedule */
                /* NS */
                ns = *(dataptr + 1);
                ptr = dataptr + 2;
                endTime = 0;
				i = 0;
                while((j < HPGP_REGION_MAX) && (i < ns))
                {
                    stpf = *ptr;
                    if (stpf & 0x1) 
                    {
                        saiwst = (sSaiWithSt *)ptr;
                        endTime =  (saiwst->etHi << 4) | saiwst->etLo; 
                        if ( (saiwst->glid == (HPGP_GLID_GPP & 0x7F)) ||
                             (saiwst->glid == (HPGP_GLID_SHARED_CSMA & 0x7F)) ||
                             (saiwst->glid == (HPGP_GLID_LOCAL_CSMA & 0x7F))) 
                        {
                            region[j].startTime =  (saiwst->stHi << 8) |  
                                                   saiwst->stLo;
                            region[j].duration = endTime - region[j].startTime;
							if ((saiwst->glid == (HPGP_GLID_SHARED_CSMA & 0x7F)) || 
							    (saiwst->glid == (HPGP_GLID_GPP & 0x7F))) 
							{
							    region[j].hybridMd  = 1;
								region[j].bcnRegion = 0;
								region[j].regionType = REGION_TYPE_SHARED_CSMA;
							}
							else
							{
							    region[j].regionType = REGION_TYPE_LOCAL_CSMA;
                                region[j].hybridMd = 0;
                                region[j].bcnRegion = 0;
							}
                            j++;
                        }
                        ptr += sizeof(sSaiWithSt);
                    }
                    else 
                    {
                        saiwost = (sSaiWithoutSt *)ptr;
                        if ( (saiwost->glid == (HPGP_GLID_GPP & 0x7F)) ||
                             (saiwost->glid == (HPGP_GLID_SHARED_CSMA & 0x7F)) ||
                             (saiwost->glid == (HPGP_GLID_LOCAL_CSMA & 0x7F))) 
                        {
                            region[j].startTime =  endTime;
                            endTime =  (saiwost->etHi << 8) | saiwost->etLo; 
                            region[j].duration = endTime - region[j].startTime;
							if ((saiwst->glid == (HPGP_GLID_SHARED_CSMA & 0x7F)) ||
							    (saiwost->glid == (HPGP_GLID_GPP & 0x7F))) 
							{
							    region[j].hybridMd = 1;
								region[j].bcnRegion = 0;
					            region[j].regionType = REGION_TYPE_SHARED_CSMA;
							}
							else
							{
							    region[j].regionType = REGION_TYPE_LOCAL_CSMA;
                                region[j].hybridMd = 0;
                                region[j].bcnRegion = 0;
							}
                            j++;
                        }
                        else
                        {
                            endTime =  (saiwost->etHi << 8) | saiwost->etLo; 
                        }
                        ptr += sizeof(sSaiWithoutSt);
                    }
                    i++;
                }
				if (j < HPGP_REGION_MAX)
				{
                    region[j].startTime  = 0;
                    region[j].bcnRegion  = 0;
                    region[j].duration   = 0;
                    region[j].hybridMd   = 0;  
				    region[j].regionType = REGION_TYPE_STAYOUT;
                    snsm->regionNum = j + 1;
                }
				else
                {
                    snsm->regionNum = HPGP_REGION_MAX;
                }
#ifdef HPGP_HAL
                //HHAL_SetCsmaRegions(snsm->region, snsm->regionNum);
#endif
//            for (j=0; j<snsm->regionNum; j++) {
//                FM_Printf(FM_HINFO, "SNSM: region %bu, start: 0x%x, duration: 0x%x \n", 
//                    j, snsm->region[j].startTime, snsm->region[j].duration);
//            }


                break;
            }
            case BEHDR_REGIONS:
            {
                beHdrRef[BEHDR_REGIONS] = beHdr;
                //bcnRef.regionEntry = beHdr;
                break;
            }
            case BEHDR_DISCOVER:
            {
#ifdef P8051
FM_Printf(FM_HINFO, "SNSM: get the discovery Beacon sched (%bu).\n", *dataptr);
#else
FM_Printf(FM_HINFO, "SNSM: get the discovery Beacon sched (%d).\n", *dataptr);
#endif
                if( (staInfo->staScb)&&(staInfo->staScb->tei == *dataptr) )
                {
                    //transmit the discover beacon
                    snsm->txDiscBcn = TRUE;
                }
                break;
            }
            case BEHDR_BPSTO:
            {
                memcpy(snsm->bpsto, dataptr, 3);
#ifdef HPGP_HAL
                /* TODO: perform sync with the any net
                 * during the network discovery 
                 */
                /* set it to MAC */
            // Process other bcn also to sync network
//            	if(bcnsrc == BCN_SRC_CCO)             
                {
                    sHpgpHalCB *hhalCb = HOMEPLUG_GetHal()->hhalCb;
                if ( HHAL_SyncNet(snsm->bpsto) == STATUS_SUCCESS && !snsm->netSync )
                {
                    /* stop scan if it is my network */
                    //HAL_ScanNet(FALSE);                   
                        snsm->netSync = TRUE;


 				        FM_HexDump(FM_MINFO, "SNSM: Setting netSync %bu \n",
						            &snsm->bpsto, 1);    
						                                
                  

                }
                
                /*FIXME: Host App should select the network , during scanning.
                         Selection could be based on Lqi/Rssi 
                         Temporarily the selection is done here */				
                if(snsm->netScan && !hhalCb->nwSelected)
                {
                    if(hhalCb->halStats.RxGoodBcnCnt >= PLC_BCNTST_SYNCTHRES ) 
                    {                            
                        HHAL_SetSnid(snid);
                        FM_Printf(FM_MINFO, "SNSM: Setting STA Snid %bu\n", snid);                             
                    }                    
                }
                    

                }
#endif
                
                break;
            }
            case BEHDR_CCO_HANDOVER:
            {
                //FM_Printf(FM_HINFO, "SNSM: CCO HO (H).\n");
                beHdrRef[BEHDR_CCO_HANDOVER] = beHdr;
                break;
            }
            case BEHDR_BCN_RELOC:
            {
                beHdrRef[BEHDR_BCN_RELOC] = beHdr;
                break;
            }
            case BEHDR_CHANGE_NUM_SLOTS:
            {
                beHdrRef[BEHDR_CHANGE_NUM_SLOTS] = beHdr;
                break;
            }
            case BEHDR_CHANGE_SNID:
            {
                beHdrRef[BEHDR_CHANGE_SNID] = beHdr;
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

    if(snsm->txDiscBcn)
    {
        if(SNSM_BuildDiscBcn(snsm, beHdrRef) != STATUS_SUCCESS)
        {
            snsm->txDiscBcn = FALSE;
        }
    } 

}

void LINKL_StaProcBcnHandler(void *cookie, sEvent *event)
{
    sLinkLayer     *linkl = (sLinkLayer *)cookie;
    sSnsm*         snsm = (sSnsm *)LINKL_GetSnsm(linkl);
    sHpgpHdr         *hpgpHdr = NULL;

    hpgpHdr = (sHpgpHdr *)event->buffDesc.buff;

    SNSM_ProcBcnHigh(snsm, event->buffDesc.dataptr, hpgpHdr->snid);

    if(snsm->txDiscBcn)
    {
        SNSM_TransmitDiscBcn(snsm); 
        snsm->txDiscBcn = FALSE;
    }
}


eStatus SNSM_SendMgmtMsg(sSnsm *snsm, u16 mmType)
{
    eStatus          status = STATUS_FAILURE;
    sEvent          *newEvent = NULL;
    sHpgpHdr        *hpgpHdr = NULL;
    sCmUaStaInd     *uaStaInfo = NULL;
    sCcDiscStaInfo  *ccDiscStaInfo = NULL;
    sCcDiscNetInfo  *ccDiscNetInfo = NULL;
    u8               numSta = 0;
    u8               numNet = 0;
    u8               i = 0;
    u16              eventSize = 0;
    u8              *dataptr = NULL;
    sScb            *staScb = NULL;

    switch(mmType)
    {
        case EVENT_TYPE_CM_UNASSOC_STA_IND:
        {
            //prepare CM_UNASSOCIATED_STA_IND
            eventSize = MAX(sizeof(sCmUaStaInd), HPGP_DATA_PAYLOAD_MIN); 
            newEvent = EVENT_Alloc(eventSize, EVENT_HPGP_MSG_HEADROOM);
            if(newEvent == NULL)
            {
                FM_Printf(FM_ERROR, "Cannot allocate an event.\n");
                return STATUS_FAILURE;
            }
            newEvent->eventHdr.eventClass = EVENT_CLASS_MSG;
            newEvent->eventHdr.type = EVENT_TYPE_CM_UNASSOC_STA_IND;
            hpgpHdr = (sHpgpHdr *)newEvent->buffDesc.buff; 
            hpgpHdr->tei = 0xFF;
            hpgpHdr->mnbc = 1;
            hpgpHdr->macAddr = bcAddr;
            hpgpHdr->snid = snsm->staInfo->snid;
            hpgpHdr->eks = HPGP_EKS_NONE;

            uaStaInfo = (sCmUaStaInd *)newEvent->buffDesc.dataptr; 
            memcpy(uaStaInfo->nid, snsm->staInfo->nid, NID_LEN);
            uaStaInfo->ccoCap = snsm->staInfo->staCap.fields.ccoCap;
            newEvent->buffDesc.datalen += eventSize;

            break;
        }
        case EVENT_TYPE_CC_DISCOVER_LIST_CNF:
        {
            staScb = snsm->staInfo->staScb;
            numSta = staScb->numDiscSta;
            numNet = staScb->numDiscNet;
            eventSize = MAX(HPGP_DATA_PAYLOAD_MIN, 
                            2 + numSta*sizeof(sCcDiscStaInfo) + 
                            numNet*sizeof(sCcDiscNetInfo)); 
            newEvent = EVENT_Alloc(eventSize, EVENT_HPGP_MSG_HEADROOM);
            if(newEvent == NULL)
            {
                FM_Printf(FM_ERROR, "Cannot allocate an event.\n");
                return STATUS_FAILURE;
            }

            newEvent->eventHdr.eventClass = EVENT_CLASS_MSG;
            newEvent->eventHdr.type = EVENT_TYPE_CC_DISCOVER_LIST_CNF;

            hpgpHdr = (sHpgpHdr *)newEvent->buffDesc.buff; 
            hpgpHdr->tei = snsm->staInfo->ccoScb->tei;
            hpgpHdr->macAddr = snsm->staInfo->ccoScb->macAddr; 
            hpgpHdr->snid = snsm->staInfo->snid;

            dataptr = newEvent->buffDesc.dataptr; 
            //station information
            *dataptr = numSta;
            dataptr++;
            i = 0;
            while(numSta || (i < DISC_STA_LIST_MAX))
            {
                if(staScb->discStaInfo[i].valid)
                { 
                    ccDiscStaInfo = (sCcDiscStaInfo *)dataptr;
                    memcpy(ccDiscStaInfo->macAddr, staScb->discStaInfo[i].macAddr, 
                           MAC_ADDR_LEN);

                    ccDiscStaInfo->tei = staScb->discStaInfo[i].tei; 
                    ccDiscStaInfo->sameNet = staScb->discStaInfo[i].sameNet; 
                    ccDiscStaInfo->snid = staScb->discStaInfo[i].snid; 
                    ccDiscStaInfo->staCap.byte = staScb->discStaInfo[i].staCap.byte; 
                    ccDiscStaInfo->sigLevel = 0;//staScb->discStaInfo[i].sigLevel; 
                    ccDiscStaInfo->avgBle = 0;// staScb->discStaInfo[i].avgBle; 
                    dataptr += sizeof(sCcDiscStaInfo);
                    newEvent->buffDesc.datalen +=sizeof(sCcDiscStaInfo); 
                    numSta--;
                }
                i++;
            }
            //network information
            *dataptr = numNet;
            dataptr++;
            i = 0;
            while(numNet || (i < DISC_NET_LIST_MAX))
            {
                if(staScb->discNetInfo[i].valid)
                { 
                    ccDiscNetInfo = (sCcDiscNetInfo *) dataptr;
                    memcpy(ccDiscNetInfo->nid, staScb->discNetInfo[i].nid, 
                           NID_LEN);

                    ccDiscNetInfo->snid = staScb->discNetInfo[i].snid; 
                    ccDiscNetInfo->hybridMode = staScb->discNetInfo[i].hybridMode; 
                    ccDiscNetInfo->numBcnSlots = staScb->discNetInfo[i].numBcnSlots; 
                    ccDiscNetInfo->coordStatus = staScb->discNetInfo[i].coordStatus; 
                    ccDiscNetInfo->offset = staScb->discNetInfo[i].offset; 
                    dataptr += sizeof(sCcDiscNetInfo);
                    newEvent->buffDesc.datalen +=sizeof(sCcDiscNetInfo); 
                    numNet--;
                }
                i++;
            }

            newEvent->buffDesc.datalen = eventSize;
                                         
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

eStatus SNSM_DeliverEvent(sSnsm *snsm, u16 eventType)
{
    sEvent       *newEvent = NULL;
    sLinkLayer   *linkl = snsm->linkl;

    newEvent = EVENT_Alloc(sizeof(snsm->staRole), 0);
    if(newEvent == NULL)
    {
        FM_Printf(FM_ERROR, "Cannot allocate an event.\n");
        return STATUS_FAILURE;
    }
    newEvent->eventHdr.eventClass = EVENT_CLASS_CTRL;
    newEvent->eventHdr.type = eventType;

    if(eventType == EVENT_TYPE_NET_DISC_IND)
    {
        *(newEvent->buffDesc.dataptr) = snsm->staRole;
        newEvent->buffDesc.datalen += sizeof(snsm->staRole);
    }

    //deliver the event to the upper layer
#ifdef CALLBACK
    linkl->deliverEvent(linkl->eventcookie, newEvent);
#else
    CTRLL_ReceiveEvent(linkl->eventcookie, newEvent);
#endif

    return STATUS_SUCCESS;
}


void SNSM_ProcEvent(sSnsm *snsm, sEvent *event)
{
    sBcnHdr      *bcnHdr = NULL;
    u8            becomeCco = FALSE;
    u8            *bcn = NULL;
    u8            bcnsrc;
    u8            staType;
    sAvlnInfoRef  avlnInfoRef;
    sEvent       *newEvent = NULL;
    sHpgpHdr     *hpgpHdr = NULL;
    sCmUaStaInd  *uaStaInfo = NULL;
    sScb         *scb = NULL;
    sStaInfo     *staInfo = NULL; 
    sCrm         *crm = NULL;
    sSnam        *snam = NULL;
//    sCnam        *cnam = NULL;
    sRxDesc      rxdesc;
//    sLinkLayer   *linkl = HPGPCTRL_GetLayer(HP_LAYER_TYPE_LINK);
    sLinkLayer   *linkl = snsm->linkl;
    staInfo = LINKL_GetStaInfo(linkl); 
    crm = LINKL_GetCrm(linkl);
    snam = LINKL_GetSnam(linkl);


    switch(snsm->state)
    {
        case SNSM_STATE_INIT:
        {
            if( (event->eventHdr.eventClass == EVENT_CLASS_CTRL) &&
                (event->eventHdr.type == EVENT_TYPE_SNSM_START) )
            {
                staType = *(event->buffDesc.dataptr);           
                switch(staType)
                {
                    case LINKL_STA_TYPE_NETDISC:
                    {
                        snsm->enableCcoSelection = 0;
                        snsm->enableCcoDetection = 1;
#ifdef HPGP_HAL
                        /* set PhyClk */
                        //HHAL_SetDevMode(DEV_MODE_CCO, LINE_MODE_DC);
                        
                        //Device should in STA mode during net discovery     
                        //HHAL_SetDevMode(DEV_MODE_CCO, gHpgpHalCB.lineMode);
                        FM_Printf(FM_MINFO,"nsm :LINKL_STA_TYPE_NETDISC , Set sta\n");

                        /* start scan */
                        //HAL_ScanNet(TRUE);
#endif
                        STM_StartTimer(snsm->bbtTimer, HPGP_TIME_BBT);
                        STM_StartTimer(snsm->usttTimer, HPGP_TIME_USAI);   
                        STM_StartTimer(snsm->discAgingTimer, 
                                       HPGP_TIME_DISC_AGING);   
                        snsm->state = SNSM_STATE_NET_DISC;
                        FM_Printf(FM_MINFO, "SNSM: Start as NET DISC.\n");
                        break;
                    }
                    case LINKL_STA_TYPE_UNASSOC:
                    {
                        snsm->enableCcoSelection = 1;
                        snsm->enableCcoDetection = 1;
#ifdef HPGP_HAL 
                        /* set PhyClk */
                        //HHAL_SetDevMode(DEV_MODE_CCO, LINE_MODE_DC);                  
                        FM_Printf(FM_MINFO,"nsm :LINKL_STA_TYPE_UNASSOC , Set Scan\n");

                        /* start scan */
                        HAL_ScanNet(TRUE);
#endif
                        STM_StartTimer(snsm->usttTimer, HPGP_TIME_USAI);   
                        STM_StartTimer(snsm->discAgingTimer, 
                                     HPGP_TIME_DISC_AGING);   
                        snsm->state = SNSM_STATE_NET_DISC;
                        break;
                    }
                    case LINKL_STA_TYPE_ASSOC:
                    {
                        FM_Printf(FM_MINFO, "SNSM: Start as ASSOC STA.\n");
                        snsm->enableCcoSelection = 0;
                        snsm->enableCcoDetection = 1;
                        STM_StartTimer(snsm->discAgingTimer, 
                                       HPGP_TIME_DISC_AGING);   
                        snsm->state = SNSM_STATE_CONN;
                        break;
                    }
                    default:
                    {
       FM_Printf(FM_WARN, "SNSM: unknown sta type for SNSM to start (%d).\n",
                           staType);
                    }
                }
            }
            break;
        }
        case SNSM_STATE_NET_DISC:
        {
            //perform network discovery procedure
            if(event->eventHdr.eventClass == EVENT_CLASS_MSG)
            {
                switch(event->eventHdr.type)
                {
                    case EVENT_TYPE_CM_UNASSOC_STA_IND:
                    {   
                        FM_Printf(FM_MINFO, "SNSM: <<< CM_UNASSOC_STA.IND.\n");
                        hpgpHdr = (sHpgpHdr *)event->buffDesc.buff;
                        uaStaInfo = (sCmUaStaInd *) (event->buffDesc.dataptr);
                        SNSM_UpdateUaStaList(snsm, uaStaInfo, hpgpHdr->macAddr);

                        //CCo selection is performed 
                        //when the STA is unassociated STA
                        if(snsm->enableCcoSelection)
                        {
                            becomeCco = SNSM_SelectCco(snsm, event);
                               
                            if(becomeCco == TRUE)
                            {
                                FM_Printf(FM_MINFO, "SNSM: Auto-selected CCo. Scan Off\n");
#ifdef HPGP_HAL
                                /* stop scan */
                                HAL_ScanNet(FALSE);
#endif
                                //(1) stop the SNSM
                                STM_StopTimer(snsm->usttTimer);   
                                STM_StopTimer(snsm->discAgingTimer);   

                                snsm->enableCcoSelection = 0;
                                snsm->enableCcoDetection = 0;
								snsm->netSync = FALSE;
								snsm->netScan  = FALSE;
                                snsm->state = SNSM_STATE_INIT;
                                //(2) stop the SNAM
                                SNAM_Stop(snam);
                                //send event CCO_SEL_IND to the upper layer
                                SNSM_DeliverEvent(snsm, EVENT_TYPE_CCO_SLCT_IND);
                                 
                            }
                        }
                        break;                    
                    }
                    default:
                    {
                    }
                }
            }
            else //control event
            {
                switch(event->eventHdr.type)
                {
                    case EVENT_TYPE_SNSM_START:
                    {
                        staType = *(event->buffDesc.dataptr);           
                        switch (staType)
                        {
                            case LINKL_STA_TYPE_UNASSOC:
                            {
                                if(!snsm->netSync && !snsm->netScan)
                                {
                                //TODO:does it occur now?
                                snsm->enableCcoSelection = 1;
                                snsm->enableCcoDetection = 1;
#ifdef HPGP_HAL
                                FM_Printf(FM_MINFO,"nsm : LINKL_STA_TYPE_UNASSOC , Set sta, Scan 2\n");
                                /* TODO: Set PhyClk in HW */
                                //HHAL_SetDevMode( 
                                //    DEV_MODE_CCO, LINE_MODE_DC);
                                //HHAL_SetDevMode(DEV_MODE_STA,gHpgpHalCB.lineMode);
                                /* start scan */
                                //HAL_ScanNet(TRUE);
#endif
                                /* these timers have started in the init state
								 * thus not need to start the timers again here
								 */
                    //          STM_StartTimer(snsm->usttTimer, HPGP_TIME_USAI);
                    //          STM_StartTimer(snsm->discAgingTimer, 
                    //                         HPGP_TIME_DISC_AGING);   
                                //stay in the net disc state
                                }
                                break;
                            }
                            case LINKL_STA_TYPE_ASSOC:
                            {
#ifdef HPGP_HAL
                                /* stop scan */
                                HAL_ScanNet(FALSE);
                                FM_Printf(FM_MINFO,"nsm : LINKL_STA_TYPE_ASSOC , Scan OFF\n");
#endif
                                //it occurs after the STA discover
                                //the CCo during the network discovery
                                //and unassoicated STA
                                STM_StopTimer(snsm->usttTimer);   
                                snsm->enableCcoSelection = 0;
                                snsm->enableCcoDetection = 0;
                                snsm->state = SNSM_STATE_CONN;
                                break;
                            }
                            default:
                            {
                            }
                        }
                        break;
                    }
/*
                    case EVENT_TYPE_SNSM_STOP:
                    {
                        //it happens when the STA becomes the CCo after
                        //network discovery
                        STM_StopTimer(snsm->usttTimer);   
                        STM_StopTimer(snsm->discAgingTimer);   

                        snsm->enableCcoSelection = 0;
                        snsm->enableCcoDetection = 0;
						snsm->netSync = FALSE;
						snsm->netScan = FALSE;
                        snsm->state = SNSM_STATE_INIT;
                        break;
                    }
*/
                    case EVENT_TYPE_TIMER_BBT_IND:
                    {
                        snsm->staRole = SNSM_DetermineStaRole(snsm);
                        if( (snsm->staRole == STA_ROLE_ACCO) ||
                            (snsm->staRole == STA_ROLE_UCCO) )
                        {
                            //the STA becomes the CCo after network discovery
#ifdef HPGP_HAL
                            /* stop scan */
                            //HAL_ScanNet(FALSE);
                            FM_Printf(FM_MINFO, "nsm: EVENT_TYPE_TIMER_BBT_IND , Scan Off");
#endif
                            //(1) stop the SNSM
                            STM_StopTimer(snsm->usttTimer);
                            STM_StopTimer(snsm->discAgingTimer);

                            snsm->enableCcoSelection = 0;
                            snsm->enableCcoDetection = 0;
                            //(2) stop the SNAM
                            SNAM_Stop(snam);
                            snsm->state = SNSM_STATE_INIT;

                        }
                        SNSM_DeliverEvent(snsm, EVENT_TYPE_NET_DISC_IND);
                        
                        break;
                    }
                    case EVENT_TYPE_CC_BCN_IND:
                    {
//                        FM_Printf(FM_MINFO, "SNSM: <<< BEACON.\n");
                        hpgpHdr = (sHpgpHdr *)event->buffDesc.buff;
                        bcnHdr = (sBcnHdr *) event->buffDesc.dataptr;
                        //CCO detetection is performed when the STA is
                        //in network discovery or unassociated STA
                        bcnsrc = SNSM_DetectCco(snsm, bcnHdr); 
                        switch(bcnsrc)
                        {
                            case BCN_SRC_CCO:
                            {
                                /* find my CCo */
                                /* save the snid from CCO */
                                staInfo->snid = hpgpHdr->snid;
								/* stop the timer as no need to 
								 * transmit the UNASSOCIATED_STA.IND
								 */
								STM_StopTimer(snsm->usttTimer);   

                                if (snsm->netSync == TRUE)
								{
                                if(snsm->enableCcoDetection)
                                {
#ifdef HPGP_HAL
                                    /* stop scan */
                                //FM_Printf(FM_MINFO,"nsm : enableCcoDetection , Scan OFF\n");
									
                                    //HAL_ScanNet(FALSE);
                                    //HHAL_SetDevMode(DEV_MODE_STA, LINE_MODE_DC);
#endif
                                    snsm->enableCcoDetection = 0;
                                    //send event CCO_DISC_IND to the ctrl
                                    SNSM_DeliverEvent(snsm, EVENT_TYPE_CCO_DISC_IND);
                                }
								}
								else if(!snsm->netScan)
								{
								    /* since the device has found the CCo, it does not need to
									 * send an UNASSOCIATED_STA.IND. Thus, 
									 * (1) change back to the STA
									 * (2) start scanning to sync with the CCo. 
									 * Note: when the device is set to scan, it performs Rx only 
									 *       cannot transmit a message.
									 */
									 
                                   FM_Printf(FM_MINFO,"nsm : BCN_SRC_CCO , Set STA, Scan On\n");
									 
								    //HHAL_SetDevMode(DEV_MODE_STA, LINE_MODE_DC);
                                    HHAL_SetDevMode(DEV_MODE_STA,gHpgpHalCB.lineMode);
								    /* start scan for sync */
									HAL_ScanNet(TRUE);
                                    snsm->netScan = TRUE;
								}
                                break;
                            }
                            case BCN_SRC_OTHER_CCO:
                            {
 
 //                               hpgpHdr = (sHpgpHdr *)event->buffDesc.buff;
                                avlnInfoRef.nid = bcnHdr->nid;
                                avlnInfoRef.tei = bcnHdr->stei;
                                avlnInfoRef.snid = hpgpHdr->snid;
                                // If other beacon detected scan network. 
                               if(!snsm->netScan)
							   {
							
                                    staInfo->snid = hpgpHdr->snid;
								    HHAL_SetDevMode(DEV_MODE_STA, gHpgpHalCB.lineMode);
                                    HAL_ScanNet(TRUE);
                                    snsm->netScan = TRUE;
                                }
           
                                //update the AVLN list
                                SNSM_UpdateAvlnList(snsm, &avlnInfoRef); 
                                //select PhyClk from AVLN list. 
                                //todo: How to get the PhyClk?
                                break;     
                            }
                            default:
                            {
                            }
                        }
                        break;                    
                    }
                    case EVENT_TYPE_TIMER_USTT_IND:
                    {
                        //check AVLN list
                        if((snsm->numAvln) && (snsm->netSync))
                        {
                            //select the phyclk from the AVLN list
                            //set the phyclk in the MAC

                            SNSM_SendMgmtMsg(snsm, EVENT_TYPE_CM_UNASSOC_STA_IND);
                        }
                        //start the USTT timer again
                        STM_StartTimer(snsm->usttTimer, HPGP_TIME_USAI);   
                        break;
                    }
                    case EVENT_TYPE_TIMER_DISC_AGING_IND:
                    {
                        SNSM_PerformAging(snsm);
						// If no avln then become unassoc CCO
						//If device is in USTA state and no avln found then become UCCO

                        //Using enableCcoSelection to know that we are in USTA mode
						if(snsm->enableCcoSelection == 1)
                        {                  
                            if(snsm->numAvln == 0)
                            {
                                // No avln become unassociated CCO
                                snsm->staRole = STA_ROLE_UCCO;
                                // stop ustt timer
                                STM_StopTimer(snsm->usttTimer);
                                
                                snsm->enableCcoSelection = 0;
                                snsm->enableCcoDetection = 0;
                                //(2) stop the SNAM
                                SNAM_Stop(snam);
                                snsm->state = SNSM_STATE_INIT;

                                SNSM_DeliverEvent(snsm, EVENT_TYPE_UCCO_IND);
                                break;                                

                            }
                        }
                        //restart the aging timer
                        STM_StartTimer(snsm->discAgingTimer, HPGP_TIME_DISC_AGING);   
                        break;
                    }
                    default:
                    {
                    }
                }
            }
            break;
        }
        case SNSM_STATE_CONN:
        {
            if(event->eventHdr.eventClass == EVENT_CLASS_MSG)
            {
                switch(event->eventHdr.type)
                {
                    case EVENT_TYPE_CC_DISCOVER_LIST_REQ:
                    {
                        hpgpHdr = (sHpgpHdr *)event->buffDesc.buff;
#ifdef P8051
           FM_Printf(FM_MMSG, "SNSM: <<< CC_DISCOVER_LIST.REQ. (tei: %bu).\n",
                               hpgpHdr->tei);
#else
           FM_Printf(FM_MMSG, "SNSM: <<< CC_DISCOVER_LIST.REQ. (tei: %d).\n",
                               hpgpHdr->tei);
#endif
                        staInfo->staScb->discUpdate = 0;
                        SNSM_SendMgmtMsg(snsm, EVENT_TYPE_CC_DISCOVER_LIST_CNF);
                        break;
                    }
                    default:
                    {
                    }
                }
            }
            else //control event
            {
                switch(event->eventHdr.type)
                {
                    case EVENT_TYPE_CC_BCN_IND:
                    {
//                        FM_Printf(FM_MINFO, "SNSM: <<< BEACON.\n");
                        //perform the STA discovery procedure
                        //perform network discovery procedure

                        hpgpHdr = (sHpgpHdr *)event->buffDesc.buff;
                        rxdesc.snid = hpgpHdr->snid;  
                        bcn = event->buffDesc.dataptr;
           
//                        LINKL_BcnRxHandler(linkl, &rxdesc, bcnHdr);
//FM_HexDump(FM_DATA, "SNSM Rx beacon:", bcn, event->buffDesc.datalen);
                        SNSM_ProcBcnLow(snsm, &rxdesc, bcn);
//                        SNSM_ProcBcn(snsm, event);

                        switch(snsm->hoSwitch)
                        {
                            case HPGP_HO_SWITCH_CCO:
                            {
                                FM_Printf(FM_HINFO, "SNSM: become the CCo.\n");
                            
                                staInfo->ccoScb->staStatus.fields.apptCcoStatus = 0;

                                //perform handover switch to the CCo role
                                //CRM has all SCBs for each STA from the  
                                //CC_HANDOVER_INFO.IND,

                                staInfo->ccoScb = staInfo->staScb; 
                                staInfo->ccoScb->staCap.fields.ccoStatus = 1;

                                //SNAM_PerformHoSwitch(snam);
                                SNAM_Stop(snam);

                                //the following function is moved 
                                //to the CNAM_START when the CCo starts
                                //start the tei timer for each STA
                                //cnam = LINKL_GetCnam(linkl);
                                //CNAM_PerformHoSwitch(cnam);
                                
                                //deliver the event to the upper layer
                                SNSM_DeliverEvent(snsm, EVENT_TYPE_CCO_HO_IND);

                                //Stop SNSM
                                STM_StopTimer(snsm->usttTimer);
                                STM_StopTimer(snsm->discAgingTimer); 

                                snsm->enableCcoSelection = 0;
                                snsm->enableCcoDetection = 0;
                                snsm->state = SNSM_STATE_INIT;
                                break;
                            }
                            case HPGP_HO_SWITCH_STA: //switch to the third STA
                            {

                                scb = CRM_AddScb(crm, snsm->nctei);
                                //scb = CRM_GetScb(crm, snsm->nctei);
                                if(scb == NULL)
                                {
                                    FM_Printf(FM_ERROR, "SNSM: Cannot allocate ascb for the CCo.\n");
                                    break;
                                }
                                
                                scb->staCap.fields.ccoStatus = 1;
                                staInfo->ccoScb = scb; 
#ifdef P8051
               FM_Printf(FM_HINFO, "SNSM: switch to the new CCo (tei: %bu).\n",
                                   scb->tei);
#else
               FM_Printf(FM_HINFO, "SNSM: switch to the new CCo (tei: %d).\n",
                                   scb->tei);
#endif
                                //for the handover, new CCo MAC address may be 
                                //known from the CCO_SET_TEI_MAP.IND
                                //otherwise , we may have a trouble in
                                //sending CC_ASSOC.REQ for TEI renew.
                                //Thus, we set HW MAC route table here for CCo`
#ifdef SIMU
                                SHAL_AddMacAddrToPortMap(
                                    HOMEPLUG_GetHal()->shal,
                                    scb->macAddr, 
                                    scb->tei);
#else
                                /* set the TEI in the data plane */

#endif
                                snsm->enableCcoDetection = 1;


                                break;
                            }
                            default:
                            {
                            }
                        }
                        snsm->hoSwitch = HPGP_HO_SWITCH_NONE;

                        if(snsm->ccoDetected)
                        {
                            //send the event to the SNAM to renew the TEI
                            newEvent = EVENT_Alloc(EVENT_DEFAULT_SIZE, 
                                                   EVENT_HPGP_CTRL_HEADROOM);
                            if(newEvent == NULL)
                            {
                                FM_Printf(FM_ERROR, "Cannot allocate an event.\n");
                                break;
                            }
                            newEvent->eventHdr.eventClass = EVENT_CLASS_CTRL;
                            newEvent->eventHdr.type = EVENT_TYPE_CCO_DISC_IND;
                            //send the event to the SNAM
                            //LINKL_SendEvent(linkl, newEvent);
                            SLIST_Put(&linkl->intEventQueue, &newEvent->link);
                            snsm->ccoDetected = 0;
                        }
                        break;
                    }
                    case EVENT_TYPE_TIMER_DISC_AGING_IND:
                    {
                        //SNSM_PerformAging(snsm);
                        scb = snsm->staInfo->staScb;
                        SCB_AgeDiscLists(scb);

                        //NOTE: noBcn is reset 
                        //after the central beacon is received
                        snsm->noBcn++; 

                        if(snsm->noBcn > NO_BCN_MAX)
                        {
                            //send event CCO_SEL_IND to the ctrl        
                            event = EVENT_Alloc(EVENT_DEFAULT_SIZE, 0);
                            if(event == NULL)
                            {
                                // Do not return from here start disc aging timer then return
                                FM_Printf(FM_ERROR, "Cannot allocate an event.\n");
                            }
                            else
                            {
                                //free all SCBs in the TEI map
                                CRM_Init(snsm->crm);

                                event->eventHdr.eventClass = EVENT_CLASS_CTRL;
                                event->eventHdr.type = EVENT_TYPE_CCO_LOST_IND;
                                //deliver the event to the upper layer
#ifdef CALLBACK
                                linkl->deliverEvent(linkl->eventcookie, event);
#else
                                CTRLL_ReceiveEvent(linkl->eventcookie, event);
#endif
                            }
                                 
                        } 


                        //restart the aging timer
                        STM_StartTimer(snsm->discAgingTimer, HPGP_TIME_DISC_AGING);   
                        break;
                    }
                    case EVENT_TYPE_SNSM_STOP:
                    {
                        //this happens when the STA leaves the network
                        STM_StopTimer(snsm->usttTimer);   
                        STM_StopTimer(snsm->discAgingTimer);   

                        snsm->enableCcoSelection = 0;
                        snsm->enableCcoDetection = 0;
						snsm->netSync = FALSE;
						snsm->netScan = FALSE;
                        snsm->state = SNSM_STATE_INIT;
                        break;
                    }
                    default:
                    {
                        
                        FM_Printf(FM_HINFO, "SNSM state connect %bu\n",event->eventHdr.type);
                    }
                }
            }
            break;
        }
        default: //SNSM_STATE_INIT
        {
            FM_Printf(FM_HINFO, "SNSM state %bu\n",snsm->state);
            //perform no operation
        }
    }
}





eStatus SNSM_Init(sSnsm *snsm, sLinkLayer *linkl)
{
    //sLinkLayer *linkLayer = (sLinkLayer *)HPGPCTRL_GetLayer(HP_LAYER_TYPE_LINK);
    
    snsm->linkl = linkl;
    snsm->staInfo = LINKL_GetStaInfo(linkl);
    snsm->crm = LINKL_GetCrm(linkl);

    snsm->state = SNSM_STATE_INIT;
    snsm->staRole = STA_ROLE_UNKNOWN;
    snsm->nctei = 0;
    snsm->hoEnabled = 0;
    snsm->discUpdate = 0;
    snsm->txDiscBcn = FALSE;
	snsm->netSync = FALSE;
	snsm->netScan = FALSE;

    snsm->hoSwitch = HPGP_HO_SWITCH_NONE;

#ifdef CALLBACK
    snsm->bbtTimer = STM_AllocTimer(LINKL_TimerHandler,
                         EVENT_TYPE_TIMER_BBT_IND, linkl);
#else
    snsm->bbtTimer = STM_AllocTimer(HP_LAYER_TYPE_LINK, 
                         EVENT_TYPE_TIMER_BBT_IND, linkl);
#endif
    if(snsm->bbtTimer == STM_TIMER_INVALID_ID)
    {
        return STATUS_FAILURE;
    }
#ifdef P8051
FM_Printf(FM_ERROR, "SNSM: bbt timer id: %bu.\n", snsm->bbtTimer);
#else
FM_Printf(FM_ERROR, "SNSM: bbt timer id: %d.\n", snsm->bbtTimer);
#endif

#ifdef CALLBACK
    snsm->usttTimer = STM_AllocTimer(LINKL_TimerHandler, 
                          EVENT_TYPE_TIMER_USTT_IND, linkl);
#else
    snsm->usttTimer = STM_AllocTimer(HP_LAYER_TYPE_LINK,
                          EVENT_TYPE_TIMER_USTT_IND, linkl);
#endif
    if(snsm->usttTimer == STM_TIMER_INVALID_ID)
    {
        return STATUS_FAILURE;
    }
#ifdef P8051
FM_Printf(FM_ERROR, "SNSM: ustt timer id: %bu.\n", snsm->usttTimer);
#else
FM_Printf(FM_ERROR, "SNSM: ustt timer id: %d.\n", snsm->usttTimer);
#endif

#ifdef CALLBACK
    snsm->discAgingTimer = STM_AllocTimer(LINKL_TimerHandler, 
                               EVENT_TYPE_TIMER_DISC_AGING_IND, linkl);
#else
    snsm->discAgingTimer = STM_AllocTimer(HP_LAYER_TYPE_LINK,
                               EVENT_TYPE_TIMER_DISC_AGING_IND, linkl);
#endif
    if(snsm->discAgingTimer == STM_TIMER_INVALID_ID)
    {
        return STATUS_FAILURE;
    }
#ifdef P8051
FM_Printf(FM_ERROR, "SNSM: disc aging timer id: %bu.\n", snsm->discAgingTimer);
#else
FM_Printf(FM_ERROR, "SNSM: disc aging timer id: %d.\n", snsm->discAgingTimer);
#endif
//    snsm->ccoInfo = LINKL_GetCcoInfo(linkLayer);
    return STATUS_SUCCESS;
}


//Start STA Mode
eStatus SNSM_Start(sSnsm *snsm, u8 staType)
{
    sEvent *newEvent = NULL;
    //sLinkLayer *linkl = (sLinkLayer *)HPGPCTRL_GetLayer(HP_LAYER_TYPE_LINK);

    newEvent = EVENT_Alloc(sizeof(sSnsmStartEvent), EVENT_HPGP_CTRL_HEADROOM);
    if(newEvent == NULL)
    {
        FM_Printf(FM_ERROR, "SNSM: Cannot allocate an event.\n");
        return STATUS_FAILURE;
    }
    
    newEvent->eventHdr.eventClass = EVENT_CLASS_CTRL;
    newEvent->eventHdr.type = EVENT_TYPE_SNSM_START;
    *(newEvent->buffDesc.dataptr) = staType;
//   if(staType == LINKL_STA_TYPE_ASSOC)
//    {
//        SLIST_Put(&linkl->intEventQueue, &newEvent->link);
//    }
//    else
//    {
        LINKL_SendEvent(snsm->linkl, newEvent);
//    }
    return STATUS_SUCCESS;
}

eStatus SNSM_Stop(sSnsm *snsm)
{
    sEvent *newEvent = NULL;
//    sLinkLayer *linkl = (sLinkLayer *)HPGPCTRL_GetLayer(HP_LAYER_TYPE_LINK);
    sLinkLayer *linkl = snsm->linkl;

    newEvent = EVENT_Alloc(0, EVENT_HPGP_CTRL_HEADROOM);
    if(newEvent == NULL)
    {
        FM_Printf(FM_ERROR, "SNSM: Cannot allocate an event.\n");
        return STATUS_FAILURE;
    }
    
    newEvent->eventHdr.eventClass = EVENT_CLASS_CTRL;
    newEvent->eventHdr.type = EVENT_TYPE_SNSM_STOP;
	// FIX: Need to process immediately
    LINKL_SendEvent(snsm->linkl, newEvent);
//    SLIST_Put(&linkl->intEventQueue, &newEvent->link);

    return STATUS_SUCCESS;
}
#endif /* STA_FUNC */
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


void SNSM_EnableHO(sSnsm *snsm)
{
    snsm->hoEnabled = 1;
}


void SNSM_EnableCcoDetection(sSnsm *snsm) //detect the central cco
{
    snsm->enableCcoDetection = 1;
}

/** =========================================================
 *
 * Edit History
 *
 * $Source: /home/cvsrepo/Hybrii_B_OSFW_Dev/firmware/hpgp/src/link/nsm.c,v $
 *
 * $Log: nsm.c,v $
 * Revision 1.11  2015/01/02 14:55:36  kiran
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
 * Revision 1.10  2014/11/11 14:52:58  ranjan
 * 1.New Folder Architecture espically in /components
 * 2.Modular arrangment of functionality in new files
 *    anticipating the need for exposing them as FW App
 *    development modules
 * 3.Other improvisation in code and .h files
 *
 * Revision 1.8  2014/08/12 08:45:43  kiran
 * 1) Event fixes
 * 2) API to change UART line control parameters
 *
 * Revision 1.7  2014/07/05 09:16:27  prashant
 * 100 Devices support- only association tested, memory adjustments
 *
 * Revision 1.6  2014/06/12 13:15:43  ranjan
 * -separated bcn,mgmt,um event pools
 * -fixed datapath issue due to previous checkin
 * -work in progress. neighbour cco detection
 *
 * Revision 1.5  2014/06/11 13:17:47  kiran
 * UART as host interface and peripheral interface supported.
 *
 * Revision 1.4  2014/06/05 08:38:41  ranjan
 * -flash function enabled for uppermac
 * - commit command after any change would flash systemprofiles
 * - verfied upper mac
 *
 * Revision 1.3  2014/05/07 20:11:08  yiming
 * For Mitsumi Test
 *
 * Revision 1.2  2014/03/10 05:58:10  ranjan
 * 1. added HomePlug BackupCCo feature. verified C&I test.(passed.) (bug 176)
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
 * Revision 1.45  2013/07/12 08:56:36  ranjan
 * -UKE Push Button Security Feature.
 * Verified : DirectEntry Security Works.Datapath Works.
 *                 command SetSecMode for UKE works.
 * Added against bug-160
 *
 * Revision 1.44  2013/03/14 11:49:18  ranjan
 * 1.handled cases  for CCo toSTA switch and  viceversa
 * 2.UM uses bcntemplate
 *
 * Revision 1.43  2012/12/14 11:06:57  ranjan
 * queue added for eth to plc datapath
 * removed mgmt tx polling
 *
 * Revision 1.42  2012/11/19 07:46:23  ranjan
 * Changes for Network discovery modes
 *
 * Revision 1.41  2012/10/25 11:38:48  prashant
 * Sniffer code added for MAC_SAP, Added new commands in MAC_SAP for sniffer, bridge,
 *  hardware settings and peer information.
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

