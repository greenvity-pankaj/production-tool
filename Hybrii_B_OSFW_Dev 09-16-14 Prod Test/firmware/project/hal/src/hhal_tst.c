/*
*
* $Id: hhal_tst.c,v 1.16 2014/06/24 16:26:45 ranjan Exp $
*
* $Source: /home/cvsrepo/Hybrii_B_OSFW_Dev/firmware/project/hal/src/hhal_tst.c,v $
*															 
* Description  : HAL test cases module.
* 
* Copyright (c) 2010-2011 Greenvity Communications, Inc.
* All rights reserved.
*
* Purpose      :
*     Implements test cases for HPGP HAL as well as common HAL.
*
*
*/

#ifdef RTX51_TINY_OS
#include <rtx51tny.h>
#endif
#include <stdio.h>
#include <string.h>
#include <intrins.h>
#include "fm.h"
#include "mmsg.h"
#include "hpgpdef.h"
#include "hal_common.h"
#include "hal.h"

#include "hal_hpgp.h"
#include "hal_eth.h"
#include "hal_tst.h"
#include "hal_cfg.h"
#ifdef _LED_DEMO_
#include "led_board.h"
#endif
#include "sys_common.h"
#include "ism.h"
#include "datapath.h"

#ifdef UART_HOST_INTF 
#include "uart_driver.h"
#endif

#ifdef PROD_TEST 
#include "hal_prod_tst.h"
#endif

u16 var1 = 0;

extern u8 testCmd;

extern u8 gpbcscorrect;
extern u8 gvalid;
extern u32 gsnid;
//extern u32 missarr[1000];
extern u32 cnt5;
extern u32 misscnt;
extern u32 debugcnt;
extern u32 TX_RXLatency_TCC[8];
extern u8 TCC_REG_485_486_val[8][2];
u32 oldgbpst; 
u32 gbcnsent = 0;
extern sHalCB gHalCB;
int getline(char *s, int lim);

#ifdef ASSOC_TEST
extern  void LM_SendAssocReq();
extern  void LM_SendGetKeyReq();
extern  void LM_SendIdentifyReq();
							   

#endif

extern u16 CSMA_REGIONS_VAL_DC[HYBRII_MAXSMAREGION_CNT];
extern u16 CSMA_REGIONS_VAL_AC[HYBRII_MAXSMAREGION_CNT];

void hhal_tst_sniff_cfg (bool sniff_en)
{
    uPlcStatusReg  plcStatus;
    
    if (sniff_en)
    {
        printf("Enable \n");
    }
    else
    {   printf("Disable \n");
    }
    printf("Sniffer Mode\n");
    plcStatus.reg = ReadU32Reg(PLC_STATUS_REG);
    plcStatus.s.promiscModeEn  = sniff_en; 
    plcStatus.s.snifferMode    = sniff_en;
    WriteU32Reg(PLC_STATUS_REG, plcStatus.reg);
}

void EHT_SimulateTx(sEthSimTxTestParams*);
//void HHT_SimulateTx(sPlcSimTxTestParams* pTestParams);
//void broadcast_CCOTEI(void);
// FIXED_LEN_ALT_ROBO test mode = Length select array
extern u16 gAltRoboLenArr[];//={101,401,801,1201};
extern u8 gAltEksTstArr[];// = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 15}; // 7 NEKs, 2 PPEKs, Unenc
//extern u16 gAltRoboLenArr[];
//extern u8 gAltEksTstArr[];

// Security test constants
u8 gDefKey[10][16] = {
{0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0A,0x0B,0x0C,0x0D,0x0E,0x0F},
{0x10,0x11,0x12,0x13,0x14,0x15,0x16,0x17,0x18,0x19,0x1A,0x1B,0x1C,0x1D,0x1E,0x1F},
{0x20,0x21,0x22,0x23,0x24,0x25,0x26,0x27,0x28,0x29,0x2A,0x2B,0x2C,0x2D,0x2E,0x2F},
{0x30,0x31,0x32,0x33,0x34,0x35,0x36,0x37,0x38,0x39,0x3A,0x3B,0x3C,0x3D,0x3E,0x3F},
{0x40,0x41,0x42,0x43,0x44,0x45,0x46,0x47,0x48,0x49,0x4A,0x4B,0x4C,0x4D,0x4E,0x4F},
{0x50,0x51,0x52,0x53,0x54,0x55,0x56,0x57,0x58,0x59,0x5A,0x5B,0x5C,0x5D,0x5E,0x5F},
{0x60,0x61,0x62,0x63,0x64,0x65,0x66,0x67,0x68,0x69,0x6A,0x6B,0x6C,0x6D,0x6E,0x6F},
{0x70,0x71,0x72,0x73,0x74,0x75,0x76,0x77,0x78,0x79,0x7A,0x7B,0x7C,0x7D,0x7E,0x7F},
{0x80,0x81,0x82,0x83,0x84,0x85,0x86,0x87,0x88,0x89,0x8A,0x8B,0x8C,0x8D,0x8E,0x8F},
{0x90,0x91,0x92,0x93,0x94,0x95,0x96,0x97,0x98,0x99,0x9A,0x9B,0x9C,0x9D,0x9E,0x9F}
//{0xA0,0xA1,0xA2,0xA3,0xA4,0xA5,0xA6,0xA7,0xA8,0xA9,0xAA,0xAB,0xAC,0xAD,0xAE,0xAF},
//{0xB0,0xB1,0xB2,0xB3,0xB4,0xB5,0xB6,0xB7,0xB8,0xB9,0xBA,0xBB,0xBC,0xBD,0xBE,0xBF},
//{0xC0,0xC1,0xC2,0xC3,0xC4,0xC5,0xC6,0xC7,0xC8,0xC9,0xCA,0xCB,0xCC,0xCD,0xCE,0xCF},
//{0xD0,0xD1,0xD2,0xD3,0xD4,0xD5,0xD6,0xD7,0xD8,0xD9,0xDA,0xDB,0xDC,0xDD,0xDE,0xDF},
//{0xE0,0xE1,0xE2,0xE3,0xE4,0xE5,0xE6,0xE7,0xE8,0xE9,0xEA,0xEB,0xEC,0xED,0xEE,0xEF},
//{0xF0,0xF1,0xF2,0xF3,0xF4,0xF5,0xF6,0xF7,0xF8,0xF9,0xFA,0xFB,0xFC,0xFD,0xFE,0xFF},
//
//
//{0xFF,0xFE,0xFD,0xFC,0xFB,0xFA,0xF9,0xF8,0xF7,0xF6,0xF5,0xF4,0xF3,0xF2,0xF1,0xF0},
//{0xEF,0xEE,0xED,0xEC,0xEB,0xEA,0xE9,0xE8,0xE7,0xE6,0xE5,0xE4,0xE3,0xE2,0xE1,0xE0},
//{0xDF,0xDE,0xDD,0xDC,0xDB,0xDA,0xD9,0xD8,0xD7,0xD6,0xD5,0xD4,0xD3,0xD2,0xD1,0xD0},
//{0xCF,0xCE,0xCD,0xCC,0xCB,0xCA,0xC9,0xC8,0xC7,0xC6,0xC5,0xC4,0xC3,0xC2,0xC1,0xC0},
//{0xBF,0xBE,0xBD,0xBC,0xBB,0xBA,0xB9,0xB8,0xB7,0xB6,0xB5,0xB4,0xB3,0xB2,0xB1,0xB0},
//{0xAF,0xAE,0xAD,0xAC,0xAB,0xAA,0xA9,0xA8,0xA7,0xA6,0xA5,0xA4,0xA3,0xA2,0xA1,0xA0},
//{0x9F,0x9E,0x9D,0x9C,0x9B,0x9A,0x99,0x98,0x97,0x96,0x95,0x94,0x93,0x92,0x91,0x90},
//{0x8F,0x8E,0x8D,0x8C,0x8B,0x8A,0x89,0x88,0x87,0x86,0x85,0x84,0x83,0x82,0x81,0x80},
//{0x7F,0x7E7,0x7D,0x7C,0x7B,0x7A,0x79,0x78,0x77,0x76,0x75,0x74,0x73,0x72,0x71,0x70},
//{0x6F,0x6E,0x6D,0x6C,0x6B,0x6A,0x69,0x68,0x67,0x66,0x65,0x64,0x63,0x62,0x61,0x60},
//{0x5F,0x5E,0x5D,0x5C,0x5B,0x5A,0x59,0x58,0x57,0x56,0x55,0x54,0x53,0x52,0x51,0x50},
//{0x4F,0x4E,0x4D,0x4C,0x4B,0x4A,0x49,0x48,0x47,0x46,0x45,0x44,0x43,0x42,0x41,0x40},
//{0x3F,0x3E,0x3D,0x3C,0x3B,0x3A,0x39,0x38,0x37,0x36,0x35,0x34,0x33,0x32,0x31,0x30},
//{0x2F,0x2E,0x2D,0x2C,0x2B,0x2A,0x29,0x28,0x27,0x26,0x25,0x24,0x23,0x22,0x21,0x20},
//{0x1F,0x1E,0x1D,0x1C,0x1B,0x1A,0x19,0x18,0x17,0x16,0x15,0x14,0x13,0x12,0x11,0x10},
//{0x0F,0x0E,0x0D,0x0C,0x0B,0x0A,0x09,0x08,0x07,0x06,0x05,0x04,0x03,0x02,0x01,0x00}
};

#ifdef ETH_BRDG_DEBUG
u8 myDebugFlag = FALSE;
u8 myDebugFlag1 = FALSE;
u32 numTxFrms=0;
u32 numTxDoneInts=0;
u32 numPlcPendingRet=0;
u32 numForcePlcTxDone=0; 
u32 ethRxFrameCnt=0;
u32 ethTxFrameCnt=0;
u32 numEthTxDoneInts=0;
u32 oldNumEthTxDoneInts=0;
u32 ethTxWriteFail=0;
u32 numEthTxCp=0;
u32 numEthTxCpReleased=0;
u32 numPlcTxCp=0;
u32 plcTxWriteFail=0;
u32 TotalRxCpCnt=0;
u32 TotalRxFrameCnt=0;
u32 plcRxFrameCnt = 0;
u32 plcTxFrameCnt = 0;
u8 EthTxQueueCnt();
u8 PlcTxQueueCnt();
extern u8 gPlcPendingHead;
extern u8 gPlcPendingTail;
extern u8 ethTxDone;
#endif
extern u8 gNekEks;                                     
u8 bcnupdate = 1;                                  
void HHT_SendBcn(u8 bcnType)
{
	u8              *bcnMPDU = gHpgpHalCB.lmBcn.cBcn.bcnBuff;//[PLC_BCN_LEN] = {0};
    u8 i;
	sHybriiPlcTxBcn* pPlcTxBcn;
	//sFrmCtrlBlk*    pAvFcBlk;
	sBcnHdr*        pBcnHdr;
	sBeHdr*         pBeHdr;
//    u16             bto;
	u8              bpstoValueAddr;
	u8              bpstoValueOffset;
	sBEntry 	*   pEntry;
	sRegion 	    v_region;
	sStEtTime       st_et;
    
    if(bcnupdate == 1) 
    {
    	pPlcTxBcn		 = (sHybriiPlcTxBcn*) bcnMPDU;
    	pBcnHdr          = &pPlcTxBcn->bcnHdr;
    	pBeHdr           = (sBeHdr*) &pPlcTxBcn->bcnHdr + sizeof(sBcnHdr);
    	
#if 1
		bcnupdate = 0;
        pPlcTxBcn->avFcBlk.bto[0] = PLC_PHY_TXLATENCY_FOR_TCC3;  //PLC_PHY_TXLATENCY_FOR_TCC2;
        pPlcTxBcn->avFcBlk.bto[1] = PLC_PHY_TXLATENCY_FOR_TCC3;
        pPlcTxBcn->avFcBlk.bto[2] = PLC_PHY_TXLATENCY_FOR_TCC3;
        pPlcTxBcn->avFcBlk.bto[3] = PLC_PHY_TXLATENCY_FOR_TCC3;
    	pEntry           = (sBEntry *)pPlcTxBcn->bcnPld;
    	memset(bcnMPDU, 0, sizeof(bcnMPDU));


    //[YM] - Beacon frame construction should be separated with Beacon frame Tx control
	// Beacon Header
    memcpy(pBcnHdr->nid, gHpgpHalCB.nid, NID_LEN);
//    pBcnHdr->nid[6] = (pBcnHdr->nid[6] & NID_EXTRA_BIT_MASK) | (HYBRID_MODE_FULL << 6);
    pBcnHdr->nid[6] = (pBcnHdr->nid[6] & 0x3F) | (HYBRID_MODE_FULL << 6);
	
	pBcnHdr->stei	 = gHpgpHalCB.selfTei;
	pBcnHdr->bt      = bcnType;
	pBcnHdr->slotusage      = 1;
	pBcnHdr->nm      = NET_MODE_CSMA_ONLY;
	pBcnHdr->ccocap  = CCO_CAP_LEVEL0;
	pBcnHdr->nbe     = 4;

    // Beacon Entry

    pEntry->beHDR = BEHDR_PERSISTENT_SCHED;
    pEntry->beLen = 6;
	pEntry->entry.persist_sch.pscd = 0;
	pEntry->entry.persist_sch.cscd = 0;
	pEntry->entry.persist_sch.ns   = 1;
	pEntry->entry.persist_sch.sai[0].sai4.stpf = 1;
	pEntry->entry.persist_sch.sai[0].sai4.stpf = 1;
	pEntry->entry.persist_sch.sai[0].sai4.glid = 0x7E;

    st_et.reg32 = MAKE_ST_ET(0x0034, 0x0cb8);
	st_et.reg32 = ctorl(st_et.reg32);

	pEntry->entry.persist_sch.sai[0].sai4.st_et[0]   = st_et.st_et[0];
	pEntry->entry.persist_sch.sai[0].sai4.st_et[1]   = st_et.st_et[1];
	pEntry->entry.persist_sch.sai[0].sai4.st_et[2]   = st_et.st_et[2];

    pEntry = (sBEntry *)((u8 *)pEntry + 8);


    pEntry->beHDR = BEHDR_REGIONS;  // 2 bytes
   /* if(gHpgpHalCB.lineMode == LINE_MODE_AC)
    {
        pEntry->beLen = 7;
        pEntry->entry.regions.nr  = 3;    
    }
    else  */
    {
        pEntry->beLen = 13;
        pEntry->entry.regions.nr  = 6;
//        pEntry->beLen = 5;
//        pEntry->entry.regions.nr  = 2;
    }    

    for(i = 0; i < pEntry->entry.regions.nr; i++)
    {
    
        if((i == 0) || (i == 2))  //just set this to test stayout region , so for AC we want 1st region and 3rd region as stayout
        {
            v_region.s.region_type = REGION_TYPE_STAYOUT;
        }
        else 
        {
            v_region.s.region_type = REGION_TYPE_SHARED_CSMA;
        }
        
        if(gHpgpHalCB.lineMode == LINE_MODE_DC)
        {
            v_region.s.region_end_time = CSMA_REGIONS_VAL_DC[i];
        }
        else if(gHpgpHalCB.lineMode == LINE_MODE_AC)
        {
            v_region.s.region_end_time = CSMA_REGIONS_VAL_AC[i];
        }
        pEntry->entry.regions.regn[i].reg16 = ctors(v_region.reg16);

    }
   /* if(gHpgpHalCB.lineMode == LINE_MODE_AC)
    {
        pEntry = (sBEntry *)((u8 *)pEntry + 9);   // pEntry->beHDR = BEHDR_REGIONS; + pEntry->beLen  // 7+ BEHDR_REGIONS 2 bytes = 9
    }
    else  */
    {
         pEntry = (sBEntry *)((u8 *)pEntry + 15);
//         pEntry = (sBEntry *)((u8 *)pEntry + 7);   
    }

    pEntry->beHDR = BEHDR_MAC_ADDR;
    pEntry->beLen = 6;
    pEntry->entry.mac_address[0] = 0xCC;
    pEntry->entry.mac_address[1] = 0x5D;
    pEntry->entry.mac_address[2] = 0x4E;
    pEntry->entry.mac_address[3] = 0x00;
    pEntry->entry.mac_address[4] = 0x35;
    pEntry->entry.mac_address[5] = 0x4E;
    pEntry = (sBEntry *)((u8 *)pEntry + 8);

    pEntry->beHDR = BEHDR_BPSTO;
    pEntry->beLen = 3;
    pEntry->entry.bpsto[0] = 0x28;
    pEntry->entry.bpsto[1] = 0x1B;
    pEntry->entry.bpsto[2] = 0x00;

    bpstoValueOffset = (u8 *)pEntry -  (u8 *)pPlcTxBcn  + 2;

    bpstoValueAddr   = (u8*) pBeHdr + sizeof(sBeHdr) - (u8*) pPlcTxBcn;
        gHpgpHalCB.lmBcn.cBcn.bpstoOffset = bpstoValueOffset;
    }

	
   // HHAL_PlcBcnWrite(bcnMPDU, bcnType, bpstoValueOffset);


#else
    //printf("bto= %bx%02bx\n", pAvFcBlk->bto0[1],pAvFcBlk->bto0[0]);
#if (PLC_BCNDATA_TXHANG_TEST  && PLC_BCNDATA_FIXED_PATTERN)
   for(i=1 ; i<PLC_BCN_LEN ; i++)
    {
        bcnMPDU[i] = 0x55;
    }
#else										
	// Beacon Header
	pBcnHdr->stei	 = gHpgpHalCB.selfTei;
	pBcnHdr->bt      = bcnType;
	pBcnHdr->nm      = NET_MODE_CSMA_ONLY;
	pBcnHdr->ccocap  = CCO_CAP_LEVEL0;
	pBcnHdr->nbe     = 1;
    memcpy(pBcnHdr->nid, gHpgpHalCB.nid, NID_LEN);

    // Beacon Entry
	pBeHdr->beType   = BEHDR_BPSTO;
	pBeHdr->beLen    = 3;

	for( i=bpstoValueAddr + 3 ; i< (PLC_BCN_PLD_LEN + HPGP_AVFC_LEN + HPGP_HP10FC_LEN) ; i++)
	{
		bcnMPDU[i] = i;
	}
		
    if(!gHpgpHalCB.halStats.TxBcnCnt)
    {
    //printf("bcnPldLen = %bd, bpstoValueAddr = %bd, lastByteIdx=%bu\n", sizeof(sHybriiPlcTxBcn)+5, bpstoValueAddr, i );
    }
#endif
    bpstoValueAddr   = (u8*) pBeHdr + sizeof(sBeHdr) - (u8*) pPlcTxBcn;
	HHAL_PlcBcnWrite(bcnMPDU, bcnType, bpstoValueAddr);
#endif

}

#if 0
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

#endif

eHpgpRoboMod HHT_GetRoboModFrmLen ( u8 stdModeSel, u16 frmLen, eHpgpHwFrmType frmType)
{
    eHpgpRoboMod		  roboMode; 
    u16                   miniRoboFrmLenMax;
    u16                   stdRoboFrmLenMax;
    u16                   hsRobo2PbFrmLenMax;
    u16                   hsRobo3PbFrmLenMax;
    if(frmType == HPGP_HW_FRMTYPE_MGMT)
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
    if(frmLen <= miniRoboFrmLenMax)
    {
        roboMode       = HPGP_ROBOMD_MINI;
    }
    else if( (frmLen > miniRoboFrmLenMax) && (frmLen <= stdRoboFrmLenMax))
    {
        if(stdModeSel)
        {
            roboMode    = HPGP_ROBOMD_STD;
        } else
        {
            roboMode    = HPGP_ROBOMD_HS;
        }                    
    }
    else
    {
        roboMode       = HPGP_ROBOMD_HS;
     }
	 return roboMode;
}


void update_powermode(u8 TxRxPowermode)
{
    char            input[10];
    u8 powermode;
   
   
    if(TxRxPowermode == 0)
    {
        do
        {
            printf("Enter Transmission Power mode  : 0 - Automotive, 1 - Normal, 2 - High Power ");
            while (getline(input, sizeof(input)) > 0)
            {
                if(sscanf(input,"%bd",&powermode) >= 1)
                break;
            }
        }while (powermode>2);
        if(powermode == 0)
        {
           mac_utils_spi_write(0x34,0x08);   //added by varsha
           mac_utils_spi_write(0x35,0x30);   //added by varsha
            
        }
        else if(powermode == 1)
        {
            mac_utils_spi_write(0x34,0x00);   //added by varsha
            mac_utils_spi_write(0x35,0x00);   //added by varsha
        }
        else if(powermode == 2)
        {
            mac_utils_spi_write(0x34,0x00);   //added by varsha
            mac_utils_spi_write(0x35,0x0f);   //added by varsha
        }
    }
    else
    {
        do
        {
            printf("Enter Receiver Power mode  : 0 - Normal, 1 - Power Saving ");
            while (getline(input, sizeof(input)) > 0)
            {
                if(sscanf(input,"%bd",&powermode) >= 1)
                break;
            }
        }while (powermode>1);  

         if(powermode == 0)
        {
           mac_utils_spi_write(0x26,0x00);   //added by varsha
          
            
        }
        else if(powermode == 1)
        {
            mac_utils_spi_write(0x26,0x1C);   //added by varsha
           
        }



    }


}



#if 0

void HHT_SimulateTx(sPlcSimTxTestParams* pTestParams)
{
//  sPlcTxFrmSwDesc plcTxFrmSwDesc;
//#ifdef Packet_grouping
    u32             Sw_command_queue[8];
    u8              cmd_num;
//#endif	
//    uPlcTxPktQCAP_Write   cap_write;
    uTxCMDQueueWrite      txCmdQueueWrtie;
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
    u8              i;
    u8              j;
    u8              quit; 
    u16             tmpPayloadLen;
	u8 				cp_localBuf[HYBRII_CELLBUF_SIZE];	// local CP buffer
    u8              dbc_pktchange;

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
                if( CHT_Poll() == 'q')
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
                        FM_Printf(FM_LINFO,"OffsetDW & DescLen resetting to %bu & %bu respectively.\n", curOffsetDW, curDescLen);
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
                FM_Printf(FM_LINFO,"curFrmLen = %u, curOffsetDW = %bu, curDescLen=%bu, free CPCnt = %bu\n", 
                                             plcTxFrmSwDesc.frmLen, tmpOffsetDW, actualDescLen, CHAL_GetFreeCPCnt());
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
            FM_Printf(FM_LINFO,"cp = %bu, cellAddr=%08lX, seqNum=%bu\n",cp,(u32)cellAddr, gHpgpHalCB.halStats.TxSeqNum);
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
#endif
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
						   printf("Write PLC_CMDQ_REQ = %u\n", Sw_command_queue[p]);
					   	}
					   if (cmd_num > 0)
					       printf("1. Write PLC_CMDQ_REG %bu times\n", cmd_num);
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
                status = HHAL_PlcTxQWrite(&plcTxFrmSwDesc);
				if (status == STATUS_FAILURE)
				{
					pkt_retry++;
					//printf("Write PLC Tx Q failed, %bu times, cp = %bu\n", pkt_retry, plcTxFrmSwDesc.cpCount);
					// [YM] - retry the packet transmission until it failed too many times

                    CHAL_DelayTicks(64);
					if (pkt_retry > 20)
					{
                        printf("\nWrite PLC Tx Q failed, cp = %bu, Quit !!!!\n", plcTxFrmSwDesc.cpCount);
					    quit = 1;
					    break;
					}
				}
                // check for user initiated exit task from infinite loop
                if( CHT_Poll() == 'q')              
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
#ifndef MPER			
            printf("Sent %ld, Received %ld HPGP frames.", gHpgpHalCB.halStats.CurTxTestFrmCnt, gHpgpHalCB.halStats.TotalRxGoodFrmCnt - gHpgpHalCB.halStats.RxGoodBcnCnt);
#else
            printf("Sent %ld, Received %ld HPGP frames.", gHpgpHalCB.halStats.CurTxTestFrmCnt, gHpgpHalCB.halStats.TotalRxGoodFrmCnt - gHpgpHalCB.halStats.RxGoodBcnCnt - gHpgpHalCB.halStats.DuplicateRxCnt);
#endif
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
                                      
					u8 userInput = CHT_Poll();  
					//printf("<E>\n");  
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
                if( CHT_Poll() == 'q')              
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
        printf("2. Write PLC_CMDQ_REG %bu times\n", cmd_num);
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


            printf("Sent %ld HPGP frames.\n", gHpgpHalCB.halStats.CurTxTestFrmCnt);
            printf("Quit Tx: Free CP Cnt = %bu, curFrmLen = %u\n", CHAL_GetFreeCPCnt(), curFrmLen);
            break;
        }       
    } // while(1)   
}

#endif

void HHT_SimulateTxTestMenu()
{
	sPlcSimTxTestParams testParams;
	// test mode variables
	u8              eks;
    eSecTestMode    secTestMode;
	u16             frmLen;
	u8              frmType;
	u8              stdModeSel = 0;
	eRoboTestMode   roboTestMode;
	eFrmMcstMode    mcstMode;
	eHpgpPlidValue  plid;
	u32             numFrames;
	eLenTestMode    lenTestMode;
    u8              alterDescLenNOffset;  
	u16             miniRoboFrmLenMax;
	u16             stdRoboFrmLenMax;
	u16             HS2PBRoboFrmLenMax;
	u16             HS3PBRoboFrmLenMax;
	u32             delay;	
	char            input[10];

    memset(&testParams, 0, sizeof(testParams));
    eks = HPGP_UNENCRYPTED_EKS;
    //Set Frame Length
	do
	{
#ifdef ER_TEST
        printf("Enter FrameLen  : 0-Inc/Mixed Len, 1 to %u -Fixed Len  :: ", ER_PACKET_LIMIT );
#else
		printf("Enter FrameLen  : 0-Inc/Mixed Len, 1 to 1536-Fixed Len  :: ");
#endif
		while (getline(input, sizeof(input)) > 0)
		{
			if(sscanf(input,"%d",&frmLen) >= 1)
			break;
		}
#ifdef ER_TEST
    }while (frmLen> ER_PACKET_LIMIT);
#else
	}while (frmLen>1536);
#endif
	// For continuous mode (frame length = 0)
	if(frmLen == 0)
	{
		do
		{
			printf("Enter Len mode : 0-IncSingleRobo , 1-IncAllRobo, 2-AlterRobo :: ");
			while (getline(input, sizeof(input)) > 0)
			{
				if(sscanf(input,"%bd",&lenTestMode) >= 1)
				break;
			}
		}while(lenTestMode>2);  
	}
	else
	{
		  lenTestMode = FIXED_LEN;
	}

	//Select Test mode for continuous Tx test mode
	if(frmLen == 0 && lenTestMode == 0)
	{
		do
		{
			printf("Enter Robo Mode : 0-Mini, 1-Std, 2-HS1PB, 3-HS2PB, 4-HS3PB, 5-AllHS :: ");
			while (getline(input, sizeof(input)) > 0)
			{
				if(sscanf(input,"%bd",&roboTestMode) >= 1)
				break;
			}
		}while(roboTestMode>5); 
	}

	//Select Frame type
	do
	{
		printf("Enter Frame Type : 0-Mgmt, 1-Data, 2-Sound, 3-Alter              :: ");
		while (getline(input, sizeof(input)) > 0)
		{
			if(sscanf(input,"%bd",&frmType) >= 1)
			break;
		}
	}while (frmType>3);

	//For sound packet
           if(frmType == 2)
            {
                do
                {
                    printf("Ack Flag : 0/1                                          :: ");
                    while (getline(input, sizeof(input)) > 0)
                    {
                        if (sscanf(input,"%bd",&testParams.saf) >= 1)
                            break;
                    }
                } while (testParams.saf > 1);
                do
                {
                    printf("Complete Flag : 0/1                                     :: ");
                    while (getline(input, sizeof(input)) > 0)
                    {
                        if (sscanf(input,"%bd",&testParams.scf) >= 1)
                            break;
                    }
                } while (testParams.scf > 1);
                do
                {
                    printf("Enter Sound Reason Code : 0-Tone Map Error, "
                           "1-No AC Line Tone Map, 2-Unusable Interval :: ");
                    while (getline(input, sizeof(input)) > 0)
                    {
                        if (sscanf(input,"%bd",&testParams.src) >= 1)
                            break;
                    }
                } while (testParams.src > 2);
            }
    // For Management packet
        if(frmType == 0)
        {
            // get robo mode max lens for Mgmt
            miniRoboFrmLenMax  = HYBRII_MINIROBO_MGMTLEN_MAX;
            stdRoboFrmLenMax   = HYBRII_STD1PBHSROBO_MGMTLEN_MAX;
#if 0
            do
	        {
                printf("Enter DT_AV : 0-SOF, 1-SOUND                            :: ");
                while (getline(input, sizeof(input)) > 0)
                {
                    if (sscanf(input,"%bd",&testParams.dt_av) >= 1)
                        break;
                }
	        } while (testParams.dt_av > 1);

            if(testParams.dt_av == 1)
            {
                do
                {
                    printf("Ack Flag : 0/1                                          :: ");
                    while (getline(input, sizeof(input)) > 0)
                    {
                        if (sscanf(input,"%bd",&testParams.saf) >= 1)
                            break;
                    }
                } while (testParams.saf > 1);
                do
                {
                    printf("Complete Flag : 0/1                                     :: ");
                    while (getline(input, sizeof(input)) > 0)
                    {
                        if (sscanf(input,"%bd",&testParams.scf) >= 1)
                            break;
                    }
                } while (testParams.scf > 1);
                do
                {
                    printf("Enter Sound Reason Code : 0-Tone Map Error, "
                           "1-No AC Line Tone Map, 2-Unusable Interval :: ");
                    while (getline(input, sizeof(input)) > 0)
                    {
                        if (sscanf(input,"%bd",&testParams.src) >= 1)
                            break;
                    }
                } while (testParams.src > 2);
            }
#endif
    }
	else   //Select Data packet as test packet
	{
		// get robo mode max lens for Data
		miniRoboFrmLenMax             = HYBRII_MINIROBO_DATALEN_MAX;
		stdRoboFrmLenMax			  = HYBRII_STD1PBHSROBO_DATALEN_MAX;
		HS2PBRoboFrmLenMax            = HYBRII_2PBHSROBO_DATALEN_MAX;
		HS3PBRoboFrmLenMax            = HYBRII_3PBHSROBO_DATALEN_MAX;
	}

	if(frmType == 2)  // Sound packet
    {
       if(frmLen > 136)
           stdModeSel = 0;
       else
           stdModeSel = 2;
    }
    else
    {
    	if( (frmLen > miniRoboFrmLenMax) && (frmLen <= stdRoboFrmLenMax))
    	{
    		printf("Enter RoboMode : 0-HSRobo, 1-StdRobo                   :: ");
    		while (getline(input, sizeof(input)) > 0)
    		{
    			if(sscanf(input,"%bd",&stdModeSel) >= 1)
    			break;
    		}		  
    	}
    
        if ((frmLen > stdRoboFrmLenMax) && (frmLen <= HS2PBRoboFrmLenMax))
    		stdModeSel = 0x1;
    	else if (frmLen > HS3PBRoboFrmLenMax)
    		stdModeSel = 0x1;
    	else
    		stdModeSel = 0x2;
	}
#ifndef MPER
    do
	{
		printf("Enter Mcst Mode : 0-Ucst, 1-Mcst, 2-Mnbcst, 3-Alter     :: ");
		while (getline(input, sizeof(input)) > 0)                             
		{
			if(sscanf(input,"%bu",&mcstMode) >= 1)
			break;
		}
	}while(mcstMode > 3);
#endif

	do
	{
		printf("Enter PLID : 0 to 3-Fixed PLID, 4-Alter                 :: ");
		while (getline(input, sizeof(input)) > 0)
		{
			if(sscanf(input,"%bu",&plid) >= 1)
			break;
		}
	}while(plid>4);
	do
	{
		printf("Enable DescLen & Offset Test ? : 0-Disable, 1-Enable    :: ");
		while (getline(input, sizeof(input)) > 0)
		{
			if(sscanf(input,"%bu",&alterDescLenNOffset) >= 1)
			break;
		}
	}while(alterDescLenNOffset != 0 && alterDescLenNOffset!= 1);
	do
	{
		printf("Enter Sec Test Mode : 0-UnEnc, 1-Enc, 2-Mix_UnEncNek, 3-Mix_UnEncNekPpek :: ");
		while (getline(input, sizeof(input)) > 0)
		{
			if(sscanf(input,"%bu",&secTestMode) >= 1)
			break;
		}
	}while(secTestMode > 3);

    if(secTestMode == 0)
	{
		eks = HPGP_UNENCRYPTED_EKS;
	}    
	else if(secTestMode == 1)
    {
		do
		{
			printf("Enter EKS : NEK-0to7, PPEK-8,9 ");
			while (getline(input, sizeof(input)) > 0)
			{
				if(sscanf(input,"%bd",&eks) >= 1)
				break;
			}
		}while( eks>0x9 );
    }
    else
	{
		eks = 0;
	}

	if(!gHpgpHalCB.diagModeEnb)
	{
		printf("Enter number of frames: 0-ContMode, N-NumOfFrames       :: ");
		while (getline(input, sizeof(input)) > 0)
		{
			if(sscanf(input,"%ld",&numFrames) >= 1)
			break;
		}
	}
	else
	{
		numFrames = 1;
	}
	printf("Enter delay (unit of 64 timer ticks)    :: ");
	while (getline(input, sizeof(input)) > 0)
	{
		if(sscanf(input,"%lu",&delay) >= 1)
		break;
	}

	// fill tx test params structure
	testParams.numFrames  = numFrames;
	if(!testParams.numFrames)
	{
		testParams.contMode = 1;
	}
	else
	{
		testParams.contMode = 0;
	}
	testParams.frmLen = frmLen;
	testParams.lenTestMode = lenTestMode;
	
    // [YM] roboTestMode value assignment
	testParams.roboTestMode   = roboTestMode;
	
	if(frmType == 3)
	{
		 testParams.altFrmTypeTest = 1;
		 testParams.frmType        = 0;
	}
	else
	{
		testParams.frmType = frmType;
		testParams.altFrmTypeTest = 0;
	}
	if(plid == 4)
	{
		  testParams.altPlidTest = 1;
	}
	else
	{
        testParams.altPlidTest = 0;
		testParams.plid = plid;
	}
	
	// [YM] StdModeSel assignment
	testParams.stdModeSel = stdModeSel;
	
    if(mcstMode == 3)
    {
        testParams.altMcstTest = 1;
        testParams.mcstMode    = 0;
    }
    else
    {
        testParams.altMcstTest = 0;
#ifndef MPER		
        testParams.mcstMode    = mcstMode;
#else
        testParams.mcstMode    = 0;   //set unicast as default mode [YM]
#endif
    }
    testParams.secTestMode = secTestMode;
	testParams.eks        = eks;
	testParams.altOffsetDescLenTest = alterDescLenNOffset;
    if(alterDescLenNOffset)
    {
		testParams.offsetDW  = 31;
		testParams.descLen   = 3;
    }
    else
    {
		testParams.offsetDW  = 0;
		testParams.descLen   = HYBRII_CELLBUF_SIZE;
    }
    testParams.delay = delay;
    // Trigger the tx test
    HHT_SimulateTx(&testParams);
}

void sendSingleFrame(u8 mcstMode)  //1 = multicast, 0 unicast
{
    
    sPlcSimTxTestParams testParams;
    memset(&testParams, 0, sizeof(testParams));
    testParams.numFrames      = 1;

	testParams.contMode = 0;

	testParams.frmLen         = 1000;
	testParams.lenTestMode    = FIXED_LEN;
	//testParams.roboTestMode   = roboTestMode;
	testParams.frmType        = 1;
    testParams.altFrmTypeTest = 0;
    testParams.altMcstTest    = 0;
	testParams.plid           = 0;
	testParams.altPlidTest    = 0;
	testParams.stdModeSel     = 0;
	testParams.mcstMode       = mcstMode;
	testParams.offsetDW       = 0;
	testParams.descLen        = HYBRII_CELLBUF_SIZE;
    testParams.secTestMode    = UNENCRYPTED;
	testParams.eks            = HPGP_UNENCRYPTED_EKS;
    testParams.altOffsetDescLenTest = 0;
	testParams.delay          = 40;
	testParams.plcMultiPktTest = 0;
	// Trigger the tx test
#ifdef Packet_grouping0
	if (plcMultiPktTest > 1)
	{
	    gHpgpHalCB.plcMultiPktTest = plcMultiPktTest;  //[YM] Enable multiple packet queueing for Tx test		
	} 
#endif	
    //printf(" (A) gHpgpHalCB.plcMultiPktTest = %bu\n", gHpgpHalCB.plcMultiPktTest);
	HHT_SimulateTx(&testParams);
}

void HHT_BasicTxMenu()
{
	sPlcSimTxTestParams testParams;
	// test mode variables
	u8              dbc, pattern, rts, cts, rsof;
    u16             frmLen;
	u8              stdModeSel;
	eRoboTestMode   roboTestMode;
	eFrmMcstMode    mcstMode;
	u8              cap;
//	u8              plcMultiPktTest;
	u32             numFrames;
	u16             miniRoboFrmLenMax;
	u16             stdRoboFrmLenMax;
	u32             delay;
	u8              input[8];

    // get robo mode max lens for Data
	miniRoboFrmLenMax             = HYBRII_MINIROBO_DATALEN_MAX;
	stdRoboFrmLenMax			  = HYBRII_STD1PBHSROBO_DATALEN_MAX;

#ifndef MPER    
    {
        printf("Do DBC Test?  : 1 = YES, other = No  :: ");
		while (getline(input, sizeof(input)) > 0)
		{
			if(sscanf(input,"%bu",&dbc) >= 1)
			break;
		}
		if (dbc != 1)
		{
		    dbc = 0;
		//printf("DBC command = %x\n", dbc);
		//else
		    printf("Do RTS/CTS Tx Test?  : 1 = YES, other = No  :: ");
            while (getline(input, sizeof(input)) > 0)
		    {
			   if(sscanf(input,"%bu",&rts) >= 1)
			   break;
		    }
		    if (rts!= 1)
			{
		       rts = 0;
			//else
			//{
			    printf("Do Reverse SOF Tx Test?  : 1 = YES, other = No  :: ");
                while (getline(input, sizeof(input)) > 0)
		        {
			       if(sscanf(input,"%bu",&rsof) >= 1)
			       break;
		        }
		        if (rsof!= 1)
		           rsof= 0;
			}
		}			
    }
#endif
#ifdef MPER
  if (dbc == 1)  // [YM] Do DBC Test
  {

	do
	{
		printf("Enter DBC Test Pattern: 1-1RoBo+1MiniRoBo, 2-3 MiniRobo, 3-2MiniRoBo+1HSRoBo1PB,\n");
		printf("4-1MiniRoBo+1HSRoBo2PB, 5-2HSRoBo1PB, 6-1MiniRoBo+1HSRoBo3PB, 7-1RoBo+1HSRoBo1PB, 8-1HSRoBo1PB+1HSRoBo2PB  :: ");
		while (getline(input, sizeof(input)) > 0)
		{
			if(sscanf(input,"%bu",&pattern) >= 1)
			break;
		}
	}while(pattern<1 && pattern>6);

	do
	{
		printf("Enter McstMode: 0-Ucst, 1-Mcst, 2=Mnbcst  :: ");
		while (getline(input, sizeof(input)) > 0)
		{
			if(sscanf(input,"%bu",&mcstMode) >= 1)
			break;
		}
	}while(mcstMode > 2);
	cap = 0;
/*
	{
		printf("Enter Cap or Plid Value for Test Packet (0~3, 4 = all caps): ");
		while (getline(input, sizeof(input)) > 0)
		{
			if(sscanf(input,"%bu",&cap) < 5)  //[YM] add cap = 4 for random select cap 0~3
			break;
		}
	}while(cap > 4); 
*/
    if(!gHpgpHalCB.diagModeEnb)
	{
		printf("Enter number of frames:  N-NumOfFrames :: ");
		while (getline(input, sizeof(input)) > 0)
		{
			if(sscanf(input,"%lu",&numFrames) >= 1)
			break;
		}
	}
	else
	{
		numFrames = 1;
	}

	printf("Enter delay (unit of 64 timer ticks)    :: ");
	while (getline(input, sizeof(input)) > 0)
	{
		if(sscanf(input,"%lu",&delay) >= 1)
		break;
	}

	// Construct Test Packet
	// fill tx test params structure
	testParams.numFrames      = numFrames;
	if(!testParams.numFrames)
	{
		testParams.contMode = 1;
	}
	else
	{
		testParams.contMode = 0;
	}
	testParams.dbc            = dbc;
	testParams.pattern        = pattern;
	testParams.frmLen         = 100;  //Not important here
	testParams.lenTestMode    = VARY_LEN;
	testParams.roboTestMode   = 0;  //Not important here
	testParams.frmType        = 1;
    testParams.altFrmTypeTest = 0;
    testParams.altMcstTest    = 0;
	if (cap == 4)
	{
	testParams.plid           = 0;  //[YM] temporary set cap to 0, cap value will be changed in sequence
	testParams.altPlidTest    = 1;
	}
	else
	{
	testParams.plid           = cap;
	testParams.altPlidTest    = 0;		
	}
	testParams.stdModeSel     = 1;  // Not important here
	testParams.mcstMode       = mcstMode;
	testParams.offsetDW       = 0;
	testParams.descLen        = HYBRII_CELLBUF_SIZE;
    testParams.secTestMode    = UNENCRYPTED;
	testParams.eks            = HPGP_UNENCRYPTED_EKS;
    testParams.altOffsetDescLenTest = 0;
	testParams.delay          = delay;
	testParams.plcMultiPktTest = 1;  // Not important here
	
  }
  else if (rts == 1)
  {
    do
	{
		printf("Enter TestMode: 0- Tx RTS, 1-Tx CTS  :: ");
		while (getline(input, sizeof(input)) > 0)
		{
			if(sscanf(input,"%bu",&cts) >= 1)
			break;
		}
	}while(cts > 1);
	
    do
	{
		printf("Enter McstMode: 0-Ucst, 1-Mcst, 2=Mnbcst  :: ");
		while (getline(input, sizeof(input)) > 0)
		{
			if(sscanf(input,"%bu",&mcstMode) >= 1)
			break;
		}
	}while(mcstMode > 2);
	cap = 0;

    if(!gHpgpHalCB.diagModeEnb)
	{
		printf("Enter number of frames:  N-NumOfFrames :: ");
		while (getline(input, sizeof(input)) > 0)
		{
			if(sscanf(input,"%lu",&numFrames) >= 1)
			break;
		}
	}
	else
	{
		numFrames = 1;
	}

	printf("Enter delay (unit of 64 timer ticks)    :: ");
	while (getline(input, sizeof(input)) > 0)
	{
		if(sscanf(input,"%lu",&delay) >= 1)
		break;
	}

	// Construct Test Packet
	// fill tx test params structure
	testParams.numFrames      = numFrames;
	if(!testParams.numFrames)
	{
		testParams.contMode = 1;
	}
	else
	{
		testParams.contMode = 0;
	}
	testParams.dbc            = 0;
	testParams.pattern        = 0;
	testParams.dt_av          = 3;
	testParams.frmLen         = 100;  //Not important here
	testParams.lenTestMode    = FIXED_LEN;
	testParams.roboTestMode   = 0;  //Not important here
	if (cts == 1)
		testParams.frmType        = HPGP_HW_FRMTYPE_CTS;
	else
	    testParams.frmType        = HPGP_HW_FRMTYPE_RTS;	
    testParams.altFrmTypeTest = 0;
    testParams.altMcstTest    = 0;
	if (cap == 4)
	{
	testParams.plid           = 0;  //[YM] temporary set cap to 0, cap value will be changed in sequence
	testParams.altPlidTest    = 1;
	}
	else
	{
	testParams.plid           = cap;
	testParams.altPlidTest    = 0;		
	}
	testParams.stdModeSel     = 1;  // Not important here
	testParams.mcstMode       = mcstMode;
	testParams.offsetDW       = 0;
	testParams.descLen        = HYBRII_CELLBUF_SIZE;
    testParams.secTestMode    = UNENCRYPTED;
	testParams.eks            = HPGP_UNENCRYPTED_EKS;
    testParams.altOffsetDescLenTest = 0;
	testParams.delay          = delay;
	testParams.plcMultiPktTest = 0;  // Not important here	
  }
  else if (rsof == 1)
  {
	
    do
	{
		printf("Enter McstMode: 0-Ucst, 1-Mcst, 2=Mnbcst  :: ");
		while (getline(input, sizeof(input)) > 0)
		{
			if(sscanf(input,"%bu",&mcstMode) >= 1)
			break;
		}
	}while(mcstMode > 2);
	cap = 0;

    if(!gHpgpHalCB.diagModeEnb)
	{
		printf("Enter number of frames:  N-NumOfFrames :: ");
		while (getline(input, sizeof(input)) > 0)
		{
			if(sscanf(input,"%lu",&numFrames) >= 1)
			break;
		}
	}
	else
	{
		numFrames = 1;
	}

	printf("Enter delay (unit of 64 timer ticks)    :: ");
	while (getline(input, sizeof(input)) > 0)
	{
		if(sscanf(input,"%lu",&delay) >= 1)
		break;
	}

	// Construct Test Packet
	// fill tx test params structure
	testParams.numFrames      = numFrames;
	if(!testParams.numFrames)
	{
		testParams.contMode = 1;
	}
	else
	{
		testParams.contMode = 0;
	}
	
	testParams.dbc            = 0;
	testParams.pattern        = 0;
	testParams.dt_av          = 5;  //RSOF
	testParams.frmLen         = 100;  //Not important here
	testParams.lenTestMode    = FIXED_LEN;
	testParams.roboTestMode   = 0;  //Not important here
	//if (cts == 1)
		//testParams.frmType        = HPGP_HW_FRMTYPE_CTS;
	//else
	  //  testParams.frmType        = HPGP_HW_FRMTYPE_RTS;
	
    testParams.altFrmTypeTest = 0;
    testParams.altMcstTest    = 0;
	if (cap == 4)
	{
	testParams.plid           = 0;  //[YM] temporary set cap to 0, cap value will be changed in sequence
	testParams.altPlidTest    = 1;
	}
	else
	{
	testParams.plid           = cap;
	testParams.altPlidTest    = 0;		
	}
	testParams.stdModeSel     = 1;  // Not important here
	testParams.mcstMode       = mcstMode;
	testParams.offsetDW       = 0;
	testParams.descLen        = HYBRII_CELLBUF_SIZE;
    testParams.secTestMode    = UNENCRYPTED;
	testParams.eks            = HPGP_UNENCRYPTED_EKS;
    testParams.altOffsetDescLenTest = 0;
	testParams.delay          = delay;
	testParams.plcMultiPktTest = 0;  // Not important here	
  }
  else
#endif  //MPER  	
  {    //regular SOF packet
	do
	{
		#ifdef ER_TEST
        printf("Enter FrameLen  : 1 to %u -Fixed Len  :: ", ER_PACKET_LIMIT);
#else
        printf("Enter FrameLen  : 1 to 1536-Fixed Len  :: ");
#endif
		while (getline(input, sizeof(input)) > 0)
		{
			if(sscanf(input,"%u",&frmLen) >= 1)
			break;
		}
#ifdef ER_TEST
    }while (frmLen<1 || frmLen> ER_PACKET_LIMIT);
#else
    }while (frmLen<1 || frmLen>1530);
#endif

#ifndef MPER
	do
	{
		printf("Enter McstMode: 0-Ucst, 1-Mcst, 2=Mnbcst  :: ");
		while (getline(input, sizeof(input)) > 0)
		{
			if(sscanf(input,"%bu",&mcstMode) >= 1)
			break;
		}
	}while(mcstMode > 2);
#endif

	if( (frmLen > miniRoboFrmLenMax) && (frmLen <= stdRoboFrmLenMax))
	{
		printf("Select Std RoboMode : 0-HSRobo, 1- StdRobo :: ");
		while (getline(input, sizeof(input)) > 0)
		{
			if(sscanf(input,"%bu",&stdModeSel) >= 1)
			break;
		}		  
	}

	{
		printf("Enter Cap or Plid Value for Test Packet (0~3, 4 = all caps): ");
		while (getline(input, sizeof(input)) > 0)
		{
			if(sscanf(input,"%bu",&cap) < 5)  //[YM] add cap = 4 for random select cap 0~3
			break;
		}
	}while(cap > 4);

	if(!gHpgpHalCB.diagModeEnb)
	{
		printf("Enter number of frames:  N-NumOfFrames :: ");
		while (getline(input, sizeof(input)) > 0)
		{
			if(sscanf(input,"%lu",&numFrames) >= 1)
			break;
		}
	}
	else
	{
		numFrames = 1;
	}
#ifdef Packet_grouping0
    // [YM] New control command for multiple packets in-queue testing 
    {
		printf("For multiple packets in-queue testing: N-NumOfFrames in queue before Tx (1 ~ 8)::");
		while (getline(input, sizeof(input)) > 0)
		{
			if(sscanf(input,"%bu",&plcMultiPktTest) > 0)
            {
                if (plcMultiPktTest > 8)
					plcMultiPktTest = 8;
                //printf(" plcMultiPktTest = %bu\n", plcMultiPktTest);
			    break;
            }
			//plcMultiPktTest = 1;   //[YM] Multiple packet transmission is not supported for Hybrii_B
		}
	}
#endif
	printf("Enter delay (unit of 64 timer ticks)    :: ");
	while (getline(input, sizeof(input)) > 0)
	{
		if(sscanf(input,"%lu",&delay) >= 1)
		break;
	}

	// fill tx test params structure
	testParams.numFrames      = numFrames;
	if(!testParams.numFrames)
	{
		testParams.contMode = 1;
	}
	else
	{
		testParams.contMode = 0;
	}
	testParams.frmLen         = frmLen;
	testParams.lenTestMode    = FIXED_LEN;
	testParams.roboTestMode   = roboTestMode;
	testParams.frmType        = 1;
    testParams.altFrmTypeTest = 0;
    testParams.altMcstTest    = 0;
	if (cap == 4)
	{
	testParams.plid           = 0;  //[YM] temporary set cap to 0, cap value will be changed in sequence
	testParams.altPlidTest    = 1;
	}
	else
	{
	testParams.plid           = cap;
	testParams.altPlidTest    = 0;		
	}
	testParams.stdModeSel     = stdModeSel;
#ifndef	MPER
	testParams.mcstMode       = mcstMode;
#else
    testParams.mcstMode       = 0;  //default set as unicast
#endif
	testParams.offsetDW       = 0;
	testParams.descLen        = HYBRII_CELLBUF_SIZE;
    testParams.secTestMode    = UNENCRYPTED;
	testParams.eks            = HPGP_UNENCRYPTED_EKS;
    testParams.altOffsetDescLenTest = 0;
	testParams.delay          = delay;
	testParams.plcMultiPktTest = 1;
	// Trigger the tx test
#ifdef Packet_grouping0
	if (plcMultiPktTest > 1)
	{
	    gHpgpHalCB.plcMultiPktTest = plcMultiPktTest;  //[YM] Enable multiple packet queueing for Tx test		
	}
#endif	
    //printf(" (A) gHpgpHalCB.plcMultiPktTest = %bu\n", gHpgpHalCB.plcMultiPktTest);
  	}  
	HHT_SimulateTx(&testParams);
	//gHpgpHalCB.plcMultiPktTest = 0;  
}

                                             
void HHT_TestMemoryTables()
{
	uAesCpuCmdStatReg   aesCpuCmd;
    eStatus             status;
    u8                  input[10];
	u8                  patternWr;
	u8                  patternRd;
	u8                  patternCmp;
	u16                 patternWr16;
	u16                 patternCmp16;
    u16                 patternRd16;
	u32                 patternWr32;
	u32                 patternCmp32;
    u32                 patternRd32;
	u16                 i;

    u8  testMode;
    
	do
	{
		printf("Enter the test: 0-DumpTables, 1-Walking 1s write readback test");
		while (getline(input, sizeof(input)) > 0)
		{
			if(sscanf(input, "%bu", &testMode) >= 1)
			break;
		}
	}while (testMode>1);

    status = STATUS_SUCCESS;

    if(testMode == 0)
    {
        printf("SSN Table :: \n");
		for( i=0 ; i<1024 ; i++)
		{
			hal_common_reg_32_write(PLC_SSNMEMADDR_REG,(u32)i);
            patternRd32 = hal_common_reg_32_read(PLC_SSNMEMDATA_REG);
			if((i & 0x0007) == 0)
			{
				printf("\n");
				printf("[0x%04X] : ", i);
			}
			printf("0x%08lX ,", patternRd32 );
		
		}
        printf("\n");
        printf("\nPPEK Addr LUT :: \n");

		aesCpuCmd.reg = ReadU32Reg(PLC_AESCPUCMDSTAT_REG);
		aesCpuCmd.s.cpuTblReq = 1;
		WriteU32Reg(PLC_AESCPUCMDSTAT_REG,aesCpuCmd.reg);
		for(i=0;i<256;i++)	
		{
			hal_common_reg_32_write(PLC_AESLUTADDR_REG,(u32)i);
			patternRd32 = hal_common_reg_32_read(PLC_AESLUTDATA_REG);
            printf("[0x%04X] : ", i);
            printf("0x%08lX \n", patternRd32 );
		}
		printf("\n");

        printf("\nAes Key Table :: \n");
		for(i=0;i<1024;i++)	
		{
			hal_common_reg_32_write(PLC_AESKEYLUTADDR_REG,(u32)i);
			patternRd32 = hal_common_reg_32_read(PLC_AESKEYLUTDATA_REG);
			if((i & 0x0007) == 0)
			{
				printf("\n");
				printf("[0x%04X] : ", i);
			}
			printf("0x%08lX ,", patternRd32 );
        }
        printf("\n");
		// Release CPU Lock on AES LUT
		aesCpuCmd.reg = ReadU32Reg(PLC_AESCPUCMDSTAT_REG);
        aesCpuCmd.s.cpuTblGnt = 0;
		aesCpuCmd.s.cpuTblReq = 0;
		WriteU32Reg(PLC_AESCPUCMDSTAT_REG, aesCpuCmd.reg);
        return;
    }
    // SSN memory Write - Readback verify
	patternWr16 = 0x01;
	for(i=0;i<1024;i++)	
	{
		hal_common_reg_32_write(PLC_SSNMEMADDR_REG,(u32)i);
		hal_common_reg_32_write(PLC_SSNMEMDATA_REG,patternWr16);
        patternWr16 = _irol_(patternWr16,1);
	}
	patternCmp16 = 0x01;
	for(i=0;i<1024;i++)	
	{
		hal_common_reg_32_write(PLC_SSNMEMADDR_REG,(u32)i);
		patternRd16 = hal_common_reg_32_read(PLC_SSNMEMDATA_REG);
		if(patternCmp16 != patternRd16)
		{
			printf("HHT_TestAesTables : SSN Addr 0x%02X mismatch: Wrote 0x%04X & Readback 0x%04X\n", i, patternCmp16, patternRd16);
            status = STATUS_FAILURE;
		}
		patternCmp16 = _irol_(patternCmp16,1);
	}
    if (status == STATUS_FAILURE)
		printf("HHT_TestAesTables : SSN RW was failed\n");
	else if (status == STATUS_SUCCESS)
		printf("HHT_TestAesTables : SSN RW was done\n");
	
	// Wait for Cpu Aes Lut access grant.
	aesCpuCmd.reg = ReadU32Reg(PLC_AESCPUCMDSTAT_REG);
	aesCpuCmd.s.cpuTblReq = 1;
	WriteU32Reg(PLC_AESCPUCMDSTAT_REG,aesCpuCmd.reg);
/*
	CHAL_DelayTicks(100);
	aesCpuCmd.reg = ReadU32Reg(PLC_AESCPUCMDSTAT_REG);
	if(!aesCpuCmd.s.cpuTblGnt)
	{
        printf("Access request to AES Tables denied.\n");
        status = STATUS_FAILURE;
		return ;
	}
*/
    // PPEK	Addr LUT Write - Readback verify
    patternWr = 0x01;
	for(i=0;i<256;i++)	
	{
		hal_common_reg_32_write(PLC_AESLUTADDR_REG,(u32)i);
		hal_common_reg_32_write(PLC_AESLUTDATA_REG,patternWr);
        patternWr = _crol_(patternWr,1);
	}
	patternCmp = 0x01;
	for(i=0;i<256;i++)	
	{
		hal_common_reg_32_write(PLC_AESLUTADDR_REG,(u32)i);
		patternRd = hal_common_reg_32_read(PLC_AESLUTDATA_REG);
		if(patternCmp != patternRd)
		{
			printf("HHT_TestAesTables : AES LUT Data 0x%02X mismatch: Wrote 0x%08lX & Readback 0x%08lX\n", i, patternCmp, patternRd);
            status = STATUS_FAILURE;
		}
		patternCmp = _crol_(patternCmp,1);
	}
    if (status == STATUS_FAILURE)
		printf("HHT_TestAesTables : AES LUT (PPEK) RW was failed\n");
	else if (status == STATUS_SUCCESS)
		printf("HHT_TestAesTables : AES LUT (PPEK) RW was done\n");

	
	
    // AES Key LUT Write - Readback verify
	patternWr32 = 0x01;
	for(i=0;i<1024;i++)	
	{
		hal_common_reg_32_write(PLC_AESKEYLUTADDR_REG,(u32)i);
		hal_common_reg_32_write(PLC_AESKEYLUTDATA_REG,(patternWr32));
        patternWr32 = _lrol_(patternWr32,1);
	}
	patternCmp32 = 0x01;
	for(i=0;i<1024;i++)	
	{
		hal_common_reg_32_write(PLC_AESKEYLUTADDR_REG,(u32)i);
		patternRd32 = hal_common_reg_32_read(PLC_AESKEYLUTDATA_REG);
		if(patternCmp32 != patternRd32)
		{
			printf("HHT_TestAesTables : AES Key LUT Data 0x%02X mismatch: Wrote 0x%08lX & Readback 0x%08lX\n", i, patternCmp32, patternRd32);
            status = STATUS_FAILURE;
		}
		patternCmp32 = _lrol_(patternCmp32,1);       
	}
    if (status == STATUS_FAILURE)
		printf("HHT_TestAesTables : AES LUT (AES KEY) RW was failed\n");
	else if (status == STATUS_SUCCESS)
		printf("HHT_TestAesTables : AES LUT (AES KEY) RW was done\n");


	
	// Release CPU Lock on AES LUT
	aesCpuCmd.reg = ReadU32Reg(PLC_AESCPUCMDSTAT_REG);
    aesCpuCmd.s.cpuTblGnt = 0;
	aesCpuCmd.s.cpuTblReq = 0;
	WriteU32Reg(PLC_AESCPUCMDSTAT_REG, aesCpuCmd.reg);
    if(status == STATUS_SUCCESS)
    {
        printf(" Test Passed : Walking 1s write-readback test passed for SSN Table, PPEK Addr LUT, AES Key LUT\n");
    }
}


void HHT_SetDiagMode()
{
	eRegFlag diagModeFlag;
	eRegFlag sackDisableFlag;
	u8 diagMode;
	u8 sackDisable;

	printf("Set Diag Mode : 1 - Set , 0 - Clear\n");
	scanf("%bd", &diagMode);
	diagModeFlag =	diagMode ? REG_FLAG_SET : REG_FLAG_CLR;
	HHAL_SetDiagMode(diagModeFlag);

	if(diagMode)
	{
		printf("Disable Sack  : 1 - Disable , 0 - Enable\n");
		scanf("%bd", &sackDisable);
		sackDisableFlag =	sackDisable ? REG_FLAG_SET: REG_FLAG_CLR;
		HHAL_SetDiagModeNoSack(sackDisableFlag);
	}
}


void HHT_SetKey()
{
    u8  secMode;
    u8  defKeyNum;
    u8  eks;		
    u8  ppeks;
	u8  input[10];

	do
	{
		printf("Select key type:  0-NEK, 1-PPEK, 2-AllKeys :: ");
		while (getline(input, sizeof(input)) > 0)
		{
			if(sscanf(input, "%bu", &secMode) >= 1)
			break;
		}
	}while (secMode>2);
    if(secMode == 0)     // NEK
    {
		do
		{
			printf("Select eks:  0 to 7 :: ");
			while (getline(input, sizeof(input)) > 0)
			{
				if(sscanf(input, "%bu", &eks) >= 1)
				break;
			}
		}while (eks>7);
		do
		{
			printf("Select def Key:  0 to 9 :: ");
			while (getline(input, sizeof(input)) > 0)
			{
				if(sscanf(input, "%bu", &defKeyNum) >= 1)
				break;
			}
		}while (defKeyNum>9);  
        HHAL_AddNEK(eks, gDefKey[defKeyNum]);
    }
    else if(secMode == 1)      //ppek
    {
		do
		{                                                                                
			printf("Select ppeks:  0 or 1 :: ");
			while (getline(input, sizeof(input)) > 0)
			{
				if(sscanf(input, "%bu", &ppeks) >= 1)
				break;
			}
		}while (ppeks!=0 && ppeks!=1);
		do
		{
			printf("Select def Key:  0 to 9 :: ");
			while (getline(input, sizeof(input)) > 0)
			{
				if(sscanf(input, "%bu", &defKeyNum) >= 1)
				break;
			}
		}while (defKeyNum>9); 
        HHAL_AddPPEK(ppeks+8, gDefKey[defKeyNum], gHpgpHalCB.remoteTei);
    }
    else
    {
        // set all NEKs
        for( eks=0 ; eks<=7 ; eks++ )
        {
            HHAL_AddNEK(eks, gDefKey[eks]);
        }
        // set two PPEKs
        HHAL_AddPPEK(eks, gDefKey[eks], gHpgpHalCB.remoteTei);
        eks++;
        HHAL_AddPPEK(eks, gDefKey[eks], gHpgpHalCB.remoteTei);
    }
}

void HHT_SetDefEks()
{
    u8  input[10];
	printf("Select def eks:  0 to 9 or F:: ");
    do
    {
    	while (getline(input, sizeof(input)) > 0)
    	{
    		if(sscanf(input, "%bx", &gNekEks) >= 1)
    		break;
    	}
    }while (gNekEks>9 && gNekEks != 0X0F); 
    printf("Default eks: %bx \n", gNekEks);
}
void HHT_SetRoboMode()
{
    eRegFlag             enbRoboMdPgm;
    ePlcPhyRxRoboMod     roboMd;
    u8                   plcNumPBs;   
    char                 input[10];

    printf("Enable RxRoboMode Register: 0-Disable, 1-Enable :: ");
	while (getline(input, sizeof(input)) > 0)
	{
		if(sscanf(input, "%bu", &enbRoboMdPgm) >= 1)
		break;
	}

    if(enbRoboMdPgm)
    {
		do
		{
			printf("Select robo mode:  0-Mini, 1-Std, 2-HS          :: ");
			while (getline(input, sizeof(input)) > 0)
			{
				if(sscanf(input, "%bu", &roboMd) >= 1)
				break;
			}
		}while (roboMd > 2);   

        if( roboMd  == 2)
        {
			do
			{
				printf("Select num PBs:  1-1PB, 2-2PB, 3-3PB            :: ");
				while (getline(input, sizeof(input)) > 0)
				{
					if(sscanf(input, "%bu", &plcNumPBs) >= 1)
					break;
				}
			}while (!plcNumPBs || plcNumPBs > 3);
        }
    }
    HHAL_PhyPgmRoboMd(enbRoboMdPgm, roboMd, plcNumPBs-1);
}


void HHT_AddrCfg()
{
    u8              tei;
    u8              remoteTei;
    u8              snid;
    u8              input[10];

    printf("Cur SNID  = 0x%bX, Cur TEI = 0x%bX, Rem TEI = 0x%bX\n", HHAL_GetSnid(), HHAL_GetTei(), gHpgpHalCB.remoteTei );    
    
	do
	{
		printf("Enter new SNID :: 0x"); 
		while (getline(input, sizeof(input)) > 0)
		{
			if(sscanf(input, "%bx", &snid) >= 1)
			break;
		}
	}while (snid > 15);
    
	do
	{
		printf("Enter new TEI  :: 0x"); 
		while (getline(input, sizeof(input)) > 0)
		{
			if(sscanf(input, "%bx", &tei) >= 1)
			break;
		}
	}while (tei > 0xFE); 

	do
	{
		printf("Enter remote TEI  :: 0x"); 
		while (getline(input, sizeof(input)) > 0)
		{
			if(sscanf(input, "%bx", &remoteTei) >= 1)
			break;
		}
	}while (remoteTei > 0xFE);   

    HHAL_SetTei(tei);
    gHpgpHalCB.remoteTei = remoteTei;
    HHAL_SetSnid(snid);
}


void HHT_DevCfg()
{
    eDevMode        devMode;
    eLineMode       lineMode;
    uPlcDevMode     plcDevMode;
    u8              input[10];

    plcDevMode.mode = HHAL_GetPlcDevMode();       

	do
	{
		printf("Cur DevMode  = %bd, Enter New DevMode  0-CCO, 1-STA :: ", plcDevMode.s.isSta);
		while (getline(input, sizeof(input)) > 0)
		{
			if(sscanf(input, "%bu", &devMode) >= 1)
			break;
		}
	}while (devMode > 2);     
	do
	{
		printf("Cur LineMode = %bd, Enter New LineMode 0-AC,  1-DC  :: ", plcDevMode.s.isDc);
		while (getline(input, sizeof(input)) > 0)
		{
			if(sscanf(input, "%bu", &lineMode) >= 1)
			break;
		}
	}while (lineMode > 2); 
    HHAL_SetDevMode(devMode, lineMode); 
    HHAL_SetDefAddrConfig();
}


void HHAL_CmdHelp()
{
    printf("HAL Test Commands:\n"
           "p memTest  - HPGP Regs Write-ReadBack test\n"
           "p xmitTest - Basic Data transmit test\n"
           "p cpTest   - CPs Req/Rel test\n"
           "p tblTest  - Test SSN, PPEKAddr, AesKey tables\n"
           "p memDump  - Memory Dump\n"
           "p diag     - Set Diag Mode\n"
           "p cBcn     - Send one Central Bcn\n"
           "p tx       - Advanced Tx Tests\n"
           "p rxcfg    - Program Rx Addr\n"
           "p devMode  - Config Device Mode\n"
           "p addr     - Config Address\n"
           "p key      - Key update\n"
           "p eks      - Set default eks\n"
           "p robo     - Program Rx Robo Mode\n"
           "p scan     - Set STA scan mode\n"
           "p stat     - Display statistics\n"
           "p rstStat  - Reset statistics\n"
#ifdef Flash_Config		   
           "p wcfg     - Write Config Data to flash\n"
           "p scfg     - FW Write Config Data to flash\n"
           "p rcfg     - Read Config Data from flash to ram \n"
#endif		   
//           "p txmap    - Set Carrier and Spectral Masks\n"
//TODO Enable this when required           "p (no)sniff- Turn off/on sniffer mode\n"
#ifdef SNIFFER
		   "p (no)swsniff	- Turn off/on eth-plc sniffer mode\n"
		   "p (no)bridge	- Turn off/on eth-plc bridge mode\n"
#endif
           "p demo str <color>      - Display GVC in a color\n"
           "p (no)led-demo  - Disable/Enable LED demo feature\n"
           "p txpowermode   - Transmission Power mode\n"
//#ifdef ER_TEST
			"p erenable      - Enable Extended Range mode\n"
			"p erdisable     - Disable Extended Range mode\n"
//#endif  //ER_TEST
			"p dbcenable      - Enable DBC Mode\n"
			"p dbcdisable     - Disable DBC Mode\n"
           "\n");
    return;
}

bool led_demo = FALSE;

void setLatency(u8 *cmdBuf)
{
    u8 TCCIterarion;
    if(sscanf(cmdBuf, "%bu", &TCCIterarion) >= 1)
    {
     
     WriteU8Reg(0x485, TCC_REG_485_486_val[TCCIterarion - 1][0]);
     WriteU8Reg(0x486, TCC_REG_485_486_val[TCCIterarion - 1][1]);
     set_plc_paramter(PLC_PHYLATENCY_SEL,  TX_RXLatency_TCC[TCCIterarion - 1]); 
    } 
}

#ifdef ETH_BRDG_DEBUG
void printDebug()
{
    uCapTxQStatusReg capTxQStat;
    sHpgpHalCB *hhalCb;	    
	       
    hhalCb = &gHpgpHalCB;

	printf("numTxFrms = 0x%lX\n", numTxFrms);
	printf("numTxDoneInts = 0x%lX\n", numTxDoneInts);
	printf("numPlcPendingRet = 0x%lX\n", numPlcPendingRet);
	printf("numForcePlcTxDone = 0x%lX\n", numForcePlcTxDone);
	printf("numTxFrms (reg 0xE94) = 0x%08lX\n", rtocl(ReadU32Reg(PLC_TxCount)));
	printf("numTxDoneInts from reg 0xE74 = 0x%08lX\n", rtocl(ReadU32Reg(0xe74)));
    capTxQStat.reg = ReadU32Reg(PLC_QDSTATUS_REG);
    printf("TX CAP=0X%lX\n",capTxQStat.reg);
    printf("# of free CPs = %bu\n", CHAL_GetFreeCPCnt());           
    printf("\nReg[0xEF0] (PLC State machine Wclk) = 0x%08lX\n", hal_common_reg_32_read(PLC_SMSHADOW1_REG));
    printf("Reg[0xEF4] (PLC State machine Rclk) = 0x%08lX\n", hal_common_reg_32_read(PLC_SMSHADOW2_REG));
    printf("Reg[0xE9C] (CP_PLCEth_Status):  0x%08lX\n", hal_common_reg_32_read(CP_PLC_ETH_STATUS));
    printf("Reg[0xD10] (Plc_SM_Hang_Interrupts):  0x%08lX\n", hal_common_reg_32_read(PLC_SM_HANG_INT));
    printf("Reg[0xDE0] (ETH Free CP Cnt):  0x%08lX\n", hal_common_reg_32_read(ETHMAC_TXFREECPCNT_REG));
    printf("Reg[0xE6C] (ETH TX QUEUE Cnt):  0x%08lX\n", hal_common_reg_32_read(ETHMAC_TXQDESCCNT_REG));
    printf("\nFrame Counters:\n");
#ifdef HYBRII_ETH	
    printf("ETH->PLC:  ETH HW RX frame count=%lu, ethRxFrameCnt=%lu, plcTxFrameCnt=%lu, plcTxWriteFail=%lu\n",rtocl(EHAL_ReadEthStatReg(1)), ethRxFrameCnt,plcTxFrameCnt, plcTxWriteFail);
    printf("PLC->ETH:  plcRxFrameCnt=%lu, ethTxFrameCnt=%lu, ETH HW TX frame count=%lu, ethTxWriteFail=%lu\n", plcRxFrameCnt, ethTxFrameCnt, rtocl(EHAL_ReadEthStatReg(0x81)),ethTxWriteFail);
	
	datapath_queue_depth(HOST_DATA_QUEUE);
	datapath_queue_depth(PLC_DATA_QUEUE);

    printf("numEthTxDoneInts = %lu, oldNumEthTxDoneInts = %lu, numEthTxFrames=%lu, ethTxDone=%bu, ETH HW TX frame count=%lu\n", 
            numEthTxDoneInts,oldNumEthTxDoneInts,ethTxFrameCnt, ethTxDone,  rtocl(EHAL_ReadEthStatReg(0x81)));
#endif	
	
    printf("\nCP Counters:\n");
    printf("ETH->PLC: numPlcTxCp=%lu(0x%lX), HW (Paul's debug) CP reg: 0x%08lX\n", numPlcTxCp, numPlcTxCp, hal_common_reg_32_read(0xDCC));
    printf("PLC->ETH: numEthTxCp=%lu, numEthTxCpReleased=%lu\n", numEthTxCp, numEthTxCpReleased);
    printf("Stats from Rx Interrupt handler:\n");
    printf("TotalRxFrameCnt=%lu (PLC Rx+ETH Rx = %lu), TotalRxCpCnt=%lu (PLC Rx+ETH R=%lu) \n", TotalRxFrameCnt, ethTxFrameCnt+ethRxFrameCnt, 
            TotalRxCpCnt, numPlcTxCp+numEthTxCp);
    hal_common_display_qc_error_stats();
}

void clearDebug()
{
    numTxFrms=0;
    numTxDoneInts=0;
    numPlcPendingRet=0;
    numForcePlcTxDone=0;
    WriteU32Reg(PLC_TxCount,0);
    ethRxFrameCnt = 0;
    ethTxFrameCnt = 0;
    plcTxFrameCnt = 0;
    plcRxFrameCnt = 0;
	
#ifdef HYBRII_ETH
    EHAL_Clear_ethHWStat();
#endif
    numEthTxDoneInts=0;
    oldNumEthTxDoneInts=0;
    ethTxWriteFail=0;
    numPlcTxCp=0;
    numEthTxCp=0;
    numEthTxCpReleased=0;
    plcTxWriteFail=0;
    TotalRxCpCnt=0;
    TotalRxFrameCnt=0;
}
#endif  //ETH_BRDG_DEBUG

#ifdef ASSOC_TEST

void hhal_set_prom (bool sniff_en)
{
    uPlcStatusReg  plcStatus;
    
    if (sniff_en)
    {
      //  printf("Enable \n");
    }
    else
    {
	//   printf("Disable \n");
    }
    //printf("Sniffer Mode\n");
    plcStatus.reg = ReadU32Reg(PLC_STATUS_REG);
    plcStatus.s.promiscModeEn  = sniff_en; 
//    plcStatus.s.snifferMode    = sniff_en;
    WriteU32Reg(PLC_STATUS_REG, plcStatus.reg);
}

#endif

void setfreq60()
{
  
  gHpgpHalCB.lineMode = LINE_MODE_DC;
#ifdef FREQ_DETECT
  FREQDET_FreqSetting(FREQUENCY_60HZ);
#endif
  HHAL_SetDevMode(gHpgpHalCB.devMode, gHpgpHalCB.lineMode); 
  

}
void HHAL_CmdHALProcess(char* CmdBuf)
{
    u8  cmd[10];
    u32 i;

    CmdBuf++;

	if (sscanf(CmdBuf, "%s", &cmd) < 1 || strcmp(cmd, "?") == 0)
	{
		HHAL_CmdHelp();
        return;
	}
	if(strcmp(cmd, "xmitTest") == 0 || strcmp(cmd, "xmittest") == 0)
	{
		HHT_BasicTxMenu();		
	}
//	else  if(strcmp(cmd, "rdc") == 0 || strcmp(cmd, "Rdc") == 0)
//	{
//		HHT_BasicTxMenu();		
//	}	
	else  if (strcmp(cmd, "stat") == 0)
	{
		HHAL_DisplayPlcStat();
	}
    else  if (strcmp(cmd, "powersave") == 0)
	{
	    //HHAL_EnablePowerSave();
	}
    
	else if (strcmp(cmd, "rststat") == 0 || strcmp(cmd, "rstStat") == 0)
	{
		HHAL_ResetPlcStat();
        HHAL_DisplayPlcStat();
	}
	else if (strcmp(cmd, "diag") == 0)
	{
		HHT_SetDiagMode();
	}
	else if(strcmp(cmd,"qd") == 0)
	{

		printf ("host");
		 datapath_queue_depth(HOST_DATA_QUEUE);
		printf ("plc");

		datapath_queue_depth(PLC_DATA_QUEUE);

		
	}
#ifdef ASSOC_TEST
	else if(strcmp(cmd,"assoc") == 0)
	{
		LM_SendAssocReq();
	}
	else if(strcmp(cmd,"idReq") == 0)
	{
		LM_SendIdentifyReq();
	}	
	else if(strcmp(cmd,"getkey") == 0)
	{
		LM_SendGetKeyReq();
	}
#endif
	else if (strcmp(cmd, "cBcn") == 0)
	{
		HHT_SendBcn(BEACON_TYPE_CENTRAL);
	}
	else if (strcmp(cmd, "tx") == 0)
	{
        HHT_SimulateTxTestMenu();
	}
	else if (strcmp(cmd, "devMode") == 0 || strcmp(cmd, "devmode") == 0)
	{
		HHT_DevCfg();
	}
#ifdef UART_HOST_INTF 
	else if(strcmp(cmd, "uartstat") == 0)
	{
		FM_Printf(FM_USER,"\n******RX Stat***********\n");
		FM_Printf(FM_USER,"Rx Count %u\n",uartRxControl.rxCount);
		FM_Printf(FM_USER,"Rx Ready %bu\n",uartRxControl.rxReady);
		FM_Printf(FM_USER,"Rx CP %bu\n",uartRxControl.rxSwDesc.cpCount);
		#ifndef UART_RAW
		FM_Printf(FM_USER,"Rx CRC %u\n",uartRxControl.crcRx);
		FM_Printf(FM_USER,"Rx Good Frame Count %lu\n",uartRxControl.goodRxFrmCnt);
		FM_Printf(FM_USER,"Rx Drop Count %u\n",uartRxControl.rxDropCount);
		#endif
		FM_Printf(FM_USER,"Rx Expected Count %u\n",uartRxControl.rxExpectedCount);
		FM_Printf(FM_USER,"\n*******TX Stat**********\n");
		FM_Printf(FM_USER,"Tx Pending Count %u\n",uartTxControl.txCount);
	#ifndef UART_RAW 
				FM_Printf(FM_USER,"Tx CRC %u\n",uartTxControl.crcTx);
	#endif
	}
#endif
#ifdef ASSOC_TEST
    else if (strcmp(cmd, "noprom") == 0)
    {
        hhal_set_prom(0);
    }
    else if (strcmp(cmd, "prom") == 0)
    {
        hhal_set_prom(1);
    }
#endif
    else if (strcmp(cmd, "setfreq60") == 0)
	{
		setfreq60();

	}
	else if (strcmp(cmd, "addr") == 0)
	{
		HHT_AddrCfg();
	}
	else if (strcmp(cmd, "txCfg") == 0 || strcmp(cmd, "txcfg") == 0)
	{
        HHAL_SetDevMode(DEV_MODE_STA, LINE_MODE_DC);
        
        HHT_DevCfg();
        gHpgpHalCB.bcnInitDone = 1;
        //WriteU8Reg(0xF06,0x06);
		
	}
    else if (strcmp(cmd, "stopbcntx") == 0 || strcmp(cmd, "stopBcntx") == 0)
    {
        gHpgpHalCB.bcnInitDone = 0;
    }
    else if (strcmp(cmd, "startbcntx") == 0 || strcmp(cmd, "startBcntx") == 0)
    {
        gHpgpHalCB.bcnInitDone = 1;
    }
     else if (strcmp(cmd, "strtbcntx") == 0 || strcmp(cmd, "strtBcntx") == 0)
    {
        gHpgpHalCB.bcnInitDone = 1;
    }

	else if (strcmp(cmd, "rxCfg") == 0 || strcmp(cmd, "rxcfg") == 0)
	{
        u8 remoteTei = HYBRII_DEFAULT_TEICCO; 
		u8 selfTei   = HYBRII_DEFAULT_TEISTA;
        
        HHAL_SetTei(selfTei);
        gHpgpHalCB.remoteTei = remoteTei;
        gHpgpHalCB.bcnInitDone = 0;
	}
   	else if (strcmp(cmd, "tcc") == 0) 
     {
            setLatency(CmdBuf+sizeof("tcc"));
     }	
    else if (strcmp(cmd, "key") == 0 ) 
    {
        HHT_SetKey();
    }
    else if (strcmp(cmd, "eks") == 0 ) 
    {
        HHT_SetDefEks();
    }
    else if (strcmp(cmd, "tblTest") == 0 || strcmp(cmd, "tbltest") == 0) 
    {
        HHT_TestMemoryTables();
    }
	else if (strcmp(cmd, "robo") == 0)
	{
		HHT_SetRoboMode();
	}
//	else if (strcmp(cmd, "txmap") == 0 ) 
//    {
//        HHT_SetTxMap();
//    }
#ifdef _LED_DEMO_
	else if (strcmp(cmd, "demo") == 0)
	{
        HHT_LedDemoTxMenu(CmdBuf+1+ strlen("demo"));
    }
    else if (strcmp(cmd, "led-demo") == 0)
    {       
         led_demo = TRUE;
    }
    else if (strcmp(cmd, "noled-demo") == 0)
    {       
         led_demo = FALSE;
    }
    else if (strcmp(cmd, "led") == 0)
    {       
         init_led_board();
         led_control(TRUE, BLUE, FALSE, 7);

    }
#endif  
    else  if (strcmp(cmd, "scan") == 0)
    {
        //HHAL_SetDefDevConfig(DEVMODE_STA, LINEMODE_DC);
        HHAL_SetSWStatReqScanFlag(REG_FLAG_SET);
        HHT_DevCfg(); 
    }
    else  if (strcmp(cmd, "set") == 0)
    {
        var1 = 6000;
    }
     else  if (strcmp(cmd, "deb") == 0)
    {
        for(i =0; i < cnt5;i++)
        {
//            printf("\n x= %lu",testArr[i]); 
        }
    }
     else  if (strcmp(cmd, "miss") == 0)
    {
       printf("\n misscnt = %lu", gHpgpHalCB.bcnmisscnt); 
     /*  for(i =0; i < cnt;i++)
       {
        printf("\n bts = %lu", missarr[i]);
       }*/
       printf("\n debugcnt = %lu", debugcnt); 
        printf("\n gsnid = %lu", gsnid); 
         printf("\n nwsel = %bu",  gHpgpHalCB.nwSelected); 
          printf("\n scanenb = %bu",  gHpgpHalCB.scanEnb);
          printf("\n pbcscorr = %bu",  gpbcscorrect);
          printf("\n gvaid = %bu",  gvalid);
            printf("\n devmode = %bu",gHpgpHalCB.devMode);
        
        
         
       
       
 
    }    
    else  if (strcmp(cmd, "sniff") == 0)
    {
        hhal_tst_sniff_cfg(TRUE); 
    }
    else  if (strcmp(cmd, "nosniff") == 0)
    {
        hhal_tst_sniff_cfg(FALSE); 
    }
#ifdef SNIFFER
	else  if (strcmp(cmd, "swsniff") == 0)
    {
         hostIntf = HOST_INTF_ETH;
         eth_plc_bridge = 1;
		 eth_plc_sniffer = 1;
    }
    else  if (strcmp(cmd, "noswsniff") == 0)
    {
		 hostIntf = HOST_INTF_NO;
		 eth_plc_sniffer = 0;
		 printf("Warnnig: eth-plc bridge is ON, if not needed turn OFF\n");
    }
	else  if (strcmp(cmd, "bridge") == 0)
    {
         eth_plc_bridge = 1;
    }
    else  if (strcmp(cmd, "nobridge") == 0)
    {
		 eth_plc_bridge = 0;
    }
#endif
    else  if (strcmp(cmd, "txpowermode") == 0)
    {
         update_powermode(0);
    }
     else  if (strcmp(cmd, "rxpowermode") == 0)
    {
         update_powermode(1);
    }
   
   else  if (strcmp(cmd, "Shortrange") == 0)
    {
        WriteU8Reg(0x48a, 0x90);
        WriteU8Reg(0x48b, 0x01);
        WriteU8Reg(0x484, 0x82);
        WriteU8Reg(0x478, 0x61);
    }
      else  if (strcmp(cmd, "Longrange") == 0)
    {

        WriteU8Reg(0x48a, 0xEA);
        WriteU8Reg(0x48b, 0x00);
        WriteU8Reg(0x484, 0x5A);
        WriteU8Reg(0x478, 0x4b);
        WriteU8Reg(0x483, 0x13);
    
    }
	//Added by YM 0206-2014, for Extended Range Mode setting	
//#ifdef ER_TEST
  	else if (strcmp(cmd, "erenable") == 0)
	{
	     WriteU8Reg(0x4F0, 0x80);
	}
	else if (strcmp(cmd, "erdisable") == 0)
	{
	     WriteU8Reg(0x4F0, 0x0);
	}
//#endif  //ER_TEST
    // For DBC Test
    else if (strcmp(cmd, "dbcenable") == 0)
	{
	    WriteU32Reg(PLC_DBC_PATTERN_REG, ctorl(0x80000000));
	}
	else if (strcmp(cmd, "dbcdisable") == 0)
	{
	     WriteU32Reg(PLC_DBC_PATTERN_REG, 0x0);
	}
#ifdef Flash_Config	
	else if (strcmp(cmd, "wcfg") == 0)
	{
	     Program_Config_Data();
		 Load_Config_Data(1, (u8 *)&sysConfig);
	}
	else if (strcmp(cmd, "rcfg") == 0)
	{
	     Load_Config_Data(0, (u8 *)&sysConfig);
	}
	else if (strcmp(cmd, "scfg") == 0)
	{
	     Load_Config_Data(1, (u8 *)&sysConfig);
	}
	else if (strcmp(cmd, "pcfg") == 0)
	{
		 unsigned int k=0;
		 //for (k=0; k< 512; k++)
		 //{
		 //    printf("cfg[%u] = 0x%bx \n", k, (u8) (0xFF & *(&sysConfig.SeqNum[0]+k)));        
		 //}
		 for (k=0; k< 8; k++)
		 {
		     printf("%bx ", (u8) sysConfig.SeqNum[k]);
		 }
		 printf("\n");
		 for (k=0; k< 8; k++)
		 {
		     printf("%bx ", (u8) sysConfig.systemName[k]);
		 }
		 printf("\n");
		 for (k=0; k< 6; k++)
		 {
		     printf("%bx ", (u8) sysConfig.macAddress[k]);
		 }
		 printf("\n");
		 printf("default NID ");
		 for (k=0; k< 7; k++)
		 {
		     printf("%bx ", (u8) sysConfig.defaultNID[k]);
		 }
		 printf("\n");
		 //for (k=0; k< 6; k++)
		 {
		     printf("STEI = %bx, DTEI = %bx", (u8) sysConfig.defaultSTEI, (u8) sysConfig.defaultDTEI);
		 }
		 printf("\n");
		 for (k=0; k< 8; k++)
		 {
		     printf("%bx ", (u8) sysConfig.zigbeeAddr[k]);
		 }
		 printf("\n");
		 //for (k=0; k< 8; k++)
		 {
		     printf("default channel = %bx ", (u8) sysConfig.defaultCH);
		 }
		 printf("\n");
		 printf("default LO leak reg 23,24 setting");
		 {
		     printf("Reg23 = %bx, Reg24 = %bx ", (u8) sysConfig.defaultLOLeak23, (u8) sysConfig.defaultLOLeak24);
		 }
		 printf("\n");
		 printf("Channel VCO calibration value\n");
		 for (k=0; k< 16; k++)
		 {
		     printf("%bx ", (u8) sysConfig.VCOCal[k]);
		 }
		 printf("\n");
	}
	else if (strcmp(cmd, "ccfg") == 0)
	{
		 unsigned int k=0;

		 for (k=0; k< 512; k++)
		 {
		     printf("before clear: cfg[%u] = 0x%bx\n", k, *(&sysConfig.SeqNum[0]+k));
             *(&sysConfig.SeqNum[0]+k) = 0;
			 printf("after clear: Cfg [%u] content = %bx\n", k, *(&sysConfig.SeqNum[0]+k));
             //printf("0x%X Addr 0x%X\n"  sysConfig,  sysConfig);
		 }
	}
#endif  //Flash_Config	
#ifdef ETH_BRDG_DEBUG
    else if (strcmp(cmd, "debug") == 0)
    {
        // toggle the debug flag
        myDebugFlag = !myDebugFlag;
        printf("\n Debug flag is %s\n", myDebugFlag ? "ON":"OFF");
	}
    else if (strcmp(cmd, "debug1") == 0)
    {
        // toggle the debug flag
        myDebugFlag1 = !myDebugFlag1;
        printf("\n Debug flag 1 is %s\n", myDebugFlag1 ? "ON":"OFF");
    }
	else  if (strcmp(cmd, "1") == 0)
    {
		 printDebug();
    }
    else  if (strcmp(cmd, "2") == 0)
    {
		 clearDebug();
    }
#endif
    else  if (strcmp(cmd, "version") == 0)
    {
        FM_Printf(FM_USER, "VERSION: %s\n",get_Version());
    }
#ifdef PROD_TEST	
    else  if (strcmp(cmd, "test") == 0)
    {
        testCmd = 1;
    }
#endif	
    else  if (strcmp(cmd, "debug") == 0)
    {
        FM_setdebug(1);
    }
    else  if (strcmp(cmd, "nodebug") == 0)
    {
        FM_setdebug(0);
    }
    else
    {
        HHAL_CmdHelp();
    }  
}	  

void updateStatisticData(sSwFrmDesc* pPlcRxFrmSwDesc)
{
	u16                tmpFrmLen;
    u8                 rxSeqNum;
    u8                 curMissFrmCnt;
	u8                 frmStart;
	u8                 frmData;
	u8                 bCorruptFrame;
    u8                 bDemoMode;
    u8                 bDuplicateFrame;
	u8                 i;
	u8                 offset;
	u16                tmpPayloadLen;
#ifdef MEM_PROTECTION
	u8 				   cp_localBuf[HYBRII_CELLBUF_SIZE];	// local CP buffer
#endif


	tmpFrmLen       = 0;   
	frmData         = 0;
	bCorruptFrame   = 0;
    bDemoMode       = 0;
    bDuplicateFrame = 0;
	tmpPayloadLen   = 0;

	frmStart =	(pPlcRxFrmSwDesc->frmType == HPGP_HW_FRMTYPE_MSDU) ? 0 : 4;

	// check desc cnt before each looping -- tbd
	for( i=0 ; i< pPlcRxFrmSwDesc->cpCount ; i++ )
	{
		u8 j;
		u8 descLen;
//		volatile u8 xdata * cellAddr;
		u8 * cellAddr;

		offset   = 0;
		descLen  = (pPlcRxFrmSwDesc->frmLen-tmpFrmLen) < HYBRII_CELLBUF_SIZE ? (pPlcRxFrmSwDesc->frmLen - tmpFrmLen) : HYBRII_CELLBUF_SIZE;
#ifdef MEM_PROTECTION
		// copy cp to local buf
		if (HHAL_CP_Get_Copy(pPlcRxFrmSwDesc->cpDesc[i].cp, &cp_localBuf[0], HYBRII_CELLBUF_SIZE) == STATUS_FAILURE)
		{
			printf("updateStatisticData: Failed to make a copy of CP %bu (cpCount=%bu). Continue with next CP\n",
				i, pPlcRxFrmSwDesc->cpCount);
			continue;
		}
		cellAddr = &cp_localBuf[0];
#else
		cellAddr = CHAL_GetAccessToCP(pPlcRxFrmSwDesc->cpArr[i].cp);
#endif
//		CP_displayLocalBuf(cellAddr);
					
					
        if (i==0) 
        {
			offset = frmStart;  
            rxSeqNum =  cellAddr[offset];   
            
#if  PLC_DATA_FIXED_PATTERN // Alternating frame bytes test           
            if (rxSeqNum & 0x01)
            {
                frmData   = 0xAA;
            }
            else
            {
                frmData   = 0x55;
            }
#else
            // Start frame data from seq num.
            frmData   = (u8)(gHpgpHalCB.TxSeqNum+1);
#endif
#if 0
			FM_Printf(FM_LINFO,"Frame#%lu: , SeqNum=%bu, CP#1=%bu, FCCSErrCnt = %lu, ICVErrCnt = %lu\n", gHpgpHalCB.halStats.TotalRxGoodFrmCnt, rxSeqNum, pPlcRxFrmSwDesc->cpDesc[i].cp, 
											hal_common_reg_32_read(PLC_FCCSERRCNT_REG),hal_common_reg_32_read(PLC_ICVERRCNT_REG));
			FM_Printf(FM_LINFO,"frmType=%bu, stei=%bu:, snid=%bu, clst=%bu, mcstMode=%bu, CPCnt=%bu, lqi,rssi=%bu,%bu\n", pPlcRxFrmSwDesc->frmType, pPlcRxFrmSwDesc->frmInfo.plc.stei,
					pPlcRxFrmSwDesc->frmInfo.plc.snid, pPlcRxFrmSwDesc->frmInfo.plc.clst, pPlcRxFrmSwDesc->frmInfo.plc.mcstMode, CHAL_GetFreeCPCnt(), 
					pPlcRxFrmSwDesc->frmInfo.plc.lqi, pPlcRxFrmSwDesc->frmInfo.plc.rssi);
#endif
           // compute miss frm count 
            if (gHpgpHalCB.halStats.TotalRxGoodFrmCnt - gHpgpHalCB.halStats.RxGoodBcnCnt != 1)
            {
                if((gHpgpHalCB.halStats.PrevRxSeqNum+1) % 256 != rxSeqNum &&	 \
				  pPlcRxFrmSwDesc->frmInfo.plc.mcstMode != HPGP_UCST)   // could be a retransmission
                {
                     curMissFrmCnt = ((gHpgpHalCB.halStats.PrevRxSeqNum+1) % 256 > rxSeqNum) ?
                                        (rxSeqNum - 0) + (255 - (gHpgpHalCB.halStats.PrevRxSeqNum+1)) :	rxSeqNum - (gHpgpHalCB.halStats.PrevRxSeqNum + 1) % 256;

					 gHpgpHalCB.halStats.TotalRxMissCnt += curMissFrmCnt;
                    // crossed miss threshold
                    if(curMissFrmCnt >= 2)
                    {
                        FM_Printf(FM_LINFO,"CurMissCnt=%bu: , CurSeqNum=%bu\n, TotalMissCnt=%lu",  curMissFrmCnt, rxSeqNum, gHpgpHalCB.halStats.TotalRxMissCnt);
                        FM_Printf(FM_LINFO,"FCCSErrCnt=%lu, ICVErrCnt=%lu, PbcsRxErrCnt=%lu, AddrFiltErrCnt=%lu\n\n", hal_common_reg_32_read(PLC_FCCSERRCNT_REG),
									hal_common_reg_32_read(PLC_ICVERRCNT_REG), hal_common_reg_32_read(PLC_PBCSRXERRCNT_REG), hal_common_reg_32_read(PLC_ADDRFILTERERRCNT_REG));
                    }
                }

                // check for duplicate frame ie., retransmission- first byte is  seqNum
                if (gHpgpHalCB.halStats.PrevRxSeqNum == rxSeqNum )
                {
                    gHpgpHalCB.halStats.DuplicateRxCnt++;
#ifdef MPER					
					gHpgpHalCB.halStats.TotalRxBytesCnt -= pPlcRxFrmSwDesc->frmLen;
#endif
                    if(!pPlcRxFrmSwDesc->frmInfo.plc.mcstMode)
                    {
//                        gHpgpHalCB.halStats.DuplicateRxCnt++;
                        bDuplicateFrame = 1;
                    }
                    // if repeated seqNum for mcst/bcst this is a corrupt frame
                    else
                    {
						 u8   mode =  pPlcRxFrmSwDesc->frmInfo.plc.mcstMode;
						 u8   prevRxSN = gHpgpHalCB.halStats.PrevRxSeqNum;
//            FM_Printf(FM_MINFO,"mcst mode=%bd, rxSeqNum=%d  PrevRx=%d\n", mode, rxSeqNum, prevRxSN);

                         bCorruptFrame   = 1;
                         bDuplicateFrame = 1;
                    }
                }                 
            }                        
            gHpgpHalCB.halStats.PrevRxSeqNum = rxSeqNum;
            offset                 += 1;
        } 

#ifdef _LED_DEMO_
        // Demo Mode Command frame interpetation
        if(!bDuplicateFrame)
        {
            bDemoMode = HHT_DemoModeRx(&cellAddr[offset]);
        }
        else
        {
            return;
        }
#endif
        if(!bDemoMode)
        {
			u8  flag = 0;

			for( j=offset ; j<descLen ; j++)
			{            
#if  PLC_BCNDATA_FIXED_PATTERN
				if( cellAddr[j]!= 0xBB)
				{
					 bCorruptFrame = 1;
					 if (flag == 0)
					 {
					 //printf("#1 cellAddr[%bu] = %02bx\n", j, cellAddr[j]);
					 flag = 1;
					 }
				}
#elif PLC_DATA_FIXED_PATTERN
				if( cellAddr[j] != frmData)
				{
					 bCorruptFrame = 1;
					 if (flag == 0)
					 {
					 //printf("#2 cellAddr[%bu] = %02bx\n", j, cellAddr[j]);
					 flag = 1;
					 }
				}
                frmData     = _cror_(frmData, 1);
#else
				if( cellAddr[j]!= frmData++)
				{
					 bCorruptFrame = 1;
					 if (flag == 0)
					 {
					 //printf("#3 cellAddr[%bu] = %02bx\n", j, cellAddr[j]);
					 flag = 1;
					 }
				}
#endif                                         
			} 
       }
#if 0
      if (bCorruptFrame)
       {
            FM_Printf(FM_MINFO,"CP[%bu] = %bu :\n", i, pPlcRxFrmSwDesc->cpDesc[i].cp);
			for( j=0 ; j<descLen ; j++) 
			{
				FM_Printf(FM_MINFO,"0x%02bX ", cellAddr[j]);
			}
			FM_Printf(FM_MINFO,"\n");
        }//FM_Printf(FM_LINFO,"\n");
#endif		
		tmpFrmLen+=descLen;
		tmpPayloadLen += (descLen - offset);					
	}
#if 0
	if (bCorruptFrame)
	{
        gHpgpHalCB.halStats.CorruptFrmCnt++;
		printf("Received Corrupt Frame; SeqNum=%bu, ssn=%u, FrmLen=%u,  FCCSErrCnt=%lu, ICVErrCnt=%lu, AddrFiltErr=%lu \n\n", rxSeqNum, pPlcRxFrmSwDesc->frmInfo.plc.ssn, 
			pPlcRxFrmSwDesc->frmLen, hal_common_reg_32_read(PLC_FCCSERRCNT_REG), hal_common_reg_32_read(PLC_ICVERRCNT_REG), hal_common_reg_32_read(PLC_ADDRFILTERERRCNT_REG));
	}
    else
    {
        if(((gHpgpHalCB.halStats.TotalRxGoodFrmCnt - gHpgpHalCB.halStats.RxGoodBcnCnt) & (u32)(0xFF)) == 0)
        {  
            printf("\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b");
            printf("Sent %ld, Received %ld HPGP frames.", gHpgpHalCB.halStats.CurTxTestFrmCnt, gHpgpHalCB.halStats.TotalRxGoodFrmCnt - gHpgpHalCB.halStats.RxGoodBcnCnt);
        }
    }  
#endif
}

//============ Process Rx Functions , Int Handlers


void HHT_ProcessPlcMacFrame(sSwFrmDesc* pPlcRxFrmSwDesc)  
{
	u16                tmpFrmLen;
//    u8                 rxSeqNum;
//    u8                 curMissFrmCnt;
//	u8                 frmStart;
	u8                 frmData;
	u8                 bCorruptFrame;
    u8                 bDemoMode;
    u8                 bDuplicateFrame;
//	u8                 cp;
	u8                 i;
//	u8                 offset;
	u16                tmpPayloadLen;

	tmpFrmLen       = 0;   
	frmData         = 0;
	bCorruptFrame   = 0;
    bDemoMode       = 0;
    bDuplicateFrame = 0;
	tmpPayloadLen   = 0;


#if 0
	frmStart =	(pPlcRxFrmSwDesc->frmType == HPGP_HW_FRMTYPE_MSDU) ? 0 : 4;

	// check desc cnt before each looping -- tbd
	for( i=0 ; i< pPlcRxFrmSwDesc->cpCnt ; i++ )
	{
		u8 j;
		u8 descLen;
		volatile u8 xdata * cellAddr;
		u16 tmpIndx;

		offset   = 0;
		descLen  = (pPlcRxFrmSwDesc->frmLen-tmpFrmLen) < HYBRII_CELLBUF_SIZE ? (pPlcRxFrmSwDesc->frmLen-tmpFrmLen):HYBRII_CELLBUF_SIZE;
		cellAddr = CHAL_GetAccessToCP(pPlcRxFrmSwDesc->cpDesc[i].cp);
#if 0
        if(i==0) 
        {
			offset = frmStart;  
            rxSeqNum =  cellAddr[offset];   
            
#if  PLC_DATA_FIXED_PATTERN // Alternating frame bytes test           
            if(rxSeqNum & 0x01)
            {
                frmData   = 0xAA;
            }
            else
            {
                frmData   = 0x55;
            }
#else
            // Start frame data from seq num.
            frmData   = (u8)(gHpgpHalCB.TxSeqNum+1);
#endif
			FM_Printf(FM_LINFO,"Frame#%lu: , SeqNum=%bu, CP#1=%bu, FCCSErrCnt = %lu, ICVErrCnt = %lu\n", gHpgpHalCB.halStats.TotalRxGoodFrmCnt, rxSeqNum, pPlcRxFrmSwDesc->cpDesc[i].cp, hal_common_reg_32_read(PLC_FCCSERRCNT_REG),hal_common_reg_32_read(PLC_ICVERRCNT_REG));
			FM_Printf(FM_LINFO,"frmType=%bu, stei=%bu:, snid=%bu, clst=%bu, mcstMode=%bu, CPCnt=%bu, lqi,rssi=%bu,%bu\n", pPlcRxFrmSwDesc->frmType, pPlcRxFrmSwDesc->frmInfo.plc.stei, pPlcRxFrmSwDesc->frmInfo.plc.snid, pPlcRxFrmSwDesc->frmInfo.plc.clst, pPlcRxFrmSwDesc->frmInfo.plc.mcstMode, CHAL_GetFreeCPCnt(), pPlcRxFrmSwDesc->frmInfo.plc.lqi, pPlcRxFrmSwDesc->frmInfo.plc.rssi);

           // compute miss frm count 
            if(gHpgpHalCB.halStats.TotalRxGoodFrmCnt - gHpgpHalCB.halStats.RxGoodBcnCnt != 1)
            {
                if(gHpgpHalCB.halStats.PrevRxSeqNum+1 != rxSeqNum &&	 \
				  pPlcRxFrmSwDesc->frmInfo.plc.mcstMode != HPGP_UCST)   // could be a retransmission
                {
                    curMissFrmCnt = ((gHpgpHalCB.halStats.PrevRxSeqNum+1) > rxSeqNum) ?
                                        (rxSeqNum - 0) + (255 - gHpgpHalCB.halStats.PrevRxSeqNum):
                                         rxSeqNum - (gHpgpHalCB.halStats.PrevRxSeqNum + 1);
					 gHpgpHalCB.halStats.TotalRxMissCnt += curMissFrmCnt;
                    // crossed miss threshold
                    if(curMissFrmCnt >= 2)
                    {
                        FM_Printf(FM_LINFO,"CurMissCnt=%bu: , CurSeqNum=%bu\n, TotalMissCnt=%lu",  curMissFrmCnt, rxSeqNum, gHpgpHalCB.halStats.TotalRxMissCnt);
                        FM_Printf(FM_LINFO,"FCCSErrCnt=%lu, ICVErrCnt=%lu, PbcsRxErrCnt=%lu, AddrFiltErrCnt=%lu\n\n", hal_common_reg_32_read(PLC_FCCSERRCNT_REG),hal_common_reg_32_read(PLC_ICVERRCNT_REG), hal_common_reg_32_read(PLC_PBCSRXERRCNT_REG), hal_common_reg_32_read(PLC_ADDRFILTERERRCNT_REG));
                    }
                }
                // check for duplicate frame ie., retransmission- first byte is  seqNum
                if(gHpgpHalCB.halStats.PrevRxSeqNum == rxSeqNum )
                {
                    if(!pPlcRxFrmSwDesc->frmInfo.plc.mcstMode)
                    {
                        gHpgpHalCB.halStats.DuplicateRxCnt++;
                        bDuplicateFrame = 1;
                    }
                    // if repeated seqNum for mcst/bcst this is a corrupt frame
                    else
                    {
                         bCorruptFrame   = 1;
                         bDuplicateFrame = 1;
                         gHpgpHalCB.halStats.DuplicateRxCnt++;
                    }
                }                 
            }                        
            gHpgpHalCB.halStats.PrevRxSeqNum = rxSeqNum;
            offset                 += 1;
        } 
#endif

        // Demo Mode Command frame interpetation
        if(!bDuplicateFrame)
        {
            bDemoMode = HHT_DemoModeRx(&cellAddr[offset]);
        }

        if(!bDemoMode)
        {
			for( j=offset ; j<descLen ; j++)
			{            
#if  PLC_BCNDATA_FIXED_PATTERN
				if( cellAddr[j]!= 0xBB)
				{
					 bCorruptFrame = 1; 
				}
#elif PLC_DATA_FIXED_PATTERN
				if( cellAddr[j]!= frmData)
				{
					 bCorruptFrame = 1;
				}
                frmData     = _cror_(frmData, 1);
#else
				if( cellAddr[j]!= frmData++)
				{
					 bCorruptFrame = 1;
				}
#endif                                         
			} 
        }    
       if(bCorruptFrame)
        {
            FM_Printf(FM_MINFO,"CP[%bu] = %bu :\n", i, pPlcRxFrmSwDesc->cpDesc[i].cp);
			for( j=0 ; j<descLen ; j++) 
			{
				FM_Printf(FM_MINFO,"0x%02bX ", cellAddr[j]);
			}
			FM_Printf(FM_MINFO,"\n");
        }//FM_Printf(FM_LINFO,"\n");
		tmpFrmLen+=descLen;
		tmpPayloadLen += (descLen - offset);					
	}
#else


#ifdef PROD_TEST
	if ((gHpgpHalCB.prodTestDevType == DEV_REF ) && (gHpgpHalCB.prodTestIsPLCTxTestActive))
#endif
	{
		updateStatisticData(pPlcRxFrmSwDesc);
	}
#endif

    for( i=0 ; i< pPlcRxFrmSwDesc->cpCount ; i++ )
	{
		CHAL_DecrementReleaseCPCnt(pPlcRxFrmSwDesc->cpArr[i].cp);
	}

#if  1
	if(bCorruptFrame)
	{
        gHpgpHalCB.halStats.CorruptFrmCnt++;
//		printf("Received Corrupt Frame; SeqNum=%bu, ssn=%u, FrmLen=%u,  FCCSErrCnt=%lu, ICVErrCnt=%lu, AddrFiltErr=%lu \n\n", rxSeqNum, pPlcRxFrmSwDesc->frmInfo.plc.ssn, pPlcRxFrmSwDesc->frmLen, hal_common_reg_32_read(PLC_FCCSERRCNT_REG), hal_common_reg_32_read(PLC_ICVERRCNT_REG), hal_common_reg_32_read(PLC_ADDRFILTERERRCNT_REG));
	}
    else
    {
        if(((gHpgpHalCB.halStats.TotalRxGoodFrmCnt - gHpgpHalCB.halStats.RxGoodBcnCnt) & (u32)(0xFF)) == 0)
        {  
            printf("\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b");
#ifndef MPER			
              printf("Sent %ld, Received %ld HPGP frames.", gHpgpHalCB.halStats.CurTxTestFrmCnt, gHpgpHalCB.halStats.TotalRxGoodFrmCnt - gHpgpHalCB.halStats.RxGoodBcnCnt);
//#else
//            printf("Sent %ld, Received %ld HPGP frames.", gHpgpHalCB.halStats.CurTxTestFrmCnt, gHpgpHalCB.halStats.TotalRxGoodFrmCnt - gHpgpHalCB.halStats.RxGoodBcnCnt - gHpgpHalCB.halStats.DuplicateRxCnt);
#endif
        }
    }
#endif
}



/*void HHAL_BPIntHandler1()
{
   
   
	u32 NTB1, rolloverpulses; //, NTB2, bpst, bpst1 ;

   // PrepareAndSendBcn();
 
   // printf("\n bpstdiff = %lu", ((oldgbpst - gbpst) * 40));
   // oldgbpst = gbpst;
	if(gHpgpHalCB.bPerAvgInitDone && gHpgpHalCB.bcnInitDone)
	{
		 
	   	//printf("\n b2");
	   	//WriteU32Reg(PLC_BPST_REG, ctorl(gbpst));
		//WriteU32Reg(PLC_SWBCNPERAVG_REG, ctorl(gPERavg));	
	
		 NTB1 = rtocl(ReadU32Reg(PLC_NTB_REG));
		//bpst = rtocl(ReadU32Reg(PLC_BPST_REG));
		//printf("\n gbpst = %lu\n",gbpst );
		//printf("\n NTB2 = %lu\n",NTB1);
		//printf("\n N1 = %lu",((gbpst - NTB1) * 40));
	    if(gRollOver)
		{
			gRollOver = 0;
			//NTB1 = NTB1 + PLC_HPGPBPINT_OFFSET + 0xB478;
			//printf("\n NTB1B = %lu",NTB1);
			//if((NTB1 + PLC_HPGPBPINT_OFFSET) < gbpst)
            rolloverpulses = (0xffffffff - NTB1);
            if((gbpst + rolloverpulses) > BCN_PREPARATION_TIME)
			{
				PrepareAndSendBcn();
			}
			else
			{
					//printf("\n roll over");
                   // printf("\n\n not enough time");
					//printf("\n bpst = %lu",gbpst);
					//printf("\n NTB1 = %lu",NTB1); 
			}
		}		
		else if(gbpst > NTB1)
		{
			if((gbpst -  NTB1) > BCN_PREPARATION_TIME)
			{
					
					PrepareAndSendBcn();
			}
			else
			{
					gHpgpHalCB.BcnLateCnt++;
					//printf("\n\n not enough time");
				//	printf("\n bpst = %lu",gbpst);
				//	printf("\n NTB1 = %lu",NTB1); 
			}
		}
		else
			{
				//printf("\n late");
			//		printf("\n bpst = %lu\n",gbpst);
				//	printf("\n NTB1 = %lu",NTB1);
					gHpgpHalCB.BcnLateCnt++;
			}	
	     
	
	    
	} 


 
}*/


void HHT_ProcessBcnHle(u8* bcn)
{ 
	sHybriiRxBcnHdr* pRxBcnHdr;
	sFrmCtrlBlk*     pFrmCtrlBlk;
    sHybriiRxBcn*    rxBcn;      
    sBcnHdr*         pBcnHdr = 0;
    sBeHdr*          pBeHdr = 0;

	rxBcn       = (sHybriiRxBcn*)bcn;
    pRxBcnHdr   = (sHybriiRxBcnHdr*)bcn; 
    pFrmCtrlBlk = (sFrmCtrlBlk*) (bcn + sizeof(sHybriiRxBcnHdr)) ;
    pBcnHdr     = (sBcnHdr*) ((u8*)pFrmCtrlBlk + sizeof(sFrmCtrlBlk)); 
    pBcnHdr->nid[NID_LEN-1] &= 0xFC;                          
	if( ((pBcnHdr->bt == BEACON_TYPE_CENTRAL) || (pBcnHdr->bt == BEACON_TYPE_PROXY)) )             
    {
        if(gHpgpHalCB.scanEnb && !gHpgpHalCB.nwSelected)
        {
            if(gHpgpHalCB.halStats.RxGoodBcnCnt >= PLC_BCNTST_SYNCTHRES ) 
            {                            
                HHAL_SetSnid(pFrmCtrlBlk->snid);                             
            }                    
        }
    }
}


void HHAL_DisplayPlcStat()
{
    u16 outStandingDescCnt;

    u16 totalDesc      = PLC_TXQ_DEPTH + PLC_TXQ_DEPTH + PLC_TXQ_DEPTH + PLC_TXQ_DEPTH;
    u16 freeDescCnt    =  (u16)(HHAL_GetPlcTxQFreeDescCnt(0) + HHAL_GetPlcTxQFreeDescCnt(1) + \
                          HHAL_GetPlcTxQFreeDescCnt(2) + HHAL_GetPlcTxQFreeDescCnt(3));  
    outStandingDescCnt = totalDesc - freeDescCnt;

    if(gHpgpHalCB.halStats.TotalRxGoodFrmCnt || gHpgpHalCB.halStats.RxErrBcnCnt)
    {
        printf("============ PLC Rx Statistics ==============\n");
#ifndef MPER		
        printf("TotalRxGoodFrmCnt = %lu\n",gHpgpHalCB.halStats.TotalRxGoodFrmCnt);
#else
        printf("TotalRxGoodFrmCnt = %lu\n",gHpgpHalCB.halStats.TotalRxGoodFrmCnt - gHpgpHalCB.halStats.DuplicateRxCnt);
#endif
        printf("TotalRxBytesCnt   = %lu\n",gHpgpHalCB.halStats.TotalRxBytesCnt);
#ifndef MPER
        printf("RxGoodDataCnt     = %lu\n",gHpgpHalCB.halStats.RxGoodDataCnt);
#else
        printf("RxGoodDataCnt     = %lu\n",gHpgpHalCB.halStats.RxGoodDataCnt - gHpgpHalCB.halStats.DuplicateRxCnt);
#endif
        printf("RxGoodBcnCnt      = %lu\n",gHpgpHalCB.halStats.RxGoodBcnCnt);
        printf("RxGoodMgmtCnt     = %lu\n",gHpgpHalCB.halStats.RxGoodMgmtCnt);
#ifdef RTX51_TINY_OS
        os_switch_task();
#else
        ISM_PollInt();
#endif
        printf("RxGoodSoundCnt    = %lu\n",gHpgpHalCB.halStats.RxGoodSoundCnt);

#ifndef MPER
        printf("TotalRxMissCnt    = %lu\n",gHpgpHalCB.halStats.TotalRxMissCnt); 		
        printf("DuplicateRxCnt    = %lu\n",gHpgpHalCB.halStats.DuplicateRxCnt); 
#endif
        printf("BcnRxIntCnt       = %lu\n",gHpgpHalCB.halStats.BcnRxIntCnt);
        printf("BcnSyncCnt        = %lu\n\n",gHpgpHalCB.halStats.BcnSyncCnt);
    }
#ifdef RTX51_TINY_OS
        os_switch_task();
#else
        ISM_PollInt();
#endif   
    if(gHpgpHalCB.halStats.TotalTxFrmCnt)
    {
        printf("============ PLC Tx Statistics ==============\n");
        printf("TotalTxFrmCnt     = %lu\n",gHpgpHalCB.halStats.TotalTxFrmCnt);
        printf("TotalTxBytesCnt   = %lu\n",gHpgpHalCB.halStats.TotalTxBytesCnt);
        printf("TxDataCnt         = %lu\n",gHpgpHalCB.halStats.TxDataCnt);
        printf("TxBcnCnt          = %lu\n",gHpgpHalCB.halStats.TxBcnCnt);
        printf("TxMgmtCnt         = %lu\n\n",gHpgpHalCB.halStats.TxMgmtCnt);
    }
#ifdef RTX51_TINY_OS
        os_switch_task();
#else
        ISM_PollInt();
#endif  
    printf("============ PLC Err Statistics =============\n");
    printf("AddrFilterErrCnt  = %lu\n",hal_common_reg_32_read(PLC_ADDRFILTERERRCNT_REG));
#ifndef MPER	
    printf("FrameCtrlErrCnt   = %lu\n",hal_common_reg_32_read(PLC_FCCSERRCNT_REG));	
    printf("PBCSRxErrCnt      = %lu\n",hal_common_reg_32_read(PLC_PBCSRXERRCNT_REG));
#endif
    printf("PBCSTxErrCnt      = %lu\n",hal_common_reg_32_read(PLC_PBCSTXERRCNT_REG));
#ifndef MPER
    printf("ICVErrCnt         = %lu\n",hal_common_reg_32_read(PLC_ICVERRCNT_REG));
#endif
#ifdef RTX51_TINY_OS
        os_switch_task();
#else
        ISM_PollInt();
#endif
    printf("RxErrBcnCnt       = %lu\n",gHpgpHalCB.halStats.RxErrBcnCnt);
    printf("PLCMpduDropCnt    = %lu\n",hal_common_reg_32_read(PLC_MPDUDROPCNT_REG));
#ifndef MPER	
    printf("CorruptFrmCnt     = %lu\n",gHpgpHalCB.halStats.CorruptFrmCnt);
#endif
    printf("MissSyncCnt       = %lu\n",gHpgpHalCB.halStats.MissSyncCnt);
    printf("STAleadCCOCount   = %lu\n",gHpgpHalCB.halStats.STAleadCCOCount);
    printf("STAlagCCOCount    = %lu\n",gHpgpHalCB.halStats.STAlagCCOCount);
    ;
#ifdef RTX51_TINY_OS
        os_switch_task();
#else
        ISM_PollInt();
#endif
    printf("PendingTxDescCnt  = %u\n",outStandingDescCnt);
    printf("PhyActRstCnt      = %bu\n",gHpgpHalCB.halStats.phyActHangRstCnt ); 
    printf("macTxStuckCnt     = %u\n",gHpgpHalCB.halStats.macTxStuckCnt);
    printf("macRxStuckCnt     = %u\n",gHpgpHalCB.halStats.macRxStuckCnt);
    printf("phyStuckCnt       = %bu\n",gHpgpHalCB.halStats.phyStuckCnt);
#ifdef RTX51_TINY_OS
        os_switch_task();
#else
        ISM_PollInt();
#endif
    printf("mpiRxStuckCnt     = %bu\n",gHpgpHalCB.halStats.mpiRxStuckCnt);
    printf("smTxStuckCnt      = %bu\n",gHpgpHalCB.halStats.smTxStuckCnt);
    printf("smRxStuckCnt      = %bu\n",gHpgpHalCB.halStats.smRxStuckCnt);
    printf("macHangRecover1   = %bu\n",gHpgpHalCB.halStats.macHangRecover1);
    printf("macHangRecover2   = %bu\n\n",gHpgpHalCB.halStats.macHangRecover2);
#ifdef RTX51_TINY_OS
        os_switch_task();
#else
        ISM_PollInt();
#endif
    printf("FreeCPCnt         = %bu\n",CHAL_GetFreeCPCnt());    
    printf("TimerIntCnt       = %lu\n",gHalCB.timerIntCnt); 
    printf("BPIntCnt          = %lu\n",gHpgpHalCB.halStats.bpIntCnt);
    printf("BcnSentIntCnt     = %lu\n",gHpgpHalCB.halStats.BcnSentIntCnt);
#ifdef RTX51_TINY_OS
        os_switch_task();
#else
        ISM_PollInt();
#endif
    printf("BPIntGap          = %u\n",gHpgpHalCB.bpIntGap);
    printf("SS1               = %lX\n",gHpgpHalCB.halStats.lastSs1);
    printf("NTBB4             = %lX\n",gHpgpHalCB.lastNtbB4);
    printf("NTBAft            = %lX\n",gHpgpHalCB.lastNtbAft);
    printf("BPST              = %lX\n\n",gHpgpHalCB.lastBpst);
#ifdef RTX51_TINY_OS
            os_switch_task();
#else
            ISM_PollInt();
#endif

    printf("============  Q Controller Statistics =============\n");
    printf("No 1st Desc             = %bu\n",gHalCB.qc_no_1st_desc);    
    printf("Too many desc           = %bu\n",gHalCB.qc_too_many_desc);    
    printf("No desc                 = %bu\n",gHalCB.qc_no_desc);    
    printf("No grant (CPU Tx Q)     = %bu\n",gHalCB.qc_no_grant);

#ifdef RTX51_TINY_OS
        os_switch_task();
#else
        ISM_PollInt();
#endif
    printf("No grant (free CP)      = %bu\n",gHalCB.cp_no_grant_free_cp);
    printf("No grant (alloc CP)     = %d\n",gHalCB.cp_no_grant_alloc_cp);
    printf("No grant (read CP mem)  = %d\n",gHalCB.cp_no_grant_read_cp);
    printf("No grant (write CP mem) = %d\n\n",gHalCB.cp_no_grant_write_cp);
}


