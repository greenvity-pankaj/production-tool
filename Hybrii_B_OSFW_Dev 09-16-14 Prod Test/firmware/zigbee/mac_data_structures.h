/**
 * @file mac_data_structures.h
 *
 * This file contains MAC related data structures, types and enums.
 *
 * $Id: mac_data_structures.h,v 1.1 2014/01/10 17:27:11 yiming Exp $
 *
 * Copyright (c) 2011, Greenvity Communication All rights reserved.
 *
 */

/* Prevent double inclusion */
#ifndef _MAC_DATA_STRUCTURES_H_
#define _MAC_DATA_STRUCTURES_H_

/* === Includes ============================================================= */

/* === Macros =============================================================== */

/* === Types ================================================================ */

/**
 * Beacon Payload type
 */
typedef struct mac_beacon_payload_s
{
    uint16_t superframe_spec;
    uint8_t  gts_spec;
    uint8_t  pending_addr_spec;
    uint8_t  *pending_addr_list_p;
    uint8_t  beacon_payload_len;
    uint8_t  *beacon_payload_p;
} mac_beacon_payload_t;

/**
 * Data Payload type
 */
typedef struct mac_data_payload_s
{
    uint8_t *payload_p;
} mac_data_payload_t;

/**
 * Association Request type
 */
typedef struct mac_assoc_req_s
{
    uint8_t capability_info;
} mac_assoc_req_t;

/**
 * Association Response type
 */
typedef struct mac_assoc_response_s
{
    uint16_t short_addr;
    uint8_t  assoc_status;
} mac_assoc_response_t;

/**
 * Disassociation Request type
 */
typedef struct mac_disassoc_req_s
{
    uint8_t disassoc_reason;
} mac_disassoc_req_t;

/**
 * Coordinator Realignment type
 */
typedef struct mac_coord_realign_s_
{
    uint16_t pan_id;
    uint16_t coord_short_addr;
    uint8_t  logical_channel;
    uint16_t short_addr;
    uint8_t  channel_page;
} mac_coord_realign_t;

/**
 * General Command frame payload type
 */
typedef union {
    mac_beacon_payload_t  beacon_data;
    mac_data_payload_t    payload_data;
    mac_assoc_req_t       assoc_req_data;
    mac_assoc_response_t  assoc_response_data;
    mac_disassoc_req_t    disassoc_req_data;
    mac_coord_realign_t   coord_realign_data;
} frame_payload_t;

/**
 * Structure containing auxiliary security header information
 */
typedef struct sec_ctrl_s
{
    uint8_t sec_level : 3;
    uint8_t key_id_mode : 2;
    uint8_t /* reserved */ : 3;
} sec_ctrl_t;


typedef struct parse_s
{
    uint16_t         fcf;
    uint8_t          frame_type;
    uint8_t          mpdu_length;
    uint8_t          sequence_number;
    uint8_t          dest_addr_mode;
    uint16_t         dest_panid;
    address_field_t  dest_addr;
    uint8_t          src_addr_mode;
    uint16_t         src_panid;
    address_field_t  src_addr;
    sec_ctrl_t       sec_ctrl;
    uint8_t          key_id_len;
    uint32_t         frame_cnt;
    uint8_t          key_id[MAX_KEY_ID_FIELD_LEN];
    uint8_t          mac_command;
    uint8_t          ppdu_link_quality;
    uint32_t         time_stamp;
    uint8_t          mac_payload_length;  /* Length of the MAC payload no FCS */
    frame_payload_t  mac_payload_data;
} parse_t;

/* === Externals ============================================================ */


/* === Prototypes =========================================================== */

#endif /* _MAC_DATA_STRUCTURES_H_ */
