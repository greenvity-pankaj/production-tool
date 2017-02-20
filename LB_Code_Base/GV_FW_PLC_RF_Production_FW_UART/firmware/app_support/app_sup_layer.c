#ifdef NO_HOST
#include <stdio.h>
#include <string.h>
#include "papdef.h"
#include "list.h"
#include "event.h"
#include "hal_common.h"
#include "dmm.h"
#include "nma.h"
#include "gv701x_osal.h"
#include "app_sup_layer.h"
#include "dmm.h"
#include "hpgpevt.h"
#include "hal_hpgp.h"
#include "hpgpdef.h"
#include "linkl.h"
#include "nma.h"
#include "nma_fw.h"
#include "green.h"
#include "fm.h"
#include "frametask.h"
#ifdef RTX51_TINY_OS
#include <rtx51tny.h>
#endif
#include "hybrii_tasks.h"
#ifdef HYBRII_SPI
#include "hal_spi.h"
#endif
#ifdef UART_HOST_INTF
#include "gv701x_uartdriver_fw.h"
#endif
#ifdef HYBRII_ETH
#include "hal_eth.h"
#endif

#define APP_MEM_POOL_SIZE         4048 //5632//5000 //
#define APP_MEM_SLAB_SEG_SIZE_0   32
#define APP_MEM_SLAB_SEG_SIZE_1   64
#define APP_MEM_SLAB_SEG_SIZE_2   150
#define APP_MEM_SLAB_SEG_SIZE_3   196 
#define APP_MEM_SLAB_SEG_SIZE_4   256 
#define APP_MEM_SLAB_SEG_SIZE_5   560
#define APP_MEM_SLAB_SEG_SIZE_6   1024

extern sNma *HOMEPLUG_GetNma() ;

#ifdef SW_BCST
#define SW_SSN_SIZE 1
#define BCST_RETRY  3
extern volatile u8 gSoftBCST;
#endif

gv701x_aps_queue_t appSupLayer;
u8 gapp_id = 0;

u8 XDATA AppMemPool[APP_MEM_POOL_SIZE];
sSlist peripheralTxQ;
//sSlist peripheralRxQ;
#if 0

static sSlabDesc AppSlabDesc[] =
{
    {4, APP_MEM_SLAB_SEG_SIZE_0},
    {5, APP_MEM_SLAB_SEG_SIZE_1},
    {6, APP_MEM_SLAB_SEG_SIZE_2},
    {6, APP_MEM_SLAB_SEG_SIZE_3},
    {6, APP_MEM_SLAB_SEG_SIZE_4},
    {0, APP_MEM_SLAB_SEG_SIZE_5},    
    {0, APP_MEM_SLAB_SEG_SIZE_6},
};
#else
static sSlabDesc AppSlabDesc[] =
{
    {4, APP_MEM_SLAB_SEG_SIZE_0},
    {2, APP_MEM_SLAB_SEG_SIZE_1},
    {6, APP_MEM_SLAB_SEG_SIZE_2},
    {6, APP_MEM_SLAB_SEG_SIZE_3},
    {5, APP_MEM_SLAB_SEG_SIZE_4},
    {0, APP_MEM_SLAB_SEG_SIZE_5},    
    {0, APP_MEM_SLAB_SEG_SIZE_6},
};
#endif
extern sDmm AppDmm;
extern u8 opMode;
#ifdef UART_HOST_INTF
extern bool hal_uart_tx (sEvent * event);
#endif
extern sHomePlugCb HomePlug;

extern void Host_RxHandler(sHaLayer *pHal, sCommonRxFrmSwDesc* pRxFrmDesc);

gv701x_aps_queue_t* Aps_init(void)
{	
	sNma * nma = HOMEPLUG_GetNma();
	/*Create transmit queue for application data tx*/
	SLIST_Init(&appSupLayer.txQueue);

	/*Create recieve queue for sending data to application*/
	SLIST_Init(&appSupLayer.rxQueue);

	/*Create recieve queue for sending response/event to application*/
	SLIST_Init(&appSupLayer.rspEvntRxQueue);

	appSupLayer.reqTxQueue = &nma->eventQueue;
	
    SLIST_Init(&peripheralTxQ);    
  //  SLIST_Init(&peripheralRxQ);

	DMM_Init(APP_POOL_ID);
	DMM_InitMemPool(APP_POOL_ID, AppMemPool, sizeof(AppMemPool), AppSlabDesc, 
					sizeof(AppSlabDesc)/sizeof(AppSlabDesc[0]));

	if(opMode == UPPER_MAC)
	{
		GV701x_TaskInit(&appSupLayer);
	    os_create_task(HYBRII_TASK_ID_APP);
		os_switch_task();	
	}
	return &appSupLayer;
}

eStatus Aps_PostDataToQueue(u8 src_port, sSwFrmDesc* plcRxFrmSwDesc)
{	
	sEvent* event = NULL;	
	u16 payloadlen, frmLen;	
	volatile u8 xdata * cellAddr1stCP;
    u8 i;
    u16 len = 0;
	
	if((src_port != PORT_PLC) && (src_port != PORT_PERIPHERAL) &&
		(src_port != PORT_ZIGBEE))
	{
		CHAL_FreeFrameCp(plcRxFrmSwDesc->cpArr, plcRxFrmSwDesc->cpCount);			
		return STATUS_FAILURE;
	}

	event = GV701x_EVENT_Alloc(plcRxFrmSwDesc->frmLen, 0);

	if(event == NULL)
	{
        CHAL_FreeFrameCp(plcRxFrmSwDesc->cpArr, plcRxFrmSwDesc->cpCount);
		gHpgpHalCB.halStats.HtoPswDropCnt++;
		return STATUS_FAILURE;
	}
    event->eventHdr.eventClass = EVENT_CLASS_DATA;
    event->eventHdr.type = 0;
	if(src_port == PORT_PLC)
	    event->eventHdr.trans = APP_PORT_PLC;
	else if(src_port == PORT_PERIPHERAL)
	    event->eventHdr.trans = APP_PORT_PERIPHERAL;
	else if(src_port == PORT_ZIGBEE)
	    event->eventHdr.trans = APP_PORT_ZIGBEE;
	
    event->buffDesc.datalen = plcRxFrmSwDesc->frmLen;
    frmLen = plcRxFrmSwDesc->frmLen;
    for(i=0; i<plcRxFrmSwDesc->cpCount; i++)
    {
    	cellAddr1stCP  = CHAL_GetAccessToCP(plcRxFrmSwDesc->cpArr[i].cp);
        if(frmLen < HYBRII_CELLBUF_SIZE)
        {
            payloadlen = frmLen;
        }
        else
	    {
            payloadlen = HYBRII_CELLBUF_SIZE;
        }
        if(cellAddr1stCP)
        {
            memcpy((u8*)(&(event->buffDesc.dataptr[len])), cellAddr1stCP, payloadlen);              
        }
        len += payloadlen;
        frmLen -= payloadlen;
    }

    /* enqueue the event to the transmit queue */
__CRIT_SECTION_BEGIN__
	SLIST_Put(&(appSupLayer.rxQueue), &event->link); 	
__CRIT_SECTION_END__

    CHAL_FreeFrameCp(plcRxFrmSwDesc->cpArr, plcRxFrmSwDesc->cpCount);
#ifdef RTX51_TINY_OS	
	os_set_ready(HYBRII_TASK_ID_APP);
#endif	

	//Should there be a os_set_ready() call here ?
    return STATUS_SUCCESS;       
}

eStatus Aps_PostRspEventToQueue(sEvent* event)
{	
	SLINK_Init(&event->link);

    /* enqueue the event to the response transmit queue */
	SLIST_Put(&(appSupLayer.rspEvntRxQueue), &event->link); 	

#ifdef RTX51_TINY_OS	
	os_set_ready(HYBRII_TASK_ID_APP);
#endif	

	//Should there be a os_set_ready() call here ?
    return STATUS_SUCCESS;       
}

#if 0
void Aps_TxData(u8 dstn_port, u8* buff, u16 len)
{
	if((dstn_port != APP_PORT_PLC) &&
		(dstn_port != APP_PORT_PERIPHERAL) &&
		(dstn_port != APP_PORT_ZIGBEE))
		return;
	
	if(len > HYBRII_CELLBUF_SIZE)
		return;
	
	if(dstn_port == APP_PORT_PLC)
	{
		eStatus status;
		u8 eth_hdr_cp = 0;
		u8 xdata* cellAddr;
		sCommonRxFrmSwDesc RxFrmDesc;
		uRxPktQDesc1* pRxPktQ1stDesc;
		uRxPktQCPDesc* pRxPktQCPDesc;
		sHaLayer *pHal = &(HomePlug.haLayer); //HOMEPLUG_GetHal();
		memset((u8*)&RxFrmDesc, 0x00, sizeof(RxFrmDesc));			
		status = CHAL_RequestCP(&eth_hdr_cp);

		if (status != STATUS_SUCCESS)
		{
			FM_Printf(FM_ERROR, "\nCP alloc fail");
			return;
		}
		
		cellAddr = CHAL_GetAccessToCP(eth_hdr_cp);
		memset(cellAddr, 0x00, HYBRII_CELLBUF_SIZE);  		
		RxFrmDesc.cpArr[0] = eth_hdr_cp;
		RxFrmDesc.cpCount = 1;
		memcpy((u8*)cellAddr, buff, (u8)len);
		pRxPktQ1stDesc = (uRxPktQDesc1*)&RxFrmDesc.hdrDesc;
		pRxPktQCPDesc  = (uRxPktQCPDesc*)&RxFrmDesc.firstCpDesc;	
		pRxPktQ1stDesc->s.frmLenLo = (u8)len;
		pRxPktQ1stDesc->s.frmLenHi = 0;
		pRxPktQCPDesc->s.cp = eth_hdr_cp;
		RxFrmDesc.hdrDesc.s.rsv4 = 0;		
		RxFrmDesc.hdrDesc.s.srcPort = PORT_APP;				
		RxFrmDesc.hdrDesc.s.dstPort = PORT_PLC;						
		Host_RxHandler(pHal, &RxFrmDesc);
	}
	else if((dstn_port == APP_PORT_PERIPHERAL) || 
		(dstn_port == APP_PORT_ZIGBEE))
	{
		sSwFrmDesc plcTxFrmSwDesc; 
		u8 eth_hdr_cp = 0;
		u8 xdata* cellAddr;
		eStatus status;				

		memset(&plcTxFrmSwDesc, 0x00, sizeof(plcTxFrmSwDesc));
		plcTxFrmSwDesc.frmType = HPGP_HW_FRMTYPE_MSDU;
		plcTxFrmSwDesc.rxPort = PORT_APP;
		if(dstn_port == APP_PORT_PERIPHERAL)				
			plcTxFrmSwDesc.txPort = PORT_PERIPHERAL;		
		else if (dstn_port == APP_PORT_ZIGBEE)
			plcTxFrmSwDesc.txPort = PORT_ZIGBEE;		

		plcTxFrmSwDesc.frmLen = len;	
		status = CHAL_RequestCP(&eth_hdr_cp);		
		if (status != STATUS_SUCCESS)
		{
			FM_Printf(FM_ERROR, "\nCP alloc fail");
			return;
		}				
		cellAddr = CHAL_GetAccessToCP(eth_hdr_cp);
		memset(cellAddr, 0x00, HYBRII_CELLBUF_SIZE);  		
		plcTxFrmSwDesc.cpArr[0].cp = eth_hdr_cp;
		plcTxFrmSwDesc.cpArr[0].len = (u8)len;		
		plcTxFrmSwDesc.cpCount = 1;
		memcpy((u8*)cellAddr, buff, (u8)len);

		//fwdAgent_handleData(&plcTxFrmSwDesc);
		queueToPeripheral(event);
	}
}
#endif

bool APS_IsPeripheralIdle()
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
void APS_ProcPeripheral()
{
    sEvent *event = NULL;
    sSlink *slink = NULL;

    /* check peripheral status */
    if(APS_IsPeripheralIdle() == FALSE)
    {
        return;
    }
	/*Read peripheral tx  Queue*/
	if(!SLIST_IsEmpty(&peripheralTxQ))
	{
__CRIT_SECTION_BEGIN__
		slink = SLIST_Pop(&peripheralTxQ);
__CRIT_SECTION_END__	
		event = SLIST_GetEntry(slink, sEvent, link);

		if(event == NULL)		
			return;

#ifdef UART_HOST_INTF		
		if(hostIntf == HOST_INTF_UART)
        {
            hal_uart_tx(event);        
        }
#endif        
        EVENT_Free(event);
	}				

    
}

extern u32 gApsRxFwd;
extern u32 gApsRxApp;
	

void APS_QueueToPeripheral(sEvent *event)
{
__CRIT_SECTION_BEGIN__

    SLIST_Put(&peripheralTxQ, &event->link);
    // pop here 
    APS_ProcPeripheral();
__CRIT_SECTION_END__
}

void Aps_ProcEvent(void *cookie, sEvent *event)
{
	/*Compiler warning suppression*/
	cookie = cookie;
	
	if((event->buffDesc.dataptr == NULL) || 
	  (event->buffDesc.datalen == 0) )
    {   
        EVENT_Free(event);
		return; 
    }
	if(event->eventHdr.eventClass != EVENT_CLASS_DATA)
	{   
        EVENT_Free(event);
		return; 
    }	
	gApsRxApp++;
    if((event->eventHdr.trans == APP_PORT_PLC) ||
	   (event->eventHdr.trans == APP_PORT_ZIGBEE))
    {
        eStatus status;
        u8 xdata* cellAddr;
        sCommonRxFrmSwDesc RxFrmDesc;
        uRxPktQDesc1* pRxPktQ1stDesc;
        uRxPktQCPDesc* pRxPktQCPDesc;
        sHaLayer *pHal = &(HomePlug.haLayer); //HOMEPLUG_GetHal();
        u16 len;
        u8 payloadlen;
        u8 i=0;
        u16 frmLen = 0;
#ifdef SW_BCST		
		static u8 swSsn = 255;
		u8 lastDescLen = 0;
		sEth2Hdr   *pktEthHdr;
		sCommonRxFrmSwDesc RxFrmDescArr[BCST_RETRY];
		u8 bcstFrame;
#endif		
	    memset((u8*)&RxFrmDesc, 0x00, sizeof(RxFrmDesc));           

        RxFrmDesc.cpCount = 0;
#ifdef SW_BCST		
		if(event->eventHdr.status & APP_DATA_TX_SWBCST_OPT)
		{
			gSoftBCST = 1;
			// do mac lookup to identify broadcast frame
			pktEthHdr = ((sEth2Hdr*)(event->buffDesc.dataptr));
			if((pktEthHdr->dstaddr[0] & 0x01))
			{
				len = event->buffDesc.datalen + SW_SSN_SIZE; // Add Software SSN size so that FW can 
				event->buffDesc.datalen = len;
				lastDescLen = len % HYBRII_CELLBUF_SIZE;
				bcstFrame = 1;
			}
			else
			{
				len = event->buffDesc.datalen;
				bcstFrame = 0;
			}
		}
		else
		{
			gSoftBCST = 0;
			bcstFrame = 0;
#endif		
	        len = event->buffDesc.datalen;
#ifdef SW_BCST	
		}
#endif		
        while(len > 0)
        {
            if(len < HYBRII_CELLBUF_SIZE)
            {
                payloadlen = len;
                RxFrmDesc.lastDescLen = len;
            }
            else
            {
                payloadlen = HYBRII_CELLBUF_SIZE;
				RxFrmDesc.lastDescLen = HYBRII_CELLBUF_SIZE;
            }
            status = CHAL_RequestCP(&RxFrmDesc.cpArr[i]);

            if (status != STATUS_SUCCESS)
            {
                FM_Printf(FM_ERROR, "\nCP alloc fail");
				EVENT_Free(event);
                return;
            }
        	cellAddr  = CHAL_GetAccessToCP(RxFrmDesc.cpArr[i]);
        	
            if(cellAddr)
            {
#ifdef SW_BCST            
            	if(bcstFrame == 1)
        		{
					if(len <= lastDescLen)
					{
						if(len > SW_SSN_SIZE)
						{
							memcpy(cellAddr,(u8*)(&(event->buffDesc.dataptr[frmLen])), payloadlen-SW_SSN_SIZE);						
						}
						cellAddr[payloadlen-SW_SSN_SIZE] = 	swSsn;	
					}
					else
					{
						memcpy(cellAddr,(u8*)(&(event->buffDesc.dataptr[frmLen])), payloadlen); 
					}
            	}
				else
				{
#endif				
					memcpy(cellAddr,(u8*)(&(event->buffDesc.dataptr[frmLen])), payloadlen); 
#ifdef SW_BCST
				}
#endif				
			}
            len -= payloadlen;
            frmLen += payloadlen;
            RxFrmDesc.cpCount++;
            i++;
        }
		  
        pRxPktQ1stDesc = (uRxPktQDesc1*)&RxFrmDesc.hdrDesc;
        pRxPktQCPDesc  = (uRxPktQCPDesc*)&RxFrmDesc.firstCpDesc;    
        pRxPktQ1stDesc->s.frmLenLo = (u8)event->buffDesc.datalen;
        pRxPktQ1stDesc->s.frmLenHi =  (event->buffDesc.datalen >> PKTQDESC1_FRMLENHI_POS);
        pRxPktQCPDesc->s.cp = RxFrmDesc.cpArr[0];
        RxFrmDesc.hdrDesc.s.rsv4 = 0;       
        RxFrmDesc.hdrDesc.s.srcPort = PORT_APP;      
		if(event->eventHdr.trans == APP_PORT_ZIGBEE)		
		{
			RxFrmDesc.hdrDesc.s.dstPort = PORT_ZIGBEE; 	
		}
		else if (event->eventHdr.trans == APP_PORT_PLC)
		{
			RxFrmDesc.hdrDesc.s.dstPort = PORT_PLC; 	
		}
				
		gApsRxFwd++;
#ifdef SW_BCST
			if(bcstFrame == 1)
	        {	//printf("tx SSN %bu\n",swSsn);
				swSsn++; 
				for(i=0;i<RxFrmDesc.cpCount;i++)
				{ u8 usage;
EA = 0;			
					usage = 0;
					while(usage != (BCST_RETRY +1))
					{
						CHAL_IncrementCPUsageCnt(RxFrmDesc.cpArr[i],1);
						usage = CHAL_GetCPUsageCnt(RxFrmDesc.cpArr[i]);
					}
EA = 1;	
#if 0					
					if(CHAL_GetCPUsageCnt(RxFrmDesc.cpArr[i])!= (BCST_RETRY+1))
					{
						printf("CP Usage issue %bu\n",CHAL_GetCPUsageCnt(RxFrmDesc.cpArr[i]));
					}
#endif					
				}
				for(i=0;i<BCST_RETRY;i++)
				{
					memcpy((u8*)&RxFrmDescArr[i],(u8*)&RxFrmDesc,sizeof(sCommonRxFrmSwDesc));
					Host_RxHandler(pHal, &RxFrmDescArr[i]);
				}			
			}
#endif		
        Host_RxHandler(pHal, &RxFrmDesc);
        EVENT_Free(event);
    }
    else if(event->eventHdr.trans == APP_PORT_PERIPHERAL)
    {
        APS_QueueToPeripheral(event);
    }
    else
    {
        EVENT_Free(event);
    }
}


u8 Aps_Proc(void *cookie)
{
    sEvent *event = NULL;
    sSlink *slink = NULL;
    u8      ret = 0;
    gv701x_aps_queue_t *pappSupLayer = (gv701x_aps_queue_t *)(&appSupLayer);

		/*Compiler warning suppression*/
		cookie = cookie;
	
	/*Read PLC Rx Queue*/
	while(!SLIST_IsEmpty(&(pappSupLayer->txQueue)))
	{
__CRIT_SECTION_BEGIN__
		slink = SLIST_Pop(&(pappSupLayer->txQueue));
__CRIT_SECTION_END__	
		event = SLIST_GetEntry(slink, sEvent, link);

		if(event == NULL)		
			break;
		
		Aps_ProcEvent(pappSupLayer, event);  		
	}				

    return ret;
}

#endif // NO_HOST

