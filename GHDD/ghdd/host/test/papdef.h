/** ========================================================
 *
 *  @file papdef.h
 *
 *  @brief Basic data type definition 
 *
 *  Copyright (C) 2010-2011, Greenvity Communications, Inc.
 *  All Rights Reserved
 *
 * =========================================================*/

#ifndef _PAPTYPE_H
#define _PAPTYPE_H

#include <stddef.h>


#define ERROR 						-1
#define SUCCESS 					0
//#define MODULE						1

#ifdef SIMU
#if defined(WIN32) || defined(_WIN32)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#else
#include <semaphore.h>
#endif
#endif //end of SIMU

#ifdef MODULE
#else
/** Basic data type */
typedef unsigned char    u8 , uint8_t, bool;
typedef unsigned short   u16, uint16_t;
typedef unsigned long    u32;

typedef struct uint64_s
{
    u32 hi_u32;
    u32 lo_u32;
} u64;
#endif

#ifndef __GNUC__
#define __PACKED__
#endif
#ifndef __PACKED__
#define __PACKED__    __attribute__((packed))
#endif

#ifdef P8051
#define __XDATA__    xdata
#ifdef SDDC
#define __CRIT_SECTION_BEGIN__    (__critical {)
#define __CRIT_SECTION_END__      (}           )
#define __INTERRUPT2__      __interrupt(2)
#else 
#define __INTERRUPT2__  interrupt 2 using 3	  //Keil C251
#define __CRIT_SECTION_BEGIN__    (EA = 0)
#define __CRIT_SECTION_END__      (EA = 1) 
#endif
#define SEM_WAIT(sem)   
#define SEM_POST(sem)  

     

#else
#define __XDATA__ 
#define __CRIT_SECTION_BEGIN__    
#define __CRIT_SECTION_END__      
#define __INTERRUPT2__    

#if defined(WIN32) || defined(_WIN32)
#define SEM_WAIT(sem)   ( WaitForSingleObject(sem, INFINITE) )
#define SEM_POST(sem)   ( ReleaseSemaphore(sem, 1, NULL) )
#else //POSIX
#ifdef MODULE
#ifdef _PLATFORM_LINUX_IMX233_
#define SEM_WAIT(sem)   //( spin_lock(sem)  )
#define SEM_POST(sem)   //( spin_unlock(sem) ) 
#else
#define SEM_WAIT(sem)   ( spin_lock(sem)  )
#define SEM_POST(sem)   ( spin_unlock(sem) ) 
#endif
#else
#define SEM_WAIT(sem)   ( sem_wait(sem)  )
#define SEM_POST(sem)   ( sem_post(sem) )   
#endif
#endif

#endif //P8051

#ifndef BIT
#define BIT(x)    ( 1L << (x) )
#endif


#ifndef MIN
/** Macro to get minimun number */
#define MIN(a, b)       ((a) < (b) ? (a) : (b))
#endif /* MIN */

#ifndef MAX
/** Macro to get maximun number */
#define MAX(a,b)        ((a) > (b) ? (a) : (b))
#endif

#ifndef NELEMENTS
/** Number of elements in aray x */
#define NELEMENTS(x)    (sizeof(x)/sizeof(x[0]))
#endif

#ifndef TRUE
#define TRUE     1
#endif

#ifndef FALSE
#define FALSE    0
#endif

#ifndef true
#define true     1
#endif

#ifndef false
#define false    0
#endif
#define MAC_ADDR_LEN    6
#define VLAN_TAG_LEN    4

#define SEM_COUNT       1


typedef enum status
{
	STATUS_SUCCESS,
	STATUS_FAILURE
} eStatus;

//#define GV_DRV_DBG
#ifdef GV_DRV_DBG
#define CDBG(msg, args...) do { 				\
		printk(KERN_INFO msg, ##args );			\
}while (0)
#else
#define CDBG(msg, args...)
#endif





//#define HAVE_NET_DEVICE_OPS


#endif /* _PAPTYPE_H */


/** =========================================================
 *
 * Edit History
 *
 * $Source: /home/cvsrepo/hybrii/firmware/common/papdef.h,v $
 *
 * $Log: papdef.h,v $
 * Revision 1.4  2011/12/05 22:09:45  son
 * Added more generic typedef's
 *
 * Revision 1.3  2011/09/14 05:52:36  yuanhua
 * Made Keil C251 compilation.
 *
 * Revision 1.2  2011/09/09 07:02:31  yuanhua
 * migrate the firmware code from the greenchip to the hybrii.
 *
 * Revision 1.3  2011/07/22 18:51:04  yuanhua
 * (1) added the hardware timer tick simulation (hts.h/hts.c) (2) Added semaphors for sync in simulation and defined macro for critical section (3) Fixed some bugs in list.h and CRM (4) Modified and fixed bugs in SHAL (5) Changed SNSM/CNSM and SNAM/CNAM for bug fix (6) Added debugging prints, and more.
 *
 * Revision 1.2  2011/06/24 14:33:18  yuanhua
 * (1) Changed event structure (2) Implemented SNSM, including the state machines in network discovery and connection states, becaon process, discover process, and handover detection (3) Integrated the HPGP and SHAL
 *
 * Revision 1.1  2011/05/06 18:31:47  kripa
 * Adding common utils and isr files for Greenchip firmware.
 *
 * Revision 1.2  2011/04/23 16:57:55  kripa
 * Added empty #define for __PACKED__ if compiler is not GCC.
 *
 * Revision 1.1  2011/04/08 21:40:42  yuanhua
 * Framework
 *
 *
 * =========================================================*/


