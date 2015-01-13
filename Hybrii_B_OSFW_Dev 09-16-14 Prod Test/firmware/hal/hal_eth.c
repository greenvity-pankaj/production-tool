/*
* $Id: hal_eth.c,v 1.9 2014/08/25 07:37:34 kiran Exp $
*
* $Source: /home/cvsrepo/Hybrii_B_OSFW_Dev/firmware/hal/hal_eth.c,v $
*
* Description : ETH HAL module.
*
* Copyright (c) 2010-2011 Greenvity Communications, Inc.
* All rights reserved.
*
* Purpose :
*     Defines Ethernet PHY/MAC register read/write , fields set/reset, Init, Tx/Rx functions.
*
*
*/

#ifdef RTX51_TINY_OS
#include <rtx51tny.h>
#endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "papdef.h"
#ifdef ROUTE
#include "hpgp_route.h"
#endif

#ifndef HPGP_HAL_TEST
#include "hal.h"
#include "linkl.h"

#include "hpgpapi.h"
#include "mac_intf_common.h"

#endif
#include "hal_reg.h"
#ifdef UM
#include "timer.h"
#include "stm.h"
#endif

#include "hal_common.h"
#include "hal_hpgp.h"
#include "hal_cfg.h"
#include "hal_eth.h"
#include "hal_tst.h"
#include "fm.h"

#if 0
#define printf(x)
#define FM_Printf(x, z)

#endif
extern sHpgpHalCB gHpgpHalCB;
#ifdef ETH_BRDG_DEBUG
extern u8 myDebugFlag;
extern u8 myDebugFlag1;
extern u32 numPlcPendingRet;
extern u32 numForcePlcTxDone; 
extern u32 numEthTxDoneInts;
extern u32 numEthTxCpReleased;
extern u32 numPlcTxCp;
extern u32 plcTxWriteFail;
extern u32 plcTxFrameCnt;

extern u32 ethTxFrameCnt;
extern u32 numEthTxCp;

extern u32 ethRxFrameCnt;
#endif

#ifdef HPGP_HAL_TEST   
extern sHalCB gHalCB;
#endif

sEthHalCB gEthHalCB;
u8 ethTxDone = 1;
//#define MAX_ETH_BUFF 1600
					 
#ifdef DEBUG_DATAPATH
extern u8 pktDbg;
extern u8 sigDbg;
#endif




#ifdef HYBRII_ETH
void EHAL_Clear_ethHWStat()
{
    WriteU8Reg(ETHMAC_STATCLR_REG, 1);
    printf("ETH TX and RX HW Stats are cleared:\n");

}

void EHAL_Print_ethHWStat()
{
    u8 i;

    printf("ETH RX HW Stats:\n");
    for (i = 0; i < 0x15; i++)
    {
        printf("    RegRd:  [0x%02bX] --> %08lu\n", i, rtocl(EHAL_ReadEthStatReg(i)));     
    }
    printf("ETH TX HW Stats:\n");
    for (i = 0x80; i < 0x96; i++)
    {
        printf("    RegRd:  [0x%02bX] --> %08lu\n", i, rtocl(EHAL_ReadEthStatReg(i)));     
    }
}
#endif

void EHAL_EthSendPause()
{
    u8 pause_Flag = 1;

    WriteU8Reg(ETHMAC_PAUSQNT_REG, 0xFF);
    WriteU8Reg(ETHMAC_PAUSQNT_REG+1, 0x10);
    WriteU8Reg(ETHMAC_SENDPAUSE_REG, pause_Flag);
}

#ifdef HYBRII_ETH
void EHAL_DoRelEthTxCP()
{
    uEthTxFreeCPCntReg  ethTxFreeCPCnt;
    uEthTxRelCPQReg     ethTxRelCPQ;
    u8 i;

	// keep releasing free CPs until there's no more in Free CP register
	while (TRUE)
	{
    	ethTxFreeCPCnt.reg = ReadU32Reg(ETHMAC_TXFREECPCNT_REG);
		if (ethTxFreeCPCnt.s.freeCPCnt == 0)
			break;
#ifdef ETH_BRDG_DEBUG
		numEthTxCpReleased += ethTxFreeCPCnt.s.freeCPCnt;
#endif
    	for( i=0 ; i<ethTxFreeCPCnt.s.freeCPCnt ; i++)
    	{
        	ethTxRelCPQ.reg = ReadU32Reg(ETHMAC_TXRELEASECPQ_REG);
        	CHAL_DecrementReleaseCPCnt((u8)ethTxRelCPQ.s.cp);
    	}
	}
}
#endif

//#ifdef HYBRII_ETH

//=============================================================================
// Procedure:   EHAL_PHYReg
//
// Description: This function Read/Write PHY Registers.
//
// Arguments:
//      u8   sddr
//      u8   *pData
//      eRegOp  regOp
//
// Returns:
//      0 - Success , 1 - Failure
//=============================================================================
eStatus EHAL_EthPhyRegOp(u8 phyAddr, u8 addr, u16 *pData, eRegOp regOp)
{
    eStatus status = STATUS_FAILURE;
#ifdef HYBRII_ETH
    uEthMacMdioCmdReg       ethMacMdioCmd;
    uEthMacMdioStatReg      ethMacMdioStat;
//    unsigned long dwTmp;
    int i;

    status = STATUS_SUCCESS;
   // MDIO GO ?

    i=2000;
    while(i--)
    {
       ethMacMdioCmd.s2.MDIOCmd4    = ReadU8Reg(ETHMAC_MDIOCMD4_REG);
       if(ethMacMdioCmd.s1.Go == 0)
       break;
    }
    if(ethMacMdioCmd.s1.Go != 0)
    {
        return STATUS_FAILURE;
     }
    /*Read Ethernet PHY Register*/

     ethMacMdioCmd.reg = 0;
    /* set the Phy reg address in MDIO Cmd register
     * with MDIOWrite bit set to 0 */
     ethMacMdioCmd.s1.PHYReg   = addr;
     ethMacMdioCmd.s1.PHYAddr1 = phyAddr & 0x7;
     ethMacMdioCmd.s1.PHYAddr2 = (phyAddr >> 3)& 0x3;

     if(regOp ==  RD)
     {
          ethMacMdioCmd.s1.MDIOWrite  = 0;
     }else
     {
          ethMacMdioCmd.s1.MDIOWrite    = 1;
          ethMacMdioCmd.s1.WriteData1  = * ((u8*)pData);
          ethMacMdioCmd.s1.WriteData2  = * ((u8*)pData + 1);
     }
     ethMacMdioCmd.s1.Go       = 1;

     WriteU8Reg(ETHMAC_MDIOCMD1_REG, ethMacMdioCmd.s2.MDIOCmd1);
     WriteU8Reg(ETHMAC_MDIOCMD2_REG, ethMacMdioCmd.s2.MDIOCmd2);
     WriteU8Reg(ETHMAC_MDIOCMD3_REG, ethMacMdioCmd.s2.MDIOCmd3);
     WriteU8Reg(ETHMAC_MDIOCMD4_REG, ethMacMdioCmd.s2.MDIOCmd4);

     i = 2000;
     while(i--)
     {
         /*  check the busy bit to see if
          *  HW has completed the register operation*/
          ethMacMdioCmd.s2.MDIOCmd4    = ReadU8Reg(ETHMAC_MDIOCMD4_REG);
          if(ethMacMdioCmd.s1.Go == 0)
          {
              break;
          }
     }
     if(i < 1)
     {
        //FM_Printf(FM_ERROR, "Failed to complete Ethernet PHY Register Read\n");
       // printf("PHYAddr = %bu, RegAddr = %bu\n", (ethMacMdioCmd.s1.PHYAddr2 << 3)| ethMacMdioCmd.s1.PHYAddr1, addr);
        return STATUS_FAILURE;
     }


      if(regOp ==  RD)
      {

      /*  Read back the Status and value */
      ethMacMdioStat.reg = 0;
      ethMacMdioStat.s2.MDIOStat1    = ReadU8Reg(ETHMAC_MDIOSTAT1_REG);
      ethMacMdioStat.s2.MDIOStat2    = ReadU8Reg(ETHMAC_MDIOSTAT2_REG);
      ethMacMdioStat.s2.MDIOStat3    = ReadU8Reg(ETHMAC_MDIOSTAT3_REG);
      ethMacMdioStat.s2.MDIOStat4    = ReadU8Reg(ETHMAC_MDIOSTAT4_REG);

      if(ethMacMdioStat.s1.RdErr)
      {
          //FM_Printf(FM_ERROR, "Error reading Ethernet PHY Register\n");
          return STATUS_FAILURE ;
      }
          * ((u8*)pData)     = ethMacMdioStat.s1.ReadData1 ;
          * ((u8*)pData + 1) = ethMacMdioStat.s1.ReadData2 ;
      }
    // printf("data is: %X",*pData);
#endif
     return status;
}


void EHAL_Init()
{

#ifdef HYBRII_ETH
    uEthPhyStatReg          ethPhyStat;
//    uEthMacModeReg          ethMacMode;
    uEthMacTxCtl1Reg        ethMacTxCtl1;
//    uEthMacTxCtl2Reg        ethMacTxCtl2;
    uEthMacRxCtlReg         ethMacRxCtl;
    uEthRxEndianReg         ethRxEnd;
    sEthMacPrtEmtThresReg   ethMacPrtEmtThres;
    sEthMacPrtFulThresReg   ethMacPrtFulThres;
    sEthMacTxBufSzReg       ethMacTxBufSz;
    u8                      i;
    eStatus status;
    u8 ethMacAddr[MAC_ADDR_LEN] = {0x00,0x11,0x22,0x33,0x44,0x55};
    u8 seedVal = (u8) gHalCB.timerIntCnt;

    //printf("====> EHAL_InitETH\n");

    // Write to Ethernet Unicast Address and Source Address registers
    for(i=0;i< MAC_ADDR_LEN;i++)
    ethMacAddr[i] = gEthHalCB.macAddrDef[i];

    // Write the Threshold registers and Buffer size
    ethMacPrtEmtThres.PartEmpty  = 0x04;//0x80;
    WriteU8Reg(ETHMAC_PRTEMTTH_REG, ethMacPrtEmtThres.PartEmpty);

    ethMacPrtFulThres.PartFull   = 0xC0; //0x80;
    WriteU8Reg(ETHMAC_PRTFULTH_REG, ethMacPrtFulThres.PartFull);

    ethMacTxBufSz.TxBufSz        = 5; // 0x80, 0x40;
    WriteU8Reg(ETHMAC_TXBUFSZ_REG, ethMacTxBufSz.TxBufSz);
    WriteU8Reg( ETHMAC_SEED_REG, seedVal );
    WriteU8Reg( ETHMAC_SLOTTM_REG, 128 );   // 10Mbps/100Mbps: slot time = 128, Gigabit: 512
    WriteU8Reg( ETHMAC_TXCTL2_REG, 0x0a);   // # of  tx retry

    // PC - don't change
 	for(i=0 ; i<32 ; i++) 
    {
       status = EHAL_EthPhyRegOp(i,ETHPHY_STAT,&ethPhyStat.reg,RD);
       if(status == STATUS_SUCCESS)
       {
          FM_Printf(FM_MINFO, "ETH Phy Addr = %bX, regAddr = %bX, data = %04X\n", i, ETHPHY_STAT,ethPhyStat.reg);
          gEthHalCB.phyChipAddr = i;
          break;
       }
    }

    if(status == STATUS_FAILURE)
    {
        FM_Printf(FM_ERROR, "ETH Phy Addr scan failed. Cannot access MDIO Reg.\n");

    }
    if(ethPhyStat.s.ANComp)
    {
        // Enable Ethernet MAC TX and RX  - Any other parameters ?
        ethMacRxCtl.reg         = 0;
        ethMacRxCtl.s.RxEn      = 1;
        ethMacRxCtl.s.SendCRC   = 0;
        WriteU8Reg( ETHMAC_RXCTL_REG, ethMacRxCtl.reg );

        ethMacTxCtl1.reg        = 0;
        ethMacTxCtl1.s.TxEn     = 1;
        ethMacTxCtl1.s.RetryEn  = 1;
        ethMacTxCtl1.s.PadEn    = 1;
        ethMacTxCtl1.s.FCSAppnd = 1;
        WriteU8Reg( ETHMAC_TXCTL1_REG, ethMacTxCtl1.reg );
    }
    else
    {
    //FM_Printf(FM_MINFO, "InitETH : Auto Neg not complete");
    }

    ethRxEnd.reg = 0;
    ethRxEnd.s.rxLittleEnd = 1;
    WriteU32Reg( ETHMAC_RXENDIAN_REG, ethRxEnd.reg );
    EHAL_ResetStat();
    memset(&ConnState, 0, sizeof(sConnState)*MAX_NUM_STATIONS); // for ETH tests
#endif
}
#ifdef HYBRII_ETH
u32 EHAL_ReadEthStatReg(u8 reg_addr)
{

    u32 stat;
    char*   pStat = &stat;
    // Read ETH MAC Status
    WriteU8Reg(ETHMAC_STATIDX_REG,reg_addr);

    pStat[0]     = ReadU8Reg(ETHMAC_STATDAT1_REG);
    pStat[1]     = ReadU8Reg(ETHMAC_STATDAT2_REG);
    pStat[2]     = ReadU8Reg(ETHMAC_STATDAT3_REG);
    pStat[3]     = ReadU8Reg(ETHMAC_STATDAT4_REG);

    return stat;

}
#endif


u8 EHAL_GetEthTxQFreeDescCnt()
{
#ifdef HYBRII_ETH
    uEthTxQDescCntReg ethTxQDescCnt;
    u8 freeDescCnt;

    ethTxQDescCnt.reg = ReadU32Reg(ETHMAC_TXQDESCCNT_REG);
    freeDescCnt = ETH_TXQ_DEPTH - ethTxQDescCnt.s.descCnt;
    //printf("PLCTxQStatusReg = 0x%08lX, PLCTxQ[%bu].freeDescCnt = %bu\n", RTOCL(capTxQStat.reg),plid, freeDescCnt);
    return  freeDescCnt;
#endif
}


#ifdef HYBRII_ETH
eStatus EHAL_EthTxQWrite(sSwFrmDesc * pEthTxFrmSwDesc)
{
	u16 mycnt=0;

    u8 frmDescCnt;
//    u8 ethTxFreeDescCnt;
    eStatus status=STATUS_SUCCESS;

    frmDescCnt = 1 + pEthTxFrmSwDesc->cpCount;
    ethTxDone = 0;
//	FM_Printf(FM_USER,"etx\n");

    // Check if the respective CAP queue has enough space to hold the descritpors.
    if(EHAL_GetEthTxQFreeDescCnt() >= frmDescCnt)
    {
        uEthTxPktQDesc1       pktQDesc1;
        uEthTxPktQCPDesc      pktQDescCP;
        u8                    i;
#ifdef ETH_BRDG_DEBUG
		u16					  frame_len, tmp_frame_len=0; //cur_oversize_frames, tmp_oversize_frames;
#endif

        //1.Create header/first descriptor
        pktQDesc1.reg        = 0;

        //1.1 Write Frame Length
        pktQDesc1.s.frmLenLo =  pEthTxFrmSwDesc->frmLen & PKTQDESC1_FRMLENLO_MASK;
        pktQDesc1.s.frmLenHi = (pEthTxFrmSwDesc->frmLen & PKTQDESC1_FRMLENHI_MASK) >> PKTQDESC1_FRMLENHI_POS;
#ifdef ETH_BRDG_DEBUG
		frame_len = (pktQDesc1.s.frmLenHi << PKTQDESC1_FRMLENHI_POS) + pktQDesc1.s.frmLenLo;
//		cur_oversize_frames = rtocl(EHAL_ReadEthStatReg(0x88));
		if (frame_len > 1518)
			printf("EHAL_EthTxQWrite: ERROR - frame len exceeds maximum len: frame_len=%d\n", frame_len);
			
		if (myDebugFlag1)
		{
			printf("EHAL_EthTxQWrite: frame_len=%d, pEthTxFrmSwDesc->frmLen\n", frame_len, pEthTxFrmSwDesc->frmLen);
		}
#endif

        // 1.6 Set First Descriptor Flag
        pktQDesc1.s.bFirstDesc   = 1;

        // Set Little Endian flag
        pktQDesc1.s.litEndian = 1;
		
#ifdef DEBUG_DATAPATH

        FM_Printf(FM_HINFO,"\nEthTxQfreeDescCnt = %bu , FreeCPCnt = %bu\n",
                    EHAL_GetEthTxQFreeDescCnt(),
                    CHAL_GetFreeCPCnt());
        FM_Printf(FM_HINFO,"EHAL_EthTxQWrite: HdrDesc    = 0x%08lX, frmLen = %u\n", 
                RTOCL(pktQDesc1.reg), pEthTxFrmSwDesc->frmLen);
#endif

// 1.7 Write first descriptor to the queue
		WriteU32Reg(ETHMAC_QUEUEDATA_REG, pktQDesc1.reg);

#ifdef DEBUG_DATAPATH
        
        if (pktDbg)
        {
            FM_Printf(FM_ERROR, "\neth tx\n");
        }

#endif
        // 6. Create CP Descriptors are write one by one
        for( i=0 ; i<pEthTxFrmSwDesc->cpCount ; i++)
        {
            pktQDescCP.reg          = 0;
            pktQDescCP.s.cp         = pEthTxFrmSwDesc->cpArr[i].cp;
            pktQDescCP.s.descLenLo  = pEthTxFrmSwDesc->cpArr[i].len & PKTQDESCCP_DESCLENLO_MASK;
            pktQDescCP.s.descLenHi  = (pEthTxFrmSwDesc->cpArr[i].len & 
                        PKTQDESCCP_DESCLENHI_MASK)>> PKTQDESCCP_DESCLENHI_POS;
#ifdef ETH_BRDG_DEBUG
			tmp_frame_len += (pktQDescCP.s.descLenHi << PKTQDESCCP_DESCLENHI_POS) + pktQDescCP.s.descLenLo;
#endif
            if( i == pEthTxFrmSwDesc->cpCount-1 )
            {
                pktQDescCP.s.lastDesc = 1;
            }
            pktQDescCP.s.offset     = pEthTxFrmSwDesc->cpArr[i].offsetU32;

            // Write CP descriptor to the queue
            // SwapWriteU32Reg(PLC_CAP_REG, pPlcTxFrmSwDesc->capValue);
            WriteU32Reg(ETHMAC_QUEUEDATA_REG, pktQDescCP.reg);

//            FM_Printf(FM_LINFO,"EHAL_EthTxQWrite: CPDesc%Bu    = 0x%08lX, Offset = %bu, descLen = %bu,cp = %bu\n",
//                                i+1,RTOCL(pktQDescCP.reg), pktQDescCP.s.offset, pEthTxFrmSwDesc->cpArr[i].len, pktQDescCP.s.cp );
#if defined(ETH_DEBUG_PACKET) || defined(DEBUG_DATAPATH)
						
#ifdef DEBUG_DATAPATH

			 if (pktDbg)
#endif			 	
			 	
            {
                u8 j;
                u8 byteOffset = (u8)pEthTxFrmSwDesc->cpArr[i].offsetU32 << 2;

                volatile u8 xdata * cellAddr = CHAL_GetAccessToCP(pEthTxFrmSwDesc->cpArr[i].cp);
                FM_Printf(FM_USER,"PktBuf%bu, addr %lu :\n", i+1, (cellAddr+byteOffset));
                FM_Printf(FM_USER, "eth offset 0x%02x \n", byteOffset);
                for( j = byteOffset ;  j < (byteOffset + pEthTxFrmSwDesc->cpArr[i].len ); j++)
                {
                    FM_Printf(FM_USER,"0x%02bX ", cellAddr[j]);
                }
                FM_Printf(FM_USER,"\n\n");
            }
#endif             
        }    
#ifdef ETH_BRDG_DEBUG
		if (frame_len != tmp_frame_len)
		{
			printf("EHAL_EthTxQWrite: ERROR - frame lens don't match: frame_len=%d, tmp_frame_len=%d\n", frame_len, tmp_frame_len);
		}
		/*
		tmp_oversize_frames = rtocl(EHAL_ReadEthStatReg(0x88));
		if (cur_oversize_frames != tmp_oversize_frames)
		{
			printf("EHAL_EthTxQWrite: frame_len %d produces an oversize frame: %d\n", tmp_oversize_frames);
		}
*/
#endif
        // update statistics
        gEthHalCB.TotalTxFrmCnt++;
        gEthHalCB.TotalTxBytesCnt+=pEthTxFrmSwDesc->frmLen;

#ifdef ETH_BRDG_DEBUG
		ethTxFrameCnt++;
		numEthTxCp += pEthTxFrmSwDesc->cpCount;
#endif
    }
    else
    {

	    ethTxDone = 1;
        status =  STATUS_FAILURE;
			printf("EHAL_EthTxQWrite(): Failed 1: EHAL_GetEthTxQFreeDescCnt()=%bu, frmDescCnt=bu\n", EHAL_GetEthTxQFreeDescCnt(), frmDescCnt);
#ifdef ETH_BRDG_DEBUG
		if (myDebugFlag1)
			printf("EHAL_EthTxQWrite(): Failed 1: EHAL_GetEthTxQFreeDescCnt()=%bu, frmDescCnt=bu\n", EHAL_GetEthTxQFreeDescCnt(), frmDescCnt);
#endif
    }
    return status;

}
#endif

#ifdef HYBRII_ETH

bool EHAL_IsTxReady()
{

	if(ethTxDone != 1)
	{

		return FALSE;
	}

	return TRUE;
}
#endif
void EHAL_ResetStat()
{
//#ifdef HYBRII_ETH
    // Rx Statistics counters
    gEthHalCB.TotalRxFrmCnt     = 0;
    gEthHalCB.TotalRxBytesCnt   = 0;

    // Tx Statistics counters
    gEthHalCB.TotalTxFrmCnt     = 0;
    gEthHalCB.TotalTxBytesCnt   = 0;

    // Test Statistics
    gEthHalCB.CurTxTestFrmCnt   = 0;
    gEthHalCB.CurTxTestBytesCnt = 0;
//#endif
}

void EHAL_DisplayEthStat()
{
//#ifdef HYBRII_ETH
    u16 outStandingDescCnt;

    outStandingDescCnt = ETH_TXQ_DEPTH - EHAL_GetEthTxQFreeDescCnt();

    if(gEthHalCB.TotalRxBytesCnt)
    {
        printf("\n============ HOST Rx Statistics ==============\n");
        printf("TotalRxFrmCnt     = %lu\n",gEthHalCB.TotalRxFrmCnt);
        printf("TotalRxBytesCnt   = %lu\n\n",gEthHalCB.TotalRxBytesCnt);
    }

    if(gEthHalCB.TotalTxFrmCnt)
    {
        printf("\n============ HOST Tx Statistics ==============\n");
        printf("TotalTxFrmCnt     = %lu\n",gEthHalCB.TotalTxFrmCnt);
        printf("TotalTxBytesCnt   = %lu\n\n",gEthHalCB.TotalTxBytesCnt);
    }
    printf("TimerIntCnt       = %lu\n",gHalCB.timerIntCnt);
    printf("FreeCPCnt         = %bu\n\n",CHAL_GetFreeCPCnt());

//#endif
}

// ConnExistSlave is called from a slave station. It returns:
//      - TRUE if there is already an Open connection for this station. 
//        It also returns the index to that station.
//      - FALSE otherwise. In this case, it returns the index to the first 
//        available station
//
#ifdef HYBRII_ETH

u8  ConnExistSlave(u8 *MACaddr, u8 *StationId)
{
    u8  firstFreeConn = 0xff;
    u8  i;

    for (i = 0; i < MAX_NUM_STATIONS; i++)
    {
        if (!memcmp(ConnState[i].myMACaddr, MACaddr, MAC_ADDR_LEN) 
            && (ConnState[i].state == GCI_STATE_OPEN))
        {
            *StationId = i;
            return(TRUE);
        } else
        {
            if (firstFreeConn == 0xff)
                firstFreeConn = i;
        }
    }
    *StationId = firstFreeConn;
    return(FALSE);
}
#endif
// ConnExistMaster is called from a master station. It returns:
//      - TRUE if there is already an Open connection for this station.
//        It also returns the index to that station
//      - FALSE otherwise. In this case, it returns the index to the first available station
//
#ifdef HYBRII_ETH

u8  ConnExistMaster(u8 *masterMACaddr, u8 *slaveMACaddr,u8 *StationId)
{
    u8  firstFreeConn = 0xff;
    u8  i;

    for (i = 0; i < MAX_NUM_STATIONS; i++)
    {
        if (!memcmp(ConnState[i].myMACaddr, masterMACaddr, MAC_ADDR_LEN)
            && !memcmp(ConnState[i].slaveMACaddr, slaveMACaddr, MAC_ADDR_LEN))
        {
            *StationId = i;
            return(TRUE);
        } else
        {
            if (firstFreeConn == 0xff)
                firstFreeConn = i;
        }
    }
    *StationId = firstFreeConn;
    return(FALSE);
}
#endif // HYBRII_ETH


void EHAL_ReleaseEthTxCPIntHandler()
{
#ifdef HYBRII_ETH
    EHAL_DoRelEthTxCP();
    ethTxDone = 1;
#ifdef ETH_BRDG_DEBUG
	numEthTxDoneInts++;
#endif
#endif
}


