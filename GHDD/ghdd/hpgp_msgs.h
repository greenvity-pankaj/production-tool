#ifndef HPGP_MSGS_H
#define HPGP_MSGS_H

#ifndef NID_LEN
#define NID_LEN                 	( 7 )
#endif

#ifndef ENC_KEY_LEN
#define ENC_KEY_LEN             	( 16 )
#endif

#ifndef MAX_PASSWD_LEN
#define MAX_PASSWD_LEN				( 64 )
#endif

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

enum h1MsgTypes
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
#define DEV_CAP_INFO_REQ 0xF0 // Used to identify GV1011/13 device from user space program through netlink 
#define DEV_CAP_INFO_CNF 0xF1 // Firmware sends this information from Firmware ready event


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
#define HOST_CMD_COMMIT_REQ			( 0x95 )
#define HOST_CMD_COMMIT_CNF			( 0x96 )
#define HOST_CMD_GET_VERSION_REQ	( 0x97 )
#define HOST_CMD_GET_VERSION_CNF	( 0x98 )
#define HOST_CMD_PSAVLN_REQ         ( 0x99 )
#define HOST_CMD_PSAVLN_CNF         ( 0xA0 )
#define HOST_CMD_PSSTA_REQ          ( 0xA1 )
#define HOST_CMD_PSSTA_CNF          ( 0xA2 )
#define HOST_CMD_GV_RESET_REQ   	( 0xA3 )// No confirm message for GV Reset. As we are resetting chip so state machine will not be able to send confirm
#define HOST_CMD_ERASE_FLASH_REQ   	( 0xA4 )
#define HOST_CMD_ERASE_FLASH_CNF   	( 0xA5 )
#define HOST_CMD_VENDORSPEC_REQ 	( 0xA8 )		// Install Mode - Vendor specific command
#define HOST_CMD_VENDORSPEC_CNF		( 0xA9 )		// Install Mode - Vendor specific command
#define HOST_CMD_COMMISSIONING_REQ  ( 0xAA )
#define HOST_CMD_COMMISSIONING_CNF  ( 0xAB )



#define aMaxBeaconPayloadLength         (aMaxPHYPacketSize - aMaxBeaconOverhead)
// host events
#define HOST_EVENT_FW_READY		( 0x01 )

typedef struct hostEventHdr
{
    u8       type;             //event type
    u8       eventClass;    //0: control event. 1: msg event, 2: mgmt event
    u8       rsvd1;
    u8       rsvd2;
}__attribute__((packed))  hostEventHdr_t;

enum mctrlState {
    MCTRL_STATE_INIT,
    MCTRL_STATE_NET_DISC,
    MCTRL_STATE_UNASSOC_STA,
    MCTRL_STATE_ASSOC_STA,
    MCTRL_STATE_UNASSOC_CCO,
    MCTRL_STATE_ASSOC_CCO,
    MCTRL_STATE_MAX
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

enum networkIndReason_e {
   NW_IND_REASON_INIT = 1,  
   NW_IND_REASON_NWDISCOVERY,
   NW_IND_REASON_HANDOVER,
   NW_IND_REASON_CCOBACKUP,
   NW_IND_REASON_CCOAPPOINT,
   NW_IND_REASON_BCNLOSS,
   NW_IND_REASON_USERCMD,
   NW_IND_REASON_RESERVED
};

typedef struct {
	u8 command;
	u8 action;
	u8 result;	
	u8 secmode;
}__attribute__((packed)) secmode_t;

typedef struct {
	u8 command;
	u8 action;
	u8 result;	
	u8 pwdlen;
	u8 passwd[MAX_PASSWD_LEN];	
	u8 seclvl;
	u8 nid[NID_LEN];
	u8 nmk[ENC_KEY_LEN];
	u8 dak[ENC_KEY_LEN];
}__attribute__((packed)) netid_t;

typedef struct {
	u8 command;
	u8 action;
	u8 result;		
}__attribute__((packed)) restartsta_t;

typedef struct {
	u8 command;
	u8 action;
	u8 result;	
	u8 netoption;
	u8 nid[NID_LEN];
}__attribute__((packed)) network_t;

typedef struct {
	u8 command;
	u8 action;
	u8 result;		
}__attribute__((packed)) netexit_t;

typedef struct {
	u8 command;
	u8 action;
	u8 result;	
	u8 reqtype;
	u8 mac_addr[MAC_ADDR_LEN];
}__attribute__((packed)) appointcco_t;

typedef struct {
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
}__attribute__((packed)) authsta_t;

typedef struct {
	u8 command;
	u8 action;
	u8 result;
	u8 datapath;
}__attribute__((packed)) datapath_t;

typedef struct {
	u8 command;
	u8 action;
	u8 result;	
	u8 sniffer;
}__attribute__((packed)) sniffer_t;

typedef struct {
	u8 command;
	u8 action;
	u8 result;	
	u8 bridge;
}__attribute__((packed)) bridge_t;

typedef struct {
	u8 command;
	u8 action;
	u8 result;	
	u8 devmode;
}__attribute__((packed)) devmode_t;

typedef union {
	struct{
		u8 er:1;
		u8 rsvd:1;
		u8 rsvd1:2;
		u8 rsvd2:4;
		u8 rsvd3:8;
	}field;
	u16 val;
} __attribute__((packed)) hw_cfg_t;


typedef union {
	struct{
		u8 rsvd:8;
		u8 rsvd1:8;
	}field;
	u16 val;
} __attribute__((packed)) hw_cfg_rsvd_t;

typedef struct {
	u8 command;
	u8 action;
	u8 result;	
	u8 mac_addr[MAC_ADDR_LEN];
	u8 linemode;
#define LINE_MODE_AC 0
#define LINE_MODE_DC 1
#define LINE_MODE_INVALID 0xff
	u8 dc_frequency;
#define FREQUENCY_50HZ 0
#define FREQUENCY_60HZ 1
	hw_cfg_t hw_cfg;
#define ER_DISABLED 0
#define ER_ENABLED 1
	hw_cfg_rsvd_t rsvd;
	u8 txpowermode;//0,1,2 or 0xff where 0xff means invalid
}__attribute__((packed)) hwspec_t;
#ifdef COMMISSIONING
typedef struct {
	u8 command;
	u8 action;
	u8 result;	
	
    u8 nid[7];  
    u8 nw_key[16];
    u16 cin;
    u16 panId; 
    u8 installFlag;
	
}__attribute__((packed))commissioning_t;
#endif
typedef struct{
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
}__attribute__((packed)) devstats_t;

typedef struct{
	u8 command;
	u8 action;
	u8 result;
	u8 noofentries;
}__attribute__((packed)) peerinfo_t;

typedef struct{		
	u8 macaddr[MAC_ADDR_LEN];
	u8 tei;
	u8 rssi;
	u8 lqi;
}__attribute__((packed)) peerinfodata;

typedef struct {
	u8 command;
	u8 action;
	u8 result;		
}__attribute__((packed)) swreset_t;

typedef struct {
	u8 command;
	u8 action;
	u8 result;		
}__attribute__((packed)) hostCmdGvreset_t;

typedef struct {
	u8 command;
	u8 action;
	u8 result;	
	u8 mode;
}__attribute__((packed)) txpwrmode_t;

typedef struct{
	u8 command;
	u8 action;
	u8 result;
}__attribute__((packed)) hostCmdCommit_t;

typedef struct{
	u8 command;
	u8 action;
	u8 result;
}__attribute__((packed)) hostCmdEraseFlash_t;

typedef struct{
	u8 command;
	u8 action;
	u8 result;
	u8 hwVer[20];
	u8 swVer[20];
}__attribute__((packed)) hostCmdGetVersion_t;

typedef struct{
	u8 command;
	u8 action;  // set, Get
	u8 result;
	u8 mode1; 	// ON,OFF
}__attribute__((packed)) hostCmdPsAvln_t;

typedef struct{
	u8 command;
	u8 action; 	 // Set,Get
	u8 result;
    u8 mode1; 	 // ON, OFF
	u8 awd; 	 // range from 0 to 14
    u8 psp; 	 // range from 0 to 10
}__attribute__((packed)) hostCmdPsSta_t;


typedef struct {
    u8 state;
    u8 reason;
}__attribute__((packed)) hostEvent_networkInd;
 
typedef struct {
    u8 mode; //GET or SET
    u8 peerMacAddress[MAC_ADDR_LEN];
    u8 peerTei;
    u8 result;
}__attribute__((packed)) hostEvent_nmkProv; //NMK Provisioning
 
 
typedef struct {
    u8 tei;
}__attribute__((packed)) hostEvent_teiRenewed;
 
typedef struct {
    u8 mode;
    u8 peerMacAddress[MAC_ADDR_LEN];
    u8 peerTei;
    u8 result;
}__attribute__((packed)) hostEvent_nekProv; //NEK Provisioning
 
typedef struct {
    u8 peerMacAddress[MAC_ADDR_LEN];
    u8 peerTei;
    u8 reason;
}__attribute__((packed)) hostEvent_peerLeave;
 
typedef struct {
    u8 reason;
}__attribute__((packed)) hostEvent_netExit;
 
/*
typedef struct {
}__attribute__((packed)) hostEvent_preBcnLoss;
 
typedef struct {
}__attribute__((packed)) hostEvent_bcnLoss;
*/

typedef struct {
    u8 peerMacAddress[MAC_ADDR_LEN];
    u8 peerTei; 
}__attribute__((packed)) hostEvent_selectedProxyCCo;
 
/*
typedef struct { 
}__attribute__((packed)) hostEvent_selectedBackupCCo;
 */
typedef struct {
    u8 ccoAddress[MAC_ADDR_LEN];
    u8 nid[NID_LEN];
    u8 staTei;
}__attribute__((packed)) hostEvent_assocInd;
 
 
typedef struct {
    u8 staAddress[MAC_ADDR_LEN];
    u8 staTei;
}__attribute__((packed)) hostEvent_staAssocInd;
 
 
typedef struct {
    u8 numOfSta;
    u8 macAddrlist;  // list of ‘ numOfSta’ mac addresses
}__attribute__((packed)) hostEvent_selectedBrdcstRepeater;

typedef struct _hostEvent_peerAssocInd_t
{
	u8	macAddress[MAC_ADDR_LEN];
	u8  tei;
}__attribute__((packed)) hostEvent_peerAssocInd;

typedef struct {
	u8 command;
	u8 action;
	u8 result;	
	u8 capability;
	u8 ghdd_init_done;
}__attribute__((packed)) gv701x_capability_t;// Provides GV device capability i.e 7011 or 7013

#endif //HPGP_MSGS_H

