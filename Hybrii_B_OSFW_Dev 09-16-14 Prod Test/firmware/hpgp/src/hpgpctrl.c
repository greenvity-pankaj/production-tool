/** =========================================================
 *
 *  @file hpgpctrl.c
 * 
 *  @brief HPGP Control Plane
 *
 *  Copyright (C) 2010-2012, Greenvity Communications, Inc.
 *  All Rights Reserved
 *  
 * ==========================================================*/
#include "papdef.h"
#include "hpgpdef.h"
#ifdef ROUTE
#include "hpgp_route.h"
#endif
#include "hpgpctrl.h"
#include "hpgpapi.h"
#include "hybrii_tasks.h"

static sHpgpCtrl *gHpgpCtrl = NULL;

void HPGPCTRL_Init(sHpgpCtrl *hpgpCtrl)
{
    gHpgpCtrl = hpgpCtrl;
    //Initialize MUX
    MUXL_Init(&hpgpCtrl->muxLayer);
    //Initialize Link Layer
    LINKL_Init(&hpgpCtrl->linkLayer);
    //Initialize Control Layer
    CTRLL_Init(&hpgpCtrl->ctrlLayer);
}

#if 0
void HPGPCTRL_Proc(sHpgpCtrl *hpgpCtrl)
{
      //Mux Layer
      MUXL_Proc(&hpgpCtrl->muxLayer);
      //Link Layer
      LINKL_Proc(&hpgpCtrl->linkLayer);
      //Control Layer
      CTRLL_Proc(&hpgpCtrl->ctrlLayer);
}
#endif


void* HPGPCTRL_GetLayer(u8 layer)
{
    switch(layer)
    {
#if 0
        case HP_LAYER_TYPE_HA:
#ifdef SIMU
            return (void*) &(gHpgpCtrl->simuHal);
#else
            return (void*) &(gHpgpCtrl->haLayer);
#endif
#endif
        case HP_LAYER_TYPE_MUX:
            return (void*) &(gHpgpCtrl->muxLayer);
        case HP_LAYER_TYPE_LINK:
            return (void*) &(gHpgpCtrl->linkLayer);
        case HP_LAYER_TYPE_CTRL:
            return (void*) &(gHpgpCtrl->ctrlLayer);
        default:
            return NULL;
    }
}


/** =========================================================
 *
 * Edit History
 *
 * $Source: /home/cvsrepo/Hybrii_B_OSFW_Dev/firmware/hpgp/src/hpgpctrl.c,v $
 *
 * $Log: hpgpctrl.c,v $
 * Revision 1.4  2014/05/28 10:58:59  prashant
 * SDK folder structure changes, Uart changes, removed htm (UI) task
 * Varified - UM, LM - iperf (UDP/TCP), overnight test pass
 *
 * Revision 1.3  2014/05/12 08:09:57  prashant
 * Route fix, STA sync issue with AV CCO and handling discover bcn issue fix
 *
 * Revision 1.2  2014/02/27 10:42:47  prashant
 * Routing code added
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
 * Revision 1.2  2012/04/13 06:15:11  yuanhua
 * integrate the HPGP protocol stack with the HAL for the beacon TX/RX and mgmt msg RX.
 *
 * Revision 1.1  2012/03/11 17:02:24  yuanhua
 * (1) added NEK auth in AKM (2) added NMA (3) modified hpgp layer data structures (4) added Makefile for linux simulation
 *
 *
 * =========================================================*/

