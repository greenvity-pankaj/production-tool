/** =======================================================
 * @file event.c
 * 
 *  @brief Event Module
 *
 *  Copyright (C) 2010-2011, Greenvity Communications, Inc.
 *  All Rights Reserved
 *  
 * ========================================================*/
#ifdef MODULE
#include <linux/module.h>
#include <linux/netlink.h>
#else
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#endif
#include "./event.h"
#include "fm.h"


sEvent * EVENT_Alloc(u16 size, u16 headroom)
{
	sEvent *event = NULL;
#ifdef MODULE
	event = (sEvent *) kmalloc(sizeof(sEvent) + size + headroom, GFP_USER);
#else
	event = (sEvent *) malloc(sizeof(sEvent) + size + headroom);
#endif
	if (event == NULL) {
#ifdef MODULE
		printk("EVENT: Fail to allocate an event.\n");
#else
		FM_Printf(FM_ERROR, "EVENT: Fail to allocate an event.\n");
#endif
	} else {
		memset(event, 0, sizeof(sEvent) + size + headroom);
		event->buffDesc.buff = (u8 *)event + sizeof(sEvent);
		event->buffDesc.dataptr = event->buffDesc.buff + headroom;
		event->buffDesc.datalen = 0;
		event->buffDesc.bufflen = size + headroom;
		event->eventHdr.status = EVENT_STATUS_COMPLETE;
		SLINK_Init(&event->link);
	}
	return event;
}


void EVENT_Free(sEvent *event)
{
#ifdef MODULE
	kfree(event);
#else
	free(event);
#endif
}


void EVENT_Assert(sEvent *event)
{
#ifdef MODULE
	if ((event->buffDesc.dataptr >= event->buffDesc.buff)&&
           ((event->buffDesc.dataptr - event->buffDesc.buff + 
             event->buffDesc.datalen) <= event->buffDesc.bufflen));  
		return;
#else
	assert((event->buffDesc.dataptr >= event->buffDesc.buff)&&
           ((event->buffDesc.dataptr - event->buffDesc.buff + 
             event->buffDesc.datalen) <= event->buffDesc.bufflen));  
#endif
}


 
/** =========================================================
 *
 * Edit History
 *
 * $Source: /home/cvsrepo/hybrii/firmware/common/event.c,v $
 *
 * $Log: event.c,v $
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


