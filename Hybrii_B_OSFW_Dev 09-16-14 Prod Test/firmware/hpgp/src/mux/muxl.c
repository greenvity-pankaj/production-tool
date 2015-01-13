/** =========================================================
 *
 *  @file mux.c
 * 
 *  @brief Muxtiplex Layer
 *
 *  Copyright (C) 2010-2011, Greenvity Communications, Inc.
 *  All Rights Reserved
 *  
 * ===========================================================*/
#ifdef RTX51_TINY_OS
#include <rtx51tny.h>
#endif
#include <string.h>
#include "papdef.h"
#ifdef ROUTE
#include "hpgp_route.h"
#endif
#include "green.h"
#include "fm.h"
#include "sched.h"
#include "muxl.h"
#include "hpgpdef.h"
#include "hpgpapi.h"
#include "ism.h"
#include "hpgpevt.h"

#include "hal.h"
#include "hybrii_tasks.h"

#if 0
eStatus MUXL_SendEvent(sMuxLayer *muxl, sEvent *event)
{

#ifdef P8051
__CRIT_SECTION_BEGIN__
#else
    SEM_WAIT(&muxl->muxSem);
#endif

    SLIST_Put(&muxl->eventQueue, &event->link);

#ifdef P8051
__CRIT_SECTION_END__
#else
    SEM_POST(&muxl->muxSem);
#endif
    return STATUS_SUCCESS;
}

#endif


eStatus MUXL_TransmitMgmtMsg(sMuxLayer *muxl, sEvent *event)
{
    sMmHdr    *mmh = NULL;
    sHpgpHdr *hpgpHdr = (sHpgpHdr *)event->buffDesc.buff;

    if( event->eventHdr.eventClass != EVENT_CLASS_MSG)
    {
        return STATUS_FAILURE;
    }

    /* add the mgmt msg header */
    if(event->buffDesc.buff+sizeof(sMmHdr) > event->buffDesc.dataptr)
    {
        FM_Printf(FM_ERROR,"MUXL: Data buffer is small\n");
        return STATUS_FAILURE;
    }

    if (((hpgpHdr->mnbc)&&(event->buffDesc.datalen > HPGP_MNBC_PAYLOAD_MAX)) ||
         (event->buffDesc.datalen > HPGP_DATA_PAYLOAD_MAX))
    {
        /* perform the msg fragment */
        FM_Printf(FM_ERROR,"MUXL: need msg fragmentation.\n");
    }

    event->buffDesc.dataptr -= sizeof(sMmHdr);  
    event->buffDesc.datalen += sizeof(sMmHdr);  
    mmh = (sMmHdr *) (event->buffDesc.dataptr);  
          
    //add mgmt msg header
    //fragment is not supported at present
    mmh->mmv = 0x1;
    mmh->mmtype = cpu_to_le16(event->eventHdr.type);
    mmh->nfmi = 0;
    mmh->fnmi = 0;
    mmh->fmsn = 0;

    return HAL_Transmit(HOMEPLUG_GetHal(), event);

}

eStatus MUX_InitMux(sMux *mux)
{
    SLIST_Init(&mux->rxQueue);
    mux->rxqlen = 0;
    mux->rxnfmi = 0;
    mux->rxfmsn = 0;
   
    return STATUS_SUCCESS;
}


int MUX_ResetReassembly(sMux *mux)
{
    sEvent *event = NULL;
    sSlink *slink = NULL;

    while(!SLIST_IsEmpty(&mux->rxQueue))
    {
        slink = SLIST_Pop(&mux->rxQueue);

        event = SLIST_GetEntry(slink, sEvent, link);
        EVENT_Free(event);
    }
    mux->rxqlen = 0;
    mux->rxnfmi = 0;
    mux->rxfmsn = 0;
   
    return STATUS_SUCCESS;
}



void MUX_Proc(sMux *mux, sEvent *event)
{
    sMmHdr *mmh = NULL; 

    //now it is a mgmt msg
    mmh = (sMmHdr *) (event->buffDesc.dataptr);  

//FM_Printf(FM_MUX, "MUX: process a mgmt msg (type = 0x%.2x).\n", mmh->mmtype);

    event->eventHdr.eventClass = EVENT_CLASS_MSG;
    event->eventHdr.type = le16_to_cpu(mmh->mmtype);
    //chop off the msg header
    event->buffDesc.dataptr += sizeof(sMmHdr);
    event->buffDesc.datalen -= sizeof(sMmHdr);

    if(mux->rxfmsn != mmh->fmsn)
    {
        //receive new mgmt info, reset RX  
        MUX_ResetReassembly(mux);
        mux->rxfmsn = mmh->fmsn;
        mux->rxnfmi = mmh->nfmi;
    }

    if(mux->rxnfmi != mmh->nfmi) 
    {
        //should not happen
        MUX_ResetReassembly(mux);
        EVENT_Free(event);
        FM_Printf(FM_WARN, "MUX: error in rx fragement.\n");
        return;

    }

    if(mux->rxnfmi == 0)
    {
        FM_Printf(FM_MUX, "MUX: deliver a mgmt msg.\n");
        //no fragment and deliver right away
#ifdef CALLBACK
        mux->deliverMgmtMsg(mux->mgmtcookie, event); 
#else
        LINKL_RecvMgmtMsg(mux->mgmtcookie, event);
#endif
    }
    else
    {
//        event->eventHdr.fnmi = mmh->fnmi; //??
        //check if it is the first fragment 

//        mux->fnmi = mmh->fnmi; //??
       
        //place the rx fragment into the rx queue in order
        //At present it is assumed that the fragments are 
        //transmitted in order of fragment number. Thus, simply
        //place the fragment at the tail.
        //Otherwise, a general approach is used.
        SLIST_Put(&mux->rxQueue, &event->link);
        mux->rxqlen++;
        if(mux->rxqlen == (mux->rxnfmi+1))
        {
           //(1) perform message reassembly
           
           //(2) deliver the assembled message 
           FM_Printf(FM_ERROR, "MUX: Message reassembly is not supported at present.\n");
        }
    }

    return;
}


u8 MUXL_Proc(void *cookie)
{
    sEvent *event = NULL;
    sSlink *slink = NULL;
    u8      ret = 0;
    sMuxLayer *muxl = (sMuxLayer *)cookie;
//FM_Printf(FM_CTRL, "MUXL: pop a beacon/mgmt.\n");

    while(!SLIST_IsEmpty(&muxl->eventQueue)
#ifndef RTX51_TINY_OS		
          && !(ret = SCHED_IsPreempted(&muxl->task))
#endif
			)
    {
#ifdef P8051
__CRIT_SECTION_BEGIN__
#else
    SEM_WAIT(&muxl->muxSem);
#endif
            slink = SLIST_Pop(&muxl->eventQueue);
#ifdef P8051
__CRIT_SECTION_END__
#else
    SEM_POST(&muxl->muxSem);
#endif
        event = SLIST_GetEntry(slink, sEvent, link);
        MUX_Proc(&muxl->mux, event);
    }

    return ret;
}


void MUXL_RecvMgmtPacket(void* cookie, sEvent *event) __REENTRANT__
{
    sMuxLayer *muxl = (sMuxLayer *)cookie;
    /* check beacon */
    if ((event->eventHdr.eventClass == EVENT_CLASS_CTRL)&&
        (event->eventHdr.type == EVENT_TYPE_CC_BCN_IND))
    {
        FM_Printf(FM_MUX, "MUX: deliver a beacon.\n");
#ifdef CALLBACK
        muxl->mux.deliverMgmtMsg(muxl->mux.mgmtcookie, event); 
#else
        LINKL_RecvMgmtMsg(muxl->mux.mgmtcookie, event);
#endif
        return;
    }
    else if (event->eventHdr.eventClass == EVENT_CLASS_MSG)
    {

#if 0	
        /* place the event to the mux layer queue */
#ifdef P8051
__CRIT_SECTION_BEGIN__
#else
        SEM_WAIT(&muxl->muxSem);
#endif
        SLIST_Put(&muxl->eventQueue, &event->link);
#ifdef P8051
__CRIT_SECTION_END__
#else
        SEM_POST(&muxl->muxSem);
#endif
        /* schedule the task */
#ifndef RTX51_TINY_OS
        SCHED_Sched(&muxl->task);
#else
        os_set_ready(HPGP_TASK_ID_MUX);
#endif

#else

		MUX_Proc(&muxl->mux, event);


#endif

    }
}
    

#if 0
//The following function is executed in interrupt context
//void MUXL_RecvMgmtPacket(void* cookie)
//void MUXL_RecvMgmtPacket(void* cookie, sTxRxDesc *rxdesc, u8 *rxbuf, u16 pktlen)
void MUXL_RecvMgmtPacket(void* cookie, sRxDesc *rxdesc, 
                         u8 *rxbuf, u16 pktlen,
                         u8 *srcMacAddr)
{
    sEvent    *event = NULL;
    sHpgpHdr  *hpgpHdr = NULL;

    //check the ETH header
//    sEthHdr *ethhdr = NULL;
//    sRxDesc rxdesc;
//    u16 pktlen = 0;
    sMuxLayer *muxl = (sMuxLayer *)cookie;
    //check DTEI: cco tei, my tei, and multicast/broadcast tei
    //memset(&rxdesc, 0, sizeof(sRxDesc));
    //read Rx descriptor and get packet length
    
//FM_Printf(FM_CTRL, "MUX: received a beacon/mgmt (%d).\n", pktlen);

    //create an event
    event = EVENT_Alloc(pktlen + MAC_ADDR_LEN, sizeof(sHpgpHdr));
    if(event == NULL)
    {
        FM_Printf(FM_ERROR, "MUXL: Fail to allcate an event.\n");
        return;
    }

    hpgpHdr = (sHpgpHdr *)event->buffDesc.buff;

    hpgpHdr->tei = rxdesc->stei;
    hpgpHdr->snid = rxdesc->snid;

    memcpy(event->buffDesc.dataptr, srcMacAddr, MAC_ADDR_LEN);
    hpgpHdr->macAddr = event->buffDesc.dataptr;
    event->buffDesc.dataptr += MAC_ADDR_LEN;

    //copy the mm from HPGP MAC to event buff
    memcpy(event->buffDesc.dataptr, rxbuf, pktlen);
    event->buffDesc.datalen = pktlen;
#ifdef SIMU
    if(rxdesc->frameType == FRAME_TYPE_BEACON)
    {
        FM_Printf(FM_MUX|FM_LINFO, "MUX: received a beacon (%d).\n", pktlen);
        event->eventHdr.eventClass = EVENT_CLASS_CTRL;
        event->eventHdr.type = EVENT_TYPE_CC_BCN_IND;
    
    }
    else //mgmt msg
    {
FM_Printf(FM_MUX, "MUX: receive a mgmt msg.\n");
        event->eventHdr.eventClass = EVENT_CLASS_MSG;

//        ethhdr = (sEthHdr *)event->buffDesc.dataptr; 
//        event->eventHdr.macAddr = ethhdr->hdr.ethII.srcaddr;
        //chop off the ethernet header
//        event->buffDesc.dataptr += sizeof(sEthHdr); 
//        event->buffDesc.datalen -= sizeof(sEthHdr); 
    
    }
#endif

    //place the event to the mux layer queue
#ifdef P8051
__CRIT_SECTION_BEGIN__
#else
    SEM_WAIT(&muxl->muxSem);
#endif
    SLIST_Put(&muxl->eventQueue, &event->link);
#ifdef P8051
__CRIT_SECTION_END__
#else
    SEM_POST(&muxl->muxSem);
#endif

}
#endif

void MUXL_RegisterMgmtMsgCallback(sMuxLayer *muxl, 
    void (*callback)(void XDATA *cookie, sEvent XDATA *event),
    void *cookie)
{
#ifdef CALLBACK
    muxl->mux.deliverMgmtMsg = callback;
#endif
    muxl->mux.mgmtcookie = cookie;
}


eStatus MUXL_Init(sMuxLayer *muxl)
{
    eStatus status = STATUS_SUCCESS;

    SLIST_Init(&muxl->eventQueue);

    HAL_RegisterRxMgmtCallback(HOMEPLUG_GetHal(), MUXL_RecvMgmtPacket, (void *)muxl); 

#ifndef P8051
#if defined(WIN32) || defined(_WIN32)
    muxl->muxSem = CreateSemaphore(
        NULL,           // default security attributes
        SEM_COUNT,      // initial count
        SEM_COUNT,      // maximum count
        NULL);          // unnamed semaphore
    if(muxl->muxSem == NULL)
#else
    if(sem_init(&muxl->muxSem, 0, SEM_COUNT))
#endif
    {
        status = STATUS_FAILURE;
    }
#endif
#ifdef RTX51_TINY_OS  
//    os_create_task(HPGP_TASK_ID_MUX);
#else   
    SCHED_InitTask(&muxl->task, HPGP_TASK_ID_MUX, "MUX", 
                   HPGP_TASK_PRI_MUX, MUXL_Proc, muxl);
#endif
    MUX_InitMux(&muxl->mux);


    FM_Printf(FM_MINFO, "MUX Layer: Initialized.\n");
    return status;
}

#ifdef RTX51_TINY_OS
void MUXL_Task(void) //_task_ HPGP_TASK_ID_MUX  
{
    sMuxLayer* muxl = (sMuxLayer*)HPGPCTRL_GetLayer(HP_LAYER_TYPE_MUX);
    while (1) {
#ifdef UART_HOST_INTF
		os_switch_task();
#else
        os_wait1(K_SIG);
#endif
        MUXL_Proc(muxl);
    }
}
#endif


/** =========================================================
 *
 * Edit History
 *
 * $Source: /home/cvsrepo/Hybrii_B_OSFW_Dev/firmware/hpgp/src/mux/muxl.c,v $
 *
 * $Log: muxl.c,v $
 * Revision 1.6  2014/06/11 13:17:47  kiran
 * UART as host interface and peripheral interface supported.
 *
 * Revision 1.5  2014/05/28 10:58:59  prashant
 * SDK folder structure changes, Uart changes, removed htm (UI) task
 * Varified - UM, LM - iperf (UDP/TCP), overnight test pass
 *
 * Revision 1.4  2014/05/12 08:09:57  prashant
 * Route fix, STA sync issue with AV CCO and handling discover bcn issue fix
 *
 * Revision 1.3  2014/02/27 10:42:47  prashant
 * Routing code added
 *
 * Revision 1.2  2014/01/10 17:18:24  yiming
 * check in Rajan 1/8/2014 code release
 *
 * Revision 1.5  2014/01/08 10:53:54  ranjan
 * Changes for LM OS support.
 * New Datapath FrameTask
 * LM and UM  datapath, feature verified.
 *
 * known issues : performance numbers needs revisit
 *
 * review : pending.
 *
 * Revision 1.4  2013/09/04 14:51:16  yiming
 * New changes for Hybrii_A code merge
 *
 * Revision 1.16  2012/10/25 11:38:48  prashant
 * Sniffer code added for MAC_SAP, Added new commands in MAC_SAP for sniffer, bridge,
 *  hardware settings and peer information.
 *
 * Revision 1.15  2012/07/15 17:31:07  yuanhua
 * (1)fixed a potential memory overwriting in MUXL (2)update prints for 8051.
 *
 * Revision 1.14  2012/07/02 23:41:34  kripa
 * Converting mmType field from host to hpgp byte order and vice versa.
 * Committed on the Free edition of March Hare Software CVSNT Client.
 * Upgrade to CVS Suite for more features and support:
 * http://march-hare.com/cvsnt/
 *
 * Revision 1.13  2012/06/05 22:37:12  son
 * UART console does not get initialized due to task ID changed
 *
 * Revision 1.12  2012/06/05 07:25:59  yuanhua
 * (1) add a scan call to the MAC during the network discovery. (2) add a tiny task in ISM for interrupt polling (3) modify the frame receiving integration. (4) modify the tiny task in STM.
 *
 * Revision 1.11  2012/06/04 23:34:34  son
 * Added RTX51 OS support
 *
 * Revision 1.10  2012/05/19 20:32:17  yuanhua
 * added non-callback option for the protocol stack.
 *
 * Revision 1.9  2012/05/17 05:05:58  yuanhua
 * (1) added the option for timer w/o callback (2) added task id and name.
 *
 * Revision 1.8  2012/05/14 05:22:29  yuanhua
 * support the SCHED without using callback functions.
 *
 * Revision 1.7  2012/04/30 04:05:57  yuanhua
 * (1) integrated the HAL mgmt Tx. (2) various updates
 *
 * Revision 1.6  2012/04/25 13:53:41  yuanhua
 * changed the HAL_Transmit prototype.
 *
 * Revision 1.5  2012/04/13 06:15:11  yuanhua
 * integrate the HPGP protocol stack with the HAL for the beacon TX/RX and mgmt msg RX.
 *
 * Revision 1.4  2012/03/11 17:02:25  yuanhua
 * (1) added NEK auth in AKM (2) added NMA (3) modified hpgp layer data structures (4) added Makefile for linux simulation
 *
 * Revision 1.3  2011/09/14 05:52:36  yuanhua
 * Made Keil C251 compilation.
 *
 * Revision 1.2  2011/09/09 07:02:31  yuanhua
 * migrate the firmware code from the greenchip to the hybrii.
 *
 * Revision 1.7  2011/08/09 22:45:44  yuanhua
 * changed to event structure, seperating HPGP-related events from the general event defination so that the general event could be used for other purposes than the HPGP.
 *
 * Revision 1.6  2011/07/30 02:43:35  yuanhua
 * (1) Split the beacon process into two parts: one requiring an immdiate response, the other tolerating the delay (2) Changed the API btw the MUX and SHAL for packet reception (3) Fixed bugs in various modules. Now, multiple STAs could successfully associate/leave the CCo
 *
 * Revision 1.5  2011/07/22 18:51:05  yuanhua
 * (1) added the hardware timer tick simulation (hts.h/hts.c) (2) Added semaphors for sync in simulation and defined macro for critical section (3) Fixed some bugs in list.h and CRM (4) Modified and fixed bugs in SHAL (5) Changed SNSM/CNSM and SNAM/CNAM for bug fix (6) Added debugging prints, and more.
 *
 * Revision 1.4  2011/07/08 22:23:48  yuanhua
 * (1) Implemented CNSM, including its state machine, beacon transmission and process, discover beacon scheduling, auto CCo selection, discover list, handover countdown, etc. (2) Updated SNSM, including discover list processing, triggering a switch to the new CCo, etc. (3) Updated CNAM and SNAM, adding the connection state in the SNAM, switch to the new CCo, etc. (4) Other updates
 *
 * Revision 1.3  2011/07/02 22:09:02  yuanhua
 * Implemented both SNAM and CNAM modules, including network join and leave procedures, systemm resource (such as TEI) allocation and release, TEI renew/release timers, and TEI reuse timer, etc.
 *
 * Revision 1.2  2011/06/24 14:33:18  yuanhua
 * (1) Changed event structure (2) Implemented SNSM, including the state machines in network discovery and connection states, becaon process, discover process, and handover detection (3) Integrated the HPGP and SHAL
 *
 * Revision 1.1  2011/05/06 19:12:25  kripa
 * Adding mux layer files to new source tree.
 *
 * Revision 1.4  2011/04/23 23:07:00  kripa
 * MUX_Proc() ;changed references to 'event->data' to 'event->eventData', since 'data' is reserved word in Keil.
 *
 * Revision 1.3  2011/04/23 19:48:45  kripa
 * Fixing stm.h and event.h inclusion, using relative paths to avoid conflict with windows system header files.
 *
 * Revision 1.2  2011/04/23 17:16:38  kripa
 * void MUX_Proc(sMux *mux, sEvent *event) ; event->class to event->eventClass.
 *
 * Revision 1.1  2011/04/08 21:42:11  yuanhua
 * Framework
 *
 *
 * ==========================================================*/

