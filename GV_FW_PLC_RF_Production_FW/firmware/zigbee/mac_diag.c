#if (defined HYBRII_802154) && (defined ZBMAC_DIAG) 
#ifdef RTX51_TINY_OS
#include <rtx51tny.h>
#include "hybrii_tasks.h"
#endif
#include <REG51.H>
#include <stdio.h>
#include <string.h>
#include "papdef.h"
#include "timer.h"
#include "hal_common.h"
#include "hal.h"
#include "hal_spi.h"
#include "gv701x_cfg.h"
#include "hybrii_802_15_4_regs.h"
#include "return_val.h"
#include "uart.h"
#include "utils_fw.h"
#include "utils.h"
#include "ui_utils.h"
#include "bmm.h"
#include "qmm.h"
#include "mac_const.h"
#include "mac_msgs.h"
#include "mac_hal.h"
#include "mac_api.h"
#include "mac.h"
#include "mac_data_structures.h"
#include "mac_internal.h"
#include "mac_security.h"
#include "mac_diag.h"
#include "gv701x_gpiodriver.h"

#ifdef PROD_TEST
#include "hal_rf_prod_test.h"
#include "fm.h"
#include "crc32.h"
#endif

extern void mac_hal_mac_busy_recover();
extern void ISM_PollInt(void);

#ifdef PROD_TEST
extern tTimerId prod_test_rf_timer;
extern void spiflash_eraseSector(u32 Sector);
extern void spiflash_wrsr_unlock(u8 const unlock);
extern sProdConfigProfile gProdFlashProfile;
#endif
#define DEBUG 0

static uint8_t xdata spi_tx_data[MAX_SPI_DATA];
uint8_t  spi_data = 0;
uint8_t test_tx_data[128];
static uint8_t beacon_payload[] = { 0x51, 0x52, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58,
                                    0x59, 0x5a, 0x5b, 0x5c, 0x5d, 0x5e, 0x5f, 0x60,
                                    0x61, 0x62, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68,
                                    0x69, 0x6a, 0x6b, 0x6c, 0x6d, 0x6e, 0x6f, 0x70,
                                    0x71, 0x72, 0x73, 0x74, 0x75, 0x75, 0x77, 0x78,
                                    0x79, 0x7a, 0x7b, 0x7c, 0x7d, 0x7e, 0x7f, 0x80 };
static uint8_t  test_key_source[SEC_KEY_SRC_MAX];
static uint16_t dst_pan_id;
static uint16_t dst_short_addr;

extern uint64_t dst_ext_addr;
static bool display_stats = FALSE;

static void mac_diag_display_ctrl_reg (void)
{
    u32 value32;

    value32 = hal_common_reg_32_read(ZigCtrl);
    printf("\nControl Reg = 0x%08lX\n", value32);
}

static void mac_diag_display_hw_rx_stats (void)
{
    u32 xdata value32;
    u32 rx_errors;
    
    if (display_stats == FALSE) {
        return;
    }

    printf("\nH/W RX Statistics:");
    value32 = hal_common_reg_32_read(ZigRxStat);
    rx_errors = (u16) hal_common_bit_field_get(value32,
                                               ZIG_RX_CRC_MASK,
                                               ZIG_RX_CRC_POS);
    printf("\n  Invalid CRC = %lu, ", rx_errors);
    rx_errors = (u16) hal_common_bit_field_get(value32,
                                               ZIG_RX_PKTS_MASK,
                                               ZIG_RX_PKTS_POS);
    printf("Packets Received = %lu\n", rx_errors);
}

#ifdef ZIGBEE_HW_DIAG_MODE
void mac_diag_tx_diag_mode_test (void)
{
    int tx_length, tx_delay, tx_repeat;
    u32 count, bytes_tx;
    u32 xdata value32;
    u8  i;

    tx_length = 0x20;
    tx_delay  = 200;
    tx_repeat = 1000;

    printf("    Zigbee TX:  Send {0x%02X bytes}", tx_length);
    value32 = (ZIG_CTRL_DIAG_EN | ZIG_CTRL_TX_EN | ZIG_CTRL_COO_EN);
    hal_common_reg_bit_set(ZigCtrl, value32);
    mac_diag_display_ctrl_reg();

    count = 0;
    bytes_tx = 0;

    /*  Fill beacon template with data */
    for (i = 1; i < 128; i++) {
        WriteU8Reg(ZigBeaconTemplate + i, test_tx_data[i - 1]);
    }
    while (1) {
        // Check TX clear
        value32 = hal_common_reg_32_read(ZigCtrl);
        if ((value32 & ZIG_CTRL_BC_VALID) != 0) {
            printf("#");
            continue;
        }

        WriteU8Reg(ZigBeaconTemplate, tx_length);
        hal_common_reg_32_write(ZigCtrl, value32 | ZIG_CTRL_BC_VALID);
        if (tx_delay == 0) {      // single tx
            break;
        }

        bytes_tx += tx_length;
        printf("\rPackets transmitted = %lu - Bytes transmitted = %lu",
	           ++count, bytes_tx);

        if (abort() == TRUE) {
            break;
        } else {
            if (--tx_repeat == 0) {
                break;
            }
        }

        mac_utils_delay_ms(tx_delay);
    }

    printf("\n");
    hal_common_reg_bit_clear(ZigCtrl, ZIG_CTRL_DIAG_EN);
}
#endif

static void mac_diag_config_tx_rx (bool promis_en)
{
    u32 xdata value32;

    value32 = (ZIG_CTRL_TX_EN          |
               ZIG_CTRL_RX_EN          |
               ZIG_CTRL_ACK_EN         |
               ZIG_CTRL_NON_BEACON);

    hybrii_mode = TRUE;
    hal_common_reg_bit_set(ZigCtrl, value32);
    mac_hal_pib_set(macPromiscuousMode, (pib_value_t *)&promis_en);

    mac_hal_zigbee_interrupt_control(FALSE,
                                     CPU_INT_ZB_BC_TX_TIME     |
                                     CPU_INT_ZB_PRE_BC_TX_TIME  );
}

extern void mac_hal_irq_handler(void);
uint8_t msdu_handle = 0;

static void mac_diag_show_config (void)
{
    uint8_t  val_8;
    uint16_t val_16;
    uint16_t short_addr;
    uint64_t ext_addr;

    mac_hal_pib_get(phyCurrentChannel, &val_8);
    printf("\nCurChannel :%bx", val_8);
    mac_hal_pib_get(I_AM_PAN_COORDINATOR, &val_8);
    printf("\nPANCoordinator :%bx", val_8);
    mac_hal_pib_get(macBeaconOrder, &val_8);
    printf("\nBcnOrder :%bu", val_8);
    mac_hal_pib_get(macSuperframeOrder, &val_8);
    printf("\nSFOrder :%bu", val_8);
    mac_hal_pib_get(macPANId, (uint8_t *)&val_16);
    mac_hal_pib_get(macShortAddress, (uint8_t *)&short_addr);
    mac_hal_pib_get(macIeeeAddress, (uint8_t *)&ext_addr);
    printf("\nSrc Pan ID: %04x, Src Short Addr: %04x, "
           "Src Ext Addr: %08lx-%08lx", val_16, short_addr,
            ext_addr.hi_u32, ext_addr.lo_u32);  
    printf("\nDst Pan ID: %04x, Dst Short Addr: %04x, "
           "Dst Ext Addr: %08lx-%08lx\n", dst_pan_id, dst_short_addr,
            dst_ext_addr.hi_u32, dst_ext_addr.lo_u32);
}

static void mac_diag_display_tx_stats (void)
{
    u32 xdata value32;
    u32 tx_errors;

    if (display_stats == FALSE) {
        return;
    }

    mac_display_tx_stats();	
    value32 = hal_common_reg_32_read(ZigTxStat1);

    printf("\nH/W TX Stats:");	
    tx_errors = (u16) hal_common_bit_field_get(value32,
                                               ZIG_TX_NO_ACK_CNT_MASK,
                                               ZIG_TX_NO_ACK_CNT_POS);
    printf("\nNoACK =%lu,", tx_errors);
    tx_errors = (u16) hal_common_bit_field_get(value32,
                                               ZIG_TX_NO_CCA_MASK,
                                               ZIG_TX_NO_CCA_POS);
    printf("NoCCA =%lu,", tx_errors);

    value32 = hal_common_reg_32_read(ZigTxStat2);
    tx_errors = (u16) hal_common_bit_field_get(value32,
                                               ZIG_TX_RETRIED_MASK,
                                               ZIG_TX_RETRIED_POS);
    printf("TxRetried =%lu", tx_errors);
    tx_errors = (u16) hal_common_bit_field_get(value32,
                                               ZIG_TX_SENT_MASK,
                                               ZIG_TX_SENT_POS);
    printf("PktsSent = lu", tx_errors);

    value32 = hal_common_reg_32_read(ZigTxStat3);
    tx_errors = (u16) hal_common_bit_field_get(value32,
                                               ZIG_TX_CCA_BUSY_MASK,
                                               ZIG_TX_CCA_BUSY_POS);
    printf("\nCCABusy =%lu,", tx_errors);
    tx_errors = (u16) hal_common_bit_field_get(value32,
                                               ZIG_TX_HANG_RECOVERED_MASK,
                                               ZIG_TX_HANG_RECOVERED_POS);
    printf("TxHangRecovered = %lu\n", tx_errors);

    mac_display_rx_stats();
    mac_diag_display_hw_rx_stats();
}

tinybool use_bc_template_g;
extern uint32_t data_req_g;

void mac_diag_tx_action (mac_diag_tx_params_t *tx_params_p)
{
    u16 pkt_size_in;
    u16 payload_size;
    u16 src_addr_mode_in;
    u16 dst_addr_mode_in;
    u8  tx_option;
    u8  max_payload;
    u8  beacon_order;
    u32 tx_pkts = 0;
    u32 tx_bytes;
    u32 tx_bc_pkts;
    u32 tx_bc_bytes;
    u32 rx_pkts;
    u32 rx_bytes;
    u16 decrypt_err;
    u32 tx_req = 0;
    u16 tx_errors = 0;    
    wpan_addr_spec_t addr_spec;
    security_info_t  sec;
    u8  abort_count;
    mac_state_t mac_state_save;
    u32 frame_counter;
    u8  seq;
#ifdef _RX_PROMIS_
    bool promis_mode = TRUE;
#else
    bool promis_mode = FALSE;
#endif
    
    pkt_size_in = tx_params_p->pkt_size_in;
    if (tx_params_p->beacon_template_in) {
        use_bc_template_g = TRUE;
    } else {
        use_bc_template_g = FALSE;
    }

    mac_diag_config_tx_rx(promis_mode);
    mac_state_save = mac_state;

    EXT_ADDR_CLEAR(addr_spec.Addr.long_address);
    if (tx_params_p->src_addr_mode_in == 0 &&
        tx_params_p->dst_addr_mode_in == 0) {
        src_addr_mode_in = FCF_SHORT_ADDR;
        dst_addr_mode_in = FCF_SHORT_ADDR;
        addr_spec.Addr.short_address = dst_short_addr;
        max_payload = aMaxPHYPacketSize - aMinMPDUOverhead - FCS_LEN;
    } else if (tx_params_p->src_addr_mode_in == 0 &&
               tx_params_p->dst_addr_mode_in == 1) {
        src_addr_mode_in = FCF_SHORT_ADDR;
        dst_addr_mode_in = FCF_LONG_ADDR;
        max_payload = aMaxPHYPacketSize - (aMinMPDUOverhead + 6) - FCS_LEN;
        ADDR_COPY_DST_SRC_64(addr_spec.Addr.long_address, dst_ext_addr);
    } else if (tx_params_p->src_addr_mode_in == 1 &&
               tx_params_p->dst_addr_mode_in == 0) {
        src_addr_mode_in = FCF_LONG_ADDR;
        dst_addr_mode_in = FCF_SHORT_ADDR;
        max_payload = aMaxPHYPacketSize - (aMinMPDUOverhead + 6) - FCS_LEN;
        addr_spec.Addr.short_address = dst_short_addr;
    } else if (tx_params_p->src_addr_mode_in == 0 &&
               tx_params_p->dst_addr_mode_in == 2) {
        src_addr_mode_in = FCF_SHORT_ADDR;
        dst_addr_mode_in = FCF_NO_ADDR;
        max_payload = aMaxPHYPacketSize - (aMinMPDUOverhead - 2) - FCS_LEN;
    } else if (tx_params_p->src_addr_mode_in == 1 &&
               tx_params_p->dst_addr_mode_in == 2) {
        src_addr_mode_in = FCF_LONG_ADDR;
        dst_addr_mode_in = FCF_NO_ADDR;
        max_payload = aMaxPHYPacketSize - (aMinMPDUOverhead + 4) - FCS_LEN;
    } else if (tx_params_p->src_addr_mode_in == 2 &&
               tx_params_p->dst_addr_mode_in == 0) {
        src_addr_mode_in = FCF_NO_ADDR;
        dst_addr_mode_in = FCF_SHORT_ADDR;
        max_payload = aMaxPHYPacketSize - (aMinMPDUOverhead - 2) - FCS_LEN;
        addr_spec.Addr.short_address = dst_short_addr;
    } else if (tx_params_p->src_addr_mode_in == 2 &&
               tx_params_p->dst_addr_mode_in == 1) {
        src_addr_mode_in = FCF_NO_ADDR;
        dst_addr_mode_in = FCF_LONG_ADDR;
        max_payload = aMaxPHYPacketSize - (aMinMPDUOverhead + 4) - FCS_LEN;
        ADDR_COPY_DST_SRC_64(addr_spec.Addr.long_address, dst_ext_addr);
    } else if (tx_params_p->src_addr_mode_in == 2 &&
               tx_params_p->dst_addr_mode_in == 2) {
        /*
         * Should get DATA-CONF with MAC_INVALID_ADDRESS
         */
        src_addr_mode_in = FCF_NO_ADDR;
        dst_addr_mode_in = FCF_NO_ADDR;
        max_payload = aMaxPHYPacketSize - (aMinMPDUOverhead - 4) - FCS_LEN;
    } else {
        src_addr_mode_in = FCF_LONG_ADDR;
        dst_addr_mode_in = FCF_LONG_ADDR;
        max_payload = aMaxPHYPacketSize - (aMinMPDUOverhead + 12) - FCS_LEN;
        ADDR_COPY_DST_SRC_64(addr_spec.Addr.long_address, dst_ext_addr);
    }
    addr_spec.AddrMode  = dst_addr_mode_in;
    addr_spec.PANId     = dst_pan_id;
    if (dst_pan_id != hal_pib_PANId) {
        max_payload -= 2;  /* No PANid Commpress */
    }
    sec.SecurityLevel   = (uint8_t)tx_params_p->sec_level_in;
    sec.KeyIdMode       = (uint8_t)tx_params_p->sec_id_mode_in &0x03;
    sec.KeyIndex        = (uint8_t)tx_params_p->sec_idx_in;

    if (0 != sec.SecurityLevel) {
        memcpy(sec.KeySource, test_key_source, SEC_KEY_SRC_MAX);
        frame_counter = 5; /* Test vector 2 in IEEE 802.14.5 Appendix C */
        mac_api_mlme_set_req(macFrameCounter, 0, &frame_counter);
#ifndef RTX51_TINY_OS
        mac_task();
#endif
        seq = 0x84;        /* Test vector 2 in IEEE 802.14.5 Appendix C */
        mac_api_mlme_set_req(macDSN, 0, &seq); 
#ifndef RTX51_TINY_OS        
        mac_task();
#endif
        max_payload -= (sec_additional_len(&sec) +
                        get_mic_length(sec.SecurityLevel));
        if (2 == tx_params_p->tx_pkt_type_in) {  // Beacon packet
            // Set up vector 0 in IEEE 802.15.4 Appendix C
            u8 beacon_payload_len;
            u8 superframe_order = 5;
            u8 association_permit = 1;
            bool pan_coor = FALSE;

            beacon_order = 5;
            if (tx_params_p->beacon_template_in) {
                /*
                 * Need to be PAN Coordinator to
                 * send beacon using beacon template
                 */
                pan_coor = TRUE;
            }
           
            mac_hal_pib_set(I_AM_PAN_COORDINATOR,
                           (void *)&pan_coor);

            beacon_payload_len = pkt_size_in; 
            mac_api_mlme_set_req(macBeaconPayloadLength, 0,
                                 &beacon_payload_len);
#ifndef RTX51_TINY_OS
            mac_task();
#endif
            mac_api_mlme_set_req(macBeaconPayload, 0,
                                 beacon_payload);
#ifndef RTX51_TINY_OS
            mac_task();
#endif
            mac_api_mlme_set_req(macBeaconOrder, 0, &beacon_order);
#ifndef RTX51_TINY_OS
            mac_task();
#endif
            mac_api_mlme_set_req(macSuperframeOrder, 0, &superframe_order);
#ifndef RTX51_TINY_OS
            mac_task();
#endif            
            mac_api_mlme_set_req(macBSN, 0, &seq);
#ifndef RTX51_TINY_OS
            mac_task();
#endif
            mac_api_mlme_set_req(macAssociationPermit, 0,
                                 &association_permit);
#ifndef RTX51_TINY_OS
            mac_task();
#endif
            mac_state = MAC_PAN_COORD_STARTED;
        }
    }

    if (pkt_size_in > max_payload) {
        pkt_size_in = max_payload;
    }
    payload_size = pkt_size_in;

    mac_clear_stats();
    data_req_g = 0;
    if (tx_params_p->ack_req_in == 0) {
        tx_option = TX_CAP;
    } else {
        tx_option = TX_CAP_ACK;
    }
    abort_count = 0;
    mac_diag_display_ctrl_reg();

    do {
        if (tx_req == (tx_pkts + tx_errors)) {
            if (abort_count) {
                break;
            }
            if (payload_size > max_payload) {
                payload_size = pkt_size_in;
            }
            switch (tx_params_p->tx_pkt_type_in) {
            case 0:
                if (0 != sec.SecurityLevel) {
#ifdef _SAME_AES_PKTS_
                    frame_counter = 5; /* Test vector 2 in IEEE Appendix C */
                    mac_api_mlme_set_req(macFrameCounter, 0, &frame_counter);
 #ifndef RTX51_TINY_OS
                    mac_task();
 #endif
                    seq = 0x84;        /* Test vector 2 in IEEE Appendix C */
                    mac_api_mlme_set_req(macDSN, 0, &seq); 
 #ifndef RTX51_TINY_OS
                    mac_task();
 #endif
#endif
                }
                mac_api_mcps_data_req(src_addr_mode_in,
                                      &addr_spec,
                                      payload_size,
                                      test_tx_data,
                                      msdu_handle++,
                                      tx_option,
                                      &sec);
                break;
            case 1:
                mac_data_build_and_tx_data_req(true, false, dst_addr_mode_in,
                                               &addr_spec.Addr,
                                               addr_spec.PANId);
                break;
            case 2:
                mac_build_and_tx_beacon(false, &sec);
                mac_hal_zigbee_interrupt_control(FALSE,
                                                 CPU_INT_ZB_BC_TX_TIME    |
                                                 CPU_INT_ZB_PRE_BC_TX_TIME );
                if (hal_common_reg_bit_test(CPU_INTSTATUS_REG,
                                            CPU_INT_ZB_PRE_BC_TX_TIME)) {
                    hal_common_reg_32_write(CPU_INTSTATUS_REG,
                                            CPU_INT_ZB_PRE_BC_TX_TIME);
                }
                break;
            case 3:
                mac_api_mlme_associate_req(12, 0, &addr_spec, 0xce, &sec);
                break;
            default:
                return;
            }
            tx_req++;
            payload_size += tx_params_p->size_inc_in;
        }
#ifdef RTX51_TINY_OS
        os_switch_task();
#else
        mac_task();

        ISM_PollInt();

        mac_hal_task();

#endif
        mac_get_tx_stats(&tx_pkts, &tx_bytes, &tx_errors,
                         &tx_bc_pkts, &tx_bc_bytes);
        mac_get_rx_stats(&rx_pkts, &rx_bytes, &decrypt_err);
        if (tx_params_p->show_stats) {
            printf("\rPkts TX = %lu/%lu (Err =%u)-"
                   "Pkts RX = %lu/%lu (Decrypt Err =%u)",
	               tx_pkts, tx_bytes, tx_errors,
                   rx_pkts, rx_bytes, decrypt_err);
        }
        if (tx_params_p->pkt_count_in != 0 &&
            tx_pkts >= tx_params_p->pkt_count_in) {
            break;
        }
        if (abort() == 1) {
            abort_count++;
            if (abort_count > 3) {
                break;
            }
        }
        mac_utils_delay_ms(tx_params_p->inter_pkt_delay_in);
    } while (1);

    mac_diag_display_ctrl_reg();
    mac_state = mac_state_save;

    //hal_common_reg_bit_clear(ZigCtrl, ZIG_CTRL_RX_EN);  /* Stop receiver */
    //mac_hal_flush_incoming_frame_queue();               /* Clean up rx   */
    mac_q_flush();
    
    beacon_order = NON_BEACON_NWK;
    mac_api_mlme_set_req(macBeaconOrder, 0, &beacon_order);
#ifndef RTX51_TINY_OS
    mac_task();
#endif
    
    mac_api_mlme_set_req(macSuperframeOrder, 0, &beacon_order);
#ifndef RTX51_TINY_OS
    mac_task();
#endif

    if (TRUE == tx_params_p->show_stats) {
        mac_diag_display_tx_stats();
    }
    if (display_stats) {
        printf("\ntx req = %lu/%lu\n", tx_req, data_req_g);
    }
}

void mac_diag_tx_test (u8 *cmd_buf_p)
{
    mac_diag_tx_params_t tx_params;

    memset(&tx_params, 0, sizeof(mac_diag_tx_params_t));
    tx_params.ack_req_in = 1;

    if (sscanf(cmd_buf_p+1, "%d %d %d %d %d %d %d %d %d %d %d %d %bu",
               &tx_params.pkt_size_in, &tx_params.size_inc_in,
               &tx_params.pkt_count_in, &tx_params.inter_pkt_delay_in,
               &tx_params.ack_req_in, &tx_params.src_addr_mode_in,
               &tx_params.dst_addr_mode_in, &tx_params.tx_pkt_type_in,
               &tx_params.beacon_template_in, &tx_params.sec_level_in,
               &tx_params.sec_id_mode_in, &tx_params.sec_idx_in,
               &tx_params.show_stats) < 1) {
        printf("\nInvalid cmd params");
        return;
    }
    if (tx_params.show_stats == 0) {
        tx_params.show_stats = TRUE;
    } else {
        tx_params.show_stats = FALSE;
    }
    mac_diag_tx_action(&tx_params);
}

void mac_diag_tx_security_test (u8 *cmd_buf_p)
{
    mac_diag_tx_params_t tx_params;
    u16                  sec_level;
    u16                  sec_id_mode;
    u8                   send_no_dst_addr;

    if (sscanf(cmd_buf_p+1, "%d %d %d %d %d %d %d %d %bd",
               &tx_params.pkt_size_in, &tx_params.size_inc_in,
               &tx_params.pkt_count_in, &tx_params.inter_pkt_delay_in,
               &tx_params.ack_req_in, &tx_params.tx_pkt_type_in,
               &tx_params.beacon_template_in, &tx_params.sec_idx_in,
               &send_no_dst_addr) < 9) {
        printf("\nInvalid cmd params");
        return;
    }

    tx_params.show_stats       = FALSE;

    tx_params.src_addr_mode_in = 1;  /* Src  long */
    tx_params.dst_addr_mode_in = 1;  /* Dest long */
    for (sec_level = 1; sec_level < 8; sec_level++) {
        tx_params.sec_level_in = sec_level;
        for (sec_id_mode = 0; sec_id_mode < 4; sec_id_mode++) {
            tx_params.sec_id_mode_in = sec_id_mode;
            mac_diag_tx_action(&tx_params);
        }
    }

    if (tx_params.tx_pkt_type_in == 3) {
        return;
    }
 
    tx_params.src_addr_mode_in = 0;  /* Src  short */
    tx_params.dst_addr_mode_in = 0;  /* Dest short */    
    for (sec_level = 1; sec_level < 8; sec_level++) {
        tx_params.sec_level_in = sec_level;
        for (sec_id_mode = 0; sec_id_mode < 4; sec_id_mode++) {
            tx_params.sec_id_mode_in = sec_id_mode;
            mac_diag_tx_action(&tx_params);
        }
    }

    tx_params.src_addr_mode_in = 0;  /* Src  short */
    tx_params.dst_addr_mode_in = 1;  /* Dest long */
    for (sec_level = 1; sec_level < 8; sec_level++) {
        tx_params.sec_level_in = sec_level;
        for (sec_id_mode = 0; sec_id_mode < 4; sec_id_mode++) {
            tx_params.sec_id_mode_in = sec_id_mode;
            mac_diag_tx_action(&tx_params);
        }
    }

    tx_params.src_addr_mode_in = 1;  /* Src  long */
    tx_params.dst_addr_mode_in = 0;  /* Dest short */
    for (sec_level = 1; sec_level < 8; sec_level++) {
        tx_params.sec_level_in = sec_level;
        for (sec_id_mode = 0; sec_id_mode < 4; sec_id_mode++) {
            tx_params.sec_id_mode_in = sec_id_mode;
            mac_diag_tx_action(&tx_params);
        }
    }

    tx_params.src_addr_mode_in = 2;  /* Src  empty */
    tx_params.dst_addr_mode_in = 0;  /* Dest short */
    for (sec_level = 1; sec_level < 8; sec_level++) {
        tx_params.sec_level_in = sec_level;
        for (sec_id_mode = 0; sec_id_mode < 4; sec_id_mode++) {
            tx_params.sec_id_mode_in = sec_id_mode;
            mac_diag_tx_action(&tx_params);
        }
    }

    tx_params.src_addr_mode_in = 2;  /* Src  empty */
    tx_params.dst_addr_mode_in = 1;  /* Dest long */
    for (sec_level = 1; sec_level < 8; sec_level++) {
        tx_params.sec_level_in = sec_level;
        for (sec_id_mode = 0; sec_id_mode < 4; sec_id_mode++) {
            tx_params.sec_id_mode_in = sec_id_mode;
            mac_diag_tx_action(&tx_params);
        }
    }

    if (send_no_dst_addr == 0) {
        return;
    }

    tx_params.src_addr_mode_in = 0;  /* Src  short */
    tx_params.dst_addr_mode_in = 2;  /* Dest empty */
    for (sec_level = 1; sec_level < 8; sec_level++) {
        tx_params.sec_level_in = sec_level;
        for (sec_id_mode = 0; sec_id_mode < 4; sec_id_mode++) {
            tx_params.sec_id_mode_in = sec_id_mode;
            mac_diag_tx_action(&tx_params);
        }
    }
    
    tx_params.src_addr_mode_in = 1;  /* Src  long */
    tx_params.dst_addr_mode_in = 2;  /* Dest empty */
    for (sec_level = 1; sec_level < 8; sec_level++) {
        tx_params.sec_level_in = sec_level;
        for (sec_id_mode = 0; sec_id_mode < 4; sec_id_mode++) {
            tx_params.sec_id_mode_in = sec_id_mode;
            mac_diag_tx_action(&tx_params);
        }
    }
}

static void mac_diag_rx_normal_mode (void)
{
    u32 pkts_count;
    u32 bytes_count;
    u16 decrypt_err;

    do {
#ifdef RTX51_TINY_OS
        os_switch_task();
#else
        ISM_PollInt();
        mac_hal_task();
#endif
        mac_get_rx_stats(&pkts_count, &bytes_count,
                         &decrypt_err);
        if (display_stats) {
        printf("\rPkts rxd = %lu/%lu (DecryptErr =%u)",
	           pkts_count, bytes_count, decrypt_err);
        } else {
            printf("\rPkts rxd = %lu/%lu",
	               pkts_count, bytes_count);
        }
        if (abort() == 1) {
            break;
        }
    } while (1);
    hal_common_reg_bit_clear(ZigCtrl, ZIG_CTRL_RX_EN);	/* Stop receiver */
    mac_hal_flush_incoming_frame_queue();               /* Clean up rx   */
    
    if (display_stats) {
        mac_display_rx_stats();
        mac_diag_display_hw_rx_stats();
        hal_common_display_qc_error_stats();
    }
}

#ifdef ZIGBEE_HW_DIAG_MODE
static void mac_diag_rx_diag_mode ()
{
    volatile u32 reg_value;
    u16          pkt_size;
    u32          pkts_count;
    u32          bytes_count;

    pkts_count = 0;
    bytes_count = 0;
    hal_common_reg_bit_set(ZigCtrl, ZIG_CTRL_DIAG_EN);
    // Wait until RX_FIFO Available (bit 0) in Rx Status Register
    do {
        reg_value = hal_common_reg_32_read(ZigCtrl);
        if (0 == (reg_value & ZIG_CTRL_RX_EN)) {
            printf("\nRead RX DISABLE");
            break;
        }
        if (reg_value & ZIG_CTRL_BC_VALID) {
            pkts_count++;
            pkt_size = ReadU8Reg(ZigBeaconTemplate) & 0x7F;
            bytes_count += pkt_size;
            printf("\rPkts rxd =%lu -Bytes rxed =%lu",
	               pkts_count, bytes_count);
            if (DEBUG == 1) {
                printf("\nRx pkt size = %d", pkt_size);
            }
            reg_value &= ~ZIG_CTRL_BC_VALID;
            if (0 == (reg_value & ZIG_CTRL_RX_EN)) {
                printf("\nWrite RX DISABLE");
                break;
            }
            hal_common_reg_32_write(ZigCtrl, reg_value);
        }
        if (abort() == 1) {
            break;
        }
    } while (1);
    hal_common_reg_bit_clear(ZigCtrl, ZIG_CTRL_DIAG_EN);

    printf("\nTotal pkts rxed = %lu - Total bytes rxed = %lu\n",
	       pkts_count, bytes_count);
}
#endif

void mac_diag_config_addr (bool tx)
{
    uint16_t src_short_addr;
    uint64_t src_ext_addr;

    dst_pan_id = 0x4321;
    
    if (tx) {
        /*
         * Address set up for TX side 
         */
        dst_short_addr = 0x02;
        dst_ext_addr.lo_u32 = 0x02;
        dst_ext_addr.hi_u32 = 0xacde4800;
        src_short_addr = 0x01;
        src_ext_addr.lo_u32 = 0x01;
        src_ext_addr.hi_u32 = 0xacde4800;
    } else {
        /*
         * Address set up for RX side 
         */
        dst_short_addr = 0x01;
        dst_ext_addr.lo_u32 = 0x01;
        dst_ext_addr.hi_u32 = 0xacde4800;
        src_short_addr = 0x02;
        src_ext_addr.lo_u32 = 0x02;
        src_ext_addr.hi_u32 = 0xacde4800;
    }
    mac_api_mlme_set_req(macPANId, 0, &dst_pan_id);
    mac_api_mlme_set_req(macShortAddress, 0, &src_short_addr);
    mac_api_mlme_set_req(macIeeeAddress, 0, &src_ext_addr);
#ifndef RTX51_TINY_OS
    mac_task(); 
#endif
}

void mac_diag_rx_test (u8 *cmd_buf_p, bit diag, bit stats_display)
{
    u16 promis_en_in = 0;
    u16 pan_coor_in = 0;
    u8  promis_en;
    u8  pan_coor_en;
    u16 ack_en_in = 0;
    uint8_t  params;

    params = sscanf(cmd_buf_p+1, "%d %d %d", &ack_en_in,
                    &promis_en_in, &pan_coor_in);

    if (params == 0 || params > 3) {
        mac_diag_config_addr(FALSE);
        return;
    }

    promis_en = FALSE;
    if (promis_en_in != 0) {
        promis_en = TRUE;
    }
#ifdef HYBRII_ASIC
#ifndef HYBRII_ASIC_A2
#ifndef HYBRII_FPGA_A2
    gv701x_cfg_zb_afe_set_vco_rx(hal_pib_CurrentChannel);
#endif
#endif
#endif
    mac_diag_config_tx_rx(promis_en);

    pan_coor_en = FALSE;
    if (pan_coor_in != 0) {
        pan_coor_en = TRUE;
    }
    mac_hal_pib_set(I_AM_PAN_COORDINATOR, (pib_value_t *)&pan_coor_en);
    
    if (ack_en_in) {
        hal_common_reg_bit_set(ZigCtrl, ZIG_CTRL_ACK_EN);
    } else {
        hal_common_reg_bit_clear(ZigCtrl, ZIG_CTRL_ACK_EN);
    }

    hal_common_reg_bit_clear(ZigCtrl, ZIG_CTRL_ING_PORT_EN);

    mac_diag_display_ctrl_reg();   
    mac_clear_stats();

    if (diag) {
#ifdef ZIGBEE_HW_DIAG_MODE
        mac_diag_rx_diag_mode();
#endif
    } else {
        if (stats_display) {
            mac_diag_rx_normal_mode();
        }
    }
}

void mac_diag_beacon_test (u8 *cmd_buf_p)
{
    uint16_t pan_id_in;
    uint16_t beacon_order_in;
    uint16_t sf_order_in;
    uint16_t pan_coor_in;
    uint16_t tx_pkt_in;
    uint32_t tx_pkts;
    uint32_t tx_bytes;
    uint16_t tx_errors;
    uint32_t tx_bc_pkts;
    uint32_t tx_bc_bytes;
    uint32_t rx_pkts;
    uint32_t rx_bytes;
    uint16_t decrypt_err;
    uint8_t  bc_order;
    uint16_t payload_size;
    uint8_t  max_payload;
    uint32_t tx_req = 1;
    bool     pan_coor;
    uint8_t  abort_count;
    wpan_addr_spec_t addr_spec;
    security_info_t  sec;

    if (sscanf(cmd_buf_p+1, "%x %d %d %d %d", &pan_id_in, &beacon_order_in,
               &sf_order_in, &pan_coor_in, &tx_pkt_in) != 5) {
        printf("\nInvalid cmd params");
        return;
    }
    printf("\nPan ID: %04x, BcnOrder: %u, "
           "SFOrder: %u, PANCoor: %u\n",
            pan_id_in, beacon_order_in,
            sf_order_in, pan_coor_in);
    
    mac_diag_config_tx_rx(FALSE);

    max_payload = aMaxPHYPacketSize - (aMinMPDUOverhead + 12) - FCS_LEN;
    ADDR_COPY_DST_SRC_64(addr_spec.Addr.long_address, dst_ext_addr);
    addr_spec.AddrMode  = FCF_LONG_ADDR;
    addr_spec.PANId     = dst_pan_id;
    if (dst_pan_id != hal_pib_PANId) {
        max_payload -= 2;  /* No PANid Commpress */
    }
          
    bc_order = NON_BEACON_NWK;
    mac_api_mlme_set_req(macBeaconOrder, 0, &bc_order);
#ifndef RTX51_TINY_OS
    mac_task();
#endif

    mac_api_mlme_set_req(macSuperframeOrder, 0, &bc_order);
#ifndef RTX51_TINY_OS
    mac_task();
#endif

    mac_api_mlme_start_req(pan_id_in, hal_pib_CurrentChannel,
                           0, 0, beacon_order_in,
                           sf_order_in, pan_coor_in, FALSE, FALSE,
                           NULL, NULL);

    mac_clear_stats();
    abort_count         = 0;
    payload_size        = 1;
    sec.SecurityLevel   = 0;
    sec.KeyIdMode       = 0;
    sec.KeyIndex        = 0;

    mac_diag_display_ctrl_reg();
    while (1) {
#ifdef RTX51_TINY_OS
        os_switch_task();
#else
        mac_task();
        ISM_PollInt();
        mac_hal_task();
#endif        
        mac_get_tx_stats(&tx_pkts, &tx_bytes, &tx_errors,
                         &tx_bc_pkts, &tx_bc_bytes);
        mac_get_rx_stats(&rx_pkts, &rx_bytes, &decrypt_err);
        printf("\rBcn TX = %lu/%lu - " 
               "TX = %lu/%lu (Err =%u) - "
               "RX = %lu/%lu (DecryptErr =%u)",
               tx_bc_pkts, tx_bc_bytes, tx_pkts, tx_bytes,
               tx_errors, rx_pkts, rx_bytes, decrypt_err);
        if (tx_pkt_in) {
            if (tx_req == (tx_pkts + tx_errors)) {
                if (abort_count) {
                    break;
                }
                if (payload_size > max_payload) {
                    payload_size = 1;
                }
                mac_api_mcps_data_req(FCF_LONG_ADDR,
                                      &addr_spec,
                                      payload_size,
                                      test_tx_data,
                                      msdu_handle++,
                                      TX_CAP_ACK,
                                      //TX_CAP,
                                      &sec);
                tx_req++;
                payload_size++;
            }
        }
        if (abort() == 1) {
            if (tx_pkt_in) {
                if (abort_count++ > 3) {
                    break;
                }
            } else {
                break;
            }
        }
    }
    mac_diag_display_ctrl_reg();
    printf("\ntx_req = %lu\n", tx_req);

    /*
     * Make sure beacon is longer send
     */
    bc_order = NON_BEACON_NWK;
    mac_api_mlme_set_req(macBeaconOrder, 0, &bc_order);
#ifndef RTX51_TINY_OS
    mac_task();
#endif

    pan_coor = FALSE;
    mac_hal_pib_set(I_AM_PAN_COORDINATOR, (void *)&pan_coor);

    mac_hal_zigbee_interrupt_control(FALSE,
                                     CPU_INT_ZB_BC_TX_TIME     |
                                     CPU_INT_ZB_PRE_BC_TX_TIME  );
}

extern uint8_t aes_test_ext_addr[8];

void mac_diag_config (u8 *cmd_buf_p)
{
    uint16_t pan_id_in;
    uint16_t short_addr_in;
    uint64_t ext_addr_in;
    uint16_t sync_state_in;
    uint8_t  params;
    uint8_t  channel;

    params = sscanf(cmd_buf_p+1, "%x %x %lx-%lx %x %x %lx-%lx %d",
                    &pan_id_in, &short_addr_in,
                    &ext_addr_in.hi_u32, &ext_addr_in.lo_u32,
                    &dst_pan_id, &dst_short_addr,
                    &dst_ext_addr.hi_u32, &dst_ext_addr.lo_u32,
                    &sync_state_in);
    if (params != 9 && params != 1) {
        printf("\nInvalid cmd params");
        return;
    }
    if (params == 1) {
        channel = (uint8_t)pan_id_in;
        if (channel < MIN_CHANNEL && channel > MAX_CHANNEL) {
            channel = MIN_CHANNEL;
        }
        printf("\nCh select: %bx (%bd)\n", channel, channel);
        mac_hal_pib_set(phyCurrentChannel, (pib_value_t *)&channel);

        return;
    }

    printf("\nSrc Pan ID: %04x, Src Short Addr: %04x, "
           "Src Ext Addr: %08lx-%08lx", pan_id_in, short_addr_in,
            ext_addr_in.hi_u32, ext_addr_in.lo_u32);  
    printf("\nDst Pan ID: %04x, Dst Short Addr: %04x, "
           "Dst Ext Addr: %08lx-%08lx\n", dst_pan_id, dst_short_addr,
            dst_ext_addr.hi_u32, dst_ext_addr.lo_u32);  

    mac_utils_64_bit_to_byte_array(dst_ext_addr, aes_test_ext_addr);
    mac_api_mlme_set_req(macPANId, 0, &pan_id_in);
#ifndef RTX51_TINY_OS
    mac_task();
#endif
    mac_api_mlme_set_req(macShortAddress, 0, &short_addr_in);
#ifndef RTX51_TINY_OS
    mac_task();
#endif
    mac_api_mlme_set_req(macIeeeAddress, 0, &ext_addr_in);
#ifndef RTX51_TINY_OS
    mac_task(); 
#endif
    mac_api_mlme_set_req(macCoordShortAddress, 0, &dst_short_addr);
#ifndef RTX51_TINY_OS
    mac_task(); 
#endif
    mac_sync_state = sync_state_in;  
}

static void mac_spi_slave_tx_direct (u8 *tx_data_p, u16 data_size,
                                     u16 num_pkts_in)
{
    uint32_t tx_pkts    = 0;
    uint32_t tx_bytes   = 0;
    uint32_t tx_done    = 0;
    uint32_t spi_busy   = 0;
    uint32_t tx_overrun = 0;

    while (1) {
        if (FALSE == hal_common_reg_bit_test(SPI_STATUS, SPI_STATUS_BUSY)) {
            if (hal_common_reg_bit_test(CPU_INTSTATUS_REG,
                                        CPU_INT_SPI_TX_DONE) ||
                tx_pkts == 0) {
                if (tx_pkts != 0) {
                    tx_done++;
                    /* Write 1 to clear */
                    hal_common_reg_32_write(CPU_INTSTATUS_REG,
                                            CPU_INT_SPI_TX_DONE);
                }
                hal_spi_tx_direct(tx_data_p, data_size);
            } else {
                tx_overrun++;
            }
        } else {
            spi_busy++;
        }

        if (abort() == TRUE) {
            break;
        }

        hal_spi_get_tx_stats(&tx_pkts, &tx_bytes);
        if (num_pkts_in && num_pkts_in == tx_pkts) {
            break;
        }
    }
    printf("\nSPI Tx Done = %lu, Tx Overrun = %lu, Busy = %lu",
           tx_done, tx_overrun, spi_busy);
}

static void mac_spi_slave_tx_dma (uint16_t max_data_size, uint16_t inc_bytes,
                                  uint16_t num_pkts_in)
{
    uint32_t tx_pkts    = 0;
    uint32_t tx_bytes   = 0;
    uint32_t tx_done    = 0;
    uint32_t tx_overrun = 0;
    uint32_t spi_busy   = 0;
    uint16_t tx_size    = 0;
    bool     tx_abort;
    uint16_t data_size;
    uint16_t data_size_start = 4;
    uint16_t more_bytes = 0;
    uint16_t tx_errs;
    uint16_t cp_alloc_errs;
    uint16_t idx;

    while (1) {
        if (inc_bytes) {
            more_bytes += inc_bytes;
            if ((more_bytes + 4) > max_data_size) {
                more_bytes = inc_bytes;
                data_size_start = 4;
            } 
        } else {
            data_size_start = max_data_size;
        }
        data_size = data_size_start + more_bytes;
        if (FALSE == hal_common_reg_bit_test(SPI_STATUS, SPI_STATUS_BUSY)) {
            if (hal_common_reg_bit_test(CPU_INTSTATUS_REG,
                                        CPU_INT_SPI_TX_DONE) ||
                tx_pkts == 0) {
                if (tx_pkts != 0) {
                    tx_done++;
                    /* Write 1 to clear */
                    hal_common_reg_32_write(CPU_INTSTATUS_REG,
                                            CPU_INT_SPI_TX_DONE);
                    hal_spi_tx_done_handler();
                }
                if (num_pkts_in && tx_done == num_pkts_in) {
                    break;
                }                                                                
                tx_abort = FALSE;
                for (idx = 0; idx < data_size; idx++) {
                    spi_tx_data[idx] = spi_data++;
                }
                if (hal_spi_tx_dma(spi_tx_data, data_size) == FALSE) {
                    tx_abort = TRUE;
                }
            } else {
                tx_overrun++;
            }
        } else {
            spi_busy++;
        }

        hal_spi_get_tx_stats(&tx_pkts, &tx_bytes);

        if (abort() == TRUE) {
            break;
        }
    }

    hal_spi_get_tx_errors_stats(&tx_errs, &cp_alloc_errs);
    printf("\nSPI Errors: Tx Errors = %u, CP Alloc Errors = %u, "
           "Tx Done = %lu, Tx Overrun = %lu, Busy = %lu",
           tx_errs, cp_alloc_errs, tx_done, tx_overrun, spi_busy);
    hal_spi_tx_cleanup();
}

static void mac_spi_rx (tinybool dma_mode, u16 data_size,
                        u16 num_pkts_in)
{
#ifndef RTX51_TINY_OS
    u32 spi_rx_pkts;
    u32 spi_rx_bytes;
    u16 spi_rx_crc_err;
    u16 spi_rx_bad_cmd;
    u16 spi_rx_bad_len;
    bool abort_test = FALSE;
#endif
 
    dma_mode = dma_mode;
    data_size = data_size;
    num_pkts_in = num_pkts_in;

    hal_spi_set_rx_cmd_len_rdy();
#ifdef RTX51_TINY_OS
    return;
#else
    while (1) {
        if (FALSE == dma_mode) {
            if (hal_common_reg_bit_test(CPU_INTSTATUS_REG,
                                   CPU_INT_SPI_RX_DONE) == TRUE) {
                hal_common_reg_bit_set(CPU_INTSTATUS_REG,
                                       CPU_INT_SPI_RX_DONE);
                hal_spi_rx_done_handler();

                dma_mode = TRUE;
            }
        } else { 
            if (hal_common_reg_bit_test(CPU_INTSTATUS_REG,
                                   CPU_INT_TXQ_NOT_EMPTY) == TRUE) {
                hal_common_reg_bit_set(CPU_INTSTATUS_REG,
                                       CPU_INT_TXQ_NOT_EMPTY);
                CHAL_CpuTxQNemptyIntHandler();
                dma_mode = FALSE;
            }
        }
        hal_spi_get_rx_stats(&spi_rx_pkts, &spi_rx_bytes);
        if (num_pkts_in && spi_rx_pkts == num_pkts_in) {
            break;
        }
        if (abort() == 1) {
            break;
        }
    }
    hal_spi_rx_cleanup();

    hal_spi_get_rx_stats(&spi_rx_pkts, &spi_rx_bytes);                          
    printf("\nRX Packets = %lu, RX Bytes = %lu\n",
           spi_rx_pkts, spi_rx_bytes);
    hal_spi_get_rx_errors_stats(&spi_rx_crc_err,
                                &spi_rx_bad_cmd,
                                &spi_rx_bad_len);
    printf("\nCRC Errors = %u, Invalid Cmd = %u, Bad Length = %u\n",
           spi_rx_crc_err, spi_rx_bad_cmd, spi_rx_bad_len);
#endif
}

void mac_diag_spi_test (u8 *cmd_buf_p)
{
    u32 rx_pkts    = 0;
    u32 rx_bytes   = 0;
    u16 data_size  = 128;
    u8  i;                
    u16 master_slave_in;
    u16 direct_dma_in;
    u16 tx_rx_in;
    u16 data_inc_in;
    u16 num_pkts_in;

    if (sscanf(cmd_buf_p+1, "%d %d %d %d %d %d", &master_slave_in,
               &direct_dma_in, &tx_rx_in, &data_size, 
               &data_inc_in, &num_pkts_in) != 6) {
        hal_spi_stats_show();
        //printf("\nInvalid command parameters");
        return;
    }
    
    if (data_size > MAX_SPI_DATA) {
        data_size = MAX_SPI_DATA;
    }
    
    hal_spi_rx_cleanup();
    hal_spi_tx_cleanup();
    hal_spi_clear_stats();
                      
    if (0 == direct_dma_in) {
        // Direct Mode
        hal_spi_set_direct_mode(master_slave_in, tx_rx_in);

        if (1 == tx_rx_in) {
            for (i = 0; i < 4; i++) {
                spi_tx_data[i] = i + 0x80;
            }
            mac_spi_slave_tx_direct(spi_tx_data, 4,
                                    num_pkts_in);
        } else {
            mac_spi_rx(direct_dma_in == 1, 4,
                       num_pkts_in);
        }
    } else {
        if (1 == tx_rx_in) {
            mac_spi_slave_tx_dma(data_size, data_inc_in,
                                 num_pkts_in);
        } else {
            mac_spi_rx(direct_dma_in == 1, data_size,
                       num_pkts_in);
        }
    }
}

void mac_diag_display_intr (void)
{
    printf("TimerIntCnt = %lu\n", gHalCB.timerIntCnt);
    printf("ExtIntCnt   = %lu\n", gHalCB.extIntCnt);
}

#ifdef Z_P_BRIDGE
void mac_diag_zb_plc_bridge_config (u8 *cmd_buf_p)
{
    uint8_t bridge_cfg_in;

    if (sscanf(cmd_buf_p+1, "%bd", &bridge_cfg_in) != 1) {
        printf("\nInvalid cmd params");
        return;
    }
    mac_hal_bridge_config(bridge_cfg_in);
}
#endif

#ifdef _LED_DEMO_
void mac_diag_remote_led_test (uint8_t *cmd_buf_p)
{
    //u16 led_state, led_color, letter, loop;
    u16 loop;
    wpan_addr_spec_t addr_spec;
    security_info_t  sec;
    u8  len;
    u32 tx_pkts;
    u32 tx_bytes;
    u32 tx_bc_pkts;
    u32 tx_bc_bytes;
    u16 tx_errors;
    u8  i = 0;

    loop = 3;

    cmd_buf_p++;
    while (cmd_buf_p[i++] == 0x20);
    if (i) {
        i--;
    }
    cmd_buf_p = &cmd_buf_p[i];
    len = strlen(cmd_buf_p) + 1;
    //printf("\ncmd = <%s> (%bu)", cmd_buf_p, len);

    mac_diag_config_tx_rx(FALSE);
    

    EXT_ADDR_CLEAR(addr_spec.Addr.long_address);
    addr_spec.Addr.short_address = dst_short_addr;
    addr_spec.AddrMode  = FCF_SHORT_ADDR;
    addr_spec.PANId     = dst_pan_id;
    sec.SecurityLevel   = 0;
    sec.KeyIdMode       = 0;
    sec.KeyIndex        = 0;

    while (loop--) {
        mac_clear_stats();
        mac_api_mcps_data_req(FCF_SHORT_ADDR,
                              &addr_spec,
                              len,
                              cmd_buf_p,
                              0,
                              TX_CAP,
                              //TX_CAP_ACK,
                              &sec);
        
        mac_pib_macDSN--;
        mac_utils_delay_ms(1);
        while (1) {
#ifdef RTX51_TINY_OS
            os_switch_task();
#else
            mac_task();
            ISM_PollInt();
#endif
            mac_get_tx_stats(&tx_pkts,
                             &tx_bytes, &tx_errors,
                             &tx_bc_pkts, &tx_bc_bytes);
            if ((tx_pkts + tx_errors) >= 1) {
                break;
            }
            if (abort() == TRUE) {
                break;
            }
        }
    }
    mac_pib_macDSN++;
}
#endif

void mac_diag_init (void)
{
    uint8_t i, c;
	
    for (i = 0; i < 128; i++) {
        if (i < 0x61) {
            c = 0x61 + i;
        } else {
            c = i;
        }
        test_tx_data[i] = c;
    }

    for (i = 0; i < SEC_KEY_SRC_MAX; i++) {
        test_key_source[i] = i + 1;
    }
    
    mac_diag_config_addr(TRUE);   	
}

void mac_diag_zb_cmd_help (void)
{
#ifdef ZIGBEE_HELP
    printf
    (
        "  zs             Display packets statistics\n"
        "  zS             Test Zigbee Transmit Security Packets\n"
        "                 Usage: zS <size> <byte increment> "
        "<number of pkts> <inter pkt delay ms> <ack req (0-1)>\n"
        "                 <pkt-type (0-3)> <bc template (0-1)> "
        "<sec key index> <include no dst addr (0-1)>\n"
#ifdef HYBRII_B
        "  za             Program Promiscuous ACK Address Table\n"
        "                 Usage: za s(how)|e(nable)|d(isable)|m(ode)|a(dd)|r(emove)\n"
        "                           s(hort)|e(xtend) s(ource)|d(estination)\n"  
        "  ze             Energy Detection Scan\n"
        "                 Usage: <Channel Bit MAP (hhhhhhhh)> <Scan Duration (0-14)>\n"
        "  zg             GPIO Test\n"
        "                 Usage: zg <pin (0-9)> <in/out (0/1)> <output (0/1)>\n"
#endif
        "  zi             SPI Test\n"
        "                 Usage: zi <slave/master (0-1)> <direct/dma <0-1)> "
        "<Rx/Tx (0-1)> <Size> <Increment by> <number of packets>\n"
        "  zc             Configure Pan ID, Short, Extended Address\n"
        "                 Usage: zc <Src PanID (hhhh)> <Src Short Addr (hhhh)> "
        "<Src Ext Addr (hhhhhhhh hhhhhhhh)> <Dst PanID (hhhh)> "
        "<Dst Short Addr (hhhh)> <Dst Ext Addr (hhhhhhhh-hhhhhhhh)\n"
        "  zb             Beacon Test (Send START-REQ)\n"
        "                 Usage: zb <PanID (hh hh)> <Beacon Order (0-15)> "       
        "<Superframe Order (0-15)> <PanCoordinator (0 or 1) Send Pkts <0-1>\n"
#ifdef Z_P_BRIDGE        
        "  zB             Enable/Disable Bridging\n"
        "                 Usage: zB 1/0\n"
#endif
        "  zt             Test Zigbee Transmit\n"
        "                 Usage: zt <size> <byte increment> "
        "<number of pkts> <inter pkt delay ms> <ack req (0-1)>\n"
        "                 <src addr mode (0-2)> <dst addr_mode (0-2)> "
        "<pkt-type (0-3)> <bc template <0-1)>\n"
        "                 <sec level (0-7)> <sec key id_mode (0-3)> "
        "<sec key index>\n"
#ifdef ZIGBEE_HW_DIAG_MODE
        "  zd             Test Zigbee Transmit in diag mode\n"
        "  zy             Test Zigbee Receive in diag mode\n"
#endif
        "  zp             Test Zigbee RX enable\n"
        "                 Usage: <On Start> <On duration> <defer (0-1)>\n"
        "  zq             Test Zigbee Receive with statistics display\n"
        "                 Usage: zq <ack-en (0-1)> <promis en (0-1)> "
        "<pan-coor (0-1)>\n"
        "  zr             Test Zigbee Receive without statistics display\n"
        "                 Usage: zq <ack-en (0-1)> <promis en (0-1)> "
        "<pan-coor (0-1)>\n"
#ifdef _LED_DEMO_
        "  zl             Remotely control light\n"
        "                 Usage: zl <state (0-1)> <color (0-2)> <letter (0-1>\n"
#endif
        "\n"
    );
#endif /* ZIGBEE_HELP */
}

void mac_diag_rx_enable (u8 *cmd_buf_p)
{
   uint32_t on_start;
   uint32_t on_dur;
   uint8_t  defer;

   if (sscanf(cmd_buf_p+2, "%lu %lu %c", &on_start, &on_dur,
              &defer) != 3) {
        printf("\nInvalid cmd params");
        return;
    } 
    mac_api_mlme_rx_enable_req(defer, on_start, on_dur);
}

#ifdef HYBRII_B
void mac_diag_ack_table_config (u8 *cmd_buf_p)
{
    uint8_t  action;
    uint8_t  s_or_x;
    uint8_t  s_or_d;
    uint16_t short_addr;
    uint64_t ext_addr;
    uint8_t  input[20];

    if (sscanf(cmd_buf_p+2, "%c %c %c", &action, &s_or_x,
               &s_or_d) != 3) {
        printf("\nInvalid cmd params");
        return;
    }
 
    if (action == 's') {         /* Show promiscuous Ack tables */
        mac_hal_promis_ack_table_show();
        return;
    } else if (action == 'e') {   /* Promiscuous Ack ensable */
        printf("Enable Promiscuous Ack\n");
        mac_hal_promis_ack_control(TRUE);
        return;
    } else if (action == 'd') {   /* Promiscuous Ack disable */
        printf("Disable Promiscuous Ack\n");
        mac_hal_promis_ack_control(FALSE);
        return;
    } else if (action == 'm') {   /* Match mode config */
        printf("Config Match Mode: ");
        if (s_or_d == 's') {
            mac_hal_promis_ack_set_match_mode(MATCH_SRC);
            printf("Source\n");
        } else if (s_or_d == 'd') {
            mac_hal_promis_ack_set_match_mode(MATCH_DST);
            printf("Destination\n");
        } else if (s_or_d == 'b'){
            mac_hal_promis_ack_set_match_mode(MATCH_DST_SRC);
            printf("Source and Destination\n");
        } else {
            mac_hal_promis_ack_set_match_mode(MATCH_NONE);
            printf("None\n");
        }
        return;
    }

    if (s_or_x == 's') {
        printf("Enter Short Address: ");
        while (getline(input, sizeof(input)) > 0) {
            if (sscanf(input, "%x", &short_addr) >= 1) {
                break;
            }
        }
    } else {
        printf("Enter Ext Addr Address: ");
        while (getline(input, sizeof(input)) > 0) {
            if (sscanf(input, "%lx-%lx",
                       &ext_addr.hi_u32, &ext_addr.lo_u32) >= 1) {
                break;
            }
        }
    }

    if (action == 'a') {
        printf("Add ");
        if (s_or_x == 's') {
            if (s_or_d == 's') {
                printf("Source ");
                mac_hal_promis_ack_short_addr_add(TRUE, short_addr);
            } else {
                printf("Destination ");
                mac_hal_promis_ack_short_addr_add(FALSE, short_addr);
            }
            printf("Short Address: %04x\n", short_addr);

        } else {
            if (s_or_d == 's') {
                printf("Source ");
                mac_hal_promis_ack_ext_addr_add(TRUE, ext_addr);
            } else {
                printf("Destination ");
                mac_hal_promis_ack_ext_addr_add(FALSE, ext_addr);
            }
            printf("Ext Address: %08lx-%08lx\n", 
                    ext_addr.hi_u32, ext_addr.lo_u32);
        }
    } else {
        printf("Delete ");
        if (s_or_x == 's') {
            if (s_or_d == 's') {
                printf("Source Short Address: %04x\n", short_addr);
                mac_hal_promis_ack_short_addr_del(TRUE, short_addr);
            } else {
                printf("Destination Short Address: %04x\n", short_addr);
                mac_hal_promis_ack_short_addr_del(FALSE, short_addr);
            }

        } else {
            if (s_or_d == 's') {
                printf("Source Ext Address: %08lx-%08lx\m", 
                       ext_addr.hi_u32, ext_addr.lo_u32);
                mac_hal_promis_ack_ext_addr_del(TRUE, ext_addr);
            } else {
                printf("Destination Ext Address: %08lx-%08lx\m", 
                       ext_addr.hi_u32, ext_addr.lo_u32);
                mac_hal_promis_ack_ext_addr_del(FALSE, ext_addr);
            }
        }
    }
}

void mac_diag_ed_scan (u8 *cmd_buf_p)
{
    uint32_t channel_map;
    uint8_t  scan_duration;
    if (sscanf(cmd_buf_p+1, "%lx %bd", &channel_map, &scan_duration) != 2) {
        printf("\nInvalid cmd params");
        return;
    }
    mac_api_mlme_scan_req(MLME_SCAN_TYPE_ED, channel_map, scan_duration,
                          0, NULL);
}

extern uint8_t gpio_int;

void mac_diag_gpio_test (u8 *cmd_buf_p)
{
    uint8_t  gpio_pin;
    uint8_t  io;
    uint8_t  value = 1;
    uint32_t gpio_cfg;
    
    if (sscanf(cmd_buf_p+1, "%bd %bd %bd", &gpio_pin, &io, &value) < 2) {
        printf("\nInvalid cmd params");
        return;
    }
    gpio_cfg = BIT(gpio_pin);
    if (io == 0) {   /* Configure for Input */
#ifdef GPIO_FPGA
        /* Input = 0 */
        hal_common_reg_bit_clear(CPU_GPIO_REG, gpio_cfg);
#else
        /* Input = 1 */
        hal_common_reg_bit_set(CPU_GPIO_REG, gpio_cfg);
#endif
        gpio_cfg = BIT(gpio_pin + 20);
        printf("\nGPIO pin %bd input ", gpio_pin);
        if (hal_common_reg_bit_test(CPU_GPIO_REG, gpio_cfg)) {
            printf("HIGH\n");
        } else {
            printf("LOW\n");
        }
        if (gpio_pin < 6) {
            printf("CPU Interrupt Status for GPIO pin %bd ", gpio_pin);
            if (hal_common_reg_8_bit_test((uint8_t)&gpio_int, 
                                          BIT(gpio_pin))) {
                printf("SET\n");
            } else {
                printf("CLEAR\n");
            }
            gpio_int = 0;
        }
    } else {         /* Configure for Ouput */
#ifdef GPIO_FPGA
        /* Output = 1 */
        hal_common_reg_bit_set(CPU_GPIO_REG, gpio_cfg);
#else
        /* Output = 0 */
        hal_common_reg_bit_clear(CPU_GPIO_REG, gpio_cfg);
#endif
        gpio_cfg = BIT(gpio_pin + 10);
        printf("\nGPIO pin %bd output ", gpio_pin);
        if (0 == value) {
            printf("LOW\n");
            hal_common_reg_bit_clear(CPU_GPIO_REG, gpio_cfg);
        } else {
            printf("HIGH\n");
            hal_common_reg_bit_set(CPU_GPIO_REG, gpio_cfg);
        }
    }
}

#endif

void mac_diag_show (u8 *cmd_buf_p)
{
    uint8_t  params;
    uint8_t display_in;

    params = sscanf(cmd_buf_p+1, "%bu", &display_in);

    if (params == 1) {
        if (display_in == 0) {
            display_stats = FALSE;
        } else {
            display_stats = TRUE;
        }
        return;
    }
    mac_diag_show_config();
    mac_diag_display_tx_stats();
}
#ifdef PROD_TEST
u8 txControl = 0;
#endif
void mac_diag_zb_cmd (char* cmd_buf_p)
{
    char action;

    if (sscanf(cmd_buf_p+1, "%c", &action) < 1) {
        mac_diag_zb_cmd_help();
        return;
    }

    switch (action) {
#ifdef HYBRII_B
        case 'a':
        {
            //uint64_t ext_addr;

            mac_diag_ack_table_config(cmd_buf_p+1);
#if 0
            /* Program Source Address Table */
            mac_hal_promis_ack_short_addr_add(TRUE, 0x00);
            ext_addr.lo_u32 = 0x001ab090;
            ext_addr.hi_u32 = 0x00158d00;
            mac_hal_promis_ack_ext_addr_add(TRUE, ext_addr);
            mac_hal_promis_ack_short_addr_add(TRUE,  0x11);

            /* Program Destination Address Table */
            mac_hal_promis_ack_short_addr_add(FALSE, 0x00);
            mac_hal_promis_ack_ext_addr_add(FALSE, ext_addr);

            mac_hal_promis_ack_short_addr_add(FALSE, 0x02);
            mac_hal_promis_ack_short_addr_add(FALSE, 0x03);
            mac_hal_promis_ack_short_addr_add(FALSE, 0x04);
            mac_hal_promis_ack_short_addr_add(FALSE, 0x05);
            mac_hal_promis_ack_short_addr_add(FALSE, 0x06);
            mac_hal_promis_ack_short_addr_add(FALSE, 0x07);
            mac_hal_promis_ack_short_addr_add(FALSE, 0x08);

            mac_hal_promis_ack_set_match_mode(MATCH_DST);
            mac_hal_promis_ack_control(TRUE);
            mac_hal_promis_ack_table_show();
#endif
            break;
        }

    case 'e':
        mac_diag_ed_scan(cmd_buf_p+1);
        break;

    case 'E':
        printf("\nErase Flash Config");
#ifndef PROD_TEST		
        spiflash_eraseConfigMem();
#else        
		spiflash_eraseConfigMem();
        spiflash_eraseSector(PROD_CONFIG_SECTOR);
		spiflash_wrsr_unlock((u8)0);
#endif		
        break;

    case 'g':
        mac_diag_gpio_test(cmd_buf_p+1);
        break;
#endif

    case 'p':
        mac_diag_rx_enable(cmd_buf_p+1);
        break;

    case 's':
        mac_diag_show(cmd_buf_p+1);
        break;

    case 'S':
        mac_diag_tx_security_test(cmd_buf_p+1);
        break;

#ifdef ZIGBEE_HW_DIAG_MODE		
    case 'D':
    case 'd':
        mac_diag_tx_diag_mode_test();
        break;

    case 'Y':
    case 'y':
        mac_diag_rx_test(cmd_buf_p+1, 1, FALSE);
        break;
#endif

    
#ifdef PROD_TEST
	case 'T':
		txControl = !txControl;
		if(txControl){
			STM_StartTimer(prod_test_rf_timer,500);
			printf("\nTest Started\n");
		}
		else{
			STM_StopTimer(prod_test_rf_timer);
			printf("\nTest Stopped\n");
		}
	break;

	case 'l':
	case 'L':
		{
			u8 cmd_arg[64];

			memset(&gProdFlashProfile,0x00,sizeof(sProdConfigProfile));
			FM_Printf(FM_USER,"\n@0x23:");				
			while (getline(cmd_arg, sizeof(cmd_arg)) > 0)
			{
				if(sscanf(cmd_arg,"%bx",&gProdFlashProfile.rfProfile.calRegister.reg23) >= 1)
				break;
			}
			
			memset(cmd_arg, 0x00, 10);	
			FM_Printf(FM_USER,"\n@0x24:");				
			while (getline(cmd_arg, sizeof(cmd_arg)) > 0)
			{
				if(sscanf(cmd_arg,"%bx",&gProdFlashProfile.rfProfile.calRegister.reg24) >= 1)
				break;
			}	
			gProdFlashProfile.signature = PROD_VALID_SIGNATURE;
			gProdFlashProfile.rfProfile.rfCalStatus = RF_CALIBRATED;
			gProdFlashProfile.rfProfile.rfCalAttemptCount = 0;
			gProdFlashProfile.rfProfile.testActionPreparePending = 0;
			gProdFlashProfile.rfProfile.autoCalibrated = RF_CAL_MANUAL;
			//gProdFlashProfile.checksum =  Gv701x_CalcCheckSum16((u8*)&gProdFlashProfile,
				//(sizeof(sProdConfigProfile) - sizeof(gProdFlashProfile.checksum)));
			gProdFlashProfile.crc =	chksum_crc32 ((u8*)&gProdFlashProfile.testIntf, (sizeof(sProdConfigProfile) - sizeof(gProdFlashProfile.crc)\
				-sizeof(gProdFlashProfile.signature)));
			FM_HexDump(FM_USER,"Flash Profile",(u8 *)&gProdFlashProfile,(sizeof(sProdConfigProfile)));
			Gv701x_FlashWriteProdProfile(PROD_CONFIG_SECTOR,&gProdFlashProfile);
		}
	break;	
#endif		
		
    case 't':
        mac_diag_tx_test(cmd_buf_p+1);
        break;

    case 'Q':
    case 'q':
        mac_diag_rx_test(cmd_buf_p+1, 0, TRUE);
        break;
			       
    case 'R':
    case 'r':
        mac_diag_rx_test(cmd_buf_p+1, 0, FALSE);
        break;

    case 'b':
        mac_diag_beacon_test(cmd_buf_p+1);
        break;

#ifdef Z_P_BRIDGE
    case 'B':
        mac_diag_zb_plc_bridge_config(cmd_buf_p+1);
        break;
#endif

    case 'c':
    case 'C':
        mac_diag_config(cmd_buf_p+1);
        break;

    case 'i':
        mac_diag_spi_test(cmd_buf_p+1);
        break;

    case 'I':
        mac_diag_display_intr();
        break;

    case 'u':
        mac_hal_mac_busy_recover();
        break;
        
    case 'z':
        GV701x_Chip_Reset();
        break; 

#ifdef _LED_DEMO_
    case 'l':
        mac_diag_remote_led_test(cmd_buf_p+1);
        break;
#endif

    case 'H':
    case 'h':
    default:
       mac_diag_zb_cmd_help();
       break;	
	}
}
#endif //(defined HYBRII_802154) && (defined ZBMAC_DIAG)