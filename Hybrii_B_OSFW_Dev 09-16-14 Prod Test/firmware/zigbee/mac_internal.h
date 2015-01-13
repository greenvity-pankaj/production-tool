/**
 * @file mac_internal.h
 *
 * MAC internal functions, globals, and macros.
 *
 * $Id: mac_internal.h,v 1.1 2014/01/10 17:27:11 yiming Exp $
 *
 * Copyright (c) 2011, Greenvity Communication All rights reserved.
 *
 */

#ifndef _MAC_INTERNAL_H_
#define _MAC_INTERNAL_H_

/* === Includes ============================================================= */

/* === Macros =============================================================== */

#define HAL_MAC_QUEUE_CAPACITY              (255)
#define NHLE_MAC_QUEUE_CAPACITY             (255)
#define INDIRECT_DATA_QUEUE_CAPACITY        (255)
#define BROADCAST_QUEUE_CAPACITY            (255)

/*
 * Beacon order used as timer interval for checking the expiration of indirect
 * transactions in a nonbeacon-enabled network
 */
#define BO_USED_FOR_MAC_PERS_TIME       (3)

/*
 * Final CAP slot in standard superframe without GTS
 */
#define FINAL_CAP_SLOT_DEFAULT          (0x0F)

#define MAC_BUSY()     mac_busy = TRUE
#define MAC_NOT_BUSY() mac_busy = FALSE 

/* === Types ================================================================ */

/**
 * MAC state type.
 */
typedef enum mac_state_e
{
    /**
     * Node is idle,
     * i.e. it is neither associated nor has started its own network
     */
    MAC_IDLE = 0,

    /* Device has successfully associated */
    MAC_ASSOCIATED = 1,

    /**
     * Coordinator successfully associated with PAN Coordinator
     * and successfully started network with same PAN-Id
     * (not as PAN Coordinator)
     */
    MAC_COORDINATOR = 2,

    /** PAN coordinator successfully started */
    MAC_PAN_COORD_STARTED = 3
} mac_state_t;


/**
 * MAC poll states.
 * These states describe the current status of the MAC for polling
 * for devices or coordinators, not for PAN coordinator.
 */
typedef enum mac_poll_state_e
{
    /**
     * No polling ongoing.
     */
    MAC_POLL_IDLE = 0,

    /**
     * Successful transmission of association request frame,
     * wait for Association response.
     */
    MAC_AWAIT_ASSOC_RESPONSE,

    /**
     * Explicit poll ongoing (MLME_POLL.request),
     * Ack after Data request frame transmission received,
     * awaiting data response. */
    MAC_POLL_EXPLICIT,

    /**
     * Implicit poll ongoing (more pending data detected, either in beacon or
     * data frame),
     * awaiting data response, */
    MAC_POLL_IMPLICIT
} mac_poll_state_t;



/**
 * Device or coordinator scan states.
 */
typedef enum mac_scan_state_e
{
    /**
     * No scanning ongoing.
     */
    MAC_SCAN_IDLE = 0,

    /* Scanning in progress. */
    /** ED scan ongoing */
    MAC_SCAN_ED,
    /** Active scan proceeding */
    MAC_SCAN_ACTIVE,
    /** Orphan scan proceeding */
    MAC_SCAN_ORPHAN,
    /** Passive scan proceeding */
    MAC_SCAN_PASSIVE
} mac_scan_state_t;



/**
 * Device or coordinator sync states.
 */
typedef enum mac_sync_state_e
{
    /** Do not track beacons */
    MAC_SYNC_NEVER = 0,
    /** Track the next beacon */
    MAC_SYNC_ONCE,
    /** Track beacons continuously */
    MAC_SYNC_TRACKING_BEACON,
    /**
     * Track beacons continuously before beeing associated in order to obtain
     * synchronization with desired network
     */
    MAC_SYNC_BEFORE_ASSOC
} mac_sync_state_t;


/**
 * MAC sleep state type.
 */
typedef enum mac_radio_state_e
{
    /**< Radio is awake */
    RADIO_AWAKE = 0,
    /**< Radio is in sleep mode */
    RADIO_SLEEPING
} mac_radio_state_t;

typedef enum mac_timer_id_e
{
    BEACON_TRACKING_TIMER = 1, /* To wake up h/w before beacon is expected */
    BEACON_MISSED_TIMER,
    POLL_WAIT_TIMER,
    DATA_PERSISTENCE_TIMER,
    SCAN_DURATION_TIMER,
    MAC_RSP_WAIT_TIMER,
    MAC_ASSO_RSP_WAIT_TIMER,
} mac_timer_id_t;

typedef struct mac_stats_s
{
    uint32_t rx_pkts_count;
    uint32_t rx_bytes_count;
    uint16_t rx_frame_too_big;
    uint16_t rx_no_buffer;
    uint16_t rx_bad_crc;
    uint16_t decrypt_error;
    uint16_t decrypt_ok;
    uint32_t tx_pkts_count;
    uint32_t tx_bytes_count;
    uint32_t tx_bc_pkts_count;
    uint32_t tx_bc_bytes_count;
    uint32_t tx_pkts_no_ack;
    uint32_t frame_pending;
    uint16_t tx_pkts_no_cca;
    uint16_t tx_errors;
} mac_stats_t;

typedef void (*handler_t)(uint8_t *);

/* === Externals ============================================================ */

/* Global data variables */
extern mac_state_t      mac_state;
extern mac_scan_state_t mac_scan_state;
extern mac_sync_state_t mac_sync_state;
extern mac_poll_state_t mac_poll_state;
extern bool             mac_busy;
extern bool             mac_rx_enabled;
extern queue_t          indirect_data_q;
extern queue_t          broadcast_q;
extern queue_t          hal_mac_q;
extern uint8_t          mac_beacon_payload[];
extern parse_t          mac_parse_data;
extern uint8_t          mac_pib_macAssociationPermit;
extern uint8_t          mac_pib_macBeaconPayloadLength;
extern uint8_t          mac_pib_macBSN;
extern uint16_t         mac_pib_macTransactionPersistenceTime;
extern uint8_t          mac_pib_macAssociatedPANCoord;
extern uint8_t          mac_pib_macAutoRequest;
extern uint8_t          mac_pib_macBattLifeExtPeriods;
extern uint64_t         mac_pib_macCoordExtendedAddress;
extern uint16_t         mac_pib_macCoordShortAddress;
extern uint8_t          mac_pib_macDSN;
extern uint16_t         mac_pib_macMaxFrameTotalWaitTime;
extern uint16_t         mac_pib_macResponseWaitTime;
extern bool             mac_pib_macRxOnWhenIdle;
extern bool             mac_pib_macSecurityEnabled;
extern mac_stats_t      mac_stats_g;
extern bool               mac_bc_data_indicated;
extern uint8_t            mac_final_cap_slot;
extern mac_radio_state_t  mac_radio_state;
extern uint8_t            *mac_conf_buf_ptr;
extern uint8_t            mac_last_dsn;
extern uint64_t           mac_last_src_addr;
extern uint16_t           mac_last_src_addr_short;
extern uint16_t           mac_scan_orig_panid;
extern uint8_t            *mac_scan_cmd_buf_ptr;
extern uint8_t            mac_scan_orig_channel;
extern uint8_t            mac_scan_orig_page;
extern bool               hybrii_mode;

/* === Prototypes =========================================================== */
extern bool mac_task(void);
extern void mac_init(void);
extern void mac_q_flush(void);
extern void mac_gen_mcps_data_conf(buffer_t *buf_ptr, uint8_t status,
                                   uint8_t handle, uint32_t timestamp);
extern void mac_clear_stats(void);
extern void mac_get_tx_stats(uint32_t *tx_count, uint32_t *tx_bytes,
                             uint16_t *tx_errors, uint32_t *tx_bc_count,
                             uint32_t *tx_bc_bytes);
extern void mac_get_rx_stats(uint32_t *rx_count, uint32_t *rx_bytes,
                             uint16_t *decrypt_err);
extern void mac_display_tx_stats(void);
extern void mac_display_rx_stats(void);
extern void mac_tx_pending_bc_data(void);
extern void mac_start_beacon_timer(void);
extern void mac_trx_wakeup(void);
extern void mac_trx_sleep(void);
extern void mac_trx_init_sleep(void);
extern uint8_t mac_get_pib_attribute_size(uint8_t pib_attribute_id);
extern retval_t set_hal_pib_internal(uint8_t attribute,
                                     pib_value_t *attribute_value);
extern void mac_mlme_comm_status(uint8_t status, buffer_t *buf_ptr);
extern void mac_gen_mlme_associate_conf(buffer_t *buf_p,
                                        uint8_t  status,
                                        uint16_t assoc_short_addr);
extern bool mac_data_build_and_tx_data_req(bool expl_poll,
                                           bool force_own_long_addr,
                                           uint8_t expl_dest_addr_mode,
                                           address_field_t *expl_dest_addr,
                                           uint16_t expl_dest_pan_id);
extern void mac_process_associate_request(buffer_t *buf_ptr);
extern bool mac_tx_coord_realignment_command(frame_msgtype_t cmd_type,
                                             buffer_t *buf_ptr,
                                             uint16_t new_panid,
                                             uint8_t new_channel,
                                             uint8_t new_page);
extern void mac_process_disassociate_notification(buffer_t *buf_ptr);
extern void mac_prep_disassoc_conf(buffer_t *buf_p, uint8_t status);
extern void mac_process_orphan_notification(buffer_t *buf_ptr);
extern void mac_process_beacon_request(buffer_t *buf_ptr);

extern void mac_idle_trans(void);
extern void mac_sync_process_coord_realign(buffer_t *buf_ptr);
extern void mac_sync_loss(uint8_t loss_reason);
extern void mac_process_beacon_frame(buffer_t *buf_ptr);
extern void mac_process_data_frame(buffer_t *buf_ptr);
extern void mac_poll_process_data_response(void);
extern void mac_process_associate_response(buffer_t *buf_ptr); 
extern void mac_build_and_tx_beacon(bool beacon_enabled,
                                    security_info_t *sec_info_p);
extern void mac_start_persistence_timer(void);
extern void mac_sync_start_missed_beacon_timer(void);
extern void mac_beacon_send_cb(void);
extern void mac_persistence_timer_cb(void *callback_parameter);
extern void mac_scan_duration_cb(void *callback_parameter);
extern void mac_poll_wait_time_cb(void *callback_parameter);
extern void mac_response_wait_cb(void *callback_parameter);
extern void mac_assocresponsetime_cb(void *callback_parameter);
extern void mac_sync_missed_beacons_cb(void *callback_parameter);
extern void mac_sync_tracking_beacons_cb(void *callback_parameter);
extern void mac_scan_send_complete(retval_t status);
extern void mac_scan_process_orphan_realign(buffer_t *buf_ptr);
extern void mac_coord_realignment_command_tx_success(uint8_t tx_status,
                                                     buffer_t *buf_p);
#endif /* _MAC_INTERNAL_H_ */
