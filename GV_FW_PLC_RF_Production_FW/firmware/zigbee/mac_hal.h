/**
 * @file mac_hal.h 
 *
 * MAC 802.15.4 Hardware Abtraction Layer 
 *
 * $Id: mac_hal.h,v 1.7 2014/11/11 14:52:59 ranjan Exp $
 *
 * Copyright (c) 2011, Greenvity Communication All rights reserved.
 *
 */
#ifdef HYBRII_802154 
#ifndef _MAC_HAL_H_
#define _MAC_HAL_H_

/* === Includes ============================================================= */

/* === Macros =============================================================== */

#define ZIGBEE_MAC_RETRY_MAX                200

/*
 * FIXME:
 * This define is for 2.4 GHz. Will take care other bands later
 * 4 bits form one symbol since O-QPSK is used
 */
/** Symbols per octet */
#define SYMBOLS_PER_OCTET                   (2)
/** Number of symbols included in the preamble */
#define NO_SYMBOLS_PREAMBLE                 (8)
/** Number of symbols included in the SFD field */
#define NO_SYMBOLS_SFD                      (2)

/**
 * Number of symbols forming the synchronization header (SHR) for the current PHY.
 * This value is the base for the PHY PIB attribute phySHRDuration.
 */
#define NO_OF_SYMBOLS_PREAMBLE_SFD    (NO_SYMBOLS_PREAMBLE + NO_SYMBOLS_SFD)

/**
 * Maximum number of symbols in a frame for the current PHY.
 * This value is the base for the PHY PIB attribute phyMaxFrameDuration.
 */
#define MAX_FRAME_DURATION                  \
     (NO_OF_SYMBOLS_PREAMBLE_SFD + (aMaxPHYPacketSize + 1) * SYMBOLS_PER_OCTET)

/**
 * The maximum time in symbols for a 32 bit timer
 */
#define MAX_SYMBOL_TIME                     (0x0FFFFFFF)

/**
 * Symbol mask for ingnoring most significant nibble
 */
#define SYMBOL_MASK                         (0x0FFFFFFF)

/**
 * This is for 2.4 GHz. Will add code to compute base on which band
 */ 
#define US_PER_SYMBOL                           16
#define HAL_CONVERT_SYMBOLS_TO_US(symbols)      ((uint32_t)(symbols) << 4)
#define HAL_CONVERT_US_TO_SYMBOLS(time)         ((time) >> 4)
#define HAL_SUPPORTED_CHANNELS                  (0x07FFF800)
/*
 * Default PIB Values
 */

/*
 * FIXME:
 * This define is for 2.4 GHz. Will take care other bands later
 * Default value of custom HAL PIB channel page
 */
#define HAL_CURRENT_PAGE_DEFAULT            (0x00)

/*
 * Default value of maximum number of symbols in a frame
 */
#define HAL_MAX_FRAME_DURATION_DEFAULT      (MAX_FRAME_DURATION)

/*
 * Default value of duration of the synchronization header (SHR) in symbols
 * for the current PHY
 */
#define HAL_SHR_DURATION_DEFAULT            (NO_OF_SYMBOLS_PREAMBLE_SFD)

/*
 * Default value of number of symbols per octet for the current PHY
 */
#define HAL_SYMBOLS_PER_OCTET_DEFAULT       (SYMBOLS_PER_OCTET)

/*
 * Default value of maximum backoff exponent used while performing csma ca
 */
#define HAL_MAXBE_DEFAULT                   (0x05)

/*
 * Default value of PIB attribute macMaxFrameRetries
 */
#define HAL_MAXFRAMERETRIES_DEFAULT         (0x08)

/*
 * Default value of maximum csma ca backoffs
 */
#define HAL_MAX_CSMA_BACKOFFS_DEFAULT       (0x04)

/*
 * Default value of minimum backoff exponent used while performing csma ca
 */
#define HAL_MINBE_DEFAULT                   (0x03)

/*
 * Value of a broadcast PAN ID
 */
#define HAL_PANID_BC_DEFAULT                (0xFFFF)

/*
 * Default value of short address
 */
#define HAL_SHORT_ADDRESS_DEFAULT           (0xFFFF)

/*
 * Default value of current channel in HAL
 */
#define HAL_CURRENT_CHANNEL_DEFAULT         (0x0B)

/*
 * Default value of promiscuous mode in HAL
 */
#define HAL_PIB_PROMISCUOUS_MODE_DEFAULT    (false)

/*
 * Default value of transmit power of transceiver: Use highest tx power
 * FIXME: Check with John Tero to see what value
 */
#define HAL_TRANSMIT_POWER_DEFAULT          (0x3)

/*
 * Default value CCA mode
 */
#define HAL_CCA_MODE_DEFAULT                (TRX_CCA_MODE2)

/*
 * Default value beacon order set to 15
 */
#define HAL_BEACON_ORDER_DEFAULT            (15)

/*
 * Default value supeframe order set to 15
 */
#define HAL_SUPERFRAME_ORDER_DEFAULT        (15)

/*
 * Default value of BeaconTxTime
 */
#define HAL_BEACON_TX_TIME_DEFAULT          (0x00000000)

/*
 * Default value of BatteryLifeExtension.
 */
#define HAL_BATTERY_LIFE_EXTENSION_DEFAULT  (false)

/*
 * Default value of PAN Coordiantor custom HAL PIB
 */
#define HAL_PAN_COORDINATOR_DEFAULT         (false)

/*
 * Beacon Interval formula: BI = aBaseSuperframeDuration * 2^BO
 * Note: Beacon interval calculated is in symbols.
 */
/**
 * Beacon Interval time in symbols
 */
#define HAL_GET_BEACON_INTERVAL_TIME(BO) \
        ((1UL * aBaseSuperframeDuration) << (BO))

/*
 * Superframe Duration formula: SI = aBaseSuperframeDuration 2^SO
 * Note: Superframe Duration in symbold
 */
/**
 * Superframe Duration time in symbols
 */
#define HAL_GET_SUPERFRAME_DURATION_TIME(SO) \
        ((1UL * aBaseSuperframeDuration) << (SO))

#define I_AM_PAN_COORDINATOR               (0x0B)

#define HAL_INCOMING_FRAME_QUEUE_CAPACITY  (255)

#define SCAN_DURATION_TO_SYMBOLS(SD) \
                        (aBaseSuperframeDuration * ((1UL<<(SD))+1))
/* === Types================================================================ */

/** Transceiver commands */
typedef enum trx_cmd_e
{
    CMD_NOP                           = (0),
    CMD_TX_START                      = (2),
    CMD_FORCE_TRX_OFF                 = (3),
    CMD_RX_ON                         = (6),
    CMD_TRX_OFF                       = (8),
    CMD_PLL_ON                        = (9),
    CMD_FORCE_PLL_ON                  = (10),
    CMD_RX_AACK_ON                    = (22),
    CMD_TX_ARET_ON                    = (25),
    CMD_TRX_SLEEP                     = (26)
} trx_cmd_t;

/** Transceiver states */
typedef enum tal_trx_status_e
{
    P_ON                            = 0,
    BUSY_RX                         = 1,
    BUSY_TX                         = 2,
    RX_ON                           = 6,
    TRX_OFF                         = 8,
    PLL_ON                          = 9,
    TRX_SLEEP                       = 15,
    BUSY_RX_AACK                    = 17,
    BUSY_TX_ARET                    = 18,
    RX_AACK_ON                      = 22,
    TX_ARET_ON                      = 25,
    RX_ON_NOCLK                     = 28,
    RX_AACK_ON_NOCLK                = 29,
    BUSY_RX_AACK_NOCLK              = 30,
    STATE_TRANSITION_IN_PROGRESS    = 31
} tal_trx_status_t;

typedef enum hal_state_e
{
    HAL_IDLE           = 0,
    HAL_TX_AUTO        = 1,
    HAL_TX_DONE        = 2,
    HAL_SLOTTED_CSMA   = 3,
    HAL_ED_RUNNING     = 4,
    HAL_ED_DONE        = 5
} hal_state_t;

typedef enum hal_aes_direction_e 
{
    HAL_AES_ENCRYPT  = 0,
    HAL_AES_DECRYPT  = 1
} hal_aes_direction_t;

/**
 * CSMA Mode supported by transceiver
 */
typedef enum tx_mode_s
{
    NO_CSMA,
    CSMA_UNSLOTTED,
    CSMA_SLOTTED,
    GTS_SLOTTED,
} tx_mode_t;

/**
 * MAC Message types
 */
typedef enum
{
    /* MAC Command Frames (table 67) */
    /* Command Frame Identifier for Association Request */
    ASSOCIATIONREQUEST          = (0x01),
    /* Command Frame Identifier for Association Response */
    ASSOCIATIONRESPONSE,
    /* Command Frame Identifier for Disassociation Notification */
    DISASSOCIATIONNOTIFICATION,
    /* Command Frame Identifier for Data Request */
    DATAREQUEST,
    /* Command Frame Identifier for PANID Conflict Notification */
    PANIDCONFLICTNOTIFICAION,
    /* Command Frame Identifier for Orphan Notification */
    ORPHANNOTIFICATION,
    /* Command Frame Identifier for Beacon Request */
    BEACONREQUEST,
    /* Command Frame Identifier for Coordinator Realignment */
    COORDINATORREALIGNMENT,

   /*
    * These are not MAC command frames but listed here as they are needed
    * in the msgtype field
    */
    /* Message is a directed orphan realignment command */
    ORPHANREALIGNMENT,
    /* Message is a beacon frame (in response to a beacon request cmd) */
    BEACON_MESSAGE,
    /* Message type field value for implicite poll without request */
    DATAREQUEST_IMPL_POLL,
    /* Message type field value for Null frame */
    NULL_FRAME,
    /* Message type field value for MCPS message */
    MCPS_MESSAGE
} frame_msgtype_t;

typedef enum
{
    AES_NONE = 0,
    AES_DECRYPT,
    AES_ENCRYPT,
} hal_aes_t;

/**
 * @brief Globally used frame information structure
 *
 * @ingroup apiMacTypes
 */
typedef struct frame_info_s
{
    frame_msgtype_t msg_type;            /* Message type                    */
    buffer_t        *buffer_header_p;    /* Pointer to buffer header        */
    uint8_t         msduHandle;          /* MSDU handle                     */
    uint16_t        persistence_time;    /* Indirect frame persistence time */
    bool            indirect_in_transit; /* Indirect frame tx ongoing       */
    uint32_t        time_stamp;          /* Timestamp information of frame  */
    uint8_t         *mpdu_p;             /* Pointer to MPDU                 */
} frame_info_t;

/* === Externals ============================================================ */
extern uint8_t   hal_pib_MaxCSMABackoffs;
extern uint8_t   hal_pib_MinBE;
extern uint8_t   hal_pib_MaxBE;
extern uint16_t  hal_pib_PANId;
extern uint16_t  hal_pib_ShortAddress;
extern uint64_t  hal_pib_IeeeAddress;
extern uint8_t   hal_pib_CurrentChannel;
extern uint32_t  hal_pib_SupportedChannels;
extern uint8_t   hal_pib_CurrentPage;
extern uint16_t  hal_pib_MaxFrameDuration;
extern uint8_t   hal_pib_SHRDuration;
extern uint8_t   hal_pib_SymbolsPerOctet;
extern uint8_t   hal_pib_MaxFrameRetries;
extern uint8_t   hal_pib_TransmitPower;
extern uint8_t   hal_pib_CCAMode;
extern bool      hal_pib_PrivatePanCoordinator;
extern bool      hal_pib_PromiscuousMode;
extern bool      hal_pib_BattLifeExt;
extern uint8_t   hal_pib_BeaconOrder;
extern uint8_t   hal_pib_SuperFrameOrder;
extern uint32_t  hal_pib_BeaconTxTime;
extern tx_mode_t hal_current_tx_mode;

/* === Prototypes =========================================================== */
extern void mac_hal_init(void);
extern void mac_hal_pib_init(void);
extern void mac_hal_pib_write_to_asic(void);
extern void mac_hal_task(void);
extern retval_t mac_hal_tx_frame(frame_info_t *tx_frame_p, tx_mode_t tx_mode,
                                 tinybool secure);
extern retval_t mac_hal_tx_beacon(frame_info_t *tx_frame_p,
                                  tinybool encrypt);
extern retval_t mac_hal_pib_set(uint8_t attribute, pib_value_t *value);
extern retval_t mac_hal_pib_get(uint8_t attribute, uint8_t *value);
extern retval_t mac_hal_aes_key_config(hal_aes_direction_t aes_dir_e,
                                       uint8_t *key_p, uint8_t *ext_addr_p);
extern retval_t mac_hal_write_frame_to_fifo(uint8_t *frame_p,
                                            uint8_t frame_len,
                                            hal_aes_t aes_type_e,
                                            tinybool use_beacon_template);
extern void mac_hal_ed_start(uint8_t scan_duration);
/* From mac_hal_intf.c */
extern void mac_hal_intf_tx_frame_done(retval_t tx_status,
                                       frame_info_t *frame_p);
extern void mac_hal_poll_rx_fifo_for_packet(void);
extern void mac_hal_flush_incoming_frame_queue(void);
extern retval_t mac_hal_rx_decrypted_data_to_buffer(uint8_t *frame_ptr);
extern uint8_t mac_hal_hw_control(uint8_t state);
extern void mac_hal_get_current_time(uint32_t *current_time);
extern void mac_hal_get_current_time_symbols(uint32_t *current_time);
extern uint32_t mac_hal_add_time_symbols(uint32_t a, uint32_t b);
extern uint32_t mac_hal_sub_time_symbols(uint32_t a, uint32_t b);
extern void mac_hal_zigbee_interrupt_control(tinybool enable, 
                                             uint32_t interrupt_types);
extern void mac_hal_frame_pending(tinybool enable);
extern void mac_hal_mac_busy_recover();
/* From mac_data.c */
extern void mac_data_rx_frame_cb(frame_info_t *rx_frame);
extern retval_t mac_hal_reset(bool set_default_pib);
extern void mac_scan_ed_end_cb(uint8_t energy_level);
extern void mac_hal_promis_ack_control(tinybool enable);
extern void mac_hal_promis_ack_match_panid(tinybool enable);
extern void mac_hal_promis_ack_set_match_mode(uint8_t match_mode);
extern tinybool mac_hal_promis_ack_short_addr_add(tinybool src, 
                                                  uint16_t short_addr);
extern tinybool mac_hal_promis_ack_ext_addr_add(tinybool src, 
                                                uint64_t ext_addr);
extern tinybool mac_hal_promis_ack_short_addr_del(tinybool src,
                                                  uint16_t short_addr);
extern tinybool mac_hal_promis_ack_ext_addr_del(tinybool src,
                                                uint64_t ext_addr);
extern void mac_hal_promis_ack_table_show(void);
extern void mac_hal_bridge_config(uint8_t enable);
extern void mac_hal_sm_handler(void);
extern bool mac_hal_rx_enable(bool bc_on, 
                              mlme_rx_enable_req_t *rxe_p);
#ifdef Z_P_BRIDGE
void mac_hal_bridge_config(uint8_t enable);
#endif

#endif /* _MAC_HAL_H_ */
#endif //HYBRII_802154

