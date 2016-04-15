/*******************************************************************************
 *
 * File:       hybrii_mac_intf.c
 * Contact:    pankaj_razdan@greenvity.com
 *
 * Description: Host Interface to Zigbee MAC(802.15.4)
 *
 * Copyright(c) 2011 by Greenvity Communication.
 *
 ******************************************************************************/
#include <linux/module.h>
#include "papdef.h"
#include "mac_intf_common.h"
#include "mac_intf.h"
#include "mac_msgs.h"

/* Prototype */

/* Utility API's */
#ifdef HYBRII_MAC_INTF_TEST
extern void zmac_hostIf_frm_parser(u8 *p_frm, const u8 frm_len);
#endif
void zmac_send_to_host(u16 frm_len);

extern void hmac_intf_downlink_primitives_handler(params_t *p, u8 *p_frm, u8 frm_len);

/* 802.15.4 Downlink  API's */
/* API parameters are as per IEEE802.15.4 spec */

void zmac_mcps_data_req(
		u8 dest_addr_mode, u16 dest_pan_id, addr_t *p_dest_addr, 
		u8 src_addr_mode, u16 src_pan_id, addr_t *p_src_addr,
		u8 msdu_len, u8 *p_msdu, u8 msdu_handle, 
		u8 tx_options, sec_t *p_sec)
{

#ifdef HYBRII_MAC_INTF_TEST
	p_zmac_mcps_data_req(
		dest_addr_mode, dest_pan_id, p_dest_addr, 
		src_addr_mode, src_pan_id, p_src_addr,
		msdu_len, p_msdu, msdu_handle,
		tx_options, p_sec);
#endif

}

void zmac_purge_request( u8 msdu_handle)
{
#ifdef HYBRII_MAC_INTF_TEST
	p_zmac_purge_request(msdu_handle);
#endif

}

void zmac_assoc_req(
		u8 logical_channel,u8 channel_page,u8 coord_addr_mode,
		u16 coord_pan_id,addr_t *pcoord_addr,u8 capability_info,
		sec_t *p_sec)
{
#ifdef HYBRII_MAC_INTF_TEST
	p_zmac_assoc_req(logical_channel, channel_page, coord_addr_mode,
				coord_pan_id, pcoord_addr, capability_info, p_sec);
#endif

}

void zmac_mlme_assoc_resp( 
		u8 *p_dev_addr, u16 assoc_short_addr, u8 status, sec_t *p_sec)
{
#ifdef HYBRII_MAC_INTF_TEST
	p_zmac_mlme_assoc_resp(p_dev_addr, assoc_short_addr, status, p_sec);
#endif	
}

void zmac_mlme_dissoc_req(
		u8 dev_addr_mode, u16 dev_pan_id, addr_t *p_dev_addr,
		u8 dissoc_reason, u8 tx_indirect, sec_t *p_sec)
{
#ifdef HYBRII_MAC_INTF_TEST
	p_zmac_mlme_dissoc_req(dev_addr_mode, dev_pan_id, p_dev_addr, 
		dissoc_reason, tx_indirect, p_sec);
#endif	

}

void zmac_mlme_get_req( u8 pib_attribute, u8 pib_attributeIndex)			
{
#ifdef HYBRII_MAC_INTF_TEST
		p_zmac_mlme_get_req(pib_attribute, pib_attributeIndex);
#endif	

}

void zmac_mlme_orphan_rsp( 
	u8 orphan_address[8], u16 short_addr, u8 assoc_member, sec_t *p_sec)
{
#ifdef HYBRII_MAC_INTF_TEST
			p_zmac_mlme_orphan_rsp(orphan_address, short_addr, assoc_member, p_sec);
#endif	

}

void zmac_mlme_reset_req( u8 set_default_pib)
{
#ifdef HYBRII_MAC_INTF_TEST
	p_zmac_mlme_reset_req(set_default_pib);
#endif	

}

void zmac_mlme_rx_enable_req( 
	u8 defer_permit, u32 tx_on_time, u32 rx_on_duration)
{
#ifdef HYBRII_MAC_INTF_TEST
	p_zmac_mlme_rx_enable_req(defer_permit, tx_on_time, rx_on_duration);
#endif	
}

void zmac_mlme_scan_req( 
	u8 scan_type, u32 scan_channels, u8 scan_duration,
	u8 channel_page, sec_t *p_sec)
{
#ifdef HYBRII_MAC_INTF_TEST
		p_zmac_mlme_scan_req(scan_type, scan_channels, scan_duration, channel_page, p_sec);
#endif	

}

void zmac_mlme_set_req(
	u8 pib_attribute, u8 pib_attributeIndex, u16 pib_attribute_length, 
	void *pib_attributeValue)
{
#ifdef HYBRII_MAC_INTF_TEST
			p_zmac_mlme_set_req(pib_attribute, pib_attributeIndex, 
				pib_attribute_length, pib_attributeValue);
#endif	

}

void zmac_mlme_start_req(
	u16 pan_id, u8 logical_channel, u8 channel_page, u32 start_time, 
	u8 beacon_order, u8 super_frame_order, u8 pan_coordinator, u8 ble, 
	u8 coord_realignment, sec_t *pcoord_realignmentSecurity, sec_t *p_beacon_sec)
{
#ifdef HYBRII_MAC_INTF_TEST
	p_zmac_mlme_start_req(pan_id, logical_channel, channel_page,
		start_time, beacon_order,super_frame_order,pan_coordinator,
		ble, coord_realignment, pcoord_realignmentSecurity, p_beacon_sec);
#endif	

}

void zmac_mlme_poll_req( 
	u8 coord_addr_mode, u16 coord_pan_id, 
	addr_t *p_coord_addr, sec_t *p_sec)
{
#ifdef HYBRII_MAC_INTF_TEST
	p_zmac_mlme_poll_req(coord_addr_mode, coord_pan_id, 
		p_coord_addr, p_sec);
#endif	

}


/* Uplink */
void zmac_mcps_data_ind(
	u8 src_addr_mode, u16 src_pan_id, addr_t *p_src_addr,
	u8 dest_addr_mode, u16 dest_pan_id, addr_t *p_dest_addr, 
	u8 msdu_len, u8 *p_msdu, u8 mpdu_link_quality, u8 dsn,
	u32 time_stamp, sec_t *p_sec)
{
	u8 i;

	/* Frame Header */
	zmac_hostIf_reset_frm();
	zmac_hostIf_form_hdr(CONTROL_FRM_ID, IEEE802_15_4_MAC_ID);

	/* Frame Payload */
	zmac_hostIf_set_primitive_id_uplink(&i, MCPS_DATA_REQUEST);
	zmac_hostIf_set_tlv(&i, SRC_ADDR_MODE, sizeof(u8), &src_addr_mode);
	zmac_hostIf_set_tlv(&i, SRC_PAN_ID, sizeof(u16), &src_pan_id);	
	zmac_hostIf_set_tlv(&i, SRC_ADDR, sizeof(addr_t), p_src_addr);
	zmac_hostIf_set_tlv(&i, DEST_ADDR_MODE, sizeof(u8), &dest_addr_mode);
	zmac_hostIf_set_tlv(&i, DEST_PAN_ID, sizeof(u16), &dest_pan_id);
	zmac_hostIf_set_tlv(&i, DEST_ADDR, sizeof(addr_t), p_dest_addr);
	zmac_hostIf_set_tlv(&i, MSDU_LENGTH, sizeof(u8), &msdu_len);
	zmac_hostIf_set_tlv(&i, MSDU_ID, msdu_len, p_msdu);
	zmac_hostIf_set_tlv(&i, MPDU_LINK_QUALITY, sizeof(u8), &mpdu_link_quality);
	zmac_hostIf_set_tlv(&i, DSN_ID, sizeof(u8), &dsn);	
	zmac_hostIf_set_tlv(&i, TIME_STAMP, sizeof(u32), &time_stamp);
	zmac_hostIf_set_tlv(&i, SECURITY_ID, sizeof(sec_t), p_sec);

	/* Frame Transmission */
	zmac_send_to_host(i);


}

void zmac_mcps_data_cnf( u8 msdu_handle, u8 status, u32 time_stamp)
{
	u8 i;
	
	/* Frame Header */
	zmac_hostIf_reset_frm();
	zmac_hostIf_form_hdr(CONTROL_FRM_ID, IEEE802_15_4_MAC_ID);

	/* Frame Payload */
	zmac_hostIf_set_primitive_id_uplink(&i, MCPS_DATA_CONFIRM);
	zmac_hostIf_set_tlv(&i, MSDU_HANDLE, sizeof(u8), &msdu_handle);
	zmac_hostIf_set_tlv(&i, PRIVM_STATUS, sizeof(u8), &status);
	zmac_hostIf_set_tlv(&i, TIME_STAMP, sizeof(u32), &time_stamp);

	/* Frame Transmission */
	zmac_send_to_host(i);
}

void zmac_mcps_purge_cnf( u8 msdu_handle, u8 status)
{
	u8 i;
	
	/* Frame Header */
	zmac_hostIf_reset_frm();
	zmac_hostIf_form_hdr(CONTROL_FRM_ID, IEEE802_15_4_MAC_ID);

	/* Frame Payload */
	zmac_hostIf_set_primitive_id_uplink(&i, MCPS_PURGE_CONFIRM);
	zmac_hostIf_set_tlv(&i, MSDU_HANDLE, sizeof(u8), &msdu_handle);
	zmac_hostIf_set_tlv(&i, PRIVM_STATUS, sizeof(u8), &status);

	/* Frame Transmission */
	zmac_send_to_host(i);

}

void zmac_mlme_assoc_ind( u8 *p_dev_addr, u8 capability_info, sec_t *p_sec)
{
	u8 i;
	
	/* Frame Header */
	zmac_hostIf_reset_frm();
	zmac_hostIf_form_hdr(CONTROL_FRM_ID, IEEE802_15_4_MAC_ID);

	/* Frame Payload */
	zmac_hostIf_set_primitive_id_uplink(&i, MLME_ASSOCIATE_INDICATION);
	zmac_hostIf_set_tlv(&i, DEVICE_ADDR, sizeof(addr_t), p_dev_addr);
	zmac_hostIf_set_tlv(&i, CAPABILITY_INFO, sizeof(u8), &capability_info);
	zmac_hostIf_set_tlv(&i, SECURITY_ID, sizeof(sec_t), p_sec);

	/* Frame Transmission */
	zmac_send_to_host(i);
}

void zmac_mlme_assoc_cnf( u16 assoc_short_addr, u8 status, sec_t *p_sec)
{
	u8 i;

	/* Frame Header */
	zmac_hostIf_reset_frm();
	zmac_hostIf_form_hdr(CONTROL_FRM_ID, IEEE802_15_4_MAC_ID);

	/* Frame Payload */
	zmac_hostIf_set_primitive_id_uplink(&i, MLME_ASSOCIATE_CONFIRM);
	zmac_hostIf_set_tlv(&i, ASSOC_SHORT_ADDR, sizeof(u16), &assoc_short_addr);
	zmac_hostIf_set_tlv(&i, PRIVM_STATUS, sizeof(u8), &status);
	zmac_hostIf_set_tlv(&i, SECURITY_ID, sizeof(sec_t), p_sec);

	/* Frame Transmission */
	zmac_send_to_host(i);

}

void zmac_mlme_disassoc_ind( addr_t *p_dev_addr, u8 dissoc_reason, sec_t *p_sec)
{
	u8 i;

	/* Frame Header */
	zmac_hostIf_reset_frm();
	zmac_hostIf_form_hdr(CONTROL_FRM_ID, IEEE802_15_4_MAC_ID);

	/* Frame Payload */
	zmac_hostIf_set_primitive_id_uplink(&i, MLME_DISASSOCIATE_INDICATION);
	zmac_hostIf_set_tlv(&i, SRC_ADDR, sizeof(addr_t), p_dev_addr);
	zmac_hostIf_set_tlv(&i, DISSOCIATE_REASON, sizeof(u8), &dissoc_reason);
	zmac_hostIf_set_tlv(&i, SECURITY_ID, sizeof(sec_t), p_sec);

	/* Frame Transmission */
	zmac_send_to_host(i);
}

void zmac_mlme_disassoc_cnf( 
	u8 status, u8 dev_addr_mode, 
	u16 dev_pan_id, addr_t *p_dev_addr)
{
	u8 i;

	/* Frame Header */
	zmac_hostIf_reset_frm();
	zmac_hostIf_form_hdr(CONTROL_FRM_ID, IEEE802_15_4_MAC_ID);

	/* Frame Payload */
	zmac_hostIf_set_primitive_id_uplink(&i, MLME_DISASSOCIATE_CONFIRM);
	zmac_hostIf_set_tlv(&i, PRIVM_STATUS, sizeof(u8), &status);
	zmac_hostIf_set_tlv(&i, SRC_ADDR_MODE, sizeof(u8), &dev_addr_mode);
	zmac_hostIf_set_tlv(&i, SRC_PAN_ID, sizeof(u16), &dev_pan_id);
	zmac_hostIf_set_tlv(&i, SRC_ADDR, sizeof(addr_t), p_dev_addr);

	/* Frame Transmission */
	zmac_send_to_host(i);

}

void zmac_mlme_beacon_notify_ind( 
	u8 bsn, pand_t *p_pan_descr, u8 pend_addr_spec, u8 *p_addr_list, 
	u8 sdu_length, u8 *p_Sdu)
{
	u8 i;

	/* Frame Header */
	zmac_hostIf_reset_frm();
	zmac_hostIf_form_hdr(CONTROL_FRM_ID, IEEE802_15_4_MAC_ID);

	/* Frame Payload */
	zmac_hostIf_set_primitive_id_uplink(&i, MLME_BEACON_NOTIFY_INDICATION);
	zmac_hostIf_set_tlv(&i, BSN_ID, sizeof(u8), &bsn);
	zmac_hostIf_set_tlv(&i, PAN_DESCRIPTOR, sizeof(pand_t), p_pan_descr);	
	zmac_hostIf_set_tlv(&i, PEND_ADDRESS, sizeof(u8), &pend_addr_spec);
	zmac_hostIf_set_tlv(&i, ADDR_LIST, sizeof(addr_t), p_addr_list);
	zmac_hostIf_set_tlv(&i, MSDU_LENGTH, sizeof(u8), &sdu_length);
	zmac_hostIf_set_tlv(&i, MSDU_ID, sdu_length, p_Sdu);

	/* Frame Transmission */
	zmac_send_to_host(i);

}

void zmac_mlme_get_cnf( 
	u8 status, u8 pib_attribute, u8 pib_attributeIndex,
	u16 pib_attribute_length, void *p_pib_attributeValue)
{
	u8 i;

	/* Frame Header */
	zmac_hostIf_reset_frm();
	zmac_hostIf_form_hdr(CONTROL_FRM_ID, IEEE802_15_4_MAC_ID);

	/* Frame Payload */
	zmac_hostIf_set_primitive_id_uplink(&i, MLME_GET_CONFIRM);
	zmac_hostIf_set_tlv(&i, PRIVM_STATUS, sizeof(u8), &status);
	zmac_hostIf_set_tlv(&i, PIB_ATTRIBUTE, sizeof(u8), &pib_attribute);
	zmac_hostIf_set_tlv(&i, PIB_ATTRIBUTE_INDEX, sizeof(u8), &pib_attributeIndex);
	zmac_hostIf_set_tlv(&i, PIB_ATTRIBUTE_LENGTH, sizeof(u16), &pib_attribute_length);
	zmac_hostIf_set_tlv(&i, PIB_ATTRIBUTE_VALUE, pib_attribute_length, p_pib_attributeValue);	

	/* Frame Transmission */
	zmac_send_to_host(i);

}

void zmac_mlme_orphan_ind( u8 *p_orphan_address, sec_t *p_sec)
{
	u8 i;

	/* Frame Header */
	zmac_hostIf_reset_frm();
	zmac_hostIf_form_hdr(CONTROL_FRM_ID, IEEE802_15_4_MAC_ID);

	/* Frame Payload */
	zmac_hostIf_set_primitive_id_uplink(&i, MLME_ORPHAN_INDICATION);
	zmac_hostIf_set_tlv(&i, ORPHAN_ADDRESS, sizeof(addr_t), p_orphan_address);
	zmac_hostIf_set_tlv(&i, SECURITY_ID, sizeof(sec_t), p_sec);

	/* Frame Transmission */
	zmac_send_to_host(i);
}

void zmac_mlme_reset_cnf( u8 status)
{
	u8 i;

	/* Frame Header */
	zmac_hostIf_reset_frm();
	zmac_hostIf_form_hdr(CONTROL_FRM_ID, IEEE802_15_4_MAC_ID);

	/* Frame Payload */
	zmac_hostIf_set_primitive_id_uplink(&i, MLME_RESET_CONFIRM);
	zmac_hostIf_set_tlv(&i, PRIVM_STATUS, sizeof(u8), &status);

	/* Frame Transmission */
	zmac_send_to_host(i);

}

void zmac_mlme_rx_enable_cnf( u8 status)
{
	u8 i;

	/* Frame Header */
	zmac_hostIf_reset_frm();
	zmac_hostIf_form_hdr(CONTROL_FRM_ID, IEEE802_15_4_MAC_ID);

	/* Frame Payload */
	zmac_hostIf_set_primitive_id_uplink(&i, MLME_RX_ENABLE_CONFIRM);
	zmac_hostIf_set_tlv(&i, PRIVM_STATUS, sizeof(u8), &status);

	/* Frame Transmission */
	zmac_send_to_host(i);
}

void zmac_mlme_comm_stats_ind( 
	u16 pan_id, u8 src_addr_mode, addr_t *p_src_addr, u8 dest_addr_mode, 
	addr_t *p_dest_addr, u8 status, sec_t *p_sec)
{
	u8 i;

	/* Frame Header */
	zmac_hostIf_reset_frm();
	zmac_hostIf_form_hdr(CONTROL_FRM_ID, IEEE802_15_4_MAC_ID);

	/* Frame Payload */
	zmac_hostIf_set_primitive_id_uplink(&i, MLME_SCAN_CONFIRM);
	zmac_hostIf_set_tlv(&i, SRC_PAN_ID, sizeof(u8), &pan_id);
	zmac_hostIf_set_tlv(&i, SRC_ADDR_MODE, sizeof(u8), &src_addr_mode);
	zmac_hostIf_set_tlv(&i, SRC_ADDR, sizeof(addr_t), p_src_addr);
	zmac_hostIf_set_tlv(&i, DEST_ADDR_MODE, sizeof(u8), &dest_addr_mode);
	zmac_hostIf_set_tlv(&i, DEST_ADDR, sizeof(addr_t), p_dest_addr);
	zmac_hostIf_set_tlv(&i, PRIVM_STATUS, sizeof(u8), &status);
	zmac_hostIf_set_tlv(&i, SECURITY_ID, sizeof(sec_t), p_sec);

	/* Frame Transmission */
	zmac_send_to_host(i);

}

void zmac_mlme_set_cnf( u8 status, u8 pib_attribute, u8 pib_attributeIndex)
{
	u8 i;

	/* Frame Header */
	zmac_hostIf_reset_frm();
	zmac_hostIf_form_hdr(CONTROL_FRM_ID, IEEE802_15_4_MAC_ID);

	/* Frame Payload */
	zmac_hostIf_set_primitive_id_uplink(&i, MLME_SET_CONFIRM);
	zmac_hostIf_set_tlv(&i, PRIVM_STATUS, sizeof(u8), &status);
	zmac_hostIf_set_tlv(&i, PIB_ATTRIBUTE, sizeof(u8), &pib_attribute);
	zmac_hostIf_set_tlv(&i, PIB_ATTRIBUTE_INDEX, sizeof(u8), &pib_attributeIndex);

	/* Frame Transmission */
	zmac_send_to_host(i);
}

void zmac_mlme_start_cnf( u8 status)
{
	u8 i;

	/* Frame Header */
	zmac_hostIf_reset_frm();
	zmac_hostIf_form_hdr(CONTROL_FRM_ID, IEEE802_15_4_MAC_ID);

	/* Frame Payload */
	zmac_hostIf_set_primitive_id_uplink(&i, MLME_START_CONFIRM);
	zmac_hostIf_set_tlv(&i, PRIVM_STATUS, sizeof(u8), &status);

	/* Frame Transmission */
	zmac_send_to_host(i);
}

void zmac_mlme_sync_loss_ind( 
	u8 loss_reason, u16 pan_id, u8 logical_channel,
	u8 channel_page, sec_t *p_sec)
{
	u8 i;

	/* Frame Header */
	zmac_hostIf_reset_frm();
	zmac_hostIf_form_hdr(CONTROL_FRM_ID, IEEE802_15_4_MAC_ID);

	/* Frame Payload */
	zmac_hostIf_set_primitive_id_uplink(&i, MLME_SYNC_LOSS_INDICATION);
	zmac_hostIf_set_tlv(&i, LOSS_REASON, sizeof(u8), &loss_reason);
	zmac_hostIf_set_tlv(&i, SRC_PAN_ID, sizeof(u16), &pan_id);
	zmac_hostIf_set_tlv(&i, LOGICAL_CHANNEL, sizeof(u8), &logical_channel);
	zmac_hostIf_set_tlv(&i, CHANNEL_PAGE, sizeof(u8), &channel_page);
	zmac_hostIf_set_tlv(&i, SECURITY_ID, sizeof(sec_t), p_sec);

	/* Frame Transmission */
	zmac_send_to_host(i);


}

void zmac_mlme_poll_cnf( u8 status)
{
	u8 i;

	/* Frame Header */
	zmac_hostIf_reset_frm();
	zmac_hostIf_form_hdr(CONTROL_FRM_ID, IEEE802_15_4_MAC_ID);

	/* Frame Payload */
	zmac_hostIf_set_primitive_id_uplink(&i, MLME_POLL_CONFIRM);
	zmac_hostIf_set_tlv(&i, PRIVM_STATUS, sizeof(u8), &status);

	/* Frame Transmission */
	zmac_send_to_host(i);
}

/* Utilities */

/* This API is called to extract TLV fields and call primitive handlers
 * @Parameters 
 * p - parameter data structure to store fields
 * p_frm - pointer to the frame received from the Host interface
 * frm_len - length of the Hybrii frame
 */
void zmac_intf_downlink_primitives_handler(
	params_t *p, u8 *p_frm, u8 frm_len)
{
	u8 idx_b, idx_w, idx_s, idx_sec, idx_a;
	memset((u8 *)p, 0x00, sizeof(params_t));

	/* Extract all TLV fields */	
	if(!zmac_hostIf_get_tlvs(
			p, &p_frm[HYBRII_FRM_HEADER_LENGTH + 2], 
			frm_len - HYBRII_FRM_HEADER_LENGTH - 2,
			is_little_endian()))
	{
		/* Invalid field in the frame - we cannot process it further */
		return;
	}

	idx_b = idx_w = idx_s = idx_sec = idx_a = 0;
	
	/* Parse prinitive IDs and call respective handlers */	
	switch(p_frm[5])
	{			 
/*
	case	MCPS_DATA_REQUEST:
		{
		zmac_mcps_data_req( 
			p->byte_data[idx_b], 
			p->short_data[idx_s], 
			&p->address_data[idx_a],
			p->byte_data[++idx_b], 
			p->short_data[++idx_s], 
			&p->address_data[++idx_a],
			p->byte_data[++idx_b], 
			p->ptr2, 
			p->byte_data[++idx_b],
			p->byte_data[++idx_b], 
			&p->security_data[idx_sec]);
		break;
		}
		
	case	MCPS_PURGE_REQUEST:		
		{
		zmac_purge_request(
			p->byte_data[idx_b]);
		break;
		}
		
	case	MLME_ASSOCIATE_REQUEST:
		{
		zmac_assoc_req(
			p->byte_data[idx_b], 
			p->byte_data[++idx_b],
			p->byte_data[++idx_b],
			p->short_data[idx_s],
			&p->address_data[idx_a],
			p->byte_data[++idx_b],
			&p->security_data[idx_sec]);
		break;
		}
		
	case	MLME_ASSOCIATE_RESPONSE:
		{
		zmac_mlme_assoc_resp( 
			p->address_data[idx_a].addr, 
			p->short_data[idx_s], 
			p->byte_data[++idx_b], 
			&p->security_data[idx_sec]);
		break;
		}
		
	case	MLME_DISASSOCIATE_RESPONSE:
		{
		zmac_mlme_dissoc_req(
			p->byte_data[idx_b], 
			p->word_data[idx_w], 
			&p->address_data[idx_a], 
			p->byte_data[++idx_b],
			p->byte_data[++idx_b], 
			&p->security_data[idx_sec]);
		break;
		}
		
	case	MLME_GET_REQUEST:
		{
		zmac_mlme_get_req( 
			p->byte_data[idx_b], 
			p->byte_data[++idx_b]);
		break;
		}
		
	case	MLME_ORPHAN_RESPONSE:
		{
		zmac_mlme_orphan_rsp(
			p->address_data[idx_a].addr, 
			p->short_data[idx_s],
			p->byte_data[idx_b], 
			&p->security_data[idx_sec]);
		break;
		}
		
	case	MLME_RESET_REQUEST:
		{
		zmac_mlme_reset_req( 
			p->byte_data[idx_b]);
		break;
		}

	case	MLME_RX_ENABLE_REQUEST:
		{
		zmac_mlme_rx_enable_req(
			p->byte_data[idx_b], 
			p->word_data[idx_w],
			p->word_data[++idx_w]);
		break;
		}
		
	case	MLME_SCAN_REQUEST:
		{
		zmac_mlme_scan_req(
			p->byte_data[idx_b], 
			p->word_data[idx_w],
			p->byte_data[++idx_b], 
			p->byte_data[++idx_b], 
			&p->security_data[idx_sec]);
		break;
		}
		
	case	MLME_SET_REQUEST:
		{
		zmac_mlme_set_req(
			p->byte_data[idx_b], 
			p->byte_data[++idx_b],
			p->short_data[idx_s], 
			p->ptr1);
		break;
		}
		
	case	MLME_START_REQUEST:
		{
		zmac_mlme_start_req(
			p->short_data[idx_s], 
			p->byte_data[idx_b],
			p->byte_data[++idx_b],
			p->word_data[idx_w], 
			p->byte_data[++idx_b],
			p->byte_data[++idx_b],
			p->byte_data[++idx_b],
			p->byte_data[++idx_b], 
			p->byte_data[++idx_b],
			&p->security_data[idx_sec],
			&p->security_data[++idx_sec]);
		break;
		}
		
	case	MLME_POLL_REQUEST:
		{
		zmac_mlme_poll_req(
			p->byte_data[idx_b], 
			p->short_data[idx_s], 
			&p->address_data[idx_a],
			&p->security_data[idx_sec]);
		break;
		}
		
	case	MLME_DISASSOCIATE_REQUEST:
		{
		zmac_mlme_dissoc_req(
			p->byte_data[idx_b], 
			p->short_data[idx_s],
			&p->address_data[idx_a],	
			p->byte_data[++idx_b],
			p->byte_data[++idx_b], 
			&p->security_data[idx_sec]);

		break;
		}
*/
	default:
		//while(0); // error // TBD
		return;
	}
}


/* This API is called to handle Control Frames
 * @Parameters 
 * p_frm - pointer to the frame received from the Host interface
 * frm_len - length of the Hybrii frame
 */	
void mac_intf_control_frm_handler(u8 *p_frm, u8 frm_len)
{
	params_t p;
	
	if(p_frm[3] == IEEE802_15_4_MAC_ID)
	{
		zmac_intf_downlink_primitives_handler(&p, p_frm, frm_len);
	}
	else if(p_frm[3] == HPGP_MAC_ID)
	{
		hmac_intf_downlink_primitives_handler(&p, p_frm, frm_len);
	}
	else
	{
		return; //error
	}

}

/* This API parses frame received from the Host interface and calls corresponding primitive handlers 
 * @Parameters
 * p_frm - pointer to the frame received from the Host interface
 * frm_len - length of the Hybrii frame
 */
void zmac_intf_frm_parser(u8 *p_frm, u16 frm_len) //TBD - make it static
{
	/* Verify CRC */
	if(!zmac_hostIf_verify_crc(p_frm, frm_len))
	{
		/* Do not process corrupt packet */
		return;
	}
	
	if(p_frm[0] == CONTROL_FRM_ID)
	{
		mac_intf_control_frm_handler(p_frm, frm_len);
	}
	else if(p_frm[0] == DATA_FRM_ID)
	{
		//TBD
	}
	else if(p_frm[0] == MGMT_FRM_ID)
	{
		//TBD
	}
	else
	{
		return; // error
	}
}


/* This API is called when data is to be sent to Host 
 * @Parameters
 * frm_len - length of the Hybrii frame
 */
void zmac_send_to_host(u16 frm_len)
{
//	*((u16 *)&p_frm[1]) = frm_len - HYBRII_FRM_HEADER_LENGTH;

	/* Set Frame length in frame header */
	frm_len -= HYBRII_FRM_HEADER_LENGTH;
	memcpy(&ufrm[1],(u8 *)&frm_len, sizeof(u16));
	frm_len += HYBRII_FRM_HEADER_LENGTH;

	/* Calculate and set CRC for the frame */
	zmac_hostIf_set_crc(ufrm, &frm_len);	

	/* Send frame to Interface bus */
#ifdef HYBRII_MAC_INTF_TEST
	zmac_hostIf_frm_parser(ufrm, frm_len);
#endif
}


