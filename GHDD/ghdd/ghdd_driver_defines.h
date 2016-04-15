#ifndef _HPGP_LINUX_DRIVER_API_H_
	#define	_HPGP_LINUX_DRIVER_API_H_

	/* NETLINK DEFINES */
	#define NETLINK_HPGP			(20)
	#define MAX_PAYLOAD 			(256)

	/* LENGTH VALUES */
	#define MAX_REQ_LEN 			(256)
	#define MAX_RSP_LEN 			(256)
	#define HDR_LEN 				(3)
#ifndef MAX_PASSWD_LEN	
	#define MAX_PASSWD_LEN			(64)
#endif

#ifndef HPGP_MAC_ADDR_LEN
	#define HPGP_MAC_ADDR_LEN			(6)
#endif

	/* COMMANDS */
	#define SET_SECURITY_MODE		(0x01)
	#define SET_DEFAULT_NET_ID		(0x02)
	#define RESTART_STA 			(0x03)
	#define SET_NETWORK 			(0x04)
	#define NET_EXIT				(0x05)
	#define APPOINT_CCO 			(0x06)
	#define AUTH_STA				(0x07)
	#define GET_SECURITY_MODE		(0x08)

	#define SET_SNIFFER 			(0x01)
	#define SET_BRIDGE				(0x02)
	#define GET_DEVICE_MODE 		(0x03)
	#define SET_DATAPATH			(0x04)
	#define GET_HARDWARE_SPEC		(0x05)
	#define GET_DEVICE_STATS		(0x06)
	#define GET_PEER_INFO			(0x09)
	#define SET_DEVICEIF			(0x0B)
	#define SET_HARDWARE_SPEC		(0x0C)
	
	
	/* RESPONSE */
	#define SET_SECURITY_MODE_RSP	(0x81)
	#define SET_DEFAULT_NET_ID_RSP	(0x82)
	#define RESTART_STA_RSP 		(0x83)
	#define SET_NETWORK_RSP 		(0x84)
	#define NET_EXIT_RSP			(0x85)
	#define APPOINT_CCO_RSP 		(0x86)
	#define AUTH_STA_RSP			(0x87)
	#define GET_SECURITY_MODE_RSP	(0x88)
	
	#define SET_DATAPATH_CNF		(0x80 | SET_DATAPATH)
	#define SET_SNIFFER_CNF			(0x80 | SET_SNIFFER)
	#define SET_BRIDGE_CNF			(0x80 | SET_BRIDGE)
	#define GET_DEVICE_MODE_CNF		(0x80 | GET_DEVICE_MODE)
	#define GET_HARDWARE_SPEC_CNF	(0x80 | GET_HARDWARE_SPEC)
	#define GET_DEVICE_STATS_CNF	(0x80 | GET_DEVICE_STATS)
	#define GET_PEER_INFO_CNF		(0x80 | GET_PEER_INFO)
	#define SET_DEVICEIF_CNF		(0x80 | SET_DEVICEIF)
	#define SET_HARDWARE_SPEC_CNF	(0x80 | SET_HARDWARE_SPEC)

	
	/* PARAMETERS */
	#define SECURITY_MODE			(0x01)
	#define PASSWORD				(0x02)
	#define SECURITY_LEVEL			(0x03)
	#define NETWORK_OPTION			(0x04)
	#define MAC_ADDRESS 			(0x05)
	#define DEVICE_PASSWORD 		(0x06)
	#define NETWORK_MGMT_KEY		(0x07)
	#define NETWORK_ID_KEY			(0x08)
	#define RESPONSE				(0x09)
	#define ONOFF					(0x0A)
	#define DEVSTATS				(0x0B)
	#define PEERINFO				(0x0C)
	#define HWSPEC					(0x0D)
	#define DEVMODE					(0x0E)
	#define NETWORK_PASSWORD		(0x0F)

	/* TYPEDEFS */
//	typedef unsigned char			u8;
//	typedef unsigned short int		u16;
//	typedef unsigned int			u32;

#endif //_HPGP_LINUX_DRIVER_API_H_

