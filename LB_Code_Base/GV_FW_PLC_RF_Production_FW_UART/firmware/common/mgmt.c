/** =========================================================
 *
 *  @file mgmt.c
 * 
 *  @brief Assoc and KeyFrame trigger
 *
 *  Copyright (C) 2010-2012, Greenvity Communications, Inc.
 *  All Rights Reserved
 *  
 * ===========================================================*/

#include <stdlib.h>
#include <string.h>
#include "list.h"
#include "event.h"
#include "ism.h"
#include "fm.h"
#include "papdef.h"
#include "hpgpevt.h"
#include "hal.h"
#include "gv701x_osal.h"
#include "nma.h"
#include "hal_common.h"
#include "hal_hpgp.h"
#include "hal_eth.h"
#include "hal_tst.h"
#include "pbkdf1.h"
#include "crc32.h"
#include "akm.h"
#include "mmsg.h"
#include "aes.h"
#include "hpgpapi.h"
#include "aes.h"
#include "event_fw.h"

u8 gNek[ENC_KEY_LEN];

extern u8 gNekEks;

u8 mgmtFrame[600];
u8 gCCoMacAddr[6];

extern u8 gEthMacAddrDef[MAC_ADDR_LEN];

u8 gStaTei = 0;

u8 dbBuff[600];
u8 *keyPkt;

u8 gQCA_NMK[ENC_KEY_LEN] = {0x50, 0xD3, 0xE4, 0x93, 0x3F, 0x85, 0x5B, 0x70, 0x40,
                            0x78, 0x4D, 0xF8, 0x15, 0xAA, 0x8D, 0xB7};

/* HPGP management message header length */
#define    HPGP_MM_HEADER_LEN       (sizeof(sEth2Hdr)+sizeof(sMmHdr))
#define    HPGP_MM_VLAN_HEADER_LEN  (sizeof(ssEth802dot3Hdr)+sizeof(sMmHdr))

#define ETH_TYPE_HPGP           0x88e1


/* encryption protocol info */
typedef struct encProtoInfo
{
    u8          pid;
    u16         prn;
    u8          pmn;
} __PACKED__ sEncProtoInfo, *psEncProtoInfo;

extern void LM_ProcFrame(u16 frameType, 
                            u8 *frameBuff,
                            u16 frameLen, 
                            sHpgpHdr *hpgpHdr);


eStatus LM_ReadFrame(
                       sSwFrmDesc *rxFrmSwDesc,
                       u8 *frameBuff,
                       u16 *frameLen)
                       
                       
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
/*    if (frmLen != ((rxFrmSwDesc->cpCnt -1)*HYBRII_CELLBUF_SIZE +
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
    *frameLen  = frmLen;
	//cellLen = HYBRII_CELLBUF_SIZE - frmOffset;
	cellLen = MIN(HYBRII_CELLBUF_SIZE - frmOffset, frmLen);
//FM_Printf(FM_MINFO,"rx data len: %d, cell len: %d, cp cnt %bu\n", buffDesc->datalen, cellLen, rxFrmSwDesc->cpCnt);
//FM_HexDump(FM_DATA|FM_$MINFO, "rx buff:", (u8 *)buffDesc->dataptr, 256 );  
     
    dataptr = frameBuff; 

	i = 0;
//    for (i = 0; i < rxFrmSwDesc->cpCnt; i++)
    while((i < rxFrmSwDesc->cpCount) && cellLen > 0)
    {
        cellAddr = CHAL_GetAccessToCP(rxFrmSwDesc->cpDesc[i].cp);
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
    CHAL_FreeFrameCp(rxFrmSwDesc->cpDesc, rxFrmSwDesc->cpCount);
    return ret;
}

eStatus LM_RecvFrame(sSwFrmDesc *rxFrmSwDesc, 
                             uRxFrmHwDesc*  pRxPktQ1stDesc,
                             uRxCpDesc*     pRxPktQCPDesc)
{
    sEvent      *event = NULL;
    sBuffDesc   *buffDesc = NULL;
    sEth2Hdr    *ethhdr = NULL;
    sHpgpHdr     lhpgpHdr;
    sHpgpHdr    *hpgpHdr = &lhpgpHdr; 
    eStatus      ret = STATUS_SUCCESS;


   // FM_Printf(FM_USER, "LM_RecvFrame \n");
    
    switch(rxFrmSwDesc->rxPort)
    {
        case PORT_PLC:
        {
             u16 frameLen;
             u16 frameType;
            sMmHdr *mmh; 
            
            if (rxFrmSwDesc->frmType == HPGP_HW_FRMTYPE_MGMT)
            {
                if (LM_ReadFrame(rxFrmSwDesc, mgmtFrame, &frameLen) == STATUS_SUCCESS)
				{

					//hpgpHdr->snid = hal->hhalCb->snid;
					hpgpHdr->snid = rxFrmSwDesc->frmInfo.plc.snid;
					hpgpHdr->tei = rxFrmSwDesc->frmInfo.plc.stei;
					//  FM_Printf(FM_USER, "HAL: Rx Snid=%bu, Stei=%bu\n", hpgpHdr->snid,
					///            hpgpHdr->tei);


					                /* process the MAC header */
					ethhdr = (sEth2Hdr *)mgmtFrame; 


					hpgpHdr->macAddr = ethhdr->srcaddr;
				
					frameLen -= sizeof(sEth2Hdr); 


					frameLen -= sizeof(sMmHdr);


					mmh = (sMmHdr *) (ethhdr + 1);  

					frameType = le16_to_cpu(mmh->mmtype);

					// FM_Printf(FM_USER,
					//       "MUX: process a mgmt msg (type = 0x%.2x).\n", mmh->mmtype);



					LM_ProcFrame(frameType, (u8*)(mmh+1), frameLen, hpgpHdr);

					CHAL_FreeFrameCp(rxFrmSwDesc->cpDesc, rxFrmSwDesc->cpCount);
				}
                else
                {

                }
           }
		break;
        }
        default:
        {
            /* deliver the data packet to the data plane */
            //hal->deliverDataPacket(hal->dataCookie, rxFrmSwDesc);
        }
    }

    return ret;
}


eStatus LM_PrepareTxFrame(sTxDesc *txInfo, 
                                    sTxFrmSwDesc *txFrmSwDesc)
{
    //FM_Printf(FM_MINFO, ">>>PrepareTxFrame:\n");

    if(txInfo->mnbc)
    {
        txFrmSwDesc->frmInfo.plc.mcstMode = HPGP_MNBCST;
    }
    else if(txInfo->dtei == 0xFF)
    {
        txFrmSwDesc->frmInfo.plc.mcstMode = HPGP_MCST;
    }
    else
    {
        txFrmSwDesc->frmInfo.plc.mcstMode = HPGP_UCST;
    }

//printf("HHAL_PrepareTxFrame=%bu, txInfo->dtei=%bu\n", HHAL_PrepareTxFrame, txInfo->dtei);
    txFrmSwDesc->frmInfo.plc.dtei = txInfo->dtei;
    txFrmSwDesc->frmInfo.plc.stei = gStaTei;
    txFrmSwDesc->frmInfo.plc.eks = txInfo->eks;
#ifdef QCA
    txFrmSwDesc->frmInfo.plc.clst = HPGP_CLST_ETH;
#else
    txFrmSwDesc->frmInfo.plc.clst = 1;
#endif
    txFrmSwDesc->frmInfo.plc.plid = txInfo->plid;
    txFrmSwDesc->frmInfo.plc.mfStart = txInfo->mfStart;
    txFrmSwDesc->frmInfo.plc.mfEnd = txInfo->mfEnd;
   
    if (txInfo->plid == PRI_LINK_ID_0) 
        txFrmSwDesc->frmInfo.plc.phyPendBlks = HPGP_PPB_CAP0;
    else
        txFrmSwDesc->frmInfo.plc.phyPendBlks = HPGP_PPB_CAP123;

    txFrmSwDesc->frmInfo.plc.roboMode = txInfo->roboMode;

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


eStatus LM_GetCpforTxFrame(sSwFrmDesc *txFrmSwDesc)
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

void LM_ProcEncryptPayloadInd(unsigned long dLen)
{
    u8          *pos = NULL;
    sMmHdr      *mmh = NULL;
    sCmGetKeyReq * req = NULL;
    sCmSetKeyReq * setReq = NULL;
    sCmGetKeyCnf *cnf = NULL;
    sCmSetKeyCnf *setCnf = NULL;
    eStatus      ret = STATUS_FAILURE;
    u8 *dbBuffPtr = keyPkt;

//        FM_HexDump(FM_USER,"keypkt",keyPkt, dbLen);

    dbBuffPtr += sizeof(sEth2Hdr);
    dLen -= sizeof(sEth2Hdr);
    
    mmh = (sMmHdr *)dbBuffPtr;
    
    
    dbBuffPtr += sizeof(sMmHdr);
    dLen -= sizeof(sMmHdr);
    pos = dbBuffPtr;
    
    switch(le16_to_cpu(mmh->mmtype))
    {
        
        case MMTYPE_CM_GET_KEY_CNF:
        {
            FM_Printf(FM_USER, "AKM: <<< CM_GET_KEY.CNF\n");
            /* Get KEY CNF msg body */
            cnf = (sCmGetKeyCnf *)pos;
            if (cnf->result == 0)
            {
                if (cnf->reqKeyType == KEY_TYPE_NEK) 
                {

                     gNekEks = cnf->eks; 
					
                     pos += sizeof(sCmGetKeyCnf);
                     memcpy(gNek, pos, ENC_KEY_LEN);
                     FM_Printf(FM_USER, "NEK EKS: %bu\n", gNekEks);
                     ///FM_HexDump(FM_USER, "NEK:", gNek, ENC_KEY_LEN);
//#ifdef HPGP_HAL
                     /* set the NEK to the LMAC for the STA */
                     HHAL_AddNEK(gNekEks, gNek);
//#endif
                    
                     ret = STATUS_SUCCESS;
                }
            }
            break;
        }

        default:
        {
        }
    }
    return;
}
void LM_DecodeEncPayloadInd(u8 *frameBuff,
                                u16 frameLen,
                                unsigned long *dbLen)                               
{
    sCmEncryPayloadInd *ind = NULL;
    sDb                *db = NULL;
    u8                 *dbBuffPtr = dbBuff;
    u8                 *pos = NULL;
    u8                  rfLen = 0;
    u32                 crc = 0;
    u8 ret;
	u8 XDATA	iv[20];
	AES_KEY 	key;
    
    pos = frameBuff;
    
    ind = (sCmEncryPayloadInd *)pos;
    pos += sizeof(sCmEncryPayloadInd);    

    *dbLen = frameLen - sizeof(sCmEncryPayloadInd);
    
//    FM_HexDump(FM_USER,"dec", pos, *dbLen);
    
    
    if(ind->peks == KEY_TYPE_NMK)
    {
        //FM_Printf(FM_USER, "decode KEY_TYPE_NMK\n");
        AES_set_encrypt_key((unsigned char*)gQCA_NMK, 8*AES_BLOCK_SIZE, &key);
    }

	memcpy(iv, ind->iv, ENC_IV_LEN);
    ret = AES_cbc_encrypt((unsigned char*)pos, 
	    			(unsigned char*)dbBuff, 
	    			*dbLen, 
	    			&key, 
	    			(unsigned char*)iv,
	    			AES_DECRYPT);
    if(ret != STATUS_SUCCESS)
    {
        //DB_Free(db);
        return;
    }

//    FM_HexDump(FM_USER, "rcv key", dbBuff, *dbLen);
    
   
    /* rf length */
    rfLen = *(dbBuffPtr + (*dbLen) - 1);

        //FM_Printf(FM_USER, "rflen %bu \n", rfLen);

    dbBuffPtr += rfLen;

    keyPkt = dbBuffPtr;
    
    *dbLen  = le16_to_cpu(ind->len);
#ifdef P8051
////FM_Printf(FM_ERROR, "AKM: recv rf len = %bu, msg len = %bu.\n", rfLen, ind->len);
#else
//FM_Printf(FM_ERROR, "AKM: recv rf len = %d, msg len = %d.\n", rfLen, ind->len);
#endif
    /* perform crc verification */
    crc = chksum_crc32(dbBuffPtr, *dbLen);

    pos = dbBuffPtr + (*dbLen);

//    FM_HexDump(FM_USER, "r crc", (u8*)&pos, 4);
//    FM_HexDump(FM_USER, "c crc", (u8*)&crc, 4);

    #if 0
    
    if (memcmp(pos, &crc, sizeof(u32))) 
    {    
        FM_Printf(FM_ERROR, "AKM: crc does not match (tx: 0x%08x, rx: 0x%08x).\n", *(u32*)pos, crc);
    }
    #endif
    
    return;
}


void LM_ProcFrame(u16 frameType, 
                        u8 *frameBuff,
                        u16 frameLen, 
                        sHpgpHdr *hpgpHdr)
{

    sCcAssocCnf   *assocCnf = NULL;


    switch (frameType)
    {
        case EVENT_TYPE_CC_ASSOC_CNF:
        {
        //process the CC_ASSOC.CNF
        assocCnf = (sCcAssocCnf *)frameBuff;
        
        if(1)//assocCnf->result == 0)
        {
            //success
            //This is the first time that we know 
            //CCo MAC address unless we send a query to the CCo
            //So, save it


            memcpy( gCCoMacAddr, 
                    hpgpHdr->macAddr, MAC_ADDR_LEN);
            
            FM_Printf(FM_USER, "LM: <<< CC_ASSOC.CNF (tei: %bu)\n", 
                                assocCnf->staTei);
            gStaTei = assocCnf->staTei;
			FM_Printf(FM_USER, "snid %bu.\n", gHpgpHalCB.snid);			
    		gHpgpHalCB.remoteTei = 1;
			//gCCoTei  = hpgpHdr->tei;
            
            HHAL_SetTei(gStaTei);

#ifdef P8051
			FM_Printf(FM_HINFO, "SNAM: start the tei lease timer (0x%lx)\n",
#else
			FM_Printf(FM_HINFO, "SNAM: start the tei lease timer (0x%.8x)\n",
#endif
			(assocCnf->leaseTime)*60000 - 5000);


        }
        else
        {
        
        }
        //for all other cases, stay in the same state and
        //retry when the access timer expires.

            break;
        }

        case EVENT_TYPE_CM_ENCRY_PAYLOAD_IND:
        {
			unsigned long dbLen;
	
           // FM_HexDump(FM_USER, "rcv frame", frameBuff, frameLen);
            
            LM_DecodeEncPayloadInd(frameBuff,
                                   frameLen,
                                   &dbLen);

            LM_ProcEncryptPayloadInd(dbLen);
            break;
        }
        default:
        {
            FM_HexDump(FM_USER, "mgmt type ", (u8*)&frameType, 2);

            //FM_HexDump(FM_USER, "dump", frameBuff, frameLen);
        }
     }

}
      
eStatus LM_WriteFrame(sTxFrmSwDesc *txFrmSwDesc, 
                        u8 *framebuff,
                        u8 datalen)
{
    s16 resLen = datalen;
    u8  numCps = txFrmSwDesc->cpCount;
    u16 cellLen = 0;
    u8  i = 0;
    u8  *dataptr = framebuff;
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
        FM_Printf(FM_ERROR, "CHAL: frame len and cp number mismatch\n");
    }
    return STATUS_SUCCESS;
}

eStatus LM_XmitMacFrame(sTxDesc *txInfo, u8 *frameBuff, u16 frameLen)
{
    u16          pbbSize = 0;
    sTxFrmSwDesc txFrmSwDesc;
//    sEth2Hdr    *ethhdr = NULL;
    eStatus      status = STATUS_FAILURE;
     u8 retriesCnt = 0;
//    u8           reTxMaxCnt = 5;


	memset(&txFrmSwDesc, 0x00, sizeof(sTxFrmSwDesc));

    txFrmSwDesc.txPort = txInfo->txPort;
    txFrmSwDesc.frmLen = frameLen;

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
	
    LM_PrepareTxFrame(txInfo, &txFrmSwDesc);

    if (LM_GetCpforTxFrame(&txFrmSwDesc) == STATUS_FAILURE)
    {
        return STATUS_FAILURE;
    }

    
    /* write the data into the frame */
    LM_WriteFrame(&txFrmSwDesc, frameBuff, frameLen);

    /* transmit the frame */
    retriesCnt=0;

   do
    {
        status = HHAL_PlcTxQWrite(&txFrmSwDesc);
    // check for pending Tx and prossess it.
        if( status == STATUS_FAILURE)               
        {
            
            // TODO try for ETH_PLC_TX_RETRIES number of times, if not success then break
            if(200 <= retriesCnt)
            {
                break;
            }
            retriesCnt++;
        }   

    }
    while(status == STATUS_FAILURE);

    FM_Printf(FM_USER,"HW Write retries %bu", retriesCnt);
    

    if (status == STATUS_FAILURE)
	{
	 	FM_Printf(FM_USER, "PLC Mac Tx Write fail\n"); 
	    CHAL_FreeFrameCp(txFrmSwDesc.cpArr, txFrmSwDesc.cpCount);
    }  
  
    return status; 

}

eStatus  LM_ProcXmit(u16 frameType, u8 *frameBuff, u16 frameLen,
                          sHpgpHdr *hpgpHdr)
{
    sTxDesc    txInfo;
    eStatus    status = STATUS_SUCCESS;

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


    txInfo.dtei = hpgpHdr->tei;
    txInfo.stei = gStaTei;
    txInfo.snid = hpgpHdr->snid;


    txInfo.mnbc = hpgpHdr->mnbc; 
#ifdef QCA
    txInfo.plid = HPGP_PLID2;
#else
    txInfo.plid = hpgpHdr->plid;
#endif

    if(frameType == EVENT_TYPE_CM_ENCRY_PAYLOAD_IND)
    {    

    #if 1
        sCmEncryPayloadInd *ind;
        u8 *payload;
        u8  payloadLen;      
        
        u8 XDATA	iv[20];
		AES_KEY 	key;

        payloadLen = frameLen - (sizeof(sMmHdr) + sizeof(sEth2Hdr) + 
                                    sizeof(sCmEncryPayloadInd));
         
        ind = (sCmEncryPayloadInd *)(frameBuff +
                                sizeof(sMmHdr) + sizeof(sEth2Hdr));
   
        payload = (frameBuff + sizeof(sMmHdr) + sizeof(sEth2Hdr)
                   + sizeof(sCmEncryPayloadInd));        
        
        memcpy(iv, ind->iv, ENC_IV_LEN);

    //    FM_HexDump(FM_USER, "key", payload, payloadLen);
        
        
  //      FM_HexDump(FM_USER, "ind", (u8*)ind, sizeof(sCmEncryPayloadInd));
        
        /* PEKS could be DAK , NMK or TEK */
        
        if (ind->peks == PEKS_NMK)
        {
    //        FM_Printf(FM_USER,"AES_set_encrypt_key\n");

            
          //  FM_HexDump(FM_USER, "nmk", gQCA_NMK, sizeof(gQCA_NMK));
        
    	    AES_set_encrypt_key((unsigned char*)gQCA_NMK, 8*AES_BLOCK_SIZE, &key);
			AES_cbc_encrypt((unsigned char*)payload, (unsigned char*)payload, payloadLen,
                         &key, (unsigned char*)iv,AES_ENCRYPT);
        }

#endif

	}


	if (frameType == MMTYPE_CM_IDENTIFY_REQ)
		{
			FM_Printf(FM_USER,"using eks\n");
        	txInfo.eks  = gNekEks;
		}
	else	
    	txInfo.eks  = HPGP_UNENCRYPTED_EKS;

    txInfo.frameType = FRAME_TYPE_MGMT;

    /* TODO: determine the port based on the destination MAC address */

    txInfo.txPort = PORT_PLC;

    switch(txInfo.txPort)
    {
        case PORT_PLC:
        {
            status = LM_XmitMacFrame(&txInfo, frameBuff, frameLen);

            break;
            }    
        default:
        {
        }
    }

    return status;
    
}


			   
eStatus LM_SendMgmtMsg(u16 mmType);


void LM_SendIdentifyReq()
{
    LM_SendMgmtMsg(MMTYPE_CM_IDENTIFY_REQ);
}

void LM_SendAssocReq()
{
    LM_SendMgmtMsg(EVENT_TYPE_CC_ASSOC_REQ);
}

void LM_SendGetKeyReq()
{
    chksum_crc32gentab ();
    LM_SendMgmtMsg(EVENT_TYPE_CM_ENCRY_PAYLOAD_IND);
}



#define MSG_PADDING_LEN(x) ( (x)&0xF ? 0x10 - ((x)&0xF) : 0)
/* PID(1 byte) + PRN(2 bytes) + PMN(1 byte) + RFLen(1 byte) */
#define HPGP_ENC_MM_TAIL_LEN   5     
#define CRC_SIZE               4
/* Padding: RF (<=15 bytes) + Padding  (<=15 bytes) */
#define HPGP_ENC_MM_PAD_MAX    30    


#define HPGP_TIME_KEY          1000     /* 100 ms */
#define HPGP_MMTYPE_ALL        0xFFFF

enum {
    AKM_NEK_NO,
    AKM_KEY_NEW_NEK,
};


u16 gPrn;
u8 gPad;

u8 gNmkIv[ENC_IV_LEN] = {0x63,  0x95,  0x21,  0xee,  0xe6,  0x29,  0x13,  0x0d,  0xbd,  0x45,  0xa8,  0xe5, 
             0x6b,  0x66,  0x98 ,  0x73};

void LM_SendEncryptedInd(u16 mmType, 
                              u8 peks, u8 avlnStatus,
                              unsigned long dbLen,
                              u8 *packetPtr)
{
        u8          *pos = NULL;
        sCmEncryPayloadInd *ind = NULL;
        sHpgpHdr    lhpgpHdr;
        sHpgpHdr    *hpgpHdr = &lhpgpHdr;
//        sEvent      *event = NULL;
        eStatus     ret = STATUS_SUCCESS;
        u8 *dbBuffPtr = dbBuff;

//        memset(mgmtFrame, 0x00, sizeof(dbBuffPtr));
        
    
     //   event->eventHdr.eventClass = EVENT_CLASS_MSG;
       // event->eventHdr.type = EVENT_TYPE_CM_ENCRY_PAYLOAD_IND;
        
      
        hpgpHdr->tei = 1;
        hpgpHdr->macAddr = gCCoMacAddr;
        hpgpHdr->snid = gHpgpHalCB.snid;

        hpgpHdr->mnbc = 0;
        hpgpHdr->eks = HPGP_EKS_NONE;
      
    
        pos = packetPtr;
        
        ind = (sCmEncryPayloadInd *)pos;
        ind->peks = peks;
        ind->pid = AUTH_PID_NEW_STA ;
        ind->prn = gPrn;
        ind->pmn = 1;
        ind->avlnStatus = avlnStatus;
        memcpy(ind->iv, gNmkIv, ENC_IV_LEN);
        
        ind->len = cpu_to_le16(dbLen - gPad 
                   - HPGP_ENC_MM_TAIL_LEN - CRC_SIZE);
#ifdef P8051
    //FM_Printf(FM_ERROR, "AKM: tx msg len = %d, pad len = %bu.\n", ind->len, akm->pad);
#else
    //FM_Printf(FM_ERROR, "AKM: tx msg len = %d, pad len = %d.\n", ind->len, akm->pad);
#endif
    
        pos += sizeof(sCmEncryPayloadInd); 
        /*  encrypt of payload  done in HAL Task*/
    
        memcpy(pos, dbBuffPtr, dbLen);
    
        pos += dbLen; 
        
       // event->buffDesc.datalen = pos - mgmtFrame;
    
        LM_ProcXmit(mmType, mgmtFrame,  (pos - mgmtFrame),
                    hpgpHdr);
                
        /* note: the event is freed by MUXL if the TX is successful */
        return;

}


void LM_BuildEncryptedPayload(u16 mmtype, unsigned long *dbLen)
{
    u8           rfLen = 0;
    u8           padLen = 0;
    u16          buffLen = 0;
    u8           size = 0;
    u8 crc2[4];// = {0x1a,0x21, 0xd5, 0x8b };// 0xD 5 5 3 0 8F } 
    u8          *pos = NULL;
    u32          crc = 0;
    u8          *crct;
    u8 *dbBuffPtr = dbBuff;
    sEth2Hdr    *ethII = NULL;
    sMmHdr      *mmh = NULL;
    sEncProtoInfo *tail = NULL;

    /* allocate a buffer for encrypted payload */
    if (((mmtype == MMTYPE_CM_GET_KEY_CNF)) ||
        ((mmtype == MMTYPE_CM_SET_KEY_REQ)))
    {
        size = ENC_KEY_LEN;
    }
    buffLen = HPGP_MM_HEADER_LEN + sizeof(uEncMgmtMsg) + size +
              CRC_SIZE + HPGP_ENC_MM_TAIL_LEN + HPGP_ENC_MM_PAD_MAX;
   

    /* may align the buffer on 128 bits (16 bytes) boundary */
    pos = dbBuffPtr;
    /* random filler */
    rfLen = 3;//rand() & 0xF;
    memset(pos, 0xAA, rfLen);
    pos += rfLen;
    /* ethernet header */
    ethII = (sEth2Hdr *)pos;
    memcpy(ethII->srcaddr, gEthMacAddrDef , MAC_ADDR_LEN);
    memcpy(ethII->dstaddr, gCCoMacAddr, MAC_ADDR_LEN);
    ethII->ethtype = HTONS(ETH_TYPE_HPGP);//SWAP_FIX;
    pos += sizeof(sEth2Hdr);  
    /* mgmt msg header */
    mmh = (sMmHdr *)pos; 
    mmh->mmv = 0x1;
    mmh->mmtype = cpu_to_le16(mmtype);//SWAP_FIX ;
    mmh->nfmi = 0;
    mmh->fnmi = 0;
    mmh->fmsn = 0;
    pos += sizeof(sMmHdr);

    switch(mmtype)
    {
        case MMTYPE_CM_GET_KEY_REQ:
        {
            /* Get KEY REQ msg body */

          //  FM_Printf(FM_USER,"getkey form\n");
            
            sCmGetKeyReq *req = (sCmGetKeyReq *)pos;
            req->reqType = 0;    /* direct */
            req->reqKeyType = KEY_TYPE_NEK ;
            memset(req->myNonce, 0xEE, 4);
            memcpy(req->nid, gHpgpHalCB.nid, NID_LEN);
            req->pid = AUTH_PID_NEW_STA;
            req->prn =rand() & 0xFFFF;

			gPrn = req->prn;
            req->pmn = 1; 
            pos += sizeof(sCmGetKeyReq);
            padLen = rfLen + HPGP_MM_HEADER_LEN + sizeof(sCmGetKeyReq) + size +
                     CRC_SIZE + HPGP_ENC_MM_TAIL_LEN;
            padLen = MSG_PADDING_LEN(padLen);
#ifdef P8051
//FM_Printf(FM_ERROR, "AKM: tx rf len = %bu, pad len = %bu.\n", rfLen, padLen);
#else
//FM_Printf(FM_ERROR, "AKM: tx rf len = %d, pad len = %d.\n", rfLen, padLen);
#endif
            break;
        }
      
        default:
        {
        }
    }
    /* CRC */
    crc = chksum_crc32(dbBuffPtr + rfLen, 
                       pos - dbBuffPtr - rfLen);

    crct = (u8*)&crc;

    crc2[0] = crct[3];

    crc2[1] = crct[2];

    crc2[2] = crct[1];

    crc2[3] = crct[0];


    
    memcpy(pos, &crc2, sizeof(u32));  
//    FM_Printf(FM_HINFO, "AKM: crc = 0x%08x.\n", crc);
    pos += CRC_SIZE;
    /* cm encrypted payload ind msg tail */
    tail = (sEncProtoInfo *)pos;
    tail->pid = AUTH_PID_NEW_STA;
    tail->prn = gPrn;
    tail->pmn = 1;
    pos += sizeof(sEncProtoInfo);
    /* padding */
   // FillRandomNumber(pos, padLen);
    pos += padLen;
    *pos = rfLen;
    pos ++;
    *dbLen = pos - dbBuffPtr;
    gPad = rfLen + padLen;
    if(*dbLen % 16) 
    {
        FM_Printf(FM_USER, "Encoded MM is not on 16 bytes boundary\n");
    }
    
    return ;
}
				  
eStatus LM_SendMgmtMsg(u16 mmType)
{
    eStatus        status = STATUS_FAILURE;
    sEvent        *newEvent = NULL;
	sCcIdentifyInd  *identify;
//    sHpgpHdr      *newHpgpHdr = NULL;
    sCcAssocReq   *assocReq = NULL;
    sCcLeaveReq   *leaveReq = NULL;
    sCcCcoApptReq *ccoApptReq = NULL;
    u16 frameLen = 0;
    u16            eventSize = 0;
//    sLinkLayer    *linkl = (sLinkLayer *)HPGPCTRL_GetLayer(HP_LAYER_TYPE_LINK);
    sHpgpHdr        HpgpHdr;
    sHpgpHdr        *newHpgpHdr = &HpgpHdr;
    sEth2Hdr *ethhdr;
    sMmHdr *mmh;
	u8 *packetPtr;

    frameLen = MAX(HPGP_DATA_PAYLOAD_MIN, sizeof(sCcAssocReq));

    frameLen += sizeof(sEth2Hdr); 
    

    //    DB_Assert(buffDesc);
    ethhdr = (sEth2Hdr *)(mgmtFrame);
    memcpy(ethhdr->srcaddr, gEthMacAddrDef, MAC_ADDR_LEN);
    ethhdr->ethtype = HTONS(ETH_TYPE_HPGP);// SWAP_FIX;
    memcpy(ethhdr->dstaddr, gCCoMacAddr, MAC_ADDR_LEN);


    frameLen += sizeof(sMmHdr);  
    mmh = (sMmHdr *)(ethhdr + 1);  
          
    //add mgmt msg header
    //fragment is not supported at present
    mmh->mmv = 0x1;
    mmh->mmtype = cpu_to_le16(mmType);
    mmh->nfmi = 0;
    mmh->fnmi = 0;
    mmh->fmsn = 0;
    


    packetPtr = (u8*)(mmh + 1);
    switch(mmType)
    {
    	case MMTYPE_CM_IDENTIFY_REQ:
		{
			
			//frameLen += MAX(HPGP_DATA_PAYLOAD_MIN, sizeof(sCcIdentifyInd));
#ifdef QCA
			newHpgpHdr->tei = 1;
#else
			newHpgpHdr->tei = staInfo->ccoScb->tei;
#endif
			newHpgpHdr->macAddr = gCCoMacAddr;
			newHpgpHdr->snid = 	gHpgpHalCB.snid;

			newHpgpHdr->eks = gNekEks;

			
       		 newHpgpHdr->mnbc = 0;
/*

u8	  greenPhyCap;
u8	  pwrSaveCap;
u8	  allocCap;
u8	  repeatCap;
u8	  homeplugAV;
u8 efl;
u8	ef; 

*/
			identify = (sCcIdentifyInd*)(mmh + 1);

			identify->greenPhyCap = 1;
			identify->pwrSaveCap  = 0;
			identify->allocCap = 0;
			
			identify->repeatCap = 0;
			identify->homeplugAV = 0xFF;
			identify->efl = 0;
			
				
			 FM_Printf(FM_MMSG, "SNAM: >>> MMTYPE_CM_IDENTIFY_IND (tei: %bu)\n",
			 				 newHpgpHdr->tei);



			LM_ProcXmit(mmType, mgmtFrame, frameLen,newHpgpHdr);

			break;


			}
        case EVENT_TYPE_CC_ASSOC_REQ:
        {            
#ifdef QCA
            newHpgpHdr->tei = 0xFF;
#else
            newHpgpHdr->tei = staInfo->ccoScb->tei;
#endif
            newHpgpHdr->macAddr = gCCoMacAddr;
            newHpgpHdr->snid = 	gHpgpHalCB.snid;

            newHpgpHdr->eks = HPGP_EKS_NONE;
 
            assocReq = (sCcAssocReq *)(mmh + 1);

            assocReq->reqType = 0;
            
            memcpy(assocReq->nid, gHpgpHalCB.nid, NID_LEN);
            
            assocReq->ccoCap = 0;
            
            assocReq->proxyNetCap = 0;

            FM_Printf(FM_USER, "SNAM: >>> CC_ASSOC.REQ (tei: %bu)\n",
                              newHpgpHdr->tei);

            
            
            LM_ProcXmit(mmType, mgmtFrame, frameLen,newHpgpHdr);

            break;
        }

        case EVENT_TYPE_CM_ENCRY_PAYLOAD_IND:
           {
		   	unsigned long dbLen = 0;
			
            LM_BuildEncryptedPayload(MMTYPE_CM_GET_KEY_REQ , &dbLen);
            
            LM_SendEncryptedInd(EVENT_TYPE_CM_ENCRY_PAYLOAD_IND,
                                PEKS_NMK, AVLN_STATUS_ASSOC_NO_PCO_CAP,
                                dbLen, packetPtr ); 
            FM_Printf(FM_MMSG, "SNAM: >>> EVENT_TYPE_CM_ENCRY_PAYLOAD_IND(tei: %bu)\n",
                               newHpgpHdr->tei);
            break;
          }
        }
//    status = MUXL_TransmitMgmtMsg(newEvent);
   
    return status;
}


