
#ifndef __LRWPAN__
#define __LRWPAN__


#define LRWPAN_SUCCESS					(0x00)
#define CHANNEL_PAGE_VALUE				(0x00)
/**
 * Maximun Number of MCPS data requests sent to FW
 */
#define MAX_MCPS_DATA_REQUEST			(0xFA)
/**
 * Maximun Number of neighbor devices - associated devices to this device
 */
#define MAX_NEIGHBOR_DEVICES			(0x0A)
/**
 * Security Key Length
 */
#define MAX_SEC_KEY_LEN					(16)
/**
 * Max IEEE802.15.4 Data length
 */
#define MAX_IEEE802_15_4_DATA			(127)
/**
 * Max IEEE802.15.4 Short Header length - Considers short addresses
 */
#define MAX_IEEE802_15_4_SHORT_HEADER 	(8)
/**
 * Max IEEE802.15.4 Short Header length - Considers extended addresses
 */
#define MAX_IEEE802_15_4_LONG_HEADER 	(25)

/**
 * Address Mode: NO ADDRESS
 */
#define FCF_NO_ADDR                     (0x00)

/**
 * Address Mode: RESERVED
 */
#define FCF_RESERVED_ADDR               (0x01)

/**
 * Address Mode: SHORT
 */
#define FCF_SHORT_ADDR                  (0x02)

/**
 * Address Mode: LONG
 */
#define FCF_LONG_ADDR                   (0x03)

/* Association status values from table 68 */

/**
 * Association status code value mlme_associate_resp().
 */
#define ASSOCIATION_SUCCESSFUL          (0)

/**
 * Association status code value (see mlme_associate_resp()) .
 */
#define PAN_AT_CAPACITY                 (1)

/**
 * Association status code value (see mlme_associate_resp()) .
 * @ingroup apiConst
 */
#define PAN_ACCESS_DENIED               (2)

/**
 * TxOptions parameter in mcps_data_req().
 */ 
#define TX_ACK							BIT(0)
#define TX_GTS							BIT(1)
#define TX_INDIRECT_BIT					BIT(2)

/**
 * Value for TxOptions parameter in mcps_data_req().
 */ 
#define TX_CAP                          (0x00)
#define TX_CAP_ACK                      (0x01)
#define TX_GTS1                         (0x02)
#define TX_GTS_ACK                      (0x03)
#define TX_INDIRECT1                    (0x04)
#define TX_INDIRECT_ACK                 (0x05)
#define TX_INDIRECT_GTS                 (0x06)
#define TX_INDIRECT_GTS_ACK             (0x07)


typedef enum {
	IDLE		= 0x00,
	DEVICE_START,		/* Only For Coordinator */
	SCANNING,
	UNASSOCIATED,
	ASSOCIATED,
	AUTHENTICATED
}lrwpan_state_t;

typedef enum {
	DEVICE_START_REQ, /* Only For Coordinator */
	DEVICE_START_CNF, /* Only For Coordinator */	
	BEACON_REQ,
	BEACON_RECEIVED,
	ASSOC_CNF,
	ASSOC_TIMEOUT,
	NETWORK_NOT_FOUND,
	DISCONNECT,
	MAC_KEY_SET
}lrwpan_event_t;

typedef enum {
	COORDINATOR,
	ENDDEVICE,
	ROUTER
}lrwpan_device_type_t;

typedef enum {
	ASSOC_SUCCESS 	= 0x00,
	LRWPAN_FULL,
	LRWPAN_ACCESS_DENIED,
	ASSOC_INVALID		= 0xFF,
}lrwpan_assoc_status_t;

typedef enum {
	ALTER_PAN_COORDINATOR	= BIT(0),
	DEVICE_TYPE				= BIT(1), /* 1 - FFD, 0 - RFD */
	POWER_SOURCE			= BIT(2),
	RX_ON_IDLE				= BIT(3),
	RESVERED				= BIT(4),
	SECURITY_CAPABILITY		= BIT(5),
	ALLOCATE_ADDRESS		= BIT(6)
}lrwpan_device_capability_t;

typedef unsigned short lrwpan_short_addr_t;
typedef u64 ieee802_15_4_addr_t;

typedef struct {
	u8 inuse;
	u8 neighbor_index;
	u8 capability_info;
	u8 state; /* IDLE, UNASSOCIATED, ASSOCIATED */
	sec_t security_info;
	lrwpan_short_addr_t nwk_addr;
	ieee802_15_4_addr_t neighbor_nwk_addr;
}lrwpan_neighbor_t;

typedef struct {
	u8 inuse;	
	/* MCPS Data Request handler */
	u8 mcps_data_req_handler;
}lrwpan_handler_t;

typedef struct {
	/* Device Type */
	lrwpan_device_type_t dev;
	/* MAC State of this device */
	lrwpan_state_t state;
	/* Channel number of this device - obtained from user */
	u8 channel;
	/* PAN ID - obtained from user */
	u16 panid;
	/* Capability of this device - obtained from user */
	u8 capability;
	/* Device IEEE802.15.4 short address */
	lrwpan_short_addr_t device_nwk_addr;
	/* Coordinator IEEE802.15.4 short address */
	lrwpan_short_addr_t coordinator_nwk_addr;
	/* Coordinator IEEE802.15.4 extended address */
	ieee802_15_4_addr_t coordinator_addr;
	/* Address mode */
	u8 coordinator_addr_mode;
	/* Devices associated to this device are neighbors */
	lrwpan_neighbor_t neighbor[MAX_NEIGHBOR_DEVICES];
	/* Security parameters */
	sec_t security_parameters;
	/* MCPS Data Handler */
	u8 mcps_data_req_handler_cntr;	
	lrwpan_handler_t handler[MAX_MCPS_DATA_REQUEST];
	/* MAC security Key - Upper layer provides security key to MAC */
	u8 mac_sec_key[MAX_SEC_KEY_LEN];	
}lrwpan_db_t;

typedef struct {
	/* Channel Number - Range 0x0B to 0x1A */
	u8 channel_num;
	/* Beacon Sending Start Time - 0x00000000 means immediately */
	u32 starttime;
	/* PAN ID */
	u16 panid;
	/* Beacon order - 0x0F is ignore */ 
	u8 beacon_order;
	/* Super frame order - 0x0F is ignore */
	u8 superframe_order;
	/* Device PAN Coordinator - TRUE or FALSE*/
	u8 pancordinator;
	/* Battery Life Extension - TRUE - Receiver of beaconing device is disabled 
	for macBattLifeExtPeriods full backoff periods after the interframe spacing 
	*/
	/* Device Capabilities */
	u8 capability;
	
	u8 battery_life_ext;

	/* Coordinator Realignment */
	u8 coord_realign;
	/* Coordinator Realign Security */
	sec_t coordrealign_sec;
	/* Beacon Security */
	sec_t beacon_sec;
	
	u8 device_type;
	u8 beacon_security;
	u8 coord_security;
}lrwpan_start_dev_t;

/* APIs */
void lrwpan_mlme_start_cnf (u8 status);
void lrwpan_mlme_beacon_notify_ind( 
	u8 bsn, pand_t *p_pan_descr, u8 pend_addr_spec,
	u8 *p_addr_list, u8 sdu_length, u8 *p_sdu );
void lrwpan_mlme_assoc_ind( 
	u8 dev_addr[8], u8 capability_info, sec_t *p_sec );
void lrwpan_mlme_assoc_cnf (u16 assoc_short_addr, 
	u8 status, sec_t *p_sec );
void lrwpan_mlme_comm_stats_ind ( 
	u16 pan_id, u8 src_addr_mode, 
	addr_t *p_src_addr, u8 dest_addr_mode,
	addr_t *p_dest_addr, u8 status, sec_t *p_sec );
bool lrwpan_mcps_data_req(
	u8 dest_addr_mode, u16 dest_pan_id, 
	addr_t *p_dest_addr, u8 src_addr_mode,
	addr_t *p_src_addr,
	u8 msdu_len, u8 *p_msdu);
void lrwpan_mcps_data_cnf(
	u8 msdu_handle, u8 status, u32 time_stamp );
bool lrwpan_mcps_data_ind(	
	u8 src_addr_mode, u16 src_pan_id, addr_t *p_src_addr,
	u8 dest_addr_mode, u16 dest_pan_id, addr_t *p_dest_addr,
	u8 msdu_len, u8 *p_msdu, u8 mpdu_link_quality,
	u8 dsn, u32 time_stamp, sec_t *p_sec );

#endif /* __LWPAN__ */

