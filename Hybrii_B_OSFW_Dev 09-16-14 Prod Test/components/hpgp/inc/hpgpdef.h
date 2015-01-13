/** ========================================================
 *
 * @file hpgpdef.h
 * 
 *  @brief HomePlug GREEN PHY Data Structure Definition
 *
 *  Copyright (C) 2010-2011, Greenvity Communications, Inc.
 *  All Rights Reserved
 *  
 * ==========================================================*/


#ifndef _HPGPDEF_H
#define _HPGPDEF_H

typedef enum _eDevMode
{
    DEV_MODE_CCO = 0,
    DEV_MODE_STA = 1
}eDevMode, *peDevMode;

typedef enum _eLineMode
{
    LINE_MODE_AC = 0,
    LINE_MODE_DC = 1
}eLineMode, *peLineMode;

typedef enum _eLineFreq
{
	FREQ_50HZ = 0,
	FREQ_60HZ
}eLineFreq;

/*Power Modes*/
typedef enum _eTxPwrMode
{
    AUTOMOTIVE_TX_POWER_MODE = 0,
    NORMAL_TX_POWER_MODE = 1,
    HIGH_TX_POWER_MODE = 2,
    DEFAULT_TX_POWER_MODE = 0xFF    
}eTxPwrMode, *peTxPwrMode;

enum staRole
{
    STA_ROLE_UNKNOWN,
    STA_ROLE_USTA,
    STA_ROLE_ASTA,
    STA_ROLE_UCCO,
    STA_ROLE_ACCO
};

enum secMode
{
    SEC_MODE_HS,
    SEC_MODE_SC,
    SEC_MODE_SC_ADD,
    SEC_MODE_SC_JOIN
};

enum {
    NETWORK_START,
    NETWORK_JOIN,
	NETWORK_JOIN_PASSIVE,
    NETWORK_LEAVE,
    NETWORK_BLACKLIST,
    NETWORK_REHAB,
};

/* Prioirty Link ID (PLID) */
enum 
{
    PRI_LINK_ID_0 = 0,
    PRI_LINK_ID_1 = 1,
    PRI_LINK_ID_2 = 2,
    PRI_LINK_ID_3 = 3
};

#define NETDISC_OPT_CCO_DET     BIT(0) //CCo detection
#define NETDISC_OPT_CCO_SEL     BIT(1) //Cco selection

#define NETDISC_ENABLE          1 
#define NETDISC_DISABLE         0 

#define MAX_DPW_LEN             32

#define PASSWORD_LEN            64
#define NID_LEN                 7
#define HFID_LEN                64

#define ENC_KEY_LEN             16
#define ENC_IV_LEN              16
#define SECLV_SC                0x0
#define SECLV_HS                0x1
#define SECLV_MASK              0x0F
#define NID_EXTRA_BIT_MASK      0x3F
#define SECLV_OFFSET            4

#define HASH_KEY_LEN            384



#define VLAN_LEN                4
#define ETH_TYPE_HPGP           0x88e1

#define HPGP_TX_RETRY_MAX       3    // times

#define HPGP_NETWORK_EXIT_REASON_USER_REQ  0   //user request
#define HPGP_NETWORK_EXIT_REASON_PWR_DOWN  1   //power down

#define HPGP_LEAVE_REQ_REASON_USER_REQ        0   //user request
#define HPGP_LEAVE_REQ_REASON_PWR_DOWN        1   // tei lease expired

#define HPGP_LEAVE_IND_REASON_USER_REQ        0   //user request
#define HPGP_LEAVE_IND_REASON_TEI_EXP         1   // tei lease expired
#define HPGP_LEAVE_IND_REASON_CCO_DOWN        2   // CCo shutting down


#define HPGP_HO_REASON_CCO_APPT    0x0   //user-appointed
#define HPGP_HO_REASON_CCO_SLCT    0x1   //CCO selection
#define HPGP_HO_REASON_CCO_LEAVE   0x2   //CCO leaving



//management message type
#define    MMTYPE_CC_CCO_APPOINT_REQ        0x0000
#define    MMTYPE_CC_CCO_APPOINT_CNF        0x0001
#define    MMTYPE_CC_CCO_APPOINT_IND        0x0002
#define    MMTYPE_CC_CCO_APPOINT_RSP        0x0003

#define    MMTYPE_CC_BACKUP_APPOINT_REQ     0x0004
#define    MMTYPE_CC_BACKUP_APPOINT_CNF     (0x0004 + 1)



#define    MMTYPE_CC_HANDOVER_REQ            0x000C 
#define    MMTYPE_CC_HANDOVER_CNF           (0x000C + 0x1)
#define    MMTYPE_CC_HANDOVER_RSP           (0x000C + 0x3)
#define    MMTYPE_CC_HANDOVER_INFO_IND      (0x0010 + 0x2)
#define    MMTYPE_CC_HANDOVER_INFO_RSP      (0x0010 + 0x3)

#define    MMTYPE_CC_DISCOVER_LIST_REQ      (0x0014 + 0x0)
#define    MMTYPE_CC_DISCOVER_LIST_CNF      (0x0014 + 0x1)
#define    MMTYPE_CC_DISCOVER_LIST_IND      (0x0014 + 0x2)
#define    MMTYPE_CC_DISCOVER_LIST_RSP      (0x0014 + 0x3)

#define    MMTYPE_CC_WHO_RU_REQ             (0x002C + 0x0)
#define    MMTYPE_CC_WHO_RU_CNF             (0x002C + 0x1)

#define    MMTYPE_CC_ASSOC_REQ              (0x0030 + 0x0)
#define    MMTYPE_CC_ASSOC_CNF              (0x0030 + 0x1)

#define    MMTYPE_CC_LEAVE_REQ              (0x0034 + 0x0)
#define    MMTYPE_CC_LEAVE_CNF              (0x0034 + 0x1)
#define    MMTYPE_CC_LEAVE_IND              (0x0034 + 0x2)
#define    MMTYPE_CC_LEAVE_RSP              (0x0034 + 0x3)

#define    MMTYPE_CC_SET_TEI_MAP_REQ        (0x0038 + 0x0)
#define    MMTYPE_CC_SET_TEI_MAP_IND        (0x0038 + 0x2)

#define    MMTYPE_CC_HP1_DET_REQ            (0x0054 + 0x0)
#define    MMTYPE_CC_HP1_DET_CNF            (0x0054 + 0x1)

#define    MMTYPE_CC_ISP_DET_REPORT_IND     (0x0064 + 0x2)

#define    MMTYPE_CC_ISP_START_RESYNC_REQ   (0x0068 + 0x0)
#define    MMTYPE_CC_ISP_FINISH_RESYNC_REQ  (0x006C + 0x0)
#define    MMTYPE_CC_ISP_RESYNC_DET_IND     (0x0070 + 0x2)
#define    MMTYPE_CC_ISP_RESYNC_TX_REQ      (0x0074 + 0x0)

#define    MMTYPE_CC_PWR_SAVE_REQ           (0x0078 + 0x0)
#define    MMTYPE_CC_PWR_SAVE_CNF           (0x0078 + 0x1)
#define    MMTYPE_CC_PWR_SAVE_EXIT_REQ      (0x007C + 0x0)
#define    MMTYPE_CC_PWR_SAVE_EXIT_CNF      (0x007C + 0x1)
#define    MMTYPE_CC_PWR_SAVE_LIST_REQ      (0x0080 + 0x0)
#define    MMTYPE_CC_PWR_SAVE_LIST_CNF      (0x0080 + 0x1)
#define    MMTYPE_CC_STOP_PWR_SAVE_REQ      (0x0084 + 0x0)
#define    MMTYPE_CC_STOP_PWR_SAVE_CNF      (0x0084 + 0x1)

#define    MMTYPE_CM_UNASSOC_STA_IND        (0x6000 + 0x2)

#define    MMTYPE_CM_ENCRY_PAYLOAD_IND      (0x6004 + 0x2)
#define    MMTYPE_CM_ENCRY_PAYLOAD_RSP      (0x6004 + 0x3)

#define    MMTYPE_CM_SET_KEY_REQ            (0x6008 + 0x0)
#define    MMTYPE_CM_SET_KEY_CNF            (0x6008 + 0x1)
#define    MMTYPE_CM_GET_KEY_REQ            (0x600C + 0x0)
#define    MMTYPE_CM_GET_KEY_CNF            (0x600C + 0x1)

#define    MMTYPE_CM_SC_JOIN_REQ            (0x6010 + 0x0)
#define    MMTYPE_CM_SC_JOIN_CNF            (0x6010 + 0x1)

#define    MMTYPE_CM_CHAN_EST_IND           (0x6014 + 0x2)

#define    MMTYPE_CM_IDENTIFY_REQ           (0x6060)
#define    MMTYPE_CM_IDENTIFY_CNF           (0x6060 + 0x1)
#define    MMTYPE_CM_IDENTIFY_IND           (0x6060 + 0x2)
#define    MMTYPE_CM_IDENTIFY_RSP           (0x6060 + 0x3)

#define    MMTYPE_CM_BRG_INFO_REQ            (0x6020 + 0x0)
#define    MMTYPE_CM_BRG_INFO_CNF            (0x6020 + 0x1)
#ifdef ROUTE
#define    MMTYPE_CM_ROUTE_INFO_REQ         (0x6050 + 0x0)
#define    MMTYPE_CM_ROUTE_INFO_CNF         (0x6050 + 0x1)
#define    MMTYPE_CM_ROUTE_INFO_IND         (0x6050 + 0x2)
#define    MMTYPE_CM_UNREACHABLE_IND        (0x6054)
#endif

#define    MMTYPE_CM_STA_IDENTIFY_REQ       (0x6060 + 0x0)
#define    MMTYPE_CM_STA_IDENTIFY_CNF       (0x6060 + 0x1)
#define    MMTYPE_CM_STA_IDENTIFY_IND       (0x6060 + 0x2)
#define    MMTYPE_CM_STA_IDENTIFY_RSP       (0x6060 + 0x3)

#define    MMTYPE_NN_INL_REQ				(0x4000 + 0x00)
#define    MMTYPE_NN_INL_CNF				(0x4000 + 0x01)

//hybrid mode
#define    HYBRID_MODE_AV                 0x0
#define    HYBRID_MODE_SHARED_CSMA        0x1
#define    HYBRID_MODE_FULL               0x2
#define    HYBRID_MODE_CSMA_ONLY          0x3

//beacon type
#define    BEACON_TYPE_CENTRAL            0x0    //central beacon
#define    BEACON_TYPE_DISCOVER           0x1    //discover beacon
#define    BEACON_TYPE_PROXY              0x2    //proxy beacon

//network mode
#define    NET_MODE_UNCCORDINATED         0x0    //uncoordinated mode
#define    NET_MODE_CCORDINATED           0x1    //coordinated mode
#define    NET_MODE_CSMA_ONLY             0x2    //csma only mode

//CCo capability
#define    CCO_CAP_LEVEL0                 0x0    //level 0 
#define    CCO_CAP_LEVEL1                 0x1    //level 1 
#define    CCO_CAP_LEVEL2                 0x2    //level 2 
#define    CCO_CAP_LEVEL3                 0x3    //level 3 

//Beacon Entry Header 
#define    BEHDR_NON_PERSISTENT_SCHED     0x00   //Non-Persistent Schedule
#define    BEHDR_PERSISTENT_SCHED         0x01   //Persistent Schedule
#define    BEHDR_REGIONS                  0x02   //region Schedule
#define    BEHDR_MAC_ADDR                 0x03   //MAC Address
#define    BEHDR_DISCOVER                 0x04   //Discover
#define    BEHDR_DISC_INFO                0x05   //Discovered Info
#define    BEHDR_BPSTO                    0x06   //Beacon Period Start Time Offset
#define    BEHDR_ENCRYP_KEY_CHANGE        0x07   //Encryption Key Change
#define    BEHDR_CCO_HANDOVER             0x08   //CCo Handover
#define    BEHDR_BCN_RELOC                0x09   //Beacon Relocation
#define    BEHDR_ACL_SYNC_CNTDOWN         0x0A   //AC Line Sync Countdown
#define    BEHDR_CHANGE_NUM_SLOTS         0x0B   //Change NumSlots
#define    BEHDR_CHANGE_HM                0x0C   //Change Hybrid Mode
#define    BEHDR_CHANGE_SNID              0x0D   //Change SNID
#define    BEHDR_RSN_INFO                 0x0E   //RSN Info Element
#define    BEHDR_ISP                      0x0F   //ISP BENTRY
#define    BEHDR_EXT_BAND_STAY_OUT        0x10   //Extended Band Stay Out
#define    BEHDR_AG_ASSIGN                0x11   //AG Assignment
#define    BEHDR_EXT_CARR_SUPPORT         0x12   //Extended Carriers Support
#define    BEHDR_PWR_SAVE                 0x13   //Power Save BENTRY
#define    BEHDR_VENDOR_SPEC              0xFF   //Vendor Specific



#define    FRAME_TYPE_BEACON              0x0   //
#define    FRAME_TYPE_MGMT                0x1   //
#define    FRAME_TYPE_DATA                0x2   //

#define    HPGP_EKS_NONE                  0xF   

#ifndef __GNUC__
//#pragma pack(1)
#endif
typedef struct eth802dot3Hdr
{
    u8    dstaddr[MAC_ADDR_LEN];
    u8    srcaddr[MAC_ADDR_LEN];
    u16   len;
} __PACKED__ sEth802dot3Hdr, *psEth802dot3Hdr;


typedef struct eth2Hdr
{
    u8    dstaddr[MAC_ADDR_LEN];  //Original Destiation Address
    u8    srcaddr[MAC_ADDR_LEN];  //Original Destiation Address
    u16   ethtype;
} __PACKED__ sEth2Hdr, *psEth2Hdr;


typedef struct ethVlanHdr
{
    u8    dstaddr[MAC_ADDR_LEN];
    u8    srcaddr[MAC_ADDR_LEN];
    u8    vlan[VLAN_LEN];
    u16   ethtype;
} __PACKED__ sEthVlanHdr, *psEthVlanHdr;


typedef struct ethHdr
{
    union 
    {
        sEth2Hdr         ethII;
        sEth802dot3Hdr   eth802dot3;
    } hdr;
} __PACKED__ sEthHdr, *psEthHdr;


typedef struct mmHdr
{
    u8         mmv;          //management message version
    u16        mmtype;       //management message type
    u8         nfmi: 4;      //number of fragments 
    u8         fnmi: 4;      //fragment number
    u8         fmsn;         //fragmentation message sequence number
} __PACKED__ sMmHdr, *psMmHdr;


/* MAC Frame (MF) Header */
typedef struct MfHdr
{
    u8    mft:     2;    /* MAC frame type */ 
    u8    mflLo:   6;    /* MAC frame length higher bits */
    u8    mflHi;         /* MAC frame length lower bits */
} __PACKED__ sMfHdr, *psMfHdr;


/* MAC frame type */
#define HPGP_MFT_PAD            0x0
#define HPGP_MFT_MSDU_NO_ATS    0x1
#define HPGP_MFT_MSDU_ATS       0x2
#define HPGP_MFT_MGMT           0x3
/* MF optional field :ATS or Confounder */
#define HPGP_MF_OPT_LEN         4    
#define HPGP_MF_ICV_LEN         4 

/* HPGP MAC Frame Header Length */
#define HPGP_MF_HDR_LEN         sizeof(sMfHdr) 
/* HPGP Optional MAC Frame Header Length */
#define HPGP_MF_OPT_HDR_LEN    (sizeof(sMfHdr) + HPGP_MF_OPT_LEN)


#ifndef __GNUC__
//#pragma pack()
#endif

/* HPGP management message header length */
#define    HPGP_MM_HEADER_LEN       (sizeof(sEth2Hdr)+sizeof(sMmHdr))
#define    HPGP_MM_VLAN_HEADER_LEN  (sizeof(ssEth802dot3Hdr)+sizeof(sMmHdr))

/* frame control block (128 bits or 16 bytes) */
typedef struct frmCtrlBlk
{
    u8    dtav:   3;     /* delimiter type */
    u8    access: 1;
    u8    snid:   4;
    u8    bts[4];        /* beacon time stamp */
    u16   bto[4];        /* beacon tranmission offset  */
    u8    fccs[3];       /* frame control check sequence */
} __PACKED__ sFrmCtrlBlk, *psFrmCtrlBlk;


#define    BEACON_PAYLOAD_SIZE     136 
#define    BEACON_LEN              (BEACON_PAYLOAD_SIZE + sizeof(sFrmCtrlBlk)) 

#define    HPGP_DATA_PAYLOAD_MIN   (46-sizeof(sMmHdr)) 
#define    HPGP_DATA_PAYLOAD_MAX   (1500-sizeof(sMmHdr))
#define    HPGP_MNBC_PAYLOAD_MAX   (488-sizeof(sMmHdr))

enum 
{
    REGION_TYPE_RSVD = 0,    /* reserved regsion */
    REGION_TYPE_SHARED_CSMA, /* shared CSMA region */
    REGION_TYPE_LOCAL_CSMA,  /* local CSMA region */
    REGION_TYPE_STAYOUT,     /* stayout region */
    REGION_TYPE_PROTECTED,   /* protected region */
    REGION_TYPE_BEACON,      /* beacon region */
};

#ifndef HPGP_HAL_TEST
typedef struct regionSchedule
{                             
    u16   startTime;          // Time from start of BP in units of 10.24 uS
    u16   duration;           // Time from end of prev region in units of 10.24 uS
    // If both startTime and duration are set to Zero, it indicates no more valid regions in BP. 
    u8    bcnRegion     :1;   // Set if region is receive only - will always be Hybrid.
    u8    hybridMd      :1;   // Set if region is hybrid mode.
    u8    regionType    :4;  
    u8    rsvd          :2;                                    
}sRegionSchedule, *psRegionSchedule;

typedef struct regionSchedule sRegionSched;
#endif

/* maximum number of session */
#define HPGP_SESS_MAX     6
/* session allocation information (SAI) */
typedef struct sai
{
    u8    stpf:    1;     /* start time present flag */
    u8    glid:    7;     /* Global Link ID */
    u16   startTime;      /* start time */
    u16   duration;       /* duration */
} sSai;

#endif
