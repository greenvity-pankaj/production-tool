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
#include "nma.h"
#include "nma_fw.h"
#ifndef HPGP_HAL_TEST
#include "hpgpapi.h"
#endif
#include "frametask.h"
#include "dmm.h"
#include "datapath.h"
#ifdef NO_HOST
#include "gv701x_osal.h"
#include "event.h"
#include "app_sup_layer.h"
#endif
#include "hpgp_msgs.h"
#ifdef UART_HOST_INTF 
#include "gv701x_uartdriver_fw.h"
#endif
#include "hybrii_tasks.h"


static sFrameTask frmTaskLayer;
extern u8 sigDbg;
#ifdef ETH_BRDG_DEBUG
extern u32 ethRxFrameCnt;
extern u32 numPlcTxCp;
#endif

#ifdef UM
static sSlist	 hostEventQ;
static eStatus  fwdAgent_HostTransmitEvent();
static eStatus fwdAgent_SendHostEvent(sEvent *event);
#endif
#ifdef Z_P_BRIDGE
extern bool zb_plc_bridging;
extern void mac_hal_zb_pkt_bridge(sSwFrmDesc* rx_frame_info_p);
#endif
extern u8 opMode;
#ifdef NO_HOST
extern sDmm AppDmm;
#endif
#ifdef UART_HOST_INTF	   	
extern void timer0Poll();
extern void STM_Proc(void);
#endif
extern void HTM_CmdRun(void);

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



bool fwdAgent_IsHostIdle(u16 size)
{
	
#ifdef NO_HOST //UART_16550
        if(size == 0)
        {
            return TRUE;
        }
		return (DMM_CheckDepth(&AppDmm, size));
		/*Compiler warning suppression*/
#else 
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
#ifdef UART_HOST_INTF 
	if(hostIntf == HOST_INTF_UART)
	{
		return (hal_uart_isTxReady());
		
	}
#endif //UART_HOST_INTF
	
	return FALSE;
#endif	/*NO_HOST*/
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
#if defined(POWERSAVE) || defined(LLP_POWERSAVE)
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
	
        u16 size = 0; 
#ifdef NO_HOST
        if(datapath_IsQueueEmpty(HOST_DATA_QUEUE) == FALSE)
        {
    	    sSwFrmDesc* tHostTxFrmSwDesc = NULL;

            if((tHostTxFrmSwDesc = 
    		             	datapath_getHeadDesc(HOST_DATA_QUEUE, 0)) != NULL)
            {
                size = tHostTxFrmSwDesc->frmLen;
            }
        }
#endif


		if (fwdAgent_IsHostIdle(size) == TRUE)
		{
			if (fwdAgent_HostTransmitEvent() == STATUS_FAILURE)
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
	Aps_Proc(NULL);		
    APS_ProcPeripheral();
#endif	
#else

	if (datapath_IsQueueEmpty(HOST_DATA_QUEUE) == FALSE)
	{
		if (fwdAgent_IsHostIdle(0) == TRUE)
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
#ifdef PROD_TEST
//extern void	CmdRun();
extern void STM_Proc();
#endif


#ifdef RTX51_TINY_OS
void Frame_Task (void)  _task_ HYBRII_TASK_ID_FRAME
{
   while (1) {
	   	//os_wait1(K_SIG);
#ifdef PROD_TEST
	//CmdRun();
	STM_Proc();
#endif

#ifdef UART_HOST_INTF	   	
    timer0Poll();
	STM_Proc();	
	if(uartRxControl.rxExpectedCount == 0)
	{
		uartRxProc();
	}
#endif	
      	Frame_Proc();//&frmTaskLayer);
#ifdef UM
		HTM_CmdRun();	// moved here from green_main to
						// allow interrupts between printfs 
#endif
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

	
	//FM_Printf(FM_USER, "pq\n");
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
		case PORT_UART:
			switch(plcTxFrmSwDesc->txPort)
			{
				case PORT_PLC:
					fwdAgent_queueToPlc(plcTxFrmSwDesc);					
				break;
#ifdef NO_HOST
				case PORT_APP:
					fwdAgent_queueToPlc(plcTxFrmSwDesc);
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
				case PORT_UART:													
					if(opMode == LOWER_MAC)
					{
							CHAL_FreeFrameCp(plcTxFrmSwDesc->cpArr, plcTxFrmSwDesc->cpCount);
							return;	
					}
					fwdAgent_queueToHost( plcTxFrmSwDesc);
				break;

#ifdef NO_HOST				
				case PORT_APP:
	                if(opMode == LOWER_MAC)
	                {
	                    CHAL_FreeFrameCp(plcTxFrmSwDesc->cpArr, plcTxFrmSwDesc->cpCount);
	                    return;
	                }
	    			fwdAgent_queueToHost( plcTxFrmSwDesc);
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
					fwdAgent_queueToPlc(plcTxFrmSwDesc);
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
					u8 xdata *cellAddr;  
					cellAddr = CHAL_GetAccessToCP(plcTxFrmSwDesc->cpArr[0].cp);
					fwdAgent_queueToHost( plcTxFrmSwDesc);
				}
				break;	
				
				case PORT_PLC:
					fwdAgent_queueToPlc(plcTxFrmSwDesc);					
				break;	
				
#ifdef HYBRII_802154				
				case PORT_ZIGBEE:
#ifdef Z_P_BRIDGE					
					if (zb_plc_bridging) {
				        mac_hal_zb_pkt_bridge(plcTxFrmSwDesc);
				    }					
#else
#endif					
				break;				
#endif
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
#ifdef NO_HOST
				case PORT_APP:
					fwdAgent_queueToHost(plcTxFrmSwDesc);
				break;						
#endif				
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
			MUXL_TransmitMgmtMsg(event);
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
	switch(mod)
	{
		case FWDAGENT_ZIGBEE_EVENT:
#ifdef HYBRII_802154
			//zb_handleEvent(event);
#endif
			break;

		case FWDAGENT_DATAPATH_EVENT:
			fwdAgent_handleEvent(event);
			break;
		
		case FWDAGENT_HOST_EVENT:
			fwdAgent_SendHostEvent(event);
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
#ifdef NO_HOST
        Aps_PostRspEventToQueue(event);
#else

		datapath_hostTransmitFrame(event->buffDesc.dataptr, 
		  				   event->buffDesc.datalen);
		
		EVENT_Free(event);
#endif
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


