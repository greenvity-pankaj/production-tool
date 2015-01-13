/** ========================================================
 *
 *  @file hybrii_tasks.h
 * 
 *  @brief Hybrii Tasks definition
 *
 *  Copyright (C) 2012, Greenvity Communications, Inc.
 *  All Rights Reserved
 *  
 * =========================================================*/

#ifndef  _HYBRII_TASKS_H
#define  _HYBRII_TASKS_H

// Required by STM (System Timer Module)
enum hpLayer
{
    HP_LAYER_TYPE_HA,
    HP_LAYER_TYPE_MUX,
    HP_LAYER_TYPE_LINK,
    HP_LAYER_TYPE_CTRL,
    HP_LAYER_TYPE_NMA,
    ZB_LAYER_TYPE_MAC,
	HP_LAYER_TYPE_APP,  //Application layer

#ifdef AUTO_PING
    HP_LAYER_TYPE_HTM,  // UI layer need during auto ping test
#endif
};
// HYBRII_TASK_ID_INI must have ID of 0. This task will be
// called 1st
#define HYBRII_TASK_ID_INIT     0
//#define HYBRII_TASK_ID_STM      1
//#define HYBRII_TASK_ID_UI  1
#ifndef RTX51_TINY_OS
 #define HYBRII_TASK_ID_ISM_POLL 10
#endif
#if defined(HYBRII_HPGP) && defined(HYBRII_ZIGBEE)
#ifdef ZIGBEE_HAL_TASK 
 #define HPGP_TASK_ID_CTRL  2
 #define HYBRII_TASK_ID_FRAME  3
 #define ZIGBEE_TASK_ID_MAC 4
 #define ZIGBEE_TASK_ID_HAL 5
 #define HYBRII_TASK_ID_UI       1
 
#else
 #define HPGP_TASK_ID_CTRL  2
 #define HYBRII_TASK_ID_FRAME  3
 #define ZIGBEE_TASK_ID_MAC 4
 #define HYBRII_TASK_ID_UI       1
 
 
#endif

#else // else of defined(HYBRII_HPGP) && defined(HYBRII_ZIGBEE)

#ifdef HYBRII_ZIGBEE
 #ifdef ZIGBEE_HAL_TASK
  #define ZIGBEE_TASK_ID_MAC 2
  #define ZIGBEE_TASK_ID_HAL 3
  #define HYBRII_TASK_ID_FRAME  4
  #define HYBRII_TASK_ID_UI       1

 #else
  #define ZIGBEE_TASK_ID_MAC 2
  #define HYBRII_TASK_ID_FRAME  3
  #define HYBRII_TASK_ID_UI       1
 #endif
#else // else of HYBRII_ZIGBEE
#if defined(RTX51_TINY_OS)		 
	#define HPGP_TASK_ID_CTRL  		1
	#define HYBRII_TASK_ID_FRAME  	2 
#ifdef HPGP_HAL_TEST	
    #define HYBRII_TASK_ID_UI       3
#endif
#ifdef NO_HOST
    #define HYBRII_TASK_ID_APP  	3

#endif

 #endif//RTX51_TINY_OS
 
 #endif //end of HYBRII_ZIGBEE
 
 
 #endif //end of defined(HYBRII_HPGP) && defined(HYBRII_ZIGBEE)


/* task priority */
#define HPGP_TASK_PRI_ISM       SCHED_PRIORITY_0
#define HPGP_TASK_PRI_MUX       SCHED_PRIORITY_1
#define HPGP_TASK_PRI_LINK      SCHED_PRIORITY_2
#define HPGP_TASK_PRI_CTRL      SCHED_PRIORITY_3
#define HPGP_TASK_PRI_NMA       SCHED_PRIORITY_4

#endif


/** =========================================================
 *
 * Edit History
 *
 * $Source: /home/cvsrepo/Hybrii_B_OSFW_Dev/firmware/common/hybrii_tasks.h,v $
 *
 * $Log: hybrii_tasks.h,v $
 * Revision 1.6  2014/06/11 13:17:46  kiran
 * UART as host interface and peripheral interface supported.
 *
 * Revision 1.5  2014/06/05 10:26:07  prashant
 * Host Interface selection isue fix, Ac sync issue fix
 *
 * Revision 1.4  2014/05/28 10:58:58  prashant
 * SDK folder structure changes, Uart changes, removed htm (UI) task
 * Varified - UM, LM - iperf (UDP/TCP), overnight test pass
 *
 * Revision 1.3  2014/02/12 11:45:16  prashant
 * Performance improvement fixes
 *
 * Revision 1.2  2014/01/10 17:02:18  yiming
 * check in Rajan 1/8/2014 code release
 *
 * Revision 1.6  2014/01/08 10:53:53  ranjan
 * Changes for LM OS support.
 * New Datapath FrameTask
 * LM and UM  datapath, feature verified.
 *
 * known issues : performance numbers needs revisit
 *
 * review : pending.
 *
 * Revision 1.1  2013/12/18 17:03:14  yiming
 * no message
 *
 * Revision 1.1  2013/12/17 21:42:26  yiming
 * no message
 *
 * Revision 1.4  2013/09/04 14:43:30  yiming
 * New changes for Hybrii_A code merge
 *
 * Revision 1.8  2013/04/09 21:21:52  son
 * Zigbee &  PLC (Lower MAC) initial commit
 *
 * Revision 1.7  2012/11/22 09:44:02  prashant
 * Code change for auto ping test, sending tei map ind out, random mac addrr generation.
 *
 * Revision 1.6  2012/07/19 20:47:13  son
 * Added UI Task ID for Zigbee
 *
 * Revision 1.5  2012/07/19 00:48:07  son
 * Change HTM task name to UI
 *
 * Revision 1.3  2012/07/14 04:09:32  kripa
 * Adding HTM Task ID.
 * Committed on the Free edition of March Hare Software CVSNT Client.
 * Upgrade to CVS Suite for more features and support:
 * http://march-hare.com/cvsnt/
 *
 * Revision 1.2  2012/07/12 22:05:55  son
 * Moved ISM Polling to ISM Task.
 * UI is now part of init task
 *
 * Revision 1.1  2012/07/10 19:20:45  son
 * Defined task ID's for hybrii fw
 *
 *
 * =========================================================*/

