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


#include "papdef.h"


enum staRole
{
	STA_ROLE_UNKNOWN,
	STA_ROLE_USTA,
	STA_ROLE_ASTA,
	STA_ROLE_UCCO,
	STA_ROLE_ACCO
};

#define NETDISC_OPT_CCO_DET     BIT(0) //CCo detection
#define NETDISC_OPT_CCO_SEL     BIT(1) //Cco selection

#define NETDISC_ENABLE          1 
#define NETDISC_DISABLE         0 

#ifndef NID_LEN
#define NID_LEN                 7
#endif
#define HFID_LEN                64

#ifndef ENC_KEY_LEN
#define ENC_KEY_LEN             	( 16 )
#endif

#define ENC_IV_LEN              16

#ifndef MAC_ADDR_LEN
#define MAC_ADDR_LEN            6
#endif
#define VLAN_LEN                4
#define ETH_TYPE_HPGP           0x88e1

#define HPGP_TX_RETRY_MAX       3    // times

#define HPGP_HO_REASON_CCO_APPT    0x0   //user-appointed
#define HPGP_HO_REASON_CCO_SLCT    0x1   //CCO selection
#define HPGP_HO_REASON_CCO_LEAVE   0x2   //CCO leaving



//management message type
#define    MMTYPE_CC_CCO_APPOINT_REQ        0x0000
#define    MMTYPE_CC_CCO_APPOINT_CNF        0x0001
#define    MMTYPE_CC_CCO_APPOINT_IND        0x0002
#define    MMTYPE_CC_CCO_APPOINT_RSP        0x0003

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

#define	   HPGP_INTERFACE					0x01
#define    LRWPAN_INTERFACE					0x02


#ifdef SIMU
typedef struct txrxDesc
{
	u8     dtei;
	u8     stei;
	u8     frameType: 4;  //beacon and management
	u8     snid:      4;  //SNID
	u8     rsvd;   //reserved
} __PACKED__ sTxRxDesc, *psTxRxDesc;

typedef sTxRxDesc sTxDesc;
typedef sTxRxDesc sRxDesc;

#else
typedef struct rxDesc
{
	u8     dtei;
	u8     stei;
	u8     frameType: 4;  //beacon and management
	u8     snid:      4;  //SNID
	u8     crcInd;        //CRC indication
	u16    rssi;
} sRxDesc, *psRxDesc;

typedef struct txDesc
{
	u8     dtei;
	u8     stei;
	u8     frameType: 4;  //beacon and management
	u8     snid:      4;  //SNID
	u8     rsvd;   //reserved
} sTxDesc, *psTxDesc;

typedef struct txrxDesc
{
	union 
	{
		sTxDesc tx;
		sRxDesc rx;
	} desc;
    
} sTxRxDesc, *psTxRxDesc;

#endif




#ifndef __GNUC__
#pragma pack(1)
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
#ifndef __GNUC__
#pragma pack()
#endif

#endif


/** =========================================================
 *
 * Edit History
 *
 * $Source: /home/cvsrepo/hybrii/firmware/hpgp/src/hpgpdef.h,v $
 *
 * $Log: hpgpdef.h,v $
 * Revision 1.4  2011/09/14 06:39:13  yuanhua
 * Added key info in the SCB and stainfo data structures
 *
 * Revision 1.3  2011/09/14 05:52:36  yuanhua
 * Made Keil C251 compilation.
 *
 * Revision 1.2  2011/09/09 07:02:31  yuanhua
 * migrate the firmware code from the greenchip to the hybrii.
 *
 * Revision 1.3  2011/07/16 17:11:23  yuanhua
 * (1)Implemented SHOM and CHOM modules, including handover procedure, SCB resource updating for HO (2) Update SNAM and CNAM modules to support uer-appointed CCo handover (3) Made the SCB resources to support the TEI MAP for the STA mode and management of associated STA resources (e.g. TEI) (4) Modified SNSM and CNSM to perform all types of handover switch (CCo handover to the new STA, STA taking over the CCo, STA switching to the new CCo)
 *
 * Revision 1.2  2011/07/08 22:23:48  yuanhua
 * (1) Implemented CNSM, including its state machine, beacon transmission and process, discover beacon scheduling, auto CCo selection, discover list, handover countdown, etc. (2) Updated SNSM, including discover list processing, triggering a switch to the new CCo, etc. (3) Updated CNAM and SNAM, adding the connection state in the SNAM, switch to the new CCo, etc. (4) Other updates
 *
 * Revision 1.1  2011/06/23 23:52:43  yuanhua
 * move green.h green.c hpgpapi.h hpgpdef.h hpgpconf.h to src directory
 *
 * Revision 1.1  2011/05/06 19:07:48  kripa
 * Adding ctrl layer files to new source tree.
 *
 * Revision 1.2  2011/04/23 17:03:37  kripa
 * Added #pragma pack directive if compiler is not GCC. ( for VC++ and Keil )
 *
 * Revision 1.1  2011/04/08 21:40:41  yuanhua
 * Framework
 *
 *
 * ========================================================*/


