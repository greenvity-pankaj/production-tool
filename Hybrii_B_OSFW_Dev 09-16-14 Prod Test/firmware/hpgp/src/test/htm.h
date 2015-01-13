/** ========================================================
 *
 *  @file htm.h
 * 
 *  @brief HomePlug GREEN PHY Data Structure Definition
 *
 *  Copyright (C) 2010-2012, Greenvity Communications, Inc.
 *  All Rights Reserved
 *  
 * ==========================================================*/


#ifndef _HTM_H
#define _HTM_H

#ifdef P8051
#include "papdef.h"
#else
#include <sys/types.h>
#endif
#include "hal_tst.h"

typedef struct htm
{
    int    opt;
}sHtm, *psHtm;


eStatus HTM_Init(sHtm *htm);
void HTM_Proc(sHtm *htm);

#ifdef AUTO_PING
u8 HHAL_RcvPing(sEth2Hdr *pEthHdr, u8 stei);
#endif
u8 linkStatus_frameRx(sEth2Hdr *pEthHdr);
#ifdef PLC_TEST
extern void HHT_SimulateTx(sPlcSimTxTestParams* pTestParams);
#endif    

#endif


/** =========================================================
 *
 * Edit History
 *
 * $Source: /home/cvsrepo/Hybrii_B_OSFW_Dev/firmware/hpgp/src/test/htm.h,v $
 *
 * $Log: htm.h,v $
 * Revision 1.2  2014/01/13 20:57:35  son
 * Added new project for plc +  zigbee code integration
 *
 * Revision 1.1  2013/12/18 17:05:59  yiming
 * no message
 *
 * Revision 1.1  2013/12/17 22:20:58  yiming
 * no message
 *
 * Revision 1.2  2013/09/04 14:49:56  yiming
 * New changes for Hybrii_A code merge
 *
 * Revision 1.5  2013/05/16 08:38:41  prashant
 * "p starttest" command merged in upper mac
 * Dignostic mode added in upper mac
 *
 * Revision 1.4  2013/04/04 12:21:54  prashant
 * Detecting PLC link failure for HMC. added project for HMC and Renesas
 *
 * Revision 1.3  2012/10/11 06:21:01  ranjan
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
 * Revision 1.2  2012/05/01 18:06:49  son
 * Fixed compilatoin issues
 *
 * Revision 1.1  2012/04/15 20:35:09  yuanhua
 * integrated beacon RX changes in HAL and added HTM for on board test.
 *
 *
 *  ========================================================= */
