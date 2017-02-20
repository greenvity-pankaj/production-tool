/** =========================================================
 * 
 *  @file chom.h
 * 
 *  @brief Handover Manager 
 *
 *  Copyright (C) 2010-2011, Greenvity Communications, Inc.
 *  All Rights Reserved
 *  
 * ==========================================================*/


#ifndef  _HOM_H
#define  _HOM_H


#include "hpgpevt.h"
#include "papdef.h"
#include "timer.h"
struct linkLayer;
enum chomState
{
    CHOM_STATE_INIT,
    CHOM_STATE_READY,
};

//CCO handover manager
typedef struct chom
{
    struct linkLayer *linkl;

    u8 state;

    u8 txRetryCnt;

}sChom, *psChom;


//STA handover manager

enum shomState
{
    SHOM_STATE_INIT,
    SHOM_STATE_IDLE,
    SHOM_STATE_WAITFOR_HO_INFO_IND,
};

typedef struct shom
{
    struct linkLayer *linkl;

    u8         state;
  
    u8         hoReady: 1;
    u8         rsvd:    7;

    //resend mgmt msg
    u8         hoResult;
    u8         txRetryCnt;

    tTimerId   hoTimer;

} sShom, *psShom;

void SHOM_ProcEvent(sShom *shom, sEvent *event);
void SHOM_Start(sShom *shom);
void SHOM_Stop(sShom *shom);
eStatus SHOM_Init(sShom *shom, struct linkLayer *linkl);




void CHOM_ProcEvent(sChom *chom, sEvent *event);

eStatus CHOM_Init(sChom *chom, struct linkLayer *linkl);

#endif


/** =========================================================
 *
 * Edit History
 *
 * $Source: /home/cvsrepo/Hybrii_B_OSFW_Dev/firmware/hpgp/src/link/hom.h,v $
 *
 * $Log: hom.h,v $
 * Revision 1.2  2014/05/28 10:58:59  prashant
 * SDK folder structure changes, Uart changes, removed htm (UI) task
 * Varified - UM, LM - iperf (UDP/TCP), overnight test pass
 *
 * Revision 1.1  2013/12/18 17:05:23  yiming
 * no message
 *
 * Revision 1.1  2013/12/17 21:47:56  yiming
 * no message
 *
 * Revision 1.4  2013/09/04 14:51:01  yiming
 * New changes for Hybrii_A code merge
 *
 * Revision 1.5  2012/04/19 16:46:30  yuanhua
 * fixed some C51 compiler errors for the integration.
 *
 * Revision 1.4  2012/04/17 23:09:50  yuanhua
 * fixed compiler errors for the hpgp hal test due to the integration changes.
 *
 * Revision 1.3  2012/04/13 06:15:11  yuanhua
 * integrate the HPGP protocol stack with the HAL for the beacon TX/RX and mgmt msg RX.
 *
 * Revision 1.2  2011/09/09 07:02:31  yuanhua
 * migrate the firmware code from the greenchip to the hybrii.
 *
 * Revision 1.4  2011/08/09 22:45:44  yuanhua
 * changed to event structure, seperating HPGP-related events from the general event defination so that the general event could be used for other purposes than the HPGP.
 *
 * Revision 1.3  2011/08/05 17:06:29  yuanhua
 * (1) added an internal queue in Link Layer for communication btw modules within Link Layer (2) Fixed bugs in CCo Handover. Now, CCo handover could be triggered by auto CCo selection, CCo handover messages work fine (3) Made some modifications in SHAL.
 *
 * Revision 1.2  2011/07/16 17:11:23  yuanhua
 * (1)Implemented SHOM and CHOM modules, including handover procedure, SCB resource updating for HO (2) Update SNAM and CNAM modules to support uer-appointed CCo handover (3) Made the SCB resources to support the TEI MAP for the STA mode and management of associated STA resources (e.g. TEI) (4) Modified SNSM and CNSM to perform all types of handover switch (CCo handover to the new STA, STA taking over the CCo, STA switching to the new CCo)
 *
 * Revision 1.1  2011/05/28 06:31:19  kripa
 * Combining corresponding STA and CCo modules.
 *
 * Revision 1.1  2011/05/06 19:10:12  kripa
 * Adding link layer files to new source tree.
 *
 * Revision 1.2  2011/04/23 19:48:45  kripa
 * Fixing stm.h and event.h inclusion, using relative paths to avoid conflict with windows system header files.
 *
 * Revision 1.1  2011/04/08 21:42:45  yuanhua
 * Framework
 *
 *
 * =========================================================*/

