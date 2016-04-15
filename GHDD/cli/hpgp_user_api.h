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
#ifndef _HPGP_USER_API_H_
#define _HPGP_USER_API_H_

#define PACKED __attribute__((packed))

/**
 * PIB attribute value type
 */
#define MAX_REQ_LEN 				( 256 )
#define MAX_RSP_LEN 				( 256 )
#define MAX_PASSWD_LEN				( 64 )

#define aMaxPHYPacketSize               (127)

#define aMaxBeaconOverhead              (75)

#define macIeeeAddress                  (0xF0)

#define MAC_MAX_KEY_ID_LOOKUP_LIST_ENTRIES  (2)

#define phySymbolsPerOctet              (0x07)
#define macBeaconPayload                (0x45)
#define macAckWaitDuration              (0x40)
#define macMinSIFSPeriod                (0x5F)
#define macKeyTable                     (0x71)
#define macPANCoordShortAddress         (0x7E)

#define MAX_PHY_PIB_ATTRIBUTE_ID            (phySymbolsPerOctet)
#define MIN_MAC_PIB_ATTRIBUTE_ID            (macAckWaitDuration)
#define MAX_MAC_PIB_ATTRIBUTE_ID            (macMinSIFSPeriod)
#define MIN_MAC_SEC_PIB_ATTRIBUTE_ID        (macKeyTable)
#define MAX_MAC_SEC_PIB_ATTRIBUTE_ID        (macPANCoordShortAddress)
#define MIN_PRIVATE_PIB_ATTRIBUTE_ID            (macIeeeAddress)


#if 0
/* Size constants for PHY PIB attributes */
static uint8_t phy_pib_size[] =
{
    sizeof(uint8_t),                // 0x00: phyCurrentChannel
    sizeof(uint32_t),               // 0x01: phyChannelsSupported
    sizeof(uint8_t),                // 0x02: phyTransmitPower
    sizeof(uint8_t),                // 0x03: phyCCAMode
    sizeof(uint8_t),                // 0x04: phyCurrentPage
    sizeof(uint16_t),               // 0x05: phyMaxFrameDuration
    sizeof(uint8_t),                // 0x06: phySHRDuration
    sizeof(uint8_t)                 // 0x07: phySymbolsPerOctet
};
#endif

/* Update this one the arry phy_pib_size is updated. */

#if 0
/* Size constants for MAC PIB attributes */
static uint8_t mac_pib_size[] =
{
    sizeof(uint8_t),                // 0x40: macAckWaitDuration
    sizeof(uint8_t),                // 0x41: macAssociationPermit
    sizeof(uint8_t),                // 0x42: macAutoRequest
    sizeof(uint8_t),                // 0x43: macBattLifeExt
    sizeof(uint8_t),                // 0x44: macBattLifeExtPeriods
    sizeof(uint8_t),                // 0x45: macBeaconPayload
    sizeof(uint8_t),                // 0x46: macBeaconPayloadLength
    sizeof(uint8_t),                // 0x47: macBeaconOrder
    sizeof(uint32_t),               // 0x48: macBeaconTxTime
    sizeof(uint8_t),                // 0x49: macBSN
    sizeof(uint64_t),               // 0x4A: macCoordExtendedAddress
    sizeof(uint16_t),               // 0x4B: macCoordShortAddress
    sizeof(uint8_t),                // 0x4C: macDSN
    sizeof(uint8_t),                // 0x4D: macGTSPermit
    sizeof(uint8_t),                // 0x4E: macMaxCSMAbackoffs
    sizeof(uint8_t),                // 0x4F: macMinBE
    sizeof(uint16_t),               // 0x50: macPANId
    sizeof(uint8_t),                // 0x51: macPromiscuousMode
    sizeof(uint8_t),                // 0x52: macRxOnWhenIdle
    sizeof(uint16_t),               // 0x53: macShortAddress
    sizeof(uint8_t),                // 0x54: macSuperframeOrder
    sizeof(uint16_t),               // 0x55: macTransactionPersistenceTime
    sizeof(uint8_t),                // 0x56: macAssociatedPANCoord
    sizeof(uint8_t),                // 0x57: macMaxBE
    sizeof(uint16_t),               // 0x58: macMaxFrameTotalWaitTime
    sizeof(uint8_t),                // 0x59: macMaxFrameRetries
    sizeof(uint16_t),               // 0x5A: macResponseWaitTime
    sizeof(uint16_t),               // 0x5B: macSyncSymbolOffset
    sizeof(uint8_t),                // 0x5C: macTimestampSupported
    sizeof(uint8_t),                // 0x5D: macSecurityEnabled
    sizeof(uint8_t),                // 0x5E: macMinLIFSPeriod
    sizeof(uint8_t)                 // 0x5F: macMinSIFSPeriod
};
#endif 

#if 0
/* Size constants for MAC Security PIB attributes */
static uint8_t mac_sec_pib_size[] =
{
    sizeof(mac_key_table_t),        // 0x71: macKeyTable
    sizeof(uint8_t),                // 0x72: macKeyTableEntries
    /* Since the structure is not packed, we need to use the hardcode value */
    17,                             // 0x73: macDeviceTable
    sizeof(uint8_t),                // 0x74: macDeviceTableEntries
    sizeof(mac_sec_lvl_table_t),    // 0x75: macSecurityLevelTable
    sizeof(uint8_t),                // 0x76: macSecurityLevelTableEntries
    sizeof(uint32_t),               // 0x77: macFrameCounter
    sizeof(uint8_t),                // 0x78: macAutoRequestSecurityLevel
    sizeof(uint8_t),                // 0x79: macAutoRequestKeyIdMode
    sizeof(uint8_t),                // 0x7A: macAutoRequestKeySource
    sizeof(uint8_t),                // 0x7B: macAutoRequestKeyIndex
    (8 * sizeof(uint8_t)),          // 0x7C: macDefaultKeySource - 8 octets
    sizeof(uint16_t),               // 0x7D: macPANCoordExtendedAddress
    sizeof(uint16_t)                // 0x7E: macPANCoordShortAddress
};
#endif

#if 0
static uint8_t private_pib_size[] =
{
    sizeof(uint64_t)                // 0xF0: macIeeeAddress
};
#endif

/**
 * The maximum size, in octets, of a beacon payload.
 */
#define aMaxBeaconPayloadLength         (aMaxPHYPacketSize - aMaxBeaconOverhead)

// host events
#define HOST_EVENT_FW_READY		( 0x01 )


#define ACTION_GET	( 0 )
#define ACTION_SET	( 1 )

void usage(char * exename);
u8 send_packet(u8 *ptr_packet, u8 packetlen);
u8 read_response(u8 *ptr_packet);
void parse_response(u8 command, u8 *ptr_packet);
void send_command(u8 command, u8 *ptr_packet, u32 pktlen);
u32 swapbytes(u32 val);

#endif //_HPGP_USER_API_H_
