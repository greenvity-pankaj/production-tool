/** =======================================================
 * @file event.h
 * 
 *  @brief Event Module
 *
 *  Copyright (C) 2010-2011, Greenvity Communications, Inc.
 *  All Rights Reserved
 *  
 * ========================================================*/


#ifndef _EVENT_H
#define _EVENT_H

#include "papdef.h"
#include "list.h"
//#include "rm.h"



#define EVENT_STATUS_COMPLETE    0
#define EVENT_STATUS_FORWARD     1




//buffer descriptor
typedef struct buffDesc
{
	u8      *dataptr;     //point to the data in buffer
	u16      datalen;
	u8      *buff;
	u16      bufflen;
} sBuffDesc, *psBuffDesc;



typedef struct eventHdr
{
	//basic event header
	u16      type;             //event type
	u8       eventClass: 2;    //0: control event. 1: msg event, 2: mgmt event
	u8       status:     2;    //event status: forward or complete
	u8       trans:      4;    //destination transaction
}sEventHdr, *psEventHdr;



typedef struct event
{
	sSlink   link;
	sBuffDesc buffDesc;
	sEventHdr eventHdr;
}sEvent, *psEvent;



sEvent *EVENT_Alloc(u16 size, u16 headroom);
void EVENT_Free(sEvent *event);

void EVENT_Assert(sEvent *event);

#endif


/** =========================================================
 *
 * Edit History
 *
 * $Source: /home/cvsrepo/hybrii/firmware/common/event.h,v $
 *
 * $Log: event.h,v $
 * Revision 1.2  2011/09/09 07:02:31  yuanhua
 * migrate the firmware code from the greenchip to the hybrii.
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
 * Revision 1.6  2011/07/16 17:11:23  yuanhua
 * (1)Implemented SHOM and CHOM modules, including handover procedure, SCB resource updating for HO (2) Update SNAM and CNAM modules to support uer-appointed CCo handover (3) Made the SCB resources to support the TEI MAP for the STA mode and management of associated STA resources (e.g. TEI) (4) Modified SNSM and CNSM to perform all types of handover switch (CCo handover to the new STA, STA taking over the CCo, STA switching to the new CCo)
 *
 * Revision 1.5  2011/07/08 22:23:48  yuanhua
 * (1) Implemented CNSM, including its state machine, beacon transmission and process, discover beacon scheduling, auto CCo selection, discover list, handover countdown, etc. (2) Updated SNSM, including discover list processing, triggering a switch to the new CCo, etc. (3) Updated CNAM and SNAM, adding the connection state in the SNAM, switch to the new CCo, etc. (4) Other updates
 *
 * Revision 1.4  2011/07/02 22:09:01  yuanhua
 * Implemented both SNAM and CNAM modules, including network join and leave procedures, systemm resource (such as TEI) allocation and release, TEI renew/release timers, and TEI reuse timer, etc.
 *
 * Revision 1.3  2011/06/24 14:33:18  yuanhua
 * (1) Changed event structure (2) Implemented SNSM, including the state machines in network discovery and connection states, becaon process, discover process, and handover detection (3) Integrated the HPGP and SHAL
 *
 * Revision 1.2  2011/05/28 06:23:56  kripa
 * combining list header files
 *
 * Revision 1.1  2011/05/06 18:31:47  kripa
 * Adding common utils and isr files for Greenchip firmware.
 *
 * Revision 1.3  2011/04/23 23:11:39  kripa
 * sEvent Struct; renamed 'u8* data' to 'u8* eventData', since data is reserved word in Keil.
 *                changed 'u8 buff[0]' to u8 buff[1]', since Keil doesnt allow empty array field.
 *
 * Revision 1.2  2011/04/23 17:11:59  kripa
 * Renamed 'u8 class' field in 'struct event' to 'eventClass'; 'class' is a keyword in VC.
 *
 * Revision 1.1  2011/04/08 21:40:41  yuanhua
 * Framework
 *
 *
 * ========================================================*/


