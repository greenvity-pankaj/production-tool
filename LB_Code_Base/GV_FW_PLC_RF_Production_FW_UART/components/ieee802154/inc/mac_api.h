 /* ========================================================
 *
 * @file: mac_api.h
 * 
 * @brief: This file contains all api's needed to communicate 
 *         with the IEEE 802.15.4 MAC firmware
 * 
 *  Copyright (C) 2010-2015, Greenvity Communications, Inc.
 *  All Rights Reserved
 *  
 * =========================================================*/
 
#ifndef _MAC_API_H_
#define _MAC_API_H_

/****************************************************************************** 
  *	Macros
  ******************************************************************************/


/**
 * Capacity of queue between MAC and Next Higher Layer
 */
#define MAC_NHLE_QUEUE_CAPACITY         255

/* The following symbolic constants are just for MAC API */

/**
 * Value for the address mode, where no address is given.
 * (see @ref wpan_addr_spec_t::AddrMode)
 * @ingroup apiConst
 */
#define WPAN_ADDRMODE_NONE              (0x00)

/**
 * Value for the address mode, where a 16 bit short address is given.
 * (see @ref wpan_addr_spec_t::AddrMode)
 * @ingroup apiConst
 */
#define WPAN_ADDRMODE_SHORT             (0x02)

/**
 * Value for the address mode, where a 64 bit long address is given.
 * (see @ref wpan_addr_spec_t::AddrMode)
 * @ingroup apiConst
 */
#define WPAN_ADDRMODE_LONG              (0x03)

/**
 * Flag value for capability information field
 * (see @ref wpan_mlme_associate_req()).
 * The alternate PAN coordinator subfield shall be set if the device is
 * capable of becoming a PAN coordinator. Otherwise,
 * the alternate PAN coordinator subfield shall be set to 0.
 * @ingroup apiConst
 */
#define WPAN_CAP_ALTPANCOORD            (0x01)

/**
 * Flag value for capability information field
 * (see @ref wpan_mlme_associate_req()).
 * The device type subfield shall be set if the device is an FFD. Otherwise,
 * the device type subfield shall be set to 0 to indicate an RFD.
 * @ingroup apiConst
 */
#define WPAN_CAP_FFD                    (0x02)

/**
 * Flag value for capability information field
 * (see @ref wpan_mlme_associate_req()).
 * The power source subfield shall be set if the device is receiving power
 * from the alternating current mains. Otherwise, the power source subfield
 * shall be set to 0.
 * @ingroup apiConst
 */
#define WPAN_CAP_PWRSOURCE              (0x04)

/**
 * Flag value for capability information field
 * (see @ref wpan_mlme_associate_req()).
 * The receiver on when idle subfield shall be set if the device does not
 * disable its receiver to conserve power during idle periods. Otherwise, the
 * receiver on when idle subfield shall be set to 0.
 * @ingroup apiConst
 */
#define WPAN_CAP_RXONWHENIDLE           (0x08)

/**
 * Flag value for capability information field
 * (see @ref wpan_mlme_associate_req()).
 * The allocate address subfield shall be set if the device wishes the
 * coordinator to allocate a short address as a result of the association
 * procedure. If this subfield is set to 0, the special short address of
 * 0xfffe shall be allocated to the device and returned through the
 * association response command. In this case, the device shall communicate
 * on the PAN using only its 64 bit extended address.
 * @ingroup apiConst
 */
#define WPAN_CAP_ALLOCADDRESS           (0x80)

/**
 * Symbolic constant for disassociate reason - initiated by parent
 * (see @ref wpan_mlme_disassociate_req())
 * @ingroup apiConst
 */
#define WPAN_DISASSOC_BYPARENT          (0x01)

/**
 * Symbolic constant for disassociate reason - initiated by child
 * (see @ref wpan_mlme_disassociate_req())
 * @ingroup apiConst
 */
#define WPAN_DISASSOC_BYCHILD           (0x02)

/**
 * Marco to extract size of short address list in PAN descriptor
 */
#define WPAN_NUM_SHORT_ADDR_PENDING(x)      ((x) & 0x7)

/**
 * Macro to extract size of extended address list in PAN descriptor
 */
#define WPAN_NUM_EXTENDED_ADDR_PENDING(x)   (((x) >> 4) & 0x7)

/****************************************************************************** 
  *	Function Prototypes
  ******************************************************************************/

/*
 * These functions have to be called from the application
 * in order to initiate an action in the communication
 * stack at the MAC level
 */
bool mac_api_mcps_data_req(uint8_t SrcAddrMode,
                           wpan_addr_spec_t *DstAddrSpec_p,
                           uint8_t msduLength,
                           uint8_t *msdu_p,
                           uint8_t msduHandle,
                           uint8_t TxOptions,
                           security_info_t *sec_p);
bool mac_api_mcps_purge_req(const uint8_t msduHandle);
bool mac_api_mlme_associate_req(uint8_t LogicalChannel,
                                uint8_t ChannelPage,
                                wpan_addr_spec_t *CoordAddrSpec_p,
                                uint8_t CapabilityInformation,
                                security_info_t *sec_p);
bool mac_api_mlme_associate_resp(uint64_t DeviceAddress,
                                 uint16_t AssocShortAddress,
                                 uint8_t status,
                                 security_info_t *sec_p);
bool mac_api_mlme_disassociate_req(wpan_addr_spec_t *DeviceAddrSpec_p,
                                   uint8_t DisassociateReason,
                                   bool TxIndirect,
                                   security_info_t *sec_p);
bool mac_api_mlme_get_req(uint8_t PIBAttribute, uint8_t PIBAttributeIndex);
bool mac_api_mlme_orphan_resp(uint64_t OrphanAddress,
                          uint16_t ShortAddress,
                          bool AssociatedMember,
                          security_info_t *sec_p);
bool mac_api_mlme_poll_req(wpan_addr_spec_t *CoordAddrSpec_p,
                           security_info_t *sec_p);
bool mac_api_mlme_reset_req(bool SetDefaultPib);
bool mac_api_mlme_set_req(uint8_t PIBAttribute,
                          uint8_t PIBAttributeIndex,
                          void *PIBAttributeValue_p);
bool mac_api_mlme_rx_enable_req(bool DeferPermit,
                                uint32_t RxOnTime,
                                uint32_t RxOnDuration);
bool mac_api_mlme_scan_req(uint8_t ScanType,
                           uint32_t ScanChannels,
                           uint8_t ScanDuration,
                           uint8_t ChannelPage,
                           security_info_t *sec_p);
bool mac_api_mlme_start_req(uint16_t PANId,
                            uint8_t LogicalChannel,
                            uint8_t ChannelPage,
                            uint32_t StartTime,
                            uint8_t BeaconOrder,
                            uint8_t SuperframeOrder,
                            bool PANCoordinator,
                            bool BatteryLifeExtension,
                            bool CoordRealignment,
                            security_info_t *CoordRealignmentSecurity_p,
                            security_info_t *BeaconSecurity_p);
bool mac_api_mlme_sync_req(uint8_t LogicalChannel,
                           uint8_t ChannelPage,
                           bool TrackBeacon);
#endif /*_MAC_API_H_*/
