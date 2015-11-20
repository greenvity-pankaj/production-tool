/**
 * @file mac_hal.c
 *
 * This file implements the radio state machine and provides general
 * functionality used by the HAL.
 *
 * $Id: mac_hal.c,v 1.9 2014/11/26 13:19:41 ranjan Exp $
 *
 * Copyright (c) 2011, Greenvity Communication All rights reserved.
 *
 */
#ifdef HYBRII_802154
/* === INCLUDES ============================================================ */
#ifdef RTX51_TINY_OS
#include <rtx51tny.h>
#include "hybrii_tasks.h"
#endif
#include <stdio.h>
#include <string.h>
#include "papdef.h"
#include "timer.h"
#include "hal_common.h"
#include "gv701x_cfg.h"
#include "hybrii_802_15_4_regs.h"
#include "return_val.h"
#include "mac_const.h"
#include "bmm.h"
#include "qmm.h"
#include "mac_msgs.h"
#include "mac_data_structures.h"
#include "mac_hal.h"
#include "mac_internal.h"
#include "utils_fw.h"
#include "mac.h"
#include "mac_security.h"
#include "hpgpdef.h"
#include "fm.h"

/* === TYPES =============================================================== */

/* === MACROS ============================================================== */

/* === GLOBALS ============================================================= */

/*
 * Global HAL variables
 * These variables are only to be used by the HAL internally.
 */

/**
 * Current state of the HAL state machine.
 */
hal_state_t hal_state;

/**
 * Current state of the transceiver.
 */
tal_trx_status_t tal_trx_status;

/**
 * Indicates if the transceiver needs to switch on its receiver by hal_task(),
 * because it could not be switched on due to buffer shortage.
 */
bool hal_rx_on_required;

/**
 * Pointer to the 15.4 frame created by the HAL to be handed over
 * to the transceiver.
 */
uint8_t *tal_frame_to_tx;

/**
 * Pointer to receive buffer that can be used to upload a frame from the trx.
 */
buffer_t *hal_rx_buffer = NULL;

/**
 * Queue that contains all frames that are uploaded from the trx, but have not
 * be processed by the MCL yet.
 */
queue_t hal_incoming_frame_queue;

/**
 * Frame pointer for the frame structure provided by the MCL.
 */
frame_info_t *mac_frame_p;

/* Last frame length for IFS handling. */
uint8_t last_frame_length;

/* Flag indicating awake end irq at successful wake-up from sleep. */
volatile bool tal_awake_end_flag;

/**
 * Timestamp
 * The timestamping is only required for beaconing networks
 * or if timestamping is explicitly enabled.
 */
uint32_t tal_rx_timestamp;

/*
 * Flag indicating if beacon transmission is currently in progress.
 */
bool tal_beacon_transmission;

bool hybrii_mode = TRUE;

tx_mode_t hal_current_tx_mode;

static uint8_t hal_encrypt_key[16];
static uint8_t hal_decrypt_key[16];
static uint8_t hal_aes_ext_addr[8];

static uint8_t max_ed_level;
uint32_t scan_sample_counter;

#ifdef Z_P_BRIDGE
bool zb_plc_bridging = TRUE;
#endif

/* === PROTOTYPES ========================================================== */

/* === IMPLEMENTATION ====================================================== */

/**
 * Adds two time values
 *
 * This function adds two time values
 *
 * a Time value 1
 * b Time value 2
 *
 * return value of a + b
 */
uint32_t mac_hal_add_time_symbols (uint32_t a, uint32_t b)
{
    return ((a + b) & SYMBOL_MASK);
}

/**
 * Subtract two time values
 *
 * This function subtracts two time values taking care of roll over.
 *
 * a Time value 1
 * b Time value 2
 *
 * return value a - b
 */
uint32_t mac_hal_sub_time_symbols (uint32_t a, uint32_t b)
{
    if (a > b) {
        return ((a - b) & SYMBOL_MASK);
    } else {
        /* This is a roll over case */
        return (((MAX_SYMBOL_TIME - b) + a) & SYMBOL_MASK);
    }
}

void mac_hal_mac_busy_recover ()
{
    uint32_t status;

    status = hal_common_reg_32_read(ZigStatus);
    if (status & ZIG_STATUS_CP_TX_FIFO_EMPTY) {
        if (TRUE == mac_busy) {
            MAC_NOT_BUSY();
            hal_common_reg_bit_set(ZigCtrl,
                                   ZIG_CTRL_TX_SOFT_RESET);
            hal_common_reg_bit_clear(ZigCtrl,
                                     ZIG_CTRL_TX_SOFT_RESET);
            mac_hal_intf_tx_frame_done(MAC_SUCCESS,
                                       mac_frame_p);
        }
    }
}

#ifdef RTX51_TINY_OS
void hal_zb_pre_bc_tx_time_handler (void)
{
    if (TRUE == hal_pib_PrivatePanCoordinator) {
        mac_beacon_send_cb();
    }
}

void hal_zb_bc_tx_time_handler (void)
{
    mac_hal_get_current_time_symbols(&hal_pib_BeaconTxTime);
    mac_hal_pib_set(macBeaconTxTime, (void *)&hal_pib_BeaconTxTime);
}

void hal_zb_bc_tx_done_hadler (void)
{
#ifdef HYBRII_ASIC
#ifndef HYBRII_ASIC_A2
#ifndef HYBRII_FPGA_A2
    gv701x_cfg_zb_afe_set_vco_rx(hal_pib_CurrentChannel);
#endif
#endif
#endif
    mac_hal_intf_tx_frame_done(HAL_BC_TX_DONE, NULL);
    hal_state = HAL_IDLE;
}

void hal_zb_tx_done_handler (void)
{
    retval_t tx_status;

    MAC_NOT_BUSY();
#ifdef HYBRII_ASIC
#ifndef HYBRII_ASIC_A2
#ifndef HYBRII_FPGA_A2
    gv701x_cfg_zb_afe_set_vco_rx(hal_pib_CurrentChannel);
#endif
#endif
#endif
#ifdef Z_P_BRIDGE
    if (NULL == mac_frame_p && FALSE == zb_plc_bridging) {
#else
    if (NULL == mac_frame_p) {
#endif
        //printf("\nInvalid mac_frame_p\n");
        return;
    }
    /* 
     * Read the timestamp. In non-beacon network, Beacon Counter is
     * a free running counter
     */
    mac_hal_get_current_time_symbols(&mac_frame_p->time_stamp);
    tx_status = hal_common_bit_field_reg_read(ZIG_TX_STATUS);
    switch (tx_status) {
    case TX_STATUS_NO_ACK:
        tx_status = MAC_NO_ACK;
        break;
    case TX_STATUS_OK_DATA_PENDING:
        tx_status = HAL_FRAME_PENDING;
        mac_stats_g.frame_pending++;
        break;
    case TX_STATUS_OK:
        tx_status = MAC_SUCCESS;
        break;
    case TX_STATUS_NO_CCA:
    case TX_STATUS_HANG_RECOVERED:
    default:
        /*
         * tx_status is defined in IEEE 802.15.4 spec.
         * There are not enough error code for each failure cases.          * Thus, all failure cases are treated as CCA failure.
         * H/W does keep track statistics for each failure case.
         */
        tx_status = MAC_CHANNEL_ACCESS_FAILURE;
        break;
    }
    mac_hal_intf_tx_frame_done(tx_status, mac_frame_p);
    mac_frame_p = NULL;
}
#else

static void mac_hal_handler_beacon_interrupt (uint32_t value)
{
    if (value & CPU_INT_ZB_PRE_BC_TX_TIME) {
        hal_zb_pre_bc_tx_time_handler();
    }

    if (value & CPU_INT_ZB_BC_TX_TIME) {
        hal_zb_bc_tx_time_handler();
    }
}

static void mac_hal_handle_zigbee_irq (void)
{    
    uint32_t value;

    EA = 0;
    value = hal_common_reg_32_read(CPU_INTSTATUS_REG);
    hal_common_reg_32_write(CPU_INTSTATUS_REG, value);
    EA = 1;

#ifndef _EXT_INT_ENABLE_
    mac_hal_handler_beacon_interrupt(value);
#endif

    if (value & CPU_INT_ZB_BC_TX_DONE) {
        hal_zb_bc_tx_done_hadler();
    }

    if (value & CPU_INT_ZB_RX_CRC_ERR) {
        /* Keeping statistics */
    }
   
    if (value & CPU_INT_ZB_TX_DONE) {
        hal_zb_tx_done_handler();      
    }

    if (value & CPU_INT_ZB_RX_DONE) {
        if (hybrii_mode) {
            CHAL_CpuTxQNemptyIntHandler();
        } else {
            /*
             * For Greenlite mode, pull the packet directly from RX FIFO
             */
            mac_hal_poll_rx_fifo_for_packet();
        }
    }
    
    if (value & CPU_INT_ZB_TX_DROP_NO_ACK) {
        /* Keeping statistics */
    }
}

/**
 * MAC interrupt handler
 *
 * This function handles the Zigbee MAC generated interrupts.
 * trx_irq_handler_cb
 */
void mac_hal_irq_handler (void)
{
    /* 
     * FIXME - Will move this function to hal_common.c
     * Need to read Interrupt status and call the appropriate
     * ISR Handling routine
     */
    mac_hal_handle_zigbee_irq();
}
#endif

void mac_hal_ed_read ()
{
    uint8_t retry = 0;
    uint8_t ed_level;

    if (scan_sample_counter) {
        scan_sample_counter--;
    }
    if (0 == scan_sample_counter) {
        hal_state = HAL_ED_DONE;

        hal_common_reg_8_bit_clear(PHY_SCAN_CONTROL, PHY_SCAN_START);
        while (hal_common_reg_8_bit_test(PHY_RSSI_REPORT_STATUS, 
                                         PHY_RSSI_REPORT_VLD) == false) {
            if (retry++ > 50) {
                return;
            }
        }
        ed_level = ReadU8Reg(PHY_SCAN_RSSI_REPORT);
        if (max_ed_level < ed_level) {
            max_ed_level = ed_level;
        }
    }
}

void mac_hal_ed_start (uint8_t scan_duration)
{
	hal_state = HAL_IDLE;
    if (HAL_IDLE != hal_state) {
        return;
    }

    hal_state = HAL_ED_RUNNING;

    max_ed_level = 0;

    scan_sample_counter = SCAN_DURATION_TO_SYMBOLS(scan_duration) / 
                          ED_SAMPLE_DURATION_SYM;
    hal_common_reg_8_bit_set(PHY_SCAN_CONTROL, PHY_SCAN_START);
}

static void mac_hal_set_tx_mode (tx_mode_t tx_mode)
{
    if (tx_mode == NO_CSMA) {
        if (hal_current_tx_mode != tx_mode) {
            hal_common_bit_field_reg_write(ZIG_TX_BKOFF_ENGINE_DIS, TRUE);
            hal_current_tx_mode = tx_mode;
        }
    } else {
        if (hal_current_tx_mode == NO_CSMA) {
            hal_common_bit_field_reg_write(ZIG_TX_BKOFF_ENGINE_DIS, FALSE);
        }
        if (tx_mode == CSMA_UNSLOTTED) {
            if (hal_current_tx_mode != tx_mode) {
                hal_common_bit_field_reg_write(ZIG_NON_BEACON_NWK, TRUE);
                hal_current_tx_mode = tx_mode;
            }
        } else {
            if (hal_current_tx_mode != tx_mode) {
                hal_common_bit_field_reg_write(ZIG_NON_BEACON_NWK, FALSE);
                hal_current_tx_mode = tx_mode;
            }
        }
    }
}

static bool mac_aes_available (uint32_t *aes_reg_value)
{
    uint16_t retry;

    retry = 0;
    while (1) {
        *aes_reg_value = hal_common_reg_32_read(ZigAESKeyAddr);
        if (*aes_reg_value & ZIG_AES_ENCRYPT_EN ||
            *aes_reg_value & ZIG_AES_DECRYPT_EN) {
            if (retry ++ > ZIGBEE_MAC_RETRY_MAX) {
                //printf("\nAES is busy\n");
                return (FALSE);
            }
        } else {
            break;
        }
    }

    return (TRUE);
}

static retval_t mac_hal_encrypt_enable (tinybool use_beacon_template)
{
    uint32_t value;

    if (FALSE == mac_aes_available(&value)) {
        return (MAC_TX_ACTIVE);
    }

#ifdef _BC_SECURE_TEST_
{
#ifdef ZBMAC_DIAG
    extern tinybool use_bc_template_g;
    
    use_beacon_template = use_bc_template_g;
#endif	
}
#endif

    value |= ZIG_AES_ENCRYPT_EN;
    if (TRUE == use_beacon_template) {
        /* 
         * Indicate to MAC ASIC that the 
         * packet will be put in Beacon Template and send according
         * to beacon interval.
         */
         value &= ~ZIG_AES_BEACON_TO_TX_FIFO;
    } else {
        /* 
         * Indicate to MAC ASIC that the 
         * packet will be sending right away after encrypt
         * This is needed as the beacon will be send in unslotted CSMA
         * S/W set this blinding regardless of packet types but the ASIC
         * only look at this info for beacon packets.
         */
         value |= ZIG_AES_BEACON_TO_TX_FIFO;
    }
    hal_common_reg_32_write(ZigAESKeyAddr, value);

    return (MAC_SUCCESS);
}

static retval_t mac_hal_decrypt_enable ()
{
    uint32_t value;

    if (FALSE == mac_aes_available(&value)) {
        return (MAC_FAILURE);
    }

    value |= ZIG_AES_DECRYPT_EN;
    hal_common_reg_32_write(ZigAESKeyAddr, value);

    return (MAC_SUCCESS);
}

retval_t mac_hal_write_frame_to_fifo (uint8_t *frame_p, uint8_t frame_len,
                                      hal_aes_t aes_type_e,
                                      tinybool use_beacon_template)
{
    uint8_t  byte_count;
    uint8_t  offset;
    uint16_t retry;
    retval_t status        = MAC_SUCCESS;
    uint32_t tx_fifo_reg   = ZigTxRxFifo;
    uint32_t tx_status_reg = ZigStatus;
    uint32_t tx_status_bit = ZIG_STATUS_CP_TX_FIFO_EMPTY;
		
    if (AES_NONE != aes_type_e) {
        if (AES_DECRYPT == aes_type_e) {
            status = mac_hal_decrypt_enable();
        } else {
            status = mac_hal_encrypt_enable(use_beacon_template);
        }
        tx_fifo_reg   = ZigAESTxRxFifo;
        tx_status_reg = ZigTxBlockEn;
        tx_status_bit = ZIG_BLK_AES_IN_FIFO_EMPTY;
    }

    if (MAC_SUCCESS != status) {
        return (status);
    }

    retry = 0;
    while (1) {
        if (hal_common_reg_bit_test(tx_status_reg, tx_status_bit)) {
            break;
        }

        if (retry ++ > ZIGBEE_MAC_RETRY_MAX) {
            return (MAC_TX_ACTIVE);
        }
    }
#ifdef HYBRII_ASIC
    if (AES_DECRYPT != aes_type_e) {
#ifndef HYBRII_ASIC_A2
#ifndef HYBRII_FPGA_A2
        gv701x_cfg_zb_afe_set_vco_tx(hal_pib_CurrentChannel);
#endif
#endif
    }
#endif
    WriteU8Reg(tx_fifo_reg, frame_len);  /* Write frame len */
    byte_count = 1;
    while (byte_count <= frame_len) {
        offset = byte_count++ % 4;
        WriteU8Reg(tx_fifo_reg + offset, *frame_p++);
    }
    /* Pad to word (4 bytes)  boundary */
    for (byte_count = offset + 1; byte_count < 4; byte_count++) {
        WriteU8Reg(tx_fifo_reg + byte_count, 0);
    }

    if (AES_DECRYPT != aes_type_e) {
        hal_state = HAL_SLOTTED_CSMA;
    }

    if (AES_ENCRYPT == aes_type_e) {
        /* 
         * FIXME - Call routine from MAC
         * to increment the FrameCounter
         */
        mac_sec_pib.FrameCounter++;
    }
    return (MAC_SUCCESS);
}


/**
 * Requests to HAL to transmit frame
 *
 * tx_frame_p - Pointer to the frame_info_t
 *
 * Return:
 * MAC_SUCCESS - if the HAL has accepted the data from the MAC for frame
 *               transmission
 */
retval_t mac_hal_tx_frame (frame_info_t *tx_frame_p, tx_mode_t tx_mode,
                           tinybool encryption)
{
    uint8_t   *frame_p;
    uint8_t   frame_len;
    retval_t  status = MAC_SUCCESS;
    hal_aes_t aes_type = AES_NONE;
    /*
     * Save the tx frame into global variable for so we can send confirmation
     * when the packet is sent
     */
    mac_frame_p = tx_frame_p;

    frame_p = tx_frame_p->mpdu_p;

    if (TRUE == encryption) {
        aes_type = AES_ENCRYPT;
    }

    /*
     * The 1st byte of the frame is the frame length
     */
    frame_len = *frame_p++;    
    if (frame_len <= aMaxPHYPacketSize) {
        if (tx_mode != GTS_SLOTTED) {
            mac_hal_set_tx_mode(tx_mode); 
            status = mac_hal_write_frame_to_fifo(frame_p, frame_len,
                                                 aes_type, FALSE);
        } else {
            /* FIXME - Add code to support GTS TX when HW supports it */
        }
    } else {
        status = MAC_FRAME_TOO_LONG;
    }

    return (status);
}

/**
 * Requests to HAL to transmit beacon frame
 *
 * tx_frame_p - Pointer to the frame_info_t
 *
 * Return:
 * MAC_SUCCESS - if the HAL has accepted the data from the MAC for frame
 *               transmission
 */
retval_t mac_hal_tx_beacon (frame_info_t *tx_frame_p, tinybool encrypt)
{
    uint8_t  *frame_p;
    uint8_t  frame_len;
    uint8_t  idx;
    uint8_t  value;

    retval_t status = MAC_SUCCESS;

    value = ReadU8Reg(ZigBeaconTemplate);
    if ((value & ZIG_BC_TEMPLATE_VALID) != 0) {
        return (MAC_FAILURE);
    }

    frame_p = tx_frame_p->mpdu_p;
    /*
     * The 1st byte of the frame is the frame length
     */
    frame_len = *frame_p & 0x7F;
    if (FALSE == encrypt) {
#ifdef HYBRII_ASIC
#ifndef HYBRII_ASIC_A2
#ifndef HYBRII_FPGA_A2
        gv701x_cfg_zb_afe_set_vco_tx(hal_pib_CurrentChannel);
#endif
#endif
#endif    
        WriteU8Reg(ZigBeaconTemplate, frame_len);

        for (idx = 1; idx <= frame_len; idx++) {
            WriteU8Reg(ZigBeaconTemplate + idx, frame_p[idx]);
        }
        WriteU8Reg(ZigBeaconTemplate,
                   frame_len | ZIG_BC_TEMPLATE_VALID);
    } else {
        status = mac_hal_write_frame_to_fifo(&frame_p[1], frame_len,
                                             AES_ENCRYPT, TRUE);
    }

    return (status);
}

/**
 * Resets HAL TX, RX and sets the default PIB values if requested
 *
 */
retval_t mac_hal_reset (bool set_default_pib)
{
    /*
     * Do the reset stuff.
     * Set the default PIBs depending on the given parameter set_default_pib.
     * Do NOT generate random seed again.
     */
    if (set_default_pib == TRUE) {
        mac_hal_pib_init();
    }

    mac_hal_pib_write_to_asic();

    return MAC_SUCCESS;
}

static void mac_hal_hw_init (void)
{
    /*
     * Initialize ASIC implement specific registers
     */
    hal_common_bit_field_reg_write(ZIG_CLK_PER_USECS, CLOCKS_PER_USECS);
    hal_common_bit_field_reg_write(ZIG_CURRENT_BIT_RATE, 100);
    hal_common_bit_field_reg_write(ZIG_US_PER_SYMBOL, US_PER_SYMBOL);
}

void mac_hal_zig_tx_rx_control (uint8_t enable)
{
    u32 xdata value32;

    value32 = ZIG_CTRL_TX_EN | ZIG_CTRL_RX_EN | ZIG_CTRL_ACK_EN;
    if (enable) {
        hal_common_reg_bit_set(ZigCtrl, value32);
    } else {
        hal_common_reg_bit_clear(ZigCtrl, value32);
    }
}

/**
 * HAL Init
 *
 */
void mac_hal_init (void)
{
    mac_frame_p = NULL;
    hal_state = HAL_IDLE;
    hal_rx_on_required = FALSE;
    hal_current_tx_mode = NO_CSMA;   // Should read Control Reg to set this
    mac_hal_hw_init();
    mac_hal_reset(TRUE);
    bmm_buffer_init();
    memset(hal_encrypt_key, 0, 16);
    memset(hal_decrypt_key, 0, 16);
    hal_rx_buffer = bmm_buffer_alloc(BUFFER_SIZE);
    if (NULL == hal_rx_buffer) {
        hal_rx_on_required = TRUE;
        printf("\nError - Cannot allocate RX buffer");
        return;
    }
    qmm_queue_init(&hal_incoming_frame_queue,
                   HAL_INCOMING_FRAME_QUEUE_CAPACITY);

    gv701x_cfg_zb_afe_init(hal_pib_CurrentChannel, TRUE);
#ifdef HYBRII_ASIC
    gv701x_cfg_zb_phy_init();
#endif

    mac_hal_zig_tx_rx_control(TRUE);
#ifdef Z_P_BRIDGE
    mac_hal_bridge_config(TRUE);
#endif

#ifdef RTX51_TINY_OS
#ifdef HAL_802154_TASK
    os_create_task(HAL_802154_TASK_ID);
#endif
#endif
}

#ifdef Z_P_BRIDGE
extern void hpgp_pkt_bridge (sCommonRxFrmSwDesc* pRxFrmDesc, u8 frmLen);
#endif

void mac_hal_qc_frame_rx (sCommonRxFrmSwDesc* rx_frame_info_p)
{
    uint8_t       frame_len;
    frame_info_t  *receive_frame_p;
    uint8_t       *frame_ptr;
    uint8_t xdata *cp_addr_p;
    uint8_t       byte_count;
    uint8_t       i, j;
    uint8_t       num_of_read;
    uint8_t       zb_pkt_size;
#ifdef _CRC_VERIFY_
    uint8_t       bytes_to_verify; 
    uint8_t       crc_byte_count;  
    uint8_t       *crc_data_p;
    uint16_t      crc16;
#endif
    frame_len = (rx_frame_info_p->hdrDesc.s.frmLenHi << 
                 PKTQDESC1_FRMLENHI_POS) | 
                rx_frame_info_p->hdrDesc.s.frmLenLo;

    if (frame_len > aMaxPHYPacketSize + 6) { // Include LQI, RSSI and 4 timestamp bytes
        mac_stats_g.rx_frame_too_big++;
        hal_common_free_frame(rx_frame_info_p);
        return;
    }

#ifdef Z_P_BRIDGE
    if (zb_plc_bridging) {
        mac_stats_g.rx_pkts_count++;
        mac_stats_g.rx_bytes_count += (frame_len - 6);
        hpgp_pkt_bridge(rx_frame_info_p, frame_len - 8);
        return;
    }
#endif

    if (NULL == hal_rx_buffer) {
        mac_stats_g.rx_no_buffer++;
        hal_common_free_frame(rx_frame_info_p);
        hal_rx_buffer = bmm_buffer_alloc(BUFFER_SIZE);

		if (hal_rx_buffer== NULL) {
			FM_Printf(FM_APP, "\nba1.0");
		}
        hal_rx_on_required = TRUE;
#ifdef RTX51_TINY_OS
#ifdef MAC_802154_TASK
#ifdef HAL_802154_TASK			
		os_set_ready(HAL_802154_TASK_ID);
#endif			
#else
		os_set_ready(HPGP_TASK_ID_CTRL);
#endif /* MAC_802154_TASK */
#ifdef UM
		pending_802154_task = TRUE;
#endif
#endif
        return;
    }

    receive_frame_p = (frame_info_t*)BMM_BUFFER_POINTER(hal_rx_buffer);
    /*
     * Need to reserve additional 3 bytes since we always read
     * a complete word if the len is not word boundary
     */
    frame_ptr = (uint8_t *)receive_frame_p + BUFFER_SIZE - frame_len -
                           LENGTH_FIELD_LEN - 3;
    receive_frame_p->mpdu_p = frame_ptr;
    /* 1st byte is the length which include FCS but not LQI, RSSI and timestamp */
    zb_pkt_size  = frame_len - LQI_LEN - ED_VAL_LEN - 4;   // 4 bytes timestamp
    *frame_ptr++ = zb_pkt_size;
    
    byte_count = MIN(frame_len, HYBRII_CELLBUF_SIZE);
    cp_addr_p = CHAL_GetAccessToCP(rx_frame_info_p->cpArr[0]);
    // FIXME printf("\n");
    num_of_read = (byte_count + 3) / sizeof(uint32_t);
#ifdef _CRC_VERIFY_
    bytes_to_verify = zb_pkt_size;
    crc_byte_count  = 0;
    crc16           = 0;
#endif

    for (i = 0; i < num_of_read; i++) {
        (uint32_t)(*frame_ptr) = ReadU32Reg(cp_addr_p + (i * 4));
#ifdef _CRC_VERIFY_
        crc_data_p = frame_ptr;
        for (j = 0; j < 4; j++) {
            if (crc_byte_count++ < bytes_to_verify) {
                crc16 = crc_ccitt_update(crc16, *crc_data_p++);
            }
        }
#endif
        // FIXME printf("%08lx ", *((u32 *)frame_ptr));
        frame_ptr += 4;
    }

    if (frame_len > HYBRII_CELLBUF_SIZE) {
        uint8_t bytes_cp;
        
        bytes_cp = frame_len - HYBRII_CELLBUF_SIZE;
        cp_addr_p = CHAL_GetAccessToCP(rx_frame_info_p->cpArr[1]);
         
        for (j = 0; j < bytes_cp; j++) {
            *frame_ptr++ = ReadU8Reg(cp_addr_p + j);
        }
    }

    /*
     * Timestamp is at after 2 bytes of LQI and RSSI. We also need to add 1 more byte
     * since length of the packet is the 1st byte in mpdu_p
     */
    receive_frame_p->time_stamp = 
        mac_utils_byte_array_to_32_bit(&receive_frame_p->mpdu_p[zb_pkt_size + 3]);
    hal_common_free_frame(rx_frame_info_p);
#ifdef _CRC_VERIFY_
    //if (crc16 != 0 || frame_len != 133) {
    if (crc16 != 0) {
        mac_stats_g.rx_bad_crc++;
        if (false == hal_pib_PromiscuousMode) {            
            for (i = 0; i < frame_len; i++) {
                //printf("%02bx ", receive_frame_p->mpdu_p[i+1]);
            }
            //printf("\n");
            return;
        }
    }
#endif 

    /* Append received frame to incoming_frame_queue and get new rx buffer. */
    qmm_queue_append(&hal_incoming_frame_queue, hal_rx_buffer);
#ifdef RTX51_TINY_OS

#ifdef HAL_802154_TASK
    os_set_ready(HAL_802154_TASK_ID);
#else
#ifdef MAC_802154_TASK
	os_set_ready(MAC_802154_TASK_ID);
#else
	os_set_ready(HPGP_TASK_ID_CTRL);
#endif /* MAC_802154_TASK */
#ifdef UM
	pending_802154_task = TRUE;
#endif
#endif
#endif
    /* The previous buffer is eaten up and a new buffer is not assigned yet. */
    hal_rx_buffer = bmm_buffer_alloc(BUFFER_SIZE);

    /* Check if receive buffer is available */
    if (NULL == hal_rx_buffer) {
        /*
         * FIXME - Turn off the Rx Transiver until we can allocate a RX 
         * buffer
         */
		FM_Printf(FM_APP, "\nba1.1");
        hal_rx_on_required = TRUE;
    }
}

/**
 *
 * trx_cmd - the trx commands
 *
 * return current trx state
 */
tal_trx_status_t mac_hal_trx_state (trx_cmd_t trx_cmd)
{
    trx_cmd = trx_cmd;
    
    return (RX_ON);
}

/**
 *
 */
uint8_t mac_hal_hw_control (uint8_t command)
{
    switch (command) {
    case PHY_TRX_OFF:
        hal_common_reg_bit_clear(ZigCtrl, ZIG_CTRL_TX_EN);
        hal_common_reg_bit_clear(ZigCtrl, ZIG_CTRL_RX_EN);
        break;
    case PHY_RX_ON:
        hal_common_reg_bit_set(ZigCtrl, ZIG_CTRL_RX_EN |
                                        ZIG_CTRL_ACK_EN);
        break;
    case PHY_TX_ON:
        hal_common_reg_bit_set(ZigCtrl, ZIG_CTRL_TX_EN);
        break;
    default:
        break;
    }

    return (command);
}

/**
 *
 */
void mac_hal_get_current_time_symbols (uint32_t *current_time)
{
    /*
     * 24-bit value per 802.15.4 spec
     */
    *current_time = hal_common_reg_32_read(ZigBeaconCounter) & 
                    0x00FFFFFF;
}

/**
 *
 */
void mac_hal_get_current_time (uint32_t *current_time)
{
    /*
     * Set TIMER READ ENABLE bit in order to read a free
     * running counter as the beacon counter and free
     * running counter are overloaded in h/w register
     * address.
     */
    hal_common_reg_bit_set(ZigTxBlockEn, ZIG_BLK_TIMER_READ_ENABLE);
    *current_time = hal_common_reg_32_read(ZigBeaconCounter) & 
                    0x00FFFFFF;
    hal_common_reg_bit_clear(ZigTxBlockEn, ZIG_BLK_TIMER_READ_ENABLE);
}

/**
 * This function parses the received frame and creates the frame_info_t
 * structure to be sent to the MAC as a parameter of hal_rx_frame_cb().
 *
 * buf_ptr - Pointer to the buffer containing the received frame
 */
void mac_hal_process_incoming_frame (buffer_t *buf_ptr)
{
    frame_info_t *receive_frame_p = (frame_info_t*)BMM_BUFFER_POINTER(buf_ptr);
   
    receive_frame_p->buffer_header_p = buf_ptr;
    mac_data_rx_frame_cb(receive_frame_p);
}

retval_t mac_hal_aes_key_config (hal_aes_direction_t aes_dir_e, uint8_t *key_p,
                                 uint8_t *ext_addr_p)
{
    uint8_t  i;
    uint32_t *var_32_p;
    tinybool is_current_key;
    uint8_t  *key_type;
    uint8_t  key_addr;
    uint16_t retry;
    uint32_t value;
    retval_t status;

    status = MAC_SUCCESS;
    is_current_key = FALSE;
    if (HAL_AES_ENCRYPT == aes_dir_e) {
        if (0 == memcmp(hal_encrypt_key, key_p, 16)) {
            is_current_key = TRUE;
        } else {
            key_type = hal_encrypt_key;
            key_addr = ZIG_AES_ENCRYPT_KEY_ADDR;
        }
    } else {
        if (0 == memcmp(hal_decrypt_key, key_p, 16)) {
            is_current_key = TRUE;
        } else {
            key_type = hal_decrypt_key;
            key_addr = ZIG_AES_DECRYPT_KEY_ADDR;
        }
    }
    if (FALSE == is_current_key) {
        if (FALSE == mac_aes_available(&value)) {
            /*
             * Cannot change key when AES is busy
             */
            return (MAC_FAILURE);
        }
        var_32_p = (uint32_t *)key_p;
        for (i = 0; i < 4; i++) {
            hal_common_bit_field_reg_write(ZIG_AES_KEY_ADDR, i + key_addr);
            WriteU32Reg(ZigAESKeyData, *var_32_p++);
            hal_common_reg_bit_set(ZigAESKeyAddr, ZIG_AES_KEY_ADDR_WRITE_EN);
            retry = 0;
            while (hal_common_reg_bit_test(ZigAESKeyAddr,
                                           ZIG_AES_KEY_ADDR_WRITE_EN)) {
                if (retry ++ > ZIGBEE_MAC_RETRY_MAX) {
                    status = MAC_FAILURE;
                    break;
                }
            }
        }
        if (MAC_SUCCESS == status) {
            memcpy(key_type, key_p, 16);  /* Update the local key */
        }
    }

    if (HAL_AES_DECRYPT == aes_dir_e &&
	    memcmp(hal_aes_ext_addr, ext_addr_p, 16)) {
        var_32_p = (uint32_t *)ext_addr_p;

        key_addr = ZIG_AES_DECRYPT_EXT_ADDR;
        for (i = 0; i < 2; i++) {
            hal_common_bit_field_reg_write(ZIG_AES_KEY_ADDR, i + key_addr);
            WriteU32Reg(ZigAESKeyData, *var_32_p++);
            hal_common_reg_bit_set(ZigAESKeyAddr, ZIG_AES_KEY_ADDR_WRITE_EN);
            retry = 0;
            while (hal_common_reg_bit_test(ZigAESKeyAddr,
                                           ZIG_AES_KEY_ADDR_WRITE_EN)) {
                if (retry ++ > ZIGBEE_MAC_RETRY_MAX) {
                    status = MAC_FAILURE;
                    break;
                }
            }
        }
        if (MAC_SUCCESS == status) {
            memcpy(hal_aes_ext_addr, ext_addr_p, 8);
        }
    }
    return (status);
}

void mac_hal_sm_handler (void)
{
    /* Handle the HAL state machines */
    switch (hal_state) {
    case HAL_IDLE:
        break;
    case HAL_ED_RUNNING:
        mac_hal_ed_read();
        break;
    case HAL_ED_DONE:
        hal_state = HAL_IDLE; 
        mac_scan_ed_end_cb(max_ed_level);       
        break;
    default:
        break;
    }
}

/**
 * This function
 * - Checks and allocates the receive buffer.
 * - Processes the HAL incoming frame queue.
 * - Implements the HAL state machine.
 */
void mac_hal_task (void)
{
    /* Check if the receiver needs to be switched on. */
    if (hal_rx_on_required && (hal_state == HAL_IDLE)) {
        /* Check if a receive buffer has not been available before. */
        if (NULL == hal_rx_buffer) { 
            hal_rx_buffer = bmm_buffer_alloc(BUFFER_SIZE);
			if(!hal_rx_buffer) {
				FM_Printf(FM_APP, "\nba:halF");
			}
        }
        if (NULL != hal_rx_buffer) {
            hal_rx_on_required = FALSE;
            if (hal_pib_PromiscuousMode) {
                /* FIXME - Turn on RX, disable ACK */
            } else {
                /* FIXME - Turn on RX, enable ACK */
            }
        }
    }

    /*
     * If the transceiver has received a frame and it has been placed
     * into the queue of the HAL, the frame needs to be processed further.
     */
    while (hal_incoming_frame_queue.size > 0) {
        buffer_t *rx_frame_p; 
		FM_Printf(FM_APP, "\nrxQ: %bu", hal_incoming_frame_queue.size);
        /* Check if there are any pending data in the incoming_frame_queue. */
        rx_frame_p = qmm_queue_remove(&hal_incoming_frame_queue, NULL);
        if (NULL != rx_frame_p) { 
            mac_hal_process_incoming_frame(rx_frame_p);
        }
    }
}

#ifdef MAC_802154_TASK
#ifdef RTX51_TINY_OS
#ifdef HAL_802154_TASK
void os_mac_hal_task (void) _task_ HAL_802154_TASK_ID
{
    while (TRUE) {
		os_switch_task();
        mac_hal_task();
    }
}
#endif
#endif
#endif
 
void mac_hal_flush_incoming_frame_queue (void)
{
    qmm_queue_flush(&hal_incoming_frame_queue);
    
    if (NULL == hal_rx_buffer) {
        hal_rx_buffer = bmm_buffer_alloc(BUFFER_SIZE);
    }
}

retval_t mac_hal_rx_decrypted_data_to_buffer (uint8_t *frame_ptr)
{
    volatile uint32_t reg_value;
    uint8_t           frame_len;
    uint8_t           i;
    retval_t          status;
    uint16_t          retry;

    retry = 0;
    while (1) {
        uint32_t decrypt_status;

        decrypt_status = hal_common_reg_32_read(ZigAESKeyAddr);
        if (0 == (decrypt_status & ZIG_AES_DECRYPT_EN)) {
            if (decrypt_status & ZIG_AES_DECRYPT_FAILURE) {
                return (MAC_FAILURE);
            } else if (decrypt_status & ZIG_AES_DECRYPT_SUCCESS) {
                if (hal_common_reg_bit_test(ZigTxBlockEn, 
                                    ZIG_BLK_AES_OUT_FIFO_EMPTY) == FALSE) {
                    break;
                }
            }
        }


        if (retry ++ > ZIGBEE_MAC_RETRY_MAX) {
            //printf("\nNo Decrypt Stat\n");
            return (MAC_FAILURE);
        }
    }

    status = MAC_FAILURE;

    reg_value = hal_common_reg_32_read(ZigAESTxRxFifo);
#ifdef _AES_DEBUG_    
    printf("\nreg_value = %08lx\n", reg_value);
#endif    
    frame_len = BIT_FIELD_GET(reg_value, CPU_TXQDESC_FRAME_LEN_MASK,
                              CPU_TXQDESC_FRAME_LEN_POS);
    if (frame_len > (aMaxPHYPacketSize - FCS_LEN)) {
        /*
         * Frame len is invalid but still need to empty the fifo
         */
        mac_stats_g.rx_frame_too_big++;
        frame_len = aMaxPHYPacketSize;
    } else {
        if (0 == frame_len) {
            //printf("\nLen is 0\n");
        }
        status = MAC_SUCCESS;
    }                                           
        
    if (frame_ptr) {
        uint8_t num_of_read;

        *frame_ptr++ = frame_len;
        num_of_read = (frame_len + 3) / sizeof(uint32_t);
#ifdef _AES_DEBUG_        
        printf("\nlen = %d\n", (uint16_t)frame_len);
#endif
        for (i = 0; i < num_of_read; i++) {
            (u32)(*frame_ptr) = ReadU32Reg(ZigAESTxRxFifo);
#ifdef _AES_DEBUG_
            printf("%08lx ", *((u32 *)frame_ptr));
#endif
            frame_ptr += 4;
        }
#ifdef _AES_DEBUG_
        printf("\n");
#endif        
    }

    return (status);
}

void mac_hal_poll_rx_fifo_for_packet (void)
{
    volatile     u32 reg_value;
    uint8_t      frame_len;
    uint8_t      i;
    uint8_t      num_of_read;
    frame_info_t *receive_frame_p;
    uint8_t      *frame_ptr;
    uint8_t      zb_pkt_size;
#ifdef _CRC_VERIFY_
    uint8_t      bytes_to_verify;
    uint8_t      crc_byte_count;
    uint8_t      j;
    uint8_t      *crc_data_p;
    uint16_t     crc16;
#endif

    if (hal_common_reg_bit_test(ZigStatus, ZIG_STATUS_RX_FIFO_AVAIL)) {
        reg_value = hal_common_reg_32_read(ZigTxRxFifo);
        frame_len = BIT_FIELD_GET(reg_value, CPU_TXQDESC_FRAME_LEN_MASK,
                                  CPU_TXQDESC_FRAME_LEN_POS);
        /*
         * The frame_len has LQI, RSSI and 4 bytes timestamp
         */
        if (frame_len > aMaxPHYPacketSize + 6) {
            mac_stats_g.rx_frame_too_big++;
        }
        
        if (hal_rx_buffer) {
            receive_frame_p = (frame_info_t*)BMM_BUFFER_POINTER(hal_rx_buffer);
            /*
             * Need to reserve additional 3 bytes since we always read
             * a complete word if the len is not word boundary
             */
            frame_ptr = (uint8_t *)receive_frame_p + BUFFER_SIZE - frame_len -
                           LENGTH_FIELD_LEN - 3;
            receive_frame_p->mpdu_p = frame_ptr;
            /* 
             * 1st byte is the length which include FCS but not LQI, RSSI
             * and 4 bytes timestamp
             */
            zb_pkt_size  = frame_len - LQI_LEN - ED_VAL_LEN - 4;
            *frame_ptr++ = zb_pkt_size;

            // FIXME printf("\n");
            num_of_read = (frame_len + 3) / sizeof(uint32_t);
#ifdef _CRC_VERIFY_
            bytes_to_verify = zb_pkt_size;
            crc_byte_count  = 0;
            crc16           = 0;
#endif
            for (i = 0; i < num_of_read; i++) {
                (uint32_t)(*frame_ptr) = ReadU32Reg(ZigTxRxFifo);
#ifdef _CRC_VERIFY_
                crc_data_p = frame_ptr;
                for (j = 0; j < 4; j++) {
                    if (crc_byte_count++ < bytes_to_verify) {
                        crc16 = crc_ccitt_update(crc16, *crc_data_p++);
                    }
                }
#endif
                // FIXME printf("%lx ", *((u32 *)frame_ptr));
                frame_ptr += 4;
            }

#ifdef _CRC_VERIFY_
            if (crc16 != 0) {
                mac_stats_g.rx_bad_crc++;
                if (false == hal_pib_PromiscuousMode) {
                    printf("\nBad CRC\n");
                    return;
                }
            }
#endif
            /*
             * Timestamp is at after 2 bytes of LQI and RSSI. We also need 
             * to add 1 more byte since length of the packet is the 1st byte
             * in mpdu_p
             */
            receive_frame_p->time_stamp = 
                mac_utils_byte_array_to_32_bit(&receive_frame_p->mpdu_p[zb_pkt_size + 3]);
            /*
             * Append received frame to incoming_frame_queue and 
             * get new rx buffer.
             */
            if (FALSE == qmm_queue_append(&hal_incoming_frame_queue, 
                hal_rx_buffer)) {
                bmm_buffer_free(hal_rx_buffer);
            } else {
#ifdef RTX51_TINY_OS
#ifdef HAL_802154_TASK
                os_set_ready(HAL_802154_TASK_ID);
#else
#ifdef MAC_802154_TASK
				os_set_ready(MAC_802154_TASK_ID);
#else
				os_set_ready(HPGP_TASK_ID_CTRL);
#endif /* MAC_802154_TASK */
#ifdef UM
				pending_802154_task = TRUE;
#endif
#endif
#endif

            }
            hal_rx_buffer = bmm_buffer_alloc(BUFFER_SIZE);

            /* Check if receive buffer is available */
            if (NULL == hal_rx_buffer) {
                /*
                 * FIXME - Turn off the Rx Transiver until we can allocate a RX
                 * buffer
                 */
                hal_rx_on_required = TRUE;
            }
        } else {
            mac_stats_g.rx_no_buffer++;
        }
    }
}

void mac_hal_zigbee_interrupt_control (tinybool enable,
                                       uint32_t interrupt_types)
{
    if (enable) {
        hal_common_reg_bit_set(CPU_INTENABLE_REG, interrupt_types);
    } else {
        hal_common_reg_bit_clear(CPU_INTENABLE_REG, interrupt_types);
    }
}

void mac_hal_frame_pending (tinybool enable)
{
    if (enable) {
        hal_common_reg_bit_set(ZigCtrl, ZIG_CTRL_FRAME_PENDING);
    } else {
        hal_common_reg_bit_clear(ZigCtrl, ZIG_CTRL_FRAME_PENDING);
    }
}

bool mac_hal_rx_enable (bool bc_on, 
                        mlme_rx_enable_req_t *rxe_p)
{
    hal_common_reg_32_write(ZigRxDeferDuration, rxe_p->RxOnDuration);
    if (FALSE == bc_on) {
        mac_hal_hw_control(PHY_RX_ON);
        hal_common_reg_32_write(ZigRxDeferStart, 0);
        hal_common_reg_bit_set(ZigCtrl, ZIG_CTRL_RX_DEFER);
    } else {
        hal_common_reg_32_write(ZigRxDeferStart, rxe_p->RxOnTime);
        if (rxe_p->DeferPermit) {
            hal_common_reg_bit_set(ZigCtrl, ZIG_CTRL_RX_DEFER);
        } else {
            hal_common_reg_bit_clear(ZigCtrl, ZIG_CTRL_RX_DEFER);
        }
    }
    hal_common_reg_bit_set(ZigCtrl, ZIG_CTRL_RX_DEFER_EN);

    return (TRUE);
}

#ifdef Z_P_BRIDGE
#ifdef NO_HOST
void mac_hal_zb_pkt_bridge (sSwFrmDesc* rx_frame_info_p) 
{
    uint8_t       frame_len;
#ifdef BRIDGE_DEBUG
    uint8_t       cp_len;
    uint8_t       tmp_len;
    uint8_t       i;
    uint8_t       j;
#endif
    uint8_t xdata *cp_addr_p;

    frame_len = rx_frame_info_p->frmLen;
#ifdef BRIDGE_DEBUG
    tmp_len = 0;	
    printf("\n");
    for (i = 0 ; i < rx_frame_info_p->cpCount; i++) {
      cp_addr_p = CHAL_GetAccessToCP(rx_frame_info_p->cpArr[i].cp);

        if ((frame_len - tmp_len) > HYBRII_CELLBUF_SIZE) {
            cp_len = HYBRII_CELLBUF_SIZE;
        } else {
            cp_len = frame_len - tmp_len;
        }
        for (j = 0; j < cp_len; j++) {
            printf("0x%02bX ", cp_addr_p[j]);
        }
        printf("\n");
        tmp_len += cp_len;
    }
#endif
    
    if (frame_len > aMaxPHYPacketSize) {
		CHAL_FreeFrameCp(rx_frame_info_p->cpArr, rx_frame_info_p->cpCount);	
        return;
    }
    mac_hal_set_tx_mode(CSMA_UNSLOTTED);
    cp_addr_p = CHAL_GetAccessToCP(rx_frame_info_p->cpArr[0].cp);
	
    if (frame_len > 3) {   /* Don't bridge ACK (3 bytes) packets */
        mac_hal_write_frame_to_fifo(cp_addr_p, frame_len,
                                    AES_NONE, FALSE);
    }
}

#else
void mac_hal_zb_pkt_bridge (sSwFrmDesc* rx_frame_info_p) //sCommonRxFrmSwDesc
{
    uint8_t       frame_len;
#ifdef BRIDGE_DEBUG
    uint8_t       cp_len;
    uint8_t       tmp_len;
    uint8_t       i;
    uint8_t       j;
#endif
    uint8_t xdata *cp_addr_p;

    frame_len = rx_frame_info_p->frmLen;
#ifdef BRIDGE_DEBUG
    tmp_len = 0;	
    //printf("\n");
    for (i = 0 ; i < rx_frame_info_p->cpCount; i++) {
      cp_addr_p = CHAL_GetAccessToCP(rx_frame_info_p->cpArr[i].cp);

        if ((frame_len - tmp_len) > HYBRII_CELLBUF_SIZE) {
            cp_len = HYBRII_CELLBUF_SIZE;
        } else {
            cp_len = frame_len - tmp_len;
        }
        for (j = 0; j < cp_len; j++) {
            //printf("0x%02bX ", cp_addr_p[j]);
        }
        //printf("\n");
        tmp_len += cp_len;
    }
#endif
    
    if ((frame_len - sizeof(sEth2Hdr)) > aMaxPHYPacketSize) {
        return;
    }
    mac_hal_set_tx_mode(CSMA_UNSLOTTED);
    cp_addr_p = CHAL_GetAccessToCP(rx_frame_info_p->cpArr[0].cp);
#if 0
#ifdef BRIDGE_DEBUG	
	printf("\nPayload:\n");
    for (j = 0; j < (frame_len - sizeof(sEth2Hdr)); j++) {
        printf("0x%02bX ", cp_addr_p[j + sizeof(sEth2Hdr)]);
    }
	printf("\n");
#endif	
#endif
    if ((frame_len - sizeof(sEth2Hdr)) > 3) {   /* Don't bridge ACK (3 bytes) packets */
        mac_hal_write_frame_to_fifo((cp_addr_p + sizeof(sEth2Hdr)), 
									(frame_len - sizeof(sEth2Hdr)),
                                    AES_NONE, FALSE);
    }
}
#endif
#endif

#ifdef HYBRII_B
static void mac_hal_promis_reg_bank (tinybool enable)
{
    if (enable) {
        hal_common_reg_bit_set(ZigRegBankSelect, ZIG_NEW_REG_BANK_EN);
    } else {
        hal_common_reg_bit_clear(ZigRegBankSelect, ZIG_NEW_REG_BANK_EN);
    }
}

void mac_hal_promis_ack_control (tinybool enable)
{
    mac_hal_promis_reg_bank(TRUE);
    if (enable) {
        hal_common_reg_bit_set(ZigPromisAckCtrl, ZIG_PROMIS_ACK_EN);
    } else {
        hal_common_reg_bit_clear(ZigPromisAckCtrl, ZIG_PROMIS_ACK_EN);
    }
	mac_hal_promis_reg_bank(FALSE);
}

void mac_hal_promis_ack_match_panid (tinybool enable)
{
    mac_hal_promis_reg_bank(TRUE);
	if (enable) {
        hal_common_reg_bit_set(ZigPromisAckCtrl, 
		                       ZIG_PROMIS_ACK_MATCH_PANID);
    } else {
        hal_common_reg_bit_clear(ZigPromisAckCtrl, 
		                       ZIG_PROMIS_ACK_MATCH_PANID);
    }
	mac_hal_promis_reg_bank(FALSE);
}

void mac_hal_promis_ack_set_match_mode (uint8_t match_mode)
{
    mac_hal_promis_reg_bank(TRUE);
    hal_common_bit_field_reg_write(ZIG_PROMIS_ACK_MATCH_MODE, match_mode);
    mac_hal_promis_reg_bank(FALSE);
}

static tinybool mac_hal_promis_ack_table_avail_entry (uint8_t table_id,
                                                      uint8_t *cur_entries_map,
                                                      uint8_t *avail_entry)
{
    uint32_t enable_map;
    uint32_t table_full_value;
    tinybool rc;

    rc = TRUE;
    mac_hal_promis_reg_bank(TRUE);
    switch (table_id) {
    case SHORT_DST_TABLE:
        enable_map = 
           hal_common_bit_field_reg_read(ZIG_PROMIS_ACK_SHORT_DST_EN);
        table_full_value = 0xFF;
        break;
    case SHORT_SRC_TABLE:
        enable_map = 
           hal_common_bit_field_reg_read(ZIG_PROMIS_ACK_SHORT_SRC_EN);
        table_full_value = 0x03;
        break;
    case EXT_DST_TABLE:
        enable_map = 
           hal_common_bit_field_reg_read(ZIG_PROMIS_ACK_EXT_DST_EN);
           table_full_value = 0xFF;
        break;
    case EXT_SRC_TABLE:
        enable_map = 
           hal_common_bit_field_reg_read(ZIG_PROMIS_ACK_EXT_SRC_EN);
        table_full_value = 0x03;
        break;
    default:
        rc = FALSE;
        break;
    }

    if (enable_map == table_full_value) {
        rc = FALSE;
    }
    
    if (rc) {
        *cur_entries_map = (uint8_t)enable_map;
        *avail_entry = 0;
        while (enable_map & 1) {  /* Find first empty entry */
            enable_map = enable_map >> 1;
            *avail_entry = *avail_entry + 1;
        }
    }
    mac_hal_promis_reg_bank(FALSE);

    return (rc);
}

tinybool mac_hal_promis_ack_short_addr_add (tinybool src, 
                                            uint16_t short_addr)
{
    uint8_t  current_entries_map;
    uint8_t  new_entries_map;
    uint16_t addr;
    uint8_t  index;

    if (TRUE == src) {
        if (mac_hal_promis_ack_table_avail_entry(SHORT_SRC_TABLE,
                                                 &current_entries_map,
                                                 &index)) {
            new_entries_map = current_entries_map | BIT(index);        
            addr = ZigPromisAckSrcShort + (index * sizeof(short_addr));
        } else {
            return (FALSE);
        }
    } else {
        if (mac_hal_promis_ack_table_avail_entry(SHORT_DST_TABLE,
                                                 &current_entries_map,
                                                 &index)) {
            new_entries_map = current_entries_map | BIT(index);        
            addr = ZigPromisAckDstShort + (index * sizeof(short_addr));
        } else {
            return (FALSE);
        }
    }
    //printf("\nindex = %bu, new_entries_map = 0x%02bX, addr = 0x%04X\n",
    //       index, new_entries_map, addr);
    mac_hal_promis_reg_bank(TRUE);
    hal_common_reg_16_write(addr, short_addr);
    /* 
     * Mark the entry is valid
     */
    if (TRUE == src) {
        hal_common_bit_field_reg_write(ZIG_PROMIS_ACK_SHORT_SRC_EN,
                                       new_entries_map);
    } else {
        hal_common_bit_field_reg_write(ZIG_PROMIS_ACK_SHORT_DST_EN,
                                       new_entries_map);
    } 
    mac_hal_promis_reg_bank(FALSE);

    return (TRUE);
}

tinybool mac_hal_promis_ack_ext_addr_add (tinybool src, 
                                          uint64_t ext_addr)
{
    uint8_t  current_entries_map;
    uint8_t  new_entries_map;
    uint16_t addr;
    uint8_t  index;

    if (TRUE == src) {
        if (mac_hal_promis_ack_table_avail_entry(EXT_SRC_TABLE,
                                                 &current_entries_map,
                                                 &index)) {
            new_entries_map = current_entries_map | BIT(index);        
            addr = ZigPromisAckSrcExt + (index * sizeof(ext_addr));
        } else {
            return (FALSE);
        }
    } else {
        if (mac_hal_promis_ack_table_avail_entry(EXT_DST_TABLE,
                                                 &current_entries_map,
                                                 &index)) {
            new_entries_map = current_entries_map | BIT(index);        
            addr = ZigPromisAckDstExt + (index * sizeof(ext_addr));
        } else {
            return (FALSE);
        }
    }
   
    mac_hal_promis_reg_bank(TRUE);
    //printf("\nAddr = 0x%04X, 0x%04X\n", addr, addr + 4);
    hal_common_reg_32_write(addr,     ext_addr.lo_u32);
    hal_common_reg_32_write(addr + 4, ext_addr.hi_u32);
    /* 
     * Mark the entry is valid
     */
    if (TRUE == src) {
        hal_common_bit_field_reg_write(ZIG_PROMIS_ACK_EXT_SRC_EN,
                                       new_entries_map);
    } else {
        hal_common_bit_field_reg_write(ZIG_PROMIS_ACK_EXT_DST_EN,
                                       new_entries_map);
    } 
    mac_hal_promis_reg_bank(FALSE);

    return (TRUE);
}

tinybool mac_hal_promis_ack_short_addr_del (tinybool src, uint16_t short_addr)
{
    uint32_t enable_map;
    uint16_t addr;
    uint16_t operation_reg;
    uint16_t entry_addr;
    uint8_t  current_map;
    uint8_t  index;
    tinybool found;

    mac_hal_promis_reg_bank(TRUE);
    if (TRUE == src) {
        enable_map = 
           hal_common_bit_field_reg_read(ZIG_PROMIS_ACK_SHORT_SRC_EN);
        operation_reg = ZigPromisAckSrcShort;
    } else {
        enable_map = 
           hal_common_bit_field_reg_read(ZIG_PROMIS_ACK_SHORT_DST_EN);
        operation_reg = ZigPromisAckDstShort;
    }
    /*
     * Search DST Short Address table
     */
    
    current_map = (uint8_t)enable_map;
    index = 0;
    found = FALSE;
    while (enable_map) {
        if (enable_map & 1) {
            addr = operation_reg + (index * sizeof(short_addr));
            entry_addr = hal_common_reg_16_read(addr);
            if (short_addr == entry_addr) {
                current_map = current_map & ~BIT(index);
                if (TRUE == src) {
                    hal_common_bit_field_reg_write(ZIG_PROMIS_ACK_SHORT_SRC_EN,
                                                   current_map);
                } else {
                    hal_common_bit_field_reg_write(ZIG_PROMIS_ACK_SHORT_DST_EN,
                                                   current_map);
                }
                found = TRUE;
                break;
            } 
        }
        enable_map = enable_map >> 1;
        index++;
    }

    mac_hal_promis_reg_bank(FALSE);

    return (found);
}

tinybool mac_hal_promis_ack_ext_addr_del (tinybool src, uint64_t ext_addr)
{
    uint32_t enable_map;
    uint16_t addr;
    uint16_t operation_reg;
    uint64_t entry_addr;
    uint8_t  current_map;
    uint8_t  index;
    tinybool found;

    mac_hal_promis_reg_bank(TRUE);

    if (TRUE == src) {
        enable_map = 
           hal_common_bit_field_reg_read(ZIG_PROMIS_ACK_EXT_SRC_EN);
        operation_reg = ZigPromisAckSrcExt;
    } else {
        /*
         * Search DST Ext Address table
         */
        enable_map = 
           hal_common_bit_field_reg_read(ZIG_PROMIS_ACK_EXT_DST_EN);
        operation_reg = ZigPromisAckDstExt;
    }

    current_map = (uint8_t)enable_map;
    index = 0;
    found = FALSE;
    while (enable_map) {
        if (enable_map & 1) {
            addr = operation_reg + (index * sizeof(ext_addr));
            entry_addr.lo_u32 = hal_common_reg_32_read(addr);
            entry_addr.hi_u32 = hal_common_reg_32_read(addr + 4);
            if (EXT_ADDR_MATCH(ext_addr, entry_addr)) {
                current_map = current_map & ~BIT(index);
                if (TRUE == src) {
                    hal_common_bit_field_reg_write(ZIG_PROMIS_ACK_EXT_SRC_EN,
                                                   current_map);
                } else {
                    hal_common_bit_field_reg_write(ZIG_PROMIS_ACK_EXT_DST_EN,
                                                   current_map);
                }
                found = TRUE;
                break;
            } 
        }
        enable_map = enable_map >> 1;
        index++;
    }
    
    mac_hal_promis_reg_bank(FALSE);

    return (found);
}

void mac_hal_promis_ack_table_show (void)
{
    uint32_t enable_map;
    uint16_t addr;
    uint16_t short_addr;
    uint64_t ext_addr;
    uint8_t  index;
    uint8_t  state, mode;

    mac_hal_promis_reg_bank(TRUE);

    /*
     * SRC Short Address table
     */
    enable_map = 
       hal_common_bit_field_reg_read(ZIG_PROMIS_ACK_SHORT_SRC_EN);
    printf("Src ShrtAddr Tb\n");
    for (index = 0; index < ZIG_PROMIS_ACK_SRC_ENTRY_MAX; index++) {  
       addr = ZigPromisAckSrcShort + (index * sizeof(short_addr));
       short_addr = hal_common_reg_16_read(addr);
       printf("%bu-%04x (%bu)\n", index, short_addr, (u8)((enable_map >> index) & 1));
    }

    /*
     * Source Ext Address table
     */
    enable_map = 
       hal_common_bit_field_reg_read(ZIG_PROMIS_ACK_EXT_SRC_EN);
    printf("Src ExtAddr Tb\n");
    for (index = 0; index < ZIG_PROMIS_ACK_SRC_ENTRY_MAX; index++) {  
       addr = ZigPromisAckSrcExt + (index * sizeof(ext_addr));
       ext_addr.lo_u32 = hal_common_reg_32_read(addr);
       ext_addr.hi_u32 = hal_common_reg_32_read(addr + 4);
       printf("%bu-%08lx-%08lx (%bu)\n", index, ext_addr.hi_u32, ext_addr.lo_u32,
              (u8)((enable_map >> index) & 1));
    }
    if (hal_common_reg_bit_test(ZigPromisAckCtrl, ZIG_PROMIS_ACK_EN)) {
        state = TRUE;
    } else {
        state = FALSE;
    }

    /*
     * DST Short Address table
     */
    enable_map = 
       hal_common_bit_field_reg_read(ZIG_PROMIS_ACK_SHORT_DST_EN);
    printf("\nDstn ShrtAddr Tb\n");
    for (index = 0; index < ZIG_PROMIS_ACK_DST_ENTRY_MAX; index++) {  
       addr = ZigPromisAckDstShort + (index * sizeof(short_addr));
       short_addr = hal_common_reg_16_read(addr);
       printf("%bu-%04x (%bu)\n", index, short_addr, (u8)((enable_map >> index) & 1));
    }

    /*
     * DST Ext Address table
     */
    enable_map = 
       hal_common_bit_field_reg_read(ZIG_PROMIS_ACK_EXT_DST_EN);
    printf("Dstn ExtAddr Tb\n");
    for (index = 0; index < ZIG_PROMIS_ACK_DST_ENTRY_MAX; index++) {  
       addr = ZigPromisAckDstExt + (index * sizeof(ext_addr));
       ext_addr.lo_u32 = hal_common_reg_32_read(addr);
       ext_addr.hi_u32 = hal_common_reg_32_read(addr + 4);
       printf("%bu-%08lx-%08lx (%bu)\n", index, ext_addr.hi_u32, ext_addr.lo_u32,
              (u8)((enable_map >> index) & 1));
    }

    mode = hal_common_bit_field_reg_read(ZIG_PROMIS_ACK_MATCH_MODE);

    printf("\nPromAck - St:%bu, M:%bu\n", state, mode);

    mac_hal_promis_reg_bank(FALSE);
}
#endif

#ifdef Z_P_BRIDGE
void mac_hal_bridge_config (uint8_t enable)
{
    if (enable) {
        zb_plc_bridging = TRUE;
        hal_common_reg_bit_set(ZigCtrl, ZIG_CTRL_FRAME_PENDING);
    } else {
        zb_plc_bridging = FALSE;
        hal_common_reg_bit_clear(ZigCtrl, ZIG_CTRL_FRAME_PENDING);
    }
    mac_hal_pib_set(macPromiscuousMode, 
                    (pib_value_t *)&zb_plc_bridging);
    mac_hal_promis_ack_control((tinybool)zb_plc_bridging);
}
#endif
#endif //HYBRII_802154
