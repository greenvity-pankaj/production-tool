/** =========================================================
 *
 *  @file green.h
 * 
 *  @brief GREEN MAIN Module
 *
 *  Copyright (C) 2010-2011, Greenvity Communications, Inc.
 *  All Rights Reserved
 *  
 * ==========================================================*/


#ifndef  _GREEN_H
#define  _GREEN_H

#ifdef SIMU
#include "hts.h"
#endif
#ifndef HPGP_HAL
//#include "sdrv.h"
#endif
#include "hpgpdef.h"
#ifdef HPGP_TEST
#include "htm.h"
#endif
#include "hal.h"
#include "hpgpctrl.h"
#include "nma.h"


typedef struct homeplugCb
{
#ifdef SIMU
    sHts       hts;
#endif
#ifdef HPGP_TEST
    sHtm       htm;
#endif

    /* network management agent */
    sNma       netMgmtAgt;
    /* hpgp control plane */
    sHpgpCtrl  hpgpCtrl; 
    /* hpgp data plane */

    /* hardware abstract layer */
    sHaLayer   haLayer; 

} sHomePlugCb, *psHomePlugCb;



#endif


/** =========================================================
 *
 * Edit History
 *
 * $Source: /home/cvsrepo/Hybrii_B_OSFW_Dev/firmware/hpgp/src/green.h,v $
 *
 * $Log: green.h,v $
 * Revision 1.2  2014/11/11 14:52:58  ranjan
 * 1.New Folder Architecture espically in /components
 * 2.Modular arrangment of functionality in new files
 *    anticipating the need for exposing them as FW App
 *    development modules
 * 3.Other improvisation in code and .h files
 *
 * Revision 1.1  2013/12/18 17:04:24  yiming
 * no message
 *
 * Revision 1.1  2013/12/17 21:45:54  yiming
 * no message
 *
 * Revision 1.4  2013/09/04 14:49:33  yiming
 * New changes for Hybrii_A code merge
 *
 * Revision 1.6  2012/10/11 06:21:00  ranjan
 * ChangeLog:
 * 1. Added HPGP_MAC_SAP to support linux host data and command path.
 *     define HPGP_MAC_SAP, NMA needs to be added in project.
 *
 * 2. Added 'p ping' command in htm.c . Feature is under AUTO_PING macro.
 *
 * 3. Extended  'p key' command to include PPEK support.
 *
 * verified :
 *   1. Datapath ping works overnite after association,auth
 *   2. HAL TEST project is intact
 *
 * Revision 1.5  2012/04/15 20:35:09  yuanhua
 * integrated beacon RX changes in HAL and added HTM for on board test.
 *
 * Revision 1.4  2012/04/13 06:15:11  yuanhua
 * integrate the HPGP protocol stack with the HAL for the beacon TX/RX and mgmt msg RX.
 *
 * Revision 1.3  2012/03/11 17:02:24  yuanhua
 * (1) added NEK auth in AKM (2) added NMA (3) modified hpgp layer data structures (4) added Makefile for linux simulation
 *
 * Revision 1.2  2011/09/09 07:02:31  yuanhua
 * migrate the firmware code from the greenchip to the hybrii.
 *
 * Revision 1.2  2011/07/22 18:51:04  yuanhua
 * (1) added the hardware timer tick simulation (hts.h/hts.c) (2) Added semaphors for sync in simulation and defined macro for critical section (3) Fixed some bugs in list.h and CRM (4) Modified and fixed bugs in SHAL (5) Changed SNSM/CNSM and SNAM/CNAM for bug fix (6) Added debugging prints, and more.
 *
 * Revision 1.1  2011/06/23 23:52:42  yuanhua
 * move green.h green.c hpgpapi.h hpgpdef.h hpgpconf.h to src directory
 *
 * Revision 1.1  2011/05/06 19:14:50  kripa
 * Adding nmp files to the new source tree.
 *
 * Revision 1.1  2011/04/08 21:40:41  yuanhua
 * Framework
 *
 *
 * =========================================================*/

