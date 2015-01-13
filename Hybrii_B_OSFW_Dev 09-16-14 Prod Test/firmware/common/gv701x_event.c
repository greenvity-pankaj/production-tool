/** =======================================================
 * @file appevent.c
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
#include "gv701x_event.h"
#include "dmm.h"
#include "fm.h"

extern sDmm AppDmm;

sAppEvent* GV701x_EVENT_Alloc(u16 size, u16 headroom) 	
{
    sAppEvent xdata *event = NULL;
    sAppEvent *eventGen;
	event = (sAppEvent *) DMM_Alloc(APP_POOL_ID, sizeof(sAppEvent) + size + headroom);
    if(event == NULL)
    {
        FM_Printf(FM_ERROR, "EVENT: Fail to allocate an event.\n");
        return NULL;
    }
    else
    {
        memset(event, 0, sizeof(sAppEvent) + size + headroom);
        event->buffDesc.buff = (u8 *)event + sizeof(sAppEvent);
        event->buffDesc.dataptr = event->buffDesc.buff + headroom;
        event->buffDesc.datalen = 0;
        event->buffDesc.bufflen = size + headroom;
        event->eventHdr.status = APP_EVENT_STATUS_COMPLETE;
        SLINK_Init(&event->link);
        eventGen = event;
    }
	return eventGen;
}

void GV701x_EVENT_Free(sAppEvent *event)	
{
    DMM_Free((u8 *)event);
}

void GV701x_EVENT_Assert(sAppEvent *event)	
{
    assert((event->buffDesc.dataptr >= event->buffDesc.buff)&&
           ((event->buffDesc.dataptr - event->buffDesc.buff + 
             event->buffDesc.datalen) <= event->buffDesc.bufflen));  
}
 
/** =========================================================
 *
 * Edit History
 *
 * $Source: /home/cvsrepo/Hybrii_B_OSFW_Dev/firmware/common/gv701x_event.c,v $
 *
 * $Log: gv701x_event.c,v $
 * Revision 1.1  2014/05/28 10:58:58  prashant
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


