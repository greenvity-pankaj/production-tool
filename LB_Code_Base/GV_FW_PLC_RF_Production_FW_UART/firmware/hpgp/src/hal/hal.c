/** =========================================================
 *
 *  @file hal.c
 * 
 *  @brief Hardware Abstract Layer
 *
 *  Copyright (C) 2010-2012, Greenvity Communications, Inc.
 *  All Rights Reserved
 *  
 * ===========================================================*/

#include <stdlib.h>
#include <string.h>
#ifdef RTX51_TINY_OS
#include <rtx51tny.h>
#endif
#include "papdef.h"
#ifdef ROUTE
#include "hpgp_route.h"
#endif
#include "list.h"
#include "event.h"
#include "ism.h"
#include "fm.h"
#include "hpgpevt.h"
#include "hal.h"
#include "nma.h"
#ifdef HPGP_HAL
#include "hal_common.h"
#include "hal_hpgp.h"
#include "hal_eth.h"
#include "hal_tst.h"
#else
#include "sdrv.h"
#endif
#include "nma_fw.h"
#include "Linkl.h"
#include "hpgpapi.h"

#ifdef AUTH_AES
#include "aes.h"
#include "dmm.h"
#endif
#ifdef UM
#include "nma.h"
#endif
#include "hybrii_tasks.h"
#include "sys_common.h"
#include "event_fw.h"


#ifndef CALLBACK
void MUXL_RecvMgmtPacket(void *cookie, sEvent *event) __REENTRANT__;
#endif
eStatus HAL_XmitMacFrame(sHaLayer *hal, sTxDesc *txInfo, sBuffDesc *buffDesc);


#ifdef HPGP_HAL
#ifndef HAL_INT
u8 HAL_RxPoll(void *cookie);
#endif
#endif

#ifdef UKE
extern u8 genTek;
#endif

void HAL_SetTei(sHaLayer *hal, u8 tei)
{
		/* Compiler warning suppression */
		hal = hal;
#ifdef HPGP_HAL
    HHAL_SetTei(tei);
#else
    SHAL_SetTei(hal->shal, tei);
#endif

}

#ifndef HPGP_HAL
void HAL_SetDevMode(sHaLayer *hal, u8 devMode)
{
    hal->devMode = devMode;
}
#endif


eStatus HAL_TransmitBeacon(sHaLayer *hal, sTxDesc *txdesc, 
                           sBuffDesc *buffDesc, u8 bpstoOffset)
{
    sFrmCtrlBlk *fcb = NULL;
    buffDesc->dataptr -= sizeof(sFrmCtrlBlk);
    buffDesc->datalen += sizeof(sFrmCtrlBlk);
    fcb = (sFrmCtrlBlk *)(buffDesc->dataptr);
    fcb->access = hal->access; 
    fcb->snid = txdesc->snid;

    /* Add other param of sFrmCtrlBlk */ // [Prashant]
    fcb->bto[0] = PLC_PHY_TXLATENCY_FOR_TCC3;
    fcb->bto[1] = PLC_PHY_TXLATENCY_FOR_TCC3;
    fcb->bto[2] = PLC_PHY_TXLATENCY_FOR_TCC3;
    fcb->bto[3] = PLC_PHY_TXLATENCY_FOR_TCC3;

#if 1
    
#ifdef HPGP_HAL

    if (txdesc->frameType == BEACON_TYPE_CENTRAL)
    {
        HHAL_SetLMBcnBuf(buffDesc->dataptr, txdesc->frameType,
                     bpstoOffset);
    }
    else
    if (txdesc->frameType == BEACON_TYPE_DISCOVER)
    {
        
        HHAL_PlcBcnWrite(buffDesc->dataptr, txdesc->frameType, 
                            bpstoOffset);
    }

	return 0;

#endif

#else


#ifdef HPGP_HAL
    return HHAL_PlcBcnWrite(buffDesc->dataptr, txdesc->frameType, 
                            bpstoOffset);
#else
    txdesc->frameType = FRAME_TYPE_BEACON;
    return SHAL_Xmit(hal->shal, txdesc, buffDesc, NULL);
#endif


#endif
}


void HAL_BcnRxIntHandler(void *cookie)
{
    sHaLayer *hal = (sHaLayer *)cookie;
    sEvent  xdata *event = NULL;
    /* create an event for the beacon */
#ifdef SNIFFER
	if(eth_plc_sniffer == 1)
	{
		// Size for (Ethernet header  + Hybri header) added
		event = EVENT_BcnAlloc(BEACON_LEN + (sizeof(sEth2Hdr) + sizeof(hostHdr_t)), sizeof(sHpgpHdr));
	}
    else
#endif

	{
    	event = EVENT_BcnAlloc(BEACON_LEN, sizeof(sHpgpHdr));
	}
    if (event == NULL)
    {
        FM_Printf(FM_ERROR, "HAL:EAF\n");
        return;
    }

    event->eventHdr.eventClass = EVENT_CLASS_CTRL;
    event->eventHdr.type = EVENT_TYPE_CC_BCN_IND;

#ifdef HPGP_HAL
    if (HHAL_BcnRxIntHandler(hal, event) == STATUS_SUCCESS)
#else
    if (SHAL_BcnRxIntHandler(hal, event) == STATUS_SUCCESS)
#endif
    {
       /* deliver the beacon to the upper layer */
#ifdef CALLBACK
        hal->deliverMgmtPacket(hal->mgmtCookie, event);
#else
	//	EVENT_Free(event);

        MUXL_RecvMgmtPacket(hal->mgmtCookie, event);
#endif
    }
    else
    {
        EVENT_Free(event);
    }
}



eStatus  HAL_Transmit(sHaLayer *hal, sEvent *event) 
{
#if 0
	static u8 q = 0;
    /* post the event to the tx queue */

	if ( q == 1)
		return STATUS_SUCCESS;

	q = 1;

	#endif
    SLIST_Put(&hal->txQueue, &event->link);

	HAL_Proc(HOMEPLUG_GetHal());


	//FM_Printf(FM_USER,"q\n");
	os_set_ready(HYBRII_TASK_ID_FRAME);
    return STATUS_SUCCESS;
}

eStatus  HAL_ProcXmit(sHaLayer *hal, sEvent *event) 
{
    sTxDesc    txInfo;
    sBuffDesc *buffDesc = NULL;
    sHpgpHdr  *hpgpHdr = NULL;
    eStatus    status = STATUS_SUCCESS;
#ifdef HPGP_HAL
//    u8         mft;
  //  u8         i;
//    u8         numPbs;
    sEth2Hdr  *ethhdr = NULL;
    u8        *dataptr = NULL;
    u16        datalen = 0;
    u16        pbbSize = 0;
    u16        mfLen = 0;
//    sBuffDesc  frmBuffDesc;
    sMfHdr    *mfHdr = NULL;
#endif

    hpgpHdr = (sHpgpHdr *)event->buffDesc.buff;
    txInfo.dtei = hpgpHdr->tei;
    txInfo.snid = hpgpHdr->snid;

	txInfo.mcst = hpgpHdr->mcst;
	
#ifdef HPGP_HAL
    txInfo.mnbc = hpgpHdr->mnbc; 
#ifdef QCA
    txInfo.plid = HPGP_PLID2;
#else
    txInfo.plid = hpgpHdr->plid;
#endif
#endif


//	FM_Printf(FM_USER,"px\n");

#ifdef LOG_FLASH
    logEvent(MGMT_MSG,0,event->eventHdr.type,&txInfo.dtei,1);
#endif
#ifdef AUTH_AES
    if(event->eventHdr.type == EVENT_TYPE_CM_ENCRY_PAYLOAD_IND)
    {    
        sCmEncryPayloadInd *ind;
        u8 *payload;
        u8  payloadLen;      
        sStaInfo    *staInfo;
        u8 XDATA	iv[20];
		AES_KEY 	key;
        
        payloadLen = event->buffDesc.datalen - (sizeof(sMmHdr) + sizeof(sCmEncryPayloadInd));
         
        ind = (sCmEncryPayloadInd *)(event->buffDesc.dataptr + sizeof(sMmHdr));
   
        payload = (event->buffDesc.dataptr + sizeof(sMmHdr) + sizeof(sCmEncryPayloadInd));        
        
        staInfo =  LINKL_GetStaInfo(((sLinkLayer *)HPGPCTRL_GetLayer(HP_LAYER_TYPE_LINK)));
        
        memcpy(iv, ind->iv, ENC_IV_LEN);

        /* PEKS could be DAK , NMK or TEK */
        
        if (ind->peks == PEKS_NMK)
        {
    	    AES_set_encrypt_key((unsigned char*)staInfo->nmk, 8*AES_BLOCK_SIZE, &key);
			AES_cbc_encrypt((unsigned char*)payload, (unsigned char*)payload, payloadLen,
                         &key, (unsigned char*)iv,AES_ENCRYPT);
        }
#ifdef UKE		
        else // tek
        {             
            // Generate tek
            if(genTek == 1)
            {
                AKM_GenerateTek();
                genTek = 0;
            }
            AES_set_encrypt_key(AKM_GetTek(), 8*AES_BLOCK_SIZE, &key);
			AES_cbc_encrypt((unsigned char*)payload, (unsigned char*)payload,
			             payloadLen, &key, (unsigned char*)iv,AES_ENCRYPT);
        }
#endif		
	
	}
#ifdef UKE	
    else if(event->eventHdr.type == EVENT_TYPE_CM_GET_KEY_CNF)
    {
        if(genTek == 1)
        {
            AKM_GenerateTek();
            genTek = 0;
        }
    }
#endif

#endif

    
    txInfo.eks  = HPGP_UNENCRYPTED_EKS;

    if (event->eventHdr.eventClass == EVENT_CLASS_MSG)
    {
        txInfo.frameType = FRAME_TYPE_MGMT;
    }
    else
    {
        txInfo.frameType = FRAME_TYPE_DATA;
    }
    buffDesc = &event->buffDesc;

#ifdef HPGP_HAL
    /* TODO: determine the port based on the destination MAC address */

    txInfo.txPort = PORT_PLC;

    switch(txInfo.txPort)
    {
        case PORT_PLC:
        {
            /* add the ethernet header */
            buffDesc->dataptr -= sizeof(sEth2Hdr); 
            buffDesc->datalen += sizeof(sEth2Hdr); 

//    DB_Assert(buffDesc);
            ethhdr = (sEth2Hdr *)(buffDesc->dataptr);
            memcpy(ethhdr->srcaddr, hal->macAddr, MAC_ADDR_LEN);
            ethhdr->ethtype = HTONS(ETH_TYPE_HPGP);// SWAP_FIX;
            memcpy(ethhdr->dstaddr, hpgpHdr->macAddr, MAC_ADDR_LEN);
            //FM_Printf(FM_MINFO, "Tx a mgmt msg: %d\n", buffDesc->datalen );
            //FM_HexDump(FM_DATA|FM_MINFO, "Tx a mgmt msg:",
            //           buffDesc->dataptr, buffDesc->datalen);
            /* determine the ROBO mode based on the RSSI and/or FER */
            /* However, due to a debug in the hardware at present, 
             * ROBO mode is determined based on the MSDU length .*/
            mfLen = buffDesc->datalen + HPGP_MF_HDR_LEN + HPGP_MF_ICV_LEN; 
#if 0			
            if ( mfLen <= HPGP_136FEC_PBB_SIZE)
            {
                txInfo.roboMode = HPGP_ROBOMD_MINI;
                txInfo.numPbs = 1;
            }  
            else if ( mfLen <= HPGP_520FEC_PBB_SIZE)
            {
                txInfo.roboMode = HPGP_ROBOMD_STD;
                txInfo.numPbs = 1;
            }
            else
            {
                txInfo.roboMode = HPGP_ROBOMD_HS;
                numPbs = mfLen/HPGP_520FEC_PBB_SIZE;
                numPbs += (mfLen == numPbs*HPGP_520FEC_PBB_SIZE ? 0: 1);
                txInfo.numPbs = numPbs;
               
            }
#endif

            status = HAL_XmitMacFrame(hal, &txInfo, buffDesc);

#if 0
            if (txInfo.frameType == FRAME_TYPE_MGMT)
            {
                buffDesc->dataptr -= HPGP_MF_OPT_LEN;
                /* add confounder here */
                dataptr = buffDesc->dataptr;
                for (i = 0; i < HPGP_MF_OPT_LEN; i++)
                {
                    dataptr[i] = rand() & 0xFF;
                }

                buffDesc->datalen += HPGP_MF_OPT_LEN;
                mft = HPGP_MFT_MGMT;
            }
            else /* data */
            {
                mft = HPGP_MFT_MSDU_NO_ATS;
            }

           /* add MAC frame header */
           buffDesc->dataptr -= sizeof(sMfHdr);
           mfHdr = (sMfHdr *) buffDesc->dataptr;     
           mfHdr->mft = mft;
           mfHdr->mflHi = (buffDesc->datalen >> 8); 
           mfHdr->mflLo = (buffDesc->datalen) & 0xFF; 
           buffDesc->datalen += sizeof(sMfHdr);

            /* determine the ROBO mode based on the RSSI and/or FER */
            /* at present, use the standard ROBO for message reliability.*/
            txInfo.roboMode = HPGP_ROBOMD_STD;

            /* then, determine the number of PBs needed for the tx frame */
            if (txInfo.roboMode == HPGP_ROBOMD_MINI)
                pbbSize = HPGP_136FEC_PBB_SIZE;
            else 
                pbbSize = HPGP_520FEC_PBB_SIZE;

            numPbs = (buffDesc->datalen + HPGP_ICV_LEN) / pbbSize;
            numPbs += ((buffDesc->datalen + HPGP_ICV_LEN == numPbs*pbbSize) ? 0: 1);

            if ( (txInfo.roboMode == HPGP_ROBOMD_MINI) ||
                 (txInfo.roboMode == HPGP_ROBOMD_STD))
            {
                dataptr = buffDesc->dataptr;
                datalen = buffDesc->datalen;
                /* send one PB at a time */
                for (i=0; i< numPbs; i ++)
                {
                    frmBuffDesc.dataptr = dataptr;
                    frmBuffDesc.datalen = MIN(datalen, pbbSize);
                    txInfo.numPbs = 1;
                    if (i == 0)
                        txInfo.mfStart = 1;
                    if (i == numPbs - 1)
                        txInfo.mfEnd = 1;
                    /* transmit the MAC frame */
                    status = HAL_XmitMacFrame(hal, &txInfo, &frmBuffDesc);
                    if (status == STATUS_SUCCESS)
                    {
                        dataptr += pbbSize;
                        datalen -= pbbSize;
                    }
                    else
                    {
                        break;
                    }
                }

            }
            else 
            {
               /* HS ROBO */
               txInfo.roboMode = HPGP_ROBOMD_HS;
               txInfo.mfStart = 1;
               txInfo.mfEnd = 1;
               txInfo.numPbs = numPbs;
               status = HAL_XmitMacFrame(hal, &txInfo, buffDesc);
            }
#endif
            break;
        }
        case PORT_ZIGBEE:
        case PORT_SPI:
        /* TODO: need to differentiate the mgmt and data */				
        default:
        {
        }
    }


	// status = HAL_XmitFrame(hal, &txInfo, buffdesc, hpgpHdr->macAddr);

#else /* HPGP_HAL */
    status = SHAL_Xmit(hal->shal, &txInfo, buffDesc, hpgpHdr->macAddr);
#endif
    if ((status == STATUS_SUCCESS) && 
        (event->eventHdr.status == EVENT_STATUS_COMPLETE))
    {
        /* free the event as the transmission is successful */
        EVENT_Free(event);
    }
    return status;
}


#ifdef HPGP_HAL

/* *****************************************************************
 * NAME :            HAL_ScanNet
 *
 * DESCRIPTION :     The function is to enable/disable scan in the MAC
 *
 * INPUT PARAMETERS :
 *           enable: TRUE  - enable the scan; 
 *                   FALSE - disable the scan
 * OUTPUT PARAMETERS:
 *           None
 * RETURN : None
 * ***************************************************************** */

void HAL_ScanNet(u8 enable)
{
    if (enable == TRUE)
    {
        HHAL_SetSWStatReqScanFlag(REG_FLAG_SET);
    }
    else
    {
        HHAL_SetSWStatReqScanFlag(REG_FLAG_CLR);
    }
}




#if 0
/* *****************************************************************
 * NAME :            HAL_FrameRxIntHandler
 *
 * DESCRIPTION :     The function is an interrupt handler for rx frame 
 *                   executed in the interrupt context.
 *
 * INPUT PARAMETERS :
 *           cookie: a pointer to the sHaLayer data structure
 * OUTPUT PARAMETERS:
 *           None
 * RETURN : None
 * ***************************************************************** */

void HAL_FrameRxIntHandler(void *cookie)
{
    sHaLayer *hal = (sHaLayer *)cookie;
  
    hal->taskBitMap |= HAL_TASK_FRM_RX;
    SCHED_Sched(&hal->task);
}

#endif

 

/* *****************************************************************
 * NAME :            HAL_ProcRx
 *
 * DESCRIPTION :     The function performs polling to recevive/forward  
 *                   frames from the LMAC hardware 
 *
 * INPUT PARAMETERS :
 *           hal: a pointer to the sHaLayer data structure
 * OUTPUT PARAMETERS:
 *           None
 * RETURN : None
 * ***************************************************************** */

#if 0
u8 HAL_RecvMacFrame(void *cookie)
{
    sEvent      *event = NULL;
    u8           frmsCnt;
    u8           descCnt;
    u8           tmp;
    sSwFrmDesc rxFrmSwDesc;
    sBuffDesc   *buffDesc = NULL;
    sEth2Hdr    *ethhdr = NULL;
    sHpgpHdr    *hpgpHdr = NULL; 
    sHaLayer    *hal = (sHaLayer *)cookie;

    frmsCnt = CHAL_GetCPUTxQFrmCount(); 
    descCnt = CHAL_GetCPUTxQDescCount();
    while(frmsCnt--) 
    {
        if (CHAL_ProcRxFrameDesc(hal, &rxFrmSwDesc) == STATUS_SUCCESS)
        {
            switch(rxFrmSwDesc.rxPort)
            {
                case PORT_PLC:
                {
                    if (rxFrmSwDesc.frmType == HPGP_HW_FRMTYPE_MGMT)
                    {
                        /* create an event for the mgmt message */
                        event = EVENT_Alloc(rxFrmSwDesc.frmLen, 
                                            sizeof(sHpgpHdr));
                        if (event == NULL)
                        {
                            FM_Printf(FM_ERROR, "HAL: Fail to allcate an event.\n");
                            CHAL_FreeFrameCp(rxFrmSwDesc.cpDesc, 
                                             rxFrmSwDesc.cpCnt);
                            /* may not be right */
                            continue;
                        }

                        buffDesc = &event->buffDesc;

                        if (CHAL_ReadFrame(hal, &rxFrmSwDesc, buffDesc) 
                            == STATUS_SUCCESS)
                        {
                            hpgpHdr = (sHpgpHdr *)event->buffDesc.buff;
                            hpgpHdr->snid = hal->hhalCb->snid;
                            hpgpHdr->tei = rxFrmSwDesc.frmInfo.plc.stei;
                            event->eventHdr.eventClass = EVENT_CLASS_MSG;
                            /* process the MAC header */
                            ethhdr = (sEth2Hdr *)event->buffDesc.dataptr; 
                            hpgpHdr->macAddr = ethhdr->srcaddr;
                            /* chop off the ethernet header */
                            event->buffDesc.dataptr += sizeof(sEth2Hdr); 
                            event->buffDesc.datalen -= sizeof(sEth2Hdr); 
                            /* deliver the mgmt message to the upper layer */
#ifdef CALLBACK
                            hal->deliverMgmtPacket(hal->mgmtCookie, event);
#else
                            MUXL_RecvMgmtPacket(hal->mgmtCookie, event);
#endif
                        }
                        else
                        {
                            EVENT_Free(event);
                        }
                    }
                    else if (rxFrmSwDesc.frmType == HPGP_HW_FRMTYPE_MSDU)
                    {
                        /* deliver the data packet to the data plane */
                        hal->deliverDataPacket(hal->dataCookie, &rxFrmSwDesc);
                    }
                    else
                    {
                        /* unknown HPGP frame type */
                    }
                    break;
                }
                case PORT_ZIGBEE:
                case PORT_SPI:
                    /* TODO: need to differentiate the mgmt and data */				
                default:
                {
                    /* deliver the data packet to the data plane */
                    hal->deliverDataPacket(hal->dataCookie, &rxFrmSwDesc);
                }
            }
            hal->frmCnt++;
        }
        else
        {
            /* TODO */
        }
    }
    
    hal->taskBitMap &= ~HAL_TASK_FRM_RX;
    /* not be preempted */
    return FALSE;
}

#endif


#if 0
#ifndef HAL_INT
u8 HAL_RxPoll(void *cookie)
{
    sHaLayer *hal = (sHaLayer *)cookie;

    /* check the bcn counter */
    while (HHAL_GetBcnCnt())
    {
        HAL_BcnRxIntHandler(hal);
    }

    HAL_RecvMacFrame(hal);

#ifndef RTX51_TINY_OS
    SCHED_Sched(&hal->task);
#endif
    /* not be preempted */
    return FALSE;
}
#endif

void HAL_EnablePoll(sHaLayer *hal)
{
    SCHED_Sched(&hal->task);
}

#endif /* HAL_INT */

u8 HAL_Proc(void *cookie)
{
    sEvent *event = NULL;
    sSlink *slink = NULL;
    u8      ret = 0;
    sHaLayer *hal = (sHaLayer *) cookie;
    if(!SLIST_IsEmpty(&hal->txQueue))
    {
        if(HHAL_IsPlcIdle() == STATUS_FAILURE)
        {
        
			//
			///FM_Printf(FM_USER, "i. \n");
            return 0;
			
        }
            
#ifdef P8051
__CRIT_SECTION_BEGIN__
#else
        SEM_WAIT(&hal->halSem);
#endif
        slink = SLIST_Pop(&hal->txQueue);
#ifdef P8051
__CRIT_SECTION_END__
#else
        SEM_POST(&hal->halSem);
#endif

        event = SLIST_GetEntry(slink, sEvent, link);
		if (HAL_ProcXmit(hal, event) == STATUS_FAILURE)
        {
            EVENT_Free(event);
        }

        return 1;
    }
    return 0;
}
  
static u8  grpMacAddr[MAC_ADDR_LEN] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};


eStatus HAL_XmitMacFrame(sHaLayer *hal, sTxDesc *txInfo, sBuffDesc *buffDesc)
{
    u16          pbbSize = 0;
    sSwFrmDesc txFrmSwDesc;
//    sEth2Hdr    *ethhdr = NULL;
    eStatus      status = STATUS_FAILURE;
    sHpgpHalCB  *hhalCb = hal->hhalCb;
     u16 retriesCnt = 0;
//    u8           reTxMaxCnt = 5;


	memset(&txFrmSwDesc, 0x00, sizeof(sSwFrmDesc));

    txFrmSwDesc.txPort = txInfo->txPort;
    txFrmSwDesc.frmLen = buffDesc->datalen;

	if (txInfo->frameType == FRAME_TYPE_MGMT)
    {
       txFrmSwDesc.frmType = HPGP_HW_FRMTYPE_MGMT;
    }
	else
	if (txInfo->frameType == FRAME_TYPE_DATA)
    {   
		txFrmSwDesc.frmType = HPGP_HW_FRMTYPE_MSDU;
    }
	else
	if (txInfo->frameType == FRAME_TYPE_BEACON)
	{	
		txFrmSwDesc.frmType = HPGP_HW_FRMTYPE_BEACON;
	}
	
    HHAL_PrepareTxFrame(hal, txInfo, &txFrmSwDesc);

    if (CHAL_GetCpforTxFrame(&txFrmSwDesc) == STATUS_FAILURE)
    {
        return STATUS_FAILURE;
    }

    /* write the data into the frame */
    CHAL_WriteFrame(&txFrmSwDesc, buffDesc);

    /* transmit the frame */
    retriesCnt=0;
//	FM_Printf(FM_USER,"w\n");
   do
    {
    	status = HHAL_PlcTxQWrite(hal, &txFrmSwDesc);
    // check for pending Tx and prossess it.
        if( status == STATUS_FAILURE)               
        {
            
            // TODO try for ETH_PLC_TX_RETRIES number of times, if not success then break
            if(ETH_PLC_TX_RETRIES <= retriesCnt)
            {
                break;
            }
            retriesCnt++;
       }   

    }
    while(status == STATUS_FAILURE);


    if (status == STATUS_FAILURE)
	{
	// 	FM_Printf(FM_ERROR, "PLC Mac Tx Write fail \n"); 
	    CHAL_FreeFrameCp(txFrmSwDesc.cpArr, txFrmSwDesc.cpCount);
    }  
    /*do
    {
        status = HHAL_PlcTxQWrite(hal, &txFrmSwDesc);
        if (status == STATUS_SUCCESS || !reTxMaxCnt)
        {
            break;
        }
        //ISM_PollInt();
        reTxMaxCnt--;
    }while(reTxMaxCnt);
    */
    return status; 

}

#endif /* HPGP_HAL */

void     HAL_RegisterXmitBcnCallback(
            sHaLayer *hal,
            void (*xmitBcnCallback)(void XDATA *cookie),
            void *cookie )
{
	/* Compiler warning suppression */
	xmitBcnCallback = xmitBcnCallback;
#ifdef CALLBACK
    hal->xmitBcn = xmitBcnCallback;
#endif
    hal->txBcnCookie = cookie;

}


void HAL_RegisterProcBcnCallback(
    sHaLayer  *hal,
    void     (*bcnCallback)(void XDATA *bcnCookie, sEvent XDATA *event),
    void      *bcnCookie )
{
		/* Compiler warning suppression */
		bcnCallback = bcnCallback;
#ifdef CALLBACK
    hal->procBcn = bcnCallback;
#endif
    hal->bcnCookie = bcnCookie;
}


void HAL_RegisterRxMgmtCallback(
    sHaLayer  *hal,
    void     (*mgmtCallback)(void XDATA *mgmtCookie, sEvent XDATA *event),
    void      *mgmtCookie )
{
		/* Compiler warning suppression */
		mgmtCallback = mgmtCallback;
#ifdef CALLBACK
    hal->deliverMgmtPacket = mgmtCallback;
#endif
    hal->mgmtCookie = mgmtCookie;
}


#ifdef HPGP_HAL
void HAL_RegisterRxDataCallback(
    sHaLayer  *hal,
    void     (*dataCallback)(void XDATA *dataCookie, sSwFrmDesc XDATA *rxFrmSwDesc),
    void      *dataCookie )
{
		/* Compiler warning suppression */
		dataCallback = dataCallback;
#ifdef CALLBACK
    hal->deliverDataPacket = dataCallback;
#endif
    hal->dataCookie = dataCookie;
}
#endif

void HAL_RegisterRxNetMgmtCallback(
     sHaLayer *hal,
     void (*netMgmtCallback)(void XDATA *netMgmtCookie, sEvent XDATA *event),
     void *netMgmtCookie )
{
		/* Compiler warning suppression */
		netMgmtCallback = netMgmtCallback;
#ifdef CALLBACK
    hal->deliverNetMgmtPacket = netMgmtCallback;
#endif
    hal->netMgmtCookie = netMgmtCookie;
}


eStatus HAL_Init(sHaLayer *hal)
{
#ifdef HPGP_HAL
    
    SLIST_Init(&hal->txQueue);
    /* common module */
    CHAL_InitHW();
    /* hpgp */
    HHAL_Init(hal, &hal->hhalCb);

    hal->hhalCb  = &gHpgpHalCB;
    /* register the frame rx interrupt handler with the ISM */
    ISM_RegisterIntHandler(MAC_INT_IRQ_CPU_TXQ_NONEMPTY, 
                           CHAL_FrameRxIntHandler, hal);

    /* register the beacon tx interrupt handler with the ISM */
 //   ISM_RegisterIntHandler(MAC_INT_IRQ_PLC_BCN_TX, 
 //                          HHAL_BcnTxIntHandler, hal);
    /* register the beacon rx interrupt handler with the ISM */
    ISM_RegisterIntHandler(MAC_INT_IRQ_PLC_BCN_RX, 
                           HAL_BcnRxIntHandler, hal);
 #ifdef HAL_INT
//   SCHED_InitTask(&hal->task, HPGP_TASK_ID_HAL, "HAL", 
//                   HPGP_TASK_PRI_HAL, HAL_RecvMacFrame, hal);
#else
//    SCHED_InitTask(&hal->task, HPGP_TASK_ID_HAL, "HAL", 
//                   HPGP_TASK_PRI_HAL, HAL_RxPoll, hal);
#endif

    /* SPI */
//    HHAL_AFEInit();
    /* ethernet */
#ifdef HYBRII_ETH									  
    EHAL_Init();
#endif





#else
    SHAL_Init(hal, &hal->shal);
#endif /* HPGP_HAL */

    return  STATUS_SUCCESS;
}

/** =========================================================
 *
 * Edit History
 *
 * $Source: /home/cvsrepo/Hybrii_B_OSFW_Dev/firmware/hpgp/src/hal/hal.c,v $
 *
 * $Log: hal.c,v $
 * Revision 1.15  2015/01/02 14:55:36  kiran
 * 1) Timer Leak fixed while freeing SCB fixed
 * 2) Software broadcast supported for LG
 * 3) UART Loopback supported for LG
 * 4) Keep Alive feature to ageout defunctional STA
 * 5) Improved flash API's for NO Host Solution
 * 6) Imporved PLC Hang recovery mechanism
 * 7) Reduced nested call tree of common path functions
 * 8) Code optimization and cleanup (unused arguments, unused local variables)
 * 9) Work around for UART hardware interrupt issues (unintended interrupts and no interrupts)
 * 10) Use of memory specific pointers instead of generic pointers
 *
 * Revision 1.14  2014/11/11 14:52:58  ranjan
 * 1.New Folder Architecture espically in /components
 * 2.Modular arrangment of functionality in new files
 *    anticipating the need for exposing them as FW App
 *    development modules
 * 3.Other improvisation in code and .h files
 *
 * Revision 1.13  2014/10/28 16:27:43  kiran
 * 1) Software recovery using Watchdog Timer
 * 2) Hardware recovery monitor and policies
 * 3) Timer Polling in Control Task and Frame task for better accuracy
 * 4) Common memory optimized by reducing prints
 * 5) Discovered netlist corruption fixed
 * 6) VCO fix in HHAL_AFEInit()
 * 7) Idata optimized by removing floating point operation
 * 8) Fixed EVENT_TYPE_CC_BCN_IND false indication during association @ CCO
 * 9) Beacon processing protected from interrupts
 * 10) Corrupted Beacons are dropped
 * 11) Some unused arguments removed to improve code size
 *
 * Revision 1.12  2014/10/15 10:42:51  ranjan
 * small fixes in um
 *
 * Revision 1.11  2014/09/05 09:28:18  ranjan
 * 1. uppermac cco-sta switching feature fix
 * 2. general stability fixes for many station associtions
 * 3. changed mgmt memory pool for many STA support
 *
 * Revision 1.10  2014/08/25 07:37:34  kiran
 * 1) RSSI & LQI support
 * 2) Fixed Sync related issues
 * 3) Fixed timer 0 timing drift for SDK
 * 4) MMSG & Error Logging in Flash
 *
 * Revision 1.9  2014/06/12 13:15:43  ranjan
 * -separated bcn,mgmt,um event pools
 * -fixed datapath issue due to previous checkin
 * -work in progress. neighbour cco detection
 *
 * Revision 1.8  2014/06/05 08:38:41  ranjan
 * -flash function enabled for uppermac
 * - commit command after any change would flash systemprofiles
 * - verfied upper mac
 *
 * Revision 1.7  2014/05/28 10:58:59  prashant
 * SDK folder structure changes, Uart changes, removed htm (UI) task
 * Varified - UM, LM - iperf (UDP/TCP), overnight test pass
 *
 * Revision 1.6  2014/05/12 08:09:57  prashant
 * Route fix, STA sync issue with AV CCO and handling discover bcn issue fix
 *
 * Revision 1.5  2014/04/24 21:50:42  yiming
 * Working Code for Mitsumi
 *
 * Revision 1.4  2014/02/27 10:42:47  prashant
 * Routing code added
 *
 * Revision 1.3  2014/02/19 10:22:40  ranjan
 * - common sync for hal_tst and upper mac project
 * - ism.c is MAC interrupt handler for hhal_tst and upper mac.
 *    chal_ext1isr function   is removed
 * - verified : lower mac sync, upper mac sync data traffic.
 *
 * Revision 1.2  2014/01/10 17:15:32  yiming
 * check in Rajan 1/8/2014 code release
 *
 * Revision 1.5  2014/01/08 10:53:54  ranjan
 * Changes for LM OS support.
 * New Datapath FrameTask
 * LM and UM  datapath, feature verified.
 *
 * known issues : performance numbers needs revisit
 *
 * review : pending.
 *
 * Revision 1.4  2013/09/04 14:50:45  yiming
 * New changes for Hybrii_A code merge
 *
 * Revision 1.41  2013/07/12 08:56:36  ranjan
 * -UKE Push Button Security Feature.
 * Verified : DirectEntry Security Works.Datapath Works.
 *                 command SetSecMode for UKE works.
 * Added against bug-160
 *
 * Revision 1.40  2013/03/22 12:21:48  prashant
 * default FM_MASK and FM_Printf modified for USER INFO
 *
 * Revision 1.39  2013/03/14 11:49:18  ranjan
 * 1.handled cases  for CCo toSTA switch and  viceversa
 * 2.UM uses bcntemplate
 *
 * Revision 1.38  2013/02/15 12:53:57  prashant
 * ASSOC.REQ changes for DEVELO
 *
 * Revision 1.37  2013/01/15 12:26:12  ranjan
 * a)fixed issues in swQ for plc->host intf datapath and
 *    swQ for host -> plc datapath
 *
 * Revision 1.36  2012/12/18 12:17:46  prashant
 * Stability checkin
 *
 * Revision 1.35  2012/12/14 11:06:57  ranjan
 * queue added for eth to plc datapath
 * removed mgmt tx polling
 *
 * Revision 1.34  2012/11/22 09:44:02  prashant
 * Code change for auto ping test, sending tei map ind out, random mac addrr generation.
 *
 * Revision 1.33  2012/11/19 07:46:23  ranjan
 * Changes for Network discovery modes
 *
 * Revision 1.32  2012/11/06 05:05:26  ranjan
 * -moved AES encryption to Hal Task
 * - verified link establishment is very stable
 *
 * Revision 1.31  2012/10/25 11:38:48  prashant
 * Sniffer code added for MAC_SAP, Added new commands in MAC_SAP for sniffer, bridge,
 *  hardware settings and peer information.
 *
 * Revision 1.30  2012/10/11 06:21:00  ranjan
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
 * Revision 1.29  2012/09/24 06:01:38  yuanhua
 * (1) Integrate the NMA and HAL in Rx path (2) add a Tx queue in HAL to have less stack size needed in tx path, and Tx in HAL is performed by polling now.
 *
 * Revision 1.28  2012/07/27 02:53:10  kripa
 * *** empty log message ***
 *
 * Revision 1.27  2012/07/25 04:34:50  kripa
 * Modified Tx routine to match the common TxQWrite routine.
 *
 * Revision 1.26  2012/07/12 05:48:39  kripa
 * Commenting out Mgmt Msg dump as this could interfere with bcn sync.
 * Committed on the Free edition of March Hare Software CVSNT Client.
 * Upgrade to CVS Suite for more features and support:
 * http://march-hare.com/cvsnt/
 *
 * Revision 1.25  2012/07/03 05:18:37  yuanhua
 * fixed an issue in HAL_XmitMacFrame(), which returns the status according to the status from HHAL_PlcTxQWrite() now.
 *
 * Revision 1.24  2012/06/22 04:33:49  yuanhua
 * added a default MAC address for test.
 *
 * Revision 1.23  2012/06/20 17:50:17  kripa
 * In Hal Rx routine, retrieved snid from rxSwDesc struct.
 * Committed on the Free edition of March Hare Software CVSNT Client.
 * Upgrade to CVS Suite for more features and support:
 * http://march-hare.com/cvsnt/
 *
 * Revision 1.22  2012/06/13 16:12:47  son
 * Initializing txInfo eks to unencrypted eks.
 * Committed on the Free edition of March Hare Software CVSNT Client.
 * Upgrade to CVS Suite for more features and support:
 * http://march-hare.com/cvsnt/
 *
 * Revision 1.21  2012/06/13 06:24:31  yuanhua
 * add code for tx bcn interrupt handler integration and data structures for region entry schedule. But they are not in execution yet.
 *
 * Revision 1.20  2012/06/11 06:29:01  yuanhua
 * changed HAL_SetBpsto to HAL_SyncNet.
 *
 * Revision 1.19  2012/06/08 05:50:57  yuanhua
 * added snid function.
 *
 * Revision 1.18  2012/06/07 06:10:29  yuanhua
 * (1) free CPs if frame tx fails (2) add compiler flag HAL_INT_HDL to differentiate the interrupt and interrupt handler. (3) enable all interrupts during the system initialization.
 *
 * Revision 1.17  2012/06/05 07:25:59  yuanhua
 * (1) add a scan call to the MAC during the network discovery. (2) add a tiny task in ISM for interrupt polling (3) modify the frame receiving integration. (4) modify the tiny task in STM.
 *
 * Revision 1.16  2012/06/04 23:33:13  son
 * Added RTX51 OS support
 *
 * Revision 1.15  2012/06/02 19:18:13  yuanhua
 * fixed an issue in the scheduler. Now the higher priority task will render its cpu resource to the lower priority task if the higher priority task is in the polling mode.
 *
 * Revision 1.14  2012/05/20 23:26:21  yuanhua
 * made the return value for HAL_RxPoll() to FALSE so that hal task would not be preempted.
 *
 * Revision 1.13  2012/05/19 22:22:16  yuanhua
 * added bcn Tx/Rx non-callback option for the ISM.
 *
 * Revision 1.12  2012/05/19 20:32:17  yuanhua
 * added non-callback option for the protocol stack.
 *
 * Revision 1.11  2012/05/14 05:22:29  yuanhua
 * support the SCHED without using callback functions.
 *
 * Revision 1.10  2012/05/12 04:11:46  yuanhua
 * (1) added list.h (2) changed the hal tx for the hw MAC implementation.
 *
 * Revision 1.9  2012/05/07 04:17:57  yuanhua
 * (1) updated hpgp Tx integration (2) added Rx poll option
 *
 * Revision 1.8  2012/04/30 04:05:57  yuanhua
 * (1) integrated the HAL mgmt Tx. (2) various updates
 *
 * Revision 1.7  2012/04/25 13:53:40  yuanhua
 * changed the HAL_Transmit prototype.
 *
 * Revision 1.6  2012/04/21 01:40:30  yuanhua
 * Added Tx descriptor data structures for integration.
 *
 * Revision 1.5  2012/04/19 16:46:30  yuanhua
 * fixed some C51 compiler errors for the integration.
 *
 * Revision 1.4  2012/04/17 23:09:50  yuanhua
 * fixed compiler errors for the hpgp hal test due to the integration changes.
 *
 * Revision 1.3  2012/04/13 06:15:11  yuanhua
 * integrate the HPGP protocol stack with the HAL for the beacon TX/RX and mgmt msg RX.
 *
 * Revision 1.2  2011/09/09 07:02:31  yuanhua
 * migrate the firmware code from the greenchip to the hybrii.
 *
 * Revision 1.2  2011/07/22 18:51:04  yuanhua
 * (1) added the hardware timer tick simulation (hts.h/hts.c) (2) Added semaphors for sync in simulation and defined macro for critical section (3) Fixed some bugs in list.h and CRM (4) Modified and fixed bugs in SHAL (5) Changed SNSM/CNSM and SNAM/CNAM for bug fix (6) Added debugging prints, and more.
 *
 * Revision 1.1  2011/05/06 19:09:17  kripa
 * Adding hal layer files to new source tree.
 *
 * Revision 1.1  2011/04/08 21:43:07  yuanhua
 * Framework
 *
 *
 * =========================================================*/

