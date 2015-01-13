/** ========================================================
 *
 *  @file nmah1.c
 * 
 *  @brief H1 Message Utility for NMA
 *
 *  Copyright (C) 2010-2012, Greenvity Communications, Inc.
 *  All Rights Reserved.
 *  
 * ==========================================================*/


#include <string.h> //for memcpy
#include <stdio.h>

#include "h1msgs.h"
#include "hpgpevt.h"
#include "hpgpdef.h"
#include "papdef.h"
#ifdef UM
#include "mac_intf_common.h"
#include "fm.h"
#endif


/* ==========================
 *  Decode H1 MGMT Messagers 
 * ========================== */

eStatus H1MSG_DecodeNmkAuthReq(sEvent *event, sNmkAuthReq *spAttr)
{
    u8 *pos = NULL;
    sH1TlvHdr *tlvHdr = NULL;

    spAttr->attrMask = 0;
    pos = event->buffDesc.dataptr;
    pos++;
    tlvHdr = (sH1TlvHdr *)pos;

    while ((pos - event->buffDesc.dataptr) < event->buffDesc.datalen)
    { 
        pos += sizeof(sH1TlvHdr);
        switch(tlvHdr->type)
        {
            case ATTR_TYPE_REQUEST_ID:
               spAttr->reqId = *pos;
               break;
            case ATTR_TYPE_DAK:
               spAttr->attrMask |= AUTH_ATTR_MASK_DAK;
               memcpy(spAttr->dak, pos, ENC_KEY_LEN);
               break;
            case ATTR_TYPE_MAC_ADDR:
               spAttr->attrMask |= AUTH_ATTR_MASK_MAC_ADDR;
               memcpy(spAttr->macAddr, pos, MAC_ADDR_LEN);
               break;
            case ATTR_TYPE_NMK:
               spAttr->attrMask |= AUTH_ATTR_MASK_NMK;
               memcpy(spAttr->nmk, pos, ENC_KEY_LEN);
               break;
            case ATTR_TYPE_NID:
               spAttr->attrMask |= AUTH_ATTR_MASK_NID;
               memcpy(spAttr->nid, pos, NID_LEN);
               break;
            case ATTR_TYPE_SL:
               spAttr->attrMask |= AUTH_ATTR_MASK_SL;
               spAttr->sl = *pos;
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



eStatus H1MSG_DecodeSetSecModeReq(sEvent *event, sSetSecModeReq *spAttr)
{
    u8 *pos = NULL;
    sH1TlvHdr *tlvHdr = NULL;

    pos = event->buffDesc.dataptr;
    pos++;
    tlvHdr = (sH1TlvHdr *)pos;

    while ((pos - event->buffDesc.dataptr) < event->buffDesc.datalen)
    { 
        pos += sizeof(sH1TlvHdr);
        switch(tlvHdr->type)
        {
            case ATTR_TYPE_SEC_MODE:
               spAttr->secMode = *pos;
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



eStatus H1MSG_DecodeGetSecModeReq(sEvent *event, sSetSecModeReq *spAttr)
{
    u8 *pos = NULL;
    sH1TlvHdr *tlvHdr = NULL;

    pos = event->buffDesc.dataptr;
    pos++;
    tlvHdr = (sH1TlvHdr *)pos;

    while ((pos - event->buffDesc.dataptr) < event->buffDesc.datalen)
    { 
        pos += sizeof(sH1TlvHdr);
        switch(tlvHdr->type)
        {
            case ATTR_TYPE_SEC_MODE:
               spAttr->secMode = *pos;
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


eStatus H1MSG_DecodeSetKeyReq(sEvent *event, sSetKeyReq *spAttr)
{
    u8 *pos = NULL;
    sH1TlvHdr *tlvHdr = NULL;

    pos = event->buffDesc.dataptr;
    pos++;
    tlvHdr = (sH1TlvHdr *)pos;
    spAttr->attrMask = 0;

    while ((pos - event->buffDesc.dataptr) < event->buffDesc.datalen)
    { 
        pos += sizeof(sH1TlvHdr);
        switch(tlvHdr->type)
        {
            case ATTR_TYPE_NMK:
               memcpy(spAttr->nmk, pos, ENC_KEY_LEN);
               break;
            case ATTR_TYPE_NID:
               spAttr->attrMask |= SET_KEY_ATTR_MASK_NID;
               memcpy(spAttr->nid, pos, NID_LEN);
               break;
            case ATTR_TYPE_SL:
               spAttr->attrMask |= SET_KEY_ATTR_MASK_SL;
               spAttr->sl = *pos;
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



eStatus H1MSG_DecodeSetPPKeysReq(sEvent *event, sSetPPKeysReq *spAttr)
{
    u8 *pos = NULL;
    sH1TlvHdr *tlvHdr = NULL;

    pos = event->buffDesc.dataptr;
    pos++;
    tlvHdr = (sH1TlvHdr *)pos;
    spAttr->attrMask = 0;

    while ((pos - event->buffDesc.dataptr) < event->buffDesc.datalen)
    { 
        pos += sizeof(sH1TlvHdr);
        switch(tlvHdr->type)
        {
            case ATTR_TYPE_PP_EKS:
               memcpy(spAttr->ppEks, pos, ENC_KEY_LEN);
               break;
            case ATTR_TYPE_PPEK:
               memcpy(spAttr->ppek, pos, ENC_KEY_LEN);
               break;
            case ATTR_TYPE_MAC_ADDR:
               memcpy(spAttr->macAddr, pos, MAC_ADDR_LEN);
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


eStatus H1MSG_DecodeSetNetworkReq(sEvent *event, sSetNetReq *spAttr)
{
    u8 *pos = NULL;
    sH1TlvHdr *tlvHdr = NULL;

    pos = event->buffDesc.dataptr;
    pos++;
    tlvHdr = (sH1TlvHdr *)pos;

    while ((pos - event->buffDesc.dataptr) < event->buffDesc.datalen)
    { 
        pos += sizeof(sH1TlvHdr);
        switch(tlvHdr->type)
        {
            case ATTR_TYPE_NID:
               memcpy(spAttr->nid, pos, NID_LEN);
               break;
            case ATTR_TYPE_REQ_TYPE:
               spAttr->reqType = *pos;
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



eStatus H1MSG_DecodeCcoApptReq(sEvent *event, sCcoApptReq *spAttr)
{
    u8 *pos = NULL;
    sH1TlvHdr *tlvHdr = NULL;

    pos = event->buffDesc.dataptr;
    pos++;
    tlvHdr = (sH1TlvHdr *)pos;

    while ((pos - event->buffDesc.dataptr) < event->buffDesc.datalen)
    { 
        pos += sizeof(sH1TlvHdr);
        switch(tlvHdr->type)
        {
            case ATTR_TYPE_REQ_TYPE:
               spAttr->reqType = *pos;
               break;
            case ATTR_TYPE_MAC_ADDR:
               memcpy(spAttr->macAddr, pos, MAC_ADDR_LEN);
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

eStatus H1MSG_DecodeONOFFReq(sEvent *event, u8 *mode)
{
    u8 *pos = NULL;
    sH1TlvHdr *tlvHdr = NULL;

    pos = event->buffDesc.dataptr;
    pos++;
    tlvHdr = (sH1TlvHdr *)pos;

    while ((pos - event->buffDesc.dataptr) < event->buffDesc.datalen)
    { 
        pos += sizeof(sH1TlvHdr);
        switch(tlvHdr->type)
        {
            case ONOFF:
               *mode = *pos;
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

eStatus H1MSG_DecodeSetHWSpecReq( sEvent *event, u8 *macAddr)
{
    u8 *pos = NULL;
    sH1TlvHdr *tlvHdr = NULL;

    pos = event->buffDesc.dataptr;
    pos++;
    tlvHdr = (sH1TlvHdr *)pos;

    while ((pos - event->buffDesc.dataptr) < event->buffDesc.datalen)
    { 
        pos += sizeof(sH1TlvHdr);
        switch(tlvHdr->type)
        {
            case ATTR_TYPE_MAC_ADDR:
               memcpy(macAddr, pos, MAC_ADDR_LEN);
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

/* ==========================
 *  Encode H1 MGMT Messagers 
 * ========================== */

sEvent *H1MSG_EncodeResultCnf(u16 h1MsgType, u8 result)
{
    u16     size = H1MSG_MGMT_HDR_SIZE;
    sEvent *event = NULL;
    u8     *pos = NULL;

    /* calculate size */
    size += TLV_HEADER_SIZE + 1;  /* result */

    /* allocate event */
    event = EVENT_Alloc(size + CRC_SIZE, H1MSG_HEADER_SIZE);
    if (event == NULL)
    {
        return NULL;
    }
    event->eventHdr.eventClass = EVENT_CLASS_CTRL;
    event->eventHdr.type = h1MsgType;

    /* encode message type */
    pos = event->buffDesc.dataptr;
    *pos = h1MsgType; 
    pos += H1MSG_MGMT_HDR_SIZE;

    /* encode Tlv's */
    ENC_TLV_ATTR(pos, ATTR_TYPE_RESULT, 1, &result);

    event->buffDesc.datalen = pos - event->buffDesc.dataptr;

    return event;
}



sEvent *H1MSG_EncodeNmkAuthCnf(sNmkAuthCnf *reqParams)
{  
    u16     size = H1MSG_MGMT_HDR_SIZE;
    sEvent *event = NULL;
    u8     *pos = NULL;

    /* calculate size */
    size += TLV_HEADER_SIZE + 1;             /* request id */
    size += TLV_HEADER_SIZE + 1;             /* status */
    if (reqParams->attrMask & AUTH_ATTR_MASK_MAC_ADDR)
        size += TLV_HEADER_SIZE + MAC_ADDR_LEN;  /* mac address */

    /* allocate event */
    event = EVENT_Alloc(size + CRC_SIZE, H1MSG_HEADER_SIZE);
    if (event == NULL)
    {
        return NULL;
    }
    event->eventHdr.eventClass = EVENT_CLASS_CTRL;
    event->eventHdr.type = APCM_AUTHORIZE_CNF;

    /* encode message type */
    pos = event->buffDesc.dataptr;
    *pos = APCM_AUTHORIZE_CNF; 
    pos += H1MSG_MGMT_HDR_SIZE;

    /* encode Tlv's */
    ENC_TLV_ATTR(pos, ATTR_TYPE_REQUEST_ID, 1, &reqParams->reqId);
    ENC_TLV_ATTR(pos, ATTR_TYPE_RESULT, 1, &reqParams->result);
    if (reqParams->attrMask & AUTH_ATTR_MASK_MAC_ADDR)
        ENC_TLV_ATTR(pos, ATTR_TYPE_MAC_ADDR, MAC_ADDR_LEN, reqParams->macAddr);

    event->buffDesc.datalen = pos - event->buffDesc.dataptr;

    return event;
}


sEvent *H1MSG_EncodeNmkAuthInd(sNmkAuthInd *reqParams)
{
    u16     size = H1MSG_MGMT_HDR_SIZE;
    sEvent *event = NULL;
    u8     *pos = NULL;

    /* calculate size */
    size += TLV_HEADER_SIZE + MAC_ADDR_LEN ;
    size += TLV_HEADER_SIZE + NID_LEN;
    size += TLV_HEADER_SIZE + 1;

    /* allocate event */
    event = EVENT_Alloc(size + CRC_SIZE, H1MSG_HEADER_SIZE);
    if (event == NULL)
    {
        return NULL;
    }
    event->eventHdr.eventClass = EVENT_CLASS_CTRL;
    event->eventHdr.type = APCM_AUTHORIZE_IND;

    /* encode message type */
    pos = event->buffDesc.dataptr;
    *pos = APCM_AUTHORIZE_IND; 
    pos += H1MSG_MGMT_HDR_SIZE;

    /* encode Tlv's */
    ENC_TLV_ATTR(pos, ATTR_TYPE_MAC_ADDR, MAC_ADDR_LEN, reqParams->macAddr);
    ENC_TLV_ATTR(pos, ATTR_TYPE_NID, NID_LEN, reqParams->nid);
    ENC_TLV_ATTR(pos, ATTR_TYPE_STATUS, 1, &reqParams->status);

    event->buffDesc.datalen = pos - event->buffDesc.dataptr;

    return event;
}


sEvent *H1MSG_EncodeSetSecModeCnf(u8 result)
{
    return H1MSG_EncodeResultCnf(APCM_SET_SECURITY_MODE_CNF, result);
}


sEvent *H1MSG_EncodeGetSecModeCnf(sGetSecModeCnf *reqParams)
{
    u16     size = H1MSG_MGMT_HDR_SIZE;
    sEvent *event = NULL;
    u8     *pos = NULL;

    /* calculate size */
    size += TLV_HEADER_SIZE + 1;      /* result */
    if (reqParams->attrMask & SEC_MODE_ATTR_MASK_SEC_MODE)
        size += TLV_HEADER_SIZE + 1;  /* security mode */

    /* allocate event */
    event = EVENT_Alloc(size + CRC_SIZE, H1MSG_HEADER_SIZE);
    if (event == NULL)
    {
        return NULL;
    }
    event->eventHdr.eventClass = EVENT_CLASS_CTRL;
    event->eventHdr.type = APCM_GET_SECURITY_MODE_CNF;

    /* encode message type */
    pos = event->buffDesc.dataptr;
    *pos = APCM_GET_SECURITY_MODE_CNF;  /* message type */
    pos += H1MSG_MGMT_HDR_SIZE;

    /* encode Tlv's */
    ENC_TLV_ATTR(pos, ATTR_TYPE_RESULT, 1, &reqParams->result);
    if (reqParams->attrMask & SEC_MODE_ATTR_MASK_SEC_MODE)
        ENC_TLV_ATTR(pos, ATTR_TYPE_SEC_MODE, 1, &reqParams->secMode);

    event->buffDesc.datalen = pos - event->buffDesc.dataptr;

    return event;
}





sEvent *H1MSG_EncodeSetKeyCnf(u8 result)
{
    return H1MSG_EncodeResultCnf(APCM_SET_KEY_CNF, result);
}



sEvent *H1MSG_EncodeGetKeyCnf(sGetKeyCnf *reqParams)
{
    u16     size = H1MSG_MGMT_HDR_SIZE;
    sEvent *event = NULL;
    u8     *pos = NULL;

    /* calculate size */
    size += TLV_HEADER_SIZE + NID_LEN;        /* NID */
    size += TLV_HEADER_SIZE + ENC_KEY_LEN;    /* NMK */

    /* allocate event */
    event = EVENT_Alloc(size + CRC_SIZE, H1MSG_HEADER_SIZE);
    if (event == NULL)
    {
        return NULL;
    }
    event->eventHdr.eventClass = EVENT_CLASS_CTRL;
    event->eventHdr.type = APCM_GET_KEY_CNF;

    /* encode message type */
    pos = event->buffDesc.dataptr;
    *pos = APCM_SET_KEY_CNF; 
    pos += H1MSG_MGMT_HDR_SIZE;

    /* encode Tlv's */
    ENC_TLV_ATTR(pos, ATTR_TYPE_NID, NID_LEN, reqParams->nid);
    ENC_TLV_ATTR(pos, ATTR_TYPE_NMK, ENC_KEY_LEN, reqParams->nmk);

    event->buffDesc.datalen = pos - event->buffDesc.dataptr;

    return event;
}



sEvent *H1MSG_EncodeSetPPKeysCnf(u8 result)
{
    return H1MSG_EncodeResultCnf(APCM_SET_PPKEYS_CNF, result);
}



sEvent *H1MSG_EncodeSetNetworkCnf(u8 result)
{
    return H1MSG_EncodeResultCnf(APCM_SET_NETWORKS_CNF, result);
}



sEvent *H1MSG_EncodeStaRestartCnf(u8 result)
{
    return H1MSG_EncodeResultCnf(APCM_STA_RESTART_CNF, result);
}


sEvent *H1MSG_EncodeNetExitCnf(u8 result)
{
    return H1MSG_EncodeResultCnf(APCM_NET_EXIT_CNF, result);
}


sEvent *H1MSG_EncodeCcoApptCnf(u8 result)
{
    return H1MSG_EncodeResultCnf(APCM_CCO_APPOINT_CNF, result);
}

sEvent *H1MSG_DevmodeRes(u16 h1MsgType, u8 result, u8 *devmode)
{
	u16 	size = H1MSG_MGMT_HDR_SIZE;
	sEvent *event = NULL;
	u8	   *pos = NULL;
	
	/* calculate size */
	size += TLV_HEADER_SIZE + 1;  /* result */

	/* allocate event */
	event = EVENT_Alloc(size + CRC_SIZE, H1MSG_HEADER_SIZE);
	if (event == NULL)
	{
		return NULL;
	}
	event->eventHdr.eventClass = EVENT_CLASS_MGMT;
	event->eventHdr.type = h1MsgType;

	/* encode message type */
	pos = event->buffDesc.dataptr;
	*pos = h1MsgType; 
	pos += H1MSG_MGMT_HDR_SIZE;

    /* encode Tlv's */
    ENC_TLV_ATTR(pos, ATTR_TYPE_RESULT, 1, &result);
	ENC_TLV_ATTR(pos, ATTR_TYPE_NW_INFO, 1, devmode);

    event->buffDesc.datalen = pos - event->buffDesc.dataptr;
	return event;
}


sEvent *H1MSG_HWSpecsRes(u16 h1MsgType, u8 result, hostHwSpec *hwSpec)
{
	u16 	size = H1MSG_MGMT_HDR_SIZE;
	sEvent *event = NULL;
	u8	   *pos = NULL;

	/* calculate size */
	size += TLV_HEADER_SIZE + 1;  /* result */

	/* allocate event */
	event = EVENT_Alloc(size + CRC_SIZE, H1MSG_HEADER_SIZE);
	if (event == NULL)
	{
		return NULL;
	}
	event->eventHdr.eventClass = EVENT_CLASS_MGMT;
	event->eventHdr.type = h1MsgType;

	/* encode message type */
	pos = event->buffDesc.dataptr;
	*pos = h1MsgType; 
	pos += H1MSG_MGMT_HDR_SIZE;

    /* encode Tlv's */
    ENC_TLV_ATTR(pos, ATTR_TYPE_RESULT, 1, &result);
    ENC_TLV_ATTR(pos, ATTR_TYPE_NW_INFO, MAC_ADDR_LEN, hwSpec->macAddr);

    event->buffDesc.datalen = pos - event->buffDesc.dataptr;
	return event;
}


sEvent *H1MSG_EncodeDeviceStatsRes(u16 h1MsgType, u8 result, host_devstats *ptrdevstats)
{
	u16 	size = H1MSG_MGMT_HDR_SIZE;
	sEvent *event = NULL;
	u8	   *pos = NULL;

	/* calculate size */
	size += TLV_HEADER_SIZE + 1;  /* result */

	/* allocate event */
	event = EVENT_Alloc(size + CRC_SIZE, H1MSG_HEADER_SIZE);
	if (event == NULL)
	{
		return NULL;
	}
	event->eventHdr.eventClass = EVENT_CLASS_MGMT;
	event->eventHdr.type = h1MsgType;

	/* encode message type */
	pos = event->buffDesc.dataptr;
	*pos = h1MsgType; 
	pos += H1MSG_MGMT_HDR_SIZE;

    /* encode Tlv's */
    ENC_TLV_ATTR(pos, ATTR_TYPE_RESULT, 1, &result);
    ENC_TLV_ATTR(pos, ATTR_TYPE_NW_INFO, sizeof(host_devstats), (u8 *)ptrdevstats);

    event->buffDesc.datalen = pos - event->buffDesc.dataptr;
	return event;
}

sEvent *H1MSG_EncodePeerInfoRes(u16 h1MsgType, u8 result, u8 entries, peerinfo *ptrpeerinfo)
{
	u16 	size = H1MSG_MGMT_HDR_SIZE;
	sEvent *event = NULL;
	u8	   *pos = NULL;

	/* calculate size */
	size += TLV_HEADER_SIZE + 1;  /* result */

	/* allocate event */
	event = EVENT_Alloc(size + CRC_SIZE, H1MSG_HEADER_SIZE);
	if (event == NULL)
	{
		return NULL;
	}
	event->eventHdr.eventClass = EVENT_CLASS_MGMT;
	event->eventHdr.type = h1MsgType;

	/* encode message type */
	pos = event->buffDesc.dataptr;
	*pos = h1MsgType; 
	pos += H1MSG_MGMT_HDR_SIZE;

    /* encode Tlv's */
    ENC_TLV_ATTR(pos, ATTR_TYPE_RESULT, 1, &result);
    ENC_TLV_ATTR(pos, ATTR_TYPE_NW_INFO, entries * sizeof(peerinfo), (u8 *)ptrpeerinfo);

    event->buffDesc.datalen = pos - event->buffDesc.dataptr;
	return event;
}


