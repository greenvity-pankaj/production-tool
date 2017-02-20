/* ========================================================
 *
 * @file: hpgp_msgs.h
 * 
 * @brief: This file contains all user hpgp message defines and types needed 
 *         communicate with the HPGP firmware
 * 
 *  Copyright (C) 2010-2015, Greenvity Communications, Inc.
 *  All Rights Reserved
 *  
 * =========================================================*/

#ifndef HPGP_MSGS_H
#define HPGP_MSGS_H

/****************************************************************************** 
  *	Macros
  ******************************************************************************/

/*Miscellaneous defines*/
#define MAX_PASSWD_LEN					(64)
#define MAX_NID_LEN                 	(7)
#define MAX_NETWORK_LIST				(6)	
#define OUID_LEN						(3)
#define VENDOR_SPEC_FIELD_LEN		    (12)


/*Host internal commands*/
#define HOST_CMD_DATAPATH_REQ			( 0x81 )
#define HOST_CMD_DATAPATH_CNF			( 0x82 )
#define HOST_CMD_SNIFFER_REQ			( 0x83 )
#define HOST_CMD_SNIFFER_CNF			( 0x84 )
#define HOST_CMD_BRIDGE_REQ				( 0x85 )
#define HOST_CMD_BRIDGE_CNF				( 0x86 )
#define HOST_CMD_DEVICE_MODE_REQ		( 0x87 )
#define HOST_CMD_DEVICE_MODE_CNF		( 0x88 )
#define HOST_CMD_HARDWARE_SPEC_REQ		( 0x89 )
#define HOST_CMD_HARDWARE_SPEC_CNF		( 0x8A )
#define HOST_CMD_DEVICE_STATS_REQ		( 0x8B )
#define HOST_CMD_DEVICE_STATS_CNF		( 0x8C )
#define HOST_CMD_PEERINFO_REQ			( 0x8D )
#define HOST_CMD_PEERINFO_CNF			( 0x8E )
#define HOST_CMD_SW_RESET_REQ  			( 0x8F )
#define HOST_CMD_SW_RESET_CNF 			( 0x90 )
#define HOST_CMD_FW_READY           	( 0x91 )
#define HOST_CMD_TX_POWER_MODE_REQ  	( 0x93 )
#define HOST_CMD_TX_POWER_MODE_CNF  	( 0x94 )
#define HOST_CMD_COMMIT_REQ         	( 0x95 )
#define HOST_CMD_COMMIT_CNF         	( 0x96 )
#define HOST_CMD_GET_VERSION_REQ		( 0x97 )
#define HOST_CMD_GET_VERSION_CNF		( 0x98 )
#define HOST_CMD_PSAVLN_REQ         	( 0x99 )
#define HOST_CMD_PSAVLN_CNF         	( 0xA0 )
#define HOST_CMD_PSSTA_REQ          	( 0xA1 )
#define HOST_CMD_PSSTA_CNF          	( 0xA2 )
#define HOST_CMD_GV_RESET_REQ   		( 0xA3 )
#define HOST_CMD_ERASE_FLASH_REQ   		( 0xA4 )
#define HOST_CMD_ERASE_FLASH_CNF   		( 0xA5 )
#define HOST_CMD_SCANNETWORK_REQ 		( 0xA6 )
#define HOST_CMD_SCANNETWORK_CNF		( 0xA7 )
#define HOST_CMD_VENDORSPEC_REQ 		( 0xA8 )
#define HOST_CMD_VENDORSPEC_CNF			( 0xA9 )

/*Used by driver and application to identify 7011(RF +PLC) and 7013*/
#define HOST_CMD_DEV_CAP_INFO_REQ 		( 0xF0 )
#define HOST_CMD_DEV_CAP_INFO_CNF 		( 0xF1 ) 
/* end */

/*Host HPGP commands*/
enum hostMsgTypes
{
    APCM_CONN_ADD_REQ = 1,
    APCM_AUTHORIZE_REQ = 14,
    APCM_AUTHORIZE_CNF,
    APCM_AUTHORIZE_IND,
    APCM_GET_SECURITY_MODE_REQ,
    APCM_GET_SECURITY_MODE_CNF,
    APCM_SET_SECURITY_MODE_REQ,
    APCM_SET_SECURITY_MODE_CNF,    /* 20 */
    APCM_GET_NETWORKS_REQ,         
    APCM_GET_NETWORKS_CNF,
    APCM_SET_NETWORKS_REQ,
    APCM_SET_NETWORKS_CNF,

    APCM_SET_KEY_REQ = 28,
    APCM_SET_KEY_CNF,
    APCM_GET_KEY_REQ,              /* 30 */
    APCM_GET_KEY_CNF,              
    APCM_STA_RESTART_REQ,
    APCM_STA_RESTART_CNF,
    APCM_NET_EXIT_REQ,
    APCM_NET_EXIT_CNF,


    APCM_SET_PPKEYS_REQ = 56,
    APCM_SET_PPKEYS_CNF,
    APCM_SET_AUTH_MODE_REQ,
    APCM_SET_AUTH_MODE_CNF,
    APCM_CCO_APPOINT_REQ,
    APCM_CCO_APPOINT_CNF,
};

/* Nwk Identification  state definition */
typedef enum _nwIdState_e
{
    NWID_STATE_INIT,
    NWID_STATE_NET_DISC,
    NWID_STATE_UNASSOC_STA,
    NWID_STATE_ASSOC_STA,
    NWID_STATE_UNASSOC_CCO,
    NWID_STATE_ASSOC_CCO,
    NWID_STATE_MAX
}nwIdState_e;

/* Host events*/
typedef enum _hostEvent_e
{
	HOST_EVENT_FW_READY = 1,
	HOST_EVENT_APP_TIMER,		
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
	HOST_EVENT_TYPE_PEER_ASSOC_IND,
	HOST_EVENT_SCAN_COMPLETE_IND
}hostEvent_e;

/* Nwk indications*/
typedef enum hostEventNetworkIndReason_e 
{
   HOST_EVENT_NW_IND_REASON_INIT = 0,  
   HOST_EVENT_NW_IND_REASON_CCO_LEAVE_IND,
   HOST_EVENT_NW_IND_REASON_CCOBACKUP,
   HOST_EVENT_NW_IND_REASON_CCOAPPOINT,
   HOST_EVENT_NW_IND_REASON_CCOHANDOVER, 
   HOST_EVENT_NW_IND_REASON_BCNLOSS,
   HOST_EVENT_NW_IND_REASON_USERCMD,
   HOST_EVENT_NW_IND_REASON_NWDISCOVERY,
   HOST_EVENT_NW_IND_REASON_AVLN_FOUND,
   HOST_EVENT_NW_IND_REASON_NO_AVLN,
   HOST_EVENT_NW_IND_REASON_AUTH_FAIL,
   HOST_EVENT_NW_IND_REASON_ASSOC_FAIL, 
   HOST_EVENT_NW_IND_REASON_ASSOC_CMD,  
   HOST_EVENT_NW_IND_REASON_NETEXIT_CMD,
   HOST_EVENT_NW_IND_REASON_RESERVED = 0xFF
};

/* Nwk indications*/
typedef enum hostEventPeerLeaveReason_e 
{
   HOST_EVENT_PEER_LEAVE_REASON_TEI_TIMEOUT,  
   HOST_EVENT_PEER_LEAVE_REASON_PEER_LEAVING,
   HOST_EVENT_PEER_LEAVE_REASON_RESERVED = 0xFF,
};

/* Set of primitives*/
typedef struct{
	u8 MACAddr[MAC_ADDR_LEN];
	u8 tei;
	u8 rssi;
	u8 lqi;
} __PACKED__ peerinfo, *ppeerinfo;

typedef struct 
{
	u8 command;
	u8 action;
	u8 result;	
	u8 secmode;
}__PACKED__ hostCmdSecMode;

typedef struct 
{
	u8 command;
	u8 action;
	u8 result;	
	u8 pwdlen;
	u8 passwd[MAX_PASSWD_LEN];	
	u8 seclvl;
	u8 nid[NID_LEN];
	u8 nmk[ENC_KEY_LEN];
	u8 dak[ENC_KEY_LEN];
}__PACKED__ hostCmdNetId;

typedef struct 
{
	u8 command;
	u8 action;
	u8 result;		
}__PACKED__ hostCmdRstSta;

typedef struct 
{
	u8 command;
	u8 action;
	u8 result;	
	u8 netoption;
	u8 nid[NID_LEN];
}__PACKED__ hostCmdNwk;

typedef struct 
{
	u8 command;
	u8 action;
	u8 result;		
}__PACKED__ hostCmdNetExit;

typedef struct 
{
	u8 command;
	u8 action;
	u8 result;	
	u8 reqtype;
	u8 mac_addr[MAC_ADDR_LEN];
}__PACKED__ hostCmdAptCco;

typedef struct 
{
	u8 command;
	u8 action;
	u8 result;
	u8 pwdlen;
	u8 passwd[MAX_PASSWD_LEN];
	u8 nwkpwdlen;
	u8 nwkpasswd[MAX_PASSWD_LEN];
	u8 mac_addr[MAC_ADDR_LEN];
	u8 dak[ENC_KEY_LEN];
    u8 nmk[ENC_KEY_LEN];
    u8 nid[NID_LEN];
	u8 seclvl;
}__PACKED__ hostCmdAuthSta;

typedef struct 
{
	u8 command;
	u8 action;
	u8 result;
	u8 datapath;
}__PACKED__ hostCmdDatapath;

typedef struct 
{
	u8 command;
	u8 action;
	u8 result;	
	u8 sniffer;
}__PACKED__ hostCmdSniffer;

typedef struct 
{
	u8 command;
	u8 action;
	u8 result;	
	u8 bridge;
}__PACKED__ hostCmdBridge;

typedef struct 
{
	u8 command;
	u8 action;
	u8 result;	
	u8 devmode;
}__PACKED__ hostCmdDevmode;

typedef struct 
{
	u8 command;
	u8 action;
	u8 result;	
}__PACKED__ hostCmdSwReset;

typedef union 
{
	struct{
		u8 er:1;
		u8 rsvd:1;
		u8 ps:2;
		u8 rsvd2:4;
		u8 rsvd3:8;
	}field;
	u16 val;
} __PACKED__ hw_cfg_t;

typedef union 
{
	struct{
		u8 rsvd:8;
		u8 rsvd1:8;
	}field;
	u16 val;
} __PACKED__ hw_cfg_rsvd_t;

typedef struct 
{
	u8 command;
	u8 action;
	u8 result;	
	u8 mac_addr[MAC_ADDR_LEN];
    u8 linemode;
	u8 dc_frequency;
	hw_cfg_t hw_cfg;
	hw_cfg_rsvd_t rsvd;
	u8 txpowermode;
}__PACKED__ hostCmdHwspec;

typedef struct 
{
	u8 command;
	u8 action;
	u8 result;		
}__PACKED__ gvreset_t;

typedef struct
{
	u8 command;
	u8 action;
	u8 result;
}__PACKED__ hostCmdEraseFlash;

typedef struct
{
	u8 command;
	u8 action;
	u8 result;
	u32 txdatapktcnt;
	u32 rxdatapktcnt;
	u32 txtotalpktcnt;
	u32 rxtotalpktcnt;
	u32 txpktdropcnt;
	u32 rxpktdropcnt;
	u32 txhostpktcnt;
	u32 rxhostpktcnt;
}__PACKED__ hostCmdDevstats;

typedef struct
{
	u8 command;
	u8 action;
	u8 result;
	u8 noofentries;
}__PACKED__ hostCmdPeerinfo;

typedef struct
{		
	u8 macaddr[MAC_ADDR_LEN];
	u8 tei;
	u8 rssi;
	u8 lqi;
}__PACKED__ peerinfodata;

typedef struct
{
	u8 command;
	u8 action;
	u8 result;
	u8 powermode;
}__PACKED__ hostCmdTxPowerMode;

typedef struct
{
	u8 command;
	u8 action;
	u8 result;
}__PACKED__ hostCmdCommit;

typedef struct
{
	u8 app_id;
    u16 type;
} __PACKED__ hostTimerEvnt_t;

typedef struct
{
	nwIdState_e  state;
	u8 			reason;
}__PACKED__ hostEvent_NetworkId;

typedef struct
{
	u8	ccoAddress[MAC_ADDR_LEN];
	u8  nid[MAX_NID_LEN];
	u8  tei;
}__PACKED__ hostEvent_assocInd;

typedef struct
{
	u8	macAddress[MAC_ADDR_LEN];
	u8  tei;
}__PACKED__ hostEvent_peerAssocInd;


typedef struct
{
	u8	macAddress[MAC_ADDR_LEN];
	u8  tei;
	u8  reason;
}__PACKED__ hostEvent_peerLeave_t;

typedef struct
{
	u8  reason;
}__PACKED__ hostEvent_nextExit_t;

typedef struct
{
	u8	ccoAddress[MAC_ADDR_LEN];
	u8  nid[MAX_NID_LEN];
	u8  tei;
}__PACKED__ hostEvent_authComplete;

typedef struct
{
	u32 TxPktCnt;
	u32 RxPktCnt;
	u32 TxPktDropCnt;
	u32 RxPktDropCnt;
	u32 TxEthPktCnt;
	u32 RxEthPktCnt;
} __PACKED__ host_devstats, *pdevstats;

typedef struct
{
	u8 tei;
	u8 ntei;
	u8 numHop;
} __PACKED__ routeEvent;

typedef struct
{
	u8 command;
	u8 action;
	u8 result;
	u8 hwVer[20];
	u8 swVer[20];
} __PACKED__ hostCmdGetVersion;

typedef struct
{
	u8 command;
	u8 action;
	u8 result;
	u8 mode;
}__PACKED__ hostCmdPsAvln;

typedef struct
{
	u8 command;
	u8 action;
	u8 result;
    u8 mode;
	u8 awd;
    u8 psp;
}__PACKED__ hostCmdPsSta;

typedef struct
{
	u8 ouid[3];
	u8 buf[VENDOR_SPEC_FIELD_LEN];
}__PACKED__ svendorota;

typedef struct
{	
	u8 command;
	u8 action;
	u8 result;	
	u32 scanTime;
} __PACKED__ hostCmdScanNet, *phostCmdScanNet;

typedef struct
{		
	u32 	 bcnRxCnt;
	u8 		 rssi;
	u8       lqi;
	u8		 valid;
//	u8		 hit   : 1;
//	u8		 rsvd1 : 6; 		  
	u8		 nid[NID_LEN];			 //network id from the beacon header
	u8		 snid;					 //frame control from the MAC 
//	u8		 hybridMode:  2;		 //from the beacon header	
//	u8		 netMode:	  2;		 //from the beacon header  
//	u8		 numBcnSlots: 3;		 //from the beacon header  
//	u8		 rsvd2: 	  1;		   
//	u8		 coordStatus;			 //coordination status
//	u16 	 offset;				 //offset btw beacon region of disovered net
	u8		 numOfSta;									   
//	u8		 slotId;
//	u8		 slotUsage;
	svendorota vendor_ota;
} hostEventScanList, *phostEventScanList;

typedef struct
{
	u8 noOfEntries;
	hostEventScanList networkList[MAX_NETWORK_LIST];
} __PACKED__ hostEvent_ScanInd_t;

typedef struct
{	
	u8 command;
	u8 action;
	u8 result;
	u8 enable;
	svendorota vendor_ota;
} __PACKED__ hostCmdVendorSpec, *phostCmdVendorSpec;


#endif /*HPGP_MSGS_H*/
