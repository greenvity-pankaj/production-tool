//#include "sched.h"
//#include "event.h"
//#include "list.h"
//#include "hal_common.h"
//#ifndef HPGP_HAL_TEST
//#include "hpgpapi.h"
//#endif

#ifdef RTX51_TINY_OS
#include <rtx51tny.h>
#endif
#include <stdio.h>
#include <string.h>
#include "fm.h"


#include "list.h"
#include "papdef.h"
#ifdef ROUTE
#include "hpgp_route.h"
#endif
#include "hpgpevt.h"

#include "hpgpdef.h"
#include "hal_common.h"
#include "hal.h"

#include "hal_hpgp.h"
#include "hal_eth.h"
#include "hal_spi.h"

#ifndef HPGP_HAL_TEST
#include "hpgpapi.h"
#endif
#include "nma.h"

//#include "h1msgs.h"
#include "frametask.h"
#include "dmm.h"
#include "datapath.h"
#ifdef NO_HOST
#include "gv701x_event.h"
#include "gv701x_aps.h"
#include "app_sup_layer.h"
#endif

static sFrameTask frmTaskLayer;
extern u8 sigDbg;
#ifdef ETH_BRDG_DEBUG
extern u32 ethRxFrameCnt;
extern u32 numPlcTxCp;
#endif

#ifdef UART_HOST_INTF 
#include "uart_driver.h"
#endif
#include "hybrii_tasks.h"
#ifdef UM

static sSlist	 hostEventQ;


static eStatus  fwdAgent_HostTransmitEvent();
static eStatus fwdAgent_SendHostEvent(sEvent *event);
#endif

extern sSwFrmDesc *datapath_getHeadDesc(queue_id_e id, u8 pop);
extern u8 opMode;

void frame_task_init(void)
{

	
#ifdef RTX51_TINY_OS

	/*Create event queue for frame task events*/
	SLIST_Init(&frmTaskLayer.eventQueue);

	/*Create semaphore for Frame task event queue*/
#ifndef P8051
#if defined(WIN32) || defined(_WIN32)
    frmTaskLayer->frmEvntSem = CreateSemaphore(
        NULL,           // default security attributes
        SEM_COUNT,      // initial count
        SEM_COUNT,      // maximum count
        NULL);          // unnamed semaphore
    if(frmTaskLayer->frmEvntSem == NULL)
#else
    if(sem_init(&frmTaskLayer->frmEvntSem, 0, SEM_COUNT))
#endif
    {
        status = STATUS_FAILURE;
    }
#endif	

	 os_create_task(HYBRII_TASK_ID_FRAME);
#endif

#ifdef UM
    SLIST_Init(&hostEventQ);	
#endif
		
	datapath_init();
#ifdef NO_HOST
	Aps_init();
#endif
	//FM_Printf(FM_HINFO,"\nFrame Task Init");
}



bool fwdAgent_IsHostIdle()
{
	
#ifdef HYBRII_ETH

	if(hostIntf == HOST_INTF_ETH)
	{

		return (EHAL_IsTxReady());
	
	}
#endif

#ifdef HYBRII_SPI

	if(hostIntf == HOST_INTF_SPI)
	{
		return (hal_spi_isTxReady());
	}
#endif   //HYBRII_SPI
#ifdef UART_HOST_INTF //UART_16550
	if(hostIntf == HOST_INTF_UART)
	{
		return (hal_uart_isTxReady());
		
	}
#endif  
	
	return FALSE;
	
	
}

u8 Frame_Proc()//void *cookie)
{
  //  sEvent *event = NULL;
  //  sSlink *slink = NULL;
//		eStatus status;
    u8 ret = 0;
 //   sFrameTask *frmtasklayer = (sFrameTask *)cookie;
#ifdef POWERSAVE
    sLinkLayer    *linkLayer = (sLinkLayer *)HPGPCTRL_GetLayer(HP_LAYER_TYPE_LINK);
    sStaInfo      *staInfo = LINKL_GetStaInfo(linkLayer);
	sHaLayer 	  *hal;
	sHpgpHalCB 	  *hhalCb;
	sScb          *scb;

	hal = (sHaLayer*)HOMEPLUG_GetHal();
	hhalCb = hal->hhalCb;
	if( hhalCb->devMode == DEV_MODE_STA)
		scb = staInfo->staScb;
	else scb = staInfo->ccoScb;
#endif

	//FM_Printf(FM_USER,"p\n");
#if 0
    while(!SLIST_IsEmpty(&frmtasklayer->eventQueue) /*&&
          !(ret = SCHED_IsPreempted(&sFrameTask->task))*/)
    {
		FM_Printf(FM_USER,"\nFrame Queue");    
#ifdef P8051
__CRIT_SECTION_BEGIN__
#else
    SEM_WAIT(&frmtasklayer->ctrlSem);
#endif

        slink = SLIST_Pop(&frmtasklayer->eventQueue);
#ifdef P8051
__CRIT_SECTION_END__
#else
    	SEM_POST(&frmtasklayer->ctrlSem);
#endif
        event = SLIST_GetEntry(slink, sEvent, link);

        if (event->eventHdr.eventClass == EVENT_CLASS_CTRL)
        {
			FM_Printf(FM_USER,"\nEvnt");            
#ifdef HPGP_HAL_TEST
			CHAL_CpuTxQNemptyIntHandler();
#else
			CHAL_FrameRxIntHandler((void*)HOMEPLUG_GetHal());
#endif
        }
        else
        {
	        event->eventHdr.status = EVENT_STATUS_COMPLETE;			
        }
		
        if(event->eventHdr.status == EVENT_STATUS_COMPLETE)
        {
            EVENT_Free(event);
        }
        
    }
#else

#ifdef UM

	if (datapath_transmitMgmtPlc() == FALSE)    
#endif
#ifdef POWERSAVE
//	if (scb->psState == PSM_PS_STATE_OFF)
//	{
		// if PS mode is on, no use transmitting here
		// because the chance of not being able to tx
		// is too great. We'll do the tx in BP Start interrupt
 		datapath_transmitDataPlc(2);
//	}
#else
 	datapath_transmitDataPlc();
#endif

#ifdef UM
    if(!SLIST_IsEmpty(&hostEventQ) || 
        (datapath_IsQueueEmpty(HOST_DATA_QUEUE) == FALSE))
	{
		if (fwdAgent_IsHostIdle() == TRUE)
		{
			if (fwdAgent_HostTransmitEvent() == STATUS_FAILURE)
			{			
				if (datapath_IsQueueEmpty(HOST_DATA_QUEUE)
										== TRUE)
				{
					return ret;
				}	 

				
			//			 FM_Printf(FM_USER, "h\n");
				datapath_transmitDataHost();
			}
		}
		else
		{
			if  ( (datapath_IsQueueEmpty(HOST_DATA_QUEUE)
										== FALSE)
					)
			{

				// Doubt : TNY OS doesn't allow TASK to re-schedule itself. its in VAIN
				
				os_set_ready(HYBRII_TASK_ID_FRAME);


			}
										

		}
	}
#ifdef NO_HOST
	if (datapath_IsQueueEmpty(APP_DATA_QUEUE) == FALSE)
	{								
		sSwFrmDesc* tHostTxFrmSwDesc = NULL;	
	    if((tHostTxFrmSwDesc =
			 datapath_getHeadDesc(APP_DATA_QUEUE, 1)) != NULL)
	    {
			tHostTxFrmSwDesc->txPort = PORT_APP;
			tHostTxFrmSwDesc->rxPort = PORT_PERIPHERAL;
			Aps_PostDataToQueue(tHostTxFrmSwDesc->rxPort, tHostTxFrmSwDesc);
	    }		
	}
#endif
#ifdef NO_HOST
	Aps_Proc(NULL);		
#endif	
#else

	if (datapath_IsQueueEmpty(HOST_DATA_QUEUE) == FALSE)
	{
		if (fwdAgent_IsHostIdle() == TRUE)
		{	

			{			
				if (datapath_IsQueueEmpty(HOST_DATA_QUEUE)
										== TRUE)
				{
					
					return ret;
				}	 

				
				datapath_transmitDataHost();
			}
		}
		else
		{
			
			if (datapath_IsQueueEmpty(HOST_DATA_QUEUE)
										== FALSE)
			{

				os_set_ready(HYBRII_TASK_ID_FRAME);


			}
										

		}

	}
#endif
	
#endif

    return ret;
}


#ifdef RTX51_TINY_OS
void Frame_Task (void)  _task_ HYBRII_TASK_ID_FRAME
{
   while (1) {
	   	//os_wait1(K_SIG);
      	Frame_Proc();//&frmTaskLayer);
        os_switch_task();
    }
}
#endif

#if 0
 /* post an event into the frame task event queue */
 eStatus FrameTask_PostEvent(enum eventType evttype, 
						 u8 evntclass, void* data_ptr, u16 len)
 {

 
#ifdef RTX51_TINY_OS

 #if 0
	 sEvent * event= EVENT_Alloc(len, 0);

	 FM_Printf(FM_ERROR, "\nPost evnt");		 
	 if(event == NULL)
	 {
		 FM_Printf(FM_ERROR, "EAllocErr\n");
		 return STATUS_FAILURE;
	 }
 
	 event->eventHdr.eventClass = evntclass;
	 event->eventHdr.type = evttype;
	 //event->buffDesc.datalen = len;

	 //memcpy(event->buffDesc.dataptr , data, len);

	 /* enqueue the event to the frame task queue */
	 SLIST_Put(&frmTaskLayer.eventQueue, &event->link);
#endif

	 os_set_ready(HYBRII_TASK_ID_FRAME);
#else
 
	//Frame_Proc();//&frmTaskLayer);
 
 #endif
	 return STATUS_SUCCESS;
}
#endif



/*
		Forwarding Agent
*/
void fwdAgent_queueToHost(sSwFrmDesc  *rxFrmSwDesc)
{
#if 0 //def UM

	u8 *cellAddr;
	sEth2Hdr *ethhdr;

	cellAddr =
			CHAL_GetAccessToCP(rxFrmSwDesc->cpArr[0].cp);
	ethhdr = (sEth2Hdr*)cellAddr;
	// Drop frm if dest MAC not match with device MAC address
	if(IS_GROUP(ethhdr->dstaddr))
	{

	}
	else if(memcmp(ethhdr->dstaddr, pHal->macAddr, MAC_ADDR_LEN) != 0)
	{
		// Drop frm
#ifdef DEBUG_DATAPATH
		if(pktDbg)
		{
			FM_Printf(FM_MINFO, "DATA drop\n");
		}
#endif
		return ; //STATUS_FAILURE;
	}

#endif

	
	if ((hostDetected == FALSE) ||(datapath_queueToHost(rxFrmSwDesc,
	   					  			rxFrmSwDesc->frmLen) == STATUS_FAILURE))
	{
        gHpgpHalCB.halStats.PtoHswDropCnt++;
		CHAL_FreeFrameCp(rxFrmSwDesc->cpArr, rxFrmSwDesc->cpCount);

	}


}
	
void fwdAgent_queueToPlc(sSwFrmDesc  *plcTxFrmSwDesc)
{

#ifdef ETH_BRDG_DEBUG
	ethRxFrameCnt++;
	numPlcTxCp += plcTxFrmSwDesc->cpCount;
#endif

	
//	FM_Printf(FM_USER, "pq\n");
	datapath_queue(PLC_DATA_QUEUE, plcTxFrmSwDesc);

}


void fwdAgent_handleData(sSwFrmDesc  *plcTxFrmSwDesc)
{
	switch(plcTxFrmSwDesc->rxPort)
	{
#ifndef NO_HOST	
		case PORT_HOST:
#endif			
        case PORT_ETH:
		case PORT_SPI:				
			switch(plcTxFrmSwDesc->txPort)
			{
				case PORT_PLC:
					fwdAgent_queueToPlc(plcTxFrmSwDesc);					
				break;
#ifdef NO_HOST
				case PORT_APP:
					Aps_PostDataToQueue(plcTxFrmSwDesc->rxPort, plcTxFrmSwDesc);
				break;				
#endif				
				default:
                    CHAL_FreeFrameCp(plcTxFrmSwDesc->cpArr, plcTxFrmSwDesc->cpCount);				
				break;
			}
		break;

		case PORT_PLC:
        switch(plcTxFrmSwDesc->txPort)
        {
#ifndef NO_HOST	
    		case PORT_HOST:
#endif			
    		case PORT_ETH:	
    		case PORT_SPI:
    			fwdAgent_queueToHost( plcTxFrmSwDesc);
    			break;
#ifdef NO_HOST				
    		case PORT_APP:
                if(opMode == LOWER_MAC)
                {
                    CHAL_FreeFrameCp(plcTxFrmSwDesc->cpArr, plcTxFrmSwDesc->cpCount);
                    return;
                }
    			Aps_PostDataToQueue(plcTxFrmSwDesc->rxPort, plcTxFrmSwDesc);
    		break;				
#endif				
			default:
                CHAL_FreeFrameCp(plcTxFrmSwDesc->cpArr, plcTxFrmSwDesc->cpCount);				
			break;
		}
		break;		
		case PORT_PERIPHERAL:
			switch(plcTxFrmSwDesc->txPort)
			{
#ifdef NO_HOST	                
				case PORT_APP:
					Aps_PostDataToQueue(plcTxFrmSwDesc->rxPort, plcTxFrmSwDesc);
				break;				
#endif				
				default:
                    CHAL_FreeFrameCp(plcTxFrmSwDesc->cpArr, plcTxFrmSwDesc->cpCount);				
				break;
			}
		break;				
#ifdef NO_HOST
		case PORT_APP:
			switch(plcTxFrmSwDesc->txPort)
			{
				case PORT_PERIPHERAL:
				{
					u8 xdata          *cellAddr;  
					cellAddr = CHAL_GetAccessToCP(plcTxFrmSwDesc->cpArr[0].cp);
					fwdAgent_queueToHost( plcTxFrmSwDesc);
				}
				break;	
		case PORT_PLC:
			fwdAgent_queueToPlc(plcTxFrmSwDesc);
			break;
				default:
                    CHAL_FreeFrameCp(plcTxFrmSwDesc->cpArr, plcTxFrmSwDesc->cpCount);					
				break;
			}
			break;
#endif	
		case PORT_ZIGBEE:
			switch(plcTxFrmSwDesc->txPort)
			{
				case PORT_HOST:
					fwdAgent_queueToHost(plcTxFrmSwDesc);
					break;
				case PORT_PLC:
					fwdAgent_queueToPlc(plcTxFrmSwDesc);
					break;
				default:
					CHAL_FreeFrameCp(plcTxFrmSwDesc->cpArr, plcTxFrmSwDesc->cpCount);
					break;		
			}
			break;
			

		default:
           CHAL_FreeFrameCp(plcTxFrmSwDesc->cpArr, plcTxFrmSwDesc->cpCount);		
		break;
	}
}

#ifdef UM


void fwdAgent_sendFrame(eHybriiPortNum dstPort,
							  sEvent *event)
{

	if (event->eventHdr.eventClass != EVENT_CLASS_MGMT)
	{
		return; // This function forward only mgmt frames to ports		
	}
	
	switch(dstPort)
	{
		case PORT_PLC:
			MUXL_TransmitMgmtMsg(NULL, event);
			break;

		case PORT_ZIGBEE:
			break;
		
		case PORT_HOST:
			SEND_HOST_EVENT(event);
			break;

		default:
			break;
				

	}

	
}



void fwdAgent_handleEvent(sEvent *event)
{
	EVENT_Free(event);
}


void fwdAgent_sendEvent(eFwdAgentModule mod,
								 sEvent *event)
{
	if (event->eventHdr.eventClass != EVENT_CLASS_CTRL)
	{

		// This function only handles Control Event to System Modules
	}
		

	switch(mod)
	{
		case FWDAGENT_ZIGBEE_EVENT:

#ifdef HYBRII_ZIGBEE

//			zb_handleEvent(event);

#endif

			break;

		case FWDAGENT_DATAPATH_EVENT:

			fwdAgent_handleEvent(event);
			break;
		
		case FWDAGENT_HOST_EVENT:
#ifdef NO_HOST
			Aps_PostRspEventToQueue(event);
#else
			fwdAgent_SendHostEvent(event);
#endif
			break;

		case FWDAGENT_HOMEPLUG_EVENT:
#ifdef UM

			CTRLL_ReceiveEvent(NULL, event);
#endif
			break;

		default:
			EVENT_Free(event);
			break;

	}

	return;
}



eStatus  fwdAgent_HostTransmitEvent()
{

    sEvent *event = NULL;
    sSlink *slink = NULL;

	if(!SLIST_IsEmpty(&hostEventQ))
	{
		
        slink = SLIST_Pop(&hostEventQ);
		
		event = SLIST_GetEntry(slink, sEvent, link);
		datapath_hostTransmitFrame(event->buffDesc.dataptr, 
		  				   event->buffDesc.datalen);
		
		EVENT_Free(event);

		return STATUS_SUCCESS;

	}

	return STATUS_FAILURE;

}



eStatus fwdAgent_SendHostEvent(sEvent *event) 
{
    /* post the event to the tx queue */
	
	if ((hostDetected == FALSE) &&
	  	(event->eventHdr.type != HOST_EVENT_FW_READY))
	{
		EVENT_Free(event);
		return STATUS_FAILURE;
	}

    SLIST_Put(&hostEventQ, &event->link);

    return STATUS_SUCCESS;
}
#endif


