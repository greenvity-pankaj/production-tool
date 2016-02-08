/** ========================================================
 *
 * @file gv_socket_interface.h
 * 
 *  @brief - client program to connect to the production tool server program
 *
 *  Copyright (C) 2010-2011, Greenvity Communications, Inc.
 *  All Rights Reserved
 *  
 * =========================================================*/


#ifndef _GV_SOCKET_INTF_
#define _GV_SOCKET_INTF_

#define clientType_DUT  0x00
#define clientType_Ref  0x01

#define MAX_FW_VER_LEN	16

/*--- Enum Declaration ---*/

// Status messages
typedef enum {

	NULL_STATE			= 0,
	RET_MSG_ERROR		= 1,
	RET_MSG_SUCCESS 	= 2,

} STATUS_MESSAGES;


// Test Id
enum
{
	TEST_ID_PLC_TX,
	TEST_ID_PLC_RX,
	TEST_ID_SPI_TX,
	TEST_ID_SPI_RX,
};


//	Header Event/Command IDs
enum {
	headerEvent,
	headerRequest,
	headerResponse,
};

//	Commands
enum {
	TOOL_CMD_PREPARE_DUT 			= 0x00,
	TOOL_CMD_PREPARE_DUT_CNF 		= 0x80,
	TOOL_CMD_PREPARE_REFERENCE 		= 0x01,
	TOOL_CMD_PREPARE_REFERENCE_CNF 	= 0x81,
	TOOL_CMD_START_TEST 			= 0x02,
	TOOL_CMD_START_TEST_CNF 		= 0x82,
	TOOL_CMD_STOP_TEST 				= 0x03,
	TOOL_CMD_STOP_TEST_CNF 			= 0x83,
	TOOL_CMD_DEVICE_RESET 			= 0x04,
	TOOL_CMD_DEVICE_RESET_CNF		= 0x84,
	TOOL_CMD_GET_RESULT 			= 0x05,
	TOOL_CMD_GET_RESULT_CNF 		= 0x85,
	TOOL_CMD_DEVICE_SEARCH 			= 0x06,
	TOOL_CMD_DEVICE_FLASH_PARAM     = 0x07,
    TOOL_CMD_DEVICE_FLASH_PARAM_CNF = 0x87
};

enum {
	EVENT_DEVICE_UP,
	EVENT_TEST_DONE
};

/*--- Structure Definitions ---*/
typedef struct _header{

	u8 protocolID;
	u16 length;
	u8 type;
	u8 id;

}PACKED header;

typedef struct _sDevUpEvnt
{
    u8	devType;
    u8	fwVer[MAX_FW_VER_LEN];
    u32	bMapTests;
	
}PACKED sDevUpEvnt, *psDevUpEvnt;


typedef struct _response{

	u8 result;
	
}PACKED response;

/*--- Externarations ---*/
char gv_interface_eth[IFNAMSIZ];
char gv_ip_addr_eth[16];


/*--- Function declarations ---*/
STATUS_MESSAGES eth_socket_init(void);
void *eth_socket_comm(void* sock);
void eth_socketSend(void* buffer, u16 len);
void close_eth_socket(void);

int set_MACAddr(void);
char *getIP(void);
int set_ip(char *iface_name, char *ip_addr);

#endif /*_GV_SOCKET_INTF_*/
