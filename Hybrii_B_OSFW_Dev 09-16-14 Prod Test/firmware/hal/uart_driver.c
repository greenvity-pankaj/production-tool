/** =========================================================
 *
 *  @file uart_driver.c
 * 
 *  @brief UART Host Interface Driver Module
 *
 *  Copyright (C) 2010-2012, Greenvity Communications, Inc.
 *  All Rights Reserved
 *  
 * ==========================================================*/

#include <REG51.H>                /* special function register declarations   */
								   /* for the intended 8051 derivative		   */
#include <stdio.h>
#include <string.h> 
#include "papdef.h"
#include "hal_common.h"
#include "hal_hpgp.h"
#include "fm.h"
#include "uart_driver.h"
#include "utils.h" 
#include "hal.h"
#include "datapath.h"
#include "mac_intf_common.h"
//#include "list.h"
#include "timer.h"
#include "stm.h"
#if ((defined NO_HOST) || (defined UART_HOST_INTF)) 
#include "gv701x_aps.h"
#endif
//#pragma NOAREGS

#define ARBITOR_REQ_MAX_TIMES 10

#ifdef UART_HOST_INTF

//u8 *baud_gen =  (u8 xdata *)UART_CLKDIV;
u8 *urxbuf   =  (u8 xdata *)UART_RXBUF;
u8 *utxbuf   =  (u8 xdata *)UART_TXBUF;
u8 *intid	 =  (u8 xdata *)UART_INTID;
union_uart_linestatus *uart_linestatus = (u8 XDATA *)UART_LINESTAT;
union_uart_modemctrl  *uart_modemctrl  = (u8 XDATA *)UART_MODEMCTRL;
union_uart_linectrl   *uart_linectrl   = (u8 XDATA *)UART_LINECTRL;
union_uart_fifoctrl   *uart_fifoctrl   = (u8 XDATA *)UART_FIFOCTRL;
union_uart_intctrl    *uart_intctrl    = (u8 XDATA *)UART_INTCTRL;
union_uart_modemstat  *uart_modemstatus = (u8 XDATA *)UART_MODEMSTAT;

xdata volatile uint8_t rxBuffer[128];
 
xdata volatile uint8_t txBuffer[128];

extern dqueue_t gDqueue[MAX_DATA_QUEUES];
extern u8 pktDbg;
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
union_uart_modemstat modemstatus_u;

#ifdef UART_GPIO_INTERRUPT
	volatile uartGpioStatus_t uartGpioStatus;
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

			*utxbuf = *(uartTxControl.pTxBuffer + dataCounterTx );
            uartcounter++;// counter for FIFO	               
            dataCounterTx++;
            uartTxControl.txCount--;
        }
			
        uartcounter = 0; 
        inst = FALSE; // Next time block of MIN_TX_BLOCK_SIZE will be pushed value based on TX FIFO INT Trigger
        if(uartTxControl.txCount == 0)
        {
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
					*(uartRxControl.pRxdataBuffer + uartRxControl.rxCount) = *urxbuf;
					uartRxControl.rxCount++;
				}
				else
				{
					rxBuff = *urxbuf;
				}
#if 1
				if(uartRxControl.rxExpectedCount != 0)// || (uartRxControl.rxCount == UART_DATA_SIZE)
				{
					if(uartRxControl.rxCount >= uartRxControl.rxExpectedCount)
					{
						uint8_t		i;
						uCpuCPReg cpuCPReg;
//						u8 num12Clks;
//						u8 curTick;
						eStatus status;
					
						uartRxControl.rxReady = 1;
			//			cpCounterRx = 0;
					
						status = STATUS_FAILURE;
					if (HHAL_Req_Gnt_Read_CPU_QD_ISR() == STATUS_SUCCESS)
					{
#ifdef NO_HOST
						if (datapath_IsQueueFullIsr(APP_DATA_QUEUE))// Discard Packet if Queue is Full
#else
						if (datapath_IsQueueFullIsr(PLC_DATA_QUEUE))// Discard Packet if Queue is Full
#endif
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
							uFreeCpCntReg  freeCpCnt;
								 
							freeCpCnt.reg = ReadU32Reg(CPU_FREECPCOUNT_REG);
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
									uartRxControl.rxSwDesc.cpArr[0].cp = (u8) cpuCPReg.s.cp;
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
							}	
							else
							{

								uartRxControl.rxSwDesc.cpCount = 1;
								uartRxControl.rxSwDesc.cpArr[0].len =  uartRxControl.rxCount;

								uartRxControl.rxSwDesc.frmLen =  uartRxControl.rxCount;
								uartRxControl.lastRxCount = uartRxControl.rxCount;	
								uartRxControl.pCellAddrRx = CHAL_GetAccessToCP(uartRxControl.rxSwDesc.cpArr[0].cp);
								uartRxControl.rxSwDesc.cpArr[0].offsetU32 = 0;
								for(i=0;i<uartRxControl.rxCount;i++)
								{
									*(uartRxControl.pCellAddrRx + i ) = *(uartRxControl.pRxdataBuffer + i);
								}
#ifdef NO_HOST				
								datapath_queueIsr(APP_DATA_QUEUE,&uartRxControl.rxSwDesc);
#else					
								uartRxControl.rxSwDesc.frmInfo.plc.dtei = 0;
								uartRxControl.rxSwDesc.frmType = DATA_FRM_ID;
								uartRxControl.rxSwDesc.txPort = PORT_PLC;
								uartRxControl.rxSwDesc.lastDescLen = uartRxControl.rxCount;
								datapath_queueIsr(PLC_DATA_QUEUE,&uartRxControl.rxSwDesc);
#endif
								uartRxControl.rxFrameCount++;
								uartRxControl.rxCount = 0;
								uartRxControl.rxReady = 0;
					
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
	EA = 1;
}

//sSwFrmDesc uplcTxFrmSwDesc;

void uartRxProc()
{
	eStatus status;
	if(uartRxControl.uartRxFlag == 1)
	{
		if(uartRxControl.tick >= uartRxControl.timeout)
		{
			EA = 0;
			uartRxControl.rxReady = 1;
#ifdef NO_HOST
			if (datapath_IsQueueFullIsr(APP_DATA_QUEUE))// Discard Packet if Queue is Full
#else
			if (datapath_IsQueueFullIsr(PLC_DATA_QUEUE))// Discard Packet if Queue is Full
#endif		
			{
				uartRxControl.rxSwDesc.cpCount = 0;
				uartRxControl.rxFrameLoss++;
				uartRxControl.rxLossSoftQ++;
				gHpgpHalCB.halStats.HtoPswDropCnt++;
			}
			else
			{
				status = CHAL_RequestCP(&uartRxControl.rxSwDesc.cpArr[0].cp);
				if(status == STATUS_SUCCESS)
				{
					uint16_t i;
					uartRxControl.rxSwDesc.cpCount = 1;
					uartRxControl.rxSwDesc.cpArr[0].len =  uartRxControl.rxCount;

					uartRxControl.rxSwDesc.frmLen =  uartRxControl.rxCount;
					uartRxControl.rxSwDesc.lastDescLen = uartRxControl.rxCount;
					uartRxControl.lastRxCount = uartRxControl.rxCount;// UART RX stat variable	
					uartRxControl.pCellAddrRx = CHAL_GetAccessToCP_ISR(uartRxControl.rxSwDesc.cpArr[0].cp);
					for(i=0;i<uartRxControl.rxCount;i++)
					{
						*(uartRxControl.pCellAddrRx + i ) = *(uartRxControl.pRxdataBuffer + i);
					}
#ifdef NO_HOST				
					datapath_queueIsr(APP_DATA_QUEUE,&uartRxControl.rxSwDesc);
#if 0
					FM_Printf(FM_USER,"To App Len = %u\n",uartRxControl.rxCount);
					if(pktDbg)
					{
						FM_HexDump(FM_HINFO,"Uart Rx Data Dump",\
														uartRxControl.pRxdataBuffer,uartRxControl.rxCount);
					}
#endif				
#else			
					uartRxControl.rxSwDesc.frmInfo.plc.dtei = 0;
					uartRxControl.rxSwDesc.frmType = DATA_FRM_ID;
					uartRxControl.rxSwDesc.txPort = PORT_PLC;
					
					datapath_queueIsr(PLC_DATA_QUEUE,&uartRxControl.rxSwDesc);
#endif		
					uartRxControl.rxFrameCount++;
				}// CP Alloc Status
				else
				{
						gHalCB.cp_no_grant_alloc_cp++;
						uartRxControl.rxFrameLoss++;
						gHpgpHalCB.halStats.HtoPswDropCnt++;
				}
			}
			uartRxControl.rxCount = 0;
			uartRxControl.rxReady = 0;
			uartRxControl.uartRxFlag = 0;
			EA = 1;
		}
	}
}

u8 GV701x_setUartRxTimeout(u32 timeout)
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
#ifdef LG_UART_CONFIG	
    uart_fifoctrl->fifoctrl   = 0x21;// LG requires RX FIFO interrupt threshold to be 1. So UART will generate interrupt of every data byte received	  
#else
	uart_fifoctrl->fifoctrl   = 0xA1;// 8 Bytes TX & RX FIFO interrupt threshold for higer baudrate
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
	GV701x_setUartRxTimeout(100);
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
	u8 intFlag=0,i;
	
	xdata uint8_t *cellAddrTx;
	intFlag = EA;
	EA = 0;
	//memcpy((uint8_t*)&uartTxControl.uartTxFrmSwDesc,(uint8_t *)pHostTxFrmSwDesc,sizeof(sSwFrmDesc));
	
	uartTxControl.pTxBuffer = txBuffer;
	uartTxControl.txCount = pHostTxFrmSwDesc->frmLen;
	uartTxControl.txDone = 0;
	//if (HHAL_Req_Gnt_Read_CPU_QD() == STATUS_SUCCESS)
	{
		cellAddrTx = CHAL_GetAccessToCP(pHostTxFrmSwDesc->cpArr[0].cp);
		for(i=0;i<uartTxControl.txCount;i++)
		{
			*(uartTxControl.pTxBuffer+i) = *(cellAddrTx + i );
		}
		//HHAL_Rel_Gnt_Read_CPU_QD();
		uart_intctrl->intctrl_field.EnTxEmptInt = 1; // enables tx interrupt
	}	
	
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
		txEnable = 1;
		return uartTxControl.txDone;
	}
	else if(uartTxControl.txModeControl == UART_TX_LOW_LEVEL)
	{
		union_uart_modemstat modemstatus_u;
		modemstatus_u.modemstat = ReadU8Reg(UART_MODEMSTAT);
		if(modemstatus_u.modemstat_field.CTS) // DCTS & CTS are set
		{
			txEnable = 1;
			return uartTxControl.txDone;
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

char getChar16550()
{
	return *urxbuf;
}

void putChar16550(u8 db)
{
	*utxbuf = db;
}

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


