/** =========================================================
 *
 *  @file crm.c
 * 
 *  @brief Central Resource Manager
 *
 *  Copyrighut (C) 2010-2011, Greenvity Communications, Inc.
 *  All Rights Reserved
 *  
 * ===========================================================*/

#include "fm.h"
#ifdef ROUTE
#include "hpgp_route.h"
#endif
#include "crm.h"
#include "hpgpapi.h"
#include "linkl.h"
#include "timer.h"
#include "stm.h"
#include <string.h>
#include "hybrii_tasks.h"
#include "sys_common.h"

#ifdef LOG_FLASH
u16 scbFreeReason = 0;
#endif

void CRM_Init(sCrm *crm)
{
    u8 i;

    //reset sta buckets in hash table
    for(i = 0; i < CRM_SCB_HASH_TABLE_SIZE; i++)
    {
        SLIST_Init(&crm->scbBucket[i]);
        crm->scbBucketSize[i] = 0;
    }

//    crm->scbBucketSize[0] = 0;
//    crm->scbBucketSize[CRM_SCB_HASH_TABLE_SIZE-1] = 0;

//    crm->discBucket = 0;
    //reset free queue 
    SLIST_Init(&crm->freeQueue);
    for(i = 0; i < CRM_SCB_MAX; i++)
    {
        memset(&crm->scb[i], 0, sizeof(sScb));
        crm->scb[i].staTimer = STM_TIMER_ID_NULL;
        crm->scb[i].teiTimer = STM_TIMER_ID_NULL;
        SLIST_Put(&crm->freeQueue, &crm->scb[i].link);
    }    
}


/*
sStaCb *SRM_GetSta(sCrm *crm, u16 tei)
{
   
}
*/


sScb *CRM_AllocScb(sCrm *crm)
{
    u8 i, index; 
    u16 size;
    u8 mintei;
    u8 maxtei;
    sScb *scb = NULL;
    sSlink *scblink;
    sScb *currscb = NULL;
    sSlink *currlink = NULL;
    sScb *nextscb = NULL;
    sSlink *nextlink = NULL;
    sSlist *bucketlist = NULL;

    //check free SCB
    if(SLIST_IsEmpty(&crm->freeQueue))
        return NULL;

//FM_Printf(FM_ERROR, "CRM: AllocScb \n"); 

    
    //search for the smallest bucket
    size = crm->scbBucketSize[0];
    index = 0;
    for(i = 0; i < CRM_SCB_HASH_TABLE_SIZE; i++)
    {
        if(crm->scbBucketSize[i] < size)
        {
           size = crm->scbBucketSize[i];
           index = i;
        }
    }

    if(size >= CRM_SCB_BUCKET_MAX)
    {
        //The table is full
        FM_Printf(FM_LINFO, "STA hash table full\n");
        return NULL;
    }

    scblink = SLIST_Pop(&crm->freeQueue);
    scb = SLIST_GetEntry(scblink, sScb, link);
//    memset(scb, 0, sizeof(sScb)); 

    if(index == 0)
    {
        mintei = 1;
    }
    else
    {
        mintei = index<<4;
    }

    if (index == 15)
    {
        maxtei = (index<<4)|0xE;
    }
    else
    {
        maxtei = (index<<4)|0xF;
    }

 
    bucketlist = &crm->scbBucket[index];

    if (SLIST_IsEmpty(bucketlist))
    {
        scb->tei = mintei;
        SLIST_Push(bucketlist, &scb->link);
        crm->scbBucketSize[index]++;
        return scb;
    }

    //at this point, bucket list is not empty
    currlink = SLIST_PeekHead(bucketlist);
    currscb = SLIST_GetEntry(currlink, sScb, link);
    if(currscb->tei > mintei)
    {
        scb->tei = mintei;
        SLIST_Push(bucketlist, &scb->link);
        crm->scbBucketSize[index]++;
        return scb;
    }

    //at this point, the first entry has the minimum tei in the bucket
    nextlink = SLIST_Next(currlink);
    while(currlink && nextlink) 
    {
        nextscb = SLIST_GetEntry(nextlink, sScb, link);
        if (currscb->tei + 1 < nextscb->tei)
        {
            scb->tei = currscb->tei+1;
            //add after the currscb
            SLIST_Add(bucketlist, &currscb->link, &scb->link);
            crm->scbBucketSize[index]++;
            return scb;
        }

        currlink = nextlink;
        currscb = nextscb;
        nextlink = SLIST_Next(nextlink);
    }
 
    // at this point, nextlink = NULL
    if(currscb->tei < maxtei)
    {
        scb->tei = currscb->tei+1;
        SLIST_Add(bucketlist, &currscb->link, &scb->link);
        //or SLIST_Put(bucketlist, &scb->link);
        crm->scbBucketSize[index]++;
        return scb;
    }
    else
    {
        //it should not come to this point
        FM_Printf(FM_ERROR, "CRM:TEI alloc err\n"); 
        return NULL;
    }
}



sScb *CRM_AddScb(sCrm *crm, u8 tei)
{
    sScb   *scb = NULL;
    sSlink *scblink = NULL;
    sScb   *currscb = NULL;
    sSlink *currlink;
    sScb   *nextscb = NULL;
    sSlink *nextlink;
    sSlist *bucketlist = NULL;
    u8      index = tei>>4;
    bucketlist = &crm->scbBucket[index];

    if (SLIST_IsEmpty(bucketlist))
    {
        scblink = SLIST_Pop(&crm->freeQueue);
        if(scblink == NULL)
        {
            return NULL;  //no resouce avaiable
        }
        scb = SLIST_GetEntry(scblink, sScb, link);
//        memset(scb, 0, sizeof(sScb)); 
        scb->tei = tei;
        SLIST_Push(bucketlist, &scb->link);
        crm->scbBucketSize[index]++;
        return scb;
    }

    //at this point, bucket list is not empty
    currlink = SLIST_PeekHead(bucketlist);
    currscb = SLIST_GetEntry(currlink, sScb, link);
    if(currscb->tei == tei)
    {
        return currscb;
    }

    if(currscb->tei > tei)
    {
        scblink = SLIST_Pop(&crm->freeQueue);
        if(scblink == NULL)
        {
            return NULL;  //no resouce avaiable
        }
        scb = SLIST_GetEntry(scblink, sScb, link);
//        memset(scb, 0, sizeof(sScb)); 
        scb->tei = tei;
        SLIST_Push(bucketlist, &scb->link);
        crm->scbBucketSize[index]++;
        return scb;
    }

 
    //now the currscb->tei < tei
    nextlink = SLIST_Next(currlink);
    while(currlink && nextlink) 
    {
        
        nextscb = SLIST_GetEntry(nextlink, sScb, link);
        if (nextscb->tei > tei)
        {
            break;
        }
        currlink = nextlink;
        currscb = nextscb;
        nextlink = SLIST_Next(nextlink);
    }

    //at this point, either nextscb->tei > tei or nextlink = NULL
    //in either case, currscb->scb <= tei

    if(currscb->tei == tei)
    {
        return currscb;
    }

    scblink = SLIST_Pop(&crm->freeQueue);
    if(scblink == NULL)
    {
        return NULL;  //no resouce avaiable
    }
    scb = SLIST_GetEntry(scblink, sScb, link);
//    memset(scb, 0, sizeof(sScb)); 
    scb->tei = tei;
    SLIST_Add(bucketlist, &currscb->link, &scb->link);
    return scb;
}

/* TEI reuse timer
void LINKL_TeiReuseTimerHandler(void* cookie)
{
    sEvent *event = NULL;
    sLinkLayer *linkLayer = (sLinkLayer *)HOMEPLUG_GetLayer(HP_LAYER_TYPE_LINK);

    //Generate a time event
    event = EVENT_Alloc(EVENT_DEFAULT_SIZE, 0);
    if(event == NULL)
    {
        FM_Printf(FM_ERROR, "CRM: Cannot allocate an event.\n");
        return;
    }
    
    event->eventHdr.type = EVENT_TYPE_TIMER_TEI_REUSE_IND;
    event->eventHdr.scb = cookie;
    //post the event to the event queue
    SLIST_Put(&linkLayer->eventQueue, &event->link);
}
*/
    



void CRM_FreeScb(sCrm *crm, sScb *scb)
{
    sScb    *currscb = NULL;
    sSlink  *currlink = NULL;
    sScb    *nextscb = NULL;
    sSlink  *nextlink;
    sSlist  *bucketlist = NULL;
    u8      index;

//    STM_FreeTimer(scb->teiReuseTimer); 
    scb->namState = STA_NAM_STATE_INIT;
    scb->homState = STA_HOM_STATE_INIT;
    scb->akmState = STA_AKM_STATE_INIT;
    //the timers will be lost if they are not freed before free the scb  
//    scb->staTimer = STM_TIMER_ID_NULL;
//    scb->teiTimer = STM_TIMER_ID_NULL;

    index = (scb->tei)>>4;
    bucketlist = &crm->scbBucket[index];

    if (SLIST_IsEmpty(bucketlist))
    {
        FM_Printf(FM_ERROR, "CRM:scb already free\n"); 
        return;
    }
#ifdef LOG_FLASH
    logEvent(SCB_UPDATE,SCB_FREE,scbFreeReason,&scb->tei,1);
#endif
    currlink = SLIST_PeekHead(bucketlist);
    currscb = SLIST_GetEntry(currlink, sScb, link);
    if (currscb->tei == scb->tei)
    {
        currlink = SLIST_Pop(bucketlist);
//currscb = SLIST_GetEntry(currlink, sScb, link);
//FM_Printf(FM_ERROR, "CRM: free scb (%d)!\n", currscb->tei); 

        //reset the scb before placing it to the free queue
        memset(currscb, 0, sizeof(sScb));
        currscb->staTimer = STM_TIMER_INVALID_ID;
        currscb->teiTimer = STM_TIMER_INVALID_ID;

        SLIST_Put(&crm->freeQueue, currlink);
        crm->scbBucketSize[index]--;
        return;
    }


    nextlink = SLIST_Next(currlink);
    while(currlink && nextlink) 
    {
        nextscb = SLIST_GetEntry(nextlink, sScb, link);
        if (scb->tei == nextscb->tei)
        {
            SLIST_Remove(bucketlist, currlink, nextlink);

            //reset the scb before placing it to the free queue
            memset(nextscb, 0, sizeof(sScb));
            nextscb->staTimer = STM_TIMER_INVALID_ID;
            nextscb->teiTimer = STM_TIMER_INVALID_ID;

            SLIST_Put(&crm->freeQueue, nextlink);
            crm->scbBucketSize[index]--;
            return; //done
        }
        currlink = nextlink;
        currscb = nextscb;
        nextlink = SLIST_Next(nextlink);
    }

    // at this point, no entry for tei is found 
    //it should not come to this point
        FM_Printf(FM_ERROR, "CRM:scb already free\n"); 
    return;
}

sScb *CRM_FindScbMacAddr(u8 *macAddr)
{
	sScb* dstScb = NULL;
    sLinkLayer    *linkl = (sLinkLayer *)HPGPCTRL_GetLayer(HP_LAYER_TYPE_LINK);
    sCrm          *crm = LINKL_GetCrm(linkl);
    dstScb = CRM_GetNextScb(crm, dstScb);
    while(dstScb)
    {
		if(memcmp(macAddr, &dstScb->macAddr, MAC_ADDR_LEN) == 0)
		{
		 	break;
		}
        dstScb = CRM_GetNextScb(crm, dstScb);
    }
   
    return dstScb;
}

sScb *CRM_GetScb(sCrm *crm, u8 tei)
{
    sScb    *scb = NULL;
    sSlist  *bucketlist = NULL;
    u8      index = tei>>4;

    bucketlist = &crm->scbBucket[index];

    //SLIST_For_Each_Entry(scb, bucketlist, link)
    SLIST_For_Each_Entry(sScb, scb, bucketlist, link)
    {
        if (scb->tei == tei)
        {
            return scb;
        }
    }

    return NULL;
}
    

u8 CRM_GetScbNum(sCrm *crm)
{
    u8 i;
    u8 numScb = 0;

    for(i = 0; i< CRM_SCB_HASH_TABLE_SIZE; i++)
    {
        numScb += crm->scbBucketSize[i];
    }

    return numScb;
}


sScb *CRM_GetNextScb(sCrm *crm, sScb* scb)
{
    sSlink  *currlink = NULL;
    sSlink  *nextlink = NULL;
    sSlist  *bucketlist = NULL;
    u8       bkt;

    if(scb == NULL)
    {
        //start from the first entry in the first bucket
        bkt = 0;
    }
    else
    {
        nextlink = SLIST_Next(&scb->link);
        if(nextlink)
        {
            return SLIST_GetEntry(nextlink, sScb, link);
        }
        else
        {
            //at this point, the scb is the last entry in its bucket 
            //thus go to the next bucket
            bkt = ((scb->tei)>>4) + 1;
            FM_Printf(FM_LINFO, "buckets(%d)\n", bkt); 
            if(bkt == CRM_SCB_HASH_TABLE_SIZE)
            {
                //reach the end of hash table
                return NULL;
            } 
        }
    }

//FM_Printf(FM_ERROR, "CRM hash buckets(%d)!\n", bkt); 
    //starting with the bucket at bkt, search for the non-empty bucket
//    while(!crm->scbBucketSize[bkt]&&(bkt<CRM_SCB_HASH_TABLE_SIZE))
    while((bkt < CRM_SCB_HASH_TABLE_SIZE) && SLIST_IsEmpty(&crm->scbBucket[bkt]))
    {
//        FM_Printf(FM_ERROR, "CRM: bucket(%d) is empty!\n", bkt); 
        bkt++;
    }

    if(bkt == CRM_SCB_HASH_TABLE_SIZE)
    {
        //reach the end of hash table
        return NULL;
    }
//FM_Printf(FM_ERROR, "CRM: nonempty hash buckets(%d)!\n", bkt); 

    bucketlist = &crm->scbBucket[bkt];
    currlink = SLIST_PeekHead(bucketlist);
    return SLIST_GetEntry(currlink, sScb, link);
}


#ifdef TEST

void CRM_RemoveBucket(sCrm *crm, u8 bkt)
{
    sSlink  *currlink = NULL;
    sScb    *currscb = NULL;
    sSlist  *bucketlist = NULL;

    bucketlist = &crm->scbBucket[bkt];
#ifdef CRM_PRINT	
    FM_Printf(FM_ERROR, "CRM:remove bucket %d\n", bkt); 
#endif
    while(!SLIST_IsEmpty(bucketlist))
    { 
       currlink = SLIST_Pop(bucketlist);
       currscb = SLIST_GetEntry(currlink, sScb, link);
#ifdef CRM_PRINT	   
FM_Printf(FM_ERROR, "CRM:remove scb %d in bucket\n", currscb->tei,  bkt); 
#endif
       SLIST_Put(&crm->freeQueue, currlink);
       crm->scbBucketSize[bkt]--;
    }

}

#endif

/** =========================================================
 *
 * Edit History
 *
 * $Source: /home/cvsrepo/Hybrii_B_OSFW_Dev/firmware/hpgp/src/link/crm.c,v $
 *
 * $Log: crm.c,v $
 * Revision 1.7  2014/08/25 07:37:34  kiran
 * 1) RSSI & LQI support
 * 2) Fixed Sync related issues
 * 3) Fixed timer 0 timing drift for SDK
 * 4) MMSG & Error Logging in Flash
 *
 * Revision 1.6  2014/07/22 10:03:52  kiran
 * 1) SDK Supports Power Save
 * 2) Uart_Driver.c cleanup
 * 3) SDK app memory pool optimization
 * 4) Prints from STM.c are commented
 * 5) Print messages are trimmed as common no memory left in common
 * 6) Prins related to Power save and some other modules are in compilation flags. To enable module specific prints please refer respective module
 *
 * Revision 1.5  2014/06/11 13:17:47  kiran
 * UART as host interface and peripheral interface supported.
 *
 * Revision 1.4  2014/05/28 10:58:59  prashant
 * SDK folder structure changes, Uart changes, removed htm (UI) task
 * Varified - UM, LM - iperf (UDP/TCP), overnight test pass
 *
 * Revision 1.3  2014/05/12 08:09:57  prashant
 * Route fix, STA sync issue with AV CCO and handling discover bcn issue fix
 *
 * Revision 1.2  2014/02/27 10:42:47  prashant
 * Routing code added
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
 * Revision 1.5  2012/10/25 11:38:48  prashant
 * Sniffer code added for MAC_SAP, Added new commands in MAC_SAP for sniffer, bridge,
 *  hardware settings and peer information.
 *
 * Revision 1.4  2012/10/11 06:21:00  ranjan
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
 * Revision 1.3  2012/07/10 04:16:37  yuanhua
 * fixed a potential array overflow in CRM_GetNextScb().
 *
 * Revision 1.2  2011/09/09 07:02:31  yuanhua
 * migrate the firmware code from the greenchip to the hybrii.
 *
 * Revision 1.7  2011/08/02 16:06:00  yuanhua
 * (1) Fixed a bug in STM (2) Made STA discovery work according to the standard, including aging timer. (3) release the resource after the STA leave (4) STA will switch to the backup CCo if the CCo failure occurs (5) At this point, the CCo could work with multiple STAs correctly, including CCo association/leave, TEI renew, TEI map updating, discovery beacon scheduling, discovery STA list updating ang aging, CCo failure, etc.
 *
 * Revision 1.6  2011/07/22 18:51:04  yuanhua
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
 * Revision 1.1  2011/05/06 19:10:12  kripa
 * Adding link layer files to new source tree.
 *
 * Revision 1.2  2011/04/24 03:38:29  kripa
 * Passing 'struct type' as argument to SLIST_For_Each_Entry() macro.
 *
 * Revision 1.1  2011/04/08 21:42:45  yuanhua
 * Framework
 *
 *
 * =========================================================*/

