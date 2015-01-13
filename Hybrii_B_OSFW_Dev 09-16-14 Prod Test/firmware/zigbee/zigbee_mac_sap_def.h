/*
* $Id: zigbee_mac_sap_def.h,v 1.1 2014/06/09 13:26:39 kiran Exp $
*
* $Source: /home/cvsrepo/Hybrii_B_OSFW_Dev/firmware/zigbee/zigbee_mac_sap_def.h,v $
*
* Description : Zigbee MAC SAP constant Header File.
*
* Copyright (c) 2012 Greenvity Communications, Inc.
* All rights reserved.
*
* Purpose :
*    
*
*/

//#define IEEE802_15_4_MAC_ID 		0x01
//#define HPGP_MAC_ID		 			0x02
#if 0
#define CONTROL_FRM_ID				0x00
#define DATA_FRM_ID					0x01
#define MGMT_FRM_ID					0x02
#define EVENT_FRM_ID            	0x03
#endif
#define MAX_ADDRESS_LIST			0x07

#define ZB_MCPS_DATA_REQUEST			(0xAA)
#define ZB_MCPS_DATA_CONFIRM			(0xAC)
#define ZB_MCPS_DATA_INDICATION			(0xAD)

#define ZB_MCPS_PURGE_REQUEST       	(0xAE)
#define ZB_MCPS_PURGE_CONFIRM			(0xAF)


#define ZB_MLME_ASSOCIATE_REQUEST		(0xB0)
#define ZB_MLME_ASSOCIATE_INDICATION	(0xB1)
#define ZB_MLME_ASSOCIATE_RES			(0xB2)
#define ZB_MLME_ASSOCIATE_CONFIRM		(0xB3)

#define ZB_MLME_DISASSOCIATE_REQ		(0xB4)
#define ZB_MLME_DISASSOCIATE_INDICATION	(0xB5)
#define ZB_MLME_DISASSOCIATE_CONFIRM	(0xB6)

#define ZB_MLME_BEACON_NOTIFY_INDICATION (0xB7)

#define ZB_MLME_GET_REQUEST				(0xB8)
#define ZB_MLME_GET_CONFIRM				(0xB9)

#define ZB_MLME_GTS_REQUEST				(0XBA)
#define ZB_MLME_GTS_INDICATION			(0XBB)
#define ZB_MLME_GTS_CONFIRM				(0XBC)

#define ZB_MLME_ORPHAN_INDICATION		(0xBD)
#define ZB_MLME_ORPHAN_RES				(0xBE)

#define ZB_MLME_RESET_REQUEST			(0xBF)
#define ZB_MLME_RESET_CONFIRM			(0xC0)

#define ZB_MLME_RX_ENABLE_REQUEST		(0xC1)
#define ZB_MLME_RX_ENABLE_CONFIRM		(0xC2)

#define ZB_MLME_SCAN_REQUEST			(0xC3)
#define ZB_MLME_SCAN_CONFIRM			(0xC4)

#define ZB_MLME_COMM_STATUS_INDICATION  (0xC5)

#define ZB_MLME_SET_REQUEST				(0xC6)
#define ZB_MLME_SET_CONFIRM				(0xC7)

#define ZB_MLME_START_REQUEST			(0xC8)
#define ZB_MLME_START_CONFIRM			(0xC9)

#define ZB_MLME_SYNC_REQUEST			(0xCA)

#define ZB_MLME_SYNC_LOSS_INDICATION 	(0xCB)

#define ZB_MLME_POLL_REQUEST			(0xCC)
#define ZB_MLME_POLL_CONFIRM			(0xCD)


#define ZIGBEE_PAYLOAD_SIZE			102

#if 0
typedef struct _hostHdr_
{
    u8 protocol  : 2;
    u8 type      : 2;
    u8 rsvd1     : 4;
	u8 rsvd;        
    u16 length;
} __PACKED__ hostHdr_t;
#endif

typedef struct {
	u8 command;
	u8 action;
	u8 result;		
}__PACKED__ hostCmdHdr_t;

typedef struct host_mcps_data_request_s	//MCPS-DATA.Request
{
	uint8_t command;
	uint8_t action;
	uint8_t result;
	uint8_t SrcAddrMode;
	wpan_addr_spec_t DstAddrSpec;
	uint8_t msduLength;
	uint8_t msduHandle;
	uint8_t TxOptions;
	security_info_t sec;
	uint8_t msdu[1];// Actual payload is 102 but data is in CP's so base address is required. Array of one element is used to provide base address in cp
} __PACKED__ host_mcps_data_request_t;

typedef struct host_mcps_data_confirm_s	// MCPS-DATA.confirm
{
	u8 command;
	u8 action;
	u8 result;
	uint8_t msduHandle;
	uint8_t status;
	uint32_t Timestamp;
} __PACKED__ host_mcps_data_confirm_t;

typedef struct host_mcps_data_ind_s
{
	uint8_t command;
	uint8_t action;
	uint8_t result;
    /**
     * The source addressing mode for this primitive corresponding to the
     * received MPDU. This value can take one of the following values:
     * 00 = no address (addressing fields omitted). 
     * 01 = reserved.
     * 02 = 16 bit short address.
     * 03 = 64 bit extended address.
     */
    uint8_t SrcAddrMode;
    /**
     * The 16 bit PAN identifier of the entity from which the MSDU was received.
     */
    uint16_t SrcPANId;
    /**
     * The individual device address of the entity from which the
     * MSDU was received.
     */
    uint64_t SrcAddr;
    /**
     * The destination addressing mode for this primitive corresponding to the
     * received MPDU. This value can take one of the following values:
     * 00 = no address (addressing fields omitted).
     * 01 = reserved.
     * 02 = 16 bit short device address. 
     * 03 = 64 bit extended device address.
     */
    uint8_t DstAddrMode;
    /**
     * The 16 bit PAN identifier of the entity to which the MSDU is
     * being transferred.
     */
    uint16_t DstPANId;
    /**
     * The individual device address of the entity to which the MSDU is
     * being transferred.
     */
    uint64_t DstAddr;
    /**
     * LQI value measured during reception of the MPDU. Lower values
     * represent lower LQI (see 6.7.8).
     */
    uint8_t mpduLinkQuality;
    /**
     * The DSN of the received data frame.
     */
    uint8_t DSN;
    /**
     * Optional. The time, in symbols, at which the data were received
     * (see 7.5.4.1).
     * The symbol boundary is described by macSyncSymbolOffset (see Table 86
     * in 7.4.1).
     * This is a 24-bit value, and the precision of this value shall be a
     * minimum of 20 bits, with the lowest 4 bits being the least significant.
     */
    uint32_t Timestamp;
    /**
     * The number of octets contained in the MSDU being indicated by the
     * MAC sublayer entity.
     */
    uint8_t msduLength;
  
    /**
     * Security information
     */
    security_info_t Security;

	/**
     * The set of octets forming the MSDU being indicated by the
     * MAC sublayer entity.
     */
    uint8_t msdu[ZIGBEE_PAYLOAD_SIZE];
	
	}__PACKED__ host_mcps_data_ind_t;

/**
 * MCPS-PURGE.request message structure.
 */
typedef struct host_mcps_purge_req_s
{
	uint8_t command;
	uint8_t action;
	uint8_t result;

    /** The handle of the MSDU to be purged from the transaction queue. */
    uint8_t msduHandle;
} __PACKED__ host_mcps_purge_req_t;

/**
 * MCPS-PURGE.confirm message structure.
 */
typedef struct host_mcps_purge_conf_s
{
	uint8_t command;
	uint8_t action;
	uint8_t result;

    /**
     * The handle of the MSDU requested to be purge from the transaction queue.
     */
    uint8_t msduHandle;
    /**
     * The status of the request to be purged an MSDU from the
     * transaction queue.
     */
    uint8_t status;
} __PACKED__ host_mcps_purge_conf_t;



typedef struct host_mlme_associate_req_s
{
	uint8_t command;
	uint8_t action;
	uint8_t result;

    /** The logical channel on which to attempt association. */
    uint8_t LogicalChannel;
    /** The channel page on which to attempt association. */
    uint8_t ChannelPage;
    /**
     * The coordinator addressing mode for this primitive and subsequent MPDU.
     * This value can take one of the following values:
     * 2 = 16 bit short address. 3 = 64 bit extended address.
     */
    wpan_addr_spec_t CoordAddress;
    /** Specifies the operational capabilities of the associating device. */
    uint8_t CapabilityInformation;
    /**
     */
    security_info_t Security;
}__PACKED__ host_mlme_associate_req_t;


typedef struct host_mlme_associate_cnf_s
{
	uint8_t command;
	uint8_t action;
	uint8_t result;

	/**
	 * The short device address allocated by the coordinator on successful
	 * association. This parameter will be equal to 0 x ffff if the association
	 * attempt was unsuccessful.
	 */
	uint16_t AssocShortAddress;
	/** The status of the association attempt. */
	uint8_t status;

} __PACKED__ host_mlme_associate_cnf_t;

/**
 * MLME-ASSOCIATE.indication message structure.
 */
typedef struct host_mlme_associate_ind_s
{
	uint8_t command;
	uint8_t action;
	uint8_t result;

    /** The address of the device requesting association. */
    uint64_t DeviceAddress;
    /** The operational capabilities of the device requesting association. */
    uint8_t CapabilityInformation;
} __PACKED__ host_mlme_associate_ind_t;


typedef struct host_mlme_disassoc_req_s
{
	uint8_t command;
	uint8_t action;
	uint8_t result;
		
	/** The addressing mode of the device to which to send the
	 * disassociation notification command.
	 */
	/** The PAN identifier of the device to which to send the disassociation
	notification command. */
	/** The address of the device to which to send the disassociation
	notification command. */
	wpan_addr_spec_t DeviceAddress;
	/** The reason for the disassociation (see 7.3.1.3.2). */
	uint8_t DisassociateReason;
	/**
	 * TRUE if the disassociation notification command is to be sent
	 * indirectly.
	 */
	uint8_t TxIndirect;
	/**
	 */
	security_info_t Security;


}__PACKED__ host_mlme_disassoc_req_t;

typedef struct host_mlme_disassoc_cnf_s
{
	uint8_t command;
	uint8_t action;
	uint8_t result;

	/** The status of the disassociation attempt. */
	uint8_t status;

	wpan_addr_spec_t DeviceAddress;

}__PACKED__ host_mlme_disassoc_cnf_t;


typedef struct host_mlme_disassoc_ind_s
{
	uint8_t command;
	uint8_t action;
	uint8_t result;

	/** The address of the device requesting disassociation. */
	uint64_t DeviceAddress;
	/** The reason for the disassociation (see 7.3.1.3.2). */
	uint8_t DisassociateReason;


}__PACKED__ host_mlme_disassoc_ind_t;

/**
 * MLME-ASSOCIATE.response message structure.
 */
typedef struct host_mlme_associate_resp_s
{
	uint8_t command;
	uint8_t action;
	uint8_t result;

    /** The address of the device requesting association. */
    uint64_t DeviceAddress;
    /**
     * The short device address allocated by the coordinator on successful
     * association. This parameter is set to 0xffff if the association was
     * unsuccessful.
     */
    uint16_t AssocShortAddress;
    /** The status of the association attempt. */
    uint8_t status;
    /**
     */
    security_info_t Security;
}__PACKED__ host_mlme_associate_resp_t;


typedef struct host_mlme_beacon_notify_ind_s
{
	uint8_t command;
	uint8_t action;
	uint8_t result;

    /** The beacon sequence number. */
    uint8_t BSN;
    /** The PANDescriptor for the received beacon. */
    pandescriptor_t PANDescriptor;
    /** The beacon pending address specification. */
	union
    	{
    		struct
    			{
					uint8_t shortaddr:3 ;
					uint8_t resv:1;
					uint8_t extaddr:3;
					uint8_t resv1:1;
			} s;
   			 uint8_t PendAddrSpec;
    	} uPendAddrSpec;
    /**
     * List of devices for which the beacon source has data. */

uint8_t	AddrList[56];
/*The maximum number of addresses pending shall be limited to seven and may comprise both short and
extended addresses. All pending short addresses shall appear first in the list followed by any extended
addresses. If the coordinator is able to store more than seven transactions, it shall indicate them in its beacon
on a first-come-first-served basis, ensuring that the beacon frame contains at most seven addresses.*/	
uint8_t	sduLength;
uint8_t	sdu[aMaxBeaconPayloadLength];////////////////////////define length of array 
//The set of octets comprising the beacon
//payload to be transferred from the MAC
//sublayer entity to the next higher layer


}__PACKED__ host_mlme_beacon_notify_ind_t;

typedef struct host_mlme_orphan_ind_s
{
	uint8_t command;
	uint8_t action;
	uint8_t result;

	/** The address of the orphaned device. */
	uint64_t OrphanAddress;
}__PACKED__ host_mlme_orphan_ind_t;


typedef struct host_mlme_orphan_resp_s
{
	uint8_t command;
	uint8_t action;
	uint8_t result;

	/** The address of the orphaned device. */
	uint64_t OrphanAddress;
	/**
	* The short address allocated to the orphaned device if it is associated
	* with this coordinator. The special short address 0 x fffe indicates that
	* no short address was allocated, and the device will use its 64 bit
	* extended address in all communications. If the device was not associated
	*  with thiscoordinator, this field will contain the value 0 x ffff and
	* be ignored on receipt.
	*/
	uint16_t ShortAddress;
	/**
	* TRUE if the orphaned device is associated with this coordinator or
	* FALSE otherwise.
	*/
	uint8_t AssociatedMember;
	/**
	* Security information - Security Level, Key ID Mode, Key Source,
	* Key Index
	*/
	security_info_t Security;
}__PACKED__ host_mlme_orphan_resp_t;

typedef struct host_mlme_reset_req_s
{
	uint8_t command;
	uint8_t action;
	uint8_t result;

    /**
     * If TRUE, the MAC sublayer is reset and all MAC PIB attributes are set to
     * their default values. If FALSE, the MAC sublayer is reset but all MAC PIB
     * attributes retain their values prior to the generation of the
     * MLME-RESET.request primitive.
     */
    uint8_t SetDefaultPIB;
} __PACKED__ host_mlme_reset_req_t;

/**
 * MLME-RESET.confirm message structure.
 */
typedef struct host_mlme_reset_conf_s
{
	uint8_t command;
	uint8_t action;
	uint8_t result;

    /** The result of the reset operation. */
    uint8_t status;
} __PACKED__ host_mlme_reset_conf_t;

/**
 * MLME-GET.request message structure.
 */
typedef struct host_mlme_get_req_s
{
	uint8_t        command;
	uint8_t        action;
	uint8_t        result;

    uint8_t        PIBAttribute;
    uint8_t        PIBAttributeIndex;
} __PACKED__ host_mlme_get_req_t;

/**
 * @brief This is the MLME-GET.confirm message structure.
 */
typedef struct host_mlme_get_conf_s
{
	uint8_t 	   command;
	uint8_t 	   action;
	uint8_t 	   result;

    uint8_t        status;
    uint8_t        PIBAttribute;
    uint8_t        PIBAttributeIndex;
    pib_value_t    PIBAttributeValue;
} __PACKED__ host_mlme_get_conf_t;


/**
 * MLME-SET.request message structure.
 */
typedef struct host_mlme_set_req_s
{
	uint8_t        command;
	uint8_t        action;
	uint8_t        result;

    uint8_t        PIBAttribute;
    uint8_t        PIBAttributeIndex;
    pib_value_t    PIBAttributeValue;
} __PACKED__ host_mlme_set_req_t;

/**
 * Host MLME-SET.confirm message structure.
 */
typedef struct host_mlme_set_conf_s
{
	uint8_t        command;
	uint8_t        action;
	uint8_t        result;

    uint8_t        status;
    uint8_t        PIBAttribute;
    uint8_t        PIBAttributeIndex;
} __PACKED__ host_mlme_set_conf_t;


/**
 * MLME-RX-ENABLE.request message structure.
 */
typedef struct host_mlme_rx_enable_req_s
{
	uint8_t command;
	uint8_t action;
	uint8_t result;

    /**
     * TRUE if the receiver enable can be deferred until during the next
     * superframe if the requested time has already passed. FALSE if the
     * receiver enable is only to be attempted in the current superframe.
     * This parameter is ignored for nonbeacon-enabled PANs.
     */
    uint8_t DeferPermit;
    /**
     * The number of symbols from the start of the superframe before the
     * receiver is to be enabled. The precision of this value is a minimum of
     * 20 bits, with the lowest 4 bits being the least significant. This
     * parameter is ignored for nonbeacon-enabled PANs.
     */
    uint32_t RxOnTime;
    /** The number of symbols for which the receiver is to be enabled. */
    uint32_t RxOnDuration;
} host_mlme_rx_enable_req_t;

/**
 * MLME-RX-ENABLE.confirm message structure.
 */
typedef struct host_mlme_rx_enable_conf_s
{
	uint8_t command;
	uint8_t action;
	uint8_t result;

    /** The result of the receiver enable request. */
    uint8_t status;
} __PACKED__ host_mlme_rx_enable_conf_t;

/**
 * MLME-SCAN.request message structure.
 */
typedef struct host_mlme_scan_req_s
{
	uint8_t command;
	uint8_t action;
	uint8_t result;

    /**
     * Indicates the type of scan performed: 0 x 00 = ED scan (FFD only).
     * 0 x 01 = active scan (FFD only). 0 x 02 = passive scan. 0 x 03 = orphan
     * scan.
     */
    uint8_t ScanType;
    /**
     * The 5 MSBs (b27, ... , b31) are reserved. The 27 LSBs (b0, b1, ... b26)
     * indicate which channels are to be scanned (1 = scan, 0 = do not scan) for
     * each of the 27 valid channels (see 6.1.2).
     */
    uint32_t ScanChannels;
    /**
     * A value used to calculate the length of time to spend scanning each
     * channel for ED, active, and passive scans. This parameter is ignored for
     * orphan scans. The time spent scanning each channel is
     * [aBaseSuperframeDuration * (2n + 1)] symbols, where n is the value of the
     * ScanDuration parameter.
     */
    uint8_t ScanDuration;
    /**
     * The channel page on which to perform the scan.
     */
    uint8_t ChannelPage;
    /**
     * Security information - Security Level, Key ID Mode, Key Source,
     * Key Index
     */
    security_info_t Security;
} __PACKED__ host_mlme_scan_req_t;

typedef struct host_mlme_comm_status_ind_s
{
	uint8_t command;
	uint8_t action;
	uint8_t result;

   
    wpan_addr_spec_t SrcAddr;
  
    wpan_addr_spec_t DstAddr;
    /** The communications status. */
    uint8_t status;
}__PACKED__ host_mlme_comm_status_ind_t;

typedef struct host_mlme_start_req_s

{
	uint8_t command;
	uint8_t action;
	uint8_t result;

    /** The PAN identifier to be used by the device. */
    uint16_t PANId;
    /**
     * The logical channel on which to start using the new superframe
     * configuration.
     */
    uint8_t LogicalChannel;
    /**
     * The channel page on which to begin using the new superframe
     * configuration.
     */
    uint8_t ChannelPage;

    /**
     * The time at which to begin transmitting beacons. If this
     * parameter is equal to 0x000000, beacon transmissions will begin
     * immediately. Otherwise, the specified time is relative to the 
     * received beacon of the coordinator with which the device synchronizes.
     * This parameter is ignored if either the BeaconOrder parameter has
     * a value of 15 or the PANCoordinator parameter is TRUE.
     * The time is specified in symbols and is rounded to a backoff slot
     * boundary.
     * This is a 24-bit value, and the precision of this value shall be a
     * minimum of 20 bits, with the lowest 4 bits being the least significant
     */
    uint32_t StartTime;
    /**
     * How often the beacon is to be transmitted. The beacon order, BO, and the
     * beacon interval, BI, are related as follows: for 0 d BO d 14, BI =
     * BaseSuperframeDuration * 2^BO symbols. If BO = 15, the coordinator will
     * not transmit a beacon, and the SuperframeOrder parameter value is
     * ignored.
     */
    uint8_t BeaconOrder;
    /**
     * The length of the active portion of the superframe, including the beacon
     * frame. The superframe order, SO, and the superframe duration, SD, are
     * related as follows: for 0 d SO d BO d 14, SD = aBaseSuperframeDuration *
     * 2^SO symbols. If SO = 15, the superframe will not be active after the
     * beacon.
     */
    uint8_t SuperframeOrder;
    /**
     * If this value is TRUE, the device will become the PAN coordinator of a
     * new PAN. If this value is FALSE, the device will begin transmitting
     * beacons on the PAN with which it is associated.
     */
    uint8_t PANCoordinator;
    /**
     * If this value is TRUE, the receiver of the beaconing device is disabled
     * macBattLifeExtPeriods full backoff periods after the interframe spacing
     * (IFS) period of the beacon frame. If this value is FALSE, the receiver of
     * the beaconing device remains enabled for the entire CAP.
     */
    uint8_t BatteryLifeExtension;
    /**
     * TRUE if a coordinator realignment command is to be transmitted prior to
     * changing the superframe configuration or FALSE otherwise.
     */
    uint8_t CoordRealignment;
    /**
     * The security info to be used for coordinator realignment command
     * frames (see Table 95 in 7.6.2.2.1).
     */
    security_info_t CoordRealignmentSecurity;
    /**
     * The security info to be used for beacon packet 
     * frames (see Table 95 in 7.6.2.2.1).
     */
    security_info_t BeaconSecurity;
} __PACKED__ host_mlme_start_req_t;

typedef struct host_mlme_start_conf_s
{
	uint8_t command;
	uint8_t action;
	uint8_t result;

    /** 
     * The result of the attempt to start using an updated superframe
     * configuration.
     */
    uint8_t status;
} __PACKED__ host_mlme_start_conf_t;

typedef struct host_mlme_sync_req_s
{
	uint8_t command;
	uint8_t action;
	uint8_t result;

    /** The logical channel on which to attempt coordinator synchronization. */
    uint8_t LogicalChannel;
    /** The channel page on which to attempt coordinator synchronization. */
    uint8_t ChannelPage;
    /**
     * TRUE if the MLME is to synchronize with the next beacon and attempt to
     * track all future beacons. FALSE if the MLME is to synchronize with only
     * the next beacon.
     */
    uint8_t TrackBeacon;
}__PACKED__ host_mlme_sync_req_t;

typedef struct host_mlme_sync_loss_ind_tag
{
	uint8_t command;
	uint8_t action;
	uint8_t result;

    /** The reason that synchronization was lost. */
    uint8_t LossReason;
    /** The PAN identifier with which the device lost synchronization or
     *  to which it was realigned.
     */
    uint16_t PANId;
    /** The logical channel on which the device lost synchronization or
     *  to which it was realigned.
     */
    uint8_t LogicalChannel;
    /** The channel page on which the device lost synchronization or
     * to which it was realigned.
     */
    uint8_t ChannelPage;
}__PACKED__ host_mlme_synhostc_loss_ind_t;


typedef struct host_mlme_poll_req_s

{
	uint8_t command;
	uint8_t action;
	uint8_t result;
    /**
     * The addressing mode of the coordinator to which the poll is intended.
     * This parameter can take one of the following values:
     * 2 = 16 bit short address.
     * 3 = 64 bit extended address.
     */
    uint8_t CoordAddrMode;
    /** The PAN identifier of the coordinator to which the poll is intended. */
    uint16_t CoordPANId; 
    /** The address of the coordinator to which the poll is intended. */
    wpan_addr_spec_t CoordAddress;
    /**
     * Security information - Security Level, Key ID Mode, Key Source,
     * Key Index
     */
    security_info_t Security;
}__PACKED__  host_mlme_poll_req_t;

typedef struct host_mlme_poll_conf_s
{
	uint8_t command;
	uint8_t action;
	uint8_t result;

    /** The status of the data request. */
    uint8_t status;
} __PACKED__ host_mlme_poll_conf_t;

typedef struct host_mlme_scan_conf_s
{

	uint8_t command;
	uint8_t action;
	uint8_t result;

    /** The status of the scan request. */
    uint8_t status;
    /**
     * ScanType
     * 00 = ED scan (FFD only)
     *.01 = active scan (FFD only)
     * 02 = passive scan. 
     * 03 = orphan scan.
     */
    uint8_t ScanType;
    /** The channel page on which the scan was performed. */
    uint8_t ChannelPage;
    /**
     * Indicates which channels given in the request were not scanned (1 = not
     * scanned, 0 = scanned or not requested). This parameter is only valid for
     * passive or active scans.
     */
    uint32_t UnscannedChannels;
    /**
     * The number of elements returned in the appropriate result
     * lists. This value is 0 for the result of an orphan scan.
     */
    uint8_t ResultListSize;
    /**
     * ResultListSize - The number of elements returned in the appropriate
     * result lists. This value is 0 for the result of an orphan scan.
     * EnergyDetectList - The number of elements returned in the appropriate
     * result lists. This value is 0 for the result of an orphan scan.
     * PANDescriptorList - The list of PAN descriptors, one for each beacon
     * found during an active or passive scan. This parameter is null for ED
     * and orphan scans.
     */
    scan_result_list_t scan_result_list[1];
} __PACKED__ host_mlme_scan_conf_t;




