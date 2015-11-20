/** ========================================================
 *
 *  @file sched.c 
 * 
 *  @brief Scheduler. This scheduler schedules the execution of software 
 *      entity according to its proiority. It performs preemption at the 
 *      level of event, instead of time slot. 
 *
 *  Copyright (C) 2010-2012, Greenvity Communications, Inc.
 *  All Rights Reserved
 *  
 * ==========================================================*/
#ifdef RTX51_TINY_OS
#include <rtx51tny.h>
#endif
#include "fm.h"
#include "sched.h"
#include <string.h>
#ifndef CALLBACK
#include "hpgpdef.h"
#include "hpgpapi.h"
#endif
#include "nma.h"
#include "nma_fw.h"

static sSched    SchedObj;
static sSched   *sched;

eStatus SCHED_Sched(sExecTask *task) __REENTRANT__
{
#ifndef RTX51_TINY_OS
    if (task->pri >= SCHED_PRIORITY_MAX)
    {
        return STATUS_FAILURE;
    }
//FM_Printf(FM_ERROR, "SCHED: enter sched pri %d (state: %d)\n", task->pri, task->state);
#ifdef P8051
    IRQ_DISABLE_INTERRUPT();
#else
    SEM_WAIT(&sched->schedSem);
#endif
    
    if (task->state != TASK_STATE_SCHED)
    {
        task->state = TASK_STATE_SCHED;
        SLIST_Put(&sched->priQ[task->pri], &task->link);
        sched->priQLen[task->pri]++;
        sched->priBitMap |= (1U << task->pri);
//FM_Printf(FM_ERROR, "SCHED: schedule the task %d, mask 0x%02x.\n", task->pri, sched->priBitMap);
    }
#ifdef P8051
    IRQ_ENABLE_INTERRUPT();
#else
    SEM_POST(&sched->schedSem);
#endif
#endif // #ifndef RTX51_TINY_OS
    return STATUS_SUCCESS;
}

#ifndef CALLBACK
u8 SCHED_ProcTask(sExecTask *task)
{
    u8 ret = 0;
#ifndef RTX51_TINY_OS
    switch(task->id)
    {
#ifdef HPGP_HAL
        case HYBRII_TASK_ID_ISM_POLL:
        {
#ifdef HAL_INT
            ret = HAL_RecvMacFrame(task->cookie);
#else
            ret = HAL_RxPoll(task->cookie);
#endif
            break;
        }
#endif /* HPGP_HAL */
        case HPGP_TASK_ID_MUX:
        {
            ret = MUXL_Proc(task->cookie);
            break;
        }
        case HPGP_TASK_ID_LINK:
        {
            ret = LINKL_Proc(task->cookie);
            break;
        }
        case HPGP_TASK_ID_CTRL:
        {
            ret = CTRLL_Proc(task->cookie);
            break;
        }
#ifdef NMA
        case HPGP_TASK_ID_NMA:
        {
            ret = NMA_Proc(task->cookie);
            break;
        }
#endif
        default:
        {
            ret = FALSE;
        }
    
    }
#endif // #ifndef RTX51_TINY_OS
    return ret;
}
#endif /* CALLBACK */


void SCHED_Proc()
{
#ifndef RTX51_TINY_OS
    u8        i = 0;
    u8        qLen = 0;
    sSlink    *link = NULL;
    sExecTask *task = NULL;

    while(1)
    {
        qLen = sched->priQLen[i];
//      while (!SLIST_IsEmpty(&sched->priQ[i]))
        while (qLen)
        {
#ifdef P8051
            IRQ_DISABLE_INTERRUPT();
#else
            SEM_WAIT(&sched->schedSem);
#endif
            link = SLIST_Pop(&sched->priQ[i]); 
            sched->priQLen[i]--;
#ifdef P8051
            IRQ_ENABLE_INTERRUPT();
#else
            SEM_POST(&sched->schedSem);
#endif
            task = SLIST_GetEntry(link, sExecTask, link);

#ifdef CALLBACK
            if ((task->exec) && (task->state == TASK_STATE_SCHED))
#else
            if (task->state == TASK_STATE_SCHED)
#endif
            {
#ifdef P8051
                IRQ_DISABLE_INTERRUPT();
#else
                SEM_WAIT(&sched->schedSem);
#endif
                task->state = TASK_STATE_IDLE;
#ifdef P8051
                IRQ_ENABLE_INTERRUPT();
#else
                SEM_POST(&sched->schedSem);
#endif
//FM_Printf(FM_ERROR, "SCHED: execute the task %d.\n", task->pri);
    
#ifdef CALLBACK
                if(task->exec(task->cookie))
#else
                if (SCHED_ProcTask(task))
#endif
                {
                    /* more to be executed */ 
                    SCHED_Sched(task);
                    i = task->highPri;
                    goto nextq;
                }
            }
            qLen--;
        }

        sched->priBitMap &= ~(1L << i);
        i++;
nextq:
        if (i >= SCHED_PRIORITY_MAX )
        {
            i = 0;
        }
    }  
#endif
}


u8 SCHED_IsPreempted(sExecTask *task) 
{
#ifndef RTX51_TINY_OS
    u8 i; 
    for (i = 0; i < task->pri; i++)
    {
        if ((sched->priBitMap) & (1L << i))
            break;
    }
    task->highPri = i;
//FM_Printf(FM_ERROR, "SCHED: current task: %d, higher task %d, %d, mask 0x%02x.\n", task->pri, task->highPri, i, sched->priBitMap);
    return (i != task->pri);
#else
    return (FALSE);
#endif  // #ifndef RTX51_TINY_OS
}


void SCHED_InitTask(sExecTask *task, u8 id, s8 *name, u8 pri, 
                    u8 (*exec)(void * cookie), void* cookie)
{
#ifndef RTX51_TINY_OS
    memset(task, 0, sizeof(sExecTask));
    memcpy(task->name, name, TASK_NAME_MAX-1);
    task->name[TASK_NAME_MAX-1] = '\0';
    task->pri = pri;
    task->id = id;
    task->exec = exec;
    task->cookie = cookie;
#endif
}
    


eStatus SCHED_Init()
{
    eStatus status = STATUS_SUCCESS;
#ifndef RTX51_TINY_OS
    u8 i;
    sched = &SchedObj;
    memset(sched, 0, sizeof(SchedObj));
    for (i = 0; i < SCHED_PRIORITY_MAX; i++)
    {
        SLIST_Init(&sched->priQ[i]);
        sched->priQLen[i] = 0;
    }
#ifndef P8051
#if defined(WIN32) || defined(_WIN32)
    sched->schedSem = CreateSemaphore(
        NULL,           // default security attributes
        SEM_COUNT,      // initial count
        SEM_COUNT,      // maximum count
        NULL);          // unnamed semaphore
    if(sched->schedSem == NULL)
#else
    if(sem_init(&sched->schedSem, 0, SEM_COUNT))
#endif
    {
        status = STATUS_FAILURE;
    }
#endif
#endif
    return status;

}


/** =========================================================
 *
 * Edit History
 *
 * $Source: /home/cvsrepo/Hybrii_B_OSFW_Dev/firmware/common/sched.c,v $
 *
 * $Log: sched.c,v $
 * Revision 1.2  2014/11/11 14:52:56  ranjan
 * 1.New Folder Architecture espically in /components
 * 2.Modular arrangment of functionality in new files
 *    anticipating the need for exposing them as FW App
 *    development modules
 * 3.Other improvisation in code and .h files
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
 * Revision 1.10  2012/07/19 21:46:07  son
 * Prepared files for zigbee integration
 *
 * Revision 1.8  2012/06/05 22:37:11  son
 * UART console does not get initialized due to task ID changed
 *
 * Revision 1.7  2012/06/05 07:25:58  yuanhua
 * (1) add a scan call to the MAC during the network discovery. (2) add a tiny task in ISM for interrupt polling (3) modify the frame receiving integration. (4) modify the tiny task in STM.
 *
 * Revision 1.6  2012/06/04 23:22:41  son
 * Added RTX51 OS support
 *
 * Revision 1.5  2012/06/02 19:18:13  yuanhua
 * fixed an issue in the scheduler. Now the higher priority task will render its cpu resource to the lower priority task if the higher priority task is in the polling mode.
 *
 * Revision 1.4  2012/05/17 05:05:58  yuanhua
 * (1) added the option for timer w/o callback (2) added task id and name.
 *
 * Revision 1.3  2012/05/14 05:22:28  yuanhua
 * support the SCHED without using callback functions.
 *
 * Revision 1.2  2012/04/15 20:35:09  yuanhua
 * integrated beacon RX changes in HAL and added HTM for on board test.
 *
 * ========================================================== */  
