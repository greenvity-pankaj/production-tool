/*
* $Id: hal_hpgpTx.c,v 1.30 2014/12/09 07:09:08 ranjan Exp $
*
* $Source: /home/cvsrepo/Hybrii_B_OSFW_Dev/firmware/hal/hal_hpgpTx.c,v $
*
* Description : HPGP HAL Transmit module.
*
* Copyright (c) 2010-2011 Greenvity Communications, Inc.
* All rights reserved.
*
* Purpose :
*     Defines beacon and data trasmit functions for HPGP.
*
*
*/
#include <stdio.h>
#include <string.h>
#include "papdef.h"
#ifdef ROUTE
#include "hpgp_route.h"
#endif
#include "hal_common.h"
#include "fm.h"
#include "hal.h"
#include "hpgpdef.h"
#include "hal_hpgp.h" 
#include "hal_reg.h"
#include "frametask.h" 
#include "datapath.h"
#include "hal_cfg.h"
#include "hal_hpgp_reset.h"
#ifdef PLC_TEST
#include "hal_tst.h" 
#endif
#include "utils_fw.h"
#include "nma.h"
#include "nma_fw.h"
#include "mmsg.h"
#ifdef UM
#include "linkl.h"
#include "hpgpapi.h"
#endif
#include "uart.h"
#include "gv701x_gpiodriver.h"

u8 gsendBcn;

#ifdef DEBUG_DATAPATH
extern u8 pktDbg;
extern u8 sigDbg;
extern u8 ethQueueDebug;
#endif
#ifdef PLC_TEST
u16 gBcnSent;
extern u8 gCCOTest;
extern u8 gSTA_TEI;
u8 gHeaderBytes[3] = {'/', '?', '!'};
#endif

#ifdef UM
#ifdef CCO_FUNC
extern void LINKL_UpdateBeacon();
#endif
#endif
#ifdef HPGP_HAL_TEST
extern void HHT_SendBcn(u8 bcnType);
#endif
#ifdef FREQ_DETECT
extern u32 PLC_MIN_AC_BPLEN;
extern u32 PLC_AC_BP_LEN; 
extern u32 PLC_MAX_AC_BPLEN; 
#endif

extern uCSMARegionReg   csmaRegArr[HYBRII_MAXSMAREGION_CNT]; 
//u16 CSMA_REGIONS_VAL_DC[HYBRII_MAXSMAREGION_CNT] = {0x1E8, 0xF43,0x00 ,0x00, 0x00, 0x00 };  //first region is 5ms and sec is 35 ms(so we have to program as 40), start point is 00
//u16 CSMA_REGIONS_VAL_DC[HYBRII_MAXSMAREGION_CNT] = {0xCA, 0xF43, 0xFFF , 0xFFF, 0xFFF, 0xFFF };  //CSMA reg. 0 is 2.068ms, reg. 1 is 40 ms, 2 and .. are set at 41.9ms with Rx Only
//u16 CSMA_REGIONS_VAL_DC[HYBRII_MAXSMAREGION_CNT] = {0xCA, 0xD5A, 0xFFF , 0xFFF, 0xFFF, 0xFFF };  //CSMA reg. 0 is 2.068ms, reg. 1 is 40 ms, 2 and .. are set at 41.9ms with Rx Only
u16 CSMA_REGIONS_VAL_DC[HYBRII_MAXSMAREGION_CNT] = {0x30, 0xF11, 0xFFF , 0xFFF, 0xFFF, 0xFFF };  //CSMA reg. 0 is 2.068ms, reg. 1 is 40 ms, 2 and .. are set at 41.9ms with Rx Only
u16 CSMA_REGIONS_VAL_DC_STA[HYBRII_MAXSMAREGION_CNT] = {52, 3907,  0xFFF, 0xFFF, 0xFFF, 0xFFF };
//u16 CSMA_REGIONS_VAL_DC[HYBRII_MAXSMAREGION_CNT] = {0xCA, 0xF43, 0xFFF , 0xFFF, 0xFFF, 0xFFF }; // TRI's  //first region is 5ms and sec is 35 ms(so we have to program as 40), start point is 00
//u16 CSMA_REGIONS_VAL_DC[HYBRII_MAXSMAREGION_CNT] = {0x98, 0xF43,0x00 ,0x00, 0x00, 0x00 };  //first region is 5ms and sec is 35 ms(so we have to program as 40), start point is 00

//STA regions
//u16 CSMA_REGIONS_VAL_AC[HYBRII_MAXSMAREGION_CNT] = {0xC3, 0xBF1,0xCB6 ,0x00, 0x00, 0x00 };  //STA regions this is sent in bcn so this is STA regions for AC
u16 CSMA_REGIONS_VAL_AC[HYBRII_MAXSMAREGION_CNT] = {0x61, 0xc65 ,0xFFF,0xFFF, 0xFFF, 0xFFF };  //STA regions this is sent in bcn so this is STA regions for AC
//u16 CSMA_REGIONS_VAL_AC_CCO[HYBRII_MAXSMAREGION_CNT] = {0xCA, 0xaf0,0xd75,0xd75,0xd75,0xd75};
#ifdef ETH_BRDG_DEBUG
extern u8 myDebugFlag;
extern u32 numTxFrms;
#endif

extern u8 opMode;
extern u32 host_intf_max_cp;

#ifndef CALLBACK
extern void LINKL_BcnTxHandler(void* cookie);
#endif
#ifdef HPGP_HAL_TEST
extern void Host_RxHandler(sCommonRxFrmSwDesc* pRxFrmDesc);
#else
extern void Host_RxHandler(sHaLayer *pHal, sCommonRxFrmSwDesc* pRxFrmDesc);
#endif

#ifdef PLC_TEST
void HHT_SimulateTx(sPlcSimTxTestParams *);

void broadcast_CCOTEI()
{
 
    u8 dataBuff[8];
    
    //send broad cast frame to each sta 
    strncpy(&dataBuff[0], &gHeaderBytes[0], sizeof(gHeaderBytes));
    dataBuff[3] = BROADCAST_CCO_TEI_TESTID;
    dataBuff[4] = gNumOfSTAAssignedTEI;
    
    Send_SinglePLCFrame(5, &dataBuff[0], DEFAULT_CCO_TEI, 0xFF);//broadcast CCO TEI

}

void sendRobomodeFrames(u16 frmLen, u32 numFrames)
{
    
    sPlcSimTxTestParams testParams; 
    memset(&testParams, 0, sizeof(testParams));
    testParams.numFrames      = numFrames;
    testParams.contMode = 0;

    testParams.frmLen         = frmLen;
    testParams.lenTestMode    = FIXED_LEN;
    //testParams.roboTestMode   = 0;//roboTestMode
    testParams.frmType        = 1;
    testParams.altFrmTypeTest = 0;
    testParams.altMcstTest    = 0;
    testParams.plid           = 0;
    testParams.altPlidTest    = 0;
    //testParams.stdModeSel     = stdModeSel;
    testParams.mcstMode       = 0;//unicast
    testParams.offsetDW       = 0;
    testParams.descLen        = HYBRII_CELLBUF_SIZE;
    testParams.secTestMode    = UNENCRYPTED;
    testParams.eks            = HPGP_UNENCRYPTED_EKS;
    testParams.altOffsetDescLenTest = 0;
    testParams.delay          = 4;  // I have changed this because Tri changed the delay ticks definition
 
    
    printf("\n >>> Transmitting %lu  Frames of Each of  Length  %d \n",numFrames, frmLen);
    // Trigger the tx test
    HHT_SimulateTx(&testParams);
  
}


void Send_SinglePLCFrame(u16 frmLen, u8 *dataBuff, u8 stei,u8 dtei)//pass dataBuf and frameLen
{
    //broadcast TEI to STAs with num of TEIS assigns to STAs
       
    
        u16 index = 0;
    u16 tmpFrmLen;
    eStatus status;
    u8 i;
    u8 j;
    u8 quit = 0; 
    u8 c;
    u16 tmpPayloadLen;
#ifndef HPGP_HAL_TEST
    sHaLayer *hal = (sHaLayer*)HOMEPLUG_GetHal();
#endif
       
     sTxFrmSwDesc    plcTxFrmSwDesc; 
    
    quit              = 0;
     memset((u8*)&plcTxFrmSwDesc, 0x00, sizeof(plcTxFrmSwDesc));
     plcTxFrmSwDesc.frmInfo.plc.eks            = 0x0F;
    plcTxFrmSwDesc.frmInfo.plc.bcnDetectFlag  = REG_FLAG_SET;// REG_FLAG_CLR;
    plcTxFrmSwDesc.frmInfo.plc.clst = HPGP_CONVLYRSAPTYPE_RSV;//HPGP_CONVLYRSAPTYPE_ETH; 

    
    
    plcTxFrmSwDesc.frmType        = HPGP_HW_FRMTYPE_MSDU;
   plcTxFrmSwDesc.frmInfo.plc.dtei = 0xFF; //dtei;
    plcTxFrmSwDesc.frmInfo.plc.plid = 0;    
    plcTxFrmSwDesc.frmInfo.plc.stei            = stei;//CCO TEI
    plcTxFrmSwDesc.frmInfo.plc.snid = HYBRII_DEFAULT_SNID;
    
    plcTxFrmSwDesc.frmInfo.plc.stdModeSel = 1;//HPGP_ROBOMD_MINI;
    gHpgpHalCB.halStats.CurTxTestFrmCnt = 0; 
     plcTxFrmSwDesc.frmLen         =  frmLen;
    plcTxFrmSwDesc.cpCount        = 0;        
      
    plcTxFrmSwDesc.frmInfo.plc.phyPendBlks    = HPGP_PPB_CAP0;
    plcTxFrmSwDesc.frmInfo.plc.mcstMode       = 1;   //mcstcastmode 

    
    tmpFrmLen   = 0;
    tmpPayloadLen = 0;
    // create cp descriptors
     while(tmpFrmLen < plcTxFrmSwDesc.frmLen)//thisw while loop will write the data to MAC buff address
     {      
        u8        cp;
        u8        tmpOffsetDW;
        u8        tmpOffsetByte;
        u8        tmpDescLen;
        u8        remDescLen;
        u8        actualDescLen;
        
        volatile u8 xdata *       cellAddr;
        
       
        // Fetch CP

        do
        {
                status = CHAL_RequestCP(&cp);

        }while (status != STATUS_SUCCESS);
        // check for user initiated exit task
        if(quit)
        {
                break;
        }
         

      
        i = plcTxFrmSwDesc.cpCount;
        // test offset and desc len - only for first CPs

        tmpOffsetDW = 0;
        tmpDescLen  = HYBRII_CELLBUF_SIZE;
        actualDescLen =  (plcTxFrmSwDesc.frmLen-tmpFrmLen)>tmpDescLen ? tmpDescLen : (plcTxFrmSwDesc.frmLen-tmpFrmLen);
        remDescLen    =  actualDescLen;

        // Fill Buffer with pattern
        cellAddr = CHAL_GetAccessToCP(cp);
//        FM_Printf(FM_LINFO,"cp = %bu, cellAddr=%08lX, seqNum=%bu\n",cp,(u32)cellAddr, gHpgpHalCB.halStats.TxSeqNum);

   
        for( j= 0 ; j< remDescLen ; j++)
        {

                cellAddr[j] = dataBuff[index++];

        }
        plcTxFrmSwDesc.cpArr[i].offsetU32 = tmpOffsetDW;
        plcTxFrmSwDesc.cpArr[i].len       = actualDescLen; 
        tmpFrmLen                        += plcTxFrmSwDesc.cpArr[i].len; 
        plcTxFrmSwDesc.cpArr[i].cp        = cp;

        plcTxFrmSwDesc.cpCount++;
        tmpPayloadLen += (plcTxFrmSwDesc.cpArr[i].len - tmpOffsetByte);


     }    
        // check for user initiated exit task
    if(status == STATUS_SUCCESS)//this will wrire swdesc to hw desc
    {
            do
            {
                // Transmit the frame
                //status = HHAL_IsPlcIdle();
                //if(status == STATUS_FAILURE)
                {
                    //HHAL_PlcPendingTx();
                }
               // else
                {
#ifdef HPGP_HAL_TEST
                    status = HHAL_PlcTxQWrite(&plcTxFrmSwDesc);
#else
                    status = HHAL_PlcTxQWrite(hal, &plcTxFrmSwDesc);
#endif
                }
                // check for user initiated exit task from infinite loop
#ifdef HPGP_HAL_TEST
                c = CHT_Poll();
#else
                c = poll_key();
#endif
                if( c == 'q')               
                {
                        // if TxQWrite failed, release CPs for current frame -- tbd
                        quit = 1;
                        break;
                }
            } while(status == STATUS_FAILURE);
    }
    gHpgpHalCB.halStats.TotalTxFrmCnt--;
    gHpgpHalCB.halStats.TxDataCnt--;

  
    if((gHpgpHalCB.halStats.CurTxTestFrmCnt & (u32)(0xFF)) == 0)
    {  
        printf("\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b");
        printf("Sent %ld, Received %ld HPGP frames.", gHpgpHalCB.halStats.CurTxTestFrmCnt, gHpgpHalCB.halStats.TotalRxGoodFrmCnt - gHpgpHalCB.halStats.RxGoodBcnCnt);
    }

      
    
}
#endif

    
void setCSMA_onSTA()
{

    uCSMARegionReg   *pCSMARgn;
    u16 csma0, csma1, csma2, csma3, csma4, csma5;



pCSMARgn = &csmaRegArr[0];
pCSMARgn->s.csma_start_time_lo = 0; //start_time;
pCSMARgn->s.csma_start_time_hi = 0;
pCSMARgn->s.csma_rxOnly = 0;
pCSMARgn->s.csma_hybrid = 1;
/*  pCSMARgn->s.csma_endtime_lo = CSMA_REGIONS_VAL[0] & 0x00FF;
pCSMARgn->s.csma_endtime_hi = ((CSMA_REGIONS_VAL[0]  & 0xFF00) >> 8);
WriteU32Reg(PLC_CSMAREGION0_REG, pCSMARgn->reg);     //since this address is not continuous we cant use for loop

pCSMARgn->s.csma_start_time_lo = 0; //start_time;
pCSMARgn->s.csma_start_time_hi = 0;
pCSMARgn->s.csma_rxOnly = 0;
pCSMARgn->s.csma_hybrid = 1;  */

if(gHpgpHalCB.lineFreq == FREQUENCY_50HZ)
{
csma0 = CSMA_REGIONS_VAL_DC_STA[0];  
csma1 = CSMA_REGIONS_VAL_DC_STA[1]; 
csma2 = CSMA_REGIONS_VAL_DC_STA[2];

csma3 = CSMA_REGIONS_VAL_DC_STA[3]; 
csma4 = CSMA_REGIONS_VAL_DC_STA[4]; 
csma5 = CSMA_REGIONS_VAL_DC_STA[5]; 			
}

else if(gHpgpHalCB.lineFreq == FREQUENCY_60HZ)
{
csma0 = CSMA_REGIONS_VAL_AC[0];  
csma1 = CSMA_REGIONS_VAL_AC[1];

csma2 = CSMA_REGIONS_VAL_AC[2];

csma3 = CSMA_REGIONS_VAL_AC[3]; 
csma4 = CSMA_REGIONS_VAL_AC[4]; 
csma5 = CSMA_REGIONS_VAL_AC[5]; 		
}

pCSMARgn->s.csma_rxOnly = TRUE;
pCSMARgn->s.csma_endtime_lo =  csma0 & 0x00FF;
pCSMARgn->s.csma_endtime_hi = (( csma0 & 0xFF00) >> 8);
WriteU32Reg(PLC_CSMAREGION0_REG, pCSMARgn->reg);

pCSMARgn->s.csma_rxOnly = FALSE;


pCSMARgn->s.csma_hybrid = 1;

pCSMARgn->s.csma_endtime_lo =  csma1 & 0x00FF;
pCSMARgn->s.csma_endtime_hi = (( csma1 & 0xFF00) >> 8);
WriteU32Reg(PLC_CSMAREGION1_REG, pCSMARgn->reg);

pCSMARgn->s.csma_rxOnly = TRUE;


pCSMARgn->s.csma_hybrid = 1;

// need to take of AC later. TRI
if(gHpgpHalCB.lineMode == LINE_MODE_DC)
{
pCSMARgn->s.csma_endtime_lo =  csma2 & 0x00FF;
pCSMARgn->s.csma_endtime_hi = (( csma2 & 0xFF00) >> 8);
pCSMARgn->s.csma_rxOnly = TRUE;
WriteU32Reg(PLC_CSMAREGION2_REG, pCSMARgn->reg);
#if 1
pCSMARgn->s.csma_endtime_lo =  csma3 & 0x00FF;
pCSMARgn->s.csma_endtime_hi = (( csma3 & 0xFF00) >> 8);
pCSMARgn->s.csma_rxOnly = TRUE;
WriteU32Reg(PLC_CSMAREGION3_REG, pCSMARgn->reg);
    
pCSMARgn->s.csma_endtime_lo =  csma4 & 0x00FF;
pCSMARgn->s.csma_endtime_hi = (( csma4 & 0xFF00) >> 8);
pCSMARgn->s.csma_rxOnly = TRUE;
WriteU32Reg(PLC_CSMAREGION4_REG, pCSMARgn->reg);    

pCSMARgn->s.csma_endtime_lo =  csma5 & 0x00FF;
pCSMARgn->s.csma_endtime_hi = (( csma5 & 0xFF00) >> 8);
pCSMARgn->s.csma_rxOnly = TRUE;
WriteU32Reg(PLC_CSMAREGION5_REG, pCSMARgn->reg);    

#endif
}


}


#if 0
void setCSMA_onCCO1()
{

	uCSMARegionReg	 *pCSMARgn;
	u16 csma0, csma1, csma2, csma3, csma4, csma5;


    
    	pCSMARgn = &csmaRegArr[0];
        pCSMARgn->s.csma_start_time_lo = 0; //start_time;
    	pCSMARgn->s.csma_start_time_hi = 0;
        pCSMARgn->s.csma_rxOnly = 0;
    	pCSMARgn->s.csma_hybrid = 1;
    /*	pCSMARgn->s.csma_endtime_lo = CSMA_REGIONS_VAL[0] & 0x00FF;
    	pCSMARgn->s.csma_endtime_hi = ((CSMA_REGIONS_VAL[0]  & 0xFF00) >> 8);
        WriteU32Reg(PLC_CSMAREGION0_REG, pCSMARgn->reg);     //since this address is not continuous we cant use for loop
       
        pCSMARgn->s.csma_start_time_lo = 0; //start_time;
    	pCSMARgn->s.csma_start_time_hi = 0;
        pCSMARgn->s.csma_rxOnly = 0;
    	pCSMARgn->s.csma_hybrid = 1;  */
        
        if(gHpgpHalCB.lineMode == LINE_MODE_DC)
        {
            csma0 = CSMA_REGIONS_VAL_DC[0];  
            csma1 = CSMA_REGIONS_VAL_DC[1]; 
            csma2 = CSMA_REGIONS_VAL_DC[2]; 
            csma3 = CSMA_REGIONS_VAL_DC[3]; 
            csma4 = CSMA_REGIONS_VAL_DC[4]; 
            csma5 = CSMA_REGIONS_VAL_DC[5]; 
        }
        else if(gHpgpHalCB.lineMode == LINE_MODE_AC)
        {
           csma0 = CSMA_REGIONS_VAL_AC[0];  
           csma1 = CSMA_REGIONS_VAL_AC[1]; 
           csma2 = CSMA_REGIONS_VAL_AC[2]; 
           csma3 = CSMA_REGIONS_VAL_AC[3]; 
           csma4 = CSMA_REGIONS_VAL_AC[4]; 
           csma5 = CSMA_REGIONS_VAL_AC[5]; 
        }
    	pCSMARgn->s.csma_endtime_lo =  csma0 & 0x00FF;
    	pCSMARgn->s.csma_endtime_hi = (( csma0 & 0xFF00) >> 8);
        pCSMARgn->s.csma_rxOnly = 1;
        WriteU32Reg(PLC_CSMAREGION0_REG, pCSMARgn->reg);

        pCSMARgn->s.csma_rxOnly = 0;
        pCSMARgn->s.csma_endtime_lo =  csma1 & 0x00FF;
    	pCSMARgn->s.csma_endtime_hi = (( csma1 & 0xFF00) >> 8);
        WriteU32Reg(PLC_CSMAREGION1_REG, pCSMARgn->reg);

		// need to take of AC later. TRI
        //if(gHpgpHalCB.lineMode == LINE_MODE_DC)
        {
			pCSMARgn->s.csma_endtime_lo =  csma2 & 0x00FF;
    		pCSMARgn->s.csma_endtime_hi = (( csma2 & 0xFF00) >> 8);
        	pCSMARgn->s.csma_rxOnly = TRUE;
			WriteU32Reg(PLC_CSMAREGION2_REG, pCSMARgn->reg);
			    
			pCSMARgn->s.csma_endtime_lo =  csma3 & 0x00FF;
    		pCSMARgn->s.csma_endtime_hi = (( csma3 & 0xFF00) >> 8);
        	pCSMARgn->s.csma_rxOnly = TRUE;
			WriteU32Reg(PLC_CSMAREGION3_REG, pCSMARgn->reg);
			    
			pCSMARgn->s.csma_endtime_lo =  csma4 & 0x00FF;
    		pCSMARgn->s.csma_endtime_hi = (( csma4 & 0xFF00) >> 8);
        	pCSMARgn->s.csma_rxOnly = TRUE;
			WriteU32Reg(PLC_CSMAREGION4_REG, pCSMARgn->reg);    

			pCSMARgn->s.csma_endtime_lo =  csma5 & 0x00FF;
    		pCSMARgn->s.csma_endtime_hi = (( csma5 & 0xFF00) >> 8);
        	pCSMARgn->s.csma_rxOnly = TRUE;
			WriteU32Reg(PLC_CSMAREGION5_REG, pCSMARgn->reg);    
		}


}
#endif
void setCSMA_onCCO()
{

	uCSMARegionReg	 *pCSMARgn;
	u16 csma0, csma1, csma2, csma3, csma4, csma5;


    
    	pCSMARgn = &csmaRegArr[0];
        pCSMARgn->s.csma_start_time_lo = 0; //start_time;
    	pCSMARgn->s.csma_start_time_hi = 0;
        pCSMARgn->s.csma_rxOnly = 0;
    	pCSMARgn->s.csma_hybrid = 1;
    /*	pCSMARgn->s.csma_endtime_lo = CSMA_REGIONS_VAL[0] & 0x00FF;
    	pCSMARgn->s.csma_endtime_hi = ((CSMA_REGIONS_VAL[0]  & 0xFF00) >> 8);
        WriteU32Reg(PLC_CSMAREGION0_REG, pCSMARgn->reg);     //since this address is not continuous we cant use for loop
       
        pCSMARgn->s.csma_start_time_lo = 0; //start_time;
    	pCSMARgn->s.csma_start_time_hi = 0;
        pCSMARgn->s.csma_rxOnly = 0;
    	pCSMARgn->s.csma_hybrid = 1;  */
        
        if(gHpgpHalCB.lineFreq == FREQUENCY_50HZ)
        {
            csma0 = CSMA_REGIONS_VAL_DC[0];  
            csma1 = CSMA_REGIONS_VAL_DC[1]; 
            csma2 = CSMA_REGIONS_VAL_DC[2]; 
            csma3 = CSMA_REGIONS_VAL_DC[3]; 
            csma4 = CSMA_REGIONS_VAL_DC[4]; 
            csma5 = CSMA_REGIONS_VAL_DC[5]; 
        }
        else  if(gHpgpHalCB.lineFreq == FREQUENCY_60HZ)
        {
           csma0 = CSMA_REGIONS_VAL_AC[0];  
           csma1 = CSMA_REGIONS_VAL_AC[1]; 
           csma2 = CSMA_REGIONS_VAL_AC[2]; 
           csma3 = CSMA_REGIONS_VAL_AC[3]; 
           csma4 = CSMA_REGIONS_VAL_AC[4]; 
           csma5 = CSMA_REGIONS_VAL_AC[5]; 
        }
    	pCSMARgn->s.csma_endtime_lo =  csma0 & 0x00FF;
    	pCSMARgn->s.csma_endtime_hi = (( csma0 & 0xFF00) >> 8);
        WriteU32Reg(PLC_CSMAREGION0_REG, pCSMARgn->reg);

        pCSMARgn->s.csma_endtime_lo =  csma1 & 0x00FF;
    	pCSMARgn->s.csma_endtime_hi = (( csma1 & 0xFF00) >> 8);
        WriteU32Reg(PLC_CSMAREGION1_REG, pCSMARgn->reg);

		// need to take of AC later. TRI
        //if(gHpgpHalCB.lineMode == LINE_MODE_DC)
        {
			pCSMARgn->s.csma_endtime_lo =  csma2 & 0x00FF;
    		pCSMARgn->s.csma_endtime_hi = (( csma2 & 0xFF00) >> 8);
        	pCSMARgn->s.csma_rxOnly = TRUE;
			WriteU32Reg(PLC_CSMAREGION2_REG, pCSMARgn->reg);
			    
			pCSMARgn->s.csma_endtime_lo =  csma3 & 0x00FF;
    		pCSMARgn->s.csma_endtime_hi = (( csma3 & 0xFF00) >> 8);
        	pCSMARgn->s.csma_rxOnly = TRUE;
			WriteU32Reg(PLC_CSMAREGION3_REG, pCSMARgn->reg);
			    
			pCSMARgn->s.csma_endtime_lo =  csma4 & 0x00FF;
    		pCSMARgn->s.csma_endtime_hi = (( csma4 & 0xFF00) >> 8);
        	pCSMARgn->s.csma_rxOnly = TRUE;
			WriteU32Reg(PLC_CSMAREGION4_REG, pCSMARgn->reg);    

			pCSMARgn->s.csma_endtime_lo =  csma5 & 0x00FF;
    		pCSMARgn->s.csma_endtime_hi = (( csma5 & 0xFF00) >> 8);
        	pCSMARgn->s.csma_rxOnly = TRUE;
			WriteU32Reg(PLC_CSMAREGION5_REG, pCSMARgn->reg);    
		}


}

/*******************************************************************
* NAME :            HHAL_PlcBcnWrite
*
* DESCRIPTION :     Transmits beacon - writes to TxBcnFifo.
*                   TxBcn format = { 16 bytes AVFC, 12 Byte Bcn Hdr, NBE, BENTRY's ..., ZeroPadding}
*                   TxBcn has to be 152 bytes array, that is zero padded.
*                
*                   
*
* INPUTS :
*       PARAMETERS:
*           u8* pTxBcn    - 152 bytes , zero padded beacon.
*           u8 bcnType    - Discover, proxy or central beacon selectio.
*           u8 bpstoValueOffset - offset within the bcn where HW should isnert BPSTO ( min value is 33, counting from byte 0)
*
* OUTPUTS :
*       None
*
*/
eStatus HHAL_PlcBcnWrite(u8* pTxBcn, u8 bcnType, u8 bpstoValueOffset)
{
    uBcnStatusReg         bcnStatus;
//    uPlcMedStatReg        plcMedStat;  
//    uPlcStatusReg         plcStatus;
    uBpstoReg             bpstoReg;
    eStatus               status;
    u32                   tempU32;
    u8*                   ptempU8Ptr;
    u32                   BcnFifoWrRegAddr;
    u32                   BPSTORegAddr;
    sFrmCtrlBlk*          pAvFcBlk;
    u8                    i;    

		/*Compiler warning suppression*/
		bcnType = bcnType;
	
    status          = STATUS_SUCCESS;
    ptempU8Ptr      = (u8 *)&tempU32; 
    bpstoValueOffset -= sizeof(sFrmCtrlBlk) ;

    // Check if the previous beacon has been sent out.
    // A sanity check only - UL could make sure beacon 
    // sent interrupt is received prior to calling this API.

    bcnStatus.reg   = ReadU32Reg(PLC_BCNSTATUS_REG);
	
	//[YM] -- underline if condition should be changed, base on what kind of beacon is tx. Different type of beacon frame should check different bit field
    /*if(  bcnStatus.s.valid3 ||  bcnStatus.s.valid2 || gHpgpHalCB.bBcnTxPending || gHpgpHalCB.bBcnNotSent)
    { 
        status     = STATUS_FAILURE;
		//[YM] If HW is busy on Beacon transmission, SW can not trigger anyther Beacon Tx.  --> skip current Beacon Tx??
        //printf("Bcn valid not cleared - cannot write beacon\n");
    } */
    // Write HP10 FC followed by AV Bcn MPDU to Bcn Tx Fifo.
    // [YM] Beacon frame construction should be separated with Beacon Tx control. 


	//FM_HexDump(FM_ERROR,"\nTxBcn:",pTxBcn, PLC_BCN_LEN);

	
    if(status == STATUS_SUCCESS) 
    {
   
   
#ifdef UM
		sLinkLayer *linkl = HPGPCTRL_GetLayer(2);	  

		sStaInfo *staInfo = LINKL_GetStaInfo(linkl);
#endif
		   
	   	pAvFcBlk = (sFrmCtrlBlk*) pTxBcn;
		// AV Frame Control	
		pAvFcBlk->access = 0;
#ifdef UM


		pAvFcBlk->snid	= staInfo->snid;
#else
		pAvFcBlk->snid	= gHpgpHalCB.snid;

#endif



#if (PLC_BCNDATA_TXHANG_TEST  && PLC_BCNDATA_FIXED_PATTERN)
    	pAvFcBlk->dtav	 = HPGP_DTAV_SOF;
#else
    	pAvFcBlk->dtav	 = HPGP_DTAV_BCN;
        pAvFcBlk->bto[0] = cpu_to_le16(gHpgpHalCB.curBcnPer - PLC_AC_BP_LEN);
#endif
         // Determine which BPST Offset register to program
				 /*Compiler warning suppression*/
#if 1				 
         if(1)//(bcnType == BEACON_TYPE_CENTRAL) || (bcnType == BEACON_TYPE_PROXY))
         {
            BcnFifoWrRegAddr = PLC_CAP3BCNFIFO_REG;
            BPSTORegAddr = PLC_CAP3BPSTOADDR_REG;
         }
#else				 
         else 
         {
            BcnFifoWrRegAddr = PLC_CAP2BCNFIFO_REG;
            BPSTORegAddr = PLC_CAP2BPSTOADDR_REG;
         }
#endif				 

         // Write BPSTOAddress Register
         bpstoReg.s.bpsto0 = bpstoValueOffset ;
         bpstoReg.s.bpsto1 = bpstoValueOffset + 1;
         bpstoReg.s.bpsto2 = bpstoValueOffset + 2;
         WriteU32Reg(BPSTORegAddr, bpstoReg.reg);

         if(!gHpgpHalCB.halStats.TxBcnCnt)  //[YM] purpose of checking this counter??
         {
             //printf("Wrote Bpsto0 Offset = %lX\n", rtocl(bpstoReg.reg));
         }

         // Write HP10 FC
         WriteU32Reg(BcnFifoWrRegAddr, gHpgpHalCB.plcTx10FC.reg);
         //memcpy(bcnMPDU, &gHpgpHalCB.plcTx10FC.reg, HPGP_HP10FC_LEN);

         // Write rest of the beacon payload. 
         //printf("BcnPld: ");
         for( i=0 ; i<PLC_BCN_LEN ; i+=4)
         {
              ptempU8Ptr[0] = pTxBcn[i];
              ptempU8Ptr[1] = pTxBcn[i+1];
              ptempU8Ptr[2] = pTxBcn[i+2];
              ptempU8Ptr[3] = pTxBcn[i+3]; 
              WriteU32Reg(BcnFifoWrRegAddr, tempU32);
              if(!gHpgpHalCB.halStats.TxBcnCnt)
              {
                  if((i & 0x0f) == 0 )
                  {
                      //printf("\n");
                  }
                  //printf("0x%02bX 0x%02bX 0x%02bX 0x%02bX ", pTxBcn[i], pTxBcn[i+1], pTxBcn[i+2], pTxBcn[i+3]);
              } 
         }
         //printf("\n");
         bcnStatus.reg  = ReadU32Reg(PLC_BCNSTATUS_REG);
		 
         if(!gHpgpHalCB.halStats.TxBcnCnt)  //[YM] purpose of checking this counter??
         {
         //printf("\nWrote %bu bytes to bcn fifo\n\n",i+HPGP_HP10FC_LEN);
         }

         /*******Critical Section Starts **********/
#ifdef Hybrii_A_PLC_SW_CSMA  //Only for Hybrii_A
         {
            // Critical section from now till writing bcn valid bit,                
            // disable Interrupts.
//            u32     bcnTxWaitTimer;
            IRQ_DISABLE_INTERRUPT();
            //EA = 0;
            plcStatus.reg  = ReadU32Reg(PLC_STATUS_REG);
            plcMedStat.reg = ReadU32Reg(PLC_MEDIUMSTATUS_REG);
				
    /*        bcnTxWaitTimer = gHalCB.timerIntCnt;
            while(plcMedStat.s.phyActive || !plcStatus.s.plcMacIdle || plcMedStat.s.crsMac || plcStatus.s.plcTxQRdy)
            {
                {
                    //gHpgpHalCB.bcnTxWaitTimer++;
                    //if(gHpgpHalCB.bcnTxWaitTimer >= PLC_BCNTX_WAIT_TIMEOUT)
                    if(gHalCB.timerIntCnt >= (bcnTxWaitTimer + PLC_BCNTX_WAIT_TIMEOUT)
                     || gHalCB.timerIntCnt < bcnTxWaitTimer)  // Wrap around case
                    {
                        // flush bcn fifo and return
                        bcnStatus.reg = 0;
                        if((bcnType == BEACON_TYPE_CENTRAL) || (bcnType == BEACON_TYPE_PROXY))
                        {
                            bcnStatus.s.flush3 = 1;
                        }
                        else
                        {
                            bcnStatus.s.flush2 = 1;
                        }
                        WriteU32Reg(PLC_BCNSTATUS_REG, bcnStatus.reg);
                        gHpgpHalCB.BcnTxWaitTimeoutCnt ++;
                        //IRQ_ENABLE_INTERRUPT();
                        //EA = 1;
                        return STATUS_FAILURE;
                    }
                }
                plcStatus.reg  = ReadU32Reg(PLC_STATUS_REG);
                plcMedStat.reg = ReadU32Reg(PLC_MEDIUMSTATUS_REG);
            }
       */
           /* if(plcMedStat.s.phyActive || !plcStatus.s.plcMacIdle || plcMedStat.s.crsMac || plcStatus.s.plcTxQRdy)
            {
               gHpgpHalCB.pendBcnType   = bcnType;
               //gHpgpHalCB.bBcnTxPending = 1;
               IRQ_ENABLE_INTERRUPT();

               printf("\n pa = %bu",plcMedStat.s.phyActive);
               printf("\n macI = %bu",plcStatus.s.plcMacIdle);
                printf("\n crs = %bu",plcMedStat.s.crsMac);
                 printf("\n plcTxQ = %bu",plcStatus.s.plcTxQRdy);

               return STATUS_SUCCESS; 
            } */


            {
                // Critical section from now till writing bcn valid                
                // Disable Rx 
                plcStatus.s.nRxEn = 1;
                WriteU32Reg(PLC_STATUS_REG, plcStatus.reg); 

            	plcMedStat.reg = ReadU32Reg(PLC_MEDIUMSTATUS_REG);
                plcStatus.s.crsBypass = 1;
                WriteU32Reg(PLC_STATUS_REG, plcStatus.reg);
                plcStatus.s.crsBypass = 0;
                WriteU32Reg(PLC_STATUS_REG, plcStatus.reg);
            }
         }
#endif  //Hybrii_A_PLC_SW_CSMA

         //[YM] this line should add extra protection with device role. And, the else condition make sense??
				/*Compiler warning suppression*/
#if 1				 
		 if(1)//(bcnType == BEACON_TYPE_CENTRAL) || (bcnType == BEACON_TYPE_PROXY))
         {
            bcnStatus.s.valid3 = 1;  
         }
#else				 
         else 
         {
            bcnStatus.s.valid2 = 1;  //??
         }
#endif				 
         //gHpgpHalCB.bBcnNotSent = 1;
         WriteU32Reg(PLC_BCNSTATUS_REG, bcnStatus.reg);
         gHpgpHalCB.halStats.TxBcnCnt++;
         gHpgpHalCB.halStats.TotalTxFrmCnt++;
         //IRQ_ENABLE_INTERRUPT();// no one is disabling interrupts
//         EA = 1;
         /*******Critical Section Ends **********/
         
    }
    //printf("BPST = %lx\n", rtocl(ReadU32Reg(PLC_BPST_REG)));
    if (status == STATUS_FAILURE)
    {
         gHpgpHalCB.halStats.macTxStuckCnt++;  
    }
    else
    {
         gHpgpHalCB.halStats.macTxStuckCnt = 0;
         gHpgpHalCB.halStats.smTxStuckCnt = 0;
    }
    return status;
}

#ifndef HPGP_HAL_TEST

void HHAL_BcnTxIntHandler()
{
    uPlcMedStatReg  plcMedStat;   
    uPlcStatusReg   plcStatus;
    static u32 prevBPInt_TimerCnt;
    static u32 curBPInt_TimerCnt;
    static u8  bpInit;
	sHpgpHalCB *hhalCb;
#ifndef HPGP_HAL_TEST
    sHaLayer *hal = (sHaLayer*)HOMEPLUG_GetHal();
#endif
    
#ifdef HPGP_HAL_TEST
    hhalCb = &gHpgpHalCB;
#else
    hhalCb = hal->hhalCb;
#endif
    // [YM] For Beacon sync timing adjust
    
    // Determine time gap between 2  consecutive HPGP BP interrupts.
    if( hhalCb->halStats.bpIntCnt )
    {
          bpInit = 1;
    } 
    if( bpInit )
    {
        prevBPInt_TimerCnt = curBPInt_TimerCnt;
    }
    curBPInt_TimerCnt   = gHalCB.timerIntCnt;
    hhalCb->bpIntGap = curBPInt_TimerCnt - prevBPInt_TimerCnt;
	hhalCb->halStats.bpIntCnt++;

    // Compute running average of prev 64 ZeroCrossing periods
    // and write to Bcn Period Average register.
    if( hhalCb->devMode == DEV_MODE_CCO && hhalCb->lineMode == LINE_MODE_AC )
    {
        //printf("HW Bcn PER = %lx\n",rtocl(ReadU32Reg(PLC_HWBCNPERLEN_REG)));
        hhalCb->curBcnPer = rtocl(ReadU32Reg(PLC_HWBCNPERCUR_REG));
        if( hhalCb->curBcnPer < PLC_MIN_AC_BPLEN )
        {
            hhalCb->curBcnPer = PLC_MIN_AC_BPLEN;
        }
        else if( hhalCb->curBcnPer > PLC_MAX_AC_BPLEN )
        {
            hhalCb->curBcnPer = PLC_MAX_AC_BPLEN;
        }

        hhalCb->bcnPerSum += hhalCb->curBcnPer;
        hhalCb->perSumCnt ++;

        if( hhalCb->perSumCnt >= PLC_BCNPERAVG_CNT )
        {
            hhalCb->bPerAvgInitDone = 1;
        }
        if( hhalCb->bPerAvgInitDone )
        {
            hhalCb->curBcnPer       = hhalCb->bcnPerSum >> PLC_BCNPERAVG_DIVCNT;
            hhalCb->bcnPerSum      -= hhalCb->curBcnPer;
        }
       // WriteU32Reg(PLC_SWBCNPERAVG_REG, ctorl(hhalCb->curBcnPer));
        //WriteU32Reg( PLC_SWBCNPERAVG_REG, ctorl(0xCB735));
        //printf("SW Bcn PER = %lx\n",gHpgpHalCB.curBcnPer);
    }

	WriteU32Reg(PLC_SWBCNPERAVG_REG, ctorl((gHpgpHalCB.curBcnPer >> 1)));
    // Prepare and send beacon here for now.
    // This will eventually be done by hpgp nsm module.
#ifdef HPGP_HAL_TEST
    HHT_BPIntHandler();
#else
#ifdef CALLBACK
    hal->xmitBcn(hal->txBcnCookie);
#else
	//LINKL_BcnTxHandler(hal->txBcnCookie);
    if(hhalCb->lmBcn.txBitmap & (1 << BEACON_TYPE_CENTRAL))
    {
	    HHAL_PlcBcnWrite(hhalCb->lmBcn.cBcn.bcnBuff, BEACON_TYPE_CENTRAL,
	                 hhalCb->lmBcn.cBcn.bpstoOffset);
    }
#endif  //CALLBACK
#endif  //HPGP_HAL_TEST

    //[YM] Expect to remove from Hybrii-B
    
    // PHY Active Hang workaround
    plcStatus.reg  = ReadU32Reg(PLC_STATUS_REG);
    plcMedStat.reg = ReadU32Reg(PLC_MEDIUMSTATUS_REG); 
    if( plcMedStat.s.phyActive && plcMedStat.s.mpiRxEn )
    {
        if(hhalCb->halStats.paRxEnHiCnt > PLC_RXPHYACT_HANG_THRES )
        {
#ifdef _RX_RECOVERY_                        
            plcStatus.s.nRxEn = 1;
            WriteU32Reg(PLC_STATUS_REG, plcStatus.reg);
            CHAL_DelayTicks(5);
            
            plcStatus.s.plcRxEnSwCtrl  = 0;
            WriteU32Reg(PLC_STATUS_REG, plcStatus.reg);

            plcStatus.s.plcRxEnSwCtrl  = 1;
            WriteU32Reg(PLC_STATUS_REG, plcStatus.reg); 
#endif  //  _RX_RECOVERY_            
            hhalCb->halStats.paRxEnHiCnt = 0;   
            hhalCb->halStats.phyActHangRstCnt++;                    
        }
        else if( hhalCb->halStats.prevBPTotalRxCnt == hhalCb->halStats.TotalRxGoodFrmCnt )
        {
            hhalCb->halStats.paRxEnHiCnt ++;
        } 
        else
        {
            hhalCb->halStats.paRxEnHiCnt = 0;  
        }                
    }
    hhalCb->halStats.prevBPTotalRxCnt = hhalCb->halStats.TotalRxGoodFrmCnt;  

#ifdef CCO_FUNC
    LINKL_UpdateBeacon();
#endif    
}

#endif


eStatus HHAL_IsPlcIdle()
{
    
    uBcnStatusReg         bcnStatus;
    uPlcStatusReg         plcStatus;
    uPlcMedStatReg        plcMedStat;
    //uCpuSwStatusReg       cpuSwStat;   //[YM] redefine cpuSwStat structure and fields
    uPlcLineControlReg    plcLineCtrl; 


    plcStatus.reg   = ReadU32Reg(PLC_STATUS_REG); 
    plcMedStat.reg  = ReadU32Reg(PLC_MEDIUMSTATUS_REG);

    bcnStatus.reg   = ReadU32Reg(PLC_BCNSTATUS_REG);
    //cpuSwStat.reg = ReadU32Reg(CPU_SWSTATUS_REG);  //[YM] Hybrii_B changes setting register to 0xFDA4-PLC Line Cycle Control register 
    plcLineCtrl.reg = ReadU32Reg(PLC_LINECTRL_REG);
    //redefine cpuSwStat structure and fields
           
    if((plcStatus.s.plcTxQRdy && plcStatus.s.plcTxQSwCtrl)  \
    //    || cpuSwStat.s.reqScan || gHpgpHalCB.bTxPending )        //[YM] redefine cpuSwStat structure and fields
         || plcLineCtrl.s.reqScanning || gHpgpHalCB.bTxPending || (!gHpgpHalCB.syncComplete)) //||( gTxDone == 0) )
    {
#if 0
   
    FM_Printf(FM_ERROR, "\n txdone p\%bu , %bu , %bu, %bu, %bu,%bu , %bu , %bu, %bu \n", plcStatus.s.plcTxQRdy,plcStatus.s.plcTxQSwCtrl,
                cpuSwStat.s.reqScan,    plcMedStat.s.phyActive, plcStatus.s.plcMacIdle,
                plcMedStat.s.crsMac , plcMedStat.s.txWindow, gHpgpHalCB.bBcnTxPending,
                gHpgpHalCB.bBcnNotSent);

#endif
#ifdef ETH_BRDG_DEBUG
		if (myDebugFlag)
		{
			printf("HHAL_IsPlcIdle: return FALSE, plcTxQRdy=%bu,plcTxQSwCtrl=%bu,reqScanning=%bu,gHpgpHalCB.bTxPending=%bu\n", 
				plcStatus.s.plcTxQRdy,plcStatus.s.plcTxQSwCtrl,plcLineCtrl.s.reqScanning,gHpgpHalCB.bTxPending);
    		printf("		phyActive=%bu, plcMacIdle=%bu,crsMac=%bu , txWindow=%bu\n",
                	plcMedStat.s.phyActive, plcStatus.s.plcMacIdle,
                	plcMedStat.s.crsMac , plcMedStat.s.txWindow);
		}
#endif
        return STATUS_FAILURE;
    }

#if 0
   
    FM_Printf(FM_ERROR, "\n txdone p\%bu , %bu , %bu, %bu, %bu,%bu , %bu , %bu, %bu \n", plcStatus.s.plcTxQRdy,plcStatus.s.plcTxQSwCtrl,
                cpuSwStat.s.reqScan,    plcMedStat.s.phyActive, plcStatus.s.plcMacIdle,
                plcMedStat.s.crsMac , plcMedStat.s.txWindow, gHpgpHalCB.bBcnTxPending,
                gHpgpHalCB.bBcnNotSent);

#endif
    
    return STATUS_SUCCESS;        
}


         
/*******************************************************************
* NAME :            HHAL_PlcTxQWrite
*
* DESCRIPTION :     Transmits data/mgmt frames - writes Hdr & CP desc. to PLCTxQ.
*
* INPUTS :
*       PARAMETERS:
*           sPlcTxFrmSwDesc     Data structure with info about tx frame.
*
* OUTPUTS :
*       None
*
*/


#ifdef HPGP_HAL_TEST
eStatus HHAL_PlcTxQWrite(sSwFrmDesc* txFrmSwDesc)
#else
eStatus HHAL_PlcTxQWrite(sHaLayer *hal, sSwFrmDesc* txFrmSwDesc)
#endif

{
    u8                    frmDescCnt;
//    u8                    plcTxFreeDescCnt;
    eStatus               status;
    uBcnStatusReg         bcnStatus;
    uPlcStatusReg         plcStatus;
    uPlcMedStatReg        plcMedStat;
    //uCpuSwStatusReg       cpuSwStat;   //[YM] redefine cpuSwStat structure and fields
    uPlcLineControlReg    plcLineCtrl;
//    uCapTxQStatusReg      capTxQStat;
    uTxFrmHwDesc          txfrmHwDesc;
    uPlcTxPktQDescVF0     vofDesc0;
    uPlcTxPktQDescVF1     vofDesc1;
    uPlcTxPktQDescVF2     vofDesc2;
    uPlcTxPktQDescVF3     vofDesc3;
    uPlcTxPktQCAP_Write   cap_write;
    uTxCpDesc             txCpDesc;
    uTxCMDQueueWrite      txCmdQueueWrtie;
    eHpgpRoboMod         roboMode;
    ePlcNumPBs            numPBs;
    u16                   flAv;
    u16                   miniRoboFrmLenMax;
    u16                   stdRoboFrmLenMax;
    u16                   hsRobo2PbFrmLenMax;
    u16                   hsRobo3PbFrmLenMax;
	u8                    CapQueueStatus;
    u8                    mfsCmd;
    u8                    i, *tmpPtr;
//    u32                   medChkB4;
	u32 				  val_32;

    status     = STATUS_SUCCESS;
    frmDescCnt = PLC_HDR_DESC_COUNT + txFrmSwDesc->cpCount; 

    bcnStatus.reg   = ReadU32Reg(PLC_BCNSTATUS_REG);  //[YM] Reason to check Beacon Status register?? 
    //cpuSwStat.reg = ReadU32Reg(CPU_SWSTATUS_REG);     //[YM] redefine cpuSwStat structure and fields
    plcLineCtrl.reg = ReadU32Reg(PLC_LINECTRL_REG);
    plcStatus.reg = ReadU32Reg(PLC_STATUS_REG);
    plcMedStat.reg = ReadU32Reg(PLC_MEDIUMSTATUS_REG); 
  
#if 1 //def HPGP_HAL_TEST  
    if((plcStatus.s.plcTxQRdy && plcStatus.s.plcTxQSwCtrl) || (1 == eth_plc_sniffer)) //[YM] Tx one packet at a time
	//if(1 == eth_plc_sniffer)	   //[YM] For Throughput Improvement
	/*
    //if(((plcStatus.s.plcTxQRdy && plcStatus.s.plcTxQSwCtrl)  \
	//     || plcLineCtrl.s.reqScanning )|| gHpgpHalCB.bTxPending )
       // || cpuSwStat.s.reqScan )|| gHpgpHalCB.bTxPending )    //[YM] redefine cpuSwStat structure and fields
	//if(((plcStatus.s.plcTxQRdy && plcStatus.s.plcTxQHwCtrl)  \
      //   || plcLineCtrl.s.reqScanning )|| gHpgpHalCB.bTxPending )
        													 */
    {
        return STATUS_FAILURE;  //[YM] what to do after checking status = Failure?? Retry? or drop packet?
    }
	else	
#endif
    {
    
    if (txFrmSwDesc->frmInfo.plc.plid == 0)
	{
	//printf("txFrmSwDesc->frmInfo.plc.plid is %bu\n", txFrmSwDesc->frmInfo.plc.plid);
		CapQueueStatus = ReadU8Reg(PLC_QDSTATUS_REG);
	//printf("assigned 1 to CapRdy\n");
	}
	else if (txFrmSwDesc->frmInfo.plc.plid == 1)
	{
		CapQueueStatus = ReadU8Reg(PLC_QDSTATUS_REG+1);
	//printf("assigned 2 to CapRdy\n");
	}
	else if (txFrmSwDesc->frmInfo.plc.plid == 2)
	{
		CapQueueStatus = ReadU8Reg(PLC_QDSTATUS_REG+2);
	//printf("assigned 4 to CapRdy\n");
	}
	else if (txFrmSwDesc->frmInfo.plc.plid == 3)
	{
		CapQueueStatus = ReadU8Reg(PLC_QDSTATUS_REG+3);
	//printf("assigned 8 to CapRdy\n");
	}     

    if ((PLC_TXQ_DEPTH - CapQueueStatus)< PLC_TX_DESC_QUEUE_TH)
    {
   //    	printf("\n 1:NOT enough descriptor space for next packet\n");
	//	printf("\n 1:PLC_Queue_Descriptor_Status = %lx\n", ReadU32Reg(PLC_QDSTATUS_REG));
		status = STATUS_FAILURE;
    }
    else 
    {
    //FM_Printf(FM_MINFO, "TxQWrite:1\n");
    /* 1.Create frame descriptor */
    memset(&txfrmHwDesc, 0x00, sizeof(txfrmHwDesc));
    memset(&vofDesc0, 0x00, sizeof(vofDesc0));
    memset(&vofDesc1, 0x00, sizeof(vofDesc1));
    memset(&vofDesc2, 0x00, sizeof(vofDesc2));
    memset(&vofDesc3, 0x00, sizeof(vofDesc3));
    memset(&txCpDesc, 0x00, sizeof(txCpDesc));
    // So frameLen/type is used to determine roboMode, PB Count,
    if(txFrmSwDesc->frmType == HPGP_HW_FRMTYPE_MGMT)
    {
        miniRoboFrmLenMax             = HYBRII_MINIROBO_MGMTLEN_MAX;
        stdRoboFrmLenMax              = HYBRII_STD1PBHSROBO_MGMTLEN_MAX;
        hsRobo2PbFrmLenMax            = HYBRII_2PBHSROBO_MGMTLEN_MAX;
        hsRobo3PbFrmLenMax            = HYBRII_3PBHSROBO_MGMTLEN_MAX;

    }
    else
    {
        miniRoboFrmLenMax             = HYBRII_MINIROBO_DATALEN_MAX;
        stdRoboFrmLenMax              = HYBRII_STD1PBHSROBO_DATALEN_MAX;
        hsRobo2PbFrmLenMax            = HYBRII_2PBHSROBO_DATALEN_MAX;
        hsRobo3PbFrmLenMax            = HYBRII_3PBHSROBO_DATALEN_MAX; 
    } 
#ifdef ETH_BRDG_DEBUG
//	if (myDebugFlag)
//		printf("PLCTXQWRITE: frame Len: %d\n",txFrmSwDesc->frmLen);
#endif
    if(txFrmSwDesc->frmType == HPGP_HW_FRMTYPE_SOUND)
    {
       if(txFrmSwDesc->frmLen > 122)
       {
         roboMode    = HPGP_ROBOMD_STD;
         flAv        = HPGP_STDROBO_FLAV;
		 numPBs      = PLC_ONE_PB;
       }
       else
       {
         roboMode       = HPGP_ROBOMD_MINI;
         flAv           = HPGP_MINIROBO_FLAV;
		 numPBs         = PLC_ONE_PB;
       }
    }
    else
    { 
    if(txFrmSwDesc->frmLen <= miniRoboFrmLenMax)  //frmLen <= 122 or 118 bytes
    {
        roboMode       = HPGP_ROBOMD_MINI;
        flAv           = HPGP_MINIROBO_FLAV;
        numPBs         = PLC_ONE_PB;
    }
    else if( (txFrmSwDesc->frmLen > miniRoboFrmLenMax) && (txFrmSwDesc->frmLen <= stdRoboFrmLenMax))  // 122 or 118 Bytes < frmLen <= 506/502 bytes
    {
//#ifdef HPGP_HAL_TEST        
        if(txFrmSwDesc->frmInfo.plc.stdModeSel)
        {
            roboMode    = HPGP_ROBOMD_STD;
            flAv        = HPGP_STDROBO_FLAV;
        }
        else
//#endif
        {   
            roboMode    = HPGP_ROBOMD_HS;                
            flAv        = HPGP_1PBHSROBO_FLAV;             
        }
        numPBs  = PLC_ONE_PB;           
    }
    else  //FrmLen > 506/502 Bytes
    {
        roboMode       = HPGP_ROBOMD_HS;
        if( (txFrmSwDesc->frmLen > stdRoboFrmLenMax) && (txFrmSwDesc->frmLen <= hsRobo2PbFrmLenMax))  //506/502 bytes < FrmLen <= 1018/1014 bytes
        {
            flAv   = HPGP_2PBHSROBO_FLAV;
            numPBs = PLC_TWO_PB;
        }
        else if(txFrmSwDesc->frmLen <= hsRobo3PbFrmLenMax)  // 1018/1014 bytes < FrmLen <= 1530/1526 bytes
        {
            flAv   = HPGP_3PBHSROBO_FLAV;
            numPBs = PLC_THREE_PB;
        }   
        else  //FrmLen > 1530/1526 bytes
        {
            // Unsupported frm len
#ifdef ETH_BRDG_DEBUG
	//		if (myDebugFlag)
	//			printf("PLCTXQWRITE: unsupported frame: %d\n",txFrmSwDesc->frmLen);
#endif 
            status = STATUS_FAILURE;
        }
    }
    }
#ifdef ETH_BRDG_DEBUG
	//if (myDebugFlag)
	//	printf("flAv=0x%x\n", flAv);
#endif
     /********************************/
    /*0.  Create Cap_write register value to HW   */
    cap_write.reg = 0;
#ifdef ETH_BRDG_DEBUG
	if (myDebugFlag)
	{
		printf("txFrmSwDesc->frmInfo.plc.dtei=%bx, txFrmSwDesc->frmInfo.plc.stei=%bx\n", txFrmSwDesc->frmInfo.plc.dtei, txFrmSwDesc->frmInfo.plc.stei);
		printf("txFrmSwDesc->frmInfo.plc.mcstMode=%bx, txFrmSwDesc->frmInfo.plc.roboMode=%bx, txFrmSwDesc->frmInfo.plc.eks=%bx\n", 
			txFrmSwDesc->frmInfo.plc.mcstMode, txFrmSwDesc->frmInfo.plc.roboMode, txFrmSwDesc->frmInfo.plc.eks);
		printf("clst=%bx, plid=%bx, pbsz=%bx,mfstart=%bx, mfend=%bx, bcnDetectFlag=%bx, scf=%bx\n", 
			txFrmSwDesc->frmInfo.plc.clst, txFrmSwDesc->frmInfo.plc.plid, txFrmSwDesc->frmInfo.plc.pbsz, txFrmSwDesc->frmInfo.plc.mfStart,
			txFrmSwDesc->frmInfo.plc.mfEnd, txFrmSwDesc->frmInfo.plc.bcnDetectFlag,txFrmSwDesc->frmInfo.plc.scf);
	}
#endif
	tmpPtr = (u8 *) &txFrmSwDesc->frmInfo.plc;
	//printf("tmpPtr[] is:\n    ");
	//for (i=0;i<20;i++)
	//	printf("%bx ", tmpPtr[i]);
	//printf("\n");
	//printf("txFrmSwDesc->frmInfo.plc.plid=%bx\n", txFrmSwDesc->frmInfo.plc.plid);
	cap_write.capw.Cap = txFrmSwDesc->frmInfo.plc.plid;
	if (txFrmSwDesc->frmInfo.plc.plid == 0)
	{
	//printf("txFrmSwDesc->frmInfo.plc.plid is %bu\n", txFrmSwDesc->frmInfo.plc.plid);
		cap_write.capw.CapRdy = 1;
#ifdef ETH_BRDG_DEBUG
		if (myDebugFlag)
			printf("assigned 1 to CapRdy\n");
#endif
	}
	else if (txFrmSwDesc->frmInfo.plc.plid == 1)
	{
		cap_write.capw.CapRdy = 2;
#ifdef ETH_BRDG_DEBUG
		if (myDebugFlag)
			printf("assigned 2 to CapRdy\n");
#endif
	}
	else if (txFrmSwDesc->frmInfo.plc.plid == 2)
	{
		cap_write.capw.CapRdy = 4;
#ifdef ETH_BRDG_DEBUG
		if (myDebugFlag)
			printf("assigned 4 to CapRdy\n");
#endif
	}
	else if (txFrmSwDesc->frmInfo.plc.plid == 3)
	{
		cap_write.capw.CapRdy = 8;
#ifdef ETH_BRDG_DEBUG
		if (myDebugFlag)
			printf("assigned 8 to CapRdy\n");
#endif
	}
#ifdef ETH_BRDG_DEBUG
	else
		if (myDebugFlag)
	 		printf("nothing\n");
#endif
		
	//printf("2. cap_write.reg=0x%lX\n", cap_write.reg);
	//printf("cap_write.capw.CapRdy=%bu\n", cap_write.capw.CapRdy);	

    
    /********************************/
    /*1.  Create first descriptor to HW              */

    txfrmHwDesc.reg = 0;
    
    /************************************/
    if (txFrmSwDesc->frmType == HPGP_HW_FRMTYPE_SOUND)
    {
        /* 1.1 Write Frame Length */
        txfrmHwDesc.sound.frmLenLo = txFrmSwDesc->frmLen & PKTQDESC1_FRMLENLO_MASK;
        txfrmHwDesc.sound.frmLenHi = (txFrmSwDesc->frmLen & PKTQDESC1_FRMLENHI_MASK) >> PKTQDESC1_FRMLENHI_POS;

        txfrmHwDesc.sound.bcst = 1;
		txfrmHwDesc.sound.mcst = 1;
        /* 1.3 Set Channel Access Priority (same as PLID) */
        txfrmHwDesc.sound.cap = txFrmSwDesc->frmInfo.plc.plid;
    
        /* 1.4 Set Frame Type and get corres. max frame length values */
        txfrmHwDesc.sound.frmType = HPGP_HW_FRMTYPE_SOUND;
            
        // 1.5 Set security Enable bit and Key Idx(EKS)
        //txfrmHwDesc.sound.secKeyIdx    = 0xF;  
		
        /* 1.6 Set dtei */
		txfrmHwDesc.sound.dteiLo1 =  txFrmSwDesc->frmInfo.plc.dtei & PKTQDESC1_DTEILO1_MASK;
        txfrmHwDesc.sound.dteiLo2 = (txFrmSwDesc->frmInfo.plc.dtei & PKTQDESC1_DTEILO2_MASK) >> PKTQDESC1_DTEILO2_POS;
        txfrmHwDesc.sound.dteiHi = (txFrmSwDesc->frmInfo.plc.dtei & PKTQDESC1_DTEIHI_MASK) >> PKTQDESC1_DTEIHI_POS;
    
        /* 1.7 Set First Descriptor Flag */
        txfrmHwDesc.sound.bFirstDesc = 1;    
    }
    else // Sof
    {

        /* 1.1 Write Frame Length */
        txfrmHwDesc.sof.frmLenLo = txFrmSwDesc->frmLen & PKTQDESC1_FRMLENLO_MASK;
        txfrmHwDesc.sof.frmLenHi = (txFrmSwDesc->frmLen & PKTQDESC1_FRMLENHI_MASK) >> PKTQDESC1_FRMLENHI_POS;

        // 1.2 Set mcst/bcst mode bits
        if(txFrmSwDesc->frmInfo.plc.mcstMode == HPGP_MNBCST)
        {
            txfrmHwDesc.sof.bcst     = 1;
        }
        else if(txFrmSwDesc->frmInfo.plc.mcstMode == HPGP_MCST)
        {
            txfrmHwDesc.sof.mcst     = 1;           
        }
        // 1.3 Set security Enable bit and Key Idx(EKS)
            txfrmHwDesc.sof.secKeyIdx    = txFrmSwDesc->frmInfo.plc.eks;    

    /* 1.4 Set Channel Access Priority (same as PLID) */
    txfrmHwDesc.sof.cap = txFrmSwDesc->frmInfo.plc.plid;

    /* 1.5 Set Frame Type and get corres. max frame length values */
    if (txFrmSwDesc->frmType == HPGP_HW_FRMTYPE_MGMT)
    {
        txfrmHwDesc.sof.frmType = HPGP_HW_FRMTYPE_MGMT;
    } 
    else
    {
        txfrmHwDesc.sof.frmType = HPGP_HW_FRMTYPE_MSDU;
    } 

    /* 1.6 Set dtei */
    txfrmHwDesc.sof.dteiLo1 =  txFrmSwDesc->frmInfo.plc.dtei & PKTQDESC1_DTEILO1_MASK;
    txfrmHwDesc.sof.dteiLo2 = (txFrmSwDesc->frmInfo.plc.dtei & PKTQDESC1_DTEILO2_MASK) >> PKTQDESC1_DTEILO2_POS;
    txfrmHwDesc.sof.dteiHi = (txFrmSwDesc->frmInfo.plc.dtei & PKTQDESC1_DTEIHI_MASK) >> PKTQDESC1_DTEIHI_POS;

    /* 1.7 Set First Descriptor Flag */
    txfrmHwDesc.sof.bFirstDesc = 1;    
    }

    /* 2. Create VOF descriptor 0 */
    vofDesc0.reg     = 0;
    if (txFrmSwDesc->frmType == HPGP_HW_FRMTYPE_SOUND)
    {
        vofDesc0.s.dt_av = HPGP_DTAV_SOUND;
    }
	else if (txFrmSwDesc->frmInfo.plc.dt_av == HPGP_DTAV_RTS_CTS)
	{
	    vofDesc0.s.dt_av = HPGP_DTAV_RTS_CTS;
	}
	else if (txFrmSwDesc->frmInfo.plc.dt_av == HPGP_DTAV_RSOF)
    {
        vofDesc0.s.dt_av = HPGP_DTAV_RSOF;
    }
	else
	    vofDesc0.s.dt_av = HPGP_DTAV_SOF;

#ifdef HPGP_HAL_TEST

    vofDesc0.s.snid  = gHpgpHalCB.snid;
#else

//    vofDesc0.s.snid  = hal->hhalCb->snid;
    vofDesc0.s.snid  = txFrmSwDesc->frmInfo.plc.snid;


#endif
    vofDesc0.s.dtei  = txFrmSwDesc->frmInfo.plc.dtei;
    vofDesc0.s.stei  = txFrmSwDesc->frmInfo.plc.stei;
    vofDesc0.s.plid  = txFrmSwDesc->frmInfo.plc.plid;

    /* 3. Create VOF descriptor 1 */
    vofDesc1.reg     = 0;        

    if (txFrmSwDesc->frmType == HPGP_HW_FRMTYPE_SOUND)
    {
        if (roboMode == HPGP_ROBOMD_MINI)
           vofDesc1.sound.pbSz = HPGP_PHYBLKSIZE_136;
        else
           vofDesc1.sound.pbSz = HPGP_PHYBLKSIZE_520;

#ifdef HPGP_HAL_TEST
        vofDesc1.sound.bdf = gHpgpHalCB.bcnDetectFlag;

#else
        vofDesc1.sound.bdf  = hal->hhalCb->bcnDetectFlag;
#endif


        vofDesc1.sound.saf  = txFrmSwDesc->frmInfo.plc.saf;
        vofDesc1.sound.scf  = txFrmSwDesc->frmInfo.plc.scf;
        vofDesc1.sound.flAvLo = flAv & PKTQDESCVF2_FRMLENAVLO_MASK;
        vofDesc1.sound.flAvHi = (flAv & PKTQDESCVF2_FRMLENAVHI_MASK) >>
        PKTQDESCVF2_FRMLENAVHI_POS;

        vofDesc1.sound.ppb   = txFrmSwDesc->frmInfo.plc.phyPendBlks;       

    }
    else  //SOF packet
    {
#if 1 //For RTS/CTS Tx Test
       if (txFrmSwDesc->frmInfo.plc.dt_av == HPGP_DTAV_RTS_CTS)
	   	  vofDesc1.sof.cfs = 0;
	   
	   // No RTS/CTS Flag in SOF VOF1 Descriptor	
#endif
	
#ifdef HPGP_HAL_TEST
        vofDesc1.sof.bdf = gHpgpHalCB.bcnDetectFlag;

#else
        vofDesc1.sof.bdf  = hal->hhalCb->bcnDetectFlag;
#endif
        vofDesc1.sof.eks  = txFrmSwDesc->frmInfo.plc.eks;
        vofDesc1.sof.pbSz = (roboMode == HPGP_ROBOMD_MINI) ?
        HPGP_PHYBLKSIZE_136 : HPGP_PHYBLKSIZE_520;
        //pktQDescVF1.s.numSym    = // - tbd    
#ifdef QCA
        vofDesc1.sof.numSym = 3;
        vofDesc1.sof.ble = 0x39;
#endif

        // Set Pb Size in frame control.  
  //      txFrmSwDesc->frmInfo.plc.phyPendBlks;
#if 1        
		//for DBC Test, if MCF bit = 1, ppb field should be set to 0xFF
		if (txFrmSwDesc->frmInfo.plc.plid == 0)
		  vofDesc1.sof.ppb        = 0xDF;
		else
          vofDesc1.sof.ppb        = 0xEF; 
#endif		
		 
		
        // tmiAv value is same as roboMode value  
        vofDesc1.sof.tmiAV      = roboMode; 
    }
#ifdef ETH_BRDG_DEBUG
	if (myDebugFlag)
	    printf("vofDesc1.sof.tmiAV = roboMode =0x%bu, pbsize = 0x%bu, ppb = 0x%bu\n", vofDesc1.sof.tmiAV, vofDesc1.sof.bdf, vofDesc1.sof.ppb);
#endif


    /* 4. Create VOF descriptor 2 */
    vofDesc2.reg     = 0;

    if (txFrmSwDesc->frmType == HPGP_HW_FRMTYPE_SOUND)
    {
        switch (txFrmSwDesc->frmInfo.plc.src)
        {
            case 0:
                vofDesc2.s1.sndRsnCd = HPGP_SRC_TONE_MAP_ERROR;
                break; 
            case 1:
                vofDesc2.s1.sndRsnCd = HPGP_SRC_NO_AC_LINE_TM;
                break; 
            case 2:
            default:
                vofDesc2.s1.sndRsnCd = HPGP_SRC_UNUSABLE_INTERVAL;
                break; 
        }
    }   
    else
    {

        vofDesc2.s.clst  = txFrmSwDesc->frmInfo.plc.clst;

        vofDesc2.s.flAvLo = flAv & PKTQDESCVF2_FRMLENAVLO_MASK;
        vofDesc2.s.flAvHi = (flAv  & PKTQDESCVF2_FRMLENAVHI_MASK) >>
                                      PKTQDESCVF2_FRMLENAVHI_POS;

        // Set MCST & MNBCST
        if(txFrmSwDesc->frmInfo.plc.mcstMode  == HPGP_MNBCST)
        {
            vofDesc2.s.mnbf   = 1;
            vofDesc2.s.mcf    = 1;
        }
        else if(txFrmSwDesc->frmInfo.plc.mcstMode  == HPGP_MCST)
        {
            vofDesc2.s.mcf    = 1;
        }
        // Set mfsCmdData or mfsCmdMgmt
        mfsCmd =  (vofDesc2.s.mcf ? HPGP_MFSCMD_NOP : HPGP_MFSCMD_INIT);     
        if(txfrmHwDesc.sof.frmType == HPGP_HW_FRMTYPE_MSDU)
        {
            vofDesc2.s.mfsCmdData =  mfsCmd;
        }
        else if(txfrmHwDesc.sof.frmType == HPGP_HW_FRMTYPE_MGMT)
        {
            vofDesc2.s.mfsCmdMgmt =  mfsCmd;
        }
#ifdef QCA
        vofDesc2.s.mfsCmdData =  HPGP_MFSCMD_NOP;

//    vofDesc2.s.mfsCmdMgmt =  HPGP_MFSCMD_NOP;

#endif
    }

    /* Set mfsCmdData or mfsCmdMgmt */

    /* 5. Create VOF descriptor 3 */
    /* MMQF and PHY side band info */
    vofDesc3.reg = 0;
    
    vofDesc3.s.bmSackI  = 0xF;;
    vofDesc3.s.phySdBdRoboMd = roboMode;;
    vofDesc3.s.phySdBdNumPBs = numPBs;
    if(txfrmHwDesc.sof.frmType == HPGP_HW_FRMTYPE_MGMT)
    {
        vofDesc3.s.mmqf = 1;
    }

/*
    
    FM_HexDump(FM_ERROR, "PLC Write HW Spec " ,(u8*)&txfrmHwDesc, sizeof(txfrmHwDesc));
    FM_HexDump(FM_ERROR, "PLC Write vofDesc0 " ,(u8*)&vofDesc0, sizeof(vofDesc0));
    FM_HexDump(FM_ERROR, "PLC Write vofDesc1 " ,(u8*)&vofDesc1, sizeof(vofDesc1));
    FM_HexDump(FM_ERROR, "PLC Write vofDesc2 " ,(u8*)&vofDesc2, sizeof(vofDesc2));
     FM_HexDump(FM_ERROR, "PLC Write vofDesc3 " ,(u8*)&vofDesc3, sizeof(vofDesc3));

*/

    //FM_Printf(FM_MINFO, "TxQWrite:2\n");

    /* 6. Write first descriptor to the queue */
    //WriteU32Reg(PLC_CAP_REG, ctorl(txFrmSwDesc->frmInfo.plc.plid));
    // [YM] check CAP queue status before push more test packet in to PLC CAP queue
    

    if (txFrmSwDesc->frmInfo.plc.plid == 0)
	{
	//printf("txFrmSwDesc->frmInfo.plc.plid is %bu\n", txFrmSwDesc->frmInfo.plc.plid);
		CapQueueStatus = ReadU8Reg(PLC_QDSTATUS_REG);
	//printf("assigned 1 to CapRdy\n");
	}
	else if (txFrmSwDesc->frmInfo.plc.plid == 1)
	{
		CapQueueStatus = ReadU8Reg(PLC_QDSTATUS_REG+1);
	//printf("assigned 2 to CapRdy\n");
	}
	else if (txFrmSwDesc->frmInfo.plc.plid == 2)
	{
		CapQueueStatus = ReadU8Reg(PLC_QDSTATUS_REG+2);
	//printf("assigned 4 to CapRdy\n");
	}
	else if (txFrmSwDesc->frmInfo.plc.plid == 3)
	{
		CapQueueStatus = ReadU8Reg(PLC_QDSTATUS_REG+3);
	//printf("assigned 8 to CapRdy\n");
	}
	//else printf("nothing\n");
    if ((PLC_TXQ_DEPTH - CapQueueStatus)< PLC_TX_DESC_QUEUE_TH)
    {
     //  	printf("\n2:NOT enough descriptor space for next packet\n");
	//	printf("\n2:PLC_Queue_Descriptor_Status = %lx\n", ReadU32Reg(PLC_QDSTATUS_REG));
		status = STATUS_FAILURE;
    }
    else
    {
	    IRQ_DISABLE_INTERRUPT();
        WriteU32Reg(PLC_CAP_REG, cap_write.reg);
		IRQ_ENABLE_INTERRUPT();
#ifdef ETH_BRDG_DEBUG
		if (myDebugFlag)
		{
	        printf("PLC_CAP_W = %lx\n",cap_write.reg);
        	printf("PLC_Queue_Descriptor_Status = %lx\n", ReadU32Reg(PLC_QDSTATUS_REG));
		}
#endif
	IRQ_DISABLE_INTERRUPT();
    WriteU32Reg(PLC_QUEUEDATA_REG, txfrmHwDesc.reg);
    WriteU32Reg(PLC_QUEUEWRITE_REG ,1);

    /* Write HP10 FC to the queue */
    WriteU32Reg(PLC_QUEUEDATA_REG, gHpgpHalCB.plcTx10FC.reg);
    WriteU32Reg(PLC_QUEUEWRITE_REG ,1);

    /* Write VOF descriptor 0 to the queue */
    WriteU32Reg(PLC_QUEUEDATA_REG, vofDesc0.reg);
    WriteU32Reg(PLC_QUEUEWRITE_REG ,1);

    /* Write VOF descriptor 1 to the queue */
    WriteU32Reg(PLC_QUEUEDATA_REG, vofDesc1.reg);       
    WriteU32Reg(PLC_QUEUEWRITE_REG ,1);

    /* Write VOF descriptor 2 to the queue */
    WriteU32Reg(PLC_QUEUEDATA_REG, vofDesc2.reg);
    WriteU32Reg(PLC_QUEUEWRITE_REG ,1);

    /* Write VOF descriptor 3 to the queue */
    WriteU32Reg(PLC_QUEUEDATA_REG, vofDesc3.reg);
    WriteU32Reg(PLC_QUEUEWRITE_REG ,1);
	IRQ_ENABLE_INTERRUPT();


//	FM_Printf(FM_USER, "tx\n");
		

    //FM_Printf(FM_MINFO, "TxQWrite:3 %bu\n",txFrmSwDesc->cpCount);

    /* 6. Create CP Descriptors are write one by one */
    for( i=0; i<txFrmSwDesc->cpCount; i++)
    {
        txCpDesc.reg            = 0;
        txCpDesc.plc.cp         = txFrmSwDesc->cpArr[i].cp;
        txCpDesc.plc.descLenLo  = txFrmSwDesc->cpArr[i].len & PKTQDESCCP_DESCLENLO_MASK;
        txCpDesc.plc.descLenHi  = (txFrmSwDesc->cpArr[i].len & PKTQDESCCP_DESCLENHI_MASK)>> PKTQDESCCP_DESCLENHI_POS;

        if( i == 0 )
        {
            txCpDesc.plc.roboMd = roboMode;
        }
        if( i == txFrmSwDesc->cpCount-1 )
        {
            txCpDesc.plc.lastDesc = 1;
        }
        txCpDesc.plc.offset = txFrmSwDesc->cpArr[i].offsetU32;
		IRQ_DISABLE_INTERRUPT();
        /* Write CP descriptor to the queue */
        WriteU32Reg(PLC_QUEUEDATA_REG, txCpDesc.reg);         
        WriteU32Reg(PLC_QUEUEWRITE_REG ,1);
        //FM_Printf(FM_LINFO,"CPn  = %lX\n",rtocl(txCpDesc.reg)); 
		IRQ_ENABLE_INTERRUPT();
    }
    //FM_Printf(FM_MINFO, "TxQWrite:4\n");
#if 0
    printf("Desc1= %lX\n",rtocl(txfrmHwDesc.reg));
    printf("HP10FCDesc = %08lX\n",rtocl(gHpgpHalCB.plcTx10FC.reg));
    printf("VF0  = %lX\n",rtocl(vofDesc0.reg));
    printf("VF1  = %lX\n",rtocl(vofDesc1.reg));
    printf("VF2  = %lX\n",rtocl(vofDesc2.reg));
    printf("VF3  = %lX\n",rtocl(vofDesc3.reg));
#endif	
/*
    FM_Printf(FM_LINFO,"Desc1= %lX\n",rtocl(txfrmHwDesc.reg));
    //FM_Printf(FM_LINFO," HP10FCDesc = %08lX\n",rtocl(gHpgpHalCB.plcTx10FC.reg));
    FM_Printf(FM_LINFO,"VF0  = %lX\n",rtocl(vofDesc0.reg));
    FM_Printf(FM_LINFO,"VF1  = %lX\n",rtocl(vofDesc1.reg));
    FM_Printf(FM_LINFO,"VF2  = %lX\n",rtocl(vofDesc2.reg));
    FM_Printf(FM_LINFO,"VF3  = %lX\n",rtocl(vofDesc3.reg));
*/

//#ifdef Packet_grouping0
//    if (gHpgpHalCB.plcMultiPktTest <= 1)  //[YM] multiple packet grouping is not supported in Hybrii_B
//#endif		
    {

    /* 7. Write PLC Command Queue Write Register to trigger HW Tx */
    txCmdQueueWrtie.reg = 0;
    txCmdQueueWrtie.s.txQ = txfrmHwDesc.sof.cap;
	txCmdQueueWrtie.s.txCap = txfrmHwDesc.sof.cap;
	txCmdQueueWrtie.s.txRobo = roboMode;
	//printf("txCmdQueueWrtie.s.txQ = %bu\n", txCmdQueueWrtie.s.txQ);
	//printf("txCmdQueueWrtie.s.txCap = %bu\n", txCmdQueueWrtie.s.txCap);
	//printf("txCmdQueueWrtie.s.txRobo = %bu\n", txCmdQueueWrtie.s.txRobo);
	IRQ_DISABLE_INTERRUPT();
	WriteU32Reg(PLC_CMDQ_REG, txCmdQueueWrtie.reg);
	IRQ_ENABLE_INTERRUPT();
#ifdef ETH_BRDG_DEBUG
	if (myDebugFlag)
	    printf("txCmdQueueWrtie.reg = %lx\n",txCmdQueueWrtie.reg);
#endif
    }
	
		val_32 = 	ReadU32Reg(0xEF0);
		//printf("REG 0xEF0 = 0X%lx\n", val_32);
		val_32 = 	ReadU32Reg(0xEF4);
		//printf("REG 0xEF4 = 0X%lx\n", val_32);

#ifdef DEBUG_DATAPATH
    
        if (pktDbg || sigDbg)
            FM_Printf(FM_ERROR,"p tx \n");
        
        if (pktDbg)
        {
            for (i=0 ; i<txFrmSwDesc->cpCount ; i++)
            {
                u8 j; 
                u8 byteOffset = (u8)txFrmSwDesc->cpArr[i].offsetU32<< 2;                 
                volatile u8 xdata * cellAddr = CHAL_GetAccessToCP(txFrmSwDesc->cpArr[i].cp);
                for (j=byteOffset; j < (byteOffset+txFrmSwDesc->cpArr[i].len); j++)
                {
                    FM_Printf(FM_ERROR,"0x%02bX ", cellAddr[j]);
                }
                FM_Printf(FM_ERROR,"\n");
            }             
    
            FM_Printf(FM_ERROR,"\n end \n");
        }
    
#endif




    // Trigger Tx if Status is success
    if(status == STATUS_SUCCESS)
    {
    // Recovery for CRS stuck case - if RxEn goes low during early phyActive.
    plcStatus.s.crsBypass = 1;

    // Check Tx window again and  Trigger Tx.
    plcStatus.s.plcTxQRdy = 1;   
/*     
    if (txFrmSwDesc->frmType == HPGP_HW_FRMTYPE_SOUND)
    {
         plcStatus.s.soundEnable = 1;
    }
*/
#ifdef DEBUG_DATAPATH

    if (sigDbg)
        FM_Printf(FM_ERROR,"plc txRdySet \n");
    
#endif

#ifdef _AES_SW_
    plcStatus.s.aesReset  = 0;
#endif
IRQ_DISABLE_INTERRUPT();
    WriteU32Reg(PLC_STATUS_REG, plcStatus.reg);
    plcStatus.s.crsBypass = 0;

    WriteU32Reg(PLC_STATUS_REG, plcStatus.reg);
IRQ_ENABLE_INTERRUPT();
    //gHpgpHalCB.halStats.TotalTxFrmCnt++;
   
    /*******Critical Section Ends **********/

#if PLC_PAYLOAD_DBGPRINT
    for (i=0 ; i<txFrmSwDesc->cpCount ; i++)
    {
        u8 j; 
        u8 byteOffset = (u8)txFrmSwDesc->cpArr[i].offsetU32<< 2;                 
        volatile u8 xdata * cellAddr = CHAL_GetAccessToCP(txFrmSwDesc->cpArr[i].cp);

        FM_Printf(FM_ERROR,"PktBuf%bu :\n", i+1);
        for (j=byteOffset; j < (byteOffset+txFrmSwDesc->cpArr[i].len); j++)
        {
            FM_Printf(FM_ERROR,"0x%02bX ", cellAddr[j]);
        }
        FM_Printf(FM_ERROR,"\n");
    }             
    
    FM_Printf(FM_ERROR,"\n");
#endif
    }
    }
    if(status == STATUS_SUCCESS || status == STATUS_DEFERRED)
    {

#ifdef ETH_BRDG_DEBUG
	 	if(status == STATUS_SUCCESS)
		{
			numTxFrms++;
			if (myDebugFlag)
				printf("PLCTXQWRITE: SUCCESS\n");
		}
#endif
        status = STATUS_SUCCESS;
        gHpgpHalCB.halStats.TotalTxFrmCnt++;
        // update statistics
        gHpgpHalCB.halStats.TotalTxBytesCnt+=txFrmSwDesc->frmLen;
        if(txfrmHwDesc.sof.frmType == HPGP_HW_FRMTYPE_MGMT)
        {
            gHpgpHalCB.halStats.TxMgmtCnt++;     
        }
        else
        {
            gHpgpHalCB.halStats.TxDataCnt++;
        }

    }                           
    //FM_Printf(FM_MINFO, "<<<TxQWrite:\n");	               
    }
    }
    return status;

}


void HHAL_Bcn3SentIntHandler()
{
    gHpgpHalCB.halStats.BcnSentIntCnt++;
    gHpgpHalCB.bBcnNotSent = 0;
}

void HHAL_PlcPendingTx()
{
//    uBcnStatusReg         bcnStatus;
//    uPlcStatusReg         plcStatus;
//    uPlcMedStatReg        plcMedStat;
    //uCpuSwStatusReg       cpuSwStat;  //[YM] redefine software structure
//    uPlcLineControlReg    plcLineCtrl;  

#if 0    
    if(gHpgpHalCB.bTxPending)
    {
        IRQ_DISABLE_INTERRUPT();
        bcnStatus.reg   = ReadU32Reg(PLC_BCNSTATUS_REG);
        //cpuSwStat.reg   = ReadU32Reg(CPU_SWSTATUS_REG);     //[YM] redefine cpuSwStat structure and fields
        plcLineCtrl.reg = ReadU32Reg(PLC_LINECTRL_REG);
        plcStatus.reg   = ReadU32Reg(PLC_STATUS_REG);           

    {
        //ReadSwapU32Reg(&csmaRegAtuCtr, PLC_CSMAREGATUCTR_REG);     //[YM] Relocate register to 0xFDDC-PLC Parameter Write Data Register
        //plcLineCtrl.reg = ReadU32Reg(PLC_LINECTRL_REG);
        //plcStatus.reg   = ReadU32Reg(PLC_STATUS_REG);
        plcMedStat.reg  = ReadU32Reg(PLC_MEDIUMSTATUS_REG); 
#ifdef ETH_BRDG_DEBUG
		if (myDebugFlag)
       		printf("HHAL_PlcPendingTx: phyActive=%bu,plcMacIdle=%bu, crsMac=%bu,plcMacIdle=%bu,crsMac=%bu,txWindow=%bu \n",
				plcMedStat.s.phyActive,plcStatus.s.plcMacIdle,plcMedStat.s.crsMac,plcMedStat.s.crsMac,plcMedStat.s.txWindow);
#endif
        if (!plcMedStat.s.phyActive && plcStatus.s.plcMacIdle && 
            !plcMedStat.s.crsMac && plcMedStat.s.txWindow )
        //    if( !plcMedStat.s.phyActive && plcStatus.s.plcMacIdle && !plcMedStat.s.crsMac && csmaRegAtuCtr > 350 )
        {   
#ifdef ETH_BRDG_DEBUG
			if (myDebugFlag)
               		printf("HHAL_PlcPendingTx: Forcing recovery...\n");
#endif
            plcStatus.s.nRxEn = 1;
#ifdef _AES_SW_
            plcStatus.s.aesReset  = 1;
#endif
            WriteU32Reg(PLC_STATUS_REG, plcStatus.reg);


            // Recovery for CRS stuck case - if RxEn goes low during early phyActive.
            plcStatus.s.crsBypass = 1;
        
            // Trigger Tx.
            plcStatus.s.plcTxQRdy = 1;

#ifdef DEBUG_DATAPATH

            if (sigDbg)
                FM_Printf(FM_ERROR,"plc txRdySet \n");

#endif

#ifdef _AES_SW_
            plcStatus.s.aesReset  = 0;
#endif
            WriteU32Reg(PLC_STATUS_REG, plcStatus.reg);
            plcStatus.s.crsBypass = 0;
        
            WriteU32Reg(PLC_STATUS_REG, plcStatus.reg);

            gHpgpHalCB.bTxPending = 0;


        }
    }
    IRQ_ENABLE_INTERRUPT();
    }
#endif
}

#ifdef Z_P_BRIDGE
void hpgp_pkt_bridge (sCommonRxFrmSwDesc* pRxFrmDesc, u8 frmLen)
{
    sSwFrmDesc   plcTxFrmSwDesc;
    sHpgpHalCB*    hhalCb;
    u8             i;
    u16            tmpFrmLen;
    u8             tmpdescLen;
    u8             wrapBit;
    u8             indexHd;

    hhalCb = &gHpgpHalCB;

    memset(&plcTxFrmSwDesc, 0x00, sizeof(sSwFrmDesc));

    if (datapath_IsQueueFull(PLC_DATA_QUEUE) 
			== TRUE)
	{
		/*
		* Drop the frame if Q is full
		*/
			
            hal_common_free_frame(pRxFrmDesc);
            return;
        
    }

    if(frmLen < HYBRII_CELLBUF_SIZE) {
        tmpdescLen = frmLen;
    } else {
        tmpdescLen = HYBRII_CELLBUF_SIZE;
    }
    tmpFrmLen = tmpdescLen;

    plcTxFrmSwDesc.cpArr[0].offsetU32         = 0;
    plcTxFrmSwDesc.cpArr[0].len               = tmpdescLen;
    plcTxFrmSwDesc.cpArr[0].cp                = pRxFrmDesc->cpArr[0];
    plcTxFrmSwDesc.cpCount                    = 1;

    plcTxFrmSwDesc.frmInfo.plc.eks            = HPGP_UNENCRYPTED_EKS;
    plcTxFrmSwDesc.frmInfo.plc.bcnDetectFlag  = REG_FLAG_SET;
    plcTxFrmSwDesc.frmInfo.plc.clst           = HPGP_CONVLYRSAPTYPE_RSV;

    plcTxFrmSwDesc.frmType                    = HPGP_HW_FRMTYPE_MSDU;
    plcTxFrmSwDesc.frmInfo.plc.dtei           = hhalCb->remoteTei;
    plcTxFrmSwDesc.frmInfo.plc.stei           = hhalCb->selfTei;
    plcTxFrmSwDesc.frmLen                     = frmLen;

    plcTxFrmSwDesc.frmInfo.plc.plid           = 0;
    plcTxFrmSwDesc.frmInfo.plc.phyPendBlks    = HPGP_PPB_CAP0;
    plcTxFrmSwDesc.frmInfo.plc.mcstMode       = HPGP_UCST; 
    plcTxFrmSwDesc.frmInfo.plc.stdModeSel     = 1;

    plcTxFrmSwDesc.frmInfo.plc.dt_av          = HPGP_DTAV_SOF;
    plcTxFrmSwDesc.frmInfo.plc.saf            = 1;
    plcTxFrmSwDesc.frmInfo.plc.clst           = 1;
    plcTxFrmSwDesc.frmInfo.plc.status         = 0;

    for (i = 1 ; i < pRxFrmDesc->cpCount ; i++) {
        if((frmLen - tmpFrmLen) > HYBRII_CELLBUF_SIZE) {
            tmpdescLen = HYBRII_CELLBUF_SIZE;
        } else {
            tmpdescLen = frmLen - tmpFrmLen;
        }
        plcTxFrmSwDesc.cpArr[i].offsetU32 = 0;
        plcTxFrmSwDesc.cpArr[i].len       = (u8)tmpdescLen;
        plcTxFrmSwDesc.cpArr[i].cp        = pRxFrmDesc->cpArr[i];
        plcTxFrmSwDesc.cpCount++;
        tmpFrmLen += tmpdescLen;
    }

    //HHAL_PlcPendingTx();

    /*
     		* Queue the packet for later TX
       */

	plcTxFrmSwDesc.txPort = pRxFrmDesc->hdrDesc.s.dstPort;
	plcTxFrmSwDesc.rxPort = pRxFrmDesc->hdrDesc.s.srcPort;

	fwdAgent_handleData(&plcTxFrmSwDesc);
}
#endif


void HHAL_BPIntHandler()
{
	
	uPlcMedStatReg  plcMedStat;   
    uPlcStatusReg   plcStatus;
	static u32 prevBPInt_TimerCnt;
    static u32 curBPInt_TimerCnt;
    static u8  bpInit;
    

	if(gHpgpHalCB.bPerAvgInitDone && gHpgpHalCB.bcnInitDone)
	{

#ifdef MISS_BCN
	    gbcnsent++;
	    if((gbcnsent == 5) || (gbcnsent == 6) || (gbcnsent == 7) || (gbcnsent == 8) || (gbcnsent == 9) || (gbcnsent == 10))// || (gbcnsent == 11))// || (gbcnsent == 10))
	    {
	        return;
	    }
	     if(gbcnsent == 11)
	     {
	         gbcnsent = 0;
	     } 
#endif
		    // Determine time gap between 2  consecutive HPGP BP interrupts.
	    if( gHpgpHalCB.halStats.bpIntCnt )
	    {
	          bpInit = 1;
	    } 
	    if( bpInit )
	    {
	        prevBPInt_TimerCnt = curBPInt_TimerCnt;
	    }
	    curBPInt_TimerCnt   = gHalCB.timerIntCnt;
	    gHpgpHalCB.bpIntGap = curBPInt_TimerCnt - prevBPInt_TimerCnt; 
		gHpgpHalCB.halStats.bpIntCnt++;
	    // Compute running average of prev 64 ZeroCrossing periods
	     // and write to Bcn Period Average register.

	   
	    // Prepare and send beacon here for now.
	    // This will eventually be done by hpgp nsm module.
	    //HHT_BPIntHandler();
	    if( ( (gHpgpHalCB.devMode == DEV_MODE_CCO)
#ifdef MCCO			
			||(gHpgpHalCB.devMode == DEV_MODE_PCCO)

#endif
			)&&
			 gHpgpHalCB.bcnInitDone )
		{
#ifdef HPGP_HAL_TEST
	        HHT_SendBcn(BEACON_TYPE_CENTRAL);
#else
	        if(gHpgpHalCB.lmBcn.txBitmap & (1 << BEACON_TYPE_CENTRAL))
#endif
	        {
	            HHAL_PlcBcnWrite(gHpgpHalCB.lmBcn.cBcn.bcnBuff, BEACON_TYPE_CENTRAL,
	                         gHpgpHalCB.lmBcn.cBcn.bpstoOffset);
	        }
		}

	    // PHY Active Hang workaround
	    plcStatus.reg  = ReadU32Reg(PLC_STATUS_REG);
	    plcMedStat.reg = ReadU32Reg(PLC_MEDIUMSTATUS_REG); 
	    if( plcMedStat.s.phyActive && plcMedStat.s.mpiRxEn )
	    {
	        if(gHpgpHalCB.halStats.paRxEnHiCnt > PLC_RXPHYACT_HANG_THRES )
	        {
#ifdef _RX_RECOVERY_                        
	            plcStatus.s.nRxEn = 1;
	            WriteU32Reg(PLC_STATUS_REG, plcStatus.reg);
	            CHAL_DelayTicks(5);
	            
	            plcStatus.s.plcRxEnSwCtrl  = 0;
	            WriteU32Reg(PLC_STATUS_REG, plcStatus.reg);

	            plcStatus.s.plcRxEnSwCtrl  = 1;
	            WriteU32Reg(PLC_STATUS_REG, plcStatus.reg); 
#endif             
	            gHpgpHalCB.halStats.paRxEnHiCnt = 0;   
	            gHpgpHalCB.halStats.phyActHangRstCnt++;                    
	        }
	        else if( gHpgpHalCB.halStats.prevBPTotalRxCnt == gHpgpHalCB.halStats.TotalRxGoodFrmCnt )
	        {
	            gHpgpHalCB.halStats.paRxEnHiCnt ++;
	        } 
	        else
	        {
	            gHpgpHalCB.halStats.paRxEnHiCnt = 0;  
	        }                
	    }
	    gHpgpHalCB.halStats.prevBPTotalRxCnt = gHpgpHalCB.halStats.TotalRxGoodFrmCnt;
#ifndef HPGP_HAL_TEST
#ifdef CCO_FUNC
	    LINKL_UpdateBeacon();
#endif	  
#endif


		}
}
