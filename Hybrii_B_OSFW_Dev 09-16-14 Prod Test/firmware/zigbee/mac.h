/**
 * @file mac.h
 *
 * IEEE MAC defines 
 *
 * $Id: mac.h,v 1.1 2014/01/10 17:27:11 yiming Exp $
 *
 * Copyright (c) 2011, Greenvity Communication All rights reserved.
 *
 */
#ifndef _MAC_H_
#define _MAC_H_

/* === Includes ============================================================= */

/* === Macros =============================================================== */

#define MAX_ALLOWED_PAN_DESCRIPTORS \
    ((BUFFER_SIZE - sizeof(mlme_scan_conf_t)) / sizeof(pandescriptor_t))
/**
 * Active/passive scan: implementation-defined maximum number of
 * PANDescriptors that can be stored.
 */
#define MAX_PANDESCRIPTORS \
        (MAX_ALLOWED_PAN_DESCRIPTORS > 5 ? 5 : MAX_ALLOWED_PAN_DESCRIPTORS)


/*
 * Defines the mask for the FCF address mode
 */
#define FCF_ADDR_MASK                   (3)

/*
 * Macro to get the source address mode.
 */
#define FCF_GET_SOURCE_ADDR_MODE(x) \
    (((x) >> FCF_SOURCE_ADDR_OFFSET) & FCF_ADDR_MASK)

/*
 * Macro to get the destination address mode.
 */
#define FCF_GET_DEST_ADDR_MODE(x)\
    (((x) >> FCF_DEST_ADDR_OFFSET) & FCF_ADDR_MASK)

/* === Types ================================================================ */

/* === Externals ============================================================ */
extern uint8_t mac_pib_macDSN;
extern queue_t nhle_mac_q;

retval_t mac_reset(uint8_t init_pib);
/* === Prototypes =========================================================== */

/** MAC API
 *
 * This module describes all MAC APIs to access the MAC functionality.
 *
 */

/* 802.15.4 MAC layer entries */
void mcps_data_request(buffer_t *buf_p);
void mcps_data_conf(buffer_t *buf_p);
void mcps_data_ind(buffer_t *buf_p);
void mcps_purge_request(buffer_t *buf_psg);
void mcps_purge_conf(buffer_t *buf_p);
void mlme_get_request(buffer_t *buf_p);
void mlme_get_conf(buffer_t *buf_p);
void mlme_reset_request(buffer_t *buf_p);
void mlme_scan_request(buffer_t *buf_p);
void mlme_scan_conf(buffer_t *buf_p);
void mlme_start_request(buffer_t *buf_p);
void mlme_start_conf(buffer_t *buf_p);
void mlme_associate_request(buffer_t *buf_p);
void mlme_associate_conf(buffer_t *buf_p);
void mlme_associate_response(buffer_t *buf_p);
void mlme_associate_ind(buffer_t *buf_p);
void mlme_disassociate_request(buffer_t *buf_p);
void mlme_disassociate_conf(buffer_t *buf_p);
void mlme_disassociate_ind(buffer_t *buf_p);
void mlme_orphan_response(buffer_t *buf_p);
void mlme_orphan_ind(buffer_t *buf_p);
void mlme_poll_request(buffer_t *buf_p);
void mlme_poll_conf(buffer_t *buf_p);
void mlme_rx_enable_request(buffer_t *buf_p);
void mlme_rx_enable_conf(buffer_t *buf_p);
void mlme_sync_request(buffer_t *buf_p);
void mlme_beacon_notify_ind(buffer_t *buf_p);
void mlme_comm_status_ind(buffer_t *buf_p);
void mlme_reset_conf(buffer_t *buf_p);
void mlme_sync_loss_ind(buffer_t *buf_p);
void mlme_set_request(buffer_t *buf_p);
void mlme_set_conf(buffer_t *buf_p);
retval_t mlme_set(uint8_t attribute, uint8_t attribute_index,
                  pib_value_t *attribute_value, bool set_trx_to_sleep);

#endif /* _MAC_H_ */
