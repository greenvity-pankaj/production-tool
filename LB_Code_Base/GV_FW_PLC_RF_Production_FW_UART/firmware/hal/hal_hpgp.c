/*
* $Id: hal_hpgp.c,v 1.73 2015/01/02 14:55:35 kiran Exp $
*
* $Source: /home/cvsrepo/Hybrii_B_OSFW_Dev/firmware/hal/hal_hpgp.c,v $
*
* Description : HPGP Hardware Abstraction Layer implementation
*
* Copyright (c) 2010-2013 Greenvity Communications, Inc.
* All rights reserved.
*
* Purpose :
*     Defines API interface for accessing HPGP memory mapped registers.
*                           for writing tx descritpors to plc tx queues
*                           for writing beacons to beacon tx fifos
*                           for reading beacons from beacon rx fifos
*/


#include <stdio.h>
#include <string.h>
#include <intrins.h>
#include "papdef.h"
#ifdef ROUTE
#include "hpgp_route.h"
#endif
#include "hal_common.h"
#include "hal.h"
#include "fm.h"
#include "hal_hpgp.h"
#include "hal_eth.h"
#include "hal_tst.h"
#include "hal_reg.h"
#include "hpgpevt.h"
#include "timer.h"
#include "stm.h"
#include "hybrii_tasks.h"
#include "nma.h"
#include "nma_fw.h"
#ifndef HPGP_HAL_TEST
#include "hpgpapi.h"
#endif	//HPGP_HAL_TEST
#include "gv701x_gpiodriver.h"
#include "fm.h"
#ifdef HPGP_HAL_TEST
#include "hal_cfg.h"
#else
#include "hal.h"
#endif	//HPGP_HAL_TEST
#include "hal_regs_def.h"
#include "uart.h"
#ifdef NO_HOST
#include "gv701x_flash.h"
#endif

#ifdef HPGP_MAC_SAP 
#ifdef LINK_STATUS
#define MAX_LINK_TEST_TX_TIMEOUT 450
#define LINK_RETRY            1
#endif	//LINK_STATUS
#endif	//HPGP_MAC_SAP

#ifdef FREQ_DETECT
    u32 AC_MIN_THRESHOLD;
    u32 PLC_DC_LINE_CYCLE_FREQENCY  =  0x7A120;
#endif
u32 gBPSTdelta;
u8 gNegativeflag = 0;
u8 gPositiveflag = 0;
u8 firsttime = 0;
u32 goldbpst;
u32 gavg;
u8 zctrack = 0;
u8 zcFlag;
u32 gbpst = 0;
u8 gRollOver;
u32 zcCCONTBold;
u32 gCCO_BTS;
u32 zcCCONTB_OLD;

u8 avgdone = 0;
u32 avg;
u8 avgcount =0;

//[YM] Define for LED relay control number
#define Light_4_Relay = 4;
#define Light_6_Relay = 6;

extern u32 misscnt;

void sendSingleFrame(u8 mcstMode);  //1 = multicast, 0 unicast
extern void datapath_init();

extern u16 var1;
//u32 missarr[1000];
u8 testflag = 1;

extern u8 opMode;
#ifdef PLC_TEST
u8 gSTA_TEI;
u8 gCount;
u8 gNumOfSTAAssignedTEI = 1;
u8 gCCOTest = 0;
extern u16 gBcnSent;
eHpgpRoboMod gRoboMode;
eHpgpRoboModLens gCurrRobomode;
#define TOTAL_NUM_OF_TX_FRAME  1000

  u16 gRobomodeAllLenTest[5][2]= {

    {HPGP_ROBOMD_MINI_100, 100},   
    {HPGP_ROBOMD_MINI_250, 250}, 
    //{HPGP_ROBOMD_HS_800, 800}, 
   // {HPGP_ROBOMD_HS_1000, 1000},
    {HPGP_ROBOMD_STD_500, 500}, 
    {HPGP_ROBOMD_HS_800, 800}, 
    {HPGP_ROBOMD_HS_1000, 1000}, 


};   
#endif
#if defined(HPGP_HAL_TEST) || defined(PLC_TEST) 

u16 gAltRoboLenArr[]={101,401,801,1201};
u8 gAltEksTstArr[] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 15}; // 7 NEKs, 2 PPEKs, Unenc
#endif

#ifdef FREQ_DETECT
u32 PLC_DC_BP_LEN     =           PLC_DC_BP_LEN_60HZ;
u32 PLC_AC_BP_LEN     =           PLC_AC_BP_LEN_60HZ;//0xCB735      //these are for 60 hz AC
u32 PLC_MIN_AC_BPLEN  =           0xCB300;
u32 PLC_MAX_AC_BPLEN  =           0xCBA00; 
#endif

u32 cnt5 = 0;
u32 oldRetrievedBTS;
u32 oldss1;
u32 OldCCOBpst;

u32 gBcnPer;
extern u32 gtimer2, gtimer1;
u8 gsyncTimeout;
u8 gBcnMissingRescanCnt;
extern u8 gflag;
sHpgpHalCB gHpgpHalCB;
#ifdef HPGP_HAL_TEST
extern sHalCB gHalCB;
sHpgpHalCB *gpHhalCb;
#endif	//HPGP_HAL_TEST

#ifdef DEBUG_DATAPATH
extern u8 sigDbg;
#endif

u32 gbpst1;
u32 gOldBTS;
#ifdef ETH_BRDG_DEBUG
extern u32 numTxDoneInts;
extern u8 myDebugFlag;
extern u8 myDebugFlag1;
#endif

/*u32 TX_RXLatency_TCC[8] = {
                   
                    0x321e0417,  //TCC =1
                   // 0x336F0417,
                    0x336F04E5, //this is according to new phy image gpphy_3S150_08032013- tx latency increased to 50microsec from 41.8 microsec
                    0x34ca0417,
                    0x361F0417,
                    0x37750417,
                    0x38c20417,
                    0x3a180417,
                    0x3b740417,  //TCC= 8

};*/

//phy image gpphy_3S150_08032013- and onwards uses phy tx latency as 0x04e5 and older phy image uses phy tx latency as 0x0417 
#ifdef B_ASICPLC
//following is for ASIC
u32 TX_RXLatency_TCC[8] = {
                0x30C104DD,   //TCC =1
                0x319304DD,
                0x325504DD,  //TCC =3   326204dd  3255
                0x332604DD,
                0x33F304DD,
                0x34C004DD,
                0x358C04DD,
                0x365A04DD, //TCC= 8

};
#else
u32 TX_RXLatency_TCC[8] = {
                   
                    0x321e04E5,  //TCC =1
                   // 0x336F0417,   old phy tx latency 0x0417
                    0x336F04E5, //this is according to new phy image gpphy_3S150_08032013- tx latency increased to 50microsec from 41.8 microsec
                    0x34ca04E5,
                    0x361F04E5,
                    0x377504E5,
                    0x38c204E5,
                    0x3a1804E5,
                    0x3b7404E5,  //TCC= 8

};
#endif
u8 TCC_REG_485_486_val[8][2] = {
                                {0x40,0x80},
                                {0x80,0x80},
                                {0xc0,0x80},
                                {0x0,0x81},
                                {0x40,0x81},
                                {0x80,0x81},
                                {0xc0,0x81},
                                {0x0,0x82},

};
u8 gDefNID[NID_LEN] = {0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77};
#define PLC_MaxPeran                    0x1B
#define PLC_MinPeran                    0x1C
u16  prevCsmaRgns[HYBRII_MAXSMAREGION_CNT] = {0};
#ifdef CCO_FUNC
#ifdef FREQ_DETECT
#ifdef UM
extern eStatus CNSM_InitRegion(sCnsm *cnsm);
#endif
#endif
#endif

#ifdef PLC_TEST
void printmsg(u8* buff, u8 len)
{
   u8 i;
   for(i = 0; i < len; i++)
   {
       printf("%bx\t", buff[i]);
   }
   printf("\n");
}

void HHT_ProcessPlcFrame(sSwFrmDesc* plcRxFrmSwDesc)
{
    
    u8 HeaderOffset = sizeof(gHeaderBytes);
    u8 TestID;
    u8 NumOFSTAAssignTEI;
    u8 offset;
    volatile u8  RxDataBuff[500];
    volatile u8 TxdataBuff[500];
    u8 STA_TEI;
    u8 i;
    u16 totalDesc ;
    u16 freeDescCnt ;
    u32 per;
    
                        
    
    sHpgpHalCB HpgpHalCB;
    plcHalStatus plc_halstatus_t;
    volatile u8 xdata * cellAddr;
    
    //printf("\n cpcounts : %bu\n", plcRxFrmSwDesc->cpCnt);
  
  

    //  /?! frames should not be considered in statistic so doing minus 1
    //gHpgpHalCB.halStats.TotalRxGoodFrmCnt--;
    //gHpgpHalCB.halStats.RxGoodDataCnt--;
    
     
     //WE assume that received data will never be more then 500 bytes since rxdabuff size is 500  and txdatabuff is also 500
    
    cellAddr = CHAL_GetAccessToCP(plcRxFrmSwDesc->cpArr[0].cp);
    TestID = cellAddr[HeaderOffset + 0];//because first three bytes will be /, ?, !       HeaderOffset = 3
    strncpy(&TxdataBuff[0],&gHeaderBytes[0], sizeof(gHeaderBytes) );
    
    
    for(i = 0; i < plcRxFrmSwDesc->cpCount; i++ )
    {
         cellAddr = CHAL_GetAccessToCP(plcRxFrmSwDesc->cpArr[i].cp);
         
         if(i == 0)//if first cp then we have /?! and test ID
         {
             offset = HeaderOffset + 1;//since we have stored 
         }
         else
             offset = 0;
         
         
         memcpy(&RxDataBuff[i* HYBRII_CELLBUF_SIZE] ,&cellAddr[offset], HYBRII_CELLBUF_SIZE);
    }
    
 
    
   // if(plcRxFrmSwDesc->frmInfo.plc.stei == DEFAULT_CCO_TEI )//source is CCO so STA is Rx
    {
        
        //printf("\nSTA : \n");
        switch(TestID)
        {
            case  BROADCAST_CCO_TEI_TESTID:
            
			//send STA TEI to CCO
		//	printf("BROADCAST_CCO_TEI_TESTID\n");
                NumOFSTAAssignTEI = RxDataBuff[0];
                STA_TEI = (NumOFSTAAssignTEI + 1);
                TxdataBuff[HeaderOffset] = ASSIGN_STA_TEI_TESTID;
                TxdataBuff[HeaderOffset + 1] = STA_TEI;
                
                //printf("\n Rx:\n");
                //printmsg(&cellAddr[0],5);
                
                //printf("\n Tx:\n");
                //printmsg((u8*)&TxdataBuff[0],5);
                
                Send_SinglePLCFrame(5, &TxdataBuff[0], STA_TEI, DEFAULT_CCO_TEI);//this frame is sent from sta to cco saying that tei is assign to sta
                break;
            
            case ACK_FOR_STA_TEI_TESTID://3
                //set robo mode to mini robo
                //send start PLC data rate test ID
                
             //    printf("\n ACK_FOR_STA_TEI_TESTID\n");
                 gSTA_TEI = RxDataBuff[0];
                 HHAL_SetTei(gSTA_TEI);
                 gHpgpHalCB.remoteTei = DEFAULT_CCO_TEI;
                 gHpgpHalCB.selfTei = gSTA_TEI;
                 TxdataBuff[HeaderOffset] = START_PLC_DATA_RATE_TESTID;
                 //printf("\n Tx:\n"); 
                 //printmsg(&TxdataBuff[0],4);
                 
                 CHAL_DelayTicks(400);
                 Send_SinglePLCFrame(4, &TxdataBuff[0], gSTA_TEI, DEFAULT_CCO_TEI);   //this frmae is sent from cco to sta saying start test to test hardware
                 HHAL_ResetPlcStat();
                
                
            break;
 
            case DATARATE_TEST_TESTID://5 
             //   printf("DATARATE_TEST_TESTID\n");
                //send pstat 
                //after send clear the p stat
                //printf("\nHeaderOffset = %bu\n", HeaderOffset);
                
                // printf("\n cpcounts : %bu\n", plcRxFrmSwDesc->cpCnt);
                //printf("\n>>>nSTA received ROBO mode frame.\n");
                TxdataBuff[HeaderOffset] = PSTAT_TESTID;
                memcpy(&TxdataBuff[HeaderOffset + 1],(char*)&gHpgpHalCB.halStats,sizeof(gHpgpHalCB.halStats));
                

              

                totalDesc      = PLC_TXQ_DEPTH + PLC_TXQ_DEPTH + PLC_TXQ_DEPTH + PLC_TXQ_DEPTH;
                freeDescCnt    =  (u16)(HHAL_GetPlcTxQFreeDescCnt(0) + HHAL_GetPlcTxQFreeDescCnt(1) + \
                                     HHAL_GetPlcTxQFreeDescCnt(2) + HHAL_GetPlcTxQFreeDescCnt(3));  

                 

                plc_halstatus_t.AddrFilterErrCnt = hal_common_reg_32_read(PLC_ADDRFILTERERRCNT_REG);
                plc_halstatus_t.FrameCtrlErrCnt = hal_common_reg_32_read(PLC_FCCSERRCNT_REG);
                plc_halstatus_t.ICVErrCnt = hal_common_reg_32_read(PLC_ICVERRCNT_REG);
                plc_halstatus_t.PBCSRxErrCnt = hal_common_reg_32_read(PLC_PBCSRXERRCNT_REG);
                plc_halstatus_t.PBCSTxErrCnt = hal_common_reg_32_read(PLC_PBCSTXERRCNT_REG);
                plc_halstatus_t.PLCMpduDropCnt = hal_common_reg_32_read(PLC_MPDUDROPCNT_REG);
                plc_halstatus_t.outStandingDescCnt = totalDesc - freeDescCnt;
                plc_halstatus_t.FreeCPcount = CHAL_GetFreeCPCnt();
                plc_halstatus_t.timerIntCnt = gHalCB.timerIntCnt;
                plc_halstatus_t.bpIntGap = gHpgpHalCB.bpIntGap;
                plc_halstatus_t.lastNtbB4 = gHpgpHalCB.lastNtbB4;
                plc_halstatus_t.lastNtbAft = gHpgpHalCB.lastNtbAft;
                plc_halstatus_t.lastBpst = gHpgpHalCB.lastBpst;
                
                
                memcpy(&TxdataBuff[HeaderOffset + 1 + sizeof(gHpgpHalCB.halStats)],(char*)&plc_halstatus_t,sizeof(plc_halstatus_t)); 
                
       
                
                
                //printmsg(&TxdataBuff[0],(sizeof(gHpgpHalCB.halStats) + 4 + sizeof(plc_halstatus_t))); 
                //clear the stat
                HHAL_ResetPlcStat();
               // printf("\nsize = %x\n", (sizeof(gHpgpHalCB.halStats) + 4 + sizeof(plc_halstatus_t)));
                
                //printf("\n >>>STA transmitting its Statistic to CCO...\n");
                Send_SinglePLCFrame((sizeof(gHpgpHalCB.halStats) + 4 + sizeof(plc_halstatus_t)), &TxdataBuff[0], gSTA_TEI, DEFAULT_CCO_TEI);
                
                
            break;
            
            /*case SEND_SYNC_DATA:
                //compare and display
                //send pxmittest dataID DATA_DURING_BCNS_TESTID
                printf("\nRxGoodBcnCnt = %lu", gHpgpHalCB.halStats.RxGoodBcnCnt);
                if(gHpgpHalCB.halStats.RxGoodBcnCnt >= NUM_OF_BCNS_FOR_CCO_TEST)
                {
                    eth_plc_bridge = 1;
                    printf("\n\t\t\t\t\t BEACONS RECEIVED SUCCESSFULLY\n");
                    printf("\n>>>STA Transmitting MINI_ROBO frames to CCO...\n\n");
                    sendRobomodeFrames(100,1000);
                    
                     
                    TxdataBuff[3] = SEND_SYNC_DATA;
                    printmsg(&TxdataBuff[0],4);
                    Send_SinglePLCFrame(4, &TxdataBuff[0], gSTA_TEI, DEFAULT_CCO_TEI);//broadcast CCO TEI

                }
                
            break;
            
            case ENABLE_BRIDGE:
                break;
            
          */

                
                
        }
    } 
  //  else//CCO is Rx
    {
        //printf("\nCCo : \n");
        switch(TestID)
        {
            case ASSIGN_STA_TEI_TESTID://2
                
                //send Ack for STA TEI to STA
              //  printf("ASSIGN_STA_TEI_TESTID\n");
                HHAL_SetTei(DEFAULT_CCO_TEI);
                gHpgpHalCB.selfTei = DEFAULT_CCO_TEI;
                gSTA_TEI = RxDataBuff[0];
                gRoboMode = HPGP_ROBOMD_MINI;
                gHpgpHalCB.remoteTei = gSTA_TEI;
                TxdataBuff[HeaderOffset] =  ACK_FOR_STA_TEI_TESTID;
                TxdataBuff[HeaderOffset + 1]  = gSTA_TEI;
                Send_SinglePLCFrame(5, &TxdataBuff[0],DEFAULT_CCO_TEI, gSTA_TEI);
                
                
            break;
            case START_PLC_DATA_RATE_TESTID://4  tx pxmitttest data    //this is rec from sta that cco has to start test by sending 1000 minirobo frames followed by std robo and hs mode
              //  printf("START_PLC_DATA_RATE_TESTID\n");
              label1:  
              HHAL_ResetPlcStat();
              gRoboMode = gRobomodeAllLenTest[gCount][0];
              sendRobomodeFrames(gRobomodeAllLenTest[gCount][1],TOTAL_NUM_OF_TX_FRAME);
              
              switch(gRoboMode)
              {
                case HPGP_ROBOMD_MINI_100:
                     // printf("\n>>>CCO Transmitting MINI_ROBO frames to STA...\n");
                      gCurrRobomode = HPGP_ROBOMD_MINI_100;
                break;
                case  HPGP_ROBOMD_MINI_250:
                     // printf("\n>>>CCO Transmitting STD_ROBO frames to STA...\n");
                      gCurrRobomode = HPGP_ROBOMD_MINI_250;
                break;
                case HPGP_ROBOMD_STD_500:
                    // printf("\n>>>CCO Transmitting STD_ROBO frames to STA...\n");  
                     gCurrRobomode = HPGP_ROBOMD_STD_500;
                break;
               
                case  HPGP_ROBOMD_HS_800:
                     //printf("\n>>>CCO Transmitting HS_ROBO frames to STA...\n");   
                     gCurrRobomode = HPGP_ROBOMD_HS_800;
                break;
                case HPGP_ROBOMD_HS_1000:
                     //printf("\n>>>CCO Transmitting HS_ROBO frames to STA...\n");   
                     gCurrRobomode = HPGP_ROBOMD_HS_1000;
                     //gCount = 0;
                break;
                 
              
            }
            strncpy(&TxdataBuff[0],&gHeaderBytes[0], sizeof(gHeaderBytes) );
            HeaderOffset = 3; 
            TxdataBuff[HeaderOffset] =  DATARATE_TEST_TESTID;
            //printmsg(&TxdataBuff[0], 4);
            Send_SinglePLCFrame(4, &TxdataBuff[0],DEFAULT_CCO_TEI, gSTA_TEI);
          
                
                
                
            break;
            case PSTAT_TESTID://6 compare and display it rec all robo mode pstat
                
                //compare and display 
                
              //  printmsg(&RxDataBuff[0], 20); 
                //printf("\n>>>CCO Received Statistcs from STA");
                memcpy((char*)&HpgpHalCB.halStats, &RxDataBuff[0], sizeof(HpgpHalCB.halStats));
                memcpy((char*)&plc_halstatus_t, &RxDataBuff[(sizeof(gHpgpHalCB.halStats) + 4)], sizeof(plc_halstatus_t)); 
               // printf("\nSTA Statistic\n\n\n");
                //HHAL_DisplayPlcStatFromRAM(&HpgpHalCB.halStats, &plc_halstatus_t);
             
                
               // if(HpgpHalCB.halStats.RxGoodDataCnt ==  gHpgpHalCB.halStats.TxDataCnt)
                //{
                   
                    printf("\n\t\t\t\t\t RxGoodDataCnt = %lu\n", HpgpHalCB.halStats.RxGoodDataCnt);
                     per = TOTAL_NUM_OF_TX_FRAME - HpgpHalCB.halStats.RxGoodDataCnt;
                    
                     per = ((per * 100)/TOTAL_NUM_OF_TX_FRAME);
                   
                     /*if(per <= 1)
                        printf("\n\t\t\t\t\tPER is Less athen 1%");
                     else
                        printf("\nPER :  %u",per);
                                                    */
                    
                    if(gCurrRobomode == HPGP_ROBOMD_MINI_100 )
                    {
                        printf("\n\t\t\t\t\t MINI ROBO MODE PER = %lu",per);
                    }
                   
                    else  if((gCurrRobomode == HPGP_ROBOMD_MINI_250) || (gCurrRobomode == HPGP_ROBOMD_STD_500) )
                    {
                        printf("\n\t\t\t\t\t STD ROBO MODE PER = %lu",per);
                    }
                    else if((gCurrRobomode == HPGP_ROBOMD_HS_800) || (gCurrRobomode == HPGP_ROBOMD_HS_1000) )
                    {
                        printf("\n\t\t\t\t\t HS PB ROBO MODE PER = %lu",per);
                       
                    }
                     
                     printf("%c\n\n",'%');
                     printf("-----------------------------------------------------------------\n\n");
                    // printf("\nFrmLen = %d\n\n ", gRobomodeAllLenTest[gCount++][1]);
                    
                     HHAL_ResetPlcStat();
                     
              //  }

                gCount++;
                if(gCount == 5)   //since this is last frame test
                {
                    gCount = 0;
                    //oldssn = 0;
                    printf("\n\t\t\t\t TEST FINISHED, PRESS ENTER");
                    break;
                    
                    
                 }
               goto label1; 
                break;
              
        }       //end of switch
    }//end of else
    
   
    for( i=0 ; i< plcRxFrmSwDesc->cpCount ; i++ )
    {
            CHAL_DecrementReleaseCPCnt(plcRxFrmSwDesc->cpArr[i].cp);
    }
    
}  
#endif

#if defined(HPGP_HAL_TEST) || defined(PLC_TEST) 

void HHT_GetMinMaxLen ( sPlcSimTxTestParams* pTestParams, u8* stdModeSel, u8* minFrmLen, u8* maxFrmLen )
{
    if(pTestParams->lenTestMode == INC_LEN_SINGLE_ROBO)
    {
        if(pTestParams->roboTestMode == MINI_ROBO_TEST)
        {
            *minFrmLen =  1 ;
            *maxFrmLen = pTestParams->frmType?HYBRII_MINIROBO_DATALEN_MAX:HYBRII_MINIROBO_MGMTLEN_MAX;
        } 
        else if(pTestParams->roboTestMode == STD_ROBO_TEST) 
        {
            *minFrmLen = pTestParams->frmType ? (HYBRII_MINIROBO_DATALEN_MAX+1):(HYBRII_MINIROBO_MGMTLEN_MAX+1);
            *maxFrmLen = pTestParams->frmType ? (HYBRII_STD1PBHSROBO_DATALEN_MAX):(HYBRII_STD1PBHSROBO_MGMTLEN_MAX);
            *stdModeSel = 1;
        } 
        else if(pTestParams->roboTestMode == HS1PB_ROBO_TEST)
        {
            *minFrmLen = pTestParams->frmType ? (HYBRII_MINIROBO_DATALEN_MAX+1):(HYBRII_MINIROBO_MGMTLEN_MAX+1);
            *maxFrmLen = pTestParams->frmType ?( HYBRII_STD1PBHSROBO_DATALEN_MAX):(HYBRII_STD1PBHSROBO_MGMTLEN_MAX);
            *stdModeSel = 0;
        }
        else if(pTestParams->roboTestMode == HS2PB_ROBO_TEST)
        {
            *minFrmLen = pTestParams->frmType ? (HYBRII_STD1PBHSROBO_DATALEN_MAX+1):(HYBRII_STD1PBHSROBO_MGMTLEN_MAX+1);
            *maxFrmLen = pTestParams->frmType ? (HYBRII_2PBHSROBO_DATALEN_MAX):(HYBRII_2PBHSROBO_MGMTLEN_MAX);
        }
        else if(pTestParams->roboTestMode == HS3PB_ROBO_TEST)
        {
            *minFrmLen = pTestParams->frmType ? (HYBRII_2PBHSROBO_DATALEN_MAX+1):(HYBRII_2PBHSROBO_MGMTLEN_MAX+1);
            *maxFrmLen = pTestParams->frmType ? (HYBRII_3PBHSROBO_DATALEN_MAX):(HYBRII_3PBHSROBO_MGMTLEN_MAX);
        }
        else if(pTestParams->roboTestMode == HSALLPB_ROBO_TEST)
        {
            *minFrmLen = pTestParams->frmType ? (HYBRII_MINIROBO_DATALEN_MAX+1):(HYBRII_MINIROBO_MGMTLEN_MAX+1);
            *maxFrmLen = pTestParams->frmType ? (HYBRII_3PBHSROBO_DATALEN_MAX):(HYBRII_3PBHSROBO_MGMTLEN_MAX);
        }
    }
    else if(pTestParams->lenTestMode == INC_LEN_ALL_ROBO)
    {
        *stdModeSel = 1;
        *minFrmLen  = 1;
        *maxFrmLen  =  pTestParams->frmType ? (HYBRII_3PBHSROBO_DATALEN_MAX):(HYBRII_3PBHSROBO_MGMTLEN_MAX);
    }
    else if(pTestParams->lenTestMode == FIXED_LEN_ALT_ROBO)
    {
        *stdModeSel = 1;
        *minFrmLen  = gAltRoboLenArr[0];
    }
}

void HHT_SimulateTx(sPlcSimTxTestParams* pTestParams)
{
//  sPlcTxFrmSwDesc plcTxFrmSwDesc;
//#ifdef Packet_grouping
    u32             Sw_command_queue[8];
    u8              cmd_num;
//#endif	
//    uPlcTxPktQCAP_Write   cap_write;
//    uTxCMDQueueWrite      txCmdQueueWrtie;
    sTxFrmSwDesc    plcTxFrmSwDesc;
    u8              stdModeSel;
    u16             stdRoboFrmLenMax;
    u8             	minFrmLen;
    u8             	maxFrmLen;
    u16             curFrmLen;     
    eFrmMcstMode    mcstMode;
    eHpgpHwFrmType  frmType;
    // mixed mode variables
    u16             tmpFrmLen;
    u8              curOffsetDW;
    u8              curDescLen;
    u8              eksArrIdx;
    uAltPlid        altPlid;
    uAltRoboLenIdx  altRoboLenIdx;        
    eStatus         status;
    u8              i, c;
    u8              j;
    u8              quit; 
    u16             tmpPayloadLen;
	u8 				cp_localBuf[HYBRII_CELLBUF_SIZE];	// local CP buffer
    u8              dbc_pktchange;
#ifndef HPGP_HAL_TEST
    sHaLayer        *hal = HOMEPLUG_GetHal();
#endif

	dbc_pktchange     = 0;
    quit              = 0;
    altPlid.val       = 0;
    altRoboLenIdx.val = 0;
    eksArrIdx         = 0;
//#ifdef Packet_grouping0	
	cmd_num           = 0;     
//#endif

    memset((u8*)&plcTxFrmSwDesc, 0x00, sizeof(plcTxFrmSwDesc));

    plcTxFrmSwDesc.frmInfo.plc.eks            = pTestParams->eks;       
    plcTxFrmSwDesc.frmInfo.plc.bcnDetectFlag  = REG_FLAG_SET;// REG_FLAG_CLR;
    plcTxFrmSwDesc.frmInfo.plc.clst = HPGP_CONVLYRSAPTYPE_RSV;//HPGP_CONVLYRSAPTYPE_ETH; 
    plcTxFrmSwDesc.frmInfo.plc.snid = HYBRII_DEFAULT_SNID;

    if(pTestParams->frmType == 0)     
    {
            plcTxFrmSwDesc.frmType = HPGP_HW_FRMTYPE_MGMT;
            frmType                = HPGP_HW_FRMTYPE_MGMT;
    }
    else if(pTestParams->frmType == 1)
    {
        plcTxFrmSwDesc.frmType        = HPGP_HW_FRMTYPE_MSDU;
        frmType                       = HPGP_HW_FRMTYPE_MSDU;
    }
    else if (pTestParams->frmType == 2) // frmType = 2
    {
        plcTxFrmSwDesc.frmType = HPGP_HW_FRMTYPE_SOUND;
        frmType                = HPGP_HW_FRMTYPE_SOUND;
        plcTxFrmSwDesc.frmInfo.plc.src     = pTestParams->src;
        plcTxFrmSwDesc.frmInfo.plc.saf     = pTestParams->saf;
        plcTxFrmSwDesc.frmInfo.plc.scf     = pTestParams->scf;
    }
	else if (pTestParams->frmType == HPGP_HW_FRMTYPE_RTS)
	{
	    plcTxFrmSwDesc.frmType        = HPGP_HW_FRMTYPE_MSDU;  //HPGP_HW_FRMTYPE_RTS;
        frmType                       = HPGP_HW_FRMTYPE_MSDU;  //HPGP_HW_FRMTYPE_RTS;
        plcTxFrmSwDesc.frmInfo.plc.dt_av = HPGP_DTAV_RTS_CTS;
	}
	else if  (pTestParams->frmType == HPGP_HW_FRMTYPE_CTS)
    {
	    plcTxFrmSwDesc.frmType        = HPGP_HW_FRMTYPE_MSDU;  //HPGP_HW_FRMTYPE_CTS;
        frmType                       = HPGP_HW_FRMTYPE_MSDU;  //HPGP_HW_FRMTYPE_CTS;
        plcTxFrmSwDesc.frmInfo.plc.dt_av = HPGP_DTAV_RTS_CTS;
	}
		
    mcstMode           = pTestParams->mcstMode;
    if(pTestParams->mcstMode == 0)   
    {
        plcTxFrmSwDesc.frmInfo.plc.dtei           = gHpgpHalCB.remoteTei;       
    }
    else
    {
        plcTxFrmSwDesc.frmInfo.plc.dtei           = 0xFF;
    }

    if(pTestParams->altPlidTest)
    {
        plcTxFrmSwDesc.frmInfo.plc.plid = pTestParams->plid; //0;
    }
    else
    {
        plcTxFrmSwDesc.frmInfo.plc.plid  = pTestParams->plid;   
    }                                
	//printf("plid = %d\n",plcTxFrmSwDesc.frmInfo.plc.plid);  //[YM] debug Hybrii_B data pkt Tx
	
    plcTxFrmSwDesc.frmInfo.plc.stei           = gHpgpHalCB.selfTei;//HYBRII_DEFAULT_TEISTA;


    stdModeSel                    = pTestParams->stdModeSel;

    // Incremental/alternating length modes
    if(pTestParams->frmLen == 0)   //Continuous Tx mode
    {
        HHT_GetMinMaxLen(pTestParams, &stdModeSel, &minFrmLen, &maxFrmLen);
        
        stdRoboFrmLenMax = plcTxFrmSwDesc.frmType==HPGP_HW_FRMTYPE_MSDU?(HYBRII_STD1PBHSROBO_DATALEN_MAX):(HYBRII_STD1PBHSROBO_MGMTLEN_MAX);               
        curFrmLen = minFrmLen;
        FM_Printf(FM_LINFO,"\nStarting from len = %u\n",curFrmLen);
    }
    else
    {
        // fixed length test
        curFrmLen = pTestParams->frmLen;
    }
    plcTxFrmSwDesc.frmInfo.plc.stdModeSel = stdModeSel;
    curOffsetDW  = pTestParams->offsetDW;
    curDescLen   = pTestParams->descLen; 
    gHpgpHalCB.halStats.CurTxTestFrmCnt = 0;
	
    //for each frame
    while(1)
    {
        u8  frmData = 0;
        tmpFrmLen   = 0;
        tmpPayloadLen = 0;

        if (plcTxFrmSwDesc.frmType == HPGP_HW_FRMTYPE_SOUND)
        {
            if(pTestParams->frmLen <= 136)
                plcTxFrmSwDesc.frmLen = pTestParams->frmLen; //136;
            else
                plcTxFrmSwDesc.frmLen = pTestParams->frmLen; //520;
        }
        else
        {
            // check for mgmt frm max len, in case this is inclen & altfrmtype test
            plcTxFrmSwDesc.frmLen         =  (plcTxFrmSwDesc.frmType == HPGP_HW_FRMTYPE_MGMT && curFrmLen > HYBRII_3PBHSROBO_MGMTLEN_MAX) ? \
                                              HYBRII_3PBHSROBO_MGMTLEN_MAX : curFrmLen ;  
        }		
		
        //[YM] Add DBC Code
	    if (pTestParams->dbc)
	    {
	        if (pTestParams->pattern == 1)  // One RoBo + One Mini RoBo
	        {
                if (dbc_pktchange > 1)
					dbc_pktchange = 0;
				if (dbc_pktchange == 0)
	            {
	                plcTxFrmSwDesc.frmLen =  HYBRII_STD1PBHSROBO_DATALEN_MAX;
				    plcTxFrmSwDesc.frmInfo.plc.stdModeSel = 1;  //Std RoBo
	            }
				else if (dbc_pktchange == 1)
				{
				    plcTxFrmSwDesc.frmLen =  HYBRII_MINIROBO_DATALEN_MAX;
				    plcTxFrmSwDesc.frmInfo.plc.stdModeSel = 0;
				}
	        }
			else if (pTestParams->pattern == 2)  // Three Mini RoBo
			{
			    	plcTxFrmSwDesc.frmLen =  HYBRII_MINIROBO_DATALEN_MAX;
				    plcTxFrmSwDesc.frmInfo.plc.stdModeSel = 0;
			}
			else if (pTestParams->pattern == 3)  //Two MiniRoBo + One HS RoBo 1 PB
			{
				if ((dbc_pktchange == 0)|(dbc_pktchange == 1))
	            {
	                plcTxFrmSwDesc.frmLen =  HYBRII_MINIROBO_DATALEN_MAX;
				    plcTxFrmSwDesc.frmInfo.plc.stdModeSel = 0;  
	            }
				else
				{
				    plcTxFrmSwDesc.frmLen =  HYBRII_STD1PBHSROBO_DATALEN_MAX;
				    plcTxFrmSwDesc.frmInfo.plc.stdModeSel = 0;
				}
			}
			else if (pTestParams->pattern == 4)  //One MiniRoBo + One HS RoBo 2PB
			{
				if (dbc_pktchange == 0)
	            {
	                plcTxFrmSwDesc.frmLen =  HYBRII_MINIROBO_DATALEN_MAX;
				    plcTxFrmSwDesc.frmInfo.plc.stdModeSel = 0;  
	            }
				else
				{
				    plcTxFrmSwDesc.frmLen =  HYBRII_2PBHSROBO_DATALEN_MAX;
				    plcTxFrmSwDesc.frmInfo.plc.stdModeSel = 0;
				}
			}
			else if (pTestParams->pattern == 5)  //Two HS RoBo with 1 PB
			{
			        plcTxFrmSwDesc.frmLen =  HYBRII_STD1PBHSROBO_DATALEN_MAX;
				    plcTxFrmSwDesc.frmInfo.plc.stdModeSel = 0;  
			}
			else if (pTestParams->pattern == 6)  //One Mini RoBo + One HS RoBo with 3 PB
			{

				if (dbc_pktchange == 0)
				{
						plcTxFrmSwDesc.frmLen =  HYBRII_MINIROBO_DATALEN_MAX;
						plcTxFrmSwDesc.frmInfo.plc.stdModeSel = 0;	
				}
				else
				{
					plcTxFrmSwDesc.frmLen =  HYBRII_3PBHSROBO_DATALEN_MAX;
				    plcTxFrmSwDesc.frmInfo.plc.stdModeSel = 0;
				}	
			}
			else if (pTestParams->pattern == 7)  //One RoBo + One HS RoBo with 1 PB
			{
				if (dbc_pktchange == 0)
				{
						plcTxFrmSwDesc.frmLen =  HYBRII_STD1PBHSROBO_DATALEN_MAX;
						plcTxFrmSwDesc.frmInfo.plc.stdModeSel = 1;	
				}
				else
				{
					plcTxFrmSwDesc.frmLen =  HYBRII_STD1PBHSROBO_DATALEN_MAX;
				    plcTxFrmSwDesc.frmInfo.plc.stdModeSel = 0;
				}	
			}
			else if (pTestParams->pattern == 8)  //One HSRoBo with 1 PB + One HSRoBo with 2 PB
			{
				if (dbc_pktchange == 0)
				{
						plcTxFrmSwDesc.frmLen =  HYBRII_STD1PBHSROBO_DATALEN_MAX;
						plcTxFrmSwDesc.frmInfo.plc.stdModeSel = 0;	
				}
				else
				{
					plcTxFrmSwDesc.frmLen =  HYBRII_2PBHSROBO_DATALEN_MAX;
				    plcTxFrmSwDesc.frmInfo.plc.stdModeSel = 0;
				}
			}
        }

            plcTxFrmSwDesc.cpCount        = 0; 
			plcTxFrmSwDesc.frmInfo.plc.phyPendBlks    = pTestParams->plid>0 ? HPGP_PPB_CAP123 : HPGP_PPB_CAP0;
            plcTxFrmSwDesc.frmInfo.plc.mcstMode       = mcstMode;       
		
		    if (mcstMode > 0)
    		    plcTxFrmSwDesc.frmInfo.plc.phyPendBlks = HPGP_PPB_MCFRPT;
    	    else
    	        plcTxFrmSwDesc.frmInfo.plc.phyPendBlks    = (pTestParams->plid > 0) ? HPGP_PPB_CAP123 : HPGP_PPB_CAP0;        	    

		
        if(pTestParams->altPlidTest)
        {
            pTestParams->plid = (gHpgpHalCB.halStats.CurTxTestFrmCnt & 0x3) ; //plid value will be changed in sequence;
            plcTxFrmSwDesc.frmInfo.plc.plid  = pTestParams->plid;    
        }
		else
			plcTxFrmSwDesc.frmInfo.plc.plid  = pTestParams->plid;
                 		
        // create cp descriptors
        while(tmpFrmLen < plcTxFrmSwDesc.frmLen)
        {       
            u8        cp;
            u8        tmpOffsetDW;
            u8        tmpOffsetByte;
            u8        tmpDescLen;
            u8        remDescLen;
            u8        actualDescLen;
            volatile u8 xdata *       cellAddr;

            tmpOffsetDW =      curOffsetDW;
            tmpDescLen  =      curDescLen; 
            // Fetch CP
			//printf("(C)\n");
            do
            {
                status = CHAL_RequestCP(&cp);
#ifdef _FIXME_
#ifdef HPGP_HAL_TEST
                c = CHT_Poll();
#else
                c = poll_key();
#endif
                if( c == 'q')
                {
                    // Realease CPs fetched so far for current frame -- tbd
                    quit = 1;
                    break;
                }

#endif
            }while (status != STATUS_SUCCESS);

			
            // check for user initiated exit task
            if(quit)
            {
                break;
            }
			
            i = plcTxFrmSwDesc.cpCount;
            // test offset and desc len - only for first CPs
            if((i==0 || i==1) && (pTestParams->frmType != 2))
            //if(i==0 || i==1)
            { 
                if(pTestParams->altOffsetDescLenTest)
                {
                    curOffsetDW--;
                    curDescLen+=4;    
                    tmpOffsetDW =      curOffsetDW;  
                    tmpDescLen  =      curDescLen;  
                    if( curOffsetDW==0 )
                    {
                        curOffsetDW  = pTestParams->offsetDW; // 0
                        curDescLen   = pTestParams->descLen;  //HYBRII_DEFAULT_SNID;
//                        FM_Printf(FM_LINFO,"OffsetDW & DescLen resetting to %bu & %bu respectively\n", curOffsetDW, curDescLen);
                    }
                } //printf("curOffsetDW = %bu, tempDescLen=%bu\n", tmpOffsetDW, tmpDescLen);                            
            }
            else if(pTestParams->frmType == 2)
            {
                  tmpOffsetDW =      0;  
                  tmpDescLen  =      curDescLen;  
            }
            else
            {
                tmpOffsetDW = 0;
                tmpDescLen  = HYBRII_CELLBUF_SIZE;
            }

            tmpOffsetByte = tmpOffsetDW << 2;
            actualDescLen =  (plcTxFrmSwDesc.frmLen-tmpFrmLen)>tmpDescLen ? tmpDescLen : (plcTxFrmSwDesc.frmLen-tmpFrmLen);
            remDescLen    =  actualDescLen;

            if(( i==0 || i==1 ) && tmpOffsetDW != 0)
            {
              //  FM_Printf(FM_LINFO,"curFrmLen = %u, curOffsetDW = %bu, curDescLen=%bu, free CPCnt = %bu\n", 
              //                               plcTxFrmSwDesc.frmLen, tmpOffsetDW, actualDescLen, CHAL_GetFreeCPCnt());
            }
            //FM_Printf(FM_LINFO,"curOffsetByte = %bu, curDescLen=%bu\n", tmpOffsetByte, actualDescLen);
            
            // Fill Buffer with pattern
#ifdef MEM_PROTECTION
			// Get local CP buffer
			cellAddr = &cp_localBuf[0];
#else
            cellAddr = CHAL_GetAccessToCP(cp);
#endif
			memset(&cp_localBuf[0], 0, HYBRII_CELLBUF_SIZE);	// clear read buf for every new test
         //   FM_Printf(FM_LINFO,"cp = %bu, cellAddr=%08lX, seqNum=%bu\n",cp,(u32)cellAddr, gHpgpHalCB.halStats.TxSeqNum);
            //printf("cp = %bu, cellAddr=%08lX, seqNum=%bu\n",cp,(u32)cellAddr, gHpgpHalCB.halStats.TxSeqNum);
			
            // Add Seq Num as first byte of first CP
            if ( i==0 )
            {
                if (pTestParams->frmType != 2)
                {
#if  PLC_BCNDATA_FIXED_PATTERN
                cellAddr[tmpOffsetByte] = 0xBB;
                // Start frame data from seq num.
                frmData   = (u8)(gHpgpHalCB.halStats.TxSeqNum+1);   

#elif PLC_DATA_FIXED_PATTERN

                cellAddr[tmpOffsetByte] = (u8)((gHpgpHalCB.halStats.TxSeqNum + 1) & 0xFF);
                // Alternatig frame bytes
                frmData = ((gHpgpHalCB.halStats.TxSeqNum + 1) & 0x01) ?  0xAA : 0x55 ;
#endif        
                tmpOffsetByte +=1;
                remDescLen    -=1; 
                }
                else 
                {                     
                    //cellAddr[tmpOffsetByte] = (u8)((gHpgpHalCB.halStats.TxSeqNum + 1) & 0xFF);
                    // Alternatig frame bytes
                    frmData = ((gHpgpHalCB.halStats.TxSeqNum + 1) & 0x01) ?  0x00 : 0x00 ;
 
                    //tmpOffsetByte +=1;
                    //remDescLen    -=1; 
                }
            }

            if (pTestParams->frmType == 2) // Sound packet 
            {
                for( j=tmpOffsetByte ; j<tmpOffsetByte+remDescLen ; j++)
                {    
                    cellAddr[j] = 0x0;
                }
            }
            else
            {
                for( j=tmpOffsetByte ; j<tmpOffsetByte+remDescLen ; j++)
                {
    #if  PLC_BCNDATA_FIXED_PATTERN
                    cellAddr[j] = 0xBB;
    #elif PLC_DATA_FIXED_PATTERN                    
                    cellAddr[j] = frmData;
                    frmData     = _cror_(frmData, 1);
    #else
                    cellAddr[j] = frmData++;
    #endif
                }
            }
#if 0       
            // [YM] debug message
			printf("CP len: %bu, offset=%bu\n", remDescLen, tmpOffsetByte);                         
			
            for( j=tmpOffsetByte ; j<tmpOffsetByte+remDescLen ; j++)
            {
                printf("0X%bx ", cellAddr[j]);
				if ((j>0) && ((j % 20) == 0))
					printf("\n");
            } 
#endif			                        
			//printf("\n");
            plcTxFrmSwDesc.cpArr[i].offsetU32 = tmpOffsetDW;
            plcTxFrmSwDesc.cpArr[i].len       = actualDescLen; 
            tmpFrmLen                        += plcTxFrmSwDesc.cpArr[i].len; 
            plcTxFrmSwDesc.cpArr[i].cp        = cp;
            plcTxFrmSwDesc.cpCount++;
            tmpPayloadLen += (plcTxFrmSwDesc.cpArr[i].len - tmpOffsetByte);

            // Alternate Encryption Test Mode
            if(pTestParams->secTestMode == ALT_UNENC_NEK)
            {
                plcTxFrmSwDesc.frmInfo.plc.eks = (plcTxFrmSwDesc.frmInfo.plc.eks  >= HPGP_MAX_NEKS) ? 0  : (plcTxFrmSwDesc.frmInfo.plc.eks + 1);
            }
            else if(pTestParams->secTestMode == ALT_UNENC_NEK_PPEK)
            {
                plcTxFrmSwDesc.frmInfo.plc.eks  = gAltEksTstArr[eksArrIdx++];
                plcTxFrmSwDesc.frmInfo.plc.dtei = gHpgpHalCB.remoteTei;
            }

#ifdef MEM_PROTECTION
			// now copy the CP local buf to the actual CP memory
			if (HHAL_CP_Put_Copy(cp, cellAddr, HYBRII_CELLBUF_SIZE) == STATUS_FAILURE)
			{
				printf("HHT_SimulateTx: Failed to put a copy of CP %bu. Continue with nex CP\n", cp);
				continue;
			}
#endif
        } 
	    
		
#ifdef Packet_grouping0
        //[YM] Check multiple packet queueing is requested or not
        if (gHpgpHalCB.plcMultiPktTest > 1 )
        {
                cmd_num++;

				//printf("1-0. cmd_num = %bu \n", cmd_num);
                /*  Write PLC Command Queue Write Register to trigger HW Tx */
                txCmdQueueWrtie.reg = 0;
                txCmdQueueWrtie.s.txQ = pTestParams->plid;
	            txCmdQueueWrtie.s.txCap = pTestParams->plid;
	            txCmdQueueWrtie.s.txRobo = pTestParams->stdModeSel;
#ifdef ETH_BRDG_DEBUG
				if (myDebugFlag)
				{
			    //printf("txCmdQueueWrtie.reg = %lx\n",txCmdQueueWrtie.reg);
				//printf("txCmdQueueWrtie.s.txQ = %bu\n", txCmdQueueWrtie.s.txQ);
				printf("txCmdQueueWrtie.s.txCap = %bu\n", txCmdQueueWrtie.s.txCap);
				//printf("txCmdQueueWrtie.s.txRobo = %bu\n", txCmdQueueWrtie.s.txRobo);
				}
#endif  //ETH_BRDG_DEBUG
				//WriteU32Reg(PLC_CMDQ_REG, txCmdQueueWrtie.reg);
				//[YM] store command queue setting value to a software queue, will write all stored command queue setting value to 
				//command queue later
				Sw_command_queue[cmd_num - 1] = txCmdQueueWrtie.reg;
				if (cmd_num > (gHpgpHalCB.plcMultiPktTest - 1))
					{
                       u8 p;
					   
					   // Write all the store command settings in sw command queue [64] to HW command queue register
					   for (p = 0; p < gHpgpHalCB.plcMultiPktTest; p++)
					   	{
					   	   WriteU32Reg(PLC_CMDQ_REG, Sw_command_queue[p]);
#ifndef MPER						   
						   printf("Write PLC_CMDQ_REQ = %u\n", Sw_command_queue[p]);
#endif  //MPER
					   	}
					   if (cmd_num > 0)
#ifndef MPER					   	
					       printf("1. Write PLC_CMDQ_REG %bu times\n", cmd_num);
#endif  //MPER
					   cmd_num = 0;
					   gHpgpHalCB.plcMultiPktTest = 0;
					}
        }
#endif	//Packet_grouping0	
        // check for user initiated exit task
        if(status == STATUS_SUCCESS)
        {
                   u16 pkt_retry = 0;
            do
            {
                // Transmit the frame;
                //printf("(D)\n");
#ifdef HPGP_HAL_TEST
                status = HHAL_PlcTxQWrite(&plcTxFrmSwDesc);
#else
                status = HHAL_PlcTxQWrite(hal, &plcTxFrmSwDesc);
#endif
				if (status == STATUS_FAILURE)
				{
					pkt_retry++;
					//printf("Write PLC Tx Q failed, %bu times, cp = %bu\n", pkt_retry, plcTxFrmSwDesc.cpCount);
					// [YM] - retry the packet transmission until it failed too many times

                    CHAL_DelayTicks(64);
					if (pkt_retry > 2000)   //[YM] extended delay loop, high PER condition will make Tx stop too quick
					{
#ifndef MPER					
                        printf("\nWrite PLC Tx Q failed, cp = %bu, Quit!!\n", plcTxFrmSwDesc.cpCount);
#endif
					    quit = 1;
					    break;
					}
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
			        pkt_retry = 0;
        }

		 
        if(status == STATUS_SUCCESS)
        {
            gHpgpHalCB.halStats.CurTxTestFrmCnt++;
            gHpgpHalCB.halStats.TxSeqNum++;

            // check for alternating plid
            plcTxFrmSwDesc.frmInfo.plc.plid = pTestParams->altPlidTest ? altPlid.s.plid++ : pTestParams->plid;

            // check for alternating frametype
            if(pTestParams->altFrmTypeTest)
            {
                plcTxFrmSwDesc.frmType = (plcTxFrmSwDesc.frmType == HPGP_HW_FRMTYPE_MSDU) ? HPGP_HW_FRMTYPE_MGMT : HPGP_HW_FRMTYPE_MSDU;
            }
            else
            {
                plcTxFrmSwDesc.frmType = frmType;
            }

            // check for alternating mcstMode
            if(pTestParams->altMcstTest)
            {
                if(mcstMode == 2)
                {
                    mcstMode = 0;
                    plcTxFrmSwDesc.frmInfo.plc.dtei           = gHpgpHalCB.remoteTei;         
                }
                else
                {
                    mcstMode++;
                    plcTxFrmSwDesc.frmInfo.plc.dtei           = 0xFF;       
                }
            }

			if (!pTestParams->dbc)
			{
            // check for incremental length single robo
            if(pTestParams->lenTestMode == INC_LEN_SINGLE_ROBO)
            {
                curFrmLen++;
                if(curFrmLen > maxFrmLen)
                {
                    FM_Printf(FM_LINFO,"\nCur Frame Len = %u, Starting over from len %u\n",curFrmLen-1,minFrmLen);
                    curFrmLen = minFrmLen;
                }
            }             
            // check for incremental length all robo
            else if(pTestParams->lenTestMode == INC_LEN_ALL_ROBO)
            {
                curFrmLen++;            
                if(curFrmLen > stdRoboFrmLenMax && plcTxFrmSwDesc.frmInfo.plc.stdModeSel)
                {
                    plcTxFrmSwDesc.frmInfo.plc.stdModeSel = 0;
                    curFrmLen                 = HYBRII_MINIROBO_DATALEN_MAX+1;
                    FM_Printf(FM_LINFO,"Switching to HS Mode; len = %u\n",curFrmLen);
                    
                }
                // restart inc len test
                if(curFrmLen > maxFrmLen)
                {
                    FM_Printf(FM_LINFO,"\nCur Frame Len = %u, Starting over from len %u\n",curFrmLen-1,minFrmLen);
                    curFrmLen                 = minFrmLen;
                    plcTxFrmSwDesc.frmInfo.plc.stdModeSel = 1;                  
                }
            } 
            // check for fixed length alternating robo
            else if(pTestParams->lenTestMode == FIXED_LEN_ALT_ROBO)
            {
                curFrmLen  = gAltRoboLenArr[altRoboLenIdx.s.idx++];
            }
			}
			else  //DBC Test
			{
			    if (dbc_pktchange == 0)
					dbc_pktchange = 1;
				else if (dbc_pktchange == 1)
					dbc_pktchange = 2;
				else
					dbc_pktchange = 0;
			}
				
        }
		
        if((gHpgpHalCB.halStats.CurTxTestFrmCnt & (u32)(0xFF)) == 0)
        {  
            printf("\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b");
            printf("Sent %ld, Received %ld HPGP frames.", gHpgpHalCB.halStats.CurTxTestFrmCnt, gHpgpHalCB.halStats.TotalRxGoodFrmCnt - gHpgpHalCB.halStats.RxGoodBcnCnt);
        }

		
        if(!pTestParams->contMode)
        {
            pTestParams->numFrames--;
//			printf(" Remaining Tx packet number = %ld\n",pTestParams->numFrames);
            if(!pTestParams->numFrames)
            {
                quit = 1;
            }
        }

		
        if(!quit && pTestParams->delay != 0xFF)
        {
            u32 delay64ticks = pTestParams->delay;
            if(delay64ticks == 0)
            {
                printf("press c to continue\n");
                while(1)
                {
#ifdef HPGP_HAL_TEST
                    u8 userInput = CHT_Poll();
#else
                    u8 userInput = poll_key();
#endif
                    if( userInput == 'c' || userInput == 'C')
                    {
                        // exit delay loop and resume transmission
                        break;                      
                    }
                    else if( userInput == 'q' || userInput == 'Q')
                    {
                        // exit delay loop and quite transmission
                        quit = 1;
                        break;
                    }
                }
            }
            else
            while(delay64ticks--)
            {
                CHAL_DelayTicks(64);
                // check for user initiated exit task from infinite loop
                
#ifdef HPGP_HAL_TEST
                c = CHT_Poll();
#else
                c = poll_key();
#endif
                if( c == 'q')              
                {
                    quit = 1;
                    break;
                }
            }  
        }
		
        if(quit )//|| CHAL_GetFreeCPCnt() < 100 || HHAL_GetPlcTxQFreeDescCnt(plcTxFrmSwDesc.plid) < 32 )
        {
//#ifdef Packet_grouping0
        //[YM] write sw command queue setting values to HW command queue
        //printf("2. Write PLC_CMDQ_REG %bu times\n", cmd_num);
		while (cmd_num > 0)
		{
           u8 p;
		   
		   //printf("(F)\n");
		   // Write all the store command settings in sw command queue [64] to HW command queue register
		   for (p = 0; p < cmd_num; p++)
		   	   WriteU32Reg(PLC_CMDQ_REG, Sw_command_queue[p]);
		   cmd_num--;
		}
//#endif


           // printf("Sent %ld HPGP frames.\n", gHpgpHalCB.halStats.CurTxTestFrmCnt);
           // printf("Quit Tx: Free CP Cnt = %bu, curFrmLen = %u\n", CHAL_GetFreeCPCnt(), curFrmLen);
            break;
        }       
    } // while(1)   
}

#endif
/*u32 TX_RXLatency_TCC[8] = {
                   
                    0x321e0417,  //TCC =1
                   // 0x336F0417,
                    0x336F04E5, //this is according to new phy image gpphy_3S150_08032013- tx latency increased to 50microsec from 41.8 microsec
                    0x34ca0417,
                    0x361F0417,
                    0x37750417,
                    0x38c20417,
                    0x3a180417,
                    0x3b740417,  //TCC= 8

};*/




#ifdef POWER_SAVE
extern u16 psNoTxFrm;

void HHAL_SetTxRxEn(u8 mode)
{
    uPlcStatusReg         plcStatus;

    if (mode == TxRxEnState.state)
    {	IRQ_DISABLE_INTERRUPT();	// this function can also be called from timer interrupt
        plcStatus.reg  = ReadU32Reg(PLC_STATUS_REG);
    	
        // mode = 0: enable, 1: disable
		if (mode == 1)
		{
			// disable TxRxEn
       		plcStatus.s.nTxEn  = 1;
        	plcStatus.s.nRxEn = 1;
        	WriteU32Reg(PLC_STATUS_REG, plcStatus.reg);
			if (ethDebugON)
  					printf("TxRxEn Disabled\n");
        	if (gHpgpHalCB.devMode == DEV_MODE_STA)
			{
            	// STA: we just disabled TxRxEn, set a timer to wake
            	// up and enable them again so we can receive
            	// next beacon. Only do this for STA, CCO uses the BCN Tx
				// interrupt and BCN Sent interrupt to wake up
            	TxRxEnState.StarttimerIntCnt = gHalCB.timerIntCnt;
            	TxRxEnState.StartRxGoodBcnCnt = gHpgpHalCB.halStats.RxGoodBcnCnt;
        	}
		}
		else
		{
			// enable TxRxEn
	    	plcStatus.reg = ReadU32Reg(PLC_STATUS_REG);
       		plcStatus.s.nTxEn  = 0;
			//plcStatus.s.randomBackoff = 1;  //added by YM 
			// below is sequence to re-enable RxEn
	    	plcStatus.s.rxSoftReset = 1;
    		WriteU32Reg(PLC_STATUS_REG, plcStatus.reg);
		    CHAL_DelayTicks(10);
		    plcStatus.s.rxSoftReset = 0;
		    WriteU32Reg(PLC_STATUS_REG, plcStatus.reg);
			if (ethDebugON)
  					printf("TxRxEn Enabled\n");
		}
        TxRxEnState.state = !mode;    // State =  0: Disabled, 1: Enabled
       	IRQ_ENABLE_INTERRUPT();
	}
}

// HHAL_DisTxRxEn is called by the HW timer interrupt every 4 ms
void HHAL_DisTxRxEn()
{
   if (!plc_powersave || TxRxEnState.state ||
   		(gHpgpHalCB.devMode == DEV_MODE_CCO))
		// this timer ISR is only used in STA
       return;
    
    if(gHalCB.timerIntCnt >= (TxRxEnState.StarttimerIntCnt + 18))
    {
        // STA should receive a central beacon every 33 ms 
        // (50 Mhz) or 44 ms (60 Mhz). gHalCB.timerIntCnt is
        // incremented every timer interrupt cycle, ie. 4ms.
        // Enable TxRxEn so it can receive the next beacon 
		if (ethDebugON)
		{
		   printf("Timer INT:ON TxRxEn\n");
		   printf("Timer INT:gHalCB.timerIntCnt=%lu,TxRxEnState.StarttimerIntCnt=%lu\n", gHalCB.timerIntCnt, TxRxEnState.StarttimerIntCnt);
		} 
        HHAL_SetTxRxEn(0);
    }
}
#endif 	   //POWER_SAVE

// this is specific to Hybrii B
void set_plc_paramter(u8 sel, u32 u32Val)
{
    union {
        u8  chval[4];
        u32 reg;
    }  val;

    val.reg = 0;
    val.chval[0] = sel;

    WriteU32Reg(PLC_PLCPARASEL_REG, val.reg);
    WriteU32Reg(PLC_PARA_DATAWR_REG, ctorl(u32Val));
}


/*******************************************************************
* NAME :            HHAL_mpiRxReset
*
* DESCRIPTION :     Reset mpi Rx MAC block
*
* INPUTS :
*       PARAMETERS:
*      
*
* OUTPUTS :
*       None
*
*/
/*	Hybrii A only
void HHAL_mpiRxReset(void)
{
    uPlcResetReg plcReset;

    plcReset.reg = 0;
    plcReset.s.mpiRxReset = 1;
    WriteU32Reg(PLC_RESET_REG, plcReset.reg);
    CHAL_DelayTicks(2);
    plcReset.reg = 0;
    WriteU32Reg(PLC_RESET_REG, plcReset.reg);
}*/


/*******************************************************************
* NAME :            HHAL_PhyPgmRoboMd
*
* DESCRIPTION :     Programs Rx to a particular Robo Md.
*
* INPUTS :
*       PARAMETERS:
*           eHpgpRpoboMod roboMd
*
* OUTPUTS :
*       None
*
*/
void HHAL_PhyPgmRoboMd(eRegFlag enbRoboMdPgm, ePlcPhyRxRoboMod roboMd, ePlcNumPBs numPBs)
{
    uPhyRxEnbRoboReg  phyRxEnbRobo;
    uPhyRxRoboMdReg   phyRxRoboMd;
    uPhyNumPBReg      phyNumPBs;

    // Write Rx Robo Mode override bit.
    phyRxEnbRobo.reg         = ReadU8Reg(PLC_RXROBOENB_REG);
    phyRxEnbRobo.s.enbRobo   = enbRoboMdPgm;
    WriteU8Reg(PLC_RXROBOENB_REG, phyRxEnbRobo.reg);

    // Write Rx Robo Mode.
    if(enbRoboMdPgm)
    {
        phyRxRoboMd.reg          = ReadU8Reg(PLC_RXROBOMD_REG);
        phyRxRoboMd.s.roboMd     = roboMd;
        WriteU8Reg(PLC_RXROBOMD_REG, phyRxRoboMd.reg);

        // Write Number of PBs
        if( roboMd == PLCPHY_ROBOMD_HS )
        {
            phyNumPBs.reg          = ReadU8Reg(PLC_RXNUMPB_REG);
            phyNumPBs.s.enbNumPBs  = 1; 
            phyNumPBs.s.numPBs     = numPBs;
            WriteU8Reg(PLC_RXNUMPB_REG, phyNumPBs.reg);
        }
    }        
}

/*******************************************************************
* NAME :            HHAL_SetACLine50HzFlag
*
* DESCRIPTION :     Sets or Clears CpuSwStatus.acLine50Hz flag.
*
* INPUTS :
*       PARAMETERS:
*           eRegFlag acLin50Hz
*
* OUTPUTS :
*       None
*
*/
/*void HHAL_SetACLine50HzFlag(eRegFlag acLin50Hz)
{
    uPlcLineControlReg plcLineCtrl;

    plcLineCtrl.reg = ReadU32Reg(PLC_LINECTRL_REG);
    plcLineCtrl.s.acCycle50Hz = acLin50Hz;
    WriteU32Reg(PLC_LINECTRL_REG, plcLineCtrl.reg);
} */

/*******************************************************************
* NAME :            HHAL_SetPlcDevMode()
*
* DESCRIPTION :     Sets PLC Device Mode.
*
* INPUTS :
*       PARAMETERS:
*           eRegFlag regFlag
*
* OUTPUTS :
*       None
*
*/
void HHAL_SetPlcDevMode(ePlcDevMode plcDevMode)
{
    uPlcDevCtrlReg PlcDevCtrl;
    PlcDevCtrl.reg = ReadU32Reg(PLC_DEVCTRL_REG);
    PlcDevCtrl.s.plcDevMode = plcDevMode;
    WriteU32Reg(PLC_DEVCTRL_REG,PlcDevCtrl.reg);
}


#ifdef FREQ_DETECT
void FREQDET_FreqSetting(u8 frequency)
{

	
 
 	FM_Printf(FM_MINFO,"freq %bu \n", frequency );
               
    if(frequency == FREQUENCY_50HZ)
    {                
        gHpgpHalCB.lineFreq = FREQUENCY_50HZ;
        if(gHpgpHalCB.lineMode == LINE_MODE_DC)
        {
        	PLC_DC_LINE_CYCLE_FREQENCY = DC_50HZ;
            PLC_DC_BP_LEN = PLC_DC_BP_LEN_50HZ;                    
            WriteU32Reg(PLC_DCLINECYCLE_REG, ctorl(PLC_DC_LINE_CYCLE_FREQENCY));
            gHpgpHalCB.curBcnPer = PLC_DC_BP_LEN;
			WriteU32Reg(PLC_SWBCNPERAVG_REG, ctorl((gHpgpHalCB.curBcnPer >> 1)));	// HW multiples it by 2
        }
        else
        {
            PLC_AC_BP_LEN = PLC_AC_BP_LEN_50HZ; 
            //WriteU32Reg(PLC_SS1TIMEOUT_REG, ctorl(PLC_AC_BP_LEN - PLC_LATE_BCN_SYNC_THRES));
            gHpgpHalCB.curBcnPer = PLC_AC_BP_LEN;
			WriteU32Reg(PLC_SWBCNPERAVG_REG, ctorl((gHpgpHalCB.curBcnPer >> 1)));	// HW multiples it by 2
            PLC_MIN_AC_BPLEN  =           MIN_50HZ_BPLEN;
            PLC_MAX_AC_BPLEN  =           MAX_50HZ_BPLEN; 
            AC_MIN_THRESHOLD = AC_MIN_THRESHOLD_50Hz;
        }
//        HHAL_SetACLine50HzFlag(REG_FLAG_SET);
        
        //WriteU32Reg(PLC_SWBCNPERAVG_REG, ctorl(gHpgpHalCB.curBcnPer));
    }
    else // 60 hz
    {                
        gHpgpHalCB.lineFreq = FREQUENCY_60HZ;
        if(gHpgpHalCB.lineMode == LINE_MODE_DC)
        {
        	PLC_DC_LINE_CYCLE_FREQENCY = DC_60HZ;		
            PLC_DC_BP_LEN = PLC_DC_BP_LEN_60HZ;                    
            WriteU32Reg(PLC_DCLINECYCLE_REG, ctorl(PLC_DC_LINE_CYCLE_FREQENCY));
            gHpgpHalCB.curBcnPer = PLC_DC_BP_LEN;                    
			WriteU32Reg(PLC_SWBCNPERAVG_REG, ctorl((gHpgpHalCB.curBcnPer >> 1)));	// HW multiples it by 2
        }
        else
        {
            PLC_AC_BP_LEN = PLC_AC_BP_LEN_60HZ; 
//            WriteU32Reg(PLC_SS1TIMEOUT_REG, ctorl(PLC_AC_BP_LEN - PLC_LATE_BCN_SYNC_THRES));
            gHpgpHalCB.curBcnPer = PLC_AC_BP_LEN;
			WriteU32Reg(PLC_SWBCNPERAVG_REG, ctorl((gHpgpHalCB.curBcnPer >> 1)));	// HW multiples it by 2
            PLC_MIN_AC_BPLEN  =           0xCB300;
            PLC_MAX_AC_BPLEN  =           0xCBA00; 
            AC_MIN_THRESHOLD = AC_MIN_THRESHOLD_60Hz;//this is done just to avoid noise or spike while detecting zero cross
        }
//        HHAL_SetACLine50HzFlag(REG_FLAG_CLR);                
    }
/*#ifndef HPGP_HAL_TEST
    CTRLL_SendFreqDetectedEvent();
#endif*/
}

#endif
/*******************************************************************
* NAME :            HHAL_GetPlcDevMode
*
* DESCRIPTION :     Returns PLC Device Mode.
*
* INPUTS :
*       None
*
* OUTPUTS :
*       None
*
*/
ePlcDevMode HHAL_GetPlcDevMode()
{
    uPlcDevCtrlReg PlcDevCtrl;

    PlcDevCtrl.reg = ReadU32Reg(PLC_DEVCTRL_REG);

    return((ePlcDevMode)PlcDevCtrl.s.plcDevMode);
}


/*******************************************************************
* NAME :            HHAL_SetSWStatReqScanFlag
*
* DESCRIPTION :     Sets or clears PLC Scan Mode.
*
* INPUTS :
*       PARAMETERS:
*           eRegFlag scanEnb
*
* OUTPUTS :
*       None
*
*/
void HHAL_SetSWStatReqScanFlag(eRegFlag scanEnb)
{
    uPlcLineControlReg plcLineCtrl;
    uPlcDevCtrlReg  plcDevCtrl;

    plcLineCtrl.reg = ReadU32Reg(PLC_LINECTRL_REG);
    plcDevCtrl.reg  = ReadU32Reg(PLC_DEVCTRL_REG);
    plcLineCtrl.s.reqScanning = scanEnb;
    //gHpgpHalCB.scanEnb    = scanEnb;   
    if(scanEnb && !gHpgpHalCB.scanEnb)
    {
        plcDevCtrl.s.snidValid    = 1;  // ask TRI
        gHpgpHalCB.syncComplete   = 0;
        gHpgpHalCB.halStats.RxGoodBcnCnt   = 0;
        gHpgpHalCB.halStats.syncTestValIdx = 0;
        //gHpgpHalCB.nwSelected     = 0;
#ifndef RELEASE
        FM_Printf(FM_MINFO, "SetSWStatReqScanFlag:Scan Enable\n");
#endif

    }
    if(!scanEnb)
    {
#ifndef RELEASE
        FM_Printf(FM_MINFO, "SetSWStatReqScanFlag:Scan Disable\n");
#endif

    }
    gHpgpHalCB.scanEnb    = scanEnb;
#ifdef QCA	
    WriteU32Reg(PLC_LINECTRL_REG,plcLineCtrl.reg);
    WriteU32Reg(PLC_DEVCTRL_REG,plcDevCtrl.reg);
#endif
	
}


/*******************************************************************
* NAME :            HHAL_GetTei
*
* DESCRIPTION :     Sets local TEI.
*
* INPUTS :
*       PARAMETERS:
*           None
*
* OUTPUTS :
*       u8 tei
*
*/
u8 HHAL_GetTei()
{
    uPlcDevCtrlReg PlcDevCtrl;
    u8             stei;
    
    PlcDevCtrl.reg  = ReadU32Reg(PLC_DEVCTRL_REG);
    stei = PlcDevCtrl.s.stei;

    return stei;         
}

/*******************************************************************
* NAME :            HHAL_SetTei
*
* DESCRIPTION :     Sets local TEI.
*
* INPUTS :
*       PARAMETERS:
*           u8 tei
*
* OUTPUTS :
*       None
*
*/
void HHAL_SetTei(u8 stei)
{
    uPlcDevCtrlReg PlcDevCtrlReg;
#ifdef HPGP_HAL_TEST
    gHpgpHalCB.selfTei  = stei;
#else
    gHpgpHalCB.tei  = stei;
#endif	 //HPGP_HAL_TEST
    PlcDevCtrlReg.reg   = ReadU32Reg(PLC_DEVCTRL_REG);
    PlcDevCtrlReg.s.stei = stei;
    WriteU32Reg(PLC_DEVCTRL_REG, PlcDevCtrlReg.reg);
}

/*******************************************************************
* NAME :            HHAL_GetSnid
*
* DESCRIPTION :     Sets SNID of the network local device has joined/created.
*
* INPUTS :
*       PARAMETERS:
*           eRegFlag regFlag
*
* OUTPUTS :
*       None
*
*/
u8 HHAL_GetSnid()
{
    uPlcDevCtrlReg PlcDevCtrlReg;
    u8             snid;

    PlcDevCtrlReg.reg       = ReadU32Reg(PLC_DEVCTRL_REG);
    snid                 = PlcDevCtrlReg.s.snid;

    return snid;
}

/*******************************************************************
* NAME :            HHAL_SetSnid
*
* DESCRIPTION :     Sets SNID of the network local device has joined/created.
*
* INPUTS :
*       PARAMETERS:
*           u8 snid
*
* OUTPUTS :
*       None
*
*/
void HHAL_SetSnid(u8 snid)
{
#ifdef MCCO
	if (gHpgpHalCB.devMode == DEV_MODE_PCCO)
	{
		HHAL_SetPassSnid(snid);
	}
	else
#endif		
	{

	uPlcDevCtrlReg PlcDevCtrlReg;

    gHpgpHalCB.snid      = snid;
    PlcDevCtrlReg.reg    = ReadU32Reg(PLC_DEVCTRL_REG);
    PlcDevCtrlReg.s.snid = snid;
    if(gHpgpHalCB.devMode == DEV_MODE_CCO || !gHpgpHalCB.scanEnb)
    {
        PlcDevCtrlReg.s.snidValid = 1;
        FM_Printf(FM_MINFO, "SetSnid:%bu\n", snid);

	}
	else
	{	       
	      gHpgpHalCB.nwSelected  = 1;
	      PlcDevCtrlReg.s.snidValid = 1;
	}
	
	gHpgpHalCB.nwSelectedSnid = snid;
	  
	WriteU32Reg(PLC_DEVCTRL_REG, PlcDevCtrlReg.reg);

	}

}

#ifdef MCCO

void HHAL_ClearPassSnid()
{
	uPlcDevCtrlReg PlcDevCtrlReg;

	PlcDevCtrlReg.reg	 = ReadU32Reg(PLC_DEVCTRL_REG);
	PlcDevCtrlReg.s.passCordValid = 0;
	WriteU32Reg(PLC_DEVCTRL_REG, PlcDevCtrlReg.reg);
}


void HHAL_DisablePassSnid()
{
	uPlcDevCtrlReg PlcDevCtrlReg;

	PlcDevCtrlReg.reg	 = ReadU32Reg(PLC_DEVCTRL_REG);
	

	PlcDevCtrlReg.s.passCordValid = 0;
	
	WriteU32Reg(PLC_DEVCTRL_REG, PlcDevCtrlReg.reg);

}

void HHAL_SetPassSnid(u8 snid)
{
    uPlcDevCtrlReg PlcDevCtrlReg;

    gHpgpHalCB.passSnid  = snid;	
    gHpgpHalCB.snid      = snid;  //TODO RAJAN. do we need to set snid
	
    PlcDevCtrlReg.reg    = ReadU32Reg(PLC_DEVCTRL_REG);
    PlcDevCtrlReg.s.passSNID = snid;

	gHpgpHalCB.nwSelected  = 1;

	gHpgpHalCB.nwSelectedSnid = snid;
		
	PlcDevCtrlReg.s.passCordValid = 1;
	PlcDevCtrlReg.s.snidValid = 1;
	 
    WriteU32Reg(PLC_DEVCTRL_REG, PlcDevCtrlReg.reg);
}


void HHAL_SetNWSnid(u8 snid)
{
    uPlcDevCtrlReg PlcDevCtrlReg;
	
    PlcDevCtrlReg.reg    = ReadU32Reg(PLC_DEVCTRL_REG);
    PlcDevCtrlReg.s.snid = snid;

    PlcDevCtrlReg.s.snidValid = 1;
	
    WriteU32Reg(PLC_DEVCTRL_REG, PlcDevCtrlReg.reg);
}

#endif

void HHAL_DisableLMBcnBuf(u8 bcnType)
{
   gHpgpHalCB.lmBcn.txBitmap &= ~((1 << bcnType));
}

void HHAL_SetLMBcnBuf(u8 *buff, u8 bcnType, u8 bpstoOffset)
{
    if (bcnType == BEACON_TYPE_CENTRAL)
    {
        memcpy(&gHpgpHalCB.lmBcn.cBcn.bcnBuff, buff, sizeof(gHpgpHalCB.lmBcn.cBcn.bcnBuff));
        gHpgpHalCB.lmBcn.cBcn.bpstoOffset = bpstoOffset;


    }
#if 0	
    else
    if (bcnType == BEACON_TYPE_DISCOVER)
    {
        memcpy(&gHpgpHalCB.lmBcn.dBcn.bcnBuff, buff, sizeof(gHpgpHalCB.lmBcn.dBcn.bcnBuff));
        gHpgpHalCB.lmBcn.dBcn.bpstoOffset = bpstoOffset;
    }
#endif

    gHpgpHalCB.lmBcn.txBitmap |= (1 << bcnType);
}   

/*******************************************************************
* NAME :            HHAL_SetCsmaRegions
*
* DESCRIPTION :     Sets CSMA Regions - takes effect only next BPST.
*                   Startime,EndTime,Duration in units of ATU = 10.24uS
*                   Starttime is number of ATUs from EndTime of prev region.
*
* INPUTS :
*       PARAMETERS:
*           eRegFlag regFlag
*
* OUTPUTS :
*       None
*
*/
void HHAL_SetCsmaRegions(sCsmaRegion* regionArr, u8 regionCnt)
{
    uCSMARegionReg  csmaRegion;
    u8 i;
    u8 csmaRegionCnt;

    csmaRegionCnt = HYBRII_MAXSMAREGION_CNT < regionCnt ? HYBRII_MAXSMAREGION_CNT: regionCnt;
    //for (j=0; j<csmaRegionCnt; j++) {
   //     FM_Printf(FM_USER, "region %bu, start: 0x%x, endTime: 0x%x rxOnly: %bu\n", 
    //        j, regionArr[j].startTime, regionArr[j].endTime, regionArr[j].rxOnly);
   // }
    for( i=0 ; i<csmaRegionCnt ; i++ )
    {
        csmaRegion.s.csma_start_time_lo = 0;//regionArr[i].startTime & CSMAREG_STARTTMLO_MASK;
        csmaRegion.s.csma_start_time_hi = 0;//(regionArr[i].startTime & CSMAREG_STARTTMHI_MASK) >> CSMAREG_STARTTMHI_POS ;
        csmaRegion.s.csma_rxOnly      = regionArr[i].rxOnly;
        csmaRegion.s.csma_endtime_lo  = regionArr[i].endTime & CSMAREG_DURATIONLO_MASK;
        csmaRegion.s.csma_endtime_hi  = (regionArr[i].endTime & CSMAREG_DURATIONHI_MASK) >> CSMAREG_DURATIONHI_POS ;
        csmaRegion.s.csma_hybrid    = regionArr[i].hybridMd;
        switch(i)
        {
            case 0:
                WriteU32Reg(PLC_CSMAREGION0_REG, csmaRegion.reg);
                break;
            case 1:
                WriteU32Reg(PLC_CSMAREGION1_REG , csmaRegion.reg);
                break;
            case 2:
                WriteU32Reg(PLC_CSMAREGION2_REG, csmaRegion.reg);
                break;
            case 3:
                WriteU32Reg(PLC_CSMAREGION3_REG, csmaRegion.reg);
                break;
            case 4:
                WriteU32Reg(PLC_CSMAREGION4_REG, csmaRegion.reg);
                break;
            case 5:
                WriteU32Reg(PLC_CSMAREGION5_REG, csmaRegion.reg);
                break;
            default:
                {

                }

        }
    }
}


/*******************************************************************
* NAME :            HHAL_SetDiagMode
*
* DESCRIPTION :     Sets or clears PLC Tx/Rx Diagnostic mode operation.
*
* INPUTS :
*       PARAMETERS:
*           eRegFlag regFlag
*
* OUTPUTS :
*       None
*
*/
void HHAL_SetDiagMode(eRegFlag regFlag)
{
    uPlcStatusReg plcStatus;

    plcStatus.reg = ReadU32Reg(PLC_STATUS_REG);
    if(regFlag)
    {
        gHpgpHalCB.diagModeEnb = 1;
    }
    else
    {
        gHpgpHalCB.diagModeEnb = 0;
    }
    plcStatus.s.contTxDiag = regFlag;
    WriteU32Reg(PLC_STATUS_REG, plcStatus.reg);
}

/*******************************************************************
* NAME :            HHAL_SetDiagModeNoSack
*
* DESCRIPTION :     Enables or Disables ACK during Diagnostic mode operation.
*
* INPUTS :
*       PARAMETERS:
*           eRegFlag regFlag
*
* OUTPUTS :
*       None
*
*/
void HHAL_SetDiagModeNoSack(eRegFlag regFlag)
{
    uPlcStatusReg plcStatus;

    plcStatus.reg = ReadU32Reg(PLC_STATUS_REG);
    plcStatus.s.noSackDiag = regFlag;
    WriteU32Reg(PLC_STATUS_REG, plcStatus.reg);
}


/*******************************************************************
* NAME :            HHAL_GetPlcTxQFreeDescCnt
*
* DESCRIPTION :     Returns no: of dwords available in a given PLC CAP Tx Q.
*
* INPUTS :
*       PARAMETERS:
*           eRegFlag regFlag
*
* OUTPUTS :
*       None
*
*/
u8 HHAL_GetPlcTxQFreeDescCnt(eHpgpPlidValue plid)
{
    uCapTxQStatusReg capTxQStat;
    u8 freeDescCnt;
    u8* descCntArr;

    capTxQStat.reg = ReadU32Reg(PLC_QDSTATUS_REG);

    descCntArr = (u8*)&capTxQStat.reg;
    freeDescCnt = PLC_TXQ_DEPTH - descCntArr[plid];
    //printf("PLCTxQStatusReg = 0x%08lX, PLCTxQ[%bu].freeDescCnt = %bu\n", SwapU32(capTxQStat.reg),plid, freeDescCnt);
    return  freeDescCnt;
}


// txBcn starts with 16 bytes avFC & has to be zero padded
// bcnLen is always 136
// Always give a non-zero valid bpstoValueOffset: minimum is 16 byte AVFC + 12 byte BcnHdr + 1 byte NBE + 2 bytes BEHDR,BELEN of BPSTO BENTRY
//                                                            = 33 (ie., 34thth byte)
eStatus HHAL_AddNEK(u8 eks, u8 nek[PLC_AES_KEYLEN] )
{
    uAesLutAddrReg      aesLutAddr;
    uAesLutDataReg      aesLutData;
    uAesKeyLutAddrReg   aesKeyLutAddr;
    uAesKeyLutDataReg   aesKeyLutData;
    uAesCpuCmdStatReg   aesCpuCmd;

    u8                  i,j;
    u8                  arrIdx;

    arrIdx          = 0;

	FM_HexDump(FM_LINK,"NEK", nek,PLC_AES_KEYLEN);
    // Wait for Cpu Aes Lut access grant.
    aesCpuCmd.reg = ReadU32Reg(PLC_AESCPUCMDSTAT_REG);
    aesCpuCmd.s.cpuTblReq = 1;
    WriteU32Reg(PLC_AESCPUCMDSTAT_REG,aesCpuCmd.reg);
/*
    CHAL_DelayTicks(100);
    aesCpuCmd.reg = ReadU32Reg(PLC_AESCPUCMDSTAT_REG);
    if(!aesCpuCmd.s.cpuTblGnt)
    {
        printf("Add NEK failed\n");  //[YM] Add debug message of set NEK failure
        return STATUS_FAILURE;
    }
*/
    // Write Key to the Key LUT.
    for(i=0; i<4; i++)
    {
        aesKeyLutAddr.reg     = 0;
        aesKeyLutAddr.sNek.isNek = 1;
        aesKeyLutAddr.sNek.idx   = i;
        aesKeyLutAddr.sNek.eks   = (eks & 0x7);
        WriteU32Reg(PLC_AESKEYLUTADDR_REG, aesKeyLutAddr.reg );
       // FM_Printf(FM_LINFO,"Wrote keyAddr dw    0x%08lX to reg#0x%08lX\n", rtocl(aesKeyLutAddr.reg), (PLC_AESKEYLUTADDR_REG));
        for(j=0 ; j<4 ; j++)
        {
            aesKeyLutData.s.key[j] = nek[arrIdx++];       
        }
        WriteU32Reg(PLC_AESKEYLUTDATA_REG, aesKeyLutData.reg);
     //   FM_Printf(FM_LINFO,"Wrote key dw        0x%08lX to reg#0x%08lX\n", rtocl(aesKeyLutData.reg), (PLC_AESKEYLUTDATA_REG));
    }

    // Write Aes LUT Addr corres. to NEK.
    aesLutAddr.reg        = 0;
    aesLutAddr.sNek.eks   = eks;
    aesLutAddr.sNek.isNek = 1;
    WriteU32Reg(PLC_AESLUTADDR_REG, aesLutAddr.reg);
#ifndef RELEASE
    FM_Printf(FM_LINFO,"Wrote AesLutAddr dw 0x%08lX to reg#0x%08lX\n", rtocl(aesLutAddr.reg), (PLC_AESLUTADDR_REG));
#endif

    // Set the NEK as valid.
    aesLutData.reg        = 0;
    aesLutData.sNek.valid = 0xFF;
    WriteU32Reg(PLC_AESLUTDATA_REG, aesLutData.reg);    
#ifndef RELEASE
    FM_Printf(FM_LINFO,"Wrote keyAddr dw    0x%08lX to reg#0x%08lX\n\n", rtocl(aesLutData.reg), (PLC_AESLUTDATA_REG));
#endif

    // Release CPU Lock on AES LUT
    aesCpuCmd.reg = ReadU32Reg(PLC_AESCPUCMDSTAT_REG);
    aesCpuCmd.s.cpuTblGnt = 0;
    aesCpuCmd.s.cpuTblReq = 0;
    WriteU32Reg(PLC_AESCPUCMDSTAT_REG, aesCpuCmd.reg);

    return STATUS_SUCCESS;
}
 
eStatus HHAL_RemoveNEK(u8 eks)
{

    uAesLutAddrReg      aesLutAddr;
    uAesLutDataReg      aesLutData;
    uAesCpuCmdStatReg   aesCpuCmd;
    eStatus             status;

    // Wait for Cpu Aes Lut access grant.
    aesCpuCmd.reg = ReadU32Reg(PLC_AESCPUCMDSTAT_REG);
    aesCpuCmd.s.cpuTblReq = 1;
    WriteU32Reg(PLC_AESCPUCMDSTAT_REG,aesCpuCmd.reg);
/*
    CHAL_DelayTicks(100);
    aesCpuCmd.reg = ReadU32Reg(PLC_AESCPUCMDSTAT_REG);
    if(!aesCpuCmd.s.cpuTblGnt || eks > 7)
    {
        return STATUS_FAILURE;
    }
*/
    // Write Aes LUT Addr corres. to NEK.
    aesLutAddr.reg     = 0;
    aesLutAddr.sNek.eks   = eks;
    aesLutAddr.sNek.isNek = 1;
    WriteU32Reg(PLC_AESLUTADDR_REG, aesLutAddr.reg);

    // Set the NEK as invalid.
    aesLutData.reg        = 0;
    aesLutData.sNek.valid = 0x0;
    WriteU32Reg(PLC_AESLUTDATA_REG, aesLutData.reg); 

    // Release CPU Lock on AES LUT
    aesCpuCmd.reg = ReadU32Reg(PLC_AESCPUCMDSTAT_REG);
    aesCpuCmd.s.cpuTblGnt = 0;
    aesCpuCmd.s.cpuTblReq = 0;
    WriteU32Reg(PLC_AESCPUCMDSTAT_REG,aesCpuCmd.reg);

    return status;
}
 
eStatus HHAL_AddPPEK(u8 eks, u8 ppek[PLC_AES_KEYLEN], u8 tei)
{
    uAesLutAddrReg      aesLutAddr;
    uAesLutDataReg      aesLutData;
    uAesKeyLutAddrReg   aesKeyLutAddr;
    uAesKeyLutDataReg   aesKeyLutData;
    uAesCpuCmdStatReg   aesCpuCmd;
    eStatus             status;
    u8                  i,j;
    u8                  arrIdx;
    u8                  keyNum;
    u8                  region;

    arrIdx          = 0;

    status  = STATUS_SUCCESS;
    // Wait for Cpu Aes Lut access grant.
    aesCpuCmd.reg = ReadU32Reg(PLC_AESCPUCMDSTAT_REG);
    aesCpuCmd.s.cpuTblReq = 1;
    WriteU32Reg(PLC_AESCPUCMDSTAT_REG,aesCpuCmd.reg);
/*
    CHAL_DelayTicks(100);
    aesCpuCmd.reg = ReadU32Reg(PLC_AESCPUCMDSTAT_REG);
    if(!aesCpuCmd.s.cpuTblGnt)
    {
        return STATUS_FAILURE;
    }
*/
    if(eks != 8 && eks != 9 )  // EKS = 0b1000 or 0b1001
    {
        status = STATUS_FAILURE;
    }
    else
    {
         region                  = eks & 0x01;

        // Write AesLutAddr corres to the tei.
        aesLutAddr.reg          = 0;
        aesLutAddr.sPpek.tei    = tei;
        aesLutAddr.sPpek.isNek  = 0;
        WriteU32Reg(PLC_AESLUTADDR_REG, aesLutAddr.reg);
#ifndef RELEASE
        FM_Printf(FM_MINFO, "Wrote AesLutAddr dw    0x%08lX to   reg#0x%08lX\n", rtocl(aesLutAddr.reg), (PLC_AESLUTADDR_REG));
#endif
        // Read back AesLutData corres to the tei.
        aesLutData.reg          = ReadU32Reg(PLC_AESLUTDATA_REG);
#ifndef RELEASE
        FM_Printf(FM_MINFO, "Readback AesLutData dw 0x%08lX from reg#0x%08lX\n", rtocl(aesLutData.reg), (PLC_AESLUTDATA_REG));
#endif
        if(aesLutData.sPpek.valid)
        {
             // Overwrite the current Key. So retrive Index of current key.
             keyNum = aesLutData.sPpek.keyNum;
        }
        else
        {
            // Find an available Key Index to add the new PPEK.
            for(i=0 ; i<32 ; i++)
            {
                if( (gHpgpHalCB.ppekValidReg & BIT(i)) == 0)
                {
                    keyNum = i;
                    gHpgpHalCB.ppekValidReg |= BIT(i);
                    break;
                }                    
            }
            if(i == 32)
            {
                // All 32 KeyEntries in selected PPEK Region are currently in use.
                // PPEK Table full.
                status = STATUS_FAILURE;
            }
        }
    }

    if(status == STATUS_SUCCESS)
    {
        // Write Key to the Key LUT.
        for(i=0; i<4; i++)
        {
            aesKeyLutAddr.reg          = 0;
            aesKeyLutAddr.sPpek.isNek  = 0;
            aesKeyLutAddr.sPpek.idx    = i;
            aesKeyLutAddr.sPpek.keyNum = keyNum;
            aesKeyLutAddr.sPpek.region = region;
            WriteU32Reg(PLC_AESKEYLUTADDR_REG, aesKeyLutAddr.reg );
   //         FM_Printf(FM_MINFO, "Wrote keyAddr dw       0x%08lX to reg#0x%08lX\n", rtocl(aesKeyLutAddr.reg), (PLC_AESKEYLUTADDR_REG));
            for(j=0 ; j<4 ; j++)
            {
                aesKeyLutData.s.key[j] = ppek[arrIdx++];       
            }
            WriteU32Reg(PLC_AESKEYLUTDATA_REG, aesKeyLutData.reg);
   //         FM_Printf(FM_MINFO, "Wrote key dw           0x%08lX to reg#0x%08lX\n", rtocl(aesKeyLutData.reg), (PLC_AESKEYLUTDATA_REG));
        }

        // Write Aes LUT Addr corres to PPEK.
        aesLutAddr.reg          = 0;
        aesLutAddr.sPpek.tei    = tei;
        aesLutAddr.sPpek.isNek  = 0;
        WriteU32Reg(PLC_AESLUTADDR_REG, aesLutAddr.reg);
  //      FM_Printf(FM_MINFO, "Wrote AesLutAddr dw    0x%08lX to reg#0x%08lX\n", rtocl(aesLutAddr.reg), (PLC_AESLUTADDR_REG));
    
        // Set the PPEK valid.
        //aesLutData.reg           = 0;
        aesLutData.sPpek.keyNum  = keyNum;
        aesLutData.sPpek.valid   = 1;
        if(region == 0)
        {
            aesLutData.sPpek.region0 = 1;
        }
        else
        {
            aesLutData.sPpek.region1 = 1;
        }

        WriteU32Reg(PLC_AESLUTDATA_REG, aesLutData.reg);    
  //      FM_Printf(FM_MINFO, "Wrote AesLutData dw    0x%08lX to reg#0x%08lX\n", rtocl(aesLutData.reg), (PLC_AESLUTDATA_REG));
    }

    // Release CPU Lock on AES LUT
    aesCpuCmd.reg = ReadU32Reg(PLC_AESCPUCMDSTAT_REG);
    aesCpuCmd.s.cpuTblGnt = 0;
    aesCpuCmd.s.cpuTblReq = 0;
    WriteU32Reg(PLC_AESCPUCMDSTAT_REG,aesCpuCmd.reg);

    return status;
}


eStatus HHAL_RemovePPEK(u8 eks, u8 tei)
{
    uAesLutAddrReg      aesLutAddr;
    uAesLutDataReg      aesLutData;
    uAesCpuCmdStatReg   aesCpuCmd;
    eStatus             status;
    u8                  region;

    // Wait for Cpu Aes Lut access grant.
    aesCpuCmd.reg = ReadU32Reg(PLC_AESCPUCMDSTAT_REG);
    aesCpuCmd.s.cpuTblReq = 1;
    WriteU32Reg(PLC_AESCPUCMDSTAT_REG,aesCpuCmd.reg);

    CHAL_DelayTicks(100);
    aesCpuCmd.reg = ReadU32Reg(PLC_AESCPUCMDSTAT_REG);
    if(eks < 8 || eks > 9)
    {
        return STATUS_FAILURE;
    }
    region                  = eks & 0x01;

    // Write Aes LUT Addr corres. to PPEK.
    aesLutAddr.reg         = 0;
    aesLutAddr.sPpek.tei   = tei;
    aesLutAddr.sPpek.isNek = 0;
    WriteU32Reg(PLC_AESLUTADDR_REG, aesLutAddr.reg);

    // Read back AesLutData corres to the tei.
    aesLutData.reg          = ReadU32Reg(PLC_AESLUTDATA_REG);
//    FM_Printf(FM_MINFO,"Read back dw 0x%08lX from reg#0x%08lX\n", rtocl(aesLutData.reg), (PLC_AESLUTDATA_REG));

    if(aesLutData.sPpek.valid)
    {
        if(region == 0)
        {
            aesLutData.sPpek.region0 = 0;
        }
        else
        {
            aesLutData.sPpek.region1 = 0;
        }
        if(aesLutData.sPpek.region0 == 0 && aesLutData.sPpek.region1 == 0)
        {
             // Set the PPEK as invalid.
            aesLutData.reg         = 0;
            aesLutData.sPpek.valid = 0x0;
            
        }
        WriteU32Reg(PLC_AESLUTDATA_REG, aesLutAddr.reg); 
    }
    else
    {
        status = STATUS_FAILURE;
    }

    // Release CPU Lock on AES LUT
    aesCpuCmd.reg = ReadU32Reg(PLC_AESCPUCMDSTAT_REG);
    aesCpuCmd.s.cpuTblGnt = 0;
    aesCpuCmd.s.cpuTblReq = 0;
    WriteU32Reg(PLC_AESCPUCMDSTAT_REG,aesCpuCmd.reg);

    return status;
}

#ifdef HPGP_HAL_TEST
void HHAL_SetDefAddrConfig()
{
#if defined(Flash_Config) && defined(Z_P_BRIDGE)    

        gHpgpHalCB.selfTei   = sysConfig.defaultSTEI;
        gHpgpHalCB.remoteTei = sysConfig.defaultDTEI;
		gHpgpHalCB.snid      = sysConfig.defaultNID[0];

#else	
	if(gHpgpHalCB.devMode == DEV_MODE_CCO)
    {
        gHpgpHalCB.selfTei   = HYBRII_DEFAULT_TEICCO;
        gHpgpHalCB.remoteTei = HYBRII_DEFAULT_TEISTA;
    }else
    {
        gHpgpHalCB.selfTei   = HYBRII_DEFAULT_TEISTA;
        gHpgpHalCB.remoteTei = HYBRII_DEFAULT_TEICCO;
    }
    gHpgpHalCB.snid      = HYBRII_DEFAULT_SNID;
#endif  //Flash_Config && Z_P_BRIDGE
#ifndef QCA
	
    HHAL_SetTei(gHpgpHalCB.selfTei);

    HHAL_SetSnid(gHpgpHalCB.snid); // this is done because when we communicate with quqlcomm or other chip snid should be set once we receive bcn from cco and should not get set at power on because 
#endif
        gHpgpHalCB.syncComplete   = 1;
}
#endif	//HPGP_HAL_TEST

void HHAL_ClearBcnInit()
{
    gHpgpHalCB.bcnInitDone = 0;
    //FM_Printf(FM_MINFO, "HHAL_ClearBcnInit\n");

}

void HHAL_SetBcnInit()
{
    gHpgpHalCB.bcnInitDone = 1;
    FM_Printf(FM_MINFO, "HHAL_SetBcnInit\n");

}

void HHAL_PowerSaveConfig()
{
    uPlcLineControlReg plcLineCtrl;
	
	plcLineCtrl.reg = ReadU32Reg(PLC_POWERSAVE_REG);
	
}

/*******************************************************************
* NAME :            HHAL_SetDevMode
*
* DESCRIPTION :     Sets Default PLC Device configuration.
*
* INPUTS :
*       PARAMETERS:
*           eRegFlag regFlag
*
* OUTPUTS :
*       None
*
*/

void HHAL_SetDevMode(eDevMode devMode, eLineMode lineMode)
{


    sCsmaRegion      csmaRegArr[5]; 
    uPlcLineControlReg plcLineCtrl;
    
   
  
    memset(csmaRegArr, 0, 5*sizeof(sCsmaRegion));
    // Added by Varsha
 
 	FM_Printf(FM_MINFO, "lineMode %bu\n", lineMode);
	
    if(lineMode == LINE_MODE_DC)
    {
        gHpgpHalCB.lineMode = LINE_MODE_DC;
        gHpgpHalCB.curBcnPer = PLC_DC_BP_LEN;
        //gHpgpHalCB.curBcnPer = PLC_AC_BP_LEN;
        WriteU32Reg(PLC_DCLINECYCLE_REG, ctorl(PLC_DC_LINE_CYCLE_FREQENCY));
		WriteU32Reg(PLC_SWBCNPERAVG_REG, ctorl((PLC_DC_BP_LEN >> 1)));
        WriteU32Reg(PLC_HWBCNPERCUR_REG, ctorl(PLC_DC_BP_LEN));
        plcLineCtrl.reg = ReadU32Reg(PLC_LINECTRL_REG);
        plcLineCtrl.s.dcmode = 1;
        WriteU32Reg(PLC_LINECTRL_REG, plcLineCtrl.reg);
         gsyncTimeout = DC_SYNC_TIMEOUT;
         gBcnMissingRescanCnt = DC_BCN_MISSING_RESCAN_CNT;  //60*4= 240= 240/40= 6 bcn missing
        //gHpgpHalCB.bPerAvgInitDone = 1;
    }
    else
    {
        gHpgpHalCB.lineMode = LINE_MODE_AC;
	    gHpgpHalCB.curBcnPer = PLC_AC_BP_LEN;
	
        WriteU32Reg(PLC_SWBCNPERAVG_REG, ctorl((PLC_AC_BP_LEN >> 1)));
        WriteU32Reg(PLC_SWBCNPERAVG_REG, ctorl((PLC_AC_BP_LEN >> 1)));
        
        set_plc_paramter(PLC_MaxPeran,PLC_MAX_AC_BPLEN);
        set_plc_paramter(PLC_MinPeran,PLC_MIN_AC_BPLEN);
        
        plcLineCtrl.reg = ReadU32Reg(PLC_LINECTRL_REG);
        plcLineCtrl.s.dcmode = 0;
        WriteU32Reg(PLC_LINECTRL_REG, plcLineCtrl.reg);
         gsyncTimeout = AC_SYNC_TIMEOUT;
         gBcnMissingRescanCnt = AC_BCN_MISSING_RESCAN_CNT;  //30*4= 120= 120/33.33= 3 bcn missing
	    set_plc_paramter(PLC_MinPeran,PLC_MIN_AC_BPLEN);
    }
#ifdef MCCO
	if(devMode == DEV_MODE_PCCO)
	{
		gHpgpHalCB.devMode	 = DEV_MODE_PCCO;
		// Write PLC Devmode register.
		//cpuSwStatus.reg = ReadU32Reg(CPU_SWSTATUS_REG);

		HHAL_SetPlcDevMode(CCO_COORDINATING);
	

		gHpgpHalCB.perSumCnt	   = 0;
		gHpgpHalCB.bPerAvgInitDone = 0;
		
#ifdef HPGP_HAL_TEST
		setCSMA_onCCO();										  
#endif
	}
	else
#endif
 
	if(devMode == DEV_MODE_CCO)
    {
        gHpgpHalCB.devMode   = DEV_MODE_CCO;
        //FM_Printf(FM_MINFO, "HHAL_SetDevMode: Dev Mode CCo\n");

        // Write PLC Devmode register.
        //cpuSwStatus.reg = ReadU32Reg(CPU_SWSTATUS_REG);
        if(lineMode == LINE_MODE_AC)
        {
            //cpuSwStatus.s.plcDevMode = CCO_ACLINE;            
            HHAL_SetPlcDevMode(CCO_ACLINE);
            // Set AC cycle frequency
            gHpgpHalCB.lineMode = LINE_MODE_AC;
/*#ifdef AC_LINECYCLE_50HZ
            HHAL_SetACLine50HzFlag(REG_FLAG_SET);
#else                   
            HHAL_SetACLine50HzFlag(REG_FLAG_CLR);
#endif   */ //AC_LINECYCLE_50HZ         
            gHpgpHalCB.perSumCnt       = 0;
            gHpgpHalCB.bPerAvgInitDone = 0;
        }
        else
        {
            
            HHAL_SetPlcDevMode(CCO_DCLINE);
            //HHAL_SetACLine50HzFlag(REG_FLAG_SET);
        }
#ifdef HPGP_HAL_TEST
        setCSMA_onCCO();                                          
#endif
    }
    else
    {
        gHpgpHalCB.devMode   = DEV_MODE_STA;
        //FM_Printf(FM_MINFO, "HHAL_SetDevMode: Dev Mode Sta\n");


        // Write PLC Devmode register.
        if(lineMode == LINE_MODE_AC)
        {
            HHAL_SetPlcDevMode(STA_CSMANW);
            gHpgpHalCB.lineMode = LINE_MODE_AC;
/*#ifdef AC_LINECYCLE_50HZ
            HHAL_SetACLine50HzFlag(REG_FLAG_SET);
#else                   
            HHAL_SetACLine50HzFlag(REG_FLAG_CLR);
#endif   */ //AC_LINECYCLE_50HZ       
       
        }
        else
        {
            HHAL_SetPlcDevMode(STA_DCLINE);
            gHpgpHalCB.lineMode = LINE_MODE_DC;
        }     
#ifdef HPGP_HAL_TEST		
		 setCSMA_onSTA();
#endif

				  
    }
	
   /* csmaRegArr[0].startTime  = 0;
    csmaRegArr[0].bcnRegion  = (devMode == DEV_MODE_STA) ? 1 : 0;
    csmaRegArr[0].duration   = 0x2FF;
    csmaRegArr[0].hybridMd   = 1;    
    
    csmaRegArr[2].startTime  = 0;
    csmaRegArr[2].bcnRegion  = (devMode == DEV_MODE_STA ) ? 1 : 0;
    csmaRegArr[2].duration   = gHpgpHalCB.bcnInitDone ? 0 : 0x5A3;
    csmaRegArr[2].hybridMd   = gHpgpHalCB.bcnInitDone ? 0 : 1;  
    
    csmaRegArr[3].startTime  = 0;
    csmaRegArr[3].bcnRegion  = 0;
    csmaRegArr[3].duration   = 0;//0x04A9//0xFF;
    csmaRegArr[3].hybridMd   = 0;//1;                         
    // Write CSMA Regions - mixed mode.
    if(lineMode == LINE_MODE_AC) 
    {

    csmaRegArr[1].startTime  = 0;
    csmaRegArr[1].bcnRegion  = 0;
    csmaRegArr[1].duration   = 0x4A1;
    csmaRegArr[1].hybridMd   = 1;
//    WriteU32Reg(PLC_SS1TIMEOUT_REG, ctorl(PLC_AC_BP_LEN - PLC_LATE_BCN_SYNC_THRES));
    set_plc_paramter(PLC_CPUSCAN_TIMEOUT_SEL, PLC_AC_BP_LEN - PLC_LATE_BCN_SYNC_THRES);
    }
    else
    {
    csmaRegArr[1].startTime  = 0;
    csmaRegArr[1].bcnRegion  = 0;
    csmaRegArr[1].duration   = 0x6A1;
    csmaRegArr[1].hybridMd   = 1;
//    WriteU32Reg(PLC_SS1TIMEOUT_REG, ctorl(PLC_DC_BP_LEN - PLC_LATE_BCN_SYNC_THRES));
    set_plc_paramter(PLC_CPUSCAN_TIMEOUT_SEL, PLC_DC_BP_LEN - PLC_LATE_BCN_SYNC_THRES);

    }
    //HHAL_SetCsmaRegions(csmaRegArr,4);
   
    WriteU8Reg(0x47F, 0x4);  // PHY AGC Threshold - For auto switch robo on RX
    */
}




// Write AFE registers wian SPI registers in PHY address space
void HHAL_AFEInit()
{
	mac_utils_spi_write(0x4, 0x32);// vco fix kiran based on Jenny's recommendation
#ifdef B_ASICPLC
    // [YM] 1.8GHz VCO Calibrate
    mac_utils_spi_write(0x4, 0x20);
    mac_utils_spi_write(0x4, 0x4);   //[YM] temporary disable AFE calibration
#endif	

#ifdef B2_ASICPLC
    // [YM] 1.8GHz VCO Calibrate
    mac_utils_spi_write(0x4, 0x20);
    mac_utils_spi_write(0x4, 0x24);   //[YM] temporary disable AFE calibration    
#endif	

/*
#if defined(HYBRII_HPGP) && defined(HYBRII_802154)
    // Program fix gain for PLC
#ifndef HYBRII_B
    mac_utils_spi_write(0x3a, 0x0a);
    mac_utils_spi_write(0x3b, 0x72);
#endif
#endif	 //defined(HYBRII_HPGP) && defined(HYBRII_802154)
*/
#ifdef _LED_DEMO_
    WriteU8Reg(0x406, 0x00);
    CHAL_DelayTicks(SPI_WRITE_DELAY);
    WriteU8Reg(0x406, 0x7F);
#endif	//_LED_DEMO_
}


void hal_hpgp_soft_reset ()
{
    uPlcStatusReg  plcStatus;
    
    plcStatus.reg = ReadU32Reg(PLC_STATUS_REG);
    plcStatus.s.txSoftReset = 1;
    plcStatus.s.rxSoftReset = 1;

    WriteU32Reg(PLC_STATUS_REG, plcStatus.reg);

    CHAL_DelayTicks(100);
    plcStatus.s.txSoftReset = 0;
    plcStatus.s.rxSoftReset = 0;

    WriteU32Reg(PLC_STATUS_REG, plcStatus.reg);
}



   
eStatus HHAL_SyncNet(u8 *bpsto) 
{
    uBpstoReg    bpstoReg;
    //u32          ntb1,ntb2;
    u32          bpstoVal; //, latency;
    u32          bpst;
    u32          ss1;
//    u32          offset;
//    u32          ss2MinusSs1;
//    u32 bps;
    u32 NTB_delta;
//    u32 x;
//    u32 E_CCOBTS;
    u32 BcnPer;
    u32 CCOBpst;
    u32 RetrievedBTS;
    u8 rollover_snapshot1 = 0;
    u8 rollover_retriveBTS = 0;
    u32 ToatalLatency;
	u32 missingBcnThreshold;
	u32 minVal=PLC_MIN_AC_BPLEN; // defaulted to AC BP Len (60 Hz)
	u32 maxVal=PLC_MAX_AC_BPLEN;

	eStatus      ret = STATUS_FAILURE;

                     

    if (!gHpgpHalCB.nwSelected)
        return ret;    

 
  
    // Process further if the beacon has been snapshot.
    if(gHpgpHalCB.snapShot)
    {		
        bpstoReg.s.bpsto0 = bpsto[0];
        bpstoReg.s.bpsto1 = bpsto[1];
        bpstoReg.s.bpsto2 = bpsto[2];

        bpstoVal = ((u32)(bpsto[2])<<16) + ((u32)(bpsto[1])<<8) + (u32)bpsto[0];
        //WriteU32Reg(PLC_BPSTOFFSET_REG, ctorl(bpstoVal));     
        
        ss1   = rtocl(ReadU32Reg(PLC_BCNSNAPSHOT1_REG));
 	   
         //ss2 = rtocl(ReadU32Reg(PLC_NTB_REG)); 
        // printf("\n ssdif= %lu", ((ss1-oldss1) * 40));
         

        if(gHpgpHalCB.syncComplete)//normal operation
        {
             if(ss1 < oldss1)
                rollover_snapshot1 = 1;
        }   
        oldss1 = ss1;
        RetrievedBTS = gCCO_BTS + PLC_PHY_RXLATENCY_FOR_TCC3;  //PLC_PHY_RXLATENCY_FOR_TCC2;//normal as well as cpu scan operation
         
         
        if(gHpgpHalCB.syncComplete)//normal operation
        {
            if(RetrievedBTS < oldRetrievedBTS)
                rollover_retriveBTS = 1;
        } 
        
        oldRetrievedBTS = RetrievedBTS; 
        
        if(RetrievedBTS > ss1)  //sta slower then cco
        {
            if(rollover_retriveBTS)
            {
                NTB_delta = RetrievedBTS + (0xffffffff - ss1);
            }
            else 
            {
                NTB_delta = (RetrievedBTS - ss1);
             
            }
            if(NTB_delta > 10)
            {
                gHpgpHalCB.halStats.STAlagCCOCount++;
            }
        
        }
        else  //sta is faster then cco so take 2's complement
        {
        
            if(rollover_snapshot1 )
            {
                NTB_delta = 0xffffffff - (ss1 + (0xffffffff - RetrievedBTS)) + 1;
            }
            else  
            {
                NTB_delta = 0xffffffff - (ss1 - RetrievedBTS) + 1;     //taken 2's complement
                
               
            }
            if((ss1 - RetrievedBTS) > 10)
            {
                gHpgpHalCB.halStats.STAleadCCOCount++;
            } 
        }
        
//        WriteU32Reg( PLC_NTBADJ_REG, ctorl(NTB_delta));  move this down to after check BCN_PER_THRESHOLD - Tri
         
      
                                           
        //ss1   = rtocl(ReadU32Reg(PLC_BCNSNAPSHOT1_REG));

       
        //ToatalLatency = (PLC_PHY_RXLATENCY_FOR_TCC2 + bpstoVal);
        ToatalLatency = (PLC_PHY_RXLATENCY_FOR_TCC3+ bpstoVal);
     


#ifdef QCA    
        ss1 += NTB_delta;//rajan
#endif             

        if( ss1 > ToatalLatency)
        {
            if(rollover_snapshot1)
                CCOBpst = ss1 + (0xffffffff - ToatalLatency); 
             else
                CCOBpst = (ss1 - ToatalLatency);
        }
        else
        {
              CCOBpst = ss1 + (0xffffffff - ToatalLatency + 1); 
        }
        
       
       if(CCOBpst > OldCCOBpst)
        BcnPer =  (CCOBpst - OldCCOBpst);
       else
            BcnPer = ((0xffffffff - OldCCOBpst) + CCOBpst);
       
       OldCCOBpst = CCOBpst; 
      // printf("\n bcn per = %lu",BcnPer); 

       //missingBcnThreshold = gHpgpHalCB.curBcnPer * 1.5;		// ie. 1.5 bcn in NTB units               
	   missingBcnThreshold = ((gHpgpHalCB.curBcnPer * 3)>>1);
       if(BcnPer > missingBcnThreshold)
       {
	   		// If we miss BCN_PER_THRESHOLD, skip the 1st good bcn
			// will start calculating beginning of 2nd good bcn
//   	       printf("\n Hit HI BCN THRESHOLD. BcnPer = %lu, missingBcnThreshold=%lu", BcnPer, missingBcnThreshold);
			
		   return STATUS_SUCCESS; 

       }

	   // make sure that the value of BcnPER fall within acceptable range
#ifdef FREQ_DETECT
	   if (gHpgpHalCB.curBcnPer == PLC_DC_BP_LEN_50HZ)
	   {
            minVal = MIN_50HZ_BPLEN;
            maxVal = MAX_50HZ_BPLEN; 
	   }
#endif	    
	   if ((BcnPer >= minVal) && (BcnPer <= maxVal))
       {
	   		// calculated bcnPER must be within acceptable range
            gBcnPer = BcnPer;
       }
       else
       {
	   		// if not within range, use default value
#ifndef MPER            
//            printf("BCNPER = 0x%lx\n", BcnPer);
#endif
	       gBcnPer = gHpgpHalCB.curBcnPer;              
       }
EA = 0;        
        WriteU32Reg( PLC_NTBADJ_REG, ctorl(NTB_delta));  

        //whenever we receives bcn we write sw per avg reg = bcnper/2 
        WriteU32Reg(PLC_SWBCNPERAVG_REG, ctorl((gBcnPer >> 1))); 
EA = 1;         
       
       // ss2MinusSs1 = (ss2 > ss1) ? (ss2 - ss1) : (0xFFFFFFFF - ss1 + ss2);
       // if(ss2MinusSs1 <= (gBcnPer - 1000) )
        {
             bpst = CCOBpst + gBcnPer;// + var1;
             gbpst1 = bpst;
               

#ifdef QCA             
             bpst -= NTB_delta; //rajan
#endif               
				EA = 0;        
				WriteU32Reg(PLC_BPST_REG, ctorl(bpst)); 
				EA = 1;			 

             gHpgpHalCB.halStats.BcnSyncCnt++;  
             gtimer2 = 0;//reset second timer at every bcn receive
             gtimer1 = 0; 
             misscnt = 0;
         }  
            
        }

       
        if(gHpgpHalCB.syncComplete)
        {
            ret = STATUS_SUCCESS;
        }

        if(!gHpgpHalCB.syncComplete && (gHpgpHalCB.halStats.BcnSyncCnt))
        {
            gHpgpHalCB.syncComplete = 1;
           // CHAL_DelayTicks(80);// Kiran: need to review. Blocking
            HHAL_SetSWStatReqScanFlag(REG_FLAG_CLR);  
           // CHAL_DelayTicks(20);// Kiran: need to review    TODO RAJAN
#ifdef MCCO 

			if (gHpgpHalCB.devMode == DEV_MODE_PCCO)
			{

				gHpgpHalCB.bPerAvgInitDone = 1;
			}
#endif

        }      
 
         //gtimer2 = 0;//reset second timer at every bcn receive
         //gtimer1 = 0;
         
         return ret;
}
// end of Hybrii B


#if 0

void HHAL_ProcessPlcTxDone()
{

    u8 tailIdx;
    sTxFrmSwDesc  *lpPlcTxFrmSwDesc;
    sHpgpHalCB *hhalCb;	
#ifndef HPGP_HAL_TEST    
    sHaLayer *hal;
#endif  	 //HPGP_HAL_TEST
    volatile u32 val;
    volatile u8 *val1; 

    
#ifdef HPGP_HAL_TEST
    hhalCb = &gHpgpHalCB;
#else
    hal = (sHaLayer*)HOMEPLUG_GetHal();

    hhalCb = hal->hhalCb;
#endif	   //HPGP_HAL_TEST

  
    val = ReadU32Reg(PLC_MEDIUMINTSTATUS_REG);

    val1 = (u8*)&val;

    if ((*val1) & 0x40)
    {
#ifdef ETH_BRDG_DEBUG
		numTxDoneInts++;
#endif
        WriteU8Reg(PLC_MEDIUMINTSTATUS_REG, 0x40);         
           
                    
        tailIdx = (hhalCb->gPendingTail & 0x7F);
        lpPlcTxFrmSwDesc =  &hhalCb->gPending[tailIdx];

        if (lpPlcTxFrmSwDesc->frmInfo.plc.status == PLC_TX_PENDING)
        {
            lpPlcTxFrmSwDesc->frmInfo.plc.status = PLC_TX_DONE;            
#ifdef DEBUG_DATAPATH                        
            if (sigDbg)
                FM_Printf(FM_ERROR," plc txDone t:%bu\n",hhalCb->gPendingTail);
#endif   //DEBUG_DATAPATH         
        }

        
    }

}

#endif


void plc_init_parameters ()
{
    uPlcIfs0Reg         plcIfs0;
    uPlcIfs1Reg         plcIfs1;

    //set_plc_paramter(PLC_PHYLATENCY_SEL,  PLC_PHYLATENCY_INIT_VALUE); 
	// Added by Varsha
	
//      u32 laten_initval;
//     u32 rx_lat = PLC_PHY_RXLATENCY;
//     u32 tx_lat = PLC_PHY_TXLATENCY;
    
	set_plc_paramter(PLC_TIMINGPARAM_SEL, PLC_TIMINGPARAM_INIT_VALUE);

    set_plc_paramter(PLC_MPIRXTIMEOUT_SEL, PLC_MPIRXTIMOUT_INIT_VALUE);

    set_plc_paramter(PLC_MPITXTIMEOUT_SEL, PLC_MPITXTIMOUT_INIT_VALUE);

    set_plc_paramter(PLC_CPUSCAN_TIMEOUT_SEL, PLC_CPUSCANTIMOUT_INIT_VALUE);

    set_plc_paramter(PLC_500USCNT_SEL, PLC_500US_COUNT);

	//Added by Varsha
	
     /*  laten_initval = PLC_PHY_RXLATENCY;
     laten_initval = laten_initval << 16;
     laten_initval += PLC_PHY_TXLATENCY;
     printf("\n latenval = %lu", laten_initval); */
     set_plc_paramter(PLC_PHYLATENCY_SEL,   TX_RXLatency_TCC[2]);  //tcc =3
	
    // IFS Registers 
    plcIfs0.reg          = 0;
    plcIfs0.s.clksPerUs  = HPGP_CLKsPerUs_DEF;
    plcIfs0.s.cifs_av    = HPGP_CIFSAV_DEF;
    plcIfs0.s.rifs_av    = HPGP_RIFSAV_DEF;
    plcIfs0.s.b2bifs     = HPGP_B2BIFS_DEF;
    set_plc_paramter(PLC_IFS0_SEL, plcIfs0.reg);

	
    plcIfs1.reg          = 0;
    plcIfs1.s.bifs       = HPGP_BIFS_DEF;
    plcIfs1.s.aifs       = HPGP_AIFS_DEF;
    set_plc_paramter(PLC_IFS1_SEL, plcIfs1.reg);

		
    set_plc_paramter(PLC_IFS2_SEL, PLC_IFS2_INIT_VALUE);

    set_plc_paramter(PLC_IFS3_SEL, PLC_IFS3_INIT_VALUE);


	// FL_AV Registers
    // Adding 1 to the actual values, since MAC HW is using a "<" comparison with each of these feilds.
    set_plc_paramter(PLC_FLAV0_SEL, PLC_FLAV0_INIT_VALUE);
	
    set_plc_paramter(PLC_FLAV1_SEL, PLC_FLAV1_INIT_VALUE);

    set_plc_paramter(PLC_FLAV2_SEL, PLC_FLAV2_INIT_VALUE);

    set_plc_paramter(PLC_FLAV3_SEL, PLC_FLAV3_INIT_VALUE);

	set_plc_paramter(PLC_FLAV4_SEL, PLC_FLAV4_INIT_VALUE);


	// 0x11 is used for Early Wake Count, 0x12 and 0x13 are not being used
//    set_plc_paramter(PLC_CRSRDYDLY0_SEL, PLC_CRSDLY0_1_INIT_VALUE); 
//    set_plc_paramter(PLC_CRSRDYDLY1_SEL, PLC_CRSDLY2_3_INIT_VALUE);
//    set_plc_paramter(PLC_CRSRDYDLY2_SEL, PLC_CRSDLY3_PRS_INIT_VALUE);
    set_plc_paramter(PLC_CRSRDYDLY0_SEL, 0); 

    set_plc_paramter(PLC_WAITCRS_SEL, PLC_WAITCRS_INIT_VALUE);

    set_plc_paramter(PLC_TXRX_TURNAROUND_SEL, PLC_TXRX_TURNAROUND_INIT_VALUE);

	set_plc_paramter(PLC_PKTTIME_SEL, PLC_PKTTIME_INIT_VALUE);
	set_plc_paramter(PLC_EIFS_SEL, PLC_EIFS_INIT_VALUE);
	set_plc_paramter(PLC_VCSPARAM0_SEL, PLC_VCSPARAM0_INIT_VALUE);
	set_plc_paramter(PLC_VCSPARAM1_SEL, PLC_VCSPARAM1_INIT_VALUE);
	set_plc_paramter(PLC_VCSPARAM2_SEL, PLC_VCSPARAM2_INIT_VALUE);
    set_plc_paramter(PLC_MaxPeran,PLC_MaxPeran_INIT_VALUE);
	set_plc_paramter(PLC_MinPeran,PLC_MinPeran_INIT_VALUE);
}

#ifdef HPGP_HAL_TEST
void HHAL_Init()
#else
void HHAL_Init(sHaLayer *hal, sHpgpHalCB **ppHhalCb)
#endif
{
    uPlcStatusReg       plcStatus;
    uPlcLineControlReg  plcLineCtrl;	
	
//	uPlcDevCtrlReg      plcDeviceCtrl;
    uAesCpuCmdStatReg   aesCpuCmd;
	uPlcSMCounterReg    plcSMCnt;
    u16 i;

//	u32                RegValue;
#ifdef POWERSAVE
	u8 byteVal;
#endif

#ifndef HPGP_HAL_TEST
    gHpgpHalCB.hal = hal;
    
    *ppHhalCb = &gHpgpHalCB;
#endif 	  //HPGP_HAL_TEST

    plcStatus.reg              = 0;
    plcStatus.s.crsBypass      = 0;
    plcStatus.s.soundEnable    = 0;
    plcStatus.s.plcTxQSwCtrl   = 1;	
#ifndef MPER
    plcStatus.s.randomBackoff = 1;  // Upper mac discovey beacon collision with datapath test. so have to enable randomBackoff
#else
    plcStatus.s.randomBackoff = 0; 
#endif
	//plcStatus.s.plcTxQHwCtrl   = 1;
	plcStatus.s.mpiChkFlush    = 1;
#ifdef _AES_SW_
    plcStatus.s.hwAesResetEnb  = 1;
#endif
// [YM] plcStatus.s.plcRxEnSwCtrl is used only on Hybrii_A 
//    plcStatus.s.plcRxEnSwCtrl  = 1;
    WriteU32Reg(PLC_STATUS_REG, plcStatus.reg);
	plcStatus.s.soundEnable    = 1;
	WriteU32Reg(PLC_STATUS_REG, plcStatus.reg);
    // Program PLC_PHYLATENCY_SEL Init Value
    plc_init_parameters();

 
	
     plcLineCtrl.reg = 0;
     plcLineCtrl.s.usTimerMark = 0;
     plcLineCtrl.s.cpuFreq = 0;//25 mhz
     //plcLineCtrl.s.dcmode = 1;
     plcLineCtrl.s.hybernate = 1;

#ifdef PLC_SW_SYNC
     plcLineCtrl.s.swSync = 1;
     plcLineCtrl.s.swPER = 1;
//      plcLineCtrl.s.swSync = 0;
    // plcLineCtrl.s.swPER = 0;
#endif
    WriteU32Reg(PLC_LINECTRL_REG, plcLineCtrl.reg); 
	
    WriteU32Reg(PLC_MAXRETRYCNT_REG, ctorl(PLC_MAXRETRYCNT_INIT_VALUE));
	
	WriteU32Reg(CPU_ETHERSA0_REG, ctorl(CPU_ETHERSA0_INIT_VALUE));
	
	WriteU32Reg(CPU_ETHERSA1_REG, ctorl(CPU_ETHERSA1_INIT_VALUE));
	
	WriteU8Reg(ETHMAC_MACMODE_REG, 0);
	WriteU8Reg(ETHMAC_TXDEFPARM_REG, 0xC);
	WriteU8Reg(ETHMAC_TXCTL1_REG, 0x11);
	WriteU8Reg(ETHMAC_RXCTL_REG, 0xF);
	
	WriteU32Reg(PLC_EARLYHPGPBPINT_REG, ctorl(PLC_EARLYHPGPBPINT_INIT_VALUE));

   
	   // Initialize SSN Memory - 256*4 DWORDS
	   for(i=0;i<1024;i++) 
	   {
		   WriteU32Reg(PLC_SSNMEMADDR_REG, ctorl(i));
		   WriteU32Reg(PLC_SSNMEMDATA_REG, 0); 
	   }
   
	   // Wait for Cpu Aes Lut access grant
	   aesCpuCmd.reg = ReadU32Reg(PLC_AESCPUCMDSTAT_REG);
	   aesCpuCmd.s.cpuTblReq = 1;
	   WriteU32Reg(PLC_AESCPUCMDSTAT_REG,aesCpuCmd.reg);
   
	   CHAL_DelayTicks(100);
   
	   // Initialize PPEK Addr table  - 256 bytes
	   for(i=0;i<256;i++)  
	   {
		   WriteU32Reg(PLC_AESLUTADDR_REG, ctorl(i));
		   WriteU32Reg(PLC_AESLUTDATA_REG, 0);
	   }
   
	   // Initialize NEK Valid table - 8 bytes
	   for(i=0;i<8;i++)  
	   {
		   WriteU32Reg(PLC_AESLUTADDR_REG, ctorl(256+i));
		   WriteU32Reg(PLC_AESLUTDATA_REG, 0);
	   }
   
	   // Initilaize AES Key Table	- 1024 DWORDS
	   // only 1152 DWORDS actually used to store keys
	   for(i=0;i<1024;i++) 
	   {
		   WriteU32Reg(PLC_AESKEYLUTADDR_REG, ctorl(i));
		   WriteU32Reg(PLC_AESKEYLUTDATA_REG, 0);
	   }
   
	   // Release CPU Lock on AES LUT
	   aesCpuCmd.reg = ReadU32Reg(PLC_AESCPUCMDSTAT_REG);
	   aesCpuCmd.s.cpuTblGnt = 0;
	   aesCpuCmd.s.cpuTblReq = 0;
	   WriteU32Reg(PLC_AESCPUCMDSTAT_REG, aesCpuCmd.reg);



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
	gHpgpHalCB.nwSelectedSnid = 0;
    gHpgpHalCB.bTxPending        = 0;
    //gHpgpHalCB.bBcnTxPending     = 0;
    gHpgpHalCB.BcnLateCnt = 0;
#ifndef HPGP_HAL_TEST	
    gHpgpHalCB.tei               = 0;
#else
    memcpy(gHpgpHalCB.nid, gDefNID, NID_LEN);
#endif	
    gHpgpHalCB.plcTx10FC.reg = HPGP_HP10FC_DEF;
#if defined(POWERSAVE) || defined(LLP_POWERSAVE)
    gHpgpHalCB.psAvln            = TRUE;
    gHpgpHalCB.psInSleepMode     = FALSE;
#endif
#ifdef LLP_POWERSAVE
    gHpgpHalCB.psSta             = TRUE;
	gHpgpHalCB.savedPsMode.psState = PSM_PS_STATE_OFF;
	gHpgpHalCB.savedPsMode.pss   = PSM_PSS_NOT_CONFIG;
#endif


    // Setup default network as DC mode CCo & configure default addr
    // HPGP stack will overwrite in due course.
#ifdef HPGP_HAL_TEST
    HHAL_SetDevMode(DEV_MODE_CCO, LINE_MODE_DC);
    HHAL_SetDefAddrConfig();
#else
    if (opMode == UPPER_MAC)
    {
	    HHAL_SetSnid(0);
	    HHAL_SetDevMode(DEV_MODE_STA, gHpgpHalCB.lineMode);//LINE_MODE_DC);
    }
    else
    {
         HHAL_SetDevMode(DEV_MODE_CCO, LINE_MODE_DC);
         if(gHpgpHalCB.devMode == DEV_MODE_CCO)
         {
             gHpgpHalCB.selfTei   = HYBRII_DEFAULT_TEICCO;
             gHpgpHalCB.remoteTei = HYBRII_DEFAULT_TEISTA;
         }else
         {
             gHpgpHalCB.selfTei   = HYBRII_DEFAULT_TEISTA;
             gHpgpHalCB.remoteTei = HYBRII_DEFAULT_TEICCO;
         }
         gHpgpHalCB.snid      = HYBRII_DEFAULT_SNID;
     	
         HHAL_SetTei(gHpgpHalCB.selfTei);
 
         HHAL_SetSnid(gHpgpHalCB.snid); // this is done because when we communicate with quqlcomm or other chip snid should be set once we receive bcn from cco and should not get set at power on because 
    }
#endif

#if 0
    //[YM] set RSSI-FIFO lock for sound packet
    WriteU32Reg(PLC_RSSI_REG, ctorl(0x1));
#endif

    // Setting for Hybrii ASIC
    WriteU8Reg(0x401, 0x03);   //Enable GAFE SPI
    WriteU8Reg(0x402, 0x10);
    // ADC PLC Clk flip
    mac_utils_spi_write(0x29,0x04);   //added by varsha
	// Enable Sync timeout 
    WriteU8Reg(0x483, 0x33);
	//WriteU8Reg(0x483, 0x13);
#ifdef B_FPGA		
    WriteU8Reg(0x4d9, 0x67); //For FPGA only, extra delay
    WriteU8Reg(0x4d8, 0x84);
	
	//Sync threshold
    //WriteU8Reg(0x48a, 0x90);     //should be re-check for ASIC, commented out for FPGA
	
	//Energy detection and AGC Saturation Threshold
    WriteU8Reg(0x479, 0x52); 
#endif
// Add Sync threshold - Rachel 0317-2014
    //WriteU8Reg(0x478, 0x11);    // For long range
	WriteU8Reg(0x478, 0x05);	  // Jenny new setting 03242014
	WriteU8Reg(0x484, 0x52);
	//WriteU8Reg(0x48A, 0xCA);
	WriteU8Reg(0x48A, 0xFF);	   // Jenny new setting 03242014
	WriteU8Reg(0x48B, 0x00);
	WriteU8Reg(0x4DE, 0xF0);	   // Jenny new setting 03242014
	WriteU8Reg(0x4DF, 0xFF);	   // Jenny new setting 03242014
	WriteU8Reg(0x4E0, 0xFF);	   // Jenny new setting 03242014
	
#if 1    
    // Add init setting for Rachel - YMCHEN 09262013
    WriteU8Reg(0x48c, 0xcb);
	WriteU8Reg(0x48d, 0x96);
	WriteU8Reg(0x48e, 0x2d);
	WriteU8Reg(0x48f, 0x5b);
	WriteU8Reg(0x490, 0xb6);
	WriteU8Reg(0x491, 0x6c);
	WriteU8Reg(0x492, 0xd9);
	WriteU8Reg(0x493, 0xb2);
	WriteU8Reg(0x494, 0x65);
	WriteU8Reg(0x495, 0xcb);
	WriteU8Reg(0x496, 0x96);
	WriteU8Reg(0x497, 0x2d);
	WriteU8Reg(0x498, 0x5b);
	WriteU8Reg(0x499, 0xb6);
	WriteU8Reg(0x49a, 0x6c);
	WriteU8Reg(0x49b, 0xd9);
	WriteU8Reg(0x49c, 0xb2);
	WriteU8Reg(0x49d, 0x65);
	WriteU8Reg(0x49e, 0xcb);
	WriteU8Reg(0x49f, 0x96);
	WriteU8Reg(0x4a0, 0x2d);
	WriteU8Reg(0x4a1, 0x5b);
	WriteU8Reg(0x4a2, 0xb6);
	WriteU8Reg(0x4a3, 0x74);
	WriteU8Reg(0x4a4, 0xc9);
	WriteU8Reg(0x4a5, 0xe2);
	WriteU8Reg(0x4a6, 0x45);
	WriteU8Reg(0x4a7, 0x0b);
	WriteU8Reg(0x4a8, 0x17);
	WriteU8Reg(0x4a9, 0xcc);
	WriteU8Reg(0x4aa, 0x9d);
	WriteU8Reg(0x4ab, 0x33);
	WriteU8Reg(0x4ac, 0x7b);
	WriteU8Reg(0x4ad, 0xd6);
	WriteU8Reg(0x4ae, 0xcc);
	WriteU8Reg(0x4af, 0x19);
	WriteU8Reg(0x4b0, 0x53);
	WriteU8Reg(0x4b1, 0xa7);
	WriteU8Reg(0x4b2, 0x4c);
	WriteU8Reg(0x4b3, 0x9e);
	WriteU8Reg(0x4b4, 0x34);
	WriteU8Reg(0x4b5, 0x71);
	WriteU8Reg(0x4b6, 0xc2);
	WriteU8Reg(0x4b7, 0xd8);
	WriteU8Reg(0x4b8, 0x31);
	WriteU8Reg(0x4b9, 0xa3);
	WriteU8Reg(0x4ba, 0x47);
	WriteU8Reg(0x4bb, 0x8d);
	WriteU8Reg(0x4bc, 0x1c);
	WriteU8Reg(0x4bd, 0x31);
	WriteU8Reg(0x4be, 0x74);
	WriteU8Reg(0x4bf, 0xc8);
	WriteU8Reg(0x4c0, 0xe0);
	WriteU8Reg(0x4c1, 0x41);
	WriteU8Reg(0x4c2, 0x03);
	WriteU8Reg(0x4c3, 0x07);
	WriteU8Reg(0x4c4, 0x14);
	WriteU8Reg(0x4c5, 0x2d);
	WriteU8Reg(0x4c6, 0x52);
	WriteU8Reg(0x4c7, 0xb8);
	WriteU8Reg(0x4c8, 0x50);
	WriteU8Reg(0x4c9, 0xc1);
	WriteU8Reg(0x4ca, 0x02);
	WriteU8Reg(0x4cb, 0x41);
	WriteU8Reg(0x4cc, 0x83);
	WriteU8Reg(0x4cd, 0x04);
	WriteU8Reg(0x4ce, 0x0e);
	WriteU8Reg(0x4cf, 0x14);
	WriteU8Reg(0x4d0, 0x30);
	WriteU8Reg(0x4d1, 0x80);
	WriteU8Reg(0x4d2, 0x51);
	WriteU8Reg(0x4d3, 0x23);
	WriteU8Reg(0x4d4, 0x86);
	WriteU8Reg(0x4d5, 0x0d);
	WriteU8Reg(0x4d6, 0x19);
	WriteU8Reg(0x4d7, 0x04);
#endif 
#if 0
    // Disable ADC hold - Rachel 0318-2014
	WriteU8Reg(0x4FA, 0x01);
	WriteU8Reg(0x44D, 0x0);
	WriteU8Reg(0x44F, 0x0);
	WriteU8Reg(0x4FA, 0x0);	
#endif
#ifdef B_ASICPLC
    mac_utils_spi_write(0x16,0x01);    // Enable PLC mode for AFE
    mac_utils_spi_write(0x36,0x12);   //added by YM, Jenny suggested 
#endif
    //He li provided following regs for DC Offset setting
     mac_utils_spi_write(0x2f,0x00);   //added by varsha
     mac_utils_spi_write(0x3a,0x08);   //added by varsha
     mac_utils_spi_write(0x3b,0x40);   //added by varsha 
     mac_utils_spi_write(0x18,0xa0);   //added by varsha
	 // add delay > 100us
	 CHAL_DelayTicks(6400);
     mac_utils_spi_write(0x18,0x00);   //added by varsha
     mac_utils_spi_write(0x2f,0x00);   //added by varsha
     mac_utils_spi_write(0x3a,0x0f);   //added by varsha 
     mac_utils_spi_write(0x3b,0x74);   //added by varsha
     mac_utils_spi_write(0x18,0xc0);   //added by varsha
	 // add delay > 100us
	 CHAL_DelayTicks(6400);
     mac_utils_spi_write(0x18,0x00);   //added by varsha
	 
#ifdef B_ASICPLC
     mac_utils_spi_write(0x36,0x0);   //added by YM, Jenny suggested 
     mac_utils_spi_write(0x16,0x0);
#endif

//#ifdef B_FPGA
     WriteU8Reg(0x47F, 0x05);
//#endif
     WriteU8Reg(0x485, TCC_REG_485_486_val[2][0]);  //TCC =3
     WriteU8Reg(0x486, TCC_REG_485_486_val[2][1]);

//#ifdef HPGP_HAL_TEST // WAR: code reactivated to reduce loss in lower mac
//    WriteU8Reg(0x478, 0x61); 
//#else
//    WriteU8Reg(0x478, 0x51); 
//#endif  

#if 0
    // [YM] PHY setting for Hybrii_B FPGA HP101/HP11 Detection 
    if (gHpgpHalCB.HP101Detection)
    {
       plcStatus.reg = ReadU32Reg(PLC_STATUS_REG);   
	   plcStatus.s.HP10Detect = 1;         //Enable MAC for HP10 detection
       WriteU32Reg(PLC_STATUS_REG,plcStatus.reg);
      	
        WriteU8Reg(0x4F8, 0x01);  //Set to high-bank phy memory
        tmp = ReadU8Reg(0x46F);
        WriteU8Reg(0x46F, tmp&0xF7);  //Enable HP101 detection bit
        WriteU8Reg(0x4F8, 0x0);  //Set back to low-bank phy memory    
    }	
#endif
    
	// [YM] Based on Jayesh's suggestion 0307-2014
        WriteU8Reg(0x4F8, 0x01);  //Set to high-bank phy memory
        WriteU8Reg(0x46F, 0xB);  //Enable HP101 detection bit
        WriteU8Reg(0x4F8, 0x0);  //Set back to low-bank phy memory 		
#ifdef B_FPGA    
    // Energy Detection Threshold
    WriteU8Reg(0x478, 0x69);	//WAR: code should be deactivated to reduce loss in lower mac	// according to geetha comments 11/6/2012
#endif
//#ifdef MPER    
    mac_utils_spi_write(0x34,0x0);   //Set default txpowermode = 2 (High Tx Power Mode)
	mac_utils_spi_write(0x35,0xF);   //added by YM 0211-2014
//#else	
//	mac_utils_spi_write(0x34,0x08);    //Set default txpowermode = 0 (Auto Power Mode)
//    mac_utils_spi_write(0x35,0x30);   //added by YM 0211-2014
//#endif	//MPER

#ifndef B_ASICPLC    
	// [YM] Set timeout value, based on He Li suggest
	WriteU8Reg(0x4DF, 0xCA);
#endif  //B_ASICPLC

#ifdef ER_TEST 	
	//[YM] change new setting value for long range 11-21-2013, make it as default for GV7013
    WriteU8Reg(0x48a, 0xEA);   //the same as current default value
    WriteU8Reg(0x48b, 0x00);
    WriteU8Reg(0x484, 0x52);
    WriteU8Reg(0x478, 0x11);
    WriteU8Reg(0x483, 0x33);
	WriteU8Reg(0x4E0, 0xFF);
	WriteU8Reg(0x4DF, 0xFF);
    //[YM] add new setting value for LED, 01-09-2014
    WriteU8Reg(0x4F0, 0x80);  //Turn off SCO in PHY, Extend Range
#endif
    // Force Hybrid Mode  
//    WriteU8Reg(0x4EB, 0x18);  


    // Reset/Init Statistics
    HHAL_ResetPlcStat();
   
    //WriteU8Reg(0xF07,0x01);           // csma 
#ifndef B_ASICPLC
    WriteU8Reg(0x423,0xC3);
////////////////////////////////////////////////////////////////
   //WriteU8Reg(0xF07,0x01);
   //WriteU8Reg(0xF06,0x00);
#else
   //WriteU8Reg(0xF07,0xC1);
   //WriteU8Reg(0xF06,0x00);
   WriteU8Reg(0x423,0x81);
#endif
   
    HHAL_AFEInit();

//Set default Rx power mode to 1 - Reduce ADC power consumption
    mac_utils_spi_write(0x26, 0x1C);   //added by Yiming, 0211-2013

	// init CP Map
	for (i = 0; i < HYBRII_CPCOUNT_MAX; i++)
	{
		uPlcCpMapIdx plcCpMapIdx;
		uPlcCpMap plcCpMap;
//		uPlcCpuWrCp plcCpuWrCp;

		plcCpMapIdx.reg = 0;
		plcCpMapIdx.s.cp_map_idx = i;
		WriteU32Reg(CPU_CPUSAGECNTIDX_REG, plcCpMapIdx.reg);

		plcCpMap.reg = 0;
		plcCpMap.s.cp_map = 1;
		WriteU32Reg(CPU_CPUSAGECNT_REG, plcCpMap.reg);

//		CHAL_DecrementReleaseCPCnt(i);
	}
//    WriteU32Reg(PLC_LINECTRL_REG,ctorl(0x400));

	// init Hang Counter Int.
	plcSMCnt.reg = RTOCL(PLC_SM_MAX_CLK_CNT);
	plcSMCnt.s.enable = TRUE;
	WriteU32Reg(PLC_SM_MAXCNT, plcSMCnt.reg);
    
// [YM] For Hybrii_B LLP LED project
#ifdef POWERSAVE_NO
    byteVal = ReadU8Reg(0x422);
	byteVal |= 0x8;
    WriteU8Reg(0x422,byteVal);
#endif
}

void HHAL_ResetPlcStat()
{   
    memset(&gHpgpHalCB.halStats, 0, sizeof(shpgpHalStats));
    
    // Clear Err Count registers
    WriteU32Reg(PLC_ADDRFILTERERRCNT_REG, 0);
    WriteU32Reg(PLC_FCCSERRCNT_REG, 0);
    WriteU32Reg(PLC_PBCSRXERRCNT_REG, 0);
    WriteU32Reg(PLC_PBCSTXERRCNT_REG, 0);
    WriteU32Reg(PLC_ICVERRCNT_REG, 0);  
    WriteU32Reg(PLC_MPDUDROPCNT_REG, 0);  
	
	gHalCB.qc_no_1st_desc = 0; 
	gHalCB.qc_too_many_desc = 0; 
	gHalCB.qc_no_desc = 0; 
	gHalCB.qc_no_grant = 0; 
   	gHalCB.cp_no_grant_alloc_cp = 0;
   	gHalCB.cp_no_grant_free_cp = 0;
   	gHalCB.cp_no_grant_write_cp = 0;
   	gHalCB.cp_no_grant_read_cp = 0;
}

// Internal functios 

void hhal_setStaSnidValid()
{
    uPlcDevCtrlReg plcDevCtrl;

    if(gHpgpHalCB.devMode == DEV_MODE_STA)
    {
        plcDevCtrl.reg = ReadU32Reg(PLC_DEVCTRL_REG);
        plcDevCtrl.s.snidValid = 1;
        WriteU32Reg(PLC_DEVCTRL_REG, plcDevCtrl.reg);
    }   
}

void hal_hpgp_mac_monitoring (void)
{
    u32             sm_status1;
    u32             sm_status2;

    uPlcMedStatReg  plcMedStat;

/*    if(gHpgpHalCB.halStats.bcnSyncCnt == 1 && ReadU32Reg(PLC_FCCSERRCNT_REG)!=0)
    {
        // Reset mpi Rx
        HHAL_mpiRxReset();
        //FM_Printf(FM_MINFO,"Resetting Mpi Rx\n");
    }*/
    if (gHpgpHalCB.halStats.macTxStuckCnt > 1000) {
        //printf("\nMAC Tx Stucked");
    }
    if (gHpgpHalCB.halStats.macRxStuckCnt > 1000) {
        //printf("\nMAC Rx Stucked");
    }
    //sm_status1 = ReadU32Reg(PLC_SMSHADOW1_REG);// & 0x00003fff;
    sm_status1 =  hal_common_reg_32_read(PLC_SMSHADOW1_REG);
    sm_status2 = ReadU32Reg(PLC_SMSHADOW2_REG); //& 0x03ffffff;

    if (sm_status1 == 0 && sm_status2 == 0) {
        plcMedStat.reg = ReadU32Reg(PLC_MEDIUMSTATUS_REG);
        if (plcMedStat.s.phyActive) {
            gHpgpHalCB.halStats.phyStuckCnt++;
            if (gHpgpHalCB.halStats.phyStuckCnt++ > 500) {
                //printf("\nPhy Stucked");
                // gHpgpHalCB.halStats.macHangRecover1++;
                //gHpgpHalCB.halStats.macRxStuckCnt = 0;
                /*
                 * MAC is idle but Phy is stuck. Reset the PHY
                 * by dropping RX Enable
                 */
                //plcStatus.reg = ReadU32Reg(PLC_STATUS_REG);
                //plcStatus.s.nRxEn = 1;
                //WriteU32Reg(PLC_STATUS_REG, plcStatus.reg);
            }
        } else {
            gHpgpHalCB.halStats.phyStuckCnt = 0;
        }
        if (plcMedStat.s.mpiRxEn == 0) {
            gHpgpHalCB.halStats.mpiRxStuckCnt++;
            if (gHpgpHalCB.halStats.phyStuckCnt++ > 500) {
                //printf("\nMPI RX Stucked");
            }
        } else {
            gHpgpHalCB.halStats.mpiRxStuckCnt = 0;
        }
    } else {
        if (sm_status2 != 0) {
            gHpgpHalCB.halStats.smTxStuckCnt++;
            if (gHpgpHalCB.halStats.smTxStuckCnt > 1000) {
                //printf("\nSM TX Stucked");
            }
        } else {
            gHpgpHalCB.halStats.smTxStuckCnt = 0;
        }
        if (sm_status1 != 0) {
            gHpgpHalCB.halStats.smRxStuckCnt++;
            if (gHpgpHalCB.halStats.smRxStuckCnt > 1000) {
               // printf("\nSM RX Stucked");
            }
        } else {
            gHpgpHalCB.halStats.smRxStuckCnt = 0;
        }
    }
}

#ifndef HPGP_HAL_TEST

eStatus  HHAL_BcnRxIntHandler(sHaLayer *hal, sEvent *event)
{
    sFrmCtrlBlk      *pFrmCtrlBlk = NULL;
    u32              *pValue32 = NULL;
    u32               value32 = 0;
    sHybriiRxBcnHdr*  pRxBcnHdr = NULL;
    u8                i;
    uPlcStatusReg     plcStatus;
    eStatus           ret = STATUS_SUCCESS;
    sHpgpHdr         *hpgpHdr = NULL;
    sHpgpHalCB       *hhalCb = hal->hhalCb;
	u8				 *rxArr;
    u8               u8val;
    
    plcStatus.reg = ReadU32Reg(PLC_STATUS_REG);

    hhalCb->halStats.BcnRxIntCnt++;

    //FM_Printf(FM_MINFO, "HHAL_BcnRxIntHandler : Bcn received, SnapShot = %bu, Id = %lX\n", pRxBcnHdr->snapShot, rtocl(ReadU32Reg(PLC_IDENTIFIER_REG)));

    // Confirm that Bcn Rx Fifo is not emplty.
    // plcStatus.s.bcnRxFifoStat;
    /* read the beacon Rx descriptor first */
    value32 = ReadU32Reg(PLC_BCNRXFIFO_REG);
    pRxBcnHdr = (sHybriiRxBcnHdr*)&value32;
    hhalCb->snapShot = pRxBcnHdr->snapShot;
    /* then, read the beacon */
	pValue32  = (u32 *)(event->buffDesc.dataptr);
#ifdef UM
	if(1 == eth_plc_sniffer)
	{
    	pValue32  += ((sizeof(sEth2Hdr) + sizeof(hostHdr_t)) / sizeof(u32)); // Ethernet header + Hybrii header
	}
#endif

	for ( i = 0; i < (BEACON_LEN >> 2); i++)
    {
        pValue32[i] = ReadU32Reg(PLC_BCNRXFIFO_REG);
    }
	event->buffDesc.datalen = BEACON_LEN;
    //plcStatus.reg = ReadU32Reg(PLC_STATUS_REG);
    //plcStatus.s.decBcnRxCnt = 1;
    //WriteU32Reg(PLC_STATUS_REG, plcStatus.reg);
    u8val = ReadU8Reg(PLC_STATUS_REG+1);
	u8val |= 0x80;		// plcBcnCntDecr (bit 15 of PLC_STATUS_REG)
	WriteU8Reg(PLC_STATUS_REG+1, u8val);
	rxArr = event->buffDesc.dataptr;
#ifdef UM 
#ifdef HYBRII_ETH
	if(1 == eth_plc_sniffer)
	{
    	EHT_FromPlcBcnTx(rxArr, PLC_BCNRX_LEN);
	}
	else
#endif
#endif
    {
        // Update statistics.

            /* TODO: if consective good beacons are received */


        if(pRxBcnHdr->fccsCorrect)
        {
            hhalCb->bcnDetectFlag = 1;
            pFrmCtrlBlk = (sFrmCtrlBlk*) (event->buffDesc.dataptr);
            hhalCb->bts = ((u32)(pFrmCtrlBlk->bts[3])<<24) + 
                          ((u32)(pFrmCtrlBlk->bts[2])<<16) + 
                          ((u32)(pFrmCtrlBlk->bts[1])<<8) + 
                           (u32)(pFrmCtrlBlk->bts[0]);                      
            /* TODO: Adjust NTB in the device */
               gCCO_NTB =  hhalCb->bts;   

               gHpgpHalCB.bcnmisscnt = 0;
               gCCO_BTS =  hhalCb->bts;
    // Extract Bto, ded
    
//            HHAL_AdjustNextBcnTime(hal, pFrmCtrlBlk->bto);
        }
        // Update statistics.
        if (pRxBcnHdr->fccsCorrect && pRxBcnHdr->pbcsCorrect && 
            !pRxBcnHdr->rsv1 && !pRxBcnHdr->rsv2 && 
            !pRxBcnHdr->rsv3 && !pRxBcnHdr->rsv4 )  
        {  
            hhalCb->halStats.TotalRxGoodFrmCnt++;
            hhalCb->halStats.RxGoodBcnCnt++;

            hpgpHdr = (sHpgpHdr *)event->buffDesc.buff;
        //            hpgpHdr->tei = rxdesc->ste;
            hpgpHdr->snid = pFrmCtrlBlk->snid;
            {
                event->buffDesc.dataptr += sizeof(sFrmCtrlBlk); 
                event->buffDesc.datalen -= sizeof(sFrmCtrlBlk); 
                /* process the high priority portion of the beacon */
#ifdef CALLBACK
                hal->procBcn(hal->bcnCookie, event);
#else
                if (hhalCb->devMode == DEV_MODE_CCO)
                {
#ifdef CCO_FUNC
                    LINKL_CcoProcBcnHandler(hal->bcnCookie, event, hhalCb->bts);
#endif
                }
                else 
                {
#ifdef STA_FUNC
                    LINKL_StaProcBcnHandler(hal->bcnCookie, event, hhalCb->bts);
#endif
                }
#endif
            }
        } 
        else
        {
            hhalCb->halStats.RxErrBcnCnt++;
            /* bad beacon is received, thus the beacon is lost */
            /* TODO: adjust the next becaon period using bto */
            hhalCb->bcnDetectFlag = 0;
            ret = STATUS_FAILURE;
        }

    }
    // Any more beacons ?
//    plcStatus.reg = ReadU32Reg(PLC_STATUS_REG);

    return ret;
}

u8 HHAL_GetBcnCnt()
{
    uPlcStatusReg     plcStatus;
    plcStatus.reg = ReadU32Reg(PLC_STATUS_REG);
    return plcStatus.s.plcBcnCnt;
}



eStatus HHAL_PrepareTxFrame(sHaLayer *hal, sTxDesc *txInfo, 
                            sSwFrmDesc *txFrmSwDesc)
{
    //FM_Printf(FM_MINFO, ">>>PrepareTxFrame:\n");

    if(txInfo->mnbc)
    {
        txFrmSwDesc->frmInfo.plc.mcstMode = HPGP_MNBCST;
    }
    else if ((txInfo->mcst == 1) || (txInfo->dtei == 0xFF))
    {
        txFrmSwDesc->frmInfo.plc.mcstMode = HPGP_MCST;
    }   
    else
    {
        txFrmSwDesc->frmInfo.plc.mcstMode = HPGP_UCST;
    }
	
//printf("HHAL_PrepareTxFrame=%bu, txInfo->dtei=%bu\n", HHAL_PrepareTxFrame, txInfo->dtei);
    txFrmSwDesc->frmInfo.plc.dtei = txInfo->dtei;
    txFrmSwDesc->frmInfo.plc.stei = hal->hhalCb->tei;
    txFrmSwDesc->frmInfo.plc.eks = txInfo->eks;

    txFrmSwDesc->frmInfo.plc.clst = HPGP_CLST_ETH;
    txFrmSwDesc->frmInfo.plc.plid = txInfo->plid;
    txFrmSwDesc->frmInfo.plc.mfStart = txInfo->mfStart;
    txFrmSwDesc->frmInfo.plc.mfEnd = txInfo->mfEnd;
   
    if (txInfo->plid == PRI_LINK_ID_0) 
        txFrmSwDesc->frmInfo.plc.phyPendBlks = HPGP_PPB_CAP0;
    else
        txFrmSwDesc->frmInfo.plc.phyPendBlks = HPGP_PPB_CAP123;

    txFrmSwDesc->frmInfo.plc.roboMode = txInfo->roboMode;


	txFrmSwDesc->frmInfo.plc.snid = txInfo->snid;


#if 0
    if (txInfo->roboMode == HPGP_ROBOMD_MINI)
    {
        txFrmSwDesc->frmInfo.plc.pbsz = HPGP_PHYBLKSIZE_136;
        txFrmSwDesc->frmInfo.plc.flav = HPGP_MINIROBO_FLAV;
        txFrmSwDesc->frmInfo.plc.numPBs = PLC_ONE_PB;
    }
    else if (txInfo->roboMode == HPGP_ROBOMD_STD)
    {
        txFrmSwDesc->frmInfo.plc.pbsz = HPGP_PHYBLKSIZE_520;
        txFrmSwDesc->frmInfo.plc.flav = HPGP_STDROBO_FLAV;
        txFrmSwDesc->frmInfo.plc.numPBs = PLC_ONE_PB;
    }
    else
    {
        /* HS ROBO */
        txFrmSwDesc->frmInfo.plc.roboMode = HPGP_ROBOMD_HS;
        txFrmSwDesc->frmInfo.plc.pbsz = HPGP_PHYBLKSIZE_520;
        if (txInfo->numPbs == 1)
        {
            txFrmSwDesc->frmInfo.plc.flav = HPGP_1PBHSROBO_FLAV;
            txFrmSwDesc->frmInfo.plc.numPBs = PLC_ONE_PB;
        }
        else if (txInfo->numPbs == 2)
        {
            txFrmSwDesc->frmInfo.plc.flav = HPGP_2PBHSROBO_FLAV;
            txFrmSwDesc->frmInfo.plc.numPBs = PLC_TWO_PB;
        }
        else if (txInfo->numPbs == 3)
        {
            txFrmSwDesc->frmInfo.plc.flav = HPGP_3PBHSROBO_FLAV;
            txFrmSwDesc->frmInfo.plc.numPBs = PLC_THREE_PB;
        }
        else 
        {
            FM_Printf(FM_ERROR,"HHAL: PB Num Err");
            return STATUS_FAILURE;
        }
    }
#endif

    //FM_Printf(FM_MINFO, "<<<PrepareTxFrame:\n");

    return STATUS_SUCCESS;    	
}

#endif


/*******************************************************************
* NAME :            HHAL_AdjustNextBcnTime
*
* DESCRIPTION :     The function is called at STA side passing the bto as input.
*                   It adjusts the BP Length based on the 2's complement bto value
*                   that CCo sends in the FC.bto[0] field.
*
*/
void HHAL_AdjustNextBcnTime(u16 *bto)
{
    u16 btoVal = le16_to_cpu(*bto);
    if (gHpgpHalCB.lineMode == LINE_MODE_AC)
    {   
        // bto is a negative 2's complement value.
        if( btoVal > 0x8000)
        {
            gHpgpHalCB.curBcnPer = PLC_AC_BP_LEN - (0x10000 - btoVal);
        }

        // bto is a positive 2's complement value
        else if( btoVal != 0x8000)
        {
            gHpgpHalCB.curBcnPer = PLC_AC_BP_LEN + btoVal;       
        }
        
        //printf("bpLen = %lx, bto=%bx%02bx, %x\n", gHpgpHalCB.curBcnPer, pFrmCtrlBlk->bto0[1], pFrmCtrlBlk->bto0[0], bto);

        if(gHpgpHalCB.devMode == DEV_MODE_STA)
        {
            WriteU32Reg( PLC_SWBCNPERAVG_REG, ctorl(gHpgpHalCB.curBcnPer));
			WriteU32Reg(PLC_SWBCNPERAVG_REG, ctorl((gHpgpHalCB.curBcnPer >> 1)));	// HW multiples it by 2

        }
    }
    else
    {
        gHpgpHalCB.curBcnPer = PLC_DC_BP_LEN;
    }
}

#ifdef UM 
     
eStatus CHAL_WriteFrame(sSwFrmDesc *txFrmSwDesc,sBuffDesc *buffdesc)
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
     //   FM_Printf(FM_ERROR, "CHAL: frame len and cp number mismatch.\n");
    }
    return STATUS_SUCCESS;
}

#endif


void doSynchronization()
{
	   
	 u32 zcCCONTB;
     
     zcCCONTB =  rtocl(ReadU32Reg(PLC_ZCNTB_REG));


     if(firsttime == 0)
     {
        firsttime = 1;
        zcCCONTBold = zcCCONTB; 
        return;
     }

	 gHpgpHalCB.syncComplete = 1;
	 
     if(zcCCONTB > zcCCONTBold)
        gHpgpHalCB.curBcnPer   = (zcCCONTB - zcCCONTBold);
     

     if((gHpgpHalCB.lineMode == LINE_MODE_AC) && (gHpgpHalCB.curBcnPer  < AC_MIN_THRESHOLD))
     {
        return;
     } 
     
     zcCCONTBold = zcCCONTB; 
      
    if( gHpgpHalCB.devMode == DEV_MODE_CCO && gHpgpHalCB.lineMode == LINE_MODE_AC )
    {
        
        //printf("\n BP = %lu", (gHpgpHalCB.curBcnPer * 40));
        if( gHpgpHalCB.curBcnPer < PLC_MIN_AC_BPLEN )
        {
            gHpgpHalCB.curBcnPer = PLC_MIN_AC_BPLEN;
        }
        else if( gHpgpHalCB.curBcnPer > PLC_MAX_AC_BPLEN )
        {
            gHpgpHalCB.curBcnPer = PLC_MAX_AC_BPLEN;
        }

        gHpgpHalCB.bcnPerSum += gHpgpHalCB.curBcnPer;
        gHpgpHalCB.perSumCnt ++;

        if( gHpgpHalCB.perSumCnt == PLC_BCNPERAVG_CNT )
        {
            gHpgpHalCB.bPerAvgInitDone = 1;
            avgdone = 1;
        }
       
        if(avgdone)
        {
             avgdone = 0;
             gavg       = gHpgpHalCB.bcnPerSum >> PLC_BCNPERAVG_DIVCNT;
             gHpgpHalCB.bcnPerSum = 0;
             gHpgpHalCB.perSumCnt = 0;
        }
        WriteU32Reg(PLC_SWBCNPERAVG_REG, ctorl((gavg >> 1)));
         
     }

    // Prepare and send beacon here for now.
    // This will eventually be done by hpgp nsm module.
        if(gHpgpHalCB.lineMode == LINE_MODE_DC)
        {
            gHpgpHalCB.bPerAvgInitDone = 1;
             gavg       =   PLC_DC_BP_LEN; 
        }

	    if(gHpgpHalCB.bPerAvgInitDone)
		{
			  	
		    if(gHpgpHalCB.devMode == DEV_MODE_CCO)
			{
			 
			    if(zctrack == 0)
                {
                 
                    gbpst =  gavg  + zcCCONTB + MAC_PROCESSING_CLOCK ;// + 0x1BAFF;//1365 is bpsto
                    zctrack = 1;
                    WriteU32Reg(PLC_BPST_REG, ctorl(gbpst));
               
                }
                else
                {
                
                     avgcount++;
                     if(zcCCONTB >  gbpst)
                     {
                        
                         if(gNegativeflag)
                         {
                           gNegativeflag = 0;
                           gPositiveflag = 1; 
                          
                         }
                         gBPSTdelta = zcCCONTB - gbpst;
                        
                        if(gBPSTdelta > 400)//16 micro sec
                        {
                            
                             gNegativeflag = 0;
                             gPositiveflag = 1;
                         }
                        
                     }
                     else
                     {
                         if(gPositiveflag)
                         {
                           gNegativeflag = 1;
                           gPositiveflag = 0; 
                          
                         }
                         gBPSTdelta =  gbpst - zcCCONTB;
                         
                         if(gBPSTdelta > 400)//4 micro sec
                         {
                            gNegativeflag = 1;
                            gPositiveflag = 0;
                           
                         }   
                        
                      }
                 
                 } 
           	 }
				 
	    } 

}  

#ifdef FREQ_DETECT
void FREQDET_DetectFrequencyUsingZC()
{   
    static u8 count = 0;
    static u8 valid60Hz =0;
    static u8 valid50Hz =0;
    u32  zcCCONTB;
    static u32 zcCCONTBOld = 0;
    u32 linecyclePer;
    u8 freq;
    
//    uPlcMedInterruptReg PlcMedInterruptReg;
   
#ifdef UM
    sLinkLayer *linkl = HPGPCTRL_GetLayer(HP_LAYER_TYPE_LINK);
#endif
    count++;

	
    zcCCONTB = rtocl(ReadU32Reg(PLC_ZCC_CCONTB_REG));

	if (count == 1)
	{
		zcCCONTBOld = zcCCONTB;
		return;

	}
    //zcCCONTB =  rtocl(ReadU32Reg(PLC_NTB_REG));
    linecyclePer = (((zcCCONTB - zcCCONTBOld) * 40) / 2);
     freq = (1000000000/ linecyclePer);
    // printf("\n freq = %lu\n", ((zcCCONTB - zcCCONTBOld) * 40)); 
    zcCCONTBOld = zcCCONTB;
   

    if((freq <= MAX_60HZ_FREQ) && (freq >= MIN_60HZ_FREQ))
    {
        valid60Hz++;
    }
    else if((freq <= MAX_50HZ_FREQ) && (freq >= MIN_50HZ_FREQ))
    {
          valid50Hz++;
    }
	else
	{
		printf("\nfl:%bu ", freq);
	}
 

    if(count == 8)
    { 
        //Retrycnt++;

		count = 0;
		
        gHpgpHalCB.gFreqCB.freqDetected = TRUE; 
        if(valid50Hz > FREQUENCY_ZC_MAX_COUNT)
        {
          
            //gHpgpHalCB.gFreqCB.freqDetectUsingZC = FALSE;
            FREQDET_FreqSetting(FREQUENCY_50HZ);            
            printf("\n50 HZ detected\n");
            FM_Printf(FM_ERROR, "\n fre= %bu",freq); 
        }
        else if(valid60Hz > FREQUENCY_ZC_MAX_COUNT)
        {
          
            //gHpgpHalCB.gFreqCB.freqDetectUsingZC = FALSE;
            FREQDET_FreqSetting(FREQUENCY_60HZ);            
            printf("\n60 HZ detected\n");
            FM_Printf(FM_ERROR, "\n fre= %bu",freq); 
        }
        else
        {
           //do not set any default frequency ,insted keep on retrying and based on   gHpgpHalCB.gFreqCB.freqDetected flag Upper MAc will recognised that freq is not being detected for some problem
#ifdef UM
           //sHaLayer *hal = HOMEPLUG_GetHal();
		   
           printf("\n No Frequency detected" );
           printf("\n switching to DC ...%bu ", freq);
           
           gHpgpHalCB.gFreqCB.freqDetected = FALSE;
		   
		   gHpgpHalCB.lineMode = LINE_MODE_DC;
		   gHpgpHalCB.lineFreq = FREQUENCY_50HZ;		  
		   
		   HHAL_SetDevMode(gHpgpHalCB.devMode, LINE_MODE_DC);
		   
		   FREQDET_FreqSetting(gHpgpHalCB.lineFreq);
#if 0 //def  MULTI_CCO_NW
		//	   linkl->ccoNsm.slotId = 0; TODO RAJAN should we comment it
#endif
           //gHpgpHalCB.gFreqCB.freqDetectUsingZC = TRUE;
#endif

        }
#ifdef UM
#ifdef CCO_FUNC
        CNSM_InitRegion(&linkl->ccoNsm);
#endif
#else
    
if(gHpgpHalCB.devMode == DEV_MODE_CCO)
    setCSMA_onCCO();
else
    setCSMA_onSTA();

#endif

       
    }
}

void FREQDET_DetectFrequencyUsingBcn(u8 snid)
{
#ifndef HPGP_HAL_TEST
    //sHaLayer *hal = HOMEPLUG_GetHal();
#endif 
    u32     calcBCNper;
   // static u8 bcnCnt = 0;

   if ((!gHpgpHalCB.nwSelected) || (gHpgpHalCB.nwSelectedSnid != snid))
   	{
		return;
   	}
   
    calcBCNper = gCCO_BTS - gOldBTS;

    //printf("\n bp = %lu", calcBCNper);
    gOldBTS = gCCO_BTS;
   // bcnCnt++;

 
        if(gHpgpHalCB.halStats.BcnSyncCnt >= FREQUENCY_BCN_MAX_COUNT)   //set the frequency after syncing four bcns
        {
            if(((calcBCNper * 40) > MIN_50HZ_BCNPER) && ((calcBCNper * 40) < MAX_50HZ_BCNPER))//38 ms   it is 50 Hz that means bcnper is 40 ms
            {                
                //gHpgpHalCB.gFreqCB.freqDetectUsingBcn = FALSE;
                FREQDET_FreqSetting(FREQUENCY_50HZ);                
                printf("\n BCN 50 HZ detected\n");
               // bcnCnt = 0;
                gHpgpHalCB.gFreqCB.freqDetected = TRUE;

            }
            else if(((calcBCNper * 40) < MIN_50HZ_BCNPER))//33.33 ms , 60 hz
            {             
               // gHpgpHalCB.gFreqCB.freqDetectUsingBcn = FALSE;
                printf("\n bp = %lu",(calcBCNper * 40));

                FREQDET_FreqSetting(FREQUENCY_60HZ);                
                printf("\nBCN 60 HZ detected\n");
                //bcnCnt = 0;
                gHpgpHalCB.gFreqCB.freqDetected = TRUE;
              
            }
        }
    

}
//#endif 

//#ifdef FREQ_DETECT
void FREQDET_FreqDetectInit(void)
{    
   // gHpgpHalCB.gFreqCB.freqDetectUsingBcn = FALSE;
   // gHpgpHalCB.gFreqCB.freqDetectUsingZC = FALSE;
    gHpgpHalCB.gFreqCB.freqDetected = FALSE;
   // gHpgpHalCB.gFreqCB.frequency = FREQUENCY_50HZ; // 50Hz
    //HHAL_SetPlcDevMode(STA_CSMANW);
   
//   gHpgpHalCB.bcnInitDone =1;
    //HHT_DevCfg();

}

void FREQDET_FreqDetectReset(void)
{    
  //  gHpgpHalCB.gFreqCB.freqDetectUsingBcn = FALSE;
  //  gHpgpHalCB.gFreqCB.freqDetectUsingZC = FALSE;
    gHpgpHalCB.gFreqCB.freqDetected = FALSE;

   // gHpgpHalCB.gFreqCB.frequency = FREQUENCY_50HZ; // 50Hz
}

#endif
