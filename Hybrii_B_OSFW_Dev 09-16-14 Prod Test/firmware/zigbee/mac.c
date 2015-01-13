/**
 * @file mac.c
 *
 * MAC Entry point
 *
 * $Id: mac.c,v 1.5 2014/06/09 13:19:46 kiran Exp $
 *
 * Copyright (c) 2011, Greenvity Communication All rights reserved.
 *
 */

/* === Includes ============================================================= */

#ifdef RTX51_TINY_OS
#include <rtx51tny.h>
#include "hybrii_tasks.h"
#endif
#include <stdio.h>
#include <string.h>
#include "papdef.h"
#include "return_val.h"
#include "bmm.h"
#include "qmm.h"
#include "mac_const.h"
#include "mac_msgs.h"
#include "mac_data_structures.h"
#include "timer.h"
#include "stm.h"
#include "mac_hal.h"
#include "mac_internal.h"
#include "mac.h"
#include "mac_security.h"


/* === Macros =============================================================== */

/* === Globals ============================================================== */
mac_stats_t mac_stats_g;

/**
 * Current state of the MAC state machine
 */
mac_state_t mac_state;

/**
 * Current state of scanning process.
 */
mac_scan_state_t mac_scan_state;

/**
 * Current state of syncronization with parent
 * (either coordinator or PAN coordinator).
 */
mac_sync_state_t mac_sync_state;

/**
 * Current state of MAC poll state machine,
 * e.g. polling for data, polling for Association Response, etc.
 */
mac_poll_state_t mac_poll_state;

/**
 * Radio sleep state
 */
mac_radio_state_t mac_radio_state;

/**
 * Final Cap Slot of current Superframe
 */
uint8_t mac_final_cap_slot;

/**
 * Flag stating that the last received beacon frame from the parent
 * indicated pending broadcast data to be received.
 */
bool mac_bc_data_indicated;

/**
 * Global parse data
 */
parse_t mac_parse_data;

/*
 * Flag indicating that RxEnable is still active.
 */
bool mac_rx_enabled;

/*
 * Variables for duplicate detection.
 * In order to detect duplicated frames, the DSN and Source Address of the
 * last received data frame need to be stored.
 */
uint8_t mac_last_dsn;
uint64_t mac_last_src_addr;
uint16_t mac_last_src_addr_short;

/* MAC PIB variables */

/**
 * Indication of whether the device is associated to the PAN through the PAN
 * coordinator. A value of TRUE indicates the device has associated through the
 * PAN coordinator. Otherwise, the value is set to FALSE.
 */
uint8_t mac_pib_macAssociatedPANCoord;

/**
 * The maximum number of CAP symbols in a beaconenabled PAN, or symbols in a
 * nonbeacon-enabled PAN, to wait either for a frame intended as a response to
 * a data request frame or for a broadcast frame following a beacon with the
 * Frame Pending subfield set to one.
 * This attribute, which shall only be set by the next higher layer, is
 * dependent upon macMinBE, macMaxBE, macMaxCSMABackoffs and the number of
 * symbols per octet. See 7.4.2 for the formula relating the attributes.
 * Maximum values:
 * O-QPSK (2.4 GHz and 900 MHz for Channel page 2): 25766
 * BPSK (900 MHz for Channel page 0): 26564
 * Both values are valid for
 * macMinBE = 8
 * macMaxBE = 8
 * macMaxCSMABackoffs = 5
 *
 * This PIB attribute is only used if basic indirect data transmission is used
 * or if beacon enabled network is enabled.
 */
uint16_t mac_pib_macMaxFrameTotalWaitTime;

/**
 * The maximum time, in multiples of aBaseSuperframeDuration, a device shall
 * wait for a response command frame to be available following a request
 * command frame.
 */
uint16_t mac_pib_macResponseWaitTime;

/**
 * Indication of whether the MAC sublayer has security enabled. A value of
 * TRUE indicates that security is enabled, while a value of FALSE indicates
 * that security is disabled.
 */
bool mac_pib_macSecurityEnabled;

/**
 * Holds the value which states whether a coordinator is currently allowing
 * association. A value of true indicates that association is permitted.
 */
uint8_t mac_pib_macAssociationPermit;

/**
 * Holds the maximum time (in superframe periods) that a indirect transaction
 * is stored by a PAN coordinator.
 */
uint16_t mac_pib_macTransactionPersistenceTime;


/**
 * Holds the sequence number added to the transmitted beacon frame.
 */
uint8_t mac_pib_macBSN;

/**
 * Holds the contents of the beacon payload.
 */
uint8_t mac_beacon_payload[aMaxBeaconPayloadLength];

/**
 * Holds the length, in octets, of the beacon payload.
 */
uint8_t mac_pib_macBeaconPayloadLength;

/**
 * Holds the value which states whether a device automatically sends a data
 * request command if its address is listed in the beacon frame. A value of true
 * indicates that the data request command is automatically sent.
 */
uint8_t mac_pib_macAutoRequest;

/**
 * Holds the value which states the number of backoff periods during which the
 * receiver is enabled following a beacon in battery life extension mode.
 * This value is dependent on the currently selected logical channel.
 */
uint8_t mac_pib_macBattLifeExtPeriods;

/**
 * Holds the 64 bit address of the coordinator with which the
 * device is associated.
 */
uint64_t mac_pib_macCoordExtendedAddress;

/**
 * Holds the 16 bit short address of the coordinator with which the device is
 * associated. A value of 0xfffe indicates that the coordinator is only using
 * its 64 bit extended address. A value of 0xffff indicates that this
 * value is unknown.
 */
uint16_t mac_pib_macCoordShortAddress;

/**
 * Holds the sequence number of the transmitted data or command frame.
 */
uint8_t mac_pib_macDSN;

/**
 * Holds the value which states whether the MAC sublayer is to enable its
 * receiver during idle periods.
 */
bool mac_pib_macRxOnWhenIdle;

/**
 * Holds the values of all security related PIB attributes.
 */
mac_sec_pib_t mac_sec_pib;

/**
 * Holds the mlme request buffer pointer, used to give the respective
 * confirmation in scan, poll and association.
 */
uint8_t *mac_conf_buf_ptr;

/**
 * Stores the original channel before start of scanning.
 */
uint8_t mac_scan_orig_channel;

/**
 * Stores the original channel page before start of scanning.
 */
uint8_t mac_scan_orig_page;

/**
 * Stores the original PAN-Id before start of scanning.
 */
uint16_t mac_scan_orig_panid;

/**
 * Holds the buffer pointer which is used to send scan command.
 */
uint8_t *mac_scan_cmd_buf_ptr;

/**
 * MAC busy state, indicates whether MAC can process any
 * request from NHLE.
 */
bool mac_busy;

/**
 * NHLE to MAC queue in which NHLE pushes all the requests to the MAC layer
 */
queue_t nhle_mac_q;

/**
 * Queue used by MAC for its internal operation. TAL pushes the incoming frames
 * in this queue.
 */
queue_t hal_mac_q;

/**
 * Queue used by MAC layer in beacon-enabled network to put in broadcast data.
 * Any broadcast data given by NHLE at a Coordinator or PAN Coordinator
 * in a beacon-enabled network is placed here by MAC.
 */
queue_t broadcast_q;


/**
 * Queue used by MAC layer to put in indirect data. Any indirect data given by
 * NHLE is placed here by MAC, until the device polls for the data.
 */
queue_t indirect_data_q;

tTimerId indirect_data_persistence_timer;
tTimerId scan_duration_timer;
tTimerId beacon_tracking_timer;
tTimerId beacon_missed_timer;
tTimerId poll_wait_timer;
tTimerId mac_rsp_wait_timer;
tTimerId mac_asso_rsp_wait_timer;

typedef void (*mac_funct_t)(buffer_t *);

static code mac_funct_t dispatch_table[MLME_LAST_MESSAGE] =
{
    NULL,
    mlme_associate_request,    // MLME_ASSOCIATE_REQUEST
    mlme_associate_response,   // MLME_ASSOCIATE_RESPONSE
    mcps_data_request,         // MCPS_DATA_REQUEST
    mcps_purge_request,        // MCPS_PURGE_REQUEST
    mlme_disassociate_request, // MLME_DISASSOCIATE_REQUEST
    mlme_set_request,          // MLME_SET_REQUEST
    mlme_orphan_response,      // MLME_ORPHAN_RESPONSE
    mlme_get_request,          // MLME_GET_REQUEST
    mlme_reset_request,        // MLME_RESET_REQUEST
    mlme_rx_enable_request,    // MLME_RX_ENABLE_REQUEST
    mlme_scan_request,         // MLME_SCAN_REQUEST
    NULL,
    mlme_start_request,        // MLME_START_REQUEST
    mlme_poll_request,         // MLME_POLL_REQUEST
    mlme_sync_request,         // MLME_SYNC_REQUEST
};

/* === Prototypes =========================================================== */


/* === Implementation ======================================================= */

void mac_q_flush (void)
{
    qmm_queue_flush(&nhle_mac_q);
    qmm_queue_flush(&hal_mac_q);
    qmm_queue_flush(&indirect_data_q);
    mac_hal_frame_pending(false);
    qmm_queue_flush(&broadcast_q);
}
 
/**
 *
 * This function decodes all MAC messages and calls the appropriate handler.
 *
 * buffer_p - Pointer to the buffer where the message type and its data 
 *            reside
 */
void mac_event_dispatcher (buffer_t *buff_p)
{
    mac_funct_t mac_funct_p;
    uint8_t *mac_msg_p = BMM_BUFFER_POINTER(buff_p);
    uint8_t cmd_code;

    cmd_code = mac_msg_p[MLME_CMD_CODE]; 
    if (cmd_code < MLME_LAST_MESSAGE) {
        mac_funct_p = dispatch_table[cmd_code];

        if (mac_funct_p != NULL) {
            mac_funct_p(buff_p);
        } else {             
            bmm_buffer_free(buff_p);
        }
    } else {
        bmm_buffer_free(buff_p);
    }
}

/**
 * @brief Runs the MAC scheduler
 *
 * This function runs the MAC scheduler.
 *
 * MLME and MCPS queues are removed alternately, starting with MLME queue.
 *
 * @return true if event is dispatched, false if no event to dispatch.
 */
bool mac_task (void)
{
    buffer_t *buff_p = NULL;
    bool processed_event = false;

    while (TRUE) {
        if (mac_busy == FALSE) {
            /* Check whether queue is empty */
            if (nhle_mac_q.size != 0) { 
                buff_p = qmm_queue_remove(&nhle_mac_q, NULL);
                
                /* If an event has been detected, handle it. */
                if (buff_p != NULL) {
                    /* Process event due to NHLE requests */
                    mac_event_dispatcher(buff_p); 
                    processed_event = true;
                }
            }
        }

        /*
         * Internal event queue should be dispatched
         * irrespective of the dispatcher state.
         */
        /* Check whether queue is empty */
        if (hal_mac_q.size != 0) {
            buff_p = qmm_queue_remove(&hal_mac_q, NULL);

            /* If an event has been detected, handle it. */
            if (buff_p != NULL) {
                mac_event_dispatcher(buff_p);
                processed_event = true;
            }
        }
#ifdef RTX51_TINY_OS
        if (nhle_mac_q.size == 0 && hal_mac_q.size == 0) {
            break;
        } else {
            /* 
             * Stay in the loop to serve the Q's but
             * momentaty give up the CPU so other tasks
             * can run
             */
            os_wait(K_TMO, 2, 0);  /* 2 ticks */
        }
#else
        break; /* Exit while loop immediately if no Tiny OS */
#endif
    }
    return processed_event;
}

#ifdef RTX51_TINY_OS
void os_mac_task (void) _task_ ZIGBEE_TASK_ID_MAC
{
    while (TRUE) {
        os_wait1(K_SIG); 
        mac_task();
        mac_hal_sm_handler();
#ifndef ZIGBEE_HAL_TASK
        mac_hal_task();
#endif
    }
}
#endif

/**
 * MAC function to wake-up the radio from sleep state
 */
void mac_trx_wakeup (void)
{
     /* FIXME - Add code to wake up the AFE */
}

/**
 * Puts the radio to sleep if this is allowed
 */
void mac_trx_sleep (void)
{
}

/**
 * Puts the radio to sleep mode 
 */
void mac_trx_init_sleep (void)
{
}

/*
 * Initializes the MAC PIBs
 *
 * This function initializes all MAC PIBs to their defaults as stated by
 * 802.15.4.
 */
static void mac_init_pib (void)
{
    uint32_t random_value;

    mac_pib_macAssociatedPANCoord = macAssociatedPANCoord_def;
    mac_pib_macMaxFrameTotalWaitTime = macMaxFrameTotalWaitTime_def;
    mac_pib_macResponseWaitTime = macResponseWaitTime_def;
    mac_pib_macSecurityEnabled = macSecurityEnabled_def;
    mac_pib_macAssociationPermit = macAssociationPermit_def;

    mac_pib_macBeaconPayloadLength = macBeaconPayloadLength_def;
    mac_hal_get_current_time(&random_value);
    mac_pib_macBSN = (uint8_t) random_value;

    mac_pib_macTransactionPersistenceTime = macTransactionPersistenceTime_def;
    mac_pib_macAutoRequest = macAutoRequest_def;
    mac_pib_macBattLifeExtPeriods = macBattLifeExtPeriods_def;
    mac_pib_macCoordExtendedAddress.lo_u32 = CLEAR_ADDR_32;
    mac_pib_macCoordExtendedAddress.hi_u32 = CLEAR_ADDR_32;
    mac_pib_macCoordShortAddress = macCoordShortAddress_def;
    mac_pib_macDSN = (uint8_t) (random_value >> 8);
    mac_pib_macRxOnWhenIdle = macRxOnWhenIdle_def;

    mac_sec_pib.KeyTableEntries = macKeyTableEntries_def;
    mac_sec_pib.DeviceTableEntries = macDeviceTable_def;
    mac_sec_pib.SecurityLevelTableEntries = macSecurityLevelTable_def;
    mac_sec_pib.FrameCounter = macFrameCounter_def;
}

/*
 * Intializes the MAC global variables
 */
static void mac_init_vars (void)
{
    mac_busy = false;
    mac_state = MAC_IDLE;
    mac_radio_state = RADIO_AWAKE;
    mac_scan_state = MAC_SCAN_IDLE;
    mac_sync_state = MAC_SYNC_NEVER;
    mac_poll_state = MAC_POLL_IDLE;
    mac_final_cap_slot = FINAL_CAP_SLOT_DEFAULT;
    mac_bc_data_indicated = false;
    mac_last_dsn = 0;
    mac_last_src_addr.lo_u32 = 0xFFFFFFFF;
    mac_last_src_addr.hi_u32 = 0xFFFFFFFF;
    mac_rx_enabled = false;
}

void mac_clear_stats (void)
{
    memset(&mac_stats_g, 0, sizeof(mac_stats_g));
}

void mac_display_tx_stats (void)
{
    printf("\nS/W TX Statistics:\n");
    printf("  No Ack = %lu, No CCA = %u, Frame Pending = %lu, Errors = %u\n", 
           mac_stats_g.tx_pkts_no_ack, mac_stats_g.tx_pkts_no_cca,
           mac_stats_g.frame_pending, mac_stats_g.tx_errors);
}

void mac_display_rx_stats (void)
{
    printf("\nS/W RX Statistics:\n");
    printf("  Packets received = %lu - Bytes received = %lu\n",
	       mac_stats_g.rx_pkts_count, mac_stats_g.rx_bytes_count);
    printf("  Decrypt Error = %u, Decrypt Ok = %u, No buffer = %u, "
           "Frame too big = %u, Bad CRC = %u\n",
           mac_stats_g.decrypt_error, mac_stats_g.decrypt_ok,
           mac_stats_g.rx_no_buffer, mac_stats_g.rx_frame_too_big,
           mac_stats_g.rx_bad_crc);
}

void mac_get_tx_stats (uint32_t *tx_count, uint32_t *tx_bytes,
                       uint16_t *tx_errors, uint32_t *tx_bc_count,
                       uint32_t *tx_bc_bytes)
{
    *tx_count    = mac_stats_g.tx_pkts_count;
    *tx_bytes    = mac_stats_g.tx_bytes_count;
    *tx_errors   = mac_stats_g.tx_errors;
    *tx_bc_count = mac_stats_g.tx_bc_pkts_count;
    *tx_bc_bytes = mac_stats_g.tx_bc_bytes_count;
}

void mac_get_rx_stats (uint32_t *rx_count, uint32_t *rx_bytes,
                       uint16_t *decrypt_err)
{
    *rx_count = mac_stats_g.rx_pkts_count;
    *rx_bytes = mac_stats_g.rx_bytes_count;
    *decrypt_err = mac_stats_g.decrypt_error;
}

/*
 * Internal MAC soft reset function
 *
 * This function resets the MAC variables, stops all running timers and
 * initializes the PIBs.
 *
 * init_pib - Boolean indicates whether PIB attributes shall be
 * initialized or not.
 */
static void mac_soft_reset (uint8_t init_pib)
{
    mac_init_vars();

    /* Trun off Phy TX and RX */
    mac_hal_hw_control(PHY_TRX_OFF);

    // FIXME - Add code to stop of running timer

    if (init_pib) {
        mac_init_pib();
    }
}

/*

 * This function resets the MAC variables, stops all running timers and
 * initializes the PIBs.
 *
 * init_pib - Boolean indicates whether PIB attributes shall be
 * initialized or not.
 *
 * @return Success or failure status
 */
retval_t mac_reset (uint8_t init_pib)
{
    retval_t status;

    /* Reset HAL */
    status = mac_hal_reset(init_pib);

    mac_soft_reset(init_pib);

    return (status);
}

/**
 * Resets the MAC helper variables and transition to idle state
 *
 * This function sets the MAC to idle state and resets
 * MAC helper variables
 */
void mac_idle_trans (void)
{
    uint16_t default_shortaddress = macShortAddress_def;
    uint16_t default_panid = macPANId_def;

    /* Wake up radio first */
    mac_trx_wakeup();

    set_hal_pib_internal(macShortAddress, (void *)&default_shortaddress);
    set_hal_pib_internal(macPANId, (void *)&default_panid);

    mac_soft_reset(true);

    /* Set radio to sleep if allowed */
    mac_trx_sleep();
}

void zb_mac_timer_handler (u16 type, void *cookie)
{
    switch (type) {
    case BEACON_TRACKING_TIMER:
        mac_sync_tracking_beacons_cb(cookie);
        break;
    case BEACON_MISSED_TIMER:
        mac_sync_missed_beacons_cb(cookie);
        break;
    case POLL_WAIT_TIMER:
        mac_poll_wait_time_cb(cookie);
        break;
    case DATA_PERSISTENCE_TIMER:
        mac_persistence_timer_cb(cookie);
        break;
    case SCAN_DURATION_TIMER:
        mac_scan_duration_cb(cookie);
        break;
    case MAC_RSP_WAIT_TIMER:
        mac_response_wait_cb(cookie); 
        break;
    case MAC_ASSO_RSP_WAIT_TIMER:
        mac_assocresponsetime_cb(cookie); 
        break;
    default:
        break;
    }
}

/**
 * Initializes the MAC sublayer
 *
 */
void mac_init (void)
{
    mac_init_vars();
    mac_clear_stats();
    mac_init_pib();
    qmm_queue_init(&nhle_mac_q, NHLE_MAC_QUEUE_CAPACITY);
    qmm_queue_init(&hal_mac_q, HAL_MAC_QUEUE_CAPACITY);
    qmm_queue_init(&indirect_data_q, INDIRECT_DATA_QUEUE_CAPACITY);
    qmm_queue_init(&broadcast_q, BROADCAST_QUEUE_CAPACITY);
#ifdef RTX51_TINY_OS
    os_create_task(ZIGBEE_TASK_ID_MAC);

    indirect_data_persistence_timer = STM_AllocTimer(ZB_LAYER_TYPE_MAC,
                                                     DATA_PERSISTENCE_TIMER,
                                                     NULL);
    if (STM_TIMER_ID_NULL == indirect_data_persistence_timer) {
       // Display error
       return;
    }
    scan_duration_timer = STM_AllocTimer(ZB_LAYER_TYPE_MAC,
                                         SCAN_DURATION_TIMER,
                                         NULL);
    if (STM_TIMER_ID_NULL == scan_duration_timer) {
       // Display error
       return;
    }

    beacon_tracking_timer = STM_AllocTimer(ZB_LAYER_TYPE_MAC,
                                          BEACON_TRACKING_TIMER,
                                          NULL);
    if (STM_TIMER_ID_NULL == beacon_tracking_timer) {
       // Display error
       return;
    }

    beacon_missed_timer = STM_AllocTimer(ZB_LAYER_TYPE_MAC,
                                         BEACON_MISSED_TIMER,
                                         NULL);
    if (STM_TIMER_ID_NULL == beacon_missed_timer) {
       // Display error
       return;
    }

    poll_wait_timer = STM_AllocTimer(ZB_LAYER_TYPE_MAC,
                                     POLL_WAIT_TIMER,
                                     NULL);
    if (STM_TIMER_ID_NULL == poll_wait_timer) {
       // Display error
       return;
    }

    mac_rsp_wait_timer = STM_AllocTimer(ZB_LAYER_TYPE_MAC,
                                        MAC_RSP_WAIT_TIMER,
                                        NULL);
    if (STM_TIMER_ID_NULL == mac_rsp_wait_timer) {
       // Display error
       return;
    }

    mac_asso_rsp_wait_timer = STM_AllocTimer(ZB_LAYER_TYPE_MAC,
                                             MAC_ASSO_RSP_WAIT_TIMER,
                                             NULL);
    if (STM_TIMER_ID_NULL == mac_asso_rsp_wait_timer) {
       // Display error
       return;
    }

#endif
    mac_hal_init();
}
