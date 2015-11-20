/** ========================================================
 *
 *  @file akm.c
 * 
 *  @brief Authentication and Key Manager 
 &*
 *  Copyright (C) 2010-2011, Greenvity Communications, Inc.
 *  All Rights Reserved
 *  
 * ==========================================================*/


#ifdef RTX51_TINY_OS
#include <rtx51tny.h>
#endif
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include "papdef.h"
#ifdef ROUTE
#include "hpgp_route.h"
#endif
#include "fm.h"
#include "list.h"
#include "timer.h"
#include "stm.h"
#include "hpgpdef.h"
#include "event.h"
#include "nma.h"
#include "nma_fw.h"
#include "hpgpapi.h"
#include "pbkdf1.h"
#include "crc32.h"
#include "linkl.h"
#include "nam.h"
#include "akm.h"
#include "mmsg.h"
#include "muxl.h"
#ifdef UKE
#include "Sha2.h"
#endif
#ifdef AUTH_AES
#include "aes.h"
#endif
#include "dmm.h"
#include "green.h"
#ifdef HPGP_HAL
#include "hal_hpgp.h"
#endif
#include "hybrii_tasks.h"
#include "sys_common.h"
#include "event_fw.h"

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
#ifdef MULTIDEV
extern void HTM_ResetNsm(void);
extern u8 devNum;
#endif

#ifdef UKE
u8 *hashKey = NULL;
u8 genTek = 0;
#endif

/* encryption protocol info */
typedef struct encProtoInfo
{
    u8          pid;
    u16         prn;
    u8          pmn;
} __PACKED__ sEncProtoInfo, *psEncProtoInfo;


extern void LINKL_TimerHandler(u16 type, void *cookie);

/* --------------------------
 * Auth and Key  manager
 * ------------------------- */

void FillRandomNumber(u8 *buff, u16 len)
{
    u16 i;

    for (i = 0; i < len; i++)
    {
        buff[i] = rand() & 0xFF;
    }
}


void LINKL_FillHpgpHdr(sHpgpHdr *hpgpHdr, u8 tei, u8 *macAddr, u8 snid, u8 mnbc,
                          u8 eks)
{
    
    hpgpHdr->tei = tei;
    hpgpHdr->macAddr = macAddr;
    hpgpHdr->snid = snid;

    hpgpHdr->mnbc = mnbc;
    hpgpHdr->eks = eks;

}

#ifdef UKE

eStatus AKM_GenerateTek()
{

    sLinkLayer *linkl = HPGPCTRL_GetLayer(HP_LAYER_TYPE_LINK);
    sAkm  *akm = &linkl->akm;

    u8 digest[32];
    
    sha256(digest, hashKey, HASH_KEY_LEN * 2 * 8);
    memcpy(akm->tek, digest, 16);
    DMM_Free(hashKey);
    hashKey = NULL;
 //   FM_HexDump(FM_HINFO,"TEK: ",akm->tek,16);
    akm->tekPeks = 2;
    return STATUS_SUCCESS;
}


u8* AKM_GetTek()
{
    sLinkLayer *linkl = HPGPCTRL_GetLayer(HP_LAYER_TYPE_LINK);


    return (&linkl->akm.tek);

}

#endif

eStatus GenerateKey(u8 *pwd, u8 pwdlen, u8 *key)
{
    u8 salt[8] = {0x08, 0x85, 0x6D, 0xAF, 0x7C, 0xF5, 0x81, 0x85};
		/*Compiler warning suppression*/
		pwd = pwd;
		pwdlen = pwdlen;
		
    /* FIXME: for test only */
    memcpy(key, salt, 8);
    return STATUS_SUCCESS;
//    return pbkdf1(pwd, pwdlen, salt, 8, 1000, key, ENC_KEY_LEN);
}


u8 AKM_GetNewEks(sAkm *akm) 
{
    akm->eks = 0;//(++(akm->eks))%0x8;
    return (akm->eks & 0x0F);
}



sDb *AKM_BuildEncPayload(sAkm *akm, u16 mmtype, void *param) 
{
    u8           rfLen = 0;
    u8           padLen = 0;
    u16          buffLen = 0;
    u8           size = 0;
    u8 crc2[4];// = {0x1a,0x21, 0xd5, 0x8b };// 0xD 5 5 3 0 8F } //
    u8          *pos = NULL;
    u32          crc = 0;
    u8          *crct;
    sDb         *db = NULL;
    sEth2Hdr    *ethII = NULL;
    sMmHdr      *mmh = NULL;
    sEncProtoInfo *tail = NULL;
    sLinkLayer  *linkl = (sLinkLayer *)HPGPCTRL_GetLayer(HP_LAYER_TYPE_LINK);
    sStaInfo    *staInfo = LINKL_GetStaInfo(linkl);

		/*Compiler warning suppression*/
		param = param;
	
    /* allocate a buffer for encrypted payload */
    if (((mmtype == MMTYPE_CM_GET_KEY_CNF) && (akm->keyType == KEY_TYPE_NEK)) ||
        ((mmtype == MMTYPE_CM_SET_KEY_REQ) && (akm->keyType == KEY_TYPE_NMK)))
    {
        size = ENC_KEY_LEN;
    }
    buffLen = HPGP_MM_HEADER_LEN + sizeof(uEncMgmtMsg) + size +
              CRC_SIZE + HPGP_ENC_MM_TAIL_LEN + HPGP_ENC_MM_PAD_MAX;
    db = DB_Alloc(buffLen, 0);
    if (db == NULL)
    {
        FM_Printf(FM_ERROR, "DBfail\n");
        return NULL;
    }

    /* may align the buffer on 128 bits (16 bytes) boundary */
    pos = db->buffDesc.dataptr;
    /* random filler */
    rfLen = rand() & 0xF;
    FillRandomNumber(pos, rfLen);
    pos += rfLen;
    /* ethernet header */
    ethII = (sEth2Hdr *)pos;
    memcpy(ethII->srcaddr, staInfo->macAddr, MAC_ADDR_LEN);
    memcpy(ethII->dstaddr, akm->peerMacAddr, MAC_ADDR_LEN);
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
            sCmGetKeyReq *req = (sCmGetKeyReq *)pos;
            req->reqType = 0;    /* direct */
            req->reqKeyType = akm->keyType;
#ifdef KEY_SPEC
            memcpy(req->myNonce, akm->myNonce, 4);
            memcpy(req->nid, staInfo->nid, NID_LEN);
#endif
            req->pid = akm->pid; 
            req->prn = akm->prn;
            req->pmn = akm->pmn; 
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
        case MMTYPE_CM_GET_KEY_CNF:
        {
            /* Get KEY CNF msg body */
            sCmGetKeyCnf * cnf = (sCmGetKeyCnf *)pos;
            cnf->result = 0;    /* key granted */
            cnf->reqKeyType = akm->keyType;
#ifdef KEY_SPEC
            memcpy(cnf->myNonce, akm->myNonce, 4);
            memcpy(cnf->yourNonce, akm->yourNonce, 4);
            memcpy(cnf->nid, staInfo->nid, NID_LEN);
#endif
            cnf->pid = akm->pid; 
            cnf->prn = akm->prn;
            cnf->pmn = akm->pmn; 
            cnf->eks = staInfo->nekEks;
            pos += sizeof(sCmGetKeyCnf);
            memcpy(pos, staInfo->nek, ENC_KEY_LEN);
            pos += ENC_KEY_LEN;
            padLen = rfLen + HPGP_MM_HEADER_LEN + sizeof(sCmGetKeyCnf) + size +
                     CRC_SIZE + HPGP_ENC_MM_TAIL_LEN;
            padLen = MSG_PADDING_LEN(padLen);
            break;
        }
#ifdef UKE
		
        case MMTYPE_CM_SET_KEY_REQ:
        {
            /* Set KEY Req msg body */
            sCmSetKeyReq* req = (sCmSetKeyReq *)pos;
            req->keyType = akm->keyType;    /* key granted */
#ifdef KEY_SPEC
            memcpy(req->myNonce, akm->myNonce, 4);
            memcpy(req->yourNonce, akm->yourNonce, 4);
            memcpy(req->nid, staInfo->nid, NID_LEN);
#endif
            req->pid = akm->pid; 
            req->prn = akm->prn;
            req->pmn = akm->pmn; 
            req->ccoCap = staInfo->ccoScb->staCap.fields.ccoCap;
            req->newEks = PEKS_NMK;
            memcpy(req->pNewKey, staInfo->nmk, ENC_KEY_LEN);            
            pos += sizeof(sCmSetKeyReq);
            padLen = rfLen + HPGP_MM_HEADER_LEN + sizeof(sCmSetKeyReq) +
                     CRC_SIZE + HPGP_ENC_MM_TAIL_LEN;
            padLen = MSG_PADDING_LEN(padLen);
            break;
        }
        case MMTYPE_CM_SET_KEY_CNF:
        {
            /* Set KEY cnf msg body */
            sCmSetKeyCnf* cnf = (sCmSetKeyCnf*)pos;
            cnf->result= 0;    /* key granted */
#ifdef KEY_SPEC
            memcpy(cnf->myNonce, akm->myNonce, 4);
            memcpy(cnf->yourNonce, akm->yourNonce, 4);
#endif
            cnf->pid = akm->pid; 
            cnf->prn = akm->prn;
            cnf->pmn = akm->pmn; 
            cnf->ccoCapability = staInfo->ccoScb->staCap.fields.ccoCap;
            pos += sizeof(sCmSetKeyCnf);
            padLen = rfLen + HPGP_MM_HEADER_LEN + sizeof(sCmSetKeyCnf) +
                     CRC_SIZE + HPGP_ENC_MM_TAIL_LEN;
            padLen = MSG_PADDING_LEN(padLen);
            break;
        }
#endif		
        default:
        {
        }
    }
    /* CRC */
    crc = chksum_crc32(db->buffDesc.dataptr + rfLen, 
                       pos - db->buffDesc.dataptr - rfLen);

    crct = (u8*)&crc;

    crc2[0] = crct[3];

    crc2[1] = crct[2];

    crc2[2] = crct[1];

    crc2[3] = crct[0];


    
    memcpy(pos, &crc2, sizeof(u32));  

    pos += CRC_SIZE;
    /* cm encrypted payload ind msg tail */
    tail = (sEncProtoInfo *)pos;
    tail->pid = akm->pid; 
    tail->prn = akm->prn;
    tail->pmn = akm->pmn; 
    pos += sizeof(sEncProtoInfo);
    /* padding */
    FillRandomNumber(pos, padLen);
    pos += padLen;
    *pos = rfLen;
    pos ++;
    db->buffDesc.datalen = pos - db->buffDesc.dataptr;
    akm->pad = rfLen + padLen;
    if(db->buffDesc.datalen % 16) 
    {
        FM_Printf(FM_ERROR, "NAal\n");
    }
    
    return db;
}

extern sHomePlugCb HomePlug;
eStatus AKM_SendEncPayloadInd(sAkm *akm, u16 mmType, sDb *db, 
                              u8 peks, u8 avlnStatus)
{
    u8          *pos = NULL;
    sCmEncryPayloadInd *ind = NULL;
    sHpgpHdr    *hpgpHdr = NULL;
    sEvent     xdata *event = NULL;
    eStatus     ret = STATUS_SUCCESS;

    sLinkLayer  *linkl = (sLinkLayer *)HPGPCTRL_GetLayer(HP_LAYER_TYPE_LINK);
    sStaInfo    *staInfo = LINKL_GetStaInfo(linkl);
	u16 size=0;	

		/*Compiler warning suppression*/
		mmType = mmType;
	size = sizeof(sCmEncryPayloadInd) + db->buffDesc.datalen;
   // event =  EVENT_Alloc(sizeof(sCmEncryPayloadInd) + db->buffDesc.datalen, 
                       //  EVENT_HPGP_MSG_HEADROOM);//kiran stack optimization

	event = (sEvent *) DMM_Alloc(MGMT_POOL_ID,sizeof(sEvent) + size + EVENT_HPGP_MSG_HEADROOM);
	if(event == NULL)
    {
        FM_Printf(FM_ERROR, "EAF\n");
        return STATUS_FAILURE;
    }
	else
	{
		memset(event, 0, sizeof(sEvent) + size + EVENT_HPGP_MSG_HEADROOM);
		event->buffDesc.buff = (u8 *)event + sizeof(sEvent);
		event->buffDesc.dataptr = event->buffDesc.buff + EVENT_HPGP_MSG_HEADROOM;
		event->buffDesc.datalen = 0;
		event->buffDesc.bufflen = size + EVENT_HPGP_MSG_HEADROOM;
		event->eventHdr.status = EVENT_STATUS_COMPLETE;
		SLINK_Init(&event->link);
	}

    event->eventHdr.eventClass = EVENT_CLASS_MSG;
    event->eventHdr.type = EVENT_TYPE_CM_ENCRY_PAYLOAD_IND;
    
    LINKL_FillHpgpHdr((sHpgpHdr *)event->buffDesc.buff,
                    akm->peerTei,
                    akm->peerMacAddr,
                    staInfo->snid,
                    0,
                    HPGP_EKS_NONE);

    pos = event->buffDesc.dataptr;
    ind = (sCmEncryPayloadInd *)pos;
    ind->peks = peks;
    ind->pid = akm->pid; 
    ind->prn = akm->prn;
    ind->pmn = akm->pmn; 
    ind->avlnStatus = avlnStatus;
    memcpy(ind->iv, akm->nmkIv, ENC_IV_LEN);
    ind->len = cpu_to_le16(db->buffDesc.datalen - akm->pad 
               - HPGP_ENC_MM_TAIL_LEN - CRC_SIZE);
#ifdef P8051
//FM_Printf(FM_ERROR, "AKM: tx msg len = %d, pad len = %bu.\n", ind->len, akm->pad);
#else
//FM_Printf(FM_ERROR, "AKM: tx msg len = %d, pad len = %d.\n", ind->len, akm->pad);
#endif

    pos += sizeof(sCmEncryPayloadInd); 
    /*  encrypt of payload  done in HAL Task*/

	memcpy(pos, db->buffDesc.dataptr, db->buffDesc.datalen);

    pos += db->buffDesc.datalen; 
    event->buffDesc.datalen = pos - event->buffDesc.dataptr;

	assert((event->buffDesc.dataptr >= event->buffDesc.buff)&&
           ((event->buffDesc.dataptr - event->buffDesc.buff + 
             event->buffDesc.datalen) <= event->buffDesc.bufflen));

    //ret = MUXL_TransmitMgmtMsg(event);

	//sMmHdr    *mmh = NULL;
    //sHpgpHdr *hpgpHdr = (sHpgpHdr *)event->buffDesc.buff;

    if( event->eventHdr.eventClass != EVENT_CLASS_MSG)
    {
		EVENT_Free(event);
        return STATUS_FAILURE;
    }

    /* add the mgmt msg header */
    if(event->buffDesc.buff+sizeof(sMmHdr) > event->buffDesc.dataptr)
    {
		EVENT_Free(event);
        return STATUS_FAILURE;
    }

    if (((((sHpgpHdr *)event->buffDesc.buff)->mnbc)&&(event->buffDesc.datalen > HPGP_MNBC_PAYLOAD_MAX)) ||
         (event->buffDesc.datalen > HPGP_DATA_PAYLOAD_MAX))
    {
        /* perform the msg fragment */
//        FM_Printf(FM_ERROR,"MUXL:need frag\n");
    }

    event->buffDesc.dataptr -= sizeof(sMmHdr);  
    event->buffDesc.datalen += sizeof(sMmHdr);  
   // mmh = (sMmHdr *) (event->buffDesc.dataptr);  
          
    //add mgmt msg header
    //fragment is not supported at present
    //  mmh->mmv = 0x1;
    //mmh->mmtype = cpu_to_le16(event->eventHdr.type);
    //mmh->nfmi = 0;
    //mmh->fnmi = 0;
    //mmh->fmsn = 0;
	((sMmHdr *)(event->buffDesc.dataptr))->mmv = 0x1;
    ((sMmHdr *)(event->buffDesc.dataptr))->mmtype = cpu_to_le16(event->eventHdr.type);
	((sMmHdr *)(event->buffDesc.dataptr))->nfmi = 0;
	((sMmHdr *)(event->buffDesc.dataptr))->fnmi = 0;
	((sMmHdr *)(event->buffDesc.dataptr))->fmsn = 0;
	
   // ret = HAL_Transmit(HOMEPLUG_GetHal(), event);
   ret = HAL_Transmit(&HomePlug.haLayer, event);
   
 
    /* note: the event is freed by MUXL if the TX is successful */
    if(ret == STATUS_FAILURE)
    {
        EVENT_Free(event);
    }
    
    return ret;
}

       
eStatus AKM_SendEncPayloadRsp(sAkm *akm, u8 result) 
{
   u8          *pos = NULL;
   sCmEncryPayloadRsp *rsp = NULL;
   sHpgpHdr    *hpgpHdr = NULL;
   sEvent    xdata  *event = NULL;
   eStatus     ret = STATUS_SUCCESS;

   sLinkLayer  *linkl = (sLinkLayer *)HPGPCTRL_GetLayer(HP_LAYER_TYPE_LINK);
   sStaInfo    *staInfo = LINKL_GetStaInfo(linkl);

   //event =  EVENT_Alloc(sizeof(sCmEncryPayloadRsp), 
     //                   EVENT_HPGP_MSG_HEADROOM);

	event = (sEvent *) DMM_Alloc(MGMT_POOL_ID,sizeof(sEvent) + sizeof(sCmEncryPayloadRsp) + EVENT_HPGP_MSG_HEADROOM);
	if(event == NULL)
    {
        FM_Printf(FM_ERROR, "EAF\n");
        return STATUS_FAILURE;
    }
	else
	{
		memset(event, 0, sizeof(sEvent) + sizeof(sCmEncryPayloadRsp) + EVENT_HPGP_MSG_HEADROOM);
		event->buffDesc.buff = (u8 *)event + sizeof(sEvent);
		event->buffDesc.dataptr = event->buffDesc.buff + EVENT_HPGP_MSG_HEADROOM;
		event->buffDesc.datalen = 0;
		event->buffDesc.bufflen = sizeof(sCmEncryPayloadRsp) + EVENT_HPGP_MSG_HEADROOM;
		event->eventHdr.status = EVENT_STATUS_COMPLETE;
		SLINK_Init(&event->link);
	}

   event->eventHdr.eventClass = EVENT_CLASS_MSG;
   event->eventHdr.type = EVENT_TYPE_CM_ENCRY_PAYLOAD_RSP;
   
   LINKL_FillHpgpHdr((sHpgpHdr *)event->buffDesc.buff,
                   akm->peerTei,
                   akm->peerMacAddr,
                   staInfo->snid,
				   0,
				   HPGP_EKS_NONE);

   pos = event->buffDesc.dataptr;
   rsp = (sCmEncryPayloadRsp*)pos;
   rsp->pid = akm->pid; 
   rsp->prn = akm->prn;
   rsp->result = result;

   pos += sizeof(sCmEncryPayloadInd); 

 
   assert((event->buffDesc.dataptr >= event->buffDesc.buff)&&
			  ((event->buffDesc.dataptr - event->buffDesc.buff + 
				event->buffDesc.datalen) <= event->buffDesc.bufflen));

	if( event->eventHdr.eventClass != EVENT_CLASS_MSG)
	{
		DMM_Free((u8 *)event);//Kiran Stack Optimization
	    return STATUS_FAILURE;
	}

	/* add the mgmt msg header */
	if(event->buffDesc.buff+sizeof(sMmHdr) > event->buffDesc.dataptr)
	{
	//    FM_Printf(FM_ERROR,"MUXL:Databuff small\n");
	    return STATUS_FAILURE;
	}

	if (((((sHpgpHdr *)event->buffDesc.buff)->mnbc)&&(event->buffDesc.datalen > HPGP_MNBC_PAYLOAD_MAX)) ||
	     (event->buffDesc.datalen > HPGP_DATA_PAYLOAD_MAX))
	{
	    /* perform the msg fragment */
	  //  FM_Printf(FM_ERROR,"MUXL:need frag\n");
	}

	event->buffDesc.dataptr -= sizeof(sMmHdr);  
	event->buffDesc.datalen += sizeof(sMmHdr);  

	((sMmHdr *)(event->buffDesc.dataptr))->mmv = 0x1;
	((sMmHdr *)(event->buffDesc.dataptr))->mmtype = cpu_to_le16(event->eventHdr.type);
	((sMmHdr *)(event->buffDesc.dataptr))->nfmi = 0;
	((sMmHdr *)(event->buffDesc.dataptr))->fnmi = 0;
	((sMmHdr *)(event->buffDesc.dataptr))->fmsn = 0;

	ret = HAL_Transmit(HOMEPLUG_GetHal(), event);


   /* note: the event is freed by MUXL if the TX is successful */
   if(ret == STATUS_FAILURE)
   {
       //EVENT_Free(event);
	   DMM_Free((u8 *)event);//Kiran Stack Optimization
   }
   
   return ret;
}
              


eStatus AKM_SendMgmtMsg(sAkm *akm, u16 mmType)
{
    sDb    *db = NULL;
    u8 peks, avlnStatus;
    eStatus ret;

    /* encode CM_GET_KEY.REQ */
    db = AKM_BuildEncPayload(akm, mmType, NULL); 

	if (db == NULL)
	{
		return STATUS_FAILURE;
	}

    switch(mmType)
    {
        case MMTYPE_CM_GET_KEY_REQ:
        {
#ifdef LOG_FLASH
            logEvent(MGMT_MSG, 0, EVENT_TYPE_CM_GET_KEY_REQ, NULL, 0);
#endif
            FM_Printf(FM_MMSG, "AKM>>CM_GET_KEY.REQ\n");
            peks = PEKS_NMK;
            avlnStatus =AVLN_STATUS_ASSOC_NO_PCO_CAP;
            break;
        }
        case MMTYPE_CM_GET_KEY_CNF:
        {
#ifdef LOG_FLASH
            logEvent(MGMT_MSG, 0, EVENT_TYPE_CM_GET_KEY_CNF, NULL, 0);
#endif
            FM_Printf(FM_MMSG, "AKM>>CM_GET_KEY.CNF\n");
            peks = PEKS_NMK;
            avlnStatus =AVLN_STATUS_ASSOC_NO_PCO_CAP;
            break;
        }
#ifdef UKE
        case MMTYPE_CM_SET_KEY_REQ:
        {
            FM_Printf(FM_MMSG, "AKM>>CM_SET_KEY.REQ\n");
            peks = akm->tekPeks;
            break;
        }
        case MMTYPE_CM_SET_KEY_CNF:
        {
            FM_Printf(FM_MMSG, "AKM>>CM_SET_KEY.CNF\n");
            peks = PEKS_NMK;
            break;
        }
#endif		
        default:
        {
        }
    }
    
    ret = AKM_SendEncPayloadInd(akm, mmType, db, peks, avlnStatus); 
    /* free the db */
	DMM_Free((u8 *)db);// Kiran Stack optimization
    return ret;
}
#ifdef UKE

eStatus AKM_SendUnEncMgmtMsg(sAkm *akm, u16 mmType)
{    
    u8          *pos = NULL;
    sCmGetKeyReq *req = NULL;
    sCmGetKeyCnf*cnf = NULL;
    sHpgpHdr    *hpgpHdr = NULL;
    sEvent     xdata *event = NULL;
    u16 eventSize;
    eStatus     ret = STATUS_SUCCESS;

    sLinkLayer  *linkl = (sLinkLayer *)HPGPCTRL_GetLayer(HP_LAYER_TYPE_LINK);
    sStaInfo    *staInfo = LINKL_GetStaInfo(linkl);

    switch(mmType)
    {
        case EVENT_TYPE_CM_GET_KEY_REQ:
        {
            FM_Printf(FM_MMSG, "AKM>>CM_GET_KEY.REQ-HASH\n");

            
            eventSize = MAX(sizeof(sCmGetKeyReq), HPGP_DATA_PAYLOAD_MIN); 
                        
            event =  EVENT_MgmtAlloc(sizeof(sCmGetKeyReq) + HASH_KEY_LEN,
									 EVENT_HPGP_MSG_HEADROOM);
            if(event == NULL)
            {
                FM_Printf(FM_ERROR, "EAF\n");
                return STATUS_FAILURE;
            }
            event->eventHdr.eventClass = EVENT_CLASS_MSG;
            event->eventHdr.type = mmType;

            LINKL_FillHpgpHdr((sHpgpHdr *)event->buffDesc.buff,
                            akm->peerTei,
                            akm->peerMacAddr,
                            staInfo->snid,
							0,
							HPGP_EKS_NONE);
            
            pos = event->buffDesc.dataptr;
            req = (sCmGetKeyReq *)pos;
            req->reqType = 0; // Direct
            req->reqKeyType = akm->keyType;
#ifdef KEY_SPEC
            memcpy(req->nid, staInfo->nid, NID_LEN);
            memcpy(req->myNonce, akm->myNonce, 4);
#endif
            req->pid = akm->pid; 
            req->prn = akm->prn;
            req->pmn = akm->pmn; 
            pos += sizeof(sCmGetKeyReq);
            // Generate hash key - 384 Len
            // Store 1st hash key
            FillRandomNumber(pos, HASH_KEY_LEN);
            memcpy(&hashKey[0], pos, HASH_KEY_LEN);
            
            
            FM_HexDump(FM_ERROR,"hashKeytx", hashKey, 20);
            
            pos += HASH_KEY_LEN;
            break;
        }
        case EVENT_TYPE_CM_GET_KEY_CNF:
        {            
            FM_Printf(FM_MMSG, "AKM>>CM_GET_KEY.CNF-HASH\n");
        
            
            eventSize = MAX(sizeof(sCmGetKeyCnf), HPGP_DATA_PAYLOAD_MIN); 
            
            event =  EVENT_MgmtAlloc(sizeof(sCmGetKeyCnf) + HASH_KEY_LEN, EVENT_HPGP_MSG_HEADROOM);
            if(event == NULL)
            {
                FM_Printf(FM_ERROR, "EAF\n");
                return STATUS_FAILURE;
            }

        
        
            event->eventHdr.eventClass = EVENT_CLASS_MSG;
            event->eventHdr.type = mmType;

            
            LINKL_FillHpgpHdr((sHpgpHdr *)event->buffDesc.buff,
                            akm->peerTei,
                            akm->peerMacAddr,
                            staInfo->snid,
                            0,
                            HPGP_EKS_NONE);

            pos = event->buffDesc.dataptr;
            cnf = (sCmGetKeyCnf *)pos;
            cnf->result = 0; // Key Confirm
            cnf->reqKeyType = akm->keyType;
#ifdef KEY_SPEC
            memcpy(cnf->nid, staInfo->nid, NID_LEN);
            memcpy(cnf->myNonce, akm->myNonce, 4);            
            memcpy(cnf->yourNonce, akm->yourNonce, 4);
#endif
            cnf->pid = akm->pid; 
            cnf->prn = akm->prn;
            cnf->pmn = akm->pmn;
#if 1

            cnf->eks = akm->tekPeks;

#else
            if(akm->eks < 2)
            {
                akm->eks = 2;
            }
            else
            {
                akm->eks++;
            }


            cnf->eks = akm->eks;

#endif            
            
            pos += sizeof(sCmGetKeyCnf);
            // Generate hash key - 384 Len
            // Store 2nd hash key
            FillRandomNumber(pos, HASH_KEY_LEN);
            memcpy(&hashKey[HASH_KEY_LEN], pos, HASH_KEY_LEN);
            
           // FM_HexDump(FM_ERROR,"hashKeytx", &hashKey[HASH_KEY_LEN], 20);
            
            // Generate tek
            genTek = 1;
            pos += HASH_KEY_LEN;
            break;
        }
        default:
            return ret; 
            break;
          
    }

    

    //event->buffDesc.datalen = eventSize;

    event->buffDesc.datalen = pos - event->buffDesc.dataptr;

   // EVENT_Assert(event);//kiran stack optimization
	assert((event->buffDesc.dataptr >= event->buffDesc.buff)&&
           ((event->buffDesc.dataptr - event->buffDesc.buff + 
             event->buffDesc.datalen) <= event->buffDesc.bufflen));
    
    ret = MUXL_TransmitMgmtMsg(event);
    /* note: the event is freed by MUXL if the TX is successful */
    if(ret == STATUS_FAILURE)
    {
        //EVENT_Free(event);
        DMM_Free((u8 *)event);//KIran stack optimization
    }
    
    return ret;    
}

#endif

eStatus AKM_DeliverEvent(u16 eventType, uEventParam *eventParam)
{
    sEvent    xdata   *event = NULL;
    sLinkLayer   *linkl = (sLinkLayer *)HPGPCTRL_GetLayer(HP_LAYER_TYPE_LINK);
    u8           *pos; 

    event = EVENT_Alloc(sizeof(uEventBody), 0);
    if(event == NULL)
    {
        FM_Printf(FM_ERROR, "AKM:EAF\n");
        return STATUS_FAILURE;
    }
    event->eventHdr.eventClass = EVENT_CLASS_CTRL;
    event->eventHdr.type = eventType;

    switch(eventType)
    {
        case EVENT_TYPE_AUTH_RSP:
        {
            pos = event->buffDesc.dataptr;
            memcpy(pos, &eventParam->authRsp, sizeof(eventParam->authRsp));
            event->buffDesc.datalen = sizeof(eventParam->authRsp);
            break;
        }
        case EVENT_TYPE_AUTH_IND:
        {
            pos = event->buffDesc.dataptr;
            memcpy(pos, &eventParam->authInd, sizeof(eventParam->authInd));
            event->buffDesc.datalen = sizeof(eventParam->authInd);
            break;
        }
        default:
        {
        }
    }
    
    /* deliver the event to the upper layer */
#ifdef CALLBACK
    linkl->deliverEvent(linkl->eventcookie, event);
#else
    CTRLL_ReceiveEvent(linkl->eventcookie, event);
#endif
    
    return STATUS_SUCCESS;
}

#ifdef UKE

eStatus AKM_ProcGetKeyReq(sAkm *akm, sEvent *event)
{
    sCmGetKeyReq       *req = NULL;
    u8                 *pos = NULL;
    
	sHpgpHdr           *hpgpHdr = (sHpgpHdr *)event->buffDesc.buff;
    eStatus             ret;
	sLinkLayer  *linkl = (sLinkLayer *)HPGPCTRL_GetLayer(HP_LAYER_TYPE_LINK);
	sStaInfo    *staInfo = LINKL_GetStaInfo(linkl);
    
    pos = event->buffDesc.dataptr;
    req = (sCmGetKeyReq *)pos;

    if ((req->pmn != 1) && (req->pmn != 0xFF)) {
        /* perform the basic verification */
        if( (akm->pid != req->pid) ||
            (akm->prn != req->prn) ||
            ((akm->pmn+1) != req->pmn) )
        {

            return STATUS_FAILURE;
        }
    }
            
    akm->pid = req->pid;
    akm->prn = req->prn;

#if 0
    ukePeer = LinkL_GetUKEPeer(linkl);
       
    if ((ukePeer, hpgpHdr->macAddr, MAC_ADDR_LEN) ||
        (akm->pid != AUTH_PID_NMK_WITH_UKE))
    {
        return STATUS_FAILURE;
    }
    
#endif

    akm->peerTei = hpgpHdr->tei;
    akm->peerMacAddr = hpgpHdr->macAddr;

    if ((req->reqType == 0) &&(req->reqKeyType == KEY_TYPE_HASH_KEY))
    {
        akm->pmn = 2;
        akm->keyType = KEY_TYPE_HASH_KEY;
#ifdef KEY_SPEC
        memcpy(akm->yourNonce, req->myNonce, 4)
#endif
        FillRandomNumber(akm->myNonce, 4);
        pos += sizeof(sCmGetKeyReq);
        memcpy(&hashKey[0], pos, HASH_KEY_LEN);
        
        STM_StartTimer(akm->TekTimer, HPGP_TEK_LIFETIME);
                    
        /* Send CM_GET_KEY.CNF */
        ret = AKM_SendUnEncMgmtMsg(akm, MMTYPE_CM_GET_KEY_CNF); 
              
    }

    return ret;
}

#endif

void AKM_SendAuthInd (sAkm *akm, u8 keyType, u8 status)
{

	uEventParam eventParam;
    sLinkLayer  *linkl = (sLinkLayer *)HPGPCTRL_GetLayer(HP_LAYER_TYPE_LINK);
    sStaInfo    *staInfo = LINKL_GetStaInfo(linkl);

		/*Compiler warning suppression*/
		akm = akm;
#ifdef UKE
    
    // Stop TEK life timer
    STM_StopTimer(akm->TekTimer);
   // rajan STM_FreeTimer(akm->TekTimer);
    // Make TEK invalid
    memset(akm->tek, 0, 16);

#endif

    eventParam.authInd.keyType = keyType;

    eventParam.authInd.secMode  = staInfo->secMode;

    eventParam.authInd.result = status;

    if(keyType == KEY_TYPE_NMK)
    {
        LINKL_SetSecurityMode(linkl, SEC_MODE_SC);    

    }
#ifdef UKE

    if ((status == STATUS_FAILURE) &&
        (hashKey != NULL))
        
    {
        DMM_Free(hashKey);
        hashKey = NULL;
    }

#endif

    AKM_DeliverEvent(EVENT_TYPE_AUTH_IND, &eventParam);



}

eStatus AKM_ProcEncPayload(sAkm *akm, u16 mmType, sDb *db, sScb *scb) 
{
    u8          *pos = NULL;
    sMmHdr      *mmh = NULL;
    sCmGetKeyReq * req = NULL;
    sCmSetKeyReq * setReq = NULL;
    sCmGetKeyCnf *cnf = NULL;
    sCmSetKeyCnf *setCnf = NULL;
    eStatus      ret = STATUS_FAILURE;
    sLinkLayer  *linkl = (sLinkLayer *)HPGPCTRL_GetLayer(HP_LAYER_TYPE_LINK);
    sStaInfo    *staInfo = LINKL_GetStaInfo(linkl);

    db->buffDesc.dataptr += sizeof(sEth2Hdr);
    db->buffDesc.datalen -= sizeof(sEth2Hdr);
    mmh = (sMmHdr *) db->buffDesc.dataptr;
    

    if ((mmType != HPGP_MMTYPE_ALL) && (le16_to_cpu(mmh->mmtype) != mmType))
        return STATUS_FAILURE;
    
    db->buffDesc.dataptr += sizeof(sMmHdr);
    db->buffDesc.datalen -= sizeof(sMmHdr);
    pos = db->buffDesc.dataptr;
    switch(le16_to_cpu(mmh->mmtype))
    {
        case MMTYPE_CM_GET_KEY_REQ:
        {
#ifdef LOG_FLASH
            logEvent(MGMT_MSG, 0, EVENT_TYPE_CM_GET_KEY_REQ, &scb->tei, 1);
#endif
            FM_Printf(FM_MMSG, "AKM<<CM_GET_KEY.REQ\n");
            /* Get KEY REQ msg body */
            req = (sCmGetKeyReq *)pos;
            if ((req->reqType == 0) &&(req->reqKeyType == KEY_TYPE_NEK))
            {
                /* TODO: check if the device is in CCo mode */
               
                
                akm->pmn = 0xFF; 
                akm->keyType = KEY_TYPE_NEK;
#ifdef KEY_SPEC
                memcpy(akm->yourNonce, req->myNonce, 4);
                FillRandomNumber(akm->myNonce, 4);
#endif
                /* encode CM_GET_KEY.CNF */
                ret = AKM_SendMgmtMsg(akm, MMTYPE_CM_GET_KEY_CNF); 
                if ( (ret == STATUS_SUCCESS ) && scb)
                {
                    scb->staStatus.fields.authStatus = 1; 

                    /*TODO: call the nsm to broadcast */
                }
                else
                {
      //              FM_Printf(FM_ERROR, "authStatus failed:%bu\n", 
        //                        scb->staStatus.fields.authStatus);
                }
            }
            break;
        }
        case MMTYPE_CM_GET_KEY_CNF:
        {
            sEvent xdata * cpltEvent = EVENT_Alloc(0, EVENT_HPGP_CTRL_HEADROOM);
#ifdef LOG_FLASH
            logEvent(MGMT_MSG, 0, EVENT_TYPE_CM_GET_KEY_CNF, &scb->tei, 1);
#endif
            FM_Printf(FM_MMSG, "AKM<<CM_GET_KEY.CNF\n");
            /* Get KEY CNF msg body */
            cnf = (sCmGetKeyCnf *)pos;
            if (cnf->result == 0)
            {
                if (cnf->reqKeyType == KEY_TYPE_NEK) 
                {

#ifdef KEY_SPEC
                     if(memcmp(akm->myNonce, cnf->yourNonce, 4) != 0)
                     {
//                         FM_Printf(FM_ERROR, "Unmatched nonce\n");
                         break;

                     }
#endif
                     staInfo->nekEks = cnf->eks; 
                     pos += sizeof(sCmGetKeyCnf);
                     memcpy(staInfo->nek, pos, ENC_KEY_LEN);
#ifndef RELEASE
//                     FM_Printf(FM_HINFO, "NEK EKS:%bu\n", staInfo->nekEks);
  //                   FM_HexDump(FM_HINFO, "NEK:", staInfo->nek, ENC_KEY_LEN);
#endif
#ifdef HPGP_HAL
                     /* set the NEK to the LMAC for the STA */
                     HHAL_AddNEK(staInfo->nekEks, staInfo->nek);
#endif
                     staInfo->staStatus.fields.authStatus = 1; 


                     AKM_SendAuthInd(akm, KEY_TYPE_NEK, STATUS_SUCCESS);
                     if(cpltEvent != NULL)
                     {
                         cpltEvent->eventHdr.eventClass = EVENT_CLASS_CTRL;
                         cpltEvent->eventHdr.type = EVENT_TYPE_AUTH_CPLT;
                         LINKL_SendEvent(linkl,cpltEvent);
                     }                     
#ifdef MULTIDEV                 
                     if(devNum != 0)
                     {
                        sHaLayer *hal = HOMEPLUG_GetHal();
                        hal->macAddr[5] = hal->macAddr[5] + 1;
                        devNum--;
                 
                        HTM_ResetNsm();
                     }
#endif
                     ret = STATUS_SUCCESS;
                }
            }
            break;
        }
#ifdef UKE
		
        case MMTYPE_CM_SET_KEY_REQ:
        {
                            
            FM_Printf(FM_MMSG, "AKM<<CM_SET_KEY.REQ\n");
            /* Get KEY REQ msg body */
            setReq = (sCmSetKeyReq *)pos;
            if ((setReq->keyType == KEY_TYPE_NMK))
            {             
                akm->pid = AUTH_PID_NMK_WITH_UKE;
                akm->pmn = 0xFF; 
                akm->keyType = KEY_TYPE_NMK;
#ifdef KEY_SPEC
                memcpy(akm->yourNonce, setReq->myNonce, 4);
                FillRandomNumber(akm->myNonce, 4);
#endif
                /* encode CM_SET_KEY.CNF */
                ret = AKM_SendMgmtMsg(akm, MMTYPE_CM_SET_KEY_CNF); 
                if (ret == STATUS_SUCCESS )
                {
                    // Set NMK   
                    staInfo->nmkPeks = setReq->newEks;
                    memcpy(staInfo->nmk, setReq->pNewKey, ENC_KEY_LEN);                    
                }
                else
                {
                    FM_Printf(FM_ERROR, "Set NMK Failed\n");
                }
            }
            FM_HexDump(FM_HINFO, "NMK:", staInfo->nmk, ENC_KEY_LEN);
            //os_switch_task();
            // Send out get key req
#if 0            
            akm->keyType = KEY_TYPE_NEK;
            akm->pid = AUTH_PID_NEW_STA; 
            akm->prn = rand() & 0xFFFF;
            akm->pmn = 1; 
            ret = AKM_SendMgmtMsg(akm, MMTYPE_CM_GET_KEY_REQ);
            /* start the key timer for a response */
            STM_StartTimer(akm->akmTimer, HPGP_TIME_KEY);
            akm->state = AKM_STATE_WAITFOR_GET_NEK_CNF;
#else

            akm->state = AKM_STATE_READY;

            AKM_SendAuthInd(akm, KEY_TYPE_NMK, STATUS_SUCCESS);
                                 


#endif


            ret = STATUS_SUCCESS;
            break;
        }
        case MMTYPE_CM_SET_KEY_CNF:
        {
         
            FM_Printf(FM_MMSG, "AKM<<CM_SET_KEY.CNF\n");

            /* Get KEY CNF msg body */

            setCnf = (sCmSetKeyCnf *)pos;
#ifdef KEY_SPEC
            if(memcmp(akm->myNonce, setCnf->yourNonce, 4) != 0)
            {
                FM_Printf(FM_ERROR, "Unmatched nonce\n");
                break;

            }
            else
#endif
            {
                  
                ret = STATUS_SUCCESS;                    
                
                AKM_SendAuthInd(akm, KEY_TYPE_NMK, setCnf->result);
            }
            
            break;
        }
#endif		
        default:
        {
        }
    }
    return ret;
}



sDb *AKM_DecodeEncPayloadInd(sAkm *akm, sEvent *event) 
{
    sCmEncryPayloadInd *ind = NULL;
    sDb                *db = NULL;
	u8 crc2[4];
    u8                 *pos = NULL;
    u8                  rfLen = 0;
    u32                 crc = 0;
    u8          		*crct;
    sHpgpHdr           *hpgpHdr = (sHpgpHdr *)event->buffDesc.buff;
#ifdef UKE	
    sScb               * scb;
#endif
    u8 ret;
#ifdef AUTH_AES

	sLinkLayer	*linkl = (sLinkLayer *)HPGPCTRL_GetLayer(HP_LAYER_TYPE_LINK);
	sStaInfo *staInfo = LINKL_GetStaInfo(linkl);

#ifdef DELAY_HACK
		u8 XDATA	*decbuf;
#endif
		u8 XDATA	iv[20];
		AES_KEY 	key;
	
		
#ifdef DELAY_HACK
		decbuf = DMM_Alloc(256);
		if ((decbuf == NULL)){
	//		FM_Printf(FM_ERROR, "Can't alloc mem\n");
			return STATUS_FAILURE;
		}
#endif
#endif
	/*Compiler warning suppression*/		
	ret = ret;

    akm->peerTei = hpgpHdr->tei;
    akm->peerMacAddr = hpgpHdr->macAddr;

    pos = event->buffDesc.dataptr;
    ind = (sCmEncryPayloadInd *)pos;

    if ((ind->pmn != 1) && (ind->pmn != 0xFF)) {
        /* perform the basic verification */
        if( (akm->pid != ind->pid) ||
            (akm->prn != ind->prn) ||
            ((akm->pmn+1) != ind->pmn) )
        {
//            FM_Printf(FM_ERROR, "Unmatched prn\n");
            return NULL;
        }
    }
    

    akm->peerTei = hpgpHdr->tei;
    akm->peerMacAddr = hpgpHdr->macAddr;

    akm->pid = ind->pid;
    akm->prn = ind->prn;
    akm->pmn = ind->pmn; 


    pos += sizeof(sCmEncryPayloadInd);
    
    db = DB_Alloc(le16_to_cpu(ind->len) + HPGP_ENC_MM_TAIL_LEN + CRC_SIZE  
                  + HPGP_ENC_MM_PAD_MAX, 0); 
    if (db == NULL)
    {
        FM_Printf(FM_ERROR, "dbfail\n");
        return NULL;
    }
    /* TODO: decrypt the payload */

    db->buffDesc.datalen = event->buffDesc.datalen - sizeof(sCmEncryPayloadInd);
#ifdef AUTH_AES
    if(ind->peks == KEY_TYPE_NMK)
    {
        //FM_Printf(FM_HINFO, "decode KEY_TYPE_NMK\n");
        //AES_set_encrypt_key((unsigned char*)staInfo->nmk, 8*AES_BLOCK_SIZE, &key);
        aes_set_key((unsigned char*)staInfo->nmk, AES_BLOCK_SIZE, &key );//kiran code optimization. Caller multiplies by 8 and called function divide by 8
		//To reduce stack aes_set_key is directly called
    }
#ifdef UKE	
    else
	 // tek
    {
        scb = (sScb*)hpgpHdr->scb;
        //AES_set_encrypt_key(akm->tek, 8*AES_BLOCK_SIZE, &key);
		aes_set_key( akm->tek, AES_BLOCK_SIZE, key );//kiran code optimization. Caller multiplies by 8 and called function divide by 8
		//To reduce stack aes_set_key is directly called
    }
#endif
	memcpy(iv, ind->iv, ENC_IV_LEN);
#ifdef DELAY_HACK
	if(db->buffDesc.datalen > 256)
	{
		FM_Printf(FM_ERROR, "Excess Buff len\n");
        return STATUS_FAILURE;
	}	
    AES_cbc_encrypt((unsigned char*)pos, 
					(unsigned char*)decbuf, 
					db->buffDesc.datalen, 
					&key, 
					(unsigned char*)iv,
					AES_DECRYPT);
		
	memcpy(db->buffDesc.dataptr, decbuf, db->buffDesc.datalen);
	DMM_Free (decbuf);
#else
    ret = AES_cbc_encrypt((unsigned char*)pos, 
	    			(unsigned char*)db->buffDesc.dataptr, 
	    			db->buffDesc.datalen, 
	    			&key, 
	    			(unsigned char*)iv,
	    			AES_DECRYPT);
    if(ret != STATUS_SUCCESS)
    {
        //DB_Free(db);
        DMM_Free((u8 *)db);// Kiran stack optimization
        return NULL;
    }
#endif	
#else
    memcpy(db->buffDesc.dataptr, pos, db->buffDesc.datalen);
#endif
   
    /* rf length */
    rfLen = *(db->buffDesc.dataptr + db->buffDesc.datalen - 1);


    db->buffDesc.dataptr += rfLen;
    db->buffDesc.datalen = le16_to_cpu(ind->len);
#ifdef P8051
//FM_Printf(FM_ERROR, "AKM: recv rf len = %bu, msg len = %bu.\n", rfLen, ind->len);
#else
//FM_Printf(FM_ERROR, "AKM: recv rf len = %d, msg len = %d.\n", rfLen, ind->len);
#endif
    /* perform crc verification */
    crc = chksum_crc32(db->buffDesc.dataptr, db->buffDesc.datalen);
   crct = (u8*)&crc;

    crc2[0] = crct[3];
    crc2[1] = crct[2];
    crc2[2] = crct[1];
    crc2[3] = crct[0];
    pos = db->buffDesc.dataptr + db->buffDesc.datalen; 
    if (memcmp(pos, &crc2, sizeof(u32))) 
    {    
#ifndef RELEASE
//        FM_Printf(FM_ERROR, "crcissue no match(tx:0x%08x,rx:0x%08x)\n", *(u32*)pos, crc);
		FM_Printf(FM_ERROR, "crcissue\n");

#endif
    }
    return db;
}



u8 AKM_ProcReady(sAkm *akm, sEvent *event)
{
    sDb *db = NULL;
    sHpgpHdr *hpgpHdr = NULL;
    u8  state = AKM_STATE_READY;
    sLinkLayer  *linkl = (sLinkLayer *)HPGPCTRL_GetLayer(HP_LAYER_TYPE_LINK);
    sStaInfo    *staInfo = LINKL_GetStaInfo(linkl);


    if( event->eventHdr.eventClass == EVENT_CLASS_MSG)
    {
        switch(event->eventHdr.type)
        {

            case EVENT_TYPE_CM_ENCRY_PAYLOAD_RSP:
                
                //FM_Printf(FM_ERROR, "AKM:EVENT_TYPE_CM_ENCRY_PAYLOAD_RSP\n");

                break;
                
            case EVENT_TYPE_CM_ENCRY_PAYLOAD_IND:
            {
//FM_Printf(FM_ERROR, "AKM: recv the Enc Payload Ind\n");
                /* decrpt the messsage */
                db = AKM_DecodeEncPayloadInd(akm, event);
                if (db != NULL)
                {
                    hpgpHdr = (sHpgpHdr *)event->buffDesc.buff;
                    /* Process a request */
                    AKM_ProcEncPayload(akm, HPGP_MMTYPE_ALL, db, hpgpHdr->scb);
                    /* free the db */
                   // DB_Free(db);
				   DMM_Free((u8 *)db);// Kiran stack optimization
                }
                else
                {
                    //send EVENT_TYPE_CM_ENCRY_PAYLOAD_RSP with fail code
                    AKM_SendEncPayloadRsp(akm, 1);
                }
                
               
                break;
             }
#ifdef UKE			
            case EVENT_TYPE_CM_GET_KEY_REQ:
            {                  

                if ((staInfo->secMode != SEC_MODE_SC_ADD) &&
                    (staInfo->secMode != SEC_MODE_SC_JOIN))
                {

                  //  FM_Printf(FM_ERROR, "CM_GET_KEY_REQ drop\n");
                    break;
                }
                    
                if(hashKey == NULL)
                {
                    hashKey = DMM_Alloc(768); // TODO release this memory
                }
                if(hashKey == NULL)
                {
                   // FM_Printf(FM_ERROR, "Hash key alloc fail\n");
                    break;
                }
                

                FM_Printf(FM_MMSG, "AKM<<CM_GET_KEY.REQ-HASH\n");
    
                AKM_ProcGetKeyReq(akm, event);
             
                break;
            }
#endif			
            default:
            {
            }
        }
    }
    else if( event->eventHdr.eventClass == EVENT_CLASS_CTRL)
    {
        switch(event->eventHdr.type)
        {
#ifdef UKE
		
            case EVENT_TYPE_ASSOC_IND:
            {
                if(staInfo->secMode == SEC_MODE_SC_ADD)
                {
                    // UKE
                    
                    hpgpHdr = (sHpgpHdr *)event->buffDesc.buff;                    

                    if(hashKey == NULL)
                    {
                        hashKey = DMM_Alloc(768); // TODO release this memory
                    }
                    if(hashKey == NULL)
                    {
                        FM_Printf(FM_ERROR, "Hash key alloc fail\n");
                        break;
                    }
                    /* start a new UKE authentication */                     

                    akm->peerTei = ((sScb*)hpgpHdr->scb)->tei;
                    
                    akm->peerMacAddr = ((sScb*)hpgpHdr->scb)->macAddr;
                    
                    akm->keyType = KEY_TYPE_HASH_KEY;
                    akm->pid = AUTH_PID_NMK_WITH_UKE; 
                    akm->prn = rand() & 0xFFFF;
                    akm->pmn = 1; 
#ifdef KEY_SPEC
                    FillRandomNumber(akm->myNonce, 4);                
#endif                   
                        // start UKE  timer
                        
                    STM_StartTimer(akm->TekTimer, HPGP_TEK_LIFETIME);
                        
                    /* send CM_GET_KEY.REQ */
                    AKM_SendUnEncMgmtMsg(akm, MMTYPE_CM_GET_KEY_REQ);
                    /* start the key timer for a response */
                    STM_StartTimer(akm->akmTimer, HPGP_TIME_KEY);
                    // UKE: Send out AKM get key req                    
                    //os_switch_task();
                    akm->txRetryCnt++;
                    state = AKM_STATE_WAITFOR_HASH_GET_CNF;
                }

                break;
            }
#endif          
            case EVENT_TYPE_AUTH_REQ:
            {
                sAuthReq *authreq = (sAuthReq *)event->buffDesc.dataptr;

                if( authreq->authType == AUTH_TYPE_NEK) 
                {
                    /* start a new NEK authentication */ 
                    akm->peerTei = staInfo->ccoScb->tei;
                    akm->peerMacAddr = staInfo->ccoScb->macAddr;
                    akm->keyType = KEY_TYPE_NEK;
                    akm->pid = AUTH_PID_NEW_STA; 
                    akm->prn = rand() & 0xFFFF;
                    akm->pmn = 1; 
#ifdef KEY_SPEC
                    FillRandomNumber(akm->myNonce, 4);
#endif
                    FillRandomNumber(akm->nmkIv, ENC_IV_LEN);

                    /* send CM_GET_KEY.REQ */
                    AKM_SendMgmtMsg(akm, MMTYPE_CM_GET_KEY_REQ);
                    /* start the key timer for a response */
                    STM_StartTimer(akm->akmTimer, HPGP_TIME_KEY);
                    akm->txRetryCnt++;
                    state = AKM_STATE_WAITFOR_GET_NEK_CNF;
                }
                break;
            }
            case EVENT_TYPE_AKM_STOP:
            {
#ifdef HPGP_HAL
                /* remvoe the NEK from the LMAC for the STA */
                HHAL_RemoveNEK(staInfo->nekEks);
#endif
                state = AKM_STATE_INIT;
                break;
            }
            default:
            {
            }
        }
    }
    return state;
}




void AKM_ProcEvent(sAkm *akm, sEvent *event)
{

    u8         *pos = NULL;
    sDb        *db = NULL;
    uEventParam eventParam;
    sLinkLayer *linkl = (sLinkLayer *)HPGPCTRL_GetLayer(HP_LAYER_TYPE_LINK);
    sStaInfo    *staInfo = LINKL_GetStaInfo(linkl);
    
#ifdef UKE
    if ((event->eventHdr.eventClass == EVENT_CLASS_CTRL) && 
        (event->eventHdr.type == EVENT_TYPE_TIMER_TEK_IND))
    {
        FM_Printf(FM_HINFO, "AKM:UKE Timeout\n");
        // Make tek invalid
        memset(akm->tek, 0, 16);
               
        if ((staInfo->secMode == SEC_MODE_SC_JOIN) ||
            (staInfo->secMode == SEC_MODE_SC_ADD))
        {
            staInfo->secMode = SEC_MODE_SC;
            AKM_SendAuthInd(akm, KEY_TYPE_NMK, STATUS_FAILURE);

            akm->state = AKM_STATE_INIT;
            
        }

        return;

    }

#endif
 
    switch(akm->state)
    {
        case AKM_STATE_INIT:
        {
            if( (event->eventHdr.eventClass == EVENT_CLASS_CTRL) && 
                (event->eventHdr.type == EVENT_TYPE_AKM_START) )
            {
                pos = event->buffDesc.dataptr;
                akm->akmMode = *pos;
                pos++; 
#ifndef RELEASE
#ifdef P8051
FM_Printf(FM_ERROR, "AKM:AKM_START(%bu,%bu)\n", akm->akmMode, *pos);
#else
FM_Printf(FM_ERROR, "AKM:AKM_START(%d)\n", *pos);
#endif
#endif
 
                akm->state = AKM_STATE_READY;
            }
            break;
        }
        case AKM_STATE_READY:
        {
            akm->state = AKM_ProcReady(akm, event);
            break;
        }
        case AKM_STATE_WAITFOR_GET_NEK_CNF:
        {
            if( event->eventHdr.eventClass == EVENT_CLASS_MSG)
            {
                if (event->eventHdr.type ==  EVENT_TYPE_CM_ENCRY_PAYLOAD_IND)
                {
                    /* decrpt the messsage */
                    db = AKM_DecodeEncPayloadInd(akm, event);
                    if (db != NULL)
                    {
                        if (AKM_ProcEncPayload(akm, MMTYPE_CM_GET_KEY_CNF, db, NULL)
                                == STATUS_SUCCESS)
                        {
                            STM_StopTimer(akm->akmTimer);
                            akm->txRetryCnt = 0;
                            eventParam.authRsp.authType = AUTH_TYPE_NEK;
                            eventParam.authRsp.result = STATUS_SUCCESS;
                            AKM_DeliverEvent(EVENT_TYPE_AUTH_RSP, &eventParam);
#ifdef AKM
	 			            SNAM_StartTEIRenew();
#endif
                            akm->state = AKM_STATE_READY;
                        }
                        /* free the db */
                        DB_Free(db);                     
                    }
                    else
                    {
                        //send EVENT_TYPE_CM_ENCRY_PAYLOAD_RSP with fail code    
                        AKM_SendEncPayloadRsp(akm, 1);
                    }
                }
            }
            else if( event->eventHdr.eventClass == EVENT_CLASS_CTRL)
            {
                if (event->eventHdr.type == EVENT_TYPE_TIMER_KEY_IND)
                {
                    if( akm->txRetryCnt <= HPGP_TX_RETRY_MAX)
                    {
                        /* resend the message */
                        AKM_SendMgmtMsg(akm, MMTYPE_CM_GET_KEY_REQ); 
                        /* start the key timer for a response */
                        STM_StartTimer(akm->akmTimer, HPGP_TIME_KEY);
                        akm->txRetryCnt++;
                        //stay in the same state
                    }
                    else
                    {
                        akm->txRetryCnt = 0;
                        /* retry exhausted */
                        eventParam.authRsp.authType = AUTH_TYPE_NEK;
                        eventParam.authRsp.result = STATUS_FAILURE;
                        /* deliver the event to the upper layer */
						AKM_SendAuthInd(akm, KEY_TYPE_NEK, STATUS_FAILURE);
                    }
                }
            }
            break;
        }
#ifdef UKE		
        case AKM_STATE_WAITFOR_HASH_GET_CNF:
        {
            if( event->eventHdr.eventClass == EVENT_CLASS_MSG)
            {
                if (event->eventHdr.type ==  EVENT_TYPE_CM_GET_KEY_CNF)
                {
                    sCmGetKeyCnf *cnf;
                    pos = event->buffDesc.dataptr;
                    FM_Printf(FM_MMSG, "AKM<<CM_GET_KEY.CNF-HASH\n");
                    /* Get KEY CNF msg body */
                    cnf = (sCmGetKeyCnf *)pos;
                    if (cnf->result == 0)
                    {
                        if (cnf->reqKeyType == KEY_TYPE_HASH_KEY) 
                        {                 
                         
                             /* perform the basic verification */
                             if((akm->pid != cnf->pid) ||
                                 (akm->prn != cnf->prn) ||
                                 ((akm->pmn+1) != cnf->pmn) ||
                                 (cnf->pmn == 0xFF))
                             {
  //                               FM_Printf(FM_ERROR, "Unmatched pmn\n");
                                 break;
                             }
#ifdef KEY_SPEC
                             if(memcmp(akm->myNonce, cnf->yourNonce, 4) != 0)
                             {
//                                 FM_Printf(FM_ERROR, "Unmatched nonce\n");
                                 break;
 
                             }
#endif
                             akm->txRetryCnt = 0;
                             STM_StopTimer(akm->akmTimer);
#ifdef KEY_SPEC
                             memcpy(akm->yourNonce, cnf->myNonce, 4);
#endif
                             pos += sizeof(sCmGetKeyCnf);
                             memcpy(&hashKey[HASH_KEY_LEN], pos, HASH_KEY_LEN);
                             /* Generate TEK usinf Hash1 and Hash2 */
                             genTek = 1;
                             // TODO Send NMK using SET_KEY.REQ
                             /* send CM_SET_KEY.REQ */
                             akm->keyType = KEY_TYPE_NMK;
                             akm->pid = AUTH_PID_NMK_WITH_UKE;
                             akm->pmn = 3; 
                    
                             AKM_SendMgmtMsg(akm, MMTYPE_CM_SET_KEY_REQ);
                             /* start the key timer for a response */
                             STM_StartTimer(akm->akmTimer, HPGP_TIME_KEY);
                             akm->txRetryCnt++;
                             akm->state = AKM_STATE_WAITFOR_NMK_SET_CNF;
                        }
                    }
                }
            }
            else if( event->eventHdr.eventClass == EVENT_CLASS_CTRL)
            {
                switch(event->eventHdr.type)
                {
                    case EVENT_TYPE_TIMER_KEY_IND:
                    {
                        if( akm->txRetryCnt <= HPGP_TX_RETRY_MAX)
                        {
                            /* resend the message */
                            AKM_SendUnEncMgmtMsg(akm, MMTYPE_CM_GET_KEY_REQ); 
                            /* start the key timer for a response */
                            STM_StartTimer(akm->akmTimer, HPGP_TIME_KEY);
                            akm->txRetryCnt++;
                            //stay in the same state
                        }
                        else
                        {
                            akm->txRetryCnt = 0;
                            /* retry exhausted */

                            AKM_SendAuthInd(akm, KEY_TYPE_NMK, STATUS_FAILURE);
                                            

                        }
                        break;
                    }
                    default:
                    {
                    }
                }
            }
            break;
        }
        case AKM_STATE_WAITFOR_NMK_SET_CNF:
        {            
            if( event->eventHdr.eventClass == EVENT_CLASS_MSG)
            {
                if (event->eventHdr.type ==  EVENT_TYPE_CM_ENCRY_PAYLOAD_IND)
                {
                    /* decrpt the messsage */
                    db = AKM_DecodeEncPayloadInd(akm, event);
                    if (db != NULL)
                    {
                     
                        if (AKM_ProcEncPayload(akm, MMTYPE_CM_SET_KEY_CNF, db, NULL)
                                == STATUS_SUCCESS)
                        {
                            STM_StopTimer(akm->akmTimer);
                           
                            akm->txRetryCnt = 0;

                            akm->state = AKM_STATE_READY;                            

                            AKM_SendAuthInd(akm, KEY_TYPE_NMK, STATUS_SUCCESS);
                            
                            
                        }
                        /* free the db */
                        DB_Free(db);
                    }
                    else
                    {
                        //send EVENT_TYPE_CM_ENCRY_PAYLOAD_RSP with fail code    
                        AKM_SendEncPayloadRsp(akm, 1);
                    }
                }
            }
            else if( event->eventHdr.eventClass == EVENT_CLASS_CTRL)
            {
                switch(event->eventHdr.type)
                {
                    case EVENT_TYPE_TIMER_KEY_IND:
                    {
                        if( akm->txRetryCnt <= HPGP_TX_RETRY_MAX)
                        {
                            /* resend the message */
                            AKM_SendMgmtMsg(akm, MMTYPE_CM_SET_KEY_REQ); 
                            /* start the key timer for a response */
                            STM_StartTimer(akm->akmTimer, HPGP_TIME_KEY);
                            akm->txRetryCnt++;
                            //stay in the same state
                        }
                        else
                        {
                            akm->txRetryCnt = 0;
                            /* retry exhausted */

                            AKM_SendAuthInd(akm, KEY_TYPE_NMK, STATUS_FAILURE);
                            
                        }
                        break;
                    }
                    default:
                    {
                    }
                }
            }
            break;
        }
#endif
		
        default:
        {
        }
    }
}



eStatus AKM_Start(sAkm *akm, u8 mode, u8 newNek )
{
    sLinkLayer *linkl = (sLinkLayer *)HPGPCTRL_GetLayer(HP_LAYER_TYPE_LINK);
    sEvent xdata *event = NULL;
    u8 *pos = NULL;
    sStaInfo *staInfo = LINKL_GetStaInfo(linkl);

   
    event = EVENT_Alloc(2, EVENT_HPGP_CTRL_HEADROOM);
    if(event == NULL)
    {
        FM_Printf(FM_ERROR, "EAF\n");
        return STATUS_FAILURE;
    }
    event->eventHdr.eventClass = EVENT_CLASS_CTRL;
    event->eventHdr.type = EVENT_TYPE_AKM_START;
    pos = event->buffDesc.dataptr;
    *pos = mode; 
    pos++;
    *pos = newNek; 
    event->buffDesc.datalen = 2;
//FM_Printf(FM_ERROR, "AKM: Start (%bu).\n", newNek);

    akm->state = AKM_STATE_READY;

    if (newNek && (linkl->mode == LINKL_STA_MODE_CCO))
    {
        u8 i;
        u8          pwd[32];
           
        for (i = 0; i < 32; i++)
        {
            pwd[i] = rand() & 0xFF;
        }
        /* generate a NEK */            
        GenerateKey(pwd, 32, staInfo->nek);
        staInfo->nekEks = AKM_GetNewEks(&linkl->akm); 
#ifdef P8051
//                    FM_Printf(FM_HINFO, "AKM: NEK EKS: %bu.\n", staInfo->nekEks);
#else
//                    FM_Printf(FM_HINFO, "AKM: NEK EKS: %.2x.\n", staInfo->nekEks);
#endif
        FM_HexDump(FM_HINFO, "AKM:NEK:", staInfo->nek, ENC_KEY_LEN);
#ifdef HPGP_HAL
        /* set the NEK to the LMAC for the CCo */
       HHAL_AddNEK(staInfo->nekEks, staInfo->nek);
#endif

    }  
                

    LINKL_SendEvent(linkl, event);
    return STATUS_SUCCESS;
}


eStatus AKM_Stop(sAkm *akm)
{
	
    sLinkLayer *linkl = (sLinkLayer *)HPGPCTRL_GetLayer(HP_LAYER_TYPE_LINK);
	  sStaInfo    *staInfo = LINKL_GetStaInfo(linkl);
	
#if 0


    sLinkLayer *linkl = (sLinkLayer *)HPGPCTRL_GetLayer(HP_LAYER_TYPE_LINK);
    sEvent *event = NULL;
    event = EVENT_Alloc(0, EVENT_HPGP_CTRL_HEADROOM);
    if(event == NULL)
    {
        FM_Printf(FM_ERROR, "EAllocErr\n");
        return STATUS_FAILURE;
    }
    event->eventHdr.eventClass = EVENT_CLASS_CTRL;
    event->eventHdr.type = EVENT_TYPE_AKM_STOP;
    LINKL_SendEvent(linkl, event);

#else
	/* remvoe the NEK from the LMAC for the STA */
	HHAL_RemoveNEK(staInfo->nekEks);

	akm->state = AKM_STATE_INIT;

#endif
	return STATUS_SUCCESS;
}


void LINKL_AkmTimerHandler(void* cookie)
{
    sEvent xdata *event = NULL;
    sLinkLayer * linkl = (sLinkLayer *)cookie;

    /* Generate a time event */
    event = EVENT_Alloc(0, EVENT_HPGP_CTRL_HEADROOM);
    if(event == NULL)
    {
        FM_Printf(FM_ERROR, "EAF\n");
        return;
    }
    event->eventHdr.eventClass = EVENT_CLASS_CTRL;
    event->eventHdr.type = EVENT_TYPE_TIMER_KEY_IND;
    /* post the event to the event queue */
    LINKL_SendEvent(linkl, event);
}
    


eStatus AKM_Init(sAkm *akm)
{
    sLinkLayer *linkl = (sLinkLayer *)HPGPCTRL_GetLayer(HP_LAYER_TYPE_LINK);
#ifdef CALLBACK
    akm->akmTimer = STM_AllocTimer(LINKL_TimerHandler,
                        EVENT_TYPE_TIMER_KEY_IND, linkl);
#else
    akm->akmTimer = STM_AllocTimer(HP_LAYER_TYPE_LINK, 
                        EVENT_TYPE_TIMER_KEY_IND, linkl);
#endif
    if(akm->akmTimer == STM_TIMER_INVALID_ID)
    {
        return STATUS_FAILURE;
    }
#ifdef UKE	
    akm->TekTimer = STM_AllocTimer(HP_LAYER_TYPE_LINK,
                        EVENT_TYPE_TIMER_TEK_IND, linkl);
    if(akm->TekTimer == STM_TIMER_INVALID_ID)
    {
        return STATUS_FAILURE;
    }
#endif
#ifdef AKM_PRINT
#ifdef P8051
//	FM_Printf(FM_ERROR, "AKM: akm timer id: %bu\n", akm->akmTimer);
#else
	//FM_Printf(FM_ERROR, "AKM: akm timer id: %d\n", akm->akmTimer);
#endif
#endif
    akm->txRetryCnt = 0;

    srand(STM_GetTick());

    chksum_crc32gentab ();

    akm->state = AKM_STATE_INIT;
    return STATUS_SUCCESS;
}



/** =========================================================
 *
 * Edit History
 *
 * $Source: /home/cvsrepo/Hybrii_B_OSFW_Dev/firmware/hpgp/src/link/akm.c,v $
 *
 * $Log: akm.c,v $
 * Revision 1.18  2015/01/02 14:55:36  kiran
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
 * Revision 1.17  2014/11/11 14:52:58  ranjan
 * 1.New Folder Architecture espically in /components
 * 2.Modular arrangment of functionality in new files
 *    anticipating the need for exposing them as FW App
 *    development modules
 * 3.Other improvisation in code and .h files
 *
 * Revision 1.16  2014/10/28 16:27:43  kiran
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
 * Revision 1.15  2014/10/15 10:42:51  ranjan
 * small fixes in um
 *
 * Revision 1.14  2014/09/25 10:57:42  prashant
 * 1. GPIO API swapping issue fixed.
 * 2. Supported 1 to 512 frame length for uart.
 * 3. list.h file cleanup (code deleted).
 * 4. Supporting minirobo for mgmt frames.
 *
 * Revision 1.13  2014/09/05 09:28:18  ranjan
 * 1. uppermac cco-sta switching feature fix
 * 2. general stability fixes for many station associtions
 * 3. changed mgmt memory pool for many STA support
 *
 * Revision 1.12  2014/08/25 07:37:34  kiran
 * 1) RSSI & LQI support
 * 2) Fixed Sync related issues
 * 3) Fixed timer 0 timing drift for SDK
 * 4) MMSG & Error Logging in Flash
 *
 * Revision 1.11  2014/07/22 10:03:52  kiran
 * 1) SDK Supports Power Save
 * 2) Uart_Driver.c cleanup
 * 3) SDK app memory pool optimization
 * 4) Prints from STM.c are commented
 * 5) Print messages are trimmed as common no memory left in common
 * 6) Prins related to Power save and some other modules are in compilation flags. To enable module specific prints please refer respective module
 *
 * Revision 1.10  2014/07/05 09:16:27  prashant
 * 100 Devices support- only association tested, memory adjustments
 *
 * Revision 1.9  2014/06/19 17:13:19  ranjan
 * -uppermac fixes for lvnet and reset command for cco and sta mode
 * -backup cco working
 *
 * Revision 1.8  2014/06/12 13:15:43  ranjan
 * -separated bcn,mgmt,um event pools
 * -fixed datapath issue due to previous checkin
 * -work in progress. neighbour cco detection
 *
 * Revision 1.7  2014/06/11 13:17:47  kiran
 * UART as host interface and peripheral interface supported.
 *
 * Revision 1.6  2014/05/28 10:58:59  prashant
 * SDK folder structure changes, Uart changes, removed htm (UI) task
 * Varified - UM, LM - iperf (UDP/TCP), overnight test pass
 *
 * Revision 1.5  2014/05/12 08:09:57  prashant
 * Route fix, STA sync issue with AV CCO and handling discover bcn issue fix
 *
 * Revision 1.4  2014/03/10 05:58:10  ranjan
 * 1. added HomePlug BackupCCo feature. verified C&I test.(passed.) (bug 176)
 *
 * Revision 1.3  2014/02/27 10:42:47  prashant
 * Routing code added
 *
 * Revision 1.2  2014/01/10 17:17:53  yiming
 * check in Rajan 1/8/2014 code release
 *
 * Revision 1.6  2014/01/08 10:53:54  ranjan
 * Changes for LM OS support.
 * New Datapath FrameTask
 * LM and UM  datapath, feature verified.
 *
 * known issues : performance numbers needs revisit
 *
 * review : pending.
 *
 * Revision 1.5  2013/10/16 07:43:38  prashant
 * Hybrii B Upper Mac compiling issues and QCA fix, added default eks code
 *
 * Revision 1.4  2013/09/04 14:51:01  yiming
 * New changes for Hybrii_A code merge
 *
 * Revision 1.21  2013/07/12 08:56:36  ranjan
 * -UKE Push Button Security Feature.
 * Verified : DirectEntry Security Works.Datapath Works.
 *                 command SetSecMode for UKE works.
 * Added against bug-160
 *
 * Revision 1.20  2013/03/22 12:21:49  prashant
 * default FM_MASK and FM_Printf modified for USER INFO
 *
 * Revision 1.19  2012/12/18 12:17:46  prashant
 * Stability checkin
 *
 * Revision 1.18  2012/11/06 05:05:26  ranjan
 * -moved AES encryption to Hal Task
 * - verified link establishment is very stable
 *
 * Revision 1.17  2012/10/25 11:38:48  prashant
 * Sniffer code added for MAC_SAP, Added new commands in MAC_SAP for sniffer, bridge,
 *  hardware settings and peer information.
 *
 * Revision 1.16  2012/10/11 06:21:00  ranjan
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
 * Revision 1.15  2012/07/15 17:31:07  yuanhua
 * (1)fixed a potential memory overwriting in MUXL (2)update prints for 8051.
 *
 * Revision 1.14  2012/07/08 18:42:20  yuanhua
 * (1)fixed some issues when ctrl layer changes its state from the UCC to ACC. (2) added a event CNSM_START.
 *
 * Revision 1.13  2012/06/30 23:36:26  yuanhua
 * return the success status for LINKL_SendEvent() when RTX51 OS is used.
 *
 * Revision 1.12  2012/06/05 07:25:59  yuanhua
 * (1) add a scan call to the MAC during the network discovery. (2) add a tiny task in ISM for interrupt polling (3) modify the frame receiving integration. (4) modify the tiny task in STM.
 *
 * Revision 1.11  2012/06/04 23:34:02  son
 * Added RTX51 OS support
 *
 * Revision 1.10  2012/05/19 20:32:17  yuanhua
 * added non-callback option for the protocol stack.
 *
 * Revision 1.9  2012/05/19 05:05:15  yuanhua
 * optimized the timer handlers in CTRL and LINK layers.
 *
 * Revision 1.8  2012/05/17 05:05:58  yuanhua
 * (1) added the option for timer w/o callback (2) added task id and name.
 *
 * Revision 1.7  2012/05/12 04:11:46  yuanhua
 * (1) added list.h (2) changed the hal tx for the hw MAC implementation.
 *
 * Revision 1.6  2012/04/30 04:05:57  yuanhua
 * (1) integrated the HAL mgmt Tx. (2) various updates
 *
 * Revision 1.5  2012/04/13 06:15:11  yuanhua
 * integrate the HPGP protocol stack with the HAL for the beacon TX/RX and mgmt msg RX.
 *
 * Revision 1.4  2012/03/11 17:02:24  yuanhua
 * (1) added NEK auth in AKM (2) added NMA (3) modified hpgp layer data structures (4) added Makefile for linux simulation
 *
 * Revision 1.3  2011/09/18 01:32:08  yuanhua
 * designed the AKM for both STA and CCo.
 *
 * Revision 1.2  2011/09/09 07:02:31  yuanhua
 * migrate the firmware code from the greenchip to the hybrii.
 *
 * Revision 1.3  2011/07/22 18:51:04  yuanhua
 * (1) added the hardware timer tick simulation (hts.h/hts.c) (2) Added semaphors for sync in simulation and defined macro for critical section (3) Fixed some bugs in list.h and CRM (4) Modified and fixed bugs in SHAL (5) Changed SNSM/CNSM and SNAM/CNAM for bug fix (6) Added debugging prints, and more.
 *
 * Revision 1.2  2011/07/02 22:09:01  yuanhua
 * Implemented both SNAM and CNAM modules, including network join and leave procedures, systemm resource (such as TEI) allocation and release, TEI renew/release timers, and TEI reuse timer, etc.
 *
 * Revision 1.1  2011/05/28 06:31:19  kripa
 * Combining corresponding STA and CCo modules.
 *
 * Revision 1.1  2011/05/06 19:10:12  kripa
 * Adding link layer files to new source tree.
 *
 * Revision 1.1  2011/04/08 21:42:45  yuanhua
 * Framework
 *
 *
 * =========================================================*/

