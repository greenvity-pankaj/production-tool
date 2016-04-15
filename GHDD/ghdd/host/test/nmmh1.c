/** ========================================================
 *
 *  @file nmmh1.c
 * 
 *  @brief H1 Message Utility for NMM
 *
 *  Copyright (C) 2010-2012, Greenvity Communications, Inc.
 *  All Rights Reserved.
 *  
 * ==========================================================*/

#ifdef MODULE
#include <linux/module.h>
#else
#include <string.h> //for memcp y
#endif
#include "h1msgs.h"
#include "hpgpevt.h"
#include "ghdd_driver_defines.h"
#include "hpgp_msgs.h"

#define ATTR_TYPE 0
#define ATTR_VAL 2
#define ATTR_TLV_SIZE 3

/* ==========================
 *  Encode H1 MGMT Messagers 
 * ========================== */

sEvent *
H1MSG_EncodeNoAttrReq(u16 h1MsgType)
{
	u16     size = H1MSG_MGMT_HDR_SIZE;
	sEvent *event = NULL;
	u8     *pos = NULL;

	/* allocate event */
	event = EVENT_Alloc(size + CRC_SIZE, H1MSG_HEADER_SIZE);
	if (event == NULL) {
		return NULL;
	}
	event->eventHdr.eventClass = EVENT_CLASS_MGMT;
	event->eventHdr.type = h1MsgType;
	pos = event->buffDesc.dataptr;
	*pos = h1MsgType;    /* message type */
	pos += H1MSG_MGMT_HDR_SIZE;

	event->buffDesc.datalen = H1MSG_MGMT_HDR_SIZE;

	return event;
}


sEvent *
H1MSG_EncodeNmkAuthReq(sNmkAuthReq *reqParams)
{
	u16     size = H1MSG_MGMT_HDR_SIZE; 
	sEvent *event = NULL;
	u8     *pos = NULL;

	/* calculate the message size */ 
	size += TLV_HEADER_SIZE + 1;                           /* request id */

	if (reqParams->attrMask & AUTH_ATTR_MASK_DAK)
		size += TLV_HEADER_SIZE + ENC_KEY_LEN;             /* DAK */
	if (reqParams->attrMask & AUTH_ATTR_MASK_MAC_ADDR)
		size += TLV_HEADER_SIZE + AUTH_ATTR_MASK_MAC_ADDR; /* mac address */
	if (reqParams->attrMask & AUTH_ATTR_MASK_NMK)
		size += TLV_HEADER_SIZE + ENC_KEY_LEN;             /* NMK */
	if (reqParams->attrMask & AUTH_ATTR_MASK_NID)
		size += TLV_HEADER_SIZE + NID_LEN;                 /* NID */
	if (reqParams->attrMask & AUTH_ATTR_MASK_SL)
		size += TLV_HEADER_SIZE + 1;                       /* SL */


	/* allocate event */
	event = EVENT_Alloc(size + CRC_SIZE, H1MSG_HEADER_SIZE);
	if (event == NULL) {
		return NULL;
	}
	event->eventHdr.eventClass = EVENT_CLASS_MGMT;
	event->eventHdr.type = APCM_AUTHORIZE_REQ;

	/* encode message type */
	pos = event->buffDesc.dataptr;
	*pos = APCM_AUTHORIZE_REQ;    
	pos += H1MSG_MGMT_HDR_SIZE;

	/* encode Tlv's */
	ENC_TLV_ATTR(pos, ATTR_TYPE_REQUEST_ID, 1, &reqParams->reqId);
    
	if (reqParams->attrMask & AUTH_ATTR_MASK_DAK) {
		ENC_TLV_ATTR(pos, ATTR_TYPE_DAK, ENC_KEY_LEN, reqParams->dak);
	}
	if (reqParams->attrMask & AUTH_ATTR_MASK_MAC_ADDR) {
		ENC_TLV_ATTR(pos, ATTR_TYPE_MAC_ADDR, MAC_ADDR_LEN, reqParams->macAddr);
	}

	if (reqParams->attrMask & AUTH_ATTR_MASK_NMK)
		ENC_TLV_ATTR(pos, ATTR_TYPE_NMK, ENC_KEY_LEN, reqParams->nmk);

	if (reqParams->attrMask & AUTH_ATTR_MASK_NID)
		ENC_TLV_ATTR(pos, ATTR_TYPE_NID, NID_LEN, reqParams->nid);

	if (reqParams->attrMask & AUTH_ATTR_MASK_SL)
		ENC_TLV_ATTR(pos, ATTR_TYPE_SL, 1, &reqParams->sl);

	event->buffDesc.datalen = pos - event->buffDesc.dataptr;

	return event;
}


sEvent *
H1MSG_EncodeSetSecModeReq(sSetSecModeReq *reqParams)
{
	u16     size = H1MSG_MGMT_HDR_SIZE;
	sEvent *event = NULL;
	u8     *pos = NULL;

	/* calculate size */
	size  += TLV_HEADER_SIZE + 1;  /* security mode */

	/* allocate event */
	event = EVENT_Alloc(size + CRC_SIZE, H1MSG_HEADER_SIZE);
	if (event == NULL) {
		return NULL;
	}
	event->eventHdr.eventClass = EVENT_CLASS_CTRL;
	event->eventHdr.type = APCM_SET_SECURITY_MODE_REQ;

	pos = event->buffDesc.dataptr;
	*pos = APCM_SET_SECURITY_MODE_REQ; /* message type */
	pos += H1MSG_MGMT_HDR_SIZE;

	/* encode Tlv's */
	ENC_TLV_ATTR(pos, ATTR_TYPE_SEC_MODE, 1, &reqParams->secMode);

	event->buffDesc.datalen = pos - event->buffDesc.dataptr;

	return event;
}


sEvent *
H1MSG_EncodeGetSecModeReq(void* req)
{
	return H1MSG_EncodeNoAttrReq(APCM_GET_SECURITY_MODE_REQ);
}




sEvent *
H1MSG_EncodeSetKeyReq(sSetKeyReq *reqParams)
{
	u16     size = H1MSG_MGMT_HDR_SIZE;
	sEvent *event = NULL;
	u8     *pos = NULL;

	/* calculate size */
	size  += TLV_HEADER_SIZE + ENC_KEY_LEN;                 /* NMK */
	if (reqParams->attrMask & SET_KEY_ATTR_MASK_NID) 
		size += TLV_HEADER_SIZE + NID_LEN;              /* NID */
	if (reqParams->attrMask & SET_KEY_ATTR_MASK_SL) 
		size += TLV_HEADER_SIZE + 1;                    /* SL  */

	/* allocate event */
	event = EVENT_Alloc(size + CRC_SIZE, H1MSG_HEADER_SIZE);
	if (event == NULL) {
		return NULL;
	}
	event->eventHdr.eventClass = EVENT_CLASS_MGMT;
	event->eventHdr.type = APCM_SET_KEY_REQ;

	pos = event->buffDesc.dataptr;
	*pos = APCM_SET_KEY_REQ;       /* message type */
	pos += H1MSG_MGMT_HDR_SIZE;
	 /* encode Tlv's */
	ENC_TLV_ATTR(pos, ATTR_TYPE_NMK, ENC_KEY_LEN, reqParams->nmk);

	if (reqParams->attrMask & SET_KEY_ATTR_MASK_NID) {
		size  = NID_LEN;
		ENC_TLV_ATTR(pos, ATTR_TYPE_NID, size, reqParams->nid);
	}

	if (reqParams->attrMask & SET_KEY_ATTR_MASK_SL) {
		ENC_TLV_ATTR(pos, ATTR_TYPE_SL, 1, &reqParams->sl);
	}

	event->buffDesc.datalen = pos - event->buffDesc.dataptr;

	return event;
} 


sEvent *
H1MSG_EncodeGetKeyReq(void * req)
{
	return H1MSG_EncodeNoAttrReq(APCM_GET_KEY_REQ);
}





sEvent *
H1MSG_EncodeSetPPKeysReq(sSetPPKeysReq *reqParams)
{
	u16     size = H1MSG_MGMT_HDR_SIZE;
	sEvent *event = NULL;
	u8     *pos = NULL;

	/* calculate size */
	size += TLV_HEADER_SIZE + ENC_KEY_LEN;             /* PP_EKS */
	size += TLV_HEADER_SIZE + ENC_KEY_LEN;             /* PPEKS */
	size += TLV_HEADER_SIZE + MAC_ADDR_LEN;            /* mac address */

	/* allocate event */
	event = EVENT_Alloc(size + CRC_SIZE, H1MSG_HEADER_SIZE);
	if (event == NULL) {
		return NULL;
	}
	event->eventHdr.eventClass = EVENT_CLASS_MGMT;
	event->eventHdr.type = APCM_SET_PPKEYS_REQ;

	pos = event->buffDesc.dataptr;
	*pos = APCM_SET_PPKEYS_REQ;       /* message type */
	pos += H1MSG_MGMT_HDR_SIZE;

	/* encode Tlv's */
	ENC_TLV_ATTR(pos, ATTR_TYPE_PP_EKS, ENC_KEY_LEN, reqParams->ppEks);
	ENC_TLV_ATTR(pos, ATTR_TYPE_PPEK, ENC_KEY_LEN, reqParams->ppek);
	ENC_TLV_ATTR(pos, ATTR_TYPE_MAC_ADDR, MAC_ADDR_LEN, reqParams->macAddr);

	event->buffDesc.datalen = pos - event->buffDesc.dataptr;

	return event;
}



sEvent *
H1MSG_EncodeSetNetworkReq(sSetNetReq *reqParams)
{
	u16     size = H1MSG_MGMT_HDR_SIZE;
	sEvent *event = NULL;
	u8     *pos = NULL;

	/* calculate size */
	size += TLV_HEADER_SIZE + 1;                   /* request type */
	size += TLV_HEADER_SIZE + NID_LEN;              /* NID */
	 /* allocate event */
	event = EVENT_Alloc(size + CRC_SIZE, H1MSG_HEADER_SIZE);
	if (event == NULL) {
		return NULL;
	}
	event->eventHdr.eventClass = EVENT_CLASS_MGMT;
	event->eventHdr.type = APCM_SET_NETWORKS_REQ;

	pos = event->buffDesc.dataptr;
	*pos =	APCM_SET_NETWORKS_REQ; /* message type */
	pos += H1MSG_MGMT_HDR_SIZE;

	/* encode Tlv's */
	ENC_TLV_ATTR(pos, ATTR_TYPE_NID, NID_LEN, reqParams->nid);
	ENC_TLV_ATTR(pos, ATTR_TYPE_REQ_TYPE, 1, &reqParams->reqType);

	event->buffDesc.datalen = pos - event->buffDesc.dataptr;

	return event;
}


sEvent *
H1MSG_EncodeStaRestartReq(void *req)
{
	return H1MSG_EncodeNoAttrReq(APCM_STA_RESTART_REQ);
}


sEvent *
H1MSG_EncodeNetExitReq(void *req)
{
	return H1MSG_EncodeNoAttrReq(APCM_NET_EXIT_REQ);
}


sEvent *
H1MSG_EncodCcoApptReq(sCcoApptReq *reqParam)
{
	u16     size = H1MSG_MGMT_HDR_SIZE;
	sEvent *event = NULL;
	u8     *pos = NULL;
	 /* calculate size */
	size += TLV_HEADER_SIZE + 1;                    /* request type */
	size += TLV_HEADER_SIZE + MAC_ADDR_LEN;         /* mac address */
	 /* allocate event */
	event = EVENT_Alloc(size + CRC_SIZE, H1MSG_HEADER_SIZE);
	if (event == NULL) {
		return NULL;
	}
	event->eventHdr.eventClass = EVENT_CLASS_MGMT;
	event->eventHdr.type = APCM_CCO_APPOINT_REQ;

	pos = event->buffDesc.dataptr;
	*pos = APCM_CCO_APPOINT_REQ; /* message type */
	pos += H1MSG_MGMT_HDR_SIZE;

	/* encode Tlv's */
	ENC_TLV_ATTR(pos, ATTR_TYPE_REQ_TYPE, 1, &reqParam->reqType);
	ENC_TLV_ATTR(pos, ATTR_TYPE_MAC_ADDR, MAC_ADDR_LEN, reqParam->macAddr);

	event->buffDesc.datalen = pos - event->buffDesc.dataptr;
   
	return event;
}


/* ==========================
 *  Decode H1 MGMT Messagers 
 * ========================== */

eStatus 
H1MSG_DecodeResultCnf(sEvent *event, u8* result)
{
	u8 *pos = NULL;
	sH1TlvHdr *tlvHdr = NULL;

	pos = event->buffDesc.dataptr;
	tlvHdr = (sH1TlvHdr *)pos;

	while ((pos - event->buffDesc.dataptr) < event->buffDesc.datalen) { 
		pos += sizeof(sH1TlvHdr);
		switch (tlvHdr->type) {
			case ATTR_TYPE_RESULT:
				*result = *pos;
			break;
			default:
			{
			}
		}
		pos += tlvHdr->len;
		tlvHdr = (sH1TlvHdr *)pos;
	}
	return STATUS_SUCCESS;
}

eStatus 
H1MSG_DecodeNmkAuthCnf(sEvent *event, sNmkAuthCnf *spAttr)
{
	u8 *pos = NULL;

	pos = event->buffDesc.dataptr;
	pos++;
#ifdef MODULE
	while ((pos - event->buffDesc.dataptr) < event->buffDesc.datalen) { 
		switch (pos[ATTR_TYPE]) {
			case ATTR_TYPE_REQUEST_ID:
		   	   spAttr->reqId = pos[ATTR_VAL];
			   pos += ATTR_TLV_SIZE;
			break;
			case ATTR_TYPE_STATUS:
			   spAttr->result = pos[ATTR_VAL];
			   pos += ATTR_TLV_SIZE;
			break;
			case ATTR_TYPE_MAC_ADDR:
			   spAttr->attrMask |= AUTH_ATTR_MASK_MAC_ADDR;
			   memcpy(spAttr->macAddr, &pos[ATTR_VAL], MAC_ADDR_LEN);
			   pos += MAC_ADDR_LEN + 2;
			break;
			default:
			{
				pos++;
			}
		}
	}
#else
	sH1TlvHdr *tlvHdr = NULL;
	tlvHdr = (sH1TlvHdr *)pos;

	while ((pos - event->buffDesc.dataptr) < event->buffDesc.datalen) { 
		pos += sizeof(sH1TlvHdr);
		switch (tlvHdr->type) {
			case ATTR_TYPE_REQUEST_ID:
				spAttr->reqId = *pos;
			break;
			case ATTR_TYPE_STATUS:
			   spAttr->result = *pos;
			break;
			case ATTR_TYPE_MAC_ADDR:
			   spAttr->attrMask |= AUTH_ATTR_MASK_MAC_ADDR;
			   memcpy(spAttr->macAddr, pos, MAC_ADDR_LEN);
			break;
			default:
			{
			}
		}
		pos += tlvHdr->len;
		tlvHdr = (sH1TlvHdr *)pos;
	}
#endif
	return STATUS_SUCCESS;
}



eStatus 
H1MSG_DecodeNmkAuthInd(sEvent *event, sNmkAuthInd *spAttr)
{
	u8 *pos = NULL;

	pos = event->buffDesc.dataptr;
	pos++;
#ifdef MODULE
	while ((pos - event->buffDesc.dataptr) < event->buffDesc.datalen) { 
		switch (pos[ATTR_TYPE]) {
			case ATTR_TYPE_REQUEST_ID:
		   	   memcpy(spAttr->nid, pos, NID_LEN);
			   pos += NID_LEN + ATTR_TLV_SIZE;
			break;
			case ATTR_TYPE_STATUS:
			   spAttr->status = pos[ATTR_VAL];
			   pos += ATTR_TLV_SIZE;
			break;
			case ATTR_TYPE_MAC_ADDR:
			   memcpy(spAttr->macAddr, &pos[ATTR_VAL], MAC_ADDR_LEN);
			   pos += MAC_ADDR_LEN + ATTR_TLV_SIZE;
			break;
			default:
			{
				pos++;
			}
		}
	}
#else
	sH1TlvHdr *tlvHdr = NULL;
	tlvHdr = (sH1TlvHdr *)pos;

	while ((pos - event->buffDesc.dataptr) < event->buffDesc.datalen) { 
		pos += sizeof(sH1TlvHdr);
		switch (tlvHdr->type) {
			case ATTR_TYPE_MAC_ADDR:
				memcpy(spAttr->macAddr, pos, MAC_ADDR_LEN);
			break;
			case ATTR_TYPE_NID:
				memcpy(spAttr->nid, pos, NID_LEN);
			break;
			case ATTR_TYPE_STATUS:
				spAttr->status = *pos;
			break;
			default:
			{
			}
		}
		pos += tlvHdr->len;
		tlvHdr = (sH1TlvHdr *)pos;
	}
#endif
	return STATUS_SUCCESS;
}


eStatus 
H1MSG_DecodeSetSecModeCnf(sEvent *event, u8 *result)
{
	return H1MSG_DecodeResultCnf(event, result);
}




eStatus 
H1MSG_DecodeGetSecModeCnf(sEvent *event, sGetSecModeCnf *spAttr)
{
	u8 *pos = NULL;

	pos = event->buffDesc.dataptr;
	pos++;
#ifdef MODULE
	while ((pos - event->buffDesc.dataptr) < event->buffDesc.datalen) { 
		switch (pos[ATTR_TYPE]) {
			case ATTR_TYPE_RESULT:
				spAttr->result = pos[ATTR_VAL];
				pos += ATTR_TLV_SIZE;
			break;
			case ATTR_TYPE_SEC_MODE:
				spAttr->secMode = pos[ATTR_VAL];
				pos += ATTR_TLV_SIZE;
			break;
			default:
			{
				pos++;
			}
		}
	}
#else
	sH1TlvHdr *tlvHdr = NULL;
	tlvHdr = (sH1TlvHdr *)pos;
	while ((pos - event->buffDesc.dataptr) < event->buffDesc.datalen) { 
		pos += sizeof(sH1TlvHdr);
		switch (tlvHdr->type) {
			case ATTR_TYPE_RESULT:
				spAttr->result = *pos;
			break;
			case ATTR_TYPE_SEC_MODE:
				spAttr->secMode = *pos;
			default:
			{
			}
		}
		pos += tlvHdr->len;
		tlvHdr = (sH1TlvHdr *)pos;
	}
#endif
	return STATUS_SUCCESS;
}




eStatus 
H1MSG_DecodeSetKeyCnf(sEvent *event, u8 *result)
{
	return H1MSG_DecodeResultCnf(event, result);
}



eStatus 
H1MSG_DecodeGetKeyCnf(sEvent *event, sGetKeyCnf *spAttr)
{
	u8 *pos = NULL;
	sH1TlvHdr *tlvHdr = NULL;

	pos = event->buffDesc.dataptr;
	tlvHdr = (sH1TlvHdr *)pos;

	while ((pos - event->buffDesc.dataptr) < event->buffDesc.datalen) { 
		pos += sizeof(sH1TlvHdr);
		switch (tlvHdr->type) {
			case ATTR_TYPE_NID:
				memcpy(spAttr->nid, pos, NID_LEN);
			break;
			case ATTR_TYPE_NMK:
				memcpy(spAttr->nmk, pos, ENC_KEY_LEN);
			break;
			default:
			{
			}
		}
		pos += tlvHdr->len;
		tlvHdr = (sH1TlvHdr *)pos;
	}
	return STATUS_SUCCESS;
}



eStatus 
H1MSG_DecodeSetPPKeyCnf(sEvent *event, u8* result)
{
	return H1MSG_DecodeResultCnf(event, result);
}



eStatus 
H1MSG_DecodeSetNetworksCnf(sEvent *event, u8 *result)
{
	return H1MSG_DecodeResultCnf(event, result);
}


eStatus 
H1MSG_DecodeStaRestartCnf(sEvent *event, u8 *result)
{
	return H1MSG_DecodeResultCnf(event, result);
}


eStatus 
H1MSG_DecodeNetExitCnf(sEvent *event, u8 *result)
{
	return H1MSG_DecodeResultCnf(event, result);
}

eStatus 
H1MSG_DecodeAppointCCoCnf(sEvent *event, u8 *result)
{
	return H1MSG_DecodeResultCnf(event, result);
}

sEvent *CreateOnOffTlvReq( u8 reqType, u8 Value )
{
	u16     size 	= H1MSG_MGMT_HDR_SIZE;
	sEvent *event 	= NULL;
	u8     *pos 	= NULL;

	/* calculate size */
	size  += TLV_HEADER_SIZE + 1;  /* security mode */

	/* allocate event */
	event = EVENT_Alloc(size + CRC_SIZE, H1MSG_HEADER_SIZE);
	if (event == NULL) {
		return NULL;
	}
	event->eventHdr.eventClass = EVENT_CLASS_MGMT;
	event->eventHdr.type = reqType;

	pos = event->buffDesc.dataptr;
	*pos = reqType; /* message type */
	pos += H1MSG_MGMT_HDR_SIZE;

	/* encode Tlv's */
	ENC_TLV_ATTR(pos, ONOFF, 1, &Value);

	event->buffDesc.datalen = pos - event->buffDesc.dataptr;

	return event;
}

sEvent *CreateHwSpecTlvReq( u8 reqType, char *MacAddr )

{
	u16     size = H1MSG_MGMT_HDR_SIZE;
	sEvent *event = NULL;
	u8     *pos = NULL;
	
	 /* calculate size */
	size += TLV_HEADER_SIZE + 1;                    /* request type */
	size += TLV_HEADER_SIZE + MAC_ADDR_LEN;         /* mac address */
	
	 /* allocate event */
	event = EVENT_Alloc(size + CRC_SIZE, H1MSG_HEADER_SIZE);
	if (event == NULL) {
		return NULL;
	}
	
	event->eventHdr.eventClass = EVENT_CLASS_MGMT;
	event->eventHdr.type = reqType;

	pos = event->buffDesc.dataptr;
	*pos = reqType; /* message type */
	pos += H1MSG_MGMT_HDR_SIZE;

	/* encode Tlv's */
	ENC_TLV_ATTR(pos, ATTR_TYPE_MAC_ADDR, 1, &reqType);
	ENC_TLV_ATTR(pos, ATTR_TYPE_MAC_ADDR, MAC_ADDR_LEN, MacAddr);

	event->buffDesc.datalen = pos - event->buffDesc.dataptr;
   
	return event;
}

