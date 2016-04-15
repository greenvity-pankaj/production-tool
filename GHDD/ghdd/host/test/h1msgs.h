/* 
 * File:   h1msgs.h
 * Author: palmchip
 *
 * Created on September 28, 2011, 11:30 AM
 */

#ifndef H1MSGS_H
#define	H1MSGS_H

#include "papdef.h"
#include "hpgpdef.h"

extern bool is_little_endian(void);
extern void *rev_memcpy(u8 *, const u8 *, unsigned int);

#define H1_FRAME_TYPE_CTRL    0
#define H1_FRAME_TYPE_DATA    1
#define H1_FRAME_TYPE_MGMT    2
#define TLVHDR_2			  1

#define CTRL_FRAME_PROTO_ZIGBEE    1
#define CTRL_FRAME_PROTO_HPGP      2

typedef struct h1Hdr
{
#if 0
    //h1 header
    u16  hdrType     : 2;
    u16  protocolInd : 1;
    u16  reserved    : 1;
    u16  crcInd      : 1;
    u16  length      : 11;
#endif
    u8   frmType;
    u8   proto;
    u16  len;
} __PACKED__ sH1Hdr, *psH1Hdr;


typedef struct h1TlvHdr
{
    u8     type;
#ifndef TLVHDR_2
    u16    len;
#else
	u8		len;
#endif
} __PACKED__ sH1TlvHdr, *psH1TlvHdr;


#define H1MSG_HEADER_SIZE               4
#ifndef TLVHDR_2
#define TLV_HEADER_SIZE                 3
#else
#define TLV_HEADER_SIZE                 2
#endif
#define H1MSG_MGMT_HDR_SIZE             1
#define CRC_SIZE                        4

#define ATTR_LEN_NW                     78
#define ATTR_LEN_NWLIST                 0     //variable
#define ATTR_LEN_STAINFO                134
#define ATTR_LEN_STAINFOLIST            0     //variable
#define ATTR_LEN_OUI                    3
#define ATTR_LEN_BACKUPCCO              1
#define ATTR_LEN_SOFTHANDOVER           1
#define ATTR_LEN_MAXFLAV                2
#define ATTR_LEN_IMPLEMENTATIONVER      2
#define ATTR_LEN_NWINFO                 18
#define ATTR_LEN_NWINFOLIST             0     //variable
#define ATTR_LEN_BEACON                 136

enum attrTypes
{
    ATTR_TYPE_NTB = 11,
    ATTR_TYPE_REQUEST_ID,
    ATTR_TYPE_DAK,
    ATTR_TYPE_MAC_ADDR,
    ATTR_TYPE_NMK,
    ATTR_TYPE_NID,
    ATTR_TYPE_SL,
    ATTR_TYPE_TEK,
    ATTR_TYPE_RESULT = 19,
    ATTR_TYPE_STATUS,
    ATTR_TYPE_SEC_MODE,
    ATTR_TYPE_NW,
    ATTR_TYPE_NW_LIST,
    ATTR_TYPE_REQ_TYPE,
    ATTR_TYPE_STA_INFO,
    ATTR_TYPE_STA_INFO_LIST,
    ATTR_TYPE_TONE_MASK,
    ATTR_TYPE_AV_VERSION,
    ATTR_TYPE_OUI,
    ATTR_TYPE_AUTO_CONNECT,
    ATTR_TYPE_SMOOTHING,
    ATTR_TYPE_CCO_CAPABILITY,
    ATTR_TYPE_PROXY_CAPABILITY,
    ATTR_TYPE_BACKUP_CCO,
    ATTR_TYPE_SOFT_HANDOVER,
    ATTR_TYPE_TWOSYMFC,
    ATTR_TYPE_MAX_FLAV,
    ATTR_TYPE_HOMEPLUG_11CAP,
    ATTR_TYPE_HOMEPLUG_10INTEROP,
    ATTR_TYPE_REGULATORY_CAP,
    ATTR_TYPE_BIDIRECTIONAL_BURSTING,
    ATTR_TYPE_IMPLEMENTATION_VER,
    ATTR_TYPE_NW_INFO,
    ATTR_TYPE_NW_INFOLIST,
    ATTR_TYPE_BEACON,
    ATTR_TYPE_HD_DURATION,
    ATTR_TYPE_PP_EKS,
    ATTR_TYPE_PPEK,
    ATTR_TYPE_AUTH_MODE,
    ATTR_TYPE_HLE_ID,
    ATTR_TYPE_HPGP_ID

};



#define DEC_TLV_ATTR(pos, attrType, attrLen, pAttr) \
    {\
        pos += TLV_HEADER_SIZE;\
        memcpy(pAttr, pos, attrLen);\
        pos += attrLen;\
    }

#ifndef TLVHDR_2
#define ENC_TLV_ATTR(pos, attrType, attrLen, attr) \
    { \
	u16 *pos16 = NULL; \
        *pos = attrType; \
        pos++; \
		pos16 = (u16*) pos; \
        *pos16 = (u16)attrLen; \
        pos += 2; \
        if(is_little_endian() == 1)\
        	memcpy(pos, attr, attrLen); \
		else\
			rev_memcpy(pos, attr, attrLen);\
        pos += attrLen; \
    }
#else
#define ENC_TLV_ATTR(pos, attrType, attrLen, attr) \
    { \
        *pos = attrType; \
        pos++; \
        *pos = attrLen; \
        pos++; \
		if(is_little_endian() == 1)\
        	memcpy(pos, attr, attrLen); \
		else\
			rev_memcpy(pos, attr, attrLen);	\
        pos += attrLen; \
    }

#endif
#define AUTH_ATTR_MASK_DAK        (1U << 0)
#define AUTH_ATTR_MASK_MAC_ADDR   (1U << 1)
#define AUTH_ATTR_MASK_NMK        (1U << 2)
#define AUTH_ATTR_MASK_NID        (1U << 3)
#define AUTH_ATTR_MASK_SL         (1U << 4)

typedef struct nmkAuthReq
{
    u8 attrMask;
    u8 reqId;
    u8 dak[ENC_KEY_LEN];
    u8 macAddr[MAC_ADDR_LEN];
    u8 nmk[ENC_KEY_LEN];
    u8 nid[NID_LEN];
    u8 sl;
} sNmkAuthReq, *spNmkAuthReq;


typedef struct nmkAuthCnf
{
    u8 attrMask;
    u8 reqId;
    u8 result;
    u8 macAddr[MAC_ADDR_LEN];
} sNmkAuthCnf, *spNmkAuthCnf;


typedef struct nmkAuthInd
{
    u8 macAddr[MAC_ADDR_LEN];
    u8 nid[NID_LEN];
    u8 status;

} sNmkAuthInd, *spNmkAuthInd;


#define SEC_MODE_ATTR_MASK_SEC_MODE  (1U << 0)

typedef struct getSecModeCnf
{
    u8 attrMask;
    u8 result;
    u8 secMode;

} sGetSecModeCnf, *spGetSecModeCnf;

typedef struct setSecModeReq
{
    u8 secMode;
} sSetSecModeReq, *spSetSecModeReq;

typedef struct setSecModeCnf
{
    u8 result;

} sSetSecModeCnf, *spSetSecModeCnf;


#define SET_KEY_ATTR_MASK_SL   (1U << 0)
#define SET_KEY_ATTR_MASK_NID  (1U << 1)
#define SET_KEY_ATTR_MASK_NMK  (1U << 2)

typedef struct setKeyReq
{
    u8 attrMask;
    u8 nmk[ENC_KEY_LEN];
    u8 nid[NID_LEN];
    u8 sl;
} sSetKeyReq, *spSetKeyReq;


typedef struct setKeyCnf
{
    u8 result;
} sSetKeyCnf, *spSetKeyCnf;

typedef struct getKeyCnf
{
    u8 nid[NID_LEN];
    u8 nmk[ENC_KEY_LEN];

} sGetKeyCnf, *spGetKeyCnf;


typedef struct setPPKeysReq
{
    u8 attrMask;
    u8 ppEks[ENC_KEY_LEN];
    u8 ppek[ENC_KEY_LEN];
    u8 macAddr[MAC_ADDR_LEN];

} sSetPPKeysReq, *spSetPPKeysReq;


typedef struct setPPKeysCnf
{
    u8 result;
} sSetPPKeysCnf, *spSetPPKeysCnf;

typedef struct setAuthModeReq
{
    u8 authMode;

} sSetAuthModeReq, *spSetAuthModeReq;

typedef struct setAuthModeCnf
{
    u8 result;

} sSetAuthModeCnf, *spSetAuthModeCnf;

typedef struct setNetReq
{
    u8 nid[NID_LEN];
    u8 reqType;
} sSetNetReq, *spSetNetReq;


typedef struct setNetCnf
{
    u8 result;
} sSetNetCnf, *spSetNetCnf;

typedef struct ccoApptReq
{
    u8 macAddr[MAC_ADDR_LEN];
    u8 reqType;
} sCcoApptReq, *spCcoApptReq;


#endif	/* H1MSGS_H */

