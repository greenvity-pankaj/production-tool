
/*******************************************************************************
 *
 * File:       hybrii_mac_intf_common.h
 * Contact:    pankaj_razdan@greenvity.com
 *
 * Description: Host interface to Hybrii MAC
 *
 * Copyright (c) 2011 by Greenvity Communication.
 *
 ******************************************************************************/
#ifndef __HYBRII_MAC_INTF_COMMON_H__
#define __HYBRII_MAC_INTF_COMMON_H__

#define MAX_HOST_CMD_LENGTH         (128)

#define IEEE802_15_4_MAC_ID 		(0x01)
#define HPGP_MAC_ID		 			(0x02)

#define CONTROL_FRM_ID				(0x00)
#define DATA_FRM_ID					(0x01)
#define MGMT_FRM_ID					(0x02)
#define EVENT_FRM_ID                (0x03)


typedef struct _hostHdr_
{
    u8 protocol  : 2;
    u8 type      : 2;
    u8 rsvd1     : 4;
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

void GV701x_CmdSend(hostHdr_t *pHostHdr, u16 frm_len);

#endif /* __HYBRII_MAC_INTF_COMMON_H__ */


