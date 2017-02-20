/*
* $Id: hal_hpgpRx.c,v 1.32 2015/01/02 14:55:35 kiran Exp $
*
* $Source: /home/cvsrepo/Hybrii_B_OSFW_Dev/firmware/hal/hal_hpgpRx.c,v $
*
* Description : HPGP HAL Receive module.
*
* Copyright (c) 2010-2011 Greenvity Communications, Inc.
* All rights reserved.
*
* Purpose :
*     Defines beacon and data receive functions for HPGP, .
*
*
*/
#ifdef RTX51_TINY_OS
#include <rtx51tny.h>
#endif

#include <stdio.h>
#include <string.h>
#include "papdef.h"
#ifdef ROUTE
#include "hpgp_route.h"
#endif
#include "fm.h"
#include "hal_common.h"
#include "hal.h"
#include "nma.h"
#include "nma_fw.h"
#include "hal_hpgp.h"
#include "hal_eth.h"
#include "hal_tst.h"
#include "hpgpevt.h"
#include "hpgpdef.h"
#ifdef HPGP_HAL_TEST
#include "hal_cfg.h"
#include "mmsg.h"
#else
#ifndef CALLBACK
#include "hpgpapi.h"
#endif
#endif
#include "frametask.h"
#include "datapath.h"
#include "hybrii_tasks.h"
#include "gv701x_gpiodriver.h"
#include "sys_common.h"
#include "event_fw.h"
#ifdef NO_HOST
#include "gv701x_uartdriver_fw.h"
#endif

#ifdef UM
#include "NSM.h"
#include "green.h"
#include "muxl.h"
extern sHomePlugCb HomePlug;
#endif
#ifdef DEBUG_DATAPATH
extern u8 pktDbg;
extern u8 sigDbg;
#endif
#ifdef PLC_TEST
u16 oldssn = 0;
#endif
u8 dropFlag = 0;

#ifdef SW_BCST
volatile u8 gSoftBCST = 1;
#define SW_SSN_SIZE 1
#endif

u8 gpbcscorrect, gvalid;
u32 gsnid;
u32 debugcnt = 0;
extern u32 gBcnPer;
u32 misscnt = 0;
u8 gbcnstart, gflag;
extern u32 gbpst1, gtimer2, gtimer1;
extern u8 opMode;
extern u8 gBcnMissingRescanCnt;
extern u16  prevCsmaRgns[HYBRII_MAXSMAREGION_CNT];
#ifdef ROUTE_TEST
u8 dropTei[3] = {0,0,0};
#endif

#ifdef PLC_TEST
extern void printmsg(u8* buff, u8 len);
extern void HHT_ProcessPlcFrame(sSwFrmDesc* plcRxFrmSwDesc);
#endif

	
#ifdef ASSOC_TEST
extern eStatus LM_RecvFrame(sSwFrmDesc *rxFrmSwDesc, 
								 uRxFrmHwDesc*	pRxPktQ1stDesc,
								 uRxCpDesc* 	pRxPktQCPDesc);
	
#endif
#ifdef ROUTE
extern sScb* ROUTE_lookUpLRT(u8 *mac);
#endif

extern void EHT_FromPlcBcnTx(u8* rxBcnByteArr, u16 frameSize);
extern void SNSM_BcnCheck(u8* bcn);


uCSMARegionReg   csmaRegArr[HYBRII_MAXSMAREGION_CNT]; 
u32              u32CSMARegs[HYBRII_MAXSMAREGION_CNT];


eStatus CHAL_ReadFrame(sSwFrmDesc *rxFrmSwDesc,
                       sBuffDesc *buffDesc)
{
    volatile u8 XDATA *cellAddr;
    u8        frmOffset = 0;
    u8        i;
    s16       frmLen = rxFrmSwDesc->frmLen;
    s16       cellLen;
    u8       *dataptr = NULL;
    sHpgpHdr *hpgpHdr = NULL;
    eStatus   ret = STATUS_SUCCESS;


    /* sanity test on frame length */
/*    if (frmLen != ((rxFrmSwDesc->cpCount -1)*HYBRII_CELLBUF_SIZE +
                  rxFrmSwDesc->lastDescLen))       
    {
        FM_Printf(FM_ERROR, "CHAL: Frame length error.\n");
        ret = STATUS_FAILURE;
        goto done;
    } */

    if (rxFrmSwDesc->rxPort == PORT_PLC)
    {
        /* the first cell of the mgmt frame contains 4-byte */
        frmOffset = ((rxFrmSwDesc->frmType == HPGP_HW_FRMTYPE_MGMT) ? 4 : 0);
    }


    frmLen -= frmOffset;
    buffDesc->datalen = frmLen;
	//cellLen = HYBRII_CELLBUF_SIZE - frmOffset;
	cellLen = MIN(HYBRII_CELLBUF_SIZE - frmOffset, frmLen);
//FM_Printf(FM_MINFO,"rx data len: %d, cell len: %d, cp cnt %bu\n", buffDesc->datalen, cellLen, rxFrmSwDesc->cpCount);
//FM_HexDump(FM_DATA|FM_MINFO, "rx buff:", (u8 *)buffDesc->dataptr, 256 );  
     
    dataptr = buffDesc->dataptr; 

	i = 0;
//    for (i = 0; i < rxFrmSwDesc->cpCount; i++)
    while((i < rxFrmSwDesc->cpCount) && cellLen > 0)
    {
        cellAddr = CHAL_GetAccessToCP(rxFrmSwDesc->cpArr[i].cp);
        memcpy(dataptr, cellAddr+frmOffset, cellLen);
        dataptr += cellLen;
        frmLen -= cellLen;
        cellLen = MIN(HYBRII_CELLBUF_SIZE, frmLen);
        frmOffset = 0;
		i++;
    }
//FM_Printf(FM_MINFO,"rx data len: %d, frm len: %d\n", buffDesc->datalen, frmLen);
//FM_HexDump(FM_DATA|FM_MINFO, "rx data:", (u8 *)buffDesc->dataptr, 256 );  
//done:
    /* free cp */
    CHAL_FreeFrameCp(rxFrmSwDesc->cpArr, rxFrmSwDesc->cpCount);
    return ret;
}

#if !defined (HPGP_HAL_TEST) && !defined(UM)

void HHAL_ProcRxFrameDesc(struct haLayer *hal, uRxFrmHwDesc *rxFrmHwDesc,
                          sSwFrmDesc *plcRxFrmSwDesc)
{
    uPlcRssiLqiReg  rssiLqi;
    uPlcStatusReg   plcStatus;
    u16             frmLen;
    u8              stei;

#ifdef _AES_SW_    
    // Reset AES Block
    plcStatus.reg = ReadU32Reg(PLC_STATUS_REG);
    plcStatus.s.aesReset = 1;
    WriteU32Reg(PLC_STATUS_REG, plcStatus.reg);

    plcStatus.reg = ReadU32Reg(PLC_STATUS_REG);
    plcStatus.s.aesReset = 0;
    WriteU32Reg(PLC_STATUS_REG, plcStatus.reg);
#endif

    /* Retrieve LQI_RSSI */
    rssiLqi.reg         = ReadU32Reg(PLC_RSSILQI_REG);
    plcRxFrmSwDesc->frmInfo.plc.rssi = rssiLqi.s.rssi;
    plcRxFrmSwDesc->frmInfo.plc.lqi  = rssiLqi.s.lqi;

    /* Extract Frame Length */
    frmLen = rxFrmHwDesc->sof.frmLenHi;
    frmLen = frmLen<<PKTQDESC1_FRMLENHI_POS | rxFrmHwDesc->sof.frmLenLo;
    plcRxFrmSwDesc->frmLen = frmLen;
//  printf("\nfrmLen : %d \n", frmLen);

    /* Extract PLC Frame Type */
    plcRxFrmSwDesc->frmType = (eHpgpHwFrmType)rxFrmHwDesc->sof.frmType;
    /* Extract source TEI */
    stei   = rxFrmHwDesc->sof.steiHi2;
    stei   = (stei << PKTQDESC1_STEIHI_POS ) | rxFrmHwDesc->sof.steiLo6;
    plcRxFrmSwDesc->frmInfo.plc.stei    = stei; 


//  printf("rx len %d\n", frmLen);
    

    if(plcRxFrmSwDesc->frmType == HPGP_HW_FRMTYPE_MSDU)
    {
        gHpgpHalCB.halStats.RxGoodDataCnt++;
        /* Extract PLC Frame Mcst Mode */
        if(rxFrmHwDesc->sof.bcst)
        {
            plcRxFrmSwDesc->frmInfo.plc.mcstMode = HPGP_MNBCST;
        }
        else if(rxFrmHwDesc->sof.mcst)
        {
            plcRxFrmSwDesc->frmInfo.plc.mcstMode = HPGP_MCST;
        }
        else
        {
            plcRxFrmSwDesc->frmInfo.plc.mcstMode = HPGP_UCST;
        }
    }
    else if(plcRxFrmSwDesc->frmType == HPGP_HW_FRMTYPE_SOUND)
    {
        /*
         * H/W does not provide STEI for Sound packet
         */
        plcRxFrmSwDesc->frmInfo.plc.stei = 0;
        gHpgpHalCB.halStats.RxGoodSoundCnt++;
    }
    else 
    {
        gHpgpHalCB.halStats.RxGoodMgmtCnt++;
    }          

    plcRxFrmSwDesc->frmInfo.plc.clst    = rxFrmHwDesc->sof.clst;


    /* Update HPGP statistics - may be needed only in diag mode */
    gHpgpHalCB.halStats.TotalRxGoodFrmCnt++;
    gHpgpHalCB.halStats.TotalRxBytesCnt += plcRxFrmSwDesc->frmLen;
    gHpgpHalCB.halStats.macRxStuckCnt = 0;
    gHpgpHalCB.halStats.smRxStuckCnt = 0;
}

#endif

#ifndef HPGP_HAL_TEST


eStatus HAL_RecvFrame(sHaLayer *hal, sSwFrmDesc *rxFrmSwDesc)
{
    sEvent    xdata  *event = NULL;
    sBuffDesc   *buffDesc = NULL;
    sEth2Hdr    *ethhdr = NULL;
    sHpgpHdr    *hpgpHdr = NULL; 
    eStatus      ret = STATUS_SUCCESS;
    volatile u8 XDATA *cellAddr;
	sLinkLayer *linkl = HPGPCTRL_GetLayer(HP_LAYER_TYPE_LINK);    
    
#ifdef ROUTE
	u16 frmlen;
    u8 i;
#endif
    u8 frmOffset;

#ifndef B1
	sScb *scb = CRM_GetScb(LINKL_GetCrm(linkl), rxFrmSwDesc->frmInfo.plc.stei);
#endif

#ifdef ROUTE
    sScb *rScb = NULL;    
    sStaInfo *staInfo = LINKL_GetStaInfo(linkl);
#endif
    switch(rxFrmSwDesc->rxPort)
    {
        case PORT_PLC:
        {
            cellAddr = CHAL_GetAccessToCP(rxFrmSwDesc->cpArr[0].cp);
            frmOffset = ((rxFrmSwDesc->frmType == HPGP_HW_FRMTYPE_MGMT) ? 4 : 0);
            
	        ethhdr = (sEth2Hdr *)&cellAddr[frmOffset];
#ifdef B1			
            if ((rxFrmSwDesc->frmInfo.plc.stei == 0) || (IS_GROUP(ethhdr->dstaddr)))
			{			
			}
            else
			{
				sScb *scb = CRM_GetScb(LINKL_GetCrm(linkl), rxFrmSwDesc->frmInfo.plc.stei);
				if (!scb)
				{
				    if (rxFrmSwDesc->frmType != HPGP_HW_FRMTYPE_MGMT)
                    {
                        gHpgpHalCB.halStats.PtoHswDropCnt++;
    					return STATUS_FAILURE;
                    }
				}
#ifdef KEEP_ALIVE				
				scb->hit = 1;
				scb->hitCount = 0;
#endif				
                if(scb->uWrapAround)
                {
                    if(rxFrmSwDesc->frmInfo.plc.ssn < 1000)
                    {                    
                        scb->uWrapAround = 0;
                        scb->uMinSSN = 0;
                    }
                }              
				if ((rxFrmSwDesc->frmInfo.plc.ssn <= scb->uMinSSN) && (scb->uMinSSN != 0))
				{
					// Drop duplicate frm
					gHpgpHalCB.halStats.DuplicateRxCnt++;
					return STATUS_FAILURE;
				}
				
				scb->uMinSSN = rxFrmSwDesc->frmInfo.plc.ssn;// + numPBs;
				
				if(scb->uMinSSN >= 1500)
				{
					scb->uWrapAround = 1;
				}

			}

          
#endif
				// Drop frm if dest MAC not match with device MAC address
				if(IS_GROUP(ethhdr->dstaddr))
				{
				}
				else if(memcmp(ethhdr->dstaddr, hal->macAddr, MAC_ADDR_LEN) == 0)
				{
					
				}
#ifdef ROUTE
				else if((rScb = ROUTE_lookUpLRT(ethhdr->dstaddr)) != NULL)
				{
					
					//FM_Printf(FM_USER, "LH: %bu\n",scb->tei);
					if(rScb->lrtEntry.routeIsInvalid == TRUE || rScb->lrtEntry.routeOnHold == TRUE)
					{
						return STATUS_FAILURE;
					}
					rxFrmSwDesc->frmInfo.plc.dtei = rScb->lrtEntry.nTei;
					rxFrmSwDesc->frmInfo.plc.eks = staInfo->nekEks;
					if(linkl->mode == LINKL_STA_MODE_CCO)
					{

						rxFrmSwDesc->frmInfo.plc.stei = staInfo->ccoScb->tei;
					}
					else
					{
						rxFrmSwDesc->frmInfo.plc.stei =  staInfo->tei;
					}


				   {


						rxFrmSwDesc->frmInfo.plc.bcnDetectFlag	= REG_FLAG_SET;// REG_FLAG_CLR;
						rxFrmSwDesc->frmInfo.plc.clst = HPGP_CONVLYRSAPTYPE_ETH;

						rxFrmSwDesc->frmInfo.plc.plid = 0;	 //[YM] This line of code has to be changed base on differnet QoS priority


						if (rxFrmSwDesc->frmInfo.plc.plid == 0)
						rxFrmSwDesc->frmInfo.plc.phyPendBlks	= HPGP_PPB_CAP0;
						else
						rxFrmSwDesc->frmInfo.plc.phyPendBlks	= HPGP_PPB_CAP123;

						rxFrmSwDesc->frmInfo.plc.mcstMode		= HPGP_UCST;  // Unicast

						rxFrmSwDesc->frmInfo.plc.stdModeSel 	= STD_ROBO_TEST; // std robo
						rxFrmSwDesc->frmInfo.plc.dt_av = HPGP_DTAV_SOF;
						rxFrmSwDesc->frmInfo.plc.saf = 1;


					}

					
					frmlen = rxFrmSwDesc->frmLen;
					for(i = 0; i< rxFrmSwDesc->cpCount; i++)
					{
						if(( i == 0) && (rxFrmSwDesc->frmType == HPGP_HW_FRMTYPE_MGMT))
						{
							rxFrmSwDesc->cpArr[i].offsetU32 = 1;						   
						}
						rxFrmSwDesc->cpArr[i].len = MIN(frmlen, HYBRII_CELLBUF_SIZE);
						frmlen = frmlen - HYBRII_CELLBUF_SIZE;
					}
					rxFrmSwDesc->txPort = PORT_PLC;
					fwdAgent_handleData(rxFrmSwDesc);
					return STATUS_SUCCESS;
				}
#endif 
				else
				{
					// Drop frm
#ifdef DEBUG_DATAPATH
					if(pktDbg)
					{
						FM_Printf(FM_MINFO, "Drop\n");
					}
#endif
					gHpgpHalCB.halStats.PtoHswDropCnt++;
					return STATUS_FAILURE;
				}
#ifndef B1
			if (!scb)
			{
				if (rxFrmSwDesc->frmType != HPGP_HW_FRMTYPE_MGMT)
				{
					gHpgpHalCB.halStats.PtoHswDropCnt++;						
					return STATUS_FAILURE;
				}
			
			}
			else
			{
				scb->rssiLqi.s.rssi = rxFrmSwDesc->frmInfo.plc.rssi;
				scb->rssiLqi.s.lqi = rxFrmSwDesc->frmInfo.plc.lqi;

#ifdef KEEP_ALIVE
				scb->hit = 1;
				scb->hitCount = 0;
#endif	

			}

			
			//printf("tei %bu rssi %bu\n", scb->tei, scb->rssiLqi.s.rssi);
			

            if ((rxFrmSwDesc->frmInfo.plc.stei == 0) || (IS_GROUP(ethhdr->dstaddr)))
			{
			
			}
            else if (scb)
			{
				
                if(scb->uWrapAround)
                {
                    if(rxFrmSwDesc->frmInfo.plc.ssn < 1000)
                    {                    
                        scb->uWrapAround = 0;
                        scb->uMinSSN = 0;
                    }
                }
                

				//printf("tei %bu rssi %bu\n", scb->tei, scb->rssiLqi.s.rssi);
				
				if ((rxFrmSwDesc->frmInfo.plc.ssn <= scb->uMinSSN) && (scb->uMinSSN != 0))
                {
				    // Drop duplicate frm
				    gHpgpHalCB.halStats.DuplicateRxCnt++;                    
                    //printf("Dup frm: %u, %u \n",rxFrmSwDesc->frmInfo.plc.ssn, scb->uMinSSN);
					return STATUS_FAILURE;
				}
                
                scb->uMinSSN = rxFrmSwDesc->frmInfo.plc.ssn;// + numPBs;
                
                if(scb->uMinSSN >= 1500)
                {
                    scb->uWrapAround = 1;
                }

			}
#endif
			if (rxFrmSwDesc->frmType == HPGP_HW_FRMTYPE_MGMT)
			{
				/* create an event for the mgmt message */
				event = EVENT_MgmtAlloc(rxFrmSwDesc->frmLen, sizeof(sHpgpHdr));
				if (event == NULL)
				{
                    FM_Printf(FM_ERROR, "HAL:EAF\n");
					return STATUS_FAILURE;
				}

				buffDesc = &event->buffDesc;

                if (CHAL_ReadFrame(rxFrmSwDesc, buffDesc) == STATUS_SUCCESS)
                {
                    hpgpHdr = (sHpgpHdr *)event->buffDesc.buff;
                    //hpgpHdr->snid = hal->hhalCb->snid;
                    hpgpHdr->snid = rxFrmSwDesc->frmInfo.plc.snid;
                    hpgpHdr->tei = rxFrmSwDesc->frmInfo.plc.stei;
  //                  FM_Printf(FM_MINFO, "HAL: Rx Snid=%bu, Stei=%bu\n", hpgpHdr->snid, hpgpHdr->tei);
                    event->eventHdr.eventClass = EVENT_CLASS_MSG;
                    /* process the MAC header */
                    ethhdr = (sEth2Hdr *)event->buffDesc.dataptr; 
#ifdef LOG_FLASH
					logEvent(MGMT_MSG,0,event->eventHdr.type,&hpgpHdr->tei,1);
#endif
#if 1					
					// Drop frm if dest MAC not match with device MAC address
					if(IS_GROUP(ethhdr->dstaddr))
					{
					}
					else if(memcmp(ethhdr->dstaddr, hal->macAddr, MAC_ADDR_LEN) == 0)
					{
					}
					
#ifdef ROUTE
					else if((scb = ROUTE_lookUpLRT(ethhdr->dstaddr)) != NULL)
					{
						if(scb->lrtEntry.routeIsInvalid == TRUE || scb->lrtEntry.routeOnHold == TRUE)
						{
							return STATUS_SUCCESS;
						}
#ifndef RELEASE
						FM_Printf(FM_USER, "LRT hit: %bu\n",scb->tei);
#endif
						hpgpHdr->tei = scb->lrtEntry.nTei;
						hpgpHdr->eks = staInfo->nekEks;
						//rxFrmSwDesc->plc.stei =  // TBD
						fwdAgent_sendFrame(PORT_PLC ,event);
						return STATUS_SUCCESS;
					}
#endif
					else
					{
						// Drop frm
#ifdef DEBUG_DATAPATH
						if(pktDbg)
						{
							FM_Printf(FM_MINFO, "MGMT drop\n");
						}
#endif
						EVENT_Free(event);
						return STATUS_SUCCESS; //STATUS_FAILURE;
					}
#endif                    
					hpgpHdr->macAddr = ethhdr->srcaddr;
//					  hpgpHdr->scb = CRM_FindScbMacAddr(hpgpHdr->macAddr);

															
					/* chop off the ethernet header */
					event->buffDesc.dataptr += sizeof(sEth2Hdr); 
					event->buffDesc.datalen -= sizeof(sEth2Hdr); 
					/* deliver the mgmt message to the upper layer */
#ifdef CALLBACK
					hal->deliverMgmtPacket(hal->mgmtCookie, event);
#else
					MUXL_RecvMgmtPacket(hal->mgmtCookie, event);
#endif
				 //   CHAL_FreeFrameCp(rxFrmSwDesc->cpArr, rxFrmSwDesc->cpCount);
				}
				else
				{
					EVENT_Free(event);
				}
			}
			else if (rxFrmSwDesc->frmType == HPGP_HW_FRMTYPE_MSDU)
			{
				/* deliver the data packet to the data plane */

					
					
					// FM_Printf(FM_USER,"DATA ssn: %u\n", rxFrmSwDesc->frmInfo.plc.ssn);

						

#ifdef ROUTE

					  //  cellAddr = CHAL_GetAccessToCP(rxFrmSwDesc->cpArr[0].cp);
					  //  ethhdr = (sEth2Hdr *)cellAddr;
						// Drop frm if dest MAC not match with device MAC address
						if(IS_GROUP(ethhdr->dstaddr))
						{
						}
						else if(memcmp(ethhdr->dstaddr, hal->macAddr, MAC_ADDR_LEN) == 0)
						{
							
						}
						else if((scb = ROUTE_lookUpLRT(ethhdr->dstaddr)) != NULL)
						{
#ifndef RELEASE							
							FM_Printf(FM_USER, "LH: %bu\n",scb->tei);
#endif
							if(scb->lrtEntry.routeIsInvalid == TRUE || scb->lrtEntry.routeOnHold == TRUE)
							{
								return STATUS_FAILURE;
							}
							rxFrmSwDesc->frmInfo.plc.dtei = scb->lrtEntry.nTei;
							rxFrmSwDesc->frmInfo.plc.eks = staInfo->nekEks;
							if(linkl->mode == LINKL_STA_MODE_CCO)
							{

								rxFrmSwDesc->frmInfo.plc.stei = staInfo->ccoScb->tei;
							}
							else
							{
								rxFrmSwDesc->frmInfo.plc.stei =  staInfo->tei;
							}


							{


								rxFrmSwDesc->frmInfo.plc.bcnDetectFlag	= REG_FLAG_SET;// REG_FLAG_CLR;
								rxFrmSwDesc->frmInfo.plc.clst = HPGP_CONVLYRSAPTYPE_ETH;

								rxFrmSwDesc->frmInfo.plc.plid = 0;	 //[YM] This line of code has to be changed base on differnet QoS priority


								if (rxFrmSwDesc->frmInfo.plc.plid == 0)
								rxFrmSwDesc->frmInfo.plc.phyPendBlks	= HPGP_PPB_CAP0;
								else
								rxFrmSwDesc->frmInfo.plc.phyPendBlks	= HPGP_PPB_CAP123;

								rxFrmSwDesc->frmInfo.plc.mcstMode		= HPGP_UCST;  // Unicast

								rxFrmSwDesc->frmInfo.plc.stdModeSel 	= STD_ROBO_TEST; // std robo
								rxFrmSwDesc->frmInfo.plc.dt_av = HPGP_DTAV_SOF;
								rxFrmSwDesc->frmInfo.plc.saf = 1;


							}
							frmlen = rxFrmSwDesc->frmLen;
							for(i = 0; i< rxFrmSwDesc->cpCount; i++)
							{
								rxFrmSwDesc->cpArr[i].offsetU32 = 0;
								rxFrmSwDesc->cpArr[i].len = MIN(frmlen, HYBRII_CELLBUF_SIZE);
								frmlen = frmlen - HYBRII_CELLBUF_SIZE;
							}
							rxFrmSwDesc->txPort = PORT_PLC;
							fwdAgent_handleData(rxFrmSwDesc);
							return STATUS_SUCCESS;
						}
						else
						{
							// Drop frm
#ifdef DEBUG_DATAPATH
							if(pktDbg)
							{
								FM_Printf(FM_MINFO, "DATA drop\n");
							}
#endif
							return STATUS_FAILURE;
						}
#endif


	//						FM_Printf(FM_USER, "prx\n");
#if (defined UART_HOST_INTF) && (defined NO_HOST)//#ifdef NO_HOST
						if(rxFrmSwDesc->frmLen > (APP_DATA_MAX_SIZE + 14))
						{
							 gHpgpHalCB.halStats.PtoHswDropCnt++;
							 return STATUS_FAILURE;
						}
#ifdef SW_BCST			
						if(gSoftBCST == 1)
						{
							if(IS_GROUP(ethhdr->dstaddr))
		                    {
		                    	//sScb *scb;
	                    		u8 lastDescLen;
								u8 xdata* cellAddr;
	                    		//scb = CRM_GetScb(LINKL_GetCrm(linkl), rxFrmSwDesc->frmInfo.plc.stei);
								//if(scb != NULL)
								//{
									lastDescLen = rxFrmSwDesc->frmLen % HYBRII_CELLBUF_SIZE;
									//printf("lastDescLen %bu\n",lastDescLen);
									cellAddr = CHAL_GetAccessToCP(rxFrmSwDesc->cpArr[(rxFrmSwDesc->cpCount)-1].cp);
									//printf("rx SSN %bu\n",cellAddr[lastDescLen - SW_SSN_SIZE]);
									if(cellAddr[lastDescLen - SW_SSN_SIZE] == scb->swSsn)
									{
										gHpgpHalCB.halStats.PtoHswDropCnt++;
										gHpgpHalCB.halStats.swBcstDropCnt++;
							 			return STATUS_FAILURE;
									}
									else
									{
										scb->swSsn = cellAddr[lastDescLen-1];
										rxFrmSwDesc->frmLen -= SW_SSN_SIZE;
										if(lastDescLen > SW_SSN_SIZE)
										{		
											rxFrmSwDesc->cpArr[(rxFrmSwDesc->cpCount)-1].len -= SW_SSN_SIZE;
											rxFrmSwDesc->lastDescLen -= SW_SSN_SIZE;
										}
										else
										{
											rxFrmSwDesc->lastDescLen = HYBRII_CELLBUF_SIZE;
											rxFrmSwDesc->cpCount--;
											CHAL_freeCP(rxFrmSwDesc->cpArr[(rxFrmSwDesc->cpCount)-1].cp);
										}
									}
								//}
							}
	                    }
#endif
#endif
						fwdAgent_handleData(rxFrmSwDesc);
			}
			else
			{
				/* unknown HPGP frame type */
			}
			break;
		}
			/* TODO: need to differentiate the mgmt and data */ 			
			/* create an event for the mgmt message */

				/* deliver the mgmt message to the upper layer */
			   // CHAL_FreeFrameCp(rxFrmSwDesc->cpArr, rxFrmSwDesc->cpCount); // [PRA] double free
		default:
		{
			/* deliver the data packet to the data plane */
			//hal->deliverDataPacket(hal->dataCookie, rxFrmSwDesc);
		}
	}

	return ret;
}

#endif

void HHAL_RxSoundIntHandler()
{
    uPlcRssiLqiReg  rssiLqi;
    // Retrieve LQI_RSSI   
    rssiLqi.reg         = ReadU32Reg(PLC_RSSI_REG);
    //printf(" SOUND int: 0xE4C REG Value= %lx, \n", rssiLqi.reg);
	
	//rssiLqi.reg         = ReadU32Reg(PLC_RSSILQI_REG);
	//printf(" SOUND 2: 0xE48 REG Value= %lx, \n", ReadU32Reg(PLC_RSSILQI_REG));
	
}


#ifdef HPGP_HAL_TEST
void HHAL_RxIntHandler(sCommonRxFrmSwDesc* pRxFrmDesc)
#else
void HHAL_RxIntHandler(sCommonRxFrmSwDesc* pRxFrmDesc, void *cookie)
#endif
{

    // If SrcPort is PLC, Frametype Bcn,Mgmt
    // Write to PLCCmdQ
//#ifdef PLC_TEST
    volatile u8 xdata * cellAddr;
//#endif
    uRxFrmHwDesc*   pRxPktQ1stDesc;
    uRxCpDesc*      pRxPktQCPDesc;
    uPlcRssiLqiReg  rssiLqi;
    sSwFrmDesc      plcRxFrmSwDesc;
    u16             frmLen;
    u16             tmpfrmLen;
    u8              snid;
    u8              stei;
    u16             ssn;
    u8              i;
    eStatus         status = STATUS_SUCCESS;
    sEth2Hdr         *ethHdr;
#ifdef UM
    sScb *scb;
#endif
#ifdef _AES_SW_    
    // Reset AES Block
    plcStatus.reg = ReadU32Reg(PLC_STATUS_REG);
    plcStatus.s.aesReset = 1;
    WriteU32Reg(PLC_STATUS_REG, plcStatus.reg);

    plcStatus.reg = ReadU32Reg(PLC_STATUS_REG);
    plcStatus.s.aesReset = 0;
    WriteU32Reg(PLC_STATUS_REG, plcStatus.reg);
#endif

#ifdef PLC_DELAY    
    CHAL_DelayTicks(500);
#endif





#if 0
    // Retrieve LQI_RSSI   -- [YM] May change to only do this on sound packet
    //rssiLqi.reg         = ReadU32Reg(PLC_RSSI_REG);
    rssiLqi.reg         = ReadU32Reg(PLC_RSSILQI_REG);
    //plcRxFrmSwDesc.frmInfo.plc.rssi = rssiLqi.s.rssi;
    //plcRxFrmSwDesc.frmInfo.plc.lqi  = rssiLqi.s.lqi;	
	printf(" E48 REG = %lx, \n", rssiLqi.reg);
	rssiLqi.reg         = ReadU32Reg(PLC_RSSI_REG);
	printf(" E4C REG = %lx, \n", rssiLqi.reg);

#endif

    pRxPktQ1stDesc      = (uRxFrmHwDesc*)&pRxFrmDesc->hdrDesc;
    pRxPktQCPDesc       = (uRxCpDesc*)&pRxFrmDesc->firstCpDesc;

    plcRxFrmSwDesc.rxPort = pRxPktQ1stDesc->sof.srcPort;
	plcRxFrmSwDesc.txPort = pRxPktQ1stDesc->sof.dstPort;
    plcRxFrmSwDesc.frmInfo.plc.rssi = pRxFrmDesc->rssiLqi.s.rssi;
    plcRxFrmSwDesc.frmInfo.plc.lqi  = pRxFrmDesc->rssiLqi.s.lqi;
    // 1.1.1 Extract PLC Frame Length
    frmLen = pRxPktQ1stDesc->sof.frmLenHi;
    frmLen = frmLen<<PKTQDESC1_FRMLENHI_POS | pRxPktQ1stDesc->sof.frmLenLo;
    plcRxFrmSwDesc.frmLen = frmLen;

    // 1.1.2 Extract PLC Frame Type
    plcRxFrmSwDesc.frmType = (eHpgpHwFrmType)pRxPktQ1stDesc->sof.frmType;
    stei   = pRxPktQ1stDesc->sof.steiHi2;
    stei   = (stei << PKTQDESC1_STEIHI_POS ) | pRxPktQ1stDesc->sof.steiLo6;

    snid   = pRxPktQCPDesc->plc.snidHi;
    snid   = (snid << PKTQDESC1_SNIDHI_POS ) | pRxPktQCPDesc->plc.snidLo;
    
    tmpfrmLen = frmLen;
    // Update HPGP statistics - may be needed only in diag mode


	/*{

    uPlcTxPktQDescVF0 *vf0 = (uPlcTxPktQDescVF0*)&pRxFrmDesc->fc[0];


	    stei = vf0->s.stei;

	    snid  = vf0->s.snid;


	}*/
#ifdef UM
    if (stei == 0)       
    {
    	cellAddr = CHAL_GetAccessToCP(pRxPktQCPDesc->plc.cp);
        //if(cellAddr != NULL)// cellAddress is never NULL. Kiran
        //{
             if (plcRxFrmSwDesc.frmType == HPGP_HW_FRMTYPE_MGMT)
             {
                 ethHdr = (sEth2Hdr*)(&(cellAddr + 4));
             }
             else
             {
                 ethHdr = (sEth2Hdr*)cellAddr;
             }
             scb = CRM_FindScbMacAddr(ethHdr->srcaddr);
             if(scb != NULL)
             {
                 stei = scb->tei;
             }
        //}
    }
#endif
    plcRxFrmSwDesc.frmInfo.plc.snid    = snid;
    plcRxFrmSwDesc.frmInfo.plc.stei    = stei; 

    ssn    = pRxPktQCPDesc->plc.ssnHi;
    ssn    = (ssn << PKTQDESC1_SSNHI_POS ) | pRxPktQCPDesc->plc.ssnLo;
#ifdef PLC_TEST
    if(opMode == LOWER_MAC)
    {
        if(pRxPktQ1stDesc->sof.bcst == 0 && pRxPktQ1stDesc->sof.mcst == 0)
        {
            if((ssn == oldssn) && (ssn != 0))
            {
                CHAL_DecrementReleaseCPCnt(pRxPktQCPDesc->plc.cp);
                for (i = 1; i < pRxFrmDesc->cpCount; i++)
                {
            		CHAL_DecrementReleaseCPCnt(pRxFrmDesc->cpArr[i]);
                }
                //printf("Dup frm: %u, %u \n",ssn, oldssn);
                return;
            }
            oldssn = ssn;
            if(oldssn == 2047)
            {
                oldssn = 0;
            }
        }
    }
#endif    
    gHpgpHalCB.halStats.TotalRxGoodFrmCnt++;
    gHpgpHalCB.halStats.TotalRxBytesCnt += plcRxFrmSwDesc.frmLen;
    gHpgpHalCB.halStats.macRxStuckCnt = 0;
    gHpgpHalCB.halStats.smRxStuckCnt = 0;

    if(plcRxFrmSwDesc.frmType == HPGP_HW_FRMTYPE_MSDU)
    {
        gHpgpHalCB.halStats.RxGoodDataCnt++;
        // 1.1.3 Extract PLC Frame Mcst Mode
        if(pRxPktQ1stDesc->sof.bcst)
        {
            plcRxFrmSwDesc.frmInfo.plc.mcstMode = HPGP_MNBCST;
        }
        else if(pRxPktQ1stDesc->sof.mcst)
        {
            plcRxFrmSwDesc.frmInfo.plc.mcstMode = HPGP_MCST;
        }
        else
        {
            plcRxFrmSwDesc.frmInfo.plc.mcstMode = HPGP_UCST;
        }
    }
    else if(plcRxFrmSwDesc.frmType == HPGP_HW_FRMTYPE_SOUND)
    {
        /*
         * H/W does not provide STEI for Sound packet
         */
        plcRxFrmSwDesc.frmInfo.plc.stei = 0;
        gHpgpHalCB.halStats.RxGoodSoundCnt++;
		//rssiLqi.reg         = ReadU32Reg(PLC_RSSILQI_REG);
        //plcRxFrmSwDesc.frmInfo.plc.rssi = rssiLqi.s.rssi;
        //plcRxFrmSwDesc.frmInfo.plc.lqi  = rssiLqi.s.lqi;	
	    //printf(" SOUND: E48 REG = %lx, \n", rssiLqi.reg);
	    rssiLqi.reg         = ReadU32Reg(PLC_RSSI_REG);
	    //printf(" SOUND: E4C REG = %lx, \n", rssiLqi.reg);		
    }
    else 
    {
        gHpgpHalCB.halStats.RxGoodMgmtCnt++;
    }
               
    plcRxFrmSwDesc.frmInfo.plc.clst    = pRxPktQ1stDesc->sof.clst;
    
    plcRxFrmSwDesc.frmInfo.plc.ssn = ssn;

    plcRxFrmSwDesc.cpArr[0].cp = pRxPktQCPDesc->plc.cp;
    plcRxFrmSwDesc.cpArr[0].offsetU32 =0;
    plcRxFrmSwDesc.lastDescLen  = pRxFrmDesc->lastDescLen;

    // 1.1.4 Read second and subsequent CP descriptors
    plcRxFrmSwDesc.cpCount = pRxFrmDesc->cpCount;

    for( i=1 ; i< plcRxFrmSwDesc.cpCount ; i++ )
    {
        plcRxFrmSwDesc.cpArr[i].cp = pRxFrmDesc->cpArr[i];
        plcRxFrmSwDesc.cpArr[i].offsetU32 = 0;
    }
    // 1.1.6 Call PLC Rx Callback -- tbd
	
#ifdef PLC_TEST
        cellAddr = CHAL_GetAccessToCP(plcRxFrmSwDesc.cpArr[0].cp);
        
        if((cellAddr[0] == '/') && 
            (cellAddr[1] == '?') &&
            (cellAddr[2] == '!'))//chek /?! header bytes for propritary protocol
        {
                
            //printf("\n cell add");
            //printmsg(&cellAddr[0],6);
           gHpgpHalCB.halStats.RxGoodDataCnt--;
            HHT_ProcessPlcFrame(&plcRxFrmSwDesc);
        }
       else
#endif
    
#ifdef HPGP_HAL_TEST
    if (plcRxFrmSwDesc.frmType == HPGP_HW_FRMTYPE_SOUND)
	// Sound 	pakcet process
	{ 
	     printf("free sound packet, dt_av=%X\n", plcRxFrmSwDesc.frmType);
		 CHAL_FreeFrameCp(plcRxFrmSwDesc.cpArr, plcRxFrmSwDesc.cpCount);
	}
	else
	{
    if(1 == eth_plc_bridge)
    //if((1 == eth_plc_bridge) && (plcRxFrmSwDesc.frmType != HPGP_HW_FRMTYPE_SOUND))
    { 
#ifdef ASSOC_TEST    	    
        if (plcRxFrmSwDesc.frmType == HPGP_HW_FRMTYPE_MGMT)
        {
             LM_RecvFrame(&plcRxFrmSwDesc,pRxPktQ1stDesc,
                            pRxPktQCPDesc);
        }
        else
#endif
        {
#ifdef HYBRII_ETH
//			memcpy((u8*)plcRxFrmSwDesc.fc, (u8*)pRxFrmDesc->fc, VF_SIZE);


			fwdAgent_handleData(&plcRxFrmSwDesc);
#endif  //HYBRII_ETH

        }
    }
    else
    {
        HHT_ProcessPlcMacFrame(&plcRxFrmSwDesc);
    }
	}  //End of frame type = SOUND

#else  //HPGP_HAL_TEST
    if(1 == eth_plc_bridge)
    {
#ifdef HYBRII_ETH
//        memcpy((u8*)plcRxFrmSwDesc.fc, (u8*)pRxFrmDesc->fc, VF_SIZE);


	   fwdAgent_handleData(&plcRxFrmSwDesc);
#else
       	   CHAL_FreeFrameCp(plcRxFrmSwDesc.cpArr, plcRxFrmSwDesc.cpCount);
#endif  //HYBRII_ETH
    }
    else
    {
        status = HAL_RecvFrame((sHaLayer *)cookie, &plcRxFrmSwDesc);
    }
    

#endif  //HPGP_HAL_TEST
    if (status == STATUS_FAILURE)
    {
        CHAL_FreeFrameCp(plcRxFrmSwDesc.cpArr, plcRxFrmSwDesc.cpCount);
    }
}


#ifdef HPGP_HAL_TEST
extern sHalCB gHalCB;
void HHAL_BcnRxIntHandler()
 {
        u32 xdata        rxBcnWordArr[(PLC_BCNRX_LEN>>2) + 5];  // 5 DWORD of Ether II header
                                                                   // and hostHdr_t
        u32              *prxBcnWordArr = NULL;
        u8*              rxBcnByteArr;
        u8               i, u8val;
        u8               bcnDataOffset;
        uPlcStatusReg    plcStatus;
        sHybriiRxBcnHdr* pRxBcnHdr;
#ifdef SNIFFER
        if(1 == eth_plc_sniffer)
        {
            bcnDataOffset = (sizeof(hostHdr_t) + sizeof(sEth2Hdr));
    }
    else
#endif
    {
        bcnDataOffset = 0;
    }


    plcStatus.reg = ReadU32Reg(PLC_STATUS_REG);
    rxBcnByteArr  = (u8*)rxBcnWordArr;

    prxBcnWordArr =  (u32*)(rxBcnByteArr + bcnDataOffset);
    gHpgpHalCB.halStats.BcnRxIntCnt++;
    gHpgpHalCB.halStats.smRxStuckCnt = 0;
    // Confirm that Bcn Rx Fifo is not emplty.  //[YM] We should check the BcnCnt to make sure there is a valid beacon in RxFifo
    do
    {
        // Read from fifo to local memory.
        for( i=0 ; i<(PLC_BCNRX_LEN>>2) ; i++)
        {
            prxBcnWordArr[i] = ReadU32Reg(PLC_BCNRXFIFO_REG);
        }
		// use R/W byte operation so we don't set plcTxQRdy here
		u8val = ReadU8Reg(PLC_STATUS_REG+1);
		u8val |= 0x80;		// plcBcnCntDecr (bit 15 of PLC_STATUS_REG)
    	WriteU8Reg(PLC_STATUS_REG+1, u8val);
       
        pRxBcnHdr = (sHybriiRxBcnHdr*)(rxBcnByteArr + (bcnDataOffset*4));
        // Update statistics.  //[YM] why check rsv1,2,3,4 bit fields??
        //if(pRxBcnHdr->fccsCorrect && pRxBcnHdr->pbcsCorrect && !pRxBcnHdr->rsv1 && !pRxBcnHdr->rsv2 && !pRxBcnHdr->rsv3 && !pRxBcnHdr->rsv4)  
        if(pRxBcnHdr->fccsCorrect && pRxBcnHdr->pbcsCorrect)
        {             
            gHpgpHalCB.halStats.TotalRxGoodFrmCnt++;
            gHpgpHalCB.halStats.RxGoodBcnCnt++;
            gHpgpHalCB.halStats.macRxStuckCnt = 0; 
#ifdef FREQ_DETECT
                if(gHpgpHalCB.gFreqCB.freqDetected == FALSE && (gHpgpHalCB.devMode == DEV_MODE_STA))
                {
                	sFrmCtrlBlk      *pFrmCtrlBlk = NULL;
					
                   	pFrmCtrlBlk = (sFrmCtrlBlk*) (rxBcnByteArr + sizeof(sHybriiRxBcnHdr)) ;
					
                    FREQDET_DetectFrequencyUsingBcn(pFrmCtrlBlk->snid);
                }
#endif
        }
        else
        {
           gHpgpHalCB.halStats.RxErrBcnCnt++;
           gHpgpHalCB.halStats.macRxStuckCnt++;  //[YM] It is not a right way to add macRxStuck count here. 
        }
        // Call BeaconProcess function
        if(pRxBcnHdr->fccsCorrect)
        {
            if(0 == eth_plc_sniffer)
            {
                HHAL_ProcBcnLow(rxBcnByteArr + bcnDataOffset * 4);
            }
        }
#ifdef HYBRII_ETH
        if(1 == eth_plc_sniffer)
        {
            EHT_FromPlcBcnTx((rxBcnByteArr + sizeof(sHybriiRxBcnHdr)), 
                                 (PLC_BCNRX_LEN-sizeof(sHybriiRxBcnHdr)));
        }
#endif
        // Any more beacons ?
        plcStatus.reg = ReadU32Reg(PLC_STATUS_REG);
    } while (plcStatus.s.plcBcnCnt);
}

void HAL_SetCsmaRegions(uCSMARegionReg* regionArr)
{
    uCSMARegionReg   *pCSMARgn;
    u8 i = 0;

    pCSMARgn = regionArr;
    

    while(i < HYBRII_MAXSMAREGION_CNT)
    {
        switch(i)
        {
            case 0:
                
                WriteU32Reg(PLC_CSMAREGION0_REG, (pCSMARgn->reg));
                
            break;
        
            case 1:
             WriteU32Reg(PLC_CSMAREGION1_REG, (pCSMARgn->reg));
                 //printf("\n ET2_low = %bu",  pCSMARgn->s.csma_endtime_lo);
                 //printf("\n ET2_h = %bu",  pCSMARgn->s.csma_endtime_hi);
            break;

            case 2:
                 WriteU32Reg(PLC_CSMAREGION2_REG, (pCSMARgn->reg));
            break;

            case 3:
                 WriteU32Reg(PLC_CSMAREGION3_REG, (pCSMARgn->reg));
            break;

            case 4:
                 WriteU32Reg(PLC_CSMAREGION4_REG, (pCSMARgn->reg));
            break;

            case 5:
             WriteU32Reg(PLC_CSMAREGION5_REG, (pCSMARgn->reg));
            break;
        }
        i++;
        pCSMARgn++;
    }
}

void process_region(u8 *dataptr)
{
//  sBEntry     *   pEntry;
    sRegion  *pRgn;
    uCSMARegionReg   *pCSMARgn;
    u8      i, rgn_num;
//    u32     new_csma;
    u16     start_time;
    u8      update = 0;
    sRegions    *pRgns;

//  pEntry = (sBEntry  *)dataptr;
//  rgn_num = pEntry->entry.region.nr;
//  pRgn = pEntry->entry.region.regn;
    pRgns = (sRegions *)dataptr;
    rgn_num = pRgns->nr;
    pRgn = pRgns->regn;

//printf("rgns : %bd\n", rgn_num);
    memset(&csmaRegArr[0], 0, sizeof(csmaRegArr));
    pCSMARgn = &csmaRegArr[0];

    start_time = 0;
	if (rgn_num > HYBRII_MAXSMAREGION_CNT)
		rgn_num = HYBRII_MAXSMAREGION_CNT;

    for (i=0; i < rgn_num; i++, pRgn++, pCSMARgn++)
    {
		// only set CSMA regions if they're different than previous values
		if (pRgn->reg16 != prevCsmaRgns[i])
		{
         	update = 1;
//			printf("update CSMA regions: pRgn->reg16=0x%x, prevCsmaRgns[%bu]=0x%x, rgn_num=%bu\n",pRgn->reg16, i, prevCsmaRgns[i],rgn_num);
			prevCsmaRgns[i] = pRgn->reg16;

	  		pRgn->reg16 = rtocs(pRgn->reg16);

        	pCSMARgn->s.csma_start_time_lo = 0; //start_time;
        	pCSMARgn->s.csma_start_time_hi = 0;
        	if(pRgn->s.region_type == REGION_TYPE_STAYOUT)
            	pCSMARgn->s.csma_rxOnly = 1;    //stayout region is rx only region
        	//pCSMARgn->s.bcnRegion = 0; //pRgn->s.region_type;
        	pCSMARgn->s.csma_hybrid = 1;
        	pCSMARgn->s.csma_endtime_lo = pRgn->s.region_end_time & 0x00FF;
        	pCSMARgn->s.csma_endtime_hi = ((pRgn->s.region_end_time & 0xFF00) >> 8);
        	//printf("type:%X\n", pRgn->s.region_type);
       		// printf("endHi: %bx\n", pCSMARgn->s.csma_endtime_hi);
       		// printf("endLo: %bx\n", pCSMARgn->s.csma_endtime_lo);
        	start_time = pRgn->s.region_end_time;
		}
//		else printf(" no update CSMA region %bu\n", i);
    }

    if (update != 0)
    {
        /*for (i=rgn_num; i < HYBRII_MAXSMAREGION_CNT; i++, pCSMARgn++)
        {
            pCSMARgn->reg = 0;  
        } */

      //  printf(" \n ET0_low = %bu", csmaRegArr[0].s.csma_endtime_lo);
       //  printf(" \n ET0_hi = %bu", csmaRegArr[0].s.csma_endtime_hi);
       //   printf(" \n ET1_low = %bu", csmaRegArr[1].s.csma_endtime_lo);
        // printf(" \n ET1_hi = %bu", csmaRegArr[1].s.csma_endtime_hi);
        

        HAL_SetCsmaRegions(csmaRegArr);
    }
}

#ifdef ASSOC_TEST
extern u8 gCCoMacAddr;
#endif
void HHAL_ProcBcnLow(u8* bcn)
{
    sHybriiRxBcnHdr* pRxBcnHdr;
    sFrmCtrlBlk*     pFrmCtrlBlk;
    sHybriiRxBcn*    rxBcn;      
    sBcnHdr*         pBcnHdr = 0;
    sBeHdr*          pBeHdr = 0;
    u8               hm; 
    u8               reqDiscList = 0;
    u8               nbe = 0;
    u8*              dataptr = 0;
    u8*              macAddr = 0;   
    u32              bts;

    rxBcn       = (sHybriiRxBcn*)bcn;
    pRxBcnHdr   = (sHybriiRxBcnHdr*)bcn; 
    pFrmCtrlBlk = (sFrmCtrlBlk*) (bcn + sizeof(sHybriiRxBcnHdr)) ;
    pBcnHdr     = (sBcnHdr*) ((u8*)pFrmCtrlBlk + sizeof(sFrmCtrlBlk)); 
    hm          = (pBcnHdr->nid[NID_LEN-1])&0xC0;
    //pBcnHdr->nid[NID_LEN-1] &= 0x3F;
    //pBcnHdr->nid[NID_LEN-1] &= 0xFC;                          
    nbe         = pBcnHdr->nbe;
    dataptr     = (u8*)pBcnHdr + sizeof(sBcnHdr);
    pBeHdr      = (sBeHdr*) dataptr;

    bts  =  ((u32)(pFrmCtrlBlk->bts[3])<<24) + ((u32)(pFrmCtrlBlk->bts[2])<<16) + ((u32)(pFrmCtrlBlk->bts[1])<<8) + (u32)(pFrmCtrlBlk->bts[0]); 
    memcpy(gHpgpHalCB.nid, pBcnHdr->nid, NID_LEN);

    gHpgpHalCB.nid[NID_LEN-1]  &= NID_EXTRA_BIT_MASK;
    // Extract Bto, deduce AC mode Cco's Bcn Period Len and write SWBcnPeriodAvg register.

   // HHAL_AdjustNextBcnTime(&pFrmCtrlBlk->bto[0]);
   /* if (gHpgpHalCB.lineMode == LINE_MODE_AC)
    {   
       
        if(gHpgpHalCB.devMode == DEV_MODE_STA)
        {
            WriteU32Reg( PLC_SWBCNPERAVG_REG, ctorl(PLC_AC_BP_LEN));//33.31
            //WriteU32Reg( PLC_SWBCNPERAVG_REG, ctorl(0xCB5E8));
            
        }
    }*/
   

    // Process Low MAC relevant BENTRYs
    if(gHpgpHalCB.devMode == DEV_MODE_STA &&  pRxBcnHdr->pbcsCorrect  && pRxBcnHdr->valid)
    {
        while(nbe)
        { 
            dataptr += sizeof(sBeHdr); //move to the start of BEENTRY 
            switch (pBeHdr->beType)
            {
                case BEHDR_MAC_ADDR:
                {
                    macAddr = dataptr;
#ifdef ASSOC_TEST
                    memcpy ((u8*)&gCCoMacAddr, macAddr, 6);
#endif
                    break;
                }
                case BEHDR_BPSTO:
                {        
                    
                     gsnid = pFrmCtrlBlk->snid;
                      
                    if( ((pBcnHdr->bt == BEACON_TYPE_CENTRAL) || (pBcnHdr->bt == BEACON_TYPE_PROXY))   &&
                          (pFrmCtrlBlk->snid == gHpgpHalCB.snid) && gHpgpHalCB.nwSelected )              
                     {
                        gHpgpHalCB.snapShot = pRxBcnHdr->snapShot;
                        gHpgpHalCB.bcnDetectFlag = 1;
                        HHAL_SyncNet(dataptr);
                     }
                    break;
                }
                case BEHDR_REGIONS:
                    process_region(dataptr); // WAR: Need to revisit code, deactivated to reduce loss in lower mac
                    break;

                default:
                {
                    break;
                }
            }
            //move to the next BEHDR
            dataptr = dataptr + pBeHdr->beLen; 
            pBeHdr = (sBeHdr*) dataptr;
            nbe--;
        }
        HHT_ProcessBcnHle(bcn);
    }
 
}


void HHAL_BcnRxTimeoutIntHandler()
{
//     uPlcLineControlReg plcLineCtrl;


  

   
   /* if(gtimer2 > gsyncTimeout)  //if time is greate then 40 ms
    {
        
        //gflag = 0;
       // printf("\n miss");
        misscnt++;
     
        gbpst1 += gBcnPer; 
        gtimer2 = 1;
        WriteU32Reg(PLC_BPST_REG,ctorl(gbpst1)); 
     }*/
    /* misscnt++;

     if(misscnt > 6)   //go for rescan
     {
        
        if(gHpgpHalCB.lineMode == LINE_MODE_DC )
            WriteU32Reg(PLC_CSMAREGION0_REG, ctorl(0x8F430000));
        else 
            WriteU32Reg(PLC_CSMAREGION0_REG, ctorl(0x8CB60000));
        
        gHpgpHalCB.scanEnb           = 0;
        HHAL_SetSWStatReqScanFlag(REG_FLAG_SET); //afetr this when we receive next bcn snid will be set and nw sel will be set
        gtimer2 = 0;
        misscnt =0;
        gHpgpHalCB.devMode = DEV_MODE_STA;
        HHAL_SetDevMode(gHpgpHalCB.devMode, gHpgpHalCB.lineMode); 
        HHAL_SetDefAddrConfig();
        printf("\n Rescan");
     }   */
     /*if(gtimer1 > gBcnMissingRescanCnt)
     {
        if(gHpgpHalCB.lineMode == LINE_MODE_DC )
            WriteU32Reg(PLC_CSMAREGION0_REG, ctorl(0x8F430000));
        else 
            WriteU32Reg(PLC_CSMAREGION0_REG, ctorl(0x8CB60000));
        
        gHpgpHalCB.scanEnb           = 0;
        HHAL_SetSWStatReqScanFlag(REG_FLAG_SET); //afetr this when we receive next bcn snid will be set and nw sel will be set
        gtimer2 = 0;
        gtimer1 = 0;
        gHpgpHalCB.devMode = DEV_MODE_STA;
        HHAL_SetDevMode(gHpgpHalCB.devMode, gHpgpHalCB.lineMode); 
        HHAL_SetDefAddrConfig();
        printf("\n rescan");
        
     }*/ 

    //if((gHalCB.timerIntCnt >= gHpgpHalCB.lastBcnRxTime + 13) && (gflag ==1)) //13 *4 = 52ms
   /* if((gHalCB.timerIntCnt >= gHpgpHalCB.lastBcnRxTime + 12) && (gflag ==1)) //13 *4 = 52ms
    {
       
        gflag = 0;
        //if(gtimer2 > 10)   //we miss bcn
        if(gtimer2 > 9)   //we miss bcn
        {
           gHpgpHalCB.lastBcnRxTime = (gHalCB.timerIntCnt - 4); 
           gflag = 1; 
           //gtimer2 = 0;
            printf("\n miss");
             if(gHpgpHalCB.lineMode == LINE_MODE_DC)
         gbpst1 += PLC_DC_BP_LEN;
         else
          gbpst1 += PLC_AC_BP_LEN;

       WriteU32Reg(PLC_BPST_REG,ctorl(gbpst1)); 
        }
       
    } */

}    

/*
void HHAL_BPIntHandler()
{
    uPlcMedStatReg  plcMedStat;   
    uPlcStatusReg   plcStatus;
    static u32 prevBPInt_TimerCnt;
    static u32 curBPInt_TimerCnt;
    static u8  bpInit;

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
    if( gHpgpHalCB.devMode == DEV_MODE_CCO && gHpgpHalCB.lineMode == LINE_MODE_AC )
    {
        //printf("HW Bcn PER = %lx\n",rtocl(ReadU32Reg(PLC_HWBCNPERLEN_REG)));
        gHpgpHalCB.curBcnPer = rtocl(ReadU32Reg(PLC_HWBCNPERCUR_REG));
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

        if( gHpgpHalCB.perSumCnt >= PLC_BCNPERAVG_CNT )
        {
            gHpgpHalCB.bPerAvgInitDone = 1;
        }
        if( gHpgpHalCB.bPerAvgInitDone )
        {
            gHpgpHalCB.curBcnPer       = gHpgpHalCB.bcnPerSum >> PLC_BCNPERAVG_DIVCNT;
            gHpgpHalCB.bcnPerSum      -= gHpgpHalCB.curBcnPer;
        }
        WriteU32Reg(PLC_SWBCNPERAVG_REG, ctorl(gHpgpHalCB.curBcnPer));
        //WriteU32Reg( PLC_SWBCNPERAVG_REG, ctorl(0xCB735));
        //printf("SW Bcn PER = %lx\n",gHpgpHalCB.curBcnPer);
    }

    // Prepare and send beacon here for now.
    // This will eventually be done by hpgp nsm module.
    HHT_BPIntHandler();

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
}
*/
#else  /* HPGP_HAL_TEST */

u8 HHAL_IsSnidMatched (sHaLayer *hal, uRxCpDesc *rxCpDesc)
{
    u8  snid;
    snid = rxCpDesc->plc.snidHi;
    snid = (snid << PKTQDESC1_SNIDHI_POS) | rxCpDesc->plc.snidLo;
    return (snid == hal->hhalCb->snid);
}

#if 0
eStatus HHAL_ProcRxFrameDesc(sHaLayer *hal, 
                           uRxFrmHwDesc *rxFrmHwDesc, 
                           sRxFrmSwDesc *rxFrmSwDesc)
{
    uPlcRssiLqiReg   rssiLqi;
    u8               stei;

    /*  LQI and RSSI */
    rssiLqi.reg = ReadU32Reg(PLC_RSSILQI_REG);
    rxFrmSwDesc->frmInfo.plc.rssi = rssiLqi.s.rssi;
    rxFrmSwDesc->frmInfo.plc.lqi = rssiLqi.s.lqi;

    /*  multicast and unicast */
    if(rxFrmHwDesc->plc.bcst)
    {
        rxFrmSwDesc->frmInfo.plc.mcstMode = HPGP_MNBCST;
    }
    else if(rxFrmHwDesc->plc.mcst)
    {
        rxFrmSwDesc->frmInfo.plc.mcstMode = HPGP_MCST;
    }
    else
    {
        rxFrmSwDesc->frmInfo.plc.mcstMode = HPGP_UCST;
    }

    /* source TEI */
    stei = rxFrmHwDesc->plc.steiHi2;
    stei = (stei << PKTQDESC1_STEIHI_POS ) | rxFrmHwDesc->plc.steiLo6;
    rxFrmSwDesc->frmInfo.plc.stei = stei;

    /* CLST */
    rxFrmSwDesc->frmInfo.plc.clst = rxFrmHwDesc->plc.clst;
    return STATUS_SUCCESS;

}

#endif



#endif /* HPGP_HAL_TEST */



void HAL_beaconRxHandler(void *cookie)
 {
 
#ifndef HPGP_HAL_TEST
	sHaLayer         *hal = (sHaLayer *)cookie;
	sHpgpHalCB       *hhalCb = hal->hhalCb;
    sEvent        xdata   *event = NULL;
    sHpgpHdr         *hpgpHdr = NULL;
#else	//#ifndef HPGP_HAL_TEST
	  // 5 DWORD of Ether II header
	sHpgpHalCB       *hhalCb = &gHpgpHalCB;														   // and hostHdr_t
#endif // #ifndef HPGP_HAL_TEST
    sBcnHdr        *bcnHdr = NULL;
	u32 xdata		 rxBcnWordArr[(PLC_BCNRX_LEN>>2) + 5];
	sFrmCtrlBlk      *pFrmCtrlBlk = NULL;

	u32              *prxBcnWordArr = NULL;
	u8*              rxBcnByteArr;
	u8               i, u8val;
	u32				 bts;
	u8               bcnDataOffset;
	sHybriiRxBcnHdr* pRxBcnHdr;
    
    uPlcStatusReg    plcStatus;
	
	if(1 == eth_plc_sniffer)
	{
		bcnDataOffset = (sizeof(hostHdr_t) + sizeof(sEth2Hdr));
	}
	else
	{
		bcnDataOffset = 0;
	}

	// use R/W byte operation so we don't set plcTxQRdy here
	EA = 0;
	plcStatus.reg = ReadU32Reg(PLC_STATUS_REG);
	EA = 1;
    // Confirm that Bcn Rx Fifo is not emplty.  //[YM] We should check the BcnCnt to make sure there is a valid beacon in RxFifo
    while (plcStatus.s.plcBcnCnt)
	{
        if(eth_plc_sniffer == 1)
        {
            // Size for (Ethernet header  + Hybri header) added
            //event = EVENT_Alloc(BEACON_LEN + (sizeof(sEth2Hdr) + sizeof(hostHdr_t)+4), sizeof(sHpgpHdr));
            rxBcnByteArr  = (u8*)(rxBcnWordArr);
        }
        else
        {
        
#ifndef HPGP_HAL_TEST
    
            event = EVENT_BcnAlloc(BEACON_LEN, sizeof(sHpgpHdr));
			if (event == NULL)
            {
				#ifndef RELEASE
                FM_Printf(FM_ERROR, "BErr\n");
				#endif
                return;
            }
			
    
            event->eventHdr.eventClass = EVENT_CLASS_CTRL;
            event->eventHdr.type = EVENT_TYPE_CC_BCN_IND;
            
            rxBcnByteArr = event->buffDesc.dataptr;
            
            event->buffDesc.datalen = BEACON_LEN;
#else
            rxBcnByteArr  = (u8*)(rxBcnWordArr);
    
#endif
    
        }
        
        prxBcnWordArr =  (u32*)((u8*)rxBcnByteArr + bcnDataOffset);
        
        hhalCb->halStats.smRxStuckCnt = 0;
		// Read from fifo to local memory.
		EA = 0;
		for( i=0 ; i<(PLC_BCNRX_LEN>>2) ; i++)
		{		
			prxBcnWordArr[i] = ReadU32Reg(PLC_BCNRXFIFO_REG);				
		}
        EA = 1;
        bcnHdr = (sBcnHdr *) (rxBcnByteArr + (sizeof(sFrmCtrlBlk) + sizeof(sHybriiRxBcnHdr)));
		hhalCb->halStats.BcnRxIntCnt++;            

		pRxBcnHdr = (sHybriiRxBcnHdr*)&prxBcnWordArr[0];

		hhalCb->snapShot = pRxBcnHdr->snapShot;
		
		// use R/W byte operation so we don't set plcTxQRdy here
		EA = 0;
		u8val = ReadU8Reg(PLC_STATUS_REG+1);
		u8val |= 0x80;		// plcBcnCntDecr (bit 15 of PLC_STATUS_REG)
		WriteU8Reg(PLC_STATUS_REG+1, u8val);
        
       	EA = 1;

		if(1 == eth_plc_sniffer)
		{
		
		
#ifdef HYBRII_ETH        
			EHT_FromPlcBcnTx((rxBcnByteArr + sizeof(sHybriiRxBcnHdr)), 
							 (PLC_BCNRX_LEN-sizeof(sHybriiRxBcnHdr)));
#endif
			return;

		}

#ifdef BCN_ERR
		if ((!(pRxBcnHdr->fccsCorrect && pRxBcnHdr->pbcsCorrect)) ||
		    ((bcnHdr->stei == 0) || (bcnHdr->bt >= 3))) //(bcnHdr->nbe > 4) || 
			{
				gHpgpHalCB.halStats.RxErrBcnCnt++;
				gHpgpHalCB.halStats.macRxStuckCnt++;  //[YM] It is not a right way to add macRxStuck count here. 

#ifndef HPGP_HAL_TEST
				//FM_HexDump(FM_ERROR,"BCN Rx Issue PBCS\n",(u8*)&bcnHdr,sizeof(sBcnHdr));
				EVENT_Free(event);

#endif
				return;
		
			}
				
#endif


		// Update statistics.  //[YM] why check rsv1,2,3,4 bit fields??
		//if(pRxBcnHdr->fccsCorrect && pRxBcnHdr->pbcsCorrect && !pRxBcnHdr->rsv1 && !pRxBcnHdr->rsv2 && !pRxBcnHdr->rsv3 && !pRxBcnHdr->rsv4)  
		if(pRxBcnHdr->fccsCorrect && pRxBcnHdr->pbcsCorrect)
		{             
			gHpgpHalCB.halStats.TotalRxGoodFrmCnt++;
			gHpgpHalCB.halStats.RxGoodBcnCnt++;
			gHpgpHalCB.halStats.macRxStuckCnt = 0; 
 #ifdef FREQ_DETECT
                if((gHpgpHalCB.gFreqCB.freqDetected == FALSE) &&  ((gHpgpHalCB.devMode == DEV_MODE_STA)
#ifdef MCCO
					||(gHpgpHalCB.devMode == DEV_MODE_PCCO)
#endif
						))
                {
                 	pFrmCtrlBlk = (sFrmCtrlBlk*) (rxBcnByteArr + sizeof(sHybriiRxBcnHdr)) ;
                    FREQDET_DetectFrequencyUsingBcn(pFrmCtrlBlk->snid);// Kiran disabled to debug 60HZ at STA
//                    printf("\n using bcn \n");
                }
#endif

		}


		pFrmCtrlBlk = (sFrmCtrlBlk*) (rxBcnByteArr + sizeof(sHybriiRxBcnHdr)) ;
		
		bts  =  ((u32)(pFrmCtrlBlk->bts[3])<<24) +
				((u32)(pFrmCtrlBlk->bts[2])<<16) + 
				((u32)(pFrmCtrlBlk->bts[1])<<8) + 
				 (u32)(pFrmCtrlBlk->bts[0]);

#ifdef PSDEBUG
						if (psDebug2)
						{
//							if (staInfo->staScb->bpCnt && !(staInfo->staScb->bpCnt % staInfo->staScb->commAwd.numBp))
								printf("RX a beacon at NTB=%lu\n", (rtocl(ReadU32Reg(PLC_NTB_REG))*40)/1000000);
						}
#endif		
#ifdef ROUTE_TEST

        for(i=0; i < 3; i++)
        {
            if(bcnHdr->stei == dropTei[i])
            {
#ifndef HPGP_HAL_TEST                
                EVENT_Free(event);                
#endif
                return;
            }
        }
		
#endif

#ifdef HPGP_HAL_TEST
		
        HHAL_ProcBcnLow(rxBcnByteArr);
#else

		hpgpHdr = (sHpgpHdr *)event->buffDesc.buff;
		hpgpHdr->snid = pFrmCtrlBlk->snid;

		event->buffDesc.dataptr += (sizeof(sFrmCtrlBlk) + sizeof(sHybriiRxBcnHdr));
		event->buffDesc.datalen -= (sizeof(sFrmCtrlBlk) + sizeof(sHybriiRxBcnHdr)); 
#ifdef BCN_ERR
		bpstoFound = FALSE;
#endif
		if ((hhalCb->devMode == DEV_MODE_CCO) && (dropFlag == 0))
		{
#ifdef CCO_FUNC
			//LINKL_CcoProcBcnHandler(hal->bcnCookie, event, bts);
			sLinkLayer	   *linkl = (sLinkLayer *)hal->bcnCookie;
   			sCnsm		  *cnsm = (sCnsm *)LINKL_GetCnsm(linkl);
   			CNSM_ProcBcnHigh(cnsm, event->buffDesc.dataptr, bts);
#endif
		}
#ifdef MCCO		
		else 
		if (hhalCb->devMode == DEV_MODE_PCCO)
		{
		    LINKL_PassiveCcoProcBcnHandler(hal->bcnCookie, event, bts);
		}
#endif		
		else if(dropFlag == 0)
		{
#ifdef STA_FUNC
		//LINKL_StaProcBcnHandler(hal->bcnCookie, event, bts);
			{	
				sLinkLayer     *linkl = (sLinkLayer *)hal->bcnCookie;
			    sSnsm*         snsm = (sSnsm *)LINKL_GetSnsm(linkl);
				SNSM_BcnCheck(event->buffDesc.dataptr);
#ifdef BCN_ERR
				if(bpstoFound == TRUE)
#endif
				{
				    SNSM_ProcBcnHigh(snsm, event->buffDesc.dataptr, hpgpHdr->snid, bts);

				    if(snsm->txDiscBcn)
				    {
				        //SNSM_TransmitDiscBcn(snsm);
						
					    sTxDesc         txinfo;
					    sBuffDesc       buffDesc;
					    u8              offset = 0; 

//					    FM_Printf(FM_HINFO, "SNSM:>>DISC BCN\n");

#ifdef SIMU
					    offset = sizeof(sFrmCtrlBlk) + sizeof(sTxDesc);
#else
					    offset = sizeof(sFrmCtrlBlk);
#endif

					    //transmit the beacon 
					    txinfo.dtei = 0xFF;
					    txinfo.stei = snsm->staInfo->staScb->tei;
					    txinfo.frameType = BEACON_TYPE_DISCOVER;
					    txinfo.snid = snsm->staInfo->snid;
					  
					    //prepare tx control information
					    buffDesc.buff = snsm->discBcnBuff;
					    buffDesc.bufflen = BEACON_BUFF_LEN;
					    buffDesc.dataptr = snsm->discBcnBuff + offset;
					    buffDesc.datalen = BEACON_PAYLOAD_SIZE;

					    //FM_HexDump(FM_DATA|FM_MINFO, "SNSM: discovery beacon:", 
					    //                             buffDesc.dataptr,
					    //                             buffDesc.datalen);

					    //HAL_TransmitBeacon(HOMEPLUG_GetHal(), &txinfo, &buffDesc, snsm->bpstoOffset);
					    HAL_TransmitBeacon(&HomePlug.haLayer, &txinfo, &buffDesc, snsm->bpstoOffset);
							
				        snsm->txDiscBcn = FALSE;
				    }

				}
#ifdef BCN_ERR
				else
				{
					//FM_Printf(FM_ERROR,"\nBL2\n");
				}
#endif				
			}
#endif
		}
#ifdef BCN_ERR
        if(bpstoFound == TRUE && dropFlag == 0)
#endif
        {
    		//MUXL_RecvMgmtPacket(hal->mgmtCookie, event);
			{
			    sMuxLayer *muxl = (sMuxLayer *)hal->mgmtCookie;
			    /* check beacon */
			    if ((event->eventHdr.eventClass == EVENT_CLASS_CTRL)&&
			        (event->eventHdr.type == EVENT_TYPE_CC_BCN_IND))
			    {
#ifdef MUX_PRINT    
			        FM_Printf(FM_MUX, "MUX: deliver a beacon\n");
#endif
#ifdef CALLBACK
			        muxl->mux.deliverMgmtMsg(muxl->mux.mgmtcookie, event); 
#else
			        //LINKL_RecvMgmtMsg(muxl->mux.mgmtcookie, event);
					{
					    sLinkLayer *linkl = (sLinkLayer *)muxl->mux.mgmtcookie;

#ifdef P8051
__CRIT_SECTION_BEGIN__
#else
					    SEM_WAIT(&linkl->linkSem);
#endif

					    //place the event to the queue
					    SLIST_Put(&linkl->eventQueue, &event->link);

#ifdef P8051
__CRIT_SECTION_END__
#else
					    SEM_POST(&linkl->linkSem);
#endif
					    /* schedule the task */
#ifndef RTX51_TINY_OS
					    SCHED_Sched(&linkl->task);
#else
						os_set_ready(HPGP_TASK_ID_CTRL);
#endif
					}
#endif
			
			    }
			    else if (event->eventHdr.eventClass == EVENT_CLASS_MSG)
			    {
					MUX_Proc(&muxl->mux, event);
					
			    }
			}
        }
#ifdef BCN_ERR
        else
        {
		//	FM_HexDump(FM_ERROR,"BCN Rx Issue BPSTO\n",(u8*)&bcnHdr,sizeof(sBcnHdr));
          //  FM_Printf(FM_ERROR, "BL1\n");
            EVENT_Free(event);
        }
#endif


#endif

		

		EA = 0;
		plcStatus.reg = ReadU32Reg(PLC_STATUS_REG);
		EA = 1;
	}

}


