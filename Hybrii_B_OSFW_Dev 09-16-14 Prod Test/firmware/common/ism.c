/** ========================================================
 *
 * @file ism.c
 * 
 *  @brief Interrupt Service Manager
 *
 *  Copyright (C) 2010-2011, Greenvity Communications, Inc.
 *  All Rights Reserved
 *  
 * =========================================================*/
#ifdef RTX51_TINY_OS
#include <rtx51tny.h>
#include "hybrii_tasks.h"
#endif  //RTX51_TINY_OS
#include <stdio.h>
#include <string.h>
#include "papdef.h"
#ifdef ROUTE
#include "hpgp_route.h"
#endif
#include "ism.h"
#include "hal_common.h"
#include "hal.h"
#include "hal_reg.h"
#include "hal_hpgp_reset.h"
#ifndef CALLBACK
#include "hpgpapi.h"
#include "hal_hpgp.h"
#endif
#ifdef HYBRII_ETH
#include "hal_eth.h"
#endif
#ifdef HYBRII_SPI
#include "hal_spi.h"
#endif
#ifdef UM
#include "frametask.h"
#endif
#ifdef CCO_FUNC
#include "linkl.h"
#endif
#include "datapath.h"
#ifdef UM
#include "mac_intf_common.h"
#endif
#include "fm.h"
#include "sys_common.h"

#ifndef CALLBACK
extern void CHAL_FrameRxIntHandler(void *cookie);
extern void hal_hpgp_mac_monitoring (void);
extern void HHAL_BcnTxIntHandler(void *cookie);
extern void LINKL_BcnTxHandler(void* cookie);
extern void HHAL_Bcn3SentIntHandler();
#endif
#ifdef ETH_BRDG_DEBUG
extern u32 oldNumEthTxDoneInts;
#endif
#define BPST_THRESHOLD 47000000
u32 goldntb1;
#ifdef POWERSAVE
u32 goldntb10;
u32 goldntb2;
bool hiberClkOffSetFlag = FALSE;
u32 EarlyWakeBcnCnt = 0;
u32 tmpPsBpIntCnt = 0;		 
u32 EarlyWBPnotEven = 0;
u32 EarlyWBPnotMOD = 0;
u8	EarlyWakeBpIntFlag = FALSE;
extern u32 bcnStartInt;
extern u32 bcnStartIntExitSleep;
extern u8 psDebug;
extern u32 earlywakeBPintCnt;
#endif
#ifdef LOG_FLASH
u32 lastITime = 0xFFFF;
u32 lastBtime = 0xFFFF;
#endif
#ifdef SW_RECOVERY
extern void Monitor_Hang();
#endif

u8 discBcnTxCnt = 0;
/* external interrupt handler control block */
u8 gpio_int = 0;
static sIsm    Ism;
#ifdef LOG_FLASH

 u32 STM_GetTick();
#endif
/* TODO: need to check the hardware spec */

void ISM_EnableCpuIrq(u8 irq)
{
    if (irq >= CPU_INT_IRQ_ALL)
        return;

    //Ism.cpuIntMask.reg |= (1L << irq);
    switch(irq)
    {
        case CPU_INT_IRQ_TIME0:
            Ism.cpuIntMask.s.timer0    = 1;
            break;
        case CPU_INT_IRQ_WATCHDOG:
            Ism.cpuIntMask.s.watchdog  = 1;
            break;
        case CPU_INT_IRQ_UART:
            Ism.cpuIntMask.s.uart0     = 1;
            break;
        case CPU_INT_IRQ_PIO:
            Ism.cpuIntMask.s.pio       = 1;
            break;
        case CPU_INT_IRQ_BTN:
            Ism.cpuIntMask.s.btn       = 1;
            break;
        case CPU_INT_IRQ_LCD:
            Ism.cpuIntMask.s.lcd       = 1;
            break;
        case CPU_INT_IRQ_DMA0:
            Ism.cpuIntMask.s.dma0      = 1;
            break;
        case CPU_INT_IRQ_DMA1:
            Ism.cpuIntMask.s.dma1      = 1;
            break;
        case CPU_INT_IRQ_UART_LITE:
            Ism.cpuIntMask.s.uartLite  = 1;
            break;
        case CPU_INT_IRQ_EXT0:
            Ism.cpuIntMask.s.ext0      = 1;
            break;
        case CPU_INT_IRQ_EXT1:
            Ism.cpuIntMask.s.ext1      = 1;
            break;
        case CPU_INT_IRQ_GLOBAL_EN:
            Ism.cpuIntMask.s.globalIntEna = 1;
            break;
        default:
            return;
    }
    WriteU32Reg(INTIRQ_8051_REG, Ism.cpuIntMask.reg);
}


void ISM_DisableCpuIrq(u8 irq)
{
    if (irq >= CPU_INT_IRQ_ALL)
        return;

    //Ism.cpuIntMask.reg &= ~(1L << irq);
    switch(irq)
    {
        case CPU_INT_IRQ_TIME0:
            Ism.cpuIntMask.s.timer0    = 0;
            break;
        case CPU_INT_IRQ_WATCHDOG:
            Ism.cpuIntMask.s.watchdog  = 0;
            break;
        case CPU_INT_IRQ_UART:
            Ism.cpuIntMask.s.uart0     = 0;
            break;
        case CPU_INT_IRQ_PIO:
            Ism.cpuIntMask.s.pio       = 0;
            break;
        case CPU_INT_IRQ_BTN:
            Ism.cpuIntMask.s.btn       = 0;
            break;
        case CPU_INT_IRQ_LCD:
            Ism.cpuIntMask.s.lcd       = 0;
            break;
        case CPU_INT_IRQ_DMA0:
            Ism.cpuIntMask.s.dma0      = 0;
            break;
        case CPU_INT_IRQ_DMA1:
            Ism.cpuIntMask.s.dma1      = 0;
            break;
        case CPU_INT_IRQ_UART_LITE:
            Ism.cpuIntMask.s.uartLite  = 0;
            break;
        case CPU_INT_IRQ_EXT0:
            Ism.cpuIntMask.s.ext0      = 0;
            break;
        case CPU_INT_IRQ_EXT1:
            Ism.cpuIntMask.s.ext1      = 0;
            break;
        case CPU_INT_IRQ_GLOBAL_EN:
            Ism.cpuIntMask.s.globalIntEna = 0;
            break;
        default:
            return;
    }
    WriteU32Reg(INTIRQ_8051_REG, Ism.cpuIntMask.reg);
}





void ISM_EnableMacIrq(u8 irq)
{
    if (irq > MAC_INT_IRQ_ALL)
        return;

    if (irq == MAC_INT_IRQ_ALL)
    {
        /* enable all */
        // 3. Enable MAC Interrupts - routed through External Int 0
        Ism.macIntMask.reg   = 0;
        //intMacEnable.reg |= CPU_INTERRUPT_ALLINTSMASK;
        Ism.macIntMask.s.ethFreeCP      = 1;
        Ism.macIntMask.s.hpgpBP         = 1;
        Ism.macIntMask.s.cpuTxQNonEmpty = 1;
        Ism.macIntMask.s.plcBcn3Sent    = 1;
        Ism.macIntMask.s.plcBcnRx       = 1;
//        Ism.macIntMask.s.hpgpBPSta      = 1;
        Ism.macIntMask.s.plcMedStatInt  = 1;
    }
    else if (irq < MAC_INT_IRQ_ALL)
    {
//        Ism.macIntMask.reg |= (1L << irq);
        switch(irq)
        {
            case MAC_INT_IRQ_PLC_MED_STAT:
                Ism.macIntMask.s.plcMedStatInt  = 1;
                break;
            case MAC_INT_IRQ_HPGP_BP_STA:
//                Ism.macIntMask.s.hpgpBPSta      = 1;
                break;
//            case MAC_INT_IRQ_ZIGBEE:
//                Ism.macIntMask.s.zigbee         = 1;
//                break;
            case MAC_INT_IRQ_PLC_BCN_RX:
                Ism.macIntMask.s.plcBcnRx       = 1;
                break;
            case MAC_INT_IRQ_PLC_BCN3_SENT:
                Ism.macIntMask.s.plcBcn3Sent    = 1;
                break;
            case MAC_INT_IRQ_PLC_BCN2_SENT:
                Ism.macIntMask.s.plcBcn2Sent    = 1;
                break;
            case MAC_INT_IRQ_PLC_FRAME_VALID:
                Ism.macIntMask.s.plcFrmValid    = 1;
                break;
            case MAC_INT_IRQ_CPU_TXQ_NONEMPTY:
                Ism.macIntMask.s.cpuTxQNonEmpty = 1;
                break;
            case MAC_INT_IRQ_PLC_BCN_TX:
                Ism.macIntMask.s.hpgpBP         = 1;
                break;
            case MAC_INT_IRQ_NEW_ETH_SA:
                Ism.macIntMask.s.newEtherSA     = 1;
                break;
            case MAC_INT_IRQ_HOST_SPI:
                Ism.macIntMask.s.hostSpiInt     = 1;
                break;
            case MAC_INT_IRQ_ETH_FREE_CP:
                Ism.macIntMask.s.ethFreeCP      = 1;
                break;
            default:
               return;
        } 
    }
        
    WriteU32Reg(CPU_INTENABLE_REG, Ism.macIntMask.reg);
}


void ISM_DisableMacIrq(u8 irq)
{
    if (irq > MAC_INT_IRQ_ALL)
        return;

    if (irq == MAC_INT_IRQ_ALL)
    {
        /* disable all */
        Ism.macIntMask.reg   = 0;
    }
    else if (irq < MAC_INT_IRQ_ALL)
    {
//        Ism.macIntMask.reg &= ~(1L << irq);
        switch(irq)
        {
            case MAC_INT_IRQ_PLC_MED_STAT:
                Ism.macIntMask.s.plcMedStatInt  = 0;
                break;
            case MAC_INT_IRQ_HPGP_BP_STA:
//                Ism.macIntMask.s.hpgpBPSta      = 0;
                break;
//            case MAC_INT_IRQ_ZIGBEE:
//                Ism.macIntMask.s.zigbee         = 0;
//                break;
            case MAC_INT_IRQ_PLC_BCN_RX:
                Ism.macIntMask.s.plcBcnRx       = 0;
                break;
            case MAC_INT_IRQ_PLC_BCN3_SENT:
                Ism.macIntMask.s.plcBcn3Sent    = 0;
                break;
            case MAC_INT_IRQ_PLC_BCN2_SENT:
                Ism.macIntMask.s.plcBcn2Sent    = 0;
                break;
            case MAC_INT_IRQ_PLC_FRAME_VALID:
                Ism.macIntMask.s.plcFrmValid    = 0;
                break;
            case MAC_INT_IRQ_CPU_TXQ_NONEMPTY:
                Ism.macIntMask.s.cpuTxQNonEmpty = 0;
                break;
            case MAC_INT_IRQ_PLC_BCN_TX:
                Ism.macIntMask.s.hpgpBP         = 0;
                break;
            case MAC_INT_IRQ_NEW_ETH_SA:
                Ism.macIntMask.s.newEtherSA     = 0;
                break;
            case MAC_INT_IRQ_HOST_SPI:
                Ism.macIntMask.s.hostSpiInt     = 0;
                break;
            case MAC_INT_IRQ_ETH_FREE_CP:
                Ism.macIntMask.s.ethFreeCP      = 0;
                break;
            default:
               return;
        } 
    }
        
    WriteU32Reg(CPU_INTENABLE_REG, Ism.macIntMask.reg);
}


void ISM_EnableInterrupts()
{
#ifdef HAL_INT
    u8051InterruptReg   int8051Enable;
#ifdef P8051
    /* Set the interrupts to Lowlevel-triggered */
    IT0 = 0;                    
    IT1 = 0;

    /* enable the external interrupt */
    EX1 = 1;
#endif
    ISM_EnableCpuIrq(CPU_INT_IRQ_EXT1);

    /* enable the global interrupt */
    int8051Enable.reg = 0;
    int8051Enable.s.ext1 = 1;
    int8051Enable.s.globalIntEna = 1;
    WriteU32Reg(INTENA_8051_REG,int8051Enable.reg);
#endif

#ifdef P8051
    /* start interrupt */
    EA  = 1;                    
#endif
#ifdef UM
#ifdef UART_HOST_INTF 
{
	u8051InterruptReg	int8051Enable;
	IT0 = 0;                    
    IT1 = 0;
    /* enable the external interrupt */
    EX1 = 1;
	EA  = 1;
	ISM_EnableCpuIrq(CPU_INT_IRQ_UART);
	int8051Enable.reg = 0;
   int8051Enable.s.uart0 = 1;
   int8051Enable.s.globalIntEna = 1;
   WriteU32Reg(INTENA_8051_REG,int8051Enable.reg);
}
#endif
#endif
}
												   
extern u32 gBPSTdelta;
extern u8 gNegativeflag;
extern u8 gPositiveflag;
extern u8 firsttime;
extern u32 goldbpst;
extern u32 gavg;
extern u8 zctrack;
extern u8 zcFlag;
extern u32 gbpst;
extern u8 gRollOver;
extern u32 zcCCONTBold;
extern u32 gCCO_BTS;
extern u32 gtimer2, gtimer1;
extern u32 zcCCONTB_OLD;



u8  checkSPITxDone()
{
    uInterruptReg intStatus;

     EA = 0;                             
    // Read interrupt status. 
    intStatus.reg = ReadU32Reg(CPU_INTSTATUS_REG);

	EA = 1;
    // Write back the value to clear the status

	if(intStatus.s.spiTxDone)
	{
		WriteU32Reg(CPU_INTSTATUS_REG, intStatus.s.spiTxDone);

		/* SPI Int Handler */
		hal_spi_tx_done_handler();
	}
	return 1;

}

extern void LINKL_SendBcnLossInd(u8 type);
#ifdef HAL_INT_HDL
/* external interrupt handler */
void ISM_Ext1Isr(void) __INTERRUPT2__
#else
void ISM_PollInt(void) 
#endif
{
    //u32  intStatus;
//    u32 delta, ntb1;
	
#ifdef RTX51_TINY_OS
	
	u8 scheduleFrameTask = 0;
#endif			

    uInterruptReg intStatus;
    uPlcMedInterruptReg PlcMedInterruptReg;
    u32           bankSelReg;
	u32 dbc_pattern;
    u32 postBpstdiff;
    u32 ntb1, diff;
#ifdef LOG_FLASH
    u32 currITime;
#endif
#ifndef HPGP_HAL_TEST
	sLinkLayer    *linkLayer = (sLinkLayer *)HPGPCTRL_GetLayer(HP_LAYER_TYPE_LINK);
#endif
#ifdef UM
	sStaInfo      *staInfo = LINKL_GetStaInfo(linkLayer);
#endif
#ifdef POWERSAVE
	sScb *scb;
	u8 loopCnt=0;
	u8 missBcnCnt = 0;

    if (linkLayer->mode == LINKL_STA_MODE_CCO)
	{
		// this station is CCO
		scb = staInfo->ccoScb;
	}
	else
	{
		// this station is STA
		scb = staInfo->staScb;
	}
#endif

    gHalCB.extIntCnt++;
#ifdef LOG_FLASH
    currITime = STM_GetTick();
    if((lastITime + 6) < currITime)
    {
        sTime t;
        tickToTime(&t, lastITime);
        logEvent(ISM_ERROR, ISM_ENTRY_ERROR, 0, &t, sizeof(sTime));
    }
    lastITime = currITime;
#endif
     EA = 0;                             
    // Read interrupt status. 
    intStatus.reg = ReadU32Reg(CPU_INTSTATUS_REG);

    // Write back the value to clear the status
    WriteU32Reg(CPU_INTSTATUS_REG, intStatus.reg);
   	PlcMedInterruptReg.reg = ReadU32Reg(PLC_MEDIUMINTSTATUS_REG);
    //	WriteU32Reg(PLC_MEDIUMINTSTATUS_REG, PlcMedInterruptReg.reg);
    EA = 1;
    // Save CP bank context
    bankSelReg = ReadU32Reg(CPU_PKTBUFBANKSEL_REG);

    // Process/Dispatch interrupts.
#ifdef HYBRII_ZIGBEE
    //while(intStatus.reg)
    while(intStatus.reg || PlcMedInterruptReg.s.zcint ||  PlcMedInterruptReg.s.plcTxDoneint)
#else
    
#ifdef POWERSAVE 
    while(intStatus.s.hpgpBP ||intStatus.s.PosthpgpBP||intStatus.s.ethFreeCP||intStatus.s.eth_txfifoRdDn||intStatus.s.spiRxDone||intStatus.s.spiTxDone||intStatus.s.newEtherSA||((!scheduleFrameTask) && intStatus.s.cpuTxQNonEmpty)||intStatus.s.plcBcn2Sent||intStatus.s.plcBcn3Sent||intStatus.s.plcBcnRx || PlcMedInterruptReg.s.zcint|| PlcMedInterruptReg.s.plcTxDoneint || PlcMedInterruptReg.s.earlywakeBP || PlcMedInterruptReg.s.bcnStart) 
#else
    //while(intStatus.s.hpgpBP ||intStatus.s.PosthpgpBP||intStatus.s.ethFreeCP||
	//	  intStatus.s.eth_txfifoRdDn || intStatus.s.spiRxDone||intStatus.s.spiTxDone||intStatus.s.newEtherSA||intStatus.s.cpuTxQNonEmpty||intStatus.s.plcBcn2Sent||intStatus.s.plcBcn3Sent||intStatus.s.plcBcnRx || PlcMedInterruptReg.s.zcint ||  PlcMedInterruptReg.s.plcTxDoneint || PlcMedInterruptReg.s.DbcHoldInt || PlcMedInterruptReg.s.HP101Detect) 
    while(intStatus.s.hpgpBP ||intStatus.s.PosthpgpBP||intStatus.s.ethFreeCP||
		  intStatus.s.eth_txfifoRdDn || intStatus.s.spiRxDone||intStatus.s.spiTxDone||intStatus.s.newEtherSA||((!scheduleFrameTask) && intStatus.s.cpuTxQNonEmpty)||intStatus.s.plcBcn2Sent||intStatus.s.plcBcn3Sent||intStatus.s.plcBcnRx || PlcMedInterruptReg.s.zcint ||  PlcMedInterruptReg.s.plcTxDoneint) 
#endif
#endif // HYBRII_ZIGBEE
    {
     
#ifdef  HYBRII_HPGP
//        HHAL_ProcessPlcTxDone();
#endif

     if(PlcMedInterruptReg.s.zcint && gHpgpHalCB.bcnInitDone) //after 33.33 ms
	 {
          	//WriteU32Reg(PLC_MEDIUMINTSTATUS_REG, PlcMedInterruptReg.reg);
		   doSynchronization();
           //printf("\n zc");
           //gHpgpHalCB.bPerAvgInitDone = 1;
     }
#ifdef FREQ_DETECT
    
    if((gHpgpHalCB.gFreqCB.freqDetected == FALSE)  && gHpgpHalCB.bcnInitDone)
    {

#ifdef HPGP_HAL_TEST
        if(PlcMedInterruptReg.s.zcint && (gHpgpHalCB.devMode == DEV_MODE_CCO))
        {

#else
        if(PlcMedInterruptReg.s.zcint && (linkLayer->mode == LINKL_STA_MODE_CCO))
        {                                             
#endif
            
            
            FREQDET_DetectFrequencyUsingZC();             
        }
    }
#endif

	if (PlcMedInterruptReg.s.plcTxDoneint)
	{
		datapath_handlePlcTxDone();

		scheduleFrameTask = 1;

		//printf("td\n");

		WriteU32Reg(PLC_MEDIUMINTSTATUS_REG, ctorl(INT_STATUS_PLC_TXDONE));

	}
	
    if(PlcMedInterruptReg.s.zcint)
    {
      	WriteU32Reg(PLC_MEDIUMINTSTATUS_REG, ctorl(INT_STATUS_ZC)); 
    }
#if 0
    if (gHpgpHalCB.HP101Detection)
    {
       if(PlcMedInterruptReg.s.HP101Detect) 
       {
            WriteU32Reg(PLC_MEDIUMINTSTATUS_REG, ctorl(INT_STATUS_HP101DETECT)); 
			printf("HP101 is detected\n");
			// Set Global variables for HP101 and HP11 detection bit to 1, which shall be used in Tx VF field
       }
    }

	if (PlcMedInterruptReg.s.DbcHoldInt)
	{
          WriteU32Reg(PLC_MEDIUMINTSTATUS_REG, ctorl(INT_STATUS_DBC_HOLD));
		  printf("DBC pattern is detected\n");
		  // Set Global variables for DBC detection bit to 1, which will be used for packet flow control
		  dbc_pattern = ReadU32Reg(PLC_DBC_PATTERN_REG);
		  printf("DBC Pattern REG = %bX\n", dbc_pattern);
	}
#endif	
#if 0   // For Sound packet process
        // [YM] When Rx a sound packet, read RSSI value from RSSI FIFO and print it out for pass to a function for further process
        if(intStatus.s.plcMSoundRxInt)
        {
           //
           HHAL_RxSoundIntHandler();
        }
#endif  

#ifdef HYBRII_HPGP
        if(intStatus.s.plcBcnRx)
        {  
#ifdef HPGP_HAL_TEST
            //HHAL_BcnRxIntHandler();
            HAL_beaconRxHandler(NULL);

#else
            //HAL_BcnRxIntHandler(Ism.macIntCb[MAC_INT_IRQ_PLC_BCN_RX].cookie);
            HAL_beaconRxHandler(Ism.macIntCb[MAC_INT_IRQ_PLC_BCN_RX].cookie);

#endif
        }

        if(intStatus.s.hpgpBP )//|| intStatus.s.hpgpBPSta)
        {
#ifdef LOG_FLASH
            u32 currBtime;
#endif
#ifdef HPGP_HAL_TEST
             HHAL_BPIntHandler();
 
#else
#ifdef CCO_FUNC
#ifdef LOG_FLASH

             currBtime = STM_GetTick();
             if((lastBtime + 10) < currBtime)
             {
                 sTime t;
                 tickToTime(&t, lastBtime);
                 logEvent(ISM_ERROR, BCN_TX_INT_ERROR, 0, &t, sizeof(sTime));
             }
             lastBtime = currBtime;
#endif
             if (!LINKL_BcnUpdateActive())
             {
                 //HHAL_BcnTxIntHandler();
                 HHAL_BPIntHandler();
             }
#endif  /* CCO_FUNC */
#ifdef POWERSAVE
			if (EarlyWakeBpIntFlag)
			{
/*
				if (psDebug)
					FM_Printf(FM_MMSG,"hpgp_Int: exit deep sleep. gHpgpHalCB.halStats.psBpIntCnt=%lu\n", gHpgpHalCB.halStats.psBpIntCnt);
*/
				PSM_exit_deep_sleep_PS();
				EarlyWakeBpIntFlag = FALSE;
			}
#endif
 
#endif //HPGP_HAL_TEST
 
         }
         if(intStatus.s.PosthpgpBP)
         {
          
           // gavg = PLC_MAX_AC_BPLEN;
             
            ntb1 = rtocl(ReadU32Reg(PLC_NTB_REG));
            postBpstdiff = (ntb1 - goldntb1) * 40;
            goldntb1 = ntb1; 
            //printf("\n diff = %lu",diff);
            
            if(zctrack)

            {
                 // printf("\n gbpst =%lu", (gbpst*40));
                 // printf("\n ntb = %lu",  (rtocl(ReadU32Reg(PLC_NTB_REG)) * 40));
                  //printf("\n gavg = %lu",  (gavg * 40));
                if(postBpstdiff > BPST_THRESHOLD) //this threshold should work for AC as well as DC
                {
                    //printf("\n diff = %lu",diff);
                    gbpst =  rtocl(ReadU32Reg(PLC_ZCNTB_REG)) + MAC_PROCESSING_CLOCK;
                    gNegativeflag =0;
                    gBPSTdelta =0;
                    gPositiveflag =0;
                } 

                  gBPSTdelta = gBPSTdelta / 8;
                 if(gNegativeflag)
                 {
                    gbpst += gavg - gBPSTdelta ;
                    //gNegativeflag = 0;
                 }
                 else  if(gPositiveflag)
                 {
                     gbpst += gavg + gBPSTdelta ;
                     //gPositiveflag = 0;
                 }
                 else
                 {
                    gbpst += gavg; 
                 } 
               
                 WriteU32Reg(PLC_BPST_REG, ctorl(gbpst));
            }
            if((gHpgpHalCB.devMode == DEV_MODE_STA)&& gHpgpHalCB.syncComplete)
            {
                if(gHpgpHalCB.bcncnt == gHpgpHalCB.halStats.BcnRxIntCnt)
                {
                   gHpgpHalCB.bcnmisscnt++; 
#ifdef POWERSAVE
					if (scb->psState == PSM_PS_STATE_ON)
					{
						missBcnCnt = scb->commAwd.numBp;
						
						if (missBcnCnt < 2)
							missBcnCnt = 2;                  
                	}


				   
					if (((scb->psState == PSM_PS_STATE_ON) && 
						((gHpgpHalCB.bcnmisscnt % missBcnCnt) == 0)) ||
				 		((scb->psState != PSM_PS_STATE_ON) &&
				 		  (gHpgpHalCB.bcnmisscnt == 4)))

					
#else
					/* 
						we rescan if 4 consecutive beacon are missed
					      this threshold should be lower then backup cco thresholds
					    	MAX_NO_BEACON_BACKUPCCO
					    	
					*/
					   
                   if(gHpgpHalCB.bcnmisscnt == 4)
#endif
                   {
					 gHpgpHalCB.bcnmisscnt = 0;

#ifdef MPER					  
                      printf("\n Rescan");
#endif

		   				gHpgpHalCB.syncComplete = 0;

#ifndef HPGP_HAL_TEST										 
{
						sSnsm*		   snsm = (sSnsm *)LINKL_GetSnsm(linkLayer);

						snsm ->netScan = 1;
//						gHpgpHalCB.nwSelected = 1
						snsm ->netSync = 0;

	 				    HHAL_SetSWStatReqScanFlag(REG_FLAG_SET);
}
#endif
				   }
#if 0 

				   if (gHpgpHalCB.bcnmisscnt > MAX_NO_BEACON_NW_DISCOVERY)
					{
                       // if(staInfo->lastUserAppCCOState == 0)
                        {
                        	gHpgpHalCB.bcnmisscnt = 0;
                        	//printf("\n MAX_NO_BEACON\n"); 
#ifdef LOG_FLASH
                       	
                            logEvent(ISM_ERROR, BCN_LOSS, EVENT_TYPE_BCN_MISS_IND, NULL, 0);
#endif
                            LINKL_SendBcnLossInd(MAX_NO_BEACON_NW_DISCOVERY);
                        }
						//Host_SendIndication(HOST_EVENT_BCN_LOSS, NULL, 0);
					}
				   else if(gHpgpHalCB.bcnmisscnt > MAX_NO_BEACON_BACKUPCCO)
					{
					    //if(staInfo->lastUserAppCCOState == 0)
                        {
  			   	    	    LINKL_SendBcnLossInd(MAX_NO_BEACON_BACKUPCCO);
                        }
   					    //Host_SendIndication(HOST_EVENT_PRE_BCN_LOSS, NULL, 0);
					}
				   
#endif					

                }
                gHpgpHalCB.bcncnt = gHpgpHalCB.halStats.BcnRxIntCnt;
            }
         } 
#endif

#ifdef ETH_BRDG_DEBUG
        if(intStatus.s.ethFreeCP)
		{
			// this is the old ETH Tx Done interrupt. It's generated after HW
			// finishes copying the pkt to ETH FIFO
			//
			oldNumEthTxDoneInts++;
		}
#endif
         
#ifdef SW_RECOVERY
         if(intStatus.s.plcSMHangInt)
        {
            //if(gHpgpHalCB.devMode == DEV_MODE_CCO) // Kiran for testing of multi device
            {
                Monitor_Hang(); 
            }
        }
#endif

        if(intStatus.s.eth_txfifoRdDn)
        {
#ifdef HYBRII_ETH
			// this is the new ETH Tx Done interrupt. It's generated after ETH
			// is done with transmitting the pkt.
            // Release ETH Tx Complete CPs
            EHAL_ReleaseEthTxCPIntHandler();

			scheduleFrameTask = 1;


#endif
        }
#ifdef HYBRII_SPI
        if(intStatus.s.spiRxDone)
        {
            /* SPI Int Handler */
            hal_spi_rx_done_handler();
        }
        if(intStatus.s.spiTxDone)
    {
        /* SPI Int Handler */
        hal_spi_tx_done_handler();

		scheduleFrameTask = 1;
     }
#endif

//[YM] comment out the hpgpBp bit checking, it wil cause Bp tx counter mismatch

        if(intStatus.s.newEtherSA)
        {
            // Read the EtherSA and invoke
            // callback function in Bridge module.
        }
#if CPU_TXQ_POLL
#else       
        if(intStatus.s.cpuTxQNonEmpty)
        {                  
#ifdef HPGP_HAL_TEST
            CHAL_CpuTxQNemptyIntHandler();
#else
            /* Frame Rx handler */
            CHAL_FrameRxIntHandler(Ism.macIntCb[MAC_INT_IRQ_CPU_TXQ_NONEMPTY].cookie);
#endif
			scheduleFrameTask = 1;
        }
#endif 
#ifdef HYBRII_HPGP     
        if(intStatus.s.plcBcn2Sent)
        {
            // Call Discovery beacon Sent Int Handler
        }
        if(intStatus.s.plcBcn3Sent)
        {
            // Call Central beacon Sent Int Handler
            HHAL_Bcn3SentIntHandler();
           /*if(zctrack)
            {
                gbpst = gavg + gbpst + MAC_PROCESSING_CLOCK ;// + 0x1BAFF;//1365 is bpsto
                 WriteU32Reg(PLC_BPST_REG, ctorl(gbpst));
            } */
        }   
           
        
#endif 
#ifdef HYBRII_ZIGBEE
        if (intStatus.s.zbPreBcnTxTime)
        {
            hal_zb_pre_bc_tx_time_handler();
        }

        if (intStatus.s.zbBcnTxTime)
        {
            hal_zb_bc_tx_time_handler();
        }

        if (intStatus.s.zbTxDone)
        {
            hal_zb_tx_done_handler();
        }
        
        if (intStatus.s.zbBcnTxDone)
        {
            hal_zb_bc_tx_done_hadler();
        }
        break;
#endif

#if 0   // For Sound packet process
        // [YM] When Rx a sound packet, read RSSI value from RSSI FIFO and print it out for pass to a function for further process
        if(intStatus.s.plcMSoundRxInt)
        {
           //
           HHAL_RxSoundIntHandler();
        }
#endif  

#ifdef POWERSAVE
	 // The order of polling is important: must poll for bpStartInt before EarlyWakeBpInt
     if(PlcMedInterruptReg.s.bcnStart)
	 {
	 	u8 modVal;

		WriteU32Reg(PLC_MEDIUMINTSTATUS_REG, ctorl(INT_STATUS_BCNSTART));

		bcnStartInt++;
			gHpgpHalCB.halStats.psBpIntCnt++;

    		if(gHpgpHalCB.devMode == DEV_MODE_CCO)
			{
				scb->bpCnt++;
			}

#ifdef PS_DEBUG
			if (psDebug)
				FM_Printf(FM_MMSG,"HPGP_INT: gHpgpHalCB.halStats.psBpIntCnt=%lu, scb->bpCnt=%d, \n", 
						gHpgpHalCB.halStats.psBpIntCnt, scb->bpCnt);
#endif
			if (scb)
			{
				if (scb->psState == PSM_PS_STATE_ON)
				{
					u8 awdCnt = 0;

//					if (psDebug)
//						printf("hpgpBP INT:  gHpgpHalCB.halStats.psBpIntCnt=%lu, EarlyWakeBcnCnt=%lu\n", gHpgpHalCB.halStats.psBpIntCnt, EarlyWakeBcnCnt);
//					if (scb->bpCnt == (EarlyWakeBcnCnt+2))	// should be only + 1, but + 1 doesn't work, why ?
					if (scb->commAwd.awdTime & 0x80)
					{
						awdCnt = (scb->commAwd.awdTime & 0x0f);
					}
					if (EarlyWakeBcnCnt && (gHpgpHalCB.halStats.psBpIntCnt == (EarlyWakeBcnCnt + awdCnt + 1)))
					{
						// this is the BP right after the AWD BP
#ifdef PS_DEBUG
						if (psDebug)
							FM_Printf(FM_MMSG, "HPGP_INT: After AWD, enter deep sleep. gHpgpHalCB.halStats.psBpIntCnt=%lu, scb->bpCnt=%d, numBp=%d,awdCnt=%bu\n", 
									gHpgpHalCB.halStats.psBpIntCnt, scb->bpCnt,  scb->commAwd.numBp, awdCnt);
#endif
						EarlyWakeBcnCnt = 0;
						PSM_enter_deep_sleep_PS();
#if 0
						WriteU32Reg(PLC_MEDIUMINTSTATUS_REG, ctorl(INT_STATUS_HIBERCLKOFF));
						hiberClkOffSetFlag = FALSE;
						EarlyWakeBcnCnt = 0;
       					PlcMedInterruptReg.reg = ReadU32Reg(PLC_MEDIUMINTSTATUS_REG);
   						if(PlcMedInterruptReg.s.hiberClkOff)
							FM_Printf(FM_MMSG,"HPGP_INT: hiberClkOff is still ON after clearing it\n");
						tmpPsBpIntCnt = gHpgpHalCB.halStats.psBpIntCnt;
#endif
					}
					else
					{
						// only STA in PS mode and it's AWD should datapath_transmitDataPlc() be called
			 			datapath_transmitDataPlc(1);
					}
				}
				else if (scb->psState == PSM_PS_STATE_WAITING_ON)
				{
					sPsSchedule tmpCommAwd;

					// this STA has been waiting for the right bp to start its AWD
#ifdef PS_DEBUG
					if (psDebug)
						FM_Printf(FM_MMSG,"HPGP_INT: scb->psState=WAITING_ON. gHpgpHalCB.halStats.psBpIntCnt=%lu, scb->bpCnt=%d, scb->commAwd.numBp=%d\n", 
								gHpgpHalCB.halStats.psBpIntCnt, scb->bpCnt,  scb->commAwd.numBp);
#endif
					PSM_cvrtPss_Awd(scb->pss, &tmpCommAwd);	// for now, store PSS in tmp place so datapath_transmitDataPlc()
															// can still tx
					if (!(scb->bpCnt % tmpCommAwd.numBp))
					{
#ifdef PS_DEBUG
						if (psDebug)
							FM_Printf(FM_MMSG,"HPGP_INT: Config PS HW for AWD. gHpgpHalCB.halStats.psBpIntCnt=%lu, scb->bpCnt=%d, scb->commAwd.numBp=%d\n", 
									gHpgpHalCB.halStats.psBpIntCnt, scb->bpCnt,  scb->commAwd.numBp);
#endif
						scb->psState = PSM_PS_STATE_ON;
						gHpgpHalCB.halStats.psBpIntCnt = scb->bpCnt;	// sync with CCO's bpCnt
						PSM_ConfigStaPsHW(scb->pss);	
					   	PSM_SetStaPsHW(TRUE);
						PSM_cvrtPss_Awd(scb->pss, &scb->commAwd);	// convert PSS to usable format
						FM_Printf(FM_MMSG, "STA Power Saving Mode is now ON\n");
					}
				}	 
			}
/*
		bcnStartInt++;
		if (EarlyWakeBpIntFlag)
		{
			if (psDebug)
				FM_Printf(FM_MMSG,"bcnStart_Int: exit deep sleep. gHpgpHalCB.halStats.psBpIntCnt=%lu\n", gHpgpHalCB.halStats.psBpIntCnt);
			PSM_exit_deep_sleep_PS();
			EarlyWakeBpIntFlag = FALSE;
			bcnStartIntExitSleep++;
		}
*/
	}

     if(PlcMedInterruptReg.s.earlywakeBP)
	 {
	 	u8 modVal;
	    sEvent *newEvent = NULL;
		u32 ntb;

		earlywakeBPintCnt++;
#ifdef PS_DEBUG
		if (psDebug)
			FM_Printf(FM_MMSG,"earlywakeBP_Int: exit deep sleep. gHpgpHalCB.halStats.psBpIntCnt=%lu, scb->bpCnt=%d \n", gHpgpHalCB.halStats.psBpIntCnt, scb->bpCnt);
#endif
		WriteU32Reg(PLC_MEDIUMINTSTATUS_REG, ctorl(INT_STATUS_EARLYWAKEUPBP));

		if (gHpgpHalCB.devMode == DEV_MODE_CCO)
		{
		 	if ( !(scb->bpCnt % 2))
			{
				// bpCnt should be an odd # in this interrupt
				// if it's even, it means that it's 1 more than it should be
				// Need to change it to odd so STA can sync with CCO
		   		scb->bpCnt--;
				EarlyWBPnotEven++;
#ifdef PS_DEBUG
				if (psDebug)
					FM_Printf(FM_MMSG,"EARLY WAKEUP INT, CCO: changed bpCnt to ODD, new scb->bpCnt=%d\n", scb->bpCnt);
#endif
			}

			// take care of the case when EarlyWakeBP interrupt occurs too early (usually 2 bps earlier)
			if (scb->commAwd.numBp)
			{
				modVal = (scb->bpCnt + 1) % scb->commAwd.numBp;
				if (modVal != 0)
				{
					EarlyWBPnotMOD++;
#ifdef PS_DEBUG
				if (psDebug)
						printf("CCO: NOT MOD with scb->commAwd.numBp=%d, gHpgpHalCB.halStats.psBpIntCnt=%lu, scb->bpCnt=%d, modVal=%bu\n", 
								scb->commAwd.numBp, gHpgpHalCB.halStats.psBpIntCnt, scb->bpCnt, modVal);
#endif
			    	scb->bpCnt += scb->commAwd.numBp - modVal;
				}
			}
			EarlyWakeBcnCnt = scb->bpCnt;
		}
		if (gHpgpHalCB.devMode == DEV_MODE_STA)
		{
			if (!(gHpgpHalCB.halStats.psBpIntCnt % 2))
			{
				// bpCnt should be an odd # in this interrupt
				// if it's even, it means that it's 1 more than it should be
				// Need to change it to odd
		   		gHpgpHalCB.halStats.psBpIntCnt--;
				EarlyWBPnotEven++;
#ifdef PS_DEBUG
				if (psDebug)
					FM_Printf(FM_MMSG,"EARLY WAKEUP INT, STA: changed bpCnt to ODD, new gHpgpHalCB.halStats.psBpIntCnt=%lu\n", gHpgpHalCB.halStats.psBpIntCnt);
#endif
			}

			// take care of the case when EarlyWakeBP interrupt occurs too early (usually 2 bps earlier)
			if (scb->commAwd.numBp)
			{
				modVal = (gHpgpHalCB.halStats.psBpIntCnt + 1) % scb->commAwd.numBp;
				if (modVal != 0)
				{
					EarlyWBPnotMOD++;
#ifdef PS_DEBUG
					if (psDebug)
						printf("STAT: NOT MOD with scb->commAwd.numBp=%d, gHpgpHalCB.halStats.psBpIntCnt=%lu, scb->bpCnt=%d, modVal=%bu\n", 
								scb->commAwd.numBp, gHpgpHalCB.halStats.psBpIntCnt, scb->bpCnt, modVal);
#endif
		    		gHpgpHalCB.halStats.psBpIntCnt += scb->commAwd.numBp - modVal;
				}
			}
			EarlyWakeBcnCnt = gHpgpHalCB.halStats.psBpIntCnt;
		}
#ifdef PS_DEBUG
		if (psDebug)
			FM_Printf(FM_MMSG,"EARLY WAKEUP INT: final: EarlyWakeBcnCnt=%lu, gHpgpHalCB.halStats.psBpIntCnt=%lu, scb->bpCnt=%d\n", 
					EarlyWakeBcnCnt, gHpgpHalCB.halStats.psBpIntCnt, scb->bpCnt);
#endif
		EarlyWakeBpIntFlag = TRUE;
		// now exit deep_sleep mode
//		PSM_exit_deep_sleep_PS();
	}

#ifdef POWERSAVE_NO 
     if(PlcMedInterruptReg.s.hiberClkOff)
	 {
	 	if (scb && (scb->psState == PSM_PS_STATE_ON))
		{
			if (!hiberClkOffSetFlag && 	(gHpgpHalCB.halStats.psBpIntCnt > tmpPsBpIntCnt))	// HW problem: HiberClkOff goes HI too fast, so wait for next BP to do this
			{
#ifdef PS_DEBUG
				if (psDebug)
	    	    	FM_Printf(FM_MMSG,"hiberClkOff int. is HI. Enter deep sleep. gHpgpHalCB.halStats.psBpIntCnt=%lu, scb->bpCnt=%d\n", gHpgpHalCB.halStats.psBpIntCnt, scb->bpCnt);
#endif
				hiberClkOffSetFlag = TRUE;
				PSM_enter_deep_sleep_PS();
			}
		}
     }
	 else
	 {
	 	if (scb && (scb->psState == PSM_PS_STATE_ON))
		{
#ifdef PS_DEBUG
			if (psDebug)
	   	    	FM_Printf(FM_MMSG,"hiberClkOff int. is LO. gHpgpHalCB.halStats.psBpIntCnt=%lu, scb->bpCnt=%d\n", gHpgpHalCB.halStats.psBpIntCnt, scb->bpCnt, gHpgpHalCB);
#endif
			
		}
	 }
#endif
#endif //POWERSAVE

        EA = 0;
        // Read interrupt status.
        intStatus.reg = ReadU32Reg(CPU_INTSTATUS_REG);
        // Write back the value to clear the status immediately
        WriteU32Reg(CPU_INTSTATUS_REG, intStatus.reg);
        PlcMedInterruptReg.reg = ReadU32Reg(PLC_MEDIUMINTSTATUS_REG);
		//if (intStatus.s.hpgpBP ||intStatus.s.PosthpgpBP||intStatus.s.ethFreeCP||intStatus.s.spiRxDone||intStatus.s.spiTxDone||intStatus.s.newEtherSA||intStatus.s.cpuTxQNonEmpty||intStatus.s.plcBcn2Sent||intStatus.s.plcBcn3Sent||intStatus.s.plcBcnRx )
		  // printf(" intStatus.reg = %lx\n", intStatus.reg);
        EA = 1;

#ifdef POWERSAVE 
		loopCnt++;
#endif

    }


	
#ifdef RTX51_TINY_OS

	if (scheduleFrameTask )
	{

		//FM_Printf(FM_USER,"s\n");
		os_set_ready(HYBRII_TASK_ID_FRAME);

	}

		
#endif	
    // Restore cp bank context
#ifndef HYBRII_FPGA
#ifdef  HYBRII_HPGP
//    HHAL_ProcessPlcTxDone();
	
#endif
#endif
    
    WriteU32Reg(CPU_PKTBUFBANKSEL_REG, bankSelReg);
#ifdef HYBRII_HPGP
    hal_hpgp_mac_monitoring();    
#endif

    return;


}

#ifndef HAL_INT
void ISM_EnableIntPolling()
{
#ifndef RTX51_TINY_OS
    SCHED_Sched(&Ism.task);
#endif
}
#endif

eStatus ISM_Init(void)
{
    memset(&Ism, 0, sizeof(sIsm));
#ifndef HAL_INT
#ifdef RTX51_TINY_OS
    //os_create_task(HYBRII_TASK_ID_ISM_POLL);
#else
    SCHED_InitTask(&Ism.task, HYBRII_TASK_ID_ISM_POLL, "ISM",
                   HPGP_TASK_PRI_ISM, ISM_PollInt, &Ism);
#endif
#endif
    return STATUS_SUCCESS;
}

eStatus ISM_RegisterIntHandler( eMacIntIrq intIrq,
                                 void (*intHdlr)(void XDATA *cookie),
                                 void* cookie)
{
    intHdlr = intHdlr;

    if(intIrq < MAC_INT_IRQ_ALL)
    {
#ifdef CALLBACK
        Ism.macIntCb[intIrq].intHandler = intHdlr;
#endif
        Ism.macIntCb[intIrq].cookie = cookie;
        return STATUS_SUCCESS;
    }
    else
    {
        return STATUS_FAILURE;
    }
}



void ISM_UnregisterIntrHandler( eMacIntIrq intIrq)
{
    if(intIrq < MAC_INT_IRQ_ALL)
    {
        Ism.macIntCb[intIrq].intHandler = NULL;
        Ism.macIntCb[intIrq].cookie = NULL;
    }
}

/*
#ifdef RTX51_TINY_OS
extern void mac_hal_irq_handler(void);
void ISM_ExtInterruptPoll (void) _task_ HYBRII_TASK_ID_ISM_POLL
{
    while (1) {
#ifdef HYBRII_HPGP
        ISM_PollInt();
#else
        // FIXME - To be removed
        mac_hal_irq_handler();
#endif
        os_switch_task();
    }
}
#endif
*/

/** =========================================================
 *
 * Edit History
 *
 * $Source: /home/cvsrepo/Hybrii_B_OSFW_Dev/firmware/common/ism.c,v $
 *
 * $Log: ism.c,v $
 * Revision 1.48  2014/09/05 09:28:18  ranjan
 * 1. uppermac cco-sta switching feature fix
 * 2. general stability fixes for many station associtions
 * 3. changed mgmt memory pool for many STA support
 *
 * Revision 1.47  2014/08/25 07:37:34  kiran
 * 1) RSSI & LQI support
 * 2) Fixed Sync related issues
 * 3) Fixed timer 0 timing drift for SDK
 * 4) MMSG & Error Logging in Flash
 *
 * Revision 1.46  2014/07/30 12:26:25  kiran
 * 1) Software Recovery for CCo
 * 2) User appointed CCo support in SDK
 * 3) Association process performance fixes
 * 4) SSN related fixes
 *
 * Revision 1.45  2014/07/22 21:04:25  tri
 * Fixed compiler error
 *
 * Revision 1.44  2014/07/22 10:03:52  kiran
 * 1) SDK Supports Power Save
 * 2) Uart_Driver.c cleanup
 * 3) SDK app memory pool optimization
 * 4) Prints from STM.c are commented
 * 5) Print messages are trimmed as common no memory left in common
 * 6) Prins related to Power save and some other modules are in compilation flags. To enable module specific prints please refer respective module
 *
 * Revision 1.43  2014/07/16 10:47:40  kiran
 * 1) Updated SDK
 * 2) Fixed Diag test in SDK
 * 3) Ethernet and SPI interfaces removed from SDK as common memory is less
 * 4) GPIO access API's added in SDK
 * 5) GV701x chip reset command supported
 * 6) Start network and Join network supported in SDK (Forced CCo and STA)
 * 7) Some bug fixed in SDK (CP free, p app command issue etc.)
 *
 * Revision 1.42  2014/07/11 10:23:37  kiran
 * power save changes
 *
 * Revision 1.41  2014/06/19 17:13:19  ranjan
 * -uppermac fixes for lvnet and reset command for cco and sta mode
 * -backup cco working
 *
 * Revision 1.40  2014/06/17 20:35:23  varsha
 * freq detect code for hal test and umac prj is under flag compilation.
 *
 * Revision 1.39  2014/06/11 15:09:43  tri
 * took out debug printf
 *
 * Revision 1.38  2014/06/05 10:26:07  prashant
 * Host Interface selection isue fix, Ac sync issue fix
 *
 * Revision 1.37  2014/05/28 10:58:58  prashant
 * SDK folder structure changes, Uart changes, removed htm (UI) task
 * Varified - UM, LM - iperf (UDP/TCP), overnight test pass
 *
 * Revision 1.36  2014/05/21 15:57:51  yiming
 * temporary uncheck DBC and HP101 checking
 *
 * Revision 1.35  2014/05/15 19:30:43  varsha
 * FREQ_DETECT code is added
 *
 * Revision 1.34  2014/05/13 20:28:18  varsha
 * This file added for sW recovery machenism
 *
 * Revision 1.33  2014/05/12 08:09:57  prashant
 * Route fix, STA sync issue with AV CCO and handling discover bcn issue fix
 *
 * Revision 1.32  2014/04/29 21:29:28  yiming
 * disable print message for Mitsumi (MPER)
 *
 * Revision 1.31  2014/04/25 21:16:51  tri
 * PS
 *
 * Revision 1.30  2014/04/24 21:50:00  yiming
 * Working Code for Mitsumi
 *
 * Revision 1.29  2014/04/23 23:21:11  tri
 * fix compiler error
 *
 * Revision 1.28  2014/04/23 23:09:10  tri
 * more PS
 *
 * Revision 1.27  2014/04/21 20:04:23  tri
 * more PS
 *
 * Revision 1.26  2014/04/21 03:10:59  tri
 * more PS
 *
 * Revision 1.25  2014/04/20 19:47:12  tri
 * more PS
 *
 * Revision 1.24  2014/04/20 05:06:24  tri
 * compiler error
 *
 * Revision 1.23  2014/04/20 05:04:57  tri
 * more PS
 *
 * Revision 1.22  2014/04/20 04:55:19  tri
 * more PS
 *
 * Revision 1.21  2014/04/15 23:07:21  tri
 * more PS
 *
 * Revision 1.20  2014/04/15 19:52:20  yiming
 * Merge new ASIC setting, Add throughput improvement code, add M_PER code
 *
 * Revision 1.19  2014/04/09 21:11:58  yiming
 * fix compile error
 *
 * Revision 1.18  2014/04/09 21:04:23  tri
 * more PS
 *
 * Revision 1.17  2014/03/27 23:51:48  tri
 * more PS
 *
 * Revision 1.16  2014/03/26 00:11:50  yiming
 * Add DBC register definition
 *
 * Revision 1.15  2014/03/25 17:01:07  son
 * Hybrii B ASIC bring up
 *
 * Revision 1.14  2014/03/19 00:13:46  tri
 * PS
 *
 * Revision 1.13  2014/03/15 17:24:06  tri
 * Power Save deep sleep
 *
 * Revision 1.12  2014/03/12 19:51:45  tri
 * added code for ETH_BRDG_DEBUG
 *
 * Revision 1.11  2014/03/10 05:58:10  ranjan
 * 1. added HomePlug BackupCCo feature. verified C&I test.(passed.) (bug 176)
 *
 * Revision 1.10  2014/02/27 10:42:47  prashant
 * Routing code added
 *
 * Revision 1.9  2014/02/26 22:56:06  tri
 * more PS code
 *
 * Revision 1.8  2014/02/19 20:30:21  son
 * Replace calling CHAL_Ext1Isr with ISM_PollInt
 *
 * Revision 1.7  2014/02/19 10:22:40  ranjan
 * - common sync for hal_tst and upper mac project
 * - ism.c is MAC interrupt handler for hhal_tst and upper mac.
 *    chal_ext1isr function   is removed
 * - verified : lower mac sync, upper mac sync data traffic.
 *
 * Revision 1.6  2014/02/14 21:09:35  varsha
 * I have added work around in posthpgpBP interrupt because we were missing polling in three device communication.
 * With this fix AC,DC, uppermac and Lower MAc sync is working fine.
 *
 * Varsha.
 *
 * Revision 1.5  2014/02/12 11:45:16  prashant
 * Performance improvement fixes
 *
 * Revision 1.4  2014/02/07 22:45:05  yiming
 * add HP101 and HP11 detection code
 *
 * Revision 1.3  2014/01/28 17:53:46  tri
 * Added Power Save code
 *
 * Revision 1.2  2014/01/10 17:02:18  yiming
 * check in Rajan 1/8/2014 code release
 *
 * Revision 1.14  2014/01/08 10:53:53  ranjan
 * Changes for LM OS support.
 * New Datapath FrameTask
 * LM and UM  datapath, feature verified.
 *
 * known issues : performance numbers needs revisit
 *
 * review : pending.
 *
 * Revision 1.13  2013/10/25 13:08:16  prashant
 * ism.c fix for zigbee, Sniffer support for lower MAC
 *
 * Revision 1.12  2013/10/21 18:59:51  son
 * Fixed compilation issue for zigbee project
 *
 * Revision 1.11  2013/10/16 07:43:37  prashant
 * Hybrii B Upper Mac compiling issues and QCA fix, added default eks code
 *
 * Revision 1.10  2013/09/20 14:19:36  yiming
 * merge Varsha SEP 16 code to Hybrii_B CVS
 *
 * Revision 1.9  2013/09/17 22:08:46  yiming
 * merge Hybrii_A ism.c to Hybrii_B CVS
 *
 * Revision 1.8  2013/09/17 22:00:03  yiming
 * fixed the compile error on 0916 sync code merge, remove older code and hpgpBPSta bit setting
 *
 * Revision 1.7  2013/09/16 22:29:38  yiming
 * Merge 0916 sync code
 *
 * Revision 1.6  2013/09/13 19:39:07  yiming
 * Merge Varsha 0911_2013 Beacon Sync code
 *
 * Revision 1.5  2013/09/04 15:49:18  yiming
 * comment out old line of code
 *
 * Revision 1.4  2013/09/04 14:43:30  yiming
 * New changes for Hybrii_A code merge
 *
 * Revision 1.3  2013/06/04 20:29:05  yiming
 * Merge 0603_2013 Hybrii A Code to Hybrii_B Test Code
 *
 * Revision 1.2  2013/01/24 00:13:46  yiming
 * Use 01-23-2013 Hybrii-A code as first Hybrii-B code base
 *
 * Revision 1.24  2013/01/22 12:41:38  prashant
 * Fixing build issues
 *
 * Revision 1.23  2013/01/17 16:06:03  ranjan
 * datapath stability fixes
 *
 * Revision 1.22  2013/01/15 12:26:11  ranjan
 * a)fixed issues in swQ for plc->host intf datapath and
 *    swQ for host -> plc datapath
 *
 * Revision 1.21  2013/01/04 16:11:22  prashant
 * SPI to PLC bridgeing added, Queue added for SPI and Ethernet
 *
 * Revision 1.20  2012/12/14 11:06:57  ranjan
 * queue added for eth to plc datapath
 * removed mgmt tx polling
 *
 * Revision 1.19  2012/11/13 22:39:43  son
 * Added Reset and RX reable primitives
 *
 * Revision 1.18  2012/10/11 06:21:00  ranjan
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
 * Revision 1.17  2012/09/08 04:01:43  son
 * Integrated SPI, Zigbee into common interrupt service function
 *
 * Revision 1.16  2012/07/19 21:46:07  son
 * Prepared files for zigbee integration
 *
 * Revision 1.15  2012/07/18 22:02:11  son
 * Changed ISM Polling task name
 *
 * Revision 1.14  2012/07/14 04:07:58  kripa
 * Reverting the change to ISM Poll task temporarily, to avoid a unknown crash.
 * Committed on the Free edition of March Hare Software CVSNT Client.
 * Upgrade to CVS Suite for more features and support:
 * http://march-hare.com/cvsnt/
 *
 * Revision 1.13  2012/07/12 22:05:55  son
 * Moved ISM Polling to ISM Task.
 * UI is now part of init task
 *
 * Revision 1.12  2012/07/04 19:08:36  kripa
 * Calling PnedingTxProc funciton from interrupt polling routine.
 * Committed on the Free edition of March Hare Software CVSNT Client.
 * Upgrade to CVS Suite for more features and support:
 * http://march-hare.com/cvsnt/
 *
 * Revision 1.11  2012/06/20 17:29:13  kripa
 * Adding Bcn3SentIntHandler()
 * Committed on the Free edition of March Hare Software CVSNT Client.
 * Upgrade to CVS Suite for more features and support:
 * http://march-hare.com/cvsnt/
 *
 * Revision 1.10  2012/06/13 06:24:31  yuanhua
 * add code for tx bcn interrupt handler integration and data structures for region entry schedule. But they are not in execution yet.
 *
 * Revision 1.9  2012/06/07 06:10:29  yuanhua
 * (1) free CPs if frame tx fails (2) add compiler flag HAL_INT_HDL to differentiate the interrupt and interrupt handler. (3) enable all interrupts during the system initialization.
 *
 * Revision 1.8  2012/06/05 22:37:11  son
 * UART console does not get initialized due to task ID changed
 *
 * Revision 1.7  2012/06/05 07:25:58  yuanhua
 * (1) add a scan call to the MAC during the network discovery. (2) add a tiny task in ISM for interrupt polling (3) modify the frame receiving integration. (4) modify the tiny task in STM.
 *
 * Revision 1.6  2012/06/04 23:09:18  son
 * Timer Handler to be called from RTX51 OS
 *
 * Revision 1.5  2012/05/19 22:22:16  yuanhua
 * added bcn Tx/Rx non-callback option for the ISM.
 *
 * Revision 1.4  2012/05/12 04:11:46  yuanhua
 * (1) added list.h (2) changed the hal tx for the hw MAC implementation.
 *
 * Revision 1.3  2012/04/19 16:46:30  yuanhua
 * fixed some C51 compiler errors for the integration.
 *
 * Revision 1.2  2012/04/13 06:15:10  yuanhua
 * integrate the HPGP protocol stack with the HAL for the beacon TX/RX and mgmt msg RX.
 *
 * Revision 1.1  2011/07/03 05:58:49  jie
 * Initial check in
 *
 * Revision 1.1  2011/05/06 18:31:47  kripa
 * Adding common utils and isr files for Greenchip firmware.
 *
 * Revision 1.1  2011/04/08 21:41:00  yuanhua
 * Framework
 *
 *
 * =========================================================*/


