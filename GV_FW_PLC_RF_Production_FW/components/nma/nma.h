/** =========================================================
 *
 *  @file nma.h
 * 
 *  @brief Network Management Agent
 *
 *  Copyright (C) 2010-2015, Greenvity Communications, Inc.
 *  All Rights Reserved
 *  
 * ==========================================================*/

#ifndef _NMA_H
#define _NMA_H

/****************************************************************************** 
  *	Includes
  ******************************************************************************/
  
#include "list.h"
#ifdef NO_HOST
#include "gv701x_osal.h"
#endif

/****************************************************************************** 
  *	Defines
  ******************************************************************************/

#define SYS_MAC_ID 							(0x00)
#define IEEE802_15_4_MAC_ID 				(0x01)
#define HPGP_MAC_ID		 					(0x02)
#define APP_MAC_ID		 					(0x03)

#define CONTROL_FRM_ID						(0x00)
#define DATA_FRM_ID							(0x01)
#define MGMT_FRM_ID							(0x02)
#define EVENT_FRM_ID                		(0x03)

/*Action values*/
#define ACTION_GET							(0)
#define ACTION_SET							(1)
#define ACTION_IND							(2)

#define PLC_NIC								BIT(0)	
#define RF_NIC								BIT(1)

#define MAX_HOST_CMD_LENGTH         		(250)

typedef struct _hostHdr_
{
    u8 protocol: 2;
    u8 type: 2;
    u8 rsvd1: 4;
	u8 rsvd;        
    u16 length;
}__PACKED__  hostHdr_t;

typedef struct hostEventHdr
{
    u8       type;          
    u8       eventClass;    
    u8       rsvd1;
    u8       rsvd2;
}__PACKED__  hostEventHdr_t;

#ifdef NO_HOST
typedef struct
{
	sEvent msg_event;
	hostHdr_t msg_hybrii_hdr;
	gv701x_app_msg_hdr_t msg_hdr;	
}mac_host_db_t;
#endif

typedef struct
{
	u8 command;
	u8 action;
	u8 result;
	mac_addr_t macaddr;
}__PACKED__ hostCmdGetMacAddress;

#define HOST_CMD_GET_MACADDRESS_REQ			( 0xAA )
#define HOST_CMD_GET_MACADDRESS_CNF			( 0xAB )

typedef struct nma
{
#ifdef NO_HOST
	sEvent msg_event;
	hostHdr_t msg_hybrii_hdr;
	gv701x_app_msg_hdr_t msg_hdr;	

	sEvent msg_event_1;
	hostHdr_t msg_hybrii_hdr_1;
	gv701x_app_msg_hdr_t msg_hdr_1;	
#endif
    sSlist   eventQueue;
}sNma, *psNma;

/****************************************************************************** 
  *	Function Prototypes
  ******************************************************************************/
  
void NMA_RecvMgmtPacket(hostHdr_t* buf, u16 packetlen);
void GV701x_CmdSend(hostHdr_t *pHostHdr, u16 frm_len);	

#endif /*_NMA_H*/
