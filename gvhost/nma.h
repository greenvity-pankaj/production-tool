
/** =========================================================
 *
 *  @file nma.h
 * 
 *  @brief Network Management Agent
 *
 *  Copyright (C) 2010-2012, Greenvity Communications, Inc.
 *  All Rights Reserved
 *  
 * ==========================================================*/

#ifndef _NMA_H
#define _NMA_H

#define IEEE802_15_4_MAC_ID 		(0x01)
#define HPGP_MAC_ID		 			(0x02)
#define APP_MAC_ID		 			(0x03)

#define CONTROL_FRM_ID				(0x00)
#define DATA_FRM_ID					(0x01)
#define MGMT_FRM_ID					(0x02)
#define EVENT_FRM_ID                (0x03)

/*Action values*/
#define ACTION_GET					(0)
#define ACTION_SET					(1)
#define ACTION_IND					(2)

#define PLC_NIC						BIT(0)	
#define ZIGBEE_NIC					BIT(1)

#define MAX_HOST_CMD_LENGTH         (250)
#define MAC_ADDR_LEN    6


typedef struct _hostHdr_
{
    u8 protocol  : 2;
    u8 type      : 2;
    u8 rsvd1     : 4;
	u8 rsvd;        
    u16 length;
}PACKED  hostHdr_t;

typedef struct hostEventHdr
{
    u8       type;          
    u8       eventClass;    
    u8       rsvd1;
    u8       rsvd2;
}PACKED  hostEventHdr_t;

typedef struct {
	u8 event;
	u8 eventproto;	
	u8 eventclass;
	u8 eventtype;	
	u16 eventdatalen;
	u8 eventdata[MAX_HOST_CMD_LENGTH];	
}gv701x_event_t;

/* MAC Address */
typedef union {
	u8 addr_8bit[MAC_ADDR_LEN];
	u16 addr_16bit[MAC_ADDR_LEN/2];
}PACKED mac_addr_t;

void NMA_RecvMgmtPacket(hostHdr_t * pHostHdr,u16 packetlen);
void GV701x_CmdSend(hostHdr_t *pHostHdr, u16 frm_len);	
#endif 
