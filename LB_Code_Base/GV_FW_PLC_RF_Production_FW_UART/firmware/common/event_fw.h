/** =======================================================
 * @file event_fw.h
 * 
 *  @brief Event Module for the firmware
 *
 *  Copyright (C) 2010-2011, Greenvity Communications, Inc.
 *  All Rights Reserved
 *  
 * ========================================================*/


#ifndef _EVENT_FW_H
#define _EVENT_FW_H

#include "papdef.h"
#include "list.h"
#include "event.h"

sDb *DB_Alloc(u16 size, u16 headroom);
void DB_Free(sDb *db);

sEvent * EVENT_BcnAlloc(u16 size, u16 headroom);
sEvent * EVENT_MgmtAlloc(u16 size, u16 headroom);
sEvent *EVENT_Alloc(u16 size, u16 headroom) __REENTRANT__ ;

#endif // _EVENT_FW_H


/** =========================================================
 *
 * Edit History
 *
 * $Source: /home/cvsrepo/Hybrii_B_OSFW_Dev/firmware/common/event_fw.h,v $
 *
 * $Log: event_fw.h,v $
 * Revision 1.1  2014/11/11 14:52:56  ranjan
 * 1.New Folder Architecture espically in /components
 * 2.Modular arrangment of functionality in new files
 *    anticipating the need for exposing them as FW App
 *    development modules
 * 3.Other improvisation in code and .h files
 *
 * Revision 1.2  2014/06/12 13:15:43  ranjan
 * -separated bcn,mgmt,um event pools
 * -fixed datapath issue due to previous checkin
 * -work in progress. neighbour cco detection
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
 * Revision 1.4  2012/05/19 05:05:15  yuanhua
 * optimized the timer handlers in CTRL and LINK layers.
 *
 * Revision 1.3  2012/03/11 17:02:24  yuanhua
 * (1) added NEK auth in AKM (2) added NMA (3) modified hpgp layer data structures (4) added Makefile for linux simulation
 *
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


