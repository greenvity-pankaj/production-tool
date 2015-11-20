/*
* $Id: hal_common.c,v 1.38 2015/01/02 14:55:35 kiran Exp $
*
* $Source: /home/cvsrepo/Hybrii_B_OSFW_Dev/firmware/hal/hal_common.c,v $
* 
* Description  : Common HAL module.
* 
* Copyright (c) 2010-2013 Greenvity Communications, Inc.
* All rights reserved.
*
* Purpose      :
*     Implements HAL APIs for accessing the common HW features like Pkt buffer.
*     Additionally implements Ext Int ISR and Timer ISRs.
*
*/
#ifdef RTX51_TINY_OS
#include <rtx51tny.h>
#endif
#include <stdio.h>
#include <string.h>
#include "fm.h"
#ifdef ROUTE
#include "hpgp_route.h"
#endif
#include "list.h"
#include "event.h"
#include "nma.h"
#include "nma_fw.h"
#ifndef HPGP_HAL_TEST
#include "hpgpapi.h"
#endif
#include "hpgpevt.h"
#include "hpgpdef.h"
#include "hal_common.h"
#include "hal.h"
#include "hal_hpgp.h"
#include "hal_eth.h"
#ifdef HYBRII_SPI
#include "hal_spi.h"
#endif
#include "hal_hpgp.h"
#include "frametask.h"
#include "datapath.h"
#include "hal_hpgp_reset.h"
#include "hal_regs_def.h"
#include "ism.h"
#include "sys_config_data_utils.h"
#include "gv701x_gpiodriver.h"


extern u8 gsyncTimeout;
sysConfig_t sysConfig;
#ifdef FREQ_DETECT

extern u32 PLC_MIN_AC_BPLEN; 
extern u32 PLC_MAX_AC_BPLEN;
extern u32 AC_MIN_THRESHOLD;
extern u32 PLC_DC_BP_LEN;

#endif
#ifdef UM
#include "sys_config_data_utils.h"
#include "ctrll.h"
#endif
#include "gv701x_gpiodriver.h"
#include "gv701x_flash_fw.h"

u8 gEthMacAddrDef[] = {0x00,0x11,0x22,0x33,0x44,0x55};
u8 gEthMacAddrBrdcast[] = {0xff,0xff,0xff,0xff,0xff,0xff};
u8  bcAddr[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

//#define BPSTO_VALUE 0x1B
u32 gtimer2, gtimer1;
extern u8 gsendBcn;
extern u8 gPlcPendingHead;
extern u8 gPlcPendingTail;
#ifdef ETH_BRDG_DEBUG
extern u8 myDebugFlag;
extern u8 myDebugFlag1;
extern u32 TotalRxCpCnt;
extern u32 TotalRxFrameCnt;
#ifdef ETH_BRDG_DEBUG
extern u32 oldNumEthTxDoneInts;
#endif
#endif
#ifdef LOG_FLASH
extern u16 *blockId;
#endif
sHalCB     gHalCB;    // Common  HAL Control Blocl   

// Flags for sniffer and eth_plc bridge
u8 eth_plc_bridge = 0;
u8 eth_plc_sniffer = 0;

#ifdef HPGP_HAL_TEST
u8 opMode = LOWER_MAC;
#else
u8 opMode = UPPER_MAC;
#endif

extern u8 spiflash_ReadByte(u32);
extern void spiflash_wrsr_unlock(u8);
extern void spiflash_WriteByte(u32, u8);

u32 gCCO_NTB;
#ifdef FREQ_DETECT
extern void HHAL_SetPlcDevMode(ePlcDevMode plcDevMode);
extern void HHAL_SetACLine50HzFlag(eRegFlag acLin50Hz);
#endif

#ifdef DEBUG_DATAPATH
u8 sigDbg = 0;
u8 pktDbg = 0;
u8 ethQueueDebug = 0;
#endif


// Interface flag
u8 hostIntf = HOST_INTF_NO;
//#ifdef UM
u8 hostDetected = FALSE;
//#endif
/*******************************************************************
* NAME :            CHAL_GetFreeCPCnt
*
* DESCRIPTION :     Get the number of free CPs available.
*
* INPUTS :
*       PARAMETERS:
*           None
*
* OUTPUTS :
*       PARAMETERS:
*           None
*
*       RETURN :
*            Type:   u8
*            Values: CP count.
*/
u8 CHAL_GetFreeCPCnt() __REENTRANT__
{
    uFreeCpCntReg  freeCpCnt;
     
    freeCpCnt.reg = ReadU32Reg(CPU_FREECPCOUNT_REG);

    return ((u8)freeCpCnt.s.cpCnt);
}


/**
 * NAME :            CHAL_AllocFrameCp
 *
 * DESCRIPTION :     
 *       Allocate an array of CPs as rquested.
 * INPUTS :
 *       PARAMETERS:
 *           u8 *cp    array of CPs for allocated CP index.
 *           u8 numCp  size of array
 *
 * OUTPUTS :
 *       PARAMETERS:
 *           None
 *
 *       RETURN 
 *           STATUS_SUCCESS if numCp CPs are allocated. 
 *           STATUS_FAILURE, otherwise.
 */

eStatus CHAL_AllocFrameCp(sCpSwDesc *cpDesc, u8 numCp)
{
    u8 i;
    u8 numFreeCp = CHAL_GetFreeCPCnt();
  
    if (numFreeCp < numCp)
        return STATUS_FAILURE;
          
    for (i = 0; i < numCp; i++)
    {
        if (CHAL_RequestCP(&(cpDesc[i].cp)) == STATUS_FAILURE)
        {
           // FM_Printf(FM_ERROR,"CHAL: error in CP request\n");
            CHAL_FreeFrameCp(cpDesc, i-1);
            return STATUS_FAILURE;
        }
    }
    return STATUS_SUCCESS;
}




/*******************************************************************
* NAME :            CHAL_GetAccessToCP
*
* DESCRIPTION :     Selects the right HW bank corres. to a CP 
*                   and returns the physical address.
*
* INPUTS :
*       PARAMETERS:
*           u8 cp    CP to be accessed.
*
* OUTPUTS :
*       PARAMETERS:
*           None
*
*       RETURN :
*            Type:   u8 xdata* 
*            Values: Cell (PktBuf) physical address.
*/
u8 XDATA* CHAL_GetAccessToCP( u8 cp)
{
    u8 XDATA *cellAddr;
    u8   bank;
    uPktBufBankSelReg pktBufBankSel;
#ifdef UART_HOST_INTF	
	u8 intFlag;
#endif

#ifdef UART_HOST_INTF
	intFlag = EA;
    EA = 0;
#endif
    bank     = (cp & 0x60) >>5;

    pktBufBankSel.reg = ReadU32Reg(CPU_PKTBUFBANKSEL_REG);
    pktBufBankSel.s.bank = bank;
    WriteU32Reg(CPU_PKTBUFBANKSEL_REG, pktBufBankSel.reg);
    cellAddr = (u8 XDATA *) ((u32)MAC_PKTBUF_BASEADDR+ (u32)( (((u32)cp) & 0x1F)<<7) );
#ifdef UART_HOST_INTF
	EA = intFlag;
#endif
    return cellAddr;
}

/*******************************************************************
* NAME :            CHAL_GetCPUsageCnt
*
* DESCRIPTION :     Get usage count corres. to a given CP. 
*                   
*
* INPUTS :
*       PARAMETERS:
*           u8 cp    CP whose usage count is being queried.
*
* OUTPUTS :
*       PARAMETERS:
*           None
*
*       RETURN :
*            Type:   u8  
*            Values: Usage count for the CP ( Range = 0 to 15 )
*/
u8 CHAL_GetCPUsageCnt(u8 cp)
{
    uCpUsageCntIdxReg   cpUsageCntIdx;
    uCpUsageCntReg      cpUsageCnt;
     
    cpUsageCntIdx.reg = 0;
    cpUsageCntIdx.s.cpIdx = cp;

    WriteU32Reg(CPU_CPUSAGECNTIDX_REG, cpUsageCntIdx.reg);
    cpUsageCnt.reg = ReadU32Reg(CPU_CPUSAGECNT_REG);

    return (cpUsageCnt.s.usageCnt);
}

/*******************************************************************
* NAME :            CHAL_IncrementCPUsageCnt
*
* DESCRIPTION :     Increase the usage count corres. to a given CP
*                   by a given value. 
*                   
*
* INPUTS :
*       PARAMETERS:
*           u8 cp      CP whose usage count is to be incremented.
*           u8 cpCnt   Value to be added to current usage count.  
*
* OUTPUTS :
*       PARAMETERS:
*           None
*
*       RETURN :
*            None
*/
void CHAL_IncrementCPUsageCnt(u8 cp, u8 cpCnt)
{
    uCpUsageCntIdxReg   cpUsageCntIdx;
    uCpUsageCntReg      cpUsageCnt;
     
    cpUsageCntIdx.reg     = 0;
    cpUsageCnt.reg        = 0;
    cpUsageCntIdx.s.cpIdx = cp;
    cpUsageCnt.s.usageCnt    = cpCnt;

    WriteU32Reg(CPU_CPUSAGECNTIDX_REG, cpUsageCntIdx.reg);
    WriteU32Reg(CPU_CPUSAGECNT_REG, cpUsageCnt.reg);

    return;
}

#if 0
void CP_displayLocalBuf(u8 *buf)
{
	u8 i;

	printf("\nContents of buf:\n");
	for (i = 0; i < HYBRII_CELLBUF_SIZE; i++)
	{
		printf("%bu ", buf[i]);
		if (i && !(i % 20))
			printf("\n");
	}
	printf("\n");
}
#endif

#ifdef STEVE // CPInitDone bit is not defined in Hybrii B. Confirm with Steve
/*******************************************************************
* NAME :            CHAL_SetSwStatCPInitDoneFlag
*
* DESCRIPTION :     Set the CP Init done flag - to be done after releasing
*                   all 128 CPs just once, during initialization. 
*                   
*
* INPUTS :
*       PARAMETERS:
*           None 
*
* OUTPUTS :
*       PARAMETERS:
*           None
*
*       RETURN :
*            None
*/
void CHAL_SetSwStatCPInitDoneFlag(eRegFlag regFlag)
{
    uCpuSwStatusReg cpuSwStatus;

    cpuSwStatus.reg = ReadU32Reg(CPU_SWSTATUS_REG);
    cpuSwStatus.s.cpInitDone = regFlag;
    WriteU32Reg(CPU_SWSTATUS_REG,cpuSwStatus.reg); 
}
#endif


u8 CHAL_GetCPUTxQDescCount()
{
    uCpuTxQStatReg   cpuTxQStat;
    u8               txQDescCnt; 

    cpuTxQStat.reg = ReadU32Reg(CPU_CPUTXSTATUS_REG);
    txQDescCnt   = (u8)cpuTxQStat.s.txQDescCntHi;
    txQDescCnt   = (txQDescCnt << CPUTXSTATUS_DESCCNTHI_POS)| cpuTxQStat.s.txQDescCntLo;
    return txQDescCnt;
}

  
u8 CHAL_GetCPUTxQFrmCount()
{
    uCpuTxQStatReg   cpuTxQStat;
    u8               txQFrmCnt; 

    cpuTxQStat.reg = ReadU32Reg(CPU_CPUTXSTATUS_REG);
    txQFrmCnt   = (u8)cpuTxQStat.s.txQFrmCnt;
    return txQFrmCnt;
}

#if 0
void CHAL_InitSW()
{
    
    //in future read flash to see line mode and program accordingly
//    uPlcLineControlReg    plcLineCtrl; 
     uCSMARegionReg	 pCSMARgn;

    gHpgpHalCB.lineMode = LINE_MODE_DC;
    gHpgpHalCB.devMode   = DEV_MODE_CCO;
    gHpgpHalCB.curBcnPer = PLC_DC_BP_LEN;
    
/*#ifdef AC_LINECYCLE_50HZ
                HHAL_SetACLine50HzFlag(REG_FLAG_SET);
#else                   
                HHAL_SetACLine50HzFlag(REG_FLAG_CLR);
#endif */   //AC_LINECYCLE_50HZ  

    pCSMARgn.s.csma_start_time_lo = 0; //start_time;
	pCSMARgn.s.csma_start_time_hi = 0;
    pCSMARgn.s.csma_rxOnly = 0;
	pCSMARgn.s.csma_hybrid = 1;

    if(gHpgpHalCB.lineMode == LINE_MODE_DC)
    {
        pCSMARgn.s.csma_endtime_lo = 0xF43 & 0x00FF;  //f42 * 10.24 = 40ms
    	pCSMARgn.s.csma_endtime_hi = ((0xF43  & 0xFF00) >> 8);
    }
    else if(gHpgpHalCB.lineMode == LINE_MODE_AC)
    {
        pCSMARgn.s.csma_endtime_lo = 0xCB6 & 0x00FF;
    	pCSMARgn.s.csma_endtime_hi = ((0xCB6  & 0xFF00) >> 8);
    }
    
    
    WriteU32Reg(PLC_CSMAREGION0_REG, pCSMARgn.reg); 
           
    gHpgpHalCB.perSumCnt       = 0;
    gHpgpHalCB.bPerAvgInitDone = 0;
//    gHpgpHalCB.halStats.TxBcnCnt = 0;
//    gHpgpHalCB.halStats.TotalTxFrmCnt = 0;
    	 // put it in initsw routine (everything till end of func)
    // Initialize HPGP HAL Control Blok  
    gHpgpHalCB.diagModeEnb       = 0;
    gHpgpHalCB.bcnInitDone       = 0;
    gHpgpHalCB.ppekValidReg      = 0;
    gHpgpHalCB.syncComplete      = 0;
    gHpgpHalCB.scanEnb           = 0;
    gHpgpHalCB.swSyncEnb         = 0;
    gHpgpHalCB.lastNtbB4         = 0;
    gHpgpHalCB.lastNtbAft        = 0;
    gHpgpHalCB.lastBpst          = 0;
    gHpgpHalCB.lastBcnRxTime     = 0;
    gHpgpHalCB.bcnPerSum         = 0;
    gHpgpHalCB.curBcnPer         = 0;
    gHpgpHalCB.perSumCnt         = 0;
    gHpgpHalCB.bPerAvgInitDone   = 0;
    gHpgpHalCB.bBcnNotSent       = 0; 
    gHpgpHalCB.BcnTxWaitTimeoutCnt = 0;
    gHpgpHalCB.nwSelected        = 0;
    gHpgpHalCB.bTxPending        = 0;
    gHpgpHalCB.BcnLateCnt = 0;
    gHpgpHalCB.halStats.STAlagCCOCount = 0;
    gHpgpHalCB.halStats.STAleadCCOCount = 0;
#ifndef HPGP_HAL_TEST	
    gHpgpHalCB.tei               = 0;
#endif	
    gHpgpHalCB.plcTx10FC.reg = HPGP_HP10FC_DEF;
    memcpy(gHpgpHalCB.nid, gDefNID, NID_LEN);

  	gHpgpHalCB.gPendingHead  = 0;
	gHpgpHalCB.gPendingTail = 0;
	memset(&gHpgpHalCB.gPending ,0x00, sizeof(gHpgpHalCB.gPending));


    //HHAL_SetSnid(0);
    //HHAL_SetDevMode(DEV_MODE_CCO, LINE_MODE_DC);
   
    
}
#endif //0
    
/*******************************************************************
* NAME :            CHAL_InitHW
*
* DESCRIPTION :     Init common resources like CPs, common HAL CB, 
*                   Enable Timer Int, Ext Int, UART and timer. 
*                   
*
* INPUTS :
*       PARAMETERS:
*           None  
*
* OUTPUTS :
*       PARAMETERS:
*           None
*
*       RETURN :
*            None
*/
void CHAL_InitHW()
{
#if defined (HPGP_HAL_TEST) || defined(UM)
//    uInterruptReg       intMacEnable;
#if INT_POLL
#else     
    u8051InterruptReg   int8051Enable;
    u8051InterruptReg   int8051Irq;
#endif
#endif
//    eRegFlag            regFlag;
    u8                  cpNum;
    //u8                  cpUsageCnt; 

    // 1. Init Cell pointers
    for (cpNum=0 ; cpNum<HYBRII_CPCOUNT_MAX ; cpNum++)
    {
         //for( cpUsageCnt=0 ; cpUsageCnt<HYBRII_CPUSAGECOUNT_MAX ; cpUsageCnt++)
         {
            CHAL_DecrementReleaseCPCnt(cpNum);
         }
    }

/* below is moved to the hal and ism */

#if defined (HPGP_HAL_TEST) || defined(UM)
    // 5. Initialize HAL Control block
    memset(&gHalCB, 0, sizeof(gHalCB));

    // 3. Enable MAC Interrupts - routed through External Int 0
    //sw does polling so no need to do int enb
	/*intMacEnable.reg   = 0;
    //intMacEnable.reg |= CPU_INTERRUPT_ALLINTSMASK;
    intMacEnable.s.ethFreeCP      = 1;
    intMacEnable.s.hpgpBP         = 1;
    intMacEnable.s.plcBcn3Sent    = 1;
    intMacEnable.s.plcBcnRx       = 1;
    intMacEnable.s.cpuTxQNonEmpty = 1;
    //intMacEnable.s.hpgpBPSta      = 1;
    intMacEnable.s.plcMedStatInt  = 1;
    WriteU32Reg(CPU_INTENABLE_REG,intMacEnable.reg); */

    // 6. Program ClksPer2Us value
    // 7. Start 1 ms timer running
    // Timer0 in Mode
    TMOD = (TMOD & 0xF0) | 0x01;
    
    // Load Timer register
    TH0 = HYBRII_MSTIMER25MHZ_HI;
    TL0 = HYBRII_MSTIMER25MHZ_LO;
    
    // Start Timer
    TR0 = 1;

    // 4. Enable 8051 interrupts
    // Enable 8051 Timer2 Int & External Int     
    
#ifdef UART_HOST_INTF 
    ET0 = 0;                    // Disable Timer0 Int
#else
#ifdef _TIMER_INTERRUPT_
    ET0 = 1;                    // Enable Timer0 Int
#endif
#endif
    IT0 = 0;                    // Set the interrupts to Lowlevel-triggered 
    IT1 = 0;
    EA  = 1;                    // Overall Int Enable
#if INT_POLL
#else

    EX1 = 1;
    int8051Irq.reg = 0;
    int8051Irq.s.ext1 = 1;
    WriteU32Reg(INTIRQ_8051_REG,int8051Irq.reg);

    int8051Enable.reg = 0;
    int8051Enable.s.ext1 = 1;
    int8051Enable.s.globalIntEna = 1;
    WriteU32Reg(INTENA_8051_REG,int8051Enable.reg);
#endif
#endif /* HPGP_HAL_TEST */
#ifdef UART_HOST_INTF 
{
	u8051InterruptReg	int8051Enable;
	u8051InterruptReg	int8051Irq;
	IT0 = 0;                    // Set the interrupts to Lowlevel-triggered 
    IT1 = 0;
	EX1 = 1;
	EA  = 1;                    // Overall Int Enable
   int8051Irq.reg = 0;
   //int8051Irq.s.ext1 = 1;
   int8051Irq.s.uart0 = 1;
   WriteU32Reg(INTIRQ_8051_REG,int8051Irq.reg);

   int8051Enable.reg = 0;
  //int8051Enable.s.ext1 = 1;
   int8051Enable.s.uart0 = 1;
   int8051Enable.s.globalIntEna = 1;
   WriteU32Reg(INTENA_8051_REG,int8051Enable.reg);
}
#endif
}

void hal_common_display_qc_error_stats (void)
{
    printf("\nQ Controller Stat:");
    printf("\n  S/W Frame Count = %u, No 1st Desc = %bu, Too many desc = %bu, "
           " No desc = %bu, qc_no_grant=%bu, cp_no_grant_free_cp=%d\n" 
           " qc_no_grant_alloc_cp=%d, qc_no_grant_write_cp=%d, qc_no_grant_read_cp=%d\n", 
		   (u16)gHalCB.frmsCnt, gHalCB.qc_no_1st_desc,
           gHalCB.qc_too_many_desc, gHalCB.qc_no_desc, gHalCB.qc_no_grant, gHalCB.cp_no_grant_free_cp, 
		   gHalCB.cp_no_grant_alloc_cp, gHalCB.cp_no_grant_write_cp, gHalCB.cp_no_grant_read_cp);
}



#ifdef HPGP_HAL_TEST
void CHAL_CpuTxQNemptyIntHandler()
#else
void CHAL_FrameRxIntHandler(void *cookie)
#endif
{
//    u8 frms_cnt;
    u8 desc_cnt;
    u8 rx_processing;
    u8 i; 
    u16 frmLen;
#if !defined (HPGP_HAL_TEST)  && !defined(UM)    
    u8  frmOffset;
    u8  snid;
    u16 ssn;
    eStatus status;

#endif
#ifdef HYBRII_B_FC
//    sPHYblockHeader  rxFrmPBHdr;
//    u32              rxFrmFC[4];  //store FC VF fileds
//    uPlcTxPktQDescVF0 rxFrmFCVF0;
//    uPlcTxPktQDescVF1 rxFrmFCVF1;
//    uPlcTxPktQDescVF2 rxFrmFCVF2;
//    uPlcTxPktQDescVF3 rxFrmFCVF3;
//	u32               rxFrmPBCS;
//	uPlcCpuRdCnt0Reg  rxFrmFcCnt;
//	uPlcCpuRdCnt1Reg  rxFrmPBCnt;
#endif

    sSwFrmDesc rxFrmSwDesc;
    uRxFrmHwDesc rxFrmHwDesc;
	uRxCpDesc    rxCpDesc;
#ifndef HPGP_HAL_TEST
    volatile u8 XDATA *cellAddr;
    sHaLayer *hal= (sHaLayer *)cookie; 
#endif


#ifdef ETH_BRDG_DEBUG
    if (myDebugFlag1)
        printf(" RX Int. Handler: recvd a frame\n");
#endif
			
	memset(&rxFrmSwDesc, 0x00, sizeof(sSwFrmDesc));
	memset(&rxFrmHwDesc, 0x00, sizeof(uRxFrmHwDesc));
	memset(&rxCpDesc, 0x00, sizeof(uRxCpDesc));
	
    //[YM] Add Hybrii_B FC and PBHdr process code here, before process Frame Payload
#ifdef HYBRII_B_FC
    rxFrmFcCnt.reg = ReadU32Reg(PLC_FC_CP_CNT_REG);
    rxFrmPBCnt.reg = ReadU32Reg(PLC_PBH_PBCS_CNT_REG);
	
#endif

//    frms_cnt = CHAL_GetCPUTxQFrmCount();
__CRIT_SECTION_BEGIN__ //kiran CP loss issue with interrupts
    desc_cnt = CHAL_GetCPUTxQDescCount();
__CRIT_SECTION_END__ //kiran

    rx_processing = 0;
	
    while (desc_cnt) {
        u8                  last_desc_len;
        uRxPktQDesc1        rx_q_desc;
        uRxPktQCPDesc       q_desc_cp;
        sCommonRxFrmSwDesc  rx_frame_info;

#ifdef ETH_BRDG_DEBUG
        if (myDebugFlag1)
            printf(" RX Int.  frms_cnt = %bu, gHalCB.frmsCnt=%bu, desc_cnt=%bu\n", frms_cnt, gHalCB.frmsCnt, desc_cnt);
#endif
        // Request for right to read the CPU TX QD
        if (HHAL_Req_Gnt_Read_CPU_QD() == STATUS_FAILURE)
            break;

        // Always expect a firstdescriptor here
        // else error cases
	
        memset(&rx_frame_info.hdrDesc, 0x00, sizeof(uRxPktQDesc1));
		memset(&rx_q_desc, 0x00, sizeof(uRxPktQDesc1));
		memset(&q_desc_cp, 0x00, sizeof(uRxPktQCPDesc));
		
 __CRIT_SECTION_BEGIN__ //kiran CP loss issue with interrupts	
        rx_q_desc.reg = ReadU32Reg(CPU_TXQDESC_REG);
        if (NO_DESCRIPTOR == rx_q_desc.reg) {
			//printf("\na\n");
            gHalCB.qc_no_desc++;
__CRIT_SECTION_END__ //kiran
            break;
        }
 __CRIT_SECTION_END__ //kiran	 
 
        if (rx_q_desc.s.bFirstDesc) {
            u8              src_port;
            bool            valid_frame; 
#ifdef DUP_CP 
            u8        privCp;
#endif	    

            src_port = rx_q_desc.s.srcPort;
            rx_frame_info.hdrDesc.reg = rx_q_desc.reg;
            rx_frame_info.cpCount = 0;
            valid_frame = TRUE;
            rx_frame_info.rssiLqi.reg = ReadU32Reg(PLC_RSSILQI_REG);//kiran
#ifdef HYBRII_B_FC  //[YM] Add Hybrii_B FC and PBHdr process code here, before process Frame Payload
                  //if ((frms_cnt != rxFrmFcCnt.s.FcCntLo)||(frms_cnt != rxFrmPBCnt.s.PbhCnt_Lo))
                    //printf(" Receive frame count does not match with FC count or PB Hdr count\n");
              if ((rxFrmFcCnt.s.FcCntLo > 0)&&(rxFrmPBCnt.s.PbhCnt_Lo> 0) && (src_port == PORT_PLC))
              {
                    rxFrmPBHdr.pbh = ReadU32Reg(PLC_PHYBLOCK_REG);
        
                    //if ((rxFrmPBHdr.s.ssn_lo > 0)||(rxFrmPBHdr.s.ssn_hi > 0))
                        //printf("Rx PHY Block needs segement process, ssn_lo = %bu, ssn_hi = %bu\n", rxFrmPBHdr.s.ssn_lo, rxFrmPBHdr.s.ssn_hi);
                    
                    rxFrmPBCS = ReadU32Reg(PLC_PBCS_DATA_REG);  //No Use for Software
        
                    // [YM] temp code, just read FC header our from FC ram, no further process
                    rxFrmFCVF0.reg = ReadU32Reg(PLC_FC_DATA_REG);
                    rxFrmFCVF1.reg = ReadU32Reg(PLC_FC_DATA_REG);
                    rxFrmFCVF2.reg = ReadU32Reg(PLC_FC_DATA_REG);
                    rxFrmFCVF3.reg = ReadU32Reg(PLC_FC_DATA_REG);

                    rx_frame_info.fc[0] = rxFrmFCVF0.reg;
                    rx_frame_info.fc[1] = rxFrmFCVF1.reg;
                    rx_frame_info.fc[2] = rxFrmFCVF2.reg;
                    rx_frame_info.fc[3] = rxFrmFCVF3.reg;
               //     printf("dt : %bx\n", rxFrmFCVF0.s.dt_av);
                    
              }
            
#endif
__CRIT_SECTION_BEGIN__ //kiran CP loss issue with interrupts

            q_desc_cp.reg = ReadU32Reg(CPU_TXQDESC_REG);
__CRIT_SECTION_END__ //kiran

            if (NO_DESCRIPTOR == q_desc_cp.reg) {
				//printf("\nb\n");
                gHalCB.qc_no_desc++;
                valid_frame = FALSE;
            } else {
                // Store first CP desciptor and the associate CP
                rx_frame_info.firstCpDesc.reg = q_desc_cp.reg;
                rx_frame_info.cpArr[rx_frame_info.cpCount++] = 
                            q_desc_cp.s.cp;
#ifdef DUP_CP               
                privCp = q_desc_cp.s.cp;
#endif
                // Read until the last descriptor
__CRIT_SECTION_BEGIN__ //kiran CP loss issue with interrupts
                while (q_desc_cp.s.lastDesc == 0) {
                    q_desc_cp.reg = ReadU32Reg(CPU_TXQDESC_REG);
                    if (NO_DESCRIPTOR == q_desc_cp.reg) {
						//printf("\nc\n");
                        gHalCB.qc_no_desc++;
                        valid_frame = FALSE;
                        break;
                    }
                    if (rx_frame_info.cpCount < HYBRII_CPPERFRMCOUNT_MAX) {
                        // Extract Cell Pointer
                        rx_frame_info.cpArr[rx_frame_info.cpCount++] = 
                            q_desc_cp.s.cp;
#ifdef DUP_CP			    
						
							{
								if(privCp != q_desc_cp.s.cp)
								{
									privCp = q_desc_cp.s.cp;
								}
								else
								{
									FM_Printf(FM_USER,"dup cp : %bu\n",q_desc_cp.s.cp);
									privCp = q_desc_cp.s.cp;
								}
							}
#endif							
                    } else {
                        // Error. Don't expect this many descriptor
                        CHAL_DecrementReleaseCPCnt(q_desc_cp.s.cp);
                        valid_frame = FALSE;
                        gHalCB.qc_too_many_desc++;
                    }
                }
__CRIT_SECTION_END__ //kiran
            }

#ifdef ETH_BRDG_DEBUG
			TotalRxCpCnt += rx_frame_info.cpCount;
		    TotalRxFrameCnt++;
#endif
						
            // 1.1.5 Increment SW copy of FramesCount 
            gHalCB.frmsCnt ++;
__CRIT_SECTION_BEGIN__ //kiran CP loss issue with interrupts

			desc_cnt = CHAL_GetCPUTxQDescCount();

            HHAL_Rel_Gnt_Read_CPU_QD();     // Release CPQD Grant after finish reading a frame
__CRIT_SECTION_END__ //kiran

            if (valid_frame == FALSE) {
                hal_common_free_frame(&rx_frame_info);
            } else {
                last_desc_len = q_desc_cp.s.descLenHi;
                last_desc_len = (last_desc_len << PKTQDESCCP_DESCLENHI_POS) | 
                                q_desc_cp.s.descLenLo;
                rx_frame_info.lastDescLen = last_desc_len;
                
                rxFrmHwDesc.reg = rx_q_desc.reg;
                frmLen = rxFrmHwDesc.gnl.frmLenHi;
                frmLen = (frmLen << PKTQDESC1_FRMLENHI_POS) | rxFrmHwDesc.gnl.frmLenLo;
                rxFrmSwDesc.frmLen = frmLen;
                rxFrmSwDesc.frmType = rxFrmHwDesc.gnl.frmType;
                rxFrmSwDesc.rxPort = rxFrmHwDesc.gnl.srcPort;

                rxFrmSwDesc.lastDescLen = rx_frame_info.lastDescLen;
                rxFrmSwDesc.cpCount = rx_frame_info.cpCount; 
#ifdef DUP_CP
				privCp = 0xFF;
#endif				
                for (i = 0; i < rxFrmSwDesc.cpCount; i++)
                {
                    rxFrmSwDesc.cpArr[i].cp = rx_frame_info.cpArr[i];
#ifdef DUP_CP
					{	
						if(privCp != rx_frame_info.cpArr[i])
						{
							privCp = rx_frame_info.cpArr[i];
						}
						else
						{
							FM_Printf(FM_ERROR,"dup cpx : %bu\n",rx_frame_info.cpArr[i]);
							privCp = rx_frame_info.cpArr[i];
						}	
					}
#endif					
                }
#ifndef HPGP_HAL_TEST
                /* find the destination Ethernet header */
                /* by reading the first CP */
               cellAddr = CHAL_GetAccessToCP(rxFrmSwDesc.cpArr[0].cp);
//FM_HexDump(FM_DATA|FM_MINFO, "rx buff:", hal->buf, HAL_BUF_LEN ); 
#endif
#ifdef ETH_BRDG_DEBUG
               if (myDebugFlag1)
                    printf("RX Int.Handler: rxFrmSwDesc.cpCnt=%bu, TakenCPCnt=%bu\n",rxFrmSwDesc.cpCount,128-CHAL_GetFreeCPCnt());
               if (myDebugFlag)
                    printf(" RX Int.Handler: src_port=%bu\n",src_port);
#endif
				rx_frame_info.hdrDesc.s.srcPort = src_port;
                switch (src_port) {
#ifdef HYBRII_802154
                case PORT_ZIGBEE:
#ifdef NO_HOST					
					rx_frame_info.hdrDesc.s.dstPort = PORT_APP; 				
#else
					rx_frame_info.hdrDesc.s.dstPort = PORT_HOST; 				
#endif
                    mac_hal_qc_frame_rx(&rx_frame_info);
                    break;
#endif
#ifdef HYBRII_SPI
                case PORT_SPI:
					rx_frame_info.hdrDesc.s.dstPort = PORT_PLC;					
#ifndef HPGP_HAL_TEST	   
                    hal_spi_frame_rx(hal, &rx_frame_info);   
#else
                    hal_spi_frame_rx(&rx_frame_info);   
#endif
                    break;
#endif
#ifdef HYBRII_ETH
                case PORT_ETH:
					rx_frame_info.hdrDesc.s.dstPort = PORT_PLC;					
#ifdef HYBRII_ETH
                         //              FM_Printf(FM_ERROR, "e r\n");

#ifndef HPGP_HAL_TEST                      
                    Host_RxHandler(hal, &rx_frame_info);
#else
                    Host_RxHandler(&rx_frame_info);
#endif
#endif
                    break;
#endif
#ifdef HYBRII_HPGP
                case PORT_PLC:
#ifdef NO_HOST					
					rx_frame_info.hdrDesc.s.dstPort = PORT_APP; 				
#else
					if(hostIntf == HOST_INTF_SPI)
                    {
#ifdef HYBRII_SPI
                        rx_frame_info.hdrDesc.s.dstPort = PORT_SPI;
#endif
                    }
                    else if(hostIntf == HOST_INTF_ETH)
                    {
                        rx_frame_info.hdrDesc.s.dstPort = PORT_ETH;
                    }
                    else if(hostIntf == HOST_INTF_UART)
                    {
                        rx_frame_info.hdrDesc.s.dstPort = PORT_UART;
                    }
                    else
                    {
					rx_frame_info.hdrDesc.s.dstPort = PORT_HOST;				
                    }
#endif
#if defined (HPGP_HAL_TEST)  || defined(UM)


#ifdef HPGP_HAL_TEST
                   //FM_Printf(FM_ERROR, "p r\n");
                   HHAL_RxIntHandler(&rx_frame_info);

#else
                   HHAL_RxIntHandler(&rx_frame_info, hal);
#endif // end of HPGP_HAL_TEST ifdef



#else
                   rxCpDesc.reg = rx_frame_info.firstCpDesc.reg;
                   ssn    = rxCpDesc.plc.ssnHi;
                   ssn    = (ssn << PKTQDESC1_SSNHI_POS ) | rxCpDesc.plc.ssnLo;
                   rxFrmSwDesc.frmInfo.plc.ssn = ssn;
                   /* Extract SNID */
                   snid   = rxCpDesc.plc.snidHi;
                   snid   = (snid << PKTQDESC1_SNIDHI_POS ) | rxCpDesc.plc.snidLo;
                   rxFrmSwDesc.frmInfo.plc.snid    = snid;
				   /* TODO: filter frames based on snid */
                    HHAL_ProcRxFrameDesc(hal, &rxFrmHwDesc,                                        
                                         &rxFrmSwDesc);
                    
                    frmOffset = ((rxFrmSwDesc.frmType == HPGP_HW_FRMTYPE_MGMT) ? 4 : 0);
                    

                    /* receive mgmt packets now, 
                     * but let the bridge to switch data packets */
                    if (rxFrmSwDesc.frmType == HPGP_HW_FRMTYPE_MGMT) 
                    {
                         status = HAL_RecvFrame(hal, &rxFrmSwDesc); 
                         if (status == STATUS_FAILURE)
                         {
                             CHAL_FreeFrameCp(rxFrmSwDesc.cpDesc, rxFrmSwDesc.cpCount);
                         }
                    }
					else
					{
                        FM_Printf(FM_ERROR,"HAL: Rx PLC data frame\n");
					}
#endif
                    break;
#endif /* HYBRII_HPGP */
                default:
                    {
	                    hal_common_free_frame(&rx_frame_info);
                        gHpgpHalCB.halStats.GswDropCnt++;
                    }
                    break;
                }

               /* TODO: call bridge to switch the data packets */
            }
        } else {
            // Error case - don't see the 1st descriptor
            // Free all CP's of the frame
            gHalCB.qc_no_1st_desc++;
            q_desc_cp.reg = rx_q_desc.reg;
            do {
                CHAL_DecrementReleaseCPCnt(q_desc_cp.s.cp);
__CRIT_SECTION_BEGIN__ //kiran CP loss issue with interrupts				
                q_desc_cp.reg = ReadU32Reg(CPU_TXQDESC_REG);
__CRIT_SECTION_END__ //kiran
                if (NO_DESCRIPTOR == q_desc_cp.reg) {
					//printf("\nd\n");
                    gHalCB.qc_no_desc++;
                    break;
                }
            } while (q_desc_cp.s.lastDesc == 0);
            gHalCB.frmsCnt ++;
        }
        if (rx_processing++ > RX_PACKETS_PROCESSING_MAX) {
            break;
        }

#ifdef Hybrii_B_FC
        rxFrmFcCnt.reg = ReadU32Reg(PLC_FC_CP_CNT_REG);
        rxFrmPBCnt.reg = ReadU32Reg(PLC_PBH_PBCS_CNT_REG);
#endif		
//        frms_cnt = CHAL_GetCPUTxQFrmCount();
//        desc_cnt = CHAL_GetCPUTxQDescCount();
#ifndef HYBRII_FPGA
#ifdef  HYBRII_HPGP
//        HHAL_ProcessPlcTxDone();
#endif

#endif
//		HHAL_Rel_Gnt_Read_CPU_QD();

#ifdef RTX51_TINY_OS
		
	//	os_set_ready(HYBRII_TASK_ID_FRAME);

//		os_switch_task();
//		FM_Printf(FM_USER,"\nx..");
#endif

    }

    // Make sure we release the CPU QD Grant
	HHAL_Rel_Gnt_Read_CPU_QD();
}
                                                                      
u32 get_TimerTick() 
{
    return (gHalCB.timerIntCnt);
	

}
#ifdef HPGP_HAL_TEST
#ifndef TIMER_POLL
void CHAL_IncTimerIntCnt() using 2
{
    gHalCB.timerIntCnt++;
    //hhal_timerHandler();
}
#endif
#endif
#ifdef SW_RECOVERY
u32 gPastRecoveryCount1 = 0, gRecoveryCount1 = 0;
u32 gPastRecoveryCount2 = 0, gRecoveryCount2 = 0;
u32 gRecoveryCount3 = 0;
u32 gL1=0;
u32 gL2=0;
u32 gL3=0;
u32 gBCNR = 0;
u8  gDiscStallCounter = 0;
u8  gBcnStallCounter  = 0;
u32 gDiscStall = 0;
u32 gBcnStall = 0;

void Monitor_Hang()
{  
  
    u32 hangIntRegRead; // Remove it
    u32 reg_value;
    u8 csmaHang;
    u8 mpirxHang;
    u8 cpuQdHang;
    u8 txdmaHang;
    u8 plcSegmentHang;
    u8 plcAESHang;
    u8 mpitxHang;
    u8 plcSOFHang;
    u8 plcBcn3Hang;
    u8 plcBcn2Hang;
    u8 plcSoundHang;


    
    
    //    FM_Printf(FM_USER,"intr b4= %lx\n",hangIntRegRead);
    
    hangIntRegRead = hal_common_reg_32_read(PLC_SM_HANG_INT);
	reg_value = hal_common_reg_32_read(PLC_SMSHADOW1_REG);
    //FM_Printf(FM_USER,"intr b41= %x\n",hangIntRegRead);
    //printf("\n intr b41= %lu\n",hangIntRegRead);
    csmaHang = (hangIntRegRead & (u32)PLC_CSMA_HANG);
    mpirxHang = (hangIntRegRead & (u32)PLC_MPIRX_HANG);
    cpuQdHang = (hangIntRegRead & (u32)PLC_CPUQD_HANG);
    txdmaHang = (hangIntRegRead & PLC_TX_DMA_HANG);
    plcSegmentHang = (hangIntRegRead & PLC_SEGMENT_HANG);
    plcAESHang = (hangIntRegRead & PLC_AES_HANG);
    mpitxHang = (hangIntRegRead & PLC_MPITX_HANG);
    plcSOFHang = (hangIntRegRead & PLC_SOF_HANG);
    plcBcn3Hang = (hangIntRegRead & PLC_BCN3_HANG);
    plcBcn2Hang = (hangIntRegRead & PLC_BCN2_HANG);
    plcSoundHang = (hangIntRegRead & PLC_SOUND_HANG);
    
    // [YM] Only for CSMA hang - CCo UDP hang case
	if (csmaHang)
	{
		 if ((reg_value & 0x07) == 7)
		 {
	         			
			 //printf("before: PLC_SM_HANG_INT 0xD10 = %lx\n", ReadU32Reg(PLC_SM_HANG_INT));
			 
		     WriteU32Reg(PLC_HYBRII_RESET, ctorl(0x10000));
			 WriteU32Reg(PLC_HYBRII_RESET, 0x0);
			 
			 //printf("after: PLC_SM_HANG_INT 0xD10 = %lx\n", ReadU32Reg(PLC_SM_HANG_INT));
			 set_plc_paramter(PLC_EIFS_SEL, PLC_EIFS_INIT_VALUE);
			 CHAL_DelayTicks(2);
             gHpgpHalCB.halStats.macHangRecover2++;
			 //printf("Reset CSMA State Machine - %bu\n", gHpgpHalCB.halStats.macHangRecover2);
		 }
		 gRecoveryCount1++;
		 //return;
	}
	else if(csmaHang || plcSOFHang || plcBcn2Hang || plcBcn3Hang || plcSoundHang || txdmaHang  || plcSegmentHang  || mpitxHang || plcAESHang)
    {
         hangIntRegRead = hal_common_reg_32_read(PLC_SM_HANG_INT);
         //printf("\n intr b41= %lu\n",hangIntRegRead);
         //printf("Non CSMA H\n");
         plc_reset_tx();
		 CHAL_DelayTicks(2);
		 gRecoveryCount2++;
		 gL2++;
    }
    //hangIntRegRead = hal_common_reg_32_read(PLC_SM_HANG_INT);
    if((mpirxHang > 0) || (cpuQdHang > 0))
    {
    	plc_reset_mpirx_cpuqd();
		CHAL_DelayTicks(2);
		gRecoveryCount2++;
		gL2++;
    } 
    /* if(plcAESHang)
    {
    		plc_reset_aes();
    		FM_Printf(FM_USER,"AES H\n");
    } */
    
    /*if(csmaHang || plcAESHang)
    {
        
        hangIntRegRead = hal_common_reg_32_read(PLC_SM_HANG_INT);
        printf("\n intr before= %lu\n",hangIntRegRead);
        plc_reset_rx();
         hangIntRegRead = hal_common_reg_32_read(PLC_SM_HANG_INT);
        printf("\n intr after2= %lu\n",hangIntRegRead);
    } */
    
    
    /*	hangIntRegRead = hal_common_reg_32_read(PLC_SM_HANG_INT);
    if(txdmaHang > 0)
        {
            plc_reset_tx();//plc_reset_txdma();
            FM_Printf(FM_USER,"TX_H\n");
        }
                               || (hangIntRegRead & PLC_BCN3_HANG) 
                               || (hangIntRegRead & PLC_SOUND_HANG))              // if other flag is set then reset whole PLC
    	{
    		plc_reset_tx();//plc_reset_warm();
    		FM_Printf(FM_USER,"PLC_warm\n");
    	}  */
    /* WriteU32Reg(PLC_CSMAREGION0_REG, ctorl(0x80CA0000));
    WriteU32Reg(PLC_CSMAREGION1_REG, ctorl(0x8D5A0000));
    WriteU32Reg(PLC_CSMAREGION2_REG, ctorl(0x8FFF0000));
    WriteU32Reg(PLC_CSMAREGION3_REG, ctorl(0x8FFF0000));
    WriteU32Reg(PLC_CSMAREGION4_REG, ctorl(0x8FFF0000));
    WriteU32Reg(PLC_CSMAREGION5_REG, ctorl(0x8FFF0000));*/
    //printf("\n intr after2= %lu\n",hangIntRegRead);
    
    

}
#endif

#ifdef HPGP_HAL_TEST
#ifdef _TIMER_INTERRUPT_
void timer_handler (void) using 2// interrupt 1
{
//    uInterruptReg intStatusRd;
//    uInterruptReg intStatusWr;

    // Reload Timer register
 //   TH0 = HYBRII_MSTIMER25MHZ_HI;
 //   TL0 = HYBRII_MSTIMER25MHZ_LO;

	  
    
    // Increment timer count
    CHAL_IncTimerIntCnt();
    gtimer2++;
    gtimer1++;
    // Timer Interrupts
  //  if((gHpgpHalCB.syncComplete) &&(gtimer2 > gsyncTimeout))
//    HHAL_BcnRxTimeoutIntHandler();
}
#endif

#if CPU_TXQ_POLL

eStatus CHAL_PollAndRcvCPUTxQ()
{
    u8 frmsCnt;
    u8 descCnt;
    eStatus status;

    status = STATUS_FAILURE;  
    frmsCnt = CHAL_GetCPUTxQFrmCount();

    if(frmsCnt)
    {
        status = STATUS_SUCCESS;
        CHAL_CpuTxQNemptyIntHandler();
    }
    return status;    
}

#endif

u8 CHT_Poll() 
{ 
#if INT_POLL
#ifdef RTX51_TINY_OS
    os_switch_task();
#else
    ISM_PollInt();
#endif
#elif CPU_TXQ_POLL
    CHAL_PollAndRcvCPUTxQ();
#endif
    return poll_key();
}

#else /* HPGP_HAL_TEST */

extern u8 HHAL_IsSnidMatched (sHaLayer *hal, uRxCpDesc *rxCpDesc);

#if 0
eStatus CHAL_ProcRxFrameDesc(sHaLayer *hal, sSwFrmDesc *rxFrmSwDesc)
{

    uRxFrmHwDesc     rxFrmHwDesc;
    uRxCpDesc        rxCpDesc;
    u16              frmLen;
    u8               firstDesc = 0;
    u8               lastDescLen;
    u8               descCnt;
    eStatus          ret = STATUS_SUCCESS;
#ifdef P8051
    u16  th0;
    u16  tl0;
    u16  timer;
#endif

    /* Always expect a firstdescriptor here 
     * else error case
     */

    /* 1. Read first descriptor and check SrcPort */
    rxFrmHwDesc.reg = ReadU32Reg(CPU_TXQDESC_REG);

#ifdef P8051
    th0 = TH0;
    tl0 = TL0;
    timer = ((th0 << 8)& 0xFF00) | tl0 ;
    //FM_Printf(FM_LINFO,"CHAL_CpuTxQIntHandler: time  = %u\n", timer);
#endif
    FM_Printf(FM_LINFO,"CHAL_CpuTxQIntHandler: HdrDesc  = 0x%08lX\n", rtocl(rxFrmHwDesc.reg));
    if(!rxFrmHwDesc.gnl.bLastDesc)
    {
        /* the Rx frame is ready */
        /* decode the common fields for all frame descriptor formats */
        rxFrmSwDesc->rxPort = rxFrmHwDesc.gnl.srcPort;
        frmLen = rxFrmHwDesc.gnl.frmLenHi;
        frmLen = (frmLen << PKTQDESC1_FRMLENHI_POS) | rxFrmHwDesc.gnl.frmLenLo;
        rxFrmSwDesc->frmLen = frmLen;
        rxFrmSwDesc->frmType = rxFrmHwDesc.gnl.frmType;

        /* decode the fields based on frame descriptor format */
        switch(rxFrmSwDesc->rxPort)
        {
            case PORT_PLC:
            {
                /* HPGP */
                HHAL_ProcRxFrameDesc(hal, &rxFrmHwDesc, rxFrmSwDesc);
            }
            default:
            {
                /* do nothing at present */
            }
        }

        /* number of descriptors for the frame */
        descCnt=CHAL_GetCPUTxQDescCount();
        rxFrmSwDesc->cpCnt = 0;
		
        /* regardless of any error, always read all CPs for the frame */
        /* read the first CP descriptor */
        rxCpDesc.reg  = ReadU32Reg(CPU_TXQDESC_REG);
        firstDesc = rxCpDesc.gnl.firstDesc;

        FM_Printf(FM_LINFO,"CHAL_CpuTxQIntHandler: CPDesc[0]  = 0x%08lX\n", 
                  rtocl(rxCpDesc.reg));

        /* read CPs in the middle */
        while((!rxCpDesc.gnl.lastDesc) && 
              (rxFrmSwDesc->cpCnt < HYBRII_CPPERFRMCOUNT_MAX))
        {
            if (rxFrmSwDesc->rxPort == PORT_PLC)
            {
                /* check snid */
                if (HHAL_IsSnidMatched(hal, &rxCpDesc) == STATUS_SUCCESS) 
                {
                    ret = STATUS_FAILURE;
                    /* it is not for my network */
                    FM_Printf(FM_ERROR,"CHAL: SNID is not matched.\n");
                }
            } 

            /* store Cell Pointer & increment count */
            rxFrmSwDesc->cpDesc[rxFrmSwDesc->cpCnt++].cp = rxCpDesc.gnl.cp;

            /* read next descriptor */
            rxCpDesc.reg  = ReadU32Reg(CPU_TXQDESC_REG);

            FM_Printf(FM_LINFO,"CHAL_CpuTxQIntHandler: CPDesc[%b]  = 0x%08lX\n",
                      rxFrmSwDesc->cpCnt-1, rtocl(rxCpDesc.reg));
        }

        /* store the last CP */
        if (rxCpDesc.gnl.lastDesc && 
            (rxFrmSwDesc->cpCnt < HYBRII_CPPERFRMCOUNT_MAX))
        {          
            if (HHAL_IsSnidMatched(hal, &rxCpDesc) == STATUS_SUCCESS) 
            {
                ret = STATUS_FAILURE;
                /* it is not for my network */
                FM_Printf(FM_ERROR,"CHAL: SNID is not matched.\n");
            }
            rxFrmSwDesc->cpDesc[rxFrmSwDesc->cpCnt++].cp = rxCpDesc.gnl.cp;
        } 
        else
        {
            FM_Printf(FM_ERROR,"CHAL: CP buffer size is too small.\n");
            ret = STATUS_FAILURE;
        }
        /* save the last desc len */
        lastDescLen = rxCpDesc.gnl.descLenHi;
        lastDescLen = (lastDescLen << PKTQDESCCP_DESCLENHI_POS)| 
                       rxCpDesc.gnl.descLenLo;
        rxFrmSwDesc->lastDescLen = lastDescLen;

        /* check error cases */
        if ((rxFrmSwDesc->cpCnt != descCnt) ||  /* case 1 */
            !firstDesc ||                       /* case 2 */
            !rxFrmHwDesc.gnl.bFirstDesc ||      /* case 3 */
            (ret == STATUS_FAILURE))            /* case 4 */
        {
            /* case 1: Validate the CP number
             *     the cp number given in the frame descriptor is not 
             *     matched with the number of CPs recevied from the CP queue  
             * case 2: Validate the first CP
             *     the first CP descriptor in the CP queue is not
             *     the first CP descriptor of a frame
             * case 3: Validate the first/last CP flag
             *     the first descriptor flag is not set for the frame,
             *     but the last descriptor flag is set for the frame.
             * case 4: Validate the SNID
             *     SNID for the Rx frame is not matched with my network 
			 *     or CP buffer is small
             */
            /* thus release all CPs */
            CHAL_FreeFrameCp(rxFrmSwDesc->cpDesc, rxFrmSwDesc->cpCnt);
            FM_Printf(FM_ERROR,"CHAL: the Rx frame is not completed.\n");
            ret = STATUS_FAILURE;
        }

    }
    else 
    {
        /* the last descriptor flag is not set for the frame.
         * thus, the Rx frame is not ready yet and
         * wait until the next interrupt 
         */
        ret = STATUS_FAILURE;
    } 
    return ret;
}

eStatus CHAL_GetCpforTxFrame(sHaLayer *hal, sTxFrmSwDesc *txFrmSwDesc)
{
    u8 numCps = 0;
    
    /* determine the number of cps required based on the frame length */
    numCps = txFrmSwDesc->frmLen / HYBRII_CELLBUF_SIZE;
    numCps += ((txFrmSwDesc->frmLen == numCps*HYBRII_CELLBUF_SIZE) ? 0: 1);
    
    /* get the cell point resource for the tx frame */
    if ((numCps <= HYBRII_CPPERFRMCOUNT_MAX) &&
        (CHAL_AllocFrameCp(txFrmSwDesc->cpArr, numCps) == STATUS_SUCCESS))
    {
        txFrmSwDesc->cpCount = numCps; 
        return STATUS_SUCCESS;
    }
 
    return STATUS_FAILURE;
}
     
eStatus CHAL_WriteFrame(sHaLayer *hal, 
                        sTxFrmSwDesc *txFrmSwDesc, 
                        sBuffDesc *buffdesc)
{
    s16 resLen = buffdesc->datalen;
    u8  numCps = txFrmSwDesc->cpCount;
    u16 cellLen = 0;
    u8  i = 0;
    u8  *dataptr = buffdesc->dataptr;
    volatile u8 XDATA *cellBlk = NULL;
    sCpSwDesc *cpDesc = NULL;


    while ((resLen > 0) && (i < numCps))
    {
        cpDesc = &txFrmSwDesc->cpArr[i];
        cellBlk = CHAL_GetAccessToCP(cpDesc->cp);
        cellLen = MIN(HYBRII_CELLBUF_SIZE, resLen);
        memcpy (cellBlk, dataptr, cellLen);
        cpDesc->offsetU32 = 0; 
        cpDesc->len = (u8)cellLen; 
        dataptr += cellLen;
        resLen -= cellLen;
        i++;
    }

    if ((resLen > 0) && (i >= numCps))
    {
        FM_Printf(FM_ERROR, "CHAL: frame len and cp number mismatch.\n");
    }
    return STATUS_SUCCESS;
}
#endif // 0
#endif /* HPGP_HAL_TEST */

eStatus CHAL_freeCP(u8 cp)
{
    uCpuCPReg cpuCPReg;
	u8 i;
    
#ifdef UART_HOST_INTF
	u8 intFlag  = EA;
	EA = 0;
#endif
    cpuCPReg.reg = 0;
    cpuCPReg.s.cp = cp;

   	WriteU32Reg(CPU_WRITECP_REG, cpuCPReg.reg); 

	for (i = 0; i < ARBITOR_REQ_MAX_TIMES; i++)
	{
//        CHAL_DelayTicks(10);	// give HW a chance to grant the freeing of a CP
    	cpuCPReg.reg = ReadU32Reg(CPU_WRITECP_REG);

    	if(cpuCPReg.s.cpValid)
    	{
#ifdef ETH_BRDG_DEBUG
    		if (myDebugFlag1)
				printf("CHAL_freeCP: cp %bu succeeded\n", cp);
#endif
#ifdef UART_HOST_INTF
    		EA = intFlag;
#endif
    		return(STATUS_SUCCESS);
    	}
	}

#ifdef ETH_BRDG_DEBUG
    if (myDebugFlag1)
		printf("CHAL_freeCP: cp %bu failed\n", cp);
#endif
#ifdef UART_HOST_INTF
    EA = intFlag;
#endif
    return(STATUS_FAILURE);
}

/*******************************************************************
* NAME :            CHAL_GetCellPointer
*
* DESCRIPTION :     Returns a free Cell Pointer, that caller module 
*                   can use to fill data/command frame contents in.
*
* INPUTS :
*       PARAMETERS:
*           None
*
* OUTPUTS :
*       PARAMETERS:
*           u8* pCp        Pointer to which CP value 
*                         is to be copied.
*
*       RETURN :
*            Type:   eStatus
*            Values: STATUS_SUCCESS, if Free CP obtained.
*                    STATUS_FAILURE, otherwise.
*/
eStatus CHAL_RequestCP(u8* pCp)
{
    uCpuCPReg cpuCPReg;
	u8 i;
	
#ifdef UART_HOST_INTF
	u8 intFlag;
	intFlag = EA;
	EA = 0;
#endif
	// return if there's no free CP
	if (CHAL_GetFreeCPCnt() == 0)
	{
#ifdef UART_HOST_INTF
		EA = intFlag;
#endif
   	    return(STATUS_FAILURE);
	}

	// Allocate a CP. If no more free CP, HW returns 0xFF
	// If HW grants access to CP module, it will return a CP index
	// and set the cpValid bit to 1, if it does not grant access
	// to the CP module, the cpValid bit will be set to 0, in
	// which case we'll need to poll for this bit until it's
	// set to 1 or we time out.
	for (i = 0; i < ARBITOR_REQ_MAX_TIMES; i++)
	{
    	cpuCPReg.reg = ReadU32Reg(CPU_REQUESTCP_REG);

    	if(cpuCPReg.s.cp == 0xFF)
    	{
			// no more free CP
			// set bit 31 to relinquish the grant request
			// and then return error
#ifdef ETH_BRDG_DEBUG
    		if (myDebugFlag1)
				printf("CHAL_ReqCP: no more free CP. Return err\n");
#endif
			break;
    	}

		if(cpuCPReg.s.cpValid)
    	{
			// alloc CP is valid
        	*pCp = (u8) cpuCPReg.s.cp;
#ifdef ETH_BRDG_DEBUG
    		if (myDebugFlag1)
				printf("CHAL_ReqCP: alloc cp %bu success\n", cpuCPReg.s.cp);
#endif
#ifdef UART_HOST_INTF
			EA = intFlag;
#endif
    		return(STATUS_SUCCESS);
    	}

        CHAL_DelayTicks(10);	// give HW a chance to grant a CP
	}

#ifdef ETH_BRDG_DEBUG
    if (myDebugFlag1)
		printf("CHAL_RequestCP: alloc cp fail\n");
#endif

	// Cannot access to CP module. Write 1 to bit 31 to tell HW we're 
	// relinquishing the CP alloc request
   	cpuCPReg.reg = 0;
   	cpuCPReg.s.cpValid = 1;
   	WriteU32Reg(CPU_REQUESTCP_REG, cpuCPReg.reg);
	 
	gHalCB.cp_no_grant_alloc_cp++;
#ifdef UART_HOST_INTF
	EA = intFlag;
#endif
    return(STATUS_FAILURE);
}


/*******************************************************************
* NAME :            CHAL_DecrementReleaseCPCnt
*
* DESCRIPTION :     Decrements the CP Usage count for a given CP, 
*                   following which HW will release CP if count has reached Zero.
*
* INPUTS :
*       PARAMETERS:
*           u8 cp    CP to be released/relinquished.
*
* OUTPUTS :
*       PARAMETERS:
*           None
*
*       RETURN :
*           None
*/
void  CHAL_DecrementReleaseCPCnt(u8 cp) //reentrant
{
	uCpuCPReg cpuCPReg;
	u8 i;

#ifdef UART_HOST_INTF
	u8 intFlag;
	intFlag = EA;
	EA = 0;
#endif
	cpuCPReg.reg = 0;
	cpuCPReg.s.cp = cp;
	
	WriteU32Reg(CPU_WRITECP_REG, cpuCPReg.reg); 

	for (i = 0; i < ARBITOR_REQ_MAX_TIMES; i++)
	{
//        CHAL_DelayTicks(10);	// give HW a chance to grant the freeing of a CP
    	cpuCPReg.reg = ReadU32Reg(CPU_WRITECP_REG);

    	if(cpuCPReg.s.cpValid)
    	{
#ifdef ETH_BRDG_DEBUG
    		if (myDebugFlag1)
				printf("CHAL_freeCP: cp %bu succeeded\n", cp);
#endif
#ifdef UART_HOST_INTF
    		EA = intFlag;
#endif
    		return;
    	}
	}
#ifdef ETH_BRDG_DEBUG
	if (myDebugFlag1)
		printf("CHAL_freeCP: cp %bu failed\n", cp);
#endif
#ifdef UART_HOST_INTF
	EA = intFlag;
#endif
	gHalCB.cp_no_grant_free_cp++; 
}


/**
 * NAME :            CHAL_FreeFrameCp
 *
 * DESCRIPTION :     
 *       Decrements the CP Usage count for an array of CPs, 
 *       following which HW will release CP if count has reached Zero.
 *
 * INPUTS :
 *       PARAMETERS:
 *           u8 *cp    array of CPs to be released/relinquished.
 *           u8 numCp  size of array
 *
 * OUTPUTS :
 *       PARAMETERS:
 *           None
 *
 *       RETURN 
 *           None
 */

void CHAL_FreeFrameCp(sCpSwDesc *cpDesc, u8 numCp) __REENTRANT__
{
	u8 i;
	u8 cp_idx;
	uCpuCPReg cpuCPReg;
	u8 fail = 0;
#ifdef UART_HOST_INTF
	u8 intFlag;
	intFlag = EA;
	EA = 0;
#endif	
    for (cp_idx = 0; cp_idx < numCp; cp_idx++)
    {
		//CHAL_DecrementReleaseCPCnt(cpDesc[i].cp);
		cpuCPReg.reg = 0;
		cpuCPReg.s.cp = cpDesc[cp_idx].cp;
		WriteU32Reg(CPU_WRITECP_REG, cpuCPReg.reg);
		for (i = 0; i < ARBITOR_REQ_MAX_TIMES; i++)
		{
	//        CHAL_DelayTicks(10);	// give HW a chance to grant the freeing of a CP
	    	cpuCPReg.reg = ReadU32Reg(CPU_WRITECP_REG);

	    	if(cpuCPReg.s.cpValid)
	    	{
#ifdef ETH_BRDG_DEBUG
	    		if (myDebugFlag1)
					printf("CHAL_freeCP: cp %bu succeeded\n", cpDesc[i].cp);
#endif
				fail = 0;
	    		break;
	    	}
			else
			{
				fail = 1;
			}
		}
		if(fail == 1)
		{
			gHalCB.cp_no_grant_free_cp++;// Failed to free CP		
		}
    }
#ifdef UART_HOST_INTF
	EA = intFlag;
#endif
}


void hal_common_free_frame (sCommonRxFrmSwDesc *rx_frame_info_p)
{
    u8 i;
	u8 cp_idx;
	uCpuCPReg cpuCPReg;
	u8 fail =0;
#ifdef UART_HOST_INTF
	u8 intFlag;
	intFlag = EA;
	EA = 0;
#endif	

	for (cp_idx = 0; cp_idx < rx_frame_info_p->cpCount; cp_idx++)
    {
		//CHAL_DecrementReleaseCPCnt(cpDesc[i].cp);
		cpuCPReg.reg = 0;
		cpuCPReg.s.cp = rx_frame_info_p->cpArr[cp_idx];
		WriteU32Reg(CPU_WRITECP_REG, cpuCPReg.reg);
		for (i = 0; i < ARBITOR_REQ_MAX_TIMES; i++)
		{
	//        CHAL_DelayTicks(10);	// give HW a chance to grant the freeing of a CP
	    	cpuCPReg.reg = ReadU32Reg(CPU_WRITECP_REG);

	    	if(cpuCPReg.s.cpValid)
	    	{
#ifdef ETH_BRDG_DEBUG
	    		if (myDebugFlag1)
					printf("CHAL_freeCP: cp %bu succeeded\n", rx_frame_info_p->cpArr[cp_idx]);
#endif
				fail = 0;
	    		break;
	    	}
			else
			{
				fail = 1;
			}
		}
		if(fail == 1)
		{
			gHalCB.cp_no_grant_free_cp++;// Failed to free CP		
		}
    }
#ifdef UART_HOST_INTF
	EA = intFlag;
#endif
}



void hal_common_reg_8_bit_set (u16 addr, u8 dat8)
{
    u8 value;

    value = ReadU8Reg(addr);
    value |= dat8;
    WriteU8Reg(addr, value);
}

void hal_common_reg_8_bit_clear (u16 addr, u8 dat8)
{
    u8 value;

    value = ReadU8Reg(addr);

    value &= ~dat8;
    WriteU8Reg(addr, value);
}

tinybool hal_common_reg_8_bit_test (u16 addr, u8 dat8)
{
    u8 value;

    value = ReadU8Reg(addr);
    if (value & dat8) {
        return (true);
    } else {
        return (false);
    }
}

u16 hal_common_reg_16_read (u32 reg_addr)
{
    volatile u16 xdata value_16;

    value_16 = ReadU16Reg(reg_addr);

    return (RTOCS(value_16));
}

void hal_common_reg_16_write (u32 addr, u16 dat16)
{
    u16 xdata dat = dat16;

    dat = CTORS(dat16);
    WriteU16Reg(addr, dat);
}

u32 hal_common_reg_32_read (u32 reg_addr)
{
    volatile u32 xdata value_32;
    reg32 r32;
    reg32 xdata rr32;

    value_32 = ReadU32Reg(reg_addr);
    r32.w = value_32;
    rr32.s.b1 = r32.s.b4;
    rr32.s.b2 = r32.s.b3;
    rr32.s.b3 = r32.s.b2;
    rr32.s.b4 = r32.s.b1;
    return rr32.w;
//    return (RTOCL(value_32));
}

void hal_common_reg_32_write (u32 addr, u32 dat32)
{
   // u32 xdata dat = dat32;
    reg32 r32;
    reg32 xdata rr32;

    //dat = CTORL(dat32);
    r32.w = dat32;
    rr32.s.b1 = r32.s.b4;
    rr32.s.b2 = r32.s.b3;
    rr32.s.b3 = r32.s.b2;
    rr32.s.b4 = r32.s.b1;
    WriteU32Reg(addr, rr32.w);
    //WriteU32Reg(addr, dat);
}

void hal_common_reg_bit_set (u32 addr, u32 dat32)
{
    u32 value;

    value = hal_common_reg_32_read(addr);
    value |= dat32;
    hal_common_reg_32_write(addr, value);
}

void hal_common_reg_bit_clear (u32 addr, u32 dat32)
{
    u32 value;

    value = hal_common_reg_32_read(addr);

    value &= ~dat32;
    hal_common_reg_32_write(addr, value);
}

tinybool hal_common_reg_bit_test (u32 addr, u32 dat32)
{
    u32 value;

    value = hal_common_reg_32_read(addr);
    if (value & dat32) {
        return (true);
    } else {
        return (false);
    }
}

u32 hal_common_bit_field_reg_read (u32 addr, u32 mask, u16 pos)
{
    u32 value;

    value = hal_common_reg_32_read(addr);
    value &= mask;
    value >>= pos;

    return (value);
}

void hal_common_bit_field_reg_write (u32 addr, u32 mask, u16 pos, u32 value)
{
    u32 current_value;

    current_value = hal_common_reg_32_read(addr);
    current_value &= ~mask;
    value <<= pos;
    value &= mask;
    value |= current_value;
    hal_common_reg_32_write(addr, value);
}

u32 hal_common_bit_field_get (u32 value, u32 field_mask, u8 field_pos)
{
    // mask the field
    value &= field_mask;

    // right shift the masked value
    value >>= field_pos;

    return value;
}

void hal_common_bit_field_set (u32 *value_p, u32 field_mask, u8 field_pos,
                               u32 field_val)
{
    // left shift fieldVal var
    field_val <<= field_pos;
    // mask the shifted value
    field_val &= field_mask;
    // clear field in reg val
    *value_p  &= ~field_mask;
    // write field to reg val
    *value_p  |= field_val;
}

void CHAL_DelayTicks(u32 num12Clks) //reentrant
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

uint8_t resetReason()
{
	uint8_t regVal;
	regVal = ReadU8Reg(RESET_STAT_REG);
	if(regVal & RESET_REASON_WDTRST)
	{
		return RESET_REASON_WDTRST;
	}
	else if(regVal & RESET_REASON_SWSYSRST)
	{
		return RESET_REASON_SWSYSRST;
	}
	else if(regVal & RESET_REASON_SWCPURST)
	{
		return RESET_REASON_SWCPURST;
	}
	else
	{
		return regVal;
	}	
}

void swResetGV701x(u8 reset)
{
	WriteU8Reg(RESET_CTRL_REG,reset);
}

void configWDT(u8 enable,u16 reload, u8 prescale)
{
	tmr1ctl_t timerCtrlReg;
	timerCtrlReg.reg = ReadU8Reg(TIMER1_CTRL_REG);
	timerCtrlReg.s.enable = 0;

	WriteU8Reg(TIMER1_CTRL_REG,timerCtrlReg.reg);
	
	timerCtrlReg.reg		 = 0x00;
	
	hal_common_reg_16_write(TIMER1_LOAD_REG,reload);
	
	timerCtrlReg.s.enable	 = enable;
	timerCtrlReg.s.mode   	 = WDT;
	timerCtrlReg.s.prescale  = prescale;
	
	WriteU8Reg(TIMER1_CTRL_REG,timerCtrlReg.reg);
}
#ifdef MEM_PROTECTION
eStatus CP_Read_Arb(u8 cp, u8 offset, u32 *tmpBfr, u8 burst_size)
{
	uCpu_Mem_Arb_Req cpuMemArbReg;
	u8 i; 
	u8 * tmp1Bfr = (u8 *) tmpBfr;

#ifdef ETH_BRDG_DEBUG
	if (myDebugFlag1)
		printf("CP_Read_Arb: cp=%bu, offset=%bu, tmpbfr[0]=%bu, burst_sz=%bu\n", cp, offset, tmp1Bfr[0], burst_size);
#endif
	// request a CP read
	cpuMemArbReg.reg = 0;
	cpuMemArbReg.s.cp_rd_wr_offset = offset;
	cpuMemArbReg.s.cp_rd_wr_cp_hi = (cp & 0x78) >> 3;	// upper 4 bits go to cp_hi
	cpuMemArbReg.s.cp_rd_wr_cp_lo = cp & 0x7;	// lower 3 bits go to cp_lo
	cpuMemArbReg.s.cp_rd_wr_req = 1;	// read request
	cpuMemArbReg.s.cp_burst_size = burst_size;
   	WriteU32Reg(CPU_MEM_ARB_REQ, cpuMemArbReg.reg);

	// Check read grant
	for (i = 0; i < ARBITOR_REQ_MAX_TIMES; i++)
	{
    	cpuMemArbReg.reg = ReadU32Reg(CPU_MEM_ARB_REQ);

    	if(cpuMemArbReg.s.cp_rd_wr_grant)
    	{
#ifdef ETH_BRDG_DEBUG
    		if (myDebugFlag1)
				printf("Read_CP: grant\n");
#endif
			break;
    	}
	}

	if (i == ARBITOR_REQ_MAX_TIMES)
	{
#ifdef ETH_BRDG_DEBUG
   		if (myDebugFlag1)
			printf("Read_CP: no grant, offset=%bu, return\n", offset);
#endif
    	gHalCB.cp_no_grant_read_cp++;
    	return(STATUS_FAILURE);
	}

	// read data
	for (i = 0; i < burst_size; i++)
	{
    	tmpBfr[i] = ReadU32Reg(CPU_MEM_ARB_DATA);
	}

    return(STATUS_SUCCESS);
}
	
eStatus CP_Write_Arb(u8 cp, u8 offset, u32 *tmpBfr, u8 burst_size)
{
	uCpu_Mem_Arb_Req cpuMemArbReg;
	u8 i; 
	u8 * tmp1Bfr = (u8 *) tmpBfr;

#ifdef ETH_BRDG_DEBUG
	if (myDebugFlag1)
		printf("CP_Write_Arb: cp=%bu, offset=%bu, tmpbfr[0]=%bu, burst_sz=%bu\n", cp, offset, tmp1Bfr[0], burst_size);
#endif
	// request a CP write
	cpuMemArbReg.reg = 0;
	cpuMemArbReg.s.cp_rd_wr_offset = offset;
	cpuMemArbReg.s.cp_rd_wr_cp_hi = (cp & 0x78) >> 3;	// upper 4 bits go to cp_hi
	cpuMemArbReg.s.cp_rd_wr_cp_lo = cp & 0x7;	// lower 3 bits go to cp_lo
	cpuMemArbReg.s.cp_rd_wr_req = 0;	// write request
	cpuMemArbReg.s.cp_burst_size = burst_size;
#ifdef ETH_BRDG_DEBUG
	if (myDebugFlag1)
		printf("CP_Write_Arb: cpuMemArbReg.reg=0x%lX, cp_rd_wr_offset=0x%bx,cp_rd_wr_cp_lo=0x%bx, cp_rd_wr_cp_hi=0x%bx\n", 
			cpuMemArbReg.reg, cpuMemArbReg.s.cp_rd_wr_offset, cpuMemArbReg.s.cp_rd_wr_cp_lo, cpuMemArbReg.s.cp_rd_wr_cp_hi);
#endif
   	WriteU32Reg(CPU_MEM_ARB_REQ, cpuMemArbReg.reg);

	// write data
	for (i = 0; i < burst_size; i++)
	{
    	WriteU32Reg(CPU_MEM_ARB_DATA, tmpBfr[i]);
	}

	// Check write grant
	for (i = 0; i < ARBITOR_REQ_MAX_TIMES; i++)
	{
    	cpuMemArbReg.reg = ReadU32Reg(CPU_MEM_ARB_REQ);

    	if(cpuMemArbReg.s.cp_rd_wr_grant)
    	{
#ifdef ETH_BRDG_DEBUG
    		if (myDebugFlag1)
				printf("Write_CP: succeeded, offset=0x%bx\n", offset);
#endif
    		return(STATUS_SUCCESS);
    	}
	}

#ifdef ETH_BRDG_DEBUG
    if (myDebugFlag1)
		printf("Write_CP: not granted, offset=0x%bx\n", offset);
#endif
    gHalCB.cp_no_grant_write_cp++;
    return(STATUS_FAILURE);
}
	
eStatus HHAL_CP_Write_Arb(u8 cp, u8 offset, u8 *dataBfr, u8 bufLen)
{
	u8 i, wr_cnt;
	u8 burst_size = MAX_CP_BURST_SIZE;
	u32 *tmpBfr = (u32 *)dataBfr;

	// skip the error checks for now to speed up the process
#ifdef DO_CHECKING
	if ((bufLen <= 0) || (bufLen <= (offset*BYTES_PER_DDWORD)) ||
		(bufLen > (HYBRII_CELLBUF_SIZE-(offset*BYTES_PER_DDWORD))))
	{
   		if (myDebugFlag1)
			printf("HHAL_CP_Write_Arb: Invalid bufLen: %bu, return\n", bufLen);
    	return(STATUS_FAILURE);
	}

	if ((offset < 0) || (offset >= (HYBRII_CELLBUF_SIZE/BYTES_PER_DDWORD)))
	{
   		if (myDebugFlag1)
			printf("HHAL_CP_Write_Arb: Invalid offset: %bu, return\n", offset);
    	return(STATUS_FAILURE);
	}

	if ((cp < 0) || (cp >= HYBRII_CPCOUNT_MAX))
	{
   		if (myDebugFlag1)
			printf("HHAL_CP_Write_Arb: Invalid cp: %bu, return\n", cp);
    	return(STATUS_FAILURE);
	}
#endif

	// This while loop writes as many burst_size times as possible.
	// And when there's not enough data left for burst_size, it
	// decrements burst_size and continues to write until
	// the last write is less than 4 bytes (1 DWORD = 4 bytes)
	while (bufLen)
	{
		if (burst_size)
		{
			wr_cnt = bufLen / (burst_size*BYTES_PER_DDWORD);// each write = 1-4 DWORD = 4-16 bytes
			if (wr_cnt)
			{
				bufLen -= wr_cnt*burst_size*BYTES_PER_DDWORD;
			}
			else
			{
				burst_size--;
			}
		}
		else
		{
			// this is the last write whose len is 4 bytes (minimum len for a write is 4 bytes)
			wr_cnt = 1;
			bufLen = 0;
			burst_size = 1;
		}

		for (i = 0; i < wr_cnt; i++)
		{
#ifdef ETH_BRDG_DEBUG
   			if (myDebugFlag1)
				printf("before calling CP_Wr_Arb: i=%bu, wr_cnt=%bu, burst_size=%bu\n",i,wr_cnt, burst_size);
#endif 
			if (CP_Write_Arb(cp, offset, tmpBfr, burst_size) == STATUS_FAILURE)
			{
				return(STATUS_FAILURE);
			}
			offset += burst_size;
			tmpBfr += burst_size;
		}
	}

	return(STATUS_SUCCESS);
}
	
eStatus HHAL_CP_Read_Arb(u8 cp, u8 offset, u8 *dataBfr, u8 bufLen)
{
	u8 i, rd_cnt;
	u8 burst_size = MAX_CP_BURST_SIZE;
	u32 *tmpBfr = (u32 *)dataBfr;

	// skip the error checks for now to speed up the process
#ifdef DO_CHECKING
	if ((bufLen <= 0) || (bufLen <= (offset*BYTES_PER_DDWORD)) ||
		(bufLen > (HYBRII_CELLBUF_SIZE-(offset*BYTES_PER_DDWORD))))
	{
   		if (myDebugFlag1)
			printf("HHAL_CP_Read_Arb: Invalid bufLen: %bu, return\n", bufLen);
    	return(STATUS_FAILURE);
	}

	if ((offset < 0) || (offset >= (HYBRII_CELLBUF_SIZE/BYTES_PER_DDWORD)))
	{
   		if (myDebugFlag1)
			printf("HHAL_CP_Read_Arb: Invalid offset: %bu, return\n", offset);
    	return(STATUS_FAILURE);
	}

	if ((cp < 0) || (cp >= HYBRII_CPCOUNT_MAX))
	{
   		if (myDebugFlag1)
			printf("HHAL_CP_Read_Arb: Invalid cp: %bu, return\n", cp);
    	return(STATUS_FAILURE);
	}
#endif

	// This while loop reads as many burst_size times as possible.
	// And when there's not enough data left for burst_size, it
	// decrements burst_size and continues to read until
	// the last read is less than 4 bytes (1 DWORD = 4 bytes)
	while (bufLen)
	{
		if (burst_size)
		{
			rd_cnt = bufLen / (burst_size*BYTES_PER_DDWORD);// each write = 1-4 DWORD = 4-16 bytes
			if (rd_cnt)
			{
				bufLen -= rd_cnt*burst_size*BYTES_PER_DDWORD;
			}
			else
			{
				burst_size--;
			}
		}
		else
		{
			// this is the last read whose len is 4 bytes (minimum len for a read is 4 bytes)
			rd_cnt = 1;
			bufLen = 0;
			burst_size = 1;
		}

		for (i = 0; i < rd_cnt; i++)
		{
			if (CP_Read_Arb(cp, offset, tmpBfr, burst_size) == STATUS_FAILURE)
			{
				return(STATUS_FAILURE);
			}
			offset += burst_size;
			tmpBfr += burst_size;
		}
	}

	return(STATUS_SUCCESS);
}

eStatus HHAL_CP_Put_Copy(u8 cp, u8 *putBuf, u8 bufLen)
{
	return(HHAL_CP_Write_Arb(cp, 0, putBuf, bufLen));
}

eStatus HHAL_CP_Get_Copy(u8 cp, u8 *getBuf, u8 bufLen)
{
	return(HHAL_CP_Read_Arb(cp, 0, getBuf, bufLen));
}
#endif // MEM_PROTECTION

eStatus HHAL_Req_Gnt_Read_CPU_QD()
{
    u16 i;

    uPlc_CpuQD_Wr_Arb_Req Wr_Arb_Req;

    // Set CPUQD_Write_Req
    Wr_Arb_Req.reg = 0;
    Wr_Arb_Req.s.cpuQDArbReq = 1;
    WriteU32Reg(PLC_CPUQDWRITEARB_REG,Wr_Arb_Req.reg);
    CHAL_DelayTicks(50);

    //Check if we get a grant
    for (i = 0; i < ARBITOR_REQ_MAX_TIMES; i++)
    {
        Wr_Arb_Req.reg = ReadU32Reg(PLC_CPUQDWRITEARB_REG); 
        if (Wr_Arb_Req.s.cpuQDArbGntStat)
            return(STATUS_SUCCESS);
        CHAL_DelayTicks(10);
    }
    gHalCB.qc_no_grant++;
    return(STATUS_FAILURE);
}

void HHAL_Rel_Gnt_Read_CPU_QD()
{
    uPlc_CpuQD_Wr_Arb_Req Wr_Arb_Req;

    // Clear CPUQD_Write_Req
    Wr_Arb_Req.reg = 0;
    WriteU32Reg(PLC_CPUQDWRITEARB_REG,Wr_Arb_Req.reg);
}

#ifdef UM

eStatus CHAL_GetCpforTxFrame(sSwFrmDesc *txFrmSwDesc)
{
    u8 numCps = 0;
    
    /* determine the number of cps required based on the frame length */
    numCps = txFrmSwDesc->frmLen / HYBRII_CELLBUF_SIZE;
    numCps += ((txFrmSwDesc->frmLen == numCps*HYBRII_CELLBUF_SIZE) ? 0: 1);
    
    /* get the cell point resource for the tx frame */
    if ((numCps <= HYBRII_CPPERFRMCOUNT_MAX) &&
        (CHAL_AllocFrameCp(txFrmSwDesc->cpArr, numCps) == STATUS_SUCCESS))
    {
        txFrmSwDesc->cpCount = numCps; 
        return STATUS_SUCCESS;
    }
 
    return STATUS_FAILURE;
}

#endif

#ifdef B_ASICPLC
#ifdef UM

///////////////////////////////////////////////// SPI Param Config Code ///////////////////////////////////////////////////

#ifdef LOG_FLASH

void spiflash_eraseLogMem()
{
	u16 i;
	printf("\n --> Delete Log memory...\n");
	for(i=273;i<512;i++)
	{
		spiflash_eraseSector(i);
		spiflash_wrsr_unlock((u8)0);
		printf(".");
	}
	
	printf("Erase done\n");
}

eStatus LogToFlash( u8 xdata *srcMemAddr, u16 bId, u16 len)
{
	
	u16 count;
	u16 tempOffset;
	EA = 0;
	if(len !=0)
	{
		tempOffset = len;
		
		//eraseBlock(bId);
		spiflash_wrsr_unlock((u8)1);
		
		for(count=0;count<len;count++)
		{
			spiflash_WriteByte((GVTY_LOG_DATA_ADDR + ((bId*GVTY_LOG_DATA_MAX)+count)),srcMemAddr[count]);
		}
		spiflash_wrsr_unlock(0);
		
		EA = 1;
		return STATUS_SUCCESS;
	}
	else
	{
		EA = 1;
		return STATUS_FAILURE;
	}
}

xdata u8 lmemAddr[GVTY_LOG_DATA_MAX];

eStatus dumpLog()
{
	
	static u16 count;
    static u16 bId;
	
	
	if(*blockId == 0)
	{
		return STATUS_FAILURE;
	}

    for(bId = 0; bId < *blockId; bId++)
    {
    	for(count = 0;count < GVTY_CONFIG_DATA_MAX;count++)
	    {	
	    	EA = 0;
		    lmemAddr[count] = spiflash_ReadByte((u32)((GVTY_LOG_DATA_ADDR) + ((bId*GVTY_LOG_DATA_MAX)+count)));
			EA = 1;
			os_switch_task();
	    }
        FM_HexDump(FM_USER,"Flash Mem Log",lmemAddr,GVTY_LOG_DATA_MAX);
    }
	
	return STATUS_SUCCESS;
}

void dumpLogMem()
{
	static u8 mem0,mem1,mem2,mem3;
	static u16 bId;
	static u16 count;
	static u16 tempAddr;
	for(bId = 0;bId < 4384;bId++)// 
	{
		tempAddr = bId*GVTY_LOG_DATA_MAX;
		mem0 = spiflash_ReadByte((u32)((GVTY_LOG_DATA_ADDR) + ((tempAddr))));
		mem1 = spiflash_ReadByte((u32)((GVTY_LOG_DATA_ADDR) + ((tempAddr)+1)));
		mem2 = spiflash_ReadByte((u32)((GVTY_LOG_DATA_ADDR) + ((tempAddr)+2)));
		mem3 = spiflash_ReadByte((u32)((GVTY_LOG_DATA_ADDR) + ((tempAddr)+3)));
		if((mem0 == 0xFF)&&(mem1 == 0xFF)&&(mem2 == 0xFF)&&(mem3 == 0xFF))
		{
			FM_Printf(FM_USER,"No additional Log Found\n");
			break;
		}
		else
		{	
			memset(lmemAddr,0x00,GVTY_LOG_DATA_MAX);
			for(count = 0;count < GVTY_LOG_DATA_MAX;count++)
		    {
		    	EA = 0;
			    lmemAddr[count] = spiflash_ReadByte((u32)((GVTY_LOG_DATA_ADDR) + ((tempAddr)+count)));
				EA = 1;
				os_switch_task();
		    }
			FM_Printf(FM_USER,"Bank ID %u:\n",bId);
	        FM_HexDump(FM_USER,"Flash Mem Log",lmemAddr,GVTY_LOG_DATA_MAX);
		}
	}

}

u16 getLastPageId()
{
	u8 mem0,mem1,mem2,mem3;
	u16 bId;
	u16 tempAddr;
	for(bId = 0;bId < 3500;bId++)// 
	{
		tempAddr = bId*GVTY_LOG_DATA_MAX;
		mem0 = spiflash_ReadByte((u32)((GVTY_LOG_DATA_ADDR) + ((tempAddr))));
		mem1 = spiflash_ReadByte((u32)((GVTY_LOG_DATA_ADDR) + ((tempAddr)+1)));
		mem2 = spiflash_ReadByte((u32)((GVTY_LOG_DATA_ADDR) + ((tempAddr)+2)));
		mem3 = spiflash_ReadByte((u32)((GVTY_LOG_DATA_ADDR) + ((tempAddr)+3)));
		if((mem0 == 0xFF)&&(mem1 == 0xFF)&&(mem2 == 0xFF)&&(mem3 == 0xFF))
		{
			return bId;
		}
		
	}
    return 0xFFFF;

}
#endif
#endif //UM
#endif // B_ASICPLC
