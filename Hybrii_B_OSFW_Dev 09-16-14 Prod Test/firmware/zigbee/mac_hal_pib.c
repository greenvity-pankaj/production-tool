/**
 * @file mac_hal_pib.c
 *
 * This file handles the HAL PIB attributes initialization and set/get
 *
 * $Id: mac_hal_pib.c,v 1.3 2014/06/10 22:46:43 yiming Exp $
 *
 * Copyright (c) 2011, Greenvity Communication All rights reserved.
 *
 */

/* === INCLUDES ============================================================ */

#include <string.h>
#include <stdio.h>	   //[YM] temporary for printf debug
#include "hal_common.h"
#include "gv701x_cfg.h"
#include "hybrii_802_15_4_regs.h"
#include "return_val.h"
#include "mac_const.h"
#include "bmm.h"
#include "qmm.h"
#include "utils.h"
#include "mac_msgs.h"
#include "mac_hal.h"

/* === TYPES =============================================================== */

/* === MACROS ============================================================== */

/* === GLOBALS ============================================================= */

/*
 * HAL PIBs
 */
/**
 * The maximum number of symbols to wait for an acknowledgment frame to
 * arrive following a transmitted data frame.
 * macAckWaitDuration = aUnitBackoffPeriod + aTurnaroundTime +
 *                      phySHRDuration + (6 * phySymbolsPerOctet)
 */
uint8_t hal_pib_macAckWaitDuration;

/**
 * The maximum number of back-offs the CSMA-CA algorithm will attempt
 * before declaring a CSMA_CA failure.
 */
uint8_t hal_pib_MaxCSMABackoffs;

/**
 * The minimum value of the backoff exponent BE in the CSMA-CA algorithm.
 */
uint8_t hal_pib_MinBE;

/**
 * 16-bit PAN ID.
 */
uint16_t hal_pib_PANId;

/**
 * Node's 16-bit short address.
 */
uint16_t hal_pib_ShortAddress;

/**
 * Node's 64-bit (IEEE) address.
 */
uint64_t hal_pib_IeeeAddress;

/**
 * Current RF channel to be used for all transmissions and receptions.
 */
uint8_t hal_pib_CurrentChannel;

/**
 * Supported channels
 */
uint32_t hal_pib_SupportedChannels;

/**
 * Current channel page; supported: page 0; high-data rate mode: 2, 16, 17
 */
uint8_t hal_pib_CurrentPage;

/**
 * Maximum number of symbols in a frame:
 * = phySHRDuration + ceiling([aMaxPHYPacketSize + 1] x phySymbolsPerOctet)
 */
uint16_t hal_pib_MaxFrameDuration;

/**
 * Duration of the synchronization header (SHR) in symbols for the current PHY.
 */
uint8_t hal_pib_SHRDuration;

/**
 * Number of symbols per octet for the current PHY.
 */
uint8_t hal_pib_SymbolsPerOctet;

/**
 * The maximum value of the backoff exponent BE in the CSMA-CA algorithm.
 */
uint8_t hal_pib_MaxBE;

/**
 * The maximum number of retries allowed after a transmission failure.
 */
uint8_t hal_pib_MaxFrameRetries;

/**
 * Default value of transmit power of transceiver
 * using IEEE defined format of phyTransmitPower.
 */
uint8_t hal_pib_TransmitPower;

/**
 * CCA Mode
 */
uint8_t hal_pib_CCAMode;

/**
 * Indicates if the node is a PAN coordinator or not.
 */
bool hal_pib_PrivatePanCoordinator;

/**
 * Promiscuous Mode
 */
bool hal_pib_PromiscuousMode;

/**
 * Indication of whether battery life extension is enabled or not.
 */
bool hal_pib_BattLifeExt;

/**
 * Beacon order
 */
uint8_t hal_pib_BeaconOrder;

/**
 * Superframe order
 */
uint8_t hal_pib_SuperFrameOrder;

/**
 * Holds the time at which last beacon was transmitted or received.
 */
uint32_t hal_pib_BeaconTxTime;

/* === PROTOTYPES ========================================================== */

/* === IMPLEMENTATION ====================================================== */
static uint8_t mac_hal_pib_tx_pwr (uint8_t tx_power)
{
    // FIXME - Add code to compute the TX Power
	tx_power = tx_power;
    return (0);
}

/**
 * Initialize the PIB
 *
 * This function initializes the HAL information base attributes
 * to their default values.
 */
void mac_hal_pib_init (void)
{
    hal_pib_MaxCSMABackoffs = HAL_MAX_CSMA_BACKOFFS_DEFAULT;
    hal_pib_MinBE = HAL_MINBE_DEFAULT;
    hal_pib_PANId = HAL_PANID_BC_DEFAULT;
    hal_pib_ShortAddress = HAL_SHORT_ADDRESS_DEFAULT;
#ifndef Flash_Config	
    hal_pib_CurrentChannel = HAL_CURRENT_CHANNEL_DEFAULT;
#else
    /* [YM] Get the channel from the flash and set
     * hal_pib_CurrentChannel = to that */
	hal_pib_CurrentChannel = sysConfig.defaultCH;
	printf("hal_pib default channel = %bx\n", hal_pib_CurrentChannel);
#endif
    hal_pib_SupportedChannels = HAL_SUPPORTED_CHANNELS;
    hal_pib_CurrentPage = HAL_CURRENT_PAGE_DEFAULT;
    hal_pib_MaxFrameDuration = HAL_MAX_FRAME_DURATION_DEFAULT;
    hal_pib_SHRDuration = HAL_SHR_DURATION_DEFAULT;
    hal_pib_SymbolsPerOctet = HAL_SYMBOLS_PER_OCTET_DEFAULT;
    hal_pib_MaxBE = HAL_MAXBE_DEFAULT;
    hal_pib_MaxFrameRetries = HAL_MAXFRAMERETRIES_DEFAULT;
    hal_pib_TransmitPower = mac_hal_pib_tx_pwr(HAL_TRANSMIT_POWER_DEFAULT);
    hal_pib_CCAMode = HAL_CCA_MODE_DEFAULT;
    hal_pib_PrivatePanCoordinator = HAL_PAN_COORDINATOR_DEFAULT;
    hal_pib_BattLifeExt = HAL_BATTERY_LIFE_EXTENSION_DEFAULT;
    hal_pib_BeaconOrder = HAL_BEACON_ORDER_DEFAULT;
    hal_pib_SuperFrameOrder = HAL_SUPERFRAME_ORDER_DEFAULT;
    hal_pib_PromiscuousMode = HAL_PIB_PROMISCUOUS_MODE_DEFAULT;
    hal_pib_macAckWaitDuration = aUnitBackoffPeriod + aTurnaroundTime +
                                 hal_pib_SHRDuration +
                                 (6 * hal_pib_SymbolsPerOctet);
}


/**
 * Write all shadow PIB variables to the MAC Asic 
 *
 * This function writes all shadow PIB variables to the MAC Asic Registers.
 */
void mac_hal_pib_write_to_asic (void)
{
    /* Configure the PAN ID */
    hal_common_bit_field_reg_write(ZIG_MAC_PAN_ID, hal_pib_PANId);

    /* Configure the IEEE Extended Address */
    hal_common_bit_field_reg_write(ZIG_MAC_ID_IEEE_EXT_ADDR_LO,
                                   hal_pib_IeeeAddress.lo_u32);
    hal_common_bit_field_reg_write(ZIG_MAC_ID_IEEE_EXT_ADDR_HI,
                                   hal_pib_IeeeAddress.hi_u32);

    /* Configure the Short Address */
    hal_common_bit_field_reg_write(ZIG_MAC_ID_SHORT, hal_pib_ShortAddress);

    /* Configure CCA Mode */
    hal_common_bit_field_reg_write(ZIG_PHY_CCA_MODE, hal_pib_CCAMode);

    /* Configure CSMA/CA parameters */
    hal_common_bit_field_reg_write(ZIG_MIN_BE, hal_pib_MinBE);
    hal_common_bit_field_reg_write(ZIG_MAX_BE, hal_pib_MaxBE);
    hal_common_bit_field_reg_write(ZIG_NO_CCA_MAX_RETRY,
                                   hal_pib_MaxCSMABackoffs);
    hal_common_bit_field_reg_write(ZIG_NO_ACK_MAX_RETRY,
                                   hal_pib_MaxFrameRetries);
   

    hal_common_bit_field_reg_write(ZIG_COO_EN, hal_pib_PrivatePanCoordinator);

    /* 
     * Always enable security in h/w. S/w will check for 
     * mac_pib_macSecurityEnabled when send/receive packets
     */
    // FIXME hal_common_bit_field_reg_write(ZIG_AES_ENGINE_EN, TRUE);

    /* FIXME - Add code to configure the channel */

    if (hal_pib_BeaconOrder == NON_BEACON_NWK) {
        hal_common_bit_field_reg_write(ZIG_NON_BEACON_NWK, TRUE);
    }

    hal_common_bit_field_reg_write(ZIG_SIFS_PERIOD, macMinSIFSPeriod_def);
    hal_common_bit_field_reg_write(ZIG_LIFS_PERIOD, macMinLIFSPeriod_def);
    hal_common_bit_field_reg_write(ZIG_ACK_TX_TIME, aTurnaroundTime);
    hal_common_bit_field_reg_write(ZIG_ACK_TIMEOUT, hal_pib_macAckWaitDuration);
}


/**
 * Gets a HAL PIB attribute
 *
 * This function is called to retrieve the HAL information base
 * attributes.
 *
 */
retval_t mac_hal_pib_get (uint8_t attribute, uint8_t *value)
{
    switch (attribute) {
    case macMaxCSMABackoffs:
        *value = hal_pib_MaxCSMABackoffs;
        break;

    case macMinBE:
        *value = hal_pib_MinBE;
        break;

    case macPANId:
        *(uint16_t *)value = hal_pib_PANId;
        break;

    case macPromiscuousMode:
        *(uint16_t *)value = hal_pib_PromiscuousMode;
        break;

    case macShortAddress:
        *(uint16_t *)value = hal_pib_ShortAddress;
        break;

    case phyCurrentChannel:
        *value = hal_pib_CurrentChannel;
        break;

    case phyChannelsSupported:
        *(uint32_t *)value = hal_pib_SupportedChannels;
        break;

    case phyTransmitPower:
        *value = hal_pib_TransmitPower;
        break;

    case phyCCAMode:
        *value = hal_pib_CCAMode;
        break;

    case phyCurrentPage:
        *value = hal_pib_CurrentPage;
        break;

    case phyMaxFrameDuration:
        *(uint16_t *)value = hal_pib_MaxFrameDuration;
        break;

    case phySymbolsPerOctet:
        *value = hal_pib_SymbolsPerOctet;
        break;

    case phySHRDuration:
        *value = hal_pib_SHRDuration;
        break;

    case macMaxBE:
        *value = hal_pib_MaxBE;
        break;

    case macMaxFrameRetries:
        *value = hal_pib_MaxFrameRetries;
        break;

    case macIeeeAddress:
        *(uint64_t *)value = hal_pib_IeeeAddress;
        break;

    case macBattLifeExt:
        *(bool *)value = hal_pib_BattLifeExt;
        break;

    case macBeaconOrder:
        *value = hal_pib_BeaconOrder;
        break;

    case macSuperframeOrder:
        *value = hal_pib_SuperFrameOrder;
        break;

    case macBeaconTxTime:
        *(uint32_t *)value = hal_pib_BeaconTxTime;
        break;

    case I_AM_PAN_COORDINATOR:
        *(bool *)value = hal_pib_PrivatePanCoordinator;
        break;

    case macAckWaitDuration:
        break;

    default:
        /* Invalid attribute id */
        return MAC_UNSUPPORTED_ATTRIBUTE;
    }

    return MAC_SUCCESS;
}

retval_t mac_hal_pib_set_trx_state_check (uint8_t attribute,
                                          pib_value_t *value)
{
    // FIXME - Need to check ASIC status before changing the attributes

    switch (attribute) {
    case macMinBE:
        hal_pib_MinBE = value->pib_value_8bit;
        /*
         * macMinBE must not be larger than macMaxBE
         */
        if (hal_pib_MinBE > hal_pib_MaxBE) {
            hal_pib_MinBE = hal_pib_MaxBE;
        }
        hal_common_bit_field_reg_write(ZIG_MIN_BE, hal_pib_MinBE);
        break;

    case macMaxBE:
        hal_pib_MaxBE = value->pib_value_8bit;
        if (hal_pib_MaxBE < hal_pib_MinBE) {
            hal_pib_MinBE = hal_pib_MaxBE;
        }
        hal_common_bit_field_reg_write(ZIG_MAX_BE, hal_pib_MaxBE);
        break;

    case macPANId:
        hal_pib_PANId = value->pib_value_16bit;
        hal_common_bit_field_reg_write(ZIG_MAC_PAN_ID, hal_pib_PANId);
        break;

    case macShortAddress:
        hal_pib_ShortAddress = value->pib_value_16bit;
        hal_common_bit_field_reg_write(ZIG_MAC_ID_SHORT, hal_pib_ShortAddress);
        break;

    case phyCurrentChannel:
        hal_pib_CurrentChannel = value->pib_value_8bit;
#ifdef HYBRII_ASIC
        gv701x_cfg_zb_afe_init(hal_pib_CurrentChannel, FALSE);
#endif
        break;

    case phyCurrentPage:
        break;

    case phyTransmitPower:
        hal_pib_TransmitPower = value->pib_value_8bit;
        /* FIXME - Add code to convert to AFE value */
        break;

    case phyCCAMode:
        hal_pib_CCAMode = value->pib_value_8bit;
        hal_common_bit_field_reg_write(ZIG_PHY_CCA_MODE, hal_pib_CCAMode);
        break;

    case macIeeeAddress:
        hal_pib_IeeeAddress.lo_u32 = value->pib_value_64bit.lo_u32;
        hal_pib_IeeeAddress.hi_u32 = value->pib_value_64bit.hi_u32;
        hal_common_bit_field_reg_write(ZIG_MAC_ID_IEEE_EXT_ADDR_LO,
                                       hal_pib_IeeeAddress.lo_u32);
        hal_common_bit_field_reg_write(ZIG_MAC_ID_IEEE_EXT_ADDR_HI,
                                       hal_pib_IeeeAddress.hi_u32);
        break;

    case I_AM_PAN_COORDINATOR:
        hal_pib_PrivatePanCoordinator = value->pib_value_bool;
        hal_common_bit_field_reg_write(ZIG_COO_EN,
                                       hal_pib_PrivatePanCoordinator);
        break;

    default:
        return MAC_UNSUPPORTED_ATTRIBUTE;
    }

    return MAC_SUCCESS;
}

/**
 * Sets a HAL PIB attribute
 *
 * This function is called to set the HAL information base
 * attributes.
 *
 */
retval_t mac_hal_pib_set (uint8_t attribute, pib_value_t *value)
{
    retval_t status = MAC_SUCCESS;

    /*
     * FIXME - Check for ED state before allow HAL PIB to be changed
     */

    switch (attribute) {
    case macMaxFrameRetries:
        hal_pib_MaxFrameRetries = value->pib_value_8bit;
        hal_common_bit_field_reg_write(ZIG_NO_ACK_MAX_RETRY,
                                       hal_pib_MaxFrameRetries);
        break;

    case macMaxCSMABackoffs:
        hal_pib_MaxCSMABackoffs = value->pib_value_8bit;
        break;

    case macBattLifeExt:
        hal_pib_BattLifeExt = value->pib_value_bool;
        break;

    case macBeaconOrder:
        if (hal_pib_BeaconOrder != value->pib_value_8bit) {
            hal_pib_BeaconOrder = value->pib_value_8bit;
            if (NON_BEACON_NWK == hal_pib_BeaconOrder) {
                hal_common_reg_bit_set(ZigCtrl, ZIG_CTRL_NON_BEACON);
                hal_common_reg_32_write(ZigTxBeaconPeriod, 0x0fffffff);
                mac_hal_zigbee_interrupt_control(FALSE,
                                                 CPU_INT_ZB_BC_TX_TIME     |
                                                 CPU_INT_ZB_PRE_BC_TX_TIME  );
            } else {
                uint32_t bi;  /* Beacon Interval in symbols */
                uint32_t interrupt_types;

                hal_common_reg_bit_clear(ZigCtrl, ZIG_CTRL_NON_BEACON);
                bi = HAL_GET_BEACON_INTERVAL_TIME(hal_pib_BeaconOrder);
                hal_common_reg_32_write(ZigTxBeaconPeriod, bi);
                hal_common_bit_field_reg_write(ZIG_PRE_BEACON_INTERVAL_INT,
                                               140);
                if (TRUE == hal_pib_PrivatePanCoordinator) {
                    interrupt_types = CPU_INT_ZB_BC_TX_TIME     |
                                      CPU_INT_ZB_PRE_BC_TX_TIME;
                } else {
                    /*
                     * Might need to use this to detect no beacon
                     * has been received
                     */
                    interrupt_types = CPU_INT_ZB_BC_TX_TIME;
                }
                mac_hal_zigbee_interrupt_control(TRUE, interrupt_types);
            }
            hal_common_reg_bit_clear(ZigTxBlockEn, ZIG_BLK_TIMING_EN);
            hal_common_reg_bit_set(ZigTxBlockEn, ZIG_BLK_TIMING_EN);
        }
        break;

    case macSuperframeOrder:
        if (hal_pib_SuperFrameOrder != value->pib_value_8bit) {
            hal_pib_SuperFrameOrder = value->pib_value_8bit;
            if (NON_BEACON_NWK != hal_pib_SuperFrameOrder) {
                uint32_t cap;  /* cap interval in symbols */

                cap = HAL_GET_SUPERFRAME_DURATION_TIME(hal_pib_SuperFrameOrder);
                /*
                 * GTS is negotiated on the fly. We only need to
                 * program the CAP region
                 */
                hal_common_reg_32_write(ZigTxCAPPeriod, cap);
            }
            hal_common_reg_bit_clear(ZigTxBlockEn, ZIG_BLK_TIMING_EN);
            hal_common_reg_bit_set(ZigTxBlockEn, ZIG_BLK_TIMING_EN);
        }
        break;

    case macBeaconTxTime:
        hal_pib_BeaconTxTime = value->pib_value_32bit;
        break;

    case macPromiscuousMode:
        hal_pib_PromiscuousMode = value->pib_value_8bit;
        hal_common_bit_field_reg_write(ZIG_PROMISCUOUS_EN,
                                       hal_pib_PromiscuousMode);
        break;

    case macAckWaitDuration:
        return MAC_READ_ONLY;
        break;

    default:
        status = mac_hal_pib_set_trx_state_check(attribute, value);
        break;

    }

    return (status);
}


