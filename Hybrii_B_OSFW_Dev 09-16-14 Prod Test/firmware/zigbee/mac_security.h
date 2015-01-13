/**
 * @file mac_security.h
 *
 * MAC security related defines.
 *
 * Copyright (c) 2011, Greenvity Communication All rights reserved.
 *
 */

#ifndef _MAC_SECURITY_H_
#define _MAC_SECURITY_H_

/* === Includes ============================================================= */

/* === Macros =============================================================== */

/**
 * The maximum number of entries supported in the macKeyTable.
 * This value is mplementation specific.
 */
#define MAC_MAX_KEY_TABLE_ENTRIES           (4)

/**
 * The maximum number of entries supported in the macDeviceTable.
 * each indicating a remote device with which this device securely communicates.
 * This value is mplementation specific.
 */
#define MAC_MAX_DEV_TABLE_ENTRIES           (16)

/**
 * The maximum number of entries supported in the macSecurityLevelTable.
 * each with information about the minimum security level expected depending on
 * incoming frame type and subtype.
 * This value is mplementation specific.
 */
#define MAC_MAX_SEC_LVL_TABLE_ENTRIES       (12)

/**
 * The maximum number of entries supported in the KeyIdLookupList
 * A list of KeyIdLookupDescriptor entries used to identify this KeyDescriptor.
 */
#define MAC_MAX_KEY_ID_LOOKUP_LIST_ENTRIES  (2)

/**
 * The maximum number of entries supported in the KeyDeviceList
 * A list of KeyDeviceDescriptor entries indicating which devices are currently
 * using this key, including their blacklist status.
 */
#define MAC_MAX_KEY_DEV_LIST_ENTRIES        (16)

/**
 * The maximum number of entries supported in the KeyUsageList
 * A list of KeyUsageDescriptor entries indicating which frame types
 * this key may be used with.
 */
#define MAC_MAX_KEY_USAGE_LIST_ENTRIES      (3)

/**
 * Default value for PIB macKeyTableEntries
 */
#define macKeyTableEntries_def              (0)

/**
 * Default value for PIB macDeviceTableEntries
 */
#define macDeviceTable_def                  (0)

/**
 * Default value for PIB macSecurityLevelTableEntries
 */
#define macSecurityLevelTable_def           (0)

/**
 * Default value for PIB macFrameCounter
 */
#define macFrameCounter_def                 (0x00000000)

/**
 * Default value for PIB macDefaultKeySource
 */
#define macDefaultKeySource_def             {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}

/**
 * Default value for KeyIdLookupListEntries
 */
#define KeyIdLookupListEntries_def          (0)

/**
 * Default value for KeyDeviceListEntries
 */
#define KeyDeviceListEntries_def            (0)

/**
 * Default value for KeyUsageListEntries
 */
#define KeyUsageListEntries_def             (0)

/**
 * Structure implementing a DeviceDescriptor.
 * See IEEE 802.15.4-2006 section 7.6.1 Table 93.
 */
typedef struct mac_device_desc_s
{
    /** The 16-bit PAN identifier of the device in this DeviceDescriptor. */
    uint16_t PANId;
    /**
     * The 16-bit short address of the device in this DeviceDescriptor.
     * A value of 0xfffe indicates that this device is using only its
     * extended address. A value of 0xffff indicates that this value is
     * unknown.
     */
    uint16_t ShortAddress;
    /**
     * The 64-bit IEEE extended address of the device in this DeviceDescriptor.
     * This element is also used in unsecuring operations on incoming frames.
     */
    uint64_t ExtAddress;
    /**
     * The incoming frame counter of the device in this DeviceDescriptor.
     * This value is used to ensure sequential freshness of frames.
     */
    uint32_t FrameCounter;
    /**
     * Indication of whether the device may override the minimum security
     * level settings defined in Table 92.
     */
    bool Exempt;
} mac_device_desc_t;



/**
 * Structure implementing a KeyUsageDescriptor.
 * See IEEE 802.15.4-2006 section 7.6.1 Table 90.
 */
typedef struct mac_key_usage_s
{
    /** See 7.2.1.1.1. */
    uint8_t Frametype;

    /** See Table 82. */
    uint8_t CommandFrameIdentifier;
} mac_key_usage_t;



/**
 * Structure implementing a KeyDeviceDescriptor.
 * See IEEE 802.15.4-2006 section 7.6.1 Table 91.
 */
typedef struct mac_key_device_desc_s
{
    /** Handle to the DeviceDescriptor corresponding to the device. */
    uint8_t DeviceDescriptorHandle;

    /**
     * Indication of whether the device indicated by DeviceDescriptorHandle
     * is uniquely associated with the KeyDescriptor, i.e., it is a link
     * key as opposed to a group key.
     */
    bool UniqueDevice;
    /**
     * Indication of whether the device indicated by DeviceDescriptorHandle
     * previously communicated with this key prior to the exhaustion of the
     * frame counter. If TRUE, this indicates that the device shall not use
     * this key further because it exhausted its use of the frame counter used
     * with this key.
     */
    bool BlackListed;
} mac_key_device_desc_t;



/**
 * Structure implementing a KeyIdLookupDescriptor.
 * See IEEE 802.15.4-2006 section 7.6.1 Table 94.
 */
typedef struct mac_key_id_lookup_desc_s
{
    /** Data used to identify the key. */
    uint8_t LookupData[9];
    /**
     * A value of 0x00 indicates a set of 5 octets, a value of 0x01 indicates
     * a set of 9 octets.
     */
    uint8_t LookupDataSize;
} mac_key_id_lookup_desc_t;



/**
 * Structure implementing the macKeyTable
 * according to IEEE 802.15.4-2006 section 7.6.1.
 */
typedef struct mac_key_table_s
{
    /** List of KeyIdLookupDescriptor entries. */
    mac_key_id_lookup_desc_t KeyIdLookupList[MAC_MAX_KEY_ID_LOOKUP_LIST_ENTRIES];
    /** The number of entries in KeyIdLookupList. */
    uint8_t KeyIdLookupListEntries;

    /** List of KeyDeviceDescriptor entries. */
    mac_key_device_desc_t KeyDeviceList[MAC_MAX_KEY_DEV_LIST_ENTRIES];
    /** The number of entries in KeyDeviceList. */
    uint8_t KeyDeviceListEntries;

    /** List of KeyUsageDescriptor entries. */
    mac_key_usage_t KeyUsageList[MAC_MAX_KEY_USAGE_LIST_ENTRIES];
    /** The number of entries in KeyUsageList. */
    uint8_t KeyUsageListEntries;

    /** Set of 16 octets - the actual value of the key. */
    uint8_t Key[16];
} mac_key_table_t;



/**
 * Structure implementing the macSecurityLevelTable
 * according to IEEE 802.15.4-2006 section 7.6.1.
 */
typedef struct mac_sec_lvl_table_s
{
    /** See 7.2.1.1.1. */
    uint8_t FrameType;

    /** See Table 82. */
    uint8_t CommandFrameIdentifier;

    /**
     * The minimal required/expected security level for incoming
     * MAC frames with the indicated frame type and, if present,
     * command frame type (see Table 95 in 7.6.2.2.1).
     */
    uint8_t SecurityMinimum;

    /**
     * Indication of whether originating devices for which the Exempt flag
     * is set may override the minimum security level indicated by the
     * SecurityMinimum element.
     * If TRUE, this indicates that for originating devices with Exempt status,
     * the incoming security level zero is acceptable, in addition to the
     * incoming security levels meeting the minimum expected security level
     * indicated by the SecurityMinimum element.
     */
    bool DeviceOverrideSecurityMinimum;
} mac_sec_lvl_table_t;



/**
 * Structure implementing the macDeviceTable
 * according to IEEE 802.15.4-2006 section 7.6.1.
 */
typedef struct mac_dev_table_s
{
    mac_device_desc_t DeviceDescriptor[1];
} mac_dev_table_t;



/* Structure implementing the MAC Security related PIB attributes */
typedef struct mac_sec_pib_s
{
    /**
     * Holds a table of KeyDescriptor entries, each containing keys and related
     * information required for secured communications.
     */
    mac_key_table_t KeyTable[MAC_MAX_KEY_TABLE_ENTRIES];
    /**
     * Holds the number of entries in macKeyTable.
     */
    uint8_t KeyTableEntries;

    /**
     * Holds a table of DeviceDescriptor entries, each indicating a remote
     * device with which this device securely communicates.
     */
    mac_dev_table_t DeviceTable[MAC_MAX_DEV_TABLE_ENTRIES];
    /**
     * Holds the number of entries in macDeviceTable.
     */
    uint8_t DeviceTableEntries;

    /**
     * Holds a table of SecurityLevelDescriptor entries, each with information
     * about the minimum security level expected depending on incoming frame
     * type and subtype.
     */
    mac_sec_lvl_table_t SecurityLevelTable[MAC_MAX_SEC_LVL_TABLE_ENTRIES];
    /**
     * Holds the number of entries in macSecurityLevelTable.
     */
    uint8_t SecurityLevelTableEntries;

    /**
     * Holds the outgoing frame counter for this device.
     */
    uint32_t FrameCounter;

    /**
     * Holds the originator of the default key used for key identifier mode 0x01.
     */
    uint8_t DefaultKeySource[8];
} mac_sec_pib_t;

/* === Externals ============================================================ */

extern mac_sec_pib_t mac_sec_pib;

/* === Prototypes =========================================================== */
extern retval_t mac_build_aux_sec_header(uint8_t **frame_ptr,
                                         security_info_t *sec_info_p,
                                         uint8_t *frame_len);
extern retval_t mac_secure(security_info_t *sec_info_p,
                           wpan_addr_spec_t *dst_addr_spec_p);
extern retval_t mac_unsecure(parse_t *mac_parse_data_p, uint8_t *mpdu_p,
                             uint8_t *mac_payload_p, uint8_t *payload_index_p);
extern uint8_t get_mic_length(uint8_t sec_level);
extern uint8_t sec_additional_len(security_info_t *sec_info_p);
#endif /* _MAC_SECURITY_H_ */
