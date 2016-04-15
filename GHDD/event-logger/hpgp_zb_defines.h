/*********************************************************************
 * File:     hpgp_user_api.h 
 * Contact:  prashant_mahajan@greenvity.com
 *
 * Description: Provides supporting macro, structure and function prototypes to 
 *                    hpgp_user_api.c file
 * 
 * Copyright(c) 2012 by Greenvity Communications.
 * 
 ********************************************************************/
#ifndef _HPGP_ZB_DEFINES_H_
#define _HPGP_ZB_DEFINES_H_

/* Typedefs */
typedef unsigned char u8;
typedef unsigned short int  u16;
typedef unsigned int u32, uint32_t;

typedef struct gv_cmd {
    u8 proto        :2;
    u8 frm          :2;
    u8 rsvd         :4;
} __attribute__ ((packed)) gv_frm_ctrl_t;  
typedef struct gv_cmd_hdr {
    gv_frm_ctrl_t fc;
    u8 reserved;
    u16 len;
}__attribute__ ((packed)) gv_cmd_hdr_t;

#define IEEE802_15_4_MAC_ID 		0x01
#define HPGP_MAC_ID		 			0x02

#define CMD_HDR_START					0x00
#define CMD_HDR_LENGTH					0x02
#define CMD_HDR_RSVD					0x01
#define CMD_HDR_LEN						sizeof (gv_cmd_hdr_t)
#define CMD_HDR_PROTOCOLTYPE_MASK		0x03
#define CMD_HDR_FRAMETYPE_MASK			0x0C

/* Frame Type */
#define CONTROL_FRM_ID					0x00
#define DATA_FRM_ID						0x01
#define MGMT_FRM_ID						0x02
#define EVENT_FRM_ID					0x03

#define MAX_REQ_LEN 				( 256 )
#define MAX_RSP_LEN 				( 256 )
#define MAX_PASSWD_LEN				( 64 )
#define MAC_ADDR_LEN				( 6 )

#ifndef NID_LEN
#define NID_LEN                 	( 7 )
#endif

#ifndef ENC_KEY_LEN
#define ENC_KEY_LEN             	( 16 )
#endif


//host commands
#define HOST_CMD_DATAPATH_REQ		( 0x81 )
#define HOST_CMD_DATAPATH_CNF		( 0x82 )
#define HOST_CMD_SNIFFER_REQ		( 0x83 )
#define HOST_CMD_SNIFFER_CNF		( 0x84 )
#define HOST_CMD_BRIDGE_REQ			( 0x85 )
#define HOST_CMD_BRIDGE_CNF			( 0x86 )
#define HOST_CMD_DEVICE_MODE_REQ	( 0x87 )
#define HOST_CMD_DEVICE_MODE_CNF	( 0x88 )
#define HOST_CMD_HARDWARE_SPEC_REQ	( 0x89 )
#define HOST_CMD_HARDWARE_SPEC_CNF	( 0x8A )
#define HOST_CMD_DEVICE_STATS_REQ	( 0x8B )
#define HOST_CMD_DEVICE_STATS_CNF	( 0x8C )
#define HOST_CMD_PEERINFO_REQ		( 0x8D )
#define HOST_CMD_PEERINFO_CNF		( 0x8E )
#define HOST_CMD_SWRESET_REQ		( 0x8F )
#define HOST_CMD_SWRESET_CNF		( 0x90 )
#define HOST_CMD_TRIGGER_EVENT_REQ	( 0x91 )
#define HOST_CMD_SET_TX_POWER_MODE_REQ	( 0x93 )
#define HOST_CMD_SET_TX_POWER_MODE_CNF	( 0x94 )
#define HOST_CMD_COMMIT_REQ   		( 0x95 )
#define HOST_CMD_COMMIT_CNF   		( 0x96 )
#define HOST_CMD_GET_VERSION_REQ	( 0x97 )
#define HOST_CMD_GET_VERSION_CNF	( 0x98 )
#define HOST_CMD_PSAVLN_REQ         ( 0x99 )
#define HOST_CMD_PSAVLN_CNF         ( 0xA0 )
#define HOST_CMD_PSSTA_REQ          ( 0xA1 )
#define HOST_CMD_PSSTA_CNF          ( 0xA2 )
#define HOST_CMD_GV_RESET_REQ   	( 0xA3 )// No confirm message for GV Reset. As we are resetting chip so state machine will not be able to send confirm
#define HOST_CMD_ERASE_FLASH_REQ   	( 0xA4 )
#define HOST_CMD_ERASE_FLASH_CNF   	( 0xA5 )
#define HOST_CMD_VENDORSPEC_REQ 	( 0xA8 )// Install Mode - Vendor specific command
#define HOST_CMD_VENDORSPEC_CNF		( 0xA9 )// Install Mode - Vendor specific command
#define HOST_CMD_COMMISSIONING_REQ  ( 0xAA )
#define HOST_CMD_COMMISSIONING_CNF  ( 0xAB )




#define aMaxPHYPacketSize               (127)

#define aMaxBeaconOverhead              (75)

/**
 * The maximum size, in octets, of a beacon payload.
 */
#define aMaxBeaconPayloadLength         (aMaxPHYPacketSize - aMaxBeaconOverhead)

// host events
#define HOST_EVENT_FW_READY		( 0x01 )

enum hostEvent_hpgp {
    EVENT_TYPE_FW_READY = 1, //Already covered in ZigBee events
    HOST_EVENT_APP_TIMER = 2,		
	HOST_EVENT_APP_CMD,		
	HOST_EVENT_TYPE_ASSOC_IND,
	HOST_EVENT_NETWORK_IND,
	HOST_EVENT_NMK_PROVISION,
	HOST_EVENT_TEI_RENEWED,    
	HOST_EVENT_AUTH_COMPLETE,
	HOST_EVENT_PEER_STA_LEAVE,
	HOST_EVENT_NET_EXIT,
	HOST_EVENT_PRE_BCN_LOSS,
	HOST_EVENT_BCN_LOSS,
	HOST_EVENT_SELECTED_PROXY_CCO,
	HOST_EVENT_SELECTED_BACKUP_CCO,
	HOST_EVENT_SELECTED_BRCST_REPEATER,
	HOST_EVENT_HANDOVER,
	HOST_EVENT_NEW_NW_DETECT,
	HOST_EVENT_TYPE_PEER_ASSOC_IND
};

#define HPGP_NETWORK_EXIT_REASON_USER_REQ  0   //user request
#define HPGP_NETWORK_EXIT_REASON_PWR_DOWN  1   //power down

#define HPGP_LEAVE_REQ_REASON_USER_REQ        0   //user request
#define HPGP_LEAVE_REQ_REASON_PWR_DOWN        1   // tei lease expired

/* Nwk indications*/
typedef enum peerLeaveReason_e {
   HOST_EVENT_PEER_LEAVE_REASON_TEI_TIMEOUT,  
   HOST_EVENT_PEER_LEAVE_REASON_PEER_LEAVING,
   HOST_EVENT_PEER_LEAVE_REASON_RESERVED = 0xFF
} peerLeaveReason;


#define ACTION_GET	( 0 )
#define ACTION_SET	( 1 )
#define SUCCESS		( 0 )
#define STATUS_FAILURE		( 1 )
#define ERROR		( 2 )


/*
non zero result :
AGEOUT
TimeOut
reserved

sta leave reason :
1) USER Request
2) Ageout
3) cco notify
*/
 
#endif //_HPGP_ZB_DEFINES_H_
