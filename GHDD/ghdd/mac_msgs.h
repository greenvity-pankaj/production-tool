/**
 * mac_msgs.h
 *
 * Message structures for the MAC.
 *
 * Copyright (c) 2011, Greenviry Communcation All rights reserved.
 *
 */

/* Prevent double inclusion */
#ifndef _MAC_MSGS_H_
#define _MAC_MSGS_H_

/* === Includes ============================================================= */


/* === Macros =============================================================== */


/* === Types ================================================================ */
typedef enum mac_msg_code_e
{
    HAL_DATA_INDICATION                 = (0x00),

    MLME_ASSOCIATE_REQUEST              = (0x01),
    MLME_ASSOCIATE_RESPONSE             = (0x02),

    MCPS_DATA_REQUEST                   = (0x03),
    MCPS_PURGE_REQUEST                  = (0x04),

    MLME_DISASSOCIATE_REQUEST           = (0x05),
    MLME_SET_REQUEST                    = (0x06),
    MLME_ORPHAN_RESPONSE                = (0x07),
    MLME_GET_REQUEST                    = (0x08),
    MLME_RESET_REQUEST                  = (0x09),
    MLME_RX_ENABLE_REQUEST              = (0x0A),
    MLME_SCAN_REQUEST                   = (0x0B),
    MLME_START_REQUEST                  = (0x0D),
    MLME_POLL_REQUEST                   = (0x0E),
    MLME_SYNC_REQUEST                   = (0x0F),

    MCPS_DATA_CONFIRM                   = (0x10),
    MCPS_DATA_INDICATION                = (0x11),
    MCPS_PURGE_CONFIRM                  = (0x12),

    MLME_ASSOCIATE_INDICATION           = (0x13),
    MLME_ASSOCIATE_CONFIRM              = (0x14),
    MLME_DISASSOCIATE_INDICATION        = (0x15),
    MLME_DISASSOCIATE_CONFIRM           = (0x16),
    MLME_BEACON_NOTIFY_INDICATION       = (0x17),
    MLME_ORPHAN_INDICATION              = (0x1A),
    MLME_SCAN_CONFIRM                   = (0x1B),
    MLME_COMM_STATUS_INDICATION         = (0x1C),
    MLME_SYNC_LOSS_INDICATION           = (0x1D),
    MLME_GET_CONFIRM                    = (0x1E),
    MLME_SET_CONFIRM                    = (0x1F),
    MLME_RESET_CONFIRM                  = (0x20),
    MLME_RX_ENABLE_CONFIRM              = (0x21),
    MLME_START_CONFIRM                  = (0x22),
    MLME_POLL_CONFIRM                   = (0x23),
    MLME_LAST_MESSAGE
} mac_msg_code_t;

/**
 * PIB attribute value type
 */
typedef union
{
    /** PIB Attribute Bool */
    bool pib_value_bool;
    /** PIB Attribute 8-bit */
    uint8_t pib_value_8bit;
    /** PIB Attribute 16-bit */
    uint16_t pib_value_16bit;
    /** PIB Attribute 32-bit */
    uint32_t pib_value_32bit;
    /** PIB Attribute 64-bit */
    uint64_t pib_value_64bit;
} __PACKED__ pib_value_t;

/**
 * MAC Address type
 */
typedef union
{
    uint16_t short_address;
    uint64_t long_address;
} __PACKED__ address_field_t;

/**
 * Device address specification structure
 *
 */
/* 
 * Possible value for address mode
 */
#define WPAN_ADDRMODE_NONE              (0x00)
#define WPAN_ADDRMODE_SHORT             (0x02)
#define WPAN_ADDRMODE_LONG              (0x03)

typedef struct wpan_addr_spec_s
{
    uint8_t AddrMode;

   /**
    * The 16 bit PAN identifier.
    */
    uint16_t PANId;

   /**
    * 16 bit address.
    */
    address_field_t Addr;
} __PACKED__ wpan_addr_spec_t;

#define SEC_KEY_SRC_MAX      8

typedef struct security_info_s
{
    /**
     * The security level purportedly used by the received data frame.
     */
    uint8_t SecurityLevel;
    /**
     * The mode used to identify the key purportedly used by
     * the originator of the received frame.
     */
    uint8_t KeyIdMode;
    /**
     * The originator of the key to be used (see 7.6.2.4.1).
     * This parameter is ignored if the KeyIdMode
     * parameter is ignored or set to 0x00.
     * Set of 0, 4, or 8 octets
     */
    uint8_t KeySource[SEC_KEY_SRC_MAX];
    /**
     * The index of the key purportedly used by the originator
     * of the received frame.
     */
    uint8_t KeyIndex;
} __PACKED__ security_info_t;

/**
 * PAN descriptor information structure
 *
 */
typedef struct pandescriptor_s
{
    /**
     * Coordinator address specification in received beacon frame.
     */
    wpan_addr_spec_t CoordAddrSpec;

    /**
     * The current logical channel used by the network.
     */
    uint8_t     LogicalChannel;

    /**
     * The current channel page occupied by the network.
     */
    uint8_t     ChannelPage;

    /**
     * Superframe specification in received beacon frame.
     */
    uint16_t    SuperframeSpec;

    /**
     * Set to true if the beacon is from a PAN coordinator accepting GTS
     * requests.
     */
    bool        GTSPermit;

    /**
     * LQI at which the beacon was received. Lower values represent poorer link
     * quality.
     */
    uint8_t     LinkQuality;

    /**
     * Time at which the beacon frame was received, in symbol counts. 
     * This quantity shall be interpreted as only 24-bits, with the most
     * significant 8-bits entirely ignored.
     */
    uint32_t    TimeStamp;
} __PACKED__ pandescriptor_t;

typedef union scan_result_list_u
{
    uint8_t ed_value[1];
    pandescriptor_t PANDescriptor;
} __PACKED__ scan_result_list_t;

/* === MCPS-SAP messages ==================================================== */

#define MLME_CMD_CODE        0
/**
 * MCPS-DATA.request message structure.
 */
typedef struct mcps_data_req_s
{
    uint8_t cmdcode;
    /**
     * The source addressing mode for this primitive and subsequent MPDU. This
     * value can take one of the following values: 0 x 00 = no address
     * (addressingfields omitted). 0 x 01 = reserved. 0 x 02 = 16 bit short
     *  address. 0 x 03 = 64 bit extended address.
     */
    uint8_t SrcAddrMode;
    /**
     * The destination addressing mode for this primitive and subsequent MPDU.
     * This value can take one of the following values: 0 x 00 = no address
     * (addressing fields omitted). 0 x 01 = reserved. 0 x 02 = 16 bit short
     * address. 0 x 03 = 64 bit extended address.
     */
    uint8_t DstAddrMode;
    /**
     * The 16 bit PAN identifier of the entity to which the MSDU is being
     * transferred.
     */
    uint16_t DstPANId;
    /**
     * The individual device address of the entity to which the MSDU is
     * being transferred.
     */
    uint64_t DstAddr;
    /**
     * The handle associated with the MSDU to be transmitted by the MAC
     * sublayer entity.
     */
    uint8_t msduHandle;
    /**
     * The transmission options for this MSDU. These are a bitwise OR of one or
     * more of the following:
     * 0 x 01 = acknowledged transmission.
     * 0 x 04 = indirect transmission.
     */
    uint8_t TxOptions;

    /**
     * Security information
     */
    security_info_t Security;

    /**
     * The number of octets contained in the MSDU to be transmitted by the
     * MAC sublayer entity.
     */
    uint8_t msduLength;
    /**
     * The set of octets forming the MSDU to be transmitted by the MAC
     * sublayer entity.
     */
    uint8_t *msdu_p;
}__PACKED__ mcps_data_req_t;

/**
 * MCPS-DATA.confirm message structure.
 */
typedef struct mcps_data_conf_s
{
    uint8_t cmdcode;	
    /** The handle associated with the MSDU being confirmed. */
    uint8_t msduHandle;
    /** The status of the last MSDU transmission. */
    uint8_t status;
    /**
     * Optional. The time, in symbols, at which the data were transmitted
     * (see 7.5.4.1). The value of this parameter will be considered valid only
     * if the value of the status parameter is MAC_SUCCESS; if the status
     * parameter is not equal to MAC_SUCCESS, the value of the Timestamp
     * parameter shall not be used for any other purpose. The symbol boundary
     * is described by macSyncSymbolOffset (see Table 86 in 7.4.1).
     * This is a 24-bit value, and the precision of this value shall be a
     * minimum of 20 bits, with the lowest 4 bits being the least significant.
     */
    uint32_t Timestamp;
} __PACKED__ mcps_data_conf_t;

/**
 * MCPS-DATA.indication message structure.
 */
typedef struct mcps_data_ind_s
{
    uint8_t cmdcode;
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
    uint8_t *msdu_p;	

} __PACKED__ mcps_data_ind_t;

/**
 * MCPS-PURGE.request message structure.
 */
typedef struct mcps_purge_req_s
{
    uint8_t cmdcode;
    /** The handle of the MSDU to be purged from the transaction queue. */
    uint8_t msduHandle;
} __PACKED__ mcps_purge_req_t;

/**
 * MCPS-PURGE.confirm message structure.
 */
typedef struct mcps_purge_conf_s
{
    uint8_t cmdcode;
    /**
     * The handle of the MSDU requested to be purge from the transaction queue.
     */
    uint8_t msduHandle;
    /**
     * The status of the request to be purged an MSDU from the
     * transaction queue.
     */
    uint8_t status;
} __PACKED__ mcps_purge_conf_t;

/* === MLME-SAP messages ==================================================== */

/**
 * MLME-ASSOCIATE.request message structure.
 */
typedef struct mlme_associate_req_s
{
    uint8_t cmdcode;
    /** The logical channel on which to attempt association. */
    uint8_t LogicalChannel;
    /** The channel page on which to attempt association. */
    uint8_t ChannelPage;
    /**
     * The coordinator addressing mode for this primitive and subsequent MPDU.
     * This value can take one of the following values:
     * 2 = 16 bit short address. 3 = 64 bit extended address.
     */
    uint8_t CoordAddrMode;
    /** The identifier of the PAN with which to associate. */
    uint16_t CoordPANId;
    /** The address of the coordinator with which to associate.*/
    address_field_t CoordAddress;
    /** Specifies the operational capabilities of the associating device. */
    uint8_t CapabilityInformation;
    /**
     */
    security_info_t Security;
}__PACKED__ mlme_associate_req_t;

/**
 * MLME-ASSOCIATE.indication message structure.
 */
typedef struct mlme_associate_ind_s
{
    uint8_t cmdcode;
    /** The address of the device requesting association. */
    uint64_t DeviceAddress;
    /** The operational capabilities of the device requesting association. */
    uint8_t CapabilityInformation;
} __PACKED__ mlme_associate_ind_t;

/**
 * MLME-ASSOCIATE.response message structure.
 */
typedef struct mlme_associate_resp_s
{
    uint8_t cmdcode;
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
} __PACKED__ mlme_associate_resp_t;

/**
 * MLME-ASSOCIATE.confirm message structure.
 */
typedef struct mlme_associate_conf_s
{
    uint8_t cmdcode;
    /**
     * The short device address allocated by the coordinator on successful
     * association. This parameter will be equal to 0 x ffff if the association
     * attempt was unsuccessful.
     */
    uint16_t AssocShortAddress;
    /** The status of the association attempt. */
    uint8_t status;
} __PACKED__ mlme_associate_conf_t;

/**
 * MLME-DISASSOCIATE.request message structure.
 */
typedef struct mlme_disassociate_req_tag
{
    uint8_t cmdcode;
    /** The addressing mode of the device to which to send the
     * disassociation notification command.
     */
    uint8_t DeviceAddrMode;
    /** The PAN identifier of the device to which to send the disassociation
    notification command. */
    uint16_t DevicePANId;
    /** The address of the device to which to send the disassociation
    notification command. */
    address_field_t DeviceAddress;
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
} __PACKED__ mlme_disassociate_req_t;

/**
 * MLME-DISASSOCIATE.indication message structure.
 */
typedef struct mlme_disassociate_ind_s
{
    uint8_t cmdcode;
    /** The address of the device requesting disassociation. */
    uint64_t DeviceAddress;
    /** The reason for the disassociation (see 7.3.1.3.2). */
    uint8_t DisassociateReason;
} __PACKED__ mlme_disassociate_ind_t;

/**
 * MLME-DISASSOCIATE.confirm message structure.
 */
typedef struct mlme_disassociate_conf_s
{
    uint8_t cmdcode;
    /** The status of the disassociation attempt. */
    uint8_t status;
    /** The addressing mode of the device that has either requested
     *  disassociation or been instructed to disassociate by its coordinator.
     */
    uint8_t DeviceAddrMode;
    /** The PAN identifier of the device that has either requested disassociation
     *  or been instructed to disassociate by its coordinator.
     */
    uint16_t DevicePANId;
    /** The address of the device that has either requested disassociation or
     *  been instructed to disassociate by its coordinator.
     */
    address_field_t DeviceAddress;
} __PACKED__ mlme_disassociate_conf_t;

/**
 * MLME-BEACON-NOTIFY.indication message structure.
 */
typedef struct mlme_beacon_notify_ind_s
{
    uint8_t cmdcode;
    /** The beacon sequence number. */
    uint8_t BSN;
    /** The PANDescriptor for the received beacon. */
    pandescriptor_t PANDescriptor;
    /** The beacon pending address specification. */
    uint8_t PendAddrSpec;
    /**
     * The number of octets contained in the beacon payload of the beacon frame
     * received by the MAC sublayer.
     */
    uint8_t sduLength;	
} __PACKED__ mlme_beacon_notify_ind_t;

/**
 * MLME-ORPHAN.indication message structure.
 */
typedef struct mlme_orphan_ind_s
{
    uint8_t cmdcode;
    /** The address of the orphaned device. */
    uint64_t OrphanAddress;
} __PACKED__ mlme_orphan_ind_t;

/**
 * MLME-ORPHAN.response message structure.
 */
typedef struct mlme_orphan_resp_s
{
    uint8_t cmdcode;
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
} __PACKED__ mlme_orphan_resp_t;

/**
 * MLME-RESET.request message structure.
 */
typedef struct mlme_reset_req_s
{
    uint8_t cmdcode;
    /**
     * If TRUE, the MAC sublayer is reset and all MAC PIB attributes are set to
     * their default values. If FALSE, the MAC sublayer is reset but all MAC PIB
     * attributes retain their values prior to the generation of the
     * MLME-RESET.request primitive.
     */
    uint8_t SetDefaultPIB;
} __PACKED__ mlme_reset_req_t;

/**
 * MLME-RESET.confirm message structure.
 */
typedef struct mlme_reset_conf_s
{
    uint8_t cmdcode;
    /** The result of the reset operation. */
    uint8_t status;
} __PACKED__ mlme_reset_conf_t;

/**
 * MLME-GET.request message structure.
 */
typedef struct mlme_get_req_s
{
    uint8_t cmdcode;
    uint8_t        PIBAttribute;
    uint8_t        PIBAttributeIndex;
} __PACKED__ mlme_get_req_t;

/**
 * @brief This is the MLME-GET.confirm message structure.
 */
typedef struct mlme_get_conf_s
{
    uint8_t        cmdcode;
    uint8_t        status;
    uint8_t        PIBAttribute;
    uint8_t        PIBAttributeIndex;
    pib_value_t    PIBAttributeValue;
} __PACKED__ mlme_get_conf_t;

/**
 * MLME-SET.request message structure.
 */
typedef struct mlme_set_req_s
{
    uint8_t        cmdcode;
    uint8_t        PIBAttribute;
    uint8_t        PIBAttributeIndex;
    pib_value_t    PIBAttributeValue;
} __PACKED__ mlme_set_req_t;

/**
 * MLME-SET.confirm message structure.
 */
typedef struct mlme_set_conf_s
{
    uint8_t        cmdcode;
    uint8_t        status;
    uint8_t        PIBAttribute;
    uint8_t        PIBAttributeIndex;
} __PACKED__ mlme_set_conf_t;

/**
 * MLME-RX-ENABLE.request message structure.
 */
typedef struct mlme_rx_enable_req_tag
{
    uint8_t cmdcode;
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
}__PACKED__ mlme_rx_enable_req_t;

/**
 * MLME-RX-ENABLE.confirm message structure.
 */
typedef struct mlme_rx_enable_conf_s
{
    uint8_t cmdcode;
    /** The result of the receiver enable request. */
    uint8_t status;
} __PACKED__ mlme_rx_enable_conf_t;

/**
 * MLME-SCAN.request message structure.
 */
typedef struct mlme_scan_req_s
{
    uint8_t cmdcode;
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
} __PACKED__ mlme_scan_req_t;

/**
 * MLME-SCAN.confirm  message structure.
 */
typedef struct mlme_scan_conf_s
{
    uint8_t cmdcode;
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
} __PACKED__ mlme_scan_conf_t;

/**
 * MLME-COMM-STATUS.indication message structure.
 */
typedef struct mlme_comm_status_ind_s
{
    uint8_t cmdcode;
    /** The 16 bit PAN identifier of the device from which the frame was
    received or to which the frame was being sent. */
    uint16_t PANId;
    /**
     * The source addressing mode for this primitive. This value can take one
     * of the following values: 
     * 00 = no address (addressing fields omitted).
     * 01 = reserved.
     * 02 = 16 bit short address.
     * 03 = 64 bit extended address.
     */
    uint8_t SrcAddrMode;
    /**
     * The source addressing mode for this primitive. This value can take one
     * of the following values:
     * 00 = no address (addressing fields omitted).
     * 01 = reserved.
     * 02 = 16 bit short address.
     * 03 = 64 bit extended address.
     */
    uint64_t SrcAddr;
    /**
     * The destination addressing mode for this primitive. This value can take
     * one of the following values:
     * 00 = no address (addressing fields omitted).
     * 01 = reserved.
     * 02 = 16 bit short address.
     * 03 = 64 bit extended address.
     */
    uint8_t DstAddrMode;
    /**
     * The individual device address of the device for which the frame was
     * intended.
     */
    uint64_t DstAddr;
    /** The communications status. */
    uint8_t status;
} __PACKED__ mlme_comm_status_ind_t;

/**
 * MLME-START.request message structure.
 */
typedef struct mlme_start_req_s
{
    uint8_t cmdcode;
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
} __PACKED__ mlme_start_req_t;

/**
 * MLME-START.confirm message structure.
 */
typedef struct mlme_start_conf_s
{
    uint8_t cmdcode;
    /** 
     * The result of the attempt to start using an updated superframe
     * configuration.
     */
    uint8_t status;
} __PACKED__ mlme_start_conf_t;

/**
 * MLME-SYNC.request message structure.
 */
typedef struct mlme_sync_req_s
{
    uint8_t cmdcode;
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
} __PACKED__ mlme_sync_req_t;

/**
 * MLME-SYNC-LOSS.indication message structure.
 */
typedef struct mlme_sync_loss_ind_tag
{
    uint8_t cmdcode;
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
} __PACKED__ mlme_sync_loss_ind_t;

/**
 * MLME-POLL.request message structure.
 */
typedef struct mlme_poll_req_s
{
    uint8_t cmdcode;
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
    address_field_t CoordAddress;
    /**
     * Security information - Security Level, Key ID Mode, Key Source,
     * Key Index
     */
    security_info_t Security;
}__PACKED__ mlme_poll_req_t;

/**
 * MLME-POLL.confirm message structure.
 */
typedef struct mlme_poll_conf_s
{
    uint8_t cmdcode;
    /** The status of the data request. */
    uint8_t status;
} __PACKED__ mlme_poll_conf_t;

/* === Externals ============================================================ */


/* === Prototypes =========================================================== */

#endif /* _MAC_MSGS_H_ */

