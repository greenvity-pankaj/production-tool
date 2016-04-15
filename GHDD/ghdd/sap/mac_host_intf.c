/*******************************************************************************
 *
 * File:       hybrii_mac_host_intf.c
 * Contact:    pankaj_razdan@greenvity.com
 *
 * Description: Host interface to Hybrii MAC
 *
 * Copyright (c) 2011 by Greenvity Communication.
 *
 ******************************************************************************/
#include <linux/module.h>
#include <net/sock.h>
#include "papdef.h"
#include "hpgpdef.h"
#include "debug_print.h"
#include "mac_intf_common.h"
#include "mac_intf.h"
#include "ghdd_driver.h"
#include "ghdd_defines.h"
#ifdef HYBRII_MAC_INTF_TEST
#include "mac_host_intf.h"
#endif

#ifdef _GHDD_MAC_SAP_
#include "nmm.h"
#include "h1msgs.h"
#include "hpgpdef.h"
#include "hpgpapi.h"
#include "hpgpevt.h" 
#include "ghdd_driver_defines.h"
#ifdef LRWPAN_DRIVER
#include "lrwpan.h" 
#endif /*LRWPAN_DRIVER */
#endif /* _GHDD_MAC_SAP_ */
#include "hpgp_msgs.h"
#include "mac_msgs.h"

#define PRINTK_PREFIX "~g1"
#define GV_ETHER					0x00
#define GV_SPI						0x01


extern unsigned int interface_type;
extern unsigned int gbl_headroom;

extern u8 rf_capability; // used to pass on GV701x capability (i.e to identify 7011 and 7013) 
extern u8 ghdd_init_done;// provides whether ghdd has completed plc init and lrwpan from app can config mlme

extern void increment_counters(u8 cmd_id, u8 display) ;

/* Utility Prototype */
void zmac_hostIf_frm_parser(u8 *p_frm, const u8 frm_len);
void zmac_hostIf_802_15_4_uplink_primitive_handler(params_t *p, u8 *p_frm, const u8 frm_len);
void zmac_hostIf_control_frm_handler(u8 *p_frm, const u16 frm_len);
void zmac_hostIf_send_to_mac (u16 frm_len, u8 intf);

/* 802.15.4 UpLink APIs */
/* API parameters are as per IEEE802.15.4 spec */
extern struct ghdd_opt hpgp_opt;
extern  struct net_device *hpgp_dev;

#ifdef _GHDD_MAC_SAP_
extern void hpgp_driver_schedule( unsigned long nmm );
extern sNmm *Host_GetNmm(void);
void hmac_intf_downlink_primitives_handler(params_t *p, 
									u8 *p_frm, u8 frm_len);
void hmac_create_sap_frame(sEvent *event);
void hmac_hostIf_hpgp_uplink_primitive_handler(	
									params_t *p, u8 *p_frm, const u16 frm_len);
extern int ghdd_net_driver_tx(struct sk_buff *skb, 
							u8 hybrii_type, u8 intf);
extern u8  *ghdd_get_mac_addr(void);
#endif /*_GHDD_MAC_SAP_ */



/* MCPS Data Indication */
void zmac_hostIf_mcps_data_ind(	
	u8 src_addr_mode, u16 src_pan_id, addr_t *p_src_addr,
	u8 dest_addr_mode, u16 dest_pan_id, addr_t *p_dest_addr,
	u8 msdu_len, u8 *p_msdu, u8 mpdu_link_quality,
	u8 dsn, u32 time_stamp, sec_t *p_sec )
{

/* User shall primitve handler */

#ifdef HYBRII_MAC_INTF_TEST
	p_zmac_hostIf_mcps_data_ind(
		 src_addr_mode,  src_pan_id,  p_src_addr,
		 dest_addr_mode,  dest_pan_id,  p_dest_addr, 
		 msdu_len, p_msdu,  mpdu_link_quality,  dsn,
		 time_stamp, p_sec );
#endif

#ifdef LRWPAN_DRIVER
	lrwpan_mcps_data_ind(
		 src_addr_mode,  src_pan_id,  p_src_addr,
		 dest_addr_mode,  dest_pan_id,	p_dest_addr, 
		 msdu_len, p_msdu,	mpdu_link_quality,	dsn,
		 time_stamp, p_sec );
#endif

}

/* MCPS Data Confirm  */
void zmac_hostIf_mcps_data_cnf(
	u8 msdu_handle, u8 status, u32 time_stamp )
{
	/* User shall primitve handler */

#ifdef HYBRII_MAC_INTF_TEST
	p_zmac_hostIf_mcps_data_cnf( msdu_handle, status, time_stamp );
#endif

#ifdef LRWPAN_DRIVER
	lrwpan_mcps_data_cnf( msdu_handle, status, time_stamp );
#endif

}

/* MCPS Purge Confirm */
void zmac_hostIf_mcps_purge_cnf( 
	u8 msdu_handle, u8 status )
{
	/* User shall primitve handler */

#ifdef HYBRII_MAC_INTF_TEST	
	p_zmac_hostIf_mcps_purge_cnf(  msdu_handle,  status );
#endif

}

/* MLME Association Indication */
void zmac_hostIf_mlme_assoc_ind( 
	u8 dev_addr[8], u8 capability_info, sec_t *p_sec )
{
	/* User shall primitve handler */

#ifdef HYBRII_MAC_INTF_TEST
	p_zmac_hostIf_mlme_assoc_ind (&dev_addr[0], capability_info, p_sec);	
#endif

#ifdef LRWPAN_DRIVER
	lrwpan_mlme_assoc_ind(dev_addr, capability_info, p_sec);
#endif
	
}

/* MLME Association Confirm */
void zmac_hostIf_mlme_assoc_cnf( 
	u16 assoc_short_addr, u8 status, sec_t *p_sec )
{
	/* User shall primitve handler */

#ifdef HYBRII_MAC_INTF_TEST
	p_zmac_hostIf_mlme_assoc_cnf (assoc_short_addr, status, p_sec);	
#endif
#ifdef LRWPAN_DRIVER
	lrwpan_mlme_assoc_cnf (assoc_short_addr, status, p_sec);
#endif
}

/* MLME Disassociation Indication */
void zmac_hostIf_mlme_disassoc_ind(
	u8 dev_addr[8], u8 dissoc_reason, sec_t *p_sec )
{
	/* User shall primitve handler */

#ifdef HYBRII_MAC_INTF_TEST
	p_zmac_hostIf_mlme_disassoc_ind( &dev_addr[0],  dissoc_reason, p_sec );	
#endif
	
}

/* MLME Disassociation Confirm  */
void zmac_hostIf_mlme_disassoc_cnf( 
	u8 status, u8 dev_addr_mode, 
	u16 dev_pan_id, addr_t *p_dev_addr )	
{
	
	/* User shall primitve handler */
	
#ifdef HYBRII_MAC_INTF_TEST
	p_zmac_hostIf_mlme_disassoc_cnf (status, dev_addr_mode, dev_pan_id, p_dev_addr);	
#endif


}

/* MLME Beacon Notify Indication */
void zmac_hostIf_mlme_beacon_notify_ind( 
	u8 bsn, pand_t *p_pan_descr, u8 pend_addr_spec,
	u8 *p_addr_list, u8 sdu_length, u8 *p_sdu )
{
	/* User shall primitve handler */

#ifdef HYBRII_MAC_INTF_TEST	
	p_zmac_hostIf_mlme_beacon_notify_ind( 
	 bsn, p_pan_descr,  pend_addr_spec, p_addr_list, 
	sdu_length, p_sdu );
#endif

#ifdef LRWPAN_DRIVER
	lrwpan_mlme_beacon_notify_ind (bsn, p_pan_descr, 
							pend_addr_spec, p_addr_list, 
							sdu_length, p_sdu);
#endif
} 

/* MLME Get Confirm  */
void zmac_hostIf_mlme_get_cnf( 
	u8 status, u8 pib_attribute, u8 pib_attributeIndex,
	u8 pib_attribute_length, void *p_pib_attributeValue )
{
	/* User shall primitve handler */

#ifdef HYBRII_MAC_INTF_TEST	
	p_zmac_hostIf_mlme_get_cnf( 
		status, pib_attribute, pib_attributeIndex,
		pib_attribute_length, p_pib_attributeValue );
#endif
}

/* MLME Orphan Indication */
void zmac_hostIf_mlme_orphan_ind(
	u8 *p_orphan_address, sec_t *p_sec )
{
	/* User shall primitve handler */

	
#ifdef HYBRII_MAC_INTF_TEST
	p_zmac_hostIf_mlme_orphan_ind(p_orphan_address, p_sec );	
#endif
}

/* MLME Reset Confirm  */
void zmac_hostIf_mlme_reset_cnf(
	u8 status )
{
	/* User shall primitve handler */

#ifdef HYBRII_MAC_INTF_TEST
	p_zmac_hostIf_mlme_reset_cnf( status );	
#endif
}

/* MLME Rx Enable Confirm  */
void zmac_hostIf_mlme_rx_enable_cnf( 
	u8 status )
{
	/* User shall primitve handler */

#ifdef HYBRII_MAC_INTF_TEST
	p_zmac_hostIf_mlme_rx_enable_cnf( status );	
#endif
}

/* MLME Energy Scan Confirm  */
void zmac_hostIf_mlme_energy_scan_cnf( 
	u8 status, u8 channel_page, 
	u8 result_list_size, pand_t *p_pan_descs )
{
	
	/* User shall primitve handler */
#ifdef HYBRII_MAC_INTF_TEST
	p_zmac_hostIf_mlme_energy_scan_cnf( 
	status, channel_page, result_list_size, p_pan_descs );	
#endif
}

/* MLME AP Scan Confirm  */
void zmac_hostIf_mlme_ap_scan_cnf(
	u8 status, u8 channel_page, u32 unscanned_channels, 
	u8 result_list_size, pand_t *p_pan_descs )
{
	/* User shall primitve handler */

#ifdef HYBRII_MAC_INTF_TEST
	p_zmac_hostIf_mlme_ap_scan_cnf( 
	 status, channel_page, unscanned_channels, result_list_size, 
	p_pan_descs );
	
#endif
}

/* MLME Orphan Scan Confirm  */
void zmac_hostIf_mlme_orphan_scan_cnf( 
	u8 status, u8 channel_page, u32 unscanned_channels )
{
	/* User shall primitve handler */

#ifdef HYBRII_MAC_INTF_TEST
	p_zmac_hostIf_mlme_orphan_scan_cnf( 
	 status,  channel_page,  unscanned_channels );	
#endif
}

/* MLME Comm Status Indication */
void zmac_hostIf_mlme_comm_stats_ind( 
	u16 pan_id, u8 src_addr_mode, 
	addr_t *p_src_addr, u8 dest_addr_mode,
	addr_t *p_dest_addr, u8 status, sec_t *p_sec )
{
	/* User shall primitve handler */

#ifdef HYBRII_MAC_INTF_TEST

p_zmac_hostIf_mlme_comm_stats_ind( 
	 pan_id, src_addr_mode, p_src_addr, 
	 dest_addr_mode, 
	 p_dest_addr, status, p_sec );

#endif
}

/* MLME Set Confirm  */
void zmac_hostIf_mlme_set_cnf( 
	u8 status, u8 pib_attribute, u8 pib_attributeIndex )
{	
	/* User shall primitve handler */

#ifdef HYBRII_MAC_INTF_TEST
	
	p_zmac_hostIf_mlme_set_cnf( status, pib_attribute, pib_attributeIndex );
#endif
}

/* MLME Start Confirm  */
void zmac_hostIf_mlme_start_cnf( u8 status )
{
	/* User shall primitve handler */

#ifdef HYBRII_MAC_INTF_TEST	
	p_zmac_hostIf_mlme_start_cnf( status );
#endif

#ifdef LRWPAN_DRIVER
	lrwpan_mlme_start_cnf (status);
#endif
}

/* MLME Sync Loss Indication */
void zmac_hostIf_mlme_sync_loss_ind( 
	u8 loss_reason, u16 pan_id, 
	u8 logical_channel, u8 channel_page, sec_t *p_sec )
{
	/* User shall primitve handler */

#ifdef HYBRII_MAC_INTF_TEST
p_zmac_hostIf_mlme_sync_loss_ind( 
	loss_reason, pan_id, logical_channel, channel_page, p_sec );
#endif
}

/* MLME Poll Confirm  */
void zmac_hostIf_mlme_poll_cnf( u8 status )
{
	/* User shall primitve handler */

#ifdef HYBRII_MAC_INTF_TEST
	p_zmac_hostIf_mlme_poll_cnf( status );
#endif

}



/* 802.15.4 Downlink API's */
/* @API parmaters - as per IEEE802.15.4 spec */

void zmac_hostIf_mcps_data_req(
	u8 dest_addr_mode, u16 dest_pan_id, 
	addr_t *p_dest_addr, u8 src_addr_mode,
	u16 src_pan_id, addr_t *p_src_addr,
	u8 msdu_len, u8 *p_msdu, u8 msdu_handle, 
	u8 tx_options, sec_t *p_sec )
{
	u8 i;
	
	/* Frame Header */
	zmac_hostIf_reset_frm ();
	zmac_hostIf_form_hdr (CONTROL_FRM_ID, IEEE802_15_4_MAC_ID);
	
	/* Frame Payload */
	zmac_hostIf_set_primitive_id_downlink (&i, MCPS_DATA_REQUEST);
	zmac_hostIf_set_tlv (&i, DEST_ADDR_MODE, sizeof(u8), &dest_addr_mode);
	zmac_hostIf_set_tlv (&i, DEST_PAN_ID, sizeof(u16), &dest_pan_id);	
	zmac_hostIf_set_tlv (&i, DEST_ADDR, sizeof(addr_t), p_dest_addr);	
	zmac_hostIf_set_tlv (&i, SRC_ADDR_MODE, sizeof(u8), &src_addr_mode);
	zmac_hostIf_set_tlv (&i, SRC_PAN_ID, sizeof(u16), &src_pan_id);	
	zmac_hostIf_set_tlv (&i, SRC_ADDR, sizeof(addr_t), p_src_addr);
	zmac_hostIf_set_tlv (&i, MSDU_LENGTH, sizeof(u8), &msdu_len);
	zmac_hostIf_set_tlv (&i, MSDU_ID, msdu_len, p_msdu);
	zmac_hostIf_set_tlv (&i, MSDU_HANDLE, sizeof(u8), &msdu_handle);
	zmac_hostIf_set_tlv (&i, TX_OPTIONS, sizeof(u8), &tx_options);
	zmac_hostIf_set_tlv (&i, SECURITY_ID, sizeof(sec_t), p_sec);

	/* Send Frame to MAC Interface */
	zmac_hostIf_send_to_mac (i, LRWPAN_INTERFACE);

	
}

void zmac_hostIf_purge_request( u8 msdu_handle )
{
	u8 i;
	
	/* Frame Header */
	zmac_hostIf_reset_frm ();
	zmac_hostIf_form_hdr (CONTROL_FRM_ID, IEEE802_15_4_MAC_ID);
	
	/* Frame Payload */
	zmac_hostIf_set_primitive_id_downlink (&i, MCPS_PURGE_REQUEST);
	zmac_hostIf_set_tlv (&i, MSDU_HANDLE, sizeof(u8), &msdu_handle);

	/* Send Frame to MAC Interface */
	zmac_hostIf_send_to_mac (i, LRWPAN_INTERFACE);
}

void zmac_hostIf_assoc_req( 
	u8 logical_channel, u8 channel_page, 
	u8 coord_addr_mode, u16 coord_pan_id,
	addr_t *p_coord_addr, u8 capability_info,
	sec_t *p_sec )
{

	u8 i;
	
	/* Frame Header */
	zmac_hostIf_reset_frm ();
	zmac_hostIf_form_hdr (CONTROL_FRM_ID, IEEE802_15_4_MAC_ID);
	
	/* Frame Payload */
	zmac_hostIf_set_primitive_id_downlink (&i, MLME_ASSOCIATE_REQUEST);
	zmac_hostIf_set_tlv (&i, LOGICAL_CHANNEL, sizeof(u8), &logical_channel);
	zmac_hostIf_set_tlv (&i, CHANNEL_PAGE, sizeof(u8), &channel_page);	
	zmac_hostIf_set_tlv (&i, COORD_ADDR_MODE, sizeof(u8), &coord_addr_mode);
	zmac_hostIf_set_tlv (&i, COOR_PAN_ID, sizeof(u16), &coord_pan_id);
	zmac_hostIf_set_tlv (&i, COOR_ADDR, sizeof(addr_t), p_coord_addr);
	zmac_hostIf_set_tlv (&i, CAPABILITY_INFO, sizeof(u8), &capability_info);
	zmac_hostIf_set_tlv (&i, SECURITY_ID, sizeof(sec_t), p_sec);

	/* Send Frame to MAC Interface */
	zmac_hostIf_send_to_mac (i, LRWPAN_INTERFACE);


}

void zmac_hostIf_mlme_assoc_resp( 
	u8 *p_dev_addr, u16 assoc_short_addr, 
	u8 status, sec_t *p_sec )
{
	u8 i;
	
	/* Frame Header */
	zmac_hostIf_reset_frm ();
	zmac_hostIf_form_hdr (CONTROL_FRM_ID, IEEE802_15_4_MAC_ID);
	
	/* Frame Payload */
	zmac_hostIf_set_primitive_id_downlink (&i, MLME_ASSOCIATE_RESPONSE);
	zmac_hostIf_set_tlv (&i, DEVICE_ADDR, sizeof(addr_t), p_dev_addr);
	zmac_hostIf_set_tlv (&i, ASSOC_SHORT_ADDR, sizeof(u16), &assoc_short_addr);
	zmac_hostIf_set_tlv (&i, PRIVM_STATUS, sizeof(u8), &status);
	zmac_hostIf_set_tlv (&i, SECURITY_ID, sizeof(sec_t), p_sec);

	/* Send Frame to MAC Interface */
	zmac_hostIf_send_to_mac (i, LRWPAN_INTERFACE);

}

void zmac_hostIf_mlme_dissoc_req( 
	u8 dev_addr_mode, u16 dev_pan_id, addr_t *p_dev_addr,
	u8 dissoc_reason, u8 tx_indirect, sec_t *p_sec )
{
	u8 i;
	
	/* Frame Header */
	zmac_hostIf_reset_frm ();
	zmac_hostIf_form_hdr (CONTROL_FRM_ID, IEEE802_15_4_MAC_ID);
	
	/* Frame Payload */
	zmac_hostIf_set_primitive_id_downlink (&i, MLME_DISASSOCIATE_REQUEST);	
	zmac_hostIf_set_tlv (&i, DEST_ADDR_MODE, sizeof(u8), &dev_addr_mode);
	zmac_hostIf_set_tlv (&i, SRC_PAN_ID, sizeof(u16), &dev_pan_id); //tbd
	zmac_hostIf_set_tlv (&i, DEVICE_ADDR, sizeof(addr_t), p_dev_addr);	
	zmac_hostIf_set_tlv (&i, DISSOCIATE_REASON, sizeof(u8), &dissoc_reason);
	zmac_hostIf_set_tlv (&i, TRANSMIT_INDIRECT, sizeof(u8), &tx_indirect);
	zmac_hostIf_set_tlv (&i, SECURITY_ID, sizeof(sec_t), p_sec);

	/* Send Frame to MAC Interface */
	zmac_hostIf_send_to_mac (i, LRWPAN_INTERFACE);

}

void zmac_hostIf_mlme_get_req( u8 pib_attribute, u8 pib_attributeIndex )
{

	u8 i;
	
	/* Frame Header */
	zmac_hostIf_reset_frm ();
	zmac_hostIf_form_hdr (CONTROL_FRM_ID, IEEE802_15_4_MAC_ID);
	
	/* Frame Payload */
	zmac_hostIf_set_primitive_id_downlink (&i, MLME_GET_REQUEST);
	zmac_hostIf_set_tlv (&i, PIB_ATTRIBUTE, sizeof(u8), &pib_attribute);
	zmac_hostIf_set_tlv (&i, PIB_ATTRIBUTE_INDEX, sizeof(u8), &pib_attributeIndex);

	/* Send Frame to MAC Interface */
	zmac_hostIf_send_to_mac (i, LRWPAN_INTERFACE);
}

void zmac_hostIf_mlme_orphan_rsp( 
	u8 *p_orphan_address, 
	u16 short_addr, u8 assoc_member, sec_t *p_sec )
{
	u8 i;
	
	/* Frame Header */
	zmac_hostIf_reset_frm ();
	zmac_hostIf_form_hdr (CONTROL_FRM_ID, IEEE802_15_4_MAC_ID);
	
	/* Frame Payload */
	zmac_hostIf_set_primitive_id_downlink (&i, MLME_ORPHAN_RESPONSE);
	zmac_hostIf_set_tlv (&i, ORPHAN_ADDRESS, sizeof(addr_t), p_orphan_address);
	zmac_hostIf_set_tlv (&i, ASSOC_SHORT_ADDR, sizeof(u16), &short_addr); //tbd
	zmac_hostIf_set_tlv (&i, ASSOCIATED_MEMBER, sizeof(u8), &assoc_member);
	zmac_hostIf_set_tlv (&i, SECURITY_ID, sizeof(sec_t), p_sec);

	/* Send Frame to MAC Interface */
	zmac_hostIf_send_to_mac (i, LRWPAN_INTERFACE);

}

void zmac_hostIf_mlme_reset_req( u8 set_default_pib )
{

	u8 i;
	
	/* Frame Header */
	zmac_hostIf_reset_frm ();
	zmac_hostIf_form_hdr (CONTROL_FRM_ID, IEEE802_15_4_MAC_ID);
	
	/* Frame Payload */
	zmac_hostIf_set_primitive_id_downlink (&i, MLME_RESET_REQUEST);
	zmac_hostIf_set_tlv (&i, SET_DEFAULT_PIB, sizeof(u8), &set_default_pib);

	/* Send Frame to MAC Interface */
	zmac_hostIf_send_to_mac (i, LRWPAN_INTERFACE);
}

void zmac_hostIf_mlme_rx_enable_req( 
	u8 defer_permit, u32 tx_on_time,  u32 rx_on_duration )
{
	u8 i;
	
	/* Frame Header */
	zmac_hostIf_reset_frm ();
	zmac_hostIf_form_hdr (CONTROL_FRM_ID, IEEE802_15_4_MAC_ID);
	
	/* Frame Payload */
	zmac_hostIf_set_primitive_id_downlink (&i, MLME_RX_ENABLE_REQUEST);
	zmac_hostIf_set_tlv (&i, DEFER_PERMIT, sizeof(u8), &defer_permit);
	zmac_hostIf_set_tlv (&i, TX_ONTIME, sizeof(u32), &tx_on_time);
	zmac_hostIf_set_tlv (&i, RX_ONDURATION, sizeof(u32), &rx_on_duration);

	/* Send Frame to MAC Interface */
	zmac_hostIf_send_to_mac (i, LRWPAN_INTERFACE);

}

void zmac_hostIf_mlme_scan_req( 
	u8 scan_type, u32 scan_channels, u8 scan_duration, 
	u8 channel_page, sec_t *p_sec )
{
	u8 i;
	
	/* Frame Header */
	zmac_hostIf_reset_frm ();
	zmac_hostIf_form_hdr (CONTROL_FRM_ID, IEEE802_15_4_MAC_ID);
	
	/* Frame Payload */
	zmac_hostIf_set_primitive_id_downlink (&i, MLME_SCAN_REQUEST);
	zmac_hostIf_set_tlv (&i, SCAN_TYPE, sizeof(u8), &scan_type);
	zmac_hostIf_set_tlv (&i, SCAN_CHANNELS, sizeof(u32), &scan_channels);
	zmac_hostIf_set_tlv (&i, SCAN_DURATION, sizeof(u8), &scan_duration);
	zmac_hostIf_set_tlv (&i, CHANNEL_PAGE, sizeof(u8), &channel_page);
	zmac_hostIf_set_tlv (&i, SECURITY_ID, sizeof(sec_t), p_sec);

	/* Send Frame to MAC Interface */
	zmac_hostIf_send_to_mac (i, LRWPAN_INTERFACE);

}

void zmac_hostIf_mlme_set_req( 
	u8 pib_attribute, u8 pib_attributeIndex, 
	u16 pib_attribute_length, 
	void *pib_attributeValue )
{
	u8 i;
	
	/* Frame Header */
	zmac_hostIf_reset_frm ();
	zmac_hostIf_form_hdr (CONTROL_FRM_ID, IEEE802_15_4_MAC_ID);
	
	/* Frame Payload */
	zmac_hostIf_set_primitive_id_downlink (&i, MLME_SET_REQUEST);
	zmac_hostIf_set_tlv (&i, PIB_ATTRIBUTE, sizeof(u8), &pib_attribute);
	zmac_hostIf_set_tlv (&i, PIB_ATTRIBUTE_INDEX, sizeof(u8), &pib_attributeIndex);
	zmac_hostIf_set_tlv (&i, PIB_ATTRIBUTE_LENGTH, sizeof(u16), &pib_attribute_length);
	zmac_hostIf_set_tlv (&i, PIB_ATTRIBUTE_VALUE, pib_attribute_length, pib_attributeValue);

	/* Send Frame to MAC Interface */
	zmac_hostIf_send_to_mac (i, LRWPAN_INTERFACE);

}



void zmac_hostIf_mlme_start_req( 
	u16 pan_id, u8 logical_channel, 
	u8 channel_page, u32 start_time,
	u8 beacon_order, u8 super_frame_order,
	u8 pan_coordinator, u8 ble, 
	u8 coord_realignment, 
	sec_t *pcoord_realignmentSecurity,
	sec_t *p_beacon_sec )
{
	u8 i;
	
	/* Frame Header */
	zmac_hostIf_reset_frm ();
	zmac_hostIf_form_hdr (CONTROL_FRM_ID, IEEE802_15_4_MAC_ID);
	
	/* Frame Payload */
	zmac_hostIf_set_primitive_id_downlink (&i, MLME_START_REQUEST);
	zmac_hostIf_set_tlv (&i, SRC_PAN_ID, sizeof(u16), &pan_id); //tbd
	zmac_hostIf_set_tlv (&i, LOGICAL_CHANNEL, sizeof(u8), &logical_channel);
	zmac_hostIf_set_tlv (&i, CHANNEL_PAGE, sizeof(u8), &channel_page);
	zmac_hostIf_set_tlv (&i, START_TIME, sizeof(u32), &start_time);
	zmac_hostIf_set_tlv (&i, BEACON_ORDER, sizeof(u8), &beacon_order);
	zmac_hostIf_set_tlv (&i, SUPER_FRAME_ORDER, sizeof(u8), &super_frame_order);
	zmac_hostIf_set_tlv (&i, PAN_COORDINATOR, sizeof(u8), &pan_coordinator);
	zmac_hostIf_set_tlv (&i, BLE_ID, sizeof(u8), &ble);	
	zmac_hostIf_set_tlv (&i, COORD_REALIGNMENT, sizeof(u8), &coord_realignment);
	zmac_hostIf_set_tlv (&i, COORD_REALIGNMENT_SECURITY, sizeof(sec_t), pcoord_realignmentSecurity);	
	zmac_hostIf_set_tlv (&i, SECURITY_ID, sizeof(sec_t), p_beacon_sec);

	/* Send Frame to MAC Interface */
	zmac_hostIf_send_to_mac (i, LRWPAN_INTERFACE);

}

void zmac_hostIf_mlme_poll_req( 
	u8 coord_addr_mode, u16 coord_pan_id,
	addr_t *p_coord_addr, sec_t *p_sec )
{
	u8 i;
	
	/* Frame Header */
	zmac_hostIf_reset_frm ();
	zmac_hostIf_form_hdr (CONTROL_FRM_ID, IEEE802_15_4_MAC_ID);
	
	/* Frame Payload */
	zmac_hostIf_set_primitive_id_downlink (&i, MLME_POLL_REQUEST);
	zmac_hostIf_set_tlv (&i, SRC_ADDR_MODE, sizeof(u8), &coord_addr_mode); 
	zmac_hostIf_set_tlv (&i, SRC_PAN_ID, sizeof(u16), &coord_pan_id);
	zmac_hostIf_set_tlv (&i, SRC_ADDR, sizeof(addr_t), p_coord_addr);	
	zmac_hostIf_set_tlv (&i, SECURITY_ID, sizeof(sec_t), p_sec);

	/* Send Frame to MAC Interface */
	zmac_hostIf_send_to_mac (i, LRWPAN_INTERFACE);

}




/* Utilities */
#ifdef _GHDD_MAC_SAP_
void hmac_hostIf_hpgp_uplink_primitive_handler(	params_t *p, 
												u8 *p_frm, const u16 frm_len)
{
	sEvent *event = NULL;
	sNmm *nmm;

	nmm = Host_GetNmm();

	/* Create Event */
	event = EVENT_Alloc(frm_len - 2, H1MSG_HEADER_SIZE);
	if (event == NULL) {
		return;
	}
	event->eventHdr.eventClass = EVENT_CLASS_CTRL;
	event->eventHdr.type =  p_frm[CMD_HDR_LEN];
	event->eventHdr.status = 0;
	event->eventHdr.trans = 0;

	if (is_little_endian ()) {
		memcpy (event->buffDesc.dataptr, p_frm, frm_len);
		
	} else {
		rev_memcpy (event->buffDesc.dataptr, p_frm,  frm_len);
	}
	event->buffDesc.datalen = frm_len;
	
	//Send event
	NMM_PostEvent(nmm, event);
	hpgp_driver_schedule(0);
}


void hmac_create_sap_frame(sEvent *event)
{
	u8 i;

	zmac_hostIf_reset_frm ();

	/* Header */
	switch(event->eventHdr.eventClass)
	{
		case(EVENT_CLASS_MGMT):	zmac_hostIf_form_hdr(MGMT_FRM_ID, HPGP_MAC_ID);	break;
		case(EVENT_CLASS_CTRL):	zmac_hostIf_form_hdr(CTRL_FRM_ID, HPGP_MAC_ID);	break;
		case(EVENT_CLASS_DATA):	zmac_hostIf_form_hdr(DATA_FRM_ID, HPGP_MAC_ID);	break;
		default:																break;
	}

	/* Primitive ID & Direction */
	switch(*(event->buffDesc.dataptr))
	{
		case (APCM_AUTHORIZE_REQ): 
		case (APCM_SET_SECURITY_MODE_REQ):
		case (APCM_GET_SECURITY_MODE_REQ):
		case (APCM_SET_KEY_REQ):
		case (APCM_GET_KEY_REQ):
		case (APCM_SET_PPKEYS_REQ):
		case (APCM_SET_NETWORKS_REQ):
		case (APCM_STA_RESTART_REQ):
		case (APCM_NET_EXIT_REQ):
		case (APCM_CCO_APPOINT_REQ):
		case (SET_DATAPATH):
		case (SET_SNIFFER):
		case (SET_BRIDGE):
		case (GET_DEVICE_MODE):
		case (GET_HARDWARE_SPEC):
		case (GET_DEVICE_STATS):
		case (GET_PEER_INFO	):
		case (SET_HARDWARE_SPEC):
			zmac_hostIf_set_primitive_id_downlink(&i, *(event->buffDesc.dataptr));
		break;
		
		case (APCM_AUTHORIZE_CNF):		  
		case (APCM_AUTHORIZE_IND):		  
		case (APCM_SET_SECURITY_MODE_CNF):		  
		case (APCM_GET_SECURITY_MODE_CNF):		 
		case (APCM_SET_KEY_CNF):		
		case (APCM_GET_KEY_CNF):		
		case (APCM_SET_PPKEYS_CNF): 	  
		case (APCM_SET_NETWORKS_CNF):		 
		case (APCM_STA_RESTART_CNF):		
		case (APCM_NET_EXIT_CNF):
		case (SET_DATAPATH_CNF):
		case (SET_SNIFFER_CNF):
		case (SET_BRIDGE_CNF):
		case (GET_DEVICE_MODE_CNF):
		case (GET_HARDWARE_SPEC_CNF):
		case (GET_DEVICE_STATS_CNF):
		case (GET_PEER_INFO_CNF):
		case (SET_HARDWARE_SPEC_CNF):
			zmac_hostIf_set_primitive_id_uplink(&i, *(event->buffDesc.dataptr));
		break;
	}
	
	/* Frame Payload */
	zmac_hostIf_set_tlv(&i, HPGP_PAYLOAD, event->buffDesc.datalen, event->buffDesc.dataptr);
	
	/* Send Frame to MAC Interface */
	zmac_hostIf_send_to_mac (i, LRWPAN_INTERFACE);

	return;
}

void hmac_intf_downlink_primitives_handler(params_t *p, u8 *p_frm, u8 frm_len)
{
//	u16 pos = 0;
	sEvent *event;
	
	event = EVENT_Alloc(frm_len + CRC_SIZE, H1MSG_HEADER_SIZE);
	if (event == NULL)
    {
        return;
    }

	switch(*p_frm)
	{
		case(CTRL_FRM_ID):			
		{
			event->eventHdr.eventClass = EVENT_CLASS_CTRL;
			break;
		}

		case(DATA_FRM_ID):
		{
			event->eventHdr.eventClass = EVENT_CLASS_DATA;
			break;
		}

		case(MGMT_FRM_ID):			
		{
			event->eventHdr.eventClass = EVENT_CLASS_MGMT;
			break;
		}

		default:
			break;
	}

	//Event datalen
	event->buffDesc.datalen = *((u16 *)p_frm) - (HYBRII_HEADER_LENGTH+1);

	//event dataptr
	memcpy(event->buffDesc.dataptr, p_frm + 5, event->buffDesc.datalen);

	//event type
	event->eventHdr.type = *(event->buffDesc.dataptr);
	
	//DIRECTION
	switch(event->eventHdr.type)
	{
		case (APCM_AUTHORIZE_REQ): 
		case (APCM_SET_SECURITY_MODE_REQ):
		case (APCM_GET_SECURITY_MODE_REQ):
		case (APCM_SET_KEY_REQ):
		case (APCM_GET_KEY_REQ):
		case (APCM_SET_PPKEYS_REQ):
		case (APCM_SET_NETWORKS_REQ):
		case (APCM_STA_RESTART_REQ):
		case (APCM_NET_EXIT_REQ):
		case (APCM_CCO_APPOINT_REQ):
			//Post Event for NMA	
		break;

		
		case (APCM_AUTHORIZE_CNF):		  
		case (APCM_AUTHORIZE_IND):		  
		case (APCM_SET_SECURITY_MODE_CNF):		  
		case (APCM_GET_SECURITY_MODE_CNF):		 
		case (APCM_SET_KEY_CNF):		
		case (APCM_GET_KEY_CNF):		
		case (APCM_SET_PPKEYS_CNF): 	  
		case (APCM_SET_NETWORKS_CNF):		 
		case (APCM_STA_RESTART_CNF):		
		case (APCM_NET_EXIT_CNF):
			//Post Event for NMM	
		break;

	}

	return;
}
extern int genl_ghdd_cli_send(u8 *mgmt_msg, int len);
u8 hmac_format_and_send_frame(u8 *ptr_packet, u32 pktlen) {
	u8 i = 0;
	netid_t 	*ptrnetid;
	network_t	*ptrnetwork;
	authsta_t	*authsta;
	sNmm *nmm;
	sNetInfo *netInfo;	
	gv_cmd_hdr_t *cmd = (gv_cmd_hdr_t *)ufrm;
	gv_cmd_hdr_t *req = (gv_cmd_hdr_t *)ptr_packet;
	u8 intf;
	u8* cmdid = (u8*)(req + 1);
		
	i += CMD_HDR_LEN;
	//DEBUG_PRINT(GHDD,DBG_FATAL,"hmac_format_and_send_frame Rx");
	if(req->fc.proto == HPGP_MAC_ID)
	{
		switch(*cmdid) 	
		{	
			case(APCM_SET_KEY_REQ): 			
				ptrnetid = (netid_t *)(cmdid);
				NMM_GenerateNmk(&(ptrnetid->passwd[0]), ptrnetid->pwdlen, &(ptrnetid->nmk[0]));
				NMM_GenerateNid(&(ptrnetid->nmk[0]), &(ptrnetid->nid[0]));
				memcpy(&ufrm[i], cmdid, sizeof(netid_t));		
				i += sizeof(netid_t);	
				//DEBUG_PRINT(GHDD,DBG_TRACE," APCM_SET_KEY_REQ Request Received");
			break;
				
			case(APCM_SET_NETWORKS_REQ):
				ptrnetwork = (network_t *)(cmdid);
				nmm =(sNmm *)Host_GetNmm();	
				netInfo = &nmm->netInfo;				
				memcpy(ptrnetwork->nid, netInfo->nid, NID_LEN);
				memcpy(&ufrm[i], cmdid, sizeof(network_t));	
				i += sizeof(network_t);	
				//DEBUG_PRINT(GHDD,DBG_TRACE," APCM_SET_NETWORKS_REQ Request Received");
			break;
				
			case(APCM_AUTHORIZE_REQ):			
				authsta = (authsta_t *)(cmdid);						
				NMM_GenerateNmk(&(authsta->nwkpasswd[0]), authsta->nwkpwdlen, &(authsta->nmk[0]));
				NMM_GenerateNid(&(authsta->nmk[0]), &(authsta->nid[0]));
				NMM_GenerateDak(&(authsta->passwd[0]), authsta->pwdlen, &(authsta->dak[0]));						
				memcpy(&ufrm[i], cmdid, sizeof(authsta_t));	
				i += sizeof(authsta_t);	
				//DEBUG_PRINT(GHDD,DBG_TRACE," APCM_AUTHORIZE_REQ Request Received");
			break;

			case(APCM_SET_SECURITY_MODE_REQ):
			case(APCM_GET_SECURITY_MODE_REQ):
			case(APCM_STA_RESTART_REQ):
			case(APCM_NET_EXIT_REQ):	
			case(APCM_CCO_APPOINT_REQ): 
			case(HOST_CMD_DATAPATH_REQ):
			case(HOST_CMD_SNIFFER_REQ): 
			case(HOST_CMD_BRIDGE_REQ):	
			case(HOST_CMD_DEVICE_MODE_REQ): 
			case(HOST_CMD_HARDWARE_SPEC_REQ):
			case(HOST_CMD_DEVICE_STATS_REQ):			
			case(HOST_CMD_PEERINFO_REQ):
			case(HOST_CMD_SWRESET_REQ):
			case(HOST_CMD_SET_TX_POWER_MODE_REQ):
			case(HOST_CMD_COMMIT_REQ):	
			case(HOST_CMD_TRIGGER_EVENT_REQ): 	
			case(HOST_CMD_GET_VERSION_REQ):
	        case(HOST_CMD_PSSTA_REQ):
	        case(HOST_CMD_PSAVLN_REQ):
			case(HOST_CMD_GV_RESET_REQ):
			case(HOST_CMD_ERASE_FLASH_REQ):
            case (HOST_CMD_COMMISSIONING_REQ):
			//case(DEV_CAP_INFO_CNF):
				DEBUG_PRINT(GHDD,DBG_TRACE,"Request Received");
				memcpy(&ufrm[i], cmdid, pktlen - sizeof(gv_cmd_hdr_t)); 	
				i += pktlen - sizeof(gv_cmd_hdr_t);	
			break;
#if 0			
			case (DEV_CAP_INFO_REQ):
				DEBUG_PRINT(GHDD,DBG_FATAL," DEV_CAP_INFO_REQ Request Received HPGP_MAC_ID");
				{

					u8 data_mem[500];
					u8 *original = data_mem;
					int datalen;
					gv_cmd_hdr_t* hdr  = (gv_cmd_hdr_t *)data_mem;
					gv701x_capability_t * cmdid = (u8 *)(hdr + 1);

					hdr->fc.proto = HPGP_MAC_ID;
					hdr->fc.frm = MGMT_FRM_ID;
					cmdid->command = DEV_CAP_INFO_CNF;
					cmdid->result = SUCCESS;
					cmdid->capability = 3;

					datalen = sizeof(gv_cmd_hdr_t) + sizeof(gv701x_capability_t);
					genl_ghdd_cli_send(original, datalen);
				}
				return(SUCCESS);
			break;
#endif			
			default:
				return (ERROR);
			break;
		}
		intf = HPGP_INTERFACE;
		zmac_hostIf_form_hdr (CONTROL_FRM_ID, HPGP_MAC_ID);
	}else if(req->fc.proto == IEEE802_15_4_MAC_ID)
	{
		switch(*cmdid) 	
		{	
			case(MCPS_DATA_REQUEST):
				DEBUG_PRINT(GHDD,DBG_TRACE,"MCPS_DATA_REQUEST %d | %.*s ", ((mcps_data_req_t *)cmdid)->msduLength, \
				((mcps_data_req_t *)cmdid)->msduLength, ((mcps_data_req_t *)cmdid)->msdu_p ) ;
				zmac_hostIf_form_hdr (CONTROL_FRM_ID, IEEE802_15_4_MAC_ID);
				memcpy(&ufrm[i], cmdid, pktlen - sizeof(gv_cmd_hdr_t)); 	
				i += pktlen - sizeof(gv_cmd_hdr_t);	
				increment_counters(MCPS_DATA_REQUEST, 0);
			break;	
			case(MCPS_PURGE_REQUEST):
			case(MLME_ASSOCIATE_REQUEST):
			case(MLME_DISASSOCIATE_REQUEST):
			case(MLME_GET_REQUEST):
			case(MLME_RESET_REQUEST):
			case(MLME_RX_ENABLE_REQUEST):
			case(MLME_SCAN_REQUEST):
			case(MLME_SET_REQUEST):
			case(MLME_START_REQUEST):
			case(MLME_POLL_REQUEST):
			case(MLME_ORPHAN_RESPONSE):
			case(MLME_ASSOCIATE_RESPONSE):
			case(MLME_SYNC_REQUEST):	
			memcpy(&ufrm[i], cmdid, pktlen - sizeof(gv_cmd_hdr_t)); 	
			i += pktlen - sizeof(gv_cmd_hdr_t);	
			break;
			
			default:
				return (ERROR);
			break;
		}
		intf = LRWPAN_INTERFACE;
		zmac_hostIf_form_hdr (CONTROL_FRM_ID, IEEE802_15_4_MAC_ID);
	}
	else if(req->fc.proto == SYS_MAC_ID){
		switch(*cmdid){
			case (DEV_CAP_INFO_REQ):
				DEBUG_PRINT(GHDD,DBG_TRACE,"SYS_MAC_ID: DEV_CAP_INFO_REQ Received");
				{

					u8 data_mem[(sizeof(gv_cmd_hdr_t) + sizeof(gv701x_capability_t))];
					u8 *original = data_mem;
					int datalen;
					gv_cmd_hdr_t* hdr  = (gv_cmd_hdr_t *)data_mem;
					gv701x_capability_t * cmdid = (gv701x_capability_t *)(hdr + 1);

					hdr->fc.proto = SYS_MAC_ID;
					hdr->fc.frm = MGMT_FRM_ID;
					cmdid->command = DEV_CAP_INFO_CNF;
					cmdid->result = SUCCESS;
					cmdid->capability = rf_capability;
					cmdid->ghdd_init_done = ghdd_init_done;
					datalen = sizeof(gv_cmd_hdr_t) + sizeof(gv701x_capability_t);
					genl_ghdd_cli_send(original, datalen);
				}
				return(SUCCESS);
			break;

			case(DEV_CAP_INFO_CNF):
				DEBUG_PRINT(GHDD,DBG_TRACE,"Request Received");
				memcpy(&ufrm[i], cmdid, pktlen - sizeof(gv_cmd_hdr_t)); 	
				i += pktlen - sizeof(gv_cmd_hdr_t);	
			break;

			default:
				return (ERROR);
			break;
		}
		
	}
	cmd->len = i - CMD_HDR_LEN;	
	zmac_hostIf_send_to_mac(i, HPGP_INTERFACE);
	return(SUCCESS);
}


#endif /* _GHDD_MAC_SAP_ */

void zmac_hostIf_802_15_4_uplink_primitive_handler (
	params_t *p, u8 *p_frm, const u8 frm_len)
{

	u8 idx_b, idx_w, idx_s, idx_sec, idx_a;
	memset ((u8 *)p, 0x00, sizeof (params_t));

	/* Extract all TLV fields */	
	if (!zmac_hostIf_get_tlvs(
			p, &p_frm[HYBRII_FRM_HEADER_LENGTH + 2], 
			frm_len - HYBRII_FRM_HEADER_LENGTH - 2,
			is_little_endian ()))
	{
		/* Invalid field in the frame - we cannot process it further */
		return;
	}

	idx_b = idx_w = idx_s = idx_sec = idx_a = 0;
	
	/* Parse prinitive IDs and call respective handlers */
	switch (p_frm[5])
	{
/*
	case MCPS_DATA_INDICATION:	
		{
		 zmac_hostIf_mcps_data_ind(
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
			p->word_data[idx_w], 
			&p->security_data[idx_sec]);
		break;
		}

	case MCPS_DATA_CONFIRM:			
		{
		zmac_hostIf_mcps_data_cnf (
			p->byte_data[idx_b], 
			p->byte_data[++idx_b], 
			p->word_data[idx_w]);
		break;
		}
		
	case MCPS_PURGE_CONFIRM:
		{
		 zmac_hostIf_mcps_purge_cnf (
		 	p->byte_data[idx_b], 
		 	p->byte_data[++idx_b]);
		break;
		}
		
	case MLME_ASSOCIATE_INDICATION:			
		{
		 zmac_hostIf_mlme_assoc_ind (
		 	p->address_data[idx_a].addr,
		 	p->byte_data[idx_b],
		 	&p->security_data[idx_sec]);
		break;
		}
		
	case MLME_ASSOCIATE_CONFIRM:			
		{
		 zmac_hostIf_mlme_assoc_cnf (
		 	p->short_data[idx_s],
		 	p->byte_data[idx_b],
		 	&p->security_data[idx_sec]);
		break;
		}
		
	case MLME_DISASSOCIATE_INDICATION:			
		{
		zmac_hostIf_mlme_disassoc_ind (
			p->address_data[idx_a].addr,
			p->byte_data[idx_b],
			&p->security_data[idx_sec]);
		break;
		}
		
	case MLME_DISASSOCIATE_CONFIRM:			
		{
		 zmac_hostIf_mlme_disassoc_cnf (
			p->byte_data[idx_b],
			p->byte_data[++idx_b],
			p->short_data[idx_s],
			&p->address_data[idx_a]);
		break;
		}
		
	case MLME_BEACON_NOTIFY_INDICATION:
		{
		zmac_hostIf_mlme_beacon_notify_ind (
			p->byte_data[idx_b],
			&p->pand,
			p->byte_data[++idx_b],
			p->ptr1,
			p->byte_data[++idx_b],
			p->ptr2);
		break;
		}
		
	case MLME_GET_CONFIRM:
		{
		 zmac_hostIf_mlme_get_cnf (
			p->byte_data[idx_b], 
			p->byte_data[++idx_b],
			p->byte_data[++idx_b],
			p->short_data[idx_s],
			p->ptr1);
		break;
		}
			
	case MLME_ORPHAN_INDICATION:
		{
		 zmac_hostIf_mlme_orphan_ind (
		 	(u8 *)&p->address_data[idx_a],
		 	&p->security_data[idx_sec]);
		break;
		}
		
	case MLME_RESET_CONFIRM:			
		{
		 zmac_hostIf_mlme_reset_cnf (
		 	p->byte_data[idx_b]);
		break;
		}
		
	case MLME_RX_ENABLE_CONFIRM:			
		{
		 zmac_hostIf_mlme_rx_enable_cnf (
		 	p->byte_data[idx_b]);
		break;
		}
		
	case MLME_ENERGYSCAN_CONFIRM:
		{
		 zmac_hostIf_mlme_energy_scan_cnf (
			p->byte_data[idx_b],
			p->byte_data[++idx_b],
			p->byte_data[++idx_b],
			&p->pand);
		break;
		}
		
	case MLME_APSCAN_CONFIRM:
		{
		 zmac_hostIf_mlme_ap_scan_cnf (
			p->byte_data[idx_b],
			p->byte_data[++idx_b],
			p->word_data[idx_w], 
			p->byte_data[++idx_b],
			&p->pand);
		break;
		}
		
	case MLME_ORPHANSCAN_CONFRM:			
		{
		 zmac_hostIf_mlme_orphan_scan_cnf (
			p->byte_data[idx_b],
			p->byte_data[++idx_b],
			p->word_data[idx_w]);
		break;
		}
		
	case MLME_COMMSTATUS_INDICATION:
		{
		zmac_hostIf_mlme_comm_stats_ind (
			p->short_data[idx_s],
			p->byte_data[idx_b],
			&p->address_data[idx_a],
			p->byte_data[++idx_b], 
			&p->address_data[++idx_a], 
			p->byte_data[++idx_b],
			&p->security_data[idx_sec]);
		break;
		}
		
	case MLME_SCAN_CONFIRM:
		{
			 zmac_hostIf_mlme_ap_scan_cnf (
				p->byte_data[idx_b],
				p->byte_data[++idx_b],
				p->byte_data[++idx_b],
				&p->pand);
			break;
		}
				
	case MLME_SET_CONFIRM:
		{
		 zmac_hostIf_mlme_set_cnf (
		 	p->byte_data[idx_b],
		 	p->byte_data[++idx_b],
		 	p->byte_data[++idx_b]);
		break;
		}
		
	case MLME_START_CONFIRM:
		{
		 zmac_hostIf_mlme_start_cnf (
		 	p->byte_data[idx_b]);
		break;
		}
		
	case MLME_SYNC_LOSS_INDICATION:
		{
		 zmac_hostIf_mlme_sync_loss_ind (
			p->byte_data[idx_b], 
			p->short_data[idx_s], 
			p->byte_data[++idx_b], 
			p->byte_data[++idx_b], 
			&p->security_data[idx_sec]);
		break;
		}
		
	case MLME_POLL_CONFIRM:
	{
		zmac_hostIf_mlme_poll_cnf(p->byte_data[idx_b]);		
		break;
	}
*/
	default:
		break;		
	}
	return;
}


/* This API is called to handle Control Frames
 * @Parameters 
 * p_frm - pointer to the frame received from the Host interface
 * frm_len - length of the Hybrii frame
 */	
void zmac_hostIf_control_frm_handler (u8 *p_frm, const u16 frm_len)
{
	params_t p;

	if (GET_CMD_PROTOCOL(p_frm[CMD_HDR_START]) == IEEE802_15_4_MAC_ID) 	{
		//hmac_hostIf_hpgp_uplink_primitive_handler (&p, p_frm, frm_len);//Kiran commented this. As it leads to kernel crash
		return;
	} else if (GET_CMD_PROTOCOL(p_frm[CMD_HDR_START]) == HPGP_MAC_ID) {
		hmac_hostIf_hpgp_uplink_primitive_handler (&p, p_frm, frm_len);
	} else {
		DEBUG_PRINT(GHDD,DBG_TRACE,"Neither LRWPAN nor HPGP (%1x)", \
			GET_CMD_PROTOCOL(p_frm[CMD_HDR_START]));			
		return; //error
	}
}


/* This API parses frame received from the Host interface and calls corresponding primitive handlers 
 * @Parameters
 * p_frm - pointer to the frame received from the Host interface
 * frm_len - length of the Hybrii frame
 */
void zmac_hostIf_frm_parser (u8 *p_frm, const u8 frm_len)
{
#if 0
    hybrii_t *pHybrii = (hybrii_t*)p_frm;
    u8 *ethFrame;
    
    if (pHybrii->type == CONTROL_FRM_ID)
    {
		zmac_hostIf_control_frm_handler (p_frm, frm_len);
    }
    else if (pHybrii->type == DATA_FRM_ID)
    {
		ethFrame = (u8*)(pHybrii + 1);
        dev->stats.rx_bytes += n;
    	skb->protocol = eth_type_trans(skb, dev);
	    netif_receive_skb(skb);
    	dev->stats.rx_packets++;
   }
   else
   {
		return; // error
   }
   #endif
}


/* This API is called when data is to be sent to Host 
 * @Parameters
 * frm_len - length of the Hybrii frame
 */
void zmac_hostIf_send_to_mac (u16 frm_len, u8 intf) {
	//static unsigned int local_instance_num = 0;
	struct sk_buff *skb = NULL; // kernel buffer to send or receive data to/from user 
    //hybrii_t *hybriiHdr = (hybrii_t*)ufrm;    
    sEth2Hdr *ethhdr;
	u8 *mac_addr = NULL;

	//DEBUG_PRINT(GHDD, DBG_INFO," ufrm[0] %x, len %d", ufrm[CMD_HDR_START],frm_len);
	if((GET_CMD_FRAMETYPE() == MGMT_FRM_ID ) ||
	   (GET_CMD_FRAMETYPE() == CTRL_FRM_ID )) {
#ifdef _GHDD_MAC_SAP_

		frm_len += 	gbl_headroom;
		skb = alloc_skb(frm_len + NET_IP_ALIGN, GFP_ATOMIC);
		if (skb == NULL) {
			DEBUG_PRINT(GHDD,DBG_ERROR,"Could not allocate skbuff");
			return;
		}

		skb_reserve(skb, NET_IP_ALIGN);

		ethhdr = (sEth2Hdr *)skb->data;

		mac_addr = ghdd_get_mac_addr();
		if (!mac_addr) {
			DEBUG_PRINT(GHDD,DBG_ERROR,"Could not get source MAC address");
			return;
		}
		memcpy (ethhdr->srcaddr, mac_addr, MAC_ADDR_LEN);
		memset (ethhdr->dstaddr, 0xFF, MAC_ADDR_LEN);
		ethhdr->ethtype = HTONS(ETH_TYPE_HPGP);
		
		skb->data = (u8 *)skb_put (skb, (frm_len));
		skb_reset_mac_header(skb);
		skb_reset_network_header(skb);		
		skb_reset_transport_header(skb);		
		
		memset(&skb->data[gbl_headroom], 0x00, skb->len - gbl_headroom);
		memcpy(&skb->data[gbl_headroom], (u8*)(&ufrm[CMD_HDR_START]),
			skb->len - gbl_headroom);		
		
#if (DEBUG_MAXLEVEL_GHDD  == DBG_TRACE)
		{
			
			unsigned int i;

			local_instance_num++;
			DEBUG_PRINT(GHDD,DBG_TRACE,"tx %04u | len %u | proto %04x | dev %s",\
				local_instance_num,skb->len,skb->protocol,skb->dev->name);
			DEBUG_PRINT(GHDD,DBG_TRACE,"tx %04u" ": sk %p|des %p|head %p|data %p|end %p|tail %p", \
				local_instance_num,skb->sk,skb->destructor,skb->head,skb->data,skb->end,skb->tail);
			DEBUG_PRINT(GHDD,DBG_TRACE,"tx %04u" ": th %p | nh %p | mh %p):", \
				local_instance_num,skb->transport_header,skb->network_header,skb->mac_header);
			for (i = 0; (i < skb->len); i++) {		
				if(i%20 == 0)
					DEBUG_PRINT(GHDD,DBG_TRACE,"tx %04u" ":skb@%p %02x:",local_instance_num,skb,i/20);
				printk("%02x ",skb->data[i]);
			}
		}
#endif

        skb->ip_summed = CHECKSUM_UNNECESSARY;
		DEBUG_PRINT(GHDD,DBG_TRACE, "Sending from MAC-SAP to HPGP Driver");
		ghdd_net_driver_tx (skb, GET_CMD_FRAMETYPE(), intf);		  	
#endif

		// Send to Mac - IF - tbd
#ifdef HYBRII_MAC_INTF_TEST
		zmac_intf_frm_parser (ufrm, frm_len);
#endif

    }else {
		DEBUG_PRINT(GHDD,DBG_ERROR,"Invalid frame type");
	}
}








