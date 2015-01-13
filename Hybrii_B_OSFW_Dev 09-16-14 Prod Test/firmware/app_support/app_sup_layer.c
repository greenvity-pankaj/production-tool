#ifdef NO_HOST
#include <string.h>
#include "papdef.h"
#include "list.h"
#include "gv701x_event.h"
#include "list.h"
#include "hal_common.h"
#include "dmm.h"
#include "gv701x_aps.h"
#include "app_sup_layer.h"
#include "dmm.h"
#include "hpgpevt.h"
#include "hal_hpgp.h"
#include "hpgpdef.h"
#include "linkl.h"
#include "green.h"
#include "fm.h"
#include "mac_intf_common.h"
#include "frametask.h"
#ifdef RTX51_TINY_OS
#include <rtx51tny.h>
#endif
#include "hybrii_tasks.h"


#define APP_MEM_POOL_SIZE         4096 //6144
#define APP_MEM_SLAB_SEG_SIZE_0   32
#define APP_MEM_SLAB_SEG_SIZE_1   64
#define APP_MEM_SLAB_SEG_SIZE_2   128 
#define APP_MEM_SLAB_SEG_SIZE_3   196 
#define APP_MEM_SLAB_SEG_SIZE_4   256 
#define APP_MEM_SLAB_SEG_SIZE_5   512 
#define APP_MEM_SLAB_SEG_SIZE_6   1024

gv701x_aps_queue_t appSupLayer;
u8 XDATA AppMemPool[APP_MEM_POOL_SIZE];

static sSlabDesc AppSlabDesc[] =
{
    {10, APP_MEM_SLAB_SEG_SIZE_0},
    {0, APP_MEM_SLAB_SEG_SIZE_1},
    {22, APP_MEM_SLAB_SEG_SIZE_2},
    {0, APP_MEM_SLAB_SEG_SIZE_3},
    {2, APP_MEM_SLAB_SEG_SIZE_4},
    {0, APP_MEM_SLAB_SEG_SIZE_5},    
    {0, APP_MEM_SLAB_SEG_SIZE_6},
};

extern sDmm AppDmm;
extern u8 opMode;
extern void GV701x_TaskInit(gv701x_aps_queue_t* appQueues);
extern sHomePlugCb HomePlug;
#if 0
extern void* HPGPCTRL_GetLayer(u8 layer);
eStatus HAL_XmitMacFrame(sHaLayer *hal, sTxDesc *txInfo, 
								sBuffDesc *buffDesc);
#endif
extern void Host_RxHandler(sHaLayer *pHal, sCommonRxFrmSwDesc* pRxFrmDesc);

void GV701x_CfgGpio21(u8 enable)
{

	if (enable)
	{
		hal_common_reg_bit_set(0x518, 0x2);
	}
	else
	{
		hal_common_reg_bit_clear(0x518, 0x2);
	}
	
}


void GV701x_CfgGpio22(u8 enable)
{
	
	if (enable)
	{
		hal_common_reg_bit_set(0x518, 0x1);
	}
	else
	{
		hal_common_reg_bit_clear(0x518, 0x1);
	}

}

gv701x_aps_queue_t* Aps_init(void)
{	
	/*Create transmit queue for application data tx*/
	SLIST_Init(&appSupLayer.txQueue);

	/*Create recieve queue for sending data to application*/
	SLIST_Init(&appSupLayer.rxQueue);

	/*Create recieve queue for sending response/event to application*/
	SLIST_Init(&appSupLayer.rspEvntRxQueue);

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
	sAppEvent* event = NULL;	
	void* payload = NULL;
	u16 payloadlen;	
	volatile u8 xdata * cellAddr1stCP;

	if((src_port != PORT_PLC) && (src_port != PORT_PERIPHERAL))
	{
		CHAL_DecrementReleaseCPCnt(plcRxFrmSwDesc->cpArr[0].cp);
		return STATUS_FAILURE;
	}
	payloadlen	= plcRxFrmSwDesc->frmLen;		
	cellAddr1stCP  = CHAL_GetAccessToCP(plcRxFrmSwDesc->cpArr[0].cp);		
	payload = cellAddr1stCP;

	event = GV701x_EVENT_Alloc(payloadlen, 0);

	if(event == NULL)
	{
		CHAL_DecrementReleaseCPCnt(plcRxFrmSwDesc->cpArr[0].cp);
		gHpgpHalCB.halStats.HtoPswDropCnt++;
		return STATUS_FAILURE;
	}
    event->eventHdr.eventClass = EVENT_CLASS_DATA;
    event->eventHdr.type = 0;
	if(src_port == PORT_PLC)
	    event->eventHdr.trans = APP_PORT_PLC;
	else if(src_port == PORT_PERIPHERAL)
	    event->eventHdr.trans = APP_PORT_PERIPHERAL;
	
    if(payload)
    {
        memcpy((u8*)(event->buffDesc.dataptr), payload, payloadlen);    
	    event->buffDesc.datalen = payloadlen;
    }

    /* enqueue the event to the transmit queue */
	SLIST_Put(&(appSupLayer.rxQueue), &event->link); 	
	CHAL_DecrementReleaseCPCnt(plcRxFrmSwDesc->cpArr[0].cp);
#ifdef RTX51_TINY_OS	
	os_set_ready(HYBRII_TASK_ID_APP);
#endif	

	//Should there be a os_set_ready() call here ?
    return STATUS_SUCCESS;       
}

eStatus Aps_PostRspEventToQueue(sEvent* event)
{	
#if 0
	sEvent* appevent = NULL;	

    if((event->eventHdr.eventClass != EVENT_CLASS_MGMT) && 
		(event->eventHdr.eventClass != EVENT_CLASS_CTRL))
    	return STATUS_FAILURE;
		
	appevent = GV701x_EVENT_Alloc(event->buffDesc.datalen, 0);

	if(appevent == NULL)
		return STATUS_FAILURE;
	
    appevent->eventHdr.eventClass = event->eventHdr.eventClass;
    appevent->eventHdr.type = event->eventHdr.type;
	
    if(event->buffDesc.dataptr)
    {
        memcpy ((u8*)(appevent->buffDesc.dataptr), 
        		(u8*)(event->buffDesc.dataptr), 
        		event->buffDesc.datalen);
		appevent->buffDesc.datalen = event->buffDesc.datalen;		
    }    
	
	/* enqueue the event to the response transmit queue */
	SLIST_Put(&(appSupLayer.rspEvntRxQueue), &event->link); 	
#else

	SLINK_Init(&event->link);

    /* enqueue the event to the response transmit queue */
	SLIST_Put(&(appSupLayer.rspEvntRxQueue), &event->link); 	
#endif

#ifdef RTX51_TINY_OS	
	os_set_ready(HYBRII_TASK_ID_APP);
#endif	

	//Should there be a os_set_ready() call here ?
    return STATUS_SUCCESS;       
}

void Aps_TxData(u8 dstn_port, u8* buff, u16 len)
{
#if 0
	sTxDesc txInfo; 	
	sLinkLayer *linkl = HPGPCTRL_GetLayer(HP_LAYER_TYPE_LINK);	
	sStaInfo* staInfo = LINKL_GetStaInfo(linkl);
	sHaLayer* hal = &(HomePlug.haLayer);// HOMEPLUG_GetHal();

	len = len;
	txInfo.dtei = 0xFF;
	txInfo.snid = staInfo->snid;
	txInfo.mnbc = 1; 
	txInfo.plid = HPGP_PLID0;			
	txInfo.eks	= HPGP_UNENCRYPTED_EKS; 	
	txInfo.frameType = FRAME_TYPE_DATA;
	HAL_XmitMacFrame(hal, &txInfo, buffDesc);
#else
	if((dstn_port != APP_PORT_PLC) &&
		(dstn_port != APP_PORT_PERIPHERAL))
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
	else if(dstn_port == APP_PORT_PERIPHERAL)
	{
		sSwFrmDesc plcTxFrmSwDesc;
		u8 eth_hdr_cp = 0;
		u8 xdata* cellAddr;
		eStatus status;				

		memset(&plcTxFrmSwDesc, 0x00, sizeof(plcTxFrmSwDesc));
		plcTxFrmSwDesc.frmType = HPGP_HW_FRMTYPE_MSDU;
		plcTxFrmSwDesc.rxPort = PORT_APP;
		plcTxFrmSwDesc.txPort = PORT_PERIPHERAL;		
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

		fwdAgent_handleData(&plcTxFrmSwDesc);
	}
#endif
}

void Aps_ProcEvent(void *cookie, sAppEvent *event)
{

	if(event == NULL)		
		return;
		

	if((event->buffDesc.dataptr == NULL) || 
	  (event->buffDesc.datalen == 0) )
		return; 

	if(event->eventHdr.eventClass != EVENT_CLASS_DATA)
		return;
	
	Aps_TxData(event->eventHdr.trans, event->buffDesc.dataptr, event->buffDesc.datalen);	
}

#if 0
void Aps_ProcCmdEvent(void *cookie, sEvent *event)
{
	//cookie = cookie;
	hostHdr_t* phostHdr;
	if(event == NULL)		
		return;

	if((event->buffDesc.dataptr == NULL) || 
	  (event->buffDesc.datalen == 0) )
		return; 

	if( (event->eventHdr.eventClass != EVENT_CLASS_CTRL) &&
		(event->eventHdr.eventClass != EVENT_CLASS_MGMT) )
		return;

	phostHdr = (hostHdr_t *)(event->buffDesc.dataptr);
	
	if(phostHdr->protocol == HPGP_MAC_ID)	
	{
		//FM_Printf(FM_USER,"\nTx to PLC MAC-SAP");
		GV701x_CmdSend((hostHdr_t *)(event->buffDesc.dataptr), 
							event->buffDesc.datalen);
	}
	else if(phostHdr->protocol == IEEE802_15_4_MAC_ID)
	{
		//FM_Printf(FM_USER,"\nTx to ZB MAC-SAP");		
	}
}
#endif

u8 Aps_Proc(void *cookie)
{
    sAppEvent *event = NULL;
    sSlink *slink = NULL;
    u8      ret = 0;
    gv701x_aps_queue_t *pappSupLayer = (gv701x_aps_queue_t *)(&appSupLayer);

	/*Read PLC Rx Queue*/
	while(!SLIST_IsEmpty(&(pappSupLayer->txQueue)))
	{
__CRIT_SECTION_BEGIN__
		slink = SLIST_Pop(&(pappSupLayer->txQueue));
__CRIT_SECTION_END__	
		event = SLIST_GetEntry(slink, sAppEvent, link);

		if(event == NULL)		
			break;
		
		Aps_ProcEvent(pappSupLayer, event);  		
		GV701x_EVENT_Free(event);		   
	}				

    return ret;
}

#if 0
u8 Aps_ProcCmd(void *cookie)
{
    sEvent *event = NULL;
    sSlink *slink = NULL;
    u8      ret = 0;
    gv701x_aps_queue_t *pappSupLayer = (sappSupLayer *)cookie;
	
	/*Read PLC Rx Queue*/
	while(!SLIST_IsEmpty(&(pappSupLayer->appCmdRxQueue)))
	{
#ifdef P8051
__CRIT_SECTION_BEGIN__
#else
	SEM_WAIT(&pappSupLayer->appCmdRxQSem);
#endif
		if(HHAL_IsPlcIdle() == STATUS_FAILURE) 
		{
			return 0;
		}	

		slink = SLIST_Pop(&(pappSupLayer->appCmdRxQueue));
#ifdef P8051
__CRIT_SECTION_END__
#else
	SEM_POST(&pappSupLayer->appCmdRxQSem);
#endif
		event = SLIST_GetEntry(slink, sEvent, link);

		//FM_Printf(FM_USER,"\nRead from PLC Q");
		Aps_ProcCmdEvent(pappSupLayer, event);  		
		EVENT_Free(&AppDmm, event);		   
	}				

    return ret;
}
#endif
#endif // NO_HOST

