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
#include "list.h"
#include "event.h"

#ifdef RTX51_TINY_OS
#include <rtx51tny.h>
#endif


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

extern u8 XDATA BcnMemPool[];
extern u8 XDATA MgmtMemPool[];

extern u8 XDATA MemPool[];


void DMM_DisplayEventBody(u8 pool_id, u8 *poolBuff)
{
	sSegDesc *pSegDesc = (sSegDesc*)poolBuff;
	u8 *ptr = poolBuff;
	u16 poolsize;
	 	
	sDmm *pDmm;
	

	if(pool_id == FW_POOL_ID)
		pDmm = &FwDmm;
	else if(pool_id == APP_POOL_ID)		
		pDmm = &AppDmm;			
	else if(pool_id == BCN_POOL_ID)		
		pDmm = &BcnDmm; 
	else if(pool_id == MGMT_POOL_ID)		
		pDmm = &MgmtDmm;

	poolsize = pDmm->poolSize;

	while(poolsize)
	{
		sEvent *event;
		
		pSegDesc = (sSegDesc*)ptr;

		if (pSegDesc->active)
		{
			event = (sEvent*)(ptr + sizeof(sSegDesc));
			
			FM_Printf(FM_USER,"cl %bu \n", event->eventHdr.eventClass);
			
			FM_HexDump(FM_USER, "ty", (u8*)&event->eventHdr.type, 2);

			FM_HexDump(FM_USER, "bd", (u8*)event->buffDesc.dataptr,
					   event->buffDesc.datalen);		
		}
		
		ptr += ( sizeof (pSegDesc) + pSegDesc->segsize );
		poolsize -= (sizeof(pSegDesc) + pSegDesc->segsize);

	}


}

void DMM_DisplayEventTypes(u8 pool_id, u8 *poolBuff)
{
	sSegDesc *pSegDesc = (sSegDesc*)poolBuff;
	u8 *ptr = poolBuff;
	sDmm *pDmm;
	
	u16 poolsize;
	

	if(pool_id == FW_POOL_ID)
		{
		pDmm = &FwDmm;
		
		}
	else if(pool_id == APP_POOL_ID)
		{
		pDmm = &AppDmm;	
		
		}
	else if(pool_id == BCN_POOL_ID)
		{
			pDmm = &BcnDmm; 
			
		}
	else if(pool_id == MGMT_POOL_ID)
		{
			pDmm = &MgmtDmm;
			

		}

	poolsize = pDmm->poolSize;
	
	while(poolsize)
	{
		sEvent *event;
		
		pSegDesc = (sSegDesc*)ptr;

		if (pSegDesc->active)
		{
			event = (sEvent*)(ptr + sizeof(sSegDesc));
			
			FM_Printf(FM_USER, "cl %bu \n", event->eventHdr.eventClass);
			
			FM_HexDump(FM_USER, "ev", (u8*)&event->eventHdr.type, 2);
		}
		
		ptr += ( sizeof(sSegDesc) + pSegDesc->segsize );
		poolsize -= (sizeof(sSegDesc) + pSegDesc->segsize);

		os_switch_task();

	}


}

void DMM_eventMem()
{
	DMM_DisplayMemUsage(&FwDmm);
	DMM_DisplayEventTypes(FW_POOL_ID, MemPool);

}

void DMM_MgmtMem()
{
	DMM_DisplayMemUsage(&MgmtDmm);
	DMM_DisplayEventTypes(MGMT_POOL_ID, MgmtMemPool);

}
void DMM_BcnMem()
{
	DMM_DisplayMemUsage(&BcnDmm);
	DMM_DisplayEventTypes(BCN_POOL_ID, BcnMemPool);
	

}

void DMM_DisplayMemUsage(sDmm* pDmm)
{
    u8 i;
    sSlab     *slab = NULL;
    FM_Printf(FM_WARN, "DMM:Disp mem usage:\n");
    for (i = 0; i < pDmm->slabnum; i++)
    {
    	u8 boolval = SLIST_IsEmpty(&slab->freelist);
		
        slab = &(pDmm->slab[i]); 
#ifdef P8051
        FM_Printf(FM_WARN, "DMM:Slab %bu\n", i);
        FM_Printf(FM_WARN, "DMM:mem seg total  %bu\n", slab->total);
        FM_Printf(FM_WARN, "DMM:mem seg use %bu\n", slab->inuse);
        FM_Printf(FM_WARN, "DMM:isEmpty %bu\n",boolval );
#else 
        FM_Printf(FM_WARN, "DMM:Slab %d\n", i);
        FM_Printf(FM_WARN, "DMM:total %d\n", slab->total);
#endif
    }

}

u8 DMM_CheckDepth(sDmm* pDmm, u16 size)
{
    u8 i;
    u8 freeCnt = 0;
    sSlab     *slab = NULL;

    size += sizeof(sEvent);

    for(i=0; i< pDmm->slabnum; i++)
    {
        slab = &(pDmm->slab[i]); 
        if(slab->segsize >= size)
        {
            freeCnt += (slab->total - slab->inuse);
        }
    }
    if(freeCnt < 3)
    {
        FM_Printf(FM_ERROR,"Pool Full: %u\n",size);
        return FALSE;
    }
    else
    {
        return TRUE;
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
		{
		pDmm = &FwDmm;
		}
	else if(pool_id == APP_POOL_ID)
		{
		pDmm = &AppDmm;	
		}
	else if(pool_id == BCN_POOL_ID)
		{
			pDmm = &BcnDmm; 
		}
	else if(pool_id == MGMT_POOL_ID)
		{
			pDmm = &MgmtDmm;

		}

	//pDmm->poolSize = poolsize;
    memset(mempool, 0, poolsize);  

    for (i = 0; i < slabnum; i++)
    {
//        FM_Printf(FM_WARN, "DMM: Slab %bu \n", i);
        slab = &(pDmm->slab[i]);  
        pDmm->slab[i].segsize = slabdesc[i].segsize;
		pDmm->slab[i].inuse = 0;
        pDmm->slab[i].total = slabdesc[i].segnum;
        segnum = 0;
        while (((memsize + sizeof(sSegDesc) + slabdesc[i].segsize) <= poolsize)
               && (segnum < slabdesc[i].segnum))
        {
            segdesc = (sSegDesc *)mem;
            SLINK_Init(&segdesc->link);          
            segdesc->segsize = slabdesc[i].segsize;
            segdesc->mem = mem;
			segdesc->active = 0;				
			segdesc->poolid = pool_id;			
			//FM_Printf(FM_USER,"seg %u \n", mem);
			//FM_HexDump(FM_USER,"seg", mem, sizeof(sSegDesc));		
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
			FM_Printf(FM_ERROR, "DMM:mem pool small\n");
			return STATUS_FAILURE;
		}
			
    }
    pDmm->slabnum = slabnum;
	pDmm->poolSize = memsize;
    FM_Printf(FM_WARN, "DMM:mem pool init done\n");
//    DMM_DisplayFreeList();
    return STATUS_SUCCESS;
}



u8 *DMM_Alloc(u8 pool_id, u16 size)
{
    u8         i;
    sSlink  xdata  *slink = NULL;
    sSegDesc xdata *segdesc = NULL; 
	sDmm* pDmm;
#ifdef UART_HOST_INTF
	u8 intFlag = EA;       
	EA = 0;
#endif

	if(pool_id == FW_POOL_ID)
		pDmm = &FwDmm;
	else if(pool_id == APP_POOL_ID)
		pDmm = &AppDmm;
	else if(pool_id == BCN_POOL_ID)
		pDmm = &BcnDmm;
	else if(pool_id == MGMT_POOL_ID)
		pDmm = &MgmtDmm;		
	else
		FM_Printf(FM_ERROR,"poolerror %bu\n", pool_id);	
	
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
				segdesc->active = 1;
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
#ifdef UART_HOST_INTF
				EA = intFlag;
#endif
                return (u8 *)segdesc + sizeof(sSegDesc);
            }
        }
    }
#ifdef UART_HOST_INTF
	EA = intFlag;
#endif
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
#ifdef UART_HOST_INTF
	u8 intFlag = EA;
	EA = 0;
#endif
	if(segdesc->poolid == FW_POOL_ID)
		pDmm = &FwDmm;	
	else if(segdesc->poolid == APP_POOL_ID)
		pDmm = &AppDmm;		
	else if(segdesc->poolid == BCN_POOL_ID)
		pDmm = &BcnDmm; 
	else if(segdesc->poolid == MGMT_POOL_ID)
		pDmm = &MgmtDmm;	
	else
		FM_Printf(FM_ERROR,"freePool %bu\n", segdesc->poolid);
		
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
			segdesc->active = 0;
            SLIST_Put(&(pDmm->slab[i].freelist), &segdesc->link);
            pDmm->slab[i].inuse--;
#ifdef UART_HOST_INTF
			EA = intFlag;
#endif
            return;
        }
    }
#ifdef UART_HOST_INTF
	EA = intFlag;
#endif
   // FM_Printf(FM_ERROR, "DMM: cannot find the memory segment to free %d.\n", segdesc->segsize);
}

/** =========================================================
 *
 * Edit History
 *
 * $Source: /home/cvsrepo/Hybrii_B_OSFW_Dev/firmware/common/dmm.c,v $
 *
 * $Log: dmm.c,v $
 * Revision 1.12  2015/01/02 14:55:35  kiran
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
 * Revision 1.11  2014/11/11 14:52:56  ranjan
 * 1.New Folder Architecture espically in /components
 * 2.Modular arrangment of functionality in new files
 *    anticipating the need for exposing them as FW App
 *    development modules
 * 3.Other improvisation in code and .h files
 *
 * Revision 1.10  2014/10/28 16:27:42  kiran
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
 * Revision 1.9  2014/09/25 10:57:41  prashant
 * 1. GPIO API swapping issue fixed.
 * 2. Supported 1 to 512 frame length for uart.
 * 3. list.h file cleanup (code deleted).
 * 4. Supporting minirobo for mgmt frames.
 *
 * Revision 1.8  2014/09/19 06:23:59  prashant
 * Uart data flow changed
 *
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

