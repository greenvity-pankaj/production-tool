/** ========================================================
 *
 *  @file stm.h
 * 
 *  @brief Dynamic Memory Manager (DMM)
 *
 *  Copyright (C) 2012, Greenvity Communications, Inc.
 *  All Rights Reserved
 *  
 * =========================================================*/

#ifndef  _DMM_H
#define  _DMM_H

#include "papdef.h"
#include "list.h"

#define DMM_SLAB_MAX          8
#define FW_POOL_ID	          1
#define APP_POOL_ID	          2
#define BCN_POOL_ID	          3
#define MGMT_POOL_ID          4



/* DMM slab descriptor */
typedef struct slabdesc
{
    u8     segnum;       /* number of memory segments in a slab */ 
    u16    segsize;      /* size of memory segments */ 
} sSlabDesc, *psSlabDesc;


/* memory segment descriptor */
typedef struct segdesc
{
    sSlink     link;         /* link */
    u16        segsize;      /* segment size */
    u8         poolid;      /* pool id*/	
    u8        *mem;
} sSegDesc, *psSegDesc;

typedef struct slab
{
    sSlist      freelist;    /* free list */
    u16         segsize;     /* size of slab memory segment */
    u8          inuse;       /* number of memory segment used */
    u8          maxuse;      /* maximum number of memory segment 
                              * used simultanousely (for statistics) */
} sSlab, *psSlab;

typedef struct dmm
{
    //timer resource
    sSlab slab[DMM_SLAB_MAX];
    u8    slabnum;

} sDmm, *psDmm;

eStatus DMM_Init(u8 pool_id);
eStatus DMM_InitMemPool(u8 pool_id, u8 *mempool, u16 poolsize, 
                     sSlabDesc *slabdesc, u8 slabnum);
u8*     DMM_Alloc(u8 pool_id, u16 size);
void    DMM_Free(u8 *mem);
void    DMM_DisplayMemUsage(sDmm* pDmm);


#endif


/** =========================================================
 *
 * Edit History
 *
 * $Source: /home/cvsrepo/Hybrii_B_OSFW_Dev/firmware/common/dmm.h,v $
 *
 * $Log: dmm.h,v $
 * Revision 1.3  2014/06/12 13:15:43  ranjan
 * -separated bcn,mgmt,um event pools
 * -fixed datapath issue due to previous checkin
 * -work in progress. neighbour cco detection
 *
 * Revision 1.2  2014/05/28 10:58:58  prashant
 * SDK folder structure changes, Uart changes, removed htm (UI) task
 * Varified - UM, LM - iperf (UDP/TCP), overnight test pass
 *
 * Revision 1.1  2013/12/18 17:03:14  yiming
 * no message
 *
 * Revision 1.1  2013/12/17 21:42:27  yiming
 * no message
 *
 * Revision 1.2  2013/01/24 00:13:46  yiming
 * Use 01-23-2013 Hybrii-A code as first Hybrii-B code base
 *
 * Revision 1.1  2012/07/24 04:23:17  yuanhua
 * added DMM code for dynamic alloction with static memory to avoid memory fragmentation.
 *
 *
 * =========================================================*/

