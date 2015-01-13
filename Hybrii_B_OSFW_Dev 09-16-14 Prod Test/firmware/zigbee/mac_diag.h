/**
 * @file mac_diag.h 
 *
 * Diagnostic module for 802.15.4 
 *
 * $Id: mac_diag.h,v 1.1 2014/01/10 17:27:11 yiming Exp $
 *
 * Copyright (c) 2011, Greenvity Communication All rights reserved.
 *
 */
#ifndef _MAC_DIAG_H_
#define _MAC_DIAG_H_

/* === Includes ============================================================= */

/* === Macros =============================================================== */

typedef struct mac_diag_tx_params_s {
    u16 size_inc_in;
    u16 pkt_count_in;
    u16 pkt_size_in;
    u16 inter_pkt_delay_in;
    u16 payload_size;
    u16 ack_req_in;
    u16 src_addr_mode_in;
    u16 dst_addr_mode_in;
    u16 tx_pkt_type_in;
    u16 beacon_template_in;
    u16 sec_level_in;
    u16 sec_id_mode_in;
    u16 sec_idx_in;
    bool show_stats;
} mac_diag_tx_params_t;

void mac_diag_subcmd_processing(uint8_t *cmd_buf_p);
void mac_diag_init(void);
void mac_diag_cmd(void);
void mac_diag_regs_test(void);
void mac_diag_tx_test(uint8_t *cmd_buf_p);
void mac_diag_tx_security_test(uint8_t *cmd_buf_p);
void mac_diag_rx_test(uint8_t *cmd_buf_p, bit diag, bit q_control);
void mac_diag_tx_diag_mode_test(void);
void mac_diag_config(uint8_t *cmd_buf_p);
void mac_diag_beacon_test(uint8_t *cmd_buf_p);
void mac_diag_spi_test(uint8_t *cmd_buf_p);
void mac_diag_remote_led_test(uint8_t *cmd_buf_p);
void mac_diag_remote_led_number	(uint8_t *cmd_buf_p);
void mac_diag_zb_cmd(char* cmd_buf_p);
#endif /* _MAC_DIAG_H_ */
