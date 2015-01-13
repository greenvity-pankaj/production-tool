/**
 * @file mac_security.c
 *
 * Handles MAC security
 *
 * $Id: mac_security.c,v 1.1 2014/01/10 17:27:11 yiming Exp $
 *
 * Copyright (c) 2011, Greenvity Communication All rights reserved.
 *
 */

/* === Includes ============================================================ */

#include <string.h>
#include "papdef.h"
#include "return_val.h"
#include "bmm.h"
#include "qmm.h"
#include "mac_const.h"
#include "mac_msgs.h"
#include "mac_data_structures.h"
#include "mac_hal.h"
#include "mac_internal.h"
#include "mac_security.h"
#include "utils.h"

/* === Macros =============================================================== */

/* Security Control Field: Security Level mask */
#define SEC_CTRL_SEC_LVL_MASK           (0x07)

/* Security Control Field: Key Identifier mask */
#define SEC_CTRL_KEY_ID_MASK            (0x03)

/* Security Control Field: Key Identifier Field position */
#define SEC_CTRL_KEY_ID_FIELD_POS       (3)

uint8_t aes_test_key[] = {0xC0, 0xC1, 0xC2, 0xC3, 0xC4, 0xC5, 0xC6, 0xC7,
0xC8, 0xC9, 0xCA, 0xCB, 0xCC, 0xCD, 0xCE, 0xCF};
uint8_t aes_test_ext_addr[8];

/* === Globals ============================================================= */

/* === Prototypes ========================================================== */

/* === Implementation ====================================================== */



/* --- Helper Functions ---------------------------------------------------- */

/*
 * Gets the length of the Key Identifier field
 *
 * This function returns the length of the Key Identifier field
 * within the Auxiliary Security Header of a secured frame based
 * on the Key Identifier Mode.
 *
 * key_id_mode Key Identifier Mode
 *
 * Length of Key Identifier field in octets.
 */
static uint8_t get_key_id_field_len (uint8_t key_id_mode)
{
    uint8_t len_field = 0;

    switch (key_id_mode) {
    case 1:
        len_field = 1;
        break;
    case 2:
        len_field = 5;
        break;
    case 3:
        len_field = 9;
        break;
    case 0:
    default:
        break;
    }

    return (len_field);
}

/**
 * Calculates the length of the MIC
 *
 * This function returns the length of the MIC depending on the given 
 * security level
 *
 * security_level Security Level of current frame
 *
 * Length of MIC in octets.
 */
uint8_t get_mic_length (uint8_t sec_level)
{
    uint8_t mic_len = 0;

    if (sec_level & 3) {
        mic_len = 1 << ((sec_level & 3) + 1);
    } else {
        mic_len = 0;
    }

    return (mic_len);
}

uint8_t sec_additional_len (security_info_t *sec_info_p)
{
    uint8_t len;

    /* Auxiliary security header length */
    len  = 1 +    /* security ctrl field */
           4 +    /* frame counter length */
           get_key_id_field_len(sec_info_p->KeyIdMode);

    return (len);
}

retval_t mac_security_search_key (uint8_t *key_lookup_data, 
                                  uint8_t key_lookup_size,
                                  uint8_t **key, uint8_t **addr)
{
    uint8_t i, k;

    // FIXME - Use test key for now
    *key = aes_test_key;
    if (addr) {
        *addr = aes_test_ext_addr;
    }
    return (MAC_SUCCESS);
    // FIXME - End of test code

    // Get key from KeyDescriptor as 7.5.8.2.5
    for (i = 0; i < mac_sec_pib.KeyTableEntries; i++) {
        for (k = 0; k < mac_sec_pib.KeyTable[i].KeyIdLookupListEntries; k++) {
            if (mac_sec_pib.KeyTable[i].KeyIdLookupList[k].LookupDataSize ==
                key_lookup_size) {
                uint16_t compare;
                uint8_t len;

                if (0 == key_lookup_size) {
                    len = 5;
                } else if (1 == key_lookup_size) {
                    len = 9;
                } else {
                    return (MAC_UNSUPPORTED_SECURITY);
                } 
                compare =
                   memcmp(mac_sec_pib.KeyTable[i].KeyIdLookupList[k].LookupData,
                          key_lookup_data, len);
                if (compare == 0) {
                    *key = mac_sec_pib.KeyTable[i].Key;
                    if (addr) {
                        *addr = (uint8_t *)
                                &mac_sec_pib.DeviceTable[i].\
                                DeviceDescriptor[i].ExtAddress;
                    }
                    return (MAC_SUCCESS);
                }
            }
        }
    }

    return (MAC_UNAVAILABLE_KEY);
}

/* --- Security Features ------------------------------------------ */

/*
 * Generates Auxiliary Security Header fields
 *
 * This function generates the required fields of the
 * Auxiliary Security Header of a secured frame based
 * on the actual security parameter.
 *
 * Status of extraction of Auxiliary Security Header fields
 */
retval_t mac_build_aux_sec_header (uint8_t **frame_ptr,
                                   security_info_t *sec_info_p,
                                   uint8_t *frame_len)
{
    uint8_t sec_info_len = sec_additional_len(sec_info_p);
    uint8_t *sec_msdu_ptr;

    *frame_ptr = *frame_ptr - sec_info_len;
    /* Add security field length to original frame length */
    *frame_len += sec_info_len;
	sec_msdu_ptr = *frame_ptr;   /* Create Auxiliary security header. */

    /* Fill in Security Control Field. */
    *sec_msdu_ptr = (sec_info_p->SecurityLevel & SEC_CTRL_SEC_LVL_MASK) |
                    ((sec_info_p->KeyIdMode & SEC_CTRL_KEY_ID_MASK) <<
                      SEC_CTRL_KEY_ID_FIELD_POS);
    sec_msdu_ptr++;

    /* Fill in Frame Counter. */
    if (mac_sec_pib.FrameCounter == 0xFFFFFFFF) {
        return (MAC_COUNTER_ERROR);
    } else {
        mac_utils_32_bit_to_byte_array(mac_sec_pib.FrameCounter, sec_msdu_ptr);
        sec_msdu_ptr += 4;
    }

    /*
     * Format the Key Identifier according to 802.15.4-2006
     * section 7.5.8.2.1 h) 4) and 7.6.2.4
     * Key Identifier contains Key Source (when KeyIdMode is > 1) and
     * Key Index subfields
     * Key Identifier is only present when KeyIdMode is not zero.
     */
    if (sec_info_p->KeyIdMode == 1) {
        *sec_msdu_ptr = sec_info_p->KeyIndex;
    } else if (sec_info_p->KeyIdMode > 1) {
        uint8_t i;
        uint8_t key_src_size;

        if (sec_info_p->KeyIdMode == 2) {
            key_src_size = 4;
        } else if (sec_info_p->KeyIdMode == 3) {
            key_src_size = 8;
        } else {
            return (MAC_UNSUPPORTED_SECURITY);
        }
        for (i = 0; i < key_src_size; i++) {
            *sec_msdu_ptr++ = sec_info_p->KeySource[i];
        }
        *sec_msdu_ptr = sec_info_p->KeyIndex;
    }

    return (MAC_SUCCESS);
}

/*
 * Outgoing frame security material retrieval procedure as described in
 * 7.5.8.2.2
 */
static retval_t outgoing_key_retrieval (security_info_t *sec_info_p,
                                        wpan_addr_spec_t *dst_addr_spec_p,
                                        uint8_t **key)
{
    uint8_t key_lookup_size;
    uint8_t i;
    uint8_t lookup_data[9];

    switch (sec_info_p->KeyIdMode) {
    case 0x00:    // implicit key identification
        if (FCF_SHORT_ADDR == dst_addr_spec_p->AddrMode) {
            /* 
             * lookup_data:
             * 2-octet Dest PANId right-concatenated
             * 2-octet dest short addr right-concatenated
             * 1-octet of 0x00
             */
            lookup_data[0] = (uint8_t)(dst_addr_spec_p->PANId >> 8);
            lookup_data[1] = (uint8_t)dst_addr_spec_p->PANId;
            lookup_data[2] = (uint8_t)
                             (dst_addr_spec_p->Addr.short_address >> 8);
            lookup_data[3] = (uint8_t)dst_addr_spec_p->Addr.short_address;
            lookup_data[4] = 0x00;
            key_lookup_size = 0; /* 0 means 5 bytes (See IEEE spec) */
        } else if (FCF_LONG_ADDR == dst_addr_spec_p->AddrMode) {
            /*
             * lookup_data:
             * 8-octet dest addr right-concatenated
             * 1-octet of 0x00
             */
             memcpy(&lookup_data[0], &dst_addr_spec_p->Addr, 8);
             lookup_data[8] = 0x00;
             key_lookup_size = 1; /* 1 means 9 bytes (See IEEE spec) */
        } else if (FCF_NO_ADDR == dst_addr_spec_p->AddrMode) {
             if (mac_pib_macCoordShortAddress < MAC_NO_SHORT_ADDR_VALUE) {
                 /*
                  * Use Short Address:
                  * lookup_data:
                  * 2-octet Src PANId right-concatenated
                  * 2-octet PAN Coord Short Address right-concatenated
                  * 1-octet of 0x00
                  */
                 lookup_data[0] = (uint8_t)(hal_pib_PANId >> 8);
                 lookup_data[1] = (uint8_t) hal_pib_PANId;
                 lookup_data[2] = (uint8_t) (mac_pib_macCoordShortAddress >> 8);
                 lookup_data[3] = (uint8_t) (mac_pib_macCoordShortAddress);
                 lookup_data[4] = 0;
                 key_lookup_size = 0; /* 0 means 5 bytes (See IEEE spec) */
             } else {
                 /*
                  * Use Ext Address:
                  * lookup_data:
                  * 8-octet PAN Coord Ext Address
                  * 1-octet of 0x00
                  */
                 memcpy(&lookup_data[0], &mac_pib_macCoordExtendedAddress, 8);
                 lookup_data[8] = 0x00;
                 key_lookup_size = 1; /* 1 means 9 bytes (See IEEE spec) */
             }
        } else {
             return (MAC_UNSUPPORTED_SECURITY);
        }
        break;

    case 0x01:    // explicit key identification
        /*
         * lookup_data:
         * 8-octet of macDefaultKeySource right-concatenated with
         * 1-octet of Key Index
         */
        for (i = 0; i < 8; i++) {
            lookup_data[i] = mac_sec_pib.DefaultKeySource[i];
        }
        lookup_data[8] = sec_info_p->KeyIndex;
        key_lookup_size = 1; // 1 means 9 bytes (See IEEE spec)
        break;

    case 0x02:
        /*
         * lookup_data: 4-octet of KeySource right-concatenated with
         * 1-octet of Key Index
         */
        for (i = 0; i < 4; i++) {
            lookup_data[i] = sec_info_p->KeySource[i];
        }
        lookup_data[5] = sec_info_p->KeyIndex;
        key_lookup_size = 0; // 0 means 5 bytes (See IEEE spec)
        break;

    case 0x03:
        /*
         * lookup_data: 8-octet of KeySource right-concatenated with
         * 1-octet of Key Index
         */
        for (i = 0; i < 8; i++) {
            lookup_data[i] = sec_info_p->KeySource[i];
        }
        lookup_data[9] = sec_info_p->KeyIndex;
        key_lookup_size = 1; // 1 means 9 bytes (See IEEE spec)
        break;

    default:
        return (MAC_UNSUPPORTED_SECURITY);
        break;
    }

    return (mac_security_search_key(lookup_data, key_lookup_size, key, NULL));
}

/*
 * This function secures the given MAC frame.
 *
 * tx_frame Frame information structure of current frame
 * security_level Security Level of current frame
 * key_id_mode Key Identifier Mode
 *
 * return retval_t MAC_SUCCESS or MAC_UNSUPPORTED_SECURITY
 */
retval_t mac_secure (security_info_t *sec_info_p,
                     wpan_addr_spec_t *dst_addr_spec_p)
{
    uint8_t *key;
    retval_t status;

    status = outgoing_key_retrieval(sec_info_p, dst_addr_spec_p, &key);

    if (status != MAC_SUCCESS) {
        return (status);
    }

    /*
     * For encryption the extended address is the extended address of the 
     * source so just provide the dummy extended address
     */
    status = mac_hal_aes_key_config(HAL_AES_ENCRYPT, key, NULL); 

    return (status);
}


/* --- Incoming Security --------------------------------------------------- */

/*
 *
 * This function extracts the actual security parameters
 * from the Auxiliary Security Header of a received secured frame.
 *
 * Return the Status of generation of Auxiliary Security Header fields
 */
static retval_t parse_aux_sec_header (parse_t *mac_parse_data,
                                      uint8_t *mac_payload)
{
    memcpy(&mac_parse_data->sec_ctrl, &mac_payload[0], 1);
    memcpy(&mac_parse_data->frame_cnt, &mac_payload[1], 4);
    mac_parse_data->key_id_len =
                get_key_id_field_len(mac_parse_data->sec_ctrl.key_id_mode);

    if (mac_parse_data->sec_ctrl.key_id_mode != 0) {
        memcpy(mac_parse_data->key_id, &mac_payload[5], 
               mac_parse_data->key_id_len);
    }

    /* See 802.15.4-2006 section 7.5.8.2.3 b) */
    if ((mac_parse_data->fcf & FCF_SECURITY_ENABLED) &&
        !(mac_parse_data->fcf & FCF_FRAME_VERSION_2006)) {
        return (MAC_UNSUPPORTED_LEGACY);
    }

    return (MAC_SUCCESS);
}

/*
 * Incoming frame security material retrieval procedure as described in
 * 7.5.8.2.4
 */
static retval_t incoming_sec_material_retrieval (parse_t *mac_parse_data,
                                                 uint8_t **key, uint8_t **addr)
{
    uint8_t lookup_data_size;
    uint8_t lookup_data[9];
    uint8_t i;
    retval_t status;

    switch (mac_parse_data->sec_ctrl.key_id_mode) {
    case 0x00: // Implicit
        if (FCF_SHORT_ADDR == mac_parse_data->src_addr_mode) {
            /*
             * Key look up:
             * 2-octet of Src PAN Id or Dest PAN Id right-concatenated
             * 2-octet dest addr right-concatenated
             * 1-octet of 0x00
             */
            if (mac_parse_data->fcf & FCF_PAN_ID_COMPRESSION) {
                lookup_data[0] = (uint8_t)(mac_parse_data->dest_panid >> 8);
                lookup_data[1] = (uint8_t)mac_parse_data->dest_panid;
            } else {
                lookup_data[0] = (uint8_t)(mac_parse_data->src_panid >> 8);
                lookup_data[1] = (uint8_t)mac_parse_data->src_panid;
            }
            lookup_data[2] = (uint8_t)
                             (mac_parse_data->src_addr.short_address >> 8);
            lookup_data[3] = (uint8_t)mac_parse_data->src_addr.short_address;
            lookup_data[4] = 0x00;
            lookup_data_size = 0; /* 0 means 5 bytes (See IEEE spec) */
        } else if (FCF_LONG_ADDR == mac_parse_data->src_addr_mode) {
            /*
             * Key look up:
             * 8-octet of Src Extended Addr right-concatenated
             * 1-octet of 0x00
             */
             memcpy(&lookup_data[0], &mac_parse_data->src_addr.long_address, 8);
             lookup_data[8] = 0x00;
             lookup_data_size = 1; /* 1 means 9 bytes (See IEEE spec) */
        } else if (FCF_NO_ADDR == mac_parse_data->src_addr_mode) {
             if (mac_pib_macCoordShortAddress < MAC_NO_SHORT_ADDR_VALUE) {
                 /*
                  * Use Short Address:
                  * lookup_data:
                  * 2-octet Dest PANId right-concatenated
                  * 2-octet PAN Coord Short Address right-concatenated
                  * 1-octet of 0x00
                  */
                 lookup_data[0] = (uint8_t)(mac_parse_data->dest_panid >> 8);
                 lookup_data[1] = (uint8_t) mac_parse_data->dest_panid;
                 lookup_data[2] = (uint8_t)(mac_pib_macCoordShortAddress >> 8);
                 lookup_data[3] = (uint8_t)(mac_pib_macCoordShortAddress);
                 lookup_data[4] = 0;
                 lookup_data_size = 0; /* 0 means 5 bytes (See IEEE spec) */
             } else {
                 /*
                  * Use Ext Address:
                  * lookup_data:
                  * 8-octet PAN Coord Ext Address
                  * 1-octet of 0x00
                  */
                 memcpy(&lookup_data[0], &mac_pib_macCoordExtendedAddress, 8);
                 lookup_data[8] = 0x00;
                 lookup_data_size = 1; /* 1 means 9 bytes (See IEEE spec) */
             }
        } else {
            return (MAC_UNSUPPORTED_SECURITY);
        }
        break;

    case 0x01: // Explicit
        /*
         * Key look up:
         * 8-octet of Src Extended Addr right-concatenated
         * 1-octet of 0x00
         */
        for (i = 0; i < 8; i++) {
            lookup_data[i] = mac_sec_pib.DefaultKeySource[i];
        }
        lookup_data[8] = mac_parse_data->key_id[0];
        lookup_data_size = 1; /* 1 means 9 bytes (See IEEE spec) */
        break;
    case 0x02: // Explicit
        /*
         * Key look up:
         * 4-octet of Key Source right-concatenated
         * 1-octet of Key Index 
         */
        memcpy(lookup_data, mac_parse_data->key_id, mac_parse_data->key_id_len);
        lookup_data_size = 0; /* 0 means 5 bytes (See IEEE spec) */
        break;

    case 0x03: // Explicit
        /*
         * Key look up:
         * 8-octet of Key Source right-concatenated
         * 1-octet of Key Index 
         */
        memcpy(lookup_data, mac_parse_data->key_id, mac_parse_data->key_id_len);
        lookup_data_size = 1; /* 1 means 9 bytes (See IEEE spec) */
        break;
    }

    // FIXME - For Test Only - Set the aes_test_ext_addr
    if (FCF_SHORT_ADDR == mac_parse_data->src_addr_mode) {
        mac_parse_data->src_addr.long_address.lo_u32 = 
                            mac_parse_data->src_addr.short_address;
        mac_parse_data->src_addr.long_address.hi_u32 = 0xACDE4800;
    }
    if (FCF_NO_ADDR != mac_parse_data->src_addr_mode) {
        mac_utils_64_bit_to_byte_array(mac_parse_data->src_addr.long_address,
                                       aes_test_ext_addr);
    } else {
        extern uint64_t dst_ext_addr;
        mac_utils_64_bit_to_byte_array(dst_ext_addr, aes_test_ext_addr);
    }
    // FIXME - End of test code

    status = mac_security_search_key(lookup_data, lookup_data_size, key, addr);

    return (status);
}

/**
 * @brief Unsecures MAC frame
 *
 * This function unsecures the given MAC frame.
 *
 * @param mac_parse_data Frame information structure of current frame
 * @param security_level Security Level of current frame
 *
 * @return retval_t MAC_SUCCESS, MAC_UNSUPPORTED_SECURITY or MAC_SECURITY_ERROR
 */
static retval_t unsecure_frame (parse_t *mac_parse_data_p, uint8_t *mpdu_p,
                                uint8_t *mac_payload_p,
                                uint8_t *payload_index_p)
{
    uint8_t *src_ieee_addr;
    uint8_t *key;
    retval_t status;

    status = incoming_sec_material_retrieval(mac_parse_data_p, &key,
                                             &src_ieee_addr);

    if (MAC_SUCCESS == status) {
        status = mac_hal_aes_key_config(HAL_AES_DECRYPT, key, src_ieee_addr);
        if (MAC_SUCCESS == status) {
            /*
             * Write the encrypted frame to TX (AES) FIFO
             * Don't write the len in mpdu_p[0] and FCS bytes
             */										
            status = mac_hal_write_frame_to_fifo(&mpdu_p[1],
                                   mac_parse_data_p->mpdu_length - FCS_LEN,
                                   AES_DECRYPT, FALSE);
            if (MAC_SUCCESS == status) {
                /*
                 * Read the decrypted frame from RX (AES) FIFO
                 * mpdu_p[0] is the length of the decrypted packet
                 */
                status = mac_hal_rx_decrypted_data_to_buffer(mpdu_p);
                if (MAC_SUCCESS == status) {
                    uint8_t sec_hdr_len;
                    uint8_t mhr_len;
                    uint8_t mic_len;

                    /*
                     * sec ctrl (1-octet) + frame counter (4-octet) = 5
                     */
                    sec_hdr_len = 5 + mac_parse_data_p->key_id_len;
                    /*
                     * mpdu_p + 1 to skip the length byte
                     */ 
                    mhr_len = mac_payload_p - (mpdu_p + 1) + sec_hdr_len;
                    mic_len =
                        get_mic_length(mac_parse_data_p->sec_ctrl.sec_level);
                    *payload_index_p = sec_hdr_len;

                    /*
                     * Get the mac command type for MAC Command packet
                     * which is the 1st byte of the payload
                     */
                    if (FCF_FRAMETYPE_MAC_CMD == 
                        mac_parse_data_p->frame_type) {
                        mac_parse_data_p->mac_command = 
                                         mac_payload_p[*payload_index_p];
                    }
                    /*
                     * Compute the mac payload length of the decrypted packet.
                     * The mpdu_length is the length of the original
                     * encrypted packet which included FCS_LEN
                     */
                    mac_parse_data_p->mac_payload_length =
                        mac_parse_data_p->mpdu_length - FCS_LEN - mhr_len -
                        mic_len;
                    /*
                     * mpdu_p[0] contains the length of the new decrypted
                     * packet that does not including FCS_LEN.
                     * Note: The FCS bytes of the decrypted packet is not
                     * valid but we don't care
                     */
                    mac_parse_data_p->mpdu_length = mpdu_p[0] + FCS_LEN; 
                    mac_stats_g.decrypt_ok++;                 
                } else {
                    mac_stats_g.decrypt_error++;
                }
            }
        }
    }

    return (status);
}

/**
 *
 * This function handles the complete unsecuring of a MAC frame.
 * This includes the extraction of the Auxiliary Security Header and
 * the actual frame decryption.
 *
 */
retval_t mac_unsecure (parse_t *mac_parse_data_p, uint8_t *mpdu_p,
                       uint8_t *mac_payload_p, uint8_t *payload_index_p)
{
    retval_t status;

    status = parse_aux_sec_header(mac_parse_data_p, mac_payload_p);
    if (status != MAC_SUCCESS) {
        return (status);
    }

    // 7.5.8.2.3 d)
    if (mac_pib_macSecurityEnabled == FALSE) {
        if (0 == mac_parse_data_p->sec_ctrl.sec_level) {
            return (MAC_SUCCESS);
        } else {
            return (MAC_UNSUPPORTED_SECURITY);
        }
    }

    // 7.5.8.2.3 c)
    if (0 == mac_parse_data_p->sec_ctrl.sec_level) {
        return (MAC_UNSUPPORTED_SECURITY);
    }

    status = unsecure_frame(mac_parse_data_p, mpdu_p, mac_payload_p,
                            payload_index_p);

    return (status);
}



