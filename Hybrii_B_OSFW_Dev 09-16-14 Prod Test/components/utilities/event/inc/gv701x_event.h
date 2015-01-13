/** =======================================================
 * @file appevent.h
 * 
 *  @brief Event Module
 *
 *  Copyright (C) 2010-2011, Greenvity Communications, Inc.
 *  All Rights Reserved
 *  
 * ========================================================*/

#ifndef GV701X_EVENT_H
#define GV701X_EVENT_H

#define APP_EVENT_CLASS_CTRL   0
#define APP_EVENT_CLASS_MSG    1
#define APP_EVENT_CLASS_MGMT   2
#define APP_EVENT_CLASS_DATA   3


#define APP_EVENT_STATUS_COMPLETE    0
#define APP_EVENT_STATUS_FORWARD     1

//buffer descriptor
typedef struct appbuffDesc
{
    u8      *dataptr;     //point to the data in buffer
    u16      datalen;
    u8      *buff;
    u16      bufflen;
} sAppBuffDesc, *psAppBuffDesc;

typedef struct appdataBuff
{
    sSlink    link;
    sAppBuffDesc buffDesc;
}sAppDb, *psAppDb;

typedef struct appeventHdr
{
    //basic event header
    u16      type;             //event type
    u8       eventClass: 2;    //0: control event. 1: msg event, 2: mgmt event
    u8       status:     2;    //event status: forward or complete
    u8       trans:      4;    //destination transaction
}sAppEventHdr, *psAppEventHdr;


typedef struct appevent
{
    sSlink   link;
    sAppBuffDesc buffDesc;
    sAppEventHdr eventHdr;
}sAppEvent, *psAppEvent;


sAppEvent* GV701x_EVENT_Alloc(u16 size, u16 headroom);

void GV701x_EVENT_Free(sAppEvent *event);

#endif // GV701X_EVENT_H
