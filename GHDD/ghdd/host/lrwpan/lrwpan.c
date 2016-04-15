#include <linux/module.h>
#include <linux/random.h>
#include <linux/jiffies.h>
#include "papdef.h"
#include "mac_intf_common.h"
#include "lrwpan.h"

#define GV_LRWPAN_DRV_DBG
#ifdef GV_LRWPAN_DRV_DBG
#define CDBG_LRWPAN(msg, args...) do { 				\
		printk(KERN_INFO msg, ##args );			\
}while (0)
#else
#define CDBG_LRWPAN(msg, args...)
#endif


lrwpan_db_t lrwpan_db;


/* Extern */
extern void zmac_hostIf_mlme_assoc_ind(
	u8 dev_addr[8], u8 capability_info, sec_t *p_sec );
extern void zmac_hostIf_mlme_assoc_cnf( 
	u16 assoc_short_addr, u8 status, sec_t *p_sec );
extern void zmac_hostIf_mlme_disassoc_ind(
	u8 dev_addr[8], u8 dissoc_reason, sec_t *p_sec );
extern void zmac_hostIf_mlme_beacon_notify_ind( 
	u8 bsn, pand_t *p_pan_descr, u8 pend_addr_spec,
	u8 *p_addr_list, u8 sdu_length, u8 *p_sdu );
extern void zmac_hostIf_mlme_energy_scan_cnf( 
	u8 status, u8 channel_page, 
	u8 result_list_size, pand_t *p_pan_descs );
extern void zmac_hostIf_mlme_comm_stats_ind( 
	u16 pan_id, u8 src_addr_mode, 
	addr_t *p_src_addr, u8 dest_addr_mode,
	addr_t *p_dest_addr, u8 status, sec_t *p_sec );
extern void zmac_hostIf_mlme_scan_req( 
	u8 scan_type, u32 scan_channels, u8 scan_duration, 
	u8 channel_page, sec_t *p_sec );
extern void zmac_hostIf_assoc_req( 
	u8 logical_channel, u8 channel_page, 
	u8 coord_addr_mode, u16 coord_pan_id,
	addr_t *p_coord_addr, u8 capability_info,
	sec_t *p_sec );

extern void zmac_hostIf_mlme_start_req( 
	u16 pan_id, u8 logical_channel, 
	u8 channel_page, u32 start_time,
	u8 beacon_order, u8 super_frame_order,
	u8 pan_coordinator, u8 ble, 
	u8 coord_realignment, 
	sec_t *pcoord_realignmentSecurity,
	sec_t *p_beacon_sec );

extern void zmac_hostIf_mlme_assoc_resp( 
	u8 *p_dev_addr, u16 assoc_short_addr, 
	u8 status, sec_t *p_sec );

extern void zmac_hostIf_mcps_data_req(
	u8 dest_addr_mode, u16 dest_pan_id, 
	addr_t *p_dest_addr, u8 src_addr_mode,
	u16 src_pan_id, addr_t *p_src_addr,
	u8 msdu_len, u8 *p_msdu, u8 msdu_handle, 
	u8 tx_options, sec_t *p_sec );


/* Local API's */
static bool is_data_tx_indirect (addr_t *p_dest_addr, u8 dest_addr_mode);

void lrwpan_init (void)
{
	memset (&lrwpan_db, 0x00, sizeof (lrwpan_db_t));
	lrwpan_db.dev = COORDINATOR;
	lrwpan_db.panid = 0xFFFF;
	lrwpan_db.mcps_data_req_handler_cntr = 0x00;
	memset (lrwpan_db.neighbor, 0x00, sizeof (lrwpan_neighbor_t) * 
											MAX_NEIGHBOR_DEVICES); 
	memset (lrwpan_db.handler, 0x00, sizeof (lrwpan_handler_t) * 
											MAX_MCPS_DATA_REQUEST);
}

void lrwpan_state_machine (lrwpan_event_t event)
{
	sec_t security;
	switch (lrwpan_db.state) {
	case IDLE:		
		switch (event) {
		case DEVICE_START_REQ:		
			if (COORDINATOR == lrwpan_db.dev) {
				lrwpan_db.state = DEVICE_START;
			} else {
				lrwpan_db.state = SCANNING;
				security.key_idx = 0x00;
				security.key_id_mode = 0x00;
				security.key_src = 0; //TODO
				security.sec_level = 0x00;
				/* TODO - Start Timer for sacnning */
				zmac_hostIf_mlme_scan_req (0, 0xFFFFFFFF, 0x0F, 0, &security);
				lrwpan_state_machine (BEACON_REQ);
			}
			
			CDBG_LRWPAN("\nLRWPAN State Change -  START, START_REQ");		
		case DEVICE_START_CNF:
		case BEACON_REQ:
		case BEACON_RECEIVED:
		case ASSOC_CNF:
		case ASSOC_TIMEOUT:
		case NETWORK_NOT_FOUND:
		case DISCONNECT:
		case MAC_KEY_SET:
		default:
			CDBG_LRWPAN("\nLRWPAN State Change -  INVALID EVENT");
			break;
			
		}
				
		break;

	case DEVICE_START:
		switch (event) {
		case DEVICE_START_CNF:		
			if (COORDINATOR == lrwpan_db.dev) {
				lrwpan_db.state = ASSOCIATED;
				CDBG_LRWPAN("\nLRWPAN State Change -  ASSOCIATED, START_CNF");
			} else {
				CDBG_LRWPAN("\nLRWPAN State Change -  INVALID EVENT");
				return;
			}
				
		case DEVICE_START_REQ:
		case BEACON_REQ:
		case BEACON_RECEIVED:
		case ASSOC_CNF:
		case ASSOC_TIMEOUT:
		case NETWORK_NOT_FOUND:
		case DISCONNECT:
		case MAC_KEY_SET:
		default:
			printk("\nLRWPAN State Change -  INVALID EVENT");
			break;
			
		}

		break;
		
		
	case SCANNING:
		switch (event) {
		case BEACON_RECEIVED:
			/* Send Association Request */
			/* TODO - Start Timer for Assoc Confirm */
			zmac_hostIf_assoc_req (lrwpan_db.channel, CHANNEL_PAGE_VALUE, 
									lrwpan_db.coordinator_addr_mode, 
									lrwpan_db.panid, 
									(addr_t *)&lrwpan_db.coordinator_addr,
									lrwpan_db.capability, 
									&lrwpan_db.security_parameters);
			
			CDBG_LRWPAN("\nLRWPAN State Change -  UNASSOCIATED, BEACON_RECEIVED");
			lrwpan_db.state = UNASSOCIATED;
			break;
		case DEVICE_START_REQ:
		case BEACON_REQ:
		case DEVICE_START_CNF:
		case ASSOC_CNF:
		case ASSOC_TIMEOUT:
		case NETWORK_NOT_FOUND:
		case DISCONNECT:
		case MAC_KEY_SET:	
		default:
			printk("\nLRWPAN State Change -  INVALID EVENT");
			break;
		}
		break;

	case UNASSOCIATED:
		switch (event) {
			case ASSOC_CNF:				
				CDBG_LRWPAN("\nLRWPAN State Change -  ASSOCIATED, ASSOC_CNF");
				lrwpan_db.state = ASSOCIATED;
				break;
			case BEACON_REQ:				
				CDBG_LRWPAN("\nLRWPAN State Change -  IDLE, BEACON_REQ");
				lrwpan_db.state = IDLE;
				lrwpan_state_machine(DEVICE_START_REQ);
				break;
			case DEVICE_START_REQ:
			case DEVICE_START_CNF:
			case BEACON_RECEIVED:
			case ASSOC_TIMEOUT:
			case NETWORK_NOT_FOUND:
			case DISCONNECT:
			case MAC_KEY_SET:					
			default:
				printk("\nLRWPAN State Change -  INVALID EVENT");
				break;
		}
		break;
		
	case ASSOCIATED:
		switch (event) {
			case ASSOC_CNF:
				/* Send Association Request */
				lrwpan_db.state = AUTHENTICATED;
				break;	
			case DEVICE_START_REQ:
			case DEVICE_START_CNF:
			case BEACON_RECEIVED:
			case ASSOC_TIMEOUT:
			case NETWORK_NOT_FOUND:
			case DISCONNECT:
			case MAC_KEY_SET:	
			default:
				printk("\nLRWPAN State Change -  INVALID EVENT");
				break;
		}
		break;

	case AUTHENTICATED:
		break;
	default:
		printk("\nLRWPAN State Change -  INVALID EVENT");
		break;
	}
}

void lrwpan_start_device (lrwpan_start_dev_t *p_startdev)
{
	sec_t beacon_sec;
	sec_t coord_realign;

	CDBG_LRWPAN("\nReceived Start Device Request from upper layer");
	CDBG_LRWPAN("\n Channel 			:%d", p_startdev->channel_num);
	CDBG_LRWPAN("\n PANID 				:%d", p_startdev->panid);
	CDBG_LRWPAN("\n Pan Coordinator 	:%d", p_startdev->pancordinator);
	CDBG_LRWPAN("\n capability 			:%d", p_startdev->capability);	

	coord_realign.key_idx = 0;
	coord_realign.key_id_mode = 0;
	coord_realign.key_src = 0;
	coord_realign.sec_level = 0;
	
	beacon_sec.key_idx = 0;
	beacon_sec.key_id_mode = 0;
	beacon_sec.key_src = 0;
	beacon_sec.sec_level = 0;

	if (p_startdev->channel_num < 0x0B && p_startdev->channel_num > 0x1A) {
		printk("\n LRWPAN : ERROR : Invalid channel number");
	}
	lrwpan_db.channel = p_startdev->channel_num;

	if (p_startdev->panid == 0xFFFF) {
		printk("\n LRWPAN : ERROR : Invalid PAN ID");
	}
	lrwpan_db.panid = p_startdev->panid;
	if (p_startdev->device_type > ENDDEVICE) {
		printk("\n LRWPAN : ERROR : Invalid device type");
	}
	lrwpan_db.dev = p_startdev->device_type;
	lrwpan_db.capability = p_startdev->capability;

	if (lrwpan_db.dev == COORDINATOR) {
		/* TODO - Get Address from the LRWPAN Net driver */
		//memcpy(&lrwpan_db.coordinator_addr, &p_startdev->coordinator_addr, 
			//	sizeof	(ieee802_15_4_addr_t));
		memcpy(&lrwpan_db.security_parameters, &p_startdev->coordrealign_sec, 
					sizeof(sec_t) );
	}
	
	zmac_hostIf_mlme_start_req (p_startdev->panid, 
							p_startdev->channel_num, 
							0, 0, 0xFF, 0xFF, 
							p_startdev->device_type, 0, 0, 
							&coord_realign, &beacon_sec);

	
	lrwpan_state_machine (DEVICE_START_REQ);
}
void lrwpan_mlme_start_cnf (u8 status)
{
	if (LRWPAN_SUCCESS == status) {
		lrwpan_state_machine (DEVICE_START_CNF);
		CDBG_LRWPAN("\nDevice Started");
		return;
	}
	lrwpan_db.state = IDLE;
}

void lrwpan_mlme_beacon_notify_ind( 
	u8 bsn, pand_t *p_pan_descr, u8 pend_addr_spec,
	u8 *p_addr_list, u8 sdu_length, u8 *p_sdu ) {

	CDBG_LRWPAN("\nBeacon Notify received");
	CDBG_LRWPAN("\n Channel 			:%d", p_pan_descr->chan.logical_channel);
	CDBG_LRWPAN("\n PANID 				:%d", p_pan_descr->coor_pan_id);
	CDBG_LRWPAN("\n Coordinator Address	:%x:%x:%x:%x:%x:%x", 
				p_pan_descr->coor_addr.addr[0], p_pan_descr->coor_addr.addr[1], 
				p_pan_descr->coor_addr.addr[2], p_pan_descr->coor_addr.addr[3],  
				p_pan_descr->coor_addr.addr[4], p_pan_descr->coor_addr.addr[5]);	

	if (lrwpan_db.panid == 0xFFFF) {
		if (lrwpan_db.panid == p_pan_descr->coor_pan_id) {
			lrwpan_db.channel = (p_pan_descr->chan.logical_channel);
			lrwpan_db.coordinator_addr_mode = p_pan_descr->coor_addr_mode;
			memcpy(&lrwpan_db.coordinator_addr, &p_pan_descr->coor_addr, 
					sizeof	(ieee802_15_4_addr_t));
			memcpy(&lrwpan_db.security_parameters, &p_pan_descr->sec, 
						sizeof(sec_t) );
			lrwpan_state_machine (BEACON_RECEIVED);
			return;
		} else {
			lrwpan_db.state = IDLE;
		}
	}
	lrwpan_state_machine (DEVICE_START_REQ);
}

void lrwpan_mlme_assoc_ind( 
	u8 dev_addr[8], u8 capability_info, sec_t *p_sec )
{
	u8 i;
	u8 match;
	u8 free_idx;
	lrwpan_assoc_status_t status;
	lrwpan_short_addr_t nwk_addr = 0;

	status = ASSOC_INVALID;
	match = FALSE;
	free_idx = 0xFF;
	for (i = 0; i < MAX_NEIGHBOR_DEVICES; i++) {
		if ((free_idx == 0xFF) && (!lrwpan_db.neighbor[i].inuse)) {
			free_idx = i;
		}
		if ((0 == memcmp(&lrwpan_db.neighbor[i].neighbor_nwk_addr, 
						dev_addr, sizeof (ieee802_15_4_addr_t))) && 
						((lrwpan_db.neighbor[i].capability_info & DEVICE_TYPE) ==
						(capability_info & DEVICE_TYPE))){
			match = i + 1;
			break;
		}
	}

	if (match) {
		/* Entry Exists in neighbor table */
		nwk_addr = lrwpan_db.neighbor[match - 1].nwk_addr;
	} else {		

		if (free_idx == MAX_NEIGHBOR_DEVICES) {
			status = PAN_AT_CAPACITY;
		} else if (lrwpan_db.security_parameters.sec_level != 
					p_sec->sec_level) {
			status = PAN_ACCESS_DENIED;
		} else {
			status = ASSOCIATION_SUCCESSFUL;		
			nwk_addr = (u16)jiffies;//srandom32(jiffies);
			memcpy (&lrwpan_db.neighbor[free_idx].neighbor_nwk_addr,
				dev_addr, sizeof (ieee802_15_4_addr_t));
			lrwpan_db.neighbor[free_idx].capability_info = capability_info;

			memcpy (&lrwpan_db.security_parameters, p_sec, sizeof(sec_t));

			lrwpan_db.neighbor[free_idx].neighbor_nwk_addr = nwk_addr;	
			lrwpan_db.neighbor[free_idx].inuse = TRUE;
			lrwpan_db.neighbor[free_idx].state = UNASSOCIATED;
		}
	}
	zmac_hostIf_mlme_assoc_resp (dev_addr, nwk_addr, status, 
		&lrwpan_db.security_parameters);
	
}

void lrwpan_mlme_assoc_cnf (u16 assoc_short_addr, u8 status, sec_t *p_sec ) {


	if (status == ASSOCIATION_SUCCESSFUL){
		lrwpan_state_machine (ASSOC_CNF);
		lrwpan_db.device_nwk_addr = assoc_short_addr;
		memcpy (&lrwpan_db.security_parameters, p_sec, sizeof(sec_t));		
	} else {
		lrwpan_state_machine (BEACON_REQ);
	}
}


void lrwpan_mlme_comm_stats_ind ( 
	u16 pan_id, u8 src_addr_mode, 
	addr_t *p_src_addr, u8 dest_addr_mode,
	addr_t *p_dest_addr, u8 status, sec_t *p_sec )
{
	u8 i;
	
	if (status == ASSOC_SUCCESS) {		
		for (i = 0; i < MAX_NEIGHBOR_DEVICES; i++) {
			if (lrwpan_db.neighbor[i].state == UNASSOCIATED) {
				if ((dest_addr_mode == FCF_LONG_ADDR) &&
					(0 == memcmp (&lrwpan_db.neighbor[i].neighbor_nwk_addr,
									p_dest_addr->addr, sizeof (ieee802_15_4_addr_t)))){
					lrwpan_db.neighbor[i].state = ASSOCIATED;
						return;
				}
			}
		}
	}
}


bool lrwpan_mcps_data_req(
	u8 dest_addr_mode, u16 dest_pan_id, 
	addr_t *p_dest_addr, u8 src_addr_mode,
	addr_t *p_src_addr,
	u8 msdu_len, u8 *p_msdu)
{
	u8 tx_options = 0;

	if (lrwpan_db.state != ASSOCIATED) {
		printk("\n Device is not associated ");
		return FALSE;
	}
	if ((dest_addr_mode != FCF_SHORT_ADDR) || 
		(dest_addr_mode != FCF_LONG_ADDR)) {
		printk("\n Destination Address mode is invalid ");
		return FALSE;
	}
	
	if ((src_addr_mode != FCF_SHORT_ADDR) || 
		(src_addr_mode != FCF_LONG_ADDR)) {
		printk("\n Source Address mode is invalid ");
		return FALSE;
	}

	if (msdu_len > MAX_IEEE802_15_4_DATA - MAX_IEEE802_15_4_SHORT_HEADER) {
		printk("\n Invalid data length ");
			return FALSE;
	}

	if (lrwpan_db.mcps_data_req_handler_cntr > MAX_MCPS_DATA_REQUEST) {
		lrwpan_db.mcps_data_req_handler_cntr = 0;
	} else {
		lrwpan_db.handler[lrwpan_db.mcps_data_req_handler_cntr].mcps_data_req_handler = 
								lrwpan_db.mcps_data_req_handler_cntr++;
		lrwpan_db.handler[lrwpan_db.mcps_data_req_handler_cntr].inuse = TRUE;
	}

	tx_options |= TX_ACK;
	/* Determine if it is indirect transmission */
	if (is_data_tx_indirect (p_dest_addr, dest_addr_mode)) {
		tx_options |= TX_INDIRECT1;
	}
	zmac_hostIf_mcps_data_req(
		dest_addr_mode, dest_pan_id, 
		p_dest_addr, src_addr_mode,
		lrwpan_db.panid, p_src_addr,
		msdu_len, p_msdu, 
		lrwpan_db.handler[lrwpan_db.mcps_data_req_handler_cntr - 1].mcps_data_req_handler, 
		tx_options, &lrwpan_db.security_parameters);
}

void lrwpan_mcps_data_cnf(
	u8 msdu_handle, u8 status, u32 time_stamp )
{
	u8 i;

	for (i = 0; i < MAX_MCPS_DATA_REQUEST; i++) {
		if (lrwpan_db.handler[i].inuse && 
			lrwpan_db.handler[i].mcps_data_req_handler == msdu_handle) {
			lrwpan_db.handler[i].inuse = FALSE;		
			lrwpan_db.handler[i].mcps_data_req_handler = 0;
			CDBG_LRWPAN("\nMCPS Data Cnf - Status - %d", status);
			/* TODO Update LRWPAN driver statitistics */
			return;
		}			
	}
	printk("\nMCPS Data Cnf - Handler Match Fail - %d", msdu_handle);	
}

bool lrwpan_mcps_data_ind(	
	u8 src_addr_mode, u16 src_pan_id, addr_t *p_src_addr,
	u8 dest_addr_mode, u16 dest_pan_id, addr_t *p_dest_addr,
	u8 msdu_len, u8 *p_msdu, u8 mpdu_link_quality,
	u8 dsn, u32 time_stamp, sec_t *p_sec )
{
	if (lrwpan_db.state != ASSOCIATED) {
		printk("\n Device is not associated ");
		return FALSE;
	}
	if ((dest_addr_mode != FCF_SHORT_ADDR) || 
		(dest_addr_mode != FCF_LONG_ADDR)) {
		printk("\n Destination Address mode is invalid ");
		return FALSE;
	}
	
	if ((src_addr_mode != FCF_SHORT_ADDR) || 
		(src_addr_mode != FCF_LONG_ADDR)) {
		printk("\n Source Address mode is invalid ");
		return FALSE;
	}

	/* TODO : Check security paramenters */
	if (msdu_len > MAX_IEEE802_15_4_DATA - MAX_IEEE802_15_4_SHORT_HEADER) {
		printk("\n Invalid data length ");
			return FALSE;
	}

	/* TODO : Send it to upper layer */

}

static bool is_data_tx_indirect (addr_t *p_dest_addr, u8 dest_addr_mode)
{
	u8 i;
	
	for (i = 0; i < MAX_NEIGHBOR_DEVICES; i++) {
		if (FCF_LONG_ADDR == dest_addr_mode) {
			if ((lrwpan_db.neighbor[i].neighbor_nwk_addr == 
								p_dest_addr->ext_addr) && 
				(lrwpan_db.neighbor[i].neighbor_nwk_addr == 
								p_dest_addr->ext_addr)) {
				if (lrwpan_db.neighbor[i].capability_info & RX_ON_IDLE) {
					return TRUE;
				}
			}
		} else if (FCF_SHORT_ADDR == dest_addr_mode) {
			if (lrwpan_db.neighbor[i].nwk_addr == p_dest_addr->shr_addr)  {
				if (lrwpan_db.neighbor[i].capability_info & RX_ON_IDLE) {
					return TRUE;
				}
			}
		}
	}
	return FALSE;
}


