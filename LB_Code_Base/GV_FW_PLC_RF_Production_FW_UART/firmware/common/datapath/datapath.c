#ifdef RTX51_TINY_OS
#include <rtx51tny.h>
#endif
#include <stdio.h>
#include <string.h>
#include "fm.h"
#include <intrins.h>
#include "papdef.h"
#ifdef ROUTE
#include "hpgp_route.h"
#endif
#include "fm.h"
#include "hal_common.h"
#include "hal.h"
#include "hal_eth.h"
#include "hal_tst.h"
#include "hal_cfg.h"
#include "hal_spi.h"
#include "hpgpdef.h"
#include "datapath.h"
#include "papdef.h"
#ifdef UM
#include "ctrll.h"
#include "linkl.h"
#include "timer.h"
#include "stm.h"
#endif
#include "nma.h"
#include "nma_fw.h"
#include "hpgpapi.h"
#include "frametask.h"
#include "hybrii_tasks.h"
#include "gv701x_gpiodriver.h"
#include "event_fw.h"
#ifdef NO_HOST
#include "gv701x_osal.h"
#endif

#ifdef NO_HOST
extern gv701x_app_msg_hdr_t msg_hdr_app_cmd;
#endif
#ifdef NO_HOST
#ifdef HYBRII_802154
extern mac_host_db_t mac_host_db;
#endif
#endif
//#define printf(x)
//#define FM_Printf(x, z)

sConnState  ConnState[MAX_NUM_STATIONS];

#if defined(POWERSAVE) || defined(LLP_POWERSAVE)
extern u32 psNoTxFrmCnt;
extern u32 psTxFrmCnt;
extern u32 psPlcTxWriteErrCnt;
extern u32 psPlcIdleErrCnt;
extern u32 psPlcTxOKCnt;
extern u32 psPclTxWriteFromBcn;
extern u32 psPclTxWriteFromFrame;
extern u32 psFrmBcnNoTxFrmCnt;
extern u32 psFrmBcnTxFrmCnt;
extern u32 psFrmBcnPlcTxWriteErrCnt;
extern u32 psFrmBcnPlcIdleErrCnt;
extern u32 psFrmBcnPlcTxOKCnt;
extern u32 psNoTxWrongBpFrmCnt;
extern u32 psFrmBcnNoTxWrongBpFrmCnt;
extern u32 psNoTxZeroAwdFrmCnt;
extern u32 psFrmBcnNoTxZeroAwdFrmCnt;
extern u8 psDebug;
#endif
#ifdef LLP_POWERSAVE
extern u8 psDebug1;
#endif
#include "utils_fw.h"
#ifdef UART_HOST_INTF
#include "gv701x_uartdriver_fw.h"
#endif
#ifdef HYBRII_HPGP
#include "hpgpapi.h"
#endif

volatile dqueue_t gDqueue[MAX_DATA_QUEUES];
extern u8 spi_payload_rx_pending;
extern u8 gEthMacAddrDef[];

extern u8 eth_plc_bridge;

volatile u8 host_intf_max_cp =0;
volatile u8 numHostCPs = 0;
static u32 plcTxTime = 0;
							   
volatile u8  gNekEks = HPGP_UNENCRYPTED_EKS;

#ifdef DEBUG_DATAPATH
extern u8 sigDbg;
extern u8 pktDbg;
extern u8 ethQueueDebug;

#endif

#ifdef FREQ_DETECT
extern u32 PLC_MIN_AC_BPLEN;
extern u32 PLC_AC_BP_LEN; 
extern u32 PLC_MAX_AC_BPLEN; 
#endif
#ifdef ETH_BRDG_DEBUG
extern u8 myDebugFlag;

extern u32 ethTxFrameCnt;
extern u32 numEthTxCp;
extern u32 plcRxFrameCnt;
extern u8 myDebugFlag1;
extern u32 numPlcPendingRet;
extern u32 numForcePlcTxDone; 
extern u32 numEthTxDoneInts;
extern u32 numEthTxCpReleased;
extern u32 numPlcTxCp;
extern u32 plcTxWriteFail;
extern u32 plcTxFrameCnt;

extern u32 ethRxFrameCnt;
extern u32 numTxDoneInts;



extern u32 ethTxWriteFail;
#endif

#ifdef SPI_DEBUG
extern u8 mySpiDebugFlag;
extern hal_spi_stats_t hal_spi_stats;
#endif

#ifdef UM


static void Host_MgmtCmdRxHandler(sCommonRxFrmSwDesc* pRxFrmDesc,
									  u16 frmLen, u8 frmType);

#endif
#ifdef NO_HOST
eStatus Aps_PostDataToQueue(u8 src_port, sSwFrmDesc* plcRxFrmSwDesc);
#endif

#ifdef UM
#ifdef PROXY_BCST

volatile u8 proxyBcst = 0;

void GV701x_EnableProxyBcst(u8 enable)
{
	proxyBcst = enable;
}

sScb *getLeastRssiScb()
{
	sLinkLayer *linkl = (sLinkLayer *)HPGPCTRL_GetLayer(HP_LAYER_TYPE_LINK);
	sScb *minRssiScb = NULL;
	sScb *scbIter =  NULL;
	u8 minRssi = 200;

	scbIter = CRM_GetNextScb(&linkl->ccoRm, scbIter);
    while(scbIter)
	{
		if (scbIter != linkl->staInfo.staScb)
		{
			if (scbIter->rssiLqi.s.rssi < minRssi)
			{
				minRssiScb = scbIter;	
				minRssi = scbIter->rssiLqi.s.rssi;
			}
		}
		scbIter = CRM_GetNextScb(&linkl->ccoRm, scbIter);
	
	}
	//printf("Least TEI RSSI %bu  %bu\n",minRssiScb->rssiLqi.s.rssi,minRssiScb->tei);
	return minRssiScb;
}
#endif // PROXY_BCST
#endif
void datapath_writeHostIntf(sSwFrmDesc *hostTxFrmSwDesc)
{

	u16				crc16 = 0;
	
	eStatus			status;

	u8			   firstCp = 0;



	if(hostTxFrmSwDesc->txPort == PORT_ETH)
	{
#ifdef HYBRII_ETH            
		status = EHAL_EthTxQWrite(hostTxFrmSwDesc);
#endif //HYBRII_ETH
		if(status != STATUS_SUCCESS)
		{
#ifdef ETH_BRDG_DEBUG
			if (myDebugFlag1)
				printf("EHT_SendToHost: EHAL_EthTxQWrite returned FAIL\n");
			ethTxWriteFail++;
		    FM_Printf(FM_ERROR, "Tx to Eth Fail\n");
#endif		
            gHpgpHalCB.halStats.PtoHswDropCnt++;
		    // free CPs 
		    CHAL_FreeFrameCp(hostTxFrmSwDesc->cpArr, hostTxFrmSwDesc->cpCount);
		} 
		numHostCPs -= hostTxFrmSwDesc->cpCount;
	}
#ifdef HYBRII_SPI
	else if(hostTxFrmSwDesc->txPort == PORT_SPI)
	{       

	
		// FM_Printf(FM_USER, "s t\n");
		status = hal_spi_tx_dma_cp(hostTxFrmSwDesc->frmLen, 
								   hostTxFrmSwDesc);

		if(status != STATUS_SUCCESS)
		{
		  ///  FM_Printf(FM_ERROR, "Tx to SPI Failed\n");
#ifdef SPI_DEBUG
		    hal_spi_stats.tx_return_err++;
			if (mySpiDebugFlag)
			{
		       	printf("Tx to SPI Fail\n"); // TODO need to take action if tx failed
			}
#endif
            gHpgpHalCB.halStats.PtoHswDropCnt++;
		    // Free CPs
		    CHAL_FreeFrameCp(hostTxFrmSwDesc->cpArr, hostTxFrmSwDesc->cpCount);
		}

		numHostCPs -= hostTxFrmSwDesc->cpCount;    
		  
	}
#endif  //HYBRII_SPI
#ifdef UART_HOST_INTF
	else if(hostTxFrmSwDesc->txPort == PORT_UART)
	{
		if(hostTxFrmSwDesc->frmLen != 0)
		{					
			status = hal_uart_tx_cp (hostTxFrmSwDesc);
			if(status != STATUS_SUCCESS)
			{

			    CHAL_FreeFrameCp(hostTxFrmSwDesc->cpArr, hostTxFrmSwDesc->cpCount);
			}
			numHostCPs -= hostTxFrmSwDesc->cpCount;
		}
	}

#endif //UART_HOST_INTF
#ifdef NO_HOST
    else if(hostTxFrmSwDesc->txPort == PORT_APP)
	{
        if(hostTxFrmSwDesc->frmLen != 0)
		{			
			Aps_PostDataToQueue(hostTxFrmSwDesc->rxPort, hostTxFrmSwDesc);
            numHostCPs -= hostTxFrmSwDesc->cpCount;
		}
    }
#endif


}



void datapath_handlePlcTxDone()
{

	u8 tailIdx;
	sSwFrmDesc *lpPlcTxFrmSwDesc;
#ifdef UART_HOST_INTF
	EA = 0;
#endif
	
#ifdef ETH_BRDG_DEBUG
		numTxDoneInts++;
#endif
		   				
		tailIdx = gDqueue[PLC_DATA_QUEUE].tail & 0x7F;

		lpPlcTxFrmSwDesc =	&gDqueue[PLC_DATA_QUEUE].desc[tailIdx];

		if (lpPlcTxFrmSwDesc->frmInfo.plc.status == PLC_TX_PENDING)
		{
			lpPlcTxFrmSwDesc->frmInfo.plc.status = PLC_TX_DONE; 		   

			

#ifdef DEBUG_DATAPATH                        
			if (sigDbg)
				  FM_Printf(FM_ERROR," plc txDone t:%bu\n",gDqueue[PLC_DATA_QUEUE].tail);
#endif   //DEBUG_DATAPATH         
		}

#ifdef UART_HOST_INTF
	EA = 1;
#endif

}
void datapath_init()
{
	memset((u8*)&gDqueue, 0, sizeof(gDqueue));
}

bool datapath_IsQueueFull(queue_id_e id)
{
	u8 head = gDqueue[id].head;
	u8 tail = gDqueue[id].tail;
#ifdef UART_HOST_INTF
	EA = 0;
#endif
	if ((head & 0x7F) != (tail  & 0x7F)||
			   ((head & 0x80) != (tail  & 0x80)))
	{

		// check if pending queue is full. if yes drop the frame or if not-full queue the frame

		if (((head & 0x80) != (tail  & 0x80)) &&
			((head & 0x7F) == (tail  & 0x7F)))
		{

#ifdef DEBUG_DATAPATH
			if (sigDbg)
			{
				FM_Printf(FM_ERROR,"q id %bu full\n", id);
			}

#endif
#ifdef UART_HOST_INTF
			EA = 1;
#endif
			return TRUE;


		}

	}
#ifdef UART_HOST_INTF
	EA = 1;
#endif
	  return FALSE;

}

extern u8 CHAL_GetFreeCPCnt() __REENTRANT__;

void datapath_queue_depth(queue_id_e id)	 
{
	u8 head = gDqueue[id].head;
	u8 tail = gDqueue[id].tail;
#ifndef MPER	
	printf ("h %bu \n ", head);
	
	printf ("t %bu \n ", tail);
	
    printf("fr = %bu\n",CHAL_GetFreeCPCnt());   
#endif	
}


void datapath_queue(queue_id_e id,
						   sSwFrmDesc *pPlcTxFrmSwDesc)
{
	u8 wrapBit;
    u8 indexHd;
	sSwFrmDesc *swDesc;

#ifdef UART_HOST_INTF
	EA = 0;
#endif
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

#ifdef UART_HOST_INTF
	EA = 1;
#endif
}


sSwFrmDesc *datapath_getHeadDesc(queue_id_e id, u8 pop)
{
	u8 wrapBit;
	u8 tailIdx;
	sSwFrmDesc  *lpPlcTxFrmSwDesc;
#ifdef UART_HOST_INTF
	EA = 0;
#endif
	tailIdx = (gDqueue[id].tail & 0x7F);
	lpPlcTxFrmSwDesc =  &gDqueue[id].desc[tailIdx];


	if (pop)
	{

		wrapBit = gDqueue[id].tail & 0x80;
		gDqueue[id].tail = ((gDqueue[id].tail & 0x7F) + 1) | wrapBit;

		if (id == PLC_DATA_QUEUE)
		{

			host_intf_max_cp -= lpPlcTxFrmSwDesc->cpCount;

		}
		if ((gDqueue[id].tail & 0x7F) == MAX_Q_BUFFER)
		{
			gDqueue[id].tail ^= 0x80;		  //inverse wraparound bi
			gDqueue[id].tail &= 0x80;

		}



	}
#ifdef UART_HOST_INTF
	EA = 1;
#endif
	return lpPlcTxFrmSwDesc;
}

bool datapath_IsQueueEmpty(queue_id_e id)
{

	u8 head = gDqueue[id].head;
	u8 tail = gDqueue[id].tail;
#ifdef UART_HOST_INTF
	EA = 0;
#endif

	if (((head & 0x7F) != (tail & 0x7F)||
		 ((head & 0x80) != (tail & 0x80))))
	{
#ifdef UART_HOST_INTF
		EA = 1;
#endif
		return FALSE;
	}
#ifdef UART_HOST_INTF
	EA = 1;
#endif
	return TRUE;
}

void datapath_resetQueue(queue_id_e id)
{
	memset((u8*)&gDqueue[id], 0x00, sizeof(dqueue_t));
	if (id  == PLC_DATA_QUEUE)
	{
		host_intf_max_cp = 0;
	}

	if (id  == HOST_DATA_QUEUE)
	{
		numHostCPs = 0;
	}
}


void EHT_SetFrameTypeLen (u8* rxBcnByteArr, u8 frmType, u16 frmLen)
{
    sMfHdr *mh_hdr;
    mh_hdr = (sMfHdr *)rxBcnByteArr;
    switch (frmType)
    {
       //MFT and MFL is not presend in Beacon and sound frames
        case HPGP_HW_FRMTYPE_MGMT:  // SOF frame
#ifdef UM
            memset(rxBcnByteArr, 0, (sizeof(sMfHdr) + CONFOUNDER_SIZE) );
#endif

            // Beacon should have different type
            // SOF frame len + confounder
            frmLen += CONFOUNDER_SIZE;
            mh_hdr->mft = HPGP_HW_FRMTYPE_MGMT;
            mh_hdr->mflHi = (frmLen >> 6) & 0xFF;
            mh_hdr->mflLo =  (frmLen & 0x3F);

            break;
        case HPGP_HW_FRMTYPE_MSDU: // Data frame
            memset(rxBcnByteArr, 0, sizeof(sMfHdr));
            mh_hdr->mft = HPGP_HW_FRMTYPE_MSDU;
            mh_hdr->mflHi = (frmLen >> 6) & 0xFF;
            mh_hdr->mflLo = frmLen & 0x3F;
            break;
        default:
            return;
    }


}

void EHT_FillEtherHeader (u8* rxBcnByteArr)
{
    sEth2Hdr*  pEth2Hdr;

    pEth2Hdr = (sEth2Hdr*)rxBcnByteArr;
    pEth2Hdr->ethtype = 0x88E1;
    memset(pEth2Hdr->dstaddr, 0xFF, MAC_ADDR_LEN);
    memcpy(pEth2Hdr->srcaddr, gEthMacAddrDef, MAC_ADDR_LEN);
}
#if defined(UM) || defined(HPGP_HAL_TEST)
u8 ETH_FillHybriiHeader(u8 *rxArr,
                        sSwFrmDesc *pPlcRxFrmSwDesc,
                        u16 frameSize,
                        u8 offset)
{
    u8 addLen = 0;
    hostHdr_t *hostHdr;

    memset(rxArr, 0, sizeof(hostHdr_t));
    addLen = sizeof(hostHdr_t);
    hostHdr = (hostHdr_t *)rxArr;
    if(pPlcRxFrmSwDesc != NULL)
    {
        hostHdr->length     = frameSize;
        switch (pPlcRxFrmSwDesc->frmType)
        {
            case HPGP_HW_FRMTYPE_SOUND:

                hostHdr->rsvd       = offset;
                break;
            case HPGP_HW_FRMTYPE_MGMT:

                hostHdr->type       = MGMT_FRM_ID;
                hostHdr->protocol   = HPGP_MAC_ID;
                //hostHdr->rsvd       = ((u16)offset << 8);

                break;

            case HPGP_HW_FRMTYPE_MSDU:
                hostHdr->type       = DATA_FRM_ID;
                hostHdr->protocol   = HPGP_MAC_ID;
               // hostHdr->rsvd       = ((u16)offset << 8);
                break;
           default:
                hostHdr->type       = 0x03;
                hostHdr->protocol   = HPGP_MAC_ID;
            //    hostHdr->rsvd       = ((u16)offset << 8);
        }
    }
    else
    {
        hostHdr->type       = MGMT_FRM_ID;
        hostHdr->protocol   = HPGP_MAC_ID;
     //   hostHdr->rsvd       = ((u16)offset << 8);
    }
    return (addLen);
}
#endif
#ifdef HYBRII_ETH
void EHT_FromPlcBcnTx (u8* rxBcnByteArr, u16 frameSize)
{
    u16              dataIdx;
    u16              curFrmLen;
    u8               cpIdx;
    u8               len;
    eStatus          status;
    sSwFrmDesc      ethTxFrmSwDesc;

    if (frameSize == 0)
    {
        return;
    }

    EHT_FillEtherHeader(rxBcnByteArr);
    len = sizeof(sEth2Hdr);
    ETH_FillHybriiHeader(&rxBcnByteArr[len],
                        NULL,
                        frameSize,
                        0);
    dataIdx                = 0;
    cpIdx                  = 0;
    // Smiffer expected 14 bytes ethernet header and 6 bytes Hybrii header
    curFrmLen              = frameSize + sizeof(sEth2Hdr)+ sizeof(hostHdr_t);  

    ethTxFrmSwDesc.frmLen  = curFrmLen;
    ethTxFrmSwDesc.cpCount = 0;

    while (curFrmLen)
    {
        u8                  cp;
        volatile u8 xdata * cellAddr;
        u8                  actualDescLen;

        status = CHAL_RequestCP(&cp);
        if (status != STATUS_SUCCESS)
        {
            return;
        }
        cellAddr = CHAL_GetAccessToCP(cp);
        if (curFrmLen > HYBRII_CELLBUF_SIZE)
        {
            actualDescLen = HYBRII_CELLBUF_SIZE;
        }
        else
        {
            actualDescLen = curFrmLen;
        }
        memcpy(cellAddr, &rxBcnByteArr[dataIdx], actualDescLen);
        dataIdx += actualDescLen;
        ethTxFrmSwDesc.cpArr[cpIdx].offsetU32 = 0;
        ethTxFrmSwDesc.cpArr[cpIdx].len  = actualDescLen;
        ethTxFrmSwDesc.cpArr[cpIdx].cp = cp;
        cpIdx++;
        ethTxFrmSwDesc.cpCount++;
        curFrmLen -= actualDescLen;
    }
    status = EHAL_EthTxQWrite(&ethTxFrmSwDesc);
    if (status == STATUS_FAILURE)
    {
        for (cpIdx = 0; cpIdx < ethTxFrmSwDesc.cpCount; cpIdx++)
        {
            CHAL_DecrementReleaseCPCnt(ethTxFrmSwDesc.cpArr[cpIdx].cp);
        }
        //printf("\nCannot send Eth packet");
    }
    else
    {
        gEthHalCB.CurTxTestFrmCnt++;
        gEthHalCB.CurTxTestBytesCnt+= frameSize;
    }
}
#endif
u8 EHT_GetSniffHdrSize (eHpgpHwFrmType frmType)
{
    u8 addLen = sizeof(sEth2Hdr);  // Ether header is common

    switch (frmType)
    {
        case HPGP_HW_FRMTYPE_SOUND:
            addLen += (VF_SIZE);
            break;
        case HPGP_HW_FRMTYPE_MGMT:
            addLen += (VF_SIZE  + sizeof(sMfHdr) + CONFOUNDER_SIZE);
            break;
        case HPGP_HW_FRMTYPE_MSDU:
            addLen += (VF_SIZE + sizeof(sMfHdr));
			break;
        default:
            break;
    }
//#ifdef HPGP_MAC_SAP
    addLen += sizeof(hostHdr_t); // Len for hybrii header
//#endif


    return (addLen);
}
#if 0
u8 EHT_FillVariantField (u8*            rxBcnByteArr,
                         sSwFrmDesc*  pPlcRxFrmSwDesc,
                         uRxFrmHwDesc*  pRxPktQ1stDesc,
                         uRxCpDesc*     pRxPktQCPDesc)
{
    u8 addLen = 0;

    switch (pPlcRxFrmSwDesc->frmType)
    {
        case HPGP_HW_FRMTYPE_SOUND:
            memset(rxBcnByteArr, 0, 16);
            addLen = 16;
            rxBcnByteArr[0]  = 0x04 | (pPlcRxFrmSwDesc->frmInfo.plc.snid << 4); // dt_av = 0b0 100
            rxBcnByteArr[1]  = pPlcRxFrmSwDesc->frmInfo.plc.stei;
            rxBcnByteArr[4]  = pRxPktQ1stDesc->sound.saf << 3 |
                               pRxPktQ1stDesc->sound.scf << 4;
            rxBcnByteArr[4]  = 0x3E;
            rxBcnByteArr[8] = pRxPktQ1stDesc->sound.srcHi << 6 |
                               pRxPktQ1stDesc->sound.srcLo;
            break;
        case HPGP_HW_FRMTYPE_MGMT:
            memset(rxBcnByteArr, 0, 16);
            addLen = 16;
            rxBcnByteArr[0]  = 0x01 | (pPlcRxFrmSwDesc->frmInfo.plc.snid << 4); // dt_av = 0b0 100
            rxBcnByteArr[1]  = pPlcRxFrmSwDesc->frmInfo.plc.stei;
            rxBcnByteArr[4]  = pRxPktQCPDesc->plc.eks << 4;
            rxBcnByteArr[8]  = 0x3E;
            rxBcnByteArr[10] = pRxPktQ1stDesc->sof.mcst << 6;
            rxBcnByteArr[11] = pRxPktQ1stDesc->sof.clst << 1;
            break;
        case HPGP_HW_FRMTYPE_MSDU:
            memset(rxBcnByteArr, 0, 16);
            addLen = 16;
            rxBcnByteArr[0]  = 0x01 | (pPlcRxFrmSwDesc->frmInfo.plc.snid << 4); // dt_av = 0b0 100
            rxBcnByteArr[1]  = pPlcRxFrmSwDesc->frmInfo.plc.stei;
            rxBcnByteArr[4]  = pRxPktQCPDesc->plc.eks << 4;
            rxBcnByteArr[8]  = 0x3E;
            rxBcnByteArr[10] = pRxPktQ1stDesc->sof.mcst << 6;
            rxBcnByteArr[11] = pRxPktQ1stDesc->sof.clst << 1;
            break;
        default:
            break;
    }

    return (addLen);
}
#endif

eStatus datapath_queueToHost (sSwFrmDesc*  pPlcRxFrmSwDesc,
                              u16            frameSize)
{
    u8                i,rsvd = 0;
#ifndef HYBRII_B
	u8				  j;
#endif
    eStatus           status;
    u8                eth_hdr_cp = 0;
    u8 xdata          *cellAddr;  
 
    sSwFrmDesc        hostTxFrmSwDesc;
    u8                actualDescLen;
    u8                headerStart;
    
#ifdef SW_RETRY
    u16                pad = 0;
#endif  //SW_RETRY
    u8                addFrameSize;
    u16               curFrmLen;
    u8                offsetAdj = 0; // 0 to 3 -  if hdrOffset is not align to 4 bytes then set
    u8 ptr;
                                    // offsetAdj to align hdrOffset

    u8 HybriLen;

    u16               crc16 = 0;
    u8                alin128 = 0;
#ifdef MEM_PROTECTION
	u8 				  cp_localBuf[HYBRII_CELLBUF_SIZE];	// local CP buffer
#endif

    status = STATUS_FAILURE;
#ifdef UM
    if((numHostCPs + pPlcRxFrmSwDesc->cpCount) >= PLC_TO_HOST_MAX_CP)
    {
        // Return fail
        FM_Printf(FM_ERROR, "H CP lim\n");
		gHpgpHalCB.halStats.PtoHswDropCnt++;
     	return (status);
    }
	numHostCPs += pPlcRxFrmSwDesc->cpCount;

#endif  //HPGP_MAC_SAP        
    memset(&hostTxFrmSwDesc, 0, sizeof(sSwFrmDesc));

    if (frameSize == 0 || hostIntf == HOST_INTF_NO)
    {
        return (status);
    }
    if (pPlcRxFrmSwDesc->frmType == HPGP_HW_FRMTYPE_MGMT)
    {
       
        pPlcRxFrmSwDesc->cpArr[0].offsetU32  = 1;

        
        frameSize -= 4; // offset in first CP.

    }

    if(1 == eth_plc_sniffer)
    {
        status = CHAL_RequestCP(&eth_hdr_cp);
        if (status != STATUS_SUCCESS)
        {
            //printf("\nFailed to allocate CP");     
#ifdef UM
            numHostCPs -= pPlcRxFrmSwDesc->cpCount;
#endif  //HPGP_MAC_SAP
            return (status);
        }
#ifdef UM
        numHostCPs += 1;
#endif  //HPGP_MAC_SAP
#ifdef MEM_PROTECTION
		if (HHAL_CP_Get_Copy(eth_hdr_cp, &cp_localBuf[0], HYBRII_CELLBUF_SIZE) == STATUS_FAILURE) 
		{
			printf("datapath_queueToHost: Failed to make a copy of CP %bu. Return\n", eth_hdr_cp);
		    return (STATUS_FAILURE);
		}
        cellAddr = &cp_localBuf[0];
#else
        cellAddr = CHAL_GetAccessToCP(eth_hdr_cp);
#endif
        addFrameSize = EHT_GetSniffHdrSize(pPlcRxFrmSwDesc->frmType);

        headerStart = (HYBRII_CELLBUF_SIZE - addFrameSize);

        offsetAdj = headerStart % sizeof(u32);

        addFrameSize += offsetAdj;

        headerStart -= offsetAdj;

        ptr = headerStart;


        EHT_FillEtherHeader(&cellAddr[ptr]);


        ptr += sizeof(sEth2Hdr);
        HybriLen = (frameSize + addFrameSize -
                    (sizeof(sEth2Hdr) + sizeof(hostHdr_t)));



        ETH_FillHybriiHeader(&cellAddr[ptr],
                                pPlcRxFrmSwDesc,
                                HybriLen,
                                offsetAdj);


        ptr += (sizeof(hostHdr_t) + offsetAdj);

/*
        EHT_FillVariantField(&cellAddr[ptr],
                                pPlcRxFrmSwDesc,
                                pRxPktQ1stDesc,
                                pRxPktQCPDesc);
*/
//        memcpy(&cellAddr[ptr], (u8*)pPlcRxFrmSwDesc->fc, VF_SIZE);

        ptr += VF_SIZE;

        EHT_SetFrameTypeLen(&cellAddr[ptr],
                                pPlcRxFrmSwDesc->frmType,
                                frameSize);

        if (pPlcRxFrmSwDesc->frmType == HPGP_HW_FRMTYPE_MGMT)
        {

            ptr += sizeof(sMfHdr) + CONFOUNDER_SIZE;

        }
        else
        {
            ptr += sizeof(sMfHdr);

        }
#ifdef MEM_PROTECTION
        if(HHAL_CP_Put_Copy(eth_hdr_cp, cp_localBuf, HYBRII_CELLBUF_SIZE) == STATUS_FAILURE)
        {
    	    printf("datapath_queueToHost: Failed to make a copy of CP %bu. Return\n", eth_hdr_cp);
            return (STATUS_FAILURE);
        }
#endif
#ifdef UM
#if 0
        hostTxFrmSwDesc->frmLen             = frameSize + addFrameSize;
        hostTxFrmSwDesc->cpArr[0].cp        = eth_hdr_cp;
        hostTxFrmSwDesc->cpArr[0].len       = addFrameSize;
        hostTxFrmSwDesc->cpArr[0].offsetU32 = headerStart / sizeof(u32);
        hostTxFrmSwDesc->cpCount            = 1;

#else
hostTxFrmSwDesc.frmLen             = frameSize + addFrameSize;
       hostTxFrmSwDesc.cpArr[0].cp        = eth_hdr_cp;
       hostTxFrmSwDesc.cpArr[0].len       = addFrameSize;
       hostTxFrmSwDesc.cpArr[0].offsetU32 = headerStart / sizeof(u32);
       hostTxFrmSwDesc.cpCount            = 1;


#endif // 0

#else
        hostTxFrmSwDesc.frmLen             = frameSize + addFrameSize;
        hostTxFrmSwDesc.cpArr[0].cp        = eth_hdr_cp;
        hostTxFrmSwDesc.cpArr[0].len       = addFrameSize;
        hostTxFrmSwDesc.cpArr[0].offsetU32 = headerStart / sizeof(u32);
        hostTxFrmSwDesc.cpCount            = 1;
#endif  //HPGP_MAC_SAP


    }
    else
    {                        
        hostTxFrmSwDesc.rxPort = pPlcRxFrmSwDesc->rxPort;
        hostTxFrmSwDesc.txPort = pPlcRxFrmSwDesc->txPort;
        if(hostIntf == HOST_INTF_SPI)
        {
            
#ifdef HYBRII_SPI

        status = CHAL_RequestCP(&eth_hdr_cp);

            if (status != STATUS_SUCCESS)
            {
                FM_Printf(FM_ERROR, "\nSPI - Failed to alloc CP");
                numHostCPs -= pPlcRxFrmSwDesc->cpCount;
                return (status);
            }

            numHostCPs += 1;
            
            cellAddr = CHAL_GetAccessToCP(eth_hdr_cp);
            // addFrameSize = sizeof(hostHdr_t) + rsvd  + sizeof(hybrii_tx_req_t);        

            addFrameSize = sizeof(hybrii_tx_req_t); 

            headerStart = sizeof(hybrii_tx_req_t);
#ifdef HYBRII_B
            hostTxFrmSwDesc.frmLen       = frameSize + addFrameSize;
#else
            hostTxFrmSwDesc.frmLen       = frameSize + addFrameSize + SPI_CRC_LEN ;
#endif
#ifdef SW_RETRY

            //        pHybrii->rsvd += pad;
#endif  //SW_RETRY

            hostTxFrmSwDesc.cpArr[0].cp        = eth_hdr_cp;
#ifndef HYBRII_B
            for (j = headerStart; j < addFrameSize; j++) {
                crc16 = crc_ccitt_update(crc16, cellAddr[j]);
            }
#endif
            hostTxFrmSwDesc.cpArr[0].offsetU32 = 0;
            hostTxFrmSwDesc.cpArr[0].len      = addFrameSize;        
            hostTxFrmSwDesc.cpCount            = 1;
#endif  //HYBRII_SPI


        }else
        {
            hostTxFrmSwDesc.cpCount            = 0;
            hostTxFrmSwDesc.frmLen             = frameSize;

        }

    }

    curFrmLen = frameSize;  
    alin128 = frameSize % HYBRII_CELLBUF_SIZE;
    for (i = 0 ; i < pPlcRxFrmSwDesc->cpCount ; i++)
    {
        if(hostIntf == HOST_INTF_SPI)
        {
	        cellAddr =
	            CHAL_GetAccessToCP(pPlcRxFrmSwDesc->cpArr[i].cp);
		}
#ifdef DEBUG_DATAPATH		
		else
		{
			cellAddr =
			CHAL_GetAccessToCP(pPlcRxFrmSwDesc->cpArr[i].cp);

		}

#endif

        if (i == 0)
        {
            actualDescLen = ((curFrmLen < HYBRII_CELLBUF_SIZE)? curFrmLen : (HYBRII_CELLBUF_SIZE -
                             (pPlcRxFrmSwDesc->cpArr[i].offsetU32 *4)));
        }
        else
        if (curFrmLen > HYBRII_CELLBUF_SIZE)
        {
            actualDescLen = HYBRII_CELLBUF_SIZE;
        }
        else
        {
            actualDescLen = curFrmLen;
        }

        
#ifdef DEBUG_DATAPATH

        if (pktDbg || sigDbg)
            FM_Printf(FM_ERROR,"p rx\n");

#endif  //DEBUG_DATAPATH

        if(hostIntf == HOST_INTF_SPI)
        {
#ifndef HYBRII_B
            for (j = 0; j < actualDescLen; j++) {
                crc16 = crc_ccitt_update(crc16, cellAddr[j]);
            }
#endif

#ifdef DEBUG_DATAPATH

                if (pktDbg) {
                    FM_HexDump(FM_ERROR,"SPI: ", cellAddr, actualDescLen);
                }
#endif  //DEBUG_DATAPATH                
#ifndef HYBRII_B        
            if(actualDescLen <= (HYBRII_CELLBUF_SIZE - SPI_CRC_LEN))
            {
                cellAddr[j] = crc16 & 0xFF;
                j++;
                cellAddr[j] = (crc16 >> 8) & 0xFF;
                actualDescLen += SPI_CRC_LEN;
                
            }
            else if(actualDescLen == (HYBRII_CELLBUF_SIZE - 1) && 
                        alin128 == (HYBRII_CELLBUF_SIZE - 1)) // len chech of descriptor (127)
            {
                cellAddr[j] = crc16 & 0xFF;
                actualDescLen += (SPI_CRC_LEN - 1);
            }
            else if(actualDescLen != HYBRII_CELLBUF_SIZE)
            {
                FM_Printf(FM_ERROR, "Error in adding CRC\n");
            }
#endif
      }
        
#ifdef DEBUG_DATAPATH
    else
     {
		if (pktDbg)
		{    
			FM_HexDump(FM_ERROR,"", cellAddr, actualDescLen);
		}
     }
#endif  //DEBUG_DATAPATH

       
        
        if(1 == eth_plc_sniffer)
        {
#ifdef UM
            hostTxFrmSwDesc.cpArr[i+1].offsetU32 = pPlcRxFrmSwDesc->cpArr[i].offsetU32;
            hostTxFrmSwDesc.cpArr[i+1].len = actualDescLen;
            hostTxFrmSwDesc.cpArr[i+1].cp  = pPlcRxFrmSwDesc->cpArr[i].cp;
#else
            hostTxFrmSwDesc.cpArr[i+1].offsetU32 = pPlcRxFrmSwDesc->cpArr[i].offsetU32;
            hostTxFrmSwDesc.cpArr[i+1].len = actualDescLen;
            hostTxFrmSwDesc.cpArr[i+1].cp  = pPlcRxFrmSwDesc->cpArr[i].cp;
#endif  //HPGP_MAC_SAP
        }
        else
        {
            if(hostIntf == HOST_INTF_SPI)
            {

                hostTxFrmSwDesc.cpArr[i+1].offsetU32 = 0;
                hostTxFrmSwDesc.cpArr[i+1].len = actualDescLen;
                hostTxFrmSwDesc.cpArr[i+1].cp  = pPlcRxFrmSwDesc->cpArr[i].cp;

            }
            else
            {

                hostTxFrmSwDesc.cpArr[i].offsetU32 = 0;
                hostTxFrmSwDesc.cpArr[i].len = actualDescLen;
                hostTxFrmSwDesc.cpArr[i].cp  = pPlcRxFrmSwDesc->cpArr[i].cp;

            }

        }

        hostTxFrmSwDesc.cpCount++;
        curFrmLen -= actualDescLen;

    }

#ifdef DEBUG_DATAPATH

    if (pktDbg)
        FM_Printf(FM_ERROR,"rx end\n");
#endif  //DEBUG_DATAPATH
#ifndef HYBRII_B
    if(hostIntf == HOST_INTF_SPI)
    {
        // If crc need to be added in next CP
        if(alin128 == (HYBRII_CELLBUF_SIZE - 1) || alin128 == 0)
        {
            status = CHAL_RequestCP(&eth_hdr_cp);
            if (status != STATUS_SUCCESS)
            {
                FM_Printf(FM_ERROR, "\nFailed to alloc CP");
                numHostCPs -= (pPlcRxFrmSwDesc->cpCount + 1);
                return (status);
            }
            numHostCPs += 1;
            cellAddr = CHAL_GetAccessToCP(eth_hdr_cp);
            if(alin128 == 0)
            {
                cellAddr[0] = crc16 & 0xFF;
                cellAddr[1] = (crc16 >> 8) & 0xFF;
            }
            else
            {
                cellAddr[0] = (crc16 >> 8) & 0xFF;
            }
#ifdef UM        
            hostTxFrmSwDesc.cpArr[i+1].offsetU32 = 0;
            hostTxFrmSwDesc.cpArr[i+1].len = (alin128 == 0)? SPI_CRC_LEN:1;
            hostTxFrmSwDesc.cpArr[i+1].cp  = eth_hdr_cp;
            hostTxFrmSwDesc.cpCount++;
#else
            hostTxFrmSwDesc.cpArr[i+1].offsetU32 = 0;
            hostTxFrmSwDesc.cpArr[i+1].len = (alin128 == 0)? SPI_CRC_LEN:1;
            hostTxFrmSwDesc.cpArr[i+1].cp  = eth_hdr_cp;
            hostTxFrmSwDesc.cpCount++;
#endif  //HPGP_MAC_SAP
        }
    }
#endif //HYBRII_B

    if(hostIntf == HOST_INTF_SPI)
    {
#ifdef HYBRII_SPI

        hostTxFrmSwDesc.frmLen -= sizeof(hybrii_tx_req_t);
#endif  //HYBRII_SPI
    }
#if 1 //def HPGP_MAC_SAP

    if (datapath_IsQueueFull(HOST_DATA_QUEUE) == TRUE)
    {

        
        numHostCPs -= hostTxFrmSwDesc.cpCount;
#ifdef HYBRII_SPI  		
		if(hostIntf == HOST_INTF_SPI)
        {            
        	CHAL_DecrementReleaseCPCnt(eth_hdr_cp);
		}
#endif		
        return STATUS_FAILURE;
            
    }

  
  
#ifdef ETH_BRDG_DEBUG
	  plcRxFrameCnt++;
#endif
	os_set_ready(HYBRII_TASK_ID_FRAME);
	datapath_queue(HOST_DATA_QUEUE, &hostTxFrmSwDesc);

	
    status = STATUS_SUCCESS;
#else
    if(hostIntf == HOST_INTF_SPI)
    {
#ifdef HYBRII_SPI
        hal_spi_tx_cleanup ();
        hal_spi_rx_cleanup ();
        hostTxFrmSwDesc.frmLen -= sizeof(hybrii_tx_req_t);
        status = hal_spi_tx_dma_cp(hostTxFrmSwDesc.frmLen, &hostTxFrmSwDesc);
#endif  //HYBRII_SPI
    }
    else if(hostIntf == HOST_INTF_ETH)
    {
        status = EHAL_EthTxQWrite(&hostTxFrmSwDesc);
    }
    
    if (status == STATUS_FAILURE)
    {
        if((eth_plc_sniffer)
#ifdef HPGP_MAC_SAP
            || (1)
#endif  //HPGP_MAC_SAP
            )
        {
            CHAL_DecrementReleaseCPCnt(eth_hdr_cp);
        }
       FM_Printf(FM_ERROR,"\nCan not send packet to interface");
    }
#endif  // 1


	return STATUS_SUCCESS;

}


#ifndef HYBRII_FPGA
void datapath_transmitDataHost()
{
	sSwFrmDesc* tHostTxFrmSwDesc = NULL;
	sSwFrmDesc hostTxFrmSwDesc;


    // If Eth interface
    if((tHostTxFrmSwDesc = 
		             	datapath_getHeadDesc(HOST_DATA_QUEUE, 1)) != NULL)
    {
         memcpy((u8*)&hostTxFrmSwDesc,
		 		(u8*)tHostTxFrmSwDesc, sizeof(hostTxFrmSwDesc));

			datapath_writeHostIntf(&hostTxFrmSwDesc);		
    }

	
} 
#endif   //HYBRII_FPGA
/*
-------------------------------------------------------------------------


  HAL_ETH.C

----------------=======================================----------------------=-------------=-
*/

eStatus hostRxPostProcess(sSwFrmDesc *plcTxFrmSwDesc)
{
	
	u8 dropFrame=0;	
	u8 dstTei=0;
	sHpgpHalCB *hhalCb;
#ifdef UM	
	sEth2Hdr *pktEthHdr;
	sHaLayer *hal;
	sLinkLayer *linkLayer;
	sStaInfo *staInfo = NULL;
#endif

#ifdef UM
	
	xdata u8 *cellAddr;

	linkLayer = HPGPCTRL_GetLayer(HP_LAYER_TYPE_LINK);
	staInfo = LINKL_GetStaInfo(linkLayer);	
	
#ifdef HPGP_HAL_TEST
		hhalCb = &gHpgpHalCB;
#else
		hal = (sHaLayer*)HOMEPLUG_GetHal();

		hhalCb = hal->hhalCb;
#endif

	if(plcTxFrmSwDesc->frmInfo.plc.dtei == 0 )
	{
		cellAddr = CHAL_GetAccessToCP(plcTxFrmSwDesc->cpArr[0].cp);

		pktEthHdr = ((sEth2Hdr*)cellAddr);
#ifdef UM		
		
		if (!MCTRL_IsAssociated())
		{

			dropFrame = 1;

		}
		else
		if( hhalCb->devMode == DEV_MODE_STA)
		{
			if(!staInfo->staStatus.fields.authStatus)
			{
				
				dropFrame = 1;
			}
		}
			
					
						
#endif
		if ((host_intf_max_cp + plcTxFrmSwDesc->cpCount)> HOST_TO_PLC_MAX_CP)
		{				


			
#ifdef DEBUG_DATAPATH
				
		if (sigDbg)
		FM_Printf(FM_ERROR,"max cp %bu\n",(plcTxFrmSwDesc->cpCount + host_intf_max_cp));

#endif					
		dropFrame = 1;

		}

		
		if ((pktEthHdr->dstaddr[0] & 0x01))
		{

		/*
		Note : dstTei should be set to 0xFF for broadcast frame.
		Has to be revisited. 0xFF was getting dropped by receiver

		*/

			dstTei = 0xFF;

		}
		else
		{
			sScb* dstScb = NULL;
			
			sCnam *ccoNam = &linkLayer->ccoNam;
			u8 devmode = LINKL_GetMode(linkLayer);

			//dstScb = CRM_FindScbMacAddr(&pktEthHdr->dstaddr);
			{
			//sScb* dstScb = NULL;
			//sLinkLayer	  *linkl = (sLinkLayer *)HPGPCTRL_GetLayer(HP_LAYER_TYPE_LINK);
				sCrm		  *crm = LINKL_GetCrm(linkLayer);
				dstScb = CRM_GetNextScb(crm, dstScb);
				while(dstScb)
				{
					if(memcmp(&pktEthHdr->dstaddr, &dstScb->macAddr, MAC_ADDR_LEN) == 0)
					{
						break;
					}
					dstScb = CRM_GetNextScb(crm, dstScb);
				}
			}

			if (!dstScb)
			{
				dropFrame = 1;
				FM_Printf(FM_ERROR,"post proc drop\n");
#if 0
				FM_HexDump(FM_USER,"DST Address",&pktEthHdr->dstaddr,MAC_ADDR_LEN);
#endif
			}
			else
			{
				if(devmode == LINKL_STA_MODE_STA)
				{
					if(0)//staInfo->staStatus.fields.authStatus != 1)
					{
						
					// dropFrame = 1;
					}
				}

				else if(devmode == LINKL_STA_MODE_CCO)
				{
					if(dstScb->staStatus.fields.authStatus != 1)
					{
						dropFrame = 1;
										
					}
				}
				else
				{
					dropFrame = 1;
				}
#ifdef ROUTE
				if(dstScb->lrtEntry.routeIsInvalid == TRUE || dstScb->lrtEntry.routeOnHold == TRUE)
				{
					dropFrame = 1;
					FM_Printf(FM_USER,"Route drop\n");
				}
				else
				{
					dstTei = dstScb->lrtEntry.nTei;
				}
#else
				dstTei = dstScb->tei;
#endif
			}


		}
#else
		dstTei = 1;
#endif

		if(1 == dropFrame)
		{
		//FM_Printf(FM_USER,"Frame dropped\n");
			datapath_getHeadDesc(PLC_DATA_QUEUE, 1);
			CHAL_FreeFrameCp(plcTxFrmSwDesc->cpArr,plcTxFrmSwDesc->cpCount);
			return STATUS_FAILURE;
		}


		if (dstTei == 0xFF)
		{
			plcTxFrmSwDesc->frmInfo.plc.mcstMode	   = HPGP_MCST;			
			plcTxFrmSwDesc->frmInfo.plc.phyPendBlks    = HPGP_PPB_MCFRPT;
#ifdef PROXY_BCST
			if(proxyBcst == 1)
			{ 
				sScb *dstScb =NULL;	
				dstScb = getLeastRssiScb();
				if(dstScb != NULL)
				{
					dstTei = dstScb->tei;
				}
				else
				{
					dstTei = 0xFF;
				}
			}
#endif
		}
		else
		{
			plcTxFrmSwDesc->frmInfo.plc.mcstMode	   = HPGP_UCST;  // Unicast
		}


#ifdef HPGP_HAL_TEST
		plcTxFrmSwDesc->frmInfo.plc.eks =  gNekEks; //HPGP_UNENCRYPTED_EKS;
		plcTxFrmSwDesc->frmInfo.plc.dtei           = hhalCb->remoteTei;
		plcTxFrmSwDesc->frmInfo.plc.stei           = hhalCb->selfTei;//HYBRII_DEFAULT_TEISTA;
#else
	
		plcTxFrmSwDesc->frmInfo.plc.eks = staInfo->nekEks;//HPGP_UNENCRYPTED_EKS;
#endif
		plcTxFrmSwDesc->frmInfo.plc.bcnDetectFlag  = REG_FLAG_SET;// REG_FLAG_CLR;
		plcTxFrmSwDesc->frmInfo.plc.clst = HPGP_CONVLYRSAPTYPE_ETH;
		plcTxFrmSwDesc->frmType        = HPGP_HW_FRMTYPE_MSDU;

#ifdef UM
		if(linkLayer->mode == LINKL_STA_MODE_CCO)
		{

		//		FM_Printf(FM_USER, "f cc\n");
#ifdef CCO_FUNC		            
			plcTxFrmSwDesc->frmInfo.plc.dtei 		  = dstTei;
			plcTxFrmSwDesc->frmInfo.plc.stei 		  = staInfo->ccoScb->tei;
#endif
		}
		else
		{
#ifdef STA_FUNC
			plcTxFrmSwDesc->frmInfo.plc.dtei 		  = dstTei;//staInfo->ccoScb->tei;
			plcTxFrmSwDesc->frmInfo.plc.stei 		  = staInfo->tei;
#endif

		}

		} 
#endif	
	plcTxFrmSwDesc->frmInfo.plc.plid = 0;   //[YM] This line of code has to be changed base on differnet QoS priority


	if (plcTxFrmSwDesc->frmInfo.plc.plid == 0)
	{
		plcTxFrmSwDesc->frmInfo.plc.phyPendBlks    = HPGP_PPB_CAP0;
	}
	else
	{
		plcTxFrmSwDesc->frmInfo.plc.phyPendBlks    = HPGP_PPB_CAP123;
	}

#ifdef UM

	plcTxFrmSwDesc->frmInfo.plc.snid = staInfo->snid;


#endif

#if 1 //def HPGP_HAL_TEST
	plcTxFrmSwDesc->frmInfo.plc.stdModeSel     = STD_ROBO_TEST; // std robo
#endif
	plcTxFrmSwDesc->frmInfo.plc.dt_av = HPGP_DTAV_SOF;
	plcTxFrmSwDesc->frmInfo.plc.saf = 1;

		
	return STATUS_SUCCESS;
}


#if defined(POWERSAVE) || defined(LLP_POWERSAVE)
void datapath_transmitDataPlc(u8 from)
#else
void datapath_transmitDataPlc()
#endif
{
	// their is something queued in hea
	eStatus status;
#ifndef HPGP_HAL_TEST    
	sHaLayer *hal;
#endif    
	sHpgpHalCB *hhalCb;
	u8  RegValue;
	u8   TxLoop;
	uPlcTxPktQCAP_Write   cap_write;
#if defined(POWERSAVE) || defined(LLP_POWERSAVE)
    sLinkLayer    *linkLayer = (sLinkLayer *)HPGPCTRL_GetLayer(HP_LAYER_TYPE_LINK);
    sStaInfo      *staInfo = LINKL_GetStaInfo(linkLayer);
	sScb          *scb;
	sScb          *dstScb;
	u32 ntb=0;
	u32 bpst=0;
	u32 timeDiff=0;
	sPsSchedule commAwd;
	//u8 tmpPss;
	u8 dtei;
	u8 modVal;
	u32 tmpBpCnt;
#endif
//#ifdef HYBRII_HPGP
#ifdef HPGP_HAL_TEST
	hhalCb = &gHpgpHalCB;
#else
	hal = (sHaLayer*)HOMEPLUG_GetHal();

	hhalCb = hal->hhalCb;
#endif
//#endif //HYBRII_HPGP
    TxLoop = 0;
	RegValue = 0;
SWQCheckAgain:


#if 1
    //RegValue = ReadU32Reg(PLC_CMDQ_STAT);
	//[YM] check CMD queue count
	RegValue = ReadU8Reg(0xEAC);			   //Check CMD queue number
	if (RegValue > 0)
	{
	    uPlcStatusReg         plcStatus;

	   //printf("RegValue = %bX\n", RegValue);
	   cap_write.capw.CapRdy = 1;	  //[YM] Temporary set CAP 0 packet only
	   WriteU32Reg(PLC_CAP_REG, cap_write.reg);

	   plcStatus.reg = ReadU32Reg(PLC_STATUS_REG);
	   plcStatus.s.plcTxQRdy = 1;
	   WriteU32Reg(PLC_STATUS_REG, plcStatus.reg);
	}
#endif

	if (datapath_IsQueueEmpty(PLC_DATA_QUEUE)
							== FALSE )
	{
		sSwFrmDesc	*lpPlcTxFrmSwDesc;

		lpPlcTxFrmSwDesc =
						datapath_getHeadDesc(PLC_DATA_QUEUE, 0);

		if (lpPlcTxFrmSwDesc->frmInfo.plc.status == PLC_TX_PENDING)
		{
#ifdef  HPGP_HAL_TEST_DONT_DO_THIS
			if( MAX_PLC_TX_TIMEOUT < (get_TimerTick() - plcTxTime))
			{
				lpPlcTxFrmSwDesc->frmInfo.plc.status = PLC_TX_DONE;
#ifdef ETH_BRDG_DEBUG
				numForcePlcTxDone++; 
				if (myDebugFlag)
					printf("LM Forcing PLC TX Done\n");
#endif
			}
#endif
			// Tx timeout handling to release Tx pending flag
#ifdef HPGP_HAL_TEST_DONT_DO_THIS           
			if( MAX_PLC_TX_TIMEOUT < (STM_GetTick() - plcTxTime))
			{
				lpPlcTxFrmSwDesc->frmInfo.plc.status = PLC_TX_DONE;
#if 1 //def DEBUG_DATAPATH
				if(1)
				{
					FM_Printf(FM_USER, "Missing PLC TX Done\n");
				}
#endif
			}
#endif
			/*
					 we dont process next frame until TX_PENDING is cleared
				*/

#ifdef ETH_BRDG_DEBUG
			numPlcPendingRet++;
#endif			

		//rajantbd 	os_set_ready(HYBRII_TASK_ID_FRAME);	
			return;
		}
		else 
		if (lpPlcTxFrmSwDesc->frmInfo.plc.status == PLC_TX_DONE)
		{
			
			lpPlcTxFrmSwDesc->frmInfo.plc.status = 0;				 
			
		//				FM_Printf(FM_USER, "pop\n");
			datapath_getHeadDesc(PLC_DATA_QUEUE, 1);
			goto SWQCheckAgain;
		}

	}
	
	if (datapath_IsQueueEmpty(PLC_DATA_QUEUE)
								== FALSE )

	{
	
		sSwFrmDesc  *lpPlcTxFrmSwDesc;	

    RegValue = ReadU8Reg(0xEAC);			   //Check CMD queue number    
	//if (RegValue > 0)					   // [YM] Push multiple paclets to Tx queue
	    status = HHAL_IsPlcIdle();
	//else
	//	status = STATUS_SUCCESS;

		if (status == STATUS_SUCCESS)
		{
			lpPlcTxFrmSwDesc =
					datapath_getHeadDesc(PLC_DATA_QUEUE, 0);

//			lpPlcTxFrmSwDesc->frmInfo.plc.retry = 0;

#ifdef POWERSAVE
#ifdef PS_PRINT
		if (psDebug)
		{
			printf("tx plc(from=%bu):\n", from);
		}
#endif		
		if (from == 1)
			psPclTxWriteFromBcn++;
		else psPclTxWriteFromFrame++;

		if (hhalCb->psInSleepMode == TRUE)
			return;	// in Sleep mode, can't Tx
			
		// STA should only talk to CCO in our implementation.
		memset(&commAwd, 0, sizeof(sPsSchedule));
		dtei = lpPlcTxFrmSwDesc->frmInfo.plc.dtei;

		if( hhalCb->devMode == DEV_MODE_STA)
		{
			u8 dteiIsCco = 0;	// 0: unknown, 1: CCO's TEI, 2: broadcast

			scb = staInfo->staScb;
			if (scb)
			{
				if (scb->psState == PSM_PS_STATE_ON)
				{
					if (dtei == staInfo->ccoScb->tei)
					{
						// DTEI is CCO, use STA's common AWD
						memcpy(&commAwd, &scb->commAwd, sizeof(sPsSchedule));
					}
					else
					{
#ifdef PS_PRINT					
					if (psDebug)
						printf("txPlc: STA: dtei %bu is not CCO\n", dtei);
#endif					
						// any other types, ie. broadcast, just use STA's PSS
						PSM_cvrtPss_Awd(scb->pss, &commAwd);
					}
				}
			}
			else
			{
#ifdef PS_PRINT			
				if (psDebug)
					printf("txPlc: STA: scb=NULL\n");
#endif				
			}
		}
		else
		{
			scb = staInfo->ccoScb;
			if (scb)
			{
				if (scb->psState == PSM_PS_STATE_ON)
				{
					if (dtei == 0xFF)
					{
						// DTEI is broadcast, use CCO's common AWD
						memcpy(&commAwd, &scb->commAwd, sizeof(sPsSchedule));
					}
					else
					{
						// DTEI is unicast, get the smaller of the 2 AWDs
						if ((dstScb = CRM_GetScb(&linkLayer->ccoRm, dtei)) != NULL)
						{
							u8 tmpPss;

							tmpPss =  scb->pss;
							PSM_getLargerPSS(&tmpPss, dstScb->pss);
							PSM_cvrtPss_Awd(tmpPss, &commAwd);
						}
						else
						{
#ifdef PS_PRINT						
							if (psDebug)
								printf("txPlc: CCO: cannot find dtei %bu\n");
#endif							
						}
					}
				}
			}
			else
			{
#ifdef PS_PRINT			
				if (psDebug)
					printf("txPlc: CCO: scb=NULL\n");
#endif				
			}
		}
#ifdef PS_PRINT

		if (psDebug)
		{
//#ifdef HYBRII_HPGP		
			printf("txPlc: commAwd.awdTime=%bu, commAwd.numBp=%d, scb->bpCnt=%d, gHpgpHalCB.halStats.psBpIntCnt=%lu\n", 
					commAwd.awdTime, commAwd.numBp, scb->bpCnt, gHpgpHalCB.halStats.psBpIntCnt);
//#endif //HYBRII_HPGP
		}
#endif
		if (commAwd.awdTime && commAwd.numBp)
		{
			// common AWD exists. This means that: 
			// if CCO: at least 1 station is in PS mode. if STA: it must be in PS mode
//			printf("scb->bpCnt=%d, scb->commAwd.numBp=%d\n", scb->bpCnt, scb->commAwd.numBp);
//#ifdef HYBRII_HPGP
			if (gHpgpHalCB.devMode == DEV_MODE_STA)
				tmpBpCnt = gHpgpHalCB.halStats.psBpIntCnt;
			else tmpBpCnt = scb->bpCnt;
//#endif //HYBRII_HPGP

			modVal = tmpBpCnt % commAwd.numBp;
			if (modVal == 0)
			{
				// start of PSP: commAwd.numBp must be of power of 2
				if (!(commAwd.awdTime & 0x80))
				{
					// AwdTime is in ms
					ntb = (rtocl(ReadU32Reg(PLC_NTB_REG))*40)/1000000;
					bpst = (rtocl(ReadU32Reg(PLC_CurBPST_REG))*40)/1000000;
					// take care of wrap-around ???
					if (ntb > bpst)
					{
						timeDiff = ntb-bpst;
					}
					else 
					{
						timeDiff = 0;
					}
#ifdef PS_PRINT					
					if (psDebug)
						printf("txPlc: ntb=%lu, bpst=%lu, timeDiff=%lu\n", ntb, bpst, timeDiff);
#endif					
					if (timeDiff > commAwd.awdTime)
					{
#ifdef PS_PRINT					
						if (psDebug)
							printf("timediff too big. Return\n");
#endif						
						if (from == 2)
							psNoTxFrmCnt++;
						else psFrmBcnNoTxFrmCnt++;
						return;
					}
				}
				// AwdTime is in # of BP, allow Tx 
#ifdef PS_PRINT				
				if (psDebug)
					printf("PSP: AWD = %bu >= modVal = %bu. OK to Tx\n", commAwd.awdTime & 0xF, modVal);
#endif				
			}
			else
			{
				// in between PSPs
				if (commAwd.awdTime & 0x80)
				{
					// AwdTime is in # of BP 
					if (modVal >= (commAwd.awdTime & 0xF))
					{
#ifdef PS_PRINT					
						if (psDebug)
							printf("AWD = %bu >= modVal = %bu. Return\n", commAwd.awdTime & 0xF, modVal);
#endif
						return;
					}
#ifdef PS_PRINT					
					if (psDebug)
						printf("AWD = %bu < modVal = %bu. OK to Tx\n", commAwd.awdTime & 0xF, modVal);
#endif
				}
				else
				{
#ifdef PS_PRINT				  
					if (psDebug)
						printf("Mod op not 0. Return\n");
#endif					
					if (from == 2)
						psNoTxWrongBpFrmCnt++;
					else  psFrmBcnNoTxWrongBpFrmCnt++;
					return;
				}
			}
		}
		else
		{
			if (from == 2)
				psNoTxZeroAwdFrmCnt++;
			else  psFrmBcnNoTxZeroAwdFrmCnt++;

			if (from == 1)
				// only send frames from ZeroCrossing interrupt and BcnRx interrupt if this sta is in PS mode
				return;
		}
		if (from == 2)
			psTxFrmCnt++;
		else psFrmBcnTxFrmCnt++;
#ifdef PS_PRINT		
		if (psDebug)
			printf("OK to PLC TX\n");
#endif		
#endif

#ifdef LLP_POWERSAVE
		if (from == 1)
			psPclTxWriteFromBcn++;
		else psPclTxWriteFromFrame++;
#ifdef PS_PRINT		
		if (psDebug1)
		{
			printf("tx plc(from=%bu): psPclTxWriteFromBcn=%lu, psPclTxWriteFromFrame=%lu\n", from, psPclTxWriteFromBcn, psPclTxWriteFromFrame);
		}
#endif		
#ifndef OLDWAY
		if (hhalCb->psInSleepMode == TRUE)
		{
			// in Sleep mode, can't Tx
			// this applies to both CCO and STA since
			// their awake/sleep periods sync with each
			// other's
#ifdef PS_PRINT			
			if (psDebug1)
				printf("PLC TX: in sleep mode, return\n");
#endif			
			return;
		}
#else
		if (hhalCb->psInSleepMode == TRUE)
		{
			// in Sleep mode, can't Tx
#ifdef PS_PRINT			
			if (psDebug1)
				printf("PLC TX: in sleep mode, return\n");
#endif			
			return;
		}
		if (hhalCb->psInSleepMode == TRUE)
			// in Sleep mode, can't Tx
			return;
		// STA should only talk to CCO in our implementation.
		memset(&commAwd, 0, sizeof(sPsSchedule));

		if( hhalCb->devMode == DEV_MODE_STA)
		{
			scb = staInfo->staScb;
		}
		else
		{
			scb = staInfo->ccoScb;
		}

		if (scb)
		{
			if (scb->psState == PSM_PS_STATE_ON)
			{
				memcpy(&commAwd, &scb->commAwd, sizeof(sPsSchedule));
			}
		}
/*
		else
		{
			if (psDebug1)
				printf("txPlc: CCO: scb=NULL\n");
		}
*/
#ifdef PS_PRINT

		if (psDebug1)
		{
			printf("txPlc: commAwd.awdTime=%bu, commAwd.numBp=%d, scb->bpCnt=%d\n", 
					commAwd.awdTime, commAwd.numBp, scb->bpCnt);
		}
#endif		
		if (commAwd.awdTime && commAwd.numBp)
		{
			// common AWD exists. This means that: 
			// if CCO: at least 1 station is in PS mode. if STA: it must be in PS mode
//			printf("scb->bpCnt=%d, scb->commAwd.numBp=%d\n", scb->bpCnt, scb->commAwd.numBp);
			tmpBpCnt = scb->bpCnt;

			modVal = tmpBpCnt % commAwd.numBp;
			if (modVal == 0)
			{
#ifdef DOTHISPART // we don't allow AWD to be in ms for now
				// start of PSP: commAwd.numBp must be of power of 2
				if (!(commAwd.awdTime & 0x80))
				{
					// AwdTime is in ms
					ntb = (rtocl(ReadU32Reg(PLC_NTB_REG))*40)/1000000;
					bpst = (rtocl(ReadU32Reg(PLC_CurBPST_REG))*40)/1000000;
					// take care of wrap-around ???
					if (ntb > bpst)
						timeDiff = ntb-bpst;
					else timeDiff = 0;
					if (psDebug1)
						printf("txPlc: ntb=%lu, bpst=%lu, timeDiff=%lu\n", ntb, bpst, timeDiff);
					if (timeDiff > commAwd.awdTime)
					{
						if (psDebug1)
							printf("timediff too big. Return\n");
						if (from == 2)
							psNoTxFrmCnt++;
						else psFrmBcnNoTxFrmCnt++;
						return;
					}
				}
#endif
#ifdef PS_PRINT

				// AwdTime is in # of BP, allow Tx
				if (psDebug1)
					printf("PSP: AWD = %bu >= modVal = %bu. OK to Tx\n", commAwd.awdTime & 0xF, modVal);
#endif				
			}
			else
			{
				// in between PSPs
				if (commAwd.awdTime & 0x80)
				{
					// AwdTime is in # of BP 
					if (modVal >= (commAwd.awdTime & 0xF))
					{
#ifdef PS_PRINT

						if (psDebug1)
							printf("AWD = %bu >= modVal = %bu. Return\n", commAwd.awdTime & 0xF, modVal);
#endif						
						return;
					}
#ifdef PS_PRINT					
					if (psDebug1)
						printf("AWD = %bu < modVal = %bu. OK to Tx\n", commAwd.awdTime & 0xF, modVal);
#endif					
				}
				else
				{
#ifdef PS_PRINT				
					if (psDebug1)
						printf("Mod op not 0. Return\n");
#endif					
					if (from == 2)
						psNoTxWrongBpFrmCnt++;
					else  psFrmBcnNoTxWrongBpFrmCnt++;
					return;
				}
			}
		}
		else
		{
			if (from == 2)
				psNoTxZeroAwdFrmCnt++;
			else  psFrmBcnNoTxZeroAwdFrmCnt++;

			if (from == 1)
			{
				// only send frames from ZeroCrossing interrupt and BcnRx interrupt if this sta is in PS mode
#ifdef PS_PRINT				
				if (psDebug1)
					printf("AWD = 0. Return\n");
#endif				
				return;
			}
		}
#endif // OLDWAY

		if (from == 2)
			psTxFrmCnt++;
		else psFrmBcnTxFrmCnt++;
#ifdef PS_PRINT		
		if (psDebug1)
			printf("OK to PLC TX\n");
#endif		
#endif // LLP_PS

		status = hostRxPostProcess(lpPlcTxFrmSwDesc);
		if(status == STATUS_FAILURE)//kiran
		{
			//FM_Printf(FM_USER,"RX Failed\n");
			return;
		}
#ifdef DEBUG_DATAPATH
		if (sigDbg)
		FM_Printf(FM_USER, "ptx\n");
#endif
//#ifdef HYBRII_HPGP
#ifdef HPGP_HAL_TEST
			status	= HHAL_PlcTxQWrite(lpPlcTxFrmSwDesc);
#else                          
			status = HHAL_PlcTxQWrite(hal, lpPlcTxFrmSwDesc);
#endif
//#endif //HYBRII_HPGP
			hhalCb->halStats.CurTxTestFrmCnt++;
#ifdef ETH_BRDG_DEBUG
			plcTxFrameCnt++;
#endif
			TxLoop++;
			if (status == STATUS_FAILURE)
			{
				u16 i;
#ifdef ETH_BRDG_DEBUG
				plcTxWriteFail++;
#endif
#if defined(POWERSAVE) || defined(LLP_POWERSAVE)
		if (from == 2)
			psPlcTxWriteErrCnt++;
		else psFrmBcnPlcTxWriteErrCnt++;
#endif
				datapath_getHeadDesc(PLC_DATA_QUEUE, 1);
                gHpgpHalCB.halStats.HtoPswDropCnt++;
				for( i=0 ; i< lpPlcTxFrmSwDesc->cpCount ; i++ )
				{
					CHAL_DecrementReleaseCPCnt(lpPlcTxFrmSwDesc->cpArr[i].cp);
				}

			}
			else
			{

				lpPlcTxFrmSwDesc->frmInfo.plc.status = PLC_TX_DONE;  //PLC_TX_PENDING;
			
#ifdef UM
				plcTxTime = STM_GetTick();
#else
//				   plcTxTime = get_TimerTick();
#endif
				lpPlcTxFrmSwDesc->frmInfo.plc.attemptCnt++;
#if defined(POWERSAVE) || defined(LLP_POWERSAVE)
		if (from == 2)
			psPlcTxOKCnt++;
		else psFrmBcnPlcTxOKCnt++;
#endif
			}
		}
		else
		{
#ifdef ETH_BRDG_DEBUG
			if (myDebugFlag)
				printf("SendToPlc: HHAL_IsPlcIdle returns FAIL\n");	
			
#endif
#ifdef POWERSAVE
			if (from == 2)
				psPlcIdleErrCnt++;
			else psFrmBcnPlcIdleErrCnt++;
#endif
//			FM_Printf(FM_USER, "per\n");

		
			os_set_ready(HYBRII_TASK_ID_FRAME);	

		}					
					

	}

}

u32 plcTxDataSeqNum = 1;
u32 plcRxDataSeqNum = 0;
#define MAX_U32_VALUE 0xFFFFFFFF


// isGCIpkt checks the 1st CP of a received ETH pkt to determine
// whether this is a Greenvity test pkt
// Return:
//      - TRUE if it's a GCI pkt
//      - FALSE otherwise
#ifdef DO_GV_ETH_TEST

u8 isGCIpkt(uRxPktQCPDesc *pRxPktQCPDesc,
            sSwFrmDesc *pEthTxFrmSwDesc, u16 frmLen,
            u8 descLen, u8 *pRetId, 
            u8 *pRetRcvPkt, u8 *pdropPkt)
{
//    volatile u8 XDATA *cellAddr;
	u8 *cellAddr; 
    u8 boardId;
    u8 rcvPkt;
    u8 nxtPktType = 0;
    u8 GCIpkt = FALSE;
#ifdef MEM_PROTECTION
	u8 cp_localBuf[HYBRII_CELLBUF_SIZE];	// local CP buffer
#endif

#ifdef MEM_PROTECTION
	// copy cp to local buf
	if (HHAL_CP_Get_Copy(pRxPktQCPDesc->s.cp, &cp_localBuf[0], sizeof(sEth2Hdr)+sizeof(sEthGCIHdr)) == STATUS_FAILURE)
	{
		printf("isGCIpkt: Failed to make a copy of CP. Return FALSE\n");
		return(FALSE);
	}
	cellAddr = &cp_localBuf[0];
#else
    cellAddr = CHAL_GetAccessToCP(pRxPktQCPDesc->s.cp);
#endif

    if (descLen >= (sizeof(sEth2Hdr)+sizeof(sEthGCIHdr)))
    {
        volatile sEth2Hdr   *pEthHdr;
        volatile sEthGCIHdr *pGCIHdr;
        u8                  tmpAddr[MAC_ADDR_LEN];

        pEthHdr = (sEth2Hdr *) cellAddr;

        if (pEthHdr->ethtype == ETH_TYPE_GREENVITY)
        {
            /* Our test pkt */
            GCIpkt = TRUE;
            pGCIHdr = (sEthGCIHdr *) (cellAddr + sizeof(sEth2Hdr));
            rcvPkt = pGCIHdr->pktType;
            boardId = 0xff;
            if (rcvPkt == CMD_CONN_REQ_PKT)
            {
                // this must be a slave, ConnState must be the 1st index
                boardId = 0;
                if (ConnState[boardId].state == GCI_STATE_OPEN)
                {
                    // this station is already OPEN for a test, do nothing
                    *pdropPkt = TRUE;
                }
                else
                {
                    // Conn Req to start of test (slave), send CONN RESP
                    // the dest MAC Addr is broadcast, copy slave addr 
                    // to dest so we can swap the addr when transmit the CONN RESP later
                    memcpy(pEthHdr->dstaddr, pGCIHdr->slaveMACaddr, MAC_ADDR_LEN);
                    nxtPktType = CMD_CONN_RESP_PKT; // Conn Response back
                }
            } else if (rcvPkt  == CMD_CONN_RESP_ACK_PKT)
            {
                // this must be a slave, ConnState must be the 1st index
                boardId = 0;
                if (ConnState[boardId].state == GCI_STATE_OPEN)
                {
                    // this connection is already OPEN, do nothing
                    *pdropPkt = TRUE;
                }
                else
                {
                    // ACK for Conn Response(slave), set station ready to 
                    // receive test data
                    ConnState[boardId].state = GCI_STATE_OPEN;
                    ConnState[boardId].testType = pGCIHdr->testType;
                    ConnState[boardId].my_numPktTx = 0;
                    ConnState[boardId].my_numPktRx = 0;
                    memcpy(ConnState[boardId].myMACaddr, pGCIHdr->slaveMACaddr, 
                            MAC_ADDR_LEN);
                    stationType = SLAVE_STATION;
                    *pdropPkt = TRUE;
                    /*printf("\nStation %bx:%bx:%bx:%bx:%bx:%bx is ready to receive data from "
                            "%bx:%bx:%bx:%bx:%bx:%bx\n", 
                            pGCIHdr->slaveMACaddr[0], pGCIHdr->slaveMACaddr[1], 
                            pGCIHdr->slaveMACaddr[2], pGCIHdr->slaveMACaddr[3],
                            pGCIHdr->slaveMACaddr[4], pGCIHdr->slaveMACaddr[5], 
                            pEthHdr->srcaddr[0], pEthHdr->srcaddr[1], 
                            pEthHdr->srcaddr[2], pEthHdr->srcaddr[3], pEthHdr->srcaddr[4], 
                            pEthHdr->srcaddr[5]);*/
                }
            } else if (rcvPkt == CMD_DISCON_REQ_PKT)
            {
                // end of test (slave), send ACK with slave's statistics
                // make sure MAC dest addr match with this station's
                if (ConnExistSlave(pEthHdr->dstaddr, &boardId) == FALSE)
                {
                    *pdropPkt = TRUE;
                }
                else
                {
                    pGCIHdr->numPktTx = ConnState[boardId].my_numPktTx;
                    pGCIHdr->numPktRx = ConnState[boardId].my_numPktRx;
                    nxtPktType = CMD_DISCON_ACK_PKT;    // ACK back
                    memset(&ConnState[boardId], 0, sizeof(sConnState));
//                  stationType = UNDEFINED_STATION;    // set it to unknown
                }
            } else if (rcvPkt == CMD_CONN_RESP_PKT)
            {
                // Conn Response (master). Set State to OPEN and send ACK
                // make sure MAC dest addr match with this station's
                if (ConnExistMaster(pEthHdr->dstaddr, pEthHdr->srcaddr, &boardId)
                         == TRUE)
                {
                    if (ConnState[boardId].state == GCI_STATE_OPEN)
                    {
                        // this connection is already OPEN, do nothing
                        *pdropPkt = TRUE;
                    }
                    else
                    {
                        stationType = MASTER_STATION;
                        ConnState[boardId].state = GCI_STATE_OPEN;
                        ConnState[boardId].my_numPktTx = 0;
                        ConnState[boardId].my_numPktRx = 0;
                        nxtPktType = CMD_CONN_RESP_ACK_PKT; // ACK back
                    }
                }
            } else if (rcvPkt == CMD_DISCON_ACK_PKT)
            {
                // DISC ACK:  Stop of test (master). Set State to CLOSED, copy the 
                // slave's statistics and return
                // make sure MAC dest addr match with this station's
                if (ConnExistMaster(pEthHdr->dstaddr, pEthHdr->srcaddr, 
                        &boardId) == TRUE)
                {
                    ConnState[boardId].state = GCI_STATE_CLOSED;
                    ConnState[boardId].slave_numPktTx = pGCIHdr->numPktTx;
                    ConnState[boardId].slave_numPktRx = pGCIHdr->numPktRx;
                }
                *pdropPkt =  TRUE;
            } else if (rcvPkt == CMD_DATA_PKT)
            {
                // DATA pkt. 2 scenarios:
                //      - if this is the master: discard the pkt
                //      - if this is a slave: if it's half duplex, discard
                //        the pkt, otherwise, xmit that same pkt
                //        back to the master
                if ((stationType != MASTER_STATION) && (stationType != SLAVE_STATION))
                {
                    *pdropPkt = TRUE;
                }
                else
                {
                    // make sure MAC dest addr match with this station's
                    if (((stationType == MASTER_STATION) && 
                        (ConnExistMaster(pEthHdr->dstaddr, pEthHdr->srcaddr, &boardId)
                             == FALSE)) ||
                        ((stationType == SLAVE_STATION) && 
                            (ConnExistSlave(pEthHdr->dstaddr, &boardId) == FALSE)))
                    {
                        *pdropPkt = TRUE;
                    }
                    else
                    {
                        // both master and slave can receive DATA pkts
                        if (ConnState[boardId].state == GCI_STATE_OPEN)
                        {
                            ConnState[boardId].my_numPktRx++;
                            if(((ConnState[boardId].my_numPktRx % 64) == 0) && 
                                (ConnState[boardId].my_numPktRx> 0))
                            {
                                /*printf("\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b"
                                       				"\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b");*/
                                if (ConnState[boardId].testType == HALF_DUPLEX_TEST)
                                {
                                    //printf("Received %li ETH Data frames", 
                                     //   ConnState[boardId].my_numPktRx);
                                }
                                else
                                {
                                    //printf("Sent %li Data frames, rcvd %li Data frames", 
                                     //   ConnState[boardId].my_numPktTx, 
                                      //  ConnState[boardId].my_numPktRx);
                                }
                            }
                            if ((stationType == SLAVE_STATION) && 
                                (ConnState[boardId].testType == FULL_DUPLEX_TEST))
                                // transmit the DATA pkt back to sender if 
                                // we are slave and this is a Full Duplex test
                                nxtPktType = CMD_DATA_PKT;
                            else
                            {
                                *pdropPkt = TRUE;
                            }
                        } else
                        {
                            *pdropPkt =  TRUE;   // bad state
                        }
                    }
                }
            } else
            {
                *pdropPkt =  TRUE;       // invalid pkt
            }

            if (!*pdropPkt)
            {
                // if it gets here, it means that we need to transmit a response 
                // pkt or it's a DATA pkt and we need to transmit it back in
                // the FULL Duplex test mode 
                // swap src and dest addresses of MAC layer
                memcpy(tmpAddr, pEthHdr->dstaddr, MAC_ADDR_LEN);
                memcpy(pEthHdr->dstaddr, pEthHdr->srcaddr, MAC_ADDR_LEN);
                memcpy(pEthHdr->srcaddr, tmpAddr, MAC_ADDR_LEN);
                pEthHdr->ethtype = ETH_TYPE_GREENVITY;
                // Greenvity's
                pGCIHdr->pktType = nxtPktType;
                if (nxtPktType == CMD_DATA_PKT)
                {
                    // DATA pkt
                    pEthTxFrmSwDesc->frmLen =  frmLen; // pkt len
                    pEthTxFrmSwDesc->cpArr[0].len = descLen; // CP len
                } else
                {
                   // CONN RESPONSE or ACK pkt
                   // pkt len is ETH+GREENVITY headers
                    pEthTxFrmSwDesc->frmLen = pEthTxFrmSwDesc->cpArr[0].len = 
                            sizeof(sEth2Hdr) + sizeof(sEthGCIHdr); 
                }
                pEthTxFrmSwDesc->cpArr[0].offsetU32 = 0;
                pEthTxFrmSwDesc->cpArr[0].cp        = pRxPktQCPDesc->s.cp;
                pEthTxFrmSwDesc->cpCount++;

#ifdef MEM_PROTECTION
				if (HHAL_CP_Put_Copy(pRxPktQCPDesc->s.cp, cellAddr, sizeof(sEth2Hdr)+sizeof(sEthGCIHdr)) == STATUS_FAILURE)
				{
					printf("isGCIpkt: Failed to put a copy of CP. Return FALSE\n");
				}
#endif
            }
        } else GCIpkt = FALSE;
    } else GCIpkt = FALSE;

    *pRetRcvPkt = rcvPkt;
    *pRetId = boardId;
    return(GCIpkt);
}       

#endif //#ifdef DO_GV_ETH_TEST

#ifdef HPGP_HAL_TEST
void Host_RxHandler(sCommonRxFrmSwDesc* pRxFrmDesc)

#else

void Host_RxHandler(sHaLayer *pHal, sCommonRxFrmSwDesc* pRxFrmDesc)

#endif

{


    uRxPktQDesc1*      pRxPktQ1stDesc;
    uRxPktQCPDesc*     pRxPktQCPDesc;
//    volatile u8 XDATA * cellAddr;
	u8					*cellAddr;
    u16                frmLen;
    u8                 i;
    u16                tmpFrmLen = 0;
    u8                 tmpdescLen = 0;
    eStatus status = STATUS_SUCCESS;
    u16                retriesCnt = 0;
    u16                lclPrintCount = 0;
    u16                lclCount = 0;
    u8                  GreenvityPkt = FALSE;
    u8                  leave = FALSE;
    u8                  myStationId = 0xff;
    u8                  rcv_pktType = 0;
#ifdef UM
    hostHdr_t			*pHybrii;
   
    u8					isCmd= 0;
	 sEth2Hdr   *pktEthHdr;
    sLinkLayer    *linkl = (sLinkLayer *)HPGPCTRL_GetLayer(HP_LAYER_TYPE_LINK);
    sStaInfo      *staInfo = LINKL_GetStaInfo(linkl);
#endif
    sHpgpHalCB			*hhalCb;
#ifdef MEM_PROTECTION
	u8					cp_localBuf[HYBRII_CELLBUF_SIZE];	// local CP buffer
#endif      
	u8				   dropFrame = 0;

    u8					isEthData = 0;


    u8 dstTei =0;

    u8 brdcast[6] = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
    //  sPlcTxFrmSwDesc plcTxFrmSwDesc;
    sSwFrmDesc    plcTxFrmSwDesc;
    sSwFrmDesc    ethTxFrmSwDesc;
	u8 alingOffset = 0;
    u8 payloadOffset = 0;
    u8 FIFOCtrl=0;

 	u8 cp = 0;
    if(hostDetected == FALSE) //hostIntf == HOST_INTF_NO)
    {
        hostIntf = HOST_INTF_ETH;
    }
//#ifdef UM
    hostDetected = TRUE;
//#endif
    memset (&plcTxFrmSwDesc, 0x00, sizeof(sSwFrmDesc));

    plcTxFrmSwDesc.cpCount        = 0;
    ethTxFrmSwDesc.cpCount        = 0;
//#ifdef HYBRII_HPGP	
#ifdef HPGP_HAL_TEST       
    hhalCb = &gHpgpHalCB;
#else   
       
    hhalCb = pHal->hhalCb;
#endif
//#endif //HYBRII_HPGP    

    pRxPktQ1stDesc = &pRxFrmDesc->hdrDesc;
    pRxPktQCPDesc  = &pRxFrmDesc->firstCpDesc;
    frmLen = pRxPktQ1stDesc->s.frmLenHi;
    frmLen = frmLen<<PKTQDESC1_FRMLENHI_POS | pRxPktQ1stDesc->s.frmLenLo;
	plcTxFrmSwDesc.txPort = pRxPktQ1stDesc->s.dstPort;
	plcTxFrmSwDesc.rxPort = pRxPktQ1stDesc->s.srcPort;	

    gEthHalCB.TotalRxFrmCnt++;
    gEthHalCB.TotalRxBytesCnt += frmLen;

	if (hostIntf == HOST_INTF_ETH) 
	{
	    if (frmLen > MAX_ETH_BUFF || frmLen < MIN_ETH_BUFF)
	    {
	    	
	        hal_common_free_frame(pRxFrmDesc);
            gHpgpHalCB.halStats.HtoPswDropCnt++;
	        return;
	    }
#if 0   //[YM] temp comment out pause function
    	// Check for FIFO Overrun reg. If it's set, send a PAUSE pkt out
    	FIFOCtrl     = ReadU8Reg(ETHMAC_FIFOCTL_REG);
    	if (FIFOCtrl & 4)
		{
        	/* Rcv FIFO Overrun flag is set, send ETH Pause pkt out */
        	EHAL_EthSendPause();
		}
#endif		
	}
    
	if (hostIntf == HOST_INTF_SPI) {
		cellAddr = CHAL_GetAccessToCP(pRxFrmDesc->cpArr[0]);

	} else if (hostIntf == HOST_INTF_ETH) {
#ifdef UM
#ifdef MEM_PROTECTION
		// copy cp to local buf
		if (HHAL_CP_Get_Copy(pRxPktQCPDesc->s.cp, &cp_localBuf[0], HYBRII_CELLBUF_SIZE) 
				== STATUS_FAILURE)
		{
			printf("Host_RxHandler: Failed to make a copy of CP. Return\n");
	        hal_common_free_frame(pRxFrmDesc);
	        return;
		}
		cellAddr = &cp_localBuf[0];
#else
		cellAddr = CHAL_GetAccessToCP(pRxFrmDesc->cpArr[0]);
#endif
#endif
	} 
	else if(hostIntf == HOST_INTF_UART)
	{
		cellAddr = CHAL_GetAccessToCP(pRxPktQCPDesc->s.cp);
	} 
    if(frmLen < HYBRII_CELLBUF_SIZE)
    {
        tmpdescLen = frmLen;
    }
    else
    {
        tmpdescLen = HYBRII_CELLBUF_SIZE;
    }

    
#ifdef DEBUG_DATAPATH
    if (pktDbg)
    {
        FM_Printf(FM_ERROR, "\n host rx\n");
        for( lclPrintCount=0; lclPrintCount<tmpdescLen; lclPrintCount++ )
        {
            FM_Printf(FM_ERROR, "0x%02bX ", *(cellAddr+lclPrintCount));
        }
        FM_Printf(FM_ERROR, "\n end \n");

    }
#endif

    tmpFrmLen = tmpdescLen;


#ifdef UM
    	staInfo = LINKL_GetStaInfo(linkl);
    // Check for MAC SAP pkt
   
       pktEthHdr = ((sEth2Hdr*)cellAddr);
              
        if (pktEthHdr->ethtype == 0x88E1)
        {            
            pHybrii = (hostHdr_t*)(((u8*)cellAddr) + sizeof(sEth2Hdr));        
     
            #if 0
            
            alingOffset = (u8)pHybrii->rsvd;  

            if( alingOffset > 3)
            {
        //      FM_Printf(FM_ERROR, "Invalid alingoffset : %bu, %d\n", alingOffset, pHybrii->rsvd);
                hal_common_free_frame(pRxFrmDesc);
                return;
            }
            
            #endif           
                        
            
            if ((pHybrii->type == CONTROL_FRM_ID) ||
                (pHybrii->type == MGMT_FRM_ID))
            {

				if (pHybrii->protocol == HPGP_MAC_ID 
#ifdef HYBRII_802154					
					|| pHybrii->protocol == IEEE802_15_4_MAC_ID
#endif					
																)
				{
					Host_MgmtCmdRxHandler(pRxFrmDesc,frmLen,pHybrii->type);		      
				}
            }
           
			hal_common_free_frame(pRxFrmDesc);
			return;


        }
        else
#endif			
		{

#ifdef UM		

			if (!MCTRL_IsAssociated())
			{

				dropFrame = 1;

			}
			else
			if( hhalCb->devMode == DEV_MODE_STA)
			{
				if(!staInfo->staStatus.fields.authStatus)
				{
					
					dropFrame = 1;
					FM_Printf(FM_ERROR,"\nsuD");
				}
			}
				
			
				
#endif


#if 1
			if (datapath_IsQueueFull(PLC_DATA_QUEUE)
					== TRUE)
			{			
#ifdef DEBUG_DATAPATH						
				if (sigDbg)
				FM_Printf(FM_ERROR,"q f\n");
#endif				
				dropFrame = 1;
			}
			
			if ((host_intf_max_cp + pRxFrmDesc->cpCount)> HOST_TO_PLC_MAX_CP)
			{									
#ifdef DEBUG_DATAPATH					
				if (sigDbg)
				FM_Printf(FM_ERROR,"max cp %bu\n",(pRxFrmDesc->cpCount + host_intf_max_cp));	
#endif					
				dropFrame = 1;

			}

			if (dropFrame)
			{
				hal_common_free_frame(pRxFrmDesc); // drop frame
#ifdef DEBUG_DATAPATH
				if (sigDbg)
				FM_Printf(FM_USER, "drop frame\n");
#endif			
                gHpgpHalCB.halStats.HtoPswDropCnt++;
				return;
			}
			
#endif

			
#ifdef UM

			isEthData = 1;
			
		    /* Check to drop the Ethernet frame based on peer's connection status */
		   /* !memcmp(pktEthHdr->dstaddr, brdcast, sizeof(brdcast)) */
		    if ((pktEthHdr->dstaddr[0] & 0x01))
		    {

		            /*
		                            Note : dstTei should be set to 0xFF for broadcast frame.
		                            Has to be revisited. 0xFF was getting dropped by receiver

		                            */

		        dstTei = 0xFF;

		    }
		    else
		    {
		        sScb* dstScb = NULL;
		        sLinkLayer *linkLayer = HPGPCTRL_GetLayer(HP_LAYER_TYPE_LINK);
		        sCnam *ccoNam = &linkLayer->ccoNam;
		        u8 devmode = LINKL_GetMode(linkLayer);
		    
		        dstScb = CRM_FindScbMacAddr(&pktEthHdr->dstaddr);

		        if (!dstScb)
		        {
		            dropFrame = 1;

		        }
		        else
		        {
		            if(devmode == LINKL_STA_MODE_STA)
		            {
		                if(!staInfo->staStatus.fields.authStatus)
		                {
		                    dropFrame = 1;
		                }
		            }

		            else if(devmode == LINKL_STA_MODE_CCO)
		            {
		                if(!dstScb->staStatus.fields.authStatus)
		                {
		                    dropFrame = 1;
		                }
		            }
		            else
		            {
		                dropFrame = 1;
		            }
#ifdef ROUTE
                    if(dstScb->lrtEntry.routeIsInvalid == TRUE || dstScb->lrtEntry.routeOnHold == TRUE)
                    {
                        dropFrame = 1;
                    }
                    else
                    {
                        dstTei = dstScb->lrtEntry.nTei;
                    }
#else
		            dstTei = dstScb->tei;
#endif
		        }


		    }


#else

	isEthData = 1;
	dstTei = 1;

#endif

		}


#ifdef UM
     isEthData = 1;
#endif		
	 
#ifdef HYBRII_ETH
    // now examine the pkt once more to see if it's
    // a Greenvity test pkt
#ifdef DO_GV_ETH_TEST // make this test compilable to save time to examine ETH header
    if ((GreenvityPkt = isGCIpkt(pRxPktQCPDesc, &ethTxFrmSwDesc, frmLen, 
            tmpdescLen, &myStationId, &rcv_pktType, &leave)) == TRUE)
    {
    }
#else
	GreenvityPkt = FALSE;
#endif //HYBRII_ETH 
#endif

    if (leave
#ifdef UM
        || dropFrame
#endif
    ) {

		//FM_Printf(FM_USER, "drop x\n");
        // free the received pkt and return
        hal_common_free_frame(pRxFrmDesc);
        gHpgpHalCB.halStats.HtoPswDropCnt++;
#ifdef DEBUG_DATAPATH			
		if(sigDbg)
				FM_Printf(FM_ERROR,"drop\n");
#endif		
        return;
    }

    if ((eth_plc_bridge
#ifdef UM
    || (isEthData == 1)
#endif
        ) && !GreenvityPkt)
    {
	    
	    plcTxFrmSwDesc.cpArr[0].offsetU32 = 0;
	    plcTxFrmSwDesc.cpArr[0].len       = tmpdescLen;
	
		plcTxFrmSwDesc.cpArr[0].cp    = pRxFrmDesc->cpArr[0];
    	plcTxFrmSwDesc.cpCount++;

//#ifdef HYBRII_HPGP
#ifdef HPGP_HAL_TEST
    plcTxFrmSwDesc.frmInfo.plc.eks =  gNekEks; //HPGP_UNENCRYPTED_EKS;

    hhalCb = &gHpgpHalCB;
#else


    hhalCb = pHal->hhalCb;
	plcTxFrmSwDesc.frmInfo.plc.eks = staInfo->nekEks;//HPGP_UNENCRYPTED_EKS;
#endif
//#endif //HYBRII_HPGP

    plcTxFrmSwDesc.frmInfo.plc.bcnDetectFlag  = REG_FLAG_SET;// REG_FLAG_CLR;
    plcTxFrmSwDesc.frmInfo.plc.clst = HPGP_CONVLYRSAPTYPE_ETH;

    plcTxFrmSwDesc.frmType        = HPGP_HW_FRMTYPE_MSDU;

	if (dstTei == 0xFF)
	{
		plcTxFrmSwDesc.frmInfo.plc.mcstMode		 = HPGP_MCST;
		plcTxFrmSwDesc.frmInfo.plc.phyPendBlks	 = HPGP_PPB_MCFRPT;
#ifdef PROXY_BCST
		if(proxyBcst == 1)
		{
			sScb *dstScb =NULL;	
			dstScb = getLeastRssiScb();
			if(dstScb != NULL)
			{
				dstTei = dstScb->tei;
			}
			else
			{
				dstTei = 0xFF;
			}
		}
#endif
	}
	else
	{
		plcTxFrmSwDesc.frmInfo.plc.mcstMode		 = HPGP_UCST;  // Unicast
	}

	
#ifdef UM
    /*
    we need to fetch scb and then populate TEIs.
    */
    if(linkl->mode == LINKL_STA_MODE_CCO)
    {

//		FM_Printf(FM_USER, "f cc\n");
#ifdef CCO_FUNC		            
        plcTxFrmSwDesc.frmInfo.plc.dtei 		  = dstTei;
        plcTxFrmSwDesc.frmInfo.plc.stei 		  = staInfo->ccoScb->tei;
#endif
    }
    else
    {
#ifdef STA_FUNC
        plcTxFrmSwDesc.frmInfo.plc.dtei 		  = dstTei;//staInfo->ccoScb->tei;
        plcTxFrmSwDesc.frmInfo.plc.stei 		  = staInfo->tei;
#endif
    }


#else
   	plcTxFrmSwDesc.frmInfo.plc.dtei           = hhalCb->remoteTei;
    plcTxFrmSwDesc.frmInfo.plc.stei           = hhalCb->selfTei;//HYBRII_DEFAULT_TEISTA;

#endif
	
    plcTxFrmSwDesc.frmLen         = frmLen;
#if 0
    plcTxFrmSwDesc.frmInfo.plc.plid = 0;   //[YM] This line of code has to be changed base on differnet QoS priority


    if (plcTxFrmSwDesc.frmInfo.plc.plid == 0)
        plcTxFrmSwDesc.frmInfo.plc.phyPendBlks    = HPGP_PPB_CAP0;
	else
		plcTxFrmSwDesc.frmInfo.plc.phyPendBlks    = HPGP_PPB_CAP123;

   
#if 1 //def HPGP_HAL_TEST
    plcTxFrmSwDesc.frmInfo.plc.stdModeSel     = STD_ROBO_TEST; // std robo
#endif
    plcTxFrmSwDesc.frmInfo.plc.dt_av = HPGP_DTAV_SOF;
    plcTxFrmSwDesc.frmInfo.plc.saf = 1;

#endif
    }

    // Read second and subsequent CP descriptors
    for( i=1 ; i< pRxFrmDesc->cpCount ; i++ )
    {
#ifdef UM
#ifdef MEM_PROTECTION
		// copy cp to local buf
		if (HHAL_CP_Get_Copy(pRxFrmDesc->cpArr[i], &cp_localBuf[0], HYBRII_CELLBUF_SIZE) == STATUS_FAILURE)
		{
			printf("Host_RxHandler: Failed to make a copy of CP %bu. Return\n", pRxFrmDesc->cpArr[i]);
	        hal_common_free_frame(pRxFrmDesc);
	        return;
		}
		//cellAddr = &cp_localBuf[0];// Kiran. Commented as no one uses cellAddr
#else
        //cellAddr = CHAL_GetAccessToCP(pRxFrmDesc->cpArr[i]);// Kiran. Commented as no one uses cellAddr
#endif
#endif

        if((frmLen - tmpFrmLen) > HYBRII_CELLBUF_SIZE)
            tmpdescLen = HYBRII_CELLBUF_SIZE;
        else
            tmpdescLen = frmLen - tmpFrmLen;

        if (GreenvityPkt)
        {
#ifdef HYBRII_ETH
            // our test pkt
            if ((rcv_pktType == CMD_DATA_PKT) && 
                (ConnState[myStationId].testType == FULL_DUPLEX_TEST))
            {
                // only DATA pkts can have more than 1 CP
                // and we're going to xmit this DATA pkt back out
                // in FULL DUPLEX mode
                ethTxFrmSwDesc.cpArr[i].offsetU32 = 0;
                ethTxFrmSwDesc.cpArr[i].len       = tmpdescLen;
                ethTxFrmSwDesc.cpArr[i].cp        = pRxFrmDesc->cpArr[i];
                ethTxFrmSwDesc.cpCount++;
                tmpFrmLen += tmpdescLen;
            }
#endif
        } else if((1 == eth_plc_bridge)
#ifdef UM
        || (isEthData == 1)
#endif
        )
        {
            //either bridging would be on or mac sap would be on

			plcTxFrmSwDesc.cpArr[i].offsetU32 = 0;
            plcTxFrmSwDesc.cpArr[i].len       = (u8)tmpdescLen; 
            plcTxFrmSwDesc.cpArr[i].cp    = pRxFrmDesc->cpArr[i];
            plcTxFrmSwDesc.cpCount++;
            tmpFrmLen += tmpdescLen;
        }

    } // for i..


    if(eth_plc_bridge || GreenvityPkt
#ifdef UM
        || (isEthData == 1)
#endif
        )
    {
        // Transmit the frame
        retriesCnt=0;
#if 0 //def HYBRII_ETH

        if(GreenvityPkt)
        {
            // Greenvity test pk, xmit it back out via ETH
            do
            {                   
            // Greenvity test pk, xmit it back out via ETH
            status = EHAL_EthTxQWrite(&ethTxFrmSwDesc);

            // check for pending Tx and process it.
            if( status != STATUS_SUCCESS)
            {
            // TODO try for ETH_PLC_TX_RETRIES number of times, if not success then break
            if(retriesCnt >= ETH_PLC_TX_RETRIES)
            {
                break;
            }
            retriesCnt++;
            }
            }while(status != STATUS_SUCCESS);

            if(status == STATUS_SUCCESS)
            {
            gHpgpHalCB.halStats.CurTxTestFrmCnt++;
            if ((rcv_pktType == CMD_DATA_PKT))
            // only the slave transmits the DATA pkt in the ETH_RCV ISR
            ConnState[myStationId].my_numPktTx++;
            }
            else
            {
            FM_Printf(FM_ERROR,"ETH, PLC or MAC SAP tx failed\n");
            // If Tx failed, free the rcv pkt
            hal_common_free_frame(pRxFrmDesc);
            }

        }
        else
#endif

		{			
			//FM_Printf(FM_USER, "hr\n");
			fwdAgent_handleData(&plcTxFrmSwDesc);
		}
        // if MAC SAP or Bridging is ON, send it to PLC
        
        // end of bridge or mac case
    } 
	else
    {
        // pkt is not for ETH testing nor bridging nor MAC SAP, free the CP
        hal_common_free_frame(pRxFrmDesc);
        gHpgpHalCB.halStats.HtoPswDropCnt++;
    } 

//        CHAL_DecrementReleaseCPCnt(pRxFrmDesc->cpArr[i]);
        //FM_Printf(FM_MINFO,"EHAL_EthRxIntHandler2: Releasing CP %02bX \n",pRxFrmDesc->cpArr[i]);
}



#ifdef UM

void Host_SendIndication(u8 eventId, u8 protocol, u8 *payload, u8 length)
{
	hostEventHdr_t	 *pHostEvent;
	hostHdr_t        *pHostHdr;
	sEvent	  xdata  *event = NULL;
#ifdef NO_HOST
	gv701x_app_msg_hdr_t* event_msg_hdr = NULL;
#endif
	sNma* nma = HOMEPLUG_GetNma();
#ifdef ROUTE    
    u8 i;
    if(HOST_EVENT_ROUTE_VALID== eventId)
    {
        FM_Printf(FM_ROUTE,"ROUTE Valid: TEI=%bu NTEI=%bu NHOP=%bu\n",payload[0],payload[1],payload[2]);
    }
    else if(HOST_EVENT_ROUTE_INVALID== eventId)
    {
        FM_Printf(FM_ROUTE,"ROUTE Invalid: TEI=%bu\n",payload[0]);
    }
    else if(HOST_EVENT_ROUTE_CHANGE == eventId)
    {
        FM_Printf(FM_ROUTE,"ROUTE Change: TEI=%bu NTEI=%bu NHOP=%bu\n",payload[0],payload[1],payload[2]);
    }
    else if(HOST_EVENT_ROUTE_HOLD== eventId)
    {
        FM_Printf(FM_ROUTE,"ROUTE Hold: TEIs = ");
        for(i = 0; i < length; i++)
        {
            FM_Printf(FM_ROUTE,"%bu ", payload[i]);
        }
        FM_Printf(FM_ROUTE,"\n");
    }
#endif

#ifdef NO_HOST
	event = GV701x_EVENT_Alloc(sizeof(hostHdr_t) + sizeof(hostEventHdr_t) + sizeof(gv701x_app_msg_hdr_t) +
								length, sizeof(hostHdr_t));
#else
    event = EVENT_Alloc(sizeof(hostHdr_t) + length + sizeof(hostEventHdr_t), sizeof(hostHdr_t));
#endif

    if(event != NULL)
    {
    	event->eventHdr.eventClass = EVENT_CLASS_CTRL;
		event->eventHdr.type = eventId;
#ifdef NO_HOST
		event_msg_hdr = (gv701x_app_msg_hdr_t*)event->buffDesc.dataptr; 	
		event_msg_hdr->src_app_id = APP_FW_MSG_APPID;
		if(protocol == HPGP_MAC_ID)
			event_msg_hdr->dst_app_id = nma->msg_hdr.src_app_id;
#ifdef HYBRII_802154		
		if(protocol == IEEE802_15_4_MAC_ID)
		{
			event_msg_hdr->dst_app_id = mac_host_db.msg_hdr.src_app_id;
		}
#endif		
		if(protocol == SYS_MAC_ID)
		{
			if(eventId == HOST_EVENT_APP_TIMER)
			{
				hostTimerEvnt_t* tmr_event = (hostTimerEvnt_t*)payload;
				event_msg_hdr->dst_app_id = tmr_event->app_id;
			}
			else if(eventId == HOST_EVENT_APP_CMD)
			{
				event_msg_hdr->dst_app_id = msg_hdr_app_cmd.dst_app_id;
			}
			else
				event_msg_hdr->dst_app_id = APP_BRDCST_MSG_APPID;
		}
		event_msg_hdr->len = sizeof(hostHdr_t) + sizeof(hostEventHdr_t) + length;			
		event_msg_hdr->type = APP_MSG_TYPE_FW;
#endif	

#ifdef NO_HOST
		pHostHdr = (hostHdr_t*)(event_msg_hdr + 1);
#else
		pHostHdr = (hostHdr_t*)event->buffDesc.dataptr;
#endif
    	
    	pHostEvent = (hostEventHdr_t*)(pHostHdr + 1);

    	pHostHdr->type = EVENT_FRM_ID;
		pHostHdr->protocol = protocol;
    	pHostHdr->rsvd = 0;
    	pHostHdr->length = sizeof(hostEventHdr_t) +  length;
#ifndef NO_HOST
    	pHostHdr->length = HTONHS(pHostHdr->length);
#endif
    	pHostEvent->eventClass = EVENT_CLASS_CTRL;
		pHostEvent->type = eventId;
		
    	if (payload)
    		memcpy ((u8*)(pHostEvent + 1),	payload, length);

#ifdef NO_HOST
		event->buffDesc.datalen = sizeof(hostHdr_t) +  
								  sizeof(hostEventHdr_t) + 
								  sizeof(gv701x_app_msg_hdr_t) + length;		
#else
    	event->buffDesc.datalen = sizeof(hostHdr_t) +  
    							  sizeof(hostEventHdr_t) + length;
#endif

    	SEND_HOST_EVENT(event);
    }

return;

}



void Host_MgmtCmdRxHandler(sCommonRxFrmSwDesc* pRxFrmDesc,
									  u16 frmLen, u8 frmType)
{
	u8					*cellAddr;
	u8 dropFrame = 0;
	u16 			   lclPrintCount = 0;
	u8 				    *cmdPtr;
	u16 			   lclCount = 0;
	u8				   copylen;
	u16 			   remainingLen;
	u8 i;
#ifdef UM
	u8 xdata ufrm[MAX_HOST_CMD_LENGTH];
#endif
	u8 payloadOffset;
#ifdef MEM_PROTECTION
	u8					cp_localBuf[HYBRII_CELLBUF_SIZE];	// local CP buffer
#endif      

	if ((frmLen <= MAX_FRAME_LEN) && ((CONTROL_FRM_ID == frmType) || (MGMT_FRM_ID == frmType)))
	{

		if(frmLen < HYBRII_CELLBUF_SIZE)
		{
			copylen = frmLen;
		}
		else
		{
			copylen = HYBRII_CELLBUF_SIZE;
		}

		remainingLen = frmLen;

		if (pRxFrmDesc->cpCount == 1) // If more than one cp are present then copy them to buffer
		{
			cmdPtr = CHAL_GetAccessToCP(pRxFrmDesc->cpArr[0])+ sizeof(sEth2Hdr);
			// First cell pointer contains ethernet header + payload. We need to offset ethernet header
			lclCount =  frmLen;
		}
		else
		{

			// Read second and subsequent CP descriptors
			for( i=0; i< pRxFrmDesc->cpCount ; i++ )
			{
				payloadOffset = (i == 0)?sizeof(sEth2Hdr):0;
#ifdef HPGP_MAC_SAP
#ifdef MEM_PROTECTION
				// copy cp to local buf
				if (HHAL_CP_Get_Copy(pRxFrmDesc->cpArr[i], &cp_localBuf[0], HYBRII_CELLBUF_SIZE) 
				== STATUS_FAILURE)
				{
				printf("Host_RxHandler: Failed to make a copy of CP %bu. Return\n", pRxFrmDesc->cpArr[i]);
				hal_common_free_frame(pRxFrmDesc);
				return;
				}
				cellAddr = &cp_localBuf[0];
#else
				cellAddr = CHAL_GetAccessToCP(pRxFrmDesc->cpArr[i]);
#endif
#endif

				for( lclPrintCount=0; lclPrintCount< ( copylen - payloadOffset);
						lclPrintCount++ )
				{
					ufrm[lclCount++] = *(cellAddr + payloadOffset + lclPrintCount);
				}
				
				remainingLen = remainingLen - copylen;

				if(remainingLen > HYBRII_CELLBUF_SIZE)
					copylen = HYBRII_CELLBUF_SIZE;
				else
					copylen = remainingLen; 				
	  		}

			cmdPtr = ufrm;
	   }
#ifndef NO_HOST		
	  // Command Handling
		NMA_RecvMgmtPacket((hostHdr_t*)cmdPtr, lclCount);
#endif
	}


}



void datapath_hostTransmitFrame(u8* TxByteArr, u16 frameSize)
{
    u16              dataIdx;
    u16              curFrmLen;
    u16              crc16 = 0;
    u8               cpIdx;
    eStatus          status;
    sSwFrmDesc  hostTxFrmSwDesc;
    u8 ethhdr[14];
    u8              firstCp = 0;
    u8 i;

		/*Compiler warning suppression*/
		i = i;
	
    if (frameSize == 0 || hostIntf == HOST_INTF_NO)
    {
        return;
    }
    //EHT_FillEtherHeader(TxByteArr);
    if(hostIntf == HOST_INTF_SPI)
    {
#ifdef HYBRII_SPI
        curFrmLen              = frameSize + sizeof(hybrii_tx_req_t);// + SPI_CRC_LEN; // [Kiran] For Hybrii B software CRC not required
        EHT_FillEtherHeader(ethhdr);
        curFrmLen              += sizeof(sEth2Hdr);

#endif   //HYBRII_SPI
    }
    else if(hostIntf == HOST_INTF_ETH)
    {
        EHT_FillEtherHeader(ethhdr);
        curFrmLen              = frameSize + sizeof(sEth2Hdr);    
    }
#ifdef UART_HOST_INTF 
	else if(hostIntf == HOST_INTF_UART)
    {
        curFrmLen              = frameSize ;//+ sizeof(sEth2Hdr);    
    }//Kiran
#endif// UART_16550
    dataIdx                = 0;
    cpIdx                  = 0;
    hostTxFrmSwDesc.frmLen  = curFrmLen;
    hostTxFrmSwDesc.cpCount = 0;

    while (curFrmLen)
    {
        u8                  cp;
        volatile u8 xdata * cellAddr;
        u8                  actualDescLen;

        status = CHAL_RequestCP(&cp);
        if (status != STATUS_SUCCESS)
        {
            return;
        }
        cellAddr = CHAL_GetAccessToCP(cp);
        if (curFrmLen > HYBRII_CELLBUF_SIZE)
        {
            actualDescLen = HYBRII_CELLBUF_SIZE;
        }
        else
        {
            actualDescLen = curFrmLen;
        }
        //memcpy(cellAddr, &TxByteArr[dataIdx], actualDescLen);
        if(firstCp == 0)
        {
          //  hostHdr_t *pHybrii;
            u8 hdrLen;
            if(hostIntf == HOST_INTF_SPI)
            {
#ifdef HYBRII_SPI
                hdrLen = sizeof(hybrii_tx_req_t);
                actualDescLen = hdrLen;

#endif  //HYBRII_SPI
            }
            else if(hostIntf == HOST_INTF_ETH)
            {
                hdrLen = (sizeof(sEth2Hdr));                
                memcpy(cellAddr, ethhdr, sizeof(sEth2Hdr));
                memcpy((cellAddr + hdrLen), &TxByteArr[dataIdx], (actualDescLen - hdrLen));
            }
#ifdef UART_HOST_INTF 
			else if(hostIntf == HOST_INTF_UART)
		    {
		     	hdrLen = 0;//UART driver creates required header for Formatted mode   
                memcpy(cellAddr, &TxByteArr[dataIdx], actualDescLen);
		    }//Kiran
#endif// UART_HOST_INTF
            firstCp = 1;
            dataIdx += (actualDescLen - hdrLen);

        }
        else
        {
            if(hostIntf == HOST_INTF_SPI)
            {
                u8 ethHdrLen = sizeof(sEth2Hdr);
				if(cpIdx == 1)
				{
	                memcpy(cellAddr, ethhdr, ethHdrLen);
	                memcpy(&cellAddr[ethHdrLen], &TxByteArr[dataIdx], actualDescLen - ethHdrLen);		
				}
				else
				{
					memcpy(cellAddr, &TxByteArr[dataIdx], actualDescLen);
				}
            }
            else
            {
                memcpy(cellAddr, &TxByteArr[dataIdx], actualDescLen);
            }
            
#if 0			
            if(hostIntf == HOST_INTF_SPI)
            {
                if(curFrmLen <= HYBRII_CELLBUF_SIZE && curFrmLen > 1)
                {
                    actualDescLen -= SPI_CRC_LEN;               
				}
                else if(curFrmLen == (HYBRII_CELLBUF_SIZE + 1))
                {
                    actualDescLen -= 1;
                }
                else if(curFrmLen == 1)
                {
                    actualDescLen -= 1;
                }
                for (i = 0; i < actualDescLen; i++) {
                    crc16 = crc_ccitt_update(crc16, cellAddr[i]);
                }
            
                if(curFrmLen <= HYBRII_CELLBUF_SIZE && curFrmLen > 1)
                {
                    cellAddr[i] = crc16 & 0xFF;
                    i++;
                    cellAddr[i] = (crc16 >> 8) & 0xFF;
                    actualDescLen += SPI_CRC_LEN;
                }
                else if(curFrmLen == (HYBRII_CELLBUF_SIZE + 1))
                {
                    cellAddr[i] = crc16 & 0xFF;
                    actualDescLen += 1;
                }
                else if(curFrmLen == 1)
                {
                    cellAddr[i] = (crc16 >> 8) & 0xFF;
                    actualDescLen += 1;
                }
            }
#endif
			if(hostIntf == HOST_INTF_SPI)
       		{
				if(cpIdx == 1)
				{
					dataIdx += (actualDescLen - sizeof(sEth2Hdr));
				}
				else
				{
					dataIdx += actualDescLen;
				}
        	}
			else
			{
				dataIdx += actualDescLen;
			}
        }
        hostTxFrmSwDesc.cpArr[cpIdx].offsetU32 = 0;
		if(hostIntf == HOST_INTF_SPI)
        {
        	hostTxFrmSwDesc.cpArr[cpIdx].len  = actualDescLen;
			curFrmLen -= actualDescLen;
		}
        else
        {
			hostTxFrmSwDesc.cpArr[cpIdx].len  = actualDescLen;
			curFrmLen -= actualDescLen;
		}
        hostTxFrmSwDesc.cpArr[cpIdx].cp = cp;
        cpIdx++;
        hostTxFrmSwDesc.cpCount++;     
    }
    
    numHostCPs += hostTxFrmSwDesc.cpCount;
    if(hostIntf == HOST_INTF_SPI)
    {
#ifdef HYBRII_SPI

        hostTxFrmSwDesc.frmLen -= sizeof(hybrii_tx_req_t);
        hostTxFrmSwDesc.txPort = PORT_SPI;
#endif  //HYBRII_SPI
    }
    else if(hostIntf == HOST_INTF_ETH)
    {
        hostTxFrmSwDesc.txPort = PORT_ETH;
    }
    else if(hostIntf == HOST_INTF_UART)
    {
        hostTxFrmSwDesc.txPort = PORT_UART;
    }
    else
    {
        for (cpIdx = 0; cpIdx < hostTxFrmSwDesc.cpCount; cpIdx++)
        {
            CHAL_DecrementReleaseCPCnt(hostTxFrmSwDesc.cpArr[cpIdx].cp);
        }
        return;
    }
	datapath_writeHostIntf(&hostTxFrmSwDesc);
	
	status = STATUS_SUCCESS;
	


/*    if(hostIntf == HOST_INTF_SPI)
    {
#ifdef HYBRII_SPI
        
        hal_spi_tx_cleanup ();
        hal_spi_rx_cleanup ();
        hostTxFrmSwDesc.frmLen -= sizeof(hybrii_tx_req_t);
        status = hal_spi_tx_dma_cp(hostTxFrmSwDesc.frmLen, &hostTxFrmSwDesc);
#endif
    }
    else if(hostIntf == HOST_INTF_ETH)
    {
        status = EHAL_EthTxQWrite(&hostTxFrmSwDesc);
    }
    */
    if (status == STATUS_FAILURE)
    {
        for (cpIdx = 0; cpIdx < hostTxFrmSwDesc.cpCount; cpIdx++)
        {
            CHAL_DecrementReleaseCPCnt(hostTxFrmSwDesc.cpArr[cpIdx].cp);
        }
        //FM_Printf(FM_ERROR, "\nCannot send Eth/SPI packet");
    }
    else
    {
        gEthHalCB.CurTxTestFrmCnt++;
        gEthHalCB.CurTxTestBytesCnt+= frameSize;
    }
}


bool datapath_transmitMgmtPlc()
{
	if (HAL_Proc(HOMEPLUG_GetHal()))
        return TRUE;
    else
        return FALSE;
}


#endif


