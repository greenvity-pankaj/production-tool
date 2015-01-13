/** ==========================================================
 *
 * @file dmm.c
 * 
 *  @brief Dynamic Memory Management
 *
 *  Copyright (C) 2012, Greenvity Communications, Inc.
 *  All Rights Reserved
 *  
 * ============================================================ */


#include <string.h>
#include "fm.h"
#include "dmm.h"
#include "sys_common.h"


sDmm BcnDmm;
sDmm MgmtDmm;
sDmm FwDmm;
sDmm AppDmm;

#if 0
void DMM_DisplayFreeList(sDmm* pDmm)
{
    u8 i;
    sSlink    *slink = NULL;
    sSegDesc  *segdesc = NULL; 
    sSlab     *slab = NULL;
    FM_Printf(FM_WARN, "DMM: Display mem segment \n");
    for (i = 0; i < pDmm->slabnum; i++)
    {
#ifdef P8051
        FM_Printf(FM_WARN, "DMM: Slab %bu \n", i);
#else
        FM_Printf(FM_WARN, "DMM: Slab %d \n", i);
#endif
        slab = &(pDmm->slab[i]);  
        while(!SLIST_IsEmpty(&(pDmm->slab[i].freelist)))
        {
            slink = SLIST_Pop(&(pDmm->slab[i].freelist));
            segdesc = SLIST_GetEntry(slink, sSegDesc, link);  
#ifdef P8051
    FM_Printf(FM_WARN, "DMM: allocate mem seg %d, mem (%p, %p), data %p\n", 
segdesc->segsize, segdesc->mem, (u8 *)segdesc, (u8 *)segdesc + sizeof(sSegDesc));
#else
    FM_Printf(FM_WARN, "DMM: allocate mem seg %d, mem (0x%lx, 0x%lx), data 0x%lx\n", 
segdesc->segsize, segdesc->mem, segdesc, (u8 *)segdesc + sizeof(sSegDesc));
#endif
FM_HexDump(FM_DATA|FM_MINFO, "segment:", (u8 *)segdesc, segdesc->segsize + sizeof(sSegDesc) );
        }
    }
}
#endif

void DMM_eventMem()
{
	DMM_DisplayMemUsage(&FwDmm);

}

void DMM_MgmtMem()
{
	DMM_DisplayMemUsage(&MgmtDmm);

}
void DMM_BcnMem()
{
	DMM_DisplayMemUsage(&BcnDmm);

}

void DMM_DisplayMemUsage(sDmm* pDmm)
{
    u8 i;
    sSlab     *slab = NULL;
    FM_Printf(FM_WARN, "DMM: Display mem usage:\n");
    for (i = 0; i < pDmm->slabnum; i++)
    {
    	u8 boolval = SLIST_IsEmpty(&slab->freelist);
		
        slab = &(pDmm->slab[i]); 
#ifdef P8051
        FM_Printf(FM_WARN, "DMM: Slab %bu \n", i);
        FM_Printf(FM_WARN, "DMM: mem seg max %bu \n", slab->maxuse);
        FM_Printf(FM_WARN, "DMM: mem seg use %bu \n", slab->inuse);
        FM_Printf(FM_WARN, "DMM: isEmpty %bu \n",boolval );
#else 
        FM_Printf(FM_WARN, "DMM: Slab %d \n", i);
        FM_Printf(FM_WARN, "DMM: mem seg %d \n", slab->maxuse);
#endif
    }

}

eStatus DMM_Init(u8 pool_id)
{
    u8 i;
	sDmm* pDmm;
	if(pool_id == FW_POOL_ID)
		pDmm = &FwDmm;
	else if(pool_id == APP_POOL_ID)
		pDmm = &AppDmm;	
	else if(pool_id == BCN_POOL_ID)
		pDmm = &BcnDmm;	
	else if(pool_id == MGMT_POOL_ID)
		pDmm = &MgmtDmm;	
	
    memset(pDmm, 0, sizeof(sDmm));

    for (i = 0; i < DMM_SLAB_MAX; i++)
    {
        SLIST_Init(&(pDmm->slab[i].freelist));
    }
    return STATUS_SUCCESS;
}



eStatus DMM_InitMemPool(u8 pool_id, u8 *mempool, u16 poolsize, sSlabDesc *slabdesc, u8 slabnum)
{

    u8  i = 0;
    u16 memsize = 0;
    u8  segnum = 0;
    u8  *mem = mempool;
    sSegDesc  *segdesc = NULL; 
    sSlab     *slab = NULL;
	sDmm* pDmm;    

    if (slabnum > DMM_SLAB_MAX)
        return STATUS_FAILURE;
   
	if(pool_id == FW_POOL_ID)
		pDmm = &FwDmm;
	else if(pool_id == APP_POOL_ID)
		pDmm = &AppDmm;	
	else if(pool_id == BCN_POOL_ID)
			pDmm = &BcnDmm; 
	else if(pool_id == MGMT_POOL_ID)
			pDmm = &MgmtDmm;	

    memset(mempool, 0, poolsize);  

    for (i = 0; i < slabnum; i++)
    {
//        FM_Printf(FM_WARN, "DMM: Slab %bu \n", i);
        slab = &(pDmm->slab[i]);  
        pDmm->slab[i].segsize = slabdesc[i].segsize;
		pDmm->slab[i].inuse = 0;
        segnum = 0;
        while (((memsize + sizeof(sSegDesc) + slabdesc[i].segsize) <= poolsize)
               && (segnum < slabdesc[i].segnum))
        {
            segdesc = (sSegDesc *)mem;
            SLINK_Init(&segdesc->link);          
            segdesc->segsize = slabdesc[i].segsize;
            segdesc->mem = mem;
            SLIST_Put(&slab->freelist, &segdesc->link);
            mem += sizeof(sSegDesc) + segdesc->segsize;
            memsize += sizeof(sSegDesc) + segdesc->segsize;
            segnum++;
#if 0

    FM_Printf(FM_WARN, "DMM: init mem seg %d, mem (%p, %p), data %p\n", 
segdesc->segsize, segdesc->mem, (u8 *)segdesc, (u8 *)segdesc + sizeof(sSegDesc));
FM_HexDump(FM_DATA|FM_MINFO, "init segment:", (u8 *)segdesc, segdesc->segsize + sizeof(sSegDesc) );
#endif
        }
		if (segnum < slabdesc[i].segnum)
		{
			FM_Printf(FM_ERROR, "DMM: the memory pool is small.\n");
			return STATUS_FAILURE;
		}
			
    }
    pDmm->slabnum = slabnum;
    FM_Printf(FM_WARN, "DMM: the memory pool init is successful.\n");
//    DMM_DisplayFreeList();
    return STATUS_SUCCESS;
}



u8 *DMM_Alloc(u8 pool_id, u16 size)
{
    u8         i;
    sSlink    *slink = NULL;
    sSegDesc  *segdesc = NULL; 
	sDmm* pDmm;
	if(pool_id == FW_POOL_ID)
		pDmm = &FwDmm;
	else if(pool_id == APP_POOL_ID)
		pDmm = &AppDmm;
	else if(pool_id == BCN_POOL_ID)
		pDmm = &BcnDmm;
	else if(pool_id == MGMT_POOL_ID)
		pDmm = &MgmtDmm;		
	
    /* search for the closest memory segment */
    for (i = 0; i < pDmm->slabnum; i++)
    {
        if (size <= pDmm->slab[i].segsize)
        {
            if (!SLIST_IsEmpty(&(pDmm->slab[i].freelist)))
            {
                slink = SLIST_Pop(&(pDmm->slab[i].freelist));
                segdesc = SLIST_GetEntry(slink, sSegDesc, link);  
				segdesc->poolid = pool_id;
                pDmm->slab[i].inuse++;
				if (pDmm->slab[i].inuse > pDmm->slab[i].maxuse)
{
				    pDmm->slab[i].maxuse = pDmm->slab[i].inuse;
}
#ifdef P8051
//    FM_Printf(FM_WARN, "DMM: alloc mem seg %d, mem (%p, %p), data %p\n", segdesc->segsize, segdesc->mem, segdesc, (u8 *)segdesc + sizeof(sSegDesc));
//    FM_HexDump(FM_DATA|FM_MINFO, "alloc segment:", (u8 *)segdesc, Dmm.slab[i].segsize + sizeof(sSegDesc) );
#else
//    FM_Printf(FM_WARN, "DMM: alloc mem seg %d, mem 0x%lx, 0x%lx, payload 0x%lx\n", segdesc->segsize, segdesc->mem, segdesc, (u8 *)segdesc + sizeof(sSegDesc));
#endif
                return (u8 *)segdesc + sizeof(sSegDesc);
            }
        }
    }
#ifdef LOG_FLASH
    logEvent(MEM_ERROR, pool_id, 0, &size, sizeof(u16));
#endif
    return NULL;
}


void DMM_Free(u8 *mem)
{
    u8         i;
    sSegDesc  *segdesc = (sSegDesc *)(mem - sizeof(sSegDesc)); 
	sDmm* pDmm;
	if(segdesc->poolid == FW_POOL_ID)
		pDmm = &FwDmm;	
	else if(segdesc->poolid == APP_POOL_ID)
		pDmm = &AppDmm;		
	else if(segdesc->poolid == BCN_POOL_ID)
		pDmm = &BcnDmm; 
	else if(segdesc->poolid == MGMT_POOL_ID)
		pDmm = &MgmtDmm;		
	
    for (i = 0; i < pDmm->slabnum; i++)
    {
        if(segdesc->segsize == pDmm->slab[i].segsize)
        {
#ifdef P8051
//      FM_Printf(FM_WARN, "DMM: free mem seg %d, mem (%p, %p), data %p\n", segdesc->segsize, segdesc->mem, segdesc, mem);
//	  FM_HexDump(FM_DATA|FM_MINFO, "free segment:", (u8 *)segdesc, segdesc->segsize + sizeof(sSegDesc) );
//	  FM_HexDump(FM_DATA|FM_MINFO, "free segment:", (u8 *)segdesc, 256);
#else
//    FM_Printf(FM_WARN, "DMM: free mem seg %d, mem 0x%lx, 0x%lx, payload 0x%lx\n", segdesc->segsize, segdesc->mem, segdesc, mem);
#endif
            SLINK_Init(&segdesc->link);
            SLIST_Put(&(pDmm->slab[i].freelist), &segdesc->link);
            pDmm->slab[i].inuse--;
            return;
        }
    }
   // FM_Printf(FM_ERROR, "DMM: cannot find the memory segment to free %d.\n", segdesc->segsize);
}

/** =========================================================
 *
 * Edit History
 *
 * $Source: /home/cvsrepo/Hybrii_B_OSFW_Dev/firmware/common/dmm.c,v $
 *
 * $Log: dmm.c,v $
 * Revision 1.7  2014/08/25 07:37:34  kiran
 * 1) RSSI & LQI support
 * 2) Fixed Sync related issues
 * 3) Fixed timer 0 timing drift for SDK
 * 4) MMSG & Error Logging in Flash
 *
 * Revision 1.6  2014/06/24 16:26:44  ranjan
 * -zigbee frame_handledata fix.
 * -added reason code for uppermac host events
 * -small cleanups
 *
 * Revision 1.5  2014/06/19 17:13:19  ranjan
 * -uppermac fixes for lvnet and reset command for cco and sta mode
 * -backup cco working
 *
 * Revision 1.4  2014/06/12 13:15:43  ranjan
 * -separated bcn,mgmt,um event pools
 * -fixed datapath issue due to previous checkin
 * -work in progress. neighbour cco detection
 *
 * Revision 1.3  2014/05/28 10:58:58  prashant
 * SDK folder structure changes, Uart changes, removed htm (UI) task
 * Varified - UM, LM - iperf (UDP/TCP), overnight test pass
 *
 * Revision 1.2  2014/01/10 17:02:18  yiming
 * check in Rajan 1/8/2014 code release
 *
 * Revision 1.3  2014/01/08 10:53:53  ranjan
 * Changes for LM OS support.
 * New Datapath FrameTask
 * LM and UM  datapath, feature verified.
 *
 * known issues : performance numbers needs revisit
 *
 * review : pending.
 *
 * Revision 1.2  2013/01/24 00:13:46  yiming
 * Use 01-23-2013 Hybrii-A code as first Hybrii-B code base
 *
 * Revision 1.5  2012/07/30 04:37:55  yuanhua
 * fixed an issue that an event memory could be overwritten in the HAL when the HAL receives a mgmt message.
 *
 * Revision 1.4  2012/07/29 02:59:22  yuanhua
 * Initialize the internel queue of CTRL Layer to fix an issue of unexpected event free error message.
 *
 * Revision 1.3  2012/07/25 04:36:08  yuanhua
 * enable the DMM.
 *
 * Revision 1.2  2012/07/24 04:27:46  yuanhua
 * update the project to include DMM.
 *
 * Revision 1.1  2012/07/24 04:23:17  yuanhua
 * added DMM code for dynamic alloction with static memory to avoid memory fragmentation.
 *
 *
 * ==========================================================*/

