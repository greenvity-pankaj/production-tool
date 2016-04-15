#ifndef _MAC_INTF_H_
#define _MAC_INTF_H_

#define MAX_MAC_IF_FRAME_LENGTH		128

#define SYS_MAC_ID					0x00
#define IEEE_802_15_4_MAC_ID 		0x01
#define HPGP_MAC_ID		 			0x02

#define CTRL_FRM_ID					0x00
#define DATA_FRM_ID					0x01
#define MGMT_FRM_ID					0x02

#define DIR_UPLINK					0x00
#define DIR_DOWNLINK				0x01

#define HYBRII_HEADER_LENGTH		4
#define MAC_IF_CRC_LENGTH			2

#endif //_MAC_INTF_H_
