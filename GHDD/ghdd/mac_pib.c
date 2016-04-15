/**
 * @file mac_pib.c
 *
 * Implements the MAC PIB attribute handling.
 *
 * $Id: mac_pib.c,v 1.1 2014/01/10 17:27:11 yiming Exp $
 *
 * Copyright (c) 2011, Greenvity Communication All rights reserved.
 *
 */


/* === Includes ============================================================ */

#include <string.h>
#include "papdef.h"
//#include "return_val.h"
//#include "mac_msgs.h"
#include "mac_const.h"

/* === Macros ============================================================== */

/* === Globals ============================================================= */

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

/* Update this one the arry phy_pib_size is updated. */
#define MAX_PHY_PIB_ATTRIBUTE_ID            (phySymbolsPerOctet)

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

#define MIN_MAC_PIB_ATTRIBUTE_ID            (macAckWaitDuration)
#define MAX_MAC_PIB_ATTRIBUTE_ID            (macMinSIFSPeriod)


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

#define MIN_MAC_SEC_PIB_ATTRIBUTE_ID        (macKeyTable)
#define MAX_MAC_SEC_PIB_ATTRIBUTE_ID        (macPANCoordShortAddress)


/* Size constants for Private PIB attributes */
static uint8_t private_pib_size[] =
{
    sizeof(uint64_t)                // 0xF0: macIeeeAddress
};

/* Update this one the arry private_pib_size is updated. */
#define MIN_PRIVATE_PIB_ATTRIBUTE_ID            (macIeeeAddress)

/* === Prototypes ========================================================== */

/* === Implementation ====================================================== */

/**
 *
 * This function re-calculates the MAC PIB attribute macMaxFrameTotalWaitTime
 * whenever one of the following PIB attributes change:
 * macMinBE
 * macMaxBE
 * macMaxCSMABackoffs
 * phyMaxFrameDuration
 *
 * See IEEE 802.15.4-2006 equation (14) in section 7.4.2.
 */
static void mac_pib_recalc_macMaxFrameTotalWaitTime (void)
{
    uint8_t m, k;

    m = (uint8_t)MIN((hal_pib_MaxBE - hal_pib_MinBE), hal_pib_MaxCSMABackoffs);

    mac_pib_macMaxFrameTotalWaitTime =
                (hal_pib_MaxCSMABackoffs - m) * ((1 << hal_pib_MaxBE) - 1);

    /* Calculate sum of equation (14). */
    for (k = 0; k < m; k++) {
        mac_pib_macMaxFrameTotalWaitTime += 1 << (hal_pib_MinBE + k);
    }

    /* Calculate the rest. */
    mac_pib_macMaxFrameTotalWaitTime *= aUnitBackoffPeriod;
    mac_pib_macMaxFrameTotalWaitTime += MAX_FRAME_DURATION;
}

/**
 *
 * This function handles an MLME-GET.request.
 * The MLME-GET.request primitive requests information about a
 * given PIB attribute.
 *
 * buff_p - Pointer to the GET request structure
 */
void mlme_get_request (buffer_t *buff_p)
{
    /* Use the mlme get request buffer for mlme get confirmation */
    mlme_get_conf_t *mgc = (mlme_get_conf_t *)BMM_BUFFER_POINTER(buff_p);
    uint8_t attribute_index = ((mlme_get_req_t *)mgc)->PIBAttributeIndex;
    pib_value_t *attribute_value = &mgc->PIBAttributeValue;
    uint8_t status = MAC_SUCCESS;

    /* Do actual PIB attribute reading */

    switch (((mlme_get_req_t *)mgc)->PIBAttribute) {
    case macAssociatedPANCoord:
        attribute_value->pib_value_8bit = mac_pib_macAssociatedPANCoord;
        break;

    case macMaxBE:
        attribute_value->pib_value_8bit = hal_pib_MaxBE;
        break;

    case macMaxFrameTotalWaitTime:
        memcpy(attribute_value, &mac_pib_macMaxFrameTotalWaitTime,
               sizeof(uint16_t));
        break;

    case macMaxFrameRetries:
        attribute_value->pib_value_8bit = hal_pib_MaxFrameRetries;
        break;

    case macResponseWaitTime:
        memcpy(attribute_value, &mac_pib_macResponseWaitTime, sizeof(uint16_t));
        break;

    case macSecurityEnabled:
        attribute_value->pib_value_8bit = mac_pib_macSecurityEnabled;
        break;

    case phyCurrentPage:
        attribute_value->pib_value_8bit = hal_pib_CurrentPage;
        break;

    case phyMaxFrameDuration:
        memcpy(attribute_value, &hal_pib_MaxFrameDuration, sizeof(uint16_t));
        break;

    case phySHRDuration:
        attribute_value->pib_value_8bit = hal_pib_SHRDuration;
        break;

    case phySymbolsPerOctet:
        attribute_value->pib_value_8bit = hal_pib_SymbolsPerOctet;
        break;

    case macAutoRequest:
        attribute_value->pib_value_8bit = mac_pib_macAutoRequest;
        break;

    case macBattLifeExt:
        attribute_value->pib_value_8bit = hal_pib_BattLifeExt;
        break;

    case macBattLifeExtPeriods:
        attribute_value->pib_value_8bit = mac_pib_macBattLifeExtPeriods;
        break;

    case macBeaconTxTime:
        memcpy(attribute_value, &hal_pib_BeaconTxTime, sizeof(uint32_t));
        break;

    case macBeaconOrder:
        attribute_value->pib_value_8bit = hal_pib_BeaconOrder;
        break;

    case macSuperframeOrder:
        attribute_value->pib_value_8bit = hal_pib_SuperFrameOrder;
        break;

    case macAssociationPermit:
        attribute_value->pib_value_8bit = mac_pib_macAssociationPermit;
        break;

    case macBeaconPayload:
        memcpy(attribute_value, mac_beacon_payload,
               mac_pib_macBeaconPayloadLength);
        break;

    case macBeaconPayloadLength:
        attribute_value->pib_value_8bit = mac_pib_macBeaconPayloadLength;
        break;

    case macBSN:
        attribute_value->pib_value_8bit = mac_pib_macBSN;
        break;

    case macTransactionPersistenceTime:
        memcpy(attribute_value, &mac_pib_macTransactionPersistenceTime,
               sizeof(uint16_t));
        break;

    case macPromiscuousMode:
        attribute_value->pib_value_8bit = hal_pib_PromiscuousMode;
        break;

    case macCoordExtendedAddress:
        memcpy(attribute_value, &mac_pib_macCoordExtendedAddress,
               sizeof(uint64_t));
        break;

    case macCoordShortAddress:
        memcpy(attribute_value, &mac_pib_macCoordShortAddress,
               sizeof(uint16_t));
        break;

    case macDSN:
        attribute_value->pib_value_8bit = mac_pib_macDSN;
        break;

    case macMaxCSMABackoffs:
        attribute_value->pib_value_8bit = hal_pib_MaxCSMABackoffs;
        break;

    case macMinBE:
        attribute_value->pib_value_8bit = hal_pib_MinBE;
        break;

    case macPANId:
        memcpy(attribute_value, &hal_pib_PANId, sizeof(uint16_t));
        break;

    case macRxOnWhenIdle:
        attribute_value->pib_value_8bit = mac_pib_macRxOnWhenIdle;
        break;

    case macShortAddress:
        memcpy(attribute_value, &hal_pib_ShortAddress, sizeof(uint16_t));
        break;

    case macIeeeAddress:
        memcpy(attribute_value, &hal_pib_IeeeAddress, sizeof(uint64_t));
        break;

    case phyCurrentChannel:
        attribute_value->pib_value_8bit = hal_pib_CurrentChannel;
        break;

    case phyChannelsSupported:
        memcpy(attribute_value, &hal_pib_SupportedChannels, sizeof(uint32_t));
        break;

    case phyTransmitPower:
        attribute_value->pib_value_8bit = hal_pib_TransmitPower;
        break;

    case phyCCAMode:
        attribute_value->pib_value_8bit = hal_pib_CCAMode;
        break;

    case macKeyTable:
        if (attribute_index >= mac_sec_pib.KeyTableEntries) {
            status = MAC_INVALID_INDEX;
        } else {
            memcpy(attribute_value,
                   &mac_sec_pib.KeyTable[attribute_index],
                   sizeof(mac_key_table_t));
        }
        break;

    case macKeyTableEntries:
        attribute_value->pib_value_8bit = mac_sec_pib.KeyTableEntries;
        break;

    case macDeviceTable:
        if (attribute_index >= mac_sec_pib.DeviceTableEntries) {
            status = MAC_INVALID_INDEX;
        } else {
            /*
             * Since the members of the mac_dev_table_t structure do
             * contain padding bytes,
             * each member needs to be filled in separately.
             */
            uint8_t *attribute_temp_ptr = (uint8_t *)attribute_value;
            /*
             * Since the members of the mac_dev_table_t structure do
             * contain padding bytes, each member needs to be filled 
             * in separately.
             */
            /* PAN-Id */
            ADDR_COPY_DST_SRC_16(*(uint16_t *)attribute_temp_ptr,
                                 mac_sec_pib.DeviceTable[attribute_index].\
                                 DeviceDescriptor[0].PANId);
            attribute_temp_ptr += sizeof(uint16_t);

            /* Short Address */
            ADDR_COPY_DST_SRC_16(*(uint16_t *)attribute_temp_ptr, 
                                 mac_sec_pib.DeviceTable[attribute_index].\
                                 DeviceDescriptor[0].ShortAddress);
            attribute_temp_ptr += sizeof(uint16_t);

            /* Extended Address */
            ADDR_COPY_DST_SRC_64(*(uint64_t *)attribute_temp_ptr,
                                 mac_sec_pib.DeviceTable[attribute_index].\
                                 DeviceDescriptor[0].ExtAddress);
            attribute_temp_ptr += sizeof(uint64_t);

            /* Extended Address */
            memcpy(attribute_temp_ptr,
                   &mac_sec_pib.DeviceTable[attribute_index].\
                   DeviceDescriptor[0].FrameCounter, sizeof(uint32_t));
            attribute_temp_ptr += sizeof(uint32_t);

            /* Exempt */
            *attribute_temp_ptr =
                   mac_sec_pib.DeviceTable[attribute_index].\
                   DeviceDescriptor[0].Exempt;
        }
        break;

    case macDeviceTableEntries:
       attribute_value->pib_value_8bit = mac_sec_pib.DeviceTableEntries;
       break;

    case macSecurityLevelTable:
        if (attribute_index >= mac_sec_pib.SecurityLevelTableEntries) {
            status = MAC_INVALID_INDEX;
        } else {
            memcpy(attribute_value,
                   &mac_sec_pib.SecurityLevelTable[attribute_index],
                   sizeof(mac_sec_lvl_table_t));
        }
        break;

    case macSecurityLevelTableEntries:
        attribute_value->pib_value_8bit = mac_sec_pib.SecurityLevelTableEntries;
        break;

    case macFrameCounter:
        memcpy(attribute_value, &mac_sec_pib.FrameCounter, sizeof(uint32_t));
        break;

    case macDefaultKeySource:
        /* Key Source length is 8 octets. */
        memcpy(attribute_value, mac_sec_pib.DefaultKeySource, 8);
        break;

    default:
        status = MAC_UNSUPPORTED_ATTRIBUTE;
        break;
    }

    mgc->PIBAttribute = ((mlme_get_req_t *)mgc)->PIBAttribute;
    mgc->PIBAttributeIndex = attribute_index;
    mgc->cmdcode      = MLME_GET_CONFIRM;
    mgc->status       = status;

    mlme_get_conf(buff_p);
}

retval_t mlme_set (uint8_t attribute, uint8_t attribute_index,
                   pib_value_t *attribute_value, bool set_trx_to_sleep)
{
    static bool trx_pib_wakeup;
    retval_t status = MAC_SUCCESS;

    switch (attribute) {
    case macAssociatedPANCoord:
        mac_pib_macAssociatedPANCoord = attribute_value->pib_value_8bit;
        break;

    case macMaxFrameTotalWaitTime:
        mac_pib_macMaxFrameTotalWaitTime = attribute_value->pib_value_16bit;
        break;

    case macResponseWaitTime:
        mac_pib_macResponseWaitTime = attribute_value->pib_value_16bit;
        break;

    case macAutoRequest:
        mac_pib_macAutoRequest = attribute_value->pib_value_8bit;
        break;

    case macBattLifeExtPeriods:
        mac_pib_macBattLifeExtPeriods = attribute_value->pib_value_8bit;
        break;

    case macAssociationPermit:
        mac_pib_macAssociationPermit = attribute_value->pib_value_8bit;
        break;

    case macBeaconPayload:
        memcpy(mac_beacon_payload, attribute_value,
               mac_pib_macBeaconPayloadLength);
        break;

    case macBeaconPayloadLength:
        /*
         * If the application sits directly  on top of the MAC,
         * this is also checked in mac_api.c.
         */
        if (attribute_value->pib_value_8bit > aMaxBeaconPayloadLength) {
            status = MAC_INVALID_PARAMETER;
            break;
        }
        mac_pib_macBeaconPayloadLength = attribute_value->pib_value_8bit;
        break;

    case macBSN:
        mac_pib_macBSN = attribute_value->pib_value_8bit;
        break;

    case macTransactionPersistenceTime:
        mac_pib_macTransactionPersistenceTime = 
                                        attribute_value->pib_value_16bit;
        break;

    case macCoordExtendedAddress:
        mac_pib_macCoordExtendedAddress = attribute_value->pib_value_64bit;
        break;

    case macCoordShortAddress:
        mac_pib_macCoordShortAddress = attribute_value->pib_value_16bit;
        break;

    case macDSN:
        mac_pib_macDSN = attribute_value->pib_value_8bit;
        break;

    case macRxOnWhenIdle:
        mac_pib_macRxOnWhenIdle = attribute_value->pib_value_8bit;
        /* Check whether radio state needs to change now, */
        if (mac_pib_macRxOnWhenIdle) {
            /* Check whether the radio needs to be woken up. */
            mac_trx_wakeup();
            /* Set transceiver to rx mode */
            mac_hal_hw_control(PHY_RX_ON);
        } else {
            /* Check whether the radio needs to be put to sleep. */
            mac_trx_sleep();
        }
        break;

    case macBattLifeExt:
    case macBeaconOrder:
    case macMaxCSMABackoffs:
    case macMaxBE:
    case macMaxFrameRetries:
    case macMinBE:
    case macPANId:
    case macPromiscuousMode:
    case macShortAddress:
    case macSuperframeOrder:
    case macIeeeAddress:
    case phyCurrentChannel:
    case phyCurrentPage:
    case phyTransmitPower:
    case phyCCAMode:
        /* Now only HAL PIB attributes are handled anymore. */
        status = mac_hal_pib_set(attribute, attribute_value);

        if (status == MAC_TRX_ASLEEP) {
            /*
             * Wake up the transceiver and repeat the attempt
             * to set the TAL PIB attribute.
             */
            //tal_trx_wakeup();
            status = mac_hal_pib_set(attribute, attribute_value);
            if (status == MAC_SUCCESS) {
                /*
                 * Set flag indicating that the trx has been woken up
                 * during PIB setting.
                 */
                trx_pib_wakeup = true;
            }
        }

    /*
     * In any case that the PIB setting was successful (no matter
     * whether the trx had to be woken up or not), the PIB attribute
     * recalculation needs to be done.
     */
    if (status == MAC_SUCCESS) {
        mac_pib_recalc_macMaxFrameTotalWaitTime();
    }
    break;

    case macAckWaitDuration:
        status = MAC_READ_ONLY;
        break;

    case macSecurityEnabled:
        mac_pib_macSecurityEnabled = attribute_value->pib_value_8bit;
        break;

    case macKeyTable:
        if (attribute_index >= mac_sec_pib.KeyTableEntries) {
            status = MAC_INVALID_INDEX;
        } else {
            memcpy(&mac_sec_pib.KeyTable[attribute_index], attribute_value,
                   sizeof(mac_key_table_t));
        }
        break;

    case macKeyTableEntries:
        if (attribute_value->pib_value_8bit > MAC_MAX_KEY_TABLE_ENTRIES) {
            status = MAC_INVALID_PARAMETER;
        } else {
            mac_sec_pib.KeyTableEntries = attribute_value->pib_value_8bit;
        }
        break;

    case macDeviceTable:
        if (attribute_index >= mac_sec_pib.DeviceTableEntries) {
            status = MAC_INVALID_INDEX;
        } else {
            uint8_t *attribute_temp_ptr = (uint8_t *)attribute_value;
            /* PAN-Id */
            ADDR_COPY_DST_SRC_16(mac_sec_pib.DeviceTable[attribute_index].\
                                 DeviceDescriptor[0].PANId,
                                 *(uint16_t *)attribute_temp_ptr);
            attribute_temp_ptr += sizeof(uint16_t);

            /* Short Address */
            ADDR_COPY_DST_SRC_16(mac_sec_pib.DeviceTable[attribute_index].\
                                 DeviceDescriptor[0].ShortAddress,
                                 *(uint16_t *)attribute_temp_ptr);
            attribute_temp_ptr += sizeof(uint16_t);

            /* Extended Address */
            ADDR_COPY_DST_SRC_64(mac_sec_pib.DeviceTable[attribute_index].\
                                 DeviceDescriptor[0].ExtAddress,
                                 *(uint64_t *)attribute_temp_ptr);
            attribute_temp_ptr += sizeof(uint64_t);

            /* Extended Address */
            memcpy(&mac_sec_pib.DeviceTable[attribute_index].\
                   DeviceDescriptor[0].FrameCounter,
                   attribute_temp_ptr, sizeof(uint32_t));
            attribute_temp_ptr += sizeof(uint32_t);

            /* Exempt */
            mac_sec_pib.DeviceTable[attribute_index].\
                   DeviceDescriptor[0].Exempt = *attribute_temp_ptr;
        }
        break;

    case macDeviceTableEntries:
        if (attribute_value->pib_value_8bit > MAC_MAX_DEV_TABLE_ENTRIES) {
            status = MAC_INVALID_PARAMETER;
        } else {
            mac_sec_pib.DeviceTableEntries = attribute_value->pib_value_8bit;
        }
        break;

    case macSecurityLevelTable:
        if (attribute_index >= mac_sec_pib.SecurityLevelTableEntries) {
            status = MAC_INVALID_INDEX;
        } else {
            memcpy(&mac_sec_pib.SecurityLevelTable[attribute_index],
                   attribute_value, sizeof(mac_sec_lvl_table_t));
        }
        break;

    case macSecurityLevelTableEntries:
        if (attribute_value->pib_value_8bit > 
            MAC_MAX_SEC_LVL_TABLE_ENTRIES) {
            status = MAC_INVALID_PARAMETER;
        } else {
            mac_sec_pib.SecurityLevelTableEntries = 
                                        attribute_value->pib_value_8bit;
        }
        break;

    case macFrameCounter:
        mac_sec_pib.FrameCounter = attribute_value->pib_value_32bit;
        break;

    case macDefaultKeySource:
        /* Key Source length is 8 octets. */
        memcpy(mac_sec_pib.DefaultKeySource, attribute_value, 8);
        break;

    default:
        status = MAC_UNSUPPORTED_ATTRIBUTE;
        break;
    }

    /*
     * In case the transceiver shall be forced back to sleep and
     * has been woken up, it is put back to sleep again.
     */
    if (set_trx_to_sleep && trx_pib_wakeup && !mac_pib_macRxOnWhenIdle) {
        //tal_trx_sleep(SLEEP_MODE_1);  FIXME
        trx_pib_wakeup = false;
    }

    return status;
}

/**
 *
 * This function handles the MLME-SET.request. The MLME-SET.request primitive
 * attempts to write the given value to the indicated PIB attribute.
 *
 * buff_p - Pointer to the SET request structure
 */
void mlme_set_request (buffer_t *buff_p)
{
    mlme_set_req_t  *msr = (mlme_set_req_t *)BMM_BUFFER_POINTER(buff_p);
    pib_value_t *attribute_value = &msr->PIBAttributeValue;
    retval_t status = MAC_SUCCESS;
    mlme_set_conf_t *msc;
    uint8_t attribute_index = msr->PIBAttributeIndex;

    /*
     * Call internal PIB attribute handling function. Always force
     * the trx back to sleep when using request primitives via the
     * MLME queue.
     */
    status = mlme_set(msr->PIBAttribute, msr->PIBAttributeIndex,
                      attribute_value, true);
    msc = (mlme_set_conf_t *)msr;
    msc->PIBAttribute = msr->PIBAttribute;
    msc->PIBAttributeIndex = attribute_index;
    msc->cmdcode      = MLME_SET_CONFIRM;
    msc->status       = status;

    /* Append the mlme set confirmation message to the MAC-NHLE queue */
    mlme_set_conf(buff_p);
}

/**
 * @brief Wakes-up the radio and sets the corresponding TAL PIB attribute
 *
 * @param attribute PIB attribute to be set
 * @param attribute_value Attribute value to be set
 *
 * @return Status of the attempt to set the TAL PIB attribute
 */
retval_t set_hal_pib_internal (uint8_t attribute, pib_value_t *attribute_value)
{
    retval_t status;

    if (RADIO_SLEEPING == mac_radio_state) {
        /* Wake up the radio */
        mac_trx_wakeup();

        status = mac_hal_pib_set(attribute, attribute_value);

        /* Set radio to sleep if allowed */
        mac_trx_sleep();

    } else {
        status = mac_hal_pib_set(attribute, attribute_value);
    }

    return status;
}

/**
 *
 * attribute_id - PIB attribute
 *
 * return Size (number of bytes) of the PIB attribute
 */
uint8_t mac_get_pib_attribute_size (uint8_t pib_attribute_id)
{
    /*
     * Since the current length of the beacon payload is not a contant, but
     * a variable, it cannot be stored in a Flash table. Therefore we need
     * to handle this PIB attribute special.
     */
    if (macBeaconPayload == pib_attribute_id) {
       return (mac_pib_macBeaconPayloadLength);
    }

    if (MAX_PHY_PIB_ATTRIBUTE_ID >= pib_attribute_id) {
       return (phy_pib_size[pib_attribute_id]);
    }

    if (MIN_MAC_PIB_ATTRIBUTE_ID <= pib_attribute_id && 
        MAX_MAC_PIB_ATTRIBUTE_ID >= pib_attribute_id) {
       return(mac_pib_size[pib_attribute_id - MIN_MAC_PIB_ATTRIBUTE_ID]);
    }

    if (MIN_MAC_SEC_PIB_ATTRIBUTE_ID <= pib_attribute_id &&
        MAX_MAC_SEC_PIB_ATTRIBUTE_ID >= pib_attribute_id) {
       return(mac_sec_pib_size[pib_attribute_id -
                               MIN_MAC_SEC_PIB_ATTRIBUTE_ID]);
    }

    if (MIN_PRIVATE_PIB_ATTRIBUTE_ID <= pib_attribute_id) {
        return(private_pib_size[pib_attribute_id -
                                MIN_PRIVATE_PIB_ATTRIBUTE_ID]);
    }

    return(0);
}
