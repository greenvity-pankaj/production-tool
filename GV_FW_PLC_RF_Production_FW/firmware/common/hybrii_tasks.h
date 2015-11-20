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
};
// HYBRII_TASK_ID_INI must have ID of 0. This task will be
// called 1st
#define HYBRII_TASK_ID_INIT     0
//#define HYBRII_TASK_ID_STM      1
#ifndef RTX51_TINY_OS
#define HYBRII_TASK_ID_ISM_POLL 10
#endif
#if defined(HYBRII_HPGP) && defined(HYBRII_802154)
#ifdef HAL_802154_TASK 
 #define HPGP_TASK_ID_CTRL  	1
 #define HYBRII_TASK_ID_FRAME   2
 #define MAC_802154_TASK_ID 	3
 #define HAL_802154_TASK_ID 	4
 #define HYBRII_TASK_ID_UI      5 
#else
#ifdef PROD_TEST
 #define HPGP_TASK_ID_CTRL  	1
 #define HYBRII_TASK_ID_FRAME   2
#else
//#error "i am here"
 #define HYBRII_TASK_ID_FRAME 1
#endif
 
#ifndef MAC_802154_TASK 
#ifdef NO_HOST
 #define HYBRII_TASK_ID_APP  	3
#endif
#else
#ifdef PROD_TEST
 #define MAC_802154_TASK_ID	    3
#else
 #define MAC_802154_TASK_ID		2
#endif
#endif 

#ifdef PROD_TEST
 #define HYBRII_TASK_ID_UI		4
#else
 #define HYBRII_TASK_ID_UI	    3//migrated to frame task
#endif

#endif

#else // else of defined(HYBRII_HPGP) && defined(HYBRII_802154)

#ifdef HYBRII_802154
#ifdef HAL_802154_TASK
	#define MAC_802154_TASK_ID    1
  	#define HAL_802154_TASK_ID    2
  	#define HYBRII_TASK_ID_FRAME  3
  	#define HYBRII_TASK_ID_UI     4   
#else
	#define MAC_802154_TASK_ID    1
	#define HYBRII_TASK_ID_FRAME  2
	#define HYBRII_TASK_ID_UI     3	 
#endif
#else // else of HYBRII_802154
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
 
 #endif //end of HYBRII_802154
 
 
 #endif //end of defined(HYBRII_HPGP) && defined(HYBRII_802154)


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
 * Revision 1.8  2015/01/02 14:55:35  kiran
 * 1) Timer Leak fixed while freeing SCB fixed
 * 2) Software broadcast supported for LG
 * 3) UART Loopback supported for LG
 * 4) Keep Alive feature to ageout defunctional STA
 * 5) Improved flash API's for NO Host Solution
 * 6) Imporved PLC Hang recovery mechanism
 * 7) Reduced nested call tree of common path functions
 * 8) Code optimization and cleanup (unused arguments, unused local variables)
 * 9) Work around for UART hardware interrupt issues (unintended interrupts and no interrupts)
 * 10) Use of memory specific pointers instead of generic pointers
 *
 * Revision 1.7  2014/11/11 14:52:56  ranjan
 * 1.New Folder Architecture espically in /components
 * 2.Modular arrangment of functionality in new files
 *    anticipating the need for exposing them as FW App
 *    development modules
 * 3.Other improvisation in code and .h files
 *
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

