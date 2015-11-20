/** ========================================================
 *
 * @file nsm_sta.c
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
#include <assert.h>
#include "papdef.h"
#ifdef ROUTE
#include "hpgp_route.h"
#endif
#include "linkl.h"
#include "nsm.h"
#include "nam.h"
#include "muxl.h"
#include "nma.h"
#include "nma_fw.h"
#include "hpgpapi.h"
#include "hpgpconf.h"
#include "fm.h"
#include "ism.h"
#include "hpgpevt.h"
#include "mmsg.h"
#include "timer.h"
#include "stm.h"
#include "hal.h"
#ifdef HPGP_HAL
#include "hal_hpgp.h"
#else
#include "sdrv.h"
#endif
#include "frametask.h"
#ifndef CALLBACK
#include "hpgpapi.h"
#endif
#include "hybrii_tasks.h"
#include "sys_common.h"
#include "hal_hpgp_reset.h"
#include "hpgp_msgs.h"
#include "event_fw.h"

#ifdef LLP_POWERSAVE
u32 lastgCCO_BTS = 0;
#endif
#ifndef UART_HOST_INTF
#define DISC_BCN
#endif


#define HPGP_TIME_BBT                  1000   //2 seconds

#define HPGP_JOINNET_TIME        		3000
#define HPGP_TIME_USAI                  1000   //1 seconds
//#define HPGP_TIME_DISC_AGING            120000 // 2 minutes
#define HPGP_TIME_BEACON_LOSS           200 // test 

#define HPGP_TIME_DISC_AGING            500 // test 
#define HPGP_TIME_DISC_AGING_UNASSOC    500 // test

#define HPGP_TIME_BEACON_LOSS            200 // test 

#define DISC_STALL_TIME					5000
#define BCN_STALL_TIME					5000

#define HPGP_TIME_DISC_PERIOD_MAX       5000   //10 seconds - MaxDiscoverPeriod
//#define HPGP_TIME_STA_AGING_CNT       5  //5* 10 seconds - MaxDiscoverPeriod
#define HPGP_IDENTIFY_CAP_TIME               500
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

#ifdef LLP_POWERSAVE
extern u8 psDebug;
#ifdef PS_RESYNC
extern u8 resyncFlag;
#endif
#endif
extern u8 syncThres;
#ifdef BCN_ERR
volatile u8 bpstoFound = FALSE;
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
sDiscNetInfo *SCB_GetDiscNetEntry(u8 *nid);
void SCB_UpdateDiscStaList(sScb *scb, sDiscStaInfoRef *discStaInfoRef);
void SCB_AgeDiscLists(sScb *scb);
extern void setCSMA_onCCO1();
#ifdef ROUTE
extern void ROUTE_prepareHoldList(sCrm *crm, sScb *scb);
eStatus ROUTE_sendRouteInfoReq(sScb *scb);
extern eStatus ROUTE_sendRouteInfo(u16 mmType, sEvent *reqEvent);
#endif

#if 1
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
volatile u32 gPastRxBcnCount = 0;
#ifdef SW_RECOVERY

static u8 gStartDiscStallTimer = 0;
extern u8 gDiscStallCounter;
extern u8 gBcnStallCounter;
extern u32 gDiscStall;
extern u32 gBcnStall;
#endif
void showStaType(u8 stamode, u8 staType)
{
    if(stamode == LINKL_STA_MODE_STA)
    {
        switch(staType)
        {
            case LINKL_STA_TYPE_SC_JOIN: 
                FM_Printf(FM_USER, "STA TYPE JOIN\n");
                break;
            case LINKL_STA_TYPE_SC_ADD: 
                FM_Printf(FM_USER, "STA TYPE ADD\n");
                break;
            case LINKL_STA_TYPE_NETDISC: 
                FM_Printf(FM_USER, "STA NETWORK DISC\n");
                break;
            case LINKL_STA_TYPE_UNASSOC: 
                FM_Printf(FM_USER, "STA UNASSOC\n");
                break;
            case LINKL_STA_TYPE_UNASSOC_PASSIVE:
                FM_Printf(FM_USER, "STA TYPE UNASSOC PASSIVE\n");
                break;
            case LINKL_STA_TYPE_ASSOC:
                FM_Printf(FM_USER, "STA ASSOC\n");
                break;
            default:                
            {

            }

        }
    }
    else if(stamode == LINKL_STA_MODE_CCO)
    {
        switch(staType)
        {
            case LINKL_CCO_TYPE_UNASSOC: 
                FM_Printf(FM_USER, "CCO UNASSOC\n");
                break;
            case LINKL_CCO_TYPE_ASSOC: 
                FM_Printf(FM_USER, "CCO ASSOC\n");
                break;
            case LINKL_CCO_TYPE_HO: 
                FM_Printf(FM_USER, "CCO HANDOVER\n");
                break;
            default:
            {

            }
        }

    }

}

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
#if 0
            

void SNSM_UpdateAvlnList(sSnsm *snsm, sAvlnInfoRef *avlnInfo)
{
#ifdef MCCO		
	sLinkLayer	  *linkl = (sLinkLayer *)HPGPCTRL_GetLayer(HP_LAYER_TYPE_LINK);
	sStaInfo *staInfo = &linkl->staInfo;
#endif
		

    u8 i;
    u8 k;
    //search through the AVLN list 
    k = AVLN_LIST_MAX;
    for(i = 0; i < AVLN_LIST_MAX; i++)
    {
    	u8 weightAvg = 0;
        if(snsm->avlnInfo[i].valid == TRUE)
        {
            if((memcmp(snsm->avlnInfo[i].nid, avlnInfo->nid, NID_LEN-1) == 0) &&
               ((snsm->avlnInfo[i].nid[NID_LEN-1]&NID_EXTRA_BIT_MASK) ==
                (avlnInfo->nid[NID_LEN-1]&NID_EXTRA_BIT_MASK)))
            {
                //found the existing AVLN
                snsm->avlnInfo[i].hit = 1;

				weightAvg = ((snsm->avlnInfo[i].rssi *80)/100) + (( avlnInfo->rssi* 20)/100);
				snsm->avlnInfo[i].rssi = weightAvg;
				snsm->avlnInfo[i].lqi = avlnInfo->lqi;
				snsm->avlnInfo[i].tei = avlnInfo->tei;
				snsm->avlnInfo[i].numOfSta = avlnInfo->numOfSta;
				snsm->avlnInfo[i].bcnRxCnt++;
				
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
        snsm->avlnInfo[k].nid[NID_LEN-1] = avlnInfo->nid[NID_LEN-1]&NID_EXTRA_BIT_MASK;
        snsm->avlnInfo[k].hit = 1;
		snsm->avlnInfo[k].tei = avlnInfo->tei;		

		//FM_Printf(FM_USER, "TEI FOUND %bu\n", avlnInfo->tei);
		
		snsm->avlnInfo[k].rssi = avlnInfo->rssi;
		snsm->avlnInfo[k].lqi = avlnInfo->lqi;
		snsm->avlnInfo[k].numOfSta = avlnInfo->numOfSta;		
        snsm->avlnInfo[k].snid = avlnInfo->snid;
#ifdef MCCO		

		staInfo->slotUsage |= BIT(avlnInfo->slotId);
		snsm->avlnInfo[k].slotId = avlnInfo->slotId;//Kiran
	//	printf("\nMulti NW: Slot ID AVLN List : %bu\n",avlnInfo->slotId);//Kiran
#endif
        snsm->numAvln++;

    }
    else
    {
    
#ifndef RELEASE 
        FM_Printf(FM_WARN, "Anetfull\n");
#endif
    }
}

#endif


//aging the discovered STA and network lists
void SNSM_PerformAging(sSnsm *snsm)
{
    u8 i;
//    sLinkLayer *linkl = NULL;
//    sEvent *event = NULL;
    sScb   *scb = NULL;

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
        
#if 0		
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
#endif        
        
    }
   SCB_AgeDiscLists(scb);
   
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
    u8           ccoCap;
    sCmUaStaInd *uaStaInfo;
#ifdef UKE	
    sCmJoinReq  *joinReq;
#endif

    sHpgpHdr    *hpgpHdr = (sHpgpHdr *)event->buffDesc.buff;
#if  0 // defined(LG_WAR)
    if(snsm->staInfo->lastUserAppCCOState != 0)
    {
        return FALSE;
    }
#endif

    if(event->eventHdr.type == EVENT_TYPE_CM_UNASSOC_STA_IND)
    {
         uaStaInfo = (sCmUaStaInd *) event->buffDesc.dataptr;

    //chceck if the NID is matched
        if(memcmp(snsm->staInfo->nid, uaStaInfo->nid, NID_LEN) != 0)
            return beCco;
        
        ccoCap  = uaStaInfo->ccoCap;

    }
#ifdef UKE	
    else
    if(event->eventHdr.type == EVENT_TYPE_CM_SC_JOIN_REQ)
    {
        if((snsm->staInfo->secMode != SEC_MODE_SC_ADD &&
            snsm->staInfo->secMode != SEC_MODE_SC_JOIN))
        {
            return beCco;
        }

        joinReq = (sCmJoinReq *) event->buffDesc.dataptr;
        
        ccoCap = joinReq->ccoCapability;
    }
#endif

    if(snsm->staInfo->staCap.fields.ccoCap > ccoCap)
    {
        beCco = TRUE;
    }
    else if (snsm->staInfo->staCap.fields.ccoCap == ccoCap)
    {
        //compare MAC address
        myMacAddr = snsm->staInfo->macAddr;
        staMacAddr = hpgpHdr->macAddr;      
#ifdef NSM_STA_PRINT		                          
FM_Printf(FM_MINFO,"my MAC Address:%.2x:%.2x:%.2x:%.2x:%.2x:%.2x\n",
                    myMacAddr[0], myMacAddr[1],
                    myMacAddr[2], myMacAddr[3],
                    myMacAddr[4], myMacAddr[5]);
FM_Printf(FM_MINFO,"peer MAC Address:%.2x:%.2x:%.2x:%.2x:%.2x:%.2x\n",
                    staMacAddr[0], staMacAddr[1],
                    staMacAddr[2], staMacAddr[3],
                    staMacAddr[4], staMacAddr[5]);
#endif
            if((memcmp(myMacAddr, staMacAddr, MAC_ADDR_LEN) > 0))
            {
                beCco = TRUE;
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
        if(snsm->staInfo->numDiscNet)
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
    bcnHdr->nid[NID_LEN-1] = (bcnHdr->nid[NID_LEN-1]&NID_EXTRA_BIT_MASK)|(staInfo->hm<<6);
    bcnHdr->stei = staInfo->staScb->tei;

	bcnHdr->slotid = snsm->slotId;
	bcnHdr->slotusage = staInfo->slotUsage;
	
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
#ifdef BCNP                
				FM_Printf(FM_MINFO, "SNSM:disc bcn update:%d,%d\n", discInfoEntry->staCap.fields.update, staInfo->staScb->discUpdate);
#endif
                discInfoEntry->numDiscSta = staInfo->numDiscSta; 
                discInfoEntry->numDiscNet = staInfo->numDiscNet; 
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
                //snsm->bpstoOffset = bcnLen + sizeof(sBeHdr);
                snsm->bpstoOffset = ((u8 *)beHdr  + sizeof(sBeHdr)) -  ((u8 *)snsm->discBcnBuff );
                break;
            }
#ifdef POWERSAVE
            case BEHDR_PWR_SAVE:
            {
                if(staInfo->staScb->psState == PSM_PS_STATE_ON)
				{
					// in DISC beacon's PS bentry, the only pertinent field is STA's PSS
					sPowerSaveEntry *pPsBentry ;

    	            beHdr->beType = BEHDR_PWR_SAVE;
					beLen = sizeof(sPowerSaveEntry);
            	    dataptr += sizeof(sBeHdr);
					pPsBentry = (sPowerSaveEntry *) dataptr;
					memcpy(pPsBentry, 0, sizeof(sPowerSaveEntry));
    				pPsBentry->tpss = staInfo->staScb->pss;
				}
				break;
            }
#endif
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
#ifdef NSM_STA_PRINT	
        FM_Printf(FM_ERROR, "SNSM:disc bcn large\n");
#endif		
        return STATUS_FAILURE;
    }

    return STATUS_SUCCESS;

}

void SNSM_TransmitDiscBcn(sSnsm *snsm) 
{
    sTxDesc         txinfo;
    sBuffDesc       buffDesc;
    u8              offset = 0; 

    FM_Printf(FM_HINFO, "SNSM:>>DISC BCN\n");

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



    bcnHdr->nid[NID_LEN-1] &= NID_EXTRA_BIT_MASK;
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
#ifdef NSM_STA_PRINT			
                FM_Printf(FM_MINFO, "SNSM:Add a scb for CCo\n");
#endif				
                staInfo->ccoScb = CRM_AddScb(crm, bcnHdr->stei);
                //NOTE: Beacon event does not contain CCo's MAC adddr
                //The STA does not know CCo's MAC address until it associates
                //with the CCo or it sends a query or receives discovery beacon
                if(!staInfo->ccoScb) 
                {
                    FM_Printf(FM_WARN, "SNSM:can't get CCo scb\n");
					return bcnsrc;					
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

void SNSM_HandleBcnLoss(sSnsm *snsm, u8 type)
{

	sBcnHdr 	 *bcnHdr = NULL;
	u8			  becomeCco = FALSE;
	u8			  *bcn = NULL;
	sEvent		 *newEvent = NULL;
	sHpgpHdr	 *hpgpHdr = NULL;
	sCmUaStaInd  *uaStaInfo = NULL;
	sScb		 *scb = NULL;
	sStaInfo	 *staInfo = NULL; 
	sCrm		 *crm = NULL;
	sEvent *event;
	sSnam		 *snam = NULL;
	
	//	  sCnam 	   *cnam = NULL;
	sLinkLayer	 *linkl  = HPGPCTRL_GetLayer(HP_LAYER_TYPE_LINK);
	

	staInfo = LINKL_GetStaInfo(linkl); 
	crm = LINKL_GetCrm(linkl);
	snam = LINKL_GetSnam(linkl);

	if (!snsm->enableBcnLossDetection)
	{
		return;

	}

	//type = *(iEvent->buffDesc.dataptr);
	
	if((type == MAX_NO_BEACON_NW_DISCOVERY) ||
	   (type == MAX_NO_BEACON_BACKUPCCO))
	{
		sCcoLostIndEvent *ccoLostInd;

		

		if (type ==  MAX_NO_BEACON_NW_DISCOVERY)
		{

			Host_SendIndication(HOST_EVENT_BCN_LOSS, HPGP_MAC_ID, NULL, 0);
			//send event CCO_SEL_IND to the ctrl		
			event = EVENT_Alloc(1, EVENT_HPGP_CTRL_HEADROOM);

			if(event == NULL)
			{
				FM_Printf(FM_ERROR, "EAF\n");
                return;
			}


			event->eventHdr.eventClass = EVENT_CLASS_CTRL;
			event->eventHdr.type = EVENT_TYPE_CCO_LOST_IND;

		
			ccoLostInd = (sCcoLostIndEvent *)event->buffDesc.dataptr;		
			//free all SCBs in the TEI map
			CRM_Init(snsm->crm);

			
			//SNSM_Stop(&linkl->staNsm);
			//SNAM_Stop(&linkl->staNam);
			

			ccoLostInd->reason	 =	0;
#ifdef SW_RECOVERY

			gStartDiscStallTimer = 1;

#endif

			snsm->state = SNSM_STATE_INIT;

			//deliver the event to the upper layer
#ifdef CALLBACK
			linkl->deliverEvent(linkl->eventcookie, event);
#else
			CTRLL_ReceiveEvent(linkl->eventcookie, event);
#endif
			snsm->enableBcnLossDetection = 0;

		}
		else
		{
			if (snsm->enableBackupCcoDetection  == 0)
			{
		
			Host_SendIndication(HOST_EVENT_PRE_BCN_LOSS, HPGP_MAC_ID, NULL, 0);
			if(staInfo->staCap.fields.backupCcoStatus)
			{

				
					
					
				//send event CCO_SEL_IND to the ctrl		
				event = EVENT_Alloc(1, EVENT_HPGP_CTRL_HEADROOM);

				if(event == NULL)
				{
					FM_Printf(FM_ERROR, "EAF\n");
                    return;
				}

				HHAL_SetSWStatReqScanFlag(REG_FLAG_CLR);

				event->eventHdr.eventClass = EVENT_CLASS_CTRL;
				event->eventHdr.type = EVENT_TYPE_CCO_LOST_IND;

				ccoLostInd = (sCcoLostIndEvent *)event->buffDesc.dataptr;		

				//perform handover switch to the CCo role
				//CRM has all SCBs for each STA from the  
				//CC_HANDOVER_INFO.IND,
				
			    SNSM_Stop(&linkl->staNsm);
        		SNAM_Stop(&linkl->staNam);
				
				staInfo->ccoScb = staInfo->staScb; 
				staInfo->ccoScb->staCap.fields.ccoStatus = 1;

				staInfo->staCap.fields.backupCcoStatus = 0;
				staInfo->staCap.fields.pcoStatus = 0;

				//deliver the event to the upper layer

				
				ccoLostInd->reason	=  1;
#ifdef CALLBACK
				linkl->deliverEvent(linkl->eventcookie, event);
#else
				CTRLL_ReceiveEvent(linkl->eventcookie, event);
#endif
			}
			else
			{
				snsm->enableBackupCcoDetection = 1;
			}
			}

		}


	} 

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
#ifdef ROUTE
    sScb           *scb = NULL;
#endif
	u16 lLen = sizeof(sFrmCtrlBlk) + sizeof(sHybriiRxBcnHdr);

		/*Compiler warning suppression*/
		discNetInfoRef = discNetInfoRef;
	
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
#ifdef NSM_STA_PRINT			
                FM_Printf(FM_HINFO, "SNSM:Detect the CCo\n");
#endif				
                snsm->enableCcoDetection = 0;
                snsm->ccoDetected = 1;
            }
            break;
        }
        case BCN_SRC_DISC:
        {
#ifdef P8051
            FM_Printf(FM_HINFO, "SNSM:<<DISC BCN(L) tei: %bu\n", bcnHdr->stei);
#else
            FM_Printf(FM_HINFO, "SNSM:<<DISC BCN(L) tei: %d\n", bcnHdr->stei);
#endif
            
#ifdef LG_WAR
            return;
#endif
            break;
        }
        case BCN_SRC_OTHER_CCO:
        {
            //It is a CCo from other networks
			#if 0  // UpdateNet list happens in SNSM_ProcEvent start
			discNetInfoRef.snid = rxdesc->snid;			
						
			discNetInfoRef.rssi = bcn[PLC_BCNRX_LEN-4-lLen]; 
			discNetInfoRef.lqi = bcn[PLC_BCNRX_LEN-3-lLen];


            //update the network list
            discNetInfoRef.nid = bcnHdr->nid;
            discNetInfoRef.hybridMode = nid7>>6;
            discNetInfoRef.netMode = bcnHdr->nm;
            discNetInfoRef.numBcnSlots = bcnHdr->numslots;
			discNetInfoRef.slotId = bcnHdr->slotid;											
//            discNetInfoRef.bpsto = snsm->bpsto; //TODO: how to get the bpsto 


			if (bcnHdr->bt != BEACON_TYPE_CENTRAL)
			{

				break;

			}
			

            SCB_UpdateDiscNetList(snsm->staInfo->staScb, &discNetInfoRef);
			#endif
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
                if (bcnsrc == BCN_SRC_CCO)
				{
					if(memcmp(staInfo->ccoScb->macAddr, macAddr, MAC_ADDR_LEN))
                {
                    //the central beacon from the backup CCo
                    //perform the CCo switch 
                    //(let the previous CCo scb aging out)

					
					if (snsm->enableBackupCcoDetection)
					{
						staInfo->ccoScb = CRM_AddScb(snsm->crm, bcnHdr->stei);

						//send CCO_DISC.IND to the SNAM to renew TEI with the
						//backup CCo
						snsm->ccoDetected = 1;

						gHpgpHalCB.syncComplete = 0; // forcing re-sync to new Backup CCo

						snsm->netSync = 0;
						
						snsm->enableBackupCcoDetection = 0;
#ifndef RELEASE

						FM_Printf(FM_HINFO, "SNSM:switch to backup CCo\n");
#endif
						}
					}
					else
					{
						snsm->enableBackupCcoDetection = 0;
					}
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

					discStaInfoRef.slotId = bcnHdr->slotid;
					
					discStaInfoRef.slotUsage = bcnHdr->slotusage;
					
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
#ifdef ROUTE
                    ROUTE_update(bcnHdr->stei);                    
#endif
                }
                else
                {
#ifndef RELEASE
                    FM_Printf(FM_WARN, "SNSM:Unknown disc STA in Bcn\n");
#endif
                }
                break;
            }
            case BEHDR_ENCRYP_KEY_CHANGE:
            {
                break;
            }
#ifdef HOM
            case BEHDR_CCO_HANDOVER:
            {
                ccoHo = (sCcoHoEntry *)dataptr;
                FM_Printf(FM_HINFO, "SNSM:HO cntdown(%d)\n", ccoHo->hcd);
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
#endif
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
void SNSM_BcnCheck(u8* bcn)
{
	u8				nbe = 0;
	u8			xdata   *dataptr = NULL;
	sBcnHdr 	xdata   *bcnHdr = NULL;
	sBeHdr      xdata   *beHdr = NULL;

	bcnHdr = (sBcnHdr *)bcn;
	nbe = bcnHdr->nbe;
	dataptr = bcn + sizeof(sBcnHdr);
	beHdr = (sBeHdr *) dataptr;
	#ifdef BCN_ERR
	bpstoFound = FALSE;
	#endif
	//(2) Process Beacon Management Information (BMI)
	//Note: According to the standard, the BENTRYs within the MBI shall 
	//be arranged in increasing order of their BEHDR values.

	//FM_HexDump(FM_DATA|FM_MINFO, "Rx Bcn:",
	//		   bcn, PLC_BCNRX_LEN);
	while(nbe)
	{ 
		dataptr += sizeof(sBeHdr); //move to the start of BEENTRY 
		switch (beHdr->beType)
		{
			case BEHDR_BPSTO:
            {
                
#ifdef BCN_ERR
                bpstoFound = TRUE;
				break;
#endif		
			}
			default:
			break;
		}
		 dataptr = dataptr + beHdr->beLen; 
        beHdr = (sBeHdr *) dataptr;
        nbe--;
	}

}
	  
void SNSM_ProcBcnHigh(sSnsm *snsm, u8* bcn, u8 snid, u32 bts)
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
	sHpgpHalCB *hhalCb = HOMEPLUG_GetHal()->hhalCb;
		
#if defined(POWERSAVE) || defined(LLP_POWERSAVE)
	sLinkLayer    *linkl = (sLinkLayer *)HPGPCTRL_GetLayer(HP_LAYER_TYPE_LINK);
	sPowerSaveEntry *pPsBentry ;
#endif

 //FM_Printf(FM_HINFO, "SNSM_ProcBcnHigh\n");
#if 1 
	// we cannot do NID comparision as we might be syncing to
	//  non-NID match CCo to send UNASSOC IND
	
//	if ((memcmp(staInfo->nid, bcnHdr->nid, NID_LEN) == 0) &&

	if ((hhalCb->nwSelected)&&(hhalCb->snid == snid))

#else
#endif

    {


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

	    bcnHdr->nid[NID_LEN-1] &= NID_EXTRA_BIT_MASK;

        if( (bcnHdr->bt == BEACON_TYPE_CENTRAL)||
            (bcnHdr->bt == BEACON_TYPE_PROXY))
        {
            //it should not occur
#ifdef NSM_STA_PRINT			
            FM_Printf(FM_MMSG|FM_LINFO, "SNSM:<<CENTRAL/PROXY BCN(H)\n");
#endif		
	    if(bcnHdr->bt == BEACON_TYPE_CENTRAL && staInfo->ccoScb != NULL)
            {
            	u8 weightAvg;
                u16 lLen = sizeof(sFrmCtrlBlk) + sizeof(sHybriiRxBcnHdr);			
				
				weightAvg = ((staInfo->ccoScb->bRssi  *80)/100) +
							 (( bcn[PLC_BCNRX_LEN-4-lLen] * 20)/100);

				
                staInfo->ccoScb->bRssi = weightAvg; 
                staInfo->ccoScb->bLqi = bcn[PLC_BCNRX_LEN-3-lLen];
               // FM_Printf(FM_USER,"R %bu ",staInfo->ccoScb->bRssi);
               // FM_Printf(FM_USER,"L %bu\n",staInfo->ccoScb->bLqi);
            }	
            bcnsrc = BCN_SRC_CCO;
        }
        else
        {
#ifdef P8051
            FM_Printf(FM_HINFO, "SNSM:<<DISC BCN(H) tei: %bu\n", bcnHdr->stei);
#else
            FM_Printf(FM_HINFO, "SNSM:<<DISC BCN(H) tei: %d\n", bcnHdr->stei);
#endif
            bcnsrc = BCN_SRC_DISC;

#ifdef LOG_FLASH
			logEvent(DISC_BCN_LOG,0,0,&bcnHdr->stei,1);
#endif

//#ifdef LG_WAR
            return;  // we do not Sync or adapt region from Discovery beacon
//#endif

        }
    }
    else
    {

#if 1
            bcnsrc = BCN_SRC_OTHER_CCO;
#ifdef BCN_ERR
     		bpstoFound = TRUE;
#endif
            
            return;  // we do not sync or region adapt to any central beacon
                     // unless it is nwselected
#else

        if( (bcnHdr->bt == BEACON_TYPE_CENTRAL)||
            (bcnHdr->bt == BEACON_TYPE_PROXY))
        {
            bcnsrc = BCN_SRC_OTHER_CCO;
            // Process other bcn for avln detection
            if(snsm->state != SNSM_STATE_NET_DISC)
            {            			
#ifdef BCN_ERR
					bpstoFound = TRUE;
#endif
                return; 
            }
        }
        else
        {
            bcnsrc = BCN_SRC_OTHER_DISC;
            //not interested in other discover beacons at present			
#ifdef BCN_ERR
     		bpstoFound = TRUE;
#endif
            
            return; // TODO TBD
        }
#endif

    }

    bcnHdr->nid[NID_LEN-1] = nid7;

	if((bcnHdr->bt == BEACON_TYPE_CENTRAL)||
	   (bcnHdr->bt == BEACON_TYPE_PROXY))
	{	  
		
		hhalCb->bcnmisscnt = 0;
#ifdef LLP_POWERSAVE
		lastgCCO_BTS = gCCO_BTS;
#endif
		gCCO_BTS = bts;
		hhalCb->bts = bts;
		hhalCb->bcnDetectFlag = 1;

		staInfo->slotUsage |= BIT(bcnHdr->slotid);

		snsm->slotId = bcnHdr->slotid;

		snsm->noBcn = 0;
	}


    /* see if the beacon region exists */
    region = snsm->region;
    j = 0;
    if (bcnHdr->numslots)
    {
        /* need to include beacon region */
        region[j].startTime = 0;
        region[j].endTime = region[j].startTime + (bcnHdr->numslots * HPGP_BEACON_SLOT_ATU);
        region[j].regionType = REGION_TYPE_BEACON;
        region[j].rxOnly = 1;
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
                u16 et = 0;
                u16 st = 0;
                u8 cscd;
                static u8 changeSch = 1;
                
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
                cscd = ((scd >> 3) & 0x7);
                
                if(changeSch == 1)
                {
                        
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
                                st = (saiwst->stHi << 8) | saiwst->stLo;
                                if(et != st)
                                {
                                    region[j].startTime =  0;//et;
                                    region[j].endTime = st;
                                    region[j].hybridMd  = 1;
        							region[j].rxOnly = 1;
                                    region[j].regionType = REGION_TYPE_STAYOUT;

                                }
                                et = endTime;
                                j++;
                                region[j].startTime =  0;//st;
                                region[j].endTime = endTime;// - region[j].startTime;
                                
        						if ((saiwst->glid == (HPGP_GLID_SHARED_CSMA & 0x7F)) || 
        						    (saiwst->glid == (HPGP_GLID_GPP & 0x7F))) 
        						{
        						    region[j].hybridMd  = 1;
        							region[j].rxOnly = 0;
        							region[j].regionType = REGION_TYPE_SHARED_CSMA;
        						}
        						else
        						{
        						    region[j].regionType = REGION_TYPE_LOCAL_CSMA;
                                    region[j].hybridMd = 1;
                                    region[j].rxOnly = 0;
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
                                region[j].startTime =  0;//endTime;
                                endTime =  (saiwost->etHi << 8) | saiwost->etLo; 
                                region[j].endTime = endTime;// - region[j].startTime;
        						if ((saiwst->glid == (HPGP_GLID_SHARED_CSMA & 0x7F)) ||
        						    (saiwost->glid == (HPGP_GLID_GPP & 0x7F))) 
        						{
        						    region[j].hybridMd = 1;
        							region[j].rxOnly = 0;
        				            region[j].regionType = REGION_TYPE_SHARED_CSMA;
        						}
        						else
        						{
        						    region[j].regionType = REGION_TYPE_LOCAL_CSMA;
                                    region[j].hybridMd = 1;
                                    region[j].rxOnly = 0;
        						}
                                et = endTime;
                                j++;
                            }
                            else
                            {
                                endTime =  (saiwost->etHi << 8) | saiwost->etLo; 
                                et = endTime;
                            }
                            ptr += sizeof(sSaiWithoutSt);
                        }
                        i++;
                    }
        			for (;j < HPGP_REGION_MAX;j++)
        			{
                        region[j].startTime  = 0;//endTime;
                        region[j].rxOnly  = 1;
                        region[j].endTime   = 0xFFF;
                        region[j].hybridMd   = 1;  
        			    region[j].regionType = REGION_TYPE_STAYOUT;
                        snsm->regionNum = j + 1;
                    }
        			//else
                    {
                        snsm->regionNum = HPGP_REGION_MAX;
                    }
                    //snsm->regionNum = HPGP_REGION_MAX;
//#ifdef HPGP_HAL
                    HHAL_SetCsmaRegions(snsm->region, snsm->regionNum);
                    changeSch = 0;

//#endif
              //      for (j=0; j<snsm->regionNum; j++) {
              //          FM_Printf(FM_USER, "SNSM: region %bu, start: 0x%x, endTime: 0x%x rxOnly: %bu\n", 
              //              j, snsm->region[j].startTime, snsm->region[j].endTime, snsm->region[j].rxOnly);
              //      }
                }
          //      printf("cscd : %bu \n", cscd);

                if(cscd == 0 || cscd == 1) // if we loss bcn (cscd = 0) still it will modify sch
                {
                    changeSch = 1;
                    
                }
                break;
            }
            case BEHDR_REGIONS:
            {
                beHdrRef[BEHDR_REGIONS] = beHdr;
                //bcnRef.regionEntry = beHdr;
               // setCSMA_onCCO1();
                break;
            }
            case BEHDR_DISCOVER:
            {
#ifdef P8051
FM_Printf(FM_HINFO, "SNSM:get discBcn sched(%bu)\n", *dataptr);
#else
FM_Printf(FM_HINFO, "SNSM:get discBcn sched(%d)\n", *dataptr);
#endif
                if( (staInfo->staScb)&&(staInfo->staScb->tei == *dataptr) )
                {
                    //transmit the discover beacon
#ifdef LG_WAR
                    snsm->txDiscBcn = FALSE;
#else
                    snsm->txDiscBcn = TRUE;
#endif
                }
                break;
            }
            case BEHDR_BPSTO:
            {
                
#ifdef BCN_ERR
                bpstoFound = TRUE;
#endif

#ifdef HPGP_HAL
                /* TODO: perform sync with the any net
                 * during the network discovery 
                 */
                /* set it to MAC */
            // Process other bcn also to sync network
            	//if((bcnsrc == BCN_SRC_CCO) || (bcnsrc == BCN_SRC_OTHER_CCO))
            	if( (bcnHdr->bt == BEACON_TYPE_CENTRAL)||
                    (bcnHdr->bt == BEACON_TYPE_PROXY))
                {

                    memcpy(snsm->bpsto, dataptr, 3);
                    if ( HHAL_SyncNet(snsm->bpsto) == STATUS_SUCCESS 
                      && !snsm->netSync )
                    {
                        /* stop scan if it is my network */
                        //HAL_ScanNet(FALSE);

                        
                        snsm->netSync = TRUE;                        
#ifndef RELEASE
                        FM_Printf(FM_MINFO, "SNSM:Setting netSync %bu\n", &snsm->bpsto[0]);    					                     			                                
#endif
                    }
#if 0

                    /*FIXME: Host App should select the network , during scanning.
                                     Selection could be based on Lqi/Rssi 
                                     Temporarily the selection is done here */				
                    if(snsm->netScan && !hhalCb->nwSelected)
                    {
    				// TODO FIX THIS
    				
                        if ((hhalCb->nwSelectedSnid == 0)  || 
                            (hhalCb->nwSelectedSnid == snid))
                        {
                            if(hhalCb->halStats.RxGoodBcnCnt >= syncThres ) 
                            {                            
                                HHAL_SetSnid(snid);
                                FM_Printf(FM_MINFO, "SNSM:Setting STA Snid %bu\n", snid);                             
                            }
                        }
                    }
                    
#endif

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
            case BEHDR_MAC_ADDR:
            {
                break;
            }
#ifdef POWERSAVE
            case BEHDR_PWR_SAVE:
            {
                if(staInfo->staScb)
				{
					pPsBentry = (sPowerSaveEntry *) dataptr;
	    			if (pPsBentry->spsf == TRUE)
					{
						// CCO wants to disable PS in AVLN
						hhalCb->psAvln = FALSE;	// AVLN PS = OFF

//              	  	FM_Printf(FM_MMSG, "CENTRAL BCN RX: staInfo=%p, staInfo->staScb=%p, staInfo->staScb->psState=%bu\n", 
//							staInfo, staInfo->staScb, staInfo->staScb->psState);
						if (staInfo->staScb->psState == PSM_PS_STATE_ON)
						{
							// if Stop SP Flag is set by CCO, STA is not required to send 
							// a PS_EXIT.REQ.
							// it just clears its PS mode
							PSM_ForcePsOff(staInfo->staScb); 
//							PSM_set_sta_PS(FALSE, 0xF);
						}
					}		
					else
					{					
						hhalCb->psAvln = TRUE;	// AVLN PS = ON
					}
					staInfo->ccoScb->psState = ((pPsBentry->tpss & 0x0f) == 0x0f) ? PSM_PS_STATE_OFF:PSM_PS_STATE_ON;  
					staInfo->ccoScb->pss = pPsBentry->tpss;
					staInfo->staScb->bpCnt = pPsBentry->bpCnt_Lo + (pPsBentry->bpCnt_Hi << 8);
/*
					if (psDebug)
						printf("SNSM_ProcBcnHigh: received bpCnt=%d, pssi=%bu\n", staInfo->staScb->bpCnt, staInfo->staScb->pssi);
*/
					if (staInfo->staScb->pssi != pPsBentry->pssi)
					{
						// PS State of AVLN has changed
						if (staInfo->staScb->psState != PSM_PS_STATE_WAITING_ON)
						{
							// if STA is waiting to enter PS mode, its AWD should be 0
							staInfo->staScb->pssi = pPsBentry->pssi;
							PSM_recalc_AW(DEV_MODE_STA);	// recalculate common Awake Window
						}
					}
#ifdef DOTHIS
					u8 numPsSta;
					u8 *tmpDataPtr;

					tmpDataPtr = dataptr + sizeof(sPowerSaveEntry);
					numPsSta = (beHdr->beLen - sizeof(sPowerSaveEntry)) / 2;
					for (i=0; i< numPsSta; i++)
					{
//						printf("numPsSta=%bu,beHdr->beLen=%bu, TEI=%bu, pss=0x%bx\n", numPsSta,  beHdr->beLen, tmpDataPtr[i*2], tmpDataPtr[(i*2)+1]);
					}
#endif
/*
					if (psDebug)
					{
						u8 i;
						u8 *tmpp = dataptr;

						for (i=0;i<beHdr->beLen;i++)
							printf("0x%bx ", tmpp[i]);
						printf("\n"); 
					}
*/
				}
				break;
           }
#endif


#ifdef LLP_POWERSAVE
            case BEHDR_PWR_SAVE:
            {
                if(linkl->hal->hhalCb->psSta && staInfo->staScb)	// STA's PS must be ON
				{
					pPsBentry = (sPowerSaveEntry *) dataptr;
	    			if (pPsBentry->spsf == TRUE)
					{
						if (staInfo->staScb->psState == PSM_PS_STATE_ON)
						{
							// CCO wants to disable PS in AVLN
							hhalCb->psAvln = FALSE;	// AVLN PS = OFF

//              	  		FM_Printf(FM_MMSG, "CENTRAL BCN RX: staInfo=%p, staInfo->staScb=%p, staInfo->staScb->psState=%bu\n", 
//								staInfo, staInfo->staScb, staInfo->staScb->psState);

							// clear PS mode
							PSM_ForcePsOff(staInfo->staScb); 
//							PSM_set_sta_PS(FALSE, 0xF);
						}
					}		
					else
					{
						u16 bpCnt = 0;					
						sPsSchedule tmpAwd;

						hhalCb->psAvln = TRUE;	// AVLN PS = ON
						bpCnt = pPsBentry->bpCnt_Hi << 8;
						bpCnt += pPsBentry->bpCnt_Lo;
#ifdef PSDEBUG
						if (psDebug2)
						{
//							if (staInfo->staScb->bpCnt && !(staInfo->staScb->bpCnt % staInfo->staScb->commAwd.numBp))
								printf("RX BpCnt %d (scb->bpCnt=%d) at CCO's NTB=%lu, NTB=%lu\n", bpCnt, staInfo->staScb->bpCnt,(bts*40)/1000000, 
									(rtocl(ReadU32Reg(PLC_NTB_REG))*40)/1000000);
						}
#endif
						if (staInfo->staScb->psState == PSM_PS_STATE_OFF)
						{
							if (pPsBentry->tpss && (pPsBentry->tpss != PSM_PSS_NOT_CONFIG))
							{
								PSM_cvrtPss_Awd(pPsBentry->tpss, &tmpAwd);	// convert PSS to AWD format
								if (!(bpCnt % tmpAwd.numBp)) // CCO does not necessarily send (bcnCnt mod	staInfo->staScb->commAwd.numBp) = 0
								{
									// CCO has PSmode enabled
									// enable STA's PS in a MOD bcn period
/*
									if (psDebug2)
										printf("Rx Bcn: detect MOD (bpCnt=%d) at NTB=%lu\n", bpCnt, (rtocl(ReadU32Reg(PLC_NTB_REG))*40)/1000000);
*/
									PSM_enable_PS(staInfo->staScb, pPsBentry->tpss, bpCnt, linkl->mode);			// enable PS
									PSM_save_PS_to_HalCB(PSM_PS_STATE_ON, pPsBentry->tpss);	// save PS data to HALCB
									memcpy(&staInfo->staScb->commAwd, &tmpAwd, sizeof(sPsSchedule));
							    	FM_Printf(FM_MMSG, "STA PS Mode is now ON (pss = 0x%bx)\n", staInfo->staScb->pss);
								}
							}
						}
						else
						{
							// PS mode is ON, check if pss was changed
							if (staInfo->staScb->pss != pPsBentry->tpss)
							{
								// disable PS mode
								// if PSS is changed to a different value
								// the next Rx bcn with MOD will enable PS 
								// with new PSS
								PSM_ForcePsOff(staInfo->staScb)	;
						    	FM_Printf(FM_MMSG, "STA PS Mode is now OFF\n");
							}
							else
							{
								// PS resync: make sure that the STA's bpCnt is the same with CCO's
								// if they are not, it mostly means that STA drifts
								// we need to resync with CCO
#ifdef DOMISSINGBPSTART
								if (((((bts*40)/1000000) - ((lastgCCO_BTS*40)/1000000)) >= (((gHpgpHalCB.curBcnPer*40)/1000000)*2)) &&
									(staInfo->staScb->bpCnt < bpCnt))
								{
									// we missed at least 1 bpStart
									// assume for now that it's only 1 bpStart is missed
									printf("MISSING BPSTART: bts=%lu, lastgCCO_BTS=%lu, gHpgpHalCB.curBcnPer=%lu, staInfo->staScb->bpCnt=%d, bpCnt=%d\n", 
										(bts*40)/1000000, (lastgCCO_BTS*40)/1000000, (gHpgpHalCB.curBcnPer*40)/1000000, staInfo->staScb->bpCnt, bpCnt);
									staInfo->staScb->bpCnt++;
								}
#endif
/*
								if (psDebug2)
								{
//									if (staInfo->staScb->bpCnt && !(staInfo->staScb->bpCnt % staInfo->staScb->commAwd.numBp))
										printf("diffBpCnt=%d BpCnt %d (scb->bpCnt=%d) at CCO's NTB=%lu, NTB=%lu, ZCNTB=%lu\n", diffBpCnt, bpCnt, staInfo->staScb->bpCnt);
								}
*/
								if (staInfo->staScb->bpCnt != bpCnt)
								{
							    	FM_Printf(FM_MMSG, "PS RESYNC: bpCnt = %u, staInfo->staScb->bpCnt = %u, pss=0x%bx, staInfo->staScb->commAwd.numBp=%u\n", 
										bpCnt, staInfo->staScb->bpCnt, staInfo->staScb->pss, staInfo->staScb->commAwd.numBp);
//									if (delaySyncCheck > 20)
									{
										staInfo->staScb->bpCnt = bpCnt;
										// disable PS for now and will enable it when receives MOD bpCnt from CCO
										PSM_ForcePsOff(staInfo->staScb);
									}
								}
							}
						}
					}
				}
				break;
           }
#endif

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

void LINKL_StaProcBcnHandler(void *cookie, sEvent *event,u32 bts)
{
    sLinkLayer     *linkl = (sLinkLayer *)cookie;
    sSnsm*         snsm = (sSnsm *)LINKL_GetSnsm(linkl);
    sHpgpHdr         *hpgpHdr = NULL;

    hpgpHdr = (sHpgpHdr *)event->buffDesc.buff;

    SNSM_ProcBcnHigh(snsm, event->buffDesc.dataptr, hpgpHdr->snid, bts);

    if(snsm->txDiscBcn)
    {
        SNSM_TransmitDiscBcn(snsm);
        snsm->txDiscBcn = FALSE;
    }
}


eStatus SNSM_SendMgmtMsg(sSnsm *snsm, u16 mmType)
{
    eStatus          status = STATUS_FAILURE;
    sEvent       xdata   *newEvent = NULL;
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
	sStaInfo        *staInfo = NULL;

	staInfo = snsm->staInfo;
	
    switch(mmType)
    {
        case EVENT_TYPE_CM_UNASSOC_STA_IND:
        {
            //prepare CM_UNASSOCIATED_STA_IND
            eventSize = MAX(sizeof(sCmUaStaInd), HPGP_DATA_PAYLOAD_MIN); 
            newEvent = EVENT_MgmtAlloc(eventSize, EVENT_HPGP_MSG_HEADROOM);
            if(newEvent == NULL)
            {
                FM_Printf(FM_ERROR, "EAF\n");
                return STATUS_FAILURE;
            }
            FM_Printf(FM_MMSG, "SNSM:>>CM_UNASSOC_STA.IND\n");
            newEvent->eventHdr.eventClass = EVENT_CLASS_MSG;
            newEvent->eventHdr.type = EVENT_TYPE_CM_UNASSOC_STA_IND;
            hpgpHdr = (sHpgpHdr *)newEvent->buffDesc.buff; 
            hpgpHdr->tei = 0xFF;
            hpgpHdr->mnbc = 1;
            hpgpHdr->mcst = 1;
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
            numSta = staInfo->numDiscSta;
            numNet = staInfo->numDiscNet;
            eventSize = MAX(HPGP_DATA_PAYLOAD_MIN,
                            2 + (u16)(numSta*sizeof(sCcDiscStaInfo)) + 
                            (u16)(numNet*sizeof(sCcDiscNetInfo))); 
            newEvent = EVENT_MgmtAlloc(eventSize, EVENT_HPGP_MSG_HEADROOM);
            if(newEvent == NULL)
            {
                FM_Printf(FM_ERROR, "EAF\n");
                return STATUS_FAILURE;
            }
            FM_Printf(FM_MMSG, "SNSM:>>CC_DISC_LIST.CNF\n");
            newEvent->eventHdr.eventClass = EVENT_CLASS_MSG;
            newEvent->eventHdr.type = EVENT_TYPE_CC_DISCOVER_LIST_CNF;

            hpgpHdr = (sHpgpHdr *)newEvent->buffDesc.buff; 
            hpgpHdr->tei = snsm->staInfo->ccoScb->tei;
            hpgpHdr->macAddr = snsm->staInfo->ccoScb->macAddr; 
            hpgpHdr->snid = snsm->staInfo->snid;			
			hpgpHdr->eks = staInfo->nekEks;
			
            dataptr = newEvent->buffDesc.dataptr; 
            //station information
            *dataptr = numSta;
            dataptr++;
            i = 0;
            while(numSta && (i < DISC_STA_LIST_MAX))
            {
                if(staInfo->discStaInfo[i].valid)
                { 
                    ccDiscStaInfo = (sCcDiscStaInfo *)dataptr;
                    memcpy(ccDiscStaInfo->macAddr, staInfo->discStaInfo[i].macAddr, 
                           MAC_ADDR_LEN);

                    ccDiscStaInfo->tei = staInfo->discStaInfo[i].tei; 
                    ccDiscStaInfo->sameNet = staInfo->discStaInfo[i].sameNet; 
                    ccDiscStaInfo->snid = staInfo->discStaInfo[i].snid; 
                    ccDiscStaInfo->staCap.byte = staInfo->discStaInfo[i].staCap.byte; 
                    ccDiscStaInfo->sigLevel = 0;//staInfo->discStaInfo[i].sigLevel; 
                    ccDiscStaInfo->avgBle = 0;//staInfo->discStaInfo[i].avgBle; 
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
            while(numNet && (i < DISC_NET_LIST_MAX))
            {
                if(staInfo->discNetInfo[i].valid)
                { 
                    ccDiscNetInfo = (sCcDiscNetInfo *) dataptr;
                    memcpy(ccDiscNetInfo->nid, staInfo->discNetInfo[i].nid, 
                           NID_LEN);

                    ccDiscNetInfo->snid = staInfo->discNetInfo[i].snid; 
                    ccDiscNetInfo->hybridMode = staInfo->discNetInfo[i].hybridMode; 
                    ccDiscNetInfo->numBcnSlots = staInfo->discNetInfo[i].numBcnSlots; 
                    ccDiscNetInfo->coordStatus = staInfo->discNetInfo[i].coordStatus; 
                    ccDiscNetInfo->offset = staInfo->discNetInfo[i].offset; 
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
            return status;
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

eStatus SNSM_DeliverEvent(sSnsm *snsm, u16 eventType)
{
    sEvent       *newEvent = NULL;
    sLinkLayer   *linkl = snsm->linkl;

    newEvent = EVENT_Alloc(sizeof(snsm->staRole), 0);
    if(newEvent == NULL)
    {
        FM_Printf(FM_ERROR, "EAF\n");
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

#if 1
	switch(event->eventHdr.type)
	{
		case EVENT_TYPE_CC_BCN_IND:
		{		
			sDiscNetInfoRef discNetInfoRef;
			u8 *dataptr;
			u8 nbe;			
			sBeHdr *beHdr;
			u8 *bcn = (u8*)event->buffDesc.dataptr;
			u16 lLen = sizeof(sFrmCtrlBlk) + sizeof(sHybriiRxBcnHdr);
			
			hpgpHdr = (sHpgpHdr *)event->buffDesc.buff;
			bcnHdr = (sBcnHdr *)bcn;		

			if (bcnHdr->bt != BEACON_TYPE_CENTRAL)
			{

				break;

			}
			
			rxdesc.snid = hpgpHdr->snid;  
				
			nbe = bcnHdr->nbe;
			dataptr = bcn + sizeof(sBcnHdr);
			beHdr = (sBeHdr *) dataptr;
						discNetInfoRef.nid = bcnHdr->nid;	
						discNetInfoRef.hybridMode = bcnHdr->nid[NID_LEN]>>6;				
						discNetInfoRef.snid = hpgpHdr->snid;
						discNetInfoRef.rssi = bcn[PLC_BCNRX_LEN-4-lLen]; 
						discNetInfoRef.lqi = bcn[PLC_BCNRX_LEN-3-lLen];
						discNetInfoRef.netMode = bcnHdr->nm;
						discNetInfoRef.numBcnSlots = bcnHdr->numslots;
						discNetInfoRef.slotId = bcnHdr->slotid;
						discNetInfoRef.slotUsage= bcnHdr->slotusage;														
#if 1 
		    while(nbe)
		    {
		        dataptr += sizeof(sBeHdr); //move to the start of BEENTRY 
		        switch (beHdr->beType)
		        {            
					case BEHDR_VENDOR_SPEC:
					{
						memcpy(&discNetInfoRef.vendor_ota, (u8*)dataptr, sizeof(svendorOta));
		//				discNetInfoRef.validVendorOta = 1;
					}
					break;	

					default:
					break;
		        }
		        dataptr = dataptr +  beHdr->beLen; 
		        beHdr = (sBeHdr *) dataptr;
		        nbe--;				
		    }			
#endif			
			SCB_UpdateDiscNetList(snsm->staInfo->staScb, &discNetInfoRef);
		}
		break;

		default:
		break;
	}
#endif		

    switch(snsm->state)
    {
        
        case SNSM_STATE_INIT:
        {
            
            if( (event->eventHdr.eventClass == EVENT_CLASS_CTRL) &&
                (event->eventHdr.type == EVENT_TYPE_SNSM_START) )
            {
            
                staType = *(event->buffDesc.dataptr);

				syncThres = (staInfo->macAddr[5] / 4);
					
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
                        //HHAL_SetDevMode(DEV_MODE_CCO,gHpgpHalCB.lineMode);
#ifndef RELEASE
                        FM_Printf(FM_MINFO,"nsm:LINKL_STA_TYPE_NETDISC,Set sta\n");
#endif

                        /* start scan */
                        //HAL_ScanNet(TRUE);
#endif
                       // if(staInfo->lastUserAppCCOState != 2)
                        {
                            STM_StartTimer(snsm->bbtTimer, HPGP_TIME_BBT);
                        }
                        STM_StartTimer(snsm->usttTimer, HPGP_TIME_USAI);   
						STM_StopTimer(snsm->discAgingTimer);
                                     
                        STM_StartTimer(snsm->discAgingTimer, 
                                       HPGP_TIME_DISC_AGING_UNASSOC);  

#ifdef SW_RECOVERY


						if (gStartDiscStallTimer)
						{
							gStartDiscStallTimer = 0;
							gPastRxBcnCount = gHpgpHalCB.halStats.RxGoodBcnCnt;// Store last bcn count to identify BCN stall

							STM_StartTimer(snsm->discStallTimer, DISC_STALL_TIME);
							//FM_Printf(FM_ERROR,"DISC STALL Timer Started\n");// only for debug. comment while release

						}
#endif
						gPastRxBcnCount = gHpgpHalCB.halStats.RxGoodBcnCnt;// Store last bcn count to identify BCN stall

						snsm->state = SNSM_STATE_NET_DISC;
                        break;
                    }
                    case LINKL_STA_TYPE_UNASSOC:
                    {
                    //    if(staInfo->lastUserAppCCOState == 0)
                        {
                            snsm->enableCcoSelection = 1;
                        }
                    //    else
                    //    {
                    //        snsm->enableCcoSelection = 0; // Joinnet - Only STA
                    //    }
                        snsm->enableCcoDetection = 1;
#ifdef HPGP_HAL 
                        /* set PhyClk */
                        //HHAL_SetDevMode(DEV_MODE_CCO, LINE_MODE_DC);                  
#ifndef RELEASE
                        FM_Printf(FM_MINFO,"nsm:LINKL_STA_TYPE_UNASSOC\n");
#endif

                        /* start scan */
                       // HAL_ScanNet(TRUE);
#endif

						STM_StopTimer(snsm->usttTimer);
                        STM_StartTimer(snsm->usttTimer, HPGP_TIME_USAI); 
						
						STM_StopTimer(snsm->discAgingTimer);
                         
                        STM_StartTimer(snsm->discAgingTimer, 
                                     HPGP_TIME_DISC_AGING_UNASSOC);   
#ifdef SW_RECOVERY
												
						if (gStartDiscStallTimer)
						{
							gStartDiscStallTimer = 0;
							gPastRxBcnCount = gHpgpHalCB.halStats.RxGoodBcnCnt;// Store last bcn count to identify BCN stall

							STM_StartTimer(snsm->discStallTimer, DISC_STALL_TIME);
					//		FM_Printf(FM_ERROR,"DISC STALL Timer Started\n");// only for debug. comment while release

						}
#endif		
						STM_StopTimer(snsm->scanToAssocTimer);
						STM_StartTimer(snsm->scanToAssocTimer, HPGP_JOINNET_TIME);


						snsm->state = SNSM_STATE_NET_DISC;
                        break;
                    }
                    case LINKL_STA_TYPE_ASSOC:
                    {
#ifndef RELEASE
                        FM_Printf(FM_MINFO, "SNSM:Start ASSOC STA\n");
#endif
                        snsm->enableCcoSelection = 0;
                        snsm->enableCcoDetection = 1;
						
						snsm->enableBcnLossDetection = 1;
						STM_StopTimer(snsm->discAgingTimer);
						
#ifdef SW_RECOVERY							
						gStartDiscStallTimer = 0;
						STM_StopTimer(snsm->discStallTimer);
						STM_StopTimer(snsm->bcnStallTimer);
						gDiscStallCounter = 0;
#endif								
                        STM_StartTimer(snsm->discAgingTimer, 
                                       HPGP_TIME_DISC_AGING);  

						STM_StartTimer(snsm->bcnLossTimer,
                                       HPGP_TIME_BEACON_LOSS);
						
                        snsm->state = SNSM_STATE_CONN;
                        break;
                    }
#ifdef UKE
                    
                    case LINKL_STA_TYPE_SC_JOIN:
                    case LINKL_STA_TYPE_SC_ADD:
                    {                      
                         
                        FM_Printf(FM_ERROR, "\n UKE:Push button\n 	\
						                     in SNSM_STATE_INIT state\n");
                        break;    
                    }
#endif					
                    default:
                    {
#ifndef RELEASE
                        FM_Printf(FM_WARN, "SNSM:unknown sta type(%d)\n", staType);
#endif
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
            u8 cco = 0;
            switch(event->eventHdr.type)
            {
#ifdef UKE			
                case EVENT_TYPE_CM_SC_JOIN_REQ: //UKE
                {
                    if(staInfo->secMode == SEC_MODE_SC_JOIN)
                    {
                        FM_Printf(FM_MMSG, "SNSM:<<CM_SC_JOIN.REQ\n");
                        // If associated
                        if(staInfo->secMode == SEC_MODE_SC_JOIN)
                        {
                            //  its assumed that is secMode == JOIN, then device is in sta mode
                            // if(snsm->enableCcoSelection)
                            {
                                //  perform CCO selection   
                                becomeCco = SNSM_SelectCco(snsm,  event);
                                // If CCO 
                                if(becomeCco)
                                {

                                    SNSM_Stop(snsm);
                                    
                                    //(2) stop the SNAM

                                    SNAM_Stop(snam);
                                            
                                    snsm->enableCcoSelection = 0;
                                    snsm->enableCcoDetection = 0;
        							snsm->netSync = FALSE;
        							snsm->netScan  = FALSE;

                                    
                                    staInfo->secMode = SEC_MODE_SC_ADD;
                                    //generateNmkAndNid(staInfo);
                                    
                                    
                                    STM_StopTimer(snsm->usttTimer);
                                    STM_StopTimer(snsm->discAgingTimer);
                                    
                                }
                            }
                        }else
                            {


                            }
                       
                        // If SC_ADD mode
                        if(staInfo->secMode == SEC_MODE_SC_ADD)
                        {
                            //send SC_JOIN.CNF
                            LINKL_SendMgmtMsg(snsm->staInfo, EVENT_TYPE_CM_SC_JOIN_CNF, 
                                        ((sHpgpHdr*)(event->buffDesc.buff))->macAddr);
                            
                            // If CCO start beacon
                            if(becomeCco)
                            {
                                //send event CCO_SEL_IND to the upper layer

                                CNAM_EnableAssocNotification(&linkl->ccoNam,
                                                    ((sHpgpHdr*)(event->buffDesc.buff))->macAddr);
                                SNSM_DeliverEvent(snsm, EVENT_TYPE_CCO_SLCT_IND);

                                    
                            }
                            
                        }
                        
                    }
                    break;
                }
                case EVENT_TYPE_CM_SC_JOIN_CNF: //UKE
                {
                    FM_Printf(FM_MMSG, "SNSM:<<CM_SC_JOIN.CNF\n");
                    if(staInfo->secMode == SEC_MODE_SC_JOIN)
                    {
                        sCmJoinCnf *joinCnf;

                        STM_StopTimer(snsm->bbtTimer);

                        // Save NID
                        joinCnf = (sCmJoinCnf *) event->buffDesc.dataptr;
                        memcpy(staInfo->nid, joinCnf->nid, NID_LEN);
                        
                        FM_HexDump(FM_HINFO, "NID in JOIN CNF Frame", joinCnf->nid, NID_LEN);

                        snsm->netSync = FALSE;
                        
                        gHpgpHalCB.halStats.RxGoodBcnCnt   = 0; // rajan  do we call scanNet here.

                        snsm->netScan  = FALSE;
                            
                        snsm->enableCcoSelection = 1;

                        snsm->enableCcoDetection = 1;                        

                        hpgpHdr = (sHpgpHdr *)event->buffDesc.buff;
                     
                        // not needed as STA will look for nid match
                        //memcpy(&linkl->akm.ukePeer,  hpgpHdr->macAddr, MAC_ADDR_LEN);
                            
                        STM_StartTimer(snsm->usttTimer, HPGP_TIME_USAI);   

						STM_StopTimer(snsm->discAgingTimer);
                        STM_StartTimer(snsm->discAgingTimer, 
                                         HPGP_TIME_DISC_AGING);
                        // IMP NOTE: What abt staCap fields
                    }
                    break;
                }
#endif				
                    case EVENT_TYPE_CM_UNASSOC_STA_IND:
                    {   
                        FM_Printf(FM_MMSG, "SNSM:<<CM_UNASSOC_STA.IND\n");
                        hpgpHdr = (sHpgpHdr *)event->buffDesc.buff;
                        uaStaInfo = (sCmUaStaInd *) (event->buffDesc.dataptr);
                        SNSM_UpdateUaStaList(snsm, uaStaInfo, hpgpHdr->macAddr);

                        //CCo selection is performed 
                        //when the STA is unassociated STA
						if(
#ifdef UKE						
                          (staInfo->secMode != SEC_MODE_SC_JOIN) &&
#else

#endif						
                          (snsm->enableCcoSelection))
                        {
                            becomeCco = SNSM_SelectCco(snsm, event);
                               
                            if(becomeCco == TRUE)
                            {
                                                    /*
#ifdef NSM_STA_PRINT							
                                FM_Printf(FM_MINFO, "SNSM:Auto-selected CCo.Scan Off\n");
#endif								
#ifdef HPGP_HAL

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
                                                        */
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
                    //            if(staInfo->lastUserAppCCOState == 0)
                                {
                                    snsm->enableCcoSelection = 1;
                                }
                     //           else
                      //          {
                      //              snsm->enableCcoSelection = 0; //  Only STA
                      //          }
                                snsm->enableCcoDetection = 1;
#ifdef HPGP_HAL
#ifndef RELEASE
                                FM_Printf(FM_MINFO,"nsm:LINKL_STA_TYPE_UNASSOC 2\n");
#endif
                                /* TODO: Set PhyClk in HW */
                                //HHAL_SetDevMode(linkl->hal, 
                                //    DEV_MODE_CCO, LINE_MODE_DC);
                                //HHAL_SetDevMode(linkl->hal, DEV_MODE_STA, gHpgpHalCB.lineMode);
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
                                	STM_StopTimer(snsm->discAgingTimer);
						
                        			STM_StartTimer(snsm->discAgingTimer, 
                                    		   HPGP_TIME_DISC_AGING_UNASSOC);
						
                                }
                                break;
                            }
                            case LINKL_STA_TYPE_ASSOC:
                            {
#ifdef HPGP_HAL
                                /* stop scan */
                                HAL_ScanNet(FALSE);
#ifndef RELEASE
                                FM_Printf(FM_MINFO,"nsm:LINKL_STA_TYPE_ASSOC\n");
#endif
#endif
                                //it occurs after the STA discover
                                //the CCo during the network discovery
                                //and unassoicated STA
                                STM_StopTimer(snsm->usttTimer);   
                                snsm->enableCcoSelection = 0;
                                snsm->enableCcoDetection = 0;
								
								snsm->enableBcnLossDetection = 1;
                                snsm->state = SNSM_STATE_CONN;
								STM_StopTimer(snsm->discAgingTimer);
							
								STM_StopTimer(snsm->bcnLossTimer);
								
								STM_StartTimer(snsm->discAgingTimer, 
												HPGP_TIME_DISC_AGING);  


								STM_StartTimer(snsm->bcnLossTimer, 
												HPGP_TIME_BEACON_LOSS);
#ifdef SW_RECOVERY								
								STM_StopTimer(snsm->discStallTimer);
								STM_StopTimer(snsm->bcnStallTimer);
								gDiscStallCounter = 0;
#endif								
                            break;
                        }
#ifdef UKE						
                        case LINKL_STA_TYPE_SC_JOIN:
                        {

                            FM_Printf(FM_ERROR , "LINKL_STA_TYPE_SC_JOIN\n");
                            
                            STM_StopTimer(snsm->discAgingTimer);   
                            STM_StopTimer(snsm->bbtTimer);
                            STM_StopTimer(snsm->usttTimer);
                           if(staInfo->secMode == SEC_MODE_SC_JOIN)
                            {
                                // If n/w discovery is on then Stop discovery
                                // Is associated
                                // if yes then leave n/w and wait for LEAVE RSP
                                //  
                                STM_StartTimer(snsm->bbtTimer, HPGP_TIME_BBT);
                                STM_StartTimer(snsm->usttTimer, HPGP_TIME_USAI);

                                snsm->enableCcoSelection = 0;
                                snsm->enableCcoDetection = 0;
                                
                                FM_Printf(FM_ERROR, "SNSM:SEC_MODE_SC_JOIN\n");
                            }                        
                            else
                            {
                                FM_Printf(FM_ERROR, "SNSM:SEC_MODE_SC or HS\n");
                            }
                                break;
                            }
#endif							
                            default:
                            {
                            }
                        }
                        break;
                    }

                    case EVENT_TYPE_SNSM_STOP:
                    {
                        //it happens when the STA becomes the CCo after
                        //network discovery
                        STM_StopTimer(snsm->usttTimer);   
                        STM_StopTimer(snsm->bbtTimer);
                        STM_StopTimer(snsm->discAgingTimer);   

                        snsm->enableCcoSelection = 0;
                        snsm->enableCcoDetection = 0;
						snsm->netSync = FALSE;
						snsm->netScan = FALSE;
                        HAL_ScanNet(FALSE);
                        snsm->state = SNSM_STATE_INIT;
                        break;
                    }

                    case EVENT_TYPE_TIMER_BBT_IND:
                    
#ifdef UKE					
                    if(staInfo->secMode == SEC_MODE_SC_JOIN)
                    {
                        FM_Printf(FM_ERROR, "nsm:BBT Exp\n");
                        staInfo->secMode = SEC_MODE_SC;

        //                                              SNSM_DeliverEvent(snsm, EVENT_TYPE_RESTART_IND);

                        snsm->enableCcoSelection = 1;
                        snsm->enableCcoDetection = 1;
                                    
#ifdef NSM_STA_PRINT
                        FM_Printf(FM_MINFO,"nsm:LINKL_STA_TYPE_UNASSOC From JOIN State\n");
#endif
                        STM_StartTimer(snsm->usttTimer, HPGP_TIME_USAI);

						STM_StopTimer(snsm->discAgingTimer);

                        STM_StartTimer(snsm->discAgingTimer, 
                                 HPGP_TIME_DISC_AGING_UNASSOC);
                        
                    }
                    else
#endif					
                    {
                        snsm->staRole = SNSM_DetermineStaRole(snsm);
                        if( (snsm->staRole == STA_ROLE_ACCO) ||
                            (snsm->staRole == STA_ROLE_UCCO) )
                        {
                            //the STA becomes the CCo after network discovery
#ifdef HPGP_HAL
                            /* stop scan */
                            //HAL_ScanNet(FALSE);
#ifndef RELEASE
                            FM_Printf(FM_MINFO, "nsm:BBT Exp\n");
#endif
#endif
                            //(1) stop the SNSM
                            STM_StopTimer(snsm->usttTimer);
                            STM_StopTimer(snsm->discAgingTimer);

                            snsm->enableCcoSelection = 0;
                            snsm->enableCcoDetection = 0;
                            //(2) stop the SNAM
                            snsm->netSync = FALSE;
                            snsm->netScan = FALSE;

                            LINKL_StopSta(linkl);

                            snsm->state = SNSM_STATE_INIT;
                        }
                        SNSM_DeliverEvent(snsm, EVENT_TYPE_NET_DISC_IND);
                        
                    }
						break;
                    case EVENT_TYPE_CC_BCN_IND:
                    {
                       // FM_Printf(FM_MINFO, "BCN\n");
                        hpgpHdr = (sHpgpHdr *)event->buffDesc.buff;
                        bcnHdr = (sBcnHdr *) event->buffDesc.dataptr;
#ifdef SW_RECOVERY						
						STM_StopTimer(snsm->bcnStallTimer);
						STM_StartTimer(snsm->bcnStallTimer,BCN_STALL_TIME);
						gBcnStallCounter = 0;
#endif						
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
                                STM_StopTimer(snsm->bbtTimer);
                                
                                if ((gHpgpHalCB.nwSelected) && 
									(gHpgpHalCB.snid != staInfo->snid))
								{

									FM_Printf(FM_MINFO,"switch sync snid %bu\n", staInfo->snid);

									snsm->netSync = FALSE;
									
									gHpgpHalCB.nwSelected = 0;
 
									gHpgpHalCB.nwSelectedSnid = staInfo->snid;

									snsm->netScan = FALSE;
									
									
								}
                                if (snsm->netSync == TRUE)
								{
                                if((snsm->enableCcoDetection)
#ifdef FREQ_DETECT
                                   && (gHpgpHalCB.gFreqCB.freqDetected == TRUE)
#endif
                                    )
                                {
#ifdef HPGP_HAL
                                    /* stop scan */
                                //FM_Printf(FM_MINFO,"nsm : enableCcoDetection , Scan OFF\n");
									
                                    //HAL_ScanNet(FALSE);
                                    //HHAL_SetDevMode(linkl->hal, DEV_MODE_STA, LINE_MODE_DC);
#endif
                                    snsm->enableCcoDetection = 0;
                                    //send event CCO_DISC_IND to the ctrl
                                    SNSM_DeliverEvent(snsm, EVENT_TYPE_CCO_DISC_IND);

								 	STM_StopTimer(snsm->scanToAssocTimer);
																		
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
									 
                                   FM_Printf(FM_MINFO,"nsm:BCN_SRC_CCO\n");
									 
								    //HHAL_SetDevMode(linkl->hal, DEV_MODE_STA, LINE_MODE_DC);
                                    HHAL_SetDevMode(DEV_MODE_STA,gHpgpHalCB.lineMode);
								    /* start scan for sync */
									HAL_ScanNet(TRUE);
                                    snsm->netScan = TRUE;
								}
								else if(snsm->netScan)									
								{

									/*FIXME: Host App should select the network , during scanning.
									Selection could be based on Lqi/Rssi 
									Temporarily the selection is done here */			  
									if(snsm->netScan && !gHpgpHalCB.nwSelected)
									{
										// TODO FIX THIS

										if ((gHpgpHalCB.nwSelectedSnid == 0)  || 
											(gHpgpHalCB.nwSelectedSnid == hpgpHdr->snid))
										{
											if(gHpgpHalCB.halStats.RxGoodBcnCnt >= syncThres ) 
											{ 						   
												HHAL_SetSnid(hpgpHdr->snid);
												FM_Printf(FM_MINFO, "SNSM:Setting STA Snid %bu\n", hpgpHdr->snid); 


											}
										}
									}
											  

								}
                                break;
                            }
                            case BCN_SRC_OTHER_CCO:
                            {
								u8 *bcn = event->buffDesc.dataptr;
 								u16 lLen = sizeof(sFrmCtrlBlk) + sizeof(sHybriiRxBcnHdr);
	 
								 avlnInfoRef.rssi = bcn[PLC_BCNRX_LEN-4-lLen]; 
								 avlnInfoRef.lqi = bcn[PLC_BCNRX_LEN-3-lLen];
								 avlnInfoRef.tei = bcnHdr->stei;

 
 //                               hpgpHdr = (sHpgpHdr *)event->buffDesc.buff;
                                avlnInfoRef.nid = bcnHdr->nid;
                                avlnInfoRef.snid = hpgpHdr->snid;
								avlnInfoRef.slotId = bcnHdr->slotid;
                                // If other beacon detected scan network. 
                               if(!snsm->netScan)
							   {
                                    staInfo->snid = hpgpHdr->snid;
								    HHAL_SetDevMode(DEV_MODE_STA,gHpgpHalCB.lineMode);
                                    HAL_ScanNet(TRUE);
                                    snsm->netScan = TRUE;
                                }
           
								   else if(snsm->netScan)								   
								   {
									   if(snsm->netScan && !gHpgpHalCB.nwSelected)
									   {
										   if ((gHpgpHalCB.nwSelectedSnid == 0)  || 
											   (gHpgpHalCB.nwSelectedSnid == hpgpHdr->snid))
										   {
											   if(gHpgpHalCB.halStats.RxGoodBcnCnt >= syncThres ) 
											   {						  
												   HHAL_SetSnid(hpgpHdr->snid);
												   FM_Printf(FM_MINFO, "SNSM:Setting STA Snid %bu\n", hpgpHdr->snid); 
											   }
										   }
									   }
								   }
                                //update the AVLN list
                              //  SNSM_UpdateAvlnList(snsm, &avlnInfoRef); 
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
					case EVENT_TYPE_TIMER_JOIN_TIMEOUT:
					{
						FM_Printf(FM_USER,"EVENT_TYPE_TIMER_JOIN_TIMEOUT\n");
						 SNSM_DeliverEvent(snsm, EVENT_TYPE_JOINNET_TIMEOUT);
						 
						break;
					}
                    case EVENT_TYPE_TIMER_USTT_IND:
                    {
                        //check AVLN list
                        if((staInfo->numDiscNet) && (snsm->netSync))
                    	{
#ifdef UKE					
	                        if(staInfo->secMode == SEC_MODE_SC_JOIN)
	                            
	                        {
	                            LINKL_SendMgmtMsg(snsm->staInfo,
												   EVENT_TYPE_CM_SC_JOIN_REQ,
												   NULL);

                        }
                        //check AVLN list
                        else
#endif						
                        {
                            //select the phyclk from the AVLN list
                            //set the phyclk in the MAC

	                            SNSM_SendMgmtMsg(snsm, 
	                            				 EVENT_TYPE_CM_UNASSOC_STA_IND);
	                        }

                    	}
						else
						{
							if(snsm->enableCcoSelection == 1)
							{				   
								if(staInfo->numDiscNet == 0)
								{
								snsm->enableCcoSelection = 0;
								// No avln become unassociated CCO
								/*
								snsm->staRole = STA_ROLE_UCCO;
								// stop ustt timer
								STM_StopTimer(snsm->usttTimer);
							
								snsm->enableCcoSelection = 0;
								snsm->enableCcoDetection = 0;
							
								snsm->netSync = FALSE;
								snsm->netScan = FALSE;
								snsm->state = SNSM_STATE_INIT;
							
								HAL_ScanNet(FALSE);
							
								LINKL_StopSta(linkl);
							
								snsm->state = SNSM_STATE_INIT;
							        */
								SNSM_DeliverEvent(snsm, EVENT_TYPE_UCCO_IND);
								break;								  
							
								}
							}

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
	        			STM_StopTimer(snsm->discAgingTimer);
						STM_StartTimer(snsm->discAgingTimer, HPGP_TIME_DISC_AGING_UNASSOC);                                   
						
	                        //Using enableCcoSelection to know that we are in USTA mode
							if(snsm->enableCcoSelection == 1)
	                        {                  
	                            if(staInfo->numDiscNet == 0)
	                            {
	                                snsm->enableCcoSelection = 0;
                                    /*
                            	                                // No avln become unassociated CCO
                            	                                snsm->staRole = STA_ROLE_UCCO;
                            	                                // stop ustt timer
                            	                                STM_StopTimer(snsm->usttTimer);
                            	                                
                            	                                snsm->enableCcoSelection = 0;
                            	                                snsm->enableCcoDetection = 0;

                            	                                snsm->netSync = FALSE;
                            	                                snsm->netScan = FALSE;
                            	                                snsm->state = SNSM_STATE_INIT;

                            	        						HAL_ScanNet(FALSE);

                            	        						LINKL_StopSta(linkl);

                            	                                snsm->state = SNSM_STATE_INIT;
                                                                    */
	                                SNSM_DeliverEvent(snsm, EVENT_TYPE_UCCO_IND);
	                                break;                                

                            }
                        }
                        //restart the aging timer
                        
                        break;
                    }
#ifdef SW_RECOVERY
					case EVENT_TYPE_TIMER_DISC_STALL_IND:
					{
						//FM_Printf(FM_ERROR,"\nDISC Stall Expired\n");
						if((!gHpgpHalCB.halStats.RxGoodBcnCnt) ||
							 (gHpgpHalCB.halStats.RxGoodBcnCnt - gPastRxBcnCount)< 30)
						{
							GV701x_Chip_Reset();// Temporary WAR as plc_tx_reset lead to Duplicate CP. Need to remove after DUP_CP resolve [Kiran]
							plc_reset_tx();
							plc_reset_rx();

							gDiscStall++;

							//FM_Printf(FM_ERROR,"\nn/w disc\n");

//							FM_Printf(FM_USER,"dsle\n");
							
							if(gDiscStallCounter >= 5)
							{
								#ifndef RELEASE
								FM_Printf(FM_USER,"dsl rst\n");
								#endif
								GV701x_Chip_Reset();
							
								//FM_Printf(FM_USER,"HW Reset DISC Stall\n");
							}
							else
							{
								#ifndef RELEASE
								FM_Printf(FM_USER,"dsl\n");
								#endif
								gDiscStallCounter++;
								STM_StartTimer(snsm->discStallTimer,DISC_STALL_TIME);
								//FM_Printf(FM_USER,"HW DISC Stall\n");
							}
							
						}
						else
						{
							STM_StopTimer(snsm->discStallTimer);
							gDiscStallCounter = 0;
						}
						break;
					}				
					case EVENT_TYPE_TIMER_BCN_STALL_IND:
					{
						plc_reset_tx();
						plc_reset_rx();
						#ifndef RELEASE
						FM_Printf(FM_USER,"\nBSR\n");
						#endif
						gBcnStall++;
						if(gBcnStallCounter >= 20)
						{
							GV701x_Chip_Reset();
							//FM_Printf(FM_USER,"HW Reset BCN Stall\n");
						}
						else
						{
							gBcnStallCounter++;
							STM_StartTimer(snsm->bcnStallTimer,BCN_STALL_TIME);
							//FM_Printf(FM_USER,"HW BCN Stall\n");
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
            break;
        }
        case SNSM_STATE_CONN:
        {
            if(event->eventHdr.eventClass == EVENT_CLASS_MSG)
            {
                switch(event->eventHdr.type)
                {
                	 case EVENT_TYPE_CM_UNASSOC_STA_IND:
                    {   
                        FM_Printf(FM_MMSG, "SNSM:<<CM_UNASSOC_STA.IND\n");

						break;
                	 }
                    case EVENT_TYPE_CC_DISCOVER_LIST_REQ:
                    {
                        hpgpHdr = (sHpgpHdr *)event->buffDesc.buff;
#ifdef P8051
           FM_Printf(FM_MMSG, "SNSM:<<<CC_DISC_LIST.REQ(tei: %bu)\n",
                               hpgpHdr->tei);
#else
           FM_Printf(FM_MMSG, "SNSM:<<CC_DISC_LIST.REQ(tei: %d)\n",
                               hpgpHdr->tei);
#endif
                        staInfo->staScb->discUpdate = 0;
                        SNSM_SendMgmtMsg(snsm, EVENT_TYPE_CC_DISCOVER_LIST_CNF);
                        break;
                    }
#ifdef UKE					
                case EVENT_TYPE_CM_SC_JOIN_REQ:                    
                {
                    if(staInfo->secMode == SEC_MODE_SC_ADD)
                    {
                    
                        FM_Printf(FM_MINFO, "SNSM:<<CM_SC_JOIN.REQ\n");

                        if ((!snam->ukePeerNotification) ||
                            (!memcmp(&((sHpgpHdr*)(event->buffDesc.buff))->macAddr,
                                       snam->ukePeer, MAC_ADDR_LEN)))
                        {
                           LINKL_SendMgmtMsg(snsm->staInfo, EVENT_TYPE_CM_SC_JOIN_CNF, 
                                        ((sHpgpHdr*)(event->buffDesc.buff))->macAddr);
                          
                           SNAM_EnableAssocNtf(snam, 
                                               ((sHpgpHdr*)(event->buffDesc.buff))->macAddr);
                           

                       }
                   }
                   break;
                } 
#endif
                    
#ifdef ROUTE
                    case EVENT_TYPE_CM_ROUTE_INFO_REQ:
                    {
                        ROUTE_sendRouteInfo(EVENT_TYPE_CM_ROUTE_INFO_CNF, event);
                        break;
                    }
                    case EVENT_TYPE_CM_ROUTE_INFO_CNF:
                    {
                        hpgpHdr = (sHpgpHdr *)event->buffDesc.buff;
                        ROUTE_procRouteInfo((sRouteInfo *)&event->buffDesc.dataptr[1], event->buffDesc.dataptr[0], hpgpHdr->tei);
                        break;
                    }
                    case EVENT_TYPE_CM_ROUTE_INFO_IND:
                    {
                        hpgpHdr = (sHpgpHdr *)event->buffDesc.buff;
                        ROUTE_procRouteInfo((sRouteInfo *)&event->buffDesc.dataptr[1], event->buffDesc.dataptr[0], hpgpHdr->tei);
                        break;
                    }
                    case EVENT_TYPE_CM_UNREACHABLE_IND:
                    {
                        u32 *unreachableNtb = event->buffDesc.dataptr;
                        hpgpHdr = (sHpgpHdr *)event->buffDesc.buff;
                        ROUTE_procUnreachableInd(&event->buffDesc.dataptr[5], 
                            event->buffDesc.dataptr[4], hpgpHdr->tei, (u32)*event->buffDesc.dataptr);
                        break;
                    }
#endif // ROUTE
                    default:
                    {
                    }
                }
            }
            else //control event
            {
                switch(event->eventHdr.type)
                {

					case EVENT_TYPE_BCN_MISS_IND:
					{

						FM_Printf(FM_HINFO, "bcn miss\n");
//						SNSM_HandleBcnLoss(snsm, event);
						break;
					}
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
#ifdef HOM

                        switch(snsm->hoSwitch)
                        {
                            case HPGP_HO_SWITCH_CCO:
                            {
                                FM_Printf(FM_HINFO, "SNSM:become CCo\n");
                            
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
                                STM_StopTimer(snsm->bcnLossTimer);
								
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
                                    FM_Printf(FM_ERROR, "SNSM:Can't alloc scb CCo\n");
                                    break;
                                }
                                
                                scb->staCap.fields.ccoStatus = 1;
                                staInfo->ccoScb = scb; 
#ifdef NSM_STA_PRINT								
#ifdef P8051
               FM_Printf(FM_HINFO, "SNSM:switch to the new CCo(tei: %bu)\n",
                                   scb->tei);
#else
               FM_Printf(FM_HINFO, "SNSM:switch to the new CCo(tei: %d)\n",
                                   scb->tei);
#endif
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
#endif
                        if(snsm->ccoDetected)
                        {
                            //send the event to the SNAM to renew the TEI
                            newEvent = EVENT_Alloc(EVENT_DEFAULT_SIZE, 
                                                   EVENT_HPGP_CTRL_HEADROOM);
                            if(newEvent == NULL)
                            {
                                FM_Printf(FM_ERROR, "EAF\n");
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

					case EVENT_TYPE_TIMER_BEACON_LOSS_IND:
					{

						  //NOTE: noBcn is reset 
                        //after the central beacon is received
                        snsm->noBcn++; 
#ifdef UM

						if (snsm->noBcn == MAX_NO_BEACON_NW_DISCOVERY)
						{

							
	 				    	HHAL_SetSWStatReqScanFlag(REG_FLAG_CLR);
							
						// if(staInfo->lastUserAppCCOState == 0)
							{

							//printf("\n MAX_NO_BEACON\n");
							//LINKL_SendBcnLossInd(MAX_NO_BEACON_NW_DISCOVERY);
							//								FM_Printf(FM_ERROR,"n/w discloss");
							snsm->noBcn= 0;
#ifdef LOG_FLASH
							logEvent(ISM_ERROR, BCN_LOSS, EVENT_TYPE_BCN_MISS_IND, NULL, 0);
#endif
#if 0
							memset((u8*)&snsm->avlnInfo, 0x00, 
							       sizeof(sAvlnInfo) * AVLN_LIST_MAX);
#endif

							staInfo->numDiscNet = 0;
							snsm->activeAvln = 0;
							SNSM_HandleBcnLoss(snsm, MAX_NO_BEACON_NW_DISCOVERY);

							}
						//Host_SendIndication(HOST_EVENT_BCN_LOSS, HPGP_MAC_ID, NULL, 0);
						}
						else if(snsm->noBcn  == MAX_NO_BEACON_BACKUPCCO)
						{

							//if(staInfo->lastUserAppCCOState == 0)
							{
							//LINKL_SendBcnLossInd(MAX_NO_BEACON_BACKUPCCO);
								SNSM_HandleBcnLoss(snsm, MAX_NO_BEACON_BACKUPCCO);

							}
							//Host_SendIndication(HOST_EVENT_PRE_BCN_LOSS, HPGP_MAC_ID, NULL, 0);
						}

#endif					


                        STM_StartTimer(snsm->bcnLossTimer, HPGP_TIME_BEACON_LOSS);

						}

						break;
						
                    case EVENT_TYPE_TIMER_DISC_AGING_IND:
                    {
                        //SNSM_PerformAging(snsm);
                        scb = snsm->staInfo->staScb;
#ifdef DISC_BCN						
                        SCB_AgeDiscLists(scb);
#endif
                      
                        //restart the aging timer
                        STM_StopTimer(snsm->discAgingTimer);
                        STM_StartTimer(snsm->discAgingTimer, HPGP_TIME_DISC_AGING);   
                        break;
                    }
                    case EVENT_TYPE_SNSM_STOP:
                    {
                        //this happens when the STA leaves the network
                        STM_StopTimer(snsm->usttTimer);   
                        STM_StopTimer(snsm->discAgingTimer);   

                        STM_StopTimer(snsm->bcnLossTimer);

                        snsm->enableCcoSelection = 0;
                        snsm->enableCcoDetection = 0;
						snsm->netSync = FALSE;
						snsm->netScan = FALSE;
                        snsm->state = SNSM_STATE_INIT;
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
#endif // ROUTE
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

	snsm->bcnLossTimer = STM_AllocTimer(HP_LAYER_TYPE_LINK, 
                   		      EVENT_TYPE_TIMER_BEACON_LOSS_IND, linkl);
#ifdef SW_RECOVERY
	snsm->discStallTimer = STM_AllocTimer(HP_LAYER_TYPE_LINK,
								EVENT_TYPE_TIMER_DISC_STALL_IND, linkl);
	snsm->bcnStallTimer  = STM_AllocTimer(HP_LAYER_TYPE_LINK ,
								EVENT_TYPE_TIMER_BCN_STALL_IND,linkl);
#endif	

	snsm->scanToAssocTimer = STM_AllocTimer(HP_LAYER_TYPE_LINK ,
							EVENT_TYPE_TIMER_JOIN_TIMEOUT, linkl);

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
#ifdef NSM_STA_PRINT	
#ifdef P8051
FM_Printf(FM_ERROR, "SNSM:bbt timer id:%bu\n", snsm->bbtTimer);
#else
FM_Printf(FM_ERROR, "SNSM:bbt timer id:%d\n", snsm->bbtTimer);
#endif
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
#ifdef NSM_STA_PRINT	
#ifdef P8051
FM_Printf(FM_ERROR, "SNSM:ustt timer id: %bu\n", snsm->usttTimer);
#else
FM_Printf(FM_ERROR, "SNSM:ustt timer id: %d\n", snsm->usttTimer);
#endif
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
#ifdef NSM_STA_PRINT	
#ifdef P8051
FM_Printf(FM_ERROR, "SNSM:disc aging timer id: %bu\n", snsm->discAgingTimer);
#else
FM_Printf(FM_ERROR, "SNSM:disc aging timer id: %d\n", snsm->discAgingTimer);
#endif
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
        FM_Printf(FM_ERROR, "SNSM:EAF\n");
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
//        SLIST_Put(&snsm->linkl->intEventQueue, &newEvent->link);

//    }
    return STATUS_SUCCESS;
}

eStatus SNSM_Stop(sSnsm *snsm)
{
    sEvent *newEvent = NULL;
//    sLinkLayer *linkl = (sLinkLayer *)HPGPCTRL_GetLayer(HP_LAYER_TYPE_LINK);
    sLinkLayer *linkl = snsm->linkl;
#if 0
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
#else
    //this happens when the STA leaves the network
         STM_StopTimer(snsm->usttTimer);   
         STM_StopTimer(snsm->discAgingTimer);  

		 
		  STM_StopTimer(snsm->scanToAssocTimer);
									
		 
         STM_StopTimer(snsm->bbtTimer); 
		 STM_StopTimer(snsm->bcnLossTimer);
#ifdef SW_RECOVERY		 
    	 STM_StopTimer(snsm->discStallTimer);
		 STM_StopTimer(snsm->bcnStallTimer);
		 gBcnStallCounter = 0;
		 gDiscStallCounter = 0;
#endif
         snsm->enableCcoSelection = 0;
         snsm->enableCcoDetection = 0;
		 snsm->enableBcnLossDetection = 0;
         snsm->netSync = FALSE;
         snsm->netScan = FALSE;
		 snsm->enableBackupCcoDetection = 0;
		 snsm->activeAvln = 0;

		 gHpgpHalCB.syncComplete   = 0;
		 gHpgpHalCB.nwSelected	   = 0;

		 
		 
#if 0 //def FREQ_DETECT								
			 gHpgpHalCB.gFreqCB.freqDetected = 0;
#endif

         
		 gHpgpHalCB.nwSelectedSnid = 0;

         snsm->state = SNSM_STATE_INIT;
    
#endif

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
 * $Source: /home/cvsrepo/Hybrii_B_OSFW_Dev/firmware/hpgp/src/link/nsm_sta.c,v $
 *
 * $Log: nsm_sta.c,v $
 * Revision 1.50  2015/03/16 11:32:25  ranjan
 * Fixes for multiCCo modes switching
 * Beacon Rx Reception issue fix
 * Verified : Overnight multicco setup and 15 minutes SPI TCP test.
 *
 * Revision 1.49  2015/02/03 13:11:34  prashant
 * Fix CCO Fix STA changes
 *
 * Revision 1.48  2015/01/19 22:47:53  tri
 * Took out LG_WAR
 *
 * Revision 1.47  2015/01/13 10:31:27  prashant
 * LLP firmware changes
 *
 * Revision 1.46  2015/01/13 02:01:57  tri
 * removed psSetFlag
 *
 * Revision 1.45  2015/01/12 08:22:13  prashant
 * Power save Changes
 *
 * Revision 1.44  2015/01/06 02:51:19  tri
 * Fixed PS resync problem
 *
 * Revision 1.43  2015/01/02 14:55:36  kiran
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
 * Revision 1.42  2014/12/09 07:09:09  ranjan
 * - multicco feature under MCCO flag
 *
 * Revision 1.41  2014/12/08 07:57:12  prashant
 * print removed
 *
 * Revision 1.40  2014/12/04 09:51:16  prashant
 * Rssi and Lqi added in beacon processing
 *
 * Revision 1.39  2014/11/11 14:52:58  ranjan
 * 1.New Folder Architecture espically in /components
 * 2.Modular arrangment of functionality in new files
 *    anticipating the need for exposing them as FW App
 *    development modules
 * 3.Other improvisation in code and .h files
 *
 * Revision 1.38  2014/10/28 16:27:43  kiran
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
 * Revision 1.37  2014/10/15 10:42:51  ranjan
 * small fixes in um
 *
 * Revision 1.36  2014/10/13 10:23:57  prashant
 * LG-Uart corruption issue fix
 *
 * Revision 1.35  2014/09/30 21:45:01  tri
 * Added LLP PS
 *
 * Revision 1.34  2014/09/05 09:28:18  ranjan
 * 1. uppermac cco-sta switching feature fix
 * 2. general stability fixes for many station associtions
 * 3. changed mgmt memory pool for many STA support
 *
 * Revision 1.33  2014/08/25 07:37:34  kiran
 * 1) RSSI & LQI support
 * 2) Fixed Sync related issues
 * 3) Fixed timer 0 timing drift for SDK
 * 4) MMSG & Error Logging in Flash
 *
 * Revision 1.32  2014/08/12 08:45:43  kiran
 * 1) Event fixes
 * 2) API to change UART line control parameters
 *
 * Revision 1.31  2014/07/30 12:26:26  kiran
 * 1) Software Recovery for CCo
 * 2) User appointed CCo support in SDK
 * 3) Association process performance fixes
 * 4) SSN related fixes
 *
 * Revision 1.30  2014/07/22 10:03:52  kiran
 * 1) SDK Supports Power Save
 * 2) Uart_Driver.c cleanup
 * 3) SDK app memory pool optimization
 * 4) Prints from STM.c are commented
 * 5) Print messages are trimmed as common no memory left in common
 * 6) Prins related to Power save and some other modules are in compilation flags. To enable module specific prints please refer respective module
 *
 * Revision 1.29  2014/07/16 10:47:40  kiran
 * 1) Updated SDK
 * 2) Fixed Diag test in SDK
 * 3) Ethernet and SPI interfaces removed from SDK as common memory is less
 * 4) GPIO access API's added in SDK
 * 5) GV701x chip reset command supported
 * 6) Start network and Join network supported in SDK (Forced CCo and STA)
 * 7) Some bug fixed in SDK (CP free, p app command issue etc.)
 *
 * Revision 1.28  2014/07/10 11:42:45  prashant
 * power save commands added
 *
 * Revision 1.27  2014/07/05 09:16:27  prashant
 * 100 Devices support- only association tested, memory adjustments
 *
 * Revision 1.26  2014/07/04 03:54:14  tri
 * Fixed bug in STA
 *
 * Revision 1.25  2014/06/24 16:26:45  ranjan
 * -zigbee frame_handledata fix.
 * -added reason code for uppermac host events
 * -small cleanups
 *
 * Revision 1.24  2014/06/23 06:56:44  prashant
 * Ssn reset fix upon device reset, Duplicate SNID fix
 *
 * Revision 1.23  2014/06/19 17:13:19  ranjan
 * -uppermac fixes for lvnet and reset command for cco and sta mode
 * -backup cco working
 *
 * Revision 1.22  2014/06/19 07:16:02  prashant
 * Region fix, frequency setting fix
 *
 * Revision 1.21  2014/06/12 13:15:44  ranjan
 * -separated bcn,mgmt,um event pools
 * -fixed datapath issue due to previous checkin
 * -work in progress. neighbour cco detection
 *
 * Revision 1.20  2014/06/11 13:17:47  kiran
 * UART as host interface and peripheral interface supported.
 *
 * Revision 1.19  2014/06/05 10:26:07  prashant
 * Host Interface selection isue fix, Ac sync issue fix
 *
 * Revision 1.18  2014/06/05 08:38:41  ranjan
 * -flash function enabled for uppermac
 * - commit command after any change would flash systemprofiles
 * - verfied upper mac
 *
 * Revision 1.17  2014/05/28 10:58:59  prashant
 * SDK folder structure changes, Uart changes, removed htm (UI) task
 * Varified - UM, LM - iperf (UDP/TCP), overnight test pass
 *
 * Revision 1.16  2014/05/21 23:02:31  tri
 * more PS
 *
 * Revision 1.15  2014/05/20 05:57:45  prashant
 * persistent schedule code updated
 *
 * Revision 1.14  2014/05/12 08:09:57  prashant
 * Route fix, STA sync issue with AV CCO and handling discover bcn issue fix
 *
 * Revision 1.13  2014/04/21 03:12:04  tri
 * more PS
 *
 * Revision 1.12  2014/04/20 19:51:28  tri
 * more PS
 *
 * Revision 1.11  2014/04/11 12:23:55  prashant
 * Under PLC_TEST macro Diagnostic Mode code added
 *
 * Revision 1.10  2014/04/09 21:10:20  tri
 * more PS
 *
 * Revision 1.9  2014/04/09 08:18:10  ranjan
 * 1. Added host events for homeplug uppermac indication (Host_SendIndication)
 * 2. timer workaround  + other fixes
 *
 * Revision 1.8  2014/03/27 23:52:06  tri
 * more PS
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
 * Revision 1.4  2014/02/26 23:15:14  tri
 * more PS code
 *
 * Revision 1.3  2014/01/28 17:47:21  tri
 * Added Power Save code
 *
 * Revision 1.2  2014/01/10 17:17:53  yiming
 * check in Rajan 1/8/2014 code release
 *
 * Revision 1.6  2014/01/08 10:53:54  ranjan
 * Changes for LM OS support.
 * New Datapath FrameTask
 * LM and UM  datapath, feature verified.
 *
 * known issues : performance numbers needs revisit
 *
 * review : pending.
 *
 * Revision 1.5  2013/10/16 07:43:38  prashant
 * Hybrii B Upper Mac compiling issues and QCA fix, added default eks code
 *
 * Revision 1.4  2013/09/04 14:51:01  yiming
 * New changes for Hybrii_A code merge
 *
 * Revision 1.9  2013/07/12 08:56:37  ranjan
 * -UKE Push Button Security Feature.
 * Verified : DirectEntry Security Works.Datapath Works.
 *                 command SetSecMode for UKE works.
 * Added against bug-160
 *
 * Revision 1.8  2013/05/23 10:09:30  prashant
 * Version command added, SPI polling waittime increased, sys_common file added
 *
 * Revision 1.7  2013/03/22 12:21:49  prashant
 * default FM_MASK and FM_Printf modified for USER INFO
 *
 * Revision 1.6  2013/03/21 07:43:26  ranjan
 * Starting NDC on "p reset" command
 *
 * Revision 1.5  2013/03/14 11:49:18  ranjan
 * 1.handled cases  for CCo toSTA switch and  viceversa
 * 2.UM uses bcntemplate
 *
 * Revision 1.4  2013/02/15 12:53:57  prashant
 * ASSOC.REQ changes for DEVELO
 *
 * Revision 1.3  2012/12/14 11:06:58  ranjan
 * queue added for eth to plc datapath
 * removed mgmt tx polling
 *
 * Revision 1.2  2012/11/19 07:46:24  ranjan
 * Changes for Network discovery modes
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

