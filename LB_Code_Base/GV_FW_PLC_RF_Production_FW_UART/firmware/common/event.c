/** =======================================================
 * @file event.c
 * 
 *  @brief Event Module
 *
 *  Copyright (C) 2010-2011, Greenvity Communications, Inc.
 *  All Rights Reserved
 *  
 * ========================================================*/

#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include "papdef.h"
#include "list.h"
#include "event.h"
#include "fm.h"
#include "dmm.h"


sDb *DB_Alloc(u16 size, u16 headroom)
{
     sDb xdata *db = NULL;
     sDb *dbGen;

//    db = ( sDb xdata *)malloc(sizeof(sDb) + size + headroom);
	db = ( sDb xdata *)DMM_Alloc(FW_POOL_ID, sizeof(sDb) + size + headroom);
    if(db == NULL)
    {
        FM_Printf(FM_ERROR, "DB: Fail to allocate a data buffer.\n");
        return NULL;
    }
    else
    {
        memset(db, 0, sizeof(sDb) + size + headroom);
        db->buffDesc.buff = (u8 *)db + sizeof(sDb);
        db->buffDesc.dataptr = db->buffDesc.buff + headroom;
        db->buffDesc.datalen = 0;
        db->buffDesc.bufflen = size + headroom;
        SLINK_Init(&db->link);
        dbGen = db;
    }
    return dbGen;
}



void DB_Free(sDb *db)
{
//    free(db);
    DMM_Free((u8 *)db);
}




sEvent * EVENT_BcnAlloc(u16 size, u16 headroom)
{
    sEvent xdata *event = NULL;
    //sEvent *eventGen;
//    event = (sEvent xdata*) malloc(sizeof(sEvent) + size + headroom);
#ifdef UART_HOST_INTF
	u8 intFlag = EA;		
	EA = 0;
#endif

	event = (sEvent *) DMM_Alloc(BCN_POOL_ID, sizeof(sEvent) + size + headroom);
    if(event == NULL)
    {
        FM_Printf(FM_ERROR, "EVENTBcn: EAF\n");
#ifdef UART_HOST_INTF
		EA = intFlag;
#endif
        return NULL;
    }
    else
    {
        memset(event, 0, sizeof(sEvent) + size + headroom);
        event->buffDesc.buff = (u8 *)event + sizeof(sEvent);
        event->buffDesc.dataptr = event->buffDesc.buff + headroom;
        event->buffDesc.datalen = 0;
        event->buffDesc.bufflen = size + headroom;
        event->eventHdr.status = EVENT_STATUS_COMPLETE;
        SLINK_Init(&event->link);
        //eventGen = event;
    }
#ifdef UART_HOST_INTF
	EA = intFlag;
#endif
    return event;//eventGen;
}


sEvent * EVENT_MgmtAlloc(u16 size, u16 headroom)
{
    sEvent xdata *event = NULL;
    //sEvent *eventGen;
	event = (sEvent *) DMM_Alloc(MGMT_POOL_ID, sizeof(sEvent) + size + headroom);
    if(event == NULL)
    {
        FM_Printf(FM_ERROR, "EVENT:EAF MGMT\n");		
        return NULL;
    }
    else
    {
        memset(event, 0, sizeof(sEvent) + size + headroom);
        event->buffDesc.buff = (u8 *)event + sizeof(sEvent);
        event->buffDesc.dataptr = event->buffDesc.buff + headroom;
        event->buffDesc.datalen = 0;
        event->buffDesc.bufflen = size + headroom;
        event->eventHdr.status = EVENT_STATUS_COMPLETE;
        SLINK_Init(&event->link);
        //eventGen = event;
    }
    return event;//eventGen;
}

sEvent* GV701x_EVENT_Alloc(u16 size, u16 headroom) 	
{
    sEvent xdata *event = NULL;
	u8 intEa = EA;
    sEvent *eventGen;
	EA = 0;
	event = (sEvent *) DMM_Alloc(APP_POOL_ID, sizeof(sEvent) + size + headroom);
	EA = intEa;
    if(event == NULL)
    {
        FM_Printf(FM_USER, "GV701x EVENTAPP: Fail \n");
        return NULL;
    }
    else
    {
        memset(event, 0, sizeof(sEvent) + size + headroom);
        event->buffDesc.buff = (u8 *)event + sizeof(sEvent);
        event->buffDesc.dataptr = event->buffDesc.buff + headroom;
        event->buffDesc.datalen = 0;
        event->buffDesc.bufflen = size + headroom;
        event->eventHdr.status = EVENT_STATUS_COMPLETE;
        SLINK_Init(&event->link);
        eventGen = event;
    }
	return eventGen;
}

static const u8 pool[] = {FW_POOL_ID,MGMT_POOL_ID,BCN_POOL_ID,APP_POOL_ID};

sEvent * EVENT_Alloc(u16 size, u16 headroom) __REENTRANT__ 
{
    sEvent xdata *event = NULL;	
#if 0	
	u8 i;
#endif	
    //sEvent *eventGen;
//    event = (sEvent xdata*) malloc(sizeof(sEvent) + size + headroom);
#if 0
#ifdef NO_HOST
	for(i=0;i<=3;i++)
#else
	for(i=0;i<=2;i++)
#endif
#endif

	{
		event = (sEvent *) DMM_Alloc(FW_POOL_ID, sizeof(sEvent) + size + headroom);
	    if(event == NULL)
	    {
	        FM_Printf(FM_ERROR, "EVENT:EAF\n");
	    }
	    else
	    {
	        memset(event, 0, sizeof(sEvent) + size + headroom);
	        event->buffDesc.buff = (u8 *)event + sizeof(sEvent);
	        event->buffDesc.dataptr = event->buffDesc.buff + headroom;
	        event->buffDesc.datalen = 0;
	        event->buffDesc.bufflen = size + headroom;
	        event->eventHdr.status = EVENT_STATUS_COMPLETE;
	        SLINK_Init(&event->link);
			return event;
	        //eventGen = event;
	    }
		
	}
    return NULL;
}


void EVENT_Free(sEvent *event)
{
//    free(event);
    DMM_Free((u8 *)event);
}


void EVENT_Assert(sEvent *event)
{
    assert((event->buffDesc.dataptr >= event->buffDesc.buff)&&
           ((event->buffDesc.dataptr - event->buffDesc.buff + 
             event->buffDesc.datalen) <= event->buffDesc.bufflen));  
}


 
/** =========================================================
 *
 * Edit History
 *
 * $Source: /home/cvsrepo/Hybrii_B_OSFW_Dev/firmware/common/event.c,v $
 *
 * $Log: event.c,v $
 * Revision 1.6  2015/01/02 14:55:35  kiran
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
 * Revision 1.5  2014/11/11 14:52:56  ranjan
 * 1.New Folder Architecture espically in /components
 * 2.Modular arrangment of functionality in new files
 *    anticipating the need for exposing them as FW App
 *    development modules
 * 3.Other improvisation in code and .h files
 *
 * Revision 1.4  2014/09/25 10:57:41  prashant
 * 1. GPIO API swapping issue fixed.
 * 2. Supported 1 to 512 frame length for uart.
 * 3. list.h file cleanup (code deleted).
 * 4. Supporting minirobo for mgmt frames.
 *
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
 * Revision 1.1  2013/12/17 21:42:26  yiming
 * no message
 *
 * Revision 1.2  2013/01/24 00:13:46  yiming
 * Use 01-23-2013 Hybrii-A code as first Hybrii-B code base
 *
 * Revision 1.7  2012/07/25 04:36:08  yuanhua
 * enable the DMM.
 *
 * Revision 1.6  2012/07/24 04:23:17  yuanhua
 * added DMM code for dynamic alloction with static memory to avoid memory fragmentation.
 *
 * Revision 1.5  2012/07/12 05:44:00  kripa
 * Use xdata pointers in Alloc() functions, to fix the 'never returns NULL' issue.
 * Committed on the Free edition of March Hare Software CVSNT Client.
 * Upgrade to CVS Suite for more features and support:
 * http://march-hare.com/cvsnt/
 *
 * Revision 1.4  2012/05/19 05:05:15  yuanhua
 * optimized the timer handlers in CTRL and LINK layers.
 *
 * Revision 1.3  2012/03/11 17:02:24  yuanhua
 * (1) added NEK auth in AKM (2) added NMA (3) modified hpgp layer data structures (4) added Makefile for linux simulation
 *
 * Revision 1.2  2011/09/09 07:02:31  yuanhua
 * migrate the firmware code from the greenchip to the hybrii.
 *
 * Revision 1.3  2011/08/09 22:45:44  yuanhua
 * changed to event structure, seperating HPGP-related events from the general event defination so that the general event could be used for other purposes than the HPGP.
 *
 * Revision 1.2  2011/06/24 14:33:18  yuanhua
 * (1) Changed event structure (2) Implemented SNSM, including the state machines in network discovery and connection states, becaon process, discover process, and handover detection (3) Integrated the HPGP and SHAL
 *
 * Revision 1.1  2011/05/06 18:31:47  kripa
 * Adding common utils and isr files for Greenchip firmware.
 *
 * Revision 1.2  2011/04/23 23:09:10  kripa
 * EVENT_Alloc(); changed 'event->data' reference to 'event->eventData'.
 *
 * Revision 1.1  2011/04/08 21:40:59  yuanhua
 * Framework
 *
 *
 * ========================================================*/


