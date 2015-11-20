/** ========================================================
 *
 *  @file sched.h 
 * 
 *  @brief Scheduler. This scheduler schedules the execution of software 
 *      modules according to the proiority. It performs the preempt at the 
 *      level of event, instead of time slot. 
 *
 *  Copyright (C) 2010-2012, Greenvity Communications, Inc.
 *  All Rights Reserved
 *  
 * ==========================================================*/
#ifndef _SCHED_H
#define _SCHED_H

#include "papdef.h"
#include "list.h"

#define SCHED_INVALID_ID     0xFF
#define SCHED_PROC_MAX       8

#define TASK_STATE_IDLE      0
#define TASK_STATE_SCHED     1

enum
{
    SCHED_PRIORITY_0,
    SCHED_PRIORITY_1,
    SCHED_PRIORITY_2,
    SCHED_PRIORITY_3,
    SCHED_PRIORITY_4,
    SCHED_PRIORITY_5,
    SCHED_PRIORITY_6,
    SCHED_PRIORITY_7,
    SCHED_PRIORITY_MAX,
};


#define TASK_NAME_MAX     8

typedef struct execTask
{
    sSlink link;
    u8     name[TASK_NAME_MAX];
    u8     id;
    u8     state   : 2;
    u8     pri     : 6;
    u8     rsvd    : 2;
    u8     highPri : 6;
    u8     (*exec)(void * cookie);
    void  *cookie;
} sExecTask, *psExecTask;


typedef struct sched
{
    u8          priBitMap;
    sSlist      priQ[SCHED_PRIORITY_MAX];    /* priority queue */
    u8          priQLen[SCHED_PRIORITY_MAX]; /* size of priority queue */

#ifndef P8051
#if defined(WIN32) || defined(_WIN32)
    HANDLE      schedSem;
#else //POSIX
    sem_t       schedSem;
#endif
#endif

} sSched, *psSched;


void    SCHED_InitTask(sExecTask *task, u8 id, s8 *name, u8 pri,
                       u8 (*exec)(void * cookie), void* cookie);

eStatus SCHED_Sched(sExecTask *execTask) __REENTRANT__;
u8      SCHED_IsPreempted(sExecTask *task);

eStatus SCHED_Init();
void    SCHED_Proc();


#endif


/** =========================================================
 *
 * Edit History
 *
 * $Source: /home/cvsrepo/Hybrii_B_OSFW_Dev/firmware/common/sched.h,v $
 *
 * $Log: sched.h,v $
 * Revision 1.1  2013/12/18 17:03:14  yiming
 * no message
 *
 * Revision 1.1  2013/12/17 21:42:26  yiming
 * no message
 *
 * Revision 1.2  2013/01/24 00:13:46  yiming
 * Use 01-23-2013 Hybrii-A code as first Hybrii-B code base
 *
 * Revision 1.6  2012/06/02 19:18:13  yuanhua
 * fixed an issue in the scheduler. Now the higher priority task will render its cpu resource to the lower priority task if the higher priority task is in the polling mode.
 *
 * Revision 1.5  2012/05/17 05:05:58  yuanhua
 * (1) added the option for timer w/o callback (2) added task id and name.
 *
 * Revision 1.4  2012/05/14 05:22:28  yuanhua
 * support the SCHED without using callback functions.
 *
 * Revision 1.3  2012/05/12 04:11:46  yuanhua
 * (1) added list.h (2) changed the hal tx for the hw MAC implementation.
 *
 * Revision 1.2  2012/04/15 20:35:09  yuanhua
 * integrated beacon RX changes in HAL and added HTM for on board test.
 *
 * ========================================================== */ 

