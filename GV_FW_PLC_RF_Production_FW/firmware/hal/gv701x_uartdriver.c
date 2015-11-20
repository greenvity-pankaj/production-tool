/** =======================================================
 * @file gv701x_uartdriver.c
 * 
 *  @brief Uart Interface Driver Module
 *
 *  Copyright (C) 2010-2014, Greenvity Communications, Inc.
 *  All Rights Reserved
 *  
 * ========================================================*/

#include <REG51.H>                /* special function register declarations   */
								   /* for the intended 8051 derivative		   */
#include <stdio.h>
#include <string.h> 
#include "papdef.h"
#include "hal_common.h"
#include "hal_hpgp.h"
#include "fm.h"
#include "gv701x_uartdriver_fw.h"
#include "utils_fw.h" 
#include "event_fw.h"
#include "hal.h"
#include "datapath.h"
#include "nma.h"
#include "timer.h"
#include "stm.h"
#ifdef NO_HOST
#include "gv701x_osal.h"
#endif
#include "gv701x_gpiodriver.h"
#include "dmm.h"
#include "gv701x_uartdriver.h"

//#pragma NOAREGS
#define TIMER_RELOAD_VALUE 	(44702)
#define TIMER_TICK10	   	(20833)
#define ARBITOR_REQ_MAX_TIMES 10

#ifdef UART_HOST_INTF

xdata volatile uint8_t rxBuffer[APP_DATA_MAX_SIZE]; 
xdata volatile uint8_t txBuffer[APP_DATA_MAX_SIZE];

//volatile u8 *urxbuf   =  (u8 xdata *)UART_RXBUF;
//volatile u8 *utxbuf   =  (u8 xdata *)UART_TXBUF;
//u8 *intid	 =  (u8 xdata *)UART_INTID;
#define UART_RX_REG 	(*(volatile u8 xdata *)(UART_RXBUF))
#define UART_TX_REG		(*(volatile u8 xdata *)(UART_TXBUF))
union_uart_linestatus *uart_linestatus = (u8 XDATA *)UART_LINESTAT;
union_uart_modemctrl  *uart_modemctrl  = (u8 XDATA *)UART_MODEMCTRL;
union_uart_linectrl   *uart_linectrl   = (u8 XDATA *)UART_LINECTRL;
union_uart_fifoctrl   *uart_fifoctrl   = (u8 XDATA *)UART_FIFOCTRL;
union_uart_intctrl    *uart_intctrl    = (u8 XDATA *)UART_INTCTRL;
union_uart_modemstat  *uart_modemstatus = (u8 XDATA *)UART_MODEMSTAT;
extern sSlist peripheralTxQ;
extern dqueue_t gDqueue[MAX_DATA_QUEUES];
extern u8 pktDbg;
#ifdef NO_HOST
extern gv701x_aps_queue_t appSupLayer;
extern sDmm AppDmm;
#endif
const uart_baudlook_t baud_lookup[11] =
{
	{ 1200,  1302},
	{ 2400,  651},
	{ 4800,  325},
	{ 9600,  162},
	{ 14400, 108},
	{ 19200, 81},
	{ 28800, 54},
	{ 38400, 40},
	{ 56000, 28},
	{ 57600, 27},
	{ 115200, 13}
};

uartRxControl_t uartRxControl; 
uartTxControl_t uartTxControl;
volatile union_uart_modemstat modemstatus_u;

volatile u8         ii;
volatile sSlink    *slink_t = NULL;
volatile sSegDesc  *segdesc_t = NULL; 
volatile sDmm* pDmm_t;  

#ifdef UART_GPIO_INTERRUPT
	volatile uartGpioStatus_t uartGpioStatus;
#endif 

#ifdef UART_LOOPBACK
#define LOOPBACK_ENABLE 1
#define LOOPBACK_DISABLE 0
u8 gLoopBack = 0;
#endif

//volatile sCommonRxFrmSwDesc lCommonRxFrmSwDesc;

extern u8 host_intf_max_cp;
 
bool datapath_IsQueueFullIsr(queue_id_e id)
{
	u8 head = gDqueue[id].head;
	u8 tail = gDqueue[id].tail;
	if ((head & 0x7F) != (tail  & 0x7F)||
			   ((head & 0x80) != (tail  & 0x80)))
	{

		// check if pending queue is full. if yes drop the frame or if not-full queue the frame

		if (((head & 0x80) != (tail  & 0x80)) &&
			((head & 0x7F) == (tail  & 0x7F)))
		{		
			return TRUE;
		}

	}	
	  return FALSE;
}

void datapath_queueIsr(queue_id_e id,
						   sSwFrmDesc *pPlcTxFrmSwDesc)
{
	u8 wrapBit;
    u8 indexHd;
	sSwFrmDesc *swDesc;

	indexHd = (gDqueue[id].head & 0x7F);		   

	swDesc = &gDqueue[id].desc[indexHd];
	
	memcpy ((void*)swDesc, (void*)pPlcTxFrmSwDesc,
		     sizeof(sSwFrmDesc)); 

	wrapBit = gDqueue[id].head & 0x80;

	swDesc->frmInfo.plc.status = 0;


	if (id == PLC_DATA_QUEUE)
	{

		host_intf_max_cp += pPlcTxFrmSwDesc->cpCount;

	}

	gDqueue[id].head = ((gDqueue[id].head & 0x7F) + 1 ) | wrapBit;
	
	if ((gDqueue[id].head & 0x7F)== MAX_Q_BUFFER)
	{
		gDqueue[id].head ^= 0x80;		  //inverse wraparound bit
		gDqueue[id].head &= 0x80;
	}

}


xdata uint8_t uartcounter;

#ifdef LG_UART_CONFIG
	volatile uint8_t txEnable = 0;
#endif
volatile uint8_t modemstatus;

/* Add a link to list head */
void SLIST_Push_Isr(struct slist *list, struct slink *link)
{
    if(list->head == NULL)
    {
        //queuee is empty
        list->head = link;
        list->tail = link;
        link->next = NULL;
    }
    else
    {
        link->next = list->head;
        list->head = link;
    }
}

/* add a link to the list tail */
void SLIST_Put_Isr(struct slist *list, struct slink *link)
{
    if(list->tail == NULL)
    {
        //queue is empty
        list->head = link;
        list->tail = link;
        link->next = NULL;
    }
    else
    {
        list->tail->next = link;
		list->tail = link;
        link->next = NULL;
    }
}
    
/* remove a link from the head of the list */
struct slink * SLIST_Pop_Isr (struct slist *list)
{
    struct slink *link = list->head;
    if( list->tail == link)
    {
        //at most one link in the queue
        list->head = NULL;
    	list->tail = NULL;
    }
    else
    {
        list->head = list->head->next;
    }
    
    return link;
}
#ifdef UART_LOOPBACK
void GV701x_SetUartLoopBack(u8 enable)
{
	if(enable)
	{
		gLoopBack = 1;
	}
	else
	{
		gLoopBack = 0;
	}
}
#endif
extern volatile uint8_t tlValue,thValue;
extern volatile uint16_t timerValue;// For Timer polling and timer reload //Kiran
extern volatile uint16_t timerDiff,timerCalc;
extern volatile sStm Stm;

void uart_handler(void) __INTERRUPT2__ using 3
{
/////////////////////////// Tx State Machine Variable Declaration ////////////////////////////
#ifndef UART_RAW
	static uint8_t txState = IDLE; // Holds state of state machine 		
	static uint16_t length = 0;
#endif
//	static uint8_t txBuff;// temporary buffer for tx data processing. Data from cp will be copied to txBuff so that it will reduce xdata access multile times
	static uint8_t inst = TRUE;// Used to identify maximum data can be pushed in FIFO.In first instance upto
							   // 16 Bytes can be pushed and onwards size depends upon tx threshold setting
	static uint16_t dataCounterTx = 0;// To hold current location of tx buffer
	static uint8_t dump =0;
//	static uint8_t txEnable = 0;
//	static uint8_t cpCounterTx = 0;
	 // Counter used to fill minimum and maximum number of characters in FIFO
	//static uint8_t accessCpTx = TRUE;
//	static uint8_t xdata *cellAddr;
	//static uint8_t lCpLen;
/////////////////////////// Tx State Machine Variable Declaration End ////////////////////////////

/////////////////////////// Rx State Machine Variable Declaration ///////////////////////////////

#ifndef UART_RAW
	static uint8_t rxState = IDLE;// Stores the state of RX State Machine	   
	static uint8_t waitEscape; // Maintains the state of incoming ESCAPE  
	
#endif

#ifdef UART_LOOPBACK
	static uint8_t loopBackLoop = 0;
	static uint8_t poolId = 0;
#endif

	//uint8_t modemstatus;
	static uint8_t rxBuff; // Temporary storage for RX FIFO
	//static uint8_t accessCpRx = TRUE;
	//	static uint8_t xdata *cellAddrRx;
	//	static uint8_t lCpLen;
	//	static uint8_t cpCounterRx = 0;
	//	static uint8_t dataCounterRx = 0;
	//	static uint16_t	   frame_len;
	//	static xdata uint8_t *cellAddrRx;
/////////////////////////// Rx State Machine Variable Declaration End ////////////////////////////	

	EA = 0;
	dump = (*(u8 xdata *)(UART_INTID)); // Failing to read this register may cause infinite unintended interrupts. 
#if 1
	if(1 == TF0)
	{
		static u8 val = 0;

		TF0 = 0;
		//Stm.timeTick++;

		if(Stm.timeTick == STM_TIME_TICK_MAX)
		{
			Stm.timeSector = !Stm.timeSector;
			Stm.timeTick = 0;
				
		}
		else
		{
			Stm.timeTick++;
		}

		TR0 = 0;
		tlValue = TL0;
		thValue = TH0;

		timerValue = (uint16_t)(thValue<<8) | tlValue;
		if(timerValue > 0)
		{
			val = timerValue / TIMER_TICK10;
			//val = (u8)timerCalc;
			//Stm.timeTick += val;
			if(Stm.timeTick == STM_TIME_TICK_MAX)
			{
				Stm.timeSector = !Stm.timeSector;
				Stm.timeTick = 0;					
			}
			else
			{
				Stm.timeTick += val;
			}
			timerCalc = timerValue % TIMER_TICK10;
			timerValue = TIMER_RELOAD_VALUE + timerCalc;
			TL0 = lo8(timerValue);
			TH0 = hi8(timerValue);
		}		
		else
		{
			val = 0;
			TL0 = lo8(TIMER_RELOAD_VALUE);
			TH0 = hi8(TIMER_RELOAD_VALUE);
		}		
		
		TR0 = 1;
		if(uartRxControl.uartRxFlag == 1) // in timer handler or in STM Proc()
		{
			uartRxControl.tick = uartRxControl.tick + ((val + 1)*10);
		}
	
	}
#endif
#if 0 //def LG_UART_CONFIG
	if(uartTxControl.txModeControl == UART_TX_LOW_EDGE)
	{
		modemstatus = ReadU8Reg(UART_MODEMSTAT);//uart_modemstatus.modemstat;
	
		if(modemstatus == 0x11) // DCTS & CTS are set
		{
		
			if(uartTxControl.txCount == 0)
			{
				// check if any frame is in queue
				//if available then unqueue copy in to local buffer and start interrupt 
				if(datapath_IsQueueEmptyIsr(HOST_DATA_QUEUE) == FALSE)
				{
					xdata uint8_t *cellAddrTx;
		 			sSwFrmDesc* pHostTxFrmSwDesc = NULL;
					uint16_t i;
					if((pHostTxFrmSwDesc =
			             	datapath_getHeadDescIsr(HOST_DATA_QUEUE, 1)) != NULL)
	    			{
						uartTxControl.pTxBuffer = txBuffer;
						uartTxControl.txCount = pHostTxFrmSwDesc->frmLen;
						uartTxControl.txDone = 0;

						cellAddrTx = CHAL_GetAccessToCP(pHostTxFrmSwDesc->cpArr[0].cp);
						for(i=0;i<uartTxControl.txCount;i++)
						{
							*(uartTxControl.pTxBuffer+i) = *(cellAddrTx + i );
						}
						CHAL_FreeFrameCp(pHostTxFrmSwDesc->cpArr, pHostTxFrmSwDesc->cpCount);
						txEnable = 1;
						uart_intctrl->intctrl_field.EnTxEmptInt = 1; // Enables tx interrupt
					}//Get head disc
				}
			}
			else
			{
				txEnable = 1;
				uart_intctrl->intctrl_field.EnTxEmptInt = 1;// Enable TX Interrupt
			}
			modemstatus = ReadU8Reg(UART_MODEMSTAT);//uart_modemstatus.modemstat;
		}
		else if(modemstatus == 0x01)// Only DCTS is set
		{
			//SBUF = 'R';
			uart_intctrl->intctrl_field.EnTxEmptInt = 0;// Disable TX Interrupt
			txEnable = 0;
			modemstatus = ReadU8Reg(UART_MODEMSTAT);//modemstatus = uart_modemstatus.modemstat;
		}
	}	
	else
	{
		txEnable = 1;
	}
#else
#ifdef UART_GPIO_INTERRUPT
	modemstatus_u.modemstat = ReadU8Reg(UART_MODEMSTAT);
	if(modemstatus_u.modemstat !=0)
	{
		if(modemstatus_u.modemstat_field.DCTS || modemstatus_u.modemstat_field.CTS)
		{
			if(modemstatus_u.modemstat_field.DCTS && modemstatus_u.modemstat_field.CTS) // DCTS & CTS are set
			{
				uartGpioStatus.field.GP_PB_IO11 = 1;
			}
			else if(modemstatus_u.modemstat_field.DCTS && !modemstatus_u.modemstat_field.CTS)// Only DCTS is set
			{
				uartGpioStatus.field.GP_PB_IO11 = 0;
			}
			modemstatus = ReadU8Reg(UART_MODEMSTAT);
		}
		if(modemstatus_u.modemstat_field.DDSR || modemstatus_u.modemstat_field.DSR)
		{
			if(modemstatus_u.modemstat_field.DDSR && modemstatus_u.modemstat_field.DSR) // DDSR & DSR are set
			{
				uartGpioStatus.field.GP_PB_IO12 = 1;
			}
			else if(modemstatus_u.modemstat_field.DDSR && !modemstatus_u.modemstat_field.DSR)// Only DDSR is set
			{
				uartGpioStatus.field.GP_PB_IO12 = 0;
			}
			modemstatus = ReadU8Reg(UART_MODEMSTAT);
		}
		if(modemstatus_u.modemstat_field.DeltaDCD || modemstatus_u.modemstat_field.DCD)
		{
			if(modemstatus_u.modemstat_field.DeltaDCD && modemstatus_u.modemstat_field.DCD) // DeltaDCD & DCD are set
			{
				uartGpioStatus.field.GP_PB_IO13 = 1;
			}
			else if(modemstatus_u.modemstat_field.DeltaDCD && !modemstatus_u.modemstat_field.DCD)// Only DeltaDCD is set
			{
				uartGpioStatus.field.GP_PB_IO13 = 0;
			}
			modemstatus = ReadU8Reg(UART_MODEMSTAT);
		}
		if(modemstatus_u.modemstat_field.TrailEdgeInd || modemstatus_u.modemstat_field.RI)
		{
			if(modemstatus_u.modemstat_field.TrailEdgeInd && modemstatus_u.modemstat_field.RI) // TrailEdgeInd & RI are set
			{
				uartGpioStatus.field.GP_PB_IO18 = 1;
			}
			else if(modemstatus_u.modemstat_field.TrailEdgeInd && !modemstatus_u.modemstat_field.RI)// Only TrailEdgeInd is set
			{
				uartGpioStatus.field.GP_PB_IO18 = 0;
			}
			modemstatus = ReadU8Reg(UART_MODEMSTAT);
		}
	
	}	
#endif
#endif 
	if((uart_linestatus->linestat_field.TxThldRegEmpt == TRUE) && (uartTxControl.txCount != 0))
	{

		uartcounter = 0;
        while((uartcounter < ((inst != TRUE) ? MIN_TX_BLOCK_SIZE:MAX_TX_BLOCK_SIZE)) 
			&& (uartTxControl.txCount != 0) 
#ifdef LG_UART_CONFIG		
			&& (txEnable == 1)
#endif			
			)
        {
			
			UART_TX_REG = txBuffer[dataCounterTx];//*(uartTxControl.pTxBuffer + dataCounterTx );
            uartcounter++;// counter for FIFO	               
            dataCounterTx++;
            uartTxControl.txCount--;
        }
			
        uartcounter = 0; 
        inst = FALSE; // Next time block of MIN_TX_BLOCK_SIZE will be pushed value based on TX FIFO INT Trigger
        if(uartTxControl.txCount == 0)
        {
#ifdef LG_UART_CONFIG	        
        	uart_fifoctrl->fifoctrl = UART_FIFO_RX_1_TX_0;// LG requires RX FIFO interrupt threshold to be 1. So UART will generate interrupt of every data byte received uart_fifoctrl->fifoctrl   = UART_FIFO_RX_1_TX_0;// LG requires RX FIFO interrupt threshold to be 1. So UART will generate interrupt of every data byte received 
#else
			uart_fifoctrl->fifoctrl = UART_FIFO_RX_8_TX_0;
#endif

			uart_intctrl->intctrl_field.EnTxEmptInt = 0;// Disable TX Interrupt 
            inst = TRUE;
            dataCounterTx = 0;
#ifdef LG_UART_CONFIG			
			txEnable = 0;
#endif
			uartTxControl.txFrameCount++;
            uartTxControl.txDone = 1;        
        }
	}// TX Interrupt processing end


	if(uart_linestatus->linestat_field.DR == TRUE)// RX State Machine
	{
			
            while(uart_linestatus->linestat_field.DR == TRUE)
            {
				if(uartRxControl.rxCount < UART_DATA_SIZE)
				{
					//*(uartRxControl.pRxdataBuffer + uartRxControl.rxCount) = (*(volatile u8 xdata *)(UART_RXBUF));
					rxBuffer[uartRxControl.rxCount] = UART_RX_REG;//(*(volatile u8 xdata *)(UART_RXBUF));
					uartRxControl.rxCount++;
				}
				else
				{
					rxBuff = UART_RX_REG;//(*(volatile u8 xdata *)(UART_RXBUF));
				}
#if 1
				if(uartRxControl.rxExpectedCount != 0)// || (uartRxControl.rxCount == UART_DATA_SIZE)
				{
					if(uartRxControl.rxCount >= uartRxControl.rxExpectedCount)
					{
						static uint16_t		i;
#ifndef NO_HOST
						static uCpuCPReg cpuCPReg;
#endif
//						u8 num12Clks;
//						u8 curTick;
						static eStatus status;
                        sEvent *event = NULL;
					    i = 0;
						uartRxControl.rxReady = 1;
			//			cpCounterRx = 0;
					
						status = STATUS_FAILURE;

#ifdef NO_HOST
                    
#ifdef UART_LOOPBACK
						for(loopBackLoop = 0;loopBackLoop < 2;loopBackLoop++)
						{
							for(poolId = FW_POOL_ID; poolId < APP_POOL_ID;poolId++)
							{
								if(loopBackLoop == 0 )
								{
#endif							
									pDmm_t = &AppDmm;
#ifdef UART_LOOPBACK								
								}
								else
								{		
									switch(poolId)
									{
										case FW_POOL_ID:
											pDmm_t = &FwDmm;
										break;
										case MGMT_POOL_ID:
											pDmm_t = &MgmtDmm;
										break;
										case BCN_POOL_ID:
											pDmm_t = &BcnDmm;
										break;
										default:
											pDmm_t = &AppDmm;
										break;
									};
								}
#endif							
								/* search for the closest memory segment */
								for (ii = 0; ii < pDmm_t->slabnum; ii++)
								{
									if ((uartRxControl.rxCount + sizeof(sEvent)) <= pDmm_t->slab[ii].segsize)
									{
										if (!SLIST_IsEmpty(&(pDmm_t->slab[ii].freelist)))
										{
											slink_t = SLIST_Pop_Isr(&(pDmm_t->slab[ii].freelist));
											
											segdesc_t = SLIST_GetEntry(slink_t, sSegDesc, link);  
											segdesc_t->poolid = APP_POOL_ID;
											pDmm_t->slab[ii].inuse++;
											if (pDmm_t->slab[ii].inuse > pDmm_t->slab[ii].maxuse)
											{
												pDmm_t->slab[ii].maxuse = pDmm_t->slab[ii].inuse;
											}
											event = (sEvent*)((u8 *)segdesc_t + sizeof(sSegDesc));
											break;
										}
									}
								}
								if(event != NULL)
								{
								  //  memset(event, 0, sizeof(sEvent) + uartRxControl.rxCount);
									event->buffDesc.buff = (u8 *)event + sizeof(sEvent);
									event->buffDesc.dataptr = event->buffDesc.buff;
									event->buffDesc.datalen = 0;
									event->buffDesc.bufflen = uartRxControl.rxCount;
									event->eventHdr.status = EVENT_STATUS_COMPLETE;
									SLINK_Init(&event->link);
			
									for(i=0;i<uartRxControl.rxCount;i++)
									{
										event->buffDesc.dataptr[i] = *(uartRxControl.pRxdataBuffer + i);
									}
								
									// datapath_queueIsr(APP_DATA_QUEUE,&uartRxControl.rxSwDesc);
									// Use Event Q
									//SLIST_Put_Isr(&peripheralRxQ, &event->link);
									event->buffDesc.datalen = uartRxControl.rxCount;
									event->eventHdr.type = DATA_FRM_ID;
									event->eventHdr.eventClass = EVENT_CLASS_DATA;
									event->eventHdr.trans = APP_PORT_PERIPHERAL;
#ifdef UART_LOOPBACK								
									if(loopBackLoop == 0)
									{	
#endif								
										uartRxControl.lastRxCount = uartRxControl.rxCount;// UART RX stat variable
										uartRxControl.rxFrameCount++;
										
										SLIST_Put_Isr(&(appSupLayer.rxQueue), &event->link);
#ifdef UART_LOOPBACK									
									}
									else
									{
										SLIST_Push_Isr(&(peripheralTxQ), &event->link);// Adds to list head
									}
#endif								
								}
								else
								{
#ifdef UART_LOOPBACK							
									if(loopBackLoop == 0)
									{
#endif								
										uartRxControl.rxFrameLoss++;
										uartRxControl.rxLossSoftQ++;
										gHpgpHalCB.halStats.HtoPswDropCnt++;
#ifdef UART_LOOPBACK									
										break;// Break the loop as app pool is not available. 
											  //Frame should be dropped and firmware should not do loopback
									}
#endif								
								}
#ifdef UART_LOOPBACK								
							}
							if(gLoopBack == LOOPBACK_DISABLE)
							{
								break;
							}	
	                    }
#endif						
						uartRxControl.rxCount = 0;//
						uartRxControl.rxReady = 0;//Add after complete loop                    
#else // NO_HOST
					if (HHAL_Req_Gnt_Read_CPU_QD_ISR() == STATUS_SUCCESS)
					{
						if (datapath_IsQueueFullIsr(PLC_DATA_QUEUE))// Discard Packet if Queue is Full
						{
							uartRxControl.rxSwDesc.cpCount = 0;
							uartRxControl.rxCount = 0;
							uartRxControl.rxReady = 0;
							uartRxControl.rxFrameLoss++;
							uartRxControl.rxLossSoftQ++;
							gHpgpHalCB.halStats.HtoPswDropCnt++;
							//dataCounterRx = 0;
						}
						else
						//status = CHAL_RequestCP(&uartRxControl.uartRxSwDesc->cpArr[0].cp);
						///////////////////////////////////
						{
						    u8 reqCp;
                            u16 tmpLen;
                            u8 descLen;
                            u16 indx = 0;
							uFreeCpCntReg  freeCpCnt;
								 
							freeCpCnt.reg = ReadU32Reg(CPU_FREECPCOUNT_REG);
                            reqCp = (uartRxControl.rxCount/HYBRII_CELLBUF_SIZE)+1;
                            
                            if(freeCpCnt.s.cpCnt >= reqCp)
                            {
                                uartRxControl.rxSwDesc.cpCount = 0;
                                uartRxControl.rxSwDesc.frmLen =  uartRxControl.rxCount;
                                tmpLen = uartRxControl.rxCount;
                                ii = 0;
                                while(tmpLen)
                                {
                                    status = STATUS_FAILURE;
                                    if(tmpLen > HYBRII_CELLBUF_SIZE)
                                    {
                                        descLen = HYBRII_CELLBUF_SIZE;
                                    }
                                    else
                                    {
                                        descLen = tmpLen;
										uartRxControl.rxSwDesc.lastDescLen = descLen;
                                    }
							        for (i = 0; i < ARBITOR_REQ_MAX_TIMES; i++)
        							{
        								cpuCPReg.reg = ReadU32Reg(CPU_REQUESTCP_REG);
        					
        								if(cpuCPReg.s.cp == 0xFF)
        								{
        									// no more free CP
        									// set bit 31 to relinquish the grant request
        									// and then return error
        									break;
        								}
        					
        								if(cpuCPReg.s.cpValid)
        								{
        									// alloc CP is valid
        									uartRxControl.rxSwDesc.cpArr[ii].cp = (u8) cpuCPReg.s.cp;
        									status = (STATUS_SUCCESS);
        									break;
        								}
        								CHAL_DelayTicks_ISR(10);
        								// give HW a chance to grant a CP
        								#if 0
        								num12Clks = 10;
        								do
        								{
        									curTick = TL0;
        									while(curTick == TL0);
        								}while(num12Clks--);
        								#endif
        								/////////////////////////////////////
        							}// for 
        							///////////////////////////////////////////////////////
        							if(status == STATUS_FAILURE)
        							{	
        								uartRxControl.rxSwDesc.cpCount = 0;
        								cpuCPReg.reg = 0;
        								cpuCPReg.s.cpValid = 1;
        								WriteU32Reg(CPU_REQUESTCP_REG, cpuCPReg.reg);
        								uartRxControl.rxCount = 0;
        								uartRxControl.rxReady = 0;
        								//dataCounterRx = 0;
        								// SBUF='Y';
        								gHalCB.cp_no_grant_alloc_cp++;
        								uartRxControl.rxFrameLoss++;
                                        break;
        							}	
        							else
        							{

        								uartRxControl.rxSwDesc.cpCount++;
        								uartRxControl.rxSwDesc.cpArr[ii].len =  descLen;        									
        								uartRxControl.pCellAddrRx = CHAL_GetAccessToCP(uartRxControl.rxSwDesc.cpArr[ii].cp);
        								uartRxControl.rxSwDesc.cpArr[ii].offsetU32 = 0;
        								for(i=0;i<descLen;i++)
        								{
        									*(uartRxControl.pCellAddrRx + i ) = uartRxControl.pRxdataBuffer[indx+i];
        								}
        								tmpLen -= descLen;
                                        indx += descLen;
                                        ii++;
                                        
        					           
        							}
                                }
                                if(status == STATUS_SUCCESS)
                                {
                                    uartRxControl.lastRxCount = uartRxControl.rxCount;
                                    uartRxControl.rxSwDesc.frmInfo.plc.dtei = 0;
    								uartRxControl.rxSwDesc.frmType = DATA_FRM_ID;
    								uartRxControl.rxSwDesc.txPort = PORT_PLC;    								
    								datapath_queueIsr(PLC_DATA_QUEUE,&uartRxControl.rxSwDesc);
    								uartRxControl.rxFrameCount++;
    								uartRxControl.rxCount = 0;
    								uartRxControl.rxReady = 0;
                                }
						    }
                            else
        					{
        						uartRxControl.rxSwDesc.cpCount = 0;
    							uartRxControl.rxCount = 0;
    							uartRxControl.rxReady = 0;
    							uartRxControl.rxFrameLoss++;
    							gHpgpHalCB.halStats.HtoPswDropCnt++;
        					}
                        }
						HHAL_Rel_Gnt_Read_CPU_QD_ISR();
					}	
					else
					{
						uartRxControl.cpuGrantfail++;
						uartRxControl.rxFrameLoss++;
						uartRxControl.rxCount = 0;
						uartRxControl.rxReady = 0;
					}
#endif // NO_HOST
				}
#endif		

			}
		
			if(uartRxControl.rxExpectedCount == 0)
			{
				uartRxControl.uartRxFlag = 1;
				uartRxControl.tick = 0;
			}
			else
			{
				uartRxControl.uartRxFlag = 0;
			}
		
		
		}
	}
	//dump = *intid;//*(u8 xdata *)UART_INTID; // Failing to read this register may cause infinite unintended interrupts. 
	if(uartTxControl.txCount == 0)
    {
#ifdef LG_UART_CONFIG	        
		uart_fifoctrl->fifoctrl = UART_FIFO_RX_1_TX_0;// LG requires RX FIFO interrupt threshold to be 1. So UART will generate interrupt of every data byte received uart_fifoctrl->fifoctrl	= UART_FIFO_RX_1_TX_0;// LG requires RX FIFO interrupt threshold to be 1. So UART will generate interrupt of every data byte received 
#else
		uart_fifoctrl->fifoctrl = UART_FIFO_RX_8_TX_0;
#endif
  		uart_intctrl->intctrl_field.EnTxEmptInt = 0;// Disable TX Interrupt 
       
    }
	else
	{
		uart_intctrl->intctrl = 0;// Disable Interrupt
		uart_intctrl->intctrl = 3;// Enable RX TX Interrupt 
	}
	EA = 1;
}

//sSwFrmDesc uplcTxFrmSwDesc;

void uartRxProc()
{
#ifndef NO_HOST
	eStatus status;
#endif
	if(uartRxControl.uartRxFlag == 1)
	{
		EA = 0;
		if(uartRxControl.tick >= uartRxControl.timeout)
		{
#ifdef NO_HOST

		    u16 i;
            sEvent *event = NULL;
#ifdef UART_LOOPBACK
			sEvent *eventLoopBack = NULL;
#endif
#endif
			uartRxControl.rxReady = 1;
#ifdef NO_HOST
#if 1
            event = GV701x_EVENT_Alloc(uartRxControl.rxCount,0);

            if (event == NULL)//GetEventIsr(event))// Discard Packet if Queue is Full
            {
                uartRxControl.rxFrameLoss++;
                uartRxControl.rxLossSoftQ++;
                gHpgpHalCB.halStats.HtoPswDropCnt++;
            }
            else
            {
            	SLINK_Init(&event->link);
                event->buffDesc.datalen = uartRxControl.rxCount;
                event->eventHdr.type = DATA_FRM_ID;
                event->eventHdr.trans = APP_PORT_PERIPHERAL;
                event->eventHdr.eventClass = EVENT_CLASS_DATA;				
				uartRxControl.lastRxCount = uartRxControl.rxCount;// UART RX stat variable	
                for(i=0;i<uartRxControl.rxCount;i++)
                {
                    event->buffDesc.dataptr[i] = rxBuffer[i]; //*(uartRxControl.pRxdataBuffer + i);
                }              
//                        datapath_queueIsr(APP_DATA_QUEUE,&uartRxControl.rxSwDesc);
                // Use Event Q
               // SLIST_Put(&peripheralRxQ, &event->link);
                SLIST_Put_Isr(&(appSupLayer.rxQueue), &event->link);

                uartRxControl.rxFrameCount++;     
            }
#endif
#ifdef UART_LOOPBACK
			
			if(gLoopBack == LOOPBACK_ENABLE)
			{
				eventLoopBack = EVENT_Alloc(uartRxControl.rxCount,0);
				if(eventLoopBack != NULL)
				{
					//SLINK_Init(&eventLoopBack->link);
	                eventLoopBack->buffDesc.datalen = uartRxControl.rxCount;
	                eventLoopBack->eventHdr.type = DATA_FRM_ID;
	                eventLoopBack->eventHdr.trans = APP_PORT_PERIPHERAL;
	                eventLoopBack->eventHdr.eventClass = EVENT_CLASS_DATA;				
	                for(i=0;i<uartRxControl.rxCount;i++)
	                {
	                    eventLoopBack->buffDesc.dataptr[i] = rxBuffer[i]; //*(uartRxControl.pRxdataBuffer + i);
	                }           
	                SLIST_Push_Isr(&(peripheralTxQ), &eventLoopBack->link);// Adds to list head
	            }
			}
#endif

			
#else // NO_HOST
			if (datapath_IsQueueFullIsr(PLC_DATA_QUEUE))// Discard Packet if Queue is Full
			{
				uartRxControl.rxSwDesc.cpCount = 0;
				uartRxControl.rxFrameLoss++;
				uartRxControl.rxLossSoftQ++;
				gHpgpHalCB.halStats.HtoPswDropCnt++;
			}
			else
			{
			    u8 reqCp;
                u16 tmpLen;
                u8 descLen;
                u16 indx = 0;
				u8 cpi = 0;
				u8 i;
				uFreeCpCntReg  freeCpCnt;
					 
				freeCpCnt.reg = ReadU32Reg(CPU_FREECPCOUNT_REG);
                reqCp = (uartRxControl.rxCount/HYBRII_CELLBUF_SIZE)+1;
                
                if(freeCpCnt.s.cpCnt >= reqCp)
                {
                    uartRxControl.rxSwDesc.cpCount = 0;
                    uartRxControl.rxSwDesc.frmLen =  uartRxControl.rxCount;
                    tmpLen = uartRxControl.rxCount;
                    while(tmpLen)
                    {
                        status = STATUS_FAILURE;
                        if(tmpLen > HYBRII_CELLBUF_SIZE)
                        {
                            descLen = HYBRII_CELLBUF_SIZE;
                        }
                        else
                        {
                            descLen = tmpLen;
							uartRxControl.rxSwDesc.lastDescLen = descLen;
                        }
				        
						status = CHAL_RequestCP(&uartRxControl.rxSwDesc.cpArr[cpi].cp);
				        
						///////////////////////////////////////////////////////
						if(status == STATUS_FAILURE)
						{	
							uartRxControl.rxSwDesc.cpCount = 0;
							uartRxControl.rxCount = 0;
							uartRxControl.rxReady = 0;
							//dataCounterRx = 0;
							// SBUF='Y';
							gHalCB.cp_no_grant_alloc_cp++;
							uartRxControl.rxFrameLoss++;
                            break;
						}	
						else
						{

							uartRxControl.rxSwDesc.cpCount++;
							uartRxControl.rxSwDesc.cpArr[cpi].len =  descLen;        									
							uartRxControl.pCellAddrRx = CHAL_GetAccessToCP(uartRxControl.rxSwDesc.cpArr[cpi].cp);
							uartRxControl.rxSwDesc.cpArr[cpi].offsetU32 = 0;
							for(i=0;i<descLen;i++)
							{
								*(uartRxControl.pCellAddrRx + i ) = uartRxControl.pRxdataBuffer[indx+i];
							}
							tmpLen -= descLen;
                            indx += descLen;
                            cpi++;
                            
						}
                    }
                    if(status == STATUS_SUCCESS)
                    {
                        uartRxControl.lastRxCount = uartRxControl.rxCount;
                        uartRxControl.rxSwDesc.frmInfo.plc.dtei = 0;
						uartRxControl.rxSwDesc.frmType = DATA_FRM_ID;
						uartRxControl.rxSwDesc.txPort = PORT_PLC;    								
						datapath_queueIsr(PLC_DATA_QUEUE,&uartRxControl.rxSwDesc);
						uartRxControl.rxFrameCount++;
						//uartRxControl.rxCount = 0;
						//uartRxControl.rxReady = 0;
                    }
			    }
                else
				{
					uartRxControl.rxSwDesc.cpCount = 0;
					//uartRxControl.rxCount = 0;
					//uartRxControl.rxReady = 0;
					uartRxControl.rxFrameLoss++;
					gHpgpHalCB.halStats.HtoPswDropCnt++;
				}
            }			
#endif // NO_HOST
			uartRxControl.rxCount = 0;
			uartRxControl.rxReady = 0;
			uartRxControl.uartRxFlag = 0;
			
		}
		EA = 1;
	}
}

u8 GV701x_SetUartRxTimeout(u32 timeout)
{
	if(timeout >= 10)
	{
		uartRxControl.timeout = timeout;
		return STATUS_SUCCESS;	
	}
	else
	{
		return STATUS_FAILURE;
	}		
}

#ifdef LG_UART_CONFIG
void GV701x_UartTxMode(u8 mode)
{
	if(mode == UART_TX_AUTO)
	{
		uartTxControl.txModeControl = UART_TX_AUTO;
		txEnable = 1;
		uart_intctrl->intctrl    = 0x01;
	}
	else if(mode == UART_TX_LOW_LEVEL)
	{
		uartTxControl.txModeControl = UART_TX_LOW_LEVEL;
		txEnable = 1;
		uart_intctrl->intctrl    = 0x01;
	}
	else if(mode == UART_TX_LOW_EDGE)
	{
		uartTxControl.txModeControl = UART_TX_LOW_EDGE;
		txEnable = 0;
		uart_intctrl->intctrl	 = 0x01; // 0x09 in case of interrupts enabled
	}
}
#endif

u8 GV701x_UartConfig(u32 baudrate, u16 rxthreshold)
{
	u8 idx;

	if((rxthreshold > UART_DATA_SIZE))
	{
		return STATUS_FAILURE;			
	}
	
	//uartRxControl.uartRxFlag = 0;
	uartRxControl.rxExpectedCount = rxthreshold;
	uartRxControl.rxCount = 0;
	uartRxControl.rxReady = 0;
	uartRxControl.tick = 0;
	uartTxControl.txCount = 0;
	uartTxControl.txDone = 1;
	for(idx = 0; idx < 11; idx++)
	{
		if(baud_lookup[idx].baud_no == baudrate)		
		{
			hal_common_reg_32_write(UART_CLKDIV,
				baud_lookup[idx].baudrate);			
			return STATUS_SUCCESS;
		}
	}
	return STATUS_FAILURE;	
}

void UART_Init16550()
{
	u8 val;
	
	uart_linectrl_t lineParam;
		
#ifdef NO_HOST
   	hal_common_reg_32_write(UART_CLKDIV,BAUDRATE_9600);
#else
	hal_common_reg_32_write(UART_CLKDIV,BAUDRATE_115200);
#endif

    /* 1-char depth tx/rx buffer, reset tx/rx buffer */
#ifdef NO_HOST	
    uart_fifoctrl->fifoctrl   = UART_FIFO_RX_1_TX_8;// LG requires RX FIFO interrupt threshold to be 1. So UART will generate interrupt of every data byte received	  
#else
	uart_fifoctrl->fifoctrl   = UART_FIFO_RX_8_TX_8;// 8 Bytes TX & RX FIFO interrupt threshold for higer baudrate
#endif	
    /* word length = 8 */
    //uart_linectrl->linectrl   = 0x03;	 
   
   	lineParam.linectrl                   = 0;
	lineParam.linectrl_field.WdLen       = UART_WORD_SIZE_8;
	lineParam.linectrl_field.StopBit     = UART_STOPBIT_1;
	lineParam.linectrl_field.ParityEn    = UART_PARITY_DISABLE;
	lineParam.linectrl_field.EvenParity  = UART_PARITY_ODD;
	lineParam.linectrl_field.ForceParity = UART_FORCEPAR_DISABLE;
	lineParam.linectrl_field.ForceTx0    = UART_SETBREAK_DISABLE;
	
    GV701x_SetUartLineParam(lineParam);
	
    /* No loopback */
    uart_modemctrl->modemctrl = 0x00;     
	
	uartRxControl.rxExpectedCount = 100;
	uartRxControl.rxCount = 0;
	uartRxControl.rxReady = 0;

#ifndef UART_RAW 
	uartRxControl.crcRx = 0;
	uartTxControl.crcTx = 0;
	uartRxControl.rxDropCount = 0;
	uartRxControl.goodRxFrmCnt = 0;
#endif	
	uartRxControl.pRxdataBuffer = rxBuffer;	
	uartTxControl.txCount = 0;
	uartTxControl.txDone = 1;
	uartRxControl.tick = 0;
	uartRxControl.uartRxFlag = 0;
	uartRxControl.lastRxCount = 0;
	uartRxControl.cpuGrantfail = 0;
	uartRxControl.rxFrameLoss = 0;
	uartRxControl.rxFrameCount = 0;
	uartTxControl.txFrameCount = 0;
	uartRxControl.rxLossSoftQ = 0;
	GV701x_SetUartRxTimeout(100);
#if 0		
	if(uartRxAllocCP(MAX_RX_RSVD_CP_CNT) == STATUS_FAILURE) // old architecture was allocating CP in advance
	{
		//FM_Printf(FM_USER,"Failed to alloc UART RX CP\n");
	}
#endif	
	
	
	val = uart_linestatus->linestatus;
#ifdef LG_UART_CONFIG
	uartTxControl.txModeControl  = UART_TX_AUTO;
#endif	
	uart_intctrl->intctrl     = 0x01;// 0x09 incase of #ifdef UART_GPIO_INTERRUPT     
	
}

void UART_EnableTxInt()
{
	uart_intctrl->intctrl_field.EnTxEmptInt = 1;
}

void UART_DisableTxInt()
{
	uart_intctrl->intctrl_field.EnTxEmptInt = 0;
}
#if 0 // for maven systems

void uartRxConfig(uint8_t *pBuffer, uint16_t expRxLen)
{
	uartRxControl.pRxdataBuffer = pBuffer;
	uartRxControl.rxExpectedCount = expRxLen;
	uartRxControl.rxCount = 0;
	uartRxControl.rxReady = 0;
}

void uartTx(uint8_t *pBuffer, uint16_t txLen)
{
	uartTxControl.pTxBuffer = pBuffer;
	uartTxControl.txCount = txLen;
	uartTxControl.txDone = 0;
	UART_EnableTxInt();
}
#endif

#if 1


bool hal_uart_tx_cp (sSwFrmDesc * pHostTxFrmSwDesc)

{
	u8 intFlag=0,i, cpi, cellLen;
    u16 tmpLen = 0, txcnt;
	
	xdata uint8_t *cellAddrTx;
	intFlag = EA;
	EA = 0;
	//memcpy((uint8_t*)&uartTxControl.uartTxFrmSwDesc,(uint8_t *)pHostTxFrmSwDesc,sizeof(sSwFrmDesc));
	
	uartTxControl.pTxBuffer = txBuffer;
	uartTxControl.txCount = pHostTxFrmSwDesc->frmLen;
	uartTxControl.txDone = 0;
	//if (HHAL_Req_Gnt_Read_CPU_QD() == STATUS_SUCCESS)
	txcnt = uartTxControl.txCount;
	for(cpi = 0; cpi < pHostTxFrmSwDesc->cpCount; cpi++)
	{
	    if(txcnt >= HYBRII_CELLBUF_SIZE)
        {
            cellLen = HYBRII_CELLBUF_SIZE;
        }
        else
        {
            cellLen = txcnt;
        }
		cellAddrTx = CHAL_GetAccessToCP(pHostTxFrmSwDesc->cpArr[cpi].cp);
		for(i=0;i<cellLen;i++)
		{
			*(uartTxControl.pTxBuffer+i+tmpLen) = *(cellAddrTx + i );
		}
        tmpLen += cellLen;
        if(uartTxControl.txCount <= tmpLen)
        {
            break;
        }
		//HHAL_Rel_Gnt_Read_CPU_QD();
	}	
    uart_intctrl->intctrl_field.EnTxEmptInt = 1; // enables tx interrupt
	
	EA = intFlag;
	return STATUS_FAILURE;
}

bool hal_uart_tx (sEvent * event)

{
	u8 intFlag=0;	
//	xdata uint8_t *cellAddrTx;
	intFlag = EA;
	EA = 0;
	//memcpy((uint8_t*)&uartTxControl.uartTxFrmSwDesc,(uint8_t *)pHostTxFrmSwDesc,sizeof(sSwFrmDesc));
#ifdef LG_UART_CONFIG	
	if(uartTxControl.txModeControl == UART_TX_AUTO)
	{
		txEnable = 1;
	}
#endif	
#ifdef NO_HOST
	uart_fifoctrl->fifoctrl   = UART_FIFO_RX_1_TX_8;
#else
	uart_fifoctrl->fifoctrl   = UART_FIFO_RX_8_TX_8;
#endif
	uartTxControl.pTxBuffer = txBuffer;
	uartTxControl.txCount = event->buffDesc.datalen;
	uartTxControl.txDone = 0;
	memcpy(uartTxControl.pTxBuffer, event->buffDesc.dataptr, event->buffDesc.datalen);
	uart_intctrl->intctrl_field.EnTxEmptInt = 1; // enables tx interrupt
	
	EA = intFlag;
	return STATUS_FAILURE;
}

#else
bool hal_uart_tx_cp (sSwFrmDesc * pHostTxFrmSwDesc)

{
	u8 intFlag=0;
	intFlag = EA;
	EA = 0;
	memcpy((uint8_t*)&uartTxControl.uartTxFrmSwDesc,(uint8_t *)pHostTxFrmSwDesc,sizeof(sSwFrmDesc));
	uartTxControl.txCount = pHostTxFrmSwDesc->frmLen;
	uartTxControl.txDone = 0;
	uart_intctrl->intctrl_field.EnTxEmptInt = 1; // enables tx interrupt
	EA = intFlag;
	return STATUS_SUCCESS;
}

#endif

#if 0

eStatus uartRxAllocCP(uint8_t cpNum)
{
	eStatus		status = STATUS_FAILURE;
	uint8_t		uartCP = 0;
	uint8_t		i,j;
	//uint8_t 	firstCP;
		
	//memset(&uartRxControl.uartRxFrmSwDesc, 0x00, sizeof(sCommonRxFrmSwDesc));
	if(datapath_IsQueueFull(PLC_DATA_QUEUE)==FALSE)
	{
		//uartRxControl.uartRxSwDesc = datapath_fetchHeadIsr(PLC_DATA_QUEUE);
			
		if(CHAL_GetFreeCPCnt() >= cpNum)
		{
			for(i=0;i<cpNum;i++)
			{
				if(i==0)
				{
					status = CHAL_RequestCP(&uartRxControl.rxSwDesc.cpArr[0].cp);
					if(status == STATUS_FAILURE)
					{	//FM_Printf(FM_USER,"Failed to alloc first CP\n");

						SBUF = 'E';
						uartRxControl.rxSwDesc.cpCount = 0;
						break;
					}	
				}	
				else
				{
					status = CHAL_RequestCP(&uartRxControl.rxSwDesc.cpArr[i].cp);//-1
					if(status == STATUS_FAILURE)
					{	
						CHAL_DecrementReleaseCPCnt(uartRxControl.rxSwDesc.cpArr[0].cp);
						for(j=1;j<=i;j++)
						{
							CHAL_DecrementReleaseCPCnt(uartRxControl.rxSwDesc.cpArr[j].cp);//in case of failure frees previously allocated cps
						}
						break;
					}
				}
							
			}
		}
		else
		{
			uartRxControl.rxSwDesc.cpCount = 0;
			//FM_Printf(FM_USER,"NO CP A\n");
			return STATUS_FAILURE;
		}
	}
	if(status == STATUS_SUCCESS)
	{
		//CHAL_IncrementCPUsageCnt(uartRxControl.rxSwDesc.cpArr[0].cp, 2); 
		
		uartRxControl.rxSwDesc.cpCount = cpNum;// This will help uart driver to identify cp availability during rx
		//FM_Printf(FM_USER,"CP Alloc Success\n");
	}
	return status;
}

#endif

bool hal_uart_isTxReady()
{
#ifdef LG_UART_CONFIG
	if(uartTxControl.txModeControl == UART_TX_AUTO)
	{
		//txEnable = 1;
		return (uartTxControl.txDone & uart_linestatus->linestat_field.TxThldRegEmpt);
	}
	else if(uartTxControl.txModeControl == UART_TX_LOW_LEVEL)
	{
		union_uart_modemstat modemstatus_u;
		modemstatus_u.modemstat = ReadU8Reg(UART_MODEMSTAT);
		if(modemstatus_u.modemstat_field.CTS) // DCTS & CTS are set
		{
			txEnable = 1;
			return (uartTxControl.txDone & uart_linestatus->linestat_field.TxThldRegEmpt);
		}
		else //if(modemstatus_u.modemstat_field.DCTS && !modemstatus_u.modemstat_field.CTS)
		{
			txEnable = 0;
			return FALSE;
		}
	}
	else
	{
		return FALSE;
	}
#else
	return uartTxControl.txDone;
#endif
}
#if 0
char getChar16550()
{
	return *urxbuf;
}

void putChar16550(u8 db)
{
	*utxbuf = db;
}
#endif
u8 txReady()
{
	return (uart_linestatus->linestat_field.TxThldRegEmpt);
}

u8 rxDataReady()
{
	return (uart_linestatus->linestat_field.DR);
}

#ifdef UART_HOST_INTF

u8 GV701x_UART_GPIO_Read(uint8_t gpio)
{
	if(modemstatus_u.modemstat & gpio)
	{
		return TRUE;
	}
	else
	{
		return FALSE;
	}
}

void GV701x_SetUARTParity(u8 pen,u8 eps)
{
	if(pen<2 && eps<2)
	{
		uart_linectrl->linectrl_field.ParityEn = pen;
		uart_linectrl->linectrl_field.EvenParity = eps;
	}
	else
	{
		uart_linectrl->linectrl_field.ParityEn = 0;
		uart_linectrl->linectrl_field.EvenParity = 0;
	}
	
	uart_linectrl->linectrl_field.WdLen = 0x03;
}

void GV701x_SetUartLineParam(union_uart_linectrl lineParam)
{
	uart_linectrl->linectrl = lineParam.linectrl;
}

#endif
#if 1
void CHAL_DelayTicks_ISR(u32 num12Clks) //reentrant
{
#ifdef P8051
    u8 curTick;
    do
    {
        curTick = TL0;
        while(curTick == TL0);
    }while(num12Clks--);
#endif
}

u8 XDATA* CHAL_GetAccessToCP_ISR( u8 cp)
{
    u8 XDATA *cellAddr;
    u8   bank;
    uPktBufBankSelReg pktBufBankSel;

    bank     = (cp & 0x60) >>5;

    pktBufBankSel.reg = ReadU32Reg(CPU_PKTBUFBANKSEL_REG);
    pktBufBankSel.s.bank = bank;
    WriteU32Reg(CPU_PKTBUFBANKSEL_REG, pktBufBankSel.reg);
    cellAddr = (u8 XDATA *) ((u32)MAC_PKTBUF_BASEADDR+ (u32)( (((u32)cp) & 0x1F)<<7) );
    return cellAddr;
}

eStatus HHAL_Req_Gnt_Read_CPU_QD_ISR()
{
    u16 i;

    uPlc_CpuQD_Wr_Arb_Req Wr_Arb_Req;

    // Set CPUQD_Write_Req
    Wr_Arb_Req.reg = 0;
    Wr_Arb_Req.s.cpuQDArbReq = 1;
    WriteU32Reg(PLC_CPUQDWRITEARB_REG,Wr_Arb_Req.reg);
    CHAL_DelayTicks_ISR(50);

    //Check if we get a grant
    for (i = 0; i < ARBITOR_REQ_MAX_TIMES; i++)
    {
        Wr_Arb_Req.reg = ReadU32Reg(PLC_CPUQDWRITEARB_REG); 
        if (Wr_Arb_Req.s.cpuQDArbGntStat)
            return(STATUS_SUCCESS);
        CHAL_DelayTicks_ISR(10);
    }
    gHalCB.qc_no_grant++;
    return(STATUS_FAILURE);
}

void HHAL_Rel_Gnt_Read_CPU_QD_ISR()
{
    uPlc_CpuQD_Wr_Arb_Req Wr_Arb_Req;
    // Clear CPUQD_Write_Req
    Wr_Arb_Req.reg = 0;
    WriteU32Reg(PLC_CPUQDWRITEARB_REG,Wr_Arb_Req.reg);
}
#endif
#endif


