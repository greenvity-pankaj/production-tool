/** =======================================================
 * @file event.h
 * 
 *  @brief All event definations and structure types can be found here
 *
 *  Copyright (C) 2010-2015, Greenvity Communications, Inc.
 *  All Rights Reserved
 *  
 * ========================================================*/


#ifndef _EVENT_H
#define _EVENT_H

/****************************************************************************** 
  *	Defines
  ******************************************************************************/
  
#define EVENT_CLASS_CTRL   						0
#define EVENT_CLASS_MSG    						1
#define EVENT_CLASS_MGMT   						2
#define EVENT_CLASS_DATA   						3

#define EVENT_STATUS_COMPLETE    				0
#define EVENT_STATUS_FORWARD     				1

//buffer descriptor
typedef struct buffDesc
{
    u8      *dataptr;
    u16      datalen;
    u8      *buff;
    u16      bufflen;
} sBuffDesc, *psBuffDesc;

typedef struct dataBuff
{
    sSlink    link;
    sBuffDesc buffDesc;
}sDb, *psDb;

typedef struct eventHdr
{
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

/****************************************************************************** 
  *	Function Prototypes
  ******************************************************************************/
  
sEvent* GV701x_EVENT_Alloc(u16 size, u16 headroom);
void EVENT_Free(sEvent *event);
void EVENT_Assert(sEvent *event);

#endif //_EVENT_H