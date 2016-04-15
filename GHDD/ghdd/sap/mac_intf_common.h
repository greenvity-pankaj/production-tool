
/*******************************************************************************
 *
 * File:       hybrii_mac_intf_common.h
 * Contact:    pankaj_razdan@greenvity.com
 *
 * Description: Host interface to Hybrii MAC
 *
 * Copyright (c) 2011 by Greenvity Communication.
 *
 ******************************************************************************/
#ifndef __HYBRII_MAC_INTF_COMMON_H__
#define __HYBRII_MAC_INTF_COMMON_H__

#define MAX_MAC_IF_FRAME_LENGTH		128

#define DIR_UPLINK					0x00
#define DIR_DOWNLINK				0x01


#define MAC_IF_CRC_LENGTH			2


/* Primitive Parameter ID Defines */
#define DEST_ADDR_MODE					0x00
#define SRC_ADDR_MODE					0x01
#define DEST_PAN_ID						0x02
#define SRC_PAN_ID						0x03
#define DEST_ADDR						0x04
#define SRC_ADDR						0x05
#define TX_OPTIONS						0x06
#define LOGICAL_CHANNEL					0x07
#define CHANNEL_PAGE					0x08
#define MSDU_HANDLE						0x09
#define CAPABILITY_INFO					0x0A
#define COORD_ADDR_MODE					0x0B
#define COOR_PAN_ID						0x0C
#define COOR_ADDR						0x0D
#define DEVICE_ADDR						0x0E
#define ASSOC_SHORT_ADDR				0x0F
#define PRIVM_STATUS					0x10
#define DISSOCIATE_REASON				0x11
#define TRANSMIT_INDIRECT				0x12
#define PIB_ATTRIBUTE					0x13
#define PIB_ATTRIBUTE_INDEX				0x14
#define PIB_ATTRIBUTE_LENGTH			0x15
#define PIB_ATTRIBUTE_VALUE				0x16
#define SCAN_TYPE						0x17
#define SCAN_CHANNELS					0x18
#define SCAN_DURATION					0x19
#define DEFER_PERMIT					0x1A
#define TX_ONTIME						0x1B
#define RX_ONDURATION					0x1C
#define START_TIME						0x1D
#define BEACON_ORDER					0x1E
#define SUPER_FRAME_ORDER				0x1F
#define PAN_COORDINATOR					0x20
#define BLE_ID							0x21
#define COORD_REALIGNMENT				0x22
#define COORD_REALIGNMENT_SECURITY		0x23
#define BEACON_SECURITY					0x24
#define MPDU_LINK_QUALITY				0x25
#define DSN_ID							0x26
#define TIME_STAMP						0x27
#define BSN_ID							0x28
#define PAN_DESCRIPTOR					0x29
#define PAN_ADDR_SPEC					0x2A
#define ADDR_LIST						0x2B
#define SDU_LENGTH						0x2C
#define SDU_ID							0x2D
#define RESULT_LIST_SIZE				0x2E
#define ORPHAN_ADDRESS					0x2F
#define LOSS_REASON						0x30
#define UNSCANNED_CHANNELS				0x31
//#define LOGICAL_CHANNEL					0x32
#define SET_DEFAULT_PIB					0x33
#define SECURITY_ID						0x34
#define KAL_MESSAGE						0x35
#define ASSOCIATED_MEMBER				0x36
#define PEND_ADDRESS					0x37

#define HPGP_PAYLOAD					0xFD	//Temp defined to be changed
#define MSDU_LENGTH						0xFE	//To be defined not present at this point
#define	MSDU_ID	   						0xFF	//To be defined not present at this point

#define MAC_ADDRESS_LENGTH		6


/* MAC-SAP Header */
/* Command header
  * Byte (0) - Frame Control
  *			Bit 0~1 - Protocol Type
  *			Bit 2~3 - Frame Type
  *			Bit 4~8 - Resvd
  * Byte (2, 3) - Length
  * Byte (4) - Reserved
  */
typedef struct gv_cmd {
	u8 proto		:2;
	u8 frm 			:2;
	u8 rsvd			:4;
} __attribute__ ((packed)) gv_frm_ctrl_t;  
typedef struct gv_cmd_hdr {
	gv_frm_ctrl_t fc;
	u8 reserved;
	u16 len;
}__attribute__ ((packed)) gv_cmd_hdr_t;

#define CMD_HDR_START					0x00
#define CMD_HDR_LENGTH					0x02
#define CMD_HDR_RSVD					0x01
#define CMD_HDR_LEN						sizeof (gv_cmd_hdr_t)
#define CMD_HDR_PROTOCOLTYPE_MASK		0x03
#define CMD_HDR_FRAMETYPE_MASK			0x0C

/* Protocol Type */
#define IEEE802_15_4_MAC_ID 		0x01
#define HPGP_MAC_ID		 			0x02
/* Get Protocol type from the command header */
#define GET_CMD_PROTOCOL(x)				((x) & CMD_HDR_PROTOCOLTYPE_MASK)

/* Frame Type */
#define CONTROL_FRM_ID					0x00
#define DATA_FRM_ID						0x01
#define MGMT_FRM_ID						0x02
#define EVENT_FRM_ID					0x03

/* Get Frame type from the command header */
#define GET_CMD_FRAMETYPE(x) 			\
	((u8)((ufrm[CMD_HDR_START] & CMD_HDR_FRAMETYPE_MASK) >> 2))


typedef union{
	u16 shr_addr;
	u64 ext_addr;
	u8 addr[MAC_ADDRESS_LENGTH];
}addr_t;

typedef struct {

	u8 sec_level;
	u8 key_id_mode;
	u8 key_src;
	u8 key_idx;
}sec_t;

typedef struct {
	u8 logical_channel : 5;
	u8 channel_page : 3;
}chan_t;

typedef struct {
	u8 coor_addr_mode;
	u16 coor_pan_id;
	addr_t coor_addr;
	chan_t chan;
	u16 super_frm_spec;
	u8 gts_permit;
	u8 link_quality;
	u32 timestamp;
	u8 sec_failure;
	sec_t sec;
}pand_t;

typedef struct {

	u8 a1;
	u16 a2;
	u32 a3;

}kal_message_details_t;

typedef struct {
	u8 byte_data[7];
	u16 short_data[2];
	u32 word_data[2];
	addr_t address_data[2];
	pand_t pan_data[1];
	sec_t security_data[2];
	kal_message_details_t kal_data[1];
	u8 *ptr1;
	u8 *ptr2;
	u8 idx_b;
	u8 idx_w;
	u8 idx_s;
	u8 idx_a;
	u8 idx_sec;
	u8 idx_p;
	u8 idx_k;
	u8 len1;
	u8 len2;
	u8 pand; //Temp variable to be corrected
}params_t;



#define HYBRII_FRM_HEADER_LENGTH	sizeof (gv_cmd_hdr_t)


extern u8 ufrm[MAX_MAC_IF_FRAME_LENGTH];

void zmac_hostIf_form_hdr(u8 frm_type, u8 protocol);
void zmac_hostIf_set_primitive_id_uplink(u8 *p_idx, u8 prim_id);
void zmac_hostIf_set_primitive_id_downlink( u8 *p_idx, u8 prim_id);
void zmac_hostIf_set_tlv (u8 *p_idx, u8 type, u8 length, void *p_value);
bool zmac_hostIf_get_tlvs (params_t *p, u8 *p_frm, u8 frm_len, bool le);
bool zmac_hostIf_verify_crc (u8 *p_frm, u16 frm_len);
void zmac_hostIf_set_crc (u8 *p_frm, u16 *p_len);
void *rev_memcpy( u8 *p_dst, const u8 *p_src, unsigned int count );
void zmac_hostIf_copy_pan_descriptor(pand_t *p_dst, pand_t *p_src);
void zmac_hostIf_convert_pand_endianess (pand_t *p_dst, pand_t *p_src);
void zmac_hostIf_send_to_mac (u16 frm_len, u8 intf);
void zmac_hostIf_reset_frm (void);
void zmac_hostIf_control_frm_handler(u8 *p_frm, const u16 frm_len);
bool is_little_endian(void);
#endif /* __HYBRII_MAC_INTF_COMMON_H__ */


