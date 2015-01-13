/* ========================================================
 *
 * @file: gv701x_task.c
 * 
 * @brief: This file contains the initialization routine and task context 
 *             in which the application needs to place its appropriate functions 
 *
 *  Copyright (C) 2010-2014, Greenvity Communications, Inc.
 *  All Rights Reserved
 *  
 * =========================================================*/

#include <string.h>
#include <rtx51tny.h>
#include "gv701x_includes.h"


/******************************************************************************
 * @fn      GV701x_TaskInit
 *
 * @brief   Initialization wrapper function where the application 
 * 		  needs to place its initialization function	
 *
 * @param   appQueues  - Firmware passes a refernce to the Tx Rx Data/event/response 
 * 				   queues that the application would use
 *
 * @return  none
 */

void GV701x_TaskInit(gv701x_aps_queue_t* appQueues)
{
	UartApp_Init(appQueues);
}

/******************************************************************************
 * @fn      GV701x_Task
 *
 * @brief   Task body in which application places any perodic or polling routines
 *
 * @param  none
 *
 * @return  none
 *
 * @note here HYBRII_TASK_ID_APP is the task id. It SHOULD NOT be changed
 */

void GV701x_Task(void) _task_ APP_TASK_ID
{
    while (1) {

		/*Wait until an event is to be serviced*/
		os_switch_task();
		/* Place the all Apllication Task routines here*/
		UartApp_Poll();
    }
}

